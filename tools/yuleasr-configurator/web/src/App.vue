<template>
  <div class="desktop-app">
    <div class="titlebar" v-if="isElectron">
      <div class="titlebar-drag"></div>
      <div class="titlebar-title">yuleASR Configurator</div>
    </div>

    <div class="toolbar">
      <div class="toolbar-left">
        <div class="toolbar-section">
          <el-tooltip content="Toggle Sidebar" placement="bottom">
            <el-button text size="small" :icon="sidebarVisible ? Fold : Expand" @click="sidebarVisible = !sidebarVisible" />
          </el-tooltip>
        </div>

        <div class="toolbar-divider"></div>

        <div class="toolbar-section">
          <el-tooltip content="New Project (Ctrl+N)" placement="bottom">
            <el-button text size="small" :icon="Plus" @click="navigateTo('/new-project')" />
          </el-tooltip>
          <el-tooltip content="Open Configuration (Ctrl+O)" placement="bottom">
            <el-button text size="small" :icon="FolderOpened" @click="handleOpen" />
          </el-tooltip>
          <el-tooltip content="Save (Ctrl+S)" placement="bottom">
            <el-button text size="small" :icon="Download" @click="handleSave" />
          </el-tooltip>
        </div>

        <div class="toolbar-divider"></div>

        <div class="toolbar-section">
          <el-button type="primary" size="small" :icon="VideoPlay" @click="handleGenerate">
            Generate Code
          </el-button>
        </div>

        <div class="toolbar-divider"></div>

        <div class="toolbar-section">
          <el-tooltip content="Dependency Graph" placement="bottom">
            <el-button text size="small" :icon="Share" @click="navigateTo('/graph')" />
          </el-tooltip>
          <el-tooltip content="Configuration Diff" placement="bottom">
            <el-button text size="small" :icon="DocumentCopy" @click="navigateTo('/diff')" />
          </el-tooltip>
          <el-tooltip content="ARXML Import/Export" placement="bottom">
            <el-button text size="small" :icon="Document" @click="navigateTo('/arxml')" />
          </el-tooltip>
        </div>
      </div>

      <div class="toolbar-right">
        <div class="backend-status" :class="{ connected: backendConnected }">
          <span class="status-dot"></span>
          <span class="status-text">{{ backendConnected ? 'Connected' : 'Offline' }}</span>
        </div>
      </div>
    </div>

    <div class="app-body">
      <transition name="sidebar">
        <div v-if="sidebarVisible" class="sidebar">
          <ProjectManager />
        </div>
      </transition>

      <div class="main-content">
        <router-view />
      </div>
    </div>

    <div class="statusbar">
      <div class="statusbar-left">
        <span class="status-item">
          <el-icon><Monitor /></el-icon>
          yuleASR v1.0.0
        </span>
        <span class="status-item" v-if="currentFile">
          <el-icon><Document /></el-icon>
          {{ currentFile }}
        </span>
      </div>
      <div class="statusbar-right">
        <span class="status-item" v-if="isElectron">
          <el-icon><Connection /></el-icon>
          Desktop Mode
        </span>
        <span class="status-item">
          {{ moduleCount }} modules
        </span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import {
  Plus,
  FolderOpened,
  Download,
  VideoPlay,
  Share,
  Monitor,
  Document,
  Connection,
  DocumentCopy,
  Fold,
  Expand,
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import { useConfigStore } from '@/stores/config'
import { useProjectStore } from '@/stores/project'
import ProjectManager from '@/components/ProjectManager.vue'
import * as api from '@/api/client'

const router = useRouter()
const route = useRoute()
const configStore = useConfigStore()
const projectStore = useProjectStore()

const isElectron = ref(false)
const backendConnected = ref(false)
const currentFile = ref('')
const canGoForward = ref(false)
const historyIndex = ref(0)
const historyStack = ref<string[]>(['/'])
const sidebarVisible = ref(true)

const moduleCount = computed(() => configStore.moduleCount)

watch(() => configStore.config, (cfg) => {
  if (cfg) projectStore.syncFromConfig()
}, { immediate: true })

function goBack() {
  if (historyIndex.value > 0) {
    historyIndex.value--
    canGoForward.value = true
    router.back()
  } else if (window.history.length > 1) {
    router.back()
  } else {
    router.push('/')
  }
}

function goForward() {
  if (historyIndex.value < historyStack.value.length - 1) {
    historyIndex.value++
    canGoForward.value = historyIndex.value < historyStack.value.length - 1
    router.forward()
  }
}

function goHome() {
  navigateTo('/')
}

function navigateTo(path: string) {
  if (historyIndex.value < historyStack.value.length - 1) {
    historyStack.value = historyStack.value.slice(0, historyIndex.value + 1)
  }
  historyStack.value.push(path)
  historyIndex.value = historyStack.value.length - 1
  canGoForward.value = false
  router.push(path)
}

function handleOpen() {
  if (isElectron.value && window.electronAPI) {
    window.electronAPI.showOpenDialog({
      properties: ['openFile'],
      filters: [
        { name: 'Configuration Files', extensions: ['yaml', 'yml', 'json'] },
        { name: 'All Files', extensions: ['*'] },
      ],
    }).then(async (result: any) => {
      if (!result.canceled && result.filePaths.length > 0) {
        const filePath = result.filePaths[0]
        const res = await window.electronAPI!.readFile(filePath)
        if (res.success && res.content) {
          try {
            const config = JSON.parse(res.content)
            configStore.replaceConfig(config)
            currentFile.value = filePath.split('/').pop() || filePath
            ElMessage.success(`Loaded: ${currentFile.value}`)
          } catch {
            navigateTo('/config')
            ElMessage.info('File loaded, navigating to config editor')
          }
        }
      }
    })
  } else {
    navigateTo('/config')
  }
}

function handleSave() {
  if (isElectron.value && window.electronAPI) {
    window.electronAPI.showSaveDialog({
      filters: [
        { name: 'YAML Files', extensions: ['yaml', 'yml'] },
        { name: 'JSON Files', extensions: ['json'] },
      ],
    }).then(async (result: any) => {
      if (!result.canceled && result.filePath) {
        const content = JSON.stringify(configStore.config, null, 2)
        const res = await window.electronAPI!.writeFile(result.filePath, content)
        if (res.success) {
          currentFile.value = result.filePath.split('/').pop() || result.filePath
          ElMessage.success(`Saved: ${currentFile.value}`)
        } else {
          ElMessage.error(`Save failed: ${res.error}`)
        }
      }
    })
  } else {
    ElMessage.info('Use the Config editor to save')
    navigateTo('/config')
  }
}

async function handleGenerate() {
  if (!configStore.config) {
    ElMessage.warning('No configuration loaded. Open or create a project first.')
    return
  }
  try {
    const result = await api.generateCode(configStore.config)
    if (result.files && result.files.length > 0) {
      ElMessage.success(`Generated ${result.files.length} files to ${result.output_dir}`)
    } else {
      ElMessage.success('Code generation complete')
    }
    router.push('/editor')
  } catch (e: any) {
    ElMessage.error(e.response?.data?.detail || e.message || 'Code generation failed')
  }
}

async function checkBackend() {
  try {
    const response = await fetch('/health')
    backendConnected.value = response.ok
  } catch {
    backendConnected.value = false
  }
}

let healthCheckInterval: ReturnType<typeof setInterval>

router.afterEach((to) => {
  const currentPath = to.path
  const lastTracked = historyStack.value[historyIndex.value]
  if (lastTracked !== currentPath) {
    const idx = historyStack.value.indexOf(currentPath)
    if (idx >= 0) {
      historyIndex.value = idx
      canGoForward.value = idx < historyStack.value.length - 1
    } else {
      historyStack.value = historyStack.value.slice(0, historyIndex.value + 1)
      historyStack.value.push(currentPath)
      historyIndex.value = historyStack.value.length - 1
      canGoForward.value = false
    }
  }
})

onMounted(() => {
  isElectron.value = !!window.electronAPI?.isElectron

  if (isElectron.value && window.electronAPI) {
    window.electronAPI.onMenuAction((action: string) => {
      switch (action) {
        case 'new-project':
          navigateTo('/new-project')
          break
        case 'save':
          handleSave()
          break
        case 'generate':
          handleGenerate()
          break
        case 'undo':
          ElMessage.info('Undo')
          break
        case 'redo':
          ElMessage.info('Redo')
          break
        case 'toggle-tree':
          sidebarVisible.value = !sidebarVisible.value
          break
        case 'toggle-properties':
          ElMessage.info('Toggle Properties')
          break
        case 'toggle-console':
          ElMessage.info('Toggle Console')
          break
        case 'export-arxml':
          navigateTo('/arxml')
          break
        case 'open-ai-assistant':
          navigateTo('/config')
          ElMessage.info('Navigate to Configuration Editor and click "AI Assistant" button')
          break
      }
    })

    window.electronAPI.onFileOpened((data: { path: string; content: string }) => {
      try {
        const config = JSON.parse(data.content)
        configStore.replaceConfig(config)
        currentFile.value = data.path.split('/').pop() || data.path
        navigateTo('/config')
        ElMessage.success(`Loaded: ${currentFile.value}`)
      } catch {
        ElMessage.error('Failed to parse configuration file')
      }
    })

    window.electronAPI.onDbcOpened((data: { path: string; content: string }) => {
      navigateTo('/dbc-import')
      ElMessage.success(`DBC file opened: ${data.path.split('/').pop()}`)
    })

    window.electronAPI.onArxmlImported((data: { path: string; content: string }) => {
      navigateTo('/arxml')
      ElMessage.info(`ARXML imported: ${data.path.split('/').pop()}`)
    })
  }

  checkBackend()
  healthCheckInterval = setInterval(checkBackend, 10000)
})

onUnmounted(() => {
  if (healthCheckInterval) {
    clearInterval(healthCheckInterval)
  }
})
</script>

<style>
html, body, #app {
  margin: 0;
  padding: 0;
  height: 100%;
  overflow: hidden;
}

.desktop-app {
  display: flex;
  flex-direction: column;
  height: 100vh;
  background: #f5f7fa;
}

.titlebar {
  display: flex;
  align-items: center;
  height: 38px;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
  position: relative;
  -webkit-app-region: drag;
}

.titlebar-drag {
  flex: 1;
}

.titlebar-title {
  position: absolute;
  left: 50%;
  transform: translateX(-50%);
  font-size: 13px;
  color: #606266;
  font-weight: 500;
  pointer-events: none;
}

.toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 44px;
  padding: 0 16px;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
  flex-shrink: 0;
}

.toolbar-left {
  display: flex;
  align-items: center;
  gap: 0;
}

.toolbar-right {
  display: flex;
  align-items: center;
}

.toolbar-section {
  display: flex;
  align-items: center;
  gap: 2px;
  padding: 0 4px;
}

.toolbar-divider {
  width: 1px;
  height: 20px;
  background: #e4e7ed;
  margin: 0 8px;
}

.backend-status {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 4px 10px;
  border-radius: 12px;
  font-size: 12px;
  color: #909399;
  background: #f4f4f5;
  transition: all 0.3s;
}

.backend-status.connected {
  color: #67c23a;
  background: #f0f9eb;
}

.backend-status .status-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: #c0c4cc;
  transition: background 0.3s;
}

.backend-status.connected .status-dot {
  background: #67c23a;
  box-shadow: 0 0 4px rgba(103, 194, 58, 0.4);
}

.backend-status .status-text {
  font-weight: 500;
}

:deep(.toolbar-section .el-button) {
  color: #606266;
}

:deep(.toolbar-section .el-button:hover) {
  color: #409eff;
  background: #ecf5ff;
}

:deep(.toolbar-section .el-button.is-text) {
  padding: 6px 8px;
}

.app-body {
  flex: 1;
  display: flex;
  overflow: hidden;
}

.sidebar {
  width: 280px;
  min-width: 280px;
  border-right: 1px solid #e4e7ed;
  background: #fafbfc;
  overflow: hidden;
  display: flex;
  flex-direction: column;
}

.sidebar-enter-active,
.sidebar-leave-active {
  transition: all 0.25s ease;
}

.sidebar-enter-from,
.sidebar-leave-to {
  width: 0;
  min-width: 0;
  opacity: 0;
}

.main-content {
  flex: 1;
  overflow: hidden;
  min-width: 0;
}

.statusbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 26px;
  padding: 0 12px;
  background: #f0f2f5;
  color: #606266;
  font-size: 12px;
  flex-shrink: 0;
  border-top: 1px solid #e4e7ed;
}

.statusbar-left,
.statusbar-right {
  display: flex;
  align-items: center;
  gap: 16px;
}

.status-item {
  display: flex;
  align-items: center;
  gap: 4px;
}
</style>
