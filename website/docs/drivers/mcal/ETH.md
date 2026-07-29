---
title: ETH
sidebar_label: ETH
description: "The ETH module provides low-level access to the Ethernet MAC controller hardware, enabling high-speed data communication"
sidebar_position: 6
---

# ETH (Ethernet) Driver

## Overview

The ETH module provides low-level access to the Ethernet MAC controller hardware, enabling high-speed data communication over IEEE 802.3 networks. It supports various Ethernet speeds (10/100/1000 Mbps) and advanced features like VLAN tagging, checksum offloading, and time stamping.

**AUTOSAR Standard**: Classic Platform 4.4.0  
**Layer**: MCAL (Microcontroller Driver)  
**Protocol**: IEEE 802.3 (Ethernet II), IEEE 802.1Q (VLAN)  
**Hardware**: NXP S32K3 (ENET) / Infineon TC3xx (GETH) / STM32 (ETH)  
**ASIL Level**: QM to ASIL-D (configurable)

## Features

- **Multi-speed Support**: 10/100/1000 Mbps (depending on hardware)
- **Full/Half Duplex**: Configurable duplex mode
- **DMA-based Transfer**: Efficient data transfer using DMA
- **Checksum Offload**: Hardware TCP/UDP/IP checksum calculation
- **VLAN Support**: IEEE 802.1Q VLAN tagging
- **Time Stamping**: IEEE 1588 PTP time stamping (gPTP/TSN)
- **Wake-on-LAN**: Remote wakeup capability
- **Multicast Filtering**: Hardware multicast hash filtering
- **Promiscuous Mode**: Debug and monitoring support

## Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│    (DDS, SOME/IP, DoIP, TCP/IP)     │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│         TCP/IP Stack (optional)     │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│       EthIf (Ethernet Interface)    │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│          ETH Driver (MCAL)          │
│  ┌─────────────────────────────────────┐ │
│  │  MAC Controller (ENET/GETH)    │ │
│  │  ───────────────────────────────── │ │
│  │  TX DMA Descriptor Ring         │ │
│  │  RX DMA Descriptor Ring         │ │
│  │  Time Stamp Unit (PTP)          │ │
│  └─────────────────────────────────────┘ │
│                  │                      │
│  ┌───────────────▼───────────────┐          │
│  │      Ethernet PHY (TJA1101/DP83848) │          │
│  └───────────────────────────────┘          │
└─────────────────────────────────────┘
```

## API Reference

### Core Functions

```c
/* Initialization */
void Eth_Init(const Eth_ConfigType* CfgPtr);

/* Controller Control */
Std_ReturnType Eth_SetControllerMode(uint8 CtrlIdx, Eth_ModeType CtrlMode);
Eth_ModeType Eth_GetControllerMode(uint8 CtrlIdx);

/* Transmission */
BufReq_ReturnType Eth_Transmit(uint8 CtrlIdx, uint8* BufIdx, 
                               Eth_FrameType* FrameType, 
                               boolean TxConfirmation, 
                               uint16 LenByte, 
                               uint8* PhysAddrPtr);

/* Reception */
void Eth_Receive(uint8 CtrlIdx, uint8* BufIdx, Eth_DataType** DataPtr);
void Eth_ReleaseRxBuffer(uint8 CtrlIdx, uint8 BufIdx);

/* Buffer Management */
Std_ReturnType Eth_ProvideTxBuffer(uint8 CtrlIdx, uint16 LenByte, 
                                   uint8* BufIdx, uint8** BufPtr);

/* PHY Management */
Std_ReturnType Eth_ReadMii(uint8 CtrlIdx, uint8 TrcvIdx, 
                           uint8 RegIdx, uint16* RegValPtr);
Std_ReturnType Eth_WriteMii(uint8 CtrlIdx, uint8 TrcvIdx, 
                            uint8 RegIdx, uint16 RegVal);

/* MAC Address */
void Eth_UpdatePhysAddrFilter(uint8 CtrlIdx, uint8* PhysAddrPtr, 
                              Eth_FilterActionType Action);

/* Wakeup */
void Eth_SetWakeUpMode(uint8 CtrlIdx, boolean Mode);

/* Version */
void Eth_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* Main Functions */
void Eth_MainFunction(void);
```

### Callback Functions

```c
/* Transmission confirmation */
void EthIf_TxConfirmation(uint8 CtrlIdx, uint8 BufIdx, Std_ReturnType Result);

/* Reception indication */
void EthIf_RxIndication(uint8 CtrlIdx, uint8 BufIdx, 
                        Eth_FrameType FrameType, 
                        boolean IsBroadcast, 
                        uint8* PhysAddrPtr, 
                        uint8* DataPtr, 
                        uint16 LenByte);

/* Mode change indication */
void EthIf_CtrlModeIndication(uint8 CtrlIdx, Eth_ModeType CtrlMode);
```

### Data Types

```c
/* Ethernet frame types */
typedef uint16 Eth_FrameType;
#define ETH_FRAMETYPE_IPV4    0x0800U
#define ETH_FRAMETYPE_ARP     0x0806U
#define ETH_FRAMETYPE_IPV6    0x86DDU
#define ETH_FRAMETYPE_VLAN    0x8100U
#define ETH_FRAMETYPE_DDS     0x88E4U  /* DDS-RTPS over Ethernet */

/* Controller modes */
typedef enum {
    ETH_MODE_DOWN = 0,
    ETH_MODE_ACTIVE
} Eth_ModeType;

/* Buffer request return type */
typedef enum {
    BUFREQ_OK = 0,
    BUFREQ_E_NOT_OK,
    BUFREQ_E_BUSY,
    BUFREQ_E_OVFL
} BufReq_ReturnType;

/* Filter actions */
typedef enum {
    ETH_ADD_TO_FILTER = 0,
    ETH_REMOVE_FROM_FILTER
} Eth_FilterActionType;

/* MAC address type */
typedef uint8 Eth_DataType;
#define ETH_PHYS_ADDR_LEN     6U
```

## Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `EthCtrlIdx` | uint8 | Controller index (0-based) |
| `EthCtrlConfig` | struct | Controller configuration |
| `EthCtrlEnableMii` | boolean | Enable MII interface |
| `EthCtrlEnableRxInterrupt` | boolean | RX interrupt enable |
| `EthCtrlEnableTxInterrupt` | boolean | TX interrupt enable |
| `EthCtrlPhyAddress` | uint8 | PHY address (0-31) |
| `EthCtrlPhyInterface` | enum | MII/RMII/RGMII/GMII |
| `EthCtrlSpeed` | enum | 10/100/1000 Mbps |
| `EthCtrlDuplexMode` | enum | Half/Full duplex |
| `EthRxBufLen` | uint16 | RX buffer length |
| `EthTxBufLen` | uint16 | TX buffer length |
| `EthRxBufTotal` | uint8 | Number of RX buffers |
| `EthTxBufTotal` | uint8 | Number of TX buffers |
| `EthEnableTimeStamp` | boolean | Enable PTP time stamping |
| `EthEnableChecksumOffload` | boolean | Enable checksum offload |

## Usage Example

```c
#include "Eth.h"
#include "Eth_Cfg.h"

/* MAC address */
static uint8 MyMacAddress[ETH_PHYS_ADDR_LEN] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

void Eth_InitExample(void)
{
    /* Initialize Ethernet driver */
    Eth_Init(&Eth_Config);
    
    /* Update MAC address filter */
    Eth_UpdatePhysAddrFilter(ETH_CONTROLLER_0, MyMacAddress, ETH_ADD_TO_FILTER);
}

void Eth_StartControllerExample(void)
{
    /* Set controller to active mode */
    Eth_SetControllerMode(ETH_CONTROLLER_0, ETH_MODE_ACTIVE);
}

void Eth_SendFrameExample(void)
{
    uint8 bufIdx;
    uint8* bufPtr;
    Std_ReturnType status;
    
    /* Request TX buffer (1514 bytes for standard frame) */
    status = Eth_ProvideTxBuffer(ETH_CONTROLLER_0, 1514, &bufIdx, &bufPtr);
    
    if (status == E_OK) {
        /* Fill Ethernet frame */
        /* Destination MAC: 6 bytes */
        bufPtr[0] = 0xFF;  /* Broadcast */
        bufPtr[1] = 0xFF;
        bufPtr[2] = 0xFF;
        bufPtr[3] = 0xFF;
        bufPtr[4] = 0xFF;
        bufPtr[5] = 0xFF;
        
        /* Source MAC: 6 bytes */
        memcpy(&bufPtr[6], MyMacAddress, 6);
        
        /* EtherType: 2 bytes (IPv4) */
        bufPtr[12] = 0x08;
        bufPtr[13] = 0x00;
        
        /* IP Packet data */
        /* ... fill payload ... */
        
        /* Transmit frame */
        Eth_Transmit(ETH_CONTROLLER_0, bufIdx, ETH_FRAMETYPE_IPV4, 
                     TRUE, 1514, MyMacAddress);
    }
}

void Eth_ReceiveFrameExample(void)
{
    uint8 bufIdx;
    Eth_DataType* dataPtr;
    
    /* Check for received frames */
    Eth_Receive(ETH_CONTROLLER_0, &bufIdx, &dataPtr);
    
    if (dataPtr != NULL) {
        /* Process received frame */
        /* dataPtr points to frame including MAC header */
        uint8* dstMac = (uint8*)dataPtr;
        uint8* srcMac = (uint8*)dataPtr + 6;
        uint16 etherType = (((uint16)dataPtr[12]) << 8) | dataPtr[13];
        
        if (etherType == ETH_FRAMETYPE_IPV4) {
            /* Process IPv4 packet */
            ProcessIPv4Packet(dataPtr + 14);
        }
        
        /* Release RX buffer */
        Eth_ReleaseRxBuffer(ETH_CONTROLLER_0, bufIdx);
    }
}

/* TX confirmation callback */
void EthIf_TxConfirmation(uint8 CtrlIdx, uint8 BufIdx, Std_ReturnType Result)
{
    if (Result == E_OK) {
        /* Transmission successful */
        TxComplete = TRUE;
    }
}

/* RX indication callback */
void EthIf_RxIndication(uint8 CtrlIdx, uint8 BufIdx, 
                        Eth_FrameType FrameType, 
                        boolean IsBroadcast, 
                        uint8* PhysAddrPtr, 
                        uint8* DataPtr, 
                        uint16 LenByte)
{
    /* Frame received */
    if (FrameType == ETH_FRAMETYPE_IPV4) {
        ProcessIPv4Packet(DataPtr);
    }
}
```

## Time Stamping (PTP/gPTP)

For TSN (Time-Sensitive Networking) applications:

```c
/* Enable time stamping */
void Eth_EnableTimeStamp(uint8 CtrlIdx)
{
    /* Configure PTP hardware clock */
    /* Enable TX/RX time stamp capture */
}

/* Get TX time stamp */
void Eth_GetTxTimeStamp(uint8 CtrlIdx, uint8 BufIdx, Eth_TimeStampType* TimeStampPtr)
{
    /* Read time stamp from DMA descriptor */
}

/* Get RX time stamp */
void Eth_GetRxTimeStamp(uint8 CtrlIdx, uint8 BufIdx, Eth_TimeStampType* TimeStampPtr)
{
    /* Read time stamp from DMA descriptor */
}
```

## Error Handling

| Error Code | Description | Detection |
|------------|-------------|-----------|
| `ETH_E_UNINIT` | Driver not initialized | API check |
| `ETH_E_PARAM_CONTROLLER` | Invalid controller index | Parameter validation |
| `ETH_E_PARAM_POINTER` | NULL pointer | Parameter validation |
| `ETH_E_PARAM_LENGTH` | Invalid length | Parameter validation |
| `ETH_E_INV_MODE` | Invalid mode transition | State check |
| `ETH_E_FRAMES_LOST` | RX frame lost (overflow) | Reception |

## Hardware Requirements

### Supported Controllers
- NXP S32K3xx (ENET with IEEE 1588)
- Infineon AURIX TC3xx (GETH with PTP)
- STM32H7 (ETH with PTP)
- Renesas RH850/U2A (EtherMAC)

### Resource Usage
| Resource | Typical Usage |
|----------|---------------|
| RAM | ~50-100 KB (buffers) |
| ROM | ~15-25 KB |
| Interrupts | 2-3 (TX/RX/Error) |
| DMA Channels | 2 (TX and RX) |
| PHY Interface | MII/RMII/RGMII |

## Dependencies

### Required Modules
- `Std_Types`, `Platform_Types`, `Compiler`
- `Det` - Error tracing
- `SchM_Eth` - Schedule manager
- `EthIf` - Upper layer interface

### Optional Modules
- `EthTrcv` - Ethernet transceiver driver
- `TcpIp` - TCP/IP stack
- `SoAd` - Socket adapter
- `DDS` - Data Distribution Service

## References

- AUTOSAR SWS Ethernet Driver
- IEEE 802.3 (Ethernet)
- IEEE 802.1Q (VLAN)
- IEEE 1588 (PTP)
- IEEE 802.1AS (gPTP for TSN)
- IEEE 1722 (AVB Transport Protocol)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-01 | Initial release |
| 1.1.0 | 2024-05 | Added PTP time stamping |
| 1.2.0 | 2024-09 | Added VLAN support |
| 1.3.0 | 2024-12 | TSN/gPTP support |
