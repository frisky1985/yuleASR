<template>
  <div class="dbc-import">
    <div class="import-header">
      <h2>DBC Smart Import</h2>
      <el-steps :active="currentStep" finish-status="success" align-center>
        <el-step title="Upload DBC" />
        <el-step title="Preview Messages" />
        <el-step title="Select & Map" />
        <el-step title="Import Result" />
      </el-steps>
    </div>

    <div class="import-content">
      <!-- Step 1: Upload -->
      <div v-if="currentStep === 0" class="step-content">
        <el-upload
          ref="uploadRef"
          class="dbc-upload"
          drag
          :auto-upload="false"
          :limit="1"
          accept=".dbc"
          :on-change="handleFileChange"
        >
          <el-icon class="el-icon--upload" :size="48"><UploadFilled /></el-icon>
          <div class="el-upload__text">
            Drop DBC file here or <em>click to browse</em>
          </div>
          <template #tip>
            <div class="el-upload__tip">
              Supports Vector DBC format (.dbc) for CAN communication description
            </div>
          </template>
        </el-upload>
      </div>

      <!-- Step 2: Preview -->
      <div v-if="currentStep === 1" class="step-content">
        <el-descriptions :column="4" border size="small" style="margin-bottom: 16px">
          <el-descriptions-item label="Version">
            <el-tag size="small">{{ parseResult?.metadata.version || 'N/A' }}</el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="Nodes">
            <el-tag type="info" size="small">{{ parseResult?.metadata.node_count || 0 }}</el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="Messages">
            <el-tag type="success" size="small">{{ parseResult?.metadata.message_count || 0 }}</el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="Signals">
            <el-tag type="warning" size="small">{{ parseResult?.metadata.signal_count || 0 }}</el-tag>
          </el-descriptions-item>
        </el-descriptions>

        <el-input
          v-model="messageSearch"
          placeholder="Search messages..."
          size="small"
          clearable
          style="margin-bottom: 12px"
        >
          <template #prefix>
            <el-icon><Search /></el-icon>
          </template>
        </el-input>

        <el-table
          :data="filteredMessages"
          stripe
          border
          max-height="400"
          size="small"
          @selection-change="handleSelectionChange"
        >
          <el-table-column type="selection" width="40" />
          <el-table-column prop="id" label="ID" width="80">
            <template #default="{ row }">
              <el-text size="small">0x{{ row.id.toString(16).toUpperCase().padStart(3, '0') }}</el-text>
            </template>
          </el-table-column>
          <el-table-column prop="name" label="Message Name" min-width="180" sortable />
          <el-table-column prop="length" label="DLC" width="60" align="center" />
          <el-table-column prop="transmitter" label="Transmitter" width="120" />
          <el-table-column prop="signal_count" label="Signals" width="80" align="center">
            <template #default="{ row }">
              <el-tag size="small">{{ row.signal_count }}</el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="comment" label="Comment" min-width="200">
            <template #default="{ row }">
              <el-text type="info" size="small">{{ row.comment || '-' }}</el-text>
            </template>
          </el-table-column>
        </el-table>

        <div class="step-actions">
          <el-text size="small">Selected: {{ selectedMessages.length }} / {{ parseResult?.messages.length || 0 }}</el-text>
        </div>
      </div>

      <!-- Step 3: Select & Map -->
      <div v-if="currentStep === 2" class="step-content">
        <el-alert
          title="Auto-Mapping Configuration"
          type="info"
          :closable="false"
          show-icon
          style="margin-bottom: 16px"
        >
          Selected {{ selectedMessages.length }} messages will be mapped to CanIf, Com, and PduR modules.
        </el-alert>

        <el-card shadow="never">
          <template #header>Mapping Preview</template>
          <el-descriptions :column="1" border size="small">
            <el-descriptions-item label="CanIf Rx Pdus">
              <el-tag type="success" size="small">{{ selectedMessages.length }} PDUs</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="CanIf Tx Pdus">
              <el-tag type="success" size="small">{{ selectedMessages.length }} PDUs</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="Com Signals">
              <el-tag type="warning" size="small">{{ totalSignalCount }} signals</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="PduR Routes">
              <el-tag type="info" size="small">{{ selectedMessages.length }} routes</el-tag>
            </el-descriptions-item>
          </el-descriptions>
        </el-card>

        <el-card v-if="conflicts.length > 0" shadow="never" style="margin-top: 12px">
          <template #header>
            <el-text type="warning">Conflicts Detected</el-text>
          </template>
          <el-alert
            v-for="(conflict, i) in conflicts"
            :key="i"
            :title="conflict.message"
            type="warning"
            show-icon
            style="margin-bottom: 8px"
          />
        </el-card>
      </div>

      <!-- Step 4: Result -->
      <div v-if="currentStep === 3" class="step-content">
        <el-result icon="success" title="Import Completed">
          <template #sub-title>
            <p>Successfully imported {{ selectedMessages.length }} messages</p>
            <p>{{ totalSignalCount }} signals mapped to Com module</p>
          </template>
          <template #extra>
            <el-button type="primary" @click="$router.push('/config')">
              Go to Configuration Editor
            </el-button>
            <el-button @click="reset">Import Another</el-button>
          </template>
        </el-result>
      </div>
    </div>

    <div class="import-footer">
      <el-button v-if="currentStep > 0" @click="currentStep--">
        Previous
      </el-button>
      <div class="footer-spacer" />
      <el-button
        v-if="currentStep < 3"
        type="primary"
        :disabled="!canProceed"
        @click="nextStep"
      >
        {{ currentStep === 2 ? 'Import' : 'Next' }}
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { UploadFilled, Search } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import { parseDbc } from '@/api/client'
import { useConfigStore } from '@/stores/config'

interface DbcMessage {
  id: number
  name: string
  length: number
  transmitter: string
  signal_count: number
  comment: string
}

interface ParseResult {
  success: boolean
  metadata: { version: string; node_count: number; message_count: number; signal_count: number }
  config: Record<string, any>
  messages: DbcMessage[]
}

interface Conflict {
  message: string
  type: 'duplicate' | 'missing-dep' | 'overlap'
}

const store = useConfigStore()

const currentStep = ref(0)
const uploadRef = ref()
const parseResult = ref<ParseResult | null>(null)
const messageSearch = ref('')
const selectedMessages = ref<DbcMessage[]>([])
const dbcContent = ref('')

const filteredMessages = computed(() => {
  if (!parseResult.value?.messages) return []
  if (!messageSearch.value) return parseResult.value.messages
  const lower = messageSearch.value.toLowerCase()
  return parseResult.value.messages.filter(
    (m) => m.name.toLowerCase().includes(lower) || m.comment?.toLowerCase().includes(lower),
  )
})

const totalSignalCount = computed(() => {
  return selectedMessages.value.reduce((sum, m) => sum + m.signal_count, 0)
})

const conflicts = computed<Conflict[]>(() => {
  const result: Conflict[] = []
  const modules = store.modules

  if (!modules.CanIf?.enabled && !modules.Can?.enabled) {
    result.push({
      message: 'Can and CanIf modules are not enabled. DBC import requires CAN stack.',
      type: 'missing-dep',
    })
  }

  const ids = selectedMessages.value.map((m) => m.id)
  const dupes = ids.filter((id, i) => ids.indexOf(id) !== i)
  if (dupes.length > 0) {
    result.push({
      message: `Duplicate message IDs detected: ${dupes.map((d) => '0x' + d.toString(16)).join(', ')}`,
      type: 'duplicate',
    })
  }

  return result
})

const canProceed = computed(() => {
  if (currentStep.value === 0) return !!parseResult.value
  if (currentStep.value === 1) return selectedMessages.value.length > 0
  if (currentStep.value === 2) return true
  return false
})

async function handleFileChange(file: any) {
  const rawFile = file.raw as File
  if (!rawFile) return

  try {
    dbcContent.value = await rawFile.text()
    const result = await parseDbc(dbcContent.value)
    parseResult.value = result as ParseResult
    ElMessage.success(`Parsed ${result.messages.length} messages from ${rawFile.name}`)
  } catch (e: any) {
    ElMessage.error(`Failed to parse DBC: ${e.message}`)
    parseResult.value = null
  }
}

function handleSelectionChange(selection: DbcMessage[]) {
  selectedMessages.value = selection
}

function nextStep() {
  if (currentStep.value === 2) {
    doImport()
  }
  currentStep.value++
}

function doImport() {
  if (!parseResult.value) return

  const config = parseResult.value.config
  for (const [moduleName, moduleConfig] of Object.entries(config)) {
    if (store.modules[moduleName]) {
      store.updateModule(moduleName, moduleConfig as any)
    }
  }

  ElMessage.success(`Imported ${selectedMessages.value.length} messages`)
}

function reset() {
  currentStep.value = 0
  parseResult.value = null
  selectedMessages.value = []
  messageSearch.value = ''
  dbcContent.value = ''
}
</script>

<style scoped>
.dbc-import {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #fff;
  padding: 20px;
}

.import-header {
  margin-bottom: 24px;
}

.import-header h2 {
  margin: 0 0 16px;
  font-size: 20px;
  color: #303133;
}

.import-content {
  flex: 1;
  overflow-y: auto;
}

.step-content {
  max-width: 900px;
  margin: 0 auto;
}

.dbc-upload {
  width: 100%;
}

.dbc-upload :deep(.el-upload-dragger) {
  padding: 40px;
}

.step-actions {
  display: flex;
  justify-content: flex-end;
  margin-top: 12px;
}

.import-footer {
  display: flex;
  align-items: center;
  padding: 16px 0 0;
  border-top: 1px solid #e4e7ed;
  margin-top: 20px;
}

.footer-spacer {
  flex: 1;
}
</style>
