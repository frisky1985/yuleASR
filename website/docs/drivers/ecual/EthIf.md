---
title: EthIf
sidebar_label: EthIf
description: "EthIf implements the AUTOSAR Ethernet Interface, providing hardware-independent access to Ethernet controllers. It manag"
sidebar_position: 10
---

# EthIf - Ethernet Interface Module

## Overview

EthIf implements the AUTOSAR Ethernet Interface, providing hardware-independent access to Ethernet controllers. It manages Ethernet frame transmission/reception, VLAN handling, and time synchronization support for automotive Ethernet networks.

## Standards

- AUTOSAR SWS Ethernet Interface
- AUTOSAR Classic Platform 4.4.0
- IEEE 802.3 - Ethernet
- IEEE 802.1Q - VLAN Tagging
- IEEE 802.1AS - Time Synchronization

## Features

### Ethernet Frame Handling

- Frame transmission and reception
- Frame type filtering and routing
- VLAN tag support
- Broadcast/multicast/unicast handling

### Controller Management

- Controller initialization
- Controller mode management (DOWN/ACTIVE)
- Link state monitoring
- MAC address configuration

### Time Synchronization

- Egress timestamping
- Ingress timestamping
- Timestamp quality indication
- gPTP (IEEE 802.1AS) support

### Wake-up Management

- Transceiver wake-up mode configuration
- Wake-up source detection
- Power management integration

## APIs

### Initialization APIs

| API | Function |
|-----|----------|
| `EthIf_Init()` | Initialize Ethernet Interface |
| `EthIf_ControllerInit()` | Initialize specific controller |
| `EthIf_GetVersionInfo()` | Get version information |

### Controller APIs

| API | Function |
|-----|----------|
| `EthIf_SetControllerMode()` | Set controller mode |
| `EthIf_GetControllerMode()` | Get controller mode |
| `EthIf_GetPhysAddr()` | Get MAC address |
| `EthIf_SetPhysAddr()` | Set MAC address |

### Transmission API

| API | Function |
|-----|----------|
| `EthIf_Transmit()` | Transmit Ethernet frame |

### Timestamp APIs

| API | Function |
|-----|----------|
| `EthIf_GetCurrentTime()` | Get current time |
| `EthIf_EnableEgressTimeStamp()` | Enable egress timestamp |
| `EthIf_GetEgressTimeStamp()` | Get egress timestamp |
| `EthIf_GetIngressTimeStamp()` | Get ingress timestamp |

### Wake-up APIs

| API | Function |
|-----|----------|
| `EthIf_GetTransceiverWakeupMode()` | Get wake-up mode |
| `EthIf_SetTransceiverWakeupMode()` | Set wake-up mode |
| `EthIf_CheckWakeup()` | Check for wake-up events |

### Callback APIs

| API | Function |
|-----|----------|
| `EthIf_RxIndication()` | Reception callback |
| `EthIf_TxConfirmation()` | Transmission confirmation |
| `EthIf_MainFunction()` | Periodic processing |

## Configuration

### Controller Configuration

| Parameter | Description |
|-----------|-------------|
| CtrlIdx | Controller index |
| EthCtrlIdx | Ethernet driver controller index |
| EthTrcvIdx | Ethernet transceiver index |
| PhysAddr | MAC address |
| Mtu | Maximum transmission unit |
| CtrlEnableWakeup | Wake-up enabled |

### Frame Owner Configuration

| Parameter | Description |
|-----------|-------------|
| FrameType | Ethernet frame type (0x0800=IP, etc.) |
| OwnerIdx | Upper layer owner index |
| HeaderByteOffsetApi | Header offset API enabled |

### VLAN Configuration

| Parameter | Description |
|-----------|-------------|
| VlanId | VLAN identifier (1-4094) |
| CtrlIdx | Associated controller |
| Priority | VLAN priority (0-7) |

### Controller Modes

| Mode | Description |
|------|-------------|
| ETHIF_MODE_DOWN | Controller disabled |
| ETHIF_MODE_ACTIVE | Controller active |

### Link States

| State | Description |
|-------|-------------|
| ETHIF_LINK_STATE_DOWN | Link down |
| ETHIF_LINK_STATE_ACTIVE | Link active |

### Supported Speeds

| Speed | Description |
|-------|-------------|
| ETHIF_SPEED_10MBPS | 10 Mbps |
| ETHIF_SPEED_100MBPS | 100 Mbps |
| ETHIF_SPEED_1GBPS | 1 Gbps |
| ETHIF_SPEED_2_5GBPS | 2.5 Gbps |
| ETHIF_SPEED_10GBPS | 10 Gbps |

## Dependencies

- **Eth** (MCAL) - Ethernet driver
- **EthTrcv** - Ethernet transceiver driver
- **SoAd** - Socket Adapter
- **DoIP** - Diagnostic over IP (optional)
- **SomeIp** - SOME/IP protocol (optional)
- **StbM** - Synchronized Time Base Manager (optional)
- **EcuM** - ECU Manager for wake-up

## Usage Example

```c
#include "EthIf.h"

void EthIf_Example(void)
{
    /* Initialize Ethernet Interface */
    EthIf_Init(&EthIf_Config);

    /* Initialize controller 0 */
    EthIf_ControllerInit(0, 0);

    /* Set controller to active mode */
    Std_ReturnType result = EthIf_SetControllerMode(0, ETHIF_MODE_ACTIVE);

    if (result == E_OK) {
        /* Get MAC address */
        uint8 macAddr[6];
        EthIf_GetPhysAddr(0, macAddr);

        /* Prepare and transmit frame */
        uint8 frameData[1500];
        /* Fill frame data... */

        result = EthIf_Transmit(0, 0x0800, frameData, 100);

        /* Enable timestamp for measurement */
        EthIf_EnableEgressTimeStamp(0, 0);
    }

    /* Main processing loop */
    while (1) {
        EthIf_MainFunction();
        /* Process other tasks */
    }
}
```

## Timestamp Usage Example

```c
void Timestamp_Example(void)
{
    EthIf_TimestampType timestamp;
    EthIf_TimestampQualityType quality;

    /* Get egress timestamp after transmission */
    EthIf_GetEgressTimeStamp(0, 0, &timestamp, &quality);

    if (quality == ETHIF_TIMESTAMP_VALID) {
        /* Use timestamp for synchronization */
        uint32 seconds = timestamp.seconds;
        uint32 nanoseconds = timestamp.nanoseconds;
    }
}
```

## Error Handling

### DET Error Codes

| Code | Description |
|------|-------------|
| ETHIF_E_INV_CTRL_IDX | Invalid controller index |
| ETHIF_E_INV_TRCV_IDX | Invalid transceiver index |
| ETHIF_E_INV_PARAM_POINTER | NULL pointer error |
| ETHIF_E_INV_MODE | Invalid mode |
| ETHIF_E_UNINIT | Module not initialized |
| ETHIF_E_INV_VLAN_IDX | Invalid VLAN index |
| ETHIF_E_INV_PHY_ADDRESS | Invalid MAC address |

## State Machine

### Controller State
```
UNINIT → DOWN → ACTIVE
   ↓       ↓
   └───────┘
```

## VLAN Support

EthIf supports VLAN tagging according to IEEE 802.1Q:

- VLAN ID range: 1-4094
- Priority code point (PCP): 3 bits (0-7)
- Drop eligible indicator (DEI): 1 bit

## Source Code

- `/home/admin/yuleASR/src/bsw/ecual/ethif/`
  - `include/EthIf.h` - Public API
  - `include/EthIf_Cfg.h` - Configuration
  - `src/EthIf.c` - Implementation
  - `src/EthIf_Lcfg.c` - Link-time configuration

## References

- AUTOSAR_SWS_EthernetInterface
- IEEE 802.3 - IEEE Standard for Ethernet
- IEEE 802.1Q - Virtual Bridged Local Area Networks
- IEEE 802.1AS - Timing and Synchronization for Time-Sensitive Applications
