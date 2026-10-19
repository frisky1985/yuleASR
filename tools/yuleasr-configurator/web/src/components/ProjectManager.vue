<template>
  <div class="project-manager">
    <!-- Search -->
    <div class="pm-search">
      <el-input
        v-model="searchText"
        placeholder="Search modules..."
        size="small"
        clearable
      >
        <template #prefix>
          <el-icon><Search /></el-icon>
        </template>
      </el-input>
    </div>

    <!-- Project header -->
    <div class="pm-header" @contextmenu.prevent="showProjectContext($event)">
      <div class="pm-project">
        <el-icon class="pm-project-icon"><Folder /></el-icon>
        <div class="pm-project-info">
          <div class="pm-project-name">{{ projectName }}</div>
          <div class="pm-project-meta">{{ ecuCount }} ECU{{ ecuCount !== 1 ? 's' : '' }} · {{ moduleCount }} modules</div>
        </div>
      </div>
    </div>

    <!-- Tree -->
    <div class="pm-tree">
      <el-tree
        ref="treeRef"
        :data="treeData"
        :props="treeProps"
        node-key="id"
        highlight-current
        :expand-on-click-node="false"
        :default-expanded-keys="defaultExpandedKeys"
        @node-contextmenu="handleContextMenu"
      >
        <template #default="{ node, data }">
          <div class="tree-node" @click.stop="onNodeClick(data)" @contextmenu.prevent="handleContextMenu($event, node, data)">
            <div class="node-left">
              <span v-if="data.type === 'project'" class="node-icon project-icon">
                <el-icon><Folder /></el-icon>
              </span>
              <span v-else-if="data.type === 'ecu'" class="node-icon ecu-icon">
                <el-icon><Cpu /></el-icon>
              </span>
              <span v-else-if="data.type === 'layer'" class="node-icon" :class="layerIconClass(data.label)">
                <el-icon><component :is="layerIcon(data.label)" /></el-icon>
              </span>
              <span v-else class="node-icon module-icon">
                <el-icon><Setting /></el-icon>
              </span>
              <span class="node-label">{{ data.label }}</span>
            </div>
            <div class="node-right">
              <span v-if="data.type === 'layer'" class="node-badge" :class="{ 'badge-enabled': data.enabledCount > 0 }">
                {{ data.enabledCount }}/{{ data.moduleCount }}
              </span>
              <span v-else-if="data.type === 'module'" class="status-badge" :class="data.enabled ? 'status-enabled' : 'status-disabled'">
                {{ data.enabled ? 'ON' : 'OFF' }}
              </span>
            </div>
          </div>
        </template>
      </el-tree>
    </div>

    <!-- Quick actions -->
    <div class="pm-actions">
      <el-button text size="small" @click="openAddEcu">
        <el-icon><Plus /></el-icon> Add ECU
      </el-button>
      <el-button text size="small" @click="handleNewProject">
        <el-icon><Plus /></el-icon> New Project
      </el-button>
    </div>

    <!-- Context Menu -->
    <teleport to="body">
      <div
        v-if="contextMenu.visible"
        class="pm-context-menu"
        :style="{ left: contextMenu.x + 'px', top: contextMenu.y + 'px' }"
        @click.stop
      >
        <template v-if="contextMenu.data?.type === 'project'">
          <div class="pm-context-item" @click="handleRenameProject"><el-icon><Edit /></el-icon><span>Rename</span></div>
          <div class="pm-context-item" @click="handleDuplicateProject"><el-icon><CopyDocument /></el-icon><span>Duplicate</span></div>
          <div class="pm-context-divider" />
          <div class="pm-context-item" @click="handleExportProject"><el-icon><Download /></el-icon><span>Export Config</span></div>
          <div class="pm-context-item" @click="handleProjectSettings"><el-icon><Setting /></el-icon><span>Settings</span></div>
        </template>
        <template v-else-if="contextMenu.data?.type === 'ecu'">
          <div class="pm-context-item" @click="openEditEcu(contextMenu.data.ecu)"><el-icon><Edit /></el-icon><span>Edit ECU</span></div>
          <div class="pm-context-divider" />
          <div class="pm-context-item pm-context-danger" @click="confirmRemoveEcu(contextMenu.data.ecu)"><el-icon><Delete /></el-icon><span>Remove ECU</span></div>
        </template>
        <template v-else-if="contextMenu.data?.type === 'module'">
          <div class="pm-context-item" @click="toggleModule(contextMenu.data)">
            <el-icon><component :is="contextMenu.data.enabled ? Close : Check" /></el-icon>
            <span>{{ contextMenu.data.enabled ? 'Disable' : 'Enable' }}</span>
          </div>
        </template>
      </div>
    </teleport>

    <!-- ECU Editor Dialog -->
    <EcuEditorDialog
      v-model="ecuDialogVisible"
      :edit-data="ecuEditData"
      @confirm="handleEcuConfirm"
    />

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
import { ref, computed, onMounted, onUnmounted, markRaw, nextTick } from 'vue'
import { useRouter } from 'vue-router'
import {
  Folder, Plus, Cpu, Setting, Edit, Delete, CopyDocument,
  Download, Search, Check, Close,
  Monitor, Connection,
} from '@element-plus/icons-vue'
import { useProjectStore } from '@/stores/project'
import { useConfigStore } from '@/stores/config'
import { ElMessage, ElMessageBox } from 'element-plus'
import EcuEditorDialog from './EcuEditorDialog.vue'
import type { EcuConfig, ModuleConfig } from '@/types'

const router = useRouter()
const projectStore = useProjectStore()
const configStore = useConfigStore()

const searchText = ref('')
const treeRef = ref()
const ecuDialogVisible = ref(false)
const ecuEditData = ref<{ id: string; name: string; description?: string; chip?: { vendor: string; series: string; model: string } } | undefined>()
const contextMenu = ref({ visible: false, x: 0, y: 0, data: null as any })
const renameDialogVisible = ref(false)
const renameForm = ref({ name: '', description: '' })
const renameInputRef = ref()

const projectName = computed(() => configStore.config?.project?.name ?? projectStore.projectName ?? 'yuleASR')
const ecuCount = computed(() => projectStore.ecus.length)
const moduleCount = computed(() => Object.keys(configStore.modules).length)

const treeProps = { children: 'children', label: 'label' }

const layerIcons: Record<string, any> = {
  MCAL: markRaw(Monitor),
  ECUAL: markRaw(Connection),
  Services: markRaw(Setting),
  OS: markRaw(Cpu),
  RTE: markRaw(Connection),
  'BSW Coordination': markRaw(Folder),
}

function layerIcon(layer: string) { return layerIcons[layer] ?? markRaw(Setting) }
function layerIconClass(layer: string): string {
  const map: Record<string, string> = { MCAL: 'icon-mcal', ECUAL: 'icon-ecual', Services: 'icon-services', OS: 'icon-os', RTE: 'icon-rte', 'BSW Coordination': 'icon-integration' }
  return map[layer] ?? ''
}

function guessLayer(name: string): string {
  const mcal = ['Mcu', 'Port', 'Dio', 'Can', 'Spi', 'Adc', 'Pwm', 'Gpt', 'Wdg', 'Icu', 'Lin', 'Eep', 'Fls', 'Eth', 'EthTrcv', 'EthSwt', 'I2c', 'Ocu']
  const ecual = ['CanIf', 'CanTp', 'IoHwAb', 'EthIf', 'MemIf', 'Fee', 'Ea', 'FrIf', 'LinIf', 'PduR', 'EthTSyn']
  if (mcal.includes(name)) return 'MCAL'
  if (ecual.includes(name)) return 'ECUAL'
  if (name === 'Os') return 'OS'
  if (name === 'Rte') return 'RTE'
  if (['BswM', 'EcuM', 'Det'].includes(name)) return 'BSW Coordination'
  return 'Services'
}

interface TreeNode {
  id: string
  label: string
  type: 'project' | 'ecu' | 'layer' | 'module'
  enabled?: boolean
  moduleCount?: number
  enabledCount?: number
  ecu?: EcuConfig
  moduleName?: string
  children?: TreeNode[]
}

const defaultExpandedKeys = computed(() => {
  const keys: string[] = []
  for (const ecu of projectStore.ecus) {
    keys.push(`ecu-${ecu.id}`)
    for (const layer of ['MCAL', 'ECUAL', 'Services', 'OS', 'RTE', 'BSW Coordination']) {
      keys.push(`ecu-${ecu.id}-layer-${layer}`)
    }
  }
  return keys
})

const treeData = computed<TreeNode[]>(() => {
  const ecus = projectStore.ecus
  if (ecus.length === 0) return []

  return ecus.map((ecu) => {
    const modules = ecu.modules
    const grouped: Record<string, { label: string; enabled: boolean; moduleName: string }[]> = {
      MCAL: [], ECUAL: [], Services: [], OS: [], RTE: [], 'BSW Coordination': [],
    }

    for (const [name, mod] of Object.entries(modules)) {
      const layer = guessLayer(name)
      if (grouped[layer]) {
        grouped[layer].push({ label: name, enabled: mod.enabled, moduleName: name })
      }
    }

    const layerNodes: TreeNode[] = Object.entries(grouped)
      .filter(([, mods]) => mods.length > 0)
      .map(([layer, mods]) => {
        const enabledCount = mods.filter((m) => m.enabled).length
        return {
          id: `ecu-${ecu.id}-layer-${layer}`,
          label: layer,
          type: 'layer' as const,
          moduleCount: mods.length,
          enabledCount,
          children: mods
            .filter((m) => !searchText.value || m.label.toLowerCase().includes(searchText.value.toLowerCase()))
            .sort((a, b) => a.label.localeCompare(b.label))
            .map((m) => ({
              id: `ecu-${ecu.id}-mod-${m.moduleName}`,
              label: m.label,
              type: 'module' as const,
              enabled: m.enabled,
              moduleName: m.moduleName,
              ecu,
            })),
        }
      })

    // Filter out empty layers when searching
    const filteredLayers = searchText.value
      ? layerNodes.filter((l) => l.children && l.children.length > 0)
      : layerNodes

    return {
      id: `ecu-${ecu.id}`,
      label: ecu.name,
      type: 'ecu' as const,
      ecu,
      children: filteredLayers,
    }
  })
})

function closeContextMenu() { contextMenu.value.visible = false }

function onNodeClick(data: TreeNode) {
  if (data.type === 'module' && data.moduleName) {
    if (data.ecu) projectStore.switchEcu(data.ecu.id)
    projectStore.selectModule(data.moduleName)
    // Highlight the tree node
    nextTick(() => { treeRef.value?.setCurrentKey(data.id) })
    router.push('/config')
  }
}

function handleContextMenu(event: MouseEvent, _node: any, data: TreeNode) {
  event.preventDefault()
  event.stopPropagation()
  contextMenu.value = { visible: true, x: event.clientX, y: event.clientY, data }
}

function showProjectContext(event: MouseEvent) {
  event.preventDefault()
  contextMenu.value = {
    visible: true,
    x: event.clientX,
    y: event.clientY,
    data: { type: 'project' },
  }
}

function toggleModule(data: TreeNode) {
  closeContextMenu()
  if (data.moduleName) {
    projectStore.toggleModule(data.moduleName)
  }
}

function openAddEcu() {
  ecuEditData.value = undefined
  ecuDialogVisible.value = true
}

function openEditEcu(ecu: EcuConfig) {
  closeContextMenu()
  ecuEditData.value = { id: ecu.id, name: ecu.name, description: ecu.description, chip: ecu.chip }
  ecuDialogVisible.value = true
}

async function handleEcuConfirm(data: { name: string; description: string; chip?: { vendor: string; series: string; model: string }; selectedModules?: string[] }) {
  if (ecuEditData.value) {
    const ecu = projectStore.ecus.find((e) => e.id === ecuEditData.value!.id)
    if (ecu) { ecu.name = data.name; ecu.description = data.description; ecu.chip = data.chip }
    ElMessage.success(`ECU "${data.name}" updated`)
  } else {
    const ecu = await projectStore.addEcu(data.name, data.chip)
    if (data.selectedModules?.length && ecu) {
      for (const modName of data.selectedModules) {
        if (!ecu.modules[modName]) {
          ecu.modules[modName] = { name: modName, enabled: true }
        }
      }
    }
    ElMessage.success(`ECU "${data.name}" added with ${data.selectedModules?.length ?? 0} modules`)
  }
  ecuEditData.value = undefined
}

async function confirmRemoveEcu(ecu: EcuConfig) {
  closeContextMenu()
  try {
    await ElMessageBox.confirm(`Remove ECU "${ecu.name}"? This cannot be undone.`, 'Confirm', { type: 'warning' })
    await projectStore.removeEcu(ecu.id)
    ElMessage.success(`ECU "${ecu.name}" removed`)
  } catch { /* cancelled */ }
}

function handleNewProject() { router.push('/new-project') }

function handleRenameProject() {
  closeContextMenu()
  renameForm.value = { name: projectName.value, description: projectStore.projectDescription }
  renameDialogVisible.value = true
  nextTick(() => renameInputRef.value?.focus())
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

function handleDuplicateProject() { closeContextMenu(); ElMessage.info('Project duplication coming soon') }
function handleExportProject() { closeContextMenu(); router.push('/config'); setTimeout(() => ElMessage.info('Click "Export ARXML" in the toolbar'), 300) }
function handleProjectSettings() { closeContextMenu(); ElMessage.info('Project settings coming soon') }

onMounted(() => { document.addEventListener('click', closeContextMenu) })
onUnmounted(() => { document.removeEventListener('click', closeContextMenu) })
</script>

<style scoped>
.project-manager {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #fafbfc;
  overflow: hidden;
}

.pm-search {
  padding: 8px 12px;
  border-bottom: 1px solid #e4e7ed;
  background: #fff;
}

.pm-header {
  padding: 10px 12px;
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

.pm-project-info { flex: 1; min-width: 0; }

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

.pm-tree {
  flex: 1;
  overflow-y: auto;
  padding: 4px 0;
}

.pm-actions {
  display: flex;
  gap: 4px;
  padding: 8px 12px;
  border-top: 1px solid #e4e7ed;
  background: #fff;
}

.pm-actions .el-button {
  flex: 1;
  justify-content: center;
}

.tree-node {
  display: flex;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  padding-right: 8px;
}

.node-left {
  display: flex;
  align-items: center;
  gap: 6px;
  flex: 1;
  min-width: 0;
}

.node-icon {
  font-size: 14px;
  flex-shrink: 0;
  display: flex;
  align-items: center;
}

.project-icon { color: #409eff; }
.ecu-icon { color: #67c23a; }
.icon-mcal { color: #409eff; }
.icon-ecual { color: #67c23a; }
.icon-services { color: #e6a23c; }
.icon-os { color: #f56c6c; }
.icon-rte { color: #909399; }
.icon-integration { color: #b37feb; }
.module-icon { color: #606266; }

.node-label {
  font-size: 13px;
  color: #303133;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.node-right {
  display: flex;
  align-items: center;
  gap: 6px;
  flex-shrink: 0;
}

.node-badge {
  font-size: 10px;
  padding: 1px 5px;
  border-radius: 8px;
  background: #f0f2f5;
  color: #909399;
}

.node-badge.badge-enabled {
  background: #f0f9eb;
  color: #67c23a;
}

.status-badge {
  font-size: 10px;
  font-weight: 600;
  padding: 1px 5px;
  border-radius: 8px;
  letter-spacing: 0.3px;
}

.status-enabled {
  background: #f0f9eb;
  color: #67c23a;
  border: 1px solid #e1f3d8;
}

.status-disabled {
  background: #f4f4f5;
  color: #909399;
  border: 1px solid #e9e9eb;
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

.pm-context-item .el-icon { font-size: 14px; width: 16px; text-align: center; }

.pm-context-divider {
  height: 1px;
  background: #e4e7ed;
  margin: 4px 0;
}

.pm-context-danger { color: #f56c6c; }
.pm-context-danger:hover { background: #fef0f0; color: #f56c6c; }

:deep(.el-tree) { background: transparent; }
:deep(.el-tree-node__content) { height: 32px; background: transparent; }
:deep(.el-tree-node__content:hover) { background: #f0f2f5; }
:deep(.el-tree-node.is-current > .el-tree-node__content) { background: #ecf5ff; }
:deep(.el-tree-node.is-current > .el-tree-node__content .node-label) { color: #409eff; font-weight: 500; }
</style>
