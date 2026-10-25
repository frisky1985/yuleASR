<template>
  <el-dialog
    v-model="visible"
    :title="isEdit ? 'Edit ECU' : 'Add New ECU'"
    :width="isEdit ? '480px' : '720px'"
    @close="handleClose"
    destroy-on-close
  >
    <!-- Edit mode: simple form -->
    <template v-if="isEdit">
      <el-form :model="form" label-width="100px" size="default">
        <el-form-item label="ECU Name" required>
          <el-input v-model="form.name" placeholder="e.g. ECU_Master" />
        </el-form-item>
        <el-form-item label="Description">
          <el-input v-model="form.description" type="textarea" :rows="2" placeholder="Optional description" />
        </el-form-item>
        <el-divider content-position="left">Target Chip</el-divider>
        <el-form-item label="Vendor">
          <el-select v-model="form.vendor" placeholder="Select vendor" clearable style="width: 100%" @change="onVendorChange">
            <el-option v-for="v in vendors" :key="v.value" :label="v.label" :value="v.value" />
          </el-select>
        </el-form-item>
        <el-form-item label="Series">
          <el-select v-model="form.series" placeholder="Select series" clearable style="width: 100%" :disabled="!form.vendor" @change="onSeriesChange">
            <el-option v-for="s in seriesList" :key="s.value" :label="s.label" :value="s.value" />
          </el-select>
        </el-form-item>
        <el-form-item label="Model">
          <el-select v-model="form.model" placeholder="Select model" clearable style="width: 100%" :disabled="!form.series">
            <el-option v-for="m in modelList" :key="m.value" :label="m.label" :value="m.value" />
          </el-select>
        </el-form-item>
      </el-form>
    </template>

    <!-- Add mode: wizard flow -->
    <template v-else>
      <div class="ecu-wizard">
        <el-steps :active="currentStep" finish-status="success" align-center style="margin-bottom: 24px">
          <el-step title="ECU Info" />
          <el-step title="Target Chip" />
          <el-step title="BSW Modules" />
          <el-step title="Review" />
        </el-steps>

        <div class="wizard-body">
          <!-- Step 0: ECU Info -->
          <div v-if="currentStep === 0" class="step-content">
            <el-form :model="form" label-width="110px" style="max-width: 500px; margin: 0 auto">
              <el-form-item label="ECU Name" required>
                <el-input v-model="form.name" placeholder="e.g. ECU_Engine" autofocus />
              </el-form-item>
              <el-form-item label="Description">
                <el-input v-model="form.description" type="textarea" :rows="3" placeholder="Describe the purpose of this ECU" />
              </el-form-item>
            </el-form>
          </div>

          <!-- Step 1: Target Chip -->
          <div v-if="currentStep === 1" class="step-content">
            <el-form label-width="110px" style="max-width: 500px; margin: 0 auto">
              <el-form-item label="Vendor" required>
                <el-select v-model="form.vendor" placeholder="Select chip vendor" clearable style="width: 100%" @change="onVendorChange">
                  <el-option v-for="v in vendors" :key="v.value" :label="v.label" :value="v.value" />
                </el-select>
              </el-form-item>
              <el-form-item label="Series" required>
                <el-select v-model="form.series" placeholder="Select chip series" clearable style="width: 100%" :disabled="!form.vendor" @change="onSeriesChange">
                  <el-option v-for="s in seriesList" :key="s.value" :label="s.label" :value="s.value" />
                </el-select>
              </el-form-item>
              <el-form-item label="Model" required>
                <el-select v-model="form.model" placeholder="Select chip model" clearable style="width: 100%" :disabled="!form.series">
                  <el-option v-for="m in modelList" :key="m.value" :label="m.label" :value="m.value" />
                </el-select>
              </el-form-item>
            </el-form>
            <el-alert
              v-if="form.model"
              :title="`Chip selected: ${form.vendor} / ${form.series} / ${form.model}`"
              type="info"
              :closable="false"
              style="max-width: 500px; margin: 16px auto 0"
            >
              <template #default>
                <span>{{ availableModules.length }} BSW modules available (chip MCAL drivers + upper layers)</span>
              </template>
            </el-alert>
          </div>

          <!-- Step 2: MCAL Modules -->
          <div v-if="currentStep === 2" class="step-content">
            <div v-if="availableModules.length === 0" style="text-align: center; padding: 40px 0">
              <el-empty description="No modules available" :image-size="80" />
            </div>
            <div v-else class="module-selection">
              <el-alert
                v-if="!form.model"
                title="Using default BSW module set"
                type="info"
                :closable="false"
                style="margin-bottom: 12px"
              >
                <template #default>
                  <span>Select a chip in the previous step to see chip-specific modules, or use the default AUTOSAR BSW set below.</span>
                </template>
              </el-alert>
              <div style="margin-bottom: 12px; display: flex; justify-content: space-between; align-items: center">
                <el-text type="info" size="small">{{ selectedModules.length }} / {{ availableModules.length }} selected</el-text>
                <div style="display: flex; gap: 8px">
                  <el-button size="small" text @click="selectAll">Select All</el-button>
                  <el-button size="small" text @click="selectRequiredOnly">Required Only</el-button>
                </div>
              </div>
              <el-checkbox-group v-model="selectedModules">
                <div v-for="group in moduleGroups" :key="group.name" class="module-group">
                  <h4>{{ group.name }} ({{ group.modules.length }})</h4>
                  <div class="module-grid">
                    <el-checkbox
                      v-for="mod in group.modules"
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

          <!-- Step 3: Review -->
          <div v-if="currentStep === 3" class="step-content">
            <el-descriptions :column="2" border style="max-width: 560px; margin: 0 auto">
              <el-descriptions-item label="ECU Name">{{ form.name }}</el-descriptions-item>
              <el-descriptions-item label="Description">{{ form.description || '-' }}</el-descriptions-item>
              <el-descriptions-item label="Chip Vendor">{{ form.vendor || '-' }}</el-descriptions-item>
              <el-descriptions-item label="Chip Series">{{ form.series || '-' }}</el-descriptions-item>
              <el-descriptions-item label="Chip Model">{{ form.model || '-' }}</el-descriptions-item>
              <el-descriptions-item label="BSW Modules">{{ selectedModules.length }}</el-descriptions-item>
            </el-descriptions>

            <el-divider />

            <h4 style="margin: 0 0 12px">Selected BSW Modules</h4>
            <div v-if="selectedModules.length === 0">
              <el-text type="info">No modules selected</el-text>
            </div>
            <div v-else>
              <el-tag
                v-for="mod in selectedModules"
                :key="mod"
                type="primary"
                style="margin: 3px"
              >
                {{ mod }}
              </el-tag>
            </div>
          </div>
        </div>
      </div>
    </template>

    <template #footer>
      <template v-if="isEdit">
        <el-button @click="handleClose">Cancel</el-button>
        <el-button type="primary" @click="handleConfirm" :disabled="!form.name">Save</el-button>
      </template>
      <template v-else>
        <el-button @click="handleClose">Cancel</el-button>
        <el-button v-if="currentStep > 0" @click="currentStep--">Previous</el-button>
        <el-button v-if="currentStep < 3" type="primary" @click="nextStep">Next</el-button>
        <el-button v-if="currentStep === 3" type="success" @click="handleConfirm" :disabled="!form.name">
          Add ECU
        </el-button>
      </template>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { ElMessage } from 'element-plus'
import * as api from '@/api/client'
import { DEFAULT_BSW_MODULES, buildBswModuleOptions, defaultBswModuleOptions } from '@/utils/defaultModules'
import { MODULE_DEPENDENCIES } from '@/stores/project'

interface EcuForm {
  name: string
  description: string
  vendor: string
  series: string
  model: string
}

const props = defineProps<{
  modelValue: boolean
  editData?: { id: string; name: string; description?: string; chip?: { vendor: string; series: string; model: string } }
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  confirm: [data: { name: string; description: string; chip?: { vendor: string; series: string; model: string }; selectedModules?: string[] }]
}>()

const visible = computed({
  get: () => props.modelValue,
  set: (v) => emit('update:modelValue', v),
})

const isEdit = computed(() => !!props.editData)
const currentStep = ref(0)

const form = ref<EcuForm>({
  name: '',
  description: '',
  vendor: '',
  series: '',
  model: '',
})

const vendors = ref<Array<{ value: string; label: string }>>([])
const seriesList = ref<Array<{ value: string; label: string }>>([])
const modelList = ref<Array<{ value: string; label: string }>>([])
const availableModules = ref<Array<{ name: string; label: string; required: boolean }>>([])
const selectedModules = ref<string[]>([])

watch(() => props.modelValue, (v) => {
  if (v) {
    if (props.editData) {
      form.value = {
        name: props.editData.name,
        description: props.editData.description ?? '',
        vendor: props.editData.chip?.vendor ?? '',
        series: props.editData.chip?.series ?? '',
        model: props.editData.chip?.model ?? '',
      }
      if (form.value.vendor) onVendorChange(form.value.vendor)
      if (form.value.series) onSeriesChange(form.value.series)
    } else {
      form.value = { name: '', description: '', vendor: '', series: '', model: '' }
      selectedModules.value = []
      currentStep.value = 0
    }
    loadVendors()
  }
})

async function loadVendors() {
  try {
    vendors.value = await api.getChipVendors()
  } catch {
    vendors.value = ['NXP', 'Infineon', 'Renesas', 'STMicroelectronics', 'Texas Instruments'].map((v) => ({ value: v, label: v }))
  }
}

async function onVendorChange(vendor: string) {
  form.value.series = ''
  form.value.model = ''
  seriesList.value = []
  modelList.value = []
  availableModules.value = []
  selectedModules.value = []
  if (!vendor) return
  try {
    seriesList.value = await api.getChipSeries(vendor)
  } catch {
    seriesList.value = []
  }
}

async function onSeriesChange(series: string) {
  form.value.model = ''
  modelList.value = []
  availableModules.value = []
  selectedModules.value = []
  if (!series) return
  try {
    modelList.value = await api.getChipModels(form.value.vendor, series)
  } catch {
    modelList.value = []
  }
}

watch(() => form.value.model, async (model) => {
  availableModules.value = []
  selectedModules.value = []
  if (!model || !form.value.vendor || !form.value.series) {
    // Fall back to default BSW modules when no chip is selected
    availableModules.value = defaultBswModuleOptions()
    selectedModules.value = DEFAULT_BSW_MODULES.filter((m) => m.required).map((m) => m.name)
    return
  }
  try {
    // Chip-specific MCAL drivers + all chip-independent upper layers
    const chipModules = await api.getChipMcalModules(form.value.vendor, form.value.series, model)
    availableModules.value = buildBswModuleOptions(chipModules)
    selectedModules.value = availableModules.value.filter((m) => m.required).map((m) => m.name)
  } catch {
    // Chip API unavailable: fall back to the full default BSW set
    availableModules.value = defaultBswModuleOptions()
    selectedModules.value = DEFAULT_BSW_MODULES.filter((m) => m.required).map((m) => m.name)
  }
})

// Auto-select transitive dependencies of checked modules
watch(selectedModules, (mods) => {
  const selected = new Set(mods)
  const queue = [...mods]
  const added: string[] = []
  while (queue.length) {
    const cur = queue.shift()!
    for (const dep of MODULE_DEPENDENCIES[cur] || []) {
      if (!selected.has(dep) && availableModules.value.some((m) => m.name === dep)) {
        selected.add(dep)
        added.push(dep)
        queue.push(dep)
      }
    }
  }
  if (added.length) selectedModules.value = [...mods, ...added]
})

const moduleGroups = computed(() => {
  const layerOrder = ['MCAL', 'ECUAL', 'Services', 'OS', 'RTE', 'Integration']
  const layerLabels: Record<string, string> = {
    'MCAL': 'MCAL - Microcontroller Abstraction',
    'ECUAL': 'ECUAL - ECU Abstraction Layer',
    'Services': 'Services Layer',
    'OS': 'Operating System',
    'RTE': 'RTE - Runtime Environment',
    'Integration': 'Integration Layer',
  }

  const groups: Record<string, typeof availableModules.value> = {}
  for (const layer of layerOrder) {
    groups[layer] = []
  }

  for (const mod of availableModules.value) {
    // Find the layer for this module from DEFAULT_BSW_MODULES
    const defaultMod = DEFAULT_BSW_MODULES.find((m) => m.name === mod.name)
    const layer = defaultMod?.layer ?? 'MCAL'
    if (groups[layer]) {
      groups[layer].push(mod)
    } else {
      groups['MCAL'].push(mod)
    }
  }

  return layerOrder
    .filter((layer) => groups[layer].length > 0)
    .map((layer) => ({
      name: layerLabels[layer] ?? layer,
      modules: groups[layer],
    }))
})

function selectAll() {
  selectedModules.value = availableModules.value.map((m) => m.name)
}

function selectRequiredOnly() {
  selectedModules.value = availableModules.value.filter((m) => m.required).map((m) => m.name)
}

function nextStep() {
  if (currentStep.value === 0 && !form.value.name.trim()) {
    ElMessage.warning('Please enter ECU name')
    return
  }
  // Chip selection is now optional - default BSW modules will be used
  currentStep.value++
}

function handleClose() {
  visible.value = false
}

function handleConfirm() {
  const chip = form.value.vendor
    ? { vendor: form.value.vendor, series: form.value.series, model: form.value.model }
    : undefined
  emit('confirm', {
    name: form.value.name,
    description: form.value.description,
    chip,
    selectedModules: selectedModules.value,
  })
  visible.value = false
}
</script>

<style scoped>
.ecu-wizard {
  min-height: 360px;
}

.wizard-body {
  min-height: 300px;
}

.step-content {
  animation: fadeIn 0.3s;
}

@keyframes fadeIn {
  from { opacity: 0; transform: translateY(8px); }
  to { opacity: 1; transform: translateY(0); }
}

.module-selection {
  max-height: 360px;
  overflow-y: auto;
  padding-right: 4px;
}

.module-group {
  margin-bottom: 16px;
}

.module-group h4 {
  margin: 0 0 8px 0;
  color: #303133;
  font-size: 14px;
  padding-bottom: 6px;
  border-bottom: 1px solid #e4e7ed;
}

.module-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
}

.module-item {
  display: flex;
  align-items: center;
  gap: 6px;
}
</style>
