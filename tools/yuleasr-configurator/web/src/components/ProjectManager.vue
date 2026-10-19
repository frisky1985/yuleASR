<template>
  <div class="project-manager">
    <!-- 项目信息头 -->
    <div class="pm-header" @contextmenu.prevent="showProjectContext($event, 'header')">
      <div class="pm-project">
        <el-icon class="pm-project-icon"><Folder /></el-icon>
        <div class="pm-project-info">
          <div class="pm-project-name">{{ projectName }}</div>
          <div class="pm-project-meta">{{ ecuCount }} ECU{{ ecuCount !== 1 ? 's' : '' }}</div>
        </div>
      </div>
    </div>

    <!-- 导航区域 -->
    <div class="pm-nav">
      <div
        v-for="item in navItems"
        :key="item.key"
        class="pm-nav-item"
        :class="{ active: activeSection === item.key }"
        @click="activeSection = item.key"
      >
        <el-icon><component :is="item.icon" /></el-icon>
        <span>{{ item.label }}</span>
      </div>
    </div>

    <!-- 内容区域 -->
    <div class="pm-content">
      <!-- 项目列表 -->
      <div v-if="activeSection === 'projects'" class="pm-section">
        <div class="pm-section-title">Projects</div>
        <div class="pm-project-list">
          <div
            v-for="proj in projectList"
            :key="proj.id"
            class="pm-project-item"
            :class="{ active: proj.id === currentProjectId }"
            @click="switchProject(proj.id)"
            @contextmenu.prevent="showProjectContext($event, 'item', proj)"
          >
            <el-icon><Folder /></el-icon>
            <span>{{ proj.name }}</span>
          </div>
        </div>
        <div class="pm-action">
          <el-button text size="small" @click="handleNewProject">
            <el-icon><Plus /></el-icon>
            New Project
          </el-button>
        </div>
      </div>

      <!-- ECU 管理 -->
      <div v-if="activeSection === 'ecus'" class="pm-section">
        <div class="pm-section-title">ECU Configuration</div>
        <div class="pm-ecu-list">
          <div
            v-for="ecu in projectStore.ecus"
            :key="ecu.id"
            class="pm-ecu-item"
            :class="{ active: ecu.id === projectStore.currentEcuId }"
            @click="projectStore.switchEcu(ecu.id)"
            @contextmenu.prevent="showEcuContext($event, ecu)"
          >
            <el-icon><Cpu /></el-icon>
            <div class="pm-ecu-info">
              <span class="pm-ecu-name">{{ ecu.name }}</span>
              <span class="pm-ecu-meta">{{ ecu.chip?.model ?? 'No chip' }} · {{ Object.keys(ecu.modules).length }} modules</span>
            </div>
            <div class="pm-ecu-actions">
              <el-icon class="pm-ecu-action-btn" @click.stop="openEditEcu(ecu)"><Edit /></el-icon>
              <el-icon class="pm-ecu-action-btn pm-ecu-action-danger" @click.stop="confirmRemoveEcu(ecu)"><Delete /></el-icon>
            </div>
          </div>
        </div>
        <div class="pm-action">
          <el-button text size="small" @click="openAddEcu">
            <el-icon><Plus /></el-icon>
            Add ECU
          </el-button>
        </div>
      </div>

      <!-- 最近任务 -->
      <div v-if="activeSection === 'recent'" class="pm-section">
        <div class="pm-section-title">Recent Modules</div>
        <div class="pm-recent-list">
          <div
            v-for="item in recentModules"
            :key="item.name"
            class="pm-recent-item"
            @click="openModule(item.name)"
          >
            <el-icon><Document /></el-icon>
            <div class="pm-recent-info">
              <span class="pm-recent-name">{{ item.name }}</span>
              <span class="pm-recent-time">{{ item.time }}</span>
            </div>
          </div>
          <div v-if="recentModules.length === 0" class="pm-empty">
            No recent modules
          </div>
        </div>
      </div>

      <!-- 工作区 -->
      <div v-if="activeSection === 'workspace'" class="pm-section">
        <div class="pm-section-title">Workspace</div>
        <div class="pm-workspace-actions">
          <div class="pm-ws-action" @click="handleOpenConfig">
            <el-icon><Setting /></el-icon>
            <span>Configuration Editor</span>
          </div>
          <div class="pm-ws-action" @click="handleOpenGraph">
            <el-icon><Share /></el-icon>
            <span>Dependency Graph</span>
          </div>
          <div class="pm-ws-action" @click="handleOpenDiff">
            <el-icon><DocumentCopy /></el-icon>
            <span>Config Diff</span>
          </div>
          <div class="pm-ws-action" @click="handleOpenEcuCompare">
            <el-icon><Switch /></el-icon>
            <span>ECU Comparison</span>
          </div>
          <div class="pm-ws-action" @click="handleImportArxml">
            <el-icon><Upload /></el-icon>
            <span>Import ARXML</span>
          </div>
          <div class="pm-ws-action" @click="handleExportArxml">
            <el-icon><Download /></el-icon>
            <span>Export ARXML</span>
          </div>
        </div>
      </div>
    </div>

    <!-- ECU Editor Dialog -->
    <EcuEditorDialog
      v-model="ecuDialogVisible"
      :edit-data="ecuEditData"
      @confirm="handleEcuConfirm"
    />

    <!-- Project Context Menu -->
    <teleport to="body">
      <div
        v-if="contextMenuVisible"
        class="pm-context-menu"
        :style="{ left: contextMenuX + 'px', top: contextMenuY + 'px' }"
        @click.stop
      >
        <div class="pm-context-item" @click="handleRenameProject">
          <el-icon><Edit /></el-icon>
          <span>Rename</span>
        </div>
        <div class="pm-context-item" @click="handleDuplicateProject">
          <el-icon><CopyDocument /></el-icon>
          <span>Duplicate</span>
        </div>
        <div class="pm-context-divider" />
        <div class="pm-context-item" @click="handleExportProject">
          <el-icon><Download /></el-icon>
          <span>Export Config</span>
        </div>
        <div class="pm-context-item" @click="handleProjectSettings">
          <el-icon><Setting /></el-icon>
          <span>Settings</span>
        </div>
      </div>
    </teleport>

    <!-- Rename Project Dialog -->
    <el-dialog v-model="renameDialogVisible" title="Rename Project" width="400px" @close="renameForm.name = projectName">
      <el-form @submit.prevent="confirmRename">
        <el-form-item label="Project Name">
          <el-input v-model="renameForm.name" placeholder="Enter new project name" ref="renameInputRef" autofocus />
        </el-form-item>
        <el-form-item label="Description">
          <el-input v-model="renameForm.description" type="textarea" :rows="2" placeholder="Optional description" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="renameDialogVisible = false">Cancel</el-button>
        <el-button type="primary" @click="confirmRename" :disabled="!renameForm.name.trim()">Save</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, markRaw, onMounted, onUnmounted } from 'vue'
import { useRouter } from 'vue-router'
import {
  Folder,
  Plus,
  Cpu,
  Document,
  Setting,
  Share,
  DocumentCopy,
  Upload,
  Download,
  List,
  Monitor,
  Edit,
  Delete,
  CopyDocument,
} from '@element-plus/icons-vue'
import { useProjectStore } from '@/stores/project'
import { useConfigStore } from '@/stores/config'
import { useEditorTabsStore } from '@/stores/editorTabs'
import { ElMessage, ElMessageBox } from 'element-plus'
import EcuEditorDialog from './EcuEditorDialog.vue'
import type { EcuConfig } from '@/types'

const router = useRouter()
const projectStore = useProjectStore()
const configStore = useConfigStore()
const tabsStore = useEditorTabsStore()

const activeSection = ref('projects')
const currentProjectId = ref('default')
const ecuDialogVisible = ref(false)
const ecuEditData = ref<{ id: string; name: string; description?: string; chip?: { vendor: string; series: string; model: string } } | undefined>()

// Context menu state
const contextMenuVisible = ref(false)
const contextMenuX = ref(0)
const contextMenuY = ref(0)
const contextMenuTarget = ref<'header' | 'item' | null>(null)
const contextMenuProj = ref<any>(null)

// Rename dialog state
const renameDialogVisible = ref(false)
const renameForm = ref({ name: '', description: '' })
const renameInputRef = ref()

function closeContextMenu() {
  contextMenuVisible.value = false
}

onMounted(() => {
  document.addEventListener('click', closeContextMenu)
})

onUnmounted(() => {
  document.removeEventListener('click', closeContextMenu)
})

const projectName = computed(() => configStore.config?.project?.name ?? 'yuleASR')
const ecuCount = computed(() => projectStore.ecus.length)

const navItems = [
  { key: 'projects', label: 'Projects', icon: markRaw(Folder) },
  { key: 'ecus', label: 'ECUs', icon: markRaw(Cpu) },
  { key: 'recent', label: 'Recent', icon: markRaw(List) },
  { key: 'workspace', label: 'Workspace', icon: markRaw(Monitor) },
]

const projectList = computed(() => [
  { id: 'default', name: projectName.value },
])

const recentModules = computed(() => {
  return tabsStore.tabs.slice(-10).reverse().map((tab) => ({
    name: tab.label,
    time: formatTime(tab.id),
  }))
})

function formatTime(_id: string): string {
  return 'Recently opened'
}

function switchProject(_id: string) {
  ElMessage.info('Project switched')
}

function handleNewProject() {
  router.push('/new-project')
}

function openAddEcu() {
  ecuEditData.value = undefined
  ecuDialogVisible.value = true
}

function openEditEcu(ecu: EcuConfig) {
  ecuEditData.value = {
    id: ecu.id,
    name: ecu.name,
    description: ecu.description,
    chip: ecu.chip,
  }
  ecuDialogVisible.value = true
}

async function handleEcuConfirm(data: { name: string; description: string; chip?: { vendor: string; series: string; model: string } }) {
  if (ecuEditData.value) {
    // Edit existing ECU
    const ecu = projectStore.ecus.find((e) => e.id === ecuEditData.value!.id)
    if (ecu) {
      ecu.name = data.name
      ecu.description = data.description
      ecu.chip = data.chip
    }
    ElMessage.success(`ECU "${data.name}" updated`)
  } else {
    // Add new ECU
    await projectStore.addEcu(data.name, data.chip)
    ElMessage.success(`ECU "${data.name}" added`)
  }
  ecuEditData.value = undefined
}

async function confirmRemoveEcu(ecu: EcuConfig) {
  try {
    await ElMessageBox.confirm(
      `Remove ECU "${ecu.name}"? This cannot be undone.`,
      'Confirm',
      { type: 'warning' },
    )
    await projectStore.removeEcu(ecu.id)
    ElMessage.success(`ECU "${ecu.name}" removed`)
  } catch {
    // Cancelled
  }
}

function showEcuContext(_event: MouseEvent, ecu: EcuConfig) {
  openEditEcu(ecu)
}

function showProjectContext(event: MouseEvent, target: 'header' | 'item', proj?: any) {
  contextMenuTarget.value = target
  contextMenuProj.value = proj ?? null
  contextMenuX.value = event.clientX
  contextMenuY.value = event.clientY
  contextMenuVisible.value = true
}

function handleRenameProject() {
  closeContextMenu()
  renameForm.value = { name: projectName.value, description: projectStore.projectDescription }
  renameDialogVisible.value = true
  setTimeout(() => renameInputRef.value?.focus(), 100)
}

async function confirmRename() {
  const newName = renameForm.value.name.trim()
  if (!newName) return
  projectStore.projectName = newName
  projectStore.projectDescription = renameForm.value.description
  if (configStore.config?.project) {
    configStore.config.project.name = newName
    configStore.config.project.description = renameForm.value.description
  }
  await projectStore.saveProject()
  ElMessage.success(`Project renamed to "${newName}"`)
  renameDialogVisible.value = false
}

function handleDuplicateProject() {
  closeContextMenu()
  ElMessage.info('Project duplication coming soon')
}

function handleExportProject() {
  closeContextMenu()
  router.push('/config')
  setTimeout(() => {
    ElMessage.info('Click "Export ARXML" in the Configuration Editor toolbar')
  }, 300)
}

function handleProjectSettings() {
  closeContextMenu()
  ElMessage.info('Project settings coming soon')
}

function openModule(name: string) {
  tabsStore.openTab(name, name, '')
  router.push('/config')
}

function handleOpenConfig() {
  router.push('/config')
}

function handleOpenGraph() {
  router.push('/graph')
}

function handleOpenDiff() {
  router.push('/diff')
}

function handleOpenEcuCompare() {
  router.push('/ecu-compare')
}

function handleImportArxml() {
  router.push('/arxml')
}

function handleExportArxml() {
  ElMessage.info('Export ARXML from Configuration Editor')
  router.push('/config')
}
</script>

<style scoped>
.project-manager {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #fafbfc;
  overflow: hidden;
}

.pm-header {
  padding: 12px 16px;
  border-bottom: 1px solid #e4e7ed;
  background: #fff;
  cursor: context-menu;
}

.pm-project {
  display: flex;
  align-items: center;
  gap: 10px;
}

.pm-project-icon {
  font-size: 20px;
  color: #409eff;
}

.pm-project-info {
  flex: 1;
  min-width: 0;
}

.pm-project-name {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.pm-project-meta {
  font-size: 11px;
  color: #909399;
  margin-top: 2px;
}

.pm-nav {
  display: flex;
  flex-direction: column;
  padding: 8px 0;
  border-bottom: 1px solid #e4e7ed;
}

.pm-nav-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px 16px;
  font-size: 13px;
  color: #606266;
  cursor: pointer;
  transition: all 0.15s;
}

.pm-nav-item:hover {
  background: #f0f2f5;
  color: #409eff;
}

.pm-nav-item.active {
  background: #ecf5ff;
  color: #409eff;
  font-weight: 500;
}

.pm-nav-item .el-icon {
  font-size: 16px;
}

.pm-content {
  flex: 1;
  overflow-y: auto;
  padding: 12px 0;
}

.pm-section {
  padding: 0 16px;
}

.pm-section-title {
  font-size: 11px;
  font-weight: 600;
  color: #909399;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin-bottom: 8px;
}

.pm-project-list,
.pm-ecu-list,
.pm-recent-list {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.pm-project-item,
.pm-ecu-item,
.pm-recent-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 10px;
  border-radius: 4px;
  font-size: 13px;
  color: #606266;
  cursor: context-menu;
  transition: background 0.15s;
}

.pm-project-item:hover,
.pm-ecu-item:hover,
.pm-recent-item:hover {
  background: #f0f2f5;
}

.pm-project-item.active,
.pm-ecu-item.active {
  background: #ecf5ff;
  color: #409eff;
}

.pm-ecu-info,
.pm-recent-info {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
}

.pm-ecu-actions {
  display: flex;
  gap: 4px;
  opacity: 0;
  transition: opacity 0.15s;
}

.pm-ecu-item:hover .pm-ecu-actions {
  opacity: 1;
}

.pm-ecu-action-btn {
  font-size: 14px;
  color: #909399;
  cursor: pointer;
  padding: 2px;
}

.pm-ecu-action-btn:hover {
  color: #409eff;
}

.pm-ecu-action-danger:hover {
  color: #f56c6c !important;
}

.pm-ecu-name,
.pm-recent-name {
  font-size: 13px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.pm-ecu-meta,
.pm-recent-time {
  font-size: 11px;
  color: #909399;
  margin-top: 1px;
}

.pm-empty {
  padding: 20px 0;
  text-align: center;
  color: #c0c4cc;
  font-size: 13px;
}

.pm-action {
  margin-top: 8px;
}

.pm-workspace-actions {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.pm-ws-action {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px 10px;
  border-radius: 4px;
  font-size: 13px;
  color: #606266;
  cursor: pointer;
  transition: background 0.15s;
}

.pm-ws-action:hover {
  background: #f0f2f5;
  color: #409eff;
}

.pm-ws-action .el-icon {
  font-size: 16px;
}

.pm-context-menu {
  position: fixed;
  z-index: 9999;
  min-width: 160px;
  background: #fff;
  border: 1px solid #e4e7ed;
  border-radius: 6px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
  padding: 4px 0;
}

.pm-context-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  font-size: 13px;
  color: #606266;
  cursor: pointer;
  transition: background 0.15s;
}

.pm-context-item:hover {
  background: #f0f2f5;
  color: #409eff;
}

.pm-context-item .el-icon {
  font-size: 14px;
  width: 16px;
  text-align: center;
}

.pm-context-divider {
  height: 1px;
  background: #e4e7ed;
  margin: 4px 0;
}
</style>
