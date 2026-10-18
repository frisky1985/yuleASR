import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export interface EditorTab {
  id: string
  moduleName: string
  label: string
  iconClass: string
  dirty: boolean
}

export const useEditorTabsStore = defineStore('editorTabs', () => {
  const tabs = ref<EditorTab[]>([])
  const activeTabId = ref<string | null>(null)

  const activeTab = computed(() =>
    tabs.value.find((t) => t.id === activeTabId.value) ?? null
  )

  function openTab(moduleName: string, label: string, iconClass: string) {
    const existing = tabs.value.find((t) => t.moduleName === moduleName)
    if (existing) {
      activeTabId.value = existing.id
      return existing
    }

    const tab: EditorTab = {
      id: `tab-${moduleName.toLowerCase()}-${Date.now()}`,
      moduleName,
      label,
      iconClass,
      dirty: false,
    }
    tabs.value.push(tab)
    activeTabId.value = tab.id
    return tab
  }

  function closeTab(tabId: string) {
    const idx = tabs.value.findIndex((t) => t.id === tabId)
    if (idx === -1) return

    tabs.value.splice(idx, 1)

    if (activeTabId.value === tabId) {
      if (tabs.value.length > 0) {
        const newIdx = Math.min(idx, tabs.value.length - 1)
        activeTabId.value = tabs.value[newIdx].id
      } else {
        activeTabId.value = null
      }
    }
  }

  function closeOtherTabs(tabId: string) {
    tabs.value = tabs.value.filter((t) => t.id === tabId)
    activeTabId.value = tabId
  }

  function closeAllTabs() {
    tabs.value = []
    activeTabId.value = null
  }

  function markDirty(tabId: string, dirty: boolean) {
    const tab = tabs.value.find((t) => t.id === tabId)
    if (tab) tab.dirty = dirty
  }

  function setActiveTab(tabId: string) {
    activeTabId.value = tabId
  }

  return {
    tabs,
    activeTabId,
    activeTab,
    openTab,
    closeTab,
    closeOtherTabs,
    closeAllTabs,
    markDirty,
    setActiveTab,
  }
})
