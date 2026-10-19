<template>
  <div class="config-editor">
    <div class="toolbar">
      <div class="toolbar-left">
        <el-breadcrumb separator="/">
          <el-breadcrumb-item :to="{ path: '/' }">Home</el-breadcrumb-item>
          <el-breadcrumb-item>Configuration</el-breadcrumb-item>
          <el-breadcrumb-item>{{ currentModuleName || 'Select Module' }}</el-breadcrumb-item>
        </el-breadcrumb>
      </div>
      <div class="toolbar-right">
        <el-button-group>
          <el-button type="primary" size="small" @click="validate" :loading="validating">
            <el-icon><Check /></el-icon>
            Validate
          </el-button>
          <el-button type="success" size="small" @click="save" :loading="saving">
            <el-icon><Download /></el-icon>
            Save
          </el-button>
          <el-button type="warning" size="small" @click="$router.push('/arxml')">
            <el-icon><Upload /></el-icon>
            Import ARXML
          </el-button>
          <el-button type="info" size="small" @click="exportArxml">
            <el-icon><UploadFilled /></el-icon>
            Export ARXML
          </el-button>
          <el-button
            v-if="currentModuleName"
            type="danger"
            size="small"
            @click="chatbotVisible = true"
          >
            <el-icon><ChatDotRound /></el-icon>
            AI Assistant
          </el-button>
        </el-button-group>
      </div>
    </div>

    <EditorTabs />

    <div class="config-panel">
      <div v-if="!currentModuleName" class="empty-state">
        <el-empty description="Select a module from the project tree to configure" />
      </div>

      <div v-else class="config-content">
        <div class="module-header">
          <div class="module-title">
            <h2>{{ currentModuleName }}</h2>
            <el-tag :type="moduleEnabled ? 'success' : 'info'" size="small">
              {{ moduleEnabled ? 'Enabled' : 'Disabled' }}
            </el-tag>
          </div>
          <div class="module-path">
            <el-text type="info" size="small">
              /{{ currentLayer }}/{{ currentModuleName }}
            </el-text>
          </div>
        </div>

        <div class="params-section">
          <div class="section-header">
            <h3>Configuration Parameters</h3>
          </div>
          <ParamTable
            :groups="paramGroups"
            :issues="validationIssues"
            @update:param="handleParamUpdate"
          />
        </div>

        <div class="status-bar">
          <div class="status-left">
            <el-text size="small">
              <el-icon><InfoFilled /></el-icon>
              Module: {{ currentModuleName }} | Parameters: {{ flattenedParamCount }}
            </el-text>
          </div>
          <div class="status-right">
            <el-text size="small" :type="validationStatus.type">
              <el-icon><component :is="validationStatus.icon" /></el-icon>
              {{ validationStatus.text }}
            </el-text>
          </div>
        </div>
      </div>
    </div>

    <el-drawer
      v-model="chatbotVisible"
      title=""
      direction="rtl"
      size="400px"
      :show-close="false"
      :with-header="false"
    >
      <ConfigChatbot
        v-if="currentModuleName"
        :module-name="currentModuleName"
        :current-config="store.modules[currentModuleName]"
        @close="chatbotVisible = false"
        @applied="handleChatbotApplied"
      />
    </el-drawer>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import {
  Check,
  Download,
  Upload,
  UploadFilled,
  InfoFilled,
  Minus,
  ChatDotRound,
} from '@element-plus/icons-vue'
import { useConfigStore } from '@/stores/config'
import { useProjectStore } from '@/stores/project'
import { useEditorTabsStore } from '@/stores/editorTabs'
import EditorTabs from '@/components/EditorTabs.vue'
import ParamTable from '@/components/ParamTable.vue'
import ConfigChatbot from '@/components/ConfigChatbot.vue'
import * as api from '@/api/client'
import { getModuleSchema } from '@/api/client'
import type { JsonSchema } from '@/types'
import { groupParams, flattenParams, setParamValue } from '@/utils/paramGrouper'
import type { ParamGroup, ParamMeta } from '@/utils/paramGrouper'
import { validateAll } from '@/utils/localValidator'
import type { ValidationIssue } from '@/utils/localValidator'

const store = useConfigStore()
const projectStore = useProjectStore()
const tabsStore = useEditorTabsStore()

const moduleSchema = ref<JsonSchema | null>(null)
const paramGroups = ref<ParamGroup[]>([])
const validationIssues = ref<ValidationIssue[]>([])
const saving = ref(false)
const validating = ref(false)
const chatbotVisible = ref(false)

const currentModuleName = computed(() => projectStore.currentModuleName)
const moduleEnabled = computed(() => store.modules[currentModuleName.value]?.enabled ?? false)

const currentLayer = computed(() => {
  const name = currentModuleName.value
  if (!name) return ''
  const mcal = ['Mcu', 'Port', 'Dio', 'Can', 'Spi', 'Adc', 'Pwm', 'Gpt', 'Wdg', 'Icu', 'Lin', 'Eep', 'Fls', 'Eth', 'EthTrcv', 'EthSwt', 'I2c', 'Ocu']
  const ecual = ['CanIf', 'CanTp', 'IoHwAb', 'EthIf', 'MemIf', 'Fee', 'Ea', 'FrIf', 'LinIf', 'PduR', 'EthTSyn']
  if (mcal.includes(name)) return 'MCAL'
  if (ecual.includes(name)) return 'ECUAL'
  if (name === 'Os') return 'OS'
  if (name === 'Rte') return 'RTE'
  if (['BswM', 'EcuM', 'Det'].includes(name)) return 'Integration'
  return 'Services'
})

const validationStatus = computed(() => {
  if (!store.validation) return { type: 'info' as const, text: 'Not validated', icon: InfoFilled }
  if (store.validation.valid) return { type: 'success' as const, text: 'Valid', icon: Check }
  return { type: 'danger' as const, text: `${store.validation.errors.length} errors`, icon: Minus }
})

const flattenedParamCount = computed(() => flattenParams(paramGroups.value).length)

watch(currentModuleName, (name) => {
  if (!name) {
    paramGroups.value = []
    validationIssues.value = []
    return
  }
  tabsStore.openTab(name, name, '')
  loadModuleParams(name)
})

watch(() => tabsStore.activeTabId, (tabId) => {
  if (!tabId) return
  const tab = tabsStore.tabs.find((t) => t.id === tabId)
  if (tab && tab.moduleName !== currentModuleName.value) {
    projectStore.selectModule(tab.moduleName)
  }
})

async function handleChatbotApplied(_changes: Record<string, any>) {
  if (!currentModuleName.value) return
  await store.loadConfig()
  projectStore.syncFromConfig()
  await loadModuleParams(currentModuleName.value)
  chatbotVisible.value = false
  ElMessage.success('Configuration updated')
}

async function loadModuleParams(moduleName: string) {
  const mod = store.modules[moduleName]
  if (!mod) {
    paramGroups.value = []
    validationIssues.value = []
    return
  }

  try {
    moduleSchema.value = await getModuleSchema(moduleName)
  } catch {
    moduleSchema.value = { type: 'object', properties: {} }
  }

  if (moduleSchema.value) {
    paramGroups.value = groupParams(moduleSchema.value, mod)
  }

  const allParams = flattenParams(paramGroups.value)
  validationIssues.value = validateAll(allParams, store.config ?? {})
}

function handleParamUpdate(param: ParamMeta) {
  if (!currentModuleName.value) return
  const mod = store.modules[currentModuleName.value]
  if (!mod) return
  const updatedConfig = setParamValue(mod, param.path, param.value)
  store.updateModule(currentModuleName.value, updatedConfig)
  const allParams = flattenParams(paramGroups.value)
  validationIssues.value = validateAll(allParams, store.config ?? {})
}

async function save() {
  saving.value = true
  try {
    await store.saveConfig()
    ElMessage.success('Configuration saved successfully')
  } catch (e: any) {
    ElMessage.error(e.message || 'Failed to save configuration')
  } finally {
    saving.value = false
  }
}

async function validate() {
  validating.value = true
  try {
    await store.validateConfig()
    if (store.validation?.valid) {
      ElMessage.success('Configuration is valid')
    } else {
      ElMessage.warning(`Validation failed: ${store.validation?.errors.length || 0} errors`)
    }
  } catch (e: any) {
    ElMessage.error(e.message || 'Validation failed')
  } finally {
    validating.value = false
  }
}

async function exportArxml() {
  if (!store.config) {
    ElMessage.warning('No configuration loaded')
    return
  }
  try {
    await api.saveConfig(store.config)
    ElMessage.success('Configuration exported')
  } catch (e: any) {
    ElMessage.error(e.response?.data?.detail || e.message)
  }
}

onMounted(async () => {
  if (!store.config) {
    try {
      await store.loadConfig()
    } catch {
      // Config load failed
    }
  }
})
</script>

<style scoped>
.config-editor {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #f5f7fa;
}

.toolbar {
  height: 50px;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 20px;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.08);
  flex-shrink: 0;
}

.toolbar-left {
  flex: 1;
}

.toolbar-right {
  display: flex;
  gap: 10px;
}

.config-panel {
  flex: 1;
  overflow-y: auto;
  padding: 20px;
  background: #f5f7fa;
}

.empty-state {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 100%;
}

.config-content {
  background: #fff;
  border-radius: 4px;
  padding: 20px;
  border: 1px solid #e4e7ed;
}

.module-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-bottom: 15px;
  border-bottom: 2px solid #409eff;
  margin-bottom: 20px;
}

.module-title {
  display: flex;
  align-items: center;
  gap: 12px;
}

.module-title h2 {
  margin: 0;
  font-size: 20px;
  color: #303133;
}

.module-path {
  color: #909399;
}

.params-section {
  margin-top: 20px;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
}

.section-header h3 {
  margin: 0;
  font-size: 16px;
  color: #303133;
}

.status-bar {
  margin-top: 20px;
  padding: 10px 15px;
  background: #f0f2f5;
  border-radius: 4px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  border: 1px solid #e4e7ed;
}

.status-left,
.status-right {
  display: flex;
  align-items: center;
  gap: 5px;
}
</style>
