# DEM (Diagnostic Event Manager) Module Specification

> **Module:** DEM (Diagnostic Event Manager)
> **Layer:** Service Layer
> **Standard:** AutoSAR Classic Platform 4.x
> **Platform:** NXP i.MX8M Mini
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

The Diagnostic Event Manager (DEM) is a Service Layer module responsible for managing diagnostic events, Diagnostic Trouble Codes (DTCs), freeze frames, extended data records, and fault memory operations. It provides the diagnostic fault memory for the ECU.

### Key Responsibilities

- **DTC Management:** Store, retrieve, and manage Diagnostic Trouble Codes
- **Diagnostic Event Processing:** Handle event status reports from SW-Cs and BSW modules
- **Freeze Frame Storage:** Capture and store snapshot data when faults are confirmed
- **Extended Data Records:** Store additional diagnostic data (occurrence counters, aging counters, etc.)
- **Debounce Algorithms:** Filter transient fault conditions using counter-based, time-based, or monitor-internal debounce
- **Aging and Displacement:** Age out old faults and manage fault memory displacement
- **Operation Cycle Management:** Track operation cycles (power, ignition, OBD drive cycle, etc.)

---

## 2. API List

### 2.1 Core Lifecycle APIs

| API | Description |
|-----|-------------|
| `Dem_Init(const Dem_ConfigType* ConfigPtr)` | Initializes the DEM module |
| `Dem_DeInit(void)` | Deinitializes the DEM module |
| `Dem_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Retrieves version information |
| `Dem_MainFunction(void)` | Periodic main function for debounce and aging processing |

### 2.2 Event Management APIs

| API | Description |
|-----|-------------|
| `Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)` | Report event status (Passed/Failed/PrePassed/PreFailed) |
| `Dem_GetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType* EventStatus)` | Get the last reported event status |
| `Dem_ResetEventStatus(Dem_EventIdType EventId)` | Reset event status and debounce counter |
| `Dem_GetEventFailed(Dem_EventIdType EventId, boolean* EventFailed)` | Get whether the event has failed |
| `Dem_GetEventTested(Dem_EventIdType EventId, boolean* EventTested)` | Get whether the event was tested this cycle |
| `Dem_GetFaultDetectionCounter(Dem_EventIdType EventId, sint8* FaultDetectionCounter)` | Get the fault detection counter |

### 2.3 DTC Management APIs

| API | Description |
|-----|-------------|
| `Dem_GetStatusOfDTC(Dem_DTCType DTC, Dem_DTCOriginType DTCOrigin, Dem_UdsStatusByteType* DTCStatus)` | Get UDS status byte of a DTC |
| `Dem_GetDTCStatusAvailabilityMask(uint8* DTCStatusMask)` | Get the supported status mask |
| `Dem_ClearDTC(Dem_DTCType DTC, Dem_DTCFormatType DTCFormat, Dem_DTCOriginType DTCOrigin)` | Clear DTC(s) from fault memory |
| `Dem_SelectDTC(Dem_DTCType DTC, Dem_DTCFormatType DTCFormat, Dem_DTCOriginType DTCOrigin)` | Select a DTC for subsequent operations |
| `Dem_GetNumberOfFilteredDTC(uint16* NumberOfFilteredDTC)` | Get number of DTCs matching filter criteria |
| `Dem_GetNextFilteredDTC(Dem_DTCType* DTC, Dem_UdsStatusByteType* DTCStatus)` | Get next filtered DTC |

### 2.4 Freeze Frame and Extended Data APIs

| API | Description |
|-----|-------------|
| `Dem_PrestoreFreezeFrame(Dem_EventIdType EventId)` | Pre-store freeze frame data |
| `Dem_ClearPrestoredFreezeFrame(Dem_EventIdType EventId)` | Clear pre-stored freeze frame data |
| `Dem_GetFreezeFrameDataByDTC(Dem_DTCType DTC, Dem_DTCOriginType DTCOrigin, uint8 RecordNumber, uint8* DestBuffer, uint16* BufferSize)` | Get freeze frame data by DTC |
| `Dem_DisableDTCRecordUpdate(void)` | Disable DTC record updates |
| `Dem_EnableDTCRecordUpdate(void)` | Enable DTC record updates |

### 2.5 Operation Cycle APIs

| API | Description |
|-----|-------------|
| `Dem_SetOperationCycleState(uint8 OperationCycleId, uint8 CycleState)` | Set operation cycle state (start/end) |
| `Dem_GetOperationCycleState(uint8 OperationCycleId, uint8* CycleState)` | Get operation cycle state |
| `Dem_RestartOperationCycle(Dem_OperationCycleType OperationCycleType)` | Restart an operation cycle |

### 2.6 DTC Setting Control APIs

| API | Description |
|-----|-------------|
| `Dem_DisableDTCSetting(Dem_DTCType DTCGroup, uint8 DTCKind)` | Disable DTC setting for a group |
| `Dem_EnableDTCSetting(Dem_DTCType DTCGroup, uint8 DTCKind)` | Enable DTC setting for a group |

---

## 3. Data Types

### 3.1 Dem_EventIdType
```c
typedef uint16 Dem_EventIdType;
```

### 3.2 Dem_DTCType
```c
typedef uint32 Dem_DtcType;
```

### 3.3 Dem_DTCStatusType
```c
typedef uint8 Dem_DTCStatusType;
```

### 3.4 Dem_EventStatusType
```c
typedef enum {
    DEM_EVENT_STATUS_PASSED = 0,
    DEM_EVENT_STATUS_FAILED,
    DEM_EVENT_STATUS_PREPASSED,
    DEM_EVENT_STATUS_PREFAILED,
    DEM_EVENT_STATUS_FDC_THRESHOLD_REACHED
} Dem_EventStatusType;
```

### 3.5 Dem_UdsStatusByteType
```c
typedef uint8 Dem_UdsStatusByteType;
```

### 3.6 Dem_DTCOriginType
```c
typedef enum {
    DEM_DTC_ORIGIN_PRIMARY_MEMORY = 0x01,
    DEM_DTC_ORIGIN_MIRROR_MEMORY = 0x02,
    DEM_DTC_ORIGIN_PERMANENT_MEMORY = 0x04,
    DEM_DTC_ORIGIN_OBD_RELEVANT_MEMORY = 0x08
} Dem_DTCOriginType;
```

### 3.7 Dem_FaultDetectionCounterType
```c
typedef sint8 Dem_FaultDetectionCounterType;
```

### 3.8 Dem_EventStateType
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
} Dem_EventStateType;
```

### 3.9 Dem_EventParameterType
```c
typedef struct {
    Dem_EventIdType EventId;
    Dem_DtcType Dtc;
    uint8 EventPriority;
    boolean EventAvailable;
    boolean EventReporting;
    uint8 EventFailureCycleCounterThreshold;
    uint8 EventConfirmationThreshold;
    boolean EventDebounceAlgorithm;
    boolean EventCounterBased;
    boolean EventTimeBased;
    boolean EventMonitorInternal;
} Dem_EventParameterType;
```

### 3.10 Dem_ConfigType
```c
typedef struct {
    const Dem_EventParameterType* EventParameters;
    uint16 NumEvents;
    const Dem_DtcParameterType* DtcParameters;
    uint16 NumDtcs;
    const Dem_FreezeFrameRecordType* FreezeFrameRecords;
    uint8 NumFreezeFrameRecords;
    const Dem_ExtendedDataRecordType* ExtendedDataRecords;
    uint8 NumExtendedDataRecords;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean ClearDtcSupported;
    boolean ClearDtcLimitation;
    boolean DtcStatusAvailabilityMask;
    boolean OBDRelevantSupport;
    boolean J1939Support;
} Dem_ConfigType;
```

---

## 4. Debounce Algorithms

### 4.1 Counter-Based Debounce

The counter-based debounce algorithm uses a signed counter that increments on PreFailed reports and decrements on PrePassed reports.

**Rules:**
- **FAILED threshold:** When counter >= `DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD` (default 127), event is considered failed
- **PASSED threshold:** When counter <= `DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD` (default -128), event is considered passed
- **Increment step:** `DEM_DEBOUNCE_COUNTER_INCREMENT` (default 1) per PreFailed report
- **Decrement step:** `DEM_DEBOUNCE_COUNTER_DECREMENT` (default 1) per PrePassed report

**Behavior:**
| Status Report | Counter Change | Result if Threshold Met |
|---------------|----------------|------------------------|
| PASSED | Set to PASSED threshold | Event passed immediately |
| FAILED | Set to FAILED threshold | Event failed immediately |
| PREPASSED | Decrement by step | May transition to passed |
| PREFAILED | Increment by step | May transition to failed |

### 4.2 Time-Based Debounce

The time-based debounce algorithm tracks the time since the last status change.

**Rules:**
- **FAILED timeout:** After `DEM_DEBOUNCE_TIME_FAILED_THRESHOLD_MS` of continuous PreFailed, event is considered failed
- **PASSED timeout:** After `DEM_DEBOUNCE_TIME_PASSED_THRESHOLD_MS` of continuous PrePassed, event is considered passed
- Requires periodic processing in `Dem_MainFunction`

### 4.3 Monitor-Internal Debounce

The monitor-internal debounce algorithm delegates debouncing to the reporting SW-C or BSW module. The module directly reports PASSED or FAILED, and DEM does not perform additional debouncing.

---

## 5. DTC Status Byte

The UDS DTC status byte follows ISO 14229-1:

| Bit | Name | Description |
|-----|------|-------------|
| 0 | Test Failed (TF) | Most recent test result was failed |
| 1 | Test Failed This Operation Cycle (TFTOC) | Test failed during current operation cycle |
| 2 | Pending DTC (PDTC) | Test failed during current or previous cycle |
| 3 | Confirmed DTC (CDTC) | Fault is confirmed (sufficient occurrences) |
| 4 | Test Not Completed Since Last Clear (TNCSLC) | Test not run since last clear |
| 5 | Test Failed Since Last Clear (TFSLC) | Test failed at least once since last clear |
| 6 | Test Not Completed This Operation Cycle (TNCTOC) | Test not run this operation cycle |
| 7 | Warning Indicator Requested (WIR) | Warning lamp should be illuminated |

---

## 6. Error Handling (DET)

When `DEM_DEV_ERROR_DETECT == STD_ON`, the following development errors are reported via `Det_ReportError`:

| Error Code | Value | Description |
|------------|-------|-------------|
| `DEM_E_UNINIT` | 0x20U | Module not initialized |
| `DEM_E_PARAM_POINTER` | 0x12U | Null pointer passed |
| `DEM_E_PARAM_DATA` | 0x11U | Invalid data (e.g., invalid EventId) |
| `DEM_E_PARAM_CONFIG` | 0x10U | Invalid configuration |
| `DEM_E_NODATAAVAILABLE` | 0x30U | Requested data not available |
| `DEM_E_WRONG_CONDITION` | 0x40U | Wrong condition for operation |
| `DEM_E_WRONG_CONFIGURATION` | 0x50U | Wrong configuration |

### DET Usage in APIs
- **Dem_Init**: Reports `DEM_E_PARAM_POINTER` if `ConfigPtr == NULL`
- **Dem_DeInit**: Reports `DEM_E_UNINIT` if module not initialized
- **Dem_SetEventStatus**: Reports `DEM_E_UNINIT` if not initialized, `DEM_E_PARAM_DATA` if invalid EventId
- **Dem_GetEventStatus**: Reports `DEM_E_UNINIT` if not initialized, `DEM_E_PARAM_POINTER` if null pointer, `DEM_E_PARAM_DATA` if invalid EventId

---

## 7. Scenarios

### Scenario 1: Event Reporting and Debounce

**Description:** A SW-C reports a sensor fault event with PreFailed status multiple times until the debounce counter reaches the failed threshold.

**Flow:**
1. SW-C calls `Dem_SetEventStatus(DEM_EVENT_SENSOR_FAULT, DEM_EVENT_STATUS_PREFAILED)`
2. DEM increments debounce counter by 1
3. Counter is below FAILED threshold, no DTC status change yet
4. Steps 1-2 repeat 127 times
5. Counter reaches FAILED threshold (127)
6. DEM updates DTC status: TestFailed, TestFailedThisOperationCycle, PendingDTC set
7. Occurrence counter increments to 1
8. On next PreFailed report, occurrence counter reaches 2
9. ConfirmedDTC is set, freeze frame is stored

**Expected Result:** DTC transitions to confirmed status after sufficient PreFailed reports.

### Scenario 2: DTC Status Update

**Description:** A confirmed DTC is cleared by a diagnostic tester.

**Flow:**
1. DTC 0x123456 has status `0x2F` (TestFailed + PendingDTC + ConfirmedDTC + TestFailedSinceLastClear + TestNotCompletedSinceLastClear)
2. Tester sends UDS service 0x14 (ClearDiagnosticInformation)
3. DCM calls `Dem_ClearDTC(DEM_DTC_GROUP_ALL, ...)`
4. DEM clears all DTC entries:
   - Status reset to `0x50` (TestNotCompletedSinceLastClear + TestNotCompletedThisOperationCycle)
   - Occurrence counter reset to 0
   - Aging counter reset to 0
   - Freeze frame invalidated
5. DEM returns E_OK

**Expected Result:** DTC status is reset to initial state.

### Scenario 3: Freeze Frame Storage

**Description:** When a DTC becomes confirmed for the first time, the DEM stores a freeze frame containing snapshot data.

**Flow:**
1. Event reports FAILED, debounce counter reaches threshold
2. DTC status updated, occurrence counter = 1
3. Event reports FAILED again, occurrence counter = 2
4. ConfirmedDTC bit transitions from 0 to 1
5. `Dem_StoreFreezeFrame()` is called for the associated DTC
6. DEM allocates a freeze frame record
7. DEM captures configured DID values into freeze frame data buffer
8. Freeze frame is marked as valid with timestamp

**Expected Result:** Freeze frame is stored and can be retrieved via `Dem_GetFreezeFrameDataByDTC`.

### Scenario 4: Fault Aging

**Description:** A confirmed DTC that has not failed for a long time is aged out of the fault memory.

**Flow:**
1. DTC 0x123456 is confirmed (ConfirmedDTC = 1, TestFailed = 0)
2. Operation cycle ends, `Dem_ProcessAging()` is triggered
3. Aging counter increments (starts from 0)
4. Multiple operation cycles pass without the fault reoccurring
5. Aging counter reaches `DEM_AGING_THRESHOLD` (40 cycles)
6. DEM clears ConfirmedDTC and PendingDTC bits
7. DEM sets `IsAged = TRUE`
8. Freeze frame data may be retained or cleared based on configuration

**Expected Result:** Aged DTC is no longer reported as confirmed; aging counter stops incrementing.

---

## 8. Dependencies

### Upper Layer Modules
- **DCM:** Diagnostic Communication Manager requests DTC information
- **Application / SW-Cs:** Report diagnostic events

### Lower Layer Modules
- **NvM:** Non-Volatile Memory for persistent fault memory storage (future integration)

### Peer Service Layer Modules
- **Dcm:** Requests DTC read/clear operations

### Common Modules
- **Det:** Development Error Tracer
- **MemMap:** Memory mapping for section placement

---

## 9. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-04-24 | YuleTech | Initial DEM specification with DTC management, debounce, and scenarios |
