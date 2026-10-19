<template>
  <div class="ecu-comparison">
    <div class="comparison-header">
      <h2>ECU Comparison</h2>
      <el-button @click="$emit('close')">
        <el-icon><Close /></el-icon> Close
      </el-button>
    </div>

    <div class="comparison-selectors">
      <div class="selector-group">
        <label>ECU A:</label>
        <el-select v-model="ecuAId" placeholder="Select ECU A">
          <el-option
            v-for="ecu in projectStore.ecus"
            :key="ecu.id"
            :label="ecu.name"
            :value="ecu.id"
          />
        </el-select>
      </div>
      <div class="selector-group">
        <label>ECU B:</label>
        <el-select v-model="ecuBId" placeholder="Select ECU B">
          <el-option
            v-for="ecu in projectStore.ecus"
            :key="ecu.id"
            :label="ecu.name"
            :value="ecu.id"
          />
        </el-select>
      </div>
    </div>

    <div v-if="ecuA && ecuB" class="comparison-content">
      <div class="comparison-stats">
        <el-statistic title="Total Modules" :value="totalModules" />
        <el-statistic title="Common Modules" :value="commonModules.length" />
        <el-statistic title="Only in A" :value="onlyInA.length" />
        <el-statistic title="Only in B" :value="onlyInB.length" />
        <el-statistic title="Config Differences" :value="differences.length" />
      </div>

      <el-tabs v-model="activeTab" class="comparison-tabs">
        <el-tab-pane label="Common Modules" name="common">
          <el-table :data="commonModules" stripe>
            <el-table-column prop="name" label="Module" width="200" />
            <el-table-column label="ECU A" width="120">
              <template #default="{ row }">
                <el-tag :type="row.enabledA ? 'success' : 'info'" size="small">
                  {{ row.enabledA ? 'Enabled' : 'Disabled' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="ECU B" width="120">
              <template #default="{ row }">
                <el-tag :type="row.enabledB ? 'success' : 'info'" size="small">
                  {{ row.enabledB ? 'Enabled' : 'Disabled' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="Status Match">
              <template #default="{ row }">
                <el-icon v-if="row.enabledA === row.enabledB" color="#67c23a"><CircleCheck /></el-icon>
                <el-icon v-else color="#f56c6c"><CircleClose /></el-icon>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>

        <el-tab-pane :label="`Only in ${ecuA.name}`" name="onlyA">
          <div v-if="onlyInA.length === 0" class="empty-message">
            No modules unique to {{ ecuA.name }}
          </div>
          <el-table v-else :data="onlyInA" stripe>
            <el-table-column prop="name" label="Module" />
            <el-table-column label="Status" width="120">
              <template #default="{ row }">
                <el-tag :type="row.enabled ? 'success' : 'info'" size="small">
                  {{ row.enabled ? 'Enabled' : 'Disabled' }}
                </el-tag>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>

        <el-tab-pane :label="`Only in ${ecuB.name}`" name="onlyB">
          <div v-if="onlyInB.length === 0" class="empty-message">
            No modules unique to {{ ecuB.name }}
          </div>
          <el-table v-else :data="onlyInB" stripe>
            <el-table-column prop="name" label="Module" />
            <el-table-column label="Status" width="120">
              <template #default="{ row }">
                <el-tag :type="row.enabled ? 'success' : 'info'" size="small">
                  {{ row.enabled ? 'Enabled' : 'Disabled' }}
                </el-tag>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>

        <el-tab-pane label="Config Differences" name="diffs">
          <div v-if="differences.length === 0" class="empty-message">
            No configuration differences found
          </div>
          <el-table v-else :data="differences" stripe>
            <el-table-column prop="module" label="Module" width="150" />
            <el-table-column prop="param" label="Parameter" width="200" />
            <el-table-column label="ECU A Value">
              <template #default="{ row }">
                <code>{{ row.valueA }}</code>
              </template>
            </el-table-column>
            <el-table-column label="ECU B Value">
              <template #default="{ row }">
                <code>{{ row.valueB }}</code>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>
      </el-tabs>
    </div>

    <div v-else class="empty-state">
      <el-empty description="Select two ECUs to compare" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Close, CircleCheck, CircleClose } from '@element-plus/icons-vue'
import { useProjectStore } from '@/stores/project'

defineEmits<{
  close: []
}>()

const projectStore = useProjectStore()

const ecuAId = ref('')
const ecuBId = ref('')
const activeTab = ref('common')

const ecuA = computed(() => projectStore.ecus.find(e => e.id === ecuAId.value))
const ecuB = computed(() => projectStore.ecus.find(e => e.id === ecuBId.value))

const allModuleNames = computed(() => {
  if (!ecuA.value || !ecuB.value) return []
  const names = new Set([
    ...Object.keys(ecuA.value.modules),
    ...Object.keys(ecuB.value.modules),
  ])
  return Array.from(names).sort()
})

const totalModules = computed(() => allModuleNames.value.length)

const commonModules = computed(() => {
  if (!ecuA.value || !ecuB.value) return []
  return allModuleNames.value
    .filter(name => name in ecuA.value!.modules && name in ecuB.value!.modules)
    .map(name => ({
      name,
      enabledA: ecuA.value!.modules[name].enabled,
      enabledB: ecuB.value!.modules[name].enabled,
    }))
})

const onlyInA = computed(() => {
  if (!ecuA.value || !ecuB.value) return []
  return Object.entries(ecuA.value.modules)
    .filter(([name]) => !(name in ecuB.value!.modules))
    .map(([name, mod]) => ({ name, enabled: mod.enabled }))
})

const onlyInB = computed(() => {
  if (!ecuA.value || !ecuB.value) return []
  return Object.entries(ecuB.value.modules)
    .filter(([name]) => !(name in ecuA.value!.modules))
    .map(([name, mod]) => ({ name, enabled: mod.enabled }))
})

const differences = computed(() => {
  if (!ecuA.value || !ecuB.value) return []
  const diffs: Array<{module: string, param: string, valueA: any, valueB: any}> = []
  
  for (const moduleName of commonModules.value.map(m => m.name)) {
    const modA = ecuA.value.modules[moduleName]
    const modB = ecuB.value.modules[moduleName]
    
    if (modA.enabled !== modB.enabled) {
      diffs.push({
        module: moduleName,
        param: 'enabled',
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
        diffs.push({
          module: moduleName,
          param,
          valueA: valA,
          valueB: valB,
        })
      }
    }
  }
  
  return diffs
})
</script>

<style scoped>
.ecu-comparison {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #fff;
}

.comparison-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 20px;
  border-bottom: 1px solid #e4e7ed;
}

.comparison-header h2 {
  margin: 0;
  font-size: 18px;
  font-weight: 600;
}

.comparison-selectors {
  display: flex;
  gap: 24px;
  padding: 16px 20px;
  background: #f5f7fa;
  border-bottom: 1px solid #e4e7ed;
}

.selector-group {
  display: flex;
  align-items: center;
  gap: 8px;
}

.selector-group label {
  font-weight: 500;
  color: #606266;
}

.comparison-content {
  flex: 1;
  overflow: auto;
  padding: 20px;
}

.comparison-stats {
  display: flex;
  gap: 32px;
  margin-bottom: 24px;
}

.comparison-tabs {
  margin-top: 20px;
}

.empty-message {
  padding: 40px;
  text-align: center;
  color: #909399;
}

.empty-state {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}
</style>
