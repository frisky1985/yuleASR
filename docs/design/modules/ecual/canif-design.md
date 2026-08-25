> **Module ID**: 0x3C  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_CANInterface  
> **Source Path**: `src/bsw/ecual/canif/`  
> **Reference Document**: `docs/modules/CanIf.md`  
> **Doc Version**: 1.0  
> **Status**: Approved

# CanIf (CAN Interface) Design Document

## 1. Module Overview

CanIf is the AUTOSAR CAN Interface module located in the ECU Abstraction Layer (ECUAL). It provides a hardware-independent abstraction between the CAN driver (MCAL) and upper layers such as PduR, Com, CanTp, CanNm, and XCP. CanIf manages CAN controller modes, PDU routing, transmission requests, reception indications, transceiver control, and wakeup handling.

### 1.1 Position in the Stack

```
┌─────────────────────────────────────────────────────────┐
│                    Upper Layers                          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐│
│  │   Com   │ │  CanTp  │ │  PduR   │ │    CanNm/XCP    ││
│  └────┬────┘ └────┬────┘ └────┬────┘ └────────┬────────┘│
├───────┴───────────┴───────────┴────────────────┴────────┤
│                   CanIf (ECUAL)                         │
├─────────────────────────────────────────────────────────┤
│                    Can (MCAL)                           │
└─────────────────────────────────────────────────────────┘
```

---

## 2. Standards & Dependencies

### 2.1 Standards

| Standard | Version | Description |
|----------|---------|-------------|
| AUTOSAR SWS CAN Interface | 4.4.0 | CAN Interface specification |
| ISO 11898 | - | CAN Data Link Layer |

### 2.2 Dependencies

| Module | Direction | Purpose |
|--------|-----------|---------|
| Can | Lower | Controller driver |
| PduR | Upper | PDU routing for Tx/Rx |
| CanSM | Upper/Peer | Controller mode and bus-off management |
| EcuM | Peer | Wakeup source handling |
| Det | Common | Development error detection |

---

## 3. Architecture Design

### 3.1 Internal Components

| Component | Responsibility |
|-----------|----------------|
| Controller Manager | Initialize/deinitialize, set/get controller mode |
| PDU Router (Tx) | Map logical TxPduId to HTH and call `Can_Write` |
| PDU Router (Rx) | Match incoming CAN ID + HRH to RxPduConfig and call `PduR_RxIndication` |
| Transceiver Manager | Set/get transceiver mode and wakeup reason (stubbed) |
| Callback Handler | Dispatch `CanIf_TxConfirmation`, `CanIf_RxIndication`, `CanIf_ControllerBusOff`, `CanIf_ControllerModeIndication` |

### 3.2 File Structure

```
src/bsw/ecual/canif/
├── include/
│   ├── CanIf.h           # Public API and types
│   └── CanIf_Cfg.h       # Pre-compile configuration
└── src/
    ├── CanIf.c           # Implementation
    └── CanIf_Lcfg.c      # Link-time PDU/HOH/controller config
```

---

## 4. State Machines

### 4.1 Controller Mode State Machine

```
        Init
         │
         ▼
    ┌─────────┐   Set STARTED    ┌─────────┐
    │ STOPPED │ ───────────────► │ STARTED │
    └────┬────◄──────────────────└────┬────┘
         │        Set STOPPED         │
         │                            │
         │ Set SLEEP                  │
         ▼                            │
    ┌─────────┐                       │
    │  SLEEP  │ ──────────────────────┘
    └─────────┘   Set STARTED
```

Controller modes:
- `CANIF_CS_UNINIT`
- `CANIF_CS_STOPPED`
- `CANIF_CS_STARTED`
- `CANIF_CS_SLEEP`

### 4.2 PDU Channel Mode

| Mode | Tx | Rx |
|------|----|----|
| `CANIF_OFFLINE` | Disabled | Disabled |
| `CANIF_TX_OFFLINE` | Disabled | Enabled |
| `CANIF_TX_OFFLINE_ACTIVE` | Disabled (Tx notifications active) | Enabled |
| `CANIF_ONLINE` | Enabled | Enabled |

---

## 5. Core Data Structures

### 5.1 Runtime State

```c
static boolean CanIf_DriverInitialized = FALSE;
static CanIf_ControllerModeType CanIf_ControllerMode[CANIF_NUM_CONTROLLERS];
static CanIf_PduModeType CanIf_PduMode[CANIF_NUM_CONTROLLERS];
static const CanIf_ConfigType* CanIf_ConfigPtr = NULL_PTR;
```

### 5.2 Tx PDU Configuration

```c
typedef struct {
    PduIdType PduId;
    CanIf_CanIdType CanId;
    CanIf_CanIdTypeType CanIdType;
    CanIf_HohType Hth;
    uint8 ControllerId;
    uint8 Length;
    boolean TxConfirmation;
    boolean UserType;
} CanIf_TxPduConfigType;
```

### 5.3 Rx PDU Configuration

```c
typedef struct {
    PduIdType PduId;
    CanIf_CanIdType CanId;
    CanIf_CanIdType CanIdMask;
    CanIf_CanIdTypeType CanIdType;
    CanIf_HohType Hrh;
    uint8 ControllerId;
    uint8 Length;
    boolean RxIndication;
} CanIf_RxPduConfigType;
```

### 5.4 Controller Configuration

```c
typedef struct {
    uint8 ControllerId;
    uint32 BaudRate;
    uint32 BaudRateConfig;
    CanIf_ControllerModeType DefaultMode;
    boolean WakeupSupport;
    boolean WakeupNotification;
    boolean BusOffNotification;
    boolean ErrorNotification;
} CanIf_ControllerConfigType;
```

### 5.5 Top-Level Configuration

```c
typedef struct {
    const CanIf_ControllerConfigType* Controllers;
    uint8 NumControllers;
    const CanIf_HrhConfigType* HrhConfigs;
    uint8 NumHrhConfigs;
    const CanIf_HthConfigType* HthConfigs;
    uint8 NumHthConfigs;
    const CanIf_TxPduConfigType* TxPdus;
    uint8 NumTxPdus;
    const CanIf_RxPduConfigType* RxPdus;
    uint8 NumRxPdus;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean DLCCheck;
    boolean SoftwareFilter;
    boolean ReadRxPduDataApi;
    boolean ReadTxPduNotifyStatusApi;
    boolean ReadRxPduNotifyStatusApi;
} CanIf_ConfigType;
```

---

## 6. API Design

### 6.1 Public API

| API | SID | Purpose | SWS 需求 |
|-----|-----|---------|----------|
| CanIf_Init | 0x01 | Initialize module | SWS_CanIf_00001 |
| CanIf_DeInit | 0x02 | Deinitialize module | SWS_CanIf_00002 |
| CanIf_SetControllerMode | 0x03 | Set controller mode | SWS_CanIf_00003 |
| CanIf_GetControllerMode | 0x04 | Get controller mode | SWS_CanIf_00004 |
| CanIf_Transmit | 0x05 | Transmit PDU | SWS_CanIf_00005 |
| CanIf_CancelTransmit | - | Cancel transmit request | SWS_CanIf_00006 |
| CanIf_SetPduMode | 0x08 | Set PDU channel mode | SWS_CanIf_00007 |
| CanIf_GetPduMode | 0x09 | Get PDU channel mode | SWS_CanIf_00008 |
| CanIf_GetVersionInfo | 0x0B | Version info | SWS_CanIf_00009 |
| CanIf_SetDynamicTxId | 0x0C | Dynamic CAN ID | SWS_CanIf_00014 |
| CanIf_CheckWakeup | 0x11 | Check wakeup events | SWS_CanIf_00015 |
| CanIf_SetTrcvMode | 0x0D | Set transceiver mode | SWS_CanIf_00016 |
| CanIf_GetTrcvMode | 0x0E | Get transceiver mode | SWS_CanIf_00017 |
| CanIf_GetTrcvWakeupReason | 0x0F | Get transceiver wakeup reason | SWS_CanIf_00018 |
| CanIf_SetTrcvWakeupMode | 0x10 | Set transceiver wakeup mode | SWS_CanIf_00019 |
| CanIf_SetBaudrate | 0x27 | Set baudrate | SWS_CanIf_00020 |
| CanIf_GetBaudrate | 0x28 | Get baudrate | SWS_CanIf_00021 |

### 6.2 Lower-Layer Callbacks

| Callback | Purpose | SWS 需求 |
|----------|---------|----------|
| CanIf_TxConfirmation | CAN driver reports Tx complete | SWS_CanIf_00010 |
| CanIf_RxIndication | CAN driver reports Rx frame | SWS_CanIf_00011 |
| CanIf_ControllerBusOff | Bus-off event | SWS_CanIf_00012 |
| CanIf_ControllerModeIndication | Controller mode changed | SWS_CanIf_00013 |

### 6.3 DET Error Codes

| Error Code | Name | Trigger |
|------------|------|---------|
| 0x01 | CANIF_E_PARAM_CANID | Invalid CAN ID |
| 0x03 | CANIF_E_PARAM_CONTROLLER | Invalid controller ID |
| 0x04 | CANIF_E_PARAM_POINTER | Null pointer |
| 0x14 | CANIF_E_UNINIT | Module not initialized |
| 0x50 | CANIF_E_INVALID_TXPDUID | Invalid Tx PDU ID |
| 0x60 | CANIF_E_INVALID_RXPDUID | Invalid Rx PDU ID |
| 0x7C | CANIF_E_ALREADY_INITIALIZED | Already initialized |

---

## 7. Processing Flows

### 7.1 Transmit Flow

1. Upper layer calls `CanIf_Transmit(TxPduId, PduInfoPtr)`.
2. CanIf validates init state, TxPduId, and pointer.
3. Checks controller mode is `STARTED` and PDU mode is not `OFFLINE`.
4. Maps TxPduId to `CanIf_TxPduConfigType`.
5. Builds `Can_PduType` with CAN ID, DLC, and data pointer.
6. Calls `Can_Write(Hth, &canPdu)`.
7. Returns `E_OK` or `E_NOT_OK`.

### 7.2 Receive Flow

1. CAN driver ISR calls `CanIf_RxIndication(Mailbox, PduInfoPtr)`.
2. CanIf iterates `RxPduConfig` table.
3. Matches `Hrh` and `CanId` (software filtering).
4. Builds `PduInfoType` and calls `PduR_RxIndication(PduId, &pduInfo)`.

### 7.3 Tx Confirmation Flow

1. CAN driver calls `CanIf_TxConfirmation(CanTxPduId)`.
2. CanIf checks `TxConfirmation` flag in Tx PDU config.
3. Calls `PduR_TxConfirmation(CanTxPduId, E_OK)`.

### 7.4 Controller Mode Change Flow

1. Upper layer (e.g., CanSM) calls `CanIf_SetControllerMode(ControllerId, Mode)`.
2. CanIf calls `Can_SetControllerMode(ControllerId, Mode)`.
3. On success, updates `CanIf_ControllerMode[ControllerId]`.

---

## 8. Configuration Design

### 8.1 Pre-compile Configuration (CanIf_Cfg.h)

| Macro | Description |
|-------|-------------|
| CANIF_NUM_CONTROLLERS | Number of CAN controllers |
| CANIF_NUM_TX_PDUS | Number of Tx L-PDUs |
| CANIF_NUM_RX_PDUS | Number of Rx L-PDUs |
| CANIF_NUM_HOH | Number of hardware object handles |
| CANIF_DEV_ERROR_DETECT | DET switch |
| CANIF_DEFAULT_BAUDRATE | Default baudrate |

### 8.2 Link-Time Configuration (CanIf_Lcfg.c)

- `CanIf_HohCfg[]`：HOH 到控制器和驱动对象的映射。
- `CanIf_TxPduCfg[]`：Tx L-PDU 配置（CAN ID、HTH、DLC）。
- `CanIf_RxPduCfg[]`：Rx L-PDU 配置（CAN ID、Mask、HRH、DLC）。
- `CanIf_ControllerCfg[]`：控制器默认模式。
- `CanIf_RxPduHohMap[][]`：HRH 到 Rx PDU 的快速查找表。

---

## 9. Error Handling & Safety

- All public APIs validate init state and parameters when `CANIF_DEV_ERROR_DETECT == STD_ON`.
- `CanIf_Transmit` returns `E_NOT_OK` if controller not started or PDU channel offline.
- Callbacks silently return if module not initialized to avoid ISR-level DET noise.
- Transceiver APIs are stubbed and return default values.

---

## 10. Memory & Performance

### 10.1 MemMap Sections

| Section | Usage |
|---------|-------|
| CANIF_START_SEC_VAR_CLEARED_UNSPECIFIED | Runtime state variables |
| CANIF_START_SEC_CONFIG_DATA_UNSPECIFIED | Constant configuration tables |
| CANIF_START_SEC_CODE | Code |

### 10.2 Resource Estimate

| Resource | Estimate | Notes |
|----------|----------|-------|
| RAM | < 1 KB | State arrays per controller + config pointer |
| ROM | Depends on PDU count | Config tables dominate |

---

## 11. Integration Guidelines

### 11.1 Initialization Sequence

1. `Can_Init(&CanConfig)`
2. `CanIf_Init(&CanIfConfig)`
3. `CanIf_SetControllerMode(0, CANIF_CS_STARTED)`
4. `CanIf_SetPduMode(0, CANIF_ONLINE)`

### 11.2 With PduR

- Tx: `PduR` -> `CanIf_Transmit` -> `Can_Write`
- Rx: `Can` -> `CanIf_RxIndication` -> `PduR_RxIndication`
- Confirmation: `Can` -> `CanIf_TxConfirmation` -> `PduR_TxConfirmation`

### 11.3 With CanSM

- CanSM requests mode changes via `CanIf_SetControllerMode`.
- CanSM receives bus-off and mode indications from CanIf callbacks.

---

## 12. Testing Strategy

### 12.1 Unit Tests

- Init/deinit sequence
- Controller mode transitions
- PDU mode control
- Tx/Rx PDU routing
- Software filter matching
- DLC check (if enabled)
- Tx confirmation callback

### 12.2 Integration Tests

- End-to-end CAN loopback: `CanIf_Transmit` -> `Can_Write` -> `CanIf_RxIndication` -> `PduR_RxIndication`
- Bus-off recovery flow with CanSM stub

---

## 13. Implementation Notes / TODO

- `CanIf_SetDynamicTxId` currently does not modify runtime config (stub).
- Transceiver management APIs (`CanIf_SetTrcvMode`, `CanIf_GetTrcvWakeupReason`, etc.) are stubs.
- `CanIf_ControllerBusOff` and `CanIf_ControllerModeIndication` update internal state but do not yet call CanSM callbacks (commented out).
- `CanIf_CancelTransmit` is a placeholder returning `E_OK`.

---

## 14. References

- AUTOSAR_SWS_CANInterface.pdf 4.4.0
- `docs/modules/CanIf.md`
- `src/bsw/ecual/canif/include/CanIf.h`
- `src/bsw/ecual/canif/src/CanIf.c`
- `src/bsw/ecual/canif/src/CanIf_Lcfg.c`
