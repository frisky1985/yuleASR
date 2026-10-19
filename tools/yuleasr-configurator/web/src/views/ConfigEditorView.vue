<template>
  <div class="config-editor">
    <!-- 顶部工具栏 -->
    <div class="toolbar">
      <div class="toolbar-left">
        <el-breadcrumb separator="/">
          <el-breadcrumb-item :to="{ path: '/' }">Home</el-breadcrumb-item>
          <el-breadcrumb-item>Configuration</el-breadcrumb-item>
          <el-breadcrumb-item>{{ currentModule?.label || 'Select Module' }}</el-breadcrumb-item>
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
            v-if="currentModule"
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

    <Splitpanes class="default-theme splitpanes--vertical">
      <!-- 左侧模块树 -->
      <Pane min-size="20" max-size="40" size="25">
        <div class="module-tree-panel">
          <ModuleTree
            :modules="store.modules"
            @node-click="handleNodeClick"
            @enable-module="enableModule"
            @disable-module="disableModule"
            @expand-all="expandAll"
            @collapse-all="collapseAll"
          />
        </div>
      </Pane>

      <!-- 右侧配置区 -->
      <Pane min-size="50">
        <div class="config-panel">
        <div v-if="!currentModule" class="empty-state">
          <el-empty description="Select a module from the tree to configure" />
        </div>

        <div v-else class="config-content">
          <!-- 模块信息头 -->
          <div class="module-header">
            <div class="module-title">
              <el-icon :class="['module-icon', currentModule.iconClass]">
                <component :is="currentModule.icon" />
              </el-icon>
              <h2>{{ currentModule.label }}</h2>
              <el-tag :type="store.modules[currentModule.label]?.enabled ? 'success' : 'info'" size="small">
                {{ store.modules[currentModule.label]?.enabled ? 'Enabled' : 'Disabled' }}
              </el-tag>
            </div>
            <div class="module-path">
              <el-text type="info" size="small">
                /{{ currentModule.category }}/{{ currentModule.label }}
              </el-text>
            </div>
          </div>

          <!-- 参数表格 (ParamTable 组件) -->
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

          <!-- 底部状态栏 -->
          <div class="status-bar">
            <div class="status-left">
              <el-text size="small">
                <el-icon><InfoFilled /></el-icon>
                Module: {{ currentModule.label }} | Parameters: {{ flattenedParamCount }}
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
      </Pane>
    </Splitpanes>

    <!-- AI Chatbot Drawer -->
    <el-drawer
      v-model="chatbotVisible"
      title=""
      direction="rtl"
      size="400px"
      :show-close="false"
      :with-header="false"
    >
      <ConfigChatbot
        v-if="currentModule"
        :module-name="currentModule.label"
        :current-config="store.modules[currentModule.label]"
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
import { Splitpanes, Pane } from 'splitpanes'
import 'splitpanes/dist/splitpanes.css'
import { useConfigStore } from '@/stores/config'
import { useEditorTabsStore } from '@/stores/editorTabs'
import ModuleTree from '@/components/ModuleTree.vue'
import EditorTabs from '@/components/EditorTabs.vue'
import ParamTable from '@/components/ParamTable.vue'
import ConfigChatbot from '@/components/ConfigChatbot.vue'
import * as api from '@/api/client'
import { getModuleSchema, validateReferences } from '@/api/client'
import type { JsonSchema } from '@/types'
import type { ReferenceIssue } from '@/api/client'
import { groupParams, flattenParams, setParamValue } from '@/utils/paramGrouper'
import type { ParamGroup, ParamMeta } from '@/utils/paramGrouper'
import { validateAll } from '@/utils/localValidator'
import type { ValidationIssue } from '@/utils/localValidator'

const store = useConfigStore()
const tabsStore = useEditorTabsStore()

interface ModuleNode {
  id: string
  label: string
  icon: any
  iconClass: string
  category?: string
  moduleCount?: number
  children?: ModuleNode[]
}

const currentModule = ref<ModuleNode | null>(null)
const moduleSchema = ref<JsonSchema | null>(null)
const paramGroups = ref<ParamGroup[]>([])
const validationIssues = ref<ValidationIssue[]>([])
const referenceIssues = ref<ReferenceIssue[]>([])
const saving = ref(false)
const validating = ref(false)
const chatbotVisible = ref(false)

const validationStatus = computed(() => {
  if (!store.validation) return { type: 'info' as const, text: 'Not validated', icon: InfoFilled }
  if (store.validation.valid) return { type: 'success' as const, text: 'Valid', icon: Check }
  return { type: 'danger' as const, text: `${store.validation.errors.length} errors`, icon: Minus }
})

const flattenedParamCount = computed(() => flattenParams(paramGroups.value).length)

function handleNodeClick(data: ModuleNode) {
  if (!data.moduleCount && data.label) {
    tabsStore.openTab(data.label, data.label, data.iconClass)
    currentModule.value = data
    loadModuleParams(data.label)
  }
}

async function handleChatbotApplied(_changes: Record<string, any>) {
  if (!currentModule.value) return
  await store.loadConfig()
  await loadModuleParams(currentModule.value.label)
  chatbotVisible.value = false
  ElMessage.success('Configuration updated')
}

watch(() => tabsStore.activeTabId, (tabId) => {
  if (!tabId) {
    currentModule.value = null
    paramGroups.value = []
    validationIssues.value = []
    return
  }
  const tab = tabsStore.tabs.find((t) => t.id === tabId)
  if (tab) {
    const mod = store.modules[tab.moduleName]
    if (mod) {
      currentModule.value = {
        id: tab.id,
        label: tab.label,
        icon: null,
        iconClass: tab.iconClass,
        category: guessLayerName(tab.moduleName),
      }
      loadModuleParams(tab.moduleName)
    }
  }
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

function enableModule(name: string) {
  store.updateModule(name, { enabled: true })
  ElMessage.success(`${name} enabled`)
}

function disableModule(name: string) {
  store.updateModule(name, { enabled: false })
  ElMessage.info(`${name} disabled`)
}

function expandAll() {
  ElMessage.info('All modules expanded')
}

function collapseAll() {
  ElMessage.info('All modules collapsed')
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
  if (!currentModule.value) return

  const moduleName = currentModule.value.label
  const mod = store.modules[moduleName]
  if (!mod) return

  const updatedConfig = setParamValue(mod, param.path, param.value)
  store.updateModule(moduleName, updatedConfig)

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
    const refResult = await validateReferences(store.config as any)
    referenceIssues.value = refResult.issues

    const schemaErrors = store.validation?.errors.length || 0
    const refErrors = refResult.summary.errors
    const refWarnings = refResult.summary.warnings

    if (store.validation?.valid && refResult.valid) {
      ElMessage.success('Configuration is valid')
    } else {
      const parts = []
      if (schemaErrors > 0) parts.push(`${schemaErrors} schema errors`)
      if (refErrors > 0) parts.push(`${refErrors} reference errors`)
      if (refWarnings > 0) parts.push(`${refWarnings} warnings`)
      ElMessage.warning(`Validation issues: ${parts.join(', ')}`)
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
      // Config load failed, user can import ARXML
    }
  }
})
</script>

<style scoped>
.config-editor {
  height: 100vh;
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
}

.toolbar-left {
  flex: 1;
}

.toolbar-right {
  display: flex;
  gap: 10px;
}

.splitpanes.default-theme {
  flex: 1;
  overflow: hidden;
}

.module-tree-panel {
  height: 100%;
  background: #fff;
  border-right: 1px solid #e4e7ed;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.config-panel {
  height: 100%;
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

.module-icon {
  font-size: 24px;
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
