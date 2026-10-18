import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export interface ChangeEntry {
  id: string
  timestamp: number
  path: string
  oldValue: any
  newValue: any
  description: string
}

export const useChangeHistoryStore = defineStore('changeHistory', () => {
  const history = ref<ChangeEntry[]>([])
  const undoStack = ref<ChangeEntry[]>([])
  const redoStack = ref<ChangeEntry[]>([])
  const maxHistory = 1000

  const hasChanges = computed(() => history.value.length > 0)
  const canUndo = computed(() => undoStack.value.length > 0)
  const canRedo = computed(() => redoStack.value.length > 0)
  const changeCount = computed(() => history.value.length)

  const recentChanges = computed(() => {
    return history.value.slice(-20).reverse()
  })

  function generateId(): string {
    return `${Date.now()}-${Math.random().toString(36).slice(2, 9)}`
  }

  function recordChange(path: string, oldValue: any, newValue: any, description?: string) {
    if (JSON.stringify(oldValue) === JSON.stringify(newValue)) return

    const entry: ChangeEntry = {
      id: generateId(),
      timestamp: Date.now(),
      path,
      oldValue,
      newValue,
      description: description || `Changed ${path}`,
    }

    history.value.push(entry)
    undoStack.value.push(entry)
    redoStack.value = []

    if (history.value.length > maxHistory) {
      history.value = history.value.slice(-maxHistory)
    }
  }

  function undo(): ChangeEntry | null {
    const entry = undoStack.value.pop()
    if (entry) {
      redoStack.value.push(entry)
    }
    return entry || null
  }

  function redo(): ChangeEntry | null {
    const entry = redoStack.value.pop()
    if (entry) {
      undoStack.value.push(entry)
    }
    return entry || null
  }

  function getChangesByPath(path: string): ChangeEntry[] {
    return history.value.filter((e) => e.path === path || e.path.startsWith(path + '.'))
  }

  function getChangesByTimeRange(start: number, end: number): ChangeEntry[] {
    return history.value.filter((e) => e.timestamp >= start && e.timestamp <= end)
  }

  function clearHistory() {
    history.value = []
    undoStack.value = []
    redoStack.value = []
  }

  function exportHistory(format: 'json' | 'csv' = 'json'): string {
    if (format === 'csv') {
      const header = 'Timestamp,Path,Old Value,New Value,Description\n'
      const rows = history.value.map((e) => {
        const time = new Date(e.timestamp).toISOString()
        const oldVal = JSON.stringify(e.oldValue).replace(/"/g, '""')
        const newVal = JSON.stringify(e.newValue).replace(/"/g, '""')
        const desc = e.description.replace(/"/g, '""')
        return `"${time}","${e.path}","${oldVal}","${newVal}","${desc}"`
      })
      return header + rows.join('\n')
    }

    return JSON.stringify(
      {
        exported: new Date().toISOString(),
        totalChanges: history.value.length,
        changes: history.value.map((e) => ({
          timestamp: new Date(e.timestamp).toISOString(),
          path: e.path,
          oldValue: e.oldValue,
          newValue: e.newValue,
          description: e.description,
        })),
      },
      null,
      2,
    )
  }

  function importHistory(json: string) {
    try {
      const data = JSON.parse(json)
      if (data.changes && Array.isArray(data.changes)) {
        history.value = data.changes.map((c: any) => ({
          id: generateId(),
          timestamp: new Date(c.timestamp).getTime(),
          path: c.path,
          oldValue: c.oldValue,
          newValue: c.newValue,
          description: c.description || '',
        }))
      }
    } catch {
      throw new Error('Invalid history format')
    }
  }

  return {
    history,
    hasChanges,
    canUndo,
    canRedo,
    changeCount,
    recentChanges,
    recordChange,
    undo,
    redo,
    getChangesByPath,
    getChangesByTimeRange,
    clearHistory,
    exportHistory,
    importHistory,
  }
})
