<template>
  <div class="module-tree">
    <div class="tree-search">
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

    <el-tree
      ref="treeRef"
      :data="filteredTreeData"
      :props="treeProps"
      node-key="id"
      highlight-current
      :expand-on-click-node="false"
      @node-click="handleNodeClick"
      @node-contextmenu="handleContextMenu"
      default-expand-all
    >
      <template #default="{ node, data }">
        <div class="tree-node" @contextmenu.prevent="handleContextMenu($event, node, data)">
          <div class="node-left">
            <svg class="node-icon-svg" :class="data.iconClass" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <template v-if="data.iconClass === 'icon-mcal' || data.iconClass === 'icon-os'">
                <rect x="4" y="4" width="16" height="16" rx="2"/>
                <rect x="9" y="9" width="6" height="6"/>
                <line x1="9" y1="1" x2="9" y2="4"/><line x1="15" y1="1" x2="15" y2="4"/>
                <line x1="9" y1="20" x2="9" y2="23"/><line x1="15" y1="20" x2="15" y2="23"/>
                <line x1="20" y1="9" x2="23" y2="9"/><line x1="20" y1="14" x2="23" y2="14"/>
                <line x1="1" y1="9" x2="4" y2="9"/><line x1="1" y1="14" x2="4" y2="14"/>
              </template>
              <template v-else-if="data.iconClass === 'icon-ecual' || data.iconClass === 'icon-rte'">
                <path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"/>
                <path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"/>
              </template>
              <template v-else-if="data.iconClass === 'icon-services'">
                <circle cx="12" cy="12" r="3"/>
                <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06A1.65 1.65 0 0 0 19.4 9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/>
              </template>
              <template v-else-if="data.iconClass === 'icon-integration'">
                <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/>
              </template>
              <template v-else>
                <rect x="4" y="4" width="16" height="16" rx="2"/>
                <rect x="9" y="9" width="6" height="6"/>
              </template>
            </svg>
            <span class="node-label">{{ data.label }}</span>
          </div>
          <div class="node-right">
            <span
              v-if="data.moduleCount !== undefined"
              class="node-badge"
              :class="{ 'badge-enabled': data.enabledCount > 0 }"
            >
              {{ data.enabledCount }}/{{ data.moduleCount }}
            </span>
            <span
              v-else-if="data.status"
              class="status-badge"
              :class="`status-${data.status}`"
            >
              {{ data.status === 'enabled' ? 'ON' : 'OFF' }}
            </span>
          </div>
        </div>
      </template>
    </el-tree>

    <!-- 右键菜单 -->
    <div
      v-if="contextMenu.visible"
      class="context-menu"
      :style="{ left: contextMenu.x + 'px', top: contextMenu.y + 'px' }"
    >
      <div
        v-for="item in contextMenuItems"
        :key="item.key"
        class="context-menu-item"
        :class="{ disabled: item.disabled }"
        @click="handleContextAction(item.key)"
      >
        <el-icon v-if="item.icon"><component :is="item.icon" /></el-icon>
        <span>{{ item.label }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, markRaw } from 'vue'
import {
  Search,
  Cpu,
  Connection,
  Setting,
  Monitor,
  Folder,
  Check,
  Close,
  Expand,
  Fold,
  Refresh,
} from '@element-plus/icons-vue'
import type { ModuleConfig } from '@/types'

interface ModuleNode {
  id: string
  label: string
  icon: any
  iconClass: string
  category?: string
  moduleCount?: number
  enabledCount?: number
  status?: 'enabled' | 'disabled'
  children?: ModuleNode[]
}

interface ContextMenuItem {
  key: string
  label: string
  icon: any
  disabled?: boolean
}

const props = defineProps<{
  modules: Record<string, ModuleConfig>
}>()

const emit = defineEmits<{
  nodeClick: [data: ModuleNode]
  enableModule: [name: string]
  disableModule: [name: string]
  expandAll: []
  collapseAll: []
  refresh: []
}>()

const searchText = ref('')
const treeRef = ref()
const contextMenu = ref({ visible: false, x: 0, y: 0, node: null as ModuleNode | null })

const treeProps = {
  children: 'children',
  label: 'label',
}

const layerIcons: Record<string, { icon: any; iconClass: string }> = {
  MCAL: { icon: markRaw(Cpu), iconClass: 'icon-mcal' },
  ECUAL: { icon: markRaw(Connection), iconClass: 'icon-ecual' },
  Services: { icon: markRaw(Setting), iconClass: 'icon-services' },
  OS: { icon: markRaw(Monitor), iconClass: 'icon-os' },
  RTE: { icon: markRaw(Connection), iconClass: 'icon-rte' },
  Integration: { icon: markRaw(Folder), iconClass: 'icon-integration' },
}

const treeData = computed<ModuleNode[]>(() => {
  const modules = props.modules
  const grouped: Record<string, ModuleNode[]> = {
    MCAL: [],
    ECUAL: [],
    Services: [],
    OS: [],
    RTE: [],
    Integration: [],
  }

  for (const [name, mod] of Object.entries(modules)) {
    const layer = guessLayer(name)
    if (grouped[layer]) {
      grouped[layer].push({
        id: `mod-${name.toLowerCase()}`,
        label: name,
        icon: layerIcons[layer].icon,
        iconClass: layerIcons[layer].iconClass,
        category: layer,
        status: mod.enabled ? 'enabled' : 'disabled',
      })
    }
  }

  return Object.entries(grouped).map(([layer, mods]) => {
    const enabledCount = mods.filter((m) => {
      const mod = modules[m.label]
      return mod?.enabled
    }).length
    return {
      id: `layer-${layer.toLowerCase()}`,
      label: layer,
      icon: layerIcons[layer].icon,
      iconClass: layerIcons[layer].iconClass,
      moduleCount: mods.length,
      enabledCount,
      children: mods.sort((a, b) => a.label.localeCompare(b.label)),
    }
  })
})

const filteredTreeData = computed(() => {
  if (!searchText.value) return treeData.value
  const filter = (nodes: ModuleNode[]): ModuleNode[] => {
    return nodes.reduce((acc, node) => {
      if (node.label.toLowerCase().includes(searchText.value.toLowerCase())) {
        acc.push(node)
      } else if (node.children) {
        const filteredChildren = filter(node.children)
        if (filteredChildren.length > 0) {
          acc.push({ ...node, children: filteredChildren })
        }
      }
      return acc
    }, [] as ModuleNode[])
  }
  return filter(treeData.value)
})

const contextMenuItems = computed<ContextMenuItem[]>(() => {
  const node = contextMenu.value.node
  if (!node) return []

  if (node.category) {
    return [
      { key: 'expand-all', label: 'Expand All', icon: markRaw(Expand) },
      { key: 'collapse-all', label: 'Collapse All', icon: markRaw(Fold) },
      { key: 'refresh', label: 'Refresh', icon: markRaw(Refresh) },
    ]
  }

  const mod = props.modules[node.label]
  const isEnabled = mod?.enabled
  return [
    {
      key: isEnabled ? 'disable' : 'enable',
      label: isEnabled ? 'Disable Module' : 'Enable Module',
      icon: markRaw(isEnabled ? Close : Check),
    },
    { key: 'expand-all', label: 'Expand All', icon: markRaw(Expand) },
    { key: 'collapse-all', label: 'Collapse All', icon: markRaw(Fold) },
  ]
})

function guessLayer(name: string): string {
  const mcal = ['Mcu', 'Port', 'Dio', 'Can', 'Spi', 'Adc', 'Pwm', 'Gpt', 'Wdg', 'Icu', 'Lin', 'Eep', 'Fls', 'Eth', 'EthTrcv', 'EthSwt', 'I2c', 'Ocu']
  const ecual = ['CanIf', 'CanTp', 'IoHwAb', 'EthIf', 'MemIf', 'Fee', 'Ea', 'FrIf', 'LinIf', 'PduR', 'EthTSyn']
  if (mcal.includes(name)) return 'MCAL'
  if (ecual.includes(name)) return 'ECUAL'
  if (name === 'Os') return 'OS'
  if (name === 'Rte') return 'RTE'
  if (['BswM', 'EcuM', 'Det'].includes(name)) return 'Integration'
  return 'Services'
}

function handleNodeClick(data: ModuleNode) {
  if (data.category) {
    emit('nodeClick', data)
  }
}

function handleContextMenu(event: MouseEvent, _node: any, data: ModuleNode) {
  event.preventDefault()
  contextMenu.value = {
    visible: true,
    x: event.clientX,
    y: event.clientY,
    node: data,
  }
}

function handleContextAction(key: string) {
  const node = contextMenu.value.node
  closeContextMenu()
  if (!node) return

  switch (key) {
    case 'enable':
      emit('enableModule', node.label)
      break
    case 'disable':
      emit('disableModule', node.label)
      break
    case 'expand-all':
      emit('expandAll')
      break
    case 'collapse-all':
      emit('collapseAll')
      break
    case 'refresh':
      emit('refresh')
      break
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
.module-tree {
  height: 100%;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.tree-search {
  padding: 10px;
  border-bottom: 1px solid #e4e7ed;
}

.tree-node {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 2px 0;
  width: 100%;
}

.node-left {
  display: flex;
  align-items: center;
  gap: 8px;
  flex: 1;
  min-width: 0;
}

.node-icon-svg {
  width: 16px;
  height: 16px;
  flex-shrink: 0;
}

.icon-mcal { color: #409eff; }
.icon-ecual { color: #67c23a; }
.icon-services { color: #e6a23c; }
.icon-os { color: #f56c6c; }
.icon-rte { color: #909399; }
.icon-integration { color: #b37feb; }
.icon-module { color: #606266; }

.node-label {
  font-size: 14px;
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
  font-size: 11px;
  padding: 1px 6px;
  border-radius: 10px;
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
  padding: 1px 6px;
  border-radius: 8px;
  letter-spacing: 0.3px;
  flex-shrink: 0;
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

.context-menu {
  position: fixed;
  z-index: 9999;
  background: #fff;
  border: 1px solid #e4e7ed;
  border-radius: 6px;
  padding: 4px 0;
  min-width: 160px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.12);
}

.context-menu-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  font-size: 13px;
  color: #303133;
  cursor: pointer;
  transition: background 0.15s;
}

.context-menu-item:hover {
  background: #f5f7fa;
  color: #409eff;
}

.context-menu-item.disabled {
  color: #c0c4cc;
  cursor: not-allowed;
}

.context-menu-item.disabled:hover {
  background: transparent;
  color: #c0c4cc;
}

:deep(.el-tree) {
  background: transparent;
  flex: 1;
  overflow-y: auto;
}

:deep(.el-tree-node__content) {
  height: 36px;
  background: transparent;
  color: #303133;
}

:deep(.el-tree-node__content:hover) {
  background: #f5f7fa;
}

:deep(.el-tree-node.is-current > .el-tree-node__content) {
  background: #ecf5ff;
  color: #409eff;
}

:deep(.el-tree-node.is-current > .el-tree-node__content .node-label) {
  color: #409eff;
}
</style>
