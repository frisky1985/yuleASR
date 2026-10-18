<template>
  <div class="module-detail-view">
    <el-page-header @back="$router.push('/modules')">
      <template #content>
        <span class="page-title">{{ moduleName }}</span>
        <el-tag v-if="moduleConfig" :type="moduleConfig.enabled ? 'success' : 'info'" style="margin-left: 12px">
          {{ moduleConfig.enabled ? 'Enabled' : 'Disabled' }}
        </el-tag>
      </template>
    </el-page-header>

    <el-card v-if="schema" style="margin-top: 20px">
      <template #header>
        <div class="card-header">
          <span>{{ schema.title || moduleName }} Configuration</span>
          <el-space>
            <el-button @click="resetDefaults">Reset to Defaults</el-button>
            <el-button type="primary" @click="save">Save Changes</el-button>
          </el-space>
        </div>
      </template>

      <SchemaForm
        :schema="schema"
        :model="moduleConfig"
        @update="handleUpdate"
      />
    </el-card>

    <el-card v-if="dependencies.length > 0" style="margin-top: 20px">
      <template #header>Dependencies</template>
      <el-space wrap>
        <el-tag
          v-for="dep in dependencies"
          :key="dep"
          @click="$router.push(`/modules/${dep}`)"
          style="cursor: pointer"
        >
          {{ dep }}
        </el-tag>
      </el-space>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from 'vue'
import { useRoute } from 'vue-router'
import { useConfigStore } from '@/stores/config'
import { getModuleSchema } from '@/api/client'
import type { JsonSchema } from '@/types'
import SchemaForm from '@/components/SchemaForm.vue'
import { ElMessage } from 'element-plus'

const props = defineProps<{ name: string }>()
const route = useRoute()
const store = useConfigStore()

const moduleName = computed(() => props.name || (route.params.name as string))
const moduleConfig = computed(() => store.modules[moduleName.value])
const schema = ref<JsonSchema | null>(null)

const dependencyMap: Record<string, string[]> = {
  CanIf: ['Can'], CanTp: ['CanIf', 'Can'], PduR: ['CanIf', 'LinIf'],
  Com: ['PduR'], NvM: ['MemIf', 'Fee', 'Ea'], Dcm: ['PduR', 'Com'],
  Dem: ['NvM'], BswM: ['EcuM'], EcuM: ['Mcu'],
}
const dependencies = computed(() => dependencyMap[moduleName.value] || [])

async function loadSchema() {
  try {
    schema.value = await getModuleSchema(moduleName.value)
  } catch {
    schema.value = null
  }
}

function handleUpdate(updates: Record<string, any>) {
  store.updateModule(moduleName.value, updates)
}

function resetDefaults() {
  if (!schema.value?.properties || !moduleConfig.value) return
  const defaults: Record<string, any> = { name: moduleName.value, enabled: moduleConfig.value.enabled }
  for (const [key, prop] of Object.entries(schema.value.properties)) {
    if (prop.default !== undefined) defaults[key] = prop.default
  }
  store.updateModule(moduleName.value, defaults)
  ElMessage.success('Reset to defaults')
}

async function save() {
  try {
    await store.saveConfig()
    ElMessage.success('Configuration saved')
  } catch (e: any) {
    ElMessage.error(e.message)
  }
}

watch(moduleName, loadSchema)

onMounted(async () => {
  if (!store.config) await store.loadConfig()
  await loadSchema()
})
</script>

<style scoped>
.page-title { font-size: 20px; font-weight: 600; }
.card-header { display: flex; justify-content: space-between; align-items: center; }
</style>
