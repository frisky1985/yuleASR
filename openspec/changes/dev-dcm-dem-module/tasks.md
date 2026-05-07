# DCM & DEM Module Development Tasks

## Status: Completed

---

## Tasks

### OpenSpec Specification
- [x] Create `openspec/changes/dev-dcm-dem-module/specs/Dcm_spec.md`
  - [x] Module overview (UDS protocol, session management, security access, service dispatch)
  - [x] UDS service list (0x10, 0x11, 0x19, 0x22, 0x2E, 0x31, 0x34, 0x36, 0x37, 0x3E)
  - [x] API list (Init, DeInit, GetVersionInfo, MainFunction, RxIndication, TxConfirmation, etc.)
  - [x] Data types (Dcm_SesCtrlType, Dcm_SecLevelType, Dcm_ProtocolType, Dcm_ConfigType, etc.)
  - [x] DET error handling documentation
  - [x] Scenario 1: Default Session Diagnostics
  - [x] Scenario 2: Extended Session Security Access
  - [x] Scenario 3: Read DTC Information
  - [x] Scenario 4: Software Flash Programming
- [x] Create `openspec/changes/dev-dcm-dem-module/specs/Dem_spec.md`
  - [x] Module overview (DTC management, events, freeze frames, extended data, aging, debounce)
  - [x] API list (Init, DeInit, SetEventStatus, GetEventStatus, GetDTCStatus, ClearDTC, etc.)
  - [x] Data types (Dem_EventIdType, Dem_DTCType, Dem_EventStatusType, etc.)
  - [x] Debounce algorithm documentation (counter-based, time-based, monitor-internal)
  - [x] DTC status byte documentation
  - [x] Scenario 1: Event Reporting and Debounce
  - [x] Scenario 2: DTC Status Update (Clear DTC)
  - [x] Scenario 3: Freeze Frame Storage
  - [x] Scenario 4: Fault Aging

### Code Fixes and Enhancements
- [x] Fix `Dcm_ConfigType` in `Dcm.h`: add `NumDIDs`, `NumRIDs`, `DIDs`, `RIDs` fields
- [x] Fix `Dem_Cfg.h`: add `DEM_NUM_ENABLE_CONDITIONS`, `DEM_NUM_STORAGE_CONDITIONS`, `DEM_AGING_CYCLE_COUNTER_THRESHOLD`
- [x] Fix `Dem.c`: `ConfigPtr->Events` -> `ConfigPtr->EventParameters`
- [x] Fix `Dcm.c`: `Dem_InternalState.ConfigPtr->DtcParameters` -> `Dem_Config.DtcParameters` (layer compliance)
- [x] Add `#include "Dem.h"` to `Dcm.c`
- [x] Add UDS 0x34 Request Download handler to `Dcm.c`
- [x] Add UDS 0x36 Transfer Data handler to `Dcm.c`
- [x] Add UDS 0x37 Request Transfer Exit handler to `Dcm.c`
- [x] Initialize transfer state in `Dcm_Init`
- [x] Add transfer state fields to `Dcm_InternalStateType`

### Unit Tests
- [x] Create `src/bsw/services/dcm/src/Dcm_test.c`
  - [x] Dcm_Init success
  - [x] Dcm_Init NULL config DET error
  - [x] UDS 0x10 session switch Default -> Extended
  - [x] UDS 0x22 read DID data
  - [x] UDS 0x3E Tester Present
  - [x] Uninitialized API error detection
  - [x] GetVersionInfo validation
- [x] Create `src/bsw/services/dem/src/Dem_test.c`
  - [x] Dem_Init success
  - [x] Dem_Init NULL config DET error
  - [x] Dem_SetEventStatus Passed/Failed
  - [x] Debounce counter increment to failed threshold
  - [x] Debounce counter decrement
  - [x] DTC status byte update (confirmed DTC)
  - [x] DTC clear
  - [x] Uninitialized API error detection
  - [x] GetVersionInfo validation

### Git
- [x] Git add all changes
- [x] Git commit with conventional message
- [x] Git push to origin/master

---

## Verification Checklist

- [x] All OpenSpec scenarios documented
- [x] UDS services 0x10-0x3E covered in specification
- [x] Code compiles without structural errors
- [x] DET error detection documented and implemented
- [x] Unit tests follow project test framework style
- [x] Naming conventions follow YuleTech AutoSAR BSW standards
