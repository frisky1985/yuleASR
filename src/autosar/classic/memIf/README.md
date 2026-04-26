# AUTOSAR Classic MemIf Module

## Overview

The Memory Interface (MemIf) module provides uniform access to memory devices (FEE, EA) for the NvM module.

**Module ID:** 0x0F  
**AUTOSAR Version:** 4.4.0

## Features

- **Device Abstraction:** Support for 1-2 memory devices
- **Unified Interface:** Common API for FEE and EA drivers
- **Asynchronous Operations:** Non-blocking read/write/erase
- **Device Selection:** Automatic routing based on device ID
- **Status Polling:** Query device and job status

## File Structure

| File | Description |
|------|-------------|
| `MemIf.h` | Public API header |
| `MemIf_Cfg.h` | Configuration parameters |
| `MemIf.c` | Module implementation |

## Key Configurations (MemIf_Cfg.h)

- `MEMIF_NUMBER_OF_DEVICES`: Number of devices (1-2)
- `MEMIF_DEVICE_0_ENABLED` / `MEMIF_DEVICE_1_ENABLED`: Device enable flags
- `MEMIF_DEVICE_0_BLOCK_COUNT`: Block count per device
- `MEMIF_DEVICE_0_BLOCK_SIZE`: Block size per device
- `MEMIF_ERASE_ENABLED`: Enable erase operation
- `MEMIF_INVALIDATE_ENABLED`: Enable invalidate operation

## API Functions

### Initialization
- `MemIf_Init()` - Initialize module and underlying devices
- `MemIf_SetMode()` - Set operating mode (SLOW/FAST)

### Operations
- `MemIf_Read()` - Read data from device
- `MemIf_Write()` - Write data to device
- `MemIf_Erase()` - Erase block
- `MemIf_Invalidate()` - Invalidate block
- `MemIf_Cancel()` - Cancel ongoing operation

### Status
- `MemIf_GetStatus()` - Get device status
- `MemIf_GetJobResult()` - Get last job result

### Cyclic
- `MemIf_MainFunction()` - Process pending operations

## Types

### MemIf_StatusType
```c
typedef enum {
    MEMIF_UNINIT = 0,        /* Module uninitialized */
    MEMIF_IDLE = 1,          /* Device idle */
    MEMIF_BUSY = 2,          /* Device busy */
    MEMIF_BUSY_INTERNAL = 3  /* Internal operation */
} MemIf_StatusType;
```

### MemIf_JobResultType
```c
typedef enum {
    MEMIF_JOB_OK = 0,           /* Success */
    MEMIF_JOB_FAILED = 1,       /* Failed */
    MEMIF_JOB_PENDING = 2,      /* Still pending */
    MEMIF_JOB_CANCELED = 3,     /* Canceled */
    MEMIF_BLOCK_INCONSISTENT = 4,
    MEMIF_BLOCK_INVALID = 5
} MemIf_JobResultType;
```

## Device Driver Configuration

The MemIf module uses function pointers to abstract underlying drivers:

```c
typedef struct {
    MemIf_InitFnPtrType         Init;
    MemIf_SetModeFnPtrType      SetMode;
    MemIf_ReadFnPtrType         Read;
    MemIf_WriteFnPtrType        Write;
    MemIf_CancelFnPtrType       Cancel;
    MemIf_GetStatusFnPtrType    GetStatus;
    MemIf_GetJobResultFnPtrType GetJobResult;
    MemIf_InvalidateFnPtrType   Invalidate;
    MemIf_EraseFnPtrType        Erase;
    MemIf_MainFunctionFnPtrType MainFunction;
    boolean                     IsFee;
    uint16_t                    BlockOffset;
    uint16_t                    NumOfBlocks;
    boolean                     Enabled;
} MemIf_DeviceDriverType;
```

## Integration with NvM

The NvM module calls MemIf functions for all memory operations:

```c
/* Example: NvM Write via MemIf */
MemIf_Write(
    DeviceId,      /* From block configuration */
    BlockNumber,   /* Block base number + data index */
    DataBuffer     /* Data to write */
);
```

## Usage Example

```c
#include "MemIf.h"

void ReadFromMemory(void)
{
    Std_ReturnType result;
    uint8_t buffer[64];
    
    /* Read from device 0, block 10 */
    result = MemIf_Read(
        0,              /* Device index */
        10,             /* Block number */
        0,              /* Block offset */
        buffer,         /* Data buffer */
        sizeof(buffer)  /* Length */
    );
    
    if (result == E_OK) {
        /* Request accepted - poll for completion */
        while (MemIf_GetStatus(0) == MEMIF_BUSY) {
            MemIf_MainFunction();
        }
        
        if (MemIf_GetJobResult(0) == MEMIF_JOB_OK) {
            /* Read successful */
        }
    }
}
```

## Copyright

Copyright (c) 2025
AUTOSAR Classic Platform Compliant Implementation
