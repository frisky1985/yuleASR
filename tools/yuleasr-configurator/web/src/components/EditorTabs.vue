<template>
  <div class="editor-tabs">
    <div
      v-for="tab in tabs"
      :key="tab.id"
      class="editor-tab"
      :class="{ active: tab.id === activeTabId }"
      @click="setActiveTab(tab.id)"
      @contextmenu.prevent="handleTabContextMenu($event, tab)"
    >
      <svg class="tab-icon" :class="tab.iconClass" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <template v-if="tab.iconClass === 'icon-mcal' || tab.iconClass === 'icon-os'">
          <rect x="4" y="4" width="16" height="16" rx="2"/>
          <rect x="9" y="9" width="6" height="6"/>
        </template>
        <template v-else-if="tab.iconClass === 'icon-ecual' || tab.iconClass === 'icon-rte'">
          <path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"/>
          <path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"/>
        </template>
        <template v-else-if="tab.iconClass === 'icon-services'">
          <circle cx="12" cy="12" r="3"/>
          <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06A1.65 1.65 0 0 0 19.4 9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/>
        </template>
        <template v-else>
          <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/>
        </template>
      </svg>
      <span class="tab-label">{{ tab.label }}</span>
      <span v-if="tab.dirty" class="tab-dirty">*</span>
      <span class="tab-close" @click.stop="closeTab(tab.id)">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="12" height="12">
          <line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/>
        </svg>
      </span>
    </div>

    <!-- Tab context menu -->
    <div
      v-if="contextMenu.visible"
      class="tab-context-menu"
      :style="{ left: contextMenu.x + 'px', top: contextMenu.y + 'px' }"
    >
      <div class="tab-context-item" @click="closeTab(contextMenu.tabId)">
        Close
      </div>
      <div class="tab-context-item" @click="closeOtherTabs(contextMenu.tabId)">
        Close Others
      </div>
      <div class="tab-context-item" @click="closeAllTabs">
        Close All
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import { useEditorTabsStore } from '@/stores/editorTabs'

const store = useEditorTabsStore()
const { tabs, activeTabId, closeTab, closeOtherTabs, closeAllTabs, setActiveTab } = store

const contextMenu = ref({ visible: false, x: 0, y: 0, tabId: '' })

function handleTabContextMenu(event: MouseEvent, tab: any) {
  event.preventDefault()
  contextMenu.value = {
    visible: true,
    x: event.clientX,
    y: event.clientY,
    tabId: tab.id,
  }
}

function closeContextMenu() {
  contextMenu.value.visible = false
}

onMounted(() => {
  document.addEventListener('click', closeContextMenu)
})

onUnmounted(() => {
  document.removeEventListener('click', closeContextMenu)
})
</script>

<style scoped>
.editor-tabs {
  display: flex;
  background: #f5f7fa;
  border-bottom: 1px solid #e4e7ed;
  overflow-x: auto;
  min-height: 36px;
}

.editor-tab {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 12px;
  font-size: 13px;
  color: #909399;
  cursor: pointer;
  border-right: 1px solid #e4e7ed;
  white-space: nowrap;
  transition: background 0.15s, color 0.15s;
  user-select: none;
}

.editor-tab:hover {
  background: #fff;
  color: #303133;
}

.editor-tab.active {
  background: #fff;
  color: #409eff;
  border-bottom: 2px solid #409eff;
}

.tab-icon {
  width: 14px;
  height: 14px;
  flex-shrink: 0;
}

.tab-label {
  max-width: 120px;
  overflow: hidden;
  text-overflow: ellipsis;
}

.tab-dirty {
  color: #e6a23c;
  font-weight: bold;
}

.tab-close {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 16px;
  height: 16px;
  border-radius: 3px;
  color: #c0c4cc;
  opacity: 0;
  transition: opacity 0.15s, background 0.15s;
}

.editor-tab:hover .tab-close {
  opacity: 1;
}

.tab-close:hover {
  background: #f5f7fa;
  color: #f56c6c;
}

.tab-context-menu {
  position: fixed;
  z-index: 9999;
  background: #fff;
  border: 1px solid #e4e7ed;
  border-radius: 6px;
  padding: 4px 0;
  min-width: 140px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.12);
}

.tab-context-item {
  padding: 8px 16px;
  font-size: 13px;
  color: #303133;
  cursor: pointer;
  transition: background 0.15s;
}

.tab-context-item:hover {
  background: #f5f7fa;
  color: #409eff;
}
</style>
