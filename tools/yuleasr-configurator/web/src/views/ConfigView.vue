<template>
  <div class="config-view">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>Configuration Editor</span>
          <el-space>
            <el-button @click="validate">Validate</el-button>
            <el-button type="primary" @click="save">Save</el-button>
          </el-space>
        </div>
      </template>

      <el-tabs v-model="activeTab">
        <el-tab-pane label="Modules" name="modules">
          <el-table :data="moduleList" stripe>
            <el-table-column prop="name" label="Module" width="200" />
            <el-table-column prop="layer" label="Layer" width="120" />
            <el-table-column label="Enabled" width="100">
              <template #default="{ row }">
                <el-switch
                  :model-value="row.enabled"
                  @change="toggleModule(row.name)"
                />
              </template>
            </el-table-column>
            <el-table-column prop="version" label="Version" width="120" />
            <el-table-column label="Actions">
              <template #default="{ row }">
                <el-button
                  size="small"
                  @click="$router.push(`/modules/${row.name}`)"
                >
                  Edit
                </el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>

        <el-tab-pane label="Project" name="project">
          <el-form
            v-if="store.config"
            :model="store.config.project"
            label-width="150px"
          >
            <el-form-item label="Project Name">
              <el-input v-model="store.config.project.name" />
            </el-form-item>
            <el-form-item label="Vendor ID">
              <el-input-number
                v-model="store.config.project.vendor_id"
                :min="0"
                :max="65535"
              />
            </el-form-item>
            <el-form-item label="Description">
              <el-input
                v-model="store.config.project.description"
                type="textarea"
                :rows="3"
              />
            </el-form-item>
          </el-form>
        </el-tab-pane>
      </el-tabs>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useConfigStore } from '@/stores/config'
import { ElMessage } from 'element-plus'

const store = useConfigStore()
const activeTab = ref('modules')

const moduleList = computed(() =>
  Object.values(store.modules).map((m) => ({
    name: m.name,
    enabled: m.enabled,
    version: m.version || '-',
    layer: guessLayer(m.name),
  }))
)

function guessLayer(name: string): string {
  const mcal = ['Mcu', 'Port', 'Dio', 'Can', 'Spi', 'Adc', 'Pwm', 'Gpt', 'Wdg', 'Icu', 'Lin', 'Eep', 'Fls']
  const ecual = ['CanIf', 'CanTp', 'IoHwAb', 'EthIf', 'MemIf', 'Fee', 'Ea', 'FrIf', 'LinIf']
  if (mcal.includes(name)) return 'MCAL'
  if (ecual.includes(name)) return 'ECUAL'
  if (['Com', 'PduR', 'NvM', 'Dcm', 'Dem'].includes(name)) return 'Services'
  if (name === 'Os') return 'OS'
  return 'Integration'
}

function toggleModule(name: string) {
  store.toggleModule(name)
}

async function validate() {
  await store.validateConfig()
  if (store.validation?.valid) {
    ElMessage.success('Configuration is valid')
  } else {
    ElMessage.error(`${store.validation?.errors.length} errors found`)
  }
}

async function save() {
  try {
    await store.saveConfig()
    ElMessage.success('Configuration saved')
  } catch (e: any) {
    ElMessage.error(e.message)
  }
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
</style>
