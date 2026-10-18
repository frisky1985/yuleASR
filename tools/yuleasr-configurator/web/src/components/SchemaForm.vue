<template>
  <div class="schema-form">
    <template v-if="schema.properties">
      <el-form label-width="200px" size="default">
        <template v-for="(propSchema, key) in schema.properties" :key="key">
          <el-form-item
            v-if="key !== 'name' && key !== 'enabled'"
            :label="formatLabel(key as string)"
            :required="schema.required?.includes(key as string)"
          >
            <SchemaField
              :schema="propSchema"
              :model-value="getModelValue(key as string)"
              @update="(val: any) => emit('update', { [key]: val })"
            />
          </el-form-item>
        </template>
      </el-form>
    </template>
    <el-empty v-else description="No schema properties available" />
  </div>
</template>

<script setup lang="ts">
import type { JsonSchema } from '@/types'
import SchemaField from './SchemaField.vue'

const props = defineProps<{
  schema: JsonSchema
  model: Record<string, any> | undefined
}>()

const emit = defineEmits<{
  update: [value: Record<string, any>]
}>()

function getModelValue(key: string): any {
  return props.model?.[key]
}

function formatLabel(key: string): string {
  return key
    .split('_')
    .map((w) => w.charAt(0).toUpperCase() + w.slice(1))
    .join(' ')
}
</script>
