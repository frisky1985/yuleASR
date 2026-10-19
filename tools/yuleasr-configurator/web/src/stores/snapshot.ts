import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { useConfigStore } from './config'

export interface ConfigSnapshot {
  id: string
  name: string
  description: string
  timestamp: number
  config: any
  tags: string[]
}

export const useSnapshotStore = defineStore('snapshot', () => {
  const snapshots = ref<ConfigSnapshot[]>([])
  const currentSnapshotId = ref<string | null>(null)

  const currentSnapshot = computed(() =>
    snapshots.value.find(s => s.id === currentSnapshotId.value) ?? null
  )

  const sortedSnapshots = computed(() =>
    [...snapshots.value].sort((a, b) => b.timestamp - a.timestamp)
  )

  function saveSnapshot(name: string, description: string = '', tags: string[] = []) {
    const configStore = useConfigStore()
    if (!configStore.config) return null

    const snapshot: ConfigSnapshot = {
      id: `snapshot-${Date.now()}`,
      name,
      description,
      timestamp: Date.now(),
      config: JSON.parse(JSON.stringify(configStore.config)),
      tags,
    }

    snapshots.value.push(snapshot)
    currentSnapshotId.value = snapshot.id

    // Persist to localStorage
    persistSnapshots()

    return snapshot
  }

  function restoreSnapshot(snapshotId: string) {
    const snapshot = snapshots.value.find(s => s.id === snapshotId)
    if (!snapshot) return false

    const configStore = useConfigStore()
    configStore.replaceConfig(JSON.parse(JSON.stringify(snapshot.config)))
    currentSnapshotId.value = snapshotId

    return true
  }

  function deleteSnapshot(snapshotId: string) {
    const index = snapshots.value.findIndex(s => s.id === snapshotId)
    if (index === -1) return false

    snapshots.value.splice(index, 1)
    if (currentSnapshotId.value === snapshotId) {
      currentSnapshotId.value = null
    }

    persistSnapshots()
    return true
  }

  function compareSnapshots(snapshotIdA: string, snapshotIdB: string) {
    const snapshotA = snapshots.value.find(s => s.id === snapshotIdA)
    const snapshotB = snapshots.value.find(s => s.id === snapshotIdB)

    if (!snapshotA || !snapshotB) return null

    const configA = snapshotA.config
    const configB = snapshotB.config

    const differences: Array<{
      type: 'module' | 'parameter'
      path: string
      valueA: any
      valueB: any
    }> = []

    // Compare modules
    const allModules = new Set([
      ...Object.keys(configA.modules || {}),
      ...Object.keys(configB.modules || {}),
    ])

    for (const moduleName of allModules) {
      const modA = configA.modules?.[moduleName]
      const modB = configB.modules?.[moduleName]

      if (!modA) {
        differences.push({
          type: 'module',
          path: moduleName,
          valueA: null,
          valueB: modB,
        })
        continue
      }

      if (!modB) {
        differences.push({
          type: 'module',
          path: moduleName,
          valueA: modA,
          valueB: null,
        })
        continue
      }

      if (modA.enabled !== modB.enabled) {
        differences.push({
          type: 'parameter',
          path: `${moduleName}.enabled`,
          valueA: modA.enabled,
          valueB: modB.enabled,
        })
      }

      const paramsA = modA.parameters || {}
      const paramsB = modB.parameters || {}
      const allParams = new Set([...Object.keys(paramsA), ...Object.keys(paramsB)])

      for (const param of allParams) {
        const valA = paramsA[param]
        const valB = paramsB[param]
        if (JSON.stringify(valA) !== JSON.stringify(valB)) {
          differences.push({
            type: 'parameter',
            path: `${moduleName}.${param}`,
            valueA: valA,
            valueB: valB,
          })
        }
      }
    }

    return {
      snapshotA,
      snapshotB,
      differences,
    }
  }

  function persistSnapshots() {
    try {
      localStorage.setItem('yuleasr-snapshots', JSON.stringify(snapshots.value))
    } catch (e) {
      console.error('Failed to persist snapshots:', e)
    }
  }

  function loadSnapshots() {
    try {
      const data = localStorage.getItem('yuleasr-snapshots')
      if (data) {
        snapshots.value = JSON.parse(data)
      }
    } catch (e) {
      console.error('Failed to load snapshots:', e)
    }
  }

  return {
    snapshots,
    currentSnapshotId,
    currentSnapshot,
    sortedSnapshots,
    saveSnapshot,
    restoreSnapshot,
    deleteSnapshot,
    compareSnapshots,
    loadSnapshots,
  }
})
