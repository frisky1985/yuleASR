import type { ParamMeta } from './paramGrouper'

export interface ValidationIssue {
  path: string
  message: string
  severity: 'error' | 'warning' | 'info'
  rule: string
}

export interface CrossParamRule {
  name: string
  description: string
  check: (config: Record<string, any>) => ValidationIssue | null
}

const CROSS_PARAM_RULES: CrossParamRule[] = [
  {
    name: 'can-requires-mcu-clock',
    description: 'Can module requires Mcu clock configuration',
    check: (config) => {
      const can = config.modules?.Can
      const mcu = config.modules?.Mcu
      if (can?.enabled && mcu && !mcu.general?.clock_configured) {
        return {
          path: 'modules.Can',
          message: 'Can module requires Mcu clock configuration. Set Mcu.general.clock_configured = true.',
          severity: 'error',
          rule: 'can-requires-mcu-clock',
        }
      }
      return null
    },
  },
  {
    name: 'cantp-requires-canif',
    description: 'CanTp module requires CanIf to be enabled',
    check: (config) => {
      const cantp = config.modules?.CanTp
      const canif = config.modules?.CanIf
      if (cantp?.enabled && !canif?.enabled) {
        return {
          path: 'modules.CanTp',
          message: 'CanTp requires CanIf to be enabled.',
          severity: 'error',
          rule: 'cantp-requires-canif',
        }
      }
      return null
    },
  },
  {
    name: 'pdur-requires-com',
    description: 'PduR module requires Com to be enabled',
    check: (config) => {
      const pdur = config.modules?.PduR
      const com = config.modules?.Com
      if (pdur?.enabled && !com?.enabled) {
        return {
          path: 'modules.PduR',
          message: 'PduR requires Com to be enabled.',
          severity: 'warning',
          rule: 'pdur-requires-com',
        }
      }
      return null
    },
  },
  {
    name: 'nvmblocks-require-nvm',
    description: 'NvM blocks require NvM to be enabled',
    check: (config) => {
      const nvmblocks = config.modules?.NvM_Blocks
      const nvm = config.modules?.NvM
      if (nvmblocks && !nvm?.enabled) {
        return {
          path: 'modules.NvM_Blocks',
          message: 'NvM_Blocks configuration requires NvM to be enabled.',
          severity: 'error',
          rule: 'nvmblocks-require-nvm',
        }
      }
      return null
    },
  },
]

export function validateParam(param: ParamMeta): ValidationIssue[] {
  const issues: ValidationIssue[] = []

  if (param.required && (param.value === undefined || param.value === null || param.value === '')) {
    issues.push({
      path: param.path,
      message: `${param.name} is required`,
      severity: 'error',
      rule: 'required',
    })
    return issues
  }

  if (param.value === undefined || param.value === null) return issues

  if (param.type === 'number' || param.type === 'integer') {
    const num = typeof param.value === 'number' ? param.value : Number(param.value)
    if (isNaN(num)) {
      issues.push({
        path: param.path,
        message: `${param.name} must be a number`,
        severity: 'error',
        rule: 'type-number',
      })
    } else {
      if (param.minimum !== undefined && num < param.minimum) {
        issues.push({
          path: param.path,
          message: `${param.name} must be >= ${param.minimum} (current: ${num})`,
          severity: 'error',
          rule: 'range-min',
        })
      }
      if (param.maximum !== undefined && num > param.maximum) {
        issues.push({
          path: param.path,
          message: `${param.name} must be <= ${param.maximum} (current: ${num})`,
          severity: 'error',
          rule: 'range-max',
        })
      }
    }
  }

  if (param.type === 'enum' && param.enum && !param.enum.includes(param.value)) {
    issues.push({
      path: param.path,
      message: `${param.name} must be one of: ${param.enum.join(', ')}`,
      severity: 'error',
      rule: 'enum-value',
    })
  }

  if (param.type === 'string' && param.pattern) {
    const regex = new RegExp(param.pattern)
    if (!regex.test(String(param.value))) {
      issues.push({
        path: param.path,
        message: `${param.name} does not match pattern: ${param.pattern}`,
        severity: 'error',
        rule: 'pattern',
      })
    }
  }

  if (param.type === 'boolean' && typeof param.value !== 'boolean') {
    issues.push({
      path: param.path,
      message: `${param.name} must be a boolean`,
      severity: 'error',
      rule: 'type-boolean',
    })
  }

  return issues
}

export function validateConfig(config: Record<string, any>): ValidationIssue[] {
  const issues: ValidationIssue[] = []

  for (const rule of CROSS_PARAM_RULES) {
    const issue = rule.check(config)
    if (issue) issues.push(issue)
  }

  return issues
}

export function validateAll(
  params: ParamMeta[],
  config: Record<string, any>,
): ValidationIssue[] {
  const paramIssues = params.flatMap(validateParam)
  const configIssues = validateConfig(config)
  return [...paramIssues, ...configIssues]
}

export function getIssuesByPath(issues: ValidationIssue[], path: string): ValidationIssue[] {
  return issues.filter((issue) => issue.path === path || issue.path.startsWith(path + '.'))
}

export function hasErrors(issues: ValidationIssue[]): boolean {
  return issues.some((i) => i.severity === 'error')
}

export function countBySeverity(issues: ValidationIssue[], severity: ValidationIssue['severity']): number {
  return issues.filter((i) => i.severity === severity).length
}
