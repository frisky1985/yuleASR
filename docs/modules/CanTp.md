# CanTp - CAN Transport Protocol Module

## Overview

CanTp implements the ISO 15765-2 CAN Transport Protocol for diagnostic and large data transmission over CAN bus. It segments messages larger than 8 bytes into multiple CAN frames and handles the flow control between sender and receiver.

For the detailed module design, see [CanTp Design Document](../design/modules/ecual/cantp-design.md).

## Standards

- ISO 15765-2:2016 - Road vehicles - Diagnostic communication over CAN
- AUTOSAR SWS CAN Transport Protocol
- AUTOSAR Classic Platform 4.4.0

## Features

### Frame Types

1. **Single Frame (SF)** - For messages ≤ 7 bytes
   - Direct transmission in one frame
   - PCI byte contains data length

2. **First Frame (FF)** - Initiates multi-frame transmission
   - Contains total message length (12-bit)
   - First 6 bytes of data

3. **Consecutive Frame (CF)** - Continues multi-frame transmission
   - Sequence number (0-15) for ordering
   - 7 bytes of data per frame

4. **Flow Control (FC)** - Receiver flow control
   - Continue To Send (CTS), Wait (WT), Overflow (OVFLW)
   - Block Size (BS) and Separation Time (STmin)

### Addressing Modes

| Mode | Description |
|------|-------------|
| Standard | Normal 11-bit addressing |
| Extended | Extended addressing with additional byte |
| Mixed 11-bit | Mixed addressing on 11-bit CAN ID |
| Mixed 29-bit | Mixed addressing on 29-bit CAN ID |
| Normal Fixed | Fixed addressing scheme |

## APIs

### Core APIs

| API | Function |
|-----|----------|
| `CanTp_Init()` | Initialize CanTp module |
| `CanTp_Shutdown()` | Shutdown CanTp module |
| `CanTp_Transmit()` | Request TP transmission |
| `CanTp_CancelTransmit()` | Cancel ongoing transmission |
| `CanTp_CancelReceive()` | Cancel ongoing reception |

### Parameter APIs

| API | Function |
|-----|----------|
| `CanTp_ChangeParameter()` | Change STmin or Block Size |
| `CanTp_ReadParameter()` | Read current parameters |

### Callback APIs

| API | Function |
|-----|----------|
| `CanTp_RxIndication()` | Reception callback from CanIf |
| `CanTp_TxConfirmation()` | Transmission confirmation from CanIf |
| `CanTp_MainFunction()` | Periodic processing (10ms typical) |

## Configuration

### Pre-compile

| Parameter | Description |
|-----------|-------------|
| `CANTP_VERSION_INFO_API` | Enable version info API |
| `CANTP_DEV_ERROR_DETECT` | Enable development error detection |
| `CANTP_MAX_CHANNEL_CNT` | Maximum number of channels |
| `CANTP_PADDING_BYTE_VALUE` | Padding byte value (0xCC typical) |

### Link-time

| Parameter | Description |
|-----------|-------------|
| `N_As` | Sender timeout for FF/SF transmission |
| `N_Bs` | Sender timeout for FC reception |
| `N_Cs` | Sender timeout for CF transmission |
| `N_Ar` | Receiver timeout for FC transmission |
| `N_Br` | Receiver timeout for CF reception |
| `N_Cr` | Receiver timeout between CF frames |

### Timeout Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| N_As | 1000ms | Transmission timeout |
| N_Bs | 1000ms | FC reception timeout |
| N_Cs | 1000ms | CF transmission timeout |
| N_Ar | 1000ms | FC transmission timeout |
| N_Br | 1000ms | Buffer handling timeout |
| N_Cr | 1000ms | CF reception timeout |

## State Machine

### TX Channel States
```
IDLE → TX_SF → IDLE (Single Frame)
IDLE → TX_FF → WAIT_FC → TX_CF → IDLE (Multi-frame)
```

### RX Channel States
```
IDLE → RX_SF → IDLE (Single Frame)
IDLE → RX_FF → SEND_FC → RX_CF → IDLE (Multi-frame)
```

## Dependencies

- **CanIf** - CAN Interface for frame transmission
- **PduR** - PDU Router for upper layer communication
- **DET** - Development Error Tracer (optional)

## Usage Example

```c
#include "CanTp.h"

void CanTp_Example(void)
{
    /* Initialize CanTp */
    CanTp_Init(&CanTp_Config);

    /* Transmit multi-frame diagnostic message */
    uint8 data[] = {0x10, 0x03, /* session control request */
                    0x22, 0xF1, 0x90, /* read data by identifier */
                    /* ... more data ... */};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = sizeof(data);

    Std_ReturnType result = CanTp_Transmit(CANTP_DIAG_TX_PDU_ID, &pduInfo);

    if (result == E_OK) {
        /* Transmission accepted */
        while (1) {
            CanTp_MainFunction();
            /* Process other tasks */
        }
    }
}
```

## Error Handling

### DET Error Codes

| Code | Description |
|------|-------------|
| CANTP_E_PARAM_CONFIG | Invalid configuration pointer |
| CANTP_E_PARAM_ID | Invalid PDU ID |
| CANTP_E_PARAM_POINTER | NULL pointer error |
| CANTP_E_UNINIT | Module not initialized |
| CANTP_E_INVALID_TX_ID | Invalid Tx NSDU ID |
| CANTP_E_INVALID_RX_ID | Invalid Rx NSDU ID |

### Runtime Error Codes

| Code | Description |
|------|-------------|
| CANTP_E_RX_TIMEOUT_CR | CF reception timeout |
| CANTP_E_TX_TIMEOUT_BS | FC reception timeout |
| CANTP_E_RX_INVALID_SN | Invalid sequence number |
| CANTP_E_RX_WFT_MAX | Wait frame counter exceeded |

## Source Code

- `/home/admin/yuleASR/src/bsw/ecual/cantp/`
  - `include/CanTp.h` - Public API
  - `include/CanTp_Cfg.h` - Configuration
  - `src/CanTp.c` - Implementation
  - `src/CanTp_Lcfg.c` - Link-time configuration

## References

- ISO 15765-2: Road vehicles - Diagnostic communication over CAN
- AUTOSAR SWS CAN Transport Protocol
