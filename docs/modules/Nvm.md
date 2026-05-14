# NVM - Non-Volatile Memory Manager

## Overview

NVM implements AUTOSAR Non-Volatile Memory Manager for managing persistent data storage. It provides abstraction over underlying Flash/EEPROM drivers with data integrity and redundancy features.

## Standards

- AUTOSAR SWS NVM (Non-Volatile Memory Manager)
- AUTOSAR Classic Platform 4.4.0
- MISRA C:2012

## Features

### Block Management
- **NV Blocks** - Non-volatile data blocks
- **RAM Mirrors** - Working copies in RAM
- **ROM Defaults** - Default values for initialization
- **Administrative Blocks** - State and error tracking

### Data Integrity
- **CRC Protection** - 8/16/32-bit CRC options
- **Redundant Storage** - Dual instance storage
- **ECC Support** - Error Correction Code
- **Write Verification** - Read-after-write verification

### Error Recovery
- **Implicit Recovery** - Automatic recovery on read
- **Explicit Recovery** - Application-triggered recovery
- **Single Block Error Recovery** - Per-block recovery
- **Complete Restore** - Full NVM restore

### Write Strategies
- **Immediate Write** - Synchronous write
- **Deferred Write** - Asynchronous queued write
- **Cyclic Write** - Periodic automatic write

## Block Types

| Type | Description |
|------|-------------|
| NATIVE | Standard NV block |
| REDUNDANT | Dual instance block |
| DATASET | Multiple datasets per block |
| UNKNOWN | Automatic detection |

## APIs

### Core APIs
| API | Function |
|-----|----------|
| `NvM_Init()` | Initialize NVM |
| `NvM_DeInit()` | Deinitialize NVM |
| `NvM_ReadBlock()` | Read block from NV memory |
| `NvM_WriteBlock()` | Write block to NV memory |
| `NvM_RestoreBlockDefaults()` | Restore default values |
| `NvM_EraseNvBlock()` | Erase NV block |
| `NvM_InvalidateNvBlock()` | Invalidate block |

### Management APIs
| API | Function |
|-----|----------|
| `NvM_ReadAll()` | Read all blocks at startup |
| `NvM_WriteAll()` | Write all blocks at shutdown |
| `NvM_CancelJobs()` | Cancel pending jobs |
| `NvM_GetErrorStatus()` | Get block error status |
| `NvM_SetBlockLockStatus()` | Lock/unlock block |

## Redundant Storage

### Dual Instance Architecture
```
Block Write:
    Primary Instance (Instance 0)
        |
        v
    Write + CRC
        |
        v
    Mirror Instance (Instance 1)
        |
        v
    Write + CRC

Block Read:
    Try Primary Instance
        |
        +-- CRC OK --> Use Primary
        |
        +-- CRC FAIL
                |
                v
        Try Mirror Instance
                |
                +-- CRC OK --> Use Mirror + Recover Primary
                |
                +-- CRC FAIL --> Use ROM Default
```

### Sequence Number
- Tracks write order
- Helps detect latest valid instance
- Used in recovery logic

## CRC Protection

### Supported CRC Types
| Type | Polynomial | Use Case |
|------|------------|----------|
| CRC8 | 0x1D | Small blocks |
| CRC16 | 0x1021 | Medium blocks |
| CRC32 | 0x04C11DB7 | Large blocks |

### CRC Calculation
```c
/* Calculate CRC over data + administrative info */
crc = Crc_CalculateCRC32(data, length, 0xFFFFFFFF, TRUE);
```

## ECC Handler

### Error Correction
- Single-bit error correction
- Double-bit error detection
- Automatic correction logging

### Error Types
| Type | Handling |
|------|----------|
| Correctable | Auto-correct + log |
| Uncorrectable | Report error + use redundant |

## Configuration

### Pre-compile
- `NVM_VERSION_INFO_API` - Enable version info
- `NVM_DEV_ERROR_DETECT` - Enable error detection
- `NVM_CRC_SUPPORTED` - Enable CRC support
- `NVM_REDUNDANT_STORAGE_ENABLED` - Enable redundancy

### Block Configuration
```c
typedef struct {
    NvM_BlockIdType BlockId;
    uint16 BlockSize;
    uint8 CRCType;
    boolean Redundant;
    uint32 WriteCycleLimit;
} NvM_BlockDescriptorType;
```

## Job Queue

### Asynchronous Processing
- Job queue for write/erase operations
- Priority-based scheduling
- Multi-request handling

### Job Types
| Type | Description |
|------|-------------|
| READ | Read block from NV |
| WRITE | Write block to NV |
| ERASE | Erase NV block |
| RESTORE | Restore defaults |

## Dependencies

- MemIf (Memory Interface)
- Fee (Flash EEPROM Emulation)
- EA (EEPROM Abstraction)
- Crc (CRC Library)
- DET (Development Error Tracer)

## Source Code

- `/home/admin/yuleASR/src/bsw/services/nvm/`
  - `include/NvM.h` - Public API
  - `include/NvM_Cfg.h` - Configuration
  - `include/NvM_EccHandler.h` - ECC handler
  - `src/NvM.c` - Core implementation (2367 lines)
  - `src/NvM_Redundant.c` - Redundant storage (299 lines)
  - `src/NvM_EccHandler.c` - ECC handling (733 lines)

## Tests

- `/home/admin/yuleASR/tests/unit/autosar/services/Nvm/`

## References

- AUTOSAR_SWS_NVRAMManager
- AUTOSAR_SWS_MemoryStack
