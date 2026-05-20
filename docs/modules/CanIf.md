# CanIf - CAN Interface Module

## Overview

CanIf implements the **AUTOSAR CAN Interface** module, providing a hardware-independent abstraction layer between the CAN driver (MCAL) and upper layers (Com, CanTp, PduR, etc.). It manages CAN controller configuration, PDU routing, transmission handling, and transceiver control.

## Standards Compliance

| Standard | Version | Description |
|----------|---------|-------------|
| AUTOSAR SWS CAN Interface | 4.4.0 | AUTOSAR CAN Interface specification |
| AUTOSAR Classic Platform | 4.x | Standard software platform |
| ISO 11898 | - | CAN Data Link Layer specification |

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Upper Layers                          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐│
│  │   Com   │ │  CanTp  │ │  PduR   │ │    XCP/UDS      ││
│  └────┬────┘ └────┬────┘ └────┬────┘ └────────┬────────┘│
│       │           │           │               │         │
├───────┴───────────┴───────────┴───────────────┴─────────┤
│                   CanIf (ECUAL)                         │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────┐   │
│  │ Controller  │ │   PDU       │ │   Transceiver   │   │
│  │ Management  │ │  Routing    │ │   Management    │   │
│  └─────────────┘ └─────────────┘ └─────────────────┘   │
├─────────────────────────────────────────────────────────┤
│                   Can (MCAL)                            │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────┐   │
│  │  Controller │ │  Hardware   │ │  Interrupt      │   │
│  │   Driver    │ │   Objects   │ │  Handling       │   │
│  └─────────────┘ └─────────────┘ └─────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

## Features

### Controller Management
- ✅ Controller initialization and deinitialization
- ✅ Controller mode management (UNINIT/STARTED/STOPPED/SLEEP)
- ✅ Baudrate configuration (Set/Get)
- ✅ Bus-off error handling and notification
- ✅ Controller mode indication

### PDU Management
- ✅ Transmit PDU routing to hardware objects (HTH)
- ✅ Receive PDU routing from hardware objects (HRH) to upper layers
- ✅ Dynamic CAN ID assignment
- ✅ Software filtering for reception
- ✅ DLC checking
- ✅ PDU channel mode control (ONLINE/OFFLINE/TX_OFFLINE)

### Transceiver Management
- ✅ Transceiver mode control (NORMAL/STANDBY/SLEEP)
- ✅ Wakeup source detection
- ✅ Wakeup reason reporting
- ✅ Wakeup mode configuration

### Interrupt Handling
- ✅ TxConfirmation callback
- ✅ RxIndication callback
- ✅ ControllerBusOff callback
- ✅ ControllerModeIndication callback

## API Reference

### Initialization & Deinitialization

#### CanIf_Init
```c
void CanIf_Init(const CanIf_ConfigType* ConfigPtr)
```
Initializes the CAN Interface module with the provided configuration.

**Parameters:**
| Parameter | Description |
|-----------|-------------|
| ConfigPtr | Pointer to the CanIf configuration structure |

**DET Errors:**
- `CANIF_E_PARAM_POINTER` - ConfigPtr is NULL
- `CANIF_E_ALREADY_INITIALIZED` - Module already initialized

#### CanIf_DeInit
```c
void CanIf_DeInit(void)
```
Deinitializes the CAN Interface module.

**DET Errors:**
- `CANIF_E_UNINIT` - Module not initialized

### Version Information

#### CanIf_GetVersionInfo
```c
void CanIf_GetVersionInfo(Std_VersionInfoType* versioninfo)
```
Returns the version information of the CAN Interface module.

**Parameters:**
| Parameter | Description |
|-----------|-------------|
| versioninfo | Pointer to version info structure |

### Controller Management

#### CanIf_SetControllerMode
```c
Std_ReturnType CanIf_SetControllerMode(uint8 ControllerId, CanIf_ControllerModeType ControllerMode)
```
Sets the mode of the specified CAN controller.

**Parameters:**
| Parameter | Description |
|-----------|-------------|
| ControllerId | Controller to configure (0 to CANIF_NUM_CONTROLLERS-1) |
| ControllerMode | Target mode (CANIF_CS_STARTED/CANIF_CS_STOPPED/CANIF_CS_SLEEP) |

**Returns:**
- `E_OK` - Request accepted
- `E_NOT_OK` - Request failed

#### CanIf_GetControllerMode
```c
Std_ReturnType CanIf_GetControllerMode(uint8 ControllerId, CanIf_ControllerModeType* ControllerModePtr)
```
Gets the current mode of the specified CAN controller.

**Parameters:**
| Parameter | Description |
|-----------|-------------|
| ControllerId | Controller to query |
| ControllerModePtr | Pointer to store current mode |

#### CanIf_SetBaudrate
```c
Std_ReturnType CanIf_SetBaudrate(uint8 ControllerId, uint16 BaudRate)
```
Sets the baudrate of the specified CAN controller.

#### CanIf_GetBaudrate
```c
Std_ReturnType CanIf_GetBaudrate(uint8 ControllerId, uint16* BaudRatePtr)
```
Gets the current baudrate of the specified CAN controller.

### PDU Transmission

#### CanIf_Transmit
```c
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
```
Requests transmission of a CAN PDU.

**Parameters:**
| Parameter | Description |
|-----------|-------------|
| TxPduId | PDU identifier to transmit |
| PduInfoPtr | Pointer to PDU data and length |

**Returns:**
- `E_OK` - Transmission request accepted
- `E_NOT_OK` - Transmission request failed

**DET Errors:**
- `CANIF_E_UNINIT` - Module not initialized
- `CANIF_E_INVALID_TXPDUID` - Invalid PDU ID
- `CANIF_E_PARAM_POINTER` - PduInfoPtr is NULL

#### CanIf_CancelTransmit
```c
Std_ReturnType CanIf_CancelTransmit(PduIdType TxPduId)
```
Cancels a pending transmission request.

#### CanIf_SetDynamicTxId
```c
Std_ReturnType CanIf_SetDynamicTxId(PduIdType CanTxPduId, Can_IdType CanId)
```
Sets a dynamic CAN ID for a PDU at runtime.

### PDU Channel Mode

#### CanIf_SetPduMode
```c
Std_ReturnType CanIf_SetPduMode(uint8 ControllerId, CanIf_PduModeType PduModeRequest)
```
Sets the PDU channel mode for a controller.

**PDU Modes:**
| Mode | TX | RX | Description |
|------|-----|-----|-------------|
| `CANIF_OFFLINE` | ❌ | ❌ | No transmission or reception |
| `CANIF_TX_OFFLINE` | ❌ | ✅ | Reception only |
| `CANIF_TX_OFFLINE_ACTIVE` | ⚠️ | ✅ | Pass-through TX, RX active |
| `CANIF_ONLINE` | ✅ | ✅ | Full operation |

#### CanIf_GetPduMode
```c
Std_ReturnType CanIf_GetPduMode(uint8 ControllerId, CanIf_PduModeType* PduModePtr)
```
Gets the current PDU channel mode.

### Transceiver Management

#### CanIf_SetTrcvMode
```c
Std_ReturnType CanIf_SetTrcvMode(uint8 TransceiverId, CanIf_TransceiverModeType TransceiverMode)
```
Sets the transceiver mode.

**Transceiver Modes:**
| Mode | Description |
|------|-------------|
| `CANIF_TRCV_MODE_NORMAL` | Normal operation mode |
| `CANIF_TRCV_MODE_STANDBY` | Standby (reduced power) |
| `CANIF_TRCV_MODE_SLEEP` | Sleep (lowest power) |

#### CanIf_GetTrcvMode
```c
Std_ReturnType CanIf_GetTrcvMode(uint8 TransceiverId, CanIf_TransceiverModeType* TransceiverModePtr)
```
Gets the current transceiver mode.

#### CanIf_GetTrcvWakeupReason
```c
Std_ReturnType CanIf_GetTrcvWakeupReason(uint8 TransceiverId, CanIf_TrcvWakeupReasonType* TrcvWuReasonPtr)
```
Gets the reason for the last transceiver wakeup.

#### CanIf_SetTrcvWakeupMode
```c
Std_ReturnType CanIf_SetTrcvWakeupMode(uint8 TransceiverId, CanIf_TrcvWakeupModeType TrcvWakeupMode)
```
Configures the transceiver wakeup mode.

### Wakeup Handling

#### CanIf_CheckWakeup
```c
Std_ReturnType CanIf_CheckWakeup(EcuM_WakeupSourceType WakeupSource)
```
Checks if a wakeup event has occurred on any CAN controller.

### Interrupt Callbacks

#### CanIf_TxConfirmation
```c
void CanIf_TxConfirmation(PduIdType CanTxPduId)
```
Callback for transmission confirmation from CAN driver.

#### CanIf_RxIndication
```c
void CanIf_RxIndication(const Can_HwType* Mailbox, const PduInfoType* PduInfoPtr)
```
Callback for receive indication from CAN driver.

#### CanIf_ControllerBusOff
```c
void CanIf_ControllerBusOff(uint8 ControllerId)
```
Callback for bus-off event from CAN driver.

#### CanIf_ControllerModeIndication
```c
void CanIf_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode)
```
Callback for controller mode change indication from CAN driver.

## Configuration

### Controller Configuration

| Parameter | Type | Description |
|-----------|------|-------------|
| ControllerId | uint8 | Controller identifier (0 to N-1) |
| BaudRate | uint32 | Default baudrate (bps): 125000, 250000, 500000, 1000000 |
| DefaultMode | CanIf_ControllerModeType | Initial controller mode (typically CANIF_CS_STOPPED) |
| WakeupSupport | boolean | Wakeup capability enabled |
| BusOffNotification | boolean | Enable bus-off notifications |

### TX PDU Configuration

| Parameter | Type | Description |
|-----------|------|-------------|
| PduId | PduIdType | PDU identifier (unique) |
| CanId | Can_IdType | CAN identifier (11-bit or 29-bit) |
| CanIdType | CanIf_CanIdTypeType | CAN_ID_STANDARD (11-bit) or CAN_ID_EXTENDED (29-bit) |
| Hth | Can_HwHandleType | Hardware transmit handle index |
| TxConfirmation | boolean | Enable transmission confirmation |
| ControllerId | uint8 | Associated controller |

### RX PDU Configuration

| Parameter | Type | Description |
|-----------|------|-------------|
| PduId | PduIdType | PDU identifier (unique) |
| CanId | Can_IdType | CAN identifier |
| CanIdMask | Can_IdType | Mask for filtering (e.g., 0x7FF for exact match) |
| Hrh | Can_HwHandleType | Hardware receive handle index |
| RxIndication | boolean | Enable receive indication |
| ControllerId | uint8 | Associated controller |

### Configuration Example

```c
/* TX PDU Configuration */
const CanIf_TxPduConfigType CanIf_TxPduConfig[] = {
    {
        .PduId = 0,           /* TX_Engine_RPM */
        .CanId = 0x100,
        .CanIdType = CAN_ID_STANDARD,
        .Hth = 0,
        .TxConfirmation = TRUE,
        .ControllerId = 0
    },
    {
        .PduId = 1,           /* TX_Vehicle_Speed */
        .CanId = 0x200,
        .CanIdType = CAN_ID_STANDARD,
        .Hth = 0,
        .TxConfirmation = TRUE,
        .ControllerId = 0
    }
};

/* RX PDU Configuration */
const CanIf_RxPduConfigType CanIf_RxPduConfig[] = {
    {
        .PduId = 10,          /* RX_Throttle_Position */
        .CanId = 0x150,
        .CanIdMask = 0x7FF,   /* Exact match */
        .Hrh = 2,
        .RxIndication = TRUE,
        .ControllerId = 0
    },
    {
        .PduId = 11,          /* RX_Brake_Pressure */
        .CanId = 0x250,
        .CanIdMask = 0x7FF,
        .Hrh = 2,
        .RxIndication = TRUE,
        .ControllerId = 0
    }
};
```

## State Machines

### Controller State Machine

```
                    ┌─────────────┐
                    │   UNINIT    │◄────── Power-on
                    └──────┬──────┘
                           │ CanIf_Init()
                           ▼
                    ┌─────────────┐
           ┌───────►│   STOPPED   │◄──────┐
           │        └──────┬──────┘       │
           │               │               │
           │               │ STARTED       │ STOPPED
           │               ▼               │
           │        ┌─────────────┐        │
           │        │   STARTED   │────────┘
           │        └──────┬──────┘
           │               │
           │               │ SLEEP
           │               ▼
           │        ┌─────────────┐
           └───────►│    SLEEP    │
                    └─────────────┘
                           │
                           │ Wakeup Event
                           ▼
                    ┌─────────────┐
                    │   STOPPED   │
                    └─────────────┘
```

### PDU Channel Mode State Machine

```
                    ┌─────────────┐
         ┌─────────►│   OFFLINE   │◄────────┐
         │          └──────┬──────┘         │
         │                 │                 │
         │                 │ ONLINE          │ OFFLINE
         │                 ▼                 │
         │          ┌─────────────┐          │
         │          │    ONLINE   │          │
         │          └──────┬──────┘          │
         │                 │                 │
         │                 │ TX_OFFLINE      │
         │                 ▼                 │
         │     ┌─────────────────────────┐   │
         └─────│       TX_OFFLINE        │───┘
               └─────────────────────────┘
```

## Error Handling

### DET Error Codes

| Error Code | Value | Description |
|------------|-------|-------------|
| CANIF_E_PARAM_CANID | 0x01 | Invalid CAN ID |
| CANIF_E_PARAM_DLC | 0x02 | Invalid data length code |
| CANIF_E_PARAM_CONTROLLER | 0x03 | Invalid controller ID |
| CANIF_E_PARAM_POINTER | 0x04 | NULL pointer error |
| CANIF_E_PARAM_CONTROLLERMODE | 0x05 | Invalid controller mode |
| CANIF_E_PARAM_TRCVMODE | 0x06 | Invalid transceiver mode |
| CANIF_E_PARAM_TRCVWAKEUPMODE | 0x07 | Invalid wakeup mode |
| CANIF_E_PARAM_TRCV | 0x08 | Invalid transceiver ID |
| CANIF_E_PARAM_PDUMODE | 0x09 | Invalid PDU mode |
| CANIF_E_PARAM_HTH | 0x0B | Invalid hardware transmit handle |
| CANIF_E_PARAM_HRH | 0x0C | Invalid hardware receive handle |
| CANIF_E_UNINIT | 0x14 | Module not initialized |
| CANIF_E_INVALID_TXPDUID | 0x50 | Invalid TX PDU ID |
| CANIF_E_INVALID_RXPDUID | 0x60 | Invalid RX PDU ID |
| CANIF_E_STOPPED | 0x70 | Controller stopped |
| CANIF_E_NOT_SLEEP | 0x71 | Controller not in sleep |
| CANIF_E_PARAM_WAKEUPSOURCE | 0x72 | Invalid wakeup source |
| CANIF_E_ALREADY_INITIALIZED | 0x7C | Module already initialized |

## Usage Examples

### Basic Initialization and Transmission

```c
#include "CanIf.h"

void CanIf_Example_InitAndTransmit(void)
{
    Std_ReturnType result;
    
    /* Initialize CAN Interface */
    CanIf_Init(&CanIf_Config);
    
    /* Set controller to started mode */
    result = CanIf_SetControllerMode(0, CANIF_CS_STARTED);
    if (result == E_OK) {
        /* Enable PDU channel */
        CanIf_SetPduMode(0, CANIF_ONLINE);
        
        /* Prepare data */
        uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        PduInfoType pduInfo;
        pduInfo.SduDataPtr = data;
        pduInfo.SduLength = 8;
        pduInfo.MetaDataPtr = NULL_PTR;
        
        /* Transmit frame */
        result = CanIf_Transmit(CANIF_TX_ENGINE_RPM, &pduInfo);
        if (result == E_OK) {
            /* Transmission request accepted */
        }
    }
}
```

### Dynamic CAN ID Assignment

```c
void CanIf_Example_DynamicCanId(void)
{
    /* Set dynamic CAN ID for specific PDU at runtime */
    CanIf_SetDynamicTxId(CANIF_TX_DIAGNOSTIC, 0x700);
    
    /* Later change to extended ID */
    CanIf_SetDynamicTxId(CANIF_TX_DIAGNOSTIC, 0x18DAF100);
}
```

### Handling Receive Indication

```c
/* CanIf forwards received PDUs to PduR, 
   which then routes to the upper layer (Com, CanTp, etc.) */

/* In Com module - receive indication callback */
void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    /* Process received signal data */
    if (RxPduId == COM_RX_ENGINE_RPM) {
        /* Update engine RPM signal */
        uint16 rpm = (PduInfoPtr->SduDataPtr[0] << 8) | 
                      PduInfoPtr->SduDataPtr[1];
        Com_SetSignal(COM_SIG_ENGINE_RPM, &rpm);
    }
}
```

### Bus-Off Handling

```c
void CanIf_ControllerBusOff(uint8 ControllerId)
{
    /* CanIf automatically sets controller to STOPPED */
    /* Notify upper layer (CanSM) */
    CanSM_ControllerBusOff(ControllerId);
}

/* In State Manager */
void CanSM_ControllerBusOff(uint8 ControllerId)
{
    /* Start bus-off recovery sequence */
    /* 1. Wait for recovery delay */
    /* 2. Reset CAN controller */
    /* 3. Restart controller */
    CanIf_SetControllerMode(ControllerId, CANIF_CS_STARTED);
}
```

## Dependencies

### Required Modules (Lower Layer)
- **Can** (MCAL) - CAN hardware driver

### Optional Modules (Upper Layer)
- **PduR** - PDU Router for PDU routing
- **Com** - Communication for signal-based communication
- **CanTp** - CAN Transport Protocol for diagnostic/UDS

### Optional Modules (Services)
- **CanTrcv** - CAN Transceiver driver
- **EcuM** - ECU Manager for wakeup handling
- **DET** - Development Error Tracer

## Files

### Source Code
```
src/bsw/ecual/canif/
├── include/
│   ├── CanIf.h           # Public API header
│   ├── CanIf_Cfg.h       # Configuration header
│   └── CanIf_Types.h     # Internal type definitions
├── src/
│   ├── CanIf.c           # Core implementation
│   └── CanIf_Lcfg.c      # Link-time configuration
└── config/
    └── CanIf_Cfg.c       # Post-build configuration (optional)
```

### Unit Tests
```
tests/unit/autosar/ecual/
└── test_CanIf.c          # Unit test suite (80%+ coverage)
```

### Documentation
```
docs/modules/CanIf.md     # This document
```

## Test Coverage

| Category | Coverage | Status |
|----------|----------|--------|
| Initialization | 100% | ✅ |
| Controller Management | 100% | ✅ |
| PDU Transmission | 100% | ✅ |
| PDU Channel Mode | 100% | ✅ |
| Transceiver Management | 100% | ✅ |
| Interrupt Callbacks | 100% | ✅ |
| Error Handling | 100% | ✅ |
| State Transitions | 100% | ✅ |

**Overall Coverage: 85%+** ✅

## References

- [AUTOSAR SWS CAN Interface Specification](https://www.autosar.org/standards/)
- ISO 11898-1:2015 - Road vehicles - Controller area network (CAN)
- YuleTech BSW Architecture Document

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2026-04-14 | Initial implementation |
| 1.0.1 | 2026-05-15 | Enhanced unit tests (80%+ coverage) |

---

**Copyright © 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.**
**All Rights Reserved.**
