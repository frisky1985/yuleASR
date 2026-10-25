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

    <!-- Workspace header -->
    <div class="pm-header" @contextmenu.prevent="showProjectContext($event)">
      <div class="pm-project">
        <el-icon class="pm-project-icon"><FolderOpened /></el-icon>
        <div class="pm-project-info">
          <div class="pm-project-name">{{ workspaceName }}</div>
          <div class="pm-project-meta">{{ projectCount }} Project{{ projectCount !== 1 ? 's' : '' }} · {{ ecuCount }} ECU{{ ecuCount !== 1 ? 's' : '' }}</div>
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
              <span v-if="data.type === 'project'" class="node-badge">
                {{ data.ecuCount }} ECU{{ data.ecuCount !== 1 ? 's' : '' }}
              </span>
              <span v-else-if="data.type === 'layer'" class="node-badge" :class="{ 'badge-enabled': data.enabledCount > 0 }">
                {{ data.enabledCount }}/{{ data.moduleCount }}
              </span>
              <span v-else-if="data.type === 'module'" class="status-dot" :class="data.enabled ? 'dot-enabled' : 'dot-disabled'">
                <span class="dot"></span>
              </span>
            </div>
          </div>
        </template>
      </el-tree>
    </div>

    <!-- Context Menu -->
    <teleport to="body">
      <div
        v-if="contextMenu.visible"
        class="pm-context-menu"
        :style="{ left: contextMenu.x + 'px', top: contextMenu.y + 'px' }"
        @click.stop
      >
        <template v-if="contextMenu.data?.type === 'workspace'">
          <div class="pm-context-item" @click="handleRenameWorkspace"><el-icon><Edit /></el-icon><span>Rename Workspace</span></div>
          <div class="pm-context-divider" />
          <div class="pm-context-item" @click="handleNewProject"><el-icon><Plus /></el-icon><span>Add Project</span></div>
        </template>
        <template v-else-if="contextMenu.data?.type === 'project'">
          <div class="pm-context-item" @click="handleRenameProject"><el-icon><Edit /></el-icon><span>Rename</span></div>
          <div class="pm-context-item" @click="handleDuplicateProject"><el-icon><CopyDocument /></el-icon><span>Duplicate</span></div>
          <div class="pm-context-divider" />
          <div class="pm-context-item" @click="handleExportProject"><el-icon><Download /></el-icon><span>Export Config</span></div>
          <div class="pm-context-item" @click="handleProjectSettings"><el-icon><Setting /></el-icon><span>Settings</span></div>
          <div class="pm-context-divider" />
          <div class="pm-context-item pm-context-danger" @click="handleDeleteProject"><el-icon><Delete /></el-icon><span>Delete</span></div>
        </template>
        <template v-else-if="contextMenu.data?.type === 'ecu'">
          <div class="pm-context-item" @click="openEditEcu(contextMenu.data.ecu)"><el-icon><Edit /></el-icon><span>Edit ECU</span></div>
          <div class="pm-context-divider" />
          <div class="pm-context-item pm-context-danger" @click="confirmRemoveEcu(contextMenu.data.ecu)"><el-icon><Delete /></el-icon><span>Remove ECU</span></div>
        </template>
        <template v-else-if="contextMenu.data?.type === 'layer'">
          <div class="pm-context-item" @click="openAddModule(contextMenu.data)"><el-icon><Plus /></el-icon><span>Add Module</span></div>
        </template>
        <template v-else-if="contextMenu.data?.type === 'module'">
          <div class="pm-context-item" @click="toggleModule(contextMenu.data)">
            <el-icon><component :is="contextMenu.data.enabled ? Close : Check" /></el-icon>
            <span>{{ contextMenu.data.enabled ? 'Disable' : 'Enable' }}</span>
          </div>
          <div class="pm-context-divider" />
          <div class="pm-context-item pm-context-danger" @click="confirmRemoveModule(contextMenu.data)"><el-icon><Delete /></el-icon><span>Remove Module</span></div>
        </template>
      </div>
    </teleport>

    <!-- ECU Editor Dialog -->
    <EcuEditorDialog
      v-model="ecuDialogVisible"
      :edit-data="ecuEditData"
      @confirm="handleEcuConfirm"
    />

    <!-- Add Module Dialog -->
    <AddModuleDialog
      v-model="addModuleDialogVisible"
      :ecu="addModuleEcu"
      :layer="addModuleLayer"
      @confirm="handleAddModuleConfirm"
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

    <!-- Project Settings Dialog -->
    <el-dialog v-model="settingsDialogVisible" title="Project Settings" width="480px">
      <el-form :model="settingsForm" label-width="120px">
        <el-form-item label="Project Name" required>
          <el-input v-model="settingsForm.name" placeholder="Enter project name" autofocus />
        </el-form-item>
        <el-form-item label="Description">
          <el-input v-model="settingsForm.description" type="textarea" :rows="3" placeholder="Project description" />
        </el-form-item>
        <el-divider content-position="left">Info</el-divider>
        <el-form-item label="ECUs">
          <el-text>{{ projectStore.currentProject?.ecus?.length ?? 0 }} ECU(s)</el-text>
        </el-form-item>
        <el-form-item label="Path">
          <el-text type="info" size="small">{{ projectStore.currentProject?.path || 'Not saved' }}</el-text>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="settingsDialogVisible = false">Cancel</el-button>
        <el-button type="primary" @click="confirmProjectSettings" :disabled="!settingsForm.name.trim()">Save</el-button>
      </template>
    </el-dialog>

    <!-- Open Project Dialog -->
    <el-dialog v-model="openProjectDialogVisible" title="Open Project" width="500px" @open="loadProjectList">
      <div v-if="projectListLoading" style="text-align: center; padding: 20px;">
        <el-icon class="is-loading"><Loading /></el-icon>
        <span style="margin-left: 8px;">Loading projects...</span>
      </div>
      <div v-else-if="projectList.length === 0" style="text-align: center; padding: 20px; color: #909399;">
        No projects found. Create a new project to get started.
      </div>
      <div v-else class="project-list">
        <div
          v-for="proj in projectList"
          :key="proj.path"
          class="project-item"
          :class="{ 'project-item-active': proj.path === projectStore.workspacePath }"
          @click="handleOpenProject(proj.path)"
        >
          <div class="project-item-info">
            <div class="project-item-name">{{ proj.name }}</div>
            <div class="project-item-meta">{{ proj.ecu_count }} ECU{{ proj.ecu_count !== 1 ? 's' : '' }} · {{ proj.modified || 'Unknown' }}</div>
            <div v-if="proj.description" class="project-item-desc">{{ proj.description }}</div>
          </div>
          <el-icon class="project-item-arrow"><ArrowRight /></el-icon>
        </div>
      </div>
      <template #footer>
        <el-button @click="openProjectDialogVisible = false">Close</el-button>
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
  FolderOpened, ArrowRight, Loading,
} from '@element-plus/icons-vue'
import { useProjectStore } from '@/stores/project'
import { useConfigStore } from '@/stores/config'
import { ElMessage, ElMessageBox } from 'element-plus'
import EcuEditorDialog from './EcuEditorDialog.vue'
import AddModuleDialog from './AddModuleDialog.vue'
import type { EcuConfig } from '@/types'
import * as api from '@/api/client'

const router = useRouter()
const projectStore = useProjectStore()
const configStore = useConfigStore()

const searchText = ref('')
const treeRef = ref()
const ecuDialogVisible = ref(false)
const ecuEditData = ref<{ id: string; name: string; description?: string; chip?: { vendor: string; series: string; model: string } } | undefined>()
const addModuleDialogVisible = ref(false)
const addModuleEcu = ref<EcuConfig | null>(null)
const addModuleLayer = ref<string>('')
const addModuleProjectId = ref<string>('')
const contextMenu = ref({ visible: false, x: 0, y: 0, data: null as any })
const renameDialogVisible = ref(false)
const renameForm = ref({ name: '', description: '' })
const renameInputRef = ref()
const openProjectDialogVisible = ref(false)
const projectList = ref<Array<{ name: string; description: string; path: string; ecu_count: number; modified: string }>>([])
const projectListLoading = ref(false)

async function loadProjectList() {
  projectListLoading.value = true
  try {
    const result = await api.listProjects()
    projectList.value = result.projects
  } catch (e: any) {
    ElMessage.error('Failed to load project list: ' + (e.message || 'Unknown error'))
  } finally {
    projectListLoading.value = false
  }
}

async function handleOpenProject(path: string) {
  try {
    await projectStore.openWorkspace(path)
    openProjectDialogVisible.value = false
    ElMessage.success(`Opened project: ${projectStore.projectName}`)
    router.push('/config')
  } catch (e: any) {
    ElMessage.error('Failed to open project: ' + (e.message || 'Unknown error'))
  }
}

const workspaceName = computed(() => projectStore.workspaceName ?? 'yuleASR Workspace')
const projectName = computed(() => configStore.config?.project?.name ?? projectStore.projectName ?? 'yuleASR')
const projectCount = computed(() => projectStore.projects.length)
const ecuCount = computed(() => projectStore.ecus.length)

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
  if (['BswM', 'EcuM'].includes(name)) return 'Integration'
  return 'Services'
}

interface TreeNode {
  id: string
  label: string
  type: 'workspace' | 'project' | 'ecu' | 'layer' | 'module'
  enabled?: boolean
  moduleCount?: number
  enabledCount?: number
  ecuCount?: number
  projectCount?: number
  ecu?: EcuConfig
  projectId?: string
  moduleName?: string
  children?: TreeNode[]
}

const defaultExpandedKeys = computed(() => {
  const keys: string[] = []
  for (const project of projectStore.projects) {
    keys.push(`project-${project.id}`)
    for (const ecu of project.ecus) {
      keys.push(`project-${project.id}-ecu-${ecu.id}`)
      for (const layer of ['MCAL', 'ECUAL', 'Services', 'OS', 'RTE', 'Integration']) {
        keys.push(`project-${project.id}-ecu-${ecu.id}-layer-${layer}`)
      }
    }
  }
  return keys
})

const treeData = computed<TreeNode[]>(() => {
  const nodes: TreeNode[] = []

  for (const project of projectStore.projects) {
    const projectNode: TreeNode = {
      id: `project-${project.id}`,
      label: project.name,
      type: 'project',
      projectId: project.id,
      ecuCount: project.ecus.length,
      children: [],
    }

    for (const ecu of project.ecus) {
      const modules = ecu.modules
      const grouped: Record<string, { label: string; enabled: boolean; moduleName: string }[]> = {
        MCAL: [], ECUAL: [], Services: [], OS: [], RTE: [], Integration: [],
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
            id: `project-${project.id}-ecu-${ecu.id}-layer-${layer}`,
            label: layer,
            type: 'layer' as const,
            moduleCount: mods.length,
            enabledCount,
            ecu,
            projectId: project.id,
            children: mods
              .filter((m) => !searchText.value || m.label.toLowerCase().includes(searchText.value.toLowerCase()))
              .sort((a, b) => a.label.localeCompare(b.label))
              .map((m) => ({
                id: `project-${project.id}-ecu-${ecu.id}-mod-${m.moduleName}`,
                label: m.label,
                type: 'module' as const,
                enabled: m.enabled,
                moduleName: m.moduleName,
                ecu,
                projectId: project.id,
              })),
          }
        })

      const filteredLayers = searchText.value
        ? layerNodes.filter((l) => l.children && l.children.length > 0)
        : layerNodes

      projectNode.children!.push({
        id: `project-${project.id}-ecu-${ecu.id}`,
        label: ecu.name,
        type: 'ecu' as const,
        ecu,
        projectId: project.id,
        children: filteredLayers,
      })
    }

    nodes.push(projectNode)
  }

  return nodes
})

function closeContextMenu() { contextMenu.value.visible = false }

function onNodeClick(data: TreeNode) {
  if (data.type === 'project' && data.projectId) {
    projectStore.switchProject(data.projectId)
    nextTick(() => { treeRef.value?.setCurrentKey(data.id) })
    router.push('/config')
  } else if (data.type === 'module' && data.moduleName) {
    if (data.ecu) projectStore.switchEcu(data.ecu.id)
    projectStore.selectModule(data.moduleName)
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
    data: { type: 'workspace' },
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
    await projectStore.addEcu(data.name, data.chip, data.selectedModules)
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

async function ensureProjectActive(projectId?: string) {
  if (projectId && projectId !== projectStore.currentProjectId) {
    await projectStore.switchProject(projectId)
  }
}

function openAddModule(data: TreeNode) {
  closeContextMenu()
  if (data.ecu) {
    addModuleEcu.value = data.ecu
    addModuleLayer.value = data.label
    addModuleProjectId.value = data.projectId || ''
    addModuleDialogVisible.value = true
  }
}

async function handleAddModuleConfirm(modules: string[]) {
  if (addModuleEcu.value) {
    // Operate on the project that owns the right-clicked ECU
    await ensureProjectActive(addModuleProjectId.value)
    for (const modName of modules) {
      await projectStore.addModule(addModuleEcu.value.id, modName)
    }
    ElMessage.success(`Added ${modules.length} module(s) to ECU "${addModuleEcu.value.name}"`)
  }
  addModuleDialogVisible.value = false
  addModuleEcu.value = null
  addModuleLayer.value = ''
  addModuleProjectId.value = ''
}

async function confirmRemoveModule(data: TreeNode) {
  closeContextMenu()
  if (!data.moduleName || !data.ecu) return
  
  // Check dependencies
  const dependents = projectStore.getModuleDependents(data.ecu.id, data.moduleName)
  let confirmMsg = `Remove module "${data.moduleName}" from ECU "${data.ecu.name}"?`
  if (dependents.length > 0) {
    confirmMsg += `\n\nWarning: The following modules depend on ${data.moduleName}:\n${dependents.join(', ')}\n\nThey will also be disabled.`
  }
  
  try {
    await ElMessageBox.confirm(confirmMsg, 'Confirm', {
      type: 'warning',
      confirmButtonText: 'Remove',
    })
    await ensureProjectActive(data.projectId)
    await projectStore.removeModule(data.ecu.id, data.moduleName)
    ElMessage.success(`Module "${data.moduleName}" removed from ECU "${data.ecu.name}"`)
  } catch { /* cancelled */ }
}

async function handleNewProject() {
  ElMessageBox.prompt('Enter project name', 'New Project', {
    confirmButtonText: 'Create',
    cancelButtonText: 'Cancel',
    inputPattern: /^[a-zA-Z0-9_-]+$/,
    inputErrorMessage: 'Only letters, numbers, underscore and hyphen allowed',
  }).then(async ({ value }) => {
    try {
      await projectStore.addProjectToWorkspace(value)
      ElMessage.success(`Project "${value}" added to workspace`)
      router.push('/config')
    } catch (e: any) {
      ElMessage.error(e.message || 'Failed to create project')
    }
  }).catch(() => {})
}

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

async function handleDuplicateProject() {
  closeContextMenu()
  const pid = contextMenu.value.data?.projectId
  if (pid) await ensureProjectActive(pid)
  const project = projectStore.currentProject
  if (!project) return

  try {
    const newName = `${project.name} (Copy)`
    // Generate new path based on original
    const originalPath = project.path || ''
    const dir = originalPath.substring(0, originalPath.lastIndexOf('/'))
    const newPath = dir ? `${dir}/${project.name.replace(/\s+/g, '_')}_copy.yuleasr` : ''
    if (!newPath) {
      throw new Error('Cannot duplicate: project has no file path')
    }
    
    // Build project data from current project
    const projectData = {
      version: '1.0',
      project: {
        name: newName,
        description: project.description,
        created: new Date().toISOString(),
        modified: new Date().toISOString(),
      },
      ecus: project.ecus.map((e) => ({
        id: e.id,
        name: e.name,
        chip: e.chip ?? { vendor: '', series: '', model: '' },
        config_path: '',
        description: e.description ?? '',
        modules: e.modules,
      })),
      ui_state: {
        sidebar_visible: true,
        sidebar_width: 240,
        active_tab: '',
        open_tabs: [],
        expanded_layers: [],
      },
      recent_files: [],
    }
    
    // Save duplicated project
    await api.saveProject(projectData, newPath)
    
    // Add to workspace (backend falls back to the default workspace file)
    await api.addProjectToWorkspace({
      workspace_path: projectStore.workspacePath || undefined,
      project_name: newName,
      project_description: project.description,
      project_path: newPath,
    })
    
    // Reload workspace to pick up the new project
    await projectStore.loadWorkspace()
    
    ElMessage.success(`Project duplicated as "${newName}"`)
  } catch (e: any) {
    ElMessage.error(`Failed to duplicate project: ${e.message || e}`)
  }
}

const settingsDialogVisible = ref(false)
const settingsForm = ref({ name: '', description: '' })

function handleProjectSettings() {
  closeContextMenu()
  const project = projectStore.currentProject
  if (!project) return
  settingsForm.value = { name: project.name, description: project.description }
  settingsDialogVisible.value = true
}

async function confirmProjectSettings() {
  const project = projectStore.currentProject
  if (!project) return
  
  try {
    // Update project name in store
    project.name = settingsForm.value.name
    project.description = settingsForm.value.description
    
    // Save to backend
    const projectData = {
      version: '1.0',
      project: {
        name: settingsForm.value.name,
        description: settingsForm.value.description,
        created: new Date().toISOString(),
        modified: new Date().toISOString(),
      },
      ecus: project.ecus.map((e) => ({
        id: e.id,
        name: e.name,
        chip: e.chip ?? { vendor: '', series: '', model: '' },
        config_path: '',
        description: e.description ?? '',
        modules: e.modules,
      })),
      ui_state: {
        sidebar_visible: true,
        sidebar_width: 240,
        active_tab: '',
        open_tabs: [],
        expanded_layers: [],
      },
      recent_files: [],
    }
    
    await api.saveProject(projectData, project.path)
    settingsDialogVisible.value = false
    ElMessage.success('Project settings saved')
  } catch (e: any) {
    ElMessage.error(`Failed to save settings: ${e.message || e}`)
  }
}

async function handleDeleteProject() {
  closeContextMenu()
  const project = projectStore.currentProject
  if (!project) return

  try {
    await ElMessageBox.confirm(
      `Delete project "${project.name}"? This will remove it from the workspace.\n\nThe project file at ${project.path || '(unsaved)'} will also be deleted.`,
      'Delete Project',
      { type: 'warning', confirmButtonText: 'Delete', cancelButtonText: 'Cancel' },
    )

    // Remove from workspace
    if (projectStore.workspacePath && project.id) {
      await api.removeProjectFromWorkspace({
        workspace_path: projectStore.workspacePath,
        project_id: project.id,
      })
    }

    // Delete the project file if it exists
    if (project.path) {
      try {
        await api.deleteProject(project.path)
      } catch {
        // File may not exist, ignore
      }
    }

    // If this was the current project, switch to another or clear
    const remaining = projectStore.projects.filter((p) => p.id !== project.id)
    if (remaining.length > 0) {
      await projectStore.loadProject(remaining[0].path)
    }

    await projectStore.loadWorkspace()
    ElMessage.success(`Project "${project.name}" deleted`)
  } catch { /* cancelled */ }
}

function handleExportProject() { closeContextMenu(); router.push('/config'); setTimeout(() => ElMessage.info('Click "Export ARXML" in the toolbar'), 300) }

async function handleRenameWorkspace() {
  closeContextMenu()
  try {
    const { value } = await ElMessageBox.prompt('Enter workspace name', 'Rename Workspace', {
      confirmButtonText: 'Save',
      cancelButtonText: 'Cancel',
      inputValue: projectStore.workspaceName,
    })
    if (value?.trim()) {
      projectStore.workspaceName = value.trim()
      await projectStore.saveWorkspace()
      ElMessage.success(`Workspace renamed to "${value.trim()}"`)
    }
  } catch { /* cancelled */ }
}

onMounted(() => { document.addEventListener('click', closeContextMenu) })
onUnmounted(() => { document.removeEventListener('click', closeContextMenu) })

defineExpose({ openAddEcu })
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
.workspace-icon { color: #909399; font-size: 16px; }
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

.status-dot {
  display: flex;
  align-items: center;
  padding: 2px;
}

.status-dot .dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  transition: all 0.2s;
}

.dot-enabled .dot {
  background: #67c23a;
  box-shadow: 0 0 4px rgba(103, 194, 58, 0.4);
}

.dot-disabled .dot {
  background: #dcdfe6;
  border: 1px solid #c0c4cc;
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
:deep(.el-tree-node__content) { height: 34px; background: transparent; }
:deep(.el-tree-node__content:hover) { background: #f0f2f5; }
:deep(.el-tree-node.is-current > .el-tree-node__content) { background: #ecf5ff; }
:deep(.el-tree-node.is-current > .el-tree-node__content .node-label) { color: #409eff; font-weight: 500; }
:deep(.el-tree-node__content .el-tree-node__expand-icon) { font-size: 14px; }

.project-list {
  max-height: 400px;
  overflow-y: auto;
}

.project-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  border: 1px solid #e4e7ed;
  border-radius: 6px;
  margin-bottom: 8px;
  cursor: pointer;
  transition: all 0.2s;
}

.project-item:hover {
  background: #f5f7fa;
  border-color: #409eff;
}

.project-item-active {
  background: #ecf5ff;
  border-color: #409eff;
}

.project-item-info {
  flex: 1;
  min-width: 0;
}

.project-item-name {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
  margin-bottom: 4px;
}

.project-item-meta {
  font-size: 12px;
  color: #909399;
}

.project-item-desc {
  font-size: 12px;
  color: #606266;
  margin-top: 4px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.project-item-arrow {
  font-size: 16px;
  color: #909399;
  flex-shrink: 0;
  margin-left: 12px;
}

.project-item:hover .project-item-arrow {
  color: #409eff;
}
</style>
