<template>
  <div class="config-chatbot">
    <div class="chatbot-header">
      <h3>
        <el-icon><ChatDotRound /></el-icon>
        AI Configuration Assistant
      </h3>
      <el-button text size="small" @click="$emit('close')">
        <el-icon><Close /></el-icon>
      </el-button>
    </div>

    <div class="chatbot-messages" ref="messagesContainer">
      <div v-if="messages.length === 0" class="empty-chat">
        <el-icon :size="48" color="#c0c4cc"><ChatDotRound /></el-icon>
        <p>Describe your configuration needs for <strong>{{ moduleName }}</strong></p>
        
        <!-- Module Context Info -->
        <div v-if="currentContextInfo.length > 0 || keyParameters.length > 0" class="module-context">
          <div class="context-header">
            <el-icon><InfoFilled /></el-icon>
            <span>Current Configuration</span>
          </div>
          <div v-if="currentContextInfo.length > 0" class="context-description">
            {{ currentContextInfo[0] }}
          </div>
          <div v-if="keyParameters.length > 0" class="context-params">
            <div v-for="param in keyParameters" :key="param.name" class="context-param">
              <span class="param-name">{{ param.name }}:</span>
              <span class="param-value">{{ formatParamValue(param.value) }}</span>
            </div>
          </div>
        </div>
        
        <div class="prompt-templates">
          <p class="templates-title">Try these examples:</p>
          <div
            v-for="(template, idx) in getPromptTemplate()"
            :key="idx"
            class="template-item"
            @click="useTemplate(template)"
          >
            <el-icon><Promotion /></el-icon>
            <span>{{ template }}</span>
          </div>
        </div>
      </div>

      <div v-for="(msg, idx) in messages" :key="idx" :class="['message', msg.role]">
        <div class="message-avatar">
          <el-icon v-if="msg.role === 'user'" :size="20"><User /></el-icon>
          <el-icon v-else :size="20" color="#409eff"><ChatDotRound /></el-icon>
        </div>
        <div class="message-content">
          <div class="message-text" v-html="formatMessage(msg.content)"></div>

          <div v-if="msg.changes" class="changes-preview">
            <div class="changes-header">
              <el-icon><Document /></el-icon>
              <span>Proposed Changes</span>
            </div>
            <div class="changes-list">
              <div v-for="(value, key) in flattenChanges(msg.changes)" :key="key" class="change-item">
                <span class="change-key">{{ key }}:</span>
                <span class="change-value">{{ value }}</span>
              </div>
            </div>
            <div v-if="msg.warnings && msg.warnings.length > 0" class="changes-warnings">
              <el-icon color="#e6a23c"><Warning /></el-icon>
              <span v-for="(warn, wi) in msg.warnings" :key="wi">{{ warn }}</span>
            </div>
            <div class="changes-actions" v-if="msg.role === 'assistant' && idx === messages.length - 1">
              <el-button size="small" type="primary" @click="applyChanges(msg)" :loading="applying">
                Apply
              </el-button>
              <el-button size="small" @click="discardChanges(msg)">
                Discard
              </el-button>
            </div>
          </div>

          <div v-if="msg.error" class="message-error">
            <el-icon><CircleClose /></el-icon>
            {{ msg.error }}
          </div>
        </div>
      </div>

      <div v-if="loading" class="message assistant">
        <div class="message-avatar">
          <el-icon :size="20" color="#409eff"><ChatDotRound /></el-icon>
        </div>
        <div class="message-content">
          <div class="typing-indicator">
            <span></span><span></span><span></span>
          </div>
        </div>
      </div>
    </div>

    <div class="chatbot-input">
      <el-input
        v-model="inputText"
        type="textarea"
        :rows="2"
        placeholder="Describe your configuration needs..."
        @keydown.enter.exact.prevent="sendMessage"
        :disabled="loading"
      />
      <el-button
        type="primary"
        @click="sendMessage"
        :loading="loading"
        :disabled="!inputText.trim() || !moduleName"
      >
        <el-icon><Promotion /></el-icon>
        Send
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, nextTick, watch, onMounted, computed } from 'vue'
import { ElMessage } from 'element-plus'
import {
  ChatDotRound,
  Close,
  User,
  Document,
  Warning,
  CircleClose,
  Promotion,
  InfoFilled,
} from '@element-plus/icons-vue'
import { chatConfig, applyChatConfig, getModuleSchema } from '@/api/client'

interface ChatMessage {
  role: 'user' | 'assistant'
  content: string
  changes?: Record<string, any>
  warnings?: string[]
  error?: string
  applied?: boolean
}

const props = defineProps<{
  moduleName: string
  currentConfig?: Record<string, any>
}>()

const emit = defineEmits<{
  close: []
  applied: [changes: Record<string, any>]
}>()

const messages = ref<ChatMessage[]>([])
const inputText = ref('')
const loading = ref(false)
const applying = ref(false)
const messagesContainer = ref<HTMLElement>()
const moduleSchema = ref<any>(null)
const moduleDescription = ref('')
const keyParameters = ref<Array<{ name: string; value: any; description: string }>>([])

// Load module schema and extract context on mount
onMounted(async () => {
  try {
    moduleSchema.value = await getModuleSchema(props.moduleName)
    moduleDescription.value = moduleSchema.value?.description || ''
    extractKeyParameters()
  } catch (e) {
    console.warn('Failed to load module schema:', e)
  }
})

function extractKeyParameters() {
  if (!moduleSchema.value?.properties || !props.currentConfig) return
  
  const properties = moduleSchema.value.properties
  const config = props.currentConfig
  
  // Extract top-level parameters
  for (const [key, def] of Object.entries(properties)) {
    if (['name', 'enabled', 'version'].includes(key)) continue
    
    const paramDef = def as any
    if (paramDef.type === 'object' && paramDef.properties) {
      // Nested object - extract first few properties
      for (const [subKey, subDef] of Object.entries(paramDef.properties)) {
        const value = config[key]?.[subKey]
        if (value !== undefined) {
          keyParameters.value.push({
            name: `${key}.${subKey}`,
            value,
            description: (subDef as any).description || '',
          })
        }
        if (keyParameters.value.length >= 5) break
      }
    } else {
      // Top-level parameter
      const value = config[key]
      if (value !== undefined) {
        keyParameters.value.push({
          name: key,
          value,
          description: paramDef.description || '',
        })
      }
    }
    
    if (keyParameters.value.length >= 5) break
  }
}

const currentContextInfo = computed(() => {
  const lines: string[] = []
  if (moduleDescription.value) {
    lines.push(moduleDescription.value)
  }
  if (props.currentConfig?.enabled !== undefined) {
    lines.push(`Status: ${props.currentConfig.enabled ? 'Enabled' : 'Disabled'}`)
  }
  return lines
})

// Module-specific prompt templates
const PROMPT_TEMPLATES: Record<string, string[]> = {
  Can: [
    '配置 CAN 总线，波特率 500kbps，3 个控制器',
    '启用错误检测和唤醒支持',
    '配置 CAN FD 支持，数据段波特率 2Mbps',
  ],
  CanIf: [
    '配置 CAN 接口，启用 Rx 指示',
    '设置 TX 确认超时时间为 100ms',
  ],
  NvM: [
    '启用冗余模式，块大小 64 字节',
    '配置最大块数 100，启用 CRC 校验',
    '设置写重试次数为 3',
  ],
  Com: [
    '配置信号超时监控，超时时间 1 秒',
    '启用网关路由，配置 10 个信号',
    '配置 Tx 确认回调',
  ],
  Dcm: [
    '配置诊断会话控制服务 (0x10)',
    '启用安全访问服务 (0x27)，配置种子长度 4 字节',
    '配置 ECU 复位服务 (0x11)',
  ],
  Dem: [
    '配置 DTC 存储，启用冻结帧',
    '设置老化计数器阈值 50',
    '配置事件内存大小 1024 字节',
  ],
  EcuM: [
    '配置启动目标状态为 RUN',
    '设置关机超时时间 5 秒',
    '配置唤醒源：CAN、LIN',
  ],
  BswM: [
    '配置模式控制规则',
    '启用网络管理集成',
  ],
  Os: [
    '配置 8 个任务，优先级轮询调度',
    '设置系统定时器周期 1ms',
    '启用资源管理，配置 5 个资源',
  ],
  PduR: [
    '配置 PDU 路由，CAN 到 Com',
    '启用 Tx 确认传递',
    '配置网关路由规则',
  ],
  Eth: [
    '配置以太网 PHY 接口',
    '设置 VLAN ID 100',
    '启用 VLAN 标签',
  ],
  Lin: [
    '配置 LIN 主节点，波特率 19200bps',
    '配置 4 个从节点',
    '启用休眠和唤醒支持',
  ],
  Spi: [
    '配置 SPI 通道，时钟频率 10MHz',
    '设置数据宽度 8 位，MSB 优先',
    '配置 3 个 SPI 设备',
  ],
  Adc: [
    '配置 ADC 分辨率 12 位',
    '设置采样时间 10 微秒',
    '启用 8 个转换通道',
  ],
  Wdg: [
    '配置看门狗超时时间 100ms',
    '启用窗口模式',
    '配置触发模式：独立窗口',
  ],
}

function getPromptTemplate(): string[] {
  return PROMPT_TEMPLATES[props.moduleName] || [
    `配置 ${props.moduleName} 模块的基本参数`,
    `启用 ${props.moduleName} 的主要功能`,
  ]
}

function formatMessage(text: string): string {
  return text.replace(/\n/g, '<br>')
}

function flattenChanges(obj: Record<string, any>, prefix = ''): Record<string, any> {
  const result: Record<string, any> = {}
  for (const [key, value] of Object.entries(obj)) {
    const fullKey = prefix ? `${prefix}.${key}` : key
    if (typeof value === 'object' && value !== null && !Array.isArray(value)) {
      Object.assign(result, flattenChanges(value, fullKey))
    } else {
      result[fullKey] = value
    }
  }
  return result
}

function scrollToBottom() {
  nextTick(() => {
    if (messagesContainer.value) {
      messagesContainer.value.scrollTop = messagesContainer.value.scrollHeight
    }
  })
}

function useTemplate(template: string) {
  inputText.value = template
}

function formatParamValue(value: any): string {
  if (typeof value === 'boolean') return value ? 'true' : 'false'
  if (typeof value === 'number') return value.toString()
  if (Array.isArray(value)) return `[${value.length} items]`
  if (typeof value === 'object') return '{...}'
  return String(value)
}

watch(messages, scrollToBottom, { deep: true })

async function sendMessage() {
  const text = inputText.value.trim()
  if (!text || !props.moduleName || loading.value) return

  messages.value.push({ role: 'user', content: text })
  inputText.value = ''
  loading.value = true

  try {
    const result = await chatConfig(
      props.moduleName,
      text,
      props.currentConfig,
    )

    if (result.success) {
      messages.value.push({
        role: 'assistant',
        content: `I've generated the following configuration changes for ${props.moduleName}:`,
        changes: result.changes,
        warnings: result.warnings,
      })
    } else {
      messages.value.push({
        role: 'assistant',
        content: 'Sorry, I could not generate configuration changes.',
        error: 'LLM output could not be parsed',
      })
    }
  } catch (err: any) {
    const msg = err.response?.data?.detail || err.message || 'Unknown error'
    messages.value.push({
      role: 'assistant',
      content: 'An error occurred while processing your request.',
      error: msg,
    })
  } finally {
    loading.value = false
  }
}

async function applyChanges(msg: ChatMessage) {
  if (!msg.changes) return

  applying.value = true
  try {
    await applyChatConfig(props.moduleName, msg.changes)
    msg.applied = true
    ElMessage.success(`Configuration applied to ${props.moduleName}`)
    emit('applied', msg.changes)
  } catch (err: any) {
    ElMessage.error(err.response?.data?.detail || 'Failed to apply changes')
  } finally {
    applying.value = false
  }
}

function discardChanges(msg: ChatMessage) {
  msg.changes = undefined
  msg.warnings = undefined
}
</script>

<style scoped>
.config-chatbot {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #fff;
  border-left: 1px solid #e4e7ed;
}

.chatbot-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  border-bottom: 1px solid #e4e7ed;
  background: #fafafa;
}

.chatbot-header h3 {
  margin: 0;
  font-size: 14px;
  font-weight: 600;
  display: flex;
  align-items: center;
  gap: 6px;
}

.chatbot-messages {
  flex: 1;
  overflow-y: auto;
  padding: 16px;
}

.empty-chat {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100%;
  color: #909399;
  text-align: center;
  padding: 20px;
}

.empty-chat p {
  margin: 8px 0 0;
  font-size: 13px;
}

.empty-chat strong {
  color: #409eff;
}

.module-context {
  margin-top: 16px;
  width: 100%;
  max-width: 320px;
  background: #f0f9ff;
  border: 1px solid #b3d8ff;
  border-radius: 8px;
  padding: 12px;
  text-align: left;
}

.context-header {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  font-weight: 600;
  color: #409eff;
  margin-bottom: 8px;
}

.context-description {
  font-size: 12px;
  color: #606266;
  margin-bottom: 8px;
  line-height: 1.4;
}

.context-params {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.context-param {
  display: flex;
  gap: 6px;
  font-size: 11px;
  font-family: 'Monaco', 'Menlo', monospace;
}

.param-name {
  color: #409eff;
  font-weight: 500;
}

.param-value {
  color: #67c23a;
}

.prompt-templates {
  margin-top: 16px;
  width: 100%;
  max-width: 320px;
}

.templates-title {
  font-size: 12px;
  color: #c0c4cc;
  margin-bottom: 8px;
}

.template-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  margin-bottom: 6px;
  background: #f5f7fa;
  border: 1px solid #e4e7ed;
  border-radius: 6px;
  font-size: 12px;
  color: #606266;
  cursor: pointer;
  transition: all 0.2s;
}

.template-item:hover {
  background: #ecf5ff;
  border-color: #409eff;
  color: #409eff;
}

.template-item .el-icon {
  flex-shrink: 0;
}

.message {
  display: flex;
  gap: 8px;
  margin-bottom: 16px;
}

.message.user {
  flex-direction: row-reverse;
}

.message-avatar {
  flex-shrink: 0;
  width: 32px;
  height: 32px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #f0f2f5;
}

.message.user .message-avatar {
  background: #409eff;
  color: #fff;
}

.message-content {
  max-width: 80%;
  min-width: 120px;
}

.message-text {
  padding: 8px 12px;
  border-radius: 8px;
  font-size: 13px;
  line-height: 1.5;
  background: #f0f2f5;
}

.message.user .message-text {
  background: #409eff;
  color: #fff;
}

.changes-preview {
  margin-top: 8px;
  border: 1px solid #e4e7ed;
  border-radius: 6px;
  overflow: hidden;
  background: #fafafa;
}

.changes-header {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 10px;
  background: #f5f7fa;
  border-bottom: 1px solid #e4e7ed;
  font-size: 12px;
  font-weight: 600;
  color: #606266;
}

.changes-list {
  padding: 8px 10px;
  font-family: 'Monaco', 'Menlo', monospace;
  font-size: 12px;
}

.change-item {
  display: flex;
  gap: 4px;
  padding: 2px 0;
}

.change-key {
  color: #409eff;
}

.change-value {
  color: #67c23a;
}

.changes-warnings {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px 10px;
  background: #fdf6ec;
  border-top: 1px solid #faecd8;
  font-size: 12px;
  color: #e6a23c;
}

.changes-actions {
  display: flex;
  gap: 8px;
  padding: 8px 10px;
  border-top: 1px solid #e4e7ed;
  background: #fff;
}

.message-error {
  display: flex;
  align-items: center;
  gap: 4px;
  margin-top: 6px;
  padding: 6px 10px;
  background: #fef0f0;
  border-radius: 4px;
  font-size: 12px;
  color: #f56c6c;
}

.typing-indicator {
  display: flex;
  gap: 4px;
  padding: 12px;
}

.typing-indicator span {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: #409eff;
  animation: typing 1.4s infinite ease-in-out;
}

.typing-indicator span:nth-child(2) {
  animation-delay: 0.2s;
}

.typing-indicator span:nth-child(3) {
  animation-delay: 0.4s;
}

@keyframes typing {
  0%, 80%, 100% {
    transform: scale(0.6);
    opacity: 0.4;
  }
  40% {
    transform: scale(1);
    opacity: 1;
  }
}

.chatbot-input {
  display: flex;
  gap: 8px;
  padding: 12px 16px;
  border-top: 1px solid #e4e7ed;
  background: #fafafa;
}

.chatbot-input :deep(.el-textarea__inner) {
  resize: none;
}
</style>
