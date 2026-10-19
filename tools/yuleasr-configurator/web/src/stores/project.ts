import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { EcuConfig, ModuleConfig } from '@/types'
import { useConfigStore } from './config'

const DEFAULT_ECU_ID = 'ecu-default'

export const useProjectStore = defineStore('project', () => {
  const currentEcuId = ref<string>(DEFAULT_ECU_ID)
  const currentModuleName = ref<string>('')
  const ecus = ref<EcuConfig[]>([])

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

    const modules: Record<string, ModuleConfig> = {}
    for (const [name, mod] of Object.entries(cfg.modules)) {
      modules[name] = { ...mod }
    }

    const existing = ecus.value.find((e) => e.id === DEFAULT_ECU_ID)
    if (existing) {
      existing.modules = modules
      existing.name = cfg.project?.name ?? 'yuleASR'
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

  function addEcu(ecu: EcuConfig) {
    ecus.value.push(ecu)
  }

  function removeEcu(id: string) {
    ecus.value = ecus.value.filter((e) => e.id !== id)
    if (currentEcuId.value === id && ecus.value.length > 0) {
      currentEcuId.value = ecus.value[0].id
    }
  }

  function switchEcu(id: string) {
    if (ecus.value.find((e) => e.id === id)) {
      currentEcuId.value = id
      currentModuleName.value = ''
    }
  }

  return {
    currentEcuId,
    currentModuleName,
    ecus,
    currentEcu,
    ecuModules,
    selectedModule,
    syncFromConfig,
    selectModule,
    toggleModule,
    addEcu,
    removeEcu,
    switchEcu,
  }
})
