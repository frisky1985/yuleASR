<template>
  <div class="project-wizard">
    <div class="wizard-header">
      <h2>Create New Project</h2>
      <el-steps :active="currentStep" finish-status="success" align-center>
        <el-step title="Project Info" />
        <el-step title="Target Chip" />
        <el-step title="MCAL Modules" />
        <el-step title="Communication" />
        <el-step title="Review" />
      </el-steps>
    </div>

    <div class="wizard-body">
      <div class="wizard-content">
      <!-- Step 1: Project Info -->
      <div v-if="currentStep === 0" class="step-content">
        <el-form :model="projectForm" label-width="120px" style="max-width: 600px">
          <el-form-item label="Project Name" required>
            <el-input v-model="projectForm.name" placeholder="Enter project name" />
          </el-form-item>
          <el-form-item label="Description">
            <el-input
              v-model="projectForm.description"
              type="textarea"
              :rows="3"
              placeholder="Project description"
            />
          </el-form-item>
          <el-form-item label="Author">
            <el-input v-model="projectForm.author" placeholder="Author name" />
          </el-form-item>
          <el-form-item label="Version">
            <el-input v-model="projectForm.version" placeholder="1.0.0" />
          </el-form-item>
        </el-form>
      </div>

      <!-- Step 2: Target Chip -->
      <div v-if="currentStep === 1" class="step-content">
        <el-form label-width="120px" style="max-width: 600px">
          <el-form-item label="Chip Vendor">
            <el-select v-model="projectForm.chipVendor" placeholder="Select vendor" clearable @change="onVendorChange">
              <el-option label="Infineon" value="infineon" />
              <el-option label="NXP" value="nxp" />
              <el-option label="Renesas" value="renesas" />
              <el-option label="STMicroelectronics" value="st" />
              <el-option label="TI" value="ti" />
            </el-select>
          </el-form-item>
          <el-form-item label="Chip Series">
            <el-select v-model="projectForm.chipSeries" placeholder="Select series" clearable :disabled="!projectForm.chipVendor" @change="onSeriesChange">
              <el-option
                v-for="series in availableSeries"
                :key="series.value"
                :label="series.label"
                :value="series.value"
              />
            </el-select>
          </el-form-item>
          <el-form-item label="Chip Model">
            <el-select v-model="projectForm.chipModel" placeholder="Select model" clearable :disabled="!projectForm.chipSeries">
              <el-option
                v-for="model in availableModels"
                :key="model.value"
                :label="model.label"
                :value="model.value"
              />
            </el-select>
          </el-form-item>
          <el-form-item label="Clock Frequency">
            <el-input-number v-model="projectForm.clockFreq" :min="1" :max="500" />
            <span class="unit">MHz</span>
          </el-form-item>
        </el-form>

        <el-alert
          v-if="projectForm.chipModel"
          :title="`Selected: ${getChipFullName()}`"
          type="info"
          :closable="false"
          style="max-width: 600px; margin-top: 20px"
        >
          <template #default>
            <p>MCAL Modules: {{ getMcalModuleCount() }} modules will be auto-configured</p>
          </template>
        </el-alert>
      </div>

      <!-- Step 3: MCAL Modules -->
      <div v-if="currentStep === 2" class="step-content">
        <el-alert
          v-if="!projectForm.chipModel"
          title="Using default BSW module set"
          type="info"
          :closable="false"
          style="margin-bottom: 16px"
        >
          <template #default>
            <span>Select a chip in the previous step to see chip-specific modules, or use the default AUTOSAR BSW set below.</span>
          </template>
        </el-alert>
        <div class="module-selection">
          <el-checkbox-group v-model="selectedMcalModules">
            <div v-for="layer in mcalModuleGroups" :key="layer.name" class="module-group">
              <h4>{{ layer.name }}</h4>
              <div class="module-grid">
                <el-checkbox
                  v-for="mod in layer.modules"
                  :key="mod.name"
                  :label="mod.name"
                  :disabled="mod.required"
                >
                  <div class="module-item">
                    <span>{{ mod.label }}</span>
                    <el-tag v-if="mod.required" type="warning" size="small">Required</el-tag>
                  </div>
                </el-checkbox>
              </div>
            </div>
          </el-checkbox-group>
        </div>
      </div>

      <!-- Step 4: Communication Import -->
      <div v-if="currentStep === 3" class="step-content">
        <el-form label-width="120px" style="max-width: 600px">
          <el-form-item label="CAN Database">
            <el-upload
              :auto-upload="false"
              :on-change="handleDbcUpload"
              accept=".dbc"
              drag
            >
              <el-icon class="el-icon--upload"><UploadFilled /></el-icon>
              <div class="el-upload__text">
                Drop DBC file here or <em>click to upload</em>
              </div>
              <template #tip>
                <div class="el-upload__tip">
                  Supports .dbc files (CAN 2.0A/B, CAN FD)
                </div>
              </template>
            </el-upload>
          </el-form-item>

          <el-form-item label="Network Config">
            <el-upload
              :auto-upload="false"
              :on-change="handleArxmlUpload"
              accept=".arxml"
              drag
            >
              <el-icon class="el-icon--upload"><UploadFilled /></el-icon>
              <div class="el-upload__text">
                Drop ARXML file here or <em>click to upload</em>
              </div>
              <template #tip>
                <div class="el-upload__tip">
                  Supports ARXML 4.0 network description files
                </div>
              </template>
            </el-upload>
          </el-form-item>

          <el-form-item label="NM Matrix">
            <el-upload
              :auto-upload="false"
              :on-change="handleNmUpload"
              accept=".json,.csv"
              drag
            >
              <el-icon class="el-icon--upload"><UploadFilled /></el-icon>
              <div class="el-upload__text">
                Drop NM matrix file here or <em>click to upload</em>
              </div>
              <template #tip>
                <div class="el-upload__tip">
                  Supports JSON or CSV NM matrix files
                </div>
              </template>
            </el-upload>
          </el-form-item>

          <el-divider />

          <el-form-item label="Communication Stack">
            <el-checkbox-group v-model="selectedCommModules">
              <el-checkbox label="CanIf">CAN Interface</el-checkbox>
              <el-checkbox label="CanTp">CAN Transport</el-checkbox>
              <el-checkbox label="PduR">PDU Router</el-checkbox>
              <el-checkbox label="Com">COM Signal</el-checkbox>
              <el-checkbox label="Dcm">Diagnostic</el-checkbox>
              <el-checkbox label="Dem">Event Memory</el-checkbox>
            </el-checkbox-group>
          </el-form-item>
        </el-form>

        <el-alert
          v-if="dbcFileInfo"
          title="DBC File Parsed"
          type="success"
          :closable="false"
          style="max-width: 600px; margin-top: 20px"
        >
          <template #default>
            <p>Messages: {{ dbcFileInfo.messageCount }}</p>
            <p>Signals: {{ dbcFileInfo.signalCount }}</p>
            <p>ECUs: {{ dbcFileInfo.ecuCount }}</p>
          </template>
        </el-alert>

        <el-alert
          v-if="nmFileInfo"
          title="NM Matrix Parsed"
          type="success"
          :closable="false"
          style="max-width: 600px; margin-top: 20px"
        >
          <template #default>
            <p>Clusters: {{ nmFileInfo.clusterCount }}</p>
            <p>Nodes: {{ nmFileInfo.nodeCount }}</p>
            <p>NM Messages: {{ nmFileInfo.messageCount }}</p>
          </template>
        </el-alert>
      </div>

      <!-- Step 5: Review -->
      <div v-if="currentStep === 4" class="step-content">
        <el-descriptions :column="2" border>
          <el-descriptions-item label="Project Name">{{ projectForm.name }}</el-descriptions-item>
          <el-descriptions-item label="Version">{{ projectForm.version }}</el-descriptions-item>
          <el-descriptions-item label="Author">{{ projectForm.author }}</el-descriptions-item>
          <el-descriptions-item label="Chip">{{ getChipFullName() }}</el-descriptions-item>
          <el-descriptions-item label="Clock">{{ projectForm.clockFreq }} MHz</el-descriptions-item>
          <el-descriptions-item label="MCAL Modules">{{ selectedMcalModules.length }}</el-descriptions-item>
          <el-descriptions-item label="DBC File">{{ dbcFileInfo?.fileName || 'None' }}</el-descriptions-item>
          <el-descriptions-item label="Comm Modules">{{ selectedCommModules.length }}</el-descriptions-item>
        </el-descriptions>

        <el-divider />

        <h4>Selected MCAL Modules</h4>
        <el-tag
          v-for="mod in selectedMcalModules"
          :key="mod"
          type="primary"
          style="margin: 4px"
        >
          {{ mod }}
        </el-tag>

        <el-divider />

        <h4>Communication Stack</h4>
        <el-tag
          v-for="mod in selectedCommModules"
          :key="mod"
          type="success"
          style="margin: 4px"
        >
          {{ mod }}
        </el-tag>
      </div>
      </div>
    </div>

    <div class="wizard-footer">
      <el-button v-if="currentStep > 0" @click="prevStep">Previous</el-button>
      <el-button v-if="currentStep < 4" type="primary" @click="nextStep">Next</el-button>
      <el-button v-if="currentStep === 4" type="success" @click="createProject" :loading="creating">
        Create Project
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { UploadFilled } from '@element-plus/icons-vue'
import * as api from '@/api/client'
import { DEFAULT_BSW_MODULES } from '@/utils/defaultModules'
import { useProjectStore } from '@/stores/project'

const router = useRouter()
const projectStore = useProjectStore()
const currentStep = ref(0)
const creating = ref(false)

interface ProjectForm {
  name: string
  description: string
  author: string
  version: string
  chipVendor: string
  chipSeries: string
  chipModel: string
  clockFreq: number
}

const projectForm = ref<ProjectForm>({
  name: '',
  description: '',
  author: '',
  version: '1.0.0',
  chipVendor: '',
  chipSeries: '',
  chipModel: '',
  clockFreq: 160,
})

// Chip database (loaded from API)
const chipVendors = ref<Array<{ value: string; label: string }>>([])
const chipSeriesList = ref<Array<{ value: string; label: string }>>([])
const chipModelsList = ref<Array<{ value: string; label: string; clock_freq_mhz: number; flash_size_kb: number; ram_size_kb: number; module_count: number }>>([])
const mcalModuleList = ref<Array<{ name: string; label: string; required: boolean; default_params: Record<string, any>; dependencies: string[] }>>(
  DEFAULT_BSW_MODULES.map((m) => ({ name: m.name, label: m.label, required: m.required, default_params: {}, dependencies: [] }))
)

// Load vendors on mount
import { onMounted } from 'vue'
onMounted(async () => {
  try {
    chipVendors.value = await api.getChipVendors()
  } catch (e) {
    console.error('Failed to load chip vendors', e)
  }
})

const availableSeries = computed(() => chipSeriesList.value)
const availableModels = computed(() => chipModelsList.value)

watch(() => projectForm.value.chipVendor, async (vendor) => {
  projectForm.value.chipSeries = ''
  projectForm.value.chipModel = ''
  chipSeriesList.value = []
  chipModelsList.value = []
  mcalModuleList.value = []
  if (vendor) {
    try {
      chipSeriesList.value = await api.getChipSeries(vendor)
    } catch (e) {
      console.error('Failed to load chip series', e)
    }
  }
})

watch(() => projectForm.value.chipSeries, async (series) => {
  projectForm.value.chipModel = ''
  chipModelsList.value = []
  mcalModuleList.value = []
  if (projectForm.value.chipVendor && series) {
    try {
      chipModelsList.value = await api.getChipModels(projectForm.value.chipVendor, series)
    } catch (e) {
      console.error('Failed to load chip models', e)
    }
  }
})

watch(() => projectForm.value.chipModel, async (model) => {
  mcalModuleList.value = []
  if (!model || !projectForm.value.chipVendor || !projectForm.value.chipSeries) {
    // Fall back to default BSW modules when no chip is selected
    mcalModuleList.value = DEFAULT_BSW_MODULES.map((m) => ({
      name: m.name,
      label: m.label,
      required: m.required,
      default_params: {},
      dependencies: [],
    }))
    selectedMcalModules.value = DEFAULT_BSW_MODULES.filter((m) => m.required).map((m) => m.name)
    return
  }
  try {
    mcalModuleList.value = await api.getChipMcalModules(
      projectForm.value.chipVendor,
      projectForm.value.chipSeries,
      model
    )
    // Auto-select required modules
    selectedMcalModules.value = mcalModuleList.value
      .filter((m) => m.required)
      .map((m) => m.name)
    // Update clock frequency
    const modelInfo = chipModelsList.value.find((m) => m.value === model)
    if (modelInfo) {
      projectForm.value.clockFreq = modelInfo.clock_freq_mhz
    }
  } catch (e) {
    console.error('Failed to load MCAL modules', e)
  }
})

const mcalModuleGroups = computed(() => {
  const layerOrder = ['MCAL', 'ECUAL', 'Services', 'OS', 'Integration']
  const layerLabels: Record<string, string> = {
    'MCAL': 'MCAL - Microcontroller Abstraction',
    'ECUAL': 'ECUAL - ECU Abstraction Layer',
    'Services': 'Services Layer',
    'OS': 'Operating System',
    'Integration': 'Integration Layer',
  }

  const allModules: Record<string, Array<{ name: string; label: string; required: boolean }>> = {}
  for (const layer of layerOrder) {
    allModules[layer] = []
  }

  for (const mod of mcalModuleList.value) {
    const defaultMod = DEFAULT_BSW_MODULES.find((m) => m.name === mod.name)
    const layer = defaultMod?.layer ?? 'MCAL'
    if (allModules[layer]) {
      allModules[layer].push(mod)
    } else {
      allModules['MCAL'].push(mod)
    }
  }

  return layerOrder
    .filter((layer) => allModules[layer].length > 0)
    .map((layer) => ({
      name: layerLabels[layer] ?? layer,
      modules: allModules[layer],
    }))
})

const selectedMcalModules = ref<string[]>(
  DEFAULT_BSW_MODULES.filter((m) => m.required).map((m) => m.name)
)
const selectedCommModules = ref<string[]>(['CanIf', 'CanTp', 'PduR', 'Com'])

interface DbcFileInfo {
  fileName: string
  messageCount: number
  signalCount: number
  ecuCount: number
}

interface NmFileInfo {
  fileName: string
  clusterCount: number
  nodeCount: number
  messageCount: number
}

const dbcFileInfo = ref<DbcFileInfo | null>(null)
const nmFileInfo = ref<NmFileInfo | null>(null)

function onVendorChange() {
  // Handled by watch
}

function onSeriesChange() {
  // Handled by watch
}

function getChipFullName(): string {
  if (!projectForm.value.chipVendor || !projectForm.value.chipSeries || !projectForm.value.chipModel) {
    return 'Not selected'
  }
  const series = chipSeriesList.value.find((s) => s.value === projectForm.value.chipSeries)
  const model = chipModelsList.value.find((m) => m.value === projectForm.value.chipModel)
  return `${series?.label || ''} - ${model?.label || ''}`
}

function getMcalModuleCount(): number {
  return mcalModuleList.value.length
}

async function handleDbcUpload(file: any) {
  try {
    const content = await file.raw.text()
    const result = await api.parseDbc(content)

    if (result.success) {
      dbcFileInfo.value = {
        fileName: file.name,
        messageCount: result.metadata.message_count,
        signalCount: result.metadata.signal_count,
        ecuCount: result.metadata.node_count,
      }
      ElMessage.success(`DBC parsed: ${result.metadata.message_count} messages, ${result.metadata.signal_count} signals`)
    } else {
      ElMessage.error('Failed to parse DBC file')
    }
  } catch (e: any) {
    ElMessage.error(e.message || 'Failed to parse DBC file')
  }
}

function handleArxmlUpload(_file: any) {
  ElMessage.info('ARXML file uploaded, parsing will be implemented')
}

async function handleNmUpload(file: any) {
  try {
    const content = await file.raw.text()
    const format = file.name.endsWith('.csv') ? 'csv' : 'json'
    const result = await api.parseNmMatrix(content, format)

    if (result.success) {
      nmFileInfo.value = {
        fileName: file.name,
        clusterCount: result.metadata.cluster_count,
        nodeCount: result.metadata.node_count,
        messageCount: result.metadata.message_count,
      }
      ElMessage.success(`NM matrix parsed: ${result.metadata.cluster_count} clusters, ${result.metadata.node_count} nodes`)
    } else {
      ElMessage.error('Failed to parse NM matrix')
    }
  } catch (e: any) {
    ElMessage.error(e.message || 'Failed to parse NM matrix')
  }
}

function nextStep() {
  if (currentStep.value === 0 && !projectForm.value.name) {
    ElMessage.warning('Please enter project name')
    return
  }
  // Step 1 (Target Chip) is optional - user can skip chip selection
  // Step 2 (MCAL) and Step 3 (Communication) have no required validation
  // Just proceed to next step
  if (currentStep.value < 4) {
    currentStep.value++
  }
}

function prevStep() {
  currentStep.value--
}

async function createProject() {
  creating.value = true
  try {
    // Build the module list from wizard selections
    const allModules = [...selectedMcalModules.value, ...selectedCommModules.value]
    const chip = projectForm.value.chipModel
      ? { vendor: projectForm.value.chipVendor, series: projectForm.value.chipSeries, model: projectForm.value.chipModel }
      : undefined

    // Create a new project inside the current workspace
    await projectStore.addProjectToWorkspace(projectForm.value.name, projectForm.value.description, {
      chip,
      mcalModules: selectedMcalModules.value,
      commModules: selectedCommModules.value,
    })

    ElMessage.success(`Project "${projectForm.value.name}" created with ${allModules.length} modules`)
    router.push('/config')
  } catch (e: any) {
    ElMessage.error(e.message || 'Failed to create project')
  } finally {
    creating.value = false
  }
}
</script>

<style scoped>
.project-wizard {
  max-width: 1000px;
  margin: 0 auto;
  display: flex;
  flex-direction: column;
  height: calc(100vh - 120px);
}

.wizard-header {
  background: #fff;
  padding: 30px;
  border-radius: 8px;
  margin-bottom: 20px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
  flex-shrink: 0;
}

.wizard-header h2 {
  margin: 0 0 30px 0;
  color: #303133;
}

.wizard-body {
  flex: 1;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  min-height: 0;
}

.wizard-content {
  background: #fff;
  padding: 30px;
  border-radius: 8px;
  margin-bottom: 20px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
  overflow-y: auto;
  flex: 1;
}

.step-content {
  animation: fadeIn 0.3s;
}

@keyframes fadeIn {
  from { opacity: 0; transform: translateY(10px); }
  to { opacity: 1; transform: translateY(0); }
}

.unit {
  margin-left: 10px;
  color: #909399;
}

.module-selection {
  max-height: 500px;
  overflow-y: auto;
}

.module-group {
  margin-bottom: 24px;
}

.module-group h4 {
  margin: 0 0 12px 0;
  color: #303133;
  font-size: 15px;
  padding-bottom: 8px;
  border-bottom: 1px solid #e4e7ed;
}

.module-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
}

.module-item {
  display: flex;
  align-items: center;
  gap: 8px;
}

.wizard-footer {
  display: flex;
  justify-content: center;
  gap: 12px;
  padding: 20px;
  background: #fff;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
  flex-shrink: 0;
}

:deep(.el-upload-dragger) {
  width: 100%;
  max-width: 360px;
  min-height: 90px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 16px;
}

:deep(.el-form-item__content) {
  display: flex;
  justify-content: flex-start;
}

:deep(.el-upload) {
  width: 100%;
  max-width: 360px;
}

:deep(.el-upload__tip) {
  font-size: 12px;
  color: #909399;
  margin-top: 8px;
}

:deep(.el-icon--upload) {
  font-size: 28px;
  color: #c0c4cc;
  margin-bottom: 6px;
}

:deep(.el-upload__text) {
  font-size: 14px;
  color: #606266;
}

:deep(.el-upload__text em) {
  color: #409eff;
  font-style: normal;
}

:deep(.el-descriptions__label) {
  width: 120px;
}
</style>
