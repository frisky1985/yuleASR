/**
 * Default AUTOSAR BSW module definitions
 * Used when no chip is selected or as a baseline module set
 */

export interface DefaultModule {
  name: string
  label: string
  layer: 'MCAL' | 'ECUAL' | 'Services' | 'OS' | 'RTE' | 'Integration'
  required: boolean
  description: string
}

export const DEFAULT_BSW_MODULES: DefaultModule[] = [
  // MCAL Layer
  { name: 'Mcu', label: 'MCU Driver', layer: 'MCAL', required: true, description: 'Microcontroller unit driver' },
  { name: 'Port', label: 'Port Driver', layer: 'MCAL', required: true, description: 'I/O port configuration' },
  { name: 'Dio', label: 'DIO Driver', layer: 'MCAL', required: false, description: 'Digital I/O driver' },
  { name: 'Gpt', label: 'GPT Driver', layer: 'MCAL', required: false, description: 'General purpose timer' },
  { name: 'Can', label: 'CAN Driver', layer: 'MCAL', required: false, description: 'CAN controller driver' },
  { name: 'Lin', label: 'LIN Driver', layer: 'MCAL', required: false, description: 'LIN controller driver' },
  { name: 'Spi', label: 'SPI Driver', layer: 'MCAL', required: false, description: 'SPI controller driver' },
  { name: 'Adc', label: 'ADC Driver', layer: 'MCAL', required: false, description: 'Analog-to-digital converter' },
  { name: 'Pwm', label: 'PWM Driver', layer: 'MCAL', required: false, description: 'Pulse width modulation' },
  { name: 'Icu', label: 'ICU Driver', layer: 'MCAL', required: false, description: 'Input capture unit' },
  { name: 'Wdg', label: 'Watchdog Driver', layer: 'MCAL', required: false, description: 'Watchdog timer driver' },
  { name: 'Fls', label: 'Flash Driver', layer: 'MCAL', required: false, description: 'Flash memory driver' },
  { name: 'Eep', label: 'EEPROM Driver', layer: 'MCAL', required: false, description: 'EEPROM driver' },
  { name: 'Eth', label: 'Ethernet Driver', layer: 'MCAL', required: false, description: 'Ethernet MAC driver' },

  // ECUAL Layer
  { name: 'CanIf', label: 'CAN Interface', layer: 'ECUAL', required: false, description: 'CAN interface layer' },
  { name: 'CanTp', label: 'CAN Transport', layer: 'ECUAL', required: false, description: 'CAN transport protocol' },
  { name: 'LinIf', label: 'LIN Interface', layer: 'ECUAL', required: false, description: 'LIN interface layer' },
  { name: 'SpiIf', label: 'SPI Interface', layer: 'ECUAL', required: false, description: 'SPI interface layer' },
  { name: 'IoHwAb', label: 'I/O Hardware Abstraction', layer: 'ECUAL', required: false, description: 'I/O hardware abstraction' },
  { name: 'PduR', label: 'PDU Router', layer: 'ECUAL', required: true, description: 'PDU routing layer' },
  { name: 'MemIf', label: 'Memory Interface', layer: 'ECUAL', required: false, description: 'Memory abstraction interface' },
  { name: 'Fee', label: 'Flash EEPROM Emulation', layer: 'ECUAL', required: false, description: 'Flash EEPROM emulation' },
  { name: 'Ea', label: 'EEPROM Abstraction', layer: 'ECUAL', required: false, description: 'EEPROM abstraction' },
  { name: 'EthIf', label: 'Ethernet Interface', layer: 'ECUAL', required: false, description: 'Ethernet interface' },

  // Services Layer
  { name: 'Com', label: 'COM Signal', layer: 'Services', required: true, description: 'Communication signal handling' },
  { name: 'NvM', label: 'NVRAM Manager', layer: 'Services', required: true, description: 'Non-volatile memory manager' },
  { name: 'Dcm', label: 'Diagnostic Communication', layer: 'Services', required: false, description: 'Diagnostic communication manager' },
  { name: 'Dem', label: 'Diagnostic Event Manager', layer: 'Services', required: false, description: 'Diagnostic event memory' },
  { name: 'Det', label: 'Development Error Tracer', layer: 'Services', required: false, description: 'Development error tracer' },
  { name: 'FiM', label: 'Function Inhibition Manager', layer: 'Services', required: false, description: 'Function inhibition manager' },
  { name: 'E2E', label: 'End-to-End Protection', layer: 'Services', required: false, description: 'End-to-End data protection' },
  { name: 'Crc', label: 'CRC Calculation', layer: 'Services', required: false, description: 'CRC calculation library' },

  // OS Layer
  { name: 'Os', label: 'Operating System', layer: 'OS', required: true, description: 'AUTOSAR OS (FreeRTOS)' },

  // Integration Layer
  { name: 'BswM', label: 'BSW Mode Manager', layer: 'Integration', required: false, description: 'BSW mode manager' },
  { name: 'EcuM', label: 'ECU State Manager', layer: 'Integration', required: true, description: 'ECU state manager' },
]

export function getDefaultModulesByLayer(layer: string): DefaultModule[] {
  return DEFAULT_BSW_MODULES.filter((m) => m.layer === layer)
}

export function getRequiredModules(): DefaultModule[] {
  return DEFAULT_BSW_MODULES.filter((m) => m.required)
}

export interface BswModuleOption {
  name: string
  label: string
  required: boolean
  description?: string
  default_params?: Record<string, any>
  dependencies?: string[]
}

/** Full default BSW set (all layers) as selectable options. */
export function defaultBswModuleOptions(): BswModuleOption[] {
  return DEFAULT_BSW_MODULES.map((m) => ({
    name: m.name,
    label: m.label,
    required: m.required,
    description: m.description,
    default_params: {},
    dependencies: [],
  }))
}

/**
 * Build the complete BSW module set for an ECU:
 * chip-specific MCAL drivers plus all chip-independent upper layers
 * (ECUAL / Services / OS / RTE / Integration).
 */
export function buildBswModuleOptions(chipModules: BswModuleOption[]): BswModuleOption[] {
  // Chip database has no MCAL entry for this model: fall back to the default set
  if (chipModules.length === 0) return defaultBswModuleOptions()

  const seen = new Set(chipModules.map((m) => m.name))
  const upperLayers: BswModuleOption[] = DEFAULT_BSW_MODULES
    .filter((m) => m.layer !== 'MCAL' && !seen.has(m.name))
    .map((m) => ({
      name: m.name,
      label: m.label,
      required: m.required,
      description: m.description,
      default_params: {},
      dependencies: [],
    }))
  return [...chipModules, ...upperLayers]
}
