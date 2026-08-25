# ADC (Analog-to-Digital Converter) Module
详细设计文档见 [Adc 设计文档](../design/modules/mcal/adc-design.md)。

## Overview

The ADC module provides standardized access to analog-to-digital converter hardware for the AUTOSAR Basic Software. It abstracts the microcontroller-specific ADC hardware and provides a uniform interface for application software to perform analog signal acquisition.

**AUTOSAR Standard**: Classic Platform 4.4.0  
**Layer**: MCAL (Microcontroller Driver)  
**Hardware**: NXP S32K3 / Infineon TC3xx / Renesas RH850  
**ASIL Level**: QM (Quality Management)

## Features

- **Multi-channel Support**: Supports up to 128 ADC channels
- **Group-based Conversion**: Organizes channels into logical groups
- **Hardware Trigger**: Supports hardware triggers for precise timing
- **Software Trigger**: Software-initiated conversions for diagnostic purposes
- **Continuous Conversion**: Background continuous sampling mode
- **One-shot Conversion**: Single-shot sampling for event-driven acquisition
- **Streaming Access**: Buffer-based data streaming for high-frequency sampling
- **Interrupt/DMA Support**: Configurable notification mechanisms
- **Power Management**: Low-power mode support

## Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│    (Sensor Reading, Monitoring)     │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         ADC Driver (MCAL)           │
│  ┌─────────┐ ┌─────────┐           │
│  │ Group 0 │ │ Group 1 │ ...       │
│  │ Ch 0-7  │ │ Ch 8-15 │           │
│  └────┬────┘ └────┬────┘           │
│       └─────────────┘               │
│              │                      │
│  ┌───────────▼────────────┐         │
│  │   ADC Hardware Unit    │         │
│  │  (Microcontroller ADC) │         │
│  └────────────────────────┘         │
└─────────────────────────────────────┘
```

## API Reference

### Core Functions

```c
/* Initialization and De-initialization */
Std_ReturnType Adc_Init(const Adc_ConfigType* ConfigPtr);
Std_ReturnType Adc_DeInit(void);

/* Group Control */
void Adc_StartGroupConversion(Adc_GroupType Group);
void Adc_StopGroupConversion(Adc_GroupType Group);

/* Data Reading */
Std_ReturnType Adc_ReadGroup(Adc_GroupType Group, Adc_ValueGroupType* DataBufferPtr);

/* Stream Mode */
Std_ReturnType Adc_SetupResultBuffer(Adc_GroupType Group, Adc_ValueGroupType* DataBufferPtr);

/* Notification */
void Adc_EnableGroupNotification(Adc_GroupType Group);
void Adc_DisableGroupNotification(Adc_GroupType Group);

/* Status and Information */
Adc_StatusType Adc_GetGroupStatus(Adc_GroupType Group);
Adc_StreamNumSampleType Adc_GetStreamLastPointer(Adc_GroupType Group);
void Adc_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

### Data Types

```c
typedef uint8 Adc_ChannelType;           /* ADC channel number */
typedef uint8 Adc_GroupType;             /* ADC group number */
typedef uint16 Adc_ValueGroupType;       /* ADC conversion result (12-bit) */
typedef uint8 Adc_PriorityType;          /* Channel priority level */

typedef enum {
    ADC_IDLE = 0,
    ADC_BUSY,
    ADC_STREAM_COMPLETED,
    ADC_STREAM_BUFFER_FULL
} Adc_StatusType;

typedef enum {
    ADC_TRIGG_SRC_SW = 0,
    ADC_TRIGG_SRC_HW
} Adc_TriggerSourceType;

typedef enum {
    ADC_CONV_MODE_ONESHOT = 0,
    ADC_CONV_MODE_CONTINUOUS
} Adc_GroupConvModeType;
```

## Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `AdcChannelId` | uint8 | Unique channel identifier |
| `AdcChannelRange` | enum | Differential/Single-ended |
| `AdcChannelResolution` | enum | 8/10/12-bit resolution |
| `AdcChannelSamplingTime` | uint8 | Sampling time in ADC clock cycles |
| `AdcGroupAccessMode` | enum | Single/Streaming access |
| `AdcGroupConversionMode` | enum | One-shot/Continuous |
| `AdcGroupTriggSrc` | enum | Software/Hardware trigger |
| `AdcStreamingNumSamples` | uint16 | Number of samples in streaming mode |

## Usage Example

```c
#include "Adc.h"
#include "Adc_Cfg.h"

/* Conversion result buffer */
static Adc_ValueGroupType AdcResultBuffer[ADC_GROUP0_CHANNELS];

void Adc_InitExample(void)
{
    Std_ReturnType status;
    
    /* Initialize ADC driver */
    status = Adc_Init(&Adc_Config);
    if (status == E_OK) {
        /* Setup result buffer for Group 0 */
        status = Adc_SetupResultBuffer(ADC_GROUP_0, AdcResultBuffer);
    }
}

void Adc_ReadSensorExample(void)
{
    Std_ReturnType status;
    uint16 temperatureRaw;
    
    /* Start conversion on Group 0 */
    Adc_StartGroupConversion(ADC_GROUP_0);
    
    /* Wait for conversion complete (or use notification) */
    while (Adc_GetGroupStatus(ADC_GROUP_0) == ADC_BUSY) {
        /* Wait */
    }
    
    /* Read conversion results */
    status = Adc_ReadGroup(ADC_GROUP_0, AdcResultBuffer);
    if (status == E_OK) {
        /* Channel 0 contains temperature sensor */
        temperatureRaw = AdcResultBuffer[0];
        
        /* Convert to temperature */
        float temperature = Adc_ConvertToCelsius(temperatureRaw);
    }
}

/* Notification callback for Group 0 */
void AdcNotificationGroup0(void)
{
    /* Conversion complete - process results */
    Adc_ReadGroup(ADC_GROUP_0, AdcResultBuffer);
    
    /* Signal application */
    SensorDataReady = TRUE;
}
```

## Error Handling

The ADC module reports errors through the DET (Default Error Tracer):

| Error Code | Description | Detection |
|------------|-------------|-----------|
| `ADC_E_UNINIT` | Driver not initialized | API call check |
| `ADC_E_ALREADY_INITIALIZED` | Double initialization | Init check |
| `ADC_E_PARAM_GROUP` | Invalid group ID | Parameter validation |
| `ADC_E_PARAM_POINTER` | NULL pointer | Parameter validation |
| `ADC_E_BUFFER_UNINIT` | Buffer not initialized | Read check |
| `ADC_E_IDLE` | Group in idle state | Stop check |
| `ADC_E_BUSY` | Group busy | Start check |

## Hardware Requirements

### Supported Microcontrollers
- NXP S32K3xx (SAR ADC, 12-bit)
- Infineon AURIX TC3xx (VADC, 12-bit)
- Renesas RH850/U2A (ADC, 12-bit)
- STM32H7 (ADC, 16-bit)

### Resource Usage
| Resource | Typical Usage |
|----------|---------------|
| RAM | ~200 bytes (config dependent) |
| ROM | ~5-8 KB |
| Interrupts | 1-2 (per ADC hardware unit) |
| DMA Channels | Optional (1 per streaming group) |

## Dependencies

### Required Modules
- `Std_Types` - Standard types and macros
- `Platform_Types` - Platform-specific types
- `Compiler` - Compiler abstraction
- `Det` - Default Error Tracer (debug builds)

### Optional Modules
- `Mcu` - Clock configuration
- `Port` - Pin multiplexing
- `Dma` - DMA support for streaming mode
- `Icu` - Hardware trigger source

## References

- AUTOSAR SWS ADC Driver (Classic Platform)
- ISO 26262 (Functional Safety, if ASIL-rated)
- NXP S32K3xx Reference Manual (ADC chapter)
- Infineon TC3xx User Manual (VADC)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-05 | Initial release |
| 1.1.0 | 2024-08 | Added streaming mode support |
| 1.2.0 | 2024-12 | Multi-core support added |