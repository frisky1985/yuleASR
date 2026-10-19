<template>
  <div class="modules-view">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>BSW Modules ({{ Object.keys(grouped).reduce((s, k) => s + grouped[k].length, 0) }})</span>
          <el-input
            v-model="search"
            placeholder="Search modules..."
            style="width: 300px"
            clearable
          />
        </div>
      </template>

      <el-row :gutter="20">
        <el-col :span="6" v-for="(mods, layer) in grouped" :key="layer">
          <el-card shadow="never" class="layer-card">
            <template #header>
              <span>{{ layer }} ({{ mods.length }})</span>
            </template>
            <div
              v-for="mod in mods"
              :key="mod.name"
              class="module-item"
              @click="$router.push(`/modules/${mod.name}`)"
            >
              <el-icon :color="mod.enabled ? '#67c23a' : '#909399'">
                <CircleCheck v-if="mod.enabled" />
                <CircleClose v-else />
              </el-icon>
              <span>{{ mod.name }}</span>
            </div>
          </el-card>
        </el-col>
      </el-row>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useConfigStore } from '@/stores/config'

const store = useConfigStore()
const search = ref('')

const grouped = computed(() => {
  const result: Record<string, Array<{ name: string; enabled: boolean }>> = {
    MCAL: [],
    ECUAL: [],
    Services: [],
    OS: [],
    'BSW Coordination': [],
  }
  for (const mod of Object.values(store.modules)) {
    if (search.value && !mod.name.toLowerCase().includes(search.value.toLowerCase())) {
      continue
    }
    const layer = guessLayer(mod.name)
    if (result[layer]) {
      result[layer].push({ name: mod.name, enabled: mod.enabled })
    }
  }
  for (const key of Object.keys(result)) {
    result[key].sort((a, b) => a.name.localeCompare(b.name))
  }
  return result
})

function guessLayer(name: string): string {
  const mcal = ['Mcu', 'Port', 'Dio', 'Can', 'Spi', 'Adc', 'Pwm', 'Gpt', 'Wdg', 'Icu', 'Lin', 'Eep', 'Fls', 'Eth', 'EthTrcv', 'EthSwt', 'I2c', 'Ocu']
  const ecual = ['CanIf', 'CanTp', 'IoHwAb', 'EthIf', 'MemIf', 'Fee', 'Ea', 'FrIf', 'LinIf', 'PduR', 'EthTSyn']
  if (mcal.includes(name)) return 'MCAL'
  if (ecual.includes(name)) return 'ECUAL'
  if (name === 'Os') return 'OS'
  if (['BswM', 'EcuM', 'Det'].includes(name)) return 'BSW Coordination'
  return 'Services'
}

onMounted(async () => {
  if (!store.config) {
    await store.loadConfig()
  }
})
</script>

<style scoped>
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.layer-card {
  margin-bottom: 20px;
}
.module-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px;
  cursor: pointer;
  border-radius: 4px;
  transition: background 0.2s;
}
.module-item:hover {
  background: #f5f7fa;
}
</style>
