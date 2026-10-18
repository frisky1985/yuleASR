<template>
  <div class="home-view">
    <div class="welcome-section">
      <h2>yuleASR Configurator</h2>
      <p class="subtitle">AUTOSAR BSW Configuration Management Tool</p>
    </div>

    <el-row :gutter="16">
      <el-col :span="8">
        <el-card shadow="hover" class="action-card">
          <div class="action-icon primary">
            <el-icon :size="32"><Plus /></el-icon>
          </div>
          <h3>New Project</h3>
          <p>Create a new AUTOSAR configuration from scratch or from a template</p>
          <el-button type="primary" @click="$router.push('/new-project')">
            Get Started
          </el-button>
        </el-card>
      </el-col>

      <el-col :span="8">
        <el-card shadow="hover" class="action-card">
          <div class="action-icon success">
            <el-icon :size="32"><FolderOpened /></el-icon>
          </div>
          <h3>Open Configuration</h3>
          <p>Load an existing YAML, JSON, or ARXML configuration file</p>
          <el-button @click="handleOpen">
            Browse Files
          </el-button>
        </el-card>
      </el-col>

      <el-col :span="8">
        <el-card shadow="hover" class="action-card">
          <div class="action-icon warning">
            <el-icon :size="32"><Setting /></el-icon>
          </div>
          <h3>Edit Configuration</h3>
          <p>Modify the current configuration with schema validation</p>
          <el-button @click="$router.push('/config')">
            Open Editor
          </el-button>
        </el-card>
      </el-col>
    </el-row>

    <el-row :gutter="16" style="margin-top: 16px">
      <el-col :span="8">
        <el-card shadow="hover" class="action-card small">
          <div class="action-icon info">
            <el-icon :size="24"><Share /></el-icon>
          </div>
          <h3>Dependency Graph</h3>
          <p>Visualize module dependencies and detect circular references</p>
          <el-button size="small" @click="$router.push('/graph')">
            View Graph
          </el-button>
        </el-card>
      </el-col>
      <el-col :span="8">
        <el-card shadow="hover" class="action-card small">
          <div class="action-icon success">
            <el-icon :size="24"><DocumentCopy /></el-icon>
          </div>
          <h3>Configuration Diff</h3>
          <p>Compare configurations and track changes</p>
          <el-button size="small" @click="$router.push('/diff')">
            Compare Configs
          </el-button>
        </el-card>
      </el-col>
      <el-col :span="8">
        <el-card shadow="hover" class="action-card small">
          <div class="action-icon primary">
            <el-icon :size="24"><Upload /></el-icon>
          </div>
          <h3>DBC Import</h3>
          <p>Import CAN database files and auto-map to Com/CanIf/PduR</p>
          <el-button size="small" @click="$router.push('/dbc-import')">
            Import DBC
          </el-button>
        </el-card>
      </el-col>
    </el-row>

    <el-row :gutter="16" style="margin-top: 16px">
      <el-col :span="12">
        <el-card shadow="hover">
          <template #header>
            <div class="card-header">
              <span>Configuration Status</span>
              <el-button type="primary" size="small" text @click="refresh">
                <el-icon><Refresh /></el-icon>
                Refresh
              </el-button>
            </div>
          </template>
          <el-descriptions :column="1" border size="small">
            <el-descriptions-item label="Config File">
              <el-tag size="small">{{ configPath }}</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="Total Modules">
              <el-tag type="info" size="small">{{ store.moduleCount }}</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="Enabled Modules">
              <el-tag type="success" size="small">{{ store.enabledModules.length }}</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="Validation">
              <el-tag v-if="store.validation?.valid" type="success" size="small">
                Valid
              </el-tag>
              <el-tag v-else type="danger" size="small">
                {{ store.validation?.errors.length ?? 0 }} errors
              </el-tag>
            </el-descriptions-item>
          </el-descriptions>
        </el-card>
      </el-col>

      <el-col :span="12">
        <el-card shadow="hover">
          <template #header>Module Layers</template>
          <div class="layer-stats">
            <div v-for="layer in layers" :key="layer.name" class="layer-item">
              <div class="layer-name">{{ layer.name }}</div>
              <div class="layer-count">{{ layer.count }}</div>
              <el-progress :percentage="getLayerPercentage(layer.count)" :show-text="false" :stroke-width="8" />
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <el-row :gutter="16" style="margin-top: 16px" v-if="store.validation">
      <el-col :span="24">
        <el-card shadow="hover">
          <template #header>Validation Results</template>
          <el-alert
            v-if="store.validation.valid"
            title="Configuration is valid"
            type="success"
            show-icon
            :closable="false"
          />
          <div v-else class="validation-errors">
            <el-alert
              v-for="(err, i) in store.validation.errors"
              :key="i"
              :title="err.message"
              type="error"
              :description="err.path"
              show-icon
              style="margin-bottom: 8px"
            />
          </div>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { onMounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useConfigStore } from '@/stores/config'
import { ElMessage } from 'element-plus'
import { Plus, FolderOpened, Setting, Refresh, Share, DocumentCopy, Upload } from '@element-plus/icons-vue'

const router = useRouter()
const store = useConfigStore()

const configPath = computed(() => store.config ? 'config/bsw_config.yaml' : 'Not loaded')

const layers = computed(() => {
  const modules = store.modules
  const layerMap: Record<string, number> = {}
  for (const mod of Object.values(modules)) {
    const layer = guessLayer(mod.name)
    layerMap[layer] = (layerMap[layer] || 0) + 1
  }
  return [
    { name: 'MCAL', count: layerMap['MCAL'] || 0 },
    { name: 'ECUAL', count: layerMap['ECUAL'] || 0 },
    { name: 'Services', count: layerMap['Services'] || 0 },
    { name: 'Other', count: (layerMap['OS'] || 0) + (layerMap['Integration'] || 0) },
  ]
})

function guessLayer(name: string): string {
  const mcal = ['Mcu', 'Port', 'Dio', 'Can', 'Spi', 'Adc', 'Pwm', 'Gpt', 'Wdg', 'Icu', 'Lin', 'Eep', 'Fls']
  const ecual = ['CanIf', 'CanTp', 'IoHwAb', 'EthIf', 'MemIf', 'Fee', 'Ea', 'FrIf', 'LinIf']
  const services = ['Com', 'PduR', 'NvM', 'Dcm', 'Dem']
  if (mcal.includes(name)) return 'MCAL'
  if (ecual.includes(name)) return 'ECUAL'
  if (services.includes(name)) return 'Services'
  if (name === 'Os') return 'OS'
  return 'Integration'
}

function getLayerPercentage(count: number): number {
  const total = store.moduleCount || 1
  return Math.round((count / total) * 100)
}

function handleOpen() {
  if (window.electronAPI?.isElectron) {
    window.electronAPI.showOpenDialog({
      properties: ['openFile'],
      filters: [
        { name: 'Configuration Files', extensions: ['yaml', 'yml', 'json', 'arxml'] },
        { name: 'All Files', extensions: ['*'] },
      ],
    }).then((result: any) => {
      if (!result.canceled && result.filePaths.length > 0) {
        ElMessage.success(`Selected: ${result.filePaths[0]}`)
        router.push('/config')
      }
    })
  } else {
    router.push('/config')
  }
}

async function refresh() {
  try {
    await store.loadConfig()
    ElMessage.success('Configuration reloaded')
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
.home-view {
  padding: 24px;
  max-width: 1400px;
  margin: 0 auto;
}

.welcome-section {
  text-align: center;
  margin-bottom: 32px;
}

.welcome-section h2 {
  font-size: 28px;
  font-weight: 600;
  color: #303133;
  margin: 0 0 8px 0;
}

.subtitle {
  font-size: 14px;
  color: #909399;
  margin: 0;
}

.action-card {
  text-align: center;
  padding: 16px 0;
}

.action-icon {
  width: 64px;
  height: 64px;
  border-radius: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
  margin: 0 auto 16px;
}

.action-icon.primary {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: #fff;
}

.action-icon.success {
  background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);
  color: #fff;
}

.action-icon.warning {
  background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
  color: #fff;
}

.action-card h3 {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
  margin: 0 0 8px 0;
}

.action-card p {
  font-size: 13px;
  color: #909399;
  margin: 0 0 16px 0;
  line-height: 1.5;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.layer-stats {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.layer-item {
  display: grid;
  grid-template-columns: 80px 40px 1fr;
  align-items: center;
  gap: 12px;
}

.layer-name {
  font-size: 13px;
  font-weight: 500;
  color: #606266;
}

.layer-count {
  font-size: 13px;
  font-weight: 600;
  color: #409eff;
  text-align: right;
}

.validation-errors {
  max-height: 200px;
  overflow-y: auto;
}
</style>
