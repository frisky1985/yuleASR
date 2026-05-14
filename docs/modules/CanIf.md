# CanIf - CAN Interface Module

## Overview

CanIf implements the AUTOSAR CAN Interface, providing a hardware-independent abstraction between the CAN driver (MCAL) and upper layers. It manages CAN controller configuration, PDU routing, and transceiver control.

## Standards

- AUTOSAR SWS CAN Interface
- AUTOSAR Classic Platform 4.4.0
- ISO 11898 - CAN Data Link Layer

## Features

### Controller Management

- Controller initialization and deinitialization
- Controller mode management (STARTED/STOPPED/SLEEP)
- Baudrate configuration
- Bus-off error handling

### PDU Management

- Transmit PDU routing to hardware objects
- Receive PDU routing to upper layers
- Dynamic CAN ID assignment
- Software filtering for reception

### Transceiver Management

- Transceiver mode control (NORMAL/STANDBY/SLEEP)
- Wakeup source detection
- Wakeup mode configuration

## APIs

### Initialization APIs

| API | Function |
|-----|----------|
| `CanIf_Init()` | Initialize CAN Interface |
| `CanIf_DeInit()` | Deinitialize CAN Interface |
| `CanIf_GetVersionInfo()` | Get version information |

### Controller APIs

| API | Function |
|-----|----------|
| `CanIf_SetControllerMode()` | Set controller mode |
| `CanIf_GetControllerMode()` | Get current controller mode |
| `CanIf_SetBaudrate()` | Set controller baudrate |
| `CanIf_GetBaudrate()` | Get current baudrate |

### Transmission APIs

| API | Function |
|-----|----------|
| `CanIf_Transmit()` | Transmit CAN PDU |
| `CanIf_CancelTransmit()` | Cancel transmit request |
| `CanIf_SetDynamicTxId()` | Set dynamic TX CAN ID |

### PDU Mode APIs

| API | Function |
|-----|----------|
| `CanIf_SetPduMode()` | Set PDU channel mode |
| `CanIf_GetPduMode()` | Get PDU channel mode |

### Transceiver APIs

| API | Function |
|-----|----------|
| `CanIf_SetTrcvMode()` | Set transceiver mode |
| `CanIf_GetTrcvMode()` | Get transceiver mode |
| `CanIf_GetTrcvWakeupReason()` | Get wakeup reason |
| `CanIf_SetTrcvWakeupMode()` | Configure wakeup mode |

### Wakeup APIs

| API | Function |
|-----|----------|
| `CanIf_CheckWakeup()` | Check for wakeup events |

## Configuration

### Controller Configuration

| Parameter | Description |
|-----------|-------------|
| ControllerId | Controller identifier |
| BaudRate | Default baudrate (bps) |
| DefaultMode | Initial controller mode |
| WakeupSupport | Wakeup capability enabled |

### PDU Configuration

#### TX PDU
| Parameter | Description |
|-----------|-------------|
| PduId | PDU identifier |
| CanId | CAN identifier |
| CanIdType | Standard (11-bit) or Extended (29-bit) |
| Hth | Hardware transmit handle |
| TxConfirmation | Tx confirmation enabled |

#### RX PDU
| Parameter | Description |
|-----------|-------------|
| PduId | PDU identifier |
| CanId | CAN identifier |
| CanIdMask | Mask for filtering |
| Hrh | Hardware receive handle |
| RxIndication | Rx indication enabled |

### Controller Modes

| Mode | Description |
|------|-------------|
| CANIF_CS_UNINIT | Uninitialized state |
| CANIF_CS_SLEEP | Sleep mode (low power) |
| CANIF_CS_STARTED | Active communication |
| CANIF_CS_STOPPED | Stopped (no communication) |

### PDU Modes

| Mode | Description |
|------|-------------|
| CANIF_OFFLINE | No TX/RX |
| CANIF_TX_OFFLINE | No TX, RX active |
| CANIF_TX_OFFLINE_ACTIVE | No TX, TX confirmation active |
| CANIF_ONLINE | Full TX/RX active |

### Transceiver Modes

| Mode | Description |
|------|-------------|
| CANIF_TRCV_MODE_NORMAL | Normal operation |
| CANIF_TRCV_MODE_STANDBY | Standby mode |
| CANIF_TRCV_MODE_SLEEP | Sleep mode |

## Dependencies

- **Can** (MCAL) - CAN hardware driver
- **PduR** - PDU Router for PDU handling
- **CanTrcv** - CAN Transceiver driver (optional)
- **EcuM** - ECU Manager for wakeup handling
- **DET** - Development Error Tracer (optional)

## Usage Example

```c
#include "CanIf.h"

void CanIf_Example(void)
{
    /* Initialize CAN Interface */
    CanIf_Init(&CanIf_Config);

    /* Set controller to started mode */
    Std_ReturnType result = CanIf_SetControllerMode(0, CANIF_CS_STARTED);

    if (result == E_OK) {
        /* Set PDU mode to online */
        CanIf_SetPduMode(0, CANIF_ONLINE);

        /* Transmit a frame */
        uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        PduInfoType pduInfo;
        pduInfo.SduDataPtr = data;
        pduInfo.SduLength = 8;

        CanIf_Transmit(CANIF_TX_PDU_ID, &pduInfo);
    }

    /* Set dynamic CAN ID for specific PDU */
    CanIf_SetDynamicTxId(CANIF_DYNAMIC_TX_PDU, 0x700);
}
```

## Error Handling

### DET Error Codes

| Code | Description |
|------|-------------|
| CANIF_E_PARAM_CANID | Invalid CAN ID |
| CANIF_E_PARAM_DLC | Invalid data length |
| CANIF_E_PARAM_CONTROLLER | Invalid controller |
| CANIF_E_PARAM_POINTER | NULL pointer error |
| CANIF_E_UNINIT | Module not initialized |
| CANIF_E_INVALID_TXPDUID | Invalid TX PDU ID |
| CANIF_E_INVALID_RXPDUID | Invalid RX PDU ID |
| CANIF_E_STOPPED | Controller stopped |

## State Machine

### Controller State
```
UNINIT → STOPPED → STARTED
   ↓        ↓         ↓
   └────────┴─────────┴── SLEEP
```

## Source Code

- `/home/admin/yuleASR/src/bsw/ecual/canif/`
  - `include/CanIf.h` - Public API
  - `include/CanIf_Cfg.h` - Configuration
  - `src/CanIf.c` - Implementation
  - `src/CanIf_Lcfg.c` - Link-time configuration

## References

- AUTOSAR_SWS_CANInterface
- ISO 11898-1:2015 - Road vehicles - Controller area network (CAN)
