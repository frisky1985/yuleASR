<template>
  <div class="project-tree">
    <!-- Project header -->
    <div class="project-header">
      <div class="project-name">
        <svg class="header-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/>
        </svg>
        <span class="name-text">{{ projectName }}</span>
      </div>
      <div class="project-meta">
        <span class="meta-item">{{ ecuCount }} ECU</span>
        <span class="meta-divider">|</span>
        <span class="meta-item">{{ totalModules }} modules</span>
      </div>
    </div>

    <!-- ECU selector (shown when multiple ECUs exist) -->
    <div v-if="ecus.length > 1" class="ecu-selector">
      <el-select
        :model-value="currentEcuId"
        size="small"
        style="width: 100%"
        @change="handleEcuSwitch"
      >
        <el-option
          v-for="ecu in ecus"
          :key="ecu.id"
          :label="ecu.name"
          :value="ecu.id"
        />
      </el-select>
    </div>

    <!-- ECU info bar -->
    <div v-if="currentEcu" class="ecu-info">
      <svg class="ecu-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <rect x="4" y="4" width="16" height="16" rx="2"/>
        <rect x="9" y="9" width="6" height="6"/>
        <line x1="9" y1="1" x2="9" y2="4"/><line x1="15" y1="1" x2="15" y2="4"/>
        <line x1="9" y1="20" x2="9" y2="23"/><line x1="15" y1="20" x2="15" y2="23"/>
        <line x1="20" y1="9" x2="23" y2="9"/><line x1="20" y1="14" x2="23" y2="14"/>
        <line x1="1" y1="9" x2="4" y2="9"/><line x1="1" y1="14" x2="4" y2="14"/>
      </svg>
      <div class="ecu-details">
        <span class="ecu-name">{{ currentEcu.name }}</span>
        <span v-if="currentEcu.chip" class="ecu-chip">{{ currentEcu.chip.vendor }} {{ currentEcu.chip.model }}</span>
      </div>
    </div>

    <!-- Module search -->
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

    <!-- Module tree -->
    <el-tree
      ref="treeRef"
      :data="filteredTreeData"
      :props="treeProps"
      node-key="id"
      :current-node-key="selectedNodeId"
      highlight-current
      :expand-on-click-node="false"
      @node-click="handleNodeClick"
      default-expand-all
    >
      <template #default="{ data }">
        <div class="tree-node">
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
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Search } from '@element-plus/icons-vue'
import { useProjectStore } from '@/stores/project'

interface TreeNode {
  id: string
  label: string
  iconClass: string
  category?: string
  moduleCount?: number
  enabledCount?: number
  status?: 'enabled' | 'disabled'
  children?: TreeNode[]
}

const projectStore = useProjectStore()
const searchText = ref('')
const treeRef = ref()

const treeProps = { children: 'children', label: 'label' }

const projectName = computed(() => projectStore.currentEcu?.name ?? 'yuleASR')
const ecus = computed(() => projectStore.ecus)
const currentEcuId = computed(() => projectStore.currentEcuId)
const currentEcu = computed(() => projectStore.currentEcu)
const ecuCount = computed(() => ecus.value.length)
const totalModules = computed(() => Object.keys(projectStore.ecuModules).length)

const selectedNodeId = computed(() => {
  const name = projectStore.currentModuleName
  return name ? `mod-${name.toLowerCase()}` : ''
})

const layerIcons: Record<string, string> = {
  MCAL: 'icon-mcal',
  ECUAL: 'icon-ecual',
  Services: 'icon-services',
  OS: 'icon-os',
  RTE: 'icon-rte',
  Integration: 'icon-integration',
}

const treeData = computed<TreeNode[]>(() => {
  const modules = projectStore.ecuModules
  const grouped: Record<string, TreeNode[]> = {
    MCAL: [], ECUAL: [], Services: [], OS: [], RTE: [], Integration: [],
  }

  for (const [name, mod] of Object.entries(modules)) {
    const layer = guessLayer(name)
    if (grouped[layer]) {
      grouped[layer].push({
        id: `mod-${name.toLowerCase()}`,
        label: name,
        iconClass: layerIcons[layer],
        category: layer,
        status: mod.enabled ? 'enabled' : 'disabled',
      })
    }
  }

  return Object.entries(grouped)
    .filter(([, mods]) => mods.length > 0)
    .map(([layer, mods]) => {
      const enabledCount = mods.filter((m) => modules[m.label]?.enabled).length
      return {
        id: `layer-${layer.toLowerCase()}`,
        label: layer,
        iconClass: layerIcons[layer],
        moduleCount: mods.length,
        enabledCount,
        children: mods.sort((a, b) => a.label.localeCompare(b.label)),
      }
    })
})

const filteredTreeData = computed(() => {
  if (!searchText.value) return treeData.value
  const filter = (nodes: TreeNode[]): TreeNode[] =>
    nodes.reduce((acc, node) => {
      if (node.label.toLowerCase().includes(searchText.value.toLowerCase())) {
        acc.push(node)
      } else if (node.children) {
        const filtered = filter(node.children)
        if (filtered.length > 0) acc.push({ ...node, children: filtered })
      }
      return acc
    }, [] as TreeNode[])
  return filter(treeData.value)
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

function handleNodeClick(data: TreeNode) {
  if (!data.moduleCount) {
    projectStore.selectModule(data.label)
  }
}

function handleEcuSwitch(id: string) {
  projectStore.switchEcu(id)
}
</script>

<style scoped>
.project-tree {
  height: 100%;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  background: #fafbfc;
}

.project-header {
  padding: 12px 12px 8px;
  border-bottom: 1px solid #ebeef5;
}

.project-name {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 4px;
}

.header-icon {
  width: 18px;
  height: 18px;
  color: #409eff;
  flex-shrink: 0;
}

.name-text {
  font-size: 14px;
  font-weight: 600;
  color: #1d2129;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.project-meta {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 11px;
  color: #86909c;
  padding-left: 26px;
}

.meta-divider {
  color: #c9cdd4;
}

.ecu-selector {
  padding: 8px 12px;
  border-bottom: 1px solid #ebeef5;
}

.ecu-info {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  background: #f2f3f5;
  border-bottom: 1px solid #ebeef5;
}

.ecu-icon {
  width: 16px;
  height: 16px;
  color: #409eff;
  flex-shrink: 0;
}

.ecu-details {
  display: flex;
  flex-direction: column;
  min-width: 0;
}

.ecu-name {
  font-size: 12px;
  font-weight: 500;
  color: #1d2129;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.ecu-chip {
  font-size: 11px;
  color: #86909c;
}

.tree-search {
  padding: 8px 12px;
  border-bottom: 1px solid #ebeef5;
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

.node-label {
  font-size: 13px;
  color: #1d2129;
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
  background: #f2f3f5;
  color: #86909c;
}

.node-badge.badge-enabled {
  background: #e8f5e9;
  color: #52c41a;
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
  background: #e8f5e9;
  color: #52c41a;
  border: 1px solid #b7eb8f;
}

.status-disabled {
  background: #f2f3f5;
  color: #86909c;
  border: 1px solid #e5e6eb;
}

:deep(.el-tree) {
  background: transparent;
  flex: 1;
  overflow-y: auto;
  padding: 4px 0;
}

:deep(.el-tree-node__content) {
  height: 32px;
  background: transparent;
  color: #1d2129;
}

:deep(.el-tree-node__content:hover) {
  background: #f2f3f5;
}

:deep(.el-tree-node.is-current > .el-tree-node__content) {
  background: #e8f3ff;
  color: #409eff;
}

:deep(.el-tree-node.is-current > .el-tree-node__content .node-label) {
  color: #409eff;
  font-weight: 500;
}
</style>
