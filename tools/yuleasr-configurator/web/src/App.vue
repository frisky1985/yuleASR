<template>
  <div class="desktop-app">
    <div class="titlebar" v-if="isElectron">
      <div class="titlebar-drag"></div>
      <div class="titlebar-title">yuleASR Configurator</div>
    </div>

    <div class="toolbar">
      <div class="toolbar-left">
        <div class="toolbar-section">
          <el-tooltip content="Back" placement="bottom">
            <el-button text size="small" :icon="ArrowLeft" :disabled="isHome" @click="goBack" />
          </el-tooltip>
          <el-tooltip content="Forward" placement="bottom">
            <el-button text size="small" :icon="ArrowRight" :disabled="!canGoForward" @click="goForward" />
          </el-tooltip>
          <el-tooltip content="Home" placement="bottom">
            <el-button text size="small" :icon="HomeFilled" :disabled="isHome" @click="goHome" />
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
          <el-tooltip content="Generate Code (F5)" placement="bottom">
            <el-button type="primary" size="small" :icon="VideoPlay" @click="handleGenerate">
              Generate
            </el-button>
          </el-tooltip>
        </div>

        <div class="toolbar-divider"></div>

        <div class="toolbar-section">
          <el-tooltip content="Configuration Editor" placement="bottom">
            <el-button text size="small" :icon="Setting" @click="navigateTo('/config')" />
          </el-tooltip>
          <el-tooltip content="Dependency Graph" placement="bottom">
            <el-button text size="small" :icon="Share" @click="navigateTo('/graph')" />
          </el-tooltip>
          <el-tooltip content="Configuration Diff" placement="bottom">
            <el-button text size="small" :icon="DocumentCopy" @click="navigateTo('/diff')" />
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

    <div class="main-content">
      <router-view />
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
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import {
  Plus,
  FolderOpened,
  Download,
  VideoPlay,
  Setting,
  Share,
  Monitor,
  Document,
  Connection,
  DocumentCopy,
  ArrowLeft,
  ArrowRight,
  HomeFilled,
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import { useConfigStore } from '@/stores/config'

const router = useRouter()
const route = useRoute()
const configStore = useConfigStore()

const isElectron = ref(false)
const backendConnected = ref(false)
const currentFile = ref('')
const moduleCount = ref(0)
const canGoForward = ref(false)
const historyIndex = ref(0)
const historyStack = ref<string[]>(['/'])

const isHome = computed(() => route.path === '/')

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
  // Truncate forward history when navigating to a new page
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

function handleGenerate() {
  ElMessage.info('Code generation triggered')
  router.push('/editor')
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
    // Browser back/forward detected — find the path in our stack
    const idx = historyStack.value.indexOf(currentPath)
    if (idx >= 0) {
      historyIndex.value = idx
      canGoForward.value = idx < historyStack.value.length - 1
    } else {
      // New navigation from browser — add to stack
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
          ElMessage.info('Toggle Module Tree')
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

.main-content {
  flex: 1;
  overflow: hidden;
}

.statusbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 24px;
  padding: 0 12px;
  background: #409eff;
  color: #fff;
  font-size: 12px;
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
