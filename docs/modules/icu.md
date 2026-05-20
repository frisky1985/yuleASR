# ICU (Input Capture Unit) Module

## Overview

The ICU (Input Capture Unit) module provides standardized access to the microcontroller's Input Capture hardware for the AUTOSAR Basic Software. It supports various measurement modes including edge detection, timestamp capture, edge counting, and signal measurement (period, pulse width, duty cycle).

**AUTOSAR Standard**: Classic Platform 4.4.0  
**Layer**: MCAL (Microcontroller Driver)  
**Hardware**: NXP S32K3xx (eMIOS)  
**ASIL Level**: QM (Quality Management)

## Features

- **Signal Edge Detection**: Detect rising, falling, or both edges on input signals
- **Timestamp Capture**: Capture timestamps of input signal edges with linear or circular buffers
- **Edge Counting**: Count the number of edges on input signals
- **Signal Measurement**: Measure period time, high time, low time, and duty cycle
- **Wakeup Support**: Configurable wakeup functionality for sleep mode
- **Notification Callbacks**: User-defined callbacks on edge detection events
- **Multi-Channel Support**: Supports up to 24 channels across 2 eMIOS instances
- **Mode Management**: Normal and Sleep operation modes

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│         (Speed Measurement, RPM Calculation)                │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│                   ICU Driver (MCAL)                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Channel 0  │  │   Channel 1  │  │   Channel N  │ ...  │
│  │ Edge Detect  │  │  Timestamp   │  │Signal Measure│      │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
│         └─────────────────┴─────────────────┘               │
│                            │                                │
│  ┌─────────────────────────▼─────────────────────────┐      │
│  │              eMIOS Hardware Units                 │      │
│  │  (Enhanced Modular Input/Output Subsystem)        │      │
│  │  • eMIOS_0: Channels 0-11                        │      │
│  │  • eMIOS_1: Channels 12-23                       │      │
│  └───────────────────────────────────────────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

## API Reference

### Core Functions

```c
/* Initialization */
void Icu_Init(const Icu_ConfigType* ConfigPtr);
void Icu_DeInit(void);

/* Mode Management */
void Icu_SetMode(Icu_ModeType Mode);

/* Wakeup Control */
void Icu_EnableWakeup(Icu_ChannelType Channel);
void Icu_DisableWakeup(Icu_ChannelType Channel);
Std_ReturnType Icu_CheckWakeup(uint32 WakeupSource);

/* Edge Detection */
void Icu_SetActivationCondition(Icu_ChannelType Channel, Icu_ActivationType Activation);
Icu_InputStateType Icu_GetInputState(Icu_ChannelType Channel);

/* Notification */
void Icu_EnableNotification(Icu_ChannelType Channel);
void Icu_DisableNotification(Icu_ChannelType Channel);

/* Timestamp */
void Icu_StartTimestamp(Icu_ChannelType Channel, uint32* BufferPtr, uint16 BufferSize, uint16 NotifyInterval);
void Icu_StopTimestamp(Icu_ChannelType Channel);
Icu_IndexType Icu_GetTimestampIndex(Icu_ChannelType Channel);

/* Edge Counting */
void Icu_EnableEdgeCount(Icu_ChannelType Channel);
void Icu_DisableEdgeCount(Icu_ChannelType Channel);
void Icu_ResetEdgeCount(Icu_ChannelType Channel);
uint16 Icu_GetEdgeNumbers(Icu_ChannelType Channel);

/* Signal Measurement */
void Icu_StartSignalMeasurement(Icu_ChannelType Channel, Icu_SignalMeasurementPropertyType MeasureKind);
void Icu_StopSignalMeasurement(Icu_ChannelType Channel);
uint16 Icu_GetTimeElapsed(Icu_ChannelType Channel);
void Icu_GetDutyCycleValues(Icu_ChannelType Channel, Icu_DutyCycleType* DutyCycleValues);

/* Utility */
uint8 Icu_GetInputLevel(Icu_ChannelType Channel);
uint32 Icu_GetSysTimestamp(void);
void Icu_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

### Data Types

```c
/* Channel Types */
typedef uint8 Icu_ChannelType;           /* Channel identifier (0-23) */
typedef uint16 Icu_IndexType;            /* Buffer index type */

/* Input State */
typedef enum {
    ICU_ACTIVE = 0,              /* Edge detected */
    ICU_IDLE                     /* No edge detected */
} Icu_InputStateType;

/* Activation/Edge Type */
typedef enum {
    ICU_FALLING_EDGE = 0,        /* Detect falling edges */
    ICU_RISING_EDGE,             /* Detect rising edges */
    ICU_BOTH_EDGES               /* Detect both edges */
} Icu_ActivationType;

/* Operation Mode */
typedef enum {
    ICU_MODE_NORMAL = 0,         /* Normal operation */
    ICU_MODE_SLEEP               /* Sleep mode (low power) */
} Icu_ModeType;

/* Measurement Mode */
typedef enum {
    ICU_MODE_SIGNAL_EDGE_DETECT = 0,  /* Simple edge detection */
    ICU_MODE_SIGNAL_MEASUREMENT,      /* Period/duty measurement */
    ICU_MODE_TIMESTAMP,               /* Timestamp capture */
    ICU_MODE_EDGE_COUNTER             /* Edge counting */
} Icu_MeasurementModeType;

/* Signal Measurement Property */
typedef enum {
    ICU_PERIOD_TIME = 0,         /* Measure period */
    ICU_HIGH_TIME,               /* Measure high time (pulse width) */
    ICU_LOW_TIME,                /* Measure low time */
    ICU_DUTY_CYCLE               /* Measure duty cycle */
} Icu_SignalMeasurementPropertyType;

/* Timestamp Buffer Type */
typedef enum {
    ICU_LINEAR_BUFFER = 0,       /* Stop when buffer full */
    ICU_CIRCULAR_BUFFER          /* Wrap around when buffer full */
} Icu_TimestampBufferType;

/* Duty Cycle Structure */
typedef struct {
    uint16 ActiveTime;           /* High time / Active time */
    uint16 PeriodTime;           /* Total period */
} Icu_DutyCycleType;

/* Channel Configuration */
typedef struct {
    Icu_ChannelType ChannelId;
    uint32 BaseAddress;
    Icu_MeasurementModeType MeasurementMode;
    Icu_ActivationType DefaultActivation;
    Icu_SignalMeasurementPropertyType SignalMeasurementProperty;
    Icu_TimestampBufferType TimestampBufferType;
    uint16 BufferSize;
    uint32* BufferPtr;
    boolean WakeupSupport;
    boolean NotificationEnabled;
    void (*NotificationFn)(void);
    uint32 ClockPrescaler;
} Icu_ChannelConfigType;

/* Driver Configuration */
typedef struct {
    const Icu_ChannelConfigType* Channels;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean WakeupFunctionalityApi;
    boolean DeInitApi;
    boolean SetModeApi;
    boolean DisableWakeupApi;
    boolean EnableWakeupApi;
    boolean CheckWakeupApi;
    boolean TimestampApi;
    boolean EdgeCountApi;
    boolean SignalMeasurementApi;
    Icu_ModeType DefaultMode;
} Icu_ConfigType;
```

### Channel Mapping

| Channel | eMIOS Instance | Channel Number | Hardware Pin |
|---------|---------------|----------------|--------------|
| 0-11 | eMIOS_0 | 0-11 | PTA0-PTA11 |
| 12-23 | eMIOS_1 | 0-11 | PTB0-PTB11 |

## Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `ICU_DEV_ERROR_DETECT` | STD_ON/OFF | Enable development error detection |
| `ICU_VERSION_INFO_API` | STD_ON/OFF | Enable version information API |
| `ICU_DE_INIT_API` | STD_ON/OFF | Enable deinitialization API |
| `ICU_SET_MODE_API` | STD_ON/OFF | Enable mode setting API |
| `ICU_DISABLE_WAKEUP_API` | STD_ON/OFF | Enable wakeup disable API |
| `ICU_ENABLE_WAKEUP_API` | STD_ON/OFF | Enable wakeup enable API |
| `ICU_CHECK_WAKEUP_API` | STD_ON/OFF | Enable wakeup check API |
| `ICU_TIMESTAMP_API` | STD_ON/OFF | Enable timestamp API |
| `ICU_EDGE_COUNT_API` | STD_ON/OFF | Enable edge counting API |
| `ICU_SIGNAL_MEASUREMENT_API` | STD_ON/OFF | Enable signal measurement API |
| `ICU_NUM_CHANNELS` | uint8 | Number of configured channels |
| `ICU_EMIOS_0_BASE_ADDR` | uint32 | eMIOS_0 base address |
| `ICU_EMIOS_1_BASE_ADDR` | uint32 | eMIOS_1 base address |

## Usage Examples

### Basic Initialization

```c
#include "Icu.h"

/* Notification callback */
void IcuEdgeNotification(void)
{
    /* Handle edge detection */
}

/* Channel configuration */
const Icu_ChannelConfigType Icu_Channels[] = {
    {
        .ChannelId = 0,
        .MeasurementMode = ICU_MODE_SIGNAL_EDGE_DETECT,
        .DefaultActivation = ICU_RISING_EDGE,
        .NotificationEnabled = TRUE,
        .NotificationFn = IcuEdgeNotification,
        .WakeupSupport = FALSE
    }
};

const Icu_ConfigType Icu_Config = {
    .Channels = Icu_Channels,
    .NumChannels = 1,
    .DefaultMode = ICU_MODE_NORMAL
};

void Icu_BasicExample(void)
{
    /* Initialize ICU driver */
    Icu_Init(&Icu_Config);
    
    /* Enable notification */
    Icu_EnableNotification(0);
    
    /* Application code... */
    
    /* Deinitialize */
    Icu_DeInit();
}
```

### Period Measurement

```c
void Icu_PeriodMeasurementExample(void)
{
    /* Configure channel for period measurement */
    Icu_ChannelConfigType chConfig = {
        .ChannelId = 1,
        .MeasurementMode = ICU_MODE_SIGNAL_MEASUREMENT,
        .SignalMeasurementProperty = ICU_PERIOD_TIME,
        .DefaultActivation = ICU_RISING_EDGE,
        .ClockPrescaler = 1
    };
    
    Icu_Init(&Icu_Config);
    
    /* Start measurement */
    Icu_StartSignalMeasurement(1, ICU_PERIOD_TIME);
    
    /* Wait for measurement... */
    
    /* Get result */
    uint16 period = Icu_GetTimeElapsed(1);
    /* period contains the measured period in timer ticks */
    
    Icu_StopSignalMeasurement(1);
    Icu_DeInit();
}
```

### Duty Cycle Measurement

```c
void Icu_DutyCycleExample(void)
{
    Icu_DutyCycleType dutyCycle;
    
    Icu_Init(&Icu_Config);
    
    /* Start duty cycle measurement */
    Icu_StartSignalMeasurement(2, ICU_DUTY_CYCLE);
    
    /* Wait for at least 2 edges... */
    
    /* Get duty cycle values */
    Icu_GetDutyCycleValues(2, &dutyCycle);
    
    /* dutyCycle.PeriodTime - Total period in ticks */
    /* dutyCycle.ActiveTime - High time in ticks */
    /* Duty Cycle % = (ActiveTime * 100) / PeriodTime */
    
    uint16 dutyPercent = (dutyCycle.ActiveTime * 100) / dutyCycle.PeriodTime;
    
    Icu_StopSignalMeasurement(2);
    Icu_DeInit();
}
```

### Timestamp Capture

```c
#define TIMESTAMP_BUFFER_SIZE 100

void Icu_TimestampExample(void)
{
    uint32 timestampBuffer[TIMESTAMP_BUFFER_SIZE];
    Icu_IndexType index;
    
    Icu_Init(&Icu_Config);
    
    /* Start timestamp capture with notification every 10 captures */
    Icu_StartTimestamp(3, timestampBuffer, TIMESTAMP_BUFFER_SIZE, 10);
    
    /* Wait for captures... */
    
    /* Get current index */
    index = Icu_GetTimestampIndex(3);
    
    /* Process captured timestamps */
    for (Icu_IndexType i = 0; i < index; i++) {
        /* Process timestampBuffer[i] */
    }
    
    Icu_StopTimestamp(3);
    Icu_DeInit();
}
```

### Edge Counting

```c
void Icu_EdgeCountExample(void)
{
    uint16 count;
    
    Icu_Init(&Icu_Config);
    
    /* Enable edge counting on channel 4 */
    Icu_EnableEdgeCount(4);
    
    /* Reset counter */
    Icu_ResetEdgeCount(4);
    
    /* Wait for edges... */
    
    /* Get edge count */
    count = Icu_GetEdgeNumbers(4);
    
    Icu_DisableEdgeCount(4);
    Icu_DeInit();
}
```

### Wakeup Functionality

```c
void Icu_WakeupExample(void)
{
    Std_ReturnType result;
    
    Icu_Init(&Icu_Config);
    
    /* Enable wakeup on channel 5 */
    Icu_EnableWakeup(5);
    
    /* Enter sleep mode */
    Icu_SetMode(ICU_MODE_SLEEP);
    
    /* In sleep mode... */
    
    /* Check for wakeup event */
    result = Icu_CheckWakeup(ICU_WAKEUP_SOURCE);
    if (result == E_OK) {
        /* Wakeup detected */
        Icu_SetMode(ICU_MODE_NORMAL);
    }
    
    Icu_DeInit();
}
```

### RPM Calculation (Automotive Application)

```c
#define PULSES_PER_REVOLUTION 60
#define TIMER_TICK_FREQ 8000000  /* 8 MHz */

uint16 CalculateRPM(Icu_ChannelType channel)
{
    uint16 periodTicks;
    uint32 periodUs;
    uint16 rpm;
    
    /* Get measured period */
    periodTicks = Icu_GetTimeElapsed(channel);
    
    if (periodTicks == 0) {
        return 0;  /* No signal */
    }
    
    /* Convert ticks to microseconds */
    periodUs = ((uint32)periodTicks * 1000000) / TIMER_TICK_FREQ;
    
    /* Calculate RPM: 60,000,000 / (period_us * pulses_per_rev) */
    rpm = (uint16)(60000000UL / ((uint32)periodUs * PULSES_PER_REVOLUTION));
    
    return rpm;
}

void Icu_RpmMeasurementExample(void)
{
    uint16 rpm;
    
    Icu_Init(&Icu_Config);
    Icu_StartSignalMeasurement(0, ICU_PERIOD_TIME);
    
    /* In main loop or notification callback */
    rpm = CalculateRPM(0);
    
    Icu_StopSignalMeasurement(0);
    Icu_DeInit();
}
```

## Error Handling

The ICU module reports errors through the DET (Default Error Tracer):

| Error Code | Description | Detection |
|------------|-------------|-----------|
| `ICU_E_UNINIT` | Driver not initialized | API call check |
| `ICU_E_PARAM_CHANNEL` | Invalid channel ID | Parameter validation |
| `ICU_E_PARAM_ACTIVATION` | Invalid activation type | Parameter validation |
| `ICU_E_PARAM_BUFFER_SIZE` | Invalid buffer size | Parameter validation |
| `ICU_E_PARAM_POINTER` | NULL pointer | Parameter validation |
| `ICU_E_ALREADY_INITIALIZED` | Double initialization | Init validation |
| `ICU_E_BUSY` | Channel already running | Start API validation |
| `ICU_E_MEASUREMENT_RUNNING` | Measurement already running | Start validation |
| `ICU_E_MEASUREMENT_NOT_RUNNING` | Measurement not running | Stop validation |
| `ICU_E_STAMP_NOT_RUNNING` | Timestamp not running | Index get validation |
| `ICU_E_EDGE_COUNTING_NOT_RUNNING` | Edge counting not running | Get numbers validation |

## Hardware Requirements

### Supported Microcontrollers
- NXP S32K3xx (eMIOS)
- Similar eMIOS-based microcontrollers

### eMIOS Registers

| Register | Offset | Description |
|----------|--------|-------------|
| EMIOS_MCR | 0x0000 | Module Configuration Register |
| EMIOS_GFR | 0x0004 | Global Flag Register |
| EMIOS_C_A | 0x0000 | Channel A Register (Capture) |
| EMIOS_C_B | 0x0004 | Channel B Register |
| EMIOS_C_CNT | 0x0008 | Channel Counter Register |
| EMIOS_C_C | 0x000C | Channel Control Register |
| EMIOS_C_S | 0x0010 | Channel Status Register |

### Resource Usage

| Resource | Typical Usage |
|----------|---------------|
| RAM | ~500 bytes (depends on channel count) |
| ROM | ~8-12 KB |
| Interrupts | One per eMIOS instance |

## Dependencies

### Required Modules
- `Std_Types` - Standard types and macros
- `Platform_Types` - Platform-specific types
- `Compiler` - Compiler abstraction
- `Det` - Default Error Tracer (debug builds)

### Optional Modules
- `Mcu` - Clock configuration
- `EcuM` - Wakeup event reporting

## Testing

The ICU module includes comprehensive unit tests covering:

- Initialization and deinitialization
- Mode setting (Normal/Sleep)
- Edge detection configuration (Rising/Falling/Both)
- Wakeup functionality
- Notification enable/disable
- Input state reading
- Timestamp capture (start/stop/index)
- Edge counting (enable/disable/reset/get)
- Signal measurement (period, duty cycle, pulse width)
- Version information
- Error handling (invalid parameters, uninitialized state)
- Boundary conditions

Run tests with:
```bash
cd /home/admin/yuleASR/tests/unit/autosar/mcal
gcc -I../../../../src/bsw/mcal/icu/include test_icu.c -o test_icu
./test_icu
```

**Test Coverage**: 80%+

### Test Results Example

```
================================================================================
                    ICU (Input Capture Unit) Unit Tests                       
================================================================================

[TEST] test_Icu_Init_ValidConfig_ShouldSucceed
  [PASS] det_called == FALSE
  [PASS] Icu_DriverInitialized == TRUE
  [PASS] Icu_DriverMode == ICU_MODE_NORMAL
...

================================================================================
                              Test Summary                                     
================================================================================
  Total Tests:  45
  Passed:       45
  Failed:       0
  Coverage:     100%
================================================================================

  [SUCCESS] All tests passed!
```

## References

- AUTOSAR SWS ICU Driver (Classic Platform 4.4.0)
- NXP S32K3xx Reference Manual (eMIOS chapter)
- ISO 26262 (Functional Safety, if ASIL-rated)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2026-04-30 | Initial release |
| 1.1.0 | 2026-05-15 | Added unit tests and documentation |

## Notes

1. **Clock Configuration**: The ICU driver relies on proper MCU clock configuration. Ensure the eMIOS clock is enabled before calling `Icu_Init()`.

2. **Pin Configuration**: Input pins must be configured using the Port driver before using ICU functionality.

3. **Interrupt Priority**: Configure appropriate interrupt priorities for eMIOS interrupts in the interrupt controller.

4. **Timer Resolution**: The measurement resolution depends on the eMIOS clock frequency and prescaler settings.

5. **Overflow Handling**: The driver handles counter overflow internally for period measurements.

6. **Notification Context**: Notification callbacks are called from interrupt context - keep them short and avoid blocking operations.

7. **Sleep Mode**: When entering sleep mode, ensure wakeup-enabled channels are properly configured.

8. **Buffer Management**: For timestamp capture, ensure the buffer remains valid while capture is active.

## Example Project Structure

```
project/
├── src/
│   ├── Icu.c              # Driver implementation
│   ├── Icu_Irq.c          # Interrupt handlers
│   ├── Icu_Lcfg.c         # Link-time configuration
│   └── Icu_Cfg.c          # Configuration
├── include/
│   ├── Icu.h              # Public header
│   ├── Icu_Private.h      # Private header
│   ├── Icu_Cfg.h          # Configuration header
│   └── Icu_Lcfg.h         # Link configuration header
└── tests/
    └── test_icu.c         # Unit tests
```
