# DEM - Diagnostic Event Manager

## Overview

DEM implements AUTOSAR Diagnostic Event Manager for DTC (Diagnostic Trouble Code) management and fault memory handling. It manages diagnostic events, stores freeze frame data, and handles DTC status information.

## Standards

- AUTOSAR SWS Diagnostic Event Manager
- ISO 14229-1 UDS Diagnostic Services
- ISO 15031-5 OBD Requirements
- AUTOSAR Classic Platform 4.4.0

## Features

### DTC Management
- **DTC Storage** - Persistent storage of diagnostic trouble codes
- **Status Byte Management** - UDS-compliant status byte handling
- **DTC Groups** - Emission-related, powertrain, chassis, body, network
- **DTC Filtering** - Filter DTCs by status mask, severity, functional unit

### Event Handling
- **Event Status** - Tested/untested, failed/passed tracking
- **Debounce Algorithms** - Counter-based and time-based debouncing
- **Fault Detection Counter** - Track fault progression
- **Operation Cycles** - Ignition cycle, warm-up cycle management

### Freeze Frame & Extended Data
- **Freeze Frame Storage** - Capture environmental data on DTC set
- **Extended Data Records** - Additional diagnostic information
- **Pre-storage** - Pre-store freeze frame before confirmed
- **Multiple Records** - Support for multiple freeze frame records

### Aging & Healing
- **DTC Aging** - Automatic aging of resolved DTCs
- **Aging Counters** - Track aging progression
- **Healing Criteria** - Configurable healing conditions

## Architecture

```
┌─────────────────────────────────────────┐
│           Applications                  │
│  (BswM, ASW Components, DCM)            │
└─────────────┬───────────────────────────┘
              │
              v
┌─────────────────────────────────────────┐
│      DEM - Diagnostic Event Manager     │
│  ┌─────────────────────────────────┐    │
│  │  Event Processing               │    │
│  │  - Debounce handling            │    │
│  │  - Status transitions           │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │  DTC Management                 │    │
│  │  - Status byte handling         │    │
│  │  - DTC filtering                │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │  Freeze Frame Management        │    │
│  │  - Storage/retrieval            │    │
│  │  - Extended data records        │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │  Operation Cycle Management     │    │
│  │  - Cycle state handling         │    │
│  │  - Aging counters               │    │
│  └─────────────────────────────────┘    │
└─────────────┬───────────────────────────┘
              │
    ┌─────────┴─────────┐
    v                   v
┌─────────┐      ┌─────────────┐
│   NVM   │      │     DCM     │
│(Storage)│      │ (Diagnostic)│
└─────────┘      └─────────────┘
```

## DTC Status Byte

### UDS Status Byte Definition
| Bit | Name | Description |
|-----|------|-------------|
| 0 | testFailed | Set when test fails |
| 1 | testFailedThisOperationCycle | Failed in current cycle |
| 2 | pendingDTC | Failed this or last cycle |
| 3 | confirmedDTC | Confirmed fault (aged) |
| 4 | testNotCompletedSinceLastClear | Not tested since clear |
| 5 | testFailedSinceLastClear | Failed since clear |
| 6 | testNotCompletedThisOperationCycle | Not tested this cycle |
| 7 | warningIndicatorRequested | MIL/warning requested |

### Status Bit 0x50 (ConfirmedDTC | TestFailedSinceLastClear)
- Confirmed DTC that has failed since last clear
- Common status for stored emissions DTCs

## Event Status Types

| Status | Value | Description |
|--------|-------|-------------|
| DEM_EVENT_STATUS_PASSED | 0x00 | Test passed |
| DEM_EVENT_STATUS_FAILED | 0x01 | Test failed |
| DEM_EVENT_STATUS_PREPASSED | 0x02 | Pre-passed (debounce) |
| DEM_EVENT_STATUS_PREFAILED | 0x03 | Pre-failed (debounce) |

## Debounce Algorithms

### Counter-Based Debounce
```c
FailedThreshold    = +127   (confirm failure)
PassedThreshold    = -128   (confirm pass)
IncrementStep      = 1      (step on failed sample)
DecrementStep      = 1      (step on passed sample)
```

### Time-Based Debounce
```c
TimeFailedThreshold    = 100ms   (time to confirm failure)
TimePassedThreshold    = 100ms   (time to confirm pass)
```

## DTC Groups

| Group | Value | Description |
|-------|-------|-------------|
| DEM_DTC_GROUP_ALL | 0xFFFFFF | All DTCs |
| DEM_DTC_GROUP_EMISSION_RELATED | 0x000001 | Emission-related |
| DEM_DTC_GROUP_POWERTRAIN | 0x010000 | Powertrain |
| DEM_DTC_GROUP_CHASSIS | 0x020000 | Chassis |
| DEM_DTC_GROUP_BODY | 0x030000 | Body |
| DEM_DTC_GROUP_NETWORK_COM | 0x040000 | Network communication |

## APIs

### Core APIs
| API | Function |
|-----|----------|
| `Dem_Init()` | Initialize DEM |
| `Dem_DeInit()` | Deinitialize DEM |
| `Dem_Shutdown()` | Shutdown DEM |
| `Dem_GetVersionInfo()` | Get version info |
| `Dem_MainFunction()` | Periodic processing |

### Event APIs
| API | Function |
|-----|----------|
| `Dem_SetEventStatus()` | Set event status (passed/failed) |
| `Dem_ResetEventStatus()` | Reset event status |
| `Dem_GetEventStatus()` | Get current event status |
| `Dem_GetEventFailed()` | Get failed status |
| `Dem_GetEventTested()` | Get tested status |
| `Dem_GetFaultDetectionCounter()` | Get fault detection counter |

### DTC APIs
| API | Function |
|-----|----------|
| `Dem_GetStatusOfDTC()` | Get DTC status byte |
| `Dem_GetDTCStatusAvailabilityMask()` | Get available status bits |
| `Dem_GetNumberOfFilteredDTC()` | Get count of filtered DTCs |
| `Dem_GetNextFilteredDTC()` | Get next filtered DTC |
| `Dem_ClearDTC()` | Clear DTC(s) |
| `Dem_SelectDTC()` | Select DTC for operations |
| `Dem_DisableDTCSetting()` | Disable DTC setting |
| `Dem_EnableDTCSetting()` | Enable DTC setting |

### Freeze Frame APIs
| API | Function |
|-----|----------|
| `Dem_PrestoreFreezeFrame()` | Pre-store freeze frame |
| `Dem_ClearPrestoredFreezeFrame()` | Clear pre-stored data |
| `Dem_GetFreezeFrameDataByDTC()` | Get freeze frame data |
| `Dem_GetExtendedDataRecordByDTC()` | Get extended data |
| `Dem_GetSizeOfExtendedDataRecordByDTC()` | Get extended data size |

### Operation Cycle APIs
| API | Function |
|-----|----------|
| `Dem_SetOperationCycleState()` | Set operation cycle state |
| `Dem_GetOperationCycleState()` | Get operation cycle state |
| `Dem_RestartOperationCycle()` | Restart operation cycle |
| `Dem_GetCycleCounter()` | Get cycle counter |

### Indicator APIs
| API | Function |
|-----|----------|
| `Dem_GetIndicatorStatus()` | Get warning indicator status |
| `Dem_SetIndicatorStatus()` | Set indicator status (internal) |

## Configuration

### Pre-compile Configuration
| Parameter | Description |
|-----------|-------------|
| `DEM_VERSION_INFO_API` | Enable version info API |
| `DEM_DEV_ERROR_DETECT` | Enable development error detection |
| `DEM_EVENT_STATUS_TRIGGERED` | Event status change notification |

### Block Configuration Example
```c
const Dem_EventParameterType DemEventParameters[] = {
    {
        .EventId = 0x0001,
        .DTC = 0x123456,
        .EventPriority = 1,
        .DebounceAlgorithm = DEM_DEBOUNCE_COUNTER_BASED,
        .DebounceParams.Counter = {
            .FailedThreshold = 127,
            .PassedThreshold = -128,
            .IncrementStep = 1,
            .DecrementStep = 1
        }
    }
};
```

## Usage Examples

### Setting Event Status
```c
#include "Dem.h"

void MonitorBatteryVoltage(void)
{
    uint16 voltage = ReadBatteryVoltage();
    
    if (voltage < BATTERY_MIN || voltage > BATTERY_MAX) {
        /* Report failed - debounce applied internally */
        Dem_SetEventStatus(DEM_EVENT_BATTERY_VOLTAGE, DEM_EVENT_STATUS_FAILED);
    } else {
        /* Report passed */
        Dem_SetEventStatus(DEM_EVENT_BATTERY_VOLTAGE, DEM_EVENT_STATUS_PASSED);
    }
}
```

### Reading DTC Status
```c
void CheckDtcStatus(void)
{
    Dem_UdsStatusByteType status;
    Std_ReturnType result;
    
    result = Dem_GetStatusOfDTC(0x123456, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status);
    
    if (result == E_OK) {
        if (status & DEM_UDS_STATUS_TF) {
            /* Test failed - fault currently present */
        }
        if (status & DEM_UDS_STATUS_CDTC) {
            /* Confirmed DTC - stored in memory */
        }
    }
}
```

### Clearing DTCs
```c
void ClearAllDtcs(void)
{
    Std_ReturnType result;
    
    /* Clear all DTCs in primary memory */
    result = Dem_ClearDTC(DEM_DTC_GROUP_ALL, 
                          DEM_DTC_FORMAT_UDS,
                          DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    
    if (result == E_OK) {
        /* Clear operation started - check status later */
    }
}
```

### Operation Cycle Management
```c
void OnIgnitionOn(void)
{
    /* Start ignition operation cycle */
    Dem_SetOperationCycleState(DEM_OPCYC_IGNITION, 
                               DEM_CYCLE_STATE_START);
}

void OnIgnitionOff(void)
{
    /* End ignition operation cycle */
    Dem_SetOperationCycleState(DEM_OPCYC_IGNITION,
                               DEM_CYCLE_STATE_END);
}
```

## Dependencies

- **NVM** - Non-volatile storage for DTCs
- **DCM** - Diagnostic communication for DTC read/clear
- **DET** - Development error tracing
- **RTE** - Runtime environment for callbacks

## Source Code

- `/home/admin/yuleASR/src/bsw/services/dem/`
  - `include/Dem.h` - Public API
  - `include/Dem_Types.h` - Type definitions
  - `include/Dem_Cfg.h` - Configuration
  - `src/Dem.c` - Core implementation

## Tests

- `/home/admin/yuleASR/tests/unit/autosar/services/Dem/`

## References

- AUTOSAR_SWS_DiagnosticEventManager
- ISO 14229-1 UDS
- ISO 15031-5 OBD
