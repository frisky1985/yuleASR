# DIO (Digital Input/Output) Module
详细设计文档见 [Dio 设计文档](../design/modules/mcal/dio-design.md)。

## Overview

The DIO module provides standardized access to the microcontroller's General Purpose Input/Output (GPIO) hardware for the AUTOSAR Basic Software. It abstracts the microcontroller-specific GPIO hardware and provides a uniform interface for application software to perform digital input/output operations.

**AUTOSAR Standard**: Classic Platform 4.4.0  
**Layer**: MCAL (Microcontroller Driver)  
**Hardware**: NXP S32K3 / Infineon TC3xx / STM32 / i.MX8M Mini  
**ASIL Level**: QM (Quality Management)

## Features

- **Channel Operations**: Read/write individual GPIO pins
- **Port Operations**: Read/write entire GPIO ports (up to 32 bits)
- **Channel Group Operations**: Read/write subsets of adjacent bits within a port
- **Channel Flip**: Toggle channel level (0→1 or 1→0)
- **Masked Write**: Write specific bits of a port without affecting others
- **Multi-Port Support**: Supports up to 8 ports with 32 channels each
- **Runtime Configuration**: Configurable initialization with different configurations

## Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│    (LED Control, Button Reading)    │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         DIO Driver (MCAL)           │
│  ┌─────────┐ ┌─────────┐           │
│  │ Port 0  │ │ Port 1  │ ...       │
│  │ Ch 0-31 │ │ Ch 0-31 │           │
│  └────┬────┘ └────┬────┘           │
│       └─────────────┘               │
│              │                      │
│  ┌───────────▼────────────┐         │
│  │   GPIO Hardware Unit   │         │
│  │  (Microcontroller GPIO)│         │
│  └────────────────────────┘         │
└─────────────────────────────────────┘
```

## API Reference

### Core Functions

```c
/* Initialization */
void Dio_Init(const Dio_ConfigType* ConfigPtr);
void Dio_DeInit(void);

/* Channel Operations */
Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId);
void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level);

/* Port Operations */
Dio_PortLevelType Dio_ReadPort(Dio_PortType PortId);
void Dio_WritePort(Dio_PortType PortId, Dio_PortLevelType Level);
void Dio_MaskedWritePort(Dio_PortType PortId, Dio_PortLevelType Level, Dio_PortLevelType Mask);

/* Channel Group Operations */
Dio_PortLevelType Dio_ReadChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr);
void Dio_WriteChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr, Dio_PortLevelType Level);

/* Channel Flip (if DIO_FLIP_CHANNEL_API == STD_ON) */
Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId);

/* Version Information (if DIO_VERSION_INFO_API == STD_ON) */
void Dio_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

### Data Types

```c
/* Channel Types */
typedef uint16 Dio_ChannelType;          /* Channel identifier (Port << 8 | Pin) */
typedef uint8  Dio_PortType;             /* Port identifier (0-7) */
typedef uint32 Dio_PortLevelType;        /* Port value (up to 32 bits) */

/* Level Types */
typedef enum {
    STD_LOW = 0,                /* Physical state 0V */
    STD_HIGH = 1                /* Physical state 5V or 3.3V */
} Dio_LevelType;

/* Channel Group Structure */
typedef struct {
    Dio_PortType port;          /* Port on which the channel group is defined */
    uint8 offset;               /* Position of the channel group on the port */
    Dio_PortLevelType mask;     /* Mask defining the position of the channel group */
} Dio_ChannelGroupType;
```

### Channel Encoding

Channels are encoded as a 16-bit value: `(Port << 8) | Pin`

| Channel | Value | Description |
|---------|-------|-------------|
| DIO_CHANNEL_A0 | 0x0000 | Port A, Pin 0 |
| DIO_CHANNEL_A7 | 0x0007 | Port A, Pin 7 |
| DIO_CHANNEL_B0 | 0x0100 | Port B, Pin 0 |
| DIO_CHANNEL_B7 | 0x0107 | Port B, Pin 7 |
| DIO_CHANNEL_C0 | 0x0200 | Port C, Pin 0 |
| DIO_CHANNEL_C7 | 0x0207 | Port C, Pin 7 |

## Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `DIO_DEV_ERROR_DETECT` | STD_ON/OFF | Enable development error detection |
| `DIO_VERSION_INFO_API` | STD_ON/OFF | Enable version information API |
| `DIO_FLIP_CHANNEL_API` | STD_ON/OFF | Enable channel flip API |
| `DIO_MASKED_WRITE_PORT_API` | STD_ON/OFF | Enable masked write port API |
| `DIO_NUM_PORTS` | uint8 | Number of ports (max 8) |
| `DIO_NUM_CHANNELS_PER_PORT` | uint8 | Channels per port (max 32) |
| `DIO_NUM_CHANNEL_GROUPS` | uint8 | Number of channel groups |

## Usage Examples

### Basic Channel Operations

```c
#include "Dio.h"

void Dio_BasicExample(void)
{
    Dio_LevelType level;
    
    /* Initialize DIO driver */
    Dio_Init(&Dio_Config);
    
    /* Write HIGH to Port A, Pin 0 */
    Dio_WriteChannel(DIO_CHANNEL_A0, STD_HIGH);
    
    /* Write LOW to Port A, Pin 1 */
    Dio_WriteChannel(DIO_CHANNEL_A1, STD_LOW);
    
    /* Read from Port A, Pin 2 */
    level = Dio_ReadChannel(DIO_CHANNEL_A2);
    if (level == STD_HIGH) {
        /* Pin is HIGH */
    }
    
    /* Deinitialize */
    Dio_DeInit();
}
```

### Port Operations

```c
void Dio_PortExample(void)
{
    Dio_PortLevelType portValue;
    
    Dio_Init(&Dio_Config);
    
    /* Write pattern to entire Port A */
    Dio_WritePort(DIO_PORT_A, 0xAA);  /* 10101010 pattern */
    
    /* Read entire Port B */
    portValue = Dio_ReadPort(DIO_PORT_B);
    
    /* Masked write - only modify bits 0-3 */
    Dio_MaskedWritePort(DIO_PORT_A, 0x0F, 0x0F);
    
    Dio_DeInit();
}
```

### Channel Group Operations

```c
void Dio_ChannelGroupExample(void)
{
    Dio_ChannelGroupType ledGroup;
    Dio_PortLevelType groupValue;
    
    Dio_Init(&Dio_Config);
    
    /* Define channel group: Port A, bits 4-7 (4 LEDs) */
    ledGroup.port = DIO_PORT_A;
    ledGroup.offset = 4;
    ledGroup.mask = 0xF0;  /* Bits 4,5,6,7 */
    
    /* Write pattern to LED group (4-bit value) */
    Dio_WriteChannelGroup(&ledGroup, 0x0A);  /* Binary: 1010 */
    
    /* Read current LED pattern */
    groupValue = Dio_ReadChannelGroup(&ledGroup);
    
    Dio_DeInit();
}
```

### LED Blinking with Flip

```c
void Dio_LedBlinkExample(void)
{
    uint32 i;
    
    Dio_Init(&Dio_Config);
    
    /* Blink LED 10 times */
    for (i = 0; i < 10; i++) {
        /* Flip LED state */
        Dio_FlipChannel(DIO_CHANNEL_A0);
        
        /* Simple delay (not for production use) */
        Delay_ms(500);
    }
    
    Dio_DeInit();
}
```

### Button Reading with Debounce

```c
boolean Dio_ReadButtonExample(void)
{
    Dio_LevelType buttonState;
    uint8 debounceCount = 0;
    uint8 i;
    
    Dio_Init(&Dio_Config);
    
    /* Simple debounce: read 5 times */
    for (i = 0; i < 5; i++) {
        buttonState = Dio_ReadChannel(DIO_CHANNEL_B0);
        if (buttonState == STD_LOW) {  /* Active low button */
            debounceCount++;
        }
        Delay_ms(1);
    }
    
    Dio_DeInit();
    
    return (debounceCount >= 3);
}
```

## Error Handling

The DIO module reports errors through the DET (Default Error Tracer):

| Error Code | Description | Detection |
|------------|-------------|-----------|
| `DIO_E_UNINIT` | Driver not initialized | API call check |
| `DIO_E_PARAM_INVALID_CHANNEL_ID` | Invalid channel ID | Parameter validation |
| `DIO_E_PARAM_INVALID_PORT_ID` | Invalid port ID | Parameter validation |
| `DIO_E_PARAM_INVALID_GROUP` | Invalid channel group | Parameter validation |
| `DIO_E_PARAM_POINTER` | NULL pointer | Parameter validation |
| `DIO_E_PARAM_CONFIG` | Invalid configuration pointer | Init validation |

## Hardware Requirements

### Supported Microcontrollers
- NXP S32K3xx (GPIO)
- Infineon AURIX TC3xx (GPIO)
- STM32H7 (GPIO)
- Renesas RH850/U2A (GPIO)
- i.MX8M Mini (GPIO1-GPIO5)

### Resource Usage

| Resource | Typical Usage |
|----------|---------------|
| RAM | ~100 bytes (config dependent) |
| ROM | ~2-4 KB |
| Interrupts | None (polling-based) |

## Dependencies

### Required Modules
- `Std_Types` - Standard types and macros
- `Platform_Types` - Platform-specific types
- `Compiler` - Compiler abstraction
- `Det` - Default Error Tracer (debug builds)

### Optional Modules
- `Mcu` - Clock configuration
- `Port` - Pin multiplexing configuration

## Hardware Registers

For i.MX8M Mini implementation:

| Register | Offset | Description |
|----------|--------|-------------|
| GPIO_DR | 0x00 | Data Register |
| GPIO_GDIR | 0x04 | Direction Register |
| GPIO_PSR | 0x08 | Pad Status Register |
| GPIO_ICR1 | 0x0C | Interrupt Control 1 |
| GPIO_IMR | 0x14 | Interrupt Mask Register |

## Testing

The DIO module includes comprehensive unit tests covering:

- Initialization and deinitialization
- Channel read/write operations
- Port read/write operations
- Channel group read/write operations
- Channel flip functionality
- Masked write operations
- Error handling (invalid parameters, uninitialized state)
- Boundary conditions
- Combined operations

Run tests with:
```bash
cd /home/admin/yuleASR/tests/unit/autosar/mcal
gcc -I../../../../src/bsw/mcal/dio/include test_dio.c -o test_dio
./test_dio
```

**Test Coverage**: 80%+

## References

- AUTOSAR SWS DIO Driver (Classic Platform 4.4.0)
- ISO 26262 (Functional Safety, if ASIL-rated)
- i.MX8M Mini Reference Manual (GPIO chapter)
- NXP S32K3xx Reference Manual
- Infineon TC3xx User Manual

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2026-04-14 | Initial release |
| 1.1.0 | 2026-05-15 | Added unit tests and documentation |

## Example Project Structure

```
project/
├── src/
│   ├── Dio.c          # Driver implementation
│   └── Dio_Cfg.c      # Configuration
├── include/
│   ├── Dio.h          # Public header
│   └── Dio_Cfg.h      # Configuration header
└── tests/
    └── test_dio.c     # Unit tests
```

## Notes

1. **Port Configuration**: DIO driver does not configure pin direction. Use the Port driver for pin direction setup.

2. **Thread Safety**: DIO operations are not inherently thread-safe. Use appropriate protection mechanisms in multi-threaded environments.

3. **Atomicity**: Port write operations are atomic for the whole port. Channel operations use read-modify-write sequences.

4. **Power Management**: The DIO driver does not handle power management. Use the MCU driver for low-power transitions.

5. **Memory Mapping**: Hardware register access uses memory-mapped I/O. Ensure proper MMU/MPU configuration.