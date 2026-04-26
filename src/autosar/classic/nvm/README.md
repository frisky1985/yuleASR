# AUTOSAR Classic NvM Module

## Overview

The Non-Volatile Memory (NvM) module provides services to the application for storing data blocks in non-volatile memory (NVRAM).

**Module ID:** 0x0E  
**AUTOSAR Version:** 4.4.0

## Features

- **Block Management:** Up to 32 configurable NV blocks
- **State Machine:** IDLE → BUSY → PENDING → IDLE
- **Job Queue:** Priority-based single linked list implementation
- **Write Protection:** Configurable per-block with time window
- **Write Retry:** Configurable retry mechanism (default: 3 retries)
- **Write Verification:** Read-after-write verification
- **CRC Protection:** CRC-8, CRC-16, CRC-32 support
- **ROM Defaults:** Restore to default values on failure
- **RAM Block Status:** Incremental write protection via SetRamBlockStatus

## File Structure

| File | Description |
|------|-------------|
| `NvM.h` | Public API header |
| `NvM_Cfg.h` | Configuration parameters |
| `NvM_Private.h` | Internal types and functions |
| `NvM.c` | Main module implementation |
| `NvM_Queue.c` | Job queue management |
| `NvM_Crc.c` | CRC calculation (8/16/32-bit) |
| `NvM_WriteProtection.c` | Write protection mechanisms |
| `NvM_Verification.c` | Write verification functions |

## Key Configurations (NvM_Cfg.h)

- `NVM_MAX_NUMBER_OF_BLOCKS`: Maximum 32 NV blocks
- `NVM_MAX_NUM_OF_WRITE_RETRIES`: Write retry count (3)
- `NVM_WRITE_PROTECTION_WINDOW_MS`: Protection window (5000ms)
- `NVM_SIZE_OF_JOB_QUEUE`: Job queue size (16)
- `NVM_MAIN_FUNCTION_PERIOD_MS`: Main function period (10ms)

## API Functions

### Core Functions
- `NvM_Init()` - Initialize module
- `NvM_MainFunction()` - Cyclic processing
- `NvM_GetVersionInfo()` - Get version information

### Block Operations
- `NvM_ReadBlock()` - Read NV block to RAM
- `NvM_WriteBlock()` - Write RAM block to NV
- `NvM_EraseNvBlock()` - Erase NV block
- `NvM_InvalidateNvBlock()` - Invalidate NV block
- `NvM_RestoreBlockDefaults()` - Restore ROM defaults

### Multi-Block Operations
- `NvM_ReadAll()` - Read all configured blocks
- `NvM_WriteAll()` - Write all modified blocks
- `NvM_CancelWriteAll()` - Cancel ongoing WriteAll

### Control Functions
- `NvM_SetRamBlockStatus()` - Mark RAM block as changed
- `NvM_SetBlockProtection()` - Enable/disable write protection
- `NvM_GetErrorStatus()` - Get operation result
- `NvM_SetDataIndex()` / `NvM_GetDataIndex()` - Dataset selection

## Integration with EcuM

The NvM module is initialized during **EcuM Init One** phase:

```c
void EcuM_AL_DriverInitOne(void)
{
    /* Initialize NvM first */
    NvM_Init(NULL_PTR);
    
    /* Other BSW modules... */
}
```

## Dependencies

- **MemIf** (Module ID 0x0F) - Memory Interface
- **FEE** or **EA** - Underlying flash/EEPROM driver
- **Det** (optional) - Development Error Tracer

## Usage Example

```c
#include "NvM.h"

/* Configuration data to store */
static uint8_t DdsConfigData[128];

void StoreDdsConfiguration(void)
{
    Std_ReturnType result;
    
    /* Write configuration to NV block 1 */
    result = NvM_WriteBlock(1, DdsConfigData);
    
    if (result == E_OK) {
        /* Request queued successfully */
    }
}

void ReadDdsConfiguration(void)
{
    Std_ReturnType result;
    
    /* Read configuration from NV block 1 */
    result = NvM_ReadBlock(1, DdsConfigData);
    
    if (result == E_OK) {
        /* Request queued successfully */
    }
}
```

## State Machine

```
    +--------+    Init     +--------+
    | UNINIT | ----------> |  IDLE  |
    +--------+             +--------+
                                 |
                                 | Job queued
                                 v
                           +-----------+
                           |    BUSY   |
                           +-----------+
                                 |
                                 | MemIf called
                                 v
                           +-----------+
                           |  PENDING  |<--+
                           +-----------+   |
                                 |         | MemIf busy
                                 | MemIf   |
                                 | done    |
                                 +---------+
```

## Copyright

Copyright (c) 2025
AUTOSAR Classic Platform Compliant Implementation
