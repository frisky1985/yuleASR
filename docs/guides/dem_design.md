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

## 2. Module Structure

### 2.1 File Organization

```
src/diagnostics/dem/
├── dem.h                 # Main API header
├── dem.c                 # Main implementation
├── dem_types.h           # Type definitions
├── dem_event.h/c         # Event management
├── dem_event.h/c         # DTC management
├── dem_nvm.h/c           # NvM interface
├── dem_freeze_frame.h/c  # Freeze frame handling
├── dem_queue.h/c         # Event queue management
├── dem_det.h             # Det integration
├── dem_Internals.h       # Internal definitions
└── config/
    ├── Dem_Cfg.h         # Compile-time configuration
    ├── Dem_Lcfg.c        # Link-time configuration
    └── Dem_PBcfg.c       # Post-build configuration
```

### 2.2 Key Data Structures

#### Event Status
```c
typedef uint8 Dem_EventStatusExtendedType;
#define DEM_UDS_STATUS_TF        0x01  // Test Failed
#define DEM_UDS_STATUS_TFTOC     0x02  // Test Failed This Operation Cycle
#define DEM_UDS_STATUS_PDTC      0x04  // Pending DTC
#define DEM_UDS_STATUS_CDTC      0x08  // Confirmed DTC
#define DEM_UDS_STATUS_TNCSLC    0x10  // Test Not Completed Since Last Clear
#define DEM_UDS_STATUS_TFSLC     0x20  // Test Failed Since Last Clear
#define DEM_UDS_STATUS_TNCTOC    0x40  // Test Not Completed This Operation Cycle
#define DEM_UDS_STATUS_WIR       0x80  // Warning Indicator Requested
```

#### Event Memory Entry
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

## 3. Configuration

### 3.1 Compile-time Configuration (Dem_Cfg.h)

Key configuration parameters:
- `DEM_CFG_MAX_NUMBER_EVENTS`: Maximum number of events (default: 100)
- `DEM_CFG_MAX_NUMBER_DTCS`: Maximum number of DTCs (default: 80)
- `DEM_CFG_EVENT_QUEUE_SIZE`: Event queue size (default: 20)
- `DEM_CFG_FreezeFrameSupport`: Enable freeze frame support
- `DEM_CFG_ExtendedDataSupport`: Enable extended data support
- `DEM_CFG_AgingSupport`: Enable aging support
- `DEM_CFG_OperationCycleSupport`: Enable operation cycle support

### 3.2 Link-time Configuration (Dem_Lcfg.c)

Static configuration tables:
- Event configuration table
- DTC configuration table
- Debounce algorithm configuration
- Operation cycle configuration
- Indicator configuration

### 3.3 Post-build Configuration (Dem_PBcfg.c)

Runtime configurable parameters:
- Callback function pointers
- Memory configuration
- DTC group configuration
- Aging configuration

## 4. Event Processing Flow

### 4.1 Normal Event Report

```
1. SW-C calls Dem_SetEventStatus(EventId, EventStatus)
2. Dem validates parameters
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

### 4.2 Debouncing

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
Uses timers to track how long a condition persists before confirming the status.

## 5. DTC Management

### 5.1 DTC Status Byte

Bit mapping according to UDS specification:
- Bit 0: testFailed
- Bit 1: testFailedThisMonitoringCycle
- Bit 2: pendingDTC
- Bit 3: confirmedDTC
- Bit 4: testNotCompletedSinceLastClear
- Bit 5: testFailedSinceLastClear
- Bit 6: testNotCompletedThisMonitoringCycle
- Bit 7: warningIndicatorRequested

### 5.2 DTC Aging

Aging process:
1. DTC must be PASSED for consecutive operation cycles
2. Aging counter increments each PASSED cycle
3. When threshold reached, DTC is aged out
4. Entry is removed from event memory

## 6. Event Queue Management

### 6.1 Priority Queue

Features:
- Three priority levels: HIGH, NORMAL, LOW
- FIFO within same priority
- Overflow handling (replace lower priority entries)
- Configurable queue size

### 6.2 Queue Processing

Processed in Dem_MainFunction() with configurable max entries per cycle to prevent blocking.

## 7. NvM Integration

### 7.1 Write Strategy

- Immediate write for critical events
- Queued write for non-critical updates
- Retry mechanism for failed writes
- Ram block status update for lazy writes

### 7.2 Data Layout

- Primary memory: Normal DTC storage
- Mirror memory: Important DTCs (duplicated)
- Permanent memory: Emission-related DTCs (cannot be cleared)

## 8. Error Handling

### 8.1 Development Error Detection

Reports errors to Det module:
- API called before initialization
- Invalid parameter values
- Null pointer arguments

### 8.2 Runtime Error Handling

- Queue overflow handling
- NvM write failure recovery
- Memory full displacement

## 9. Performance Considerations

### 9.1 Timing

- Dem_SetEventStatus: < 50us (typical)
- Dem_MainFunction: < 5ms (configurable)
- NvM write: Depends on underlying storage

### 9.2 Memory Usage

- RAM: ~10KB for 100 events with full configuration
- ROM: Depends on configuration tables
- NVM: Configurable based on entry count

## 10. Integration Guidelines

### 10.1 With Dcm

- Dcm reads DTCs via Dem API
- Dem reports status changes to Dcm
- Clear DTC request handling

### 10.2 With NvM

- NvM blocks configured for each memory type
- Write callbacks from NvM to Dem
- Read during Dem_Init

### 10.3 With BswM

- Dem reports mode requests on status changes
- BswM can query Dem for system state

## 11. Testing

### 11.1 Unit Tests

Located in `tests/dem/`:
- test_dem_event.c: Event management tests
- test_dem_dtc.c: DTC management tests
- test_dem_queue.c: Queue management tests

### 11.2 Integration Tests

- Full diagnostic workflow
- NvM persistence verification
- Stress testing with high event rates

## 12. References

- AUTOSAR SWS Diagnostic Event Manager (Document ID: 032)
- ISO 14229-1 (UDS)
- ISO 15031-5 (OBD)
