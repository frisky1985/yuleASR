> **Module ID**: 0x54  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_DiagnosticEventManager  
> **Source Path**: `src/bsw/services/dem/`  
> **Reference Document**: `docs/modules/DEM.md`  
> **Doc Version**: 1.1  
> **Status**: Approved

# Dem (Diagnostic Event Manager) Design Document

## 1. Overview

The Diagnostic Event Manager (Dem) is responsible for processing diagnostic events reported by software components (SW-Cs) or Basic Software (BSW) modules and storing event-related data in non-volatile memory.

### 1.1 Features

- Event status management (PASSED/FAILED/PREPASSED/PREFAILED)
- Debouncing support (Counter-based, Time-based, Monitor-based)
- DTC (Diagnostic Trouble Code) management
- Freeze frame and extended data recording
- Event memory management (Primary/Mirror/Permanent)
- Operation cycle management
- Indicator (MIL/WIF) management
- UDS status byte handling
- Aging and displacement handling
- Asynchronous event processing with priority queue

### 1.2 Architecture

```
+----------------------------------------------------------+
|                    Application Layer                      |
|  (SW-Cs report events via RTE or direct function calls)   |
+------------------------+----------------+------------------+
                         |                |
                         v                v
+------------------------+---+    +-------+----------+
|    Dem API Layer           |    |   Dem Callback   |
|  (Dem_SetEventStatus,     |    |   (Notifications |
|   Dem_ResetEventStatus,   |    |    to BswM/Fim)  |
|   Dem_GetEventStatus...)  |    +------------------+
+-----------+---------------+
            |
            v
+-----------+---------------+
|    Dem Core Processing    |
|  - Event status handling  |
|  - Debouncing             |
|  - DTC status update      |
|  - Queue management       |
+-----------+---------------+
            |
    +-------+-------+
    |               |
    v               v
+---+-----+   +-----+---+
|   NvM   |   |   Dcm   |
| Interface|   | Interface|
+---------+   +---------+
```

---

## 2. Module Structure

### 2.1 File Organization

Current implementation files:

```
src/bsw/services/dem/
├── include/
│   ├── Dem.h             # Main API header
│   ├── Dem_Types.h       # Type definitions
│   ├── Dem_Cfg.h         # Compile-time configuration
│   ├── Dem_Lcfg.h        # Link-time configuration
│   ├── Dem_Pbcfg.h       # Post-build configuration
│   ├── Dem_Int.h         # Internal definitions
│   └── Dem_Error.h       # Error handling
└── src/
    ├── Dem.c             # Main implementation
    ├── Dem_Int.c         # Internal implementation
    ├── Dem_Cfg.c         # Compile-time configuration tables
    ├── Dem_Pbcfg.c       # Post-build configuration
    └── Dem_test.c        # Unit tests
```

### 2.2 Standards & Dependencies

| Standard | Version | Description |
|----------|---------|-------------|
| AUTOSAR SWS DEM | 4.4.0 | Diagnostic Event Manager specification |
| ISO 14229-1 | - | UDS protocol |
| ISO 15031-5 | - | OBD requirements |

| Dependency | Direction | Purpose |
|------------|-----------|---------|
| Dcm | Upper | Read DTCs, clear DTCs, service requests |
| NvM | Lower | Persistent storage of event memory |
| BswM | Peer | Mode requests on status changes |
| FiM | Peer | Function inhibition based on event status |
| Det | Common | Development error detection |

---

## 3. Key Data Structures

### 3.1 UDS DTC Status Byte

```c
typedef uint8 Dem_UdsStatusByteType;

#define DEM_UDS_STATUS_TF        0x01  /* Test Failed */
#define DEM_UDS_STATUS_TFTOC     0x02  /* Test Failed This Operation Cycle */
#define DEM_UDS_STATUS_PDTC      0x04  /* Pending DTC */
#define DEM_UDS_STATUS_CDTC      0x08  /* Confirmed DTC */
#define DEM_UDS_STATUS_TNCSLC    0x10  /* Test Not Completed Since Last Clear */
#define DEM_UDS_STATUS_TFSLC     0x20  /* Test Failed Since Last Clear */
#define DEM_UDS_STATUS_TNCTOC    0x40  /* Test Not Completed This Operation Cycle */
#define DEM_UDS_STATUS_WIR       0x80  /* Warning Indicator Requested */
```

### 3.2 Internal Event State

```c
typedef struct {
    Dem_EventStatusType LastReportedStatus;
    uint8 DTCStatus;
    Dem_FaultDetectionCounterType FaultDetectionCounter;
    sint16 DebounceCounter;
    boolean TestFailedThisOperationCycle;
    boolean TestCompletedThisOperationCycle;
    uint8 OccurrenceCounter;
    uint8 AgingCounter;
    boolean IsAged;
    uint32 LastReportTimestamp;
    uint32 TimeInCurrentStatus;
} Dem_EventStateType;
```

### 3.3 Event Parameter Configuration

```c
typedef struct {
    Dem_EventIdType EventId;
    Dem_DtcType Dtc;
    uint8 EventPriority;
    boolean EventAvailable;
    boolean EventReporting;
    uint8 EventFailureCycleCounterThreshold;
    uint8 EventConfirmationThreshold;
    Dem_DebounceAlgorithmType DebounceAlgorithm;
    sint16 DebounceCounterFailedThreshold;
    sint16 DebounceCounterPassedThreshold;
    uint16 DebounceTimeFailedThresholdMs;
    uint16 DebounceTimePassedThresholdMs;
} Dem_EventParameterType;
```

### 3.4 DTC Entry

```c
typedef struct {
    Dem_DtcType DTC;
    uint8 Status;
    uint16 OccurrenceCounter;
    uint8 AgingCounter;
    uint8 AgingThreshold;
    boolean IsAged;
    boolean IsSuppressed;
    boolean IsDeleted;
    uint16 NvMBlockId;
    boolean IsNvMDataValid;
} Dem_DTCEntryType;
```

### 3.5 Event Memory Entry

```c
typedef struct {
    Dem_EventIdType EventId;
    uint32 DTC;
    Dem_EventStatusExtendedType EventStatus;
    Dem_DTCStatusMaskType DTCStatus;
    uint16 OccurrenceCounter;
    uint8 AgingCounter;
    uint16 Timestamp;
    boolean ExtendedDataRecorded;
    boolean FreezeFrameRecorded;
} Dem_EventMemoryEntryType;
```

---

## 4. Configuration

### 4.1 Compile-time Configuration (Dem_Cfg.h)

Key configuration parameters:
- `DEM_NUM_EVENTS`: Maximum number of events
- `DEM_NUM_DTCS`: Maximum number of DTCs
- `DEM_EVENT_QUEUE_SIZE`: Event queue size
- `DEM_NUM_OPERATION_CYCLES`: Number of operation cycles
- `DEM_NUM_ENABLE_CONDITIONS`: Number of enable conditions
- `DEM_NUM_STORAGE_CONDITIONS`: Number of storage conditions
- `DEM_DEV_ERROR_DETECT`: Development error detection switch

### 4.2 Link-time Configuration (Dem_Cfg.c / Dem_Lcfg.c)

Static configuration tables:
- Event parameter table (`EventParameters`)
- DTC parameter table (`DtcParameters`)
- Debounce threshold table
- Operation cycle configuration
- Indicator configuration

### 4.3 Post-build Configuration (Dem_Pbcfg.c)

Runtime configurable parameters:
- Callback function pointers
- Memory configuration
- DTC group configuration

---

## 5. Event Processing Flow

### 5.1 Normal Event Report

```
1. SW-C calls Dem_SetEventStatus(EventId, EventStatus)
2. Dem validates parameters (init state, EventId range)
3. Debouncing algorithm processes the status
4. If debounce completed:
   a. Update event status
   b. Update DTC status
   c. Handle freeze frame
   d. Handle extended data
   e. Update indicators
   f. Notify callbacks (BswM, Fim)
5. Queue for NvM write if needed
```

### 5.2 Debouncing Algorithms

#### Counter-based Debouncing

```c
if (EventStatus == DEM_EVENT_STATUS_PREFAILED) {
    Counter += IncrementStep;
    if (Counter >= FailedThreshold) {
        FinalStatus = DEM_EVENT_STATUS_FAILED;
    }
} else if (EventStatus == DEM_EVENT_STATUS_PREPASSED) {
    Counter -= DecrementStep;
    if (Counter <= PassedThreshold) {
        FinalStatus = DEM_EVENT_STATUS_PASSED;
    }
}
```

#### Time-based Debouncing

- State machine: `DEM_TIME_DEBOUNCE_IDLE`, `DEM_TIME_DEBOUNCE_PREFAILED`, `DEM_TIME_DEBOUNCE_PREPASSED`.
- `Dem_MainFunction` increments elapsed time.
- When elapsed time exceeds configured threshold, final FAILED/PASSED status is applied.

#### Monitor-based Debouncing

- Direct mapping: `DEM_EVENT_STATUS_FAILED` -> fault counter at failed threshold; `DEM_EVENT_STATUS_PASSED` -> passed threshold.
- No intermediate debounce state.

---

## 6. DTC Management

### 6.1 DTC Status Byte

Bit mapping according to UDS specification (ISO 14229-1):
- Bit 0: testFailed
- Bit 1: testFailedThisMonitoringCycle
- Bit 2: pendingDTC
- Bit 3: confirmedDTC
- Bit 4: testNotCompletedSinceLastClear
- Bit 5: testFailedSinceLastClear
- Bit 6: testNotCompletedThisMonitoringCycle
- Bit 7: warningIndicatorRequested

### 6.2 DTC Aging

Aging process:
1. DTC must be PASSED for consecutive operation cycles
2. Aging counter increments each PASSED cycle
3. When threshold reached, DTC is aged out
4. Entry is removed from event memory

### 6.3 DTC Groups

| Group | Value | Description |
|-------|-------|-------------|
| DEM_DTC_GROUP_ALL | 0xFFFFFF | All DTCs |
| DEM_DTC_GROUP_EMISSION_RELATED | 0x000001 | Emission-related |
| DEM_DTC_GROUP_POWERTRAIN | 0x010000 | Powertrain |
| DEM_DTC_GROUP_CHASSIS | 0x020000 | Chassis |
| DEM_DTC_GROUP_BODY | 0x030000 | Body |
| DEM_DTC_GROUP_NETWORK_COM | 0x040000 | Network communication |

---

## 7. Event Queue Management

### 7.1 Priority Queue

- Three priority levels: HIGH, NORMAL, LOW
- FIFO within same priority
- Overflow handling (replace lower priority entries)
- Configurable queue size (`DEM_EVENT_QUEUE_SIZE`)

### 7.2 Queue Processing

Processed in `Dem_MainFunction()` with configurable max entries per cycle to prevent blocking.

---

## 8. NvM Integration

### 8.1 Write Strategy

- Immediate write for critical events
- Queued write for non-critical updates
- Retry mechanism for failed writes
- Ram block status update for lazy writes

### 8.2 Data Layout

- Primary memory: Normal DTC storage
- Mirror memory: Important DTCs (duplicated)
- Permanent memory: Emission-related DTCs (cannot be cleared)

---

## 9. API Design

### 9.1 Core API

| API | SID | Purpose | SWS 需求 |
|-----|-----|---------|---------|
| Dem_Init | 0x01 | Initialize DEM | SWS_Dem_00001 |
| Dem_DeInit / Dem_Shutdown | 0x02 | Deinitialize | SWS_Dem_00002 / SWS_Dem_00003 |
| Dem_GetVersionInfo | 0x03 | Version info | SWS_Dem_00029 |
| Dem_SetEventStatus | 0x04 | Report event status | SWS_Dem_00004 |
| Dem_ResetEventStatus | 0x05 | Reset event status | SWS_Dem_00005 |
| Dem_GetEventStatus | 0x08 | Read event status | SWS_Dem_00006 |
| Dem_ClearDTC | 0x0F | Clear DTC(s) | SWS_Dem_00014 |
| Dem_MainFunction | 0x1A | Cyclic processing | SWS_Dem_00032 |
| Dem_SetOperationCycleState | 0x1B | Set operation cycle | SWS_Dem_00025 |

### 9.2 DET Error Codes

| Error Code | Value | Trigger |
|------------|-------|---------|
| DEM_E_PARAM_CONFIG | 0x10 | Invalid configuration |
| DEM_E_PARAM_DATA | 0x11 | Invalid data |
| DEM_E_PARAM_POINTER | 0x12 | Null pointer |
| DEM_E_UNINIT | 0x20 | Module not initialized |
| DEM_E_PARAM_EVENT_ID | 0x13 | Invalid event ID |

---

## 10. Error Handling

### 10.1 Development Error Detection

Reports errors to Det module:
- API called before initialization
- Invalid parameter values
- Null pointer arguments

### 10.2 Runtime Error Handling

- Queue overflow handling
- NvM write failure recovery
- Memory full displacement

---

## 11. Performance Considerations

### 11.1 Timing

| Operation | Target |
|-----------|--------|
| Dem_SetEventStatus | < 50us |
| Dem_MainFunction | < 5ms (configurable) |
| NvM write | Depends on underlying storage |

### 11.2 Memory Usage

| Resource | Estimate |
|----------|----------|
| RAM | ~10KB for 100 events with full configuration |
| ROM | Depends on configuration tables |
| NVM | Configurable based on entry count |

---

## 12. Integration Guidelines

### 12.1 With Dcm

- Dcm reads DTCs via Dem API
- Dem reports status changes to Dcm
- Clear DTC request handling

### 12.2 With NvM

- NvM blocks configured for each memory type
- Write callbacks from NvM to Dem
- Read during Dem_Init

### 12.3 With BswM

- Dem reports mode requests on status changes
- BswM can query Dem for system state

---

## 13. Testing

### 13.1 Unit Tests

| Test File | Coverage |
|-----------|----------|
| Dem_test.c | Init, event status, debounce, DTC status, clear DTC |

### 13.2 Integration Tests

- Full diagnostic workflow
- NvM persistence verification
- Stress testing with high event rates

---

## 14. Implementation Notes / TODO

- v1.1.0 critical fix: separated type definitions to `Dem_Types.h`; fixed null pointer dereference (`ConfigPtr->Events` -> `EventParameters`); added complete time-based debounce support.
- Legacy code remains under `src/bsw/services/dem/legacy/` for reference but is not used by current build.
- Extended data record support added in v1.1.0.

---

## 15. References

- AUTOSAR SWS Diagnostic Event Manager (Document ID: 032)
- `docs/modules/DEM.md`
- `src/bsw/services/dem/include/Dem.h`
- `src/bsw/services/dem/include/Dem_Types.h`
- `src/bsw/services/dem/src/Dem.c`
- `src/bsw/services/dem/src/Dem_Int.c`
- ISO 14229-1 (UDS)
- ISO 15031-5 (OBD)

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Dem | — | DEM 模块级需求归属 |
| SWS_Dem_00007 | `Dem_GetEventFailed` | 测试 test_Dem_GetEventFailed 覆盖: Dem_GetEventFailed 场景 |
| SWS_Dem_00008 | `Dem_GetEventTested` | 测试 test_Dem_GetEventTested 覆盖: Dem_GetEventTested 场景 |
| SWS_Dem_00009 | `Dem_GetFaultDetectionCounter` | 测试 test_Dem_GetFaultDetectionCounter 覆盖: Dem_GetFaultDetectionCounter 场景 |
| SWS_Dem_00016 | `Dem_DTCSettingControl` | 测试 test_Dem_DTCSettingControl 覆盖: Dem_DTCSettingControl 场景 |
