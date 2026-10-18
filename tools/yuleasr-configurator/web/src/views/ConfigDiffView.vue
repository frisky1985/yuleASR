<template>
  <div class="config-diff">
    <div class="diff-toolbar">
      <el-select v-model="diffMode" size="small" style="width: 200px">
        <el-option label="Current vs Last Saved" value="current-vs-saved" />
        <el-option label="Compare Two Files" value="two-files" />
        <el-option label="Change History" value="history" />
      </el-select>

      <template v-if="diffMode === 'two-files'">
        <el-button size="small" @click="selectLeftFile">
          <el-icon><FolderOpened /></el-icon>
          Left: {{ leftFileName || 'Select File' }}
        </el-button>
        <el-button size="small" @click="selectRightFile">
          <el-icon><FolderOpened /></el-icon>
          Right: {{ rightFileName || 'Select File' }}
        </el-button>
      </template>

      <div class="toolbar-spacer" />

      <el-button-group size="small">
        <el-button :type="viewMode === 'unified' ? 'primary' : ''" @click="viewMode = 'unified'">
          Unified
        </el-button>
        <el-button :type="viewMode === 'split' ? 'primary' : ''" @click="viewMode = 'split'">
          Split
        </el-button>
      </el-button-group>

      <el-button size="small" @click="exportDiff">
        <el-icon><Download /></el-icon>
        Export
      </el-button>
    </div>

    <div class="diff-summary">
      <el-descriptions :column="4" border size="small">
        <el-descriptions-item label="Added">
          <el-tag type="success">{{ stats.added }}</el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="Modified">
          <el-tag type="warning">{{ stats.modified }}</el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="Deleted">
          <el-tag type="danger">{{ stats.deleted }}</el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="Unchanged">
          <el-tag type="info">{{ stats.unchanged }}</el-tag>
        </el-descriptions-item>
      </el-descriptions>
    </div>

    <div class="diff-content">
      <template v-if="viewMode === 'unified'">
        <div v-for="change in changes" :key="change.path" class="diff-row" :class="`diff-${change.type}`">
          <div class="diff-path">
            <el-icon v-if="change.type === 'added'" color="#67c23a"><Plus /></el-icon>
            <el-icon v-else-if="change.type === 'modified'" color="#e6a23c"><Edit /></el-icon>
            <el-icon v-else-if="change.type === 'deleted'" color="#f56c6c"><Delete /></el-icon>
            <span>{{ change.path }}</span>
          </div>
          <div class="diff-values">
            <div v-if="change.type !== 'added'" class="diff-old">
              <el-text type="info" size="small">{{ formatValue(change.oldValue) }}</el-text>
            </div>
            <div v-if="change.type !== 'deleted'" class="diff-new">
              <el-text type="primary" size="small">{{ formatValue(change.newValue) }}</el-text>
            </div>
          </div>
          <div class="diff-actions">
            <el-button
              v-if="change.type === 'modified' || change.type === 'deleted'"
              size="small"
              text
              @click="revertChange(change)"
            >
              Revert
            </el-button>
          </div>
        </div>
      </template>

      <template v-else>
        <div class="split-view">
          <div class="split-left">
            <div class="split-header">Left (Original)</div>
            <div class="split-content">
              <pre>{{ JSON.stringify(leftConfig, null, 2) }}</pre>
            </div>
          </div>
          <div class="split-right">
            <div class="split-header">Right (Modified)</div>
            <div class="split-content">
              <pre>{{ JSON.stringify(rightConfig, null, 2) }}</pre>
            </div>
          </div>
        </div>
      </template>

      <el-empty v-if="changes.length === 0 && diffMode !== 'two-files'" description="No changes detected" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { FolderOpened, Download, Plus, Edit, Delete } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import { useConfigStore } from '@/stores/config'
import { useChangeHistoryStore } from '@/stores/changeHistory'

interface ConfigChange {
  path: string
  type: 'added' | 'modified' | 'deleted'
  oldValue: any
  newValue: any
}

const store = useConfigStore()
const historyStore = useChangeHistoryStore()

const diffMode = ref<'current-vs-saved' | 'two-files' | 'history'>('current-vs-saved')
const viewMode = ref<'unified' | 'split'>('unified')
const leftConfig = ref<Record<string, any> | null>(null)
const rightConfig = ref<Record<string, any> | null>(null)
const leftFileName = ref('')
const rightFileName = ref('')
const savedConfig = ref<Record<string, any> | null>(null)

const changes = computed<ConfigChange[]>(() => {
  if (diffMode.value === 'current-vs-saved') {
    if (!store.config || !savedConfig.value) return []
    return diffConfigs(savedConfig.value, store.config)
  }

  if (diffMode.value === 'two-files') {
    if (!leftConfig.value || !rightConfig.value) return []
    return diffConfigs(leftConfig.value, rightConfig.value)
  }

  if (diffMode.value === 'history') {
    return historyStore.history.map((entry) => ({
      path: entry.path,
      type: 'modified' as const,
      oldValue: entry.oldValue,
      newValue: entry.newValue,
    }))
  }

  return []
})

const stats = computed(() => {
  return {
    added: changes.value.filter((c) => c.type === 'added').length,
    modified: changes.value.filter((c) => c.type === 'modified').length,
    deleted: changes.value.filter((c) => c.type === 'deleted').length,
    unchanged: 0,
  }
})

watch(
  () => store.config,
  (newConfig) => {
    if (newConfig && !savedConfig.value) {
      savedConfig.value = JSON.parse(JSON.stringify(newConfig))
    }
  },
  { immediate: true },
)

function diffConfigs(oldConfig: Record<string, any>, newConfig: Record<string, any>): ConfigChange[] {
  const changes: ConfigChange[] = []
  const allPaths = new Set<string>()

  function collectPaths(obj: any, prefix: string) {
    for (const [key, value] of Object.entries(obj)) {
      const path = prefix ? `${prefix}.${key}` : key
      if (value && typeof value === 'object' && !Array.isArray(value)) {
        collectPaths(value, path)
      } else {
        allPaths.add(path)
      }
    }
  }

  collectPaths(oldConfig, '')
  collectPaths(newConfig, '')

  for (const path of allPaths) {
    const oldValue = getNestedValue(oldConfig, path)
    const newValue = getNestedValue(newConfig, path)

    if (oldValue === undefined && newValue !== undefined) {
      changes.push({ path, type: 'added', oldValue: undefined, newValue })
    } else if (oldValue !== undefined && newValue === undefined) {
      changes.push({ path, type: 'deleted', oldValue, newValue: undefined })
    } else if (JSON.stringify(oldValue) !== JSON.stringify(newValue)) {
      changes.push({ path, type: 'modified', oldValue, newValue })
    }
  }

  return changes.sort((a, b) => a.path.localeCompare(b.path))
}

function getNestedValue(obj: any, path: string): any {
  const keys = path.split('.')
  let current = obj
  for (const key of keys) {
    if (current === null || current === undefined) return undefined
    current = current[key]
  }
  return current
}

function formatValue(value: any): string {
  if (value === undefined) return '(undefined)'
  if (value === null) return '(null)'
  if (typeof value === 'boolean') return value ? 'true' : 'false'
  if (typeof value === 'object') return JSON.stringify(value, null, 2)
  return String(value)
}

function revertChange(change: ConfigChange) {
  if (!store.config) return

  const keys = change.path.split('.')
  let current = store.config as any

  for (let i = 0; i < keys.length - 1; i++) {
    if (!(keys[i] in current)) {
      current[keys[i]] = {}
    }
    current = current[keys[i]]
  }

  const lastKey = keys[keys.length - 1]
  if (change.type === 'deleted') {
    current[lastKey] = change.oldValue
  } else {
    current[lastKey] = change.oldValue
  }

  ElMessage.success(`Reverted: ${change.path}`)
}

async function selectLeftFile() {
  if (window.electronAPI) {
    const result = await window.electronAPI.showOpenDialog({
      filters: [{ name: 'Config Files', extensions: ['yaml', 'yml', 'json'] }],
    })
    if (result && result.length > 0) {
      const res = await window.electronAPI.readFile(result[0])
      if (res.success && res.content) {
        leftConfig.value = JSON.parse(res.content)
        leftFileName.value = result[0].split('/').pop() || 'file'
      }
    }
  }
}

async function selectRightFile() {
  if (window.electronAPI) {
    const result = await window.electronAPI.showOpenDialog({
      filters: [{ name: 'Config Files', extensions: ['yaml', 'yml', 'json'] }],
    })
    if (result && result.length > 0) {
      const res = await window.electronAPI.readFile(result[0])
      if (res.success && res.content) {
        rightConfig.value = JSON.parse(res.content)
        rightFileName.value = result[0].split('/').pop() || 'file'
      }
    }
  }
}

function exportDiff() {
  const exportData = {
    timestamp: new Date().toISOString(),
    mode: diffMode.value,
    stats: stats.value,
    changes: changes.value,
  }

  const blob = new Blob([JSON.stringify(exportData, null, 2)], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `config-diff-${Date.now()}.json`
  a.click()
  URL.revokeObjectURL(url)

  ElMessage.success('Diff exported')
}
</script>

<style scoped>
.config-diff {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #fff;
}

.diff-toolbar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px;
  border-bottom: 1px solid #e4e7ed;
  background: #fafafa;
}

.toolbar-spacer {
  flex: 1;
}

.diff-summary {
  padding: 10px;
  border-bottom: 1px solid #e4e7ed;
}

.diff-content {
  flex: 1;
  overflow-y: auto;
  padding: 10px;
}

.diff-row {
  display: flex;
  align-items: flex-start;
  gap: 12px;
  padding: 10px;
  border-bottom: 1px solid #e4e7ed;
}

.diff-added {
  background: #f0f9eb;
}

.diff-modified {
  background: #fdf6ec;
}

.diff-deleted {
  background: #fef0f0;
}

.diff-path {
  display: flex;
  align-items: center;
  gap: 6px;
  min-width: 250px;
  font-family: monospace;
  font-size: 13px;
}

.diff-values {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.diff-old {
  text-decoration: line-through;
  opacity: 0.6;
}

.diff-new {
  font-weight: 500;
}

.diff-actions {
  display: flex;
  gap: 4px;
}

.split-view {
  display: flex;
  gap: 10px;
  height: 100%;
}

.split-left,
.split-right {
  flex: 1;
  display: flex;
  flex-direction: column;
  border: 1px solid #e4e7ed;
  border-radius: 4px;
}

.split-header {
  padding: 8px 12px;
  background: #f5f7fa;
  border-bottom: 1px solid #e4e7ed;
  font-weight: 500;
}

.split-content {
  flex: 1;
  overflow: auto;
  padding: 10px;
}

.split-content pre {
  margin: 0;
  font-size: 12px;
  line-height: 1.5;
}
</style>
