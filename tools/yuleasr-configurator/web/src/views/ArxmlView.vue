<template>
  <div class="arxml-view">
    <el-card style="margin-bottom: 20px">
      <template #header>
        <div class="card-header">
          <span>ARXML Configuration Workflow</span>
          <el-tag type="info">Import → Edit → Export</el-tag>
        </div>
      </template>
      <el-steps :active="currentStep" finish-status="success" align-center>
        <el-step title="1. Import" description="Upload ARXML file" />
        <el-step title="2. Preview" description="Review imported modules" />
        <el-step title="3. Apply" description="Load into editor" />
        <el-step title="4. Edit" description="Modify via GUI" />
        <el-step title="5. Export" description="Download ARXML" />
      </el-steps>
    </el-card>

    <el-row :gutter="20">
      <el-col :span="12">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>Import ARXML</span>
            </div>
          </template>

          <el-upload
            ref="uploadRef"
            :auto-upload="false"
            :limit="1"
            accept=".arxml,.xml"
            :on-change="handleFileChange"
            :on-remove="handleFileRemove"
          >
            <el-button type="primary">
              <el-icon><Upload /></el-icon>
              Select ARXML File
            </el-button>
            <template #tip>
              <div class="el-upload__tip">
                Supports AUTOSAR ARXML 4.0 format (.arxml / .xml)
              </div>
            </template>
          </el-upload>

          <el-button
            type="success"
            @click="importArxml"
            :disabled="!selectedFile"
            :loading="importing"
            style="margin-top: 20px"
          >
            <el-icon><Download /></el-icon>
            Parse ARXML
          </el-button>

          <el-alert
            v-if="importResult"
            :title="importResult.success ? 'Parse Successful' : 'Parse Failed'"
            :type="importResult.success ? 'success' : 'error'"
            :description="importResultDescription"
            show-icon
            style="margin-top: 20px"
          />
        </el-card>
      </el-col>

      <el-col :span="12">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>Export ARXML</span>
            </div>
          </template>

          <el-alert
            title="Export current configuration to AUTOSAR ARXML format"
            type="info"
            show-icon
            :closable="false"
          />

          <el-space style="margin-top: 20px">
            <el-button
              type="primary"
              @click="exportArxml"
              :loading="exporting"
              :disabled="!store.config"
            >
              <el-icon><View /></el-icon>
              Preview ARXML
            </el-button>
            <el-button
              type="success"
              @click="downloadArxml"
              :disabled="!exportedArxml"
            >
              <el-icon><Download /></el-icon>
              Download .arxml
            </el-button>
          </el-space>

          <el-input
            v-if="exportedArxml"
            v-model="exportedArxml"
            type="textarea"
            :rows="10"
            readonly
            style="margin-top: 20px; font-family: monospace; font-size: 12px"
          />
        </el-card>
      </el-col>
    </el-row>

    <el-card v-if="importResult?.config" style="margin-top: 20px">
      <template #header>
        <div class="card-header">
          <span>Imported Modules ({{ importResult.modules.length }})</span>
          <el-space>
            <el-button size="small" @click="applyImported('replace')" type="warning">
              Replace Current Config
            </el-button>
            <el-button size="small" @click="applyImported('merge')" type="primary">
              Merge into Current Config
            </el-button>
          </el-space>
        </div>
      </template>

      <el-descriptions :column="3" border style="margin-bottom: 20px">
        <el-descriptions-item label="Modules Imported">
          <el-tag type="success">{{ importResult.modules.length }}</el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="Warnings">
          <el-tag :type="importResult.warnings.length ? 'warning' : 'success'">
            {{ importResult.warnings.length }}
          </el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="Status">
          <el-tag type="info">Ready to apply</el-tag>
        </el-descriptions-item>
      </el-descriptions>

      <el-alert
        v-if="importResult.warnings.length"
        title="Parse Warnings"
        type="warning"
        :closable="false"
        style="margin-bottom: 20px"
      >
        <ul style="margin: 0; padding-left: 20px">
          <li v-for="(w, i) in importResult.warnings" :key="i">{{ w }}</li>
        </ul>
      </el-alert>

      <el-table :data="importedModuleList" stripe max-height="400">
        <el-table-column prop="name" label="Module" width="200" />
        <el-table-column label="Enabled" width="100">
          <template #default="{ row }">
            <el-tag :type="row.enabled ? 'success' : 'info'" size="small">
              {{ row.enabled ? 'Yes' : 'No' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="Parameters" width="120">
          <template #default="{ row }">
            {{ row.paramCount }}
          </template>
        </el-table-column>
        <el-table-column label="Status">
          <template #default="{ row }">
            <el-tag
              :type="existingModules[row.name] ? 'warning' : 'success'"
              size="small"
            >
              {{ existingModules[row.name] ? 'Exists (will merge)' : 'New' }}
            </el-tag>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <el-card v-if="appliedMessage" style="margin-top: 20px">
      <el-alert
        :title="appliedMessage"
        type="success"
        show-icon
        :closable="true"
      >
        <template #default>
          <div style="margin-top: 10px">
            <el-space>
              <el-button type="primary" size="small" @click="$router.push('/modules')">
                Browse Modules
              </el-button>
              <el-button type="primary" size="small" @click="$router.push('/config')">
                Edit Config
              </el-button>
              <el-button type="success" size="small" @click="exportArxml">
                Export ARXML
              </el-button>
            </el-space>
          </div>
        </template>
      </el-alert>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import axios from 'axios'
import { ElMessage, ElMessageBox } from 'element-plus'
import type { UploadFile } from 'element-plus'
import { useConfigStore } from '@/stores/config'

const store = useConfigStore()

const uploadRef = ref()
const selectedFile = ref<File | null>(null)
const importing = ref(false)
const exporting = ref(false)
const importResult = ref<any>(null)
const exportedArxml = ref('')
const appliedMessage = ref('')

const currentStep = computed(() => {
  if (appliedMessage.value) return 4
  if (importResult.value?.config) return 2
  if (selectedFile.value) return 1
  return 0
})

const importedModuleList = computed(() => {
  if (!importResult.value?.config?.modules) return []
  return Object.entries(importResult.value.config.modules).map(([name, mod]: [string, any]) => ({
    name,
    enabled: mod.enabled ?? true,
    paramCount: Object.keys(mod).filter(k => k !== 'name' && k !== 'enabled').length,
  }))
})

const existingModules = computed(() => store.modules)

const importResultDescription = computed(() => {
  if (!importResult.value) return ''
  if (importResult.value.success) {
    return `Parsed ${importResult.value.modules.length} modules from ARXML`
  }
  return 'Failed to parse ARXML file'
})

function handleFileChange(file: UploadFile) {
  selectedFile.value = file.raw || null
  importResult.value = null
  appliedMessage.value = ''
}

function handleFileRemove() {
  selectedFile.value = null
  importResult.value = null
}

async function importArxml() {
  if (!selectedFile.value) return

  importing.value = true
  try {
    const content = await selectedFile.value.text()
    const response = await axios.post('/api/arxml/import', { content })
    importResult.value = response.data
    ElMessage.success(`Parsed ${response.data.modules.length} modules from ARXML`)
  } catch (e: any) {
    ElMessage.error(e.response?.data?.detail || e.message)
    importResult.value = { success: false, modules: [], warnings: [], config: null }
  } finally {
    importing.value = false
  }
}

async function applyImported(mode: 'replace' | 'merge') {
  if (!importResult.value?.config) return

  const importedCount = importResult.value.modules.length

  if (mode === 'replace' && store.config) {
    try {
      await ElMessageBox.confirm(
        `This will replace the current configuration (${store.moduleCount} modules) with the imported ${importedCount} modules. Continue?`,
        'Replace Configuration',
        { confirmButtonText: 'Replace', cancelButtonText: 'Cancel', type: 'warning' }
      )
    } catch {
      return
    }
  }

  if (mode === 'replace') {
    store.replaceConfig(importResult.value.config)
    appliedMessage.value = `Replaced configuration with ${importedCount} modules from ARXML. You can now edit modules in the GUI and export back to ARXML.`
  } else {
    store.mergeModules(importResult.value.config.modules)
    appliedMessage.value = `Merged ${importedCount} modules into current configuration (${store.moduleCount} total). You can now edit modules in the GUI and export back to ARXML.`
  }

  await store.validateConfig()
  ElMessage.success(appliedMessage.value.split('.')[0])
}

async function exportArxml() {
  if (!store.config) {
    ElMessage.warning('No configuration loaded')
    return
  }

  exporting.value = true
  try {
    const response = await axios.post('/api/arxml/export', { config: store.config })
    exportedArxml.value = response.data.arxml
    ElMessage.success(`Exported ${response.data.module_count} modules to ARXML`)
  } catch (e: any) {
    ElMessage.error(e.response?.data?.detail || e.message)
  } finally {
    exporting.value = false
  }
}

function downloadArxml() {
  if (!exportedArxml.value) return
  const blob = new Blob([exportedArxml.value], { type: 'application/xml' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = 'bsw_config.arxml'
  a.click()
  URL.revokeObjectURL(url)
}
</script>

<style scoped>
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>
