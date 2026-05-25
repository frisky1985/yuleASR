# EthIf - Ethernet Interface Module

## Overview

EthIf implements the AUTOSAR Ethernet Interface, providing hardware-independent access to Ethernet controllers. It manages Ethernet frame transmission/reception, VLAN handling, time synchronization support, and wake-up functionality for automotive Ethernet networks.

**Module ID:** 0x41 (65)  
**Vendor ID:** 0x01 (YuleTech)  
**AutoSAR Version:** Classic Platform 4.4.0  
**Module Version:** 1.0.0

## Standards Compliance

- AUTOSAR SWS Ethernet Interface
- AUTOSAR Classic Platform 4.4.0
- IEEE 802.3 - Ethernet Standard
- IEEE 802.1Q - VLAN Tagging
- IEEE 802.1AS - Time Synchronization
- IEEE 802.1Qav/bv - Time-Sensitive Networking (TSN)

## Features

### Ethernet Frame Handling

- **Transmission:** Asynchronous frame transmission with confirmation
- **Reception:** Frame reception with indication callbacks
- **Frame Filtering:** Type-based frame routing to upper layers
- **VLAN Support:** IEEE 802.1Q VLAN tagging
- **Broadcast/Multicast/Unicast:** All addressing modes supported

### Controller Management

- **Multi-Controller:** Support for up to 2 Ethernet controllers
- **Mode Management:** DOWN/ACTIVE state control
- **Link Monitoring:** Link state detection and reporting
- **MAC Configuration:** Dynamic MAC address configuration
- **Speed/Duplex:** Support for 10/100/1000 Mbps and half/full duplex

### Time Synchronization

- **Hardware Timestamping:** Egress and ingress timestamp capture
- **Timestamp Quality:** Valid/Invalid/NotSupported indication
- **gPTP Support:** IEEE 802.1AS Precision Time Protocol
- **Time Base:** Seconds and nanoseconds precision

### Wake-up Management

- **Wake-up Modes:** Enable/Disable/Clear modes
- **Transceiver Control:** Wake-up signal configuration
- **EcuM Integration:** Wake-up source reporting
- **Power Management:** Low power state handling

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Upper Layers                          │
│  (SoAd, DoIP, SomeIp, StbM, EcuM)                       │
└───────────────────────┬─────────────────────────────────┘
                        │ EthIf APIs
┌───────────────────────▼─────────────────────────────────┐
│              EthIf - Ethernet Interface                  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐   │
│  │ Tx/Rx    │ │ Controller│ │ Timestamp│ │ Wake-up  │   │
│  │ Handler  │ │ Manager  │ │ Manager  │ │ Handler  │   │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘   │
└───────────────────────┬─────────────────────────────────┘
                        │ MCAL APIs
┌───────────────────────▼─────────────────────────────────┐
│              Eth / EthTrcv (MCAL)                        │
│           Ethernet Driver / Transceiver                  │
└─────────────────────────────────────────────────────────┘
```

## APIs

### Initialization APIs

| API | Service ID | Function |
|-----|------------|----------|
| `EthIf_Init()` | 0x01 | Initialize Ethernet Interface module |
| `EthIf_DeInit()` | - | De-initialize module (internal) |
| `EthIf_ControllerInit()` | 0x02 | Initialize specific controller |
| `EthIf_GetVersionInfo()` | 0x0B | Get version information |

### Controller APIs

| API | Service ID | Function | Parameters |
|-----|------------|----------|------------|
| `EthIf_SetControllerMode()` | 0x03 | Set controller mode | CtrlIdx, CtrlMode |
| `EthIf_GetControllerMode()` | 0x04 | Get controller mode | CtrlIdx, CtrlModePtr |
| `EthIf_GetPhysAddr()` | 0x05 | Get MAC address | CtrlIdx, PhysAddrPtr |
| `EthIf_SetPhysAddr()` | 0x06 | Set MAC address | CtrlIdx, PhysAddrPtr |
| `EthIf_GetCtrlIdx()` | 0x08 | Get controller index | - |

### Transmission/Reception APIs

| API | Service ID | Function | Parameters |
|-----|------------|----------|------------|
| `EthIf_Transmit()` | 0x0C | Transmit Ethernet frame | CtrlIdx, FrameType, Data, Length |
| `EthIf_Receive()` | - | Receive Ethernet frame | Data, Length |
| `EthIf_RxIndication()` | - | Reception callback | CtrlIdx, FrameType, IsBroadcast, SrcMac, Data, Length |
| `EthIf_TxConfirmation()` | - | Tx confirmation callback | CtrlIdx, BufIdx |

### Timestamp APIs

| API | Service ID | Function | Parameters |
|-----|------------|----------|------------|
| `EthIf_GetCurrentTime()` | 0x0D | Get current time | CtrlIdx, TimeStampPtr |
| `EthIf_EnableEgressTimeStamp()` | 0x0E | Enable egress timestamp | CtrlIdx, BufIdx |
| `EthIf_GetEgressTimeStamp()` | 0x0F | Get egress timestamp | CtrlIdx, BufIdx, TimeStampPtr, QualityPtr |
| `EthIf_GetIngressTimeStamp()` | 0x10 | Get ingress timestamp | CtrlIdx, DataPtr, TimeStampPtr, QualityPtr |

### Wake-up APIs

| API | Service ID | Function | Parameters |
|-----|------------|----------|------------|
| `EthIf_GetTransceiverWakeupMode()` | 0x13 | Get wake-up mode | CtrlIdx, WakeupModePtr |
| `EthIf_SetTransceiverWakeupMode()` | 0x14 | Set wake-up mode | CtrlIdx, WakeupMode |
| `EthIf_CheckWakeup()` | 0x15 | Check for wake-up | CtrlIdx |

### Periodic API

| API | Service ID | Function |
|-----|------------|----------|
| `EthIf_MainFunction()` | 0x11 | Periodic processing (called by OS) |

## Type Definitions

### Controller Mode
```c
typedef enum {
    ETHIF_MODE_DOWN = 0,    /* Controller disabled */
    ETHIF_MODE_ACTIVE       /* Controller active */
} EthIf_ControllerModeType;
```

### Link State
```c
typedef enum {
    ETHIF_LINK_STATE_DOWN = 0,   /* Link down */
    ETHIF_LINK_STATE_ACTIVE      /* Link active */
} EthIf_LinkStateType;
```

### Speed Types
```c
typedef enum {
    ETHIF_SPEED_10MBPS = 0,     /* 10 Mbps */
    ETHIF_SPEED_100MBPS,        /* 100 Mbps */
    ETHIF_SPEED_1GBPS,          /* 1 Gbps */
    ETHIF_SPEED_2_5GBPS,        /* 2.5 Gbps */
    ETHIF_SPEED_10GBPS          /* 10 Gbps */
} EthIf_SpeedType;
```

### Duplex Types
```c
typedef enum {
    ETHIF_DUPLEX_HALF = 0,      /* Half duplex */
    ETHIF_DUPLEX_FULL           /* Full duplex */
} EthIf_DuplexType;
```

### Timestamp Quality
```c
typedef enum {
    ETHIF_TIMESTAMP_VALID = 0,          /* Valid timestamp */
    ETHIF_TIMESTAMP_INVALID,            /* Invalid timestamp */
    ETHIF_TIMESTAMP_NOT_SUPPORTED       /* Timestamp not supported */
} EthIf_TimestampQualityType;
```

### Wake-up Mode
```c
typedef enum {
    ETHIF_TRCV_WU_ENABLE = 0,   /* Enable wake-up */
    ETHIF_TRCV_WU_DISABLE,      /* Disable wake-up */
    ETHIF_TRCV_WU_CLEAR         /* Clear wake-up flags */
} EthIf_TransceiverWakeupModeType;
```

### MAC Address
```c
typedef uint8 EthIf_MacAddrType[6];  /* 48-bit MAC address */
```

### Timestamp
```c
typedef struct {
    uint32 seconds;         /* Seconds since epoch */
    uint32 nanoseconds;     /* Nanoseconds (0-999999999) */
} EthIf_TimestampType;
```

## Configuration

### Pre-Compile Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| `ETHIF_DEV_ERROR_DETECT` | STD_ON | Enable Development Error Detection |
| `ETHIF_VERSION_INFO_API` | STD_ON | Enable version info API |
| `ETHIF_ENABLE_WAKEUP_MODE_API` | STD_ON | Enable wake-up mode API |
| `ETHIF_GET_WAKEUP_MODE_API` | STD_ON | Enable get wake-up mode API |
| `ETHIF_GET_CTRL_IDX_API` | STD_ON | Enable get controller index API |
| `ETHIF_GET_VLAN_IDX_API` | STD_ON | Enable get VLAN index API |
| `ETHIF_GET_AND_RESET_MEASUREMENT_DATA_API` | STD_ON | Enable measurement data API |
| `ETHIF_GET_CURRENT_TIME_API` | STD_ON | Enable current time API |
| `ETHIF_ENABLE_EGRESS_TIMESTAMP_API` | STD_ON | Enable egress timestamp API |
| `ETHIF_GET_EGRESS_TIMESTAMP_API` | STD_ON | Enable get egress timestamp API |
| `ETHIF_GET_INGRESS_TIMESTAMP_API` | STD_ON | Enable get ingress timestamp API |
| `ETHIF_WAKEUP_SUPPORT` | STD_ON | Enable wake-up support |
| `ETHIF_SWITCH_SUPPORT` | STD_ON | Enable switch support |
| `ETHIF_TIME_SYNC_ENABLED` | STD_ON | Enable time synchronization |
| `ETHIF_GPTP_SUPPORT` | STD_ON | Enable gPTP support |

### Numeric Configuration

| Parameter | Value | Description |
|-----------|-------|-------------|
| `ETHIF_NUM_CONTROLLERS` | 2 | Number of Ethernet controllers |
| `ETHIF_NUM_FRAME_OWNERS` | 8 | Number of frame owner configurations |
| `ETHIF_NUM_VLANS` | 4 | Number of VLAN configurations |
| `ETHIF_NUM_SWITCH_PORTS` | 4 | Number of switch ports |
| `ETHIF_MTU_DEFAULT` | 1500 | Default MTU size |
| `ETHIF_MTU_JUMBO` | 9000 | Jumbo frame MTU size |
| `ETHIF_MAIN_FUNCTION_PERIOD_MS` | 5 | Main function period in ms |

### VLAN Configuration

| VLAN ID | Purpose |
|---------|---------|
| 1 | Default VLAN |
| 100 | Diagnostic (DoIP) |
| 200 | ADAS |
| 300 | Infotainment |

### Frame Types

| Frame Type | Value | Protocol |
|------------|-------|----------|
| `ETHIF_FRAMETYPE_IPV4` | 0x0800 | IPv4 |
| `ETHIF_FRAMETYPE_IPV6` | 0x86DD | IPv6 |
| `ETHIF_FRAMETYPE_ARP` | 0x0806 | ARP |
| `ETHIF_FRAMETYPE_VLAN` | 0x8100 | 802.1Q VLAN |
| `ETHIF_FRAMETYPE_SOMEIP` | 0x88E0 | SOME/IP |
| `ETHIF_FRAMETYPE_TSN` | 0x88F7 | TSN (gPTP) |

## Error Codes

### DET Error Codes

| Code | Value | Description |
|------|-------|-------------|
| `ETHIF_E_INV_CTRL_IDX` | 0x01 | Invalid controller index |
| `ETHIF_E_INV_TRCV_IDX` | 0x02 | Invalid transceiver index |
| `ETHIF_E_INV_SWITCH_IDX` | 0x03 | Invalid switch index |
| `ETHIF_E_INV_SWITCH_GRP_IDX` | 0x04 | Invalid switch group index |
| `ETHIF_E_INV_PARAM_POINTER` | 0x05 | NULL pointer error |
| `ETHIF_E_INV_MODE` | 0x06 | Invalid mode |
| `ETHIF_E_INV_CONFIG` | 0x07 | Invalid configuration |
| `ETHIF_E_INV_VLAN_IDX` | 0x08 | Invalid VLAN index |
| `ETHIF_E_INV_MTU` | 0x09 | Invalid MTU |
| `ETHIF_E_INV_TIMER` | 0x0A | Invalid timer |
| `ETHIF_E_INV_TIMESTAMP_TYPE` | 0x0B | Invalid timestamp type |
| `ETHIF_E_INV_WAKEUP_MODE` | 0x0C | Invalid wake-up mode |
| `ETHIF_E_INV_PHY_ADDRESS` | 0x0D | Invalid MAC address |
| `ETHIF_E_INV_FRAME_TYPE` | 0x0E | Invalid frame type |
| `ETHIF_E_INV_FRAME_ID` | 0x0F | Invalid frame ID |
| `ETHIF_E_INV_CHANNEL` | 0x10 | Invalid channel |
| `ETHIF_E_UNINIT` | 0x20 | Module not initialized |
| `ETHIF_E_ALREADY_INITIALIZED` | 0x21 | Module already initialized |

## State Machine

### Module State
```
UNINIT ──EthIf_Init()──> INIT ──EthIf_DeInit()──> UNINIT
```

### Controller State
```
                      EthIf_SetControllerMode(DOWN)
DOWN <───────────────────────────────────────────────────────> ACTIVE
      ──────────────────────────────────────────────────────>
                      EthIf_SetControllerMode(ACTIVE)
```

## Usage Examples

### Basic Initialization
```c
#include "EthIf.h"

void EthIf_InitExample(void)
{
    /* Initialize Ethernet Interface module */
    EthIf_Init(&EthIf_Config);
    
    /* Initialize controller 0 */
    EthIf_ControllerInit(0, 0);
    
    /* Set controller to active mode */
    Std_ReturnType result = EthIf_SetControllerMode(0, ETHIF_MODE_ACTIVE);
    
    if (result == E_OK) {
        /* Configure MAC address */
        uint8 macAddr[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        EthIf_SetPhysAddr(0, macAddr);
    }
}
```

### Frame Transmission
```c
void EthIf_TransmitExample(void)
{
    uint8 frameData[1500];
    uint16 frameLen = 100;
    
    /* Prepare Ethernet frame */
    /* Destination MAC: 6 bytes */
    frameData[0] = 0x00; frameData[1] = 0x11; frameData[2] = 0x22;
    frameData[3] = 0x33; frameData[4] = 0x44; frameData[5] = 0x55;
    
    /* Source MAC: 6 bytes */
    frameData[6] = 0x66; frameData[7] = 0x77; frameData[8] = 0x88;
    frameData[9] = 0x99; frameData[10] = 0xAA; frameData[11] = 0xBB;
    
    /* EtherType: IPv4 (0x0800) */
    frameData[12] = 0x08; frameData[13] = 0x00;
    
    /* Payload */
    /* ... fill payload data ... */
    
    /* Enable egress timestamp for this transmission */
    EthIf_EnableEgressTimeStamp(0, 0);
    
    /* Transmit frame */
    Std_ReturnType result = EthIf_Transmit(0, 0x0800, frameData, frameLen);
    
    if (result == E_OK) {
        /* Transmission initiated successfully */
        /* Wait for TxConfirmation callback */
    }
}
```

### Frame Reception
```c
/* Reception indication callback (called by Eth driver) */
void EthIf_RxIndication(
    uint8 CtrlIdx,
    EthIf_FrameType FrameType,
    boolean IsBroadcast,
    const uint8* PhysAddrPtr,
    const uint8* DataPtr,
    uint16 LenByte)
{
    /* Process received frame based on frame type */
    switch (FrameType) {
        case 0x0800:  /* IPv4 */
            /* Process IPv4 packet */
            break;
        case 0x0806:  /* ARP */
            /* Process ARP packet */
            break;
        case 0x86DD:  /* IPv6 */
            /* Process IPv6 packet */
            break;
        default:
            /* Unknown frame type */
            break;
    }
}
```

### Time Synchronization
```c
void EthIf_TimestampExample(void)
{
    EthIf_TimestampType timestamp;
    EthIf_TimestampQualityType quality;
    
    /* Get current time from controller */
    Std_ReturnType result = EthIf_GetCurrentTime(0, &timestamp);
    
    if (result == E_OK) {
        uint32 seconds = timestamp.seconds;
        uint32 nanoseconds = timestamp.nanoseconds;
        /* Use timestamp for synchronization */
    }
    
    /* Get egress timestamp after transmission */
    EthIf_GetEgressTimeStamp(0, 0, &timestamp, &quality);
    
    if (quality == ETHIF_TIMESTAMP_VALID) {
        /* Valid timestamp for PTP calculation */
    }
    
    /* Get ingress timestamp for received frame */
    EthIf_GetIngressTimeStamp(0, receivedData, &timestamp, &quality);
}
```

### Wake-up Management
```c
void EthIf_WakeupExample(void)
{
    /* Enable wake-up on transceiver 0 */
    EthIf_SetTransceiverWakeupMode(0, ETHIF_TRCV_WU_ENABLE);
    
    /* ... enter low power mode ... */
    
    /* Check for wake-up events */
    EthIf_CheckWakeup(0);
    
    /* Get current wake-up mode */
    EthIf_TransceiverWakeupModeType mode;
    EthIf_GetTransceiverWakeupMode(0, &mode);
    
    /* Clear wake-up flags */
    EthIf_SetTransceiverWakeupMode(0, ETHIF_TRCV_WU_CLEAR);
}
```

### Main Function (Periodic)
```c
/* Called periodically by OS (e.g., every 5ms) */
void EthIf_MainFunctionExample(void)
{
    /* Process periodic tasks:
     * - Link state monitoring
     * - Buffer management
     * - Timeout handling
     */
    EthIf_MainFunction();
}
```

## Dependencies

### Required Dependencies

| Module | Layer | Purpose |
|--------|-------|---------|
| `Eth` | MCAL | Ethernet driver |
| `EthTrcv` | MCAL | Ethernet transceiver driver |
| `Det` | Service | Development error tracing |
| `SchM` | Service | Schedule manager |

### Optional Dependencies

| Module | Layer | Purpose |
|--------|-------|---------|
| `SoAd` | ECUAL | Socket Adapter for TCP/IP |
| `DoIP` | Service | Diagnostic over IP |
| `SomeIp` | Service | SOME/IP protocol |
| `StbM` | Service | Synchronized Time Base Manager |
| `EcuM` | Service | ECU Manager for wake-up |
| `BswM` | Service | BSW Mode Manager |

## File Structure

```
src/bsw/ecual/ethif/
├── include/
│   ├── EthIf.h         # Public API header
│   └── EthIf_Cfg.h     # Configuration header
└── src/
    ├── EthIf.c         # Core implementation
    └── EthIf_Lcfg.c    # Link-time configuration

tests/unit/autosar/ecual/
├── test_EthIf.c        # Original unit tests
└── test_ethif.c        # Comprehensive unit tests (coverage 80%+)

docs/modules/
└── EthIf.md            # This documentation
```

## Unit Tests

Comprehensive unit tests are provided in `test_ethif.c` covering:

### Test Categories

| Category | Test Count | Coverage Target |
|----------|------------|-----------------|
| Initialization | 5 | 100% |
| Controller Management | 6 | 90% |
| MAC Address | 4 | 90% |
| Transmission | 4 | 90% |
| Reception | 4 | 85% |
| Callbacks | 2 | 90% |
| Timestamp | 7 | 85% |
| Main Function | 2 | 90% |
| Type Definitions | 7 | 100% |
| Service IDs | 2 | 100% |
| Configuration | 3 | 100% |
| Integrated Flows | 4 | 80% |

**Total Tests:** 50+  
**Target Coverage:** 80%+

## Testing

### Build Tests
```bash
cd /home/admin/yuleASR
mkdir -p build && cd build
cmake ..
make test_ethif
```

### Run Tests
```bash
./tests/unit/autosar/ecual/test_ethif
```

### Coverage Report
```bash
make coverage
gcovr -r .. --html --html-details -o coverage.html
```

## Performance Characteristics

| Metric | Typical Value |
|--------|---------------|
| Initialization Time | < 1ms |
| Frame Transmission Latency | < 100us |
| Frame Reception Latency | < 100us |
| Main Function Execution | < 500us |
| Timestamp Accuracy | < 100ns (with HW support) |
| Memory Footprint | ~5-10 KB RAM |
| Code Size | ~20-30 KB Flash |

## References

- [AUTOSAR_SWS_EthernetInterface.pdf](https://www.autosar.org/standards/classic-platform/)
- IEEE 802.3-2018 - IEEE Standard for Ethernet
- IEEE 802.1Q-2018 - Bridges and Bridged Networks
- IEEE 802.1AS-2020 - Timing and Synchronization for Time-Sensitive Applications
- IEEE 802.1Qav-2009 - Forwarding and Queuing Enhancements for Time-Sensitive Streams

## Revision History

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2026-04-14 | YuleTech | Initial implementation |
| 1.1.0 | 2026-05-15 | YuleTech | Added comprehensive unit tests |

---

**© 2026 Shanghai Yule Electronics Technology Co., Ltd. All rights reserved.**
