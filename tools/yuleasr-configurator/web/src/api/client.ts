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

export interface ReferenceTarget {
  label: string
  value: string | number
}

export interface ReferenceAutocompleteResult {
  targets: ReferenceTarget[]
  target_module: string
  target_field: string
}

export async function getReferenceTargets(module: string, field: string, query = ''): Promise<ReferenceAutocompleteResult> {
  const response = await api.post('/config/reference-autocomplete', { module, field, query })
  return response.data
}

// Project file API
export interface ProjectFileData {
  version: string
  project: { name: string; description: string; created: string; modified: string }
  ecus: Array<{ id: string; name: string; chip: any; config_path: string; description: string }>
  ui_state: { sidebar_visible: boolean; sidebar_width: number; active_tab: string; open_tabs: string[]; expanded_layers: string[] }
  recent_files: Array<{ path: string; opened: string }>
}

export async function getProject(): Promise<ProjectFileData> {
  const response = await api.get('/project')
  return response.data
}

export async function saveProject(data: ProjectFileData): Promise<void> {
  await api.post('/project', data)
}

export async function addEcu(ecu: { id: string; name: string; chip: any; config_path?: string; description?: string }): Promise<void> {
  await api.post('/project/ecu', ecu)
}

export async function removeEcu(ecuId: string): Promise<void> {
  await api.delete(`/project/ecu/${ecuId}`)
}

export async function addRecentFile(path: string): Promise<void> {
  await api.post('/project/recent', { path })
}

export async function saveUiState(state: any): Promise<void> {
  await api.post('/project/ui-state', state)
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

// ARXML Export/Import API
export async function exportArxml(config?: Config): Promise<{ success: boolean; arxml: string; module_count: number }> {
  const response = await api.post('/arxml/export', { config })
  return response.data
}

export async function importArxml(content: string): Promise<{
  success: boolean
  modules: Array<{ name: string; enabled: boolean; parameters: number }>
  warnings: string[]
  config: Record<string, any>
}> {
  const response = await api.post('/arxml/import', { content })
  return response.data
}

export async function createProject(data: {
  name: string
  description?: string
  author?: string
  version?: string
  chip?: { vendor: string; series: string; model: string; clock_freq_mhz?: number }
  mcal_modules: string[]
  comm_modules: string[]
}): Promise<{ success: boolean; project_path: string; config_path: string; ecu_id: string; module_count: number }> {
  const response = await api.post('/project/create', data)
  return response.data
}
