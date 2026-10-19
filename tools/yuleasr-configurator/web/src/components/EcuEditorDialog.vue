<template>
  <el-dialog
    v-model="visible"
    :title="isEdit ? 'Edit ECU' : 'Add ECU'"
    width="480px"
    @close="handleClose"
  >
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

      <el-form-item v-if="form.model" label="MCAL Modules">
        <el-tag v-for="mod in mcalModules" :key="mod.name" size="small" style="margin: 2px">{{ mod.label }}</el-tag>
        <el-text v-if="mcalModules.length === 0" type="info" size="small">Select a model to see available MCAL modules</el-text>
      </el-form-item>
    </el-form>

    <template #footer>
      <el-button @click="handleClose">Cancel</el-button>
      <el-button type="primary" @click="handleConfirm" :disabled="!form.name">
        {{ isEdit ? 'Save' : 'Add' }}
      </el-button>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import * as api from '@/api/client'

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
  confirm: [data: { name: string; description: string; chip?: { vendor: string; series: string; model: string } }]
}>()

const visible = computed({
  get: () => props.modelValue,
  set: (v) => emit('update:modelValue', v),
})

const isEdit = computed(() => !!props.editData)

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
const mcalModules = ref<Array<{ name: string; label: string; required: boolean }>>([])

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
  mcalModules.value = []
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
  mcalModules.value = []
  if (!series) return
  try {
    modelList.value = await api.getChipModels(form.value.vendor, series)
  } catch {
    modelList.value = []
  }
}

watch(() => form.value.model, async (model) => {
  mcalModules.value = []
  if (!model || !form.value.vendor || !form.value.series) return
  try {
    mcalModules.value = await api.getChipMcalModules(form.value.vendor, form.value.series, model)
  } catch {
    mcalModules.value = []
  }
})

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
  })
  visible.value = false
}
</script>
