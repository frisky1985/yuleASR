import axios from 'axios'
import type { Config, SchemaInfo, ValidationResult, InitOrderResult } from '@/types'

const api = axios.create({
  baseURL: '/api',
  timeout: 30000,
})

export async function getConfig(path?: string): Promise<Config> {
  const params = path ? { path } : {}
  const response = await api.get('/config', { params })
  return response.data.config
}

export async function saveConfig(config: Config, path?: string): Promise<void> {
  await api.post('/config', { config, path })
}

export async function validateConfig(config: Config): Promise<ValidationResult> {
  const response = await api.post('/config/validate', { config })
  return response.data
}

export interface ReferenceIssue {
  severity: 'error' | 'warning' | 'info'
  type: string
  source: string
  target: string
  message: string
  suggestion?: string
}

export interface ReferenceValidationResult {
  valid: boolean
  issues: ReferenceIssue[]
  summary: { errors: number; warnings: number; info: number }
}

export async function validateReferences(config: Config): Promise<ReferenceValidationResult> {
  const response = await api.post('/config/validate-references', { config })
  return response.data
}

export async function getSchemas(): Promise<Record<string, SchemaInfo[]>> {
  const response = await api.get('/schemas')
  return response.data.schemas
}

export async function getModuleSchema(moduleName: string): Promise<any> {
  const response = await api.get(`/schemas/${moduleName}`)
  return response.data
}

export async function getModules(): Promise<string[]> {
  const response = await api.get('/modules')
  return response.data.modules
}

export async function getInitOrder(): Promise<InitOrderResult> {
  const response = await api.get('/init-order')
  return response.data
}

export async function generateCode(config?: Config): Promise<{ output_dir: string; files: string[] }> {
  const response = await api.post('/generate', { config })
  return response.data
}

export function createWebSocket(onMessage: (data: any) => void): WebSocket {
  const ws = new WebSocket(`ws://${window.location.hostname}:8000/ws`)
  ws.onmessage = (event) => {
    const data = JSON.parse(event.data)
    onMessage(data)
  }
  return ws
}

export async function parseDbc(content: string): Promise<{
  success: boolean
  metadata: { version: string; node_count: number; message_count: number; signal_count: number }
  config: Record<string, any>
  messages: Array<{ id: number; name: string; length: number; transmitter: string; signal_count: number; comment: string }>
}> {
  const response = await api.post('/dbc/parse', content, {
    headers: { 'Content-Type': 'text/plain' },
  })
  return response.data
}

// Chip Database API
export async function getChipVendors(): Promise<Array<{ value: string; label: string }>> {
  const response = await api.get('/chips/vendors')
  return response.data.vendors
}

export async function getChipSeries(vendor: string): Promise<Array<{ value: string; label: string }>> {
  const response = await api.get('/chips/series', { params: { vendor } })
  return response.data.series
}

export async function getChipModels(vendor: string, series: string): Promise<Array<{
  value: string
  label: string
  clock_freq_mhz: number
  flash_size_kb: number
  ram_size_kb: number
  module_count: number
}>> {
  const response = await api.get('/chips/models', { params: { vendor, series } })
  return response.data.models
}

export async function getChipMcalModules(vendor: string, series: string, model: string): Promise<Array<{
  name: string
  label: string
  required: boolean
  default_params: Record<string, any>
  dependencies: string[]
}>> {
  const response = await api.get('/chips/mcal-modules', { params: { vendor, series, model } })
  return response.data.modules
}

export async function getChipInfo(vendor: string, series: string, model: string): Promise<{
  name: string
  label: string
  vendor: string
  series: string
  clock_freq_mhz: number
  flash_size_kb: number
  ram_size_kb: number
  mcal_modules: Array<{ name: string; label: string; required: boolean }>
  notes: string
}> {
  const response = await api.get('/chips/info', { params: { vendor, series, model } })
  return response.data.chip
}

// Network Management API
export async function parseNmMatrix(content: string, format: string = 'json'): Promise<{
  success: boolean
  metadata: { version: string; cluster_count: number; node_count: number; message_count: number }
  config: Record<string, any>
}> {
  const response = await api.post('/nm/parse', content, {
    params: { format },
    headers: { 'Content-Type': 'text/plain' },
  })
  return response.data
}

// LLM Chatbot API
export async function chatConfig(
  moduleName: string,
  userRequest: string,
  currentConfig?: Record<string, any>,
  model?: string,
): Promise<{
  success: boolean
  changes: Record<string, any>
  raw_response: string
  warnings: string[]
}> {
  const response = await api.post(
    '/chat/config',
    {
      module_name: moduleName,
      user_request: userRequest,
      current_config: currentConfig,
      model,
    },
    { timeout: 300000 },
  )
  return response.data
}

export async function applyChatConfig(
  moduleName: string,
  changes: Record<string, any>,
): Promise<{ success: boolean; module: string; path: string }> {
  const response = await api.post('/chat/config/apply', {
    module_name: moduleName,
    changes,
  })
  return response.data
}
