import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { EcuConfig, ModuleConfig } from '@/types'
import { useConfigStore } from './config'
import * as api from '@/api/client'
import { DEFAULT_BSW_MODULES } from '@/utils/defaultModules'

const DEFAULT_ECU_ID = 'ecu-default'

// Module dependency map: module -> modules it requires (transitively selected in UI)
export const MODULE_DEPENDENCIES: Record<string, string[]> = {
  'Can': ['CanIf', 'CanTp'],
  'CanIf': ['CanTp', 'PduR', 'Com'],
  'CanTp': ['PduR', 'Com'],
  'PduR': ['Com'],
  'Com': [],
  'Eth': ['EthIf'],
  'EthIf': ['EthTSyn'],
  'Spi': ['MemIf'],
  'MemIf': ['Fee', 'Ea'],
  'Fee': ['NvM'],
  'Ea': ['NvM'],
  'NvM': [],
  'Lin': ['LinIf'],
  'LinIf': [],
  'FrIf': [],
  'IoHwAb': [],
  'Det': [],
  'BswM': [],
  'EcuM': [],
  'Os': [],
  'Rte': [],
}

export interface ProjectInWorkspace {
  id: string
  name: string
  description: string
  path: string
  ecus: EcuConfig[]
}

export const useProjectStore = defineStore('project', () => {
  // Workspace state
  const workspacePath = ref<string>('')
  const workspaceName = ref('yuleASR Workspace')
  const workspaceDescription = ref('')
  const projects = ref<ProjectInWorkspace[]>([])
  const currentProjectId = ref<string>('')

  // Current project state (backward compatible)
  const currentEcuId = ref<string>(DEFAULT_ECU_ID)
  const currentModuleName = ref<string>('')
  const ecus = ref<EcuConfig[]>([])
  const projectName = ref('yuleASR')
  const projectDescription = ref('')

  const currentProject = computed(() =>
    projects.value.find((p) => p.id === currentProjectId.value) ?? projects.value[0],
  )

  const currentEcu = computed(() =>
    ecus.value.find((e) => e.id === currentEcuId.value) ?? ecus.value[0],
  )

  const ecuModules = computed(() => currentEcu.value?.modules ?? {})

  const selectedModule = computed<ModuleConfig | null>(() => {
    if (!currentModuleName.value) return null
    return ecuModules.value[currentModuleName.value] ?? null
  })

  function syncFromConfig() {
    const configStore = useConfigStore()
    const cfg = configStore.config
    if (!cfg) return

    if (cfg.project?.name) projectName.value = cfg.project.name
    if (cfg.project?.description) projectDescription.value = cfg.project.description

    const modules: Record<string, ModuleConfig> = {}
    for (const [name, mod] of Object.entries(cfg.modules)) {
      modules[name] = { ...mod }
    }

    const existing = ecus.value.find((e) => e.id === DEFAULT_ECU_ID)
    if (existing) {
      existing.modules = modules
      if (cfg.project?.name) existing.name = cfg.project.name
    } else {
      ecus.value = [
        {
          id: DEFAULT_ECU_ID,
          name: cfg.project?.name ?? 'yuleASR',
          description: cfg.project?.description,
          modules,
        },
      ]
    }

    if (!currentModuleName.value && Object.keys(modules).length > 0) {
      currentModuleName.value = Object.keys(modules).sort()[0]
    }
  }

  function selectModule(name: string) {
    currentModuleName.value = name
  }

  function toggleModule(name: string) {
    const mod = ecuModules.value[name]
    if (mod) {
      mod.enabled = !mod.enabled
      const configStore = useConfigStore()
      if (configStore.config?.modules[name]) {
        configStore.config.modules[name].enabled = mod.enabled
      }
    }
  }

  async function addEcu(
    name: string,
    chip?: { vendor: string; series: string; model: string },
    selectedModules?: string[],
  ) {
    const id = `ecu-${Date.now()}`
    const modules: Record<string, ModuleConfig> = {}
    
    const moduleNames = selectedModules && selectedModules.length > 0
      ? selectedModules
      : DEFAULT_BSW_MODULES.filter((m) => m.required).map((m) => m.name)
    
    for (const modName of moduleNames) {
      modules[modName] = { name: modName, enabled: true }
    }
    
    const ecu: EcuConfig = {
      id,
      name,
      description: '',
      chip,
      modules,
    }
    ecus.value.push(ecu)
    
    // Ensure there's a project to add the ECU to
    if (!currentProject.value) {
      // Create a default project if none exists
      const defaultProject: ProjectInWorkspace = {
        id: `project-${Date.now()}`,
        name: projectName.value || 'yuleASR',
        description: projectDescription.value,
        path: '',
        ecus: [],
      }
      projects.value.push(defaultProject)
      currentProjectId.value = defaultProject.id
    }
    
    // Also update current project
    if (currentProject.value) {
      currentProject.value.ecus.push(ecu)
    }
    
    try {
      await api.addEcu({ id, name, chip: chip ?? { vendor: '', series: '', model: '' }, modules }, currentProject.value?.path || undefined)
    } catch {
      // Backend may not be available
    }
    return ecu
  }

  async function removeEcu(id: string) {
    ecus.value = ecus.value.filter((e) => e.id !== id)
    if (currentProject.value) {
      currentProject.value.ecus.splice(0, currentProject.value.ecus.length, ...currentProject.value.ecus.filter((e) => e.id !== id))
    }
    if (currentEcuId.value === id && ecus.value.length > 0) {
      currentEcuId.value = ecus.value[0].id
    }
    try {
      await api.removeEcu(id, currentProject.value?.path || undefined)
    } catch {
      // Backend may not be available
    }
  }

  async function addModule(ecuId: string, moduleName: string) {
    const ecu = ecus.value.find((e) => e.id === ecuId)
    if (!ecu) return
    
    if (ecu.modules[moduleName]) {
      // Module already exists, just enable it
      ecu.modules[moduleName].enabled = true
    } else {
      ecu.modules[moduleName] = { name: moduleName, enabled: true }
    }
    
    // Update current project
    if (currentProject.value) {
      const projectEcu = currentProject.value.ecus.find((e) => e.id === ecuId)
      if (projectEcu) {
        projectEcu.modules[moduleName] = ecu.modules[moduleName]
      }
    }
    
    try {
      await api.addModule(ecuId, moduleName, currentProject.value?.path || undefined)
    } catch {
      // Backend may not be available
    }
  }

  async function removeModule(ecuId: string, moduleName: string) {
    const ecu = ecus.value.find((e) => e.id === ecuId)
    if (!ecu) return
    
    // Get dependents and disable them
    const dependents = getModuleDependents(ecuId, moduleName)
    for (const dep of dependents) {
      if (ecu.modules[dep]) {
        ecu.modules[dep].enabled = false
      }
    }
    
    // Remove the module
    delete ecu.modules[moduleName]
    
    // Update current project
    if (currentProject.value) {
      const projectEcu = currentProject.value.ecus.find((e) => e.id === ecuId)
      if (projectEcu) {
        delete projectEcu.modules[moduleName]
        for (const dep of dependents) {
          if (projectEcu.modules[dep]) {
            projectEcu.modules[dep].enabled = false
          }
        }
      }
    }
    
    // Update current module if it was removed
    if (currentModuleName.value === moduleName) {
      currentModuleName.value = Object.keys(ecu.modules).sort()[0] || ''
    }
    
    try {
      await api.removeModule(ecuId, moduleName, currentProject.value?.path || undefined)
    } catch {
      // Backend may not be available
    }
  }

  function getModuleDependents(ecuId: string, moduleName: string): string[] {
    const ecu = ecus.value.find((e) => e.id === ecuId)
    if (!ecu) return []
    
    const directDependents = MODULE_DEPENDENCIES[moduleName] || []
    // Filter to only include modules that are actually in this ECU
    return directDependents.filter((dep) => ecu.modules[dep] && ecu.modules[dep].enabled)
  }

  function switchEcu(id: string) {
    if (ecus.value.find((e) => e.id === id)) {
      currentEcuId.value = id
      currentModuleName.value = ''
    }
  }

  async function loadWorkspace(path?: string) {
    try {
      if (path) workspacePath.value = path
      const data = await api.getWorkspace(workspacePath.value || undefined)
      workspaceName.value = data.workspace.name
      workspaceDescription.value = data.workspace.description
      projects.value = data.projects.map((p: any) => ({
        id: p.id,
        name: p.name,
        description: p.description,
        path: p.path,
        ecus: [],
      }))
      if (projects.value.length > 0) {
        if (!currentProjectId.value) {
          currentProjectId.value = projects.value[0].id
        }
        await loadProject(projects.value.find((p) => p.id === currentProjectId.value)?.path || projects.value[0].path)
      }
    } catch {
      // Backend may not be available
    }
  }

  async function loadProject(path?: string) {
    try {
      const data = await api.getProject(path || currentProject.value?.path || undefined)
      projectName.value = data.project.name
      projectDescription.value = data.project.description
      if (data.ecus.length > 0) {
        ecus.value = data.ecus.map((e: any) => ({
          id: e.id,
          name: e.name,
          description: e.description,
          chip: e.chip,
          modules: e.modules || {},
        }))
        currentEcuId.value = ecus.value[0].id
        
        // Update current project's ecus - use splice for reactivity
        if (currentProject.value) {
          currentProject.value.ecus.splice(0, currentProject.value.ecus.length, ...ecus.value)
        }
      }
    } catch {
      // Backend may not be available
    }
  }

  async function saveProject() {
    try {
      await api.saveProject({
        version: '1.0',
        project: {
          name: projectName.value,
          description: projectDescription.value,
          created: '',
          modified: '',
        },
        ecus: ecus.value.map((e) => ({
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
      }, currentProject.value?.path || undefined)
    } catch {
      // Backend may not be available
    }
  }

  async function saveWorkspace() {
    try {
      await api.saveWorkspace({
        version: '1.0',
        workspace: {
          name: workspaceName.value,
          description: workspaceDescription.value,
          created: '',
          modified: '',
        },
        projects: projects.value.map((p) => ({
          id: p.id,
          name: p.name,
          description: p.description,
          path: p.path,
        })),
      }, workspacePath.value || undefined)
    } catch {
      // Backend may not be available
    }
  }

  async function createWorkspace(name: string, description = '') {
    const result = await api.createWorkspace({ name, description })
    if (result.success) {
      workspacePath.value = result.workspace_path
      workspaceName.value = name
      workspaceDescription.value = description
      projects.value = []
      currentProjectId.value = ''
      ecus.value = []
      projectName.value = ''
      projectDescription.value = ''
    }
    return result
  }

  async function openWorkspace(path: string) {
    workspacePath.value = path
    await loadWorkspace()
  }

  async function switchProject(projectId: string) {
    const project = projects.value.find((p) => p.id === projectId)
    if (project) {
      currentProjectId.value = projectId
      await loadProject(project.path)
    }
  }

  async function addProjectToWorkspace(
    name: string,
    description = '',
    options?: {
      chip?: { vendor: string; series: string; model: string }
      mcalModules?: string[]
      commModules?: string[]
    },
  ) {
    // Create new project file
    const projectResult = await api.createProject({
      name,
      description,
      chip: options?.chip,
      mcal_modules: options?.mcalModules ?? DEFAULT_BSW_MODULES.filter((m) => m.required).map((m) => m.name),
      comm_modules: options?.commModules ?? ['CanIf', 'CanTp', 'PduR', 'Com'],
    })

    if (projectResult.success) {
      // Add to workspace
      await api.addProjectToWorkspace({
        workspace_path: workspacePath.value || undefined,
        project_name: name,
        project_description: description,
        project_path: projectResult.project_path,
      })

      // Reload workspace
      await loadWorkspace()

      // Switch to new project
      const newProject = projects.value.find((p) => p.name === name)
      if (newProject) {
        await switchProject(newProject.id)
      }
    }

    return projectResult
  }

  return {
    // Workspace
    workspacePath,
    workspaceName,
    workspaceDescription,
    projects,
    currentProjectId,
    currentProject,
    
    // Project (backward compatible)
    currentEcuId,
    currentModuleName,
    ecus,
    projectName,
    projectDescription,
    currentEcu,
    ecuModules,
    selectedModule,
    
    // Methods
    syncFromConfig,
    selectModule,
    toggleModule,
    addEcu,
    removeEcu,
    addModule,
    removeModule,
    getModuleDependents,
    switchEcu,
    loadWorkspace,
    loadProject,
    saveProject,
    saveWorkspace,
    createWorkspace,
    openWorkspace,
    switchProject,
    addProjectToWorkspace,
  }
})
