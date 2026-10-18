<template>
  <div class="schema-field">
    <template v-if="schema.type === 'boolean'">
      <el-switch :model-value="modelValue" @update:model-value="emit('update', $event)" />
      <span v-if="schema.description" class="field-desc">{{ schema.description }}</span>
    </template>

    <template v-else-if="schema.type === 'integer' || schema.type === 'number'">
      <el-input-number
        :model-value="modelValue"
        @update:model-value="emit('update', $event)"
        :min="schema.minimum"
        :max="schema.maximum"
        controls-position="right"
      />
      <span v-if="schema.description" class="field-desc">{{ schema.description }}</span>
    </template>

    <template v-else-if="schema.type === 'string'">
      <template v-if="schema.enum">
        <el-select :model-value="modelValue" @update:model-value="emit('update', $event)">
          <el-option v-for="opt in schema.enum" :key="opt" :label="String(opt)" :value="opt" />
        </el-select>
      </template>
      <template v-else-if="schema.pattern">
        <el-input
          :model-value="modelValue"
          @update:model-value="emit('update', $event)"
          :placeholder="`Pattern: ${schema.pattern}`"
        />
      </template>
      <template v-else>
        <el-input
          :model-value="modelValue"
          @update:model-value="emit('update', $event)"
        />
      </template>
      <span v-if="schema.description" class="field-desc">{{ schema.description }}</span>
    </template>

    <template v-else-if="schema.type === 'array'">
      <div class="array-field">
        <div v-for="(item, idx) in (modelValue || [])" :key="idx" class="array-item">
          <SchemaField
            :schema="schema.items || { type: 'string' }"
            :model-value="item"
            @update="(val: any) => updateArray(idx, val)"
          />
          <el-button type="danger" size="small" @click="removeArrayItem(idx)">
            <el-icon><Delete /></el-icon>
          </el-button>
        </div>
        <el-button size="small" @click="addArrayItem">Add Item</el-button>
      </div>
    </template>

    <template v-else-if="schema.type === 'object' && schema.properties">
      <el-collapse>
        <el-collapse-item :title="schema.description || 'Object'">
          <div class="object-field">
            <div v-for="(propSchema, key) in schema.properties" :key="key" class="object-field-item">
              <span class="object-field-label">{{ formatLabel(key as string) }}:</span>
              <SchemaField
                :schema="propSchema"
                :model-value="modelValue?.[key]"
                @update="(val: any) => updateObject(key as string, val)"
              />
            </div>
          </div>
        </el-collapse-item>
      </el-collapse>
    </template>

    <template v-else>
      <el-input
        :model-value="typeof modelValue === 'string' ? modelValue : JSON.stringify(modelValue)"
        @update:model-value="emit('update', $event)"
        disabled
      />
    </template>
  </div>
</template>

<script setup lang="ts">
import type { JsonSchema } from '@/types'

const props = defineProps<{
  schema: JsonSchema
  modelValue: any
}>()

const emit = defineEmits<{
  update: [value: any]
}>()

function formatLabel(key: string): string {
  return key.split('_').map((w) => w.charAt(0).toUpperCase() + w.slice(1)).join(' ')
}

function updateArray(index: number, value: any) {
  const arr = [...(props.modelValue || [])]
  arr[index] = value
  emit('update', arr)
}

function addArrayItem() {
  const arr = [...(props.modelValue || [])]
  const itemType = props.schema.items?.type || 'string'
  arr.push(itemType === 'integer' ? 0 : itemType === 'boolean' ? false : '')
  emit('update', arr)
}

function removeArrayItem(index: number) {
  const arr = [...(props.modelValue || [])]
  arr.splice(index, 1)
  emit('update', arr)
}

function updateObject(key: string, value: any) {
  emit('update', { ...(props.modelValue || {}), [key]: value })
}
</script>

<style scoped>
.field-desc {
  margin-left: 12px;
  color: #909399;
  font-size: 12px;
}
.array-field {
  width: 100%;
}
.array-item {
  display: flex;
  gap: 8px;
  align-items: center;
  margin-bottom: 8px;
}
.object-field-item {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 8px;
}
.object-field-label {
  min-width: 120px;
  font-weight: 500;
}
</style>
