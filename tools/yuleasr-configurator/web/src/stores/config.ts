import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { Config, ModuleConfig, ValidationResult } from '@/types'
import * as api from '@/api/client'

export const useConfigStore = defineStore('config', () => {
  const config = ref<Config | null>(null)
  const validation = ref<ValidationResult | null>(null)
  const loading = ref(false)
  const error = ref<string | null>(null)

  const modules = computed(() => config.value?.modules ?? {})
  const moduleCount = computed(() => Object.keys(modules.value).length)
  const enabledModules = computed(() =>
    Object.values(modules.value).filter((m) => m.enabled)
  )

  async function loadConfig(path?: string) {
    loading.value = true
    error.value = null
    try {
      config.value = await api.getConfig(path)
      await validateConfig()
    } catch (e: any) {
      error.value = e.message
      throw e
    } finally {
      loading.value = false
    }
  }

  async function saveConfig(path?: string) {
    if (!config.value) return
    loading.value = true
    error.value = null
    try {
      await api.saveConfig(config.value, path)
      await validateConfig()
    } catch (e: any) {
      error.value = e.message
      throw e
    } finally {
      loading.value = false
    }
  }

  async function validateConfig() {
    if (!config.value) return
    try {
      validation.value = await api.validateConfig(config.value)
    } catch (e: any) {
      validation.value = {
        valid: false,
        errors: [{ path: '', message: e.message }],
        warnings: [],
      }
    }
  }

  function updateModule(name: string, updates: Partial<ModuleConfig>) {
    if (!config.value) return
    config.value.modules[name] = {
      ...config.value.modules[name],
      ...updates,
    }
  }

  function toggleModule(name: string) {
    if (!config.value) return
    config.value.modules[name].enabled = !config.value.modules[name].enabled
  }

  function replaceConfig(newConfig: Config) {
    config.value = newConfig
  }

  function mergeModules(importedModules: Record<string, ModuleConfig>) {
    if (!config.value) return
    for (const [name, mod] of Object.entries(importedModules)) {
      if (config.value.modules[name]) {
        config.value.modules[name] = { ...config.value.modules[name], ...mod }
      } else {
        config.value.modules[name] = mod
      }
    }
  }

  return {
    config,
    validation,
    loading,
    error,
    modules,
    moduleCount,
    enabledModules,
    loadConfig,
    saveConfig,
    validateConfig,
    updateModule,
    toggleModule,
    replaceConfig,
    mergeModules,
  }
})
