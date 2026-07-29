---
title: LIN
sidebar_label: LIN
description: "The LIN module provides low-level access to the LIN (Local Interconnect Network) hardware controller, enabling cost-effe"
sidebar_position: 13
---

# LIN (Local Interconnect Network) Driver

## Overview

The LIN module provides low-level access to the LIN (Local Interconnect Network) hardware controller, enabling cost-effective serial communication for automotive applications. LIN is primarily used for low-speed body electronics such as door locks, seats, mirrors, and lighting.

**AUTOSAR Standard**: Classic Platform 4.4.0  
**Layer**: MCAL (Microcontroller Driver)  
**Protocol**: LIN 2.2A, ISO 17987 (LIN)  
**Hardware**: NXP S32K3 (LPUART/LIN) / Infineon TC3xx (ASCLIN) / STM32 (USART/LIN)  
**ASIL Level**: QM to ASIL-B (configurable)

## Features

- **Master/Slave Support**: Supports both master and slave node operation
- **LIN 2.2A Compliant**: Classic checksum and enhanced checksum
- **Automatic Synchronization**: Automatic baud rate synchronization
- **Wakeup Support**: Sleep and wakeup functionality
- **Error Detection**: Frame error, parity error, timeout detection
- **Multi-channel**: Support for multiple LIN channels
- **DMA Support**: DMA-based data transfer for low CPU load
- **Diagnostic Support**: Supports diagnostic frames (0x3C/0x3D)

## Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│    (Door Control, Seat ECU, etc.)   │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│       LinIf (LIN Interface)         │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│          LIN Driver (MCAL)          │
│  ┌─────────────────────────────────────┐ │
│  │  Master Node                      │ │
│  │  ───────────────────────────────── │ │
│  │  Schedule Table Management          │ │
│  │  Header Generation (Break/Sync/PID) │ │
│  └─────────────────────────────────────┘ │
│  ┌─────────────────────────────────────┐ │
│  │  Slave Node                       │ │
│  │  ───────────────────────────────── │ │
│  │  PID Recognition                    │ │
│  │  Response Transmission/Reception    │ │
│  └─────────────────────────────────────┘ │
└─────────────────────────────────────┘
```

## LIN Frame Structure

```
+-----------+----------+--------+--------+----------+
|  Break    |  Sync    |  PID   |  Data  |  Checksum|
| (13+ bits)|  0x55    |(8 bits)|(0-8 B) |  (8 bits)|
+-----------+----------+--------+--------+----------+
   Master      Master    Master   Slave     Slave

PID = Protected Identifier (6-bit ID + 2-bit parity)
```

## API Reference

### Core Functions

```c
/* Initialization */
void Lin_Init(const Lin_ConfigType* Config);

/* Channel Control */
Std_ReturnType Lin_SendFrame(uint8 Channel, const Lin_PduType* PduInfoPtr);
Std_ReturnType Lin_SendResponse(uint8 Channel, const Lin_PduType* PduInfoPtr);
Std_ReturnType Lin_DisableResponse(uint8 Channel);

/* Wakeup */
Std_ReturnType Lin_WakeUp(uint8 Channel);
Std_ReturnType Lin_WakeUpInternal(uint8 Channel);

/* Status and Control */
Lin_StatusType Lin_GetStatus(uint8 Channel, uint8** Lin_SduPtr);
void Lin_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* Go to Sleep */
Std_ReturnType Lin_GoToSleep(uint8 Channel);
Std_ReturnType Lin_GoToSleepInternal(uint8 Channel);
```

### Data Types

```c
typedef uint8 Lin_ChannelType;      /* LIN channel number */
typedef uint8 Lin_FramePidType;     /* Protected identifier (0x00-0x3F) */
typedef uint8 Lin_FrameCsModelType; /* Checksum model */
typedef uint8 Lin_FrameResponseType;/* Response type */

typedef struct {
    Lin_FramePidType Pid;           /* Protected identifier */
    Lin_FrameCsModelType Cs;        /* Checksum model (Classic/Enhanced) */
    Lin_FrameResponseType Drc;      /* Direction (Tx/Rx) */
    uint8 Dl;                       /* Data length */
    uint8* SduPtr;                  /* Data pointer */
} Lin_PduType;

/* Status types */
typedef enum {
    LIN_NOT_OK = 0,
    LIN_TX_OK,
    LIN_TX_BUSY,
    LIN_TX_HEADER_ERROR,
    LIN_TX_ERROR,
    LIN_RX_OK,
    LIN_RX_BUSY,
    LIN_RX_ERROR,
    LIN_RX_NO_RESPONSE,
    LIN_OPERATIONAL,
    LIN_CH_SLEEP
} Lin_StatusType;

/* Frame checksum models */
typedef enum {
    LIN_CLASSIC_CS = 0,    /* Classic checksum (data only) */
    LIN_ENHANCED_CS        /* Enhanced checksum (PID + data) */
} Lin_FrameCsModelType;

/* Frame response direction */
typedef enum {
    LIN_FRAMERESPONSE_TX = 0,  /* Master Tx, Slave Rx */
    LIN_FRAMERESPONSE_RX,      /* Master Rx, Slave Tx */
    LIN_FRAMERESPONSE_IGNORE   /* No response */
} Lin_FrameResponseType;
```

## Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `LinChannelId` | uint8 | Unique channel identifier |
| `LinNodeType` | enum | Master/Slave node type |
| `LinChannelBaudRate` | uint16 | Baud rate (typically 19200 or 9600) |
| `LinChannelWakeupSupport` | boolean | Wakeup capability enabled |
| `LinChannelTransceiver` | ref | Transceiver reference |
| `LinFrameId` | uint8 | Frame identifier (0-59) |
| `LinFrameLength` | uint8 | Data length (1-8 bytes) |
| `LinFrameChecksumType` | enum | Classic or Enhanced checksum |
| `LinFrameDirection` | enum | Tx/Rx direction |

## Usage Example

### Master Node

```c
#include "Lin.h"
#include "Lin_Cfg.h"

/* LIN frame data */
static uint8 LinTxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

void Lin_MasterInit(void)
{
    /* Initialize LIN driver */
    Lin_Init(&Lin_Config);
}

void Lin_MasterSendFrame(void)
{
    Lin_PduType LinPdu;
    Std_ReturnType status;
    
    /* Prepare LIN PDU (Frame ID 0x10) */
    LinPdu.Pid = 0x10;                          /* Protected ID */
    LinPdu.Cs = LIN_ENHANCED_CS;                /* Enhanced checksum */
    LinPdu.Drc = LIN_FRAMERESPONSE_TX;          /* Master transmits response */
    LinPdu.Dl = 8;                              /* 8 bytes data */
    LinPdu.SduPtr = LinTxData;                  /* Data pointer */
    
    /* Send LIN frame */
    status = Lin_SendFrame(LIN_CHANNEL_0, &LinPdu);
    
    if (status == E_OK) {
        /* Frame transmission initiated */
    }
}

void Lin_MasterReceiveFrame(void)
{
    Lin_PduType LinPdu;
    static uint8 RxBuffer[8];
    
    /* Prepare for reception (Frame ID 0x20) */
    LinPdu.Pid = 0x20;
    LinPdu.Cs = LIN_ENHANCED_CS;
    LinPdu.Drc = LIN_FRAMERESPONSE_RX;          /* Receive response from slave */
    LinPdu.Dl = 8;
    LinPdu.SduPtr = RxBuffer;
    
    /* Send header, expect slave response */
    Lin_SendFrame(LIN_CHANNEL_0, &LinPdu);
}

void Lin_MasterWakeupSlaves(void)
{
    /* Send wakeup signal to wake up sleeping slaves */
    Lin_WakeUp(LIN_CHANNEL_0);
}

void Lin_MasterGoToSleep(void)
{
    /* Send go-to-sleep command */
    Lin_GoToSleep(LIN_CHANNEL_0);
}
```

### Slave Node

```c
#include "Lin.h"

static uint8 SlaveResponseData[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

void Lin_SlaveInit(void)
{
    Lin_Init(&Lin_Config);
}

void Lin_SlaveSendResponse(uint8 Pid)
{
    Lin_PduType LinPdu;
    
    if (Pid == 0x20) {  /* Frame ID configured for this slave */
        LinPdu.Pid = Pid;
        LinPdu.Cs = LIN_ENHANCED_CS;
        LinPdu.Drc = LIN_FRAMERESPONSE_TX;
        LinPdu.Dl = 8;
        LinPdu.SduPtr = SlaveResponseData;
        
        /* Send response when requested by master */
        Lin_SendResponse(LIN_CHANNEL_0, &LinPdu);
    }
}

void Lin_SlaveDisableResponse(void)
{
    /* Disable automatic response (for event-triggered frames) */
    Lin_DisableResponse(LIN_CHANNEL_0);
}
```

## Error Handling

| Error Code | Description | Detection |
|------------|-------------|-----------|
| `LIN_E_UNINIT` | Driver not initialized | API check |
| `LIN_E_INVALID_CHANNEL` | Invalid channel | Parameter validation |
| `LIN_E_INVALID_POINTER` | NULL pointer | Parameter validation |
| `LIN_E_STATE_TRANSITION` | Invalid state transition | State check |
| `LIN_E_PARAM_POINTER` | Invalid PDU pointer | API check |
| `LIN_E_NOK` | General error | Operation failure |

## Diagnostic Frames

LIN supports diagnostic frames (IDs 0x3C and 0x3D) for UDS over LIN:

```c
/* Master request frame (0x3C) */
void Lin_DiagnosticMasterRequest(void)
{
    Lin_PduType DiagPdu;
    uint8 DiagData[8] = {0x10, 0x03, 0x31, 0x01, 0xFF, 0x00, 0x00, 0x00};
    
    DiagPdu.Pid = 0x3C;  /* Diagnostic master request */
    DiagPdu.Cs = LIN_CLASSIC_CS;
    DiagPdu.Drc = LIN_FRAMERESPONSE_TX;
    DiagPdu.Dl = 8;
    DiagPdu.SduPtr = DiagData;
    
    Lin_SendFrame(LIN_CHANNEL_0, &DiagPdu);
}

/* Slave response frame (0x3D) */
void Lin_DiagnosticSlaveResponse(void)
{
    /* Slave automatically responds to 0x3C with 0x3D */
    /* Response data contains diagnostic information */
}
```

## Hardware Requirements

### Supported Controllers
- NXP S32K3xx (LPUART with LIN mode)
- Infineon AURIX TC3xx (ASCLIN)
- STM32H7 (USART with LIN mode)
- Renesas RH850/U2A (RLIN3)

### LIN Transceivers
- NXP TJA1021
- Infineon TLE7259
- Melexis TH8056
- Texas Instruments TLIN1021

### Resource Usage
| Resource | Typical Usage |
|----------|---------------|
| RAM | ~200-500 bytes per channel |
| ROM | ~5-10 KB |
| Interrupts | 1 per channel |
| Baud Rate | 9600 or 19200 bps |

## Dependencies

### Required Modules
- `Std_Types`, `Platform_Types`, `Compiler`
- `Det` - Error tracing
- `SchM_Lin` - Schedule manager
- `LinIf` - LIN interface (upper layer)

### Optional Modules
- `LinTrcv` - LIN transceiver driver
- `Dcm` - Diagnostic communication (UDS over LIN)
- `PduR` - PDU router (for diagnostic frames)

## References

- AUTOSAR SWS LIN Driver
- LIN Specification 2.2A
- ISO 17987 (Road vehicles - Local Interconnect Network)
- LIN Physical Layer Specification

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-01 | Initial LIN 2.2A support |
| 1.1.0 | 2024-06 | Added ISO 17987 compliance |
| 1.2.0 | 2024-10 | Enhanced error detection |
