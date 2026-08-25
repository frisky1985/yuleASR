# EEP (EEPROM Driver) Module
详细设计文档见 [Eep 设计文档](../design/modules/mcal/eep-design.md)。

## Overview

The EEP module provides standardized access to the microcontroller's EEPROM (Electrically Erasable Programmable Read-Only Memory) hardware for the AUTOSAR Basic Software. It abstracts the microcontroller-specific EEPROM hardware and provides a uniform interface for application software to perform persistent data storage operations.

**AUTOSAR Standard**: Classic Platform 4.4.0  
**Layer**: MCAL (Microcontroller Driver)  
**Hardware**: NXP S32K3 / Infineon TC3xx / STM32 / i.MX8M Mini  
**ASIL Level**: QM (Quality Management)

## Features

- **Read Operations**: Read data from EEPROM memory
- **Write Operations**: Write data to EEPROM memory
- **Erase Operations**: Erase EEPROM memory sectors
- **Asynchronous Operations**: Non-blocking read/write/erase with job completion callback
- **Job Management**: Cancel ongoing operations, query job status and result
- **State Management**: Track driver state (UNINIT, IDLE, BUSY)
- **Error Detection**: Development error detection and reporting
- **Configurable Timing**: Job call cycle configuration for operation timing

## Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│   (NvM, Fee, Parameter Storage)     │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         EEP Driver (MCAL)           │
│  ┌─────────┐ ┌─────────┐           │
│  │  Read   │ │  Write  │           │
│  │  Job    │ │  Job    │           │
│  └────┬────┘ └────┬────┘           │
│       └─────────────┘               │
│              │                      │
│  ┌───────────▼────────────┐         │
│  │   EEPROM Controller    │         │
│  │ (Microcontroller EEPROM│         │
│  │      Hardware)         │         │
│  └────────────────────────┘         │
└─────────────────────────────────────┘
```

## API Reference

### Core Functions

```c
/* Initialization */
void Eep_Init(const Eep_ConfigType* ConfigPtr);
void Eep_DeInit(void);

/* Read Operations */
Std_ReturnType Eep_Read(Eep_AddressType Address, uint8* DataPtr, Eep_LengthType Length);

/* Write Operations */
Std_ReturnType Eep_Write(Eep_AddressType Address, const uint8* DataPtr, Eep_LengthType Length);

/* Erase Operations */
Std_ReturnType Eep_Erase(Eep_AddressType Address, Eep_LengthType Length);

/* Job Control */
void Eep_Cancel(void);
Eep_StatusType Eep_GetStatus(void);
Eep_JobResultType Eep_GetJobResult(void);

/* Main Function (Cyclic) */
void Eep_MainFunction(void);
```

### Data Types

```c
/* Address and Length Types */
typedef uint32 Eep_AddressType;          /* EEPROM address */
typedef uint32 Eep_LengthType;           /* Data length in bytes */

/* Job Result Type */
typedef enum {
    EEP_JOB_OK = 0,             /* Job completed successfully */
    EEP_JOB_PENDING,            /* Job is still pending */
    EEP_JOB_FAILED,             /* Job failed */
    EEP_JOB_CANCELED            /* Job was canceled */
} Eep_JobResultType;

/* Status Type */
typedef enum {
    EEP_UNINIT = 0,             /* Driver not initialized */
    EEP_IDLE,                   /* Driver initialized, no operation */
    EEP_BUSY                    /* Operation in progress */
} Eep_StatusType;

/* Configuration Type */
typedef struct {
    Eep_AddressType BaseAddress;        /* EEPROM base address */
    Eep_LengthType Size;                /* EEPROM size in bytes */
    uint32 JobCallCycle;                /* Job processing cycle time in ms */
} Eep_ConfigType;
```

### Error Codes

| Error Code | Value | Description |
|------------|-------|-------------|
| `EEP_E_NO_ERROR` | 0x00 | No error |
| `EEP_E_PARAM_POINTER` | 0x01 | NULL pointer error |
| `EEP_E_PARAM_ADDRESS` | 0x02 | Invalid address |
| `EEP_E_PARAM_LENGTH` | 0x03 | Invalid length |
| `EEP_E_UNINIT` | 0x04 | Driver not initialized |
| `EEP_E_BUSY` | 0x05 | Driver busy |

### Service IDs

| Service ID | Value | Description |
|------------|-------|-------------|
| `EEP_SID_INIT` | 0x01 | Eep_Init() |
| `EEP_SID_DEINIT` | 0x02 | Eep_DeInit() |
| `EEP_SID_READ` | 0x03 | Eep_Read() |
| `EEP_SID_WRITE` | 0x04 | Eep_Write() |
| `EEP_SID_ERASE` | 0x05 | Eep_Erase() |
| `EEP_SID_CANCEL` | 0x06 | Eep_Cancel() |
| `EEP_SID_GET_STATUS` | 0x07 | Eep_GetStatus() |
| `EEP_SID_GET_JOB_RESULT` | 0x08 | Eep_GetJobResult() |

## Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `EEP_DEV_ERROR_DETECT` | STD_ON/OFF | Enable development error detection |
| `EEP_VERSION_INFO_API` | STD_ON/OFF | Enable version information API |
| `EEP_CANCEL_API` | STD_ON/OFF | Enable cancel API |
| `EEP_BASE_ADDRESS` | uint32 | EEPROM base address |
| `EEP_SIZE` | uint32 | EEPROM size in bytes |
| `EEP_PAGE_SIZE` | uint8 | EEPROM page size for write operations |
| `EEP_WRITE_CYCLE_TIME` | uint32 | Write cycle time in milliseconds |
| `EEP_ERASE_CYCLE_TIME` | uint32 | Erase cycle time in milliseconds |
| `EEP_JOB_CALL_CYCLE` | uint32 | Job processing cycle time in milliseconds |

## Usage Examples

### Basic Initialization

```c
#include "Eep.h"

void Eep_Example_Init(void)
{
    /* Define EEPROM configuration */
    const Eep_ConfigType eepConfig = {
        .BaseAddress = 0x08080000U,
        .Size = 0x00010000U,        /* 64KB EEPROM */
        .JobCallCycle = 10U         /* 10ms cycle time */
    };
    
    /* Initialize EEPROM driver */
    Eep_Init(&eepConfig);
}
```

### Read Operation

```c
void Eep_Example_Read(void)
{
    uint8 readBuffer[32];
    Std_ReturnType result;
    
    /* Start read operation from address 0x100, 32 bytes */
    result = Eep_Read(0x100, readBuffer, 32);
    
    if (result == E_OK) {
        /* Wait for operation to complete (polling) */
        while (Eep_GetStatus() == EEP_BUSY) {
            Eep_MainFunction();  /* Process the job */
        }
        
        /* Check result */
        if (Eep_GetJobResult() == EEP_JOB_OK) {
            /* Read successful, data in readBuffer */
        }
    }
}
```

### Write Operation

```c
void Eep_Example_Write(void)
{
    uint8 writeData[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    Std_ReturnType result;
    
    /* Start write operation to address 0x200, 16 bytes */
    result = Eep_Write(0x200, writeData, 16);
    
    if (result == E_OK) {
        /* Wait for operation to complete */
        while (Eep_GetStatus() == EEP_BUSY) {
            Eep_MainFunction();
        }
        
        if (Eep_GetJobResult() == EEP_JOB_OK) {
            /* Write successful */
        }
    }
}
```

### Erase Operation

```c
void Eep_Example_Erase(void)
{
    Std_ReturnType result;
    
    /* Erase 256 bytes starting from address 0x300 */
    result = Eep_Erase(0x300, 256);
    
    if (result == E_OK) {
        /* Wait for operation to complete */
        while (Eep_GetStatus() == EEP_BUSY) {
            Eep_MainFunction();
        }
        
        if (Eep_GetJobResult() == EEP_JOB_OK) {
            /* Erase successful - memory now contains 0xFF */
        }
    }
}
```

### Cancel Operation

```c
void Eep_Example_Cancel(void)
{
    uint8 buffer[100];
    Std_ReturnType result;
    
    /* Start a long read operation */
    result = Eep_Read(0x400, buffer, 100);
    
    if (result == E_OK) {
        /* After some time, decide to cancel */
        if (some_condition) {
            Eep_Cancel();
            
            if (Eep_GetJobResult() == EEP_JOB_CANCELED) {
                /* Operation was canceled */
            }
        }
    }
}
```

### Complete Read-Modify-Write Cycle

```c
void Eep_Example_ReadModifyWrite(void)
{
    uint8 data[8];
    Std_ReturnType result;
    
    /* Read current data */
    result = Eep_Read(0x500, data, 8);
    if (result != E_OK) return;
    
    while (Eep_GetStatus() == EEP_BUSY) {
        Eep_MainFunction();
    }
    
    if (Eep_GetJobResult() != EEP_JOB_OK) return;
    
    /* Modify data */
    data[0] = 0xAA;
    data[1] = 0xBB;
    
    /* Write back */
    result = Eep_Write(0x500, data, 8);
    if (result != E_OK) return;
    
    while (Eep_GetStatus() == EEP_BUSY) {
        Eep_MainFunction();
    }
    
    /* Verify write result */
    if (Eep_GetJobResult() == EEP_JOB_OK) {
        /* Success */
    }
}
```

## State Machine

```
                    ┌─────────────┐
                    │   UNINIT    │
                    └──────┬──────┘
                           │ Eep_Init()
                           ▼
                    ┌─────────────┐
         ┌─────────│    IDLE     │◄────────┐
         │         └──────┬──────┘         │
         │                │                │
         │   Eep_Read()   │   Eep_Write()  │
         │                │                │
         │                ▼                │
         │   ┌─────────────────────────┐   │
         │   │         BUSY            │   │
         │   │  (READ/WRITE/ERASE)     │   │
         │   └────────────┬────────────┘   │
         │                │                │
         │   Job Complete │   Eep_Cancel() │
         │                │                │
         └────────────────┘────────────────┘
                           │
                    Eep_DeInit()
                           ▼
                    ┌─────────────┐
                    │   UNINIT    │
                    └─────────────┘
```

## Job Processing Flow

```
Application          EEP Driver          EEPROM HW
     │                    │                   │
     │ Eep_Read/Write/    │                   │
     │ Erase()            │                   │
     │───────────────────►│                   │
     │                    │                   │
     │    return E_OK     │                   │
     │◄───────────────────│                   │
     │                    │                   │
     │                    │    (internal)     │
     │                    │                   │
     │ Eep_MainFunction() │                   │
     │    (cyclic call)   │                   │
     │───────────────────►│                   │
     │                    │    Process Job    │
     │                    │──────────────────►│
     │                    │                   │
     │                    │    Complete Job   │
     │                    │◄──────────────────│
     │                    │                   │
     │ Eep_GetStatus()    │                   │
     │───────────────────►│                   │
     │ return EEP_IDLE    │                   │
     │◄───────────────────│                   │
     │                    │                   │
     │ Eep_GetJobResult() │                   │
     │───────────────────►│                   │
     │ return EEP_JOB_OK  │                   │
     │◄───────────────────│                   │
```

## Memory Organization

```
EEPROM Address Space (Example: 64KB EEPROM)

0x08080000 ┌──────────────────┐
           │   Reserved       │  (4KB)
           │   (System Area)  │
0x08081000 ├──────────────────┤
           │   Parameter      │  (16KB)
           │   Storage        │
0x08085000 ├──────────────────┤
           │   User Data      │  (20KB)
           │   Area 1         │
0x0808A000 ├──────────────────┤
           │   User Data      │  (20KB)
           │   Area 2         │
0x0808F000 ├──────────────────┤
           │   Reserved       │  (4KB)
           │   (Wear Leveling)│
0x08090000 └──────────────────┘
```

## Integration Requirements

### Module Dependencies

```
EEP
├── Det (Development Error Tracer) - Optional
│   └── Used for error reporting when EEP_DEV_ERROR_DETECT == STD_ON
├── MemIf (Memory Interface) - Upper Layer
│   └── Abstracts EEPROM/Flash access
└── NvM (NVRAM Manager) - Upper Layer
    └── Manages non-volatile data storage
```

### File Structure

```
mcal/eep/
├── include/
│   ├── Eep.h              /* Public header */
│   └── Eep_Cfg.h          /* Configuration header */
└── src/
    └── Eep.c              /* Implementation */
```

## Testing

### Unit Test Coverage

| Test Category | Coverage |
|--------------|----------|
| Initialization | 100% |
| Read Operations | 100% |
| Write Operations | 100% |
| Erase Operations | 100% |
| Cancel Operations | 100% |
| Status/Result Queries | 100% |
| Error Handling | 100% |
| **Total** | **> 80%** |

### Test Scenarios

1. **Initialization Tests**
   - Valid configuration
   - NULL pointer error detection

2. **Read Operation Tests**
   - Normal read operations
   - Uninitialized driver error
   - NULL pointer error
   - Zero length error
   - Busy state rejection

3. **Write Operation Tests**
   - Normal write operations
   - Uninitialized driver error
   - NULL pointer error
   - Zero length error
   - Busy state rejection

4. **Erase Operation Tests**
   - Normal erase operations
   - Uninitialized driver error
   - Busy state rejection

5. **Cancel Operation Tests**
   - Cancel ongoing job
   - Cancel when uninitialized
   - Cancel when idle

6. **Status Query Tests**
   - UNINIT state
   - IDLE state
   - BUSY state (READ/WRITE/ERASE)

7. **Job Result Tests**
   - JOB_OK
   - JOB_PENDING
   - JOB_CANCELED

8. **Main Function Tests**
   - Process read job
   - Process write job
   - Process erase job
   - Handle uninitialized state

## Performance Considerations

### Timing Parameters

| Operation | Typical Time | Notes |
|-----------|--------------|-------|
| Read | 10-100 us | Depends on length |
| Write | 10-20 ms | Per page |
| Erase | 20-50 ms | Per sector |

### Memory Usage

| Component | Size | Notes |
|-----------|------|-------|
| Code | ~2 KB | Depends on configuration |
| RAM | ~64 B | State variables, buffers |

## Limitations

1. **Single Job**: Only one EEPROM operation can be active at a time
2. **No Interrupt Support**: All operations are polled via Eep_MainFunction()
3. **No Callback**: No job end notification callback implemented
4. **Simple Error Handling**: Limited error recovery mechanisms

## References

1. AUTOSAR Specification of EEPROM Driver
2. AUTOSAR Layered Software Architecture
3. Microcontroller Reference Manual (EEPROM Controller)
4. AUTOSAR SWS Development Error Tracer

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-05-15 | Initial implementation |

## Authors

- YuleTech AutoSAR Team