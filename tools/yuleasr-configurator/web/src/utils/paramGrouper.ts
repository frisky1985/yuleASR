import type { JsonSchema } from '@/types'

export interface ParamMeta {
  name: string
  path: string
  type: 'string' | 'number' | 'integer' | 'boolean' | 'enum' | 'array' | 'object'
  value: any
  defaultValue: any
  description: string
  required: boolean
  enum?: any[]
  minimum?: number
  maximum?: number
  pattern?: string
  visibility: 'basic' | 'advanced' | 'expert'
  group: string
  groupLabel: string
  editable: boolean
}

export interface ParamGroup {
  name: string
  label: string
  description: string
  visibility: 'basic' | 'advanced' | 'expert'
  params: ParamMeta[]
  expanded: boolean
  childGroups: ParamGroup[]
}

const SKIP_KEYS = new Set(['name', 'enabled', 'version'])

const VISIBILITY_HINTS: Record<string, 'basic' | 'advanced' | 'expert'> = {
  dev_error_detect: 'advanced',
  version_info_api: 'expert',
  deinit_api: 'expert',
}

function inferVisibility(name: string, schema: JsonSchema): 'basic' | 'advanced' | 'expert' {
  if (VISIBILITY_HINTS[name]) return VISIBILITY_HINTS[name]
  if (schema.const !== undefined) return 'expert'
  if (name.startsWith('_')) return 'expert'
  return 'basic'
}

function inferType(schema: JsonSchema): ParamMeta['type'] {
  if (schema.enum) return 'enum'
  if (schema.type === 'string') return 'string'
  if (schema.type === 'integer') return 'integer'
  if (schema.type === 'number') return 'number'
  if (schema.type === 'boolean') return 'boolean'
  if (schema.type === 'array') return 'array'
  if (schema.type === 'object') return 'object'
  return 'string'
}

function getNestedValue(obj: any, path: string): any {
  const keys = path.split('.')
  let current = obj
  for (const key of keys) {
    if (current === null || current === undefined) return undefined
    current = current[key]
  }
  return current
}

function buildParam(
  name: string,
  path: string,
  schema: JsonSchema,
  configValue: any,
  required: boolean,
  groupName: string,
  groupLabel: string,
): ParamMeta {
  const type = inferType(schema)
  const value = configValue !== undefined ? configValue : schema.default
  const visibility = inferVisibility(name, schema)
  const editable = schema.const === undefined

  return {
    name,
    path,
    type,
    value,
    defaultValue: schema.default,
    description: schema.description || name,
    required,
    enum: schema.enum,
    minimum: schema.minimum,
    maximum: schema.maximum,
    pattern: schema.pattern,
    visibility,
    group: groupName,
    groupLabel,
    editable,
  }
}

function buildGroup(
  groupName: string,
  groupSchema: JsonSchema,
  configObj: any,
  requiredFields: string[],
  parentPath: string,
): ParamGroup {
  const label = groupSchema.description || groupName
  const params: ParamMeta[] = []
  const childGroups: ParamGroup[] = []

  if (groupSchema.properties) {
    for (const [propName, propSchema] of Object.entries(groupSchema.properties)) {
      if (SKIP_KEYS.has(propName)) continue

      const propPath = parentPath ? `${parentPath}.${propName}` : propName
      const isRequired = requiredFields.includes(propName)
      const configValue = getNestedValue(configObj, propName)

      if (propSchema.type === 'object' && propSchema.properties) {
        childGroups.push(buildGroup(propName, propSchema, configValue || {}, propSchema.required || [], propPath))
      } else if (propSchema.type === 'array' && propSchema.items) {
        params.push(buildParam(propName, propPath, propSchema, configValue, isRequired, groupName, label))
      } else {
        params.push(buildParam(propName, propPath, propSchema, configValue, isRequired, groupName, label))
      }
    }
  }

  const groupVisibility = params.length > 0
    ? params.reduce((min, p) => {
        const order = { basic: 0, advanced: 1, expert: 2 }
        return order[p.visibility] > order[min] ? p.visibility : min
      }, 'basic' as 'basic' | 'advanced' | 'expert')
    : 'basic'

  return {
    name: groupName,
    label,
    description: groupSchema.description || '',
    visibility: groupVisibility,
    params,
    expanded: true,
    childGroups,
  }
}

export function groupParams(schema: JsonSchema, config: Record<string, any>): ParamGroup[] {
  const groups: ParamGroup[] = []
  const requiredFields = schema.required || []

  if (!schema.properties) return groups

  for (const [propName, propSchema] of Object.entries(schema.properties)) {
    if (SKIP_KEYS.has(propName)) continue

    const isRequired = requiredFields.includes(propName)

    if (propSchema.type === 'object' && propSchema.properties) {
      const configValue = config[propName] || {}
      groups.push(buildGroup(propName, propSchema, configValue, propSchema.required || [], propName))
    } else {
      const configValue = config[propName]
      const param = buildParam(
        propName,
        propName,
        propSchema,
        configValue,
        isRequired,
        '_root',
        'General',
      )

      let rootGroup = groups.find((g) => g.name === '_root')
      if (!rootGroup) {
        rootGroup = {
          name: '_root',
          label: 'General',
          description: 'Top-level parameters',
          visibility: 'basic',
          params: [],
          expanded: true,
          childGroups: [],
        }
        groups.unshift(rootGroup)
      }
      rootGroup.params.push(param)
    }
  }

  return groups
}

export function flattenParams(groups: ParamGroup[]): ParamMeta[] {
  const result: ParamMeta[] = []

  function walk(groupList: ParamGroup[]) {
    for (const group of groupList) {
      result.push(...group.params)
      walk(group.childGroups)
    }
  }

  walk(groups)
  return result
}

export function filterParamsByVisibility(
  groups: ParamGroup[],
  level: 'basic' | 'advanced' | 'expert',
): ParamGroup[] {
  const order = { basic: 0, advanced: 1, expert: 2 }
  const maxOrder = order[level]

  function filterGroupList(groupList: ParamGroup[]): ParamGroup[] {
    return groupList
      .filter((g) => order[g.visibility] <= maxOrder)
      .map((g) => ({
        ...g,
        params: g.params.filter((p) => order[p.visibility] <= maxOrder),
        childGroups: filterGroupList(g.childGroups),
      }))
      .filter((g) => g.params.length > 0 || g.childGroups.length > 0)
  }

  return filterGroupList(groups)
}

export function filterParamsBySearch(groups: ParamGroup[], search: string): ParamGroup[] {
  if (!search.trim()) return groups
  const lower = search.toLowerCase()

  function filterGroupList(groupList: ParamGroup[]): ParamGroup[] {
    return groupList
      .map((g) => {
        const matchedParams = g.params.filter(
          (p) =>
            p.name.toLowerCase().includes(lower) ||
            p.description.toLowerCase().includes(lower) ||
            p.path.toLowerCase().includes(lower),
        )
        const matchedChildren = filterGroupList(g.childGroups)
        if (matchedParams.length > 0 || matchedChildren.length > 0) {
          return { ...g, params: matchedParams, childGroups: matchedChildren }
        }
        return null
      })
      .filter((g): g is ParamGroup => g !== null)
  }

  return filterGroupList(groups)
}

export function setParamValue(
  config: Record<string, any>,
  paramPath: string,
  value: any,
): Record<string, any> {
  const keys = paramPath.split('.')
  const result = JSON.parse(JSON.stringify(config))
  let current = result

  for (let i = 0; i < keys.length - 1; i++) {
    if (!(keys[i] in current)) {
      current[keys[i]] = {}
    }
    current = current[keys[i]]
  }

  current[keys[keys.length - 1]] = value
  return result
}
