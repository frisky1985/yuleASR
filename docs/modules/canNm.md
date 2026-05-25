# CanNm - CAN Network Management

## Overview

CanNm implements the AUTOSAR CAN Network Management protocol, providing coordinated management of network communication across ECUs in a CAN network. It controls network startup and shutdown based on communication needs, enabling power-saving sleep modes when communication is not required.

## Standards

- AUTOSAR SWS CAN Network Management
- AUTOSAR Classic Platform 4.4.0
- ISO 11898 - CAN Data Link Layer
- MISRA C:2012

## Features

### Network Management States

- **Bus Sleep Mode** - Network is inactive, minimal power consumption
- **Prepare Bus Sleep Mode** - Transition state before sleep
- **Repeat Message Mode** - Rapid NM message transmission for node detection
- **Normal Operation Mode** - Standard NM message transmission
- **Ready Sleep Mode** - Ready to enter sleep, monitoring bus activity

### State Machine

```
                    +------------------+
                    |    Uninit        |
                    +--------+---------+
                             | Init()
                             v
                    +--------+---------+
         +--------->|   Bus Sleep      |<-----------------+
         |          +--------+---------+                  |
         |                   | NetworkRequest()           |
         |                   v                            |
         |          +--------+---------+                  |
         |          | Repeat Message   |                  |
         |          +--------+---------+                  |
         |                   | Timeout                    |
         |                   v                            |
         |          +--------+---------+                  |
         |   +----->| Normal Operation |<-------+         |
         |   |      +--------+---------+        |         |
         |   |                   |              |         |
         |   |                   | Release()    |         |
         |   |                   v              |         |
         |   |      +--------+---------+        |         |
         |   |      |  Ready Sleep     |        |         |
         |   |      +--------+---------+        |         |
         |   |                   |              |         |
         |   |                   | Timeout      |         |
         |   |                   v              |         |
         |   |      +--------+---------+        |         |
         +---+      | Prepare Bus Sleep|        +---------+
    NetworkRequest() +--------+---------+  RepeatMsg Rqst
                             |
                             | Timeout
                             v
                    +--------+---------+
                    |   Bus Sleep      |
                    +------------------+
```

### Core Functions

- **Network Request/Release** - Request or release network communication
- **Passive Startup** - Join network without requesting active communication
- **State Machine Management** - Automatic state transitions based on timers and requests
- **Periodic Message Transmission** - Cyclic NM message transmission in Network Mode
- **Remote Sleep Detection** - Detect when all remote nodes are ready for sleep
- **Wakeup Detection** - Detect network activity and wake from Bus Sleep

### PDU Structure

```
+--------+--------+--------+--------+--------+--------+--------+--------+
| Byte 0 | Byte 1 | Byte 2 | Byte 3 | Byte 4 | Byte 5 | Byte 6 | Byte 7 |
+--------+--------+--------+--------+--------+--------+--------+--------+
|  NID   |  CBV   |        User Data (6 bytes)                          |
+--------+--------+--------+--------+--------+--------+--------+--------+
```

- **NID** (Byte 0) - Node Identifier
- **CBV** (Byte 1) - Control Bit Vector
  - Bit 0: Repeat Message Request
  - Bit 2: Active Wakeup Bit
  - Bit 4: NM Coordinator Sleep Bit

## Architecture

```
┌─────────────────────────────────────────┐
│         ComM / EcuM / BswM              │
│     (Communication/Mode Management)     │
└─────────────┬───────────────────────────┘
              │
              v
┌─────────────────────────────────────────┐
│            Nm Interface                 │
│    (Generic Network Management API)     │
└─────────────┬───────────────────────────┘
              │
              v
┌─────────────────────────────────────────┐
│              CanNm                      │
│  ┌───────────────────────────────────┐  │
│  │  State Machine                    │  │
│  │  - Bus Sleep / Network Modes      │  │
│  │  - Repeat Msg / Normal Op / Sleep │  │
│  └───────────────────────────────────┘  │
│  ┌───────────────────────────────────┐  │
│  │  Message Transmission             │  │
│  │  - Periodic NM messages           │  │
│  │  - Immediate transmissions        │  │
│  └───────────────────────────────────┘  │
│  ┌───────────────────────────────────┐  │
│  │  Reception Handling               │  │
│  │  - Wakeup detection               │  │
│  │  - Repeat msg request handling    │  │
│  └───────────────────────────────────┘  │
└─────────────┬───────────────────────────┘
              │
              v
┌─────────────────────────────────────────┐
│              CanIf                      │
│       (CAN Interface Layer)             │
└─────────────┬───────────────────────────┘
              │
              v
┌─────────────────────────────────────────┐
│              CAN Driver                 │
│          (MCAL Layer)                   │
└─────────────────────────────────────────┘
```

## APIs

### Initialization APIs

| API | Function |
|-----|----------|
| `CanNm_Init()` | Initialize CAN NM module |
| `CanNm_DeInit()` | Deinitialize CAN NM module |
| `CanNm_GetVersionInfo()` | Get version information |

### Network Control APIs

| API | Function |
|-----|----------|
| `CanNm_NetworkRequest()` | Request network startup/keep active |
| `CanNm_NetworkRelease()` | Release network (allow sleep) |
| `CanNm_PassiveStartUp()` | Join network passively |

### State Query APIs

| API | Function |
|-----|----------|
| `CanNm_GetState()` | Get current NM state and mode |
| `CanNm_CheckRemoteSleepIndication()` | Check remote sleep status |

### PDU Data APIs

| API | Function |
|-----|----------|
| `CanNm_SetUserData()` | Set user data in NM PDU |
| `CanNm_GetUserData()` | Get user data from NM PDU |
| `CanNm_SetSleepReadyBit()` | Set sleep ready bit in CBV |
| `CanNm_TriggerTransmit()` | Trigger transmit callback |

### Communication Control APIs

| API | Function |
|-----|----------|
| `CanNm_DisableCommunication()` | Disable NM PDU transmission |
| `CanNm_EnableCommunication()` | Enable NM PDU transmission |

### Callback APIs (called by CanIf)

| API | Function |
|-----|----------|
| `CanNm_RxIndication()` | NM message received indication |
| `CanNm_TxConfirmation()` | Transmission confirmation |

### Main Function

| API | Function |
|-----|----------|
| `CanNm_MainFunction()` | Periodic processing (10ms typical) |

## NM States

| State | Value | Description |
|-------|-------|-------------|
| `NM_STATE_UNINIT` | 0x00 | Module not initialized |
| `NM_STATE_BUS_SLEEP` | 0x01 | Bus in sleep mode |
| `NM_STATE_PREPARE_BUS_SLEEP` | 0x02 | Preparing for sleep |
| `NM_STATE_READY_SLEEP` | 0x03 | Ready for sleep |
| `NM_STATE_NORMAL_OPERATION` | 0x04 | Normal operation |
| `NM_STATE_REPEAT_MESSAGE` | 0x05 | Repeat message state |

## NM Modes

| Mode | Value | Description |
|------|-------|-------------|
| `NM_MODE_BUS_SLEEP` | 0x00 | Bus sleep mode |
| `NM_MODE_PREPARE_BUS_SLEEP` | 0x01 | Prepare bus sleep mode |
| `NM_MODE_SYNCHRONIZE` | 0x02 | Synchronize mode |
| `NM_MODE_NETWORK` | 0x03 | Network mode (active) |

## Configuration

### Timing Parameters

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `NmTimeoutTime` | 1000ms | 10-5000ms | Timeout for NM messages before bus-off |
| `RepeatMessageTime` | 400ms | 10-65535ms | Duration of Repeat Message State |
| `WaitBusSleepTime` | 1000ms | 10-65535ms | Duration of Prepare Bus Sleep State |
| `MessageCycleTime` | 100ms | 10-65535ms | Period between NM message transmissions |
| `MessageCycleOffset` | 10ms | 0-65535ms | Offset for first transmission |
| `ImmediateNmCycleTime` | 20ms | 10-65535ms | Fast cycle time for immediate transmissions |
| `ImmediateNmTransmissions` | 3 | 1-255 | Number of immediate transmissions |
| `RemoteSleepIndTime` | 2000ms | 10-65535ms | Time for remote sleep indication |

### Feature Switches

| Parameter | Default | Description |
|-----------|---------|-------------|
| `CANNM_DEV_ERROR_DETECT` | STD_ON | Development error detection |
| `CANNM_VERSION_INFO_API` | STD_ON | Version info API available |
| `CANNM_COM_CONTROL_ENABLED` | STD_ON | Communication control APIs |
| `CANNM_COORDINATOR_SUPPORT_ENABLED` | STD_ON | Coordinator synchronization |
| `CANNM_PASSIVE_MODE_ENABLED` | STD_OFF | Passive mode (no transmission) |
| `CANNM_NODE_DETECTION_ENABLED` | STD_ON | Node detection support |
| `CANNM_NODE_ID_ENABLED` | STD_ON | Node ID in NM PDU |
| `CANNM_REMOTE_SLEEP_IND_ENABLED` | STD_ON | Remote sleep indication |
| `CANNM_USER_DATA_ENABLED` | STD_ON | User data in NM PDU |
| `CANNM_PN_ENABLED` | STD_ON | Partial networking support |
| `CANNM_STATE_CHANGE_IND_ENABLED` | STD_ON | State change notifications |
| `CANNM_IMMEDIATE_TRANSMIT_ENABLED` | STD_ON | Immediate transmissions |
| `CANNM_ACTIVE_WAKEUP_BIT_ENABLED` | STD_ON | Active wakeup bit in CBV |

### Channel Configuration

```c
typedef struct {
    uint8                       ChannelId;              /* Channel identifier */
    uint16                      NmTimeoutTime;          /* NM timeout in ms */
    uint16                      RepeatMessageTime;      /* Repeat message time */
    uint16                      WaitBusSleepTime;       /* Wait bus sleep time */
    uint16                      MessageCycleTime;       /* Message cycle time */
    uint16                      MessageCycleOffset;     /* Message cycle offset */
    uint16                      ImmediateNmCycleTime;   /* Immediate cycle time */
    uint8                       ImmediateNmTransmissions; /* Immediate TX count */
    uint8                       NidPosition;            /* Node ID position */
    uint8                       CbvPosition;            /* CBV position */
    boolean                     NodeDetectionEnabled;   /* Node detection */
    boolean                     NodeIdEnabled;          /* Node ID enabled */
    boolean                     PassiveModeEnabled;     /* Passive mode */
    boolean                     RemoteSleepIndEnabled;  /* Remote sleep ind */
    boolean                     ActiveWakeupBitEnabled; /* Active wakeup bit */
    boolean                     ComControlEnabled;      /* Communication control */
    boolean                     CoordinatorSyncSupport; /* Coordinator sync */
    uint8                       PduLength;              /* PDU length */
    uint8                       NodeId;                 /* Node ID */
    PduIdType                   TxPduId;              /* Transmit PDU ID */
    PduIdType                   RxPduId;              /* Receive PDU ID */
} CanNm_ChannelConfigType;
```

## Dependencies

- **CanIf** - CAN Interface for PDU transmission/reception
- **Nm** - Generic Network Management Interface
- **ComM** - Communication Manager for mode requests
- **EcuM** - ECU Manager for wakeup handling
- **DET** - Development Error Tracer (optional)

## Usage Examples

### Network Startup

```c
#include "CanNm.h"

void StartNetwork(void)
{
    Std_ReturnType result;
    
    /* Request network startup - active wakeup */
    result = CanNm_NetworkRequest(CANNM_CHANNEL_0);
    
    if (result == E_OK) {
        /* Network request accepted, CanNm will:
         * 1. Enter Repeat Message State
         * 2. Send immediate NM messages
         * 3. Transition to Normal Operation after timeout
         */
    }
}
```

### Network Shutdown

```c
void ShutdownNetwork(void)
{
    Std_ReturnType result;
    
    /* Release network to allow sleep transition */
    result = CanNm_NetworkRelease(CANNM_CHANNEL_0);
    
    if (result == E_OK) {
        /* Network release accepted, CanNm will:
         * 1. Transition to Ready Sleep (stop transmitting)
         * 2. Wait for NM timeout
         * 3. Transition to Prepare Bus Sleep
         * 4. Finally enter Bus Sleep mode
         */
    }
}
```

### Passive Startup

```c
void PassiveJoinNetwork(void)
{
    Std_ReturnType result;
    
    /* Join network without requesting it to stay active */
    result = CanNm_PassiveStartUp(CANNM_CHANNEL_0);
    
    if (result == E_OK) {
        /* Passive startup accepted:
         * - Enter Repeat Message State
         * - Participate in network without keeping it active
         * - Will transition to Ready Sleep after Repeat Message Time
         */
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
    
    result = CanNm_GetState(CANNM_CHANNEL_0, &state, &mode);
    
    if (result == E_OK) {
        switch (state) {
            case NM_STATE_REPEAT_MESSAGE:
                /* Handle repeat message state - fast NM messages */
                break;
                
            case NM_STATE_NORMAL_OPERATION:
                /* Handle normal operation - active communication */
                break;
                
            case NM_STATE_READY_SLEEP:
                /* Handle ready sleep - ready for shutdown */
                break;
                
            case NM_STATE_BUS_SLEEP:
                /* Handle bus sleep - network inactive */
                break;
                
            default:
                break;
        }
    }
}
```

### Communication Control

```c
void DisableNmCommunication(void)
{
    Std_ReturnType result;
    
    result = CanNm_DisableCommunication(CANNM_CHANNEL_0);
    
    if (result == E_OK) {
        /* NM PDU transmission disabled
         * Reception continues normally
         */
    }
}

void EnableNmCommunication(void)
{
    Std_ReturnType result;
    
    result = CanNm_EnableCommunication(CANNM_CHANNEL_0);
    
    if (result == E_OK) {
        /* NM PDU transmission re-enabled */
    }
}
```

### User Data Handling

```c
void SetNmUserData(void)
{
    uint8 userData[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    Std_ReturnType result;
    
    result = CanNm_SetUserData(CANNM_CHANNEL_0, userData);
    
    if (result == E_OK) {
        /* User data will be included in next NM message transmission */
    }
}

void GetNmUserData(void)
{
    uint8 userData[6];
    Std_ReturnType result;
    
    result = CanNm_GetUserData(CANNM_CHANNEL_0, userData);
    
    if (result == E_OK) {
        /* userData now contains the received user data from last NM message */
    }
}
```

### Sleep Ready Coordination

```c
void SetSleepReady(void)
{
    Std_ReturnType result;
    
    /* Indicate that this ECU is ready for sleep */
    result = CanNm_SetSleepReadyBit(CANNM_CHANNEL_0, TRUE);
    
    if (result == E_OK) {
        /* Sleep ready bit set in CBV
         * Other nodes can see this and coordinate sleep
         */
    }
}

void ClearSleepReady(void)
{
    Std_ReturnType result;
    
    /* Indicate that this ECU is not ready for sleep */
    result = CanNm_SetSleepReadyBit(CANNM_CHANNEL_0, FALSE);
}
```

## Error Handling

### DET Error Codes

| Code | Description |
|------|-------------|
| `CANNM_E_NOT_INITIALIZED` | Module not initialized |
| `CANNM_E_INVALID_CHANNEL` | Invalid channel handle |
| `CANNM_E_INVALID_PDUID` | Invalid PDU identifier |
| `CANNM_E_PARAM_POINTER` | NULL pointer error |
| `CANNM_E_ALREADY_INITIALIZED` | Double initialization |
| `CANNM_E_NETWORK_TIMEOUT` | NM message timeout |

### Production Errors

| Error | Description |
|-------|-------------|
| `CANNM_E_NETWORK_TIMEOUT` | No NM message received within NmTimeoutTime |

## Testing

### Unit Tests

Unit tests are located at:
- `/home/admin/yuleASR/tests/unit/autosar/ecual/test_canNm.c`

Test coverage includes:
- Initialization and deinitialization
- Network request and release
- State machine transitions
- Message transmission and reception
- User data handling
- Communication control
- Error handling
- Multiple channel operation

### Test Execution

```bash
cd /home/admin/yuleASR
cmake -B build -DENABLE_TESTS=ON
cmake --build build
./build/tests/unit/autosar/ecual/test_canNm
```

## Source Code

- `/home/admin/yuleASR/src/bsw/ecual/canNm/`
  - `include/CanNm.h` - Public API
  - `include/CanNm_Cfg.h` - Configuration
  - `src/CanNm.c` - Core implementation
  - `src/CanNm_Lcfg.c` - Link-time configuration

## References

- AUTOSAR_SWS_CANNetworkManagement
- AUTOSAR Classic Platform 4.4.0
- ISO 11898-1:2015 - Road vehicles - Controller area network (CAN)
