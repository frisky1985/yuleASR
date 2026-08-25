# CAN (Controller Area Network) Driver

## Overview

The CAN module provides low-level access to the CAN controller hardware, enabling communication over the Controller Area Network bus. It implements the CAN 2.0B protocol and supports both standard (11-bit) and extended (29-bit) identifiers.

**AUTOSAR Standard**: Classic Platform 4.4.0  
**Layer**: MCAL (Microcontroller Driver)  
**Protocol**: CAN 2.0A/B, ISO 11898  
**Hardware**: NXP S32K3 (FlexCAN) / Infineon TC3xx (MultiCAN+) / STM32 (bxCAN/FDCAN)  
**ASIL Level**: QM to ASIL-D (configurable)

## Features

- **Multi-Controller Support**: Supports multiple CAN controllers per ECU
- **Hardware Filtering**: Acceptance filters for message filtering
- **Full CAN 2.0B**: Standard (11-bit) and Extended (29-bit) identifiers
- **Flexible Data Rate**: CAN FD support (up to 8 Mbps data phase)
- **Interrupt/Driven**: Interrupt or polling-based operation
- **Multiple Mailboxes**: Configurable TX/RX mailboxes
- **Error Handling**: Bus-off detection and recovery
- **Wakeup Support**: Network wakeup capability
- **Loopback Mode**: Self-test and diagnostics

## Architecture

```
┌─────────────────────────────────────┐
│         Application Layer           │
│    (Diagnostic, Network Mgmt)       │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│           CAN Driver                  │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐   │
│  │  Ctrl 0  │ │  Ctrl 1  │ │  Ctrl 2  │   │
│  │─────────────│ │─────────────│ │─────────────│   │
│  │ TX MB    │ │ TX MB    │ │ TX MB    │   │
│  │ RX MB    │ │ RX MB    │ │ RX MB    │   │
│  │ Filters  │ │ Filters  │ │ Filters  │   │
│  └─────────────┘ └─────────────┘ └─────────────┘   │
│                  │                      │
│  ┌───────────────▼───────────────┐          │
│  │     CAN Transceiver Hardware        │          │
│  │   (TJA1101, TJA1043, etc.)          │          │
│  └───────────────────────────────┘          │
└─────────────────────────────────────┘
```

## API Reference

### Core Functions

```c
/* Initialization */
void Can_Init(const Can_ConfigType* Config);

/* Controller Mode Control */
Std_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition);

/* Transmission */
Std_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo);

/* Reception Handling */
void Can_MainFunction_Read(void);
void Can_MainFunction_Write(void);
void Can_MainFunction_BusOff(void);
void Can_MainFunction_Wakeup(void);
void Can_MainFunction_Mode(void);

/* Interrupt Control */
void Can_DisableControllerInterrupts(uint8 Controller);
void Can_EnableControllerInterrupts(uint8 Controller);

/* Wakeup Check */
Can_ReturnType Can_CheckWakeup(uint8 Controller);

/* Version Info */
void Can_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* Controllers Map */
uint8 Can_GetControllerIndex(uint8 Controller);
```

### Callback Functions

```c
/* Reception callback */
void CanIf_RxIndication(const Can_HwType* Mailbox, const PduInfoType* PduInfoPtr);

/* Transmission confirmation */
void CanIf_TxConfirmation(PduIdType CanTxPduId);

/* Controller mode indication */
void CanIf_ControllerModeIndication(uint8 ControllerId, Can_ControllerStateType ControllerMode);

/* Error notification */
void CanIf_ControllerBusOff(uint8 ControllerId);
```

### Data Types

```c
typedef uint8 Can_HwHandleType;          /* Hardware object handle */
typedef uint8 Can_ControllerStateType;   /* Controller state */
typedef uint32 Can_IdType;               /* CAN identifier (11/29-bit) */

typedef struct {
    Can_IdType id;                       /* CAN identifier */
    uint8 length;                        /* Data length (0-8 bytes, or 0-64 for CAN FD) */
    uint8* sdu;                          /* Data pointer */
    PduIdType swPduHandle;               /* PDU handle for confirmation */
} Can_PduType;

typedef struct {
    Can_IdType CanId;                    /* CAN identifier */
    Can_HwHandleType Hoh;                /* Hardware object handle */
    uint8 ControllerId;                  /* Controller ID */
} Can_HwType;

/* Controller states */
typedef enum {
    CAN_CS_UNINIT = 0,
    CAN_CS_STARTED,
    CAN_CS_STOPPED,
    CAN_CS_SLEEP
} Can_ControllerStateType;

/* Return types */
typedef enum {
    CAN_OK = 0,
    CAN_NOT_OK,
    CAN_BUSY
} Can_ReturnType;
```

## Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `CanControllerId` | uint8 | Unique controller identifier |
| `CanControllerBaseAddress` | uint32 | Hardware base address |
| `CanTxProcessing` | enum | Interrupt/Polling mode for TX |
| `CanRxProcessing` | enum | Interrupt/Polling mode for RX |
| `CanBusoffProcessing` | enum | Interrupt/Polling for bus-off |
| `CanWakeupProcessing` | enum | Interrupt/Polling for wakeup |
| `CanControllerDefaultBaudrate` | uint32 | Default baudrate (bps) |
| `CanControllerFdBaudrateConfig` | struct | CAN FD timing parameters |
| `CanHardwareObjectCount` | uint16 | Number of hardware objects |
| `CanHwObjectType` | enum | Transmit/Receive object |
| `CanIdType` | enum | Standard/Extended/Mixed |
| `CanObjectId` | uint16 | Hardware object ID |
| `CanFilterCode` | uint32 | Acceptance filter code |
| `CanFilterMask` | uint32 | Acceptance filter mask |

## Usage Example

```c
#include "Can.h"
#include "Can_Cfg.h"

/* CAN message buffer */
static uint8 CanTxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

void Can_InitExample(void)
{
    /* Initialize CAN driver */
    Can_Init(&Can_Config);
}

void Can_StartControllerExample(void)
{
    Std_ReturnType status;
    
    /* Start CAN controller 0 */
    status = Can_SetControllerMode(CAN_CONTROLLER_0, CAN_CS_STARTED);
    
    if (status == E_OK) {
        /* Controller started successfully */
        CanController0Active = TRUE;
    }
}

void Can_SendMessageExample(void)
{
    Can_PduType CanPdu;
    Std_ReturnType status;
    
    /* Prepare CAN PDU */
    CanPdu.id = 0x123;                     /* Standard CAN ID */
    CanPdu.length = 8;                     /* 8 bytes data */
    CanPdu.sdu = CanTxData;                /* Data pointer */
    CanPdu.swPduHandle = TX_PDU_ID;        /* PDU handle */
    
    /* Send message using hardware transmit handle 0 */
    status = Can_Write(CAN_HTH_0, &CanPdu);
    
    if (status == CAN_OK) {
        /* Transmission initiated successfully */
    } else if (status == CAN_BUSY) {
        /* Hardware buffer full, retry later */
    }
}

void Can_StopControllerExample(void)
{
    /* Stop CAN controller 0 */
    Can_SetControllerMode(CAN_CONTROLLER_0, CAN_CS_STOPPED);
    CanController0Active = FALSE;
}

/* Reception callback (called from ISR) */
void CanIf_RxIndication(const Can_HwType* Mailbox, const PduInfoType* PduInfoPtr)
{
    if (Mailbox->CanId == 0x456) {
        /* Process received message */
        memcpy(RxBuffer, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
        RxDataReady = TRUE;
    }
}

/* Transmission confirmation callback */
void CanIf_TxConfirmation(PduIdType CanTxPduId)
{
    if (CanTxPduId == TX_PDU_ID) {
        /* Message transmitted successfully */
        TxComplete = TRUE;
    }
}
```

## CAN FD Support
详细设计文档见 [Can 设计文档](../design/modules/mcal/can-design.md)。

The driver supports CAN Flexible Data-rate (CAN FD) with:
- **Data Phase**: Up to 8 Mbps (configurable)
- **Payload**: Up to 64 bytes
- **Bit Rate Switching**: Automatic switching between arbitration and data phase

```c
/* CAN FD message example */
void Can_SendCanFdMessage(void)
{
    Can_PduType CanPdu;
    uint8 CanFdData[64];  /* CAN FD supports up to 64 bytes */
    
    /* Fill data */
    for (int i = 0; i < 64; i++) {
        CanFdData[i] = i;
    }
    
    CanPdu.id = 0x100;           /* CAN ID */
    CanPdu.length = 64;          /* 64 bytes (CAN FD) */
    CanPdu.sdu = CanFdData;
    CanPdu.swPduHandle = TX_FD_PDU_ID;
    
    /* Send CAN FD message */
    Can_Write(CAN_FD_HTH_0, &CanPdu);
}
```

## Error Handling

| Error Code | Description | Detection |
|------------|-------------|-----------|
| `CAN_E_UNINIT` | Driver not initialized | API check |
| `CAN_E_PARAM_CONTROLLER` | Invalid controller ID | Parameter validation |
| `CAN_E_PARAM_DLC` | Invalid data length | TX validation |
| `CAN_E_PARAM_POINTER` | NULL pointer | Parameter validation |
| `CAN_E_TRANSITION` | Invalid mode transition | State check |
| `CAN_E_DATALOST` | Message lost (overwrite) | Reception |

## Bus-Off Recovery

```c
void Can_MainFunction_BusOff(void)
{
    /* Called periodically by SchM */
    /* Handles automatic bus-off recovery */
}
```

The driver automatically recovers from bus-off state:
1. Detection of bus-off condition
2. Wait for 128 occurrences of 11 consecutive recessive bits
3. Transition to STOPPED state
4. Notify upper layer via `CanIf_ControllerBusOff()`

## Hardware Requirements

### Supported Controllers
- NXP S32K3xx (FlexCAN with CAN FD)
- Infineon AURIX TC3xx (MultiCAN+ with CAN FD)
- STM32H7 (FDCAN)
- Renesas RH850/U2A (RS-CANFD)

### Resource Usage
| Resource | Typical Usage |
|----------|---------------|
| RAM | ~500-2000 bytes (controller dependent) |
| ROM | ~10-20 KB |
| Interrupts | 1-4 per controller |
| Hardware Mailboxes | 16-128 (configurable) |

## Dependencies

### Required Modules
- `Std_Types`, `Platform_Types`, `Compiler`
- `Det` - Error tracing
- `SchM_Can` - Schedule manager callbacks
- `CanIf` - Upper layer interface

### Optional Modules
- `EcuM` - Wakeup handling
- `Mcu` - Clock configuration
- `Port` - Pin configuration
- `Irq` - Interrupt routing

## References

- AUTOSAR SWS CAN Driver
- ISO 11898-1 (CAN data link layer)
- ISO 11898-2 (CAN physical layer)
- ISO 11898-2:2016 (CAN FD)
- Bosch CAN Specification 2.0B

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-01 | Initial CAN 2.0B support |
| 1.1.0 | 2024-06 | Added CAN FD support |
| 1.2.0 | 2024-10 | Multi-core support |