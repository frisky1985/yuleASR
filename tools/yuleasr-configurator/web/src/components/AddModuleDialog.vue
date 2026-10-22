<template>
  <el-dialog
    v-model="visible"
    :title="`Add Module to ${ecu?.name || 'ECU'}`"
    width="600px"
    @close="handleClose"
    destroy-on-close
  >
    <div class="add-module-dialog">
      <div class="layer-filter" v-if="!layer">
        <el-form-item label="Layer">
          <el-select v-model="selectedLayer" placeholder="Select layer" style="width: 100%">
            <el-option v-for="l in layers" :key="l" :label="l" :value="l" />
          </el-select>
        </el-form-item>
      </div>

      <div v-if="availableModules.length === 0" style="text-align: center; padding: 40px 0">
        <el-empty description="No modules available" :image-size="80" />
      </div>
      <div v-else>
        <div class="module-header">
          <el-text type="info" size="small">{{ selectedModules.length }} / {{ availableModules.length }} selected</el-text>
          <div class="module-actions">
            <el-button size="small" text @click="selectAll">Select All</el-button>
            <el-button size="small" text @click="selectNone">Clear</el-button>
          </div>
        </div>

        <div class="module-list">
          <el-checkbox-group v-model="selectedModules">
            <div v-for="mod in availableModules" :key="mod.name" class="module-item">
              <el-checkbox :label="mod.name" :disabled="isExisting(mod.name)">
                <div class="module-info">
                  <span class="module-name">{{ mod.label }}</span>
                  <el-tag v-if="isExisting(mod.name)" type="info" size="small">Already added</el-tag>
                  <el-tag v-else-if="mod.required" type="warning" size="small">Required</el-tag>
                </div>
              </el-checkbox>
            </div>
          </el-checkbox-group>
        </div>
      </div>
    </div>

    <template #footer>
      <el-button @click="handleClose">Cancel</el-button>
      <el-button type="primary" @click="handleConfirm" :disabled="selectedModules.length === 0">
        Add {{ selectedModules.length }} Module{{ selectedModules.length !== 1 ? 's' : '' }}
      </el-button>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { ElMessage } from 'element-plus'
import type { EcuConfig } from '@/types'
import { DEFAULT_BSW_MODULES } from '@/utils/defaultModules'

const props = defineProps<{
  modelValue: boolean
  ecu: EcuConfig | null
  layer?: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  confirm: [modules: string[]]
}>()

const visible = computed({
  get: () => props.modelValue,
  set: (v) => emit('update:modelValue', v),
})

const selectedLayer = ref(props.layer || 'MCAL')
const selectedModules = ref<string[]>([])

const layers = ['MCAL', 'ECUAL', 'Services', 'OS', 'RTE', 'Integration']

const availableModules = computed(() => {
  const layer = selectedLayer.value || props.layer
  const modules = DEFAULT_BSW_MODULES.filter((m) => m.layer === layer)
  return modules.map((m) => ({
    name: m.name,
    label: m.label,
    required: m.required,
  }))
})

function isExisting(moduleName: string): boolean {
  return props.ecu?.modules[moduleName] != null
}

function selectAll() {
  selectedModules.value = availableModules.value
    .filter((m) => !isExisting(m.name))
    .map((m) => m.name)
}

function selectNone() {
  selectedModules.value = []
}

function handleClose() {
  visible.value = false
}

function handleConfirm() {
  emit('confirm', selectedModules.value)
  visible.value = false
}

watch(() => props.modelValue, (v) => {
  if (v) {
    selectedLayer.value = props.layer || 'MCAL'
    selectedModules.value = []
  }
})
</script>

<style scoped>
.add-module-dialog {
  min-height: 300px;
}

.layer-filter {
  margin-bottom: 16px;
}

.module-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
  padding-bottom: 8px;
  border-bottom: 1px solid #e4e7ed;
}

.module-actions {
  display: flex;
  gap: 8px;
}

.module-list {
  max-height: 360px;
  overflow-y: auto;
  padding-right: 4px;
}

.module-item {
  padding: 8px 0;
  border-bottom: 1px solid #f0f2f5;
}

.module-item:last-child {
  border-bottom: none;
}

.module-info {
  display: flex;
  align-items: center;
  gap: 8px;
}

.module-name {
  font-size: 13px;
  color: #303133;
}
</style>
