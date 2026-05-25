# NM - Network Management Interface

## Overview

NM implements AUTOSAR Generic Network Management Interface, providing a standardized API for network management across different bus systems (CAN, LIN, FlexRay, Ethernet). It abstracts bus-specific network management implementations.

## Standards

- AUTOSAR SWS Network Management Interface
- AUTOSAR Classic Platform 4.4.0
- MISRA C:2012

## Features

### Network Management States
- **Bus Sleep** - Bus is in sleep mode, minimal power consumption
- **Prepare Bus Sleep** - Transition to sleep, waiting for all nodes ready
- **Synchronize** - Synchronization state for coordinated shutdown
- **Network** - Active network communication

### State Machine
- **Repeat Message** - Rapid NM message transmission at startup
- **Normal Operation** - Standard NM operation
- **Ready Sleep** - Ready for sleep, checking bus activity
- **Bus Sleep** - Bus in sleep mode

### Communication Control
- **Network Request** - Request network start/keep active
- **Network Release** - Release network, allow sleep transition
- **Passive Startup** - Participate without requesting network
- **Disable/Enable Communication** - Control PDU transmission

### Remote Sleep Detection
- **Remote Sleep Indication** - Detect all remote nodes ready for sleep
- **Remote Sleep Cancellation** - Detect remote nodes active again

## Architecture

```
┌────────────────────────────────────────┐
│        Application Software (ASW)        │
│        (BswM, ComM, EcuM)                │
└─────────────┬──────────────────────────┘
              │
              v
┌────────────────────────────────────────┐
│      NM - Network Management Interface   │
│  ┌──────────────────────────────────┐       │
│  │  State Management              │       │
│  │  - State machine handling      │       │
│  │  - Mode transitions            │       │
│  │  - State notifications         │       │
│  └──────────────────────────────────┘       │
│  ┌──────────────────────────────────┐       │
│  │  Network Control               │       │
│  │  - NetworkRequest()            │       │
│  │  - NetworkRelease()            │       │
│  │  - PassiveStartUp()            │       │
│  └──────────────────────────────────┘       │
│  ┌──────────────────────────────────┐       │
│  │  Communication Control         │       │
│  │  - DisableCommunication()      │       │
│  │  - EnableCommunication()       │       │
│  └──────────────────────────────────┘       │
└─────────────┬─────────────┬─────────────┘
              │                   │
    ┌─────────┴─────────┐     v
    v                   v         ┌─────────┐
┌─────────┐      ┌─────────────┐       │  UdpNm  │
│  CanNm   │      │     LinNm      │       └─────────┘
│(CAN NM) │      │ (LIN NM)      │
└─────────┘      └─────────────┘
```

## NM States

| State | Value | Description |
|-------|-------|-------------|
| `NM_STATE_UNINIT` | 0x00 | Module not initialized |
| `NM_STATE_BUS_SLEEP` | 0x01 | Bus in sleep mode |
| `NM_STATE_PREPARE_BUS_SLEEP` | 0x02 | Preparing for sleep |
| `NM_STATE_READY_SLEEP` | 0x03 | Ready for sleep |
| `NM_STATE_NORMAL_OPERATION` | 0x04 | Normal operation |
| `NM_STATE_REPEAT_MESSAGE` | 0x05 | Repeat message state |
| `NM_STATE_SYNCHRONIZE` | 0x06 | Synchronizing |

## NM Modes

| Mode | Value | Description |
|------|-------|-------------|
| `NM_MODE_BUS_SLEEP` | 0x00 | Bus sleep mode |
| `NM_MODE_PREPARE_BUS_SLEEP` | 0x01 | Prepare bus sleep mode |
| `NM_MODE_SYNCHRONIZE` | 0x02 | Synchronize mode |
| `NM_MODE_NETWORK` | 0x03 | Network mode |

## Bus NM Types

| Type | Value | Description |
|------|-------|-------------|
| `NM_BUSNM_CANNM` | 0x00 | CAN Network Management |
| `NM_BUSNM_FRNM` | 0x01 | FlexRay Network Management |
| `NM_BUSNM_UDPNM` | 0x02 | UDP Network Management |
| `NM_BUSNM_LINNM` | 0x03 | LIN Network Management |

## APIs

### Core APIs
| API | Function |
|-----|----------|
| `Nm_Init()` | Initialize NM module |
| `Nm_DeInit()` | Deinitialize NM module |
| `Nm_GetVersionInfo()` | Get version information |

### Network Control APIs
| API | Function |
|-----|----------|
| `Nm_NetworkRequest()` | Request network startup/keep active |
| `Nm_NetworkRelease()` | Release network (allow sleep) |
| `Nm_PassiveStartUp()` | Start NM passively |

### Communication Control APIs
| API | Function |
|-----|----------|
| `Nm_DisableCommunication()` | Disable PDU transmission |
| `Nm_EnableCommunication()` | Enable PDU transmission |

### State Query APIs
| API | Function |
|-----|----------|
| `Nm_GetState()` | Get current NM state |
| `Nm_GetMode()` | Get current NM mode |
| `Nm_GetLocalNodeIdentifier()` | Get local node ID |

### PDU Data APIs
| API | Function |
|-----|----------|
| `Nm_GetPduData()` | Get NM PDU data |
| `Nm_GetUserData()` | Get user data from NM PDU |
| `Nm_SetUserData()` | Set user data in NM PDU |
| `Nm_RepeatMessageRequest()` | Request repeat message state |

### Remote Sleep APIs
| API | Function |
|-----|----------|
| `Nm_CheckRemoteSleepIndication()` | Check remote sleep status |
| `Nm_GetCoordinatorSleepReady()` | Get coordinator sleep readiness |

### Main Function
| API | Function |
|-----|----------|
| `Nm_MainFunction()` | Periodic processing |

### Callback Functions (called by BusNm)
| API | Function |
|-----|----------|
| `Nm_BusSleepModeEntry()` | Enter bus sleep mode callback |
| `Nm_PrepareBusSleepModeEntry()` | Enter prepare bus sleep callback |
| `Nm_NetworkModeEntry()` | Enter network mode callback |
| `Nm_NetworkStartIndication()` | Network start indication |
| `Nm_RxIndication()` | NM message received |
| `Nm_StateChangeNotification()` | State change notification |
| `Nm_RemoteSleepIndication()` | Remote sleep detected |
| `Nm_RemoteSleepCancellation()` | Remote sleep cancelled |

## Configuration

### Configuration Type
```c
typedef struct {
    uint8 dummy;  /* Placeholder for future extensions */
} Nm_ConfigType;
```

### State Change Callback
```c
typedef void (*Nm_StateChangeNotificationCallbackType)(
    Nm_ChannelHandleType nmNetworkHandle,
    Nm_StateType nmPreviousState,
    Nm_StateType nmCurrentState
);
```

## Usage Examples

### Network Startup
```c
#include "Nm.h"

void StartNetwork(void)
{
    Std_ReturnType result;
    
    /* Request network startup */
    result = Nm_NetworkRequest(NM_CHANNEL_CAN0);
    
    if (result == E_OK) {
        /* Network request accepted */
    }
}
```

### Network Shutdown
```c
void ShutdownNetwork(void)
{
    Std_ReturnType result;
    
    /* Release network to allow sleep transition */
    result = Nm_NetworkRelease(NM_CHANNEL_CAN0);
    
    if (result == E_OK) {
        /* Network release accepted */
    }
}
```

### Passive Startup
```c
void PassiveJoinNetwork(void)
{
    Std_ReturnType result;
    
    /* Join network without requesting it to stay active */
    result = Nm_PassiveStartUp(NM_CHANNEL_CAN0);
    
    if (result == E_OK) {
        /* Passive startup accepted */
    }
}
```

### State Monitoring
```c
void MonitorNetworkState(void)
{
    Nm_StateType state;
    Nm_ModeType mode;
    Std_ReturnType result;
    
    result = Nm_GetState(NM_CHANNEL_CAN0, &state);
    if (result == E_OK) {
        switch (state) {
            case NM_STATE_REPEAT_MESSAGE:
                /* Handle repeat message state */
                break;
            case NM_STATE_NORMAL_OPERATION:
                /* Handle normal operation */
                break;
            case NM_STATE_READY_SLEEP:
                /* Handle ready sleep */
                break;
            case NM_STATE_BUS_SLEEP:
                /* Handle bus sleep */
                break;
            default:
                break;
        }
    }
    
    result = Nm_GetMode(NM_CHANNEL_CAN0, &mode);
    if (result == E_OK && mode == NM_MODE_NETWORK) {
        /* Network is active */
    }
}
```

### Communication Control
```c
void DisableNmCommunication(void)
{
    Std_ReturnType result;
    
    result = Nm_DisableCommunication(NM_CHANNEL_CAN0);
    
    if (result == E_OK) {
        /* Communication disabled */
    }
}

void EnableNmCommunication(void)
{
    Std_ReturnType result;
    
    result = Nm_EnableCommunication(NM_CHANNEL_CAN0);
    
    if (result == E_OK) {
        /* Communication enabled */
    }
}
```

### Remote Sleep Detection
```c
void CheckRemoteSleep(void)
{
    boolean remoteSleepInd;
    Std_ReturnType result;
    
    result = Nm_CheckRemoteSleepIndication(NM_CHANNEL_CAN0, &remoteSleepInd);
    
    if (result == E_OK && remoteSleepInd == TRUE) {
        /* All remote nodes ready for sleep */
    }
}
```

## State Machine

```
                    +---------------+
                    |   Uninit      |
                    +-------+-------+
                            |
                            | Init()
                            v
                    +---------------+
         +--------->|  Bus Sleep    |<---------+
         |          +-------+-------+          |
         |                  |                  |
         | Nm_NetworkReq()  |                  |
         |                  | Nm_NetworkRel()  |
         |                  v                  |
         |          +---------------+          |
         |          | Repeat Msg    |          |
         |          +-------+-------+          |
         |                  |                  |
         |                  | Timeout          |
         |                  v                  |
         |          +---------------+          |
         |   +----->| Normal Op     |<----+    |
         |   |      +-------+-------+     |    |
         |   |              |             |    |
         |   | Nm_RepeatMsg |             |    |
         |   | Request()    |             |    |
         |   |              |             |    |
         |   |              | Nm_Network  |    |
         |   |              | Rel()       |    |
         |   |              v             |    |
         |   |      +---------------+     |    |
         |   |      | Ready Sleep   |     |    |
         |   |      +-------+-------+     |    |
         |   |              |             |    |
         |   |              | Timeout     |    |
         |   |              v             |    |
         |   |      +---------------+     |    |
         +---+      | Prepare Sleep |     +----+
    Nm_Network      +-------+-------+
    Request()               |
                            | Timeout
                            v
                    +---------------+
                    |  Bus Sleep    |
                    +---------------+
```

## Dependencies

- **CanNm** - CAN Network Management (optional)
- **LinNm** - LIN Network Management (optional)
- **UdpNm** - UDP Network Management (optional)
- **ComM** - Communication Manager
- **BswM** - Basic Software Mode Manager

## Source Code

- `/home/admin/yuleASR/src/bsw/services/nm/`
  - `include/Nm.h` - Public API
  - `include/Nm_Cfg.h` - Configuration
  - `src/Nm.c` - Core implementation

## Tests

- `/home/admin/yuleASR/tests/unit/autosar/services/Nm/`

## References

- AUTOSAR_SWS_NetworkManagementInterface
- AUTOSAR Classic Platform 4.4.0
