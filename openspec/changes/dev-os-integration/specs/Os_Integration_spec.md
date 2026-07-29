# OS Layer BSW Integration Specification

> **Module:** OS Integration (EcuM, BswM, OS Alarm)
> **Layer:** Integration Layer / OS Layer
> **Standard:** AutoSAR Classic Platform 4.x
> **Platform:** NXP i.MX8M Mini
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

This specification defines the integration between the OS layer and the BSW (Basic Software) stack, covering three key areas:

1. **EcuM (ECU State Manager)** - Phased startup and orderly shutdown sequences
2. **BswM (BSW Mode Manager)** - Mode arbitration and action execution
3. **OS Alarm Integration** - Cyclic scheduling of BSW MainFunctions

### Key Responsibilities
- Coordinate MCAL, ECUAL, and Service layer initialization in the correct order
- Manage ECU lifecycle states: STARTUP -> RUN -> SHUTDOWN/SLEEP
- Arbitrate mode requests from multiple BSW users
- Schedule periodic BSW MainFunctions via OS Alarms

---

## 2. EcuM API List

### 2.1 Core Lifecycle APIs

| API | Description |
|-----|-------------|
| `EcuM_Init(void)` | Phase I: Initialize MCAL drivers, then start OS (never returns) |
| `EcuM_StartupTwo(void)` | Phase II: Initialize ECUAL layer (called after OS start) |
| `EcuM_StartupThree(void)` | Phase III: Initialize Service layer |
| `EcuM_Shutdown(void)` | Orderly shutdown: Service -> ECUAL -> MCAL |
| `EcuM_Startup(void)` | Legacy combined startup (calls Phase I + StartOS) |

### 2.2 State and Wakeup APIs

| API | Description |
|-----|-------------|
| `EcuM_GetState(void)` | Get current ECU state |
| `EcuM_SetWakeupEvent(EcuM_WakeupSourceType)` | Set a wakeup event |
| `EcuM_ClearWakeupEvent(EcuM_WakeupSourceType)` | Clear validated wakeup events |
| `EcuM_ValidateWakeupEvent(EcuM_WakeupSourceType)` | Validate a pending wakeup event |
| `EcuM_GetPendingWakeupEvents(void)` | Get pending wakeup events |
| `EcuM_GetValidatedWakeupEvents(void)` | Get validated wakeup events |
| `EcuM_RequestShutdown(void)` | Request ECU shutdown |
| `EcuM_RequestSleep(uint8)` | Request ECU sleep mode |
| `EcuM_MainFunction(void)` | Periodic processing |
| `EcuM_GetVersionInfo(Std_VersionInfoType*)` | Get version information |

### 2.3 Startup Sequence

```
Reset Vector
    |
    v
EcuM_Init()
    |-- Phase I: MCAL Init
    |   |-- Mcu_Init()
    |   |-- Mcu_InitClock()
    |   |-- Mcu_DistributePllClock()
    |   |-- Port_Init()
    |   |-- Gpt_Init()
    |   |-- Can_Init()
    |   |-- Spi_Init()
    |   |-- Adc_Init()
    |   |-- Pwm_Init()
    |   |-- Wdg_Init()
    |
    v
StartOS(DEFAULT_APPMODE)  <-- Never returns
    |
    v
StartupHook / Init Task
    |
    v
EcuM_StartupTwo()
    |-- Phase II: ECUAL Init
    |   |-- CanIf_Init()
    |   |-- CanTp_Init()
    |   |-- MemIf_Init()
    |   |-- Fee_Init()
    |   |-- Ea_Init()
    |   |-- EthIf_Init()
    |   |-- LinIf_Init()
    |   |-- IoHwAb_Init()
    |
    v
EcuM_StartupThree()
    |-- Phase III: Service Init
    |   |-- PduR_Init()
    |   |-- Com_Init()
    |   |-- NvM_Init()
    |   |-- Dcm_Init()
    |   |-- Dem_Init()
    |   |-- BswM_Init()
    |
    v
BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_RUN)
    |
    v
ECU STATE: RUN
```

### 2.4 Shutdown Sequence

```
EcuM_Shutdown()
    |
    |-- Phase I: Service DeInit (reverse order)
    |   |-- BswM_Deinit()
    |   |-- PduR_DeInit()
    |   |-- Com_DeInit()
    |   |-- Dcm_DeInit()
    |
    v
EcuM_OnGoOffTwo()
    |-- Phase II: ECUAL + MCAL DeInit
    |   |-- CanIf_DeInit()
    |   |-- IoHwAb_DeInit()
    |   |-- Pwm_DeInit()
    |   |-- Adc_DeInit()
    |   |-- Gpt_DeInit()
    |
    v
ShutdownOS(E_OS_OK)
```

---

## 3. BswM API List

### 3.1 Core APIs

| API | Description |
|-----|-------------|
| `BswM_Init(const BswM_ConfigType* ConfigPtr)` | Initialize mode manager |
| `BswM_Deinit(void)` | Deinitialize mode manager |
| `BswM_RequestMode(BswM_UserType, BswM_ModeType)` | Request mode change |
| `BswM_GetCurrentMode(BswM_UserType)` | Get current mode for a user |
| `BswM_GetState(void)` | Get BswM module state |
| `BswM_MainFunction(void)` | Mode arbitration and action execution |
| `BswM_GetVersionInfo(Std_VersionInfoType*)` | Get version information |

### 3.2 Mode Definitions

| Mode | Value | Description |
|------|-------|-------------|
| `BSWM_MODE_STARTUP` | 0x00 | Startup phase |
| `BSWM_MODE_RUN` | 0x01 | Normal run mode |
| `BSWM_MODE_POST_RUN` | 0x02 | Pre-shutdown preparation |
| `BSWM_MODE_SHUTDOWN` | 0x03 | Shutdown mode |
| `BSWM_MODE_SLEEP` | 0x04 | Sleep mode |

### 3.3 Mode Arbitration Logic

The BswM_MainFunction performs the following steps:

1. **Process Mode Requests**: Drain the mode request queue, updating each user's requested mode
2. **Run Arbitration**: Evaluate all user modes, select the highest priority mode
3. **Execute Mode Actions**: If arbitration result changed, execute associated actions:
   - RUN: Start COM I-PDU groups, enable PDU routing
   - SHUTDOWN: Stop COM I-PDU groups, disable PDU routing
   - SLEEP: Prepare communication for sleep
4. **Execute Rules**: Evaluate all configured rules and execute true/false action lists

---

## 4. OS Alarm Configuration

### 4.1 Alarm IDs and Mapping

| Alarm ID | Period | BSW MainFunction(s) |
|----------|--------|---------------------|
| `OsAlarm_BswM_MainFunction` | 10ms | `BswM_MainFunction()` |
| `OsAlarm_Com_MainFunction` | 10ms | `Com_MainFunctionRx()` + `Com_MainFunctionTx()` |
| `OsAlarm_CanIf_MainFunction` | 10ms | `CanIf_MainFunction()` (if available) |
| `OsAlarm_Dcm_MainFunction` | 10ms | `Dcm_MainFunction()` |
| `OsAlarm_NvM_MainFunction` | 100ms | `NvM_MainFunction()` |
| `OsAlarm_Dem_MainFunction` | 100ms | `Dem_MainFunction()` |

### 4.2 Alarm Dispatcher

```c
void Os_Callback_Alarm(AlarmType AlarmID)
{
    switch (AlarmID)
    {
        case OsAlarm_BswM_MainFunction:  BswM_MainFunction(); break;
        case OsAlarm_Com_MainFunction:   Com_MainFunctionRx(); Com_MainFunctionTx(); break;
        case OsAlarm_Dcm_MainFunction:   Dcm_MainFunction(); break;
        case OsAlarm_NvM_MainFunction:   NvM_MainFunction(); break;
        case OsAlarm_Dem_MainFunction:   Dem_MainFunction(); break;
        default: break;
    }
}
```

---

## 5. Error Handling (DET)

### EcuM DET Errors

| Error Code | Value | Description |
|------------|-------|-------------|
| `ECUM_E_UNINIT` | 0x03 | Module not initialized |
| `ECUM_E_STATE_TRANSITION` | 0x04 | Invalid state transition |
| `ECUM_E_PARAM_POINTER` | 0x02 | Null pointer |

### BswM DET Errors

| Error Code | Value | Description |
|------------|-------|-------------|
| `BSWM_E_UNINIT` | 0x06 | Module not initialized |
| `BSWM_E_PARAM_CONFIG` | 0x01 | Invalid config pointer |
| `BSWM_E_REQ_USER_OUT_OF_RANGE` | 0x03 | Invalid requesting user |
| `BSWM_E_REQ_MODE_OUT_OF_RANGE` | 0x04 | Invalid requested mode |
| `BSWM_E_PARAM_POINTER` | 0x02 | Null pointer |

---

## 6. Scenarios

### Scenario 1: ECU Cold Startup

**Description:** Power-on reset triggers the complete phased startup sequence.

**Flow:**
1. Reset vector calls `EcuM_Init()`
2. `EcuM_Init` initializes internal state
3. Phase I: All MCAL drivers initialized in order (Mcu -> Port -> Gpt -> Can -> Spi -> Adc -> Pwm -> Wdg)
4. `StartOS(DEFAULT_APPMODE)` is called, scheduler starts
5. StartupHook/InitTask calls `EcuM_StartupTwo()`
6. Phase II: All ECUAL modules initialized
7. `EcuM_StartupThree()` is called
8. Phase III: All Service modules initialized
9. `BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_RUN)`
10. ECU enters RUN state

**Expected Result:** All BSW modules initialized in correct order, OS running, ECU in RUN state.

### Scenario 2: Mode Switch to RUN

**Description:** BswM arbitrates a mode request from EcuM to switch to RUN.

**Flow:**
1. EcuM calls `BswM_RequestMode(BSWM_USER_ECU_STATE, BSWM_MODE_RUN)`
2. Request is queued in BswM mode request queue
3. `BswM_MainFunction()` processes the queue
4. Arbitration evaluates all user modes, selects RUN
5. Mode action executed: COM I-PDU groups started, PDU routing enabled
6. Configured rules evaluated and executed

**Expected Result:** BswM enters RUN mode, communication enabled.

### Scenario 3: OS Alarm Cyclic Scheduling

**Description:** OS Alarms expire periodically and dispatch to BSW MainFunctions.

**Flow:**
1. `SetRelAlarm(OsAlarm_BswM_MainFunction, 10, 10)` sets 10ms cyclic alarm
2. FreeRTOS timer expires after 10ms
3. `Os_AlarmCallback()` invoked
4. Alarm config lookup finds `OsAlarm_BswM_MainFunction_Callback`
5. Callback calls `Os_Callback_Alarm(OsAlarm_BswM_MainFunction)`
6. Dispatcher calls `BswM_MainFunction()`
7. Same pattern for Com (10ms), Dcm (10ms), NvM (100ms), Dem (100ms)

**Expected Result:** All BSW MainFunctions called at configured periodic rates.

### Scenario 4: ECU Orderly Shutdown

**Description:** Application requests orderly shutdown via EcuM.

**Flow:**
1. Application calls `EcuM_RequestShutdown()`
2. `EcuM_MainFunction()` detects shutdown request
3. `EcuM_Shutdown()` called
4. Phase I: Service layer deinitialized (reverse init order)
5. Phase II: ECUAL and MCAL deinitialized
6. `ShutdownOS(E_OS_OK)` called
7. OS scheduler stops, system halts

**Expected Result:** All modules deinitialized in reverse order, clean shutdown.

---

## 7. Dependencies

### Upper Layer
- **Application / RTE**: Calls EcuM_Init, EcuM_StartupTwo/Three from startup code

### Same Layer (Integration)
- **EcuM**: Coordinates with BswM for mode notifications
- **BswM**: Receives mode requests from EcuM and other BSW users

### Lower Layers
- **MCAL**: Mcu, Port, Dio, Gpt, Can, Spi, Adc, Pwm, Wdg
- **ECUAL**: CanIf, CanTp, MemIf, Fee, Ea, EthIf, LinIf, IoHwAb
- **Service**: PduR, Com, NvM, Dcm, Dem
- **OS**: StartOS, ShutdownOS, Alarm APIs

### Common Modules
- **Det**: Development Error Tracer
- **MemMap**: Memory mapping for section placement

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-04-24 | YuleTech | Initial OS Integration specification |
