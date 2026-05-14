# COM - Communication Services

## Overview

COM implements AUTOSAR Communication Services for signal-based communication between ECU components. It provides services for sending and receiving signals and signal groups with support for various transmission modes and filters.

## Standards

- AUTOSAR SWS Communication Services
- AUTOSAR Classic Platform 4.4.0
- MISRA C:2012

## Features

### Signal Management
- **Signal Transmission** - Send individual signals
- **Signal Reception** - Receive and decode signals
- **Signal Groups** - Atomic access to grouped signals
- **Shadow Buffers** - Consistent signal group updates
- **Signal Invalidation** - Mark signals as invalid

### Transmission Modes
- **Direct** - Immediate transmission on send
- **Periodic** - Cyclic transmission
- **Mixed** - Periodic with event-triggered updates
- **None** - No automatic transmission

### Transfer Properties
- **Triggered** - Always trigger transmission
- **Triggered On Change** - Only trigger on value change
- **Triggered On Change Without Repetition** - Single trigger on change
- **Pending** - Queue for transmission

### Signal Filtering
- **Always** - Always pass through
- **Never** - Never pass through
- **Masked New Equals X** - Masked value equals reference
- **Masked New Differs X** - Masked value differs from reference
- **Masked New Differs Masked Old** - Detect changes
- **New Is Within** - Value in range
- **New Is Outside** - Value outside range
- **One Every N** - Periodic pass through

### I-PDU Management
- **I-PDU Groups** - Organize I-PDUs into groups
- **Group Control** - Start/stop I-PDU groups
- **Deadline Monitoring** - Timeout supervision
- **Transmission Mode Switching** - Dynamic mode change

## Architecture

```
┌────────────────────────────────────────┐
│        Application Software (ASW)        │
│        (RTE Interface)                   │
└─────────────┬──────────────────────────┘
              │
              v
┌────────────────────────────────────────┐
│       COM - Communication Services      │
│  ┌──────────────────────────────────┐       │
│  │  Signal Interface              │       │
│  │  - SendSignal()                │       │
│  │  - ReceiveSignal()             │       │
│  │  - SendSignalGroup()           │       │
│  │  - ReceiveSignalGroup()        │       │
│  └──────────────────────────────────┘       │
│  ┌──────────────────────────────────┐       │
│  │  I-PDU Management              │       │
│  │  - Pack/Unpack signals         │       │
│  │  - Transmission modes          │       │
│  │  - Signal routing              │       │
│  └──────────────────────────────────┘       │
│  ┌──────────────────────────────────┐       │
│  │  Main Functions                │       │
│  │  - MainFunctionRx()            │       │
│  │  - MainFunctionTx()            │       │
│  │  - MainFunctionRouteSignals()  │       │
│  └──────────────────────────────────┘       │
└─────────────┬──────────────────────────┘
              │
              v
┌────────────────────────────────────────┐
│            PDU Router (PduR)                 │
└────────────────────────────────────────┘
```

## Transmission Modes

| Mode | Description |
|------|-------------|
| `COM_DIRECT` | Transmit immediately on send |
| `COM_PERIODIC` | Transmit at fixed intervals |
| `COM_MIXED` | Periodic with immediate update trigger |
| `COM_NONE` | No automatic transmission |

## Transfer Properties

| Property | Description |
|----------|-------------|
| `COM_TRIGGERED` | Always trigger transmission |
| `COM_TRIGGERED_ON_CHANGE` | Trigger only on value change |
| `COM_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION` | Single trigger on change |
| `COM_PENDING` | Queue for next transmission |

## Filter Algorithms

| Algorithm | Description |
|-----------|-------------|
| `COM_ALWAYS` | Always accept signal |
| `COM_NEVER` | Never accept signal |
| `COM_MASKED_NEW_EQUALS_X` | (New & Mask) == X |
| `COM_MASKED_NEW_DIFFERS_X` | (New & Mask) != X |
| `COM_MASKED_NEW_DIFFERS_MASKED_OLD` | (New & Mask) != (Old & Mask) |
| `COM_NEW_IS_WITHIN` | Min <= New <= Max |
| `COM_NEW_IS_OUTSIDE` | New < Min or New > Max |
| `COM_ONE_EVERY_N` | Accept every Nth sample |

## Signal Status

| Status | Value | Description |
|--------|-------|-------------|
| `COM_SIG_VALID` | 0x10 | Signal valid |
| `COM_SIG_INVALID` | 0x20 | Signal invalid |
| `COM_SIG_ERROR` | 0x02 | Signal error |
| `COM_SIG_TIMEOUT` | 0x08 | Signal timeout |

## APIs

### Core APIs
| API | Function |
|-----|----------|
| `Com_Init()` | Initialize COM module |
| `Com_DeInit()` | Deinitialize COM module |
| `Com_GetVersionInfo()` | Get version information |
| `Com_GetStatus()` | Get module status |
| `Com_GetConfigurationId()` | Get configuration ID |

### Signal APIs
| API | Function |
|-----|----------|
| `Com_SendSignal()` | Send a signal |
| `Com_ReceiveSignal()` | Receive a signal |
| `Com_InvalidateSignal()` | Invalidate a signal |
| `Com_SendSignalGroup()` | Send a signal group |
| `Com_ReceiveSignalGroup()` | Receive a signal group |
| `Com_InvalidateSignalGroup()` | Invalidate a signal group |

### I-PDU Control APIs
| API | Function |
|-----|----------|
| `Com_IpduGroupControl()` | Start/stop I-PDU groups |
| `Com_ReceptionDMControl()` | Enable/disable deadline monitoring |
| `Com_EnableReceptionDM()` | Enable deadline monitoring |
| `Com_DisableReceptionDM()` | Disable deadline monitoring |
| `Com_TriggerIPDUSend()` | Trigger I-PDU transmission |
| `Com_SwitchIpduTxMode()` | Switch transmission mode |

### Main Functions
| API | Function |
|-----|----------|
| `Com_MainFunctionRx()` | Process reception |
| `Com_MainFunctionTx()` | Process transmission |
| `Com_MainFunctionRouteSignals()` | Route gateway signals |

### Callbacks (from PduR)
| API | Function |
|-----|----------|
| `Com_RxIndication()` | Reception indication |
| `Com_TxConfirmation()` | Transmission confirmation |
| `Com_TriggerTransmit()` | Trigger transmit callback |

## Configuration

### Pre-compile Configuration
| Parameter | Description |
|-----------|-------------|
| `COM_VERSION_INFO_API` | Enable version info API |
| `COM_DEV_ERROR_DETECT` | Enable development error detection |

### Signal Configuration Example
```c
const Com_SignalConfigType ComSignals[] = {
    {
        .SignalId = 0x0001,
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .FilterAlgorithm = COM_ALWAYS,
        .FilterMask = 0xFFFF,
        .FilterX = 0,
        .SignalGroupRef = COM_NO_SIGNALGROUP
    }
};
```

### I-PDU Configuration Example
```c
const Com_IPduConfigType ComIPdus[] = {
    {
        .PduId = 0x100,
        .DataLength = 8,
        .RepeatingEnabled = TRUE,
        .NumRepetitions = 3,
        .TimeBetweenRepetitions = 20,  /* ms */
        .TimePeriod = 100              /* ms */
    }
};
```

## Usage Examples

### Sending a Signal
```c
#include "Com.h"

void SendEngineSpeed(void)
{
    uint16 engineSpeed = GetEngineSpeed();
    uint8 result;
    
    result = Com_SendSignal(COM_SIGNAL_ENGINE_SPEED, &engineSpeed);
    
    if (result == E_OK) {
        /* Signal queued successfully */
    }
}
```

### Receiving a Signal
```c
void ReceiveVehicleSpeed(void)
{
    uint16 vehicleSpeed;
    uint8 result;
    
    result = Com_ReceiveSignal(COM_SIGNAL_VEHICLE_SPEED, &vehicleSpeed);
    
    if (result == E_OK) {
        /* Process received speed */
        ProcessVehicleSpeed(vehicleSpeed);
    }
}
```

### Signal Group Usage
```c
typedef struct {
    uint16 engineSpeed;
    uint8 engineTemp;
    uint8 throttlePos;
} EngineDataType;

void SendEngineData(void)
{
    EngineDataType engineData;
    
    /* Update shadow buffer */
    engineData.engineSpeed = GetEngineSpeed();
    engineData.engineTemp = GetEngineTemp();
    engineData.throttlePos = GetThrottlePos();
    
    /* Copy to shadow buffer and trigger send */
    Com_SendSignalGroup(COM_SIGNALGROUP_ENGINE);
}
```

### I-PDU Group Control
```c
void EnableCommunication(void)
{
    Com_IpduGroupVector ipduGroupVector;
    
    /* Enable all I-PDU groups */
    memset(ipduGroupVector, 0xFF, sizeof(ipduGroupVector));
    
    Com_IpduGroupControl(ipduGroupVector, TRUE);
}
```

### Deadline Monitoring
```c
void SetupDeadlineMonitoring(void)
{
    Com_IpduGroupVector ipduGroupVector;
    
    memset(ipduGroupVector, 0, sizeof(ipduGroupVector));
    /* Set bit for specific I-PDU group */
    ipduGroupVector[0] = 0x01;
    
    Com_EnableReceptionDM(ipduGroupVector);
}
```

## Dependencies

- **PduR** - PDU Router for I-PDU transmission/reception
- **DET** - Development error tracing
- **RTE** - Runtime environment interface

## Source Code

- `/home/admin/yuleASR/src/bsw/services/com/`
  - `include/Com.h` - Public API
  - `include/Com_Cfg.h` - Configuration
  - `include/ComStack_Types.h` - Common types
  - `src/Com.c` - Core implementation
  - `src/Com_Lcfg.c` - Link-time configuration

## Tests

- `/home/admin/yuleASR/tests/unit/autosar/services/Com/`

## References

- AUTOSAR_SWS_ComServices
- AUTOSAR Classic Platform 4.4.0
