# MCAL Hardware Abstraction Layer

## Overview

This directory contains the Hardware Abstraction Layer (HAL) implementation for the yuleASR Classic AUTOSAR BSW platform. The HAL provides platform-independent interfaces for MCAL modules, supporting multiple ARM Cortex-M platforms through conditional compilation.

## Supported Platforms

### Flash Driver (Fls_Hw)
- **STM32** (F4/F7/H7 series) - Internal Flash Controller
- **NXP i.MX RT** - FlexSPI controller
- **NXP S32K** - Flash controller with FCCOB interface
- **GENERIC** - Mock implementation for unit testing

### Watchdog Driver (Wdg_Hw)
- **STM32** - IWDG (Independent Watchdog) and WWDG (Window Watchdog)
- **NXP i.MX RT** - WDOG controller
- **NXP S32K** - WDOG with unlock sequence
- **GENERIC** - Mock implementation for unit testing

## Platform Selection

Select the target platform by defining one of the following compiler flags:

```c
#define STM32       /* For STM32F4/F7 series */
#define STM32H7     /* For STM32H7 specific */
#define NXP_IMXRT   /* For i.MX RT series */
#define NXP_S32K    /* For S32K series */
#define GENERIC     /* Default: Mock implementation for testing */
```

### Compiler Flags Example

GCC:
```bash
gcc -DSTM32 -c Fls_Hw.c -o Fls_Hw.o
gcc -DNXP_IMXRT -c Wdg_Hw.c -o Wdg_Hw.o
```

## File Structure

```
src/bsw/mcal/
├── fls/
│   ├── include/
│   │   ├── Fls.h           /* Main Flash Driver interface */
│   │   ├── Fls_Cfg.h       /* Flash configuration */
│   │   └── Fls_Hw.h        /* Hardware abstraction header */
│   └── src/
│       ├── Fls.c           /* Main Flash Driver implementation */
│       └── Fls_Hw.c        /* Hardware abstraction implementation */
├── wdg/
│   ├── include/
│   │   ├── Wdg.h           /* Main Watchdog Driver interface */
│   │   ├── Wdg_Cfg.h       /* Watchdog configuration */
│   │   └── Wdg_Hw.h        /* Hardware abstraction header */
│   └── src/
│       ├── Wdg.c           /* Main Watchdog Driver implementation */
│       └── Wdg_Hw.c        /* Hardware abstraction implementation */
```

## API Reference

### Flash Hardware Abstraction API

#### Initialization
```c
Std_ReturnType Fls_Hw_Init(const Fls_Hw_ConfigType* ConfigPtr);
Std_ReturnType Fls_Hw_DeInit(void);
```

#### Flash Operations
```c
Std_ReturnType Fls_Hw_EraseSector(uint32 SectorAddress);
Std_ReturnType Fls_Hw_WriteWord(uint32 Address, uint32 Data);
Std_ReturnType Fls_Hw_WriteBuffer(uint32 Address, const uint8* DataPtr, uint32 Length);
Std_ReturnType Fls_Hw_ReadWord(uint32 Address, uint32* DataPtr);
Std_ReturnType Fls_Hw_ReadBuffer(uint32 Address, uint8* DataPtr, uint32 Length);
```

#### Status and Control
```c
Fls_Hw_StatusType Fls_Hw_GetStatus(void);
Fls_Hw_ErrorType Fls_Hw_GetLastError(void);
Std_ReturnType Fls_Hw_Unlock(void);
Std_ReturnType Fls_Hw_Lock(void);
Std_ReturnType Fls_Hw_WaitForOperation(uint32 TimeoutMs);
void Fls_Hw_ClearFlags(void);
```

#### Sector Management
```c
uint32 Fls_Hw_GetSectorNumber(uint32 Address);
uint32 Fls_Hw_GetSectorSize(uint32 SectorNumber);
```

#### Verification
```c
Std_ReturnType Fls_Hw_Verify(uint32 Address, const uint8* DataPtr, uint32 Length);
```

#### Interrupt Handler
```c
void Fls_Hw_IRQHandler(void);
```

### Watchdog Hardware Abstraction API

#### Initialization
```c
Std_ReturnType Wdg_Hw_Init(const Wdg_Hw_ConfigType* ConfigPtr);
Std_ReturnType Wdg_Hw_DeInit(void);
```

#### Watchdog Operations
```c
Std_ReturnType Wdg_Hw_SetTriggerCondition(uint16 Timeout);
Std_ReturnType Wdg_Hw_Trigger(void);
Std_ReturnType Wdg_Hw_Disable(void);
```

#### Status and Information
```c
Wdg_Hw_StatusType Wdg_Hw_GetStatus(void);
boolean Wdg_Hw_IsEnabled(void);
uint32 Wdg_Hw_GetCounter(void);
Wdg_Hw_ResetReasonType Wdg_Hw_GetResetReason(void);
```

#### Configuration
```c
Std_ReturnType Wdg_Hw_SetWindow(uint32 StartValue, uint32 EndValue);
Std_ReturnType Wdg_Hw_SetEarlyWarningInterrupt(uint32 Threshold);
void Wdg_Hw_ClearInterruptFlag(void);
```

#### Interrupt Handler
```c
void Wdg_Hw_IRQHandler(void);
```

## Configuration Types

### Flash Hardware Configuration
```c
typedef struct {
    uint32 flashBaseAddress;    /* Flash base address */
    uint32 flashSize;           /* Total flash size */
    uint32 sectorCount;         /* Number of sectors */
    uint32 pageSize;            /* Programming page size */
    boolean useInterrupts;      /* Enable interrupt mode */
    uint32 timeoutMs;           /* Operation timeout in ms */
    uint32 clockFreqHz;         /* Flash controller clock */
} Fls_Hw_ConfigType;
```

### Watchdog Hardware Configuration
```c
typedef enum {
    WDG_HW_TYPE_NONE = 0,
    WDG_HW_TYPE_IWDG,           /* Independent Watchdog */
    WDG_HW_TYPE_WWDG,           /* Window Watchdog */
    WDG_HW_TYPE_EXTERNAL        /* External watchdog */
} Wdg_Hw_TypeType;

typedef struct {
    Wdg_Hw_TypeType wdgType;
    union {
        Wdg_Hw_IwdgConfigType iwdg;
        Wdg_Hw_WwdgConfigType wwdg;
    } config;
    boolean disableAllowed;
} Wdg_Hw_ConfigType;
```

## Usage Examples

### Flash Hardware Initialization
```c
#include "Fls_Hw.h"

Fls_Hw_ConfigType flashConfig = {
    .flashBaseAddress = 0x08000000u,
    .flashSize = 0x00100000u,       /* 1 MB */
    .sectorCount = 16u,
    .pageSize = 4u,                 /* 32-bit words */
    .useInterrupts = FALSE,
    .timeoutMs = 1000u,
    .clockFreqHz = 16000000u
};

void Flash_Init(void)
{
    Std_ReturnType result = Fls_Hw_Init(&flashConfig);
    if (result != E_OK) {
        /* Handle error */
    }
}

void Flash_WriteData(uint32 address, const uint8* data, uint32 length)
{
    /* Unlock flash */
    Fls_Hw_Unlock();
    
    /* Erase sector first */
    Fls_Hw_EraseSector(address);
    
    /* Write data */
    Fls_Hw_WriteBuffer(address, data, length);
    
    /* Lock flash */
    Fls_Hw_Lock();
}
```

### Watchdog Hardware Initialization
```c
#include "Wdg_Hw.h"

Wdg_Hw_ConfigType wdgConfig = {
    .wdgType = WDG_HW_TYPE_IWDG,
    .config.iwdg = {
        .baseAddress = 0x40003000u,
        .clockFreqHz = 32000u,
        .useInterrupt = FALSE,
        .windowModeEnabled = FALSE,
        .windowStart = 0u,
        .windowEnd = 1000u,     /* 1 second timeout */
        .prescaler = 0u
    },
    .disableAllowed = FALSE
};

void Watchdog_Init(void)
{
    Std_ReturnType result = Wdg_Hw_Init(&wdgConfig);
    if (result != E_OK) {
        /* Handle error */
    }
}

void Watchdog_Refresh(void)
{
    Wdg_Hw_Trigger();
}
```

## Unit Testing

Unit tests are provided in the `tests/unit/mcal/` directory:

- `test_fls_hw.c` - Flash hardware abstraction tests
- `test_wdg_hw.c` - Watchdog hardware abstraction tests

### Running Tests

```bash
cd /home/admin/yuleASR

# Compile and run Flash tests
gcc -DGENERIC -I src/bsw/common -I src/bsw/mcal/fls/include \
    src/bsw/mcal/fls/src/Fls_Hw.c tests/unit/mcal/test_fls_hw.c -o test_fls_hw
./test_fls_hw

# Compile and run Watchdog tests
gcc -DGENERIC -I src/bsw/common -I src/bsw/mcal/wdg/include \
    src/bsw/mcal/wdg/src/Wdg_Hw.c tests/unit/mcal/test_wdg_hw.c -o test_wdg_hw
./test_wdg_hw
```

## Integration Notes

1. **Critical Sections**: The HAL uses macros `FLS_HW_ENTER_CRITICAL()` and `FLS_HW_EXIT_CRITICAL()` for interrupt protection. These should be mapped to OS-specific critical section functions.

2. **Error Handling**: All HAL APIs integrate with the DET (Development Error Tracer) module when `FLS_DEV_ERROR_DETECT` or `WDG_DEV_ERROR_DETECT` is enabled.

3. **Register Access**: Platform-specific register access is implemented using volatile pointers. Ensure proper memory barriers are used for cache-coherent systems.

4. **Timeout Handling**: All blocking operations include timeout handling based on the configured timeout value.

## Compliance

- AUTOSAR R22-11
- MISRA C:2012
- ISO 26262 (ASIL-D ready)

## License

Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
