<template>
  <div class="param-table">
    <div class="param-table-toolbar">
      <el-input
        v-model="searchText"
        placeholder="Search parameters..."
        size="small"
        clearable
        style="width: 250px"
      >
        <template #prefix>
          <el-icon><Search /></el-icon>
        </template>
      </el-input>

      <el-select v-model="visibilityLevel" size="small" style="width: 120px">
        <el-option label="Basic" value="basic" />
        <el-option label="Advanced" value="advanced" />
        <el-option label="Expert" value="expert" />
      </el-select>

      <div class="toolbar-spacer" />

      <el-button-group size="small">
        <el-button :type="viewMode === 'table' ? 'primary' : ''" @click="viewMode = 'table'">
          <el-icon><Grid /></el-icon>
        </el-button>
        <el-button :type="viewMode === 'form' ? 'primary' : ''" @click="viewMode = 'form'">
          <el-icon><List /></el-icon>
        </el-button>
      </el-button-group>

      <el-button size="small" @click="expandAll">
        <el-icon><ArrowDown /></el-icon>
      </el-button>
      <el-button size="small" @click="collapseAll">
        <el-icon><ArrowRight /></el-icon>
      </el-button>
    </div>

    <div class="param-table-content">
      <template v-if="viewMode === 'table'">
        <div v-for="group in filteredGroups" :key="group.name" class="param-group">
          <div class="group-header" @click="toggleGroup(group.name)">
            <el-icon class="expand-icon" :class="{ expanded: expandedGroups[group.name] }">
              <ArrowRight />
            </el-icon>
            <span class="group-label">{{ group.label }}</span>
            <el-tag size="small" type="info">{{ group.params.length }}</el-tag>
          </div>

          <el-table
            v-if="expandedGroups[group.name]"
            :data="group.params"
            size="small"
            stripe
            border
            style="width: 100%"
          >
            <el-table-column prop="name" label="Parameter" min-width="200" sortable>
              <template #default="{ row }">
                <div class="param-name-cell">
                  <span class="param-name">{{ row.name }}</span>
                  <el-tag v-if="row.required" type="danger" size="small">Required</el-tag>
                  <el-tooltip v-if="row.description" :content="row.description" placement="top">
                    <el-icon class="info-icon"><InfoFilled /></el-icon>
                  </el-tooltip>
                </div>
              </template>
            </el-table-column>

            <el-table-column label="Value" min-width="250">
              <template #default="{ row }">
                <div class="param-value-cell">
                  <component
                    :is="getEditor(row)"
                    v-model="row.value"
                    @change="onParamChange(row)"
                    :class="{ 'has-error': hasParamError(row.path) }"
                  />
                  <div v-if="hasParamError(row.path)" class="param-errors">
                    <el-text v-for="err in getParamErrors(row.path)" :key="err.message" type="danger" size="small">
                      {{ err.message }}
                    </el-text>
                  </div>
                </div>
              </template>
            </el-table-column>

            <el-table-column prop="type" label="Type" width="90" align="center">
              <template #default="{ row }">
                <el-tag size="small" :type="getTypeTagType(row.type)">
                  {{ row.type }}
                </el-tag>
              </template>
            </el-table-column>

            <el-table-column prop="defaultValue" label="Default" width="100">
              <template #default="{ row }">
                <el-text type="info" size="small">{{ formatValue(row.defaultValue) }}</el-text>
              </template>
            </el-table-column>

            <el-table-column label="Status" width="70" align="center">
              <template #default="{ row }">
                <el-icon v-if="isChanged(row)" color="#67c23a">
                  <Check />
                </el-icon>
                <el-icon v-else color="#c0c4cc">
                  <Minus />
                </el-icon>
              </template>
            </el-table-column>
          </el-table>

          <div v-if="group.childGroups.length > 0 && expandedGroups[group.name]" class="child-groups">
            <param-table
              v-for="child in group.childGroups"
              :key="child.name"
              :groups="[child]"
              :issues="issues"
              @update:param="$emit('update:param', $event)"
            />
          </div>
        </div>
      </template>

      <template v-else>
        <el-form label-width="200px" style="max-width: 800px">
          <template v-for="group in filteredGroups" :key="group.name">
            <div class="form-group-header">{{ group.label }}</div>
            <el-form-item v-for="param in group.params" :key="param.path" :label="param.name">
              <div class="form-item-content">
                <component
                  :is="getEditor(param)"
                  v-model="param.value"
                  @change="onParamChange(param)"
                  style="width: 300px"
                />
                <el-text type="info" size="small" class="param-desc">
                  {{ param.description }}
                </el-text>
              </div>
            </el-form-item>
          </template>
        </el-form>
      </template>

      <el-empty v-if="filteredGroups.length === 0" description="No parameters found" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import {
  Search,
  Grid,
  List,
  ArrowDown,
  ArrowRight,
  Check,
  Minus,
  InfoFilled,
} from '@element-plus/icons-vue'
import { ElInput, ElInputNumber, ElSwitch, ElSelect, ElOption } from 'element-plus'
import type { ParamGroup, ParamMeta } from '@/utils/paramGrouper'
import { filterParamsByVisibility, filterParamsBySearch } from '@/utils/paramGrouper'
import type { ValidationIssue } from '@/utils/localValidator'

const props = defineProps<{
  groups: ParamGroup[]
  issues: ValidationIssue[]
}>()

const emit = defineEmits<{
  'update:param': [param: ParamMeta]
}>()

const searchText = ref('')
const visibilityLevel = ref<'basic' | 'advanced' | 'expert'>('basic')
const viewMode = ref<'table' | 'form'>('table')
const expandedGroups = ref<Record<string, boolean>>({})

const filteredGroups = computed(() => {
  let groups = filterParamsByVisibility(props.groups, visibilityLevel.value)
  if (searchText.value) {
    groups = filterParamsBySearch(groups, searchText.value)
  }
  return groups
})

function getTypeTagType(type: string): 'primary' | 'success' | 'warning' | 'info' | 'danger' {
  const map: Record<string, 'primary' | 'success' | 'warning' | 'info' | 'danger'> = {
    string: 'primary',
    number: 'success',
    integer: 'success',
    boolean: 'warning',
    enum: 'info',
  }
  return map[type] || 'info'
}

function getEditor(param: ParamMeta) {
  if (!param.editable) return 'span'

  if (param.type === 'boolean') return ElSwitch
  if (param.type === 'enum' && param.enum) return ElSelect
  if (param.type === 'number' || param.type === 'integer') return ElInputNumber
  return ElInput
}

function formatValue(value: any): string {
  if (value === undefined || value === null) return '-'
  if (typeof value === 'boolean') return value ? 'true' : 'false'
  return String(value)
}

function isChanged(param: ParamMeta): boolean {
  return param.value !== param.defaultValue
}

function hasParamError(path: string): boolean {
  return props.issues.some((i) => i.path === path && i.severity === 'error')
}

function getParamErrors(path: string): ValidationIssue[] {
  return props.issues.filter((i) => i.path === path)
}

function onParamChange(param: ParamMeta) {
  emit('update:param', param)
}

function toggleGroup(groupName: string) {
  expandedGroups.value[groupName] = !expandedGroups.value[groupName]
}

function expandAll() {
  for (const group of filteredGroups.value) {
    expandedGroups.value[group.name] = true
  }
}

function collapseAll() {
  for (const group of filteredGroups.value) {
    expandedGroups.value[group.name] = false
  }
}
</script>

<style scoped>
.param-table {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.param-table-toolbar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px;
  border-bottom: 1px solid #e4e7ed;
  background: #fafafa;
}

.toolbar-spacer {
  flex: 1;
}

.param-table-content {
  flex: 1;
  overflow-y: auto;
  padding: 10px;
}

.param-group {
  margin-bottom: 16px;
  border: 1px solid #e4e7ed;
  border-radius: 4px;
  background: #fff;
}

.group-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 12px;
  background: #f5f7fa;
  cursor: pointer;
  user-select: none;
  border-bottom: 1px solid #e4e7ed;
}

.group-header:hover {
  background: #ecf5ff;
}

.expand-icon {
  transition: transform 0.2s;
}

.expand-icon.expanded {
  transform: rotate(90deg);
}

.group-label {
  font-weight: 500;
  color: #303133;
  flex: 1;
}

.child-groups {
  padding: 10px;
  background: #fafafa;
  border-top: 1px solid #e4e7ed;
}

.param-name-cell {
  display: flex;
  align-items: center;
  gap: 6px;
}

.param-name {
  font-weight: 500;
}

.info-icon {
  color: #909399;
  cursor: help;
}

.param-value-cell {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.param-errors {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.form-group-header {
  font-weight: 500;
  font-size: 14px;
  color: #303133;
  padding: 10px 0;
  margin-top: 10px;
  border-bottom: 2px solid #409eff;
  margin-bottom: 10px;
}

.form-item-content {
  display: flex;
  align-items: center;
  gap: 12px;
}

.param-desc {
  color: #909399;
}

.has-error {
  border-color: #f56c6c !important;
}

:deep(.el-table) {
  font-size: 13px;
}

:deep(.el-table th) {
  background: #f5f7fa;
  font-weight: 500;
}

:deep(.el-input-number) {
  width: 100%;
}
</style>
