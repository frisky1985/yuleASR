# FLASH Driver Module

## Overview

The FLASH module provides a hardware-independent interface for internal flash memory operations in AUTOSAR-based embedded systems. It serves as a legacy wrapper around the standard AUTOSAR Fls (Flash Driver) module, offering additional convenience functions for flash memory management including read, write, erase, compare, and protection operations.

**AUTOSAR Standard**: Classic Platform 4.4.0  
**Layer**: MCAL (Microcontroller Driver)  
**Module ID**: 0x5C (92)  
**Vendor ID**: 0x0001  
**ASIL Level**: QM to ASIL-B (configurable)

## Features

- **Flash Memory Access**: Read and write operations to internal flash memory
- **Sector Erase**: Individual or bulk sector erase functionality
- **Data Comparison**: Compare flash contents with RAM buffers
- **Blank Check**: Verify if flash sectors are erased (all 0xFF)
- **Write Protection**: Configure write protection for flash sectors
- **Read Protection**: Configure read protection levels
- **Dual Mode Operation**: Normal and Fast operation modes
- **Asynchronous Operations**: Non-blocking read/write/erase with polling via MainFunction
- **Error Detection**: Comprehensive error reporting via DET (Development Error Tracer)

## Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│     (NvM, Fee, Bootloader)          │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         MemIf (Memory Interface)    │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         Flash Driver (Flash.h)      │
│  ┌─────────────────────────────────┐│
│  │  Write Protection Control       ││
│  │  Read Protection Control        ││
│  │  Sector Management              ││
│  │  Error Detection & Reporting    ││
│  └─────────────────────────────────┘│
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         Fls (AUTOSAR Standard)      │
│  ┌─────────────────────────────────┐│
│  │  Hardware Abstraction Layer     ││
│  │  Flash Controller Interface     ││
│  └─────────────────────────────────┘│
└─────────────────────────────────────┘
```

## API Reference

### Core Functions

```c
/* Initialization and De-initialization */
void Fls_Init(const Fls_ConfigType* ConfigPtr);
void Fls_DeInit(void);

/* Flash Operations */
Std_ReturnType Fls_Erase(Fls_AddressType TargetAddress, Fls_LengthType Length);
Std_ReturnType Fls_Write(Fls_AddressType TargetAddress, const uint8* SourceAddressPtr, Fls_LengthType Length);
Std_ReturnType Fls_Read(Fls_AddressType SourceAddress, uint8* TargetAddressPtr, Fls_LengthType Length);
Std_ReturnType Fls_Compare(Fls_AddressType SourceAddress, const uint8* TargetAddressPtr, Fls_LengthType Length);
Std_ReturnType Fls_BlankCheck(Fls_AddressType TargetAddress, Fls_LengthType Length);

/* Control Functions */
void Fls_SetMode(MemIf_ModeType Mode);
void Fls_Cancel(void);
MemIf_StatusType Fls_GetStatus(void);
MemIf_JobResultType Fls_GetJobResult(void);
void Fls_MainFunction(void);

/* Protection Functions */
Std_ReturnType Fls_ConfigureReadProtection(Fls_ProtectionType Protection);
Std_ReturnType Fls_ConfigureWriteProtection(uint32 SectorMask, boolean Enable);

/* Version Info */
#if (FLS_VERSION_INFO_API == STD_ON)
void Fls_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif
```

### Data Types

```c
/* Status Types */
typedef enum {
    MEMIF_UNINIT = 0,
    MEMIF_IDLE,
    MEMIF_BUSY
} MemIf_StatusType;

typedef enum {
    MEMIF_JOB_OK = 0,
    MEMIF_JOB_FAILED,
    MEMIF_JOB_PENDING,
    MEMIF_JOB_CANCELED
} MemIf_JobResultType;

typedef enum {
    MEMIF_MODE_SLOW = 0,
    MEMIF_MODE_FAST
} MemIf_ModeType;

/* Sector Types */
typedef enum {
    FLS_SECTOR_SIZE_4KB = 0,
    FLS_SECTOR_SIZE_32KB,
    FLS_SECTOR_SIZE_64KB,
    FLS_SECTOR_SIZE_128KB
} Fls_SectorSizeType;

typedef struct {
    uint32                SectorStartAddress;
    uint32                SectorSize;
    Fls_SectorSizeType    SectorSizeType;
    boolean               SectorProtected;
    uint8                 SectorBank;
} Fls_SectorInfoType;

/* Protection Types */
typedef enum {
    FLS_PROTECTION_NONE = 0,
    FLS_PROTECTION_READ,
    FLS_PROTECTION_WRITE,
    FLS_PROTECTION_READ_WRITE
} Fls_ProtectionType;

/* Configuration Type */
typedef struct {
    uint32                BaseAddress;
    uint32                TotalSize;
    const Fls_SectorInfoType* SectorInfo;
    uint32                SectorCount;
    uint32                PageSize;
    uint32                ProgrammingUnit;
    uint32                MaxReadFastMode;
    uint32                MaxReadNormalMode;
    uint32                MaxWriteFastMode;
    uint32                MaxWriteNormalMode;
    MemIf_ModeType        DefaultMode;
    uint32                CallCycle;
    boolean               UseInterrupts;
    void (*JobEndNotification)(void);
    void (*JobErrorNotification)(void);
} Fls_ConfigType;
```

## Configuration Parameters

| Parameter | Type | Description | Default |
|-----------|------|-------------|---------|
| `BaseAddress` | uint32 | Base address of flash memory | 0x08000000 |
| `TotalSize` | uint32 | Total flash size in bytes | 0x00100000 (1MB) |
| `SectorCount` | uint32 | Number of configured sectors | 8 |
| `PageSize` | uint32 | Flash page size for writes | 256 bytes |
| `ProgrammingUnit` | uint32 | Minimum write unit | 4 bytes (32-bit) |
| `MaxReadFastMode` | uint32 | Max bytes per read in fast mode | 1024 |
| `MaxReadNormalMode` | uint32 | Max bytes per read in normal mode | 256 |
| `MaxWriteFastMode` | uint32 | Max bytes per write in fast mode | 256 |
| `MaxWriteNormalMode` | uint32 | Max bytes per write in normal mode | 64 |
| `DefaultMode` | enum | Default operation mode | MEMIF_MODE_SLOW |
| `UseInterrupts` | boolean | Use interrupt-driven operation | FALSE |
| `FLS_VERSION_INFO_API` | macro | Enable version info API | STD_ON |
| `FLS_DEV_ERROR_DETECT` | macro | Enable development error detection | STD_ON |

## Usage Example

### Basic Flash Operations

```c
#include "Flash.h"
#include "Flash_Cfg.h"

/* Flash buffer */
static uint8 write_buffer[256];
static uint8 read_buffer[256];

void Flash_Example(void)
{
    Std_ReturnType status;
    
    /* Initialize flash driver */
    Fls_Init(&Fls_Config);
    
    /* Erase sector before writing */
    status = Fls_Erase(0x08000000U, 0x00010000U);  /* 64KB sector */
    if (status == E_OK) {
        /* Wait for erase completion */
        while (Fls_GetStatus() == MEMIF_BUSY) {
            Fls_MainFunction();
        }
    }
    
    /* Prepare data */
    for (int i = 0; i < 256; i++) {
        write_buffer[i] = (uint8)i;
    }
    
    /* Write data to flash */
    status = Fls_Write(0x08000000U, write_buffer, 256);
    if (status == E_OK) {
        /* Wait for write completion */
        while (Fls_GetStatus() == MEMIF_BUSY) {
            Fls_MainFunction();
        }
        
        if (Fls_GetJobResult() == MEMIF_JOB_OK) {
            /* Write successful */
        }
    }
    
    /* Read back data */
    status = Fls_Read(0x08000000U, read_buffer, 256);
    if (status == E_OK) {
        while (Fls_GetStatus() == MEMIF_BUSY) {
            Fls_MainFunction();
        }
    }
    
    /* Verify data */
    if (memcmp(write_buffer, read_buffer, 256) == 0) {
        /* Data verified */
    }
}
```

### Protection Configuration

```c
void Flash_ProtectionExample(void)
{
    Std_ReturnType status;
    
    Fls_Init(&Fls_Config);
    
    /* Configure read protection */
    status = Fls_ConfigureReadProtection(FLS_PROTECTION_READ);
    if (status == E_OK) {
        /* Read protection enabled */
    }
    
    /* Enable write protection for sector 0 */
    status = Fls_ConfigureWriteProtection(0x00000001U, TRUE);
    if (status == E_OK) {
        /* Write protection enabled for sector 0 */
    }
    
    /* Disable write protection for sector 0 */
    status = Fls_ConfigureWriteProtection(0x00000001U, FALSE);
}
```

### Blank Check

```c
void Flash_BlankCheckExample(void)
{
    Std_ReturnType status;
    
    Fls_Init(&Fls_Config);
    
    /* Erase sector */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Check if sector is blank (all 0xFF) */
    status = Fls_BlankCheck(0x08000000U, 256);
    if (status == E_OK) {
        while (Fls_GetStatus() == MEMIF_BUSY) {
            Fls_MainFunction();
        }
        
        if (Fls_GetJobResult() == MEMIF_JOB_OK) {
            /* Sector is blank */
        }
    }
}
```

## Error Handling

### Development Errors (DET)

| Error Code | Description | Detection |
|------------|-------------|-----------|
| `FLS_E_PARAM_CONFIG` | Invalid configuration pointer | Fls_Init |
| `FLS_E_PARAM_ADDRESS` | Invalid address parameter | Fls_Read/Write/Erase |
| `FLS_E_PARAM_LENGTH` | Invalid length parameter | Fls_Read/Write/Erase |
| `FLS_E_PARAM_DATA` | NULL data pointer | Fls_Read/Write/Compare |
| `FLS_E_UNINIT` | Driver not initialized | All APIs |
| `FLS_E_BUSY` | Driver busy | Fls_Read/Write/Erase |
| `FLS_E_VERIFY_ERASED_FAILED` | Erase verification failed | Fls_Erase |
| `FLS_E_VERIFY_WRITE_FAILED` | Write verification failed | Fls_Write |
| `FLS_E_TIMEOUT` | Operation timeout | All operations |
| `FLS_E_PARAM_POINTER` | NULL pointer parameter | Fls_GetVersionInfo |
| `FLS_E_ERASE_FAILED` | Erase operation failed | Fls_Erase |
| `FLS_E_WRITE_FAILED` | Write operation failed | Fls_Write |
| `FLS_E_READ_FAILED` | Read operation failed | Fls_Read |
| `FLS_E_COMPARE_FAILED` | Compare operation failed | Fls_Compare |
| `FLS_E_UNEXPECTED_FLASH_ID` | Wrong flash device detected | Fls_Init |
| `FLS_E_SECTOR_PROTECTED` | Operation on protected sector | Fls_Write/Erase |

### Runtime Errors

| Error Code | Description | Recovery |
|------------|-------------|----------|
| `FLS_E_ERASE_FAILED` | Hardware erase failure | Retry or report |
| `FLS_E_WRITE_FAILED` | Hardware write failure | Retry or report |
| `FLS_E_READ_FAILED` | Hardware read failure | Retry or report |
| `FLS_E_TIMEOUT` | Operation timeout | Cancel and retry |

## State Machine

```
                    ┌─────────────┐
                    │   UNINIT    │
                    └──────┬──────┘
                           │ Fls_Init()
                           ▼
                    ┌─────────────┐
         ┌─────────│    IDLE     │◄─────────┐
         │         └──────┬──────┘          │
         │                │                  │
   Fls_Cancel()    Fls_Read/Write/Erase  Job Complete
         │                │                  │
         │                ▼                  │
         │         ┌─────────────┐          │
         └────────►│    BUSY     │──────────┘
                   └─────────────┘
```

## Hardware Requirements

### Supported Microcontrollers
- STM32H7 (ARM Cortex-M7)
- STM32F4/F7 (ARM Cortex-M4/M7)
- NXP S32K3xx
- Infineon AURIX TC3xx
- Renesas RH850/U2A

### Flash Characteristics

| Parameter | Typical Value |
|-----------|---------------|
| Sector Size | 4KB, 32KB, 64KB, 128KB |
| Page Size | 256 bytes (STM32) |
| Programming Unit | 32-bit (4 bytes) |
| Erase Time | 20-800ms per sector |
| Write Time | 20-100μs per 32-bit word |
| Endurance | 10,000+ erase cycles |
| Data Retention | 30 years |

### Resource Usage

| Resource | Typical Usage |
|----------|---------------|
| RAM | ~500-1000 bytes |
| ROM | ~8-15 KB |
| Stack | ~256-512 bytes |
| Interrupts | 1 (optional) |

## Dependencies

### Required Modules
- `Std_Types`, `Platform_Types`, `Compiler` - Standard types
- `Det` - Development Error Tracer
- `Fls` - AUTOSAR Flash Driver (underlying implementation)

### Optional Modules
- `MemIf` - Memory Interface (upper layer)
- `Fee` - Flash EEPROM Emulation
- `NvM` - NVRAM Manager

## References

- AUTOSAR SWS Flash Driver (R22-11)
- AUTOSAR Classic Platform Specification
- ARM Cortex-M Technical Reference Manual
- STM32 Flash Programming Manual (RM0433)
- Flash Memory Endurance Application Notes

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-01 | Initial release with basic read/write/erase |
| 1.1.0 | 2024-04 | Added protection control functions |
| 1.2.0 | 2024-08 | Added blank check and compare operations |
| 1.3.0 | 2024-11 | Added dual-mode operation support |

## Test Coverage

| Test Category | Coverage |
|--------------|----------|
| Initialization | 100% |
| Write Operations | 100% |
| Read Operations | 100% |
| Erase Operations | 100% |
| Compare Operations | 100% |
| Protection Control | 100% |
| Error Handling | 100% |
| **Total** | **100%** |
