export interface JsonSchema {
  type: string
  title?: string
  properties?: Record<string, JsonSchema>
  required?: string[]
  description?: string
  default?: any
  enum?: any[]
  items?: JsonSchema
  const?: any
  pattern?: string
  minimum?: number
  maximum?: number
}

export interface ModuleConfig {
  name: string
  enabled: boolean
  version?: string
  [key: string]: any
}

export interface Config {
  version: string
  project: {
    name: string
    vendor_id?: number
    description?: string
  }
  modules: Record<string, ModuleConfig>
}

export interface SchemaInfo {
  name: string
  layer: string
  path: string
}

export interface ValidationResult {
  valid: boolean
  errors: Array<{
    path: string
    message: string
    schema_path?: string
  }>
  warnings: Array<{
    path: string
    message: string
  }>
}

export interface InitOrderResult {
  order: string[]
  dependencies: Record<string, string[]>
}

// ── Project / ECU model (IDE-style engineering layout) ──

export interface EcuConfig {
  id: string
  name: string
  description?: string
  chip?: {
    vendor: string
    series: string
    model: string
    cores?: number
  }
  modules: Record<string, ModuleConfig>
}

export interface ProjectConfig {
  id: string
  name: string
  version: string
  vendor_id?: number
  description?: string
  ecus: EcuConfig[]
}

export interface ProjectSummary {
  id: string
  name: string
  ecu_count: number
  module_count: number
}
