import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { EcuConfig, ModuleConfig } from '@/types'
import { useConfigStore } from './config'
import * as api from '@/api/client'
import { DEFAULT_BSW_MODULES } from '@/utils/defaultModules'

const DEFAULT_ECU_ID = 'ecu-default'

export const useProjectStore = defineStore('project', () => {
  const currentEcuId = ref<string>(DEFAULT_ECU_ID)
  const currentModuleName = ref<string>('')
  const ecus = ref<EcuConfig[]>([])
  const projectName = ref('yuleASR')
  const projectDescription = ref('')

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
    
    // Initialize with selected modules or default modules
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
    try {
      await api.addEcu({ id, name, chip: chip ?? { vendor: '', series: '', model: '' } })
    } catch {
      // Backend may not be available
    }
    return ecu
  }

  async function removeEcu(id: string) {
    ecus.value = ecus.value.filter((e) => e.id !== id)
    if (currentEcuId.value === id && ecus.value.length > 0) {
      currentEcuId.value = ecus.value[0].id
    }
    try {
      await api.removeEcu(id)
    } catch {
      // Backend may not be available
    }
  }

  function switchEcu(id: string) {
    if (ecus.value.find((e) => e.id === id)) {
      currentEcuId.value = id
      currentModuleName.value = ''
    }
  }

  async function loadProject() {
    try {
      const data = await api.getProject()
      projectName.value = data.project.name
      projectDescription.value = data.project.description
      if (data.ecus.length > 0) {
        ecus.value = data.ecus.map((e: any) => ({
          id: e.id,
          name: e.name,
          description: e.description,
          chip: e.chip,
          modules: {},
        }))
        currentEcuId.value = ecus.value[0].id
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
        })),
        ui_state: {
          sidebar_visible: true,
          sidebar_width: 240,
          active_tab: '',
          open_tabs: [],
          expanded_layers: [],
        },
        recent_files: [],
      })
    } catch {
      // Backend may not be available
    }
  }

  return {
    currentEcuId,
    currentModuleName,
    ecus,
    projectName,
    projectDescription,
    currentEcu,
    ecuModules,
    selectedModule,
    syncFromConfig,
    selectModule,
    toggleModule,
    addEcu,
    removeEcu,
    switchEcu,
    loadProject,
    saveProject,
  }
})
