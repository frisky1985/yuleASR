<template>
  <div class="config-editor">
    <!-- Toolbar -->
    <div class="ce-toolbar">
      <div class="ce-breadcrumb">
        <el-breadcrumb separator="/">
          <el-breadcrumb-item>Configuration</el-breadcrumb-item>
          <el-breadcrumb-item v-if="selectedModuleName">{{ selectedModuleName }}</el-breadcrumb-item>
        </el-breadcrumb>
      </div>
      <div class="ce-actions">
        <el-button size="small" @click="validate" :loading="validating">
          <el-icon><Check /></el-icon> Validate
        </el-button>
        <el-button size="small" @click="save" :loading="saving">
          <el-icon><Download /></el-icon> Save
        </el-button>
        <el-divider direction="vertical" />
        <el-button size="small" @click="$router.push('/arxml')">
          <el-icon><Upload /></el-icon> ARXML
        </el-button>
        <el-button v-if="selectedModuleName" size="small" @click="chatbotVisible = true">
          <el-icon><ChatDotRound /></el-icon> AI
        </el-button>
      </div>
    </div>

    <!-- Content -->
    <div class="ce-content">
      <div v-if="!selectedModuleName" class="ce-empty">
        <div class="ce-empty-content">
          <el-icon :size="48" color="#c0c4cc"><Setting /></el-icon>
          <h3>Select a Module</h3>
          <p>Choose a module from the project tree on the left to view and edit its configuration parameters.</p>
          <div class="ce-empty-hints">
            <div class="hint-item">
              <el-icon color="#409eff"><Monitor /></el-icon>
              <span>{{ moduleCount }} modules available</span>
            </div>
            <div class="hint-item">
              <el-icon color="#67c23a"><Check /></el-icon>
              <span>{{ enabledCount }} enabled</span>
            </div>
          </div>
        </div>
      </div>

      <div v-else class="ce-config">
        <div class="ce-module-header">
          <div class="ce-module-title">
            <h2>{{ selectedModuleName }}</h2>
            <el-tag :type="moduleEnabled ? 'success' : 'info'" size="small" effect="plain">
              {{ moduleEnabled ? 'Enabled' : 'Disabled' }}
            </el-tag>
            <el-text type="info" size="small" style="margin-left: auto">
              {{ moduleLayer }}
            </el-text>
          </div>
        </div>

        <div class="ce-params">
          <div class="ce-section-header">
            <h3>Configuration Parameters</h3>
          </div>
          <ParamTable
            :groups="paramGroups"
            :issues="validationIssues"
            :module-name="selectedModuleName"
            @update:param="handleParamUpdate"
          />
        </div>

        <div class="ce-status-bar">
          <div class="ce-status-left">
            <el-text size="small">
              <el-icon><InfoFilled /></el-icon>
              Module: {{ selectedModuleName }} | Parameters: {{ flattenedParamCount }}
            </el-text>
          </div>
          <div class="ce-status-right">
            <el-text size="small" :type="validationStatus.type">
              <el-icon><component :is="validationStatus.icon" /></el-icon>
              {{ validationStatus.text }}
            </el-text>
          </div>
        </div>
      </div>
    </div>

    <!-- AI Chatbot Drawer -->
    <el-drawer
      v-model="chatbotVisible"
      direction="rtl"
      size="400px"
      :show-close="false"
      :with-header="false"
    >
      <ConfigChatbot
        v-if="selectedModuleName"
        :module-name="selectedModuleName"
        :current-config="store.modules[selectedModuleName]"
        @close="chatbotVisible = false"
        @applied="handleChatbotApplied"
      />
    </el-drawer>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { Check, Download, Upload, InfoFilled, ChatDotRound, Setting, Monitor } from '@element-plus/icons-vue'
import { useConfigStore } from '@/stores/config'
import { useProjectStore } from '@/stores/project'
import ParamTable from '@/components/ParamTable.vue'
import ConfigChatbot from '@/components/ConfigChatbot.vue'
import { getModuleSchema, validateReferences } from '@/api/client'
import type { JsonSchema } from '@/types'
import type { ReferenceIssue } from '@/api/client'
import { groupParams, flattenParams, setParamValue } from '@/utils/paramGrouper'
import type { ParamGroup, ParamMeta } from '@/utils/paramGrouper'
import { validateAll } from '@/utils/localValidator'
import type { ValidationIssue } from '@/utils/localValidator'

const store = useConfigStore()
const projectStore = useProjectStore()

const moduleSchema = ref<JsonSchema | null>(null)
const paramGroups = ref<ParamGroup[]>([])
const validationIssues = ref<ValidationIssue[]>([])
const referenceIssues = ref<ReferenceIssue[]>([])
const saving = ref(false)
const validating = ref(false)
const chatbotVisible = ref(false)

const selectedModuleName = computed(() => projectStore.currentModuleName)
const moduleEnabled = computed(() => {
  if (!selectedModuleName.value) return false
  return store.modules[selectedModuleName.value]?.enabled ?? false
})

function guessLayerName(name: string): string {
  const mcal = ['Mcu', 'Port', 'Dio', 'Can', 'Spi', 'Adc', 'Pwm', 'Gpt', 'Wdg', 'Icu', 'Lin', 'Eep', 'Fls', 'Eth', 'EthTrcv', 'EthSwt', 'I2c', 'Ocu']
  const ecual = ['CanIf', 'CanTp', 'IoHwAb', 'EthIf', 'MemIf', 'Fee', 'Ea', 'FrIf', 'LinIf', 'PduR', 'EthTSyn']
  if (mcal.includes(name)) return 'MCAL'
  if (ecual.includes(name)) return 'ECUAL'
  if (name === 'Os') return 'OS'
  if (name === 'Rte') return 'RTE'
  if (['BswM', 'EcuM', 'Det'].includes(name)) return 'BSW Coordination'
  return 'Services'
}

const moduleLayer = computed(() => selectedModuleName.value ? guessLayerName(selectedModuleName.value) : '')

const validationStatus = computed(() => {
  if (!store.validation) return { type: 'info' as const, text: 'Not validated', icon: InfoFilled }
  if (store.validation.valid) return { type: 'success' as const, text: 'Valid', icon: Check }
  return { type: 'danger' as const, text: `${store.validation.errors.length} errors`, icon: InfoFilled }
})

const flattenedParamCount = computed(() => flattenParams(paramGroups.value).length)
const moduleCount = computed(() => Object.keys(store.modules).length)
const enabledCount = computed(() => Object.values(store.modules).filter((m: any) => m.enabled).length)

async function loadModuleParams(moduleName: string) {
  const mod = store.modules[moduleName]
  if (!mod) { paramGroups.value = []; validationIssues.value = []; return }

  try { moduleSchema.value = await getModuleSchema(moduleName) }
  catch { moduleSchema.value = { type: 'object', properties: {} } }

  if (moduleSchema.value) paramGroups.value = groupParams(moduleSchema.value, mod, moduleName)
  const allParams = flattenParams(paramGroups.value)
  validationIssues.value = validateAll(allParams, store.config ?? {})
}

watch(selectedModuleName, (name) => {
  if (name) loadModuleParams(name)
  else { paramGroups.value = []; validationIssues.value = [] }
})

function handleParamUpdate(param: ParamMeta) {
  if (!selectedModuleName.value) return
  const mod = store.modules[selectedModuleName.value]
  if (!mod) return
  const updatedConfig = setParamValue(mod, param.path, param.value)
  store.updateModule(selectedModuleName.value, updatedConfig)
  const allParams = flattenParams(paramGroups.value)
  validationIssues.value = validateAll(allParams, store.config ?? {})
}

async function handleChatbotApplied() {
  if (!selectedModuleName.value) return
  await store.loadConfig()
  await loadModuleParams(selectedModuleName.value)
  chatbotVisible.value = false
  ElMessage.success('Configuration updated')
}

async function save() {
  saving.value = true
  try { await store.saveConfig(); ElMessage.success('Configuration saved successfully') }
  catch (e: any) { ElMessage.error(e.message || 'Failed to save configuration') }
  finally { saving.value = false }
}

async function validate() {
  validating.value = true
  try {
    await store.validateConfig()
    const refResult = await validateReferences(store.config as any)
    referenceIssues.value = refResult.issues
    const schemaErrors = store.validation?.errors.length || 0
    const refErrors = refResult.summary.errors
    const refWarnings = refResult.summary.warnings
    if (store.validation?.valid && refResult.valid) ElMessage.success('Configuration is valid')
    else {
      const parts = []
      if (schemaErrors > 0) parts.push(`${schemaErrors} schema errors`)
      if (refErrors > 0) parts.push(`${refErrors} reference errors`)
      if (refWarnings > 0) parts.push(`${refWarnings} warnings`)
      ElMessage.warning(`Validation issues: ${parts.join(', ')}`)
    }
  } catch (e: any) { ElMessage.error(e.message || 'Validation failed') }
  finally { validating.value = false }
}

onMounted(async () => {
  if (!store.config) {
    try { await store.loadConfig() } catch { /* user can import ARXML */ }
  }
  if (selectedModuleName.value) await loadModuleParams(selectedModuleName.value)
})
</script>

<style scoped>
.config-editor {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #f5f7fa;
  overflow: hidden;
}

.ce-toolbar {
  height: 44px;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 16px;
  flex-shrink: 0;
}

.ce-breadcrumb { flex: 1; }
.ce-actions { display: flex; gap: 8px; }

.ce-content {
  flex: 1;
  overflow-y: auto;
  padding: 16px;
}

.ce-empty {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 100%;
}

.ce-empty-content {
  text-align: center;
  max-width: 360px;
}

.ce-empty-content h3 {
  margin: 16px 0 8px;
  font-size: 18px;
  color: #303133;
  font-weight: 500;
}

.ce-empty-content p {
  margin: 0 0 20px;
  font-size: 14px;
  color: #909399;
  line-height: 1.6;
}

.ce-empty-hints {
  display: flex;
  justify-content: center;
  gap: 24px;
}

.hint-item {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 13px;
  color: #606266;
}

.ce-config {
  background: #fff;
  border-radius: 8px;
  padding: 20px;
  border: 1px solid #e4e7ed;
}

.ce-module-header {
  display: flex;
  align-items: center;
  padding-bottom: 12px;
  border-bottom: 2px solid #409eff;
  margin-bottom: 16px;
}

.ce-module-title {
  display: flex;
  align-items: center;
  gap: 10px;
  width: 100%;
}

.ce-module-title h2 {
  margin: 0;
  font-size: 18px;
  color: #303133;
}

.ce-params { margin-top: 16px; }

.ce-section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.ce-section-header h3 {
  margin: 0;
  font-size: 15px;
  color: #303133;
}

.ce-status-bar {
  margin-top: 16px;
  padding: 8px 12px;
  background: #f0f2f5;
  border-radius: 4px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  border: 1px solid #e4e7ed;
}

.ce-status-left, .ce-status-right {
  display: flex;
  align-items: center;
  gap: 4px;
}
</style>
