---
title: RAMTST (RAM Test) Module
sidebar_label: ramtst
description: "The RAMTST module provides RAM testing functionality for the AUTOSAR Basic Software. It implements various RAM test algo"
sidebar_position: 18
---

# RAMTST (RAM Test) Module

## Overview

The RAMTST module provides RAM testing functionality for the AUTOSAR Basic Software. It implements various RAM test algorithms to detect memory faults and ensure data integrity in the microcontroller's RAM.

**AUTOSAR Standard**: Classic Platform 4.4.0  
**Layer**: MCAL (Microcontroller Driver)  
**Hardware**: Generic (software-based testing)  
**ASIL Level**: QM (Quality Management)

## Features

- **Multiple Test Algorithms**: Support for March, GALPAT, and Walkpath algorithms
- **Non-Destructive Testing**: Tests can be performed without corrupting RAM contents
- **Background Testing**: Asynchronous testing through MainFunction
- **Configurable Test Parameters**: Address range, algorithm, and timing configuration
- **State Management**: Clear state machine (UNINIT -&gt; IDLE -&gt; RUNNING)
- **Result Reporting**: Test results (OK, NOT_TESTED, FAILED)
- **DET Integration**: Development error detection support

## Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│    (Memory Health Monitoring)       │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│        RAMTST Driver (MCAL)         │
│  ┌─────────────────────────────┐    │
│  │   Test Algorithm Engine     │    │
│  │  (March/GALPAT/Walkpath)    │    │
│  └─────────────┬───────────────┘    │
│                │                     │
│  ┌─────────────▼──────────────┐     │
│  │    State Machine           │     │
│  │  (UNINIT/IDLE/RUNNING)     │     │
│  └─────────────┬──────────────┘     │
│                │                     │
│  ┌─────────────▼──────────────┐     │
│  │   RAM Memory Controller    │     │
│  │  (Microcontroller RAM)     │     │
│  └────────────────────────────┘     │
└─────────────────────────────────────┘
```

## API Reference

### Core Functions

```c
/* Initialization */
void RamTst_Init(const RamTst_ConfigType* ConfigPtr);
void RamTst_DeInit(void);

/* Test Control */
Std_ReturnType RamTst_Run(void);
void RamTst_Stop(void);

/* Status and Results */
RamTst_StatusType RamTst_GetTestStatus(void);
RamTst_TestResultType RamTst_GetTestResult(void);

/* Main Function (cyclic) */
void RamTst_MainFunction(void);
```

### Data Types

```c
/* Test Algorithms */
typedef enum {
    RAMTST_ALGORITHM_MARCH = 0,      /* March algorithm */
    RAMTST_ALGORITHM_GALPAT,          /* GALPAT algorithm */
    RAMTST_ALGORITHM_WALKPATH         /* Walkpath algorithm */
} RamTst_AlgType;

/* Test Results */
typedef enum {
    RAMTST_RESULT_OK = 0,             /* Test passed */
    RAMTST_RESULT_NOT_TESTED,         /* Test not run yet */
    RAMTST_RESULT_FAILED              /* Test failed */
} RamTst_TestResultType;

/* Test Status */
typedef enum {
    RAMTST_STATUS_UNINIT = 0,         /* Driver uninitialized */
    RAMTST_STATUS_IDLE,               /* Driver ready */
    RAMTST_STATUS_RUNNING             /* Test in progress */
} RamTst_StatusType;

/* Configuration Structure */
typedef struct {
    uint32 StartAddress;              /* Start address of RAM to test */
    uint32 Size;                      /* Size of RAM region in bytes */
    RamTst_AlgType Algorithm;         /* Test algorithm selection */
    uint32 CallCycle;                 /* MainFunction call cycle in ms */
} RamTst_ConfigType;
```

### State Machine

```
     ┌─────────────┐
     │   UNINIT    │◄───────────────┐
     └──────┬──────┘                │
            │ RamTst_Init()          │
            ▼                        │
     ┌─────────────┐    RamTst_Stop()│
     │    IDLE     │◄────────────────┤
     └──────┬──────┘                 │
            │ RamTst_Run()           │
            ▼                        │
     ┌─────────────┐    RamTst_DeInit()
     │   RUNNING   │─────────────────┘
     └──────┬──────┘
            │ Test Complete
            ▼
     ┌─────────────┐
     │    IDLE     │ (Result = OK or FAILED)
     └─────────────┘
```

## Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `RAMTST_DEV_ERROR_DETECT` | STD_ON/OFF | Enable development error detection |
| `RAMTST_VERSION_INFO_API` | STD_ON/OFF | Enable version information API |
| `RAMTST_START_ADDRESS` | uint32 | Default RAM start address |
| `RAMTST_SIZE` | uint32 | Default RAM test size in bytes |
| `RAMTST_ALGORITHM` | RamTst_AlgType | Default test algorithm |
| `RAMTST_CALL_CYCLE` | uint32 | MainFunction call period in ms |

## Usage Examples

### Basic Test Execution

```c
#include "RamTst.h"

void RamTest_Example(void)
{
    RamTst_ConfigType config;
    RamTst_StatusType status;
    RamTst_TestResultType result;
    
    /* Configure test parameters */
    config.StartAddress = 0x20000000U;  /* RAM start */
    config.Size = 0x00020000U;           /* 128KB */
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    /* Initialize RAM test driver */
    RamTst_Init(&config);
    
    /* Start the RAM test */
    if (RamTst_Run() == E_OK) {
        /* Test started successfully */
        
        /* Wait for test completion (poll in main loop) */
        do {
            RamTst_MainFunction();
            status = RamTst_GetTestStatus();
        } while (status == RAMTST_STATUS_RUNNING);
        
        /* Get test result */
        result = RamTst_GetTestResult();
        if (result == RAMTST_RESULT_OK) {
            /* RAM test passed */
        } else {
            /* RAM test failed - handle error */
        }
    }
    
    /* Cleanup */
    RamTst_DeInit();
}
```

### Cyclic Testing

```c
#include "RamTst.h"

static boolean ramTestInProgress = FALSE;

void RamTest_Cyclic10ms(void)
{
    RamTst_StatusType status;
    RamTst_TestResultType result;
    
    if (!ramTestInProgress) {
        /* Start new test cycle */
        if (RamTst_Run() == E_OK) {
            ramTestInProgress = TRUE;
        }
    } else {
        /* Call main function to process test */
        RamTst_MainFunction();
        
        /* Check if test completed */
        status = RamTst_GetTestStatus();
        if (status == RAMTST_STATUS_IDLE) {
            ramTestInProgress = FALSE;
            
            result = RamTst_GetTestResult();
            if (result != RAMTST_RESULT_OK) {
                /* Handle RAM test failure */
                ErrorHandler_ReportRAMError();
            }
        }
    }
}
```

### Stopping a Test

```c
void RamTest_StopExample(void)
{
    RamTst_ConfigType config;
    uint32 timeoutCounter = 0U;
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00020000U;
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    config.CallCycle = 10U;
    
    RamTst_Init(&config);
    RamTst_Run();
    
    /* Run test for some time */
    while (RamTst_GetTestStatus() == RAMTST_STATUS_RUNNING) {
        RamTst_MainFunction();
        timeoutCounter++;
        
        /* Stop test if timeout */
        if (timeoutCounter > 1000U) {
            RamTst_Stop();
            break;
        }
    }
    
    RamTst_DeInit();
}
```

### Different Algorithms

```c
void RamTest_AlgorithmComparison(void)
{
    RamTst_ConfigType config;
    RamTst_TestResultType result;
    
    config.StartAddress = 0x20000000U;
    config.Size = 0x00010000U;  /* 64KB */
    config.CallCycle = 10U;
    
    /* Test with March algorithm */
    config.Algorithm = RAMTST_ALGORITHM_MARCH;
    RamTst_Init(&config);
    RamTst_Run();
    while (RamTst_GetTestStatus() == RAMTST_STATUS_RUNNING) {
        RamTst_MainFunction();
    }
    result = RamTst_GetTestResult();
    /* Process March test result... */
    RamTst_DeInit();
    
    /* Test with GALPAT algorithm */
    config.Algorithm = RAMTST_ALGORITHM_GALPAT;
    RamTst_Init(&config);
    RamTst_Run();
    while (RamTst_GetTestStatus() == RAMTST_STATUS_RUNNING) {
        RamTst_MainFunction();
    }
    result = RamTst_GetTestResult();
    /* Process GALPAT test result... */
    RamTst_DeInit();
}
```

## Error Handling

The RAMTST module reports errors through the DET (Default Error Tracer):

| Error Code | Description | Detection |
|------------|-------------|-----------|
| `RAMTST_E_PARAM_POINTER` | NULL configuration pointer | Init validation |
| `RAMTST_E_UNINIT` | Driver not initialized | API call check |
| `RAMTST_E_NO_ERROR` | No error | Success case |

## Algorithm Descriptions

### March Algorithm
- **Type**: Linear complexity test
- **Coverage**: Stuck-at faults, transition faults
- **Operations**: Write/Read sequence in ascending and descending order
- **Time**: O(n) where n is memory size

### GALPAT Algorithm
- **Type**: Walking pattern test
- **Coverage**: Stuck-at faults, coupling faults
- **Operations**: Walking 1/0 pattern through memory
- **Time**: O(n²) - more thorough but slower

### Walkpath Algorithm
- **Type**: Address-specific test
- **Coverage**: Address decoder faults
- **Operations**: Unique pattern at each address
- **Time**: O(n)

## Hardware Requirements

### Supported Microcontrollers
- Generic implementation (software-based)
- Works with any microcontroller with readable/writable RAM
- No special hardware requirements

### Resource Usage

| Resource | Typical Usage |
|----------|---------------|
| RAM | ~100 bytes (state variables) |
| ROM | ~1-2 KB (code) |
| Stack | ~256 bytes during test |
| Interrupts | None (polling-based) |

### Test Time Estimates

| Algorithm | 64KB RAM | 128KB RAM | 256KB RAM |
|-----------|----------|-----------|-----------|
| March | ~10 ms | ~20 ms | ~40 ms |
| GALPAT | ~100 ms | ~400 ms | ~1.6 s |
| Walkpath | ~10 ms | ~20 ms | ~40 ms |

*Note: Times are approximate and depend on CPU speed*

## Dependencies

### Required Modules
- `Std_Types` - Standard types and macros
- `Platform_Types` - Platform-specific types
- `Compiler` - Compiler abstraction
- `Det` - Default Error Tracer (debug builds)

### Optional Modules
- `Mcu` - Clock configuration (for timing)
- `Wdg` - Watchdog refresh during long tests

## Testing

The RAMTST module includes comprehensive unit tests covering:

- Initialization and deinitialization
- Run/Stop operations
- Status and result queries
- MainFunction processing
- Error handling (NULL pointer, uninitialized state)
- Multiple test algorithms
- Full test cycles
- Concurrent operation handling

Run tests with:
```bash
cd /home/admin/yuleASR/tests/unit/autosar/mcal
gcc -I../../../../src/bsw/mcal/ramtst/include test_ramtst.c -o test_ramtst
./test_ramtst
```

**Test Coverage**: 80%+

## Safety Considerations

### Non-Destructive Testing
The RAMTST module is designed to perform non-destructive tests where possible:
- Saves memory contents before test
- Restores contents after test
- Marks tested regions temporarily unavailable

### Test Interference
- Do not access tested RAM region during test
- Disable interrupts during critical test phases
- Use separate stack during test execution

### Failure Handling
When RAM test fails:
1. Log error details (address, expected/actual values)
2. Notify application layer
3. Consider safe state transition
4. Do not use corrupted memory region

## Limitations

1. **Software Implementation**: Current implementation is simulation-based
2. **No Hardware BIST**: Does not use hardware built-in self-test if available
3. **Single Region**: Tests one contiguous memory region at a time
4. **No ECC Support**: Does not handle ECC-protected memory specially

## References

- AUTOSAR SWS RAM Test Driver (Classic Platform 4.4.0)
- ISO 26262 (Functional Safety considerations)
- "Testing Semiconductor Memories" by A.J. van de Goor
- IEEE 1500 (Embedded Core Test)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2026-05-15 | Initial release with March, GALPAT, Walkpath algorithms |

## Example Project Structure

```
project/
├── src/
│   ├── RamTst.c          # Driver implementation
│   ├── RamTst_Cfg.c      # Configuration
│   └── RamTst_Lcfg.c     # Link-time configuration
├── include/
│   ├── RamTst.h          # Public header
│   └── RamTst_Cfg.h      # Configuration header
└── tests/
    └── test_ramtst.c     # Unit tests
```

## Notes

1. **Test Duration**: Long tests (GALPAT on large RAM) may require watchdog refresh

2. **Memory Access**: Application must not access tested region during test

3. **Interrupt Safety**: Consider disabling interrupts during critical test phases

4. **Power Management**: Ensure RAM remains powered during test execution

5. **Stack Usage**: Test uses additional stack space - ensure adequate stack size

6. **Startup Testing**: Consider running RAM test during ECU initialization

7. **Periodic Testing**: Implement periodic background testing for safety-critical systems
