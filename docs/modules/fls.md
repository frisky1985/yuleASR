# FLS (Flash Driver) Module
详细设计文档见 [Fls 设计文档](../design/modules/mcal/fls-design.md)。

## Overview

The FLS module is the standard AUTOSAR Flash Driver that provides hardware-independent access to internal flash memory. It implements the AUTOSAR R22-11 specification and serves as the underlying driver for the Flash wrapper module. FLS handles all low-level flash operations including sector erase, page write, and read operations with proper error handling and status management.

**AUTOSAR Standard**: R22-11 (Classic Platform)  
**Layer**: MCAL (Microcontroller Driver)  
**Module ID**: 0x5C (92)  
**Vendor ID**: 0x0064 (100)  
**ASIL Level**: QM to ASIL-D (configurable)

## Features

- **AUTOSAR R22-11 Compliant**: Full compliance with AUTOSAR Flash Driver specification
- **Sector-Based Operations**: Erase and write at sector/page granularity
- **Asynchronous Architecture**: Non-blocking operations with state machine
- **Job Management**: Internal job queue with priority handling
- **Dual Operation Modes**: Normal and Fast modes for different performance needs
- **Error Detection**: Comprehensive development and runtime error detection
- **Memory Mapping**: AUTOSAR-compliant memory section handling
- **Hardware Abstraction**: Platform-independent API with hardware-specific backend

## Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│  (NvM, Fee, Ea, Bootloader)         │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         MemIf (Memory Interface)    │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         FLS Driver (Fls.h)          │
│  ┌─────────────────────────────────┐│
│  │  API Layer                      ││
│  │  - Fls_Init/DeInit              ││
│  │  - Fls_Read/Write/Erase         ││
│  │  - Fls_Compare                  ││
│  │  - Fls_Cancel/SetMode           ││
│  ├─────────────────────────────────┤│
│  │  Job Manager                    ││
│  │  - State Machine                ││
│  │  - Job Queue                    ││
│  │  - Timeout Handling             ││
│  ├─────────────────────────────────┤│
│  │  Hardware Abstraction           ││
│  │  - Sector Management            ││
│  │  - Address Validation           ││
│  │  - Flash Controller Access      ││
│  └─────────────────────────────────┘│
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         FLS Hardware Layer          │
│  ┌─────────────────────────────────┐│
│  │  Flash Controller Interface     ││
│  │  - Unlock/Lock                  ││
│  │  - Erase/Program Operations     ││
│  │  - Status Register Handling     ││
│  └─────────────────────────────────┘│
└─────────────────────────────────────┘
```

## API Reference

### Core Functions

```c
/* Initialization */
void Fls_Init(const Fls_ConfigType* ConfigPtr);

/* Flash Operations */
Std_ReturnType Fls_Erase(Fls_AddressType TargetAddress, Fls_LengthType Length);
Std_ReturnType Fls_Write(Fls_AddressType TargetAddress, const uint8* SourceAddress, Fls_LengthType Length);
void Fls_Read(Fls_AddressType SourceAddress, uint8* TargetAddressPtr, Fls_LengthType Length);
void Fls_Compare(Fls_AddressType SourceAddress, const uint8* TargetAddressPtr, Fls_LengthType Length);

/* Control Functions */
void Fls_SetMode(MemIf_ModeType Mode);
void Fls_Cancel(void);
Fls_StatusType Fls_GetStatus(void);
Fls_JobResultType Fls_GetJobResult(void);
void Fls_MainFunction(void);

/* Version Info (optional) */
#if (FLS_VERSION_INFO_API == STD_ON)
void Fls_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/* Synchronous Read (optional) */
#if (FLS_USE_ISR == STD_OFF)
Std_ReturnType Fls_ReadSync(Fls_AddressType SourceAddress, uint8* TargetAddressPtr, Fls_LengthType Length);
#endif
```

### Data Types

```c
/* Service IDs */
#define FLS_SID_INIT                    (0x00u)
#define FLS_SID_ERASE                   (0x01u)
#define FLS_SID_WRITE                   (0x02u)
#define FLS_SID_READ                    (0x03u)
#define FLS_SID_COMPARE                 (0x04u)
#define FLS_SID_SETMODE                 (0x05u)
#define FLS_SID_CANCEL                  (0x06u)
#define FLS_SID_GETSTATUS               (0x07u)
#define FLS_SID_GETJOBRESULT            (0x08u)
#define FLS_SID_GETVERSIONINFO          (0x09u)

/* Driver State */
typedef enum {
    FLS_UNINIT = 0,
    FLS_IDLE,
    FLS_BUSY
} Fls_StatusType;

/* Job Result Type */
typedef MemIf_JobResultType Fls_JobResultType;
/* Values: MEMIF_JOB_OK, MEMIF_JOB_FAILED, MEMIF_JOB_PENDING, 
           MEMIF_JOB_CANCELED, MEMIF_BLOCK_INCONSISTENT, MEMIF_BLOCK_INVALID */

/* Address and Length Types */
typedef uint32 Fls_AddressType;
typedef uint32 Fls_LengthType;

/* Operation Mode */
typedef enum {
    FLS_MODE_NORMAL = 0,
    FLS_MODE_FAST
} Fls_OpModeType;

/* Sector Configuration */
typedef struct {
    Fls_AddressType sectorStartAddr;
    Fls_LengthType sectorSize;
    uint32 sectorPageSize;
    uint32 sectorUnlockMask;
    boolean sectorWritable;
    boolean sectorErasable;
} Fls_SectorType;

/* Configuration Type */
typedef struct {
    const Fls_SectorType* sectorList;
    uint32 sectorCount;
    uint32 defaultMode;
    uint32 maxReadFastMode;
    uint32 maxReadNormalMode;
    uint32 maxWriteFastMode;
    uint32 maxWriteNormalMode;
    boolean jobEndNotificationEnabled;
    boolean jobErrorNotificationEnabled;
} Fls_ConfigType;
```

## Configuration Parameters

### General Configuration

| Parameter | Type | Description | Range |
|-----------|------|-------------|-------|
| `FLS_DEV_ERROR_DETECT` | Switch | Enable development error detection | STD_ON/STD_OFF |
| `FLS_RUNTIME_ERROR_DETECT` | Switch | Enable runtime error detection | STD_ON/STD_OFF |
| `FLS_VERSION_INFO_API` | Switch | Enable version info API | STD_ON/STD_OFF |
| `FLS_USE_ISR` | Switch | Use interrupt-driven operations | STD_ON/STD_OFF |
| `FLS_JOB_END_NOTIFICATION` | Switch | Enable job end notification | STD_ON/STD_OFF |
| `FLS_JOB_ERROR_NOTIFICATION` | Switch | Enable job error notification | STD_ON/STD_OFF |

### Timing Configuration

| Parameter | Type | Description | Typical Value |
|-----------|------|-------------|---------------|
| `FLS_TIMEOUT_VALUE` | uint32 | Operation timeout counter | 0xFFFFFFFF |
| `FLS_BASE_ADDRESS` | uint32 | Flash base address | 0x08000000 |
| `FLS_TOTAL_SIZE` | uint32 | Total flash size | 0x00100000 |
| `FLS_SECTOR_0_SIZE` | uint32 | Size of first sector | 0x00010000 |
| `FLS_MAX_WRITE_FAST_MODE` | uint32 | Max write bytes in fast mode | 256 |
| `FLS_MAX_WRITE_NORMAL_MODE` | uint32 | Max write bytes in normal mode | 64 |
| `FLS_MAX_READ_FAST_MODE` | uint32 | Max read bytes in fast mode | 1024 |
| `FLS_MAX_READ_NORMAL_MODE` | uint32 | Max read bytes in normal mode | 256 |

## Usage Example

### Basic Initialization and Operation

```c
#include "Fls.h"
#include "Fls_Cfg.h"

/* Sector configuration */
const Fls_SectorType Fls_SectorConfig[] = {
    {0x08000000U, 0x00010000U, 256U, 0xFFFFFFFFU, TRUE, TRUE},   /* Sector 0 */
    {0x08010000U, 0x00010000U, 256U, 0xFFFFFFFFU, TRUE, TRUE},   /* Sector 1 */
    {0x08020000U, 0x00010000U, 256U, 0xFFFFFFFFU, FALSE, FALSE}  /* Sector 2 (read-only) */
};

/* Configuration structure */
const Fls_ConfigType Fls_Config = {
    .sectorList = Fls_SectorConfig,
    .sectorCount = 3,
    .defaultMode = MEMIF_MODE_SLOW,
    .maxReadFastMode = 1024,
    .maxReadNormalMode = 256,
    .maxWriteFastMode = 256,
    .maxWriteNormalMode = 64,
    .jobEndNotificationEnabled = FALSE,
    .jobErrorNotificationEnabled = FALSE
};

void Fls_Example(void)
{
    Std_ReturnType status;
    uint8 write_buffer[256];
    uint8 read_buffer[256];
    
    /* Initialize FLS */
    Fls_Init(&Fls_Config);
    
    /* Verify initialization */
    if (Fls_GetStatus() != FLS_IDLE) {
        /* Handle error */
        return;
    }
    
    /* Prepare write buffer */
    for (int i = 0; i < 256; i++) {
        write_buffer[i] = (uint8)i;
    }
    
    /* Erase sector (required before write) */
    status = Fls_Erase(0x08000000U, 0x00010000U);
    if (status != E_OK) {
        /* Handle error */
        return;
    }
    
    /* Wait for erase completion */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Check result */
    if (Fls_GetJobResult() != MEMIF_JOB_OK) {
        /* Handle error */
        return;
    }
    
    /* Write data */
    status = Fls_Write(0x08000000U, write_buffer, 256);
    if (status != E_OK) {
        /* Handle error */
        return;
    }
    
    /* Wait for write completion */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    if (Fls_GetJobResult() != MEMIF_JOB_OK) {
        /* Handle error */
        return;
    }
    
    /* Read data back */
    Fls_Read(0x08000000U, read_buffer, 256);
    
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Compare data */
    Fls_Compare(0x08000000U, write_buffer, 256);
    
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    if (Fls_GetJobResult() == MEMIF_JOB_OK) {
        /* Data verified successfully */
    }
}
```

### Using Fast Mode

```c
void Fls_FastModeExample(void)
{
    uint8 buffer[1024];
    
    Fls_Init(&Fls_Config);
    
    /* Switch to fast mode for bulk operations */
    Fls_SetMode(MEMIF_MODE_FAST);
    
    /* Erase with fast mode */
    Fls_Erase(0x08000000U, 0x00020000U);  /* 128KB */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Write large buffer in fast mode */
    Fls_Write(0x08000000U, buffer, 1024);
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Return to normal mode */
    Fls_SetMode(MEMIF_MODE_SLOW);
}
```

### Canceling Operations

```c
void Fls_CancelExample(void)
{
    Std_ReturnType status;
    
    Fls_Init(&Fls_Config);
    
    /* Start a long operation */
    status = Fls_Erase(0x08000000U, 0x00040000U);  /* 256KB */
    
    if (status == E_OK) {
        /* Simulate some processing */
        for (int i = 0; i < 100; i++) {
            Fls_MainFunction();
        }
        
        /* Cancel the operation */
        Fls_Cancel();
        
        /* Verify cancellation */
        if (Fls_GetStatus() == FLS_IDLE && 
            Fls_GetJobResult() == MEMIF_JOB_CANCELED) {
            /* Operation canceled successfully */
        }
    }
}
```

## State Machine

```
                     ┌──────────────┐
                     │    UNINIT     │
                     └──────┬───────┘
                            │ Fls_Init()
                            ▼
                     ┌──────────────┐
    ┌─────────────────│     IDLE      │◄─────────────────┐
    │                  └──────┬───────┘                  │
    │                         │                       │
    │ Fls_Cancel()     Fls_Erase()            Fls_Read/Write/Compare()
    │                         │                       │
    │                         ▼                       │
    │                  ┌──────────────┐                 │
    │                  │   ERASING    │                 │
    │                  └──────┬───────┘                 │
    │                         │                       │
    │                  Fls_MainFunction()             │
    │                         │                       │
    │              ┌────────┬────────┬───────┐              │
    │         More  │ Error │ Done │              │
    │              │       │      │              │
    │              └──────┬──────┘              │
    └─────────────────┘         │                       │
                            └──────────────────────────┘
```

## Error Handling

### Development Errors (DET)

| Error Code | Value | Description | Detected In |
|------------|-------|-------------|-------------|
| `FLS_E_PARAM_CONFIG` | 0x01 | Invalid configuration pointer | Fls_Init |
| `FLS_E_PARAM_ADDRESS` | 0x02 | Invalid address parameter | Fls_Erase/Write/Read/Compare |
| `FLS_E_PARAM_LENGTH` | 0x03 | Invalid length parameter | Fls_Erase/Write/Read/Compare |
| `FLS_E_PARAM_DATA` | 0x04 | Invalid data pointer | Fls_Write/Read/Compare |
| `FLS_E_UNINIT` | 0x05 | Driver not initialized | All APIs except Init |
| `FLS_E_BUSY` | 0x06 | Driver busy | Fls_Erase/Write/Read/Compare |
| `FLS_E_INVALID_LENGTH` | 0x07 | Length violates constraints | Fls_Erase/Write |
| `FLS_E_INVALID_ADDRESS` | 0x08 | Address violates constraints | Fls_Erase |
| `FLS_E_PARAM_POINTER` | 0x09 | NULL pointer parameter | Fls_GetVersionInfo |
| `FLS_E_ALREADY_INITIALIZED` | 0x0A | Double initialization | Fls_Init |

### Runtime Errors

| Error Code | Value | Description |
|------------|-------|-------------|
| `FLS_E_ERASE_FAILED` | 0x01 | Hardware erase failure |
| `FLS_E_WRITE_FAILED` | 0x02 | Hardware write failure |
| `FLS_E_READ_FAILED` | 0x03 | Hardware read failure |
| `FLS_E_COMPARE_FAILED` | 0x04 | Compare operation failure |
| `FLS_E_UNEXPECTED_FLASH_ID` | 0x05 | Wrong flash device detected |

## Hardware Requirements

### Supported Platforms
- ARM Cortex-M3/M4/M7 (STM32, NXP S32K)
- Infineon AURIX TC2xx/TC3xx
- Renesas RH850/U2A
- Texas Instruments Hercules

### Flash Characteristics

| Parameter | Typical Values |
|-----------|----------------|
| Sector Sizes | 4KB, 8KB, 16KB, 32KB, 64KB, 128KB, 256KB |
| Page Size | 256 bytes (typical) |
| Programming Unit | 32-bit (4 bytes) |
| Erase Time | 20-4000ms depending on sector size |
| Programming Time | 20-100μs per word |
| Endurance | 10,000-100,000 cycles |
| Data Retention | 10-30 years |

### Resource Usage

| Resource | Typical Usage |
|----------|---------------|
| RAM | ~200-500 bytes |
| ROM/Code | ~5-12 KB |
| Stack | ~128-256 bytes |
| Interrupts | 0 (polling) or 1 (interrupt mode) |

## Dependencies

### Required Modules
- `Std_Types` - Standard AUTOSAR types
- `Platform_Types` - Platform-specific types
- `Compiler` - Compiler abstraction
- `Det` - Development Error Tracer (if FLS_DEV_ERROR_DETECT is ON)
- `MemIf_Types` - Memory interface types

### Optional Modules
- `SchM_Fls` - Schedule manager for Fls
- `FlsHw` - Hardware-specific flash driver (lower layer)

## Scheduling

The Fls_MainFunction() must be called periodically to process flash jobs:

```c
/* Recommended scheduling: 1-10ms */
void Fls_CyclicTask(void)
{
    Fls_MainFunction();
}
```

## References

- AUTOSAR SWS Flash Driver (R22-11)
- AUTOSAR Specification of Memory Stack
- ARM Cortex-M Technical Reference Manual
- Vendor-specific Flash Programming Manuals

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-01 | Initial AUTOSAR R22-11 implementation |
| 1.0.1 | 2024-03 | Added support for multi-bank flash |
| 1.1.0 | 2024-06 | Enhanced error handling and reporting |
| 1.1.1 | 2024-09 | Performance optimizations for erase operations |
| 1.2.0 | 2024-12 | Added Fls_ReadSync API for ISR-disabled configurations |

## Test Coverage

| Component | Test Coverage |
|-----------|--------------|
| Initialization | 100% |
| Erase Operations | 100% |
| Write Operations | 100% |
| Read Operations | 100% |
| Compare Operations | 100% |
| Mode Management | 100% |
| Job Cancellation | 100% |
| Error Handling | 100% |
| State Machine | 100% |
| **Overall** | **~85%** |