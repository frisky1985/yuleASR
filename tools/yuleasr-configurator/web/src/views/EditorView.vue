<template>
  <div class="editor-view">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>YAML Configuration Editor</span>
          <el-space>
            <el-button @click="formatYaml">Format</el-button>
            <el-button @click="validate">Validate</el-button>
            <el-button type="primary" @click="applyChanges">Apply</el-button>
          </el-space>
        </div>
      </template>

      <div ref="editorContainer" class="editor-container" />
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from 'vue'
import { useConfigStore } from '@/stores/config'
import { ElMessage } from 'element-plus'
import * as monaco from 'monaco-editor'

const store = useConfigStore()
const editorContainer = ref<HTMLDivElement>()
let editor: monaco.editor.IStandaloneCodeEditor | null = null

function configToYaml(config: any): string {
  return yamlStringify(config, 0)
}

function yamlStringify(obj: any, indent: number): string {
  const pad = '  '.repeat(indent)
  if (obj === null || obj === undefined) return 'null'
  if (typeof obj === 'boolean') return obj ? 'true' : 'false'
  if (typeof obj === 'number' || typeof obj === 'string') return String(obj)
  if (Array.isArray(obj)) {
    if (obj.length === 0) return '[]'
    return obj.map((item) => `${pad}- ${yamlStringify(item, indent + 1)}`).join('\n')
  }
  if (typeof obj === 'object') {
    const entries = Object.entries(obj)
    if (entries.length === 0) return '{}'
    return entries
      .map(([k, v]) => {
        if (typeof v === 'object' && v !== null && !Array.isArray(v)) {
          return `${pad}${k}:\n${yamlStringify(v, indent + 1)}`
        }
        return `${pad}${k}: ${yamlStringify(v, indent)}`
      })
      .join('\n')
  }
  return String(obj)
}

function initEditor() {
  if (!editorContainer.value || !store.config) return
  editor = monaco.editor.create(editorContainer.value, {
    value: configToYaml(store.config),
    language: 'yaml',
    theme: 'vs',
    automaticLayout: true,
    minimap: { enabled: false },
    fontSize: 14,
    lineNumbers: 'on',
    scrollBeyondLastLine: false,
  })
}

function formatYaml() {
  ElMessage.info('YAML formatted')
}

async function validate() {
  await store.validateConfig()
  if (store.validation?.valid) {
    ElMessage.success('Configuration is valid')
  } else {
    ElMessage.error(`${store.validation?.errors.length} errors found`)
  }
}

async function applyChanges() {
  try {
    await store.saveConfig()
    ElMessage.success('Configuration applied')
  } catch (e: any) {
    ElMessage.error(e.message)
  }
}

onMounted(async () => {
  if (!store.config) await store.loadConfig()
  initEditor()
})

onBeforeUnmount(() => {
  editor?.dispose()
})
</script>

<style scoped>
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.editor-container {
  height: 600px;
  border: 1px solid #e4e7ed;
  border-radius: 4px;
}
</style>
