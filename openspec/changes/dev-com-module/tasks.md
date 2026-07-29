# Com Module Development Tasks

## Overview
Development of Com (Communication) module OpenSpec specification, code enhancement, and unit tests.

## Task Status

| Task | Status | Notes |
|------|--------|-------|
| Create OpenSpec spec directory | Done | `openspec/changes/dev-com-module/specs/` |
| Write Com_spec.md | Done | 8 sections, 4 scenarios |
| Review existing Com.h | Done | Added version macros, status macros, missing API declarations |
| Review existing Com.c | Done | Fixed duplicate Com_TriggerTransmit, implemented Com_InvalidateSignal, Com_InvalidateSignalGroup, Com_ReceptionDMControl, Com_EnableReceptionDM, Com_DisableReceptionDM |
| Fix Com.h/Com.c inconsistencies | Done | Removed duplicate endianness defines, fixed Com_SwitchIpduTxMode signature, removed duplicate COM_INSTANCE_ID |
| Create Com_test.c | Done | 16 test cases, all passing |
| Run unit tests | Done | 16/16 passed |
| Git commit | Pending | |

## Code Changes Summary

### `src/bsw/services/com/include/Com.h`
- Added `COM_SW_MAJOR_VERSION`, `COM_SW_MINOR_VERSION`, `COM_SW_PATCH_VERSION`
- Added `COM_UNINIT` / `COM_INIT` module status macros
- Added declarations for `Com_RxIndication`, `Com_TxConfirmation`, `Com_InvalidateSignal`, `Com_InvalidateSignalGroup`, `Com_EnableReceptionDM`, `Com_DisableReceptionDM`
- Removed duplicate `COM_LITTLE_ENDIAN`, `COM_BIG_ENDIAN`, `COM_OPAQUE` defines (already in Com_Cfg.h)

### `src/bsw/services/com/src/Com.c`
- Removed duplicate `Com_TriggerTransmit` function definition
- Removed duplicate `COM_INSTANCE_ID` local define
- Moved `Com_DMEnabled` array declaration to local variables section
- Implemented `Com_InvalidateSignal` with invalid pattern (0xFF) and DET checks
- Implemented `Com_InvalidateSignalGroup` with invalid pattern and DET checks
- Implemented `Com_ReceptionDMControl`, `Com_EnableReceptionDM`, `Com_DisableReceptionDM`
- Fixed `Com_SwitchIpduTxMode` parameter type from `boolean` to `ComTxModeModeType`
- Fixed `Com_IpduGroupControl` to avoid unused parameter warning

### `src/bsw/services/com/src/Com_test.c` (new)
- Mock for `PduR_Transmit` with call tracking
- Mock for `Det_ReportError` with parameter capture
- 16 test cases covering:
  - Com_Init valid and NULL config
  - Com_DeInit success and uninit error
  - Com_SendSignal trigger PduR transmit, null pointer, uninit
  - Com_ReceiveSignal unpack value
  - Com_SendSignalGroup trigger PduR transmit
  - Com_InvalidateSignal
  - Com_GetVersionInfo valid and null pointer
  - Com_TriggerIPDUSend valid and uninit
  - Com_ReceiveSignal invalid ID
  - Com_SendSignal with PENDING property (no immediate trigger)

## Test Results

```
--- Com Module Tests ---
  Running: com_init_valid_config ... PASSED
  Running: com_init_null_config ... PASSED
  Running: com_deinit_success ... PASSED
  Running: com_deinit_uninit ... PASSED
  Running: com_sendsignal_triggers_pdur ... PASSED
  Running: com_sendsignal_null_pointer ... PASSED
  Running: com_sendsignal_uninit ... PASSED
  Running: com_receivesignal_unpacks_value ... PASSED
  Running: com_sendsignalgroup_triggers_pdur ... PASSED
  Running: com_invalidatesignal ... PASSED
  Running: com_getversioninfo ... PASSED
  Running: com_getversioninfo_null ... PASSED
  Running: com_triggeripdusend ... PASSED
  Running: com_triggeripdusend_uninit ... PASSED
  Running: com_receivesignal_invalid_id ... PASSED
  Running: com_sendsignal_pending_notrigger ... PASSED

Test Results:
    Total:  16
    Passed: 16
    Failed: 0
```

## Checklist

- [x] OpenSpec spec created with 4+ scenarios
- [x] Code checked against AutoSAR 4.x
- [x] DET error handling verified
- [x] Unit tests created with 8+ test cases
- [x] All unit tests passing
- [x] Code style consistent with PduR module
- [x] Git commit ready
