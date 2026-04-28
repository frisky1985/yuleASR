# OS Layer BSW Integration - Tasks

## Status

| Item | Status |
|------|--------|
| OpenSpec Spec | Done |
| EcuM Implementation | Done |
| BswM Implementation | Done |
| OS Alarm Integration | Done |
| MemMap Update | Done |
| Unit Tests | Done |
| Git Commit | Pending |

## Completed Tasks

- [x] Read existing EcuM, BswM, OS source files and understand current state
- [x] Improve EcuM module with complete phased startup/shutdown sequence
  - Phase I (EcuM_Init): MCAL init + StartOS
  - Phase II (EcuM_StartupTwo): ECUAL init
  - Phase III (EcuM_StartupThree): Service init + BswM RUN request
  - Orderly shutdown: Service -> ECUAL -> MCAL deinit
  - DET error detection for state transitions
- [x] Improve BswM module with mode management
  - Mode arbitration logic (highest priority wins)
  - Mode action execution (RUN/SHUTDOWN/SLEEP)
  - BswM_GetState API
  - DET error detection
- [x] Add OS Alarm integration with BSW MainFunctions
  - Updated Os_Cfg.h with BSW-specific alarm IDs
  - Updated Os_Cfg.c alarm table with proper periods
  - Implemented Os_Callback_Alarm dispatcher
  - Alarm callbacks for BswM, Com, Dcm, NvM, Dem
- [x] Update MemMap.h with ECUM and BSWM memory partitions
- [x] Create OpenSpec specification (Os_Integration_spec.md)
  - 4 scenarios: cold startup, mode switch, cyclic scheduling, orderly shutdown
- [x] Create unit tests
  - EcuM_test.c: MCAL init order, ECUAL init order, Service init order, shutdown reverse order, state tracking, wakeup events
  - BswM_test.c: Init/Deinit, mode request queuing, action execution, rule evaluation, DET errors, arbitration

## Files Modified / Created

### Source Code
- `src/bsw/integration/EcuM.c` - Rewritten with phased startup/shutdown
- `src/bsw/integration/EcuM.h` - Added StartupTwo/Three APIs
- `src/bsw/integration/BswM.c` - Improved mode arbitration and actions
- `src/bsw/integration/BswM.h` - Added BswM_StateType and GetState API
- `src/bsw/os/src/Os_Cfg.c` - Updated alarm configs and callbacks
- `src/bsw/os/include/Os_Cfg.h` - Updated alarm IDs
- `src/bsw/os/include/Os.h` - Added Os_Callback_Alarm declaration
- `src/bsw/general/inc/MemMap.h` - Added ECUM/BSWM sections

### Tests
- `src/bsw/integration/EcuM_test.c` - Unit tests for EcuM
- `src/bsw/integration/BswM_test.c` - Unit tests for BswM

### Documentation
- `openspec/changes/dev-os-integration/specs/Os_Integration_spec.md` - OpenSpec specification
- `openspec/changes/dev-os-integration/tasks.md` - This file
