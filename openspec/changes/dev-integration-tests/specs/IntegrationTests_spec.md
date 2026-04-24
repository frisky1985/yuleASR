# Cross-Layer Integration Test Framework

## Overview

This specification defines the cross-layer integration test framework for the YuleTech AutoSAR BSW stack. The framework validates end-to-end data flows across all BSW layers:

```
RTE (Run Time Environment)
    ^
Services (Com, PduR, NvM, Dcm, Dem, BswM)
    ^
ECUAL (CanIf, CanTp, MemIf, Fee, Ea, EthIf, LinIf, IoHwAb)
    ^
MCAL (Mcu, Port, Dio, Gpt, Can, Spi, Adc, Pwm, Wdg)
    ^
Hardware (Mocked)
```

### Goals

1. Verify MCAL -> ECUAL -> Service -> RTE data path integrity
2. Validate module initialization sequence (EcuM Phase I/II/III)
3. Test error handling paths through DET mock capture
4. Provide fast, deterministic test execution without hardware dependencies

## Architecture

### Mock/Stub Layer

All hardware dependencies and external interfaces are replaced with lightweight mocks:

| Mock | Replaced Module | Purpose |
|:-----|:----------------|:--------|
| `Can_Mock` | CAN Driver (MCAL) | Virtual CAN message buffer for TX/RX tracking |
| `Fee_Mock` | Flash EEPROM Emulation (ECUAL) | Virtual NV memory array for persistent storage tests |
| `Gpt_Mock` | GPT Driver (MCAL) | Manual time advancement for timing verification |
| `Det_Mock` | Development Error Tracer | Capture and verify DET error reports |
| `OS_Mock` | Operating System | Alarm registration and callback dispatch |
| `Mcu/Port/Dio/Spi/Adc/Pwm/Wdg Stubs` | MCAL Drivers | Minimal no-op implementations |
| `MemIf/Ea/EthIf/LinIf/IoHwAb Stubs` | ECUAL Modules | Minimal no-op implementations |
| `Rte_Mock` | RTE Layer | SWC-facing read interfaces for end-to-end validation |

### Test Configuration

Test-specific configuration objects are provided for:

- **CanIf**: 1 controller, 8 TX/RX PDUs with mapped CAN IDs
- **Com**: 8 signals, 8 IPDUs with EngineSpeed, VehicleSpeed, ThrottlePosition
- **CanTp**: 1 channel with diagnostic NSDU configuration
- **NvM**: 8 NVRAM blocks including EngineConfig block
- **Dcm**: 1 DID (0xF190 VIN) with read function
- **BswM**: 1 rule with RUN/SHUTDOWN mode actions

## Test Scenarios

### Scenario 1: CAN Signal End-to-End

**Name**: `test_can_signal_end_to_end`

**Covered Layers**: MCAL (Can) -> ECUAL (CanIf) -> Service (PduR, Com) -> RTE

**Steps**:
1. Initialize all layers via `setUp` (MCAL -> ECUAL -> Service)
2. Set CAN controller mode to STARTED and PDU mode to ONLINE
3. Construct a CAN frame with Engine Speed = 500 RPM (little-endian 16-bit)
4. Call `CanIf_RxIndication` with CAN ID 0x101 (Engine Command RX)
5. Data flow: CanIf -> PduR_RxIndication -> Com_RxIndication
6. Call `Com_MainFunctionRx` to process received IPDU
7. Call `Com_ReceiveSignal(IT_COM_SIGNAL_ENGINE_SPEED, &value)`
8. Call `Rte_Read_SwcEngineCtrl_PpEngineSpeed_EngineSpeed(&value)`

**Expected Results**:
- `Com_ReceiveSignal` returns 500
- `Rte_Read_SwcEngineCtrl_PpEngineSpeed_EngineSpeed` returns RTE_E_OK with 500
- No DET errors captured

### Scenario 2: NV Data Recovery

**Name**: `test_nvm_startup_recovery`

**Covered Layers**: ECUAL (Fee) -> Service (NvM) -> RTE

**Steps**:
1. Initialize all layers via `setUp`
2. Pre-set `Fee_Mock` memory block 1 with EngineConfig data:
   - maxRpm = 6000, idleSpeed = 800, redlineRpm = 7000, cylinderCount = 4
3. Call `NvM_ReadBlock(IT_NVM_BLOCK_ID_ENGINE_CONFIG, &readConfig)`
4. Poll `NvM_MainFunction` and `Fee_MainFunction` until job completes
5. Verify read data matches pre-set values
6. Call `Rte_Read_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig(&readConfig)`

**Expected Results**:
- `NvM_ReadBlock` accepts the request (E_OK)
- After polling, `NvM_GetErrorStatus` returns NVM_REQ_OK
- Read config matches: maxRpm=6000, idleSpeed=800
- RTE interface returns RTE_E_OK with correct values
- No DET errors captured

### Scenario 3: Diagnostic Request End-to-End

**Name**: `test_diagnostic_request_end_to_end`

**Covered Layers**: MCAL (Can) -> ECUAL (CanIf, CanTp) -> Service (PduR, Dcm)

**Steps**:
1. Initialize all layers via `setUp`
2. Build UDS 0x22 ReadDataByIdentifier request for DID 0xF190 (VIN)
3. Simulate decoded diagnostic payload via `PduR_RxIndication` to DCM
4. Call `Dcm_MainFunction` to process the request
5. Verify DCM generates a positive response transmitted through the stack
6. Check `Can_Mock` TX buffer for response frame with CAN ID 0x708

**Expected Results**:
- Response frame found in CAN TX mock with CAN ID 0x708
- Response payload contains: 0x62 (positive response SID), 0xF1, 0x90, VIN data
- No DET errors captured

### Scenario 4: Mode Switch (BswM -> EcuM)

**Name**: `test_mode_switch_bswm_ecum`

**Covered Layers**: Integration (BswM, EcuM) -> Service (Com, PduR)

**Steps**:
1. Initialize all layers via `setUp`
2. Verify initial BswM mode is RUN
3. Call `BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_SHUTDOWN)`
4. Call `BswM_MainFunction` to execute arbitration and mode actions
5. Verify BswM mode changed to SHUTDOWN
6. Verify Com I-PDU group control was called to stop communication
7. Call `EcuM_RequestShutdown` and `EcuM_MainFunction`
8. Verify EcuM state transitions to SHUTDOWN

**Expected Results**:
- `BswM_GetCurrentMode` returns BSWM_MODE_SHUTDOWN after processing
- `EcuM_GetState` returns ECUM_STATE_SHUTDOWN
- No DET errors captured

### Scenario 5: OS Alarm Cyclic Scheduling

**Name**: `test_os_alarm_cyclic_scheduling`

**Covered Layers**: OS -> Integration (BswM) -> Service (Com, NvM, Dem)

**Steps**:
1. Initialize all layers via `setUp`
2. Set relative alarms:
   - BswM 10ms, Com 10ms, NvM 100ms, Dem 100ms
3. Advance GPT mock time by 10ms
4. Call `Os_Callback_Alarm` for BswM and Com alarms
5. Advance GPT mock time by 100ms
6. Call `Os_Callback_Alarm` for NvM and Dem alarms

**Expected Results**:
- All alarms are active after SetRelAlarm
- Alarm callbacks execute without DET errors
- `mock_det_report_error_called` remains 0 throughout

## Test Coverage

| Layer | Modules Tested | Coverage |
|:------|:---------------|:---------|
| MCAL | Can, Gpt, Mcu, Port, Dio, Spi, Adc, Pwm, Wdg | Initialization + Can TX/RX |
| ECUAL | CanIf, CanTp, MemIf, Fee, Ea, EthIf, LinIf, IoHwAb | Init + CanIf RX/TX + Fee R/W |
| Service | PduR, Com, NvM, Dcm, Dem, BswM | Full routing + signal handling |
| Integration | EcuM, BswM | Startup sequence + mode switch |
| RTE | Rte_Read_* mock interfaces | End-to-end data verification |

## How to Run

### Compilation

```bash
cd src/bsw/integration/tests
# Example GCC command (adjust paths as needed)
gcc -I../../../common/include \
    -I../../mcal/can/include \
    -I../../ecual/canif/include \
    -I../../ecual/cantp/include \
    -I../../services/pdur/include \
    -I../../services/com/include \
    -I../../services/nvm/include \
    -I../../services/dcm/include \
    -I../../services/dem/include \
    -I../../integration \
    -I../../os/include \
    -I../../rte/include \
    -I../../../../tests/unit \
    integration_test.c \
    test_runner.c \
    ../../ecual/canif/src/CanIf.c \
    ../../services/pdur/src/PduR.c \
    ../../services/pdur/src/PduR_Lcfg.c \
    ../../services/com/src/Com.c \
    ../../services/nvm/src/NvM.c \
    ../../services/dcm/src/Dcm.c \
    ../../services/dem/src/Dem.c \
    ../../integration/BswM.c \
    ../../integration/EcuM.c \
    ../../ecual/cantp/src/CanTp.c \
    -o integration_tests.exe
```

### Execution

```bash
./integration_tests.exe
```

Expected output:
```
===============================================
  YuleTech BSW Unit Test Framework
===============================================

  Cross-Layer Integration Test Suite
  MCAL -> ECUAL -> Service -> RTE
  -----------------------------------
  Running: test_can_signal_end_to_end ... PASSED
  Running: test_nvm_startup_recovery ... PASSED
  Running: test_diagnostic_request_end_to_end ... PASSED
  Running: test_mode_switch_bswm_ecum ... PASSED
  Running: test_os_alarm_cyclic_scheduling ... PASSED

===============================================
  Test Results:
    Total:  5
    Passed: 5
    Failed: 0
===============================================
```

## Constraints

- All tests use mocks for hardware dependencies; no real CAN controller or Flash required
- DET error detection is enabled (STD_ON) to validate error paths
- Test configuration uses reduced queue sizes and buffer counts for fast execution
- No modification to tested module source code; only mock linkage and test config
