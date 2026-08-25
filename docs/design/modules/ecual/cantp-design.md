> **Module ID**: 0x3D  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_CANTransportProtocol  
> **Source Path**: `src/bsw/ecual/cantp/`  
> **Reference Document**: `docs/modules/CanTp.md`  
> **Doc Version**: 1.0  
> **Status**: Approved

# CanTp (CAN Transport Protocol) Design Document

## 1. Module Overview

CanTp implements the ISO 15765-2 CAN Transport Protocol for the AUTOSAR ECU Abstraction Layer (ECUAL). It segments upper-layer PDUs that exceed the 8-byte CAN payload into a sequence of CAN frames and reassembles received segmented frames back into a single PDU. CanTp sits between the PDU Router (`PduR`) and the CAN Interface (`CanIf`), and is primarily used for diagnostic and large-data communication.

### 1.1 Position in the Stack

```
┌─────────────────────────────────────────────────────────┐
│  PduR / Dcm / Com (Upper Layers)                        │
├─────────────────────────────────────────────────────────┤
│  CanTp (ECUAL)                                          │
├─────────────────────────────────────────────────────────┤
│  CanIf (ECUAL)                                          │
├─────────────────────────────────────────────────────────┤
│  Can (MCAL)                                             │
└─────────────────────────────────────────────────────────┘
```

---

## 2. Standards & Dependencies

### 2.1 Standards

| Standard | Version | Description |
|----------|---------|-------------|
| AUTOSAR SWS CAN Transport Protocol | 4.4.0 | AUTOSAR CanTp specification |
| ISO 15765-2 | 2016 | Road vehicles — Diagnostic communication over CAN |

### 2.2 Dependencies

| Module | Direction | Purpose |
|--------|-----------|---------|
| PduR | Upper | Receives transmit requests and delivers reassembled SDUs; forwards Tx confirmations |
| CanIf | Lower | Sends and receives individual CAN frames |
| Det | Common | Development error detection (optional) |

---

## 3. Architecture Design

### 3.1 Internal Components

| Component | Responsibility |
|-----------|----------------|
| Channel Manager | Allocates/resets channels, maps SDU IDs to NSDU configuration |
| Tx Segmenter | Splits outgoing SDUs into Single Frame, First Frame, or Consecutive Frames |
| Rx Reassembler | Receives SF/FF/CF, validates sequence numbers, and reassembles the SDU |
| Flow-Control Handler | Sends/receives Flow Control frames with CTS/WT/OVFLW status, BS, and STmin |
| Timeout Manager | Decrements per-channel timers in `CanTp_MainFunction` and aborts on timeout |
| Parameter Manager | Supports runtime read/write of `TP_STMIN` and `TP_BS` (when enabled) |

### 3.2 File Structure

```
src/bsw/ecual/cantp/
├── include/
│   ├── CanTp.h           # Public API, types, SIDs, error codes
│   └── CanTp_Cfg.h       # Pre-compile configuration
└── src/
    ├── CanTp.c           # Core implementation
    └── CanTp_Lcfg.c      # Link-time NSDU/channel/general configuration
```

---

## 4. State Machines

### 4.1 Channel State Definitions

```c
typedef enum {
    CANTP_CH_IDLE = 0,
    CANTP_CH_TX_SF,         /* Transmitting Single Frame */
    CANTP_CH_TX_FF,         /* Transmitting First Frame */
    CANTP_CH_TX_CF,         /* Transmitting Consecutive Frames */
    CANTP_CH_RX_SF,         /* Receiving Single Frame */
    CANTP_CH_RX_FF,         /* Receiving First Frame */
    CANTP_CH_RX_CF,         /* Receiving Consecutive Frames */
    CANTP_CH_TX_WAIT_FC,    /* Waiting for Flow Control */
    CANTP_CH_RX_WAIT_FC     /* Waiting to send Flow Control */
} CanTp_ChannelStateType;
```

### 4.2 Transmit Channel State Machine

```
IDLE ──(len ≤ 7)──► TX_SF ──(TxConfirmation)──► IDLE

IDLE ──(len > 7)──► TX_FF ──(TxConfirmation)──► TX_WAIT_FC
                            │
              (FC CTS/WT/OVFLW)
                            ▼
            TX_WAIT_FC ──CTS──► TX_CF ──(last CF)──► IDLE
                              │
                              └──(more data)──► TX_CF
```

- `TX_SF`: single-frame transmission in progress, guarded by `N_As`.
- `TX_FF`: first frame sent, waiting for `CanIf` confirmation.
- `TX_WAIT_FC`: waiting for receiver Flow Control, guarded by `N_Bs`.
- `TX_CF`: sending consecutive frames, guarded by `N_Cs` and receiver `STmin`/`BS`.

### 4.3 Receive Channel State Machine

```
IDLE ──(SF received)──► RX_SF ──(forward to PduR)──► IDLE

IDLE ──(FF received)──► RX_FF ──(send FC.CTS)──► RX_CF
                              │
              (next CF with expected SN)
                              ▼
            RX_CF ──(complete)──► IDLE
```

- `RX_SF`: single-frame reception, immediately forwarded to `PduR`.
- `RX_FF`: first frame received, Flow Control sent back.
- `RX_CF`: receiving consecutive frames, guarded by `N_Cr`.

---

## 5. Core Data Structures

### 5.1 Channel Runtime State

```c
typedef struct {
    CanTp_ChannelStateType State;
    PduIdType ActiveNsduId;
    uint16 DataLength;
    uint16 DataIndex;
    uint8 SequenceNumber;
    uint8 BlockSize;
    uint8 STmin;
    uint8 WftCounter;
    uint16 Timer;
    uint8 Buffer[CANTP_CHANNEL_BUFFER_SIZE];
    boolean TxConfirmed;
    boolean RxIndicated;
    const CanTp_TxNsduConfigType* TxNsduConfig;
    const CanTp_RxNsduConfigType* RxNsduConfig;
} CanTp_ChannelRuntimeType;
```

Runtime fields track the current ISO-TP transaction, including copied payload, sequence number, peer flow-control parameters, and the active timer loaded from the NSDU configuration.

### 5.2 NSDU Configuration Types

```c
typedef struct {
    CanTp_PduIdType CanTpTxNPduId;
    PduIdType CanTpTxNPduConfirmationId;
    PduIdType CanTpTxFcNPduId;
    uint16 CanTpNas;                /* N_As timeout in ms */
    uint16 CanTpNbs;                /* N_Bs timeout in ms */
    uint16 CanTpNcs;                /* N_Cs timeout in ms */
    uint8 CanTpTxAddressingFormat;
    uint8 CanTpTxPaddingActivation;
    uint8 CanTpTxTaType;
    uint16 CanTpTxMaxMessageLength;
    uint8 CanTpTxAddress;
    uint8 CanTpTxPriority;
} CanTp_TxNsduConfigType;

typedef struct {
    CanTp_PduIdType CanTpRxNPduId;
    PduIdType CanTpRxNSduId;
    PduIdType CanTpRxFcNPduConfirmationId;
    uint16 CanTpNar;                /* N_Ar timeout in ms */
    uint16 CanTpNbr;                /* N_Br timeout in ms */
    uint16 CanTpNcr;                /* N_Cr timeout in ms */
    uint8 CanTpRxAddressingFormat;
    uint8 CanTpRxPaddingActivation;
    uint8 CanTpRxTaType;
    uint16 CanTpRxMaxMessageLength;
    uint8 CanTpRxAddress;
    uint8 CanTpRxWftMax;
    uint8 CanTpRxPriority;
    uint16 CanTpBs;                 /* Block Size */
    uint16 CanTpSTmin;              /* Minimum Separation Time */
} CanTp_RxNsduConfigType;
```

### 5.3 Channel and Root Configuration

```c
typedef struct {
    CanTp_ChannelType ChannelId;
    CanTp_ChannelModeType ChannelMode;
    uint8 NumTxNsdu;
    uint8 NumRxNsdu;
    const CanTp_TxNsduConfigType* TxNsduConfigs;
    const CanTp_RxNsduConfigType* RxNsduConfigs;
} CanTp_ChannelConfigType;

typedef struct {
    const CanTp_GeneralConfigType* GeneralConfig;
    const CanTp_ChannelConfigType* ChannelConfigs;
    uint8 NumChannels;
} CanTp_ConfigType;
```

The link-time root `CanTp_Config` is passed to `CanTp_Init`.

---

## 6. API Design

### 6.1 Public Interfaces

| API | Signature | Function | SID | SWS 需求 |
|-----|-----------|----------|-----|----------|
| `CanTp_Init` | `void CanTp_Init(const CanTp_ConfigType* CfgPtr)` | Initialize module and reset channels | 0x01 | SWS_CanTp_00001 |
| `CanTp_Shutdown` | `void CanTp_Shutdown(void)` | Shutdown module and release channels | 0x02 | SWS_CanTp_00002 |
| `CanTp_Transmit` | `Std_ReturnType CanTp_Transmit(PduIdType CanTpTxSduId, const PduInfoType* CanTpTxInfoPtr)` | Request TP transmission | 0x03 | SWS_CanTp_00003 |
| `CanTp_CancelTransmit` | `Std_ReturnType CanTp_CancelTransmit(PduIdType CanTpTxSduId)` | Cancel an active transmission | 0x04 | SWS_CanTp_00004 |
| `CanTp_CancelReceive` | `Std_ReturnType CanTp_CancelReceive(PduIdType CanTpRxSduId)` | Cancel an active reception | 0x05 | SWS_CanTp_00005 |
| `CanTp_ChangeParameter` | `Std_ReturnType CanTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value)` | Change STmin/BS | 0x06 | SWS_CanTp_00006 |
| `CanTp_ReadParameter` | `Std_ReturnType CanTp_ReadParameter(PduIdType id, TPParameterType parameter, uint16* value)` | Read STmin/BS | 0x07 | SWS_CanTp_00007 |
| `CanTp_GetVersionInfo` | `void CanTp_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Return version info | 0x08 | SWS_CanTp_00008 |

### 6.2 Callbacks / Scheduled Functions

| Function | Trigger | Purpose | SWS 需求 |
|----------|---------|---------|----------|
| `CanTp_RxIndication` | `CanIf_RxIndication` | Classify incoming PCI and drive Rx state machine | SWS_CanTp_00009 |
| `CanTp_TxConfirmation` | `CanIf_TxConfirmation` | Advance Tx state machine after frame sent | SWS_CanTp_00010 |
| `CanTp_MainFunction` | Cyclic (5 ms) | Timeout supervision and periodic processing | SWS_CanTp_00011 |

### 6.3 Service IDs and DET Error Codes

| SID | API | Main DET Codes |
|-----|-----|----------------|
| 0x01 | Init | `CANTP_E_PARAM_CONFIG` |
| 0x02 | Shutdown | `CANTP_E_UNINIT` |
| 0x03 | Transmit | `CANTP_E_UNINIT`, `CANTP_E_INVALID_TX_ID`, `CANTP_E_PARAM_POINTER` |
| 0x04 | CancelTransmit | `CANTP_E_UNINIT` |
| 0x05 | CancelReceive | `CANTP_E_UNINIT` |
| 0x06 | ChangeParameter | `CANTP_E_UNINIT` |
| 0x07 | ReadParameter | `CANTP_E_UNINIT`, `CANTP_E_PARAM_POINTER` |
| 0x08 | GetVersionInfo | `CANTP_E_PARAM_POINTER` |
| 0x42 | RxIndication | — |
| 0x43 | TxConfirmation | — |

Key DET error codes from `CanTp.h`:

| Code | Name | Meaning |
|------|------|---------|
| 0x01 | `CANTP_E_PARAM_CONFIG` | Invalid configuration pointer |
| 0x02 | `CANTP_E_PARAM_ID` | Invalid PDU/SDU ID |
| 0x03 | `CANTP_E_PARAM_POINTER` | NULL pointer argument |
| 0x20 | `CANTP_E_UNINIT` | Module not initialized |
| 0x30 | `CANTP_E_INVALID_TX_ID` | Invalid Tx NSDU ID |
| 0x40 | `CANTP_E_INVALID_RX_ID` | Invalid Rx NSDU ID |
| 0x70 | `CANTP_E_INVALID_TX_LENGTH` | Invalid Tx SDU length |
| 0x80 | `CANTP_E_INVALID_RX_LENGTH` | Invalid Rx SDU length |

---

## 7. Processing Flows

### 7.1 Transmit Request (`CanTp_Transmit`)

1. Validate module initialized, SDU ID, and pointer (DET).
2. Find a free channel; fail if none available.
3. Look up `CanTp_TxNsduConfigType` by `CanTpTxSduId`.
4. Copy the full SDU into the channel buffer (`CANTP_CHANNEL_BUFFER_SIZE`).
5. If `DataLength <= 7`, send a Single Frame and enter `CANTP_CH_TX_SF` with `N_As`.
6. Otherwise send a First Frame, then enter `CANTP_CH_TX_WAIT_FC` with `N_Bs`.

### 7.2 Single Frame Transmission

```c
sfFrame[0] = CANTP_PCI_TYPE_SF | (Length & 0x0F);
sfFrame[1..Length] = Data[0..Length-1];
/* Remaining bytes padded with CANTP_PADDING_BYTE_VALUE */
CanIf_Transmit(CANTP_CANIF_TX_PDU_ID, &pduInfo);
```

### 7.3 First Frame / Consecutive Frame Transmission

First Frame:

```c
ffFrame[0] = CANTP_PCI_TYPE_FF | ((MessageLength >> 8) & 0x0F);
ffFrame[1] = MessageLength & 0xFF;
ffFrame[2..7] = first 6 bytes of payload;
DataIndex = 6;
SequenceNumber = 1;
```

Consecutive Frame:

```c
cfFrame[0] = CANTP_PCI_TYPE_CF | (SequenceNumber & 0x0F);
cfFrame[1..n] = next up to 7 payload bytes;
SequenceNumber = (SequenceNumber + 1) & 0x0F;
DataIndex += bytesToSend;
```

### 7.4 Flow Control Reception (Tx Side)

On receiving an FC frame while in `CANTP_CH_TX_WAIT_FC`:

- **CTS**: store `BlockSize` and `STmin`, transition to `CANTP_CH_TX_CF`, load `N_Cs`, and send the first CF.
- **WT**: reload `N_Bs` timer.
- **OVFLW**: reset the channel (receiver cannot handle the message).

### 7.5 Single Frame Reception

```c
sfDl = pci & 0x0F;
if (sfDl > 0 && sfDl <= 7) {
    copy sfDl bytes to channel buffer;
    PduR_RxIndication(CANTP_RX_DIAG_PHYSICAL, &pduInfo);
    reset channel;
}
```

### 7.6 Multi-Frame Reception

First Frame:

1. Decode 12-bit message length from `FF` PCI.
2. Validate length (`> 7` and `<= CANTP_MAX_MESSAGE_LENGTH`).
3. Allocate channel, copy first 6 bytes, set expected `SequenceNumber = 1`.
4. Send Flow Control `CTS` with default `BS` and `STmin`.
5. Enter `CANTP_CH_RX_CF` and load `N_Cr`.

Consecutive Frame:

1. Verify the active Rx channel is in `CANTP_CH_RX_CF` and `SequenceNumber` matches.
2. Copy up to 7 bytes into the buffer and advance `DataIndex`.
3. Reload `N_Cr`; if `DataIndex >= DataLength`, forward complete SDU to `PduR` and reset channel.

### 7.7 Timeout Handling (`CanTp_MainFunction`)

For every non-idle channel:

1. Decrement the timer each cycle (resolution = `CANTP_MAIN_FUNCTION_PERIOD_MS`).
2. On timer expiry:
   - `TX_SF` / `TX_FF` → `N_As` timeout, reset channel.
   - `TX_WAIT_FC` → `N_Bs` timeout, reset channel.
   - `TX_CF` → `N_Cs` timeout, send next CF and reload `N_Cs`.
   - `RX_CF` → `N_Cr` timeout, reset channel.

### 7.8 Tx Confirmation (`CanTp_TxConfirmation`)

- `TX_SF` → notify `PduR_TxConfirmation(ActiveNsduId, E_OK)` and reset.
- `TX_FF` → transition to `TX_WAIT_FC`, reload `N_Bs`.
- `TX_CF` → if all data sent, notify `PduR_TxConfirmation`; else reload `N_Cs` for the next frame.

---

## 8. Configuration Design

### 8.1 Pre-compile Configuration (`CanTp_Cfg.h`)

| Macro | Value | Description |
|-------|-------|-------------|
| `CANTP_DEV_ERROR_DETECT` | `STD_ON` | Development error detection |
| `CANTP_VERSION_INFO_API` | `STD_ON` | Version info API |
| `CANTP_NUM_CHANNELS` | 2 | Number of configured channels |
| `CANTP_NUM_TX_NSDU` | 4 | Number of Tx NSDUs |
| `CANTP_NUM_RX_NSDU` | 4 | Number of Rx NSDUs |
| `CANTP_DYNAMIC_CHANNEL_ALLOCATION` | `STD_OFF` | Static channel allocation |
| `CANTP_PADDING_BYTE` | `STD_ON` | Padding enabled |
| `CANTP_PADDING_BYTE_VALUE` | 204 (0xCC) | Padding byte value |
| `CANTP_CHANGE_PARAMETER_API` | `STD_ON` | Runtime parameter change enabled |
| `CANTP_READ_PARAMETER_API` | `STD_ON` | Runtime parameter read enabled |
| `CANTP_MAX_CHANNEL_CNT` | 4 | Maximum runtime channels |
| `CANTP_MAX_MESSAGE_LENGTH` | 4095 | Max ISO-TP message length (classic CAN) |
| `CANTP_CHANNEL_BUFFER_SIZE` | 64 | Per-channel buffer size |
| `CANTP_CAN_FRAME_LENGTH` | 8 | CAN frame length |
| `CANTP_MAIN_FUNCTION_PERIOD_MS` | 5 | MainFunction period |

### 8.2 Default Timing Parameters

| Parameter | Default (ms) | Description |
|-----------|--------------|-------------|
| `CANTP_NAS_DEFAULT` | 25 | `N_As` default (Tx confirmation timeout) |
| `CANTP_NBS_DEFAULT` | 75 | `N_Bs` default (FC reception timeout) |
| `CANTP_NCS_DEFAULT` | 25 | `N_Cs` default (CF transmission timeout) |
| `CANTP_NAR_DEFAULT` | 25 | `N_Ar` default (FC transmission timeout) |
| `CANTP_NBR_DEFAULT` | 75 | `N_Br` default (buffer availability timeout) |
| `CANTP_NCR_DEFAULT` | 150 | `N_Cr` default (CF reception timeout) |
| `CANTP_BS_DEFAULT` | 8 | Default Block Size |
| `CANTP_STMIN_DEFAULT` | 20 | Default minimum separation time |
| `CANTP_WFT_MAX_DEFAULT` | 8 | Max wait frames |

### 8.3 Link-time Configuration (`CanTp_Lcfg.c`)

- `CanTp_TxNsduConfigs[]` — per-Tx-NSDU timing, PDU IDs, addressing, TA type, priority.
- `CanTp_RxNsduConfigs[]` — per-Rx-NSDU timing, PDU IDs, addressing, TA type, BS/STmin.
- `CanTp_ChannelConfigs[]` — channel list referencing the NSDU tables.
- `CanTp_GeneralConfig` — global switches and period.
- `CanTp_Config` — root configuration pointer passed to `CanTp_Init`.

---

## 9. Error Handling & Safety

### 9.1 Runtime Error Codes

Key runtime error codes defined in `CanTp.h`:

| Code | Name | Trigger |
|------|------|---------|
| 0x01 | `CANTP_E_RX_COM` | Reception communication error |
| 0x02 | `CANTP_E_TX_COM` | Transmission communication error |
| 0x03 | `CANTP_E_RX_TIMEOUT_ARR` | `N_Ar` timeout |
| 0x04 | `CANTP_E_RX_TIMEOUT_BS` | `N_Bs` timeout |
| 0x05 | `CANTP_E_RX_TIMEOUT_CR` | `N_Cr` timeout |
| 0x06 | `CANTP_E_TX_TIMEOUT_AS` | `N_As` timeout |
| 0x08 | `CANTP_E_TX_TIMEOUT_CS` | `N_Cs` timeout |
| 0x09 | `CANTP_E_RX_INVALID_SN` | Unexpected sequence number |
| 0x0A | `CANTP_E_RX_INVALID_FS` | Invalid Flow Status |
| 0x0B | `CANTP_E_RX_UNEXPECTED_FC` | Unexpected FC frame |
| 0x0C | `CANTP_E_RX_WFT_MAX` | Wait frame counter exceeded |
| 0x15 | `CANTP_E_RX_SF_WRONG_LEN` | Invalid SF data length |
| 0x16 | `CANTP_E_RX_FF_WRONG_LEN` | Invalid FF data length |
| 0x17 | `CANTP_E_RX_CF_WRONG_SN` | Wrong CF sequence number |

### 9.2 Safety Mechanisms

- All SDU IDs validated against configured ranges before use.
- NULL-pointer checks wrapped by `CANTP_DEV_ERROR_DETECT`.
- Per-channel buffers isolate concurrent transactions.
- Sequence-number checking prevents frame insertion/reordering errors.
- Timer expiry aborts stuck transactions and releases the channel.

---

## 10. Memory & Performance

### 10.1 MemMap Sections

| Section | Usage |
|---------|-------|
| `CANTP_START_SEC_VAR_CLEARED_UNSPECIFIED` | Zero-initialized runtime variables (`CanTp_ChannelRuntime`, init flags, config pointer) |
| `CANTP_START_SEC_CONFIG_DATA_UNSPECIFIED` | Link-time configuration tables (`CanTp_Lcfg.c`) |
| `CANTP_START_SEC_CODE` | Module code (`CanTp.c`) |

### 10.2 Resource Estimation

| Resource | Estimate | Note |
|----------|----------|------|
| RAM | `CANTP_MAX_CHANNEL_CNT * (CANTP_CHANNEL_BUFFER_SIZE + ~30 bytes overhead)` | Per-channel runtime |
| ROM | Configuration tables + code | Depends on NSDU/channel counts |
| Stack | Low | No recursion; only local 8-byte frame buffers |
| CPU | Periodic load in `CanTp_MainFunction` every 5 ms | Linear in number of channels |

---

## 11. Integration Guide

### 11.1 Upper Layer (PduR)

- PduR calls `CanTp_Transmit` with the Tx SDU ID and `PduInfoType`.
- CanTp calls `PduR_RxIndication` with the reassembled SDU (currently hard-coded to `CANTP_RX_DIAG_PHYSICAL`).
- CanTp calls `PduR_TxConfirmation` when a transmission completes successfully.

### 11.2 Lower Layer (CanIf)

- CanTp uses `CanIf_Transmit` to send SF/FF/CF/FC frames.
- CanIf invokes `CanTp_RxIndication` for incoming CAN frames.
- CanIf invokes `CanTp_TxConfirmation` after successful transmission.

### 11.3 Initialization Order

1. Initialize `CanIf` and underlying CAN driver.
2. Call `CanTp_Init(&CanTp_Config)`.
3. Upper layers may start transmitting/receiving.

---

## 12. Test Strategy

### 12.1 Unit Tests

| Scenario | Coverage |
|----------|----------|
| Init/Shutdown | Null config, double init, shutdown resets state |
| Single Frame | Tx/Rx of 1–7 byte messages |
| Multi-Frame | Tx/Rx of messages > 7 bytes up to 64/4095 bytes |
| Flow Control | CTS, WT, OVFLW handling |
| Sequence Number | Wrap-around from 15 to 0, invalid SN rejection |
| Timeouts | `N_As`, `N_Bs`, `N_Cs`, `N_Cr` expiry |
| Parameter API | `CanTp_ChangeParameter` / `CanTp_ReadParameter` for STmin/BS |
| Cancellation | `CanTp_CancelTransmit` and `CanTp_CancelReceive` |

### 12.2 Integration Tests

| Scenario | Purpose |
|----------|---------|
| End-to-end diagnostic request/response | Verify CanTp + CanIf + PduR + Dcm path |
| Bus-off during multi-frame | Confirm timeout abort and resource release |
| Peer STmin enforcement | Validate inter-frame separation |
| Functional vs physical addressing | Ensure functional SF does not send FC |

---

## 13. Implementation Notes / TODO

- **Addressing**: current implementation uses standard addressing only; extended/mixed/normal-fixed addressing is defined in the types but not yet implemented in frame construction.
- **STmin enforcement**: `CanTp_MainFunction` reserves a placeholder for STmin-based CF pacing; a separate STmin sub-timer should be added for full ISO-TP compliance.
- **Buffer handling**: the receiver does not perform `PduR` buffer-request polling (`N_Br`); data is copied into an internal channel buffer and forwarded only when complete.
- **Wait frames**: `WftCounter` is present in the runtime structure but not incremented/decremented on FC.WT reception.
- **Rx SDU routing**: reassembled SDUs are currently forwarded with the constant ID `CANTP_RX_DIAG_PHYSICAL`; full multi-NSDU routing should map by `RxPduId`.
- **CanIf PDU IDs**: SF/FF/CF transmissions and FC reception use fixed PDU IDs from configuration; dynamic per-NSDU mapping may be needed for complex topologies.
- **Padding**: padding is always applied when enabled; support for CAN-FD frames (>8 bytes) is not included in this version.

---

## 14. References

1. AUTOSAR_SWS_CANTransportProtocol.pdf — AUTOSAR Classic Platform 4.4.0
2. ISO 15765-2:2016 — Road vehicles — Diagnostic communication over CAN
3. `docs/modules/CanTp.md`
4. `src/bsw/ecual/cantp/include/CanTp.h`
5. `src/bsw/ecual/cantp/include/CanTp_Cfg.h`
6. `src/bsw/ecual/cantp/src/CanTp.c`
7. `src/bsw/ecual/cantp/src/CanTp_Lcfg.c`

## 需求追溯表

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_CanTp | — | CANTP 模块级需求归属 |
