<template>
  <div class="code-preview">
    <div class="preview-toolbar">
      <el-select v-model="selectedModule" size="small" placeholder="Select module" style="width: 150px">
        <el-option
          v-for="mod in enabledModules"
          :key="mod"
          :label="mod"
          :value="mod"
        />
      </el-select>

      <el-select v-model="selectedFile" size="small" style="width: 150px">
        <el-option label="Header (.h)" value="header" />
        <el-option label="Source (.c)" value="source" />
        <el-option label="Config (.cfg)" value="config" />
      </el-select>

      <div class="toolbar-spacer" />

      <el-button size="small" @click="refreshPreview">
        <el-icon><Refresh /></el-icon>
        Refresh
      </el-button>

      <el-button size="small" @click="copyCode">
        <el-icon><DocumentCopy /></el-icon>
        Copy
      </el-button>
    </div>

    <div ref="editorContainer" class="editor-container"></div>

    <div class="preview-footer">
      <el-text size="small" type="info">
        Lines: {{ lineCount }} | Size: {{ codeSize }}
      </el-text>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue'
import { Refresh, DocumentCopy } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import * as monaco from 'monaco-editor'
import { useConfigStore } from '@/stores/config'

const store = useConfigStore()

const editorContainer = ref<HTMLElement>()
const selectedModule = ref('')
const selectedFile = ref<'header' | 'source' | 'config'>('header')

let editor: monaco.editor.IStandaloneCodeEditor | null = null

const enabledModules = computed(() => {
  return Object.keys(store.modules).filter((name) => store.modules[name].enabled)
})

const previewCode = computed(() => {
  if (!selectedModule.value || !store.config) return '// No module selected'

  const mod = store.modules[selectedModule.value]
  if (!mod) return '// Module not found'

  if (selectedFile.value === 'header') {
    return generateHeader(selectedModule.value, mod)
  } else if (selectedFile.value === 'source') {
    return generateSource(selectedModule.value, mod)
  } else {
    return generateConfig(selectedModule.value, mod)
  }
})

const lineCount = computed(() => {
  return previewCode.value.split('\n').length
})

const codeSize = computed(() => {
  const bytes = new Blob([previewCode.value]).size
  if (bytes < 1024) return `${bytes} B`
  return `${(bytes / 1024).toFixed(1)} KB`
})

function generateHeader(moduleName: string, config: any): string {
  const guard = `${moduleName.toUpperCase()}_H`
  const lines = [
    `/**`,
    ` * @file ${moduleName}.h`,
    ` * @brief AUTOSAR ${moduleName} Module Configuration`,
    ` * @generated yuleASR Configurator`,
    ` */`,
    ``,
    `#ifndef ${guard}`,
    `#define ${guard}`,
    ``,
    `#include "Std_Types.h"`,
    ``,
  ]

  if (config.general) {
    lines.push(`/* General Configuration */`)
    for (const [key, value] of Object.entries(config.general)) {
      const macro = `${moduleName.toUpperCase()}_${key.toUpperCase()}`
      if (typeof value === 'boolean') {
        lines.push(`#define ${macro} ${value ? 'STD_ON' : 'STD_OFF'}`)
      } else if (typeof value === 'number') {
        lines.push(`#define ${macro} ${value}`)
      }
    }
    lines.push(``)
  }

  lines.push(`/* API Functions */`)
  lines.push(`void ${moduleName}_Init(void);`)
  lines.push(`void ${moduleName}_Deinit(void);`)
  lines.push(``)
  lines.push(`#endif /* ${guard} */`)

  return lines.join('\n')
}

function generateSource(moduleName: string, config: any): string {
  const lines = [
    `/**`,
    ` * @file ${moduleName}.c`,
    ` * @brief AUTOSAR ${moduleName} Module Implementation`,
    ` * @generated yuleASR Configurator`,
    ` */`,
    ``,
    `#include "${moduleName}.h"`,
    ``,
    `/* Module Configuration */`,
  ]

  if (config.general) {
    lines.push(`static const struct {`)
    for (const [key, value] of Object.entries(config.general)) {
      const type = typeof value === 'boolean' ? 'boolean' : typeof value === 'number' ? 'uint32' : 'string'
      lines.push(`    ${type} ${key};`)
    }
    lines.push(`} ${moduleName}_Config = {`)

    const values = Object.entries(config.general).map(([key, value]) => {
      if (typeof value === 'boolean') return `    .${key} = ${value ? 'TRUE' : 'FALSE'}`
      if (typeof value === 'number') return `    .${key} = ${value}`
      return `    .${key} = "${value}"`
    })
    lines.push(values.join(',\n'))
    lines.push(`};`)
    lines.push(``)
  }

  lines.push(`void ${moduleName}_Init(void)`)
  lines.push(`{`)
  lines.push(`    /* Initialize ${moduleName} module */`)
  lines.push(`}`)
  lines.push(``)
  lines.push(`void ${moduleName}_Deinit(void)`)
  lines.push(`{`)
  lines.push(`    /* Deinitialize ${moduleName} module */`)
  lines.push(`}`)

  return lines.join('\n')
}

function generateConfig(moduleName: string, config: any): string {
  const lines = [
    `# ${moduleName} Configuration`,
    `# Generated by yuleASR Configurator`,
    `# ${new Date().toISOString()}`,
    ``,
  ]

  function flatten(obj: any, prefix: string = '') {
    for (const [key, value] of Object.entries(obj)) {
      const path = prefix ? `${prefix}.${key}` : key
      if (value && typeof value === 'object' && !Array.isArray(value)) {
        flatten(value, path)
      } else {
        lines.push(`${path} = ${JSON.stringify(value)}`)
      }
    }
  }

  flatten(config)
  return lines.join('\n')
}

function initEditor() {
  if (!editorContainer.value) return

  editor = monaco.editor.create(editorContainer.value, {
    value: previewCode.value,
    language: selectedFile.value === 'config' ? 'yaml' : 'c',
    theme: 'vs',
    readOnly: true,
    minimap: { enabled: false },
    scrollBeyondLastLine: false,
    fontSize: 13,
    lineNumbers: 'on',
    renderWhitespace: 'selection',
    automaticLayout: true,
  })
}

function refreshPreview() {
  if (editor) {
    editor.setValue(previewCode.value)
    ElMessage.success('Preview refreshed')
  }
}

function copyCode() {
  navigator.clipboard.writeText(previewCode.value).then(() => {
    ElMessage.success('Code copied to clipboard')
  })
}

watch([selectedModule, selectedFile], () => {
  if (editor) {
    editor.setValue(previewCode.value)
    monaco.editor.setModelLanguage(
      editor.getModel()!,
      selectedFile.value === 'config' ? 'yaml' : 'c',
    )
  }
})

onMounted(() => {
  if (enabledModules.value.length > 0) {
    selectedModule.value = enabledModules.value[0]
  }
  initEditor()
})

onUnmounted(() => {
  if (editor) {
    editor.dispose()
  }
})
</script>

<style scoped>
.code-preview {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #fff;
}

.preview-toolbar {
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

.editor-container {
  flex: 1;
  overflow: hidden;
}

.preview-footer {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  padding: 8px 12px;
  border-top: 1px solid #e4e7ed;
  background: #fafafa;
}
</style>
