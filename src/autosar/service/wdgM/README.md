# Watchdog Manager (WdgM) Module

## Overview

This directory contains the implementation of the AUTOSAR R22-11 Watchdog Manager (WdgM) Service Layer module. The WdgM provides comprehensive supervision capabilities for safety-critical automotive applications with ASIL-D compliance.

## Features

### Core Supervision Types

1. **Alive Supervision**
   - Monitors cyclic execution of supervised entities
   - Configurable expected alive indications per supervision cycle
   - Configurable min/max margins for tolerance
   - Supports up to 4 alive supervisions per supervised entity

2. **Deadline Supervision**
   - Monitors execution time between checkpoints
   - Configurable minimum and maximum deadlines
   - Tracks start and end checkpoint timestamps
   - Supports up to 2 deadline supervisions per supervised entity

3. **Logical Supervision**
   - Program flow monitoring via checkpoint transitions
   - Graph-based transition validation
   - Supports external and internal checkpoints
   - Configurable transition tables per supervised entity

### Mode Management

- **OFF Mode**: All supervision deactivated, watchdog disabled
- **SLOW Mode**: Reduced supervision for low-power operation
- **FAST Mode**: Full supervision for normal operation
- Mode-specific SE activation/deactivation
- Dynamic mode switching with BswM integration

### Supervised Entities

- Maximum 32 supervised entities (SEs)
- Per-SE failure tolerance configuration
- Individual local status tracking (OK, FAILED, EXPIRED, DEACTIVATED)
- SE-specific supervision configuration per mode

### Integration

- **WdgIf Integration**: Hardware watchdog control via WdgIf_SetMode/Trigger
- **BswM Integration**: Mode request ports and action lists
- **EcuM Integration**: Startup/shutdown mode control
- **DEM Integration**: Production error reporting (optional)

## Architecture

### Module Structure

```
wdgM/
├── wdgM.h           # Main WdgM API header
├── wdgM.c           # Core implementation
├── wdgM_Cfg.h       # Configuration switches and parameters
├── wdgM_Lcfg.c      # Link-time configuration (SEs, Modes)
├── wdgM_BswM.h      # BswM integration header
├── wdgM_BswM.c      # BswM integration implementation
├── wdgIf.h          # Watchdog interface header
├── wdgIf.c          # Watchdog interface implementation
├── CMakeLists.txt   # Build configuration
└── README.md        # This file
```

### Configuration Example

The `wdgM_Lcfg.c` provides a reference configuration with:

- **14 Supervised Entities**:
  - OS Task 1ms, 10ms, 100ms
  - Com Main, Dcm Main, Dem Main, NvM Main
  - BswM Main
  - Application Task 1 & 2
  - Ethernet RX/TX
  - DDS Main
  - SecOC Main

- **3 Modes**:
  - OFF: All deactivated
  - SLOW: Critical SEs only (OS 10ms, 100ms, NvM, BswM, SecOC)
  - FAST: All SEs with full supervision

## API Reference

### Initialization/Deinitialization

```c
Std_ReturnType WdgM_Init(const WdgM_ConfigType *configPtr);
Std_ReturnType WdgM_DeInit(void);
```

### Mode Management

```c
Std_ReturnType WdgM_SetMode(WdgM_ModeType mode);
Std_ReturnType WdgM_GetMode(WdgM_ModeType *mode);
WdgM_GlobalStatusType WdgM_GetGlobalStatus(void);
```

### Supervision

```c
Std_ReturnType WdgM_CheckpointReached(
    WdgM_SupervisedEntityIdType seId,
    WdgM_CheckpointIdType checkpointId
);

Std_ReturnType WdgM_CheckpointReachedExtended(
    WdgM_SupervisedEntityIdType seId,
    WdgM_CheckpointIdType checkpointId,
    WdgM_CheckpointSourceType source
);

void WdgM_MainFunction(void);
```

### Status Queries

```c
Std_ReturnType WdgM_GetLocalStatus(
    WdgM_SupervisedEntityIdType seId,
    WdgM_LocalStatusType *status
);

Std_ReturnType WdgM_GetFirstExpiredSEID(
    WdgM_SupervisedEntityIdType *seId
);
```

### BswM Integration

```c
void WdgM_RequestMode(WdgM_ModeType mode);
WdgM_ModeType WdgM_GetCurrentMode(void);
```

## Usage Example

```c
#include "autosar/service/wdgM/wdgM.h"
#include "autosar/service/wdgM/wdgM_Cfg.h"

// External configuration
extern const WdgM_ConfigType WdgM_Config;

void Init_WdgM(void)
{
    // Initialize WdgM
    Std_ReturnType result = WdgM_Init(&WdgM_Config);
    if (result != E_OK) {
        // Handle error
    }
}

void Task_10ms(void)
{
    // Report start checkpoint
    WdgM_CheckpointReached(WDGM_SE_ID_OS_TASK_10MS, 
                           WDGM_CP_OS_10MS_START);
    
    // ... task logic ...
    
    // Report intermediate checkpoint
    WdgM_CheckpointReached(WDGM_SE_ID_OS_TASK_10MS, 
                           WDGM_CP_OS_10MS_CHECKPOINT_1);
    
    // ... more task logic ...
    
    // Report end checkpoint
    WdgM_CheckpointReached(WDGM_SE_ID_OS_TASK_10MS, 
                           WDGM_CP_OS_10MS_END);
}

void MainFunction_WdgM(void)
{
    // Call cyclically (typically every 10ms)
    WdgM_MainFunction();
}
```

## Safety Considerations

### ASIL-D Compliance

- Defensive programming with parameter validation
- Safe state transition handling
- Production error reporting to DEM
- Immediate reset on critical failures (configurable)
- Double-inverse protection for critical variables
- Error injection detection

### Error Handling

| Error Code | Description | Action |
|------------|-------------|--------|
| WDGM_E_ALIVE_SUPERVISION | Alive count out of range | Increment failure counter, report to DEM |
| WDGM_E_DEADLINE_SUPERVISION | Deadline exceeded | Increment failure counter, report to DEM |
| WDGM_E_LOGICAL_SUPERVISION | Invalid checkpoint transition | Increment failure counter, report to DEM |
| WDGM_E_SET_MODE | Mode transition failed | Report to DEM, maintain current mode |

## Testing

Unit tests are provided in `tests/unit/classic/test_wdgm.c`:

- Initialization/deinitialization tests
- Mode management tests
- Checkpoint handling tests
- Alive/deadline/logical supervision tests
- Status transition tests
- BswM integration tests
- Error handling tests

Run tests with:
```bash
mkdir build && cd build
cmake -DBUILD_TESTS=ON ..
make
ctest -R wdgm
```

## MISRA C:2012 Compliance

The implementation follows MISRA C:2012 guidelines:

- No dynamic memory allocation
- Explicit type conversions
- Single exit point for functions
- No recursion
- No goto statements
- All code paths have defined behavior
- Defensive programming patterns

## Standards Compliance

- AUTOSAR Classic Platform R22-11
- ASIL-D Safety Level
- MISRA C:2012
- ISO 26262 functional safety

## Future Enhancements

- External logical supervision support
- RTE interface generation
- DEM error reporting integration
- OS resource protection
- Multi-core supervision support

## License

Copyright (c) 2024 - Proprietary
See main project LICENSE file for details.
