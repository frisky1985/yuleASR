<template>
  <div class="ref-autocomplete">
    <el-autocomplete
      :model-value="modelValue"
      :fetch-suggestions="fetchSuggestions"
      :placeholder="placeholder"
      clearable
      style="width: 100%"
      @select="onSelect"
      @input="onInput"
    >
      <template #default="{ item }">
        <div class="ref-option">
          <span class="ref-option-label">{{ item.label }}</span>
          <el-tag size="small" type="info">{{ item.value }}</el-tag>
        </div>
      </template>
      <template #prefix>
        <el-icon class="ref-icon"><Link /></el-icon>
      </template>
    </el-autocomplete>
    <el-tag v-if="targetModule" size="small" type="warning" class="ref-target-tag">
      → {{ targetModule }}
    </el-tag>
  </div>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import { Link } from '@element-plus/icons-vue'
import * as api from '@/api/client'

const props = defineProps<{
  modelValue: any
  sourceModule: string
  sourceField: string
  targetModule: string
  targetField: string
}>()

const emit = defineEmits<{
  'update:modelValue': [value: any]
}>()

const suggestions = ref<Array<{ label: string; value: string | number }>>([])
const loading = ref(false)

const placeholder = `Reference to ${props.targetModule}.${props.targetField}`

let debounceTimer: ReturnType<typeof setTimeout> | null = null

async function fetchSuggestions(query: string, cb: (items: any[]) => void) {
  if (debounceTimer) clearTimeout(debounceTimer)
  debounceTimer = setTimeout(async () => {
    loading.value = true
    try {
      const result = await api.getReferenceTargets(props.sourceModule, props.sourceField, query)
      suggestions.value = result.targets
      cb(result.targets)
    } catch {
      cb([])
    } finally {
      loading.value = false
    }
  }, 200)
}

function onSelect(item: { label: string; value: string | number }) {
  emit('update:modelValue', item.value)
}

function onInput(value: string) {
  emit('update:modelValue', value)
}

watch(() => props.modelValue, (val) => {
  if (val === undefined || val === null || val === '') {
    // Fetch all targets when empty
    fetchSuggestions('', (items) => { suggestions.value = items })
  }
}, { immediate: true })
</script>

<style scoped>
.ref-autocomplete {
  display: flex;
  align-items: center;
  gap: 6px;
}

.ref-icon {
  color: #e6a23c;
  font-size: 14px;
}

.ref-option {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
}

.ref-option-label {
  flex: 1;
  font-size: 13px;
}

.ref-target-tag {
  flex-shrink: 0;
}
</style>
