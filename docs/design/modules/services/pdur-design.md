> **Module ID**: 0x69  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_PDURouter  
> **Source Path**: `src/bsw/services/pdur/`  
> **Reference Document**: `docs/modules/PDUR.md`  
> **Doc Version**: 1.0  
> **Status**: Approved

# PduR (PDU Router) Design Document

## 1. Module Overview

PduR is the AUTOSAR PDU Router located in the Service Layer. It provides routing services for I-PDUs between upper-layer modules (Com, Dcm, CanTp, LinTp, etc.) and lower-layer interface modules (CanIf, LinIf, EthIf, SoAd, etc.). PduR is protocol-agnostic and relies purely on static routing tables configured at link time.

### 1.1 Position in the Stack

```
┌─────────────────────────────────────────────────────────┐
│  Com / Dcm / CanTp / LinTp / SomeIpTp (Upper Layers)    │
├─────────────────────────────────────────────────────────┤
│  PduR (Services)                                        │
├─────────────────────────────────────────────────────────┤
│  CanIf / LinIf / EthIf / SoAd (Lower Interface Layers)  │
├─────────────────────────────────────────────────────────┤
│  Can / Lin / Eth (MCAL)                                 │
└─────────────────────────────────────────────────────────┘
```

---

## 2. Standards & Dependencies

### 2.1 Standards

| Standard | Version | Description |
|----------|---------|-------------|
| AUTOSAR SWS PDU Router | 4.4.0 | PDU Router specification |

### 2.2 Dependencies

| Module | Direction | Purpose |
|--------|-----------|---------|
| Com / Dcm / CanTp | Upper | Source/destination of PDUs |
| CanIf / LinIf / EthIf / SoAd | Lower | Interface to bus drivers |
| Det | Common | Development error detection |

---

## 3. Architecture Design

### 3.1 Internal Components

| Component | Responsibility |
|-----------|----------------|
| Routing Table Lookup | Find routing path by source PDU ID and module type |
| Downward Routing | `PduR_Transmit` from upper layer to lower interface |
| Upward Routing | `PduR_RxIndication` from lower interface to upper layer |
| Confirmation Routing | `PduR_TxConfirmation` from lower to upper layer |
| Trigger Transmit Routing | `PduR_TriggerTransmit` from lower to upper layer |
| FIFO Queue Manager | Deferred routing with FIFO buffering |
| Routing Path Group Manager | Enable/disable groups of routing paths |

### 3.2 File Structure

```
src/bsw/services/pdur/
├── include/
│   ├── PduR.h           # Public API and types
│   ├── PduR_Cfg.h       # Pre-compile configuration
│   ├── PduR_DoIP.h      # DoIP-specific routing API
│   └── PduR_LinTp.h     # LIN TP routing API
└── src/
    ├── PduR.c           # Core routing implementation
    ├── PduR_Lcfg.c      # Link-time routing tables
    └── PduR_test.c      # Unit tests
```

---

## 4. State Machine

### 4.1 Module State

```
        Init
         │
         ▼
    ┌─────────┐   DeInit    ┌─────────┐
    │  UNINIT │ ◄────────── │  INIT   │
    └─────────┘             └─────────┘
```

- `PDUR_STATE_UNINIT`：模块未初始化，除 `PduR_Init` 外 API 报错。
- `PDUR_STATE_INIT`：模块已初始化，可进行路由。

### 4.2 Routing Path State

Each routing path has:
- `IsEnabled`：路径是否使能（受路由路径组控制）
- `FifoQueue`：FIFO 队列状态（用于延迟路由）

---

## 5. Core Data Structures

### 5.1 Internal State

```c
typedef struct
{
    uint8 State;
    const PduR_ConfigType* ConfigPtr;
    PduR_RoutingPathStateType PathStates[PDUR_NUMBER_OF_ROUTING_PATHS];
} PduR_InternalStateType;
```

### 5.2 Routing Path State

```c
typedef struct
{
    PduR_FifoQueueType FifoQueue;
    boolean IsEnabled;
} PduR_RoutingPathStateType;
```

### 5.3 FIFO Queue

```c
typedef struct
{
    PduR_FifoEntryType Entries[PDUR_MAX_FIFO_DEPTH];
    uint8 Head;
    uint8 Tail;
    uint8 Count;
} PduR_FifoQueueType;
```

### 5.4 Source PDU Configuration

```c
typedef struct {
    PduIdType SourcePduId;
    uint8 SourceModule;
    PduLengthType SduLength;
} PduR_SrcPduConfigType;
```

### 5.5 Destination PDU Configuration

```c
typedef struct {
    PduIdType DestPduId;
    uint8 DestModule;
    PduR_DestPduProcessingType Processing;
    uint8 FifoDepth;
} PduR_DestPduConfigType;
```

### 5.6 Routing Path Configuration

```c
typedef struct {
    PduR_SrcPduConfigType SrcPdu;
    const PduR_DestPduConfigType* DestPdus;
    uint8 NumDestPdus;
    PduR_RoutingPathType PathType;
    boolean GatewayOperation;
} PduR_RoutingPathConfigType;
```

---

## 6. API Design

### 6.1 Public API

| API | SID | Purpose | SWS 需求 |
|-----|-----|---------|----------|
| PduR_Init | 0xF0 | Initialize router | SWS_PduR_00001 |
| PduR_DeInit | 0xF1 | Deinitialize router | SWS_PduR_00002 |
| PduR_GetVersionInfo | 0xF2 | Version info | SWS_PduR_00012 |
| PduR_Transmit | 0x49 | Route PDU downward | SWS_PduR_00003 |
| PduR_CancelTransmitRequest | 0x4A | Cancel transmit | SWS_PduR_00007 |
| PduR_CancelReceiveRequest | 0x4B | Cancel receive | SWS_PduR_00008 |
| PduR_ChangeParameterRequest | 0x4C | Change TP parameter | SWS_PduR_00009 |
| PduR_EnableRouting | - | Enable routing path group | SWS_PduR_00010 |
| PduR_DisableRouting | - | Disable routing path group | SWS_PduR_00011 |
| PduR_MainFunction | 0xEF | Cyclic FIFO processing | SWS_PduR_00013 |

### 6.2 Lower-Layer Callbacks

| Callback | Purpose | SWS 需求 |
|----------|---------|----------|
| PduR_RxIndication | Lower layer reports received PDU | SWS_PduR_00004 |
| PduR_TxConfirmation | Lower layer reports transmit result | SWS_PduR_00005 |
| PduR_TriggerTransmit | Lower layer requests PDU data | SWS_PduR_00006 |

### 6.3 Module Type Constants

| Constant | Value | Module |
|----------|-------|--------|
| PDUR_MODULE_COM | 0x01 | COM |
| PDUR_MODULE_CANIF | 0x02 | CanIf |
| PDUR_MODULE_DCM | 0x03 | Dcm |
| PDUR_MODULE_CANTP | 0x04 | CanTp |
| PDUR_MODULE_LINTP | 0x05 | LinTp |
| PDUR_MODULE_DOIP | 0x06 | DoIP |

### 6.4 DET Error Codes

| Error Code | Name | Trigger |
|------------|------|---------|
| 0x01 | PDUR_E_PARAM_POINTER | Null pointer |
| 0x02 | PDUR_E_PARAM_CONFIG | Invalid config |
| 0x03 | PDUR_E_INVALID_REQUEST | Invalid request |
| 0x04 | PDUR_E_PDU_ID_INVALID | PDU ID invalid |
| 0x05 | PDUR_E_ROUTING_PATH_GROUP_INVALID | Invalid group |
| 0x07 | PDUR_E_UNINIT | Not initialized |
| 0x20 | PDUR_E_LOIF_TXCONF_WITHOUT_REQ | Unexpected Tx confirmation |
| 0x21 | PDUR_E_LOIF_RXIND_WITHOUT_REQ | Unexpected Rx indication |

---

## 7. Processing Flows

### 7.1 Downward Transmit Flow

1. Upper layer (Com/Dcm) calls `PduR_Transmit(TxPduId, PduInfoPtr)`.
2. PduR validates state and parameters.
3. `PduR_FindRoutingPath` searches routing table with `SourceModule == PDUR_MODULE_COM` or `PDUR_MODULE_DCM`.
4. For each destination in the path:
   - If `DestModule == PDUR_MODULE_CANIF`, call `CanIf_Transmit`.
5. Return `E_OK` if at least one destination succeeds.

### 7.2 Upward Receive Flow

1. Lower layer (CanIf) calls `PduR_RxIndication(RxPduId, PduInfoPtr)`.
2. PduR finds routing path with `SourceModule == PDUR_MODULE_CANIF`.
3. For each destination:
   - If `DestModule == PDUR_MODULE_COM`, call `Com_RxIndication`.
   - If `DestModule == PDUR_MODULE_DCM`, call `Dcm_RxIndication`.

### 7.3 Transmit Confirmation Flow

1. Lower layer calls `PduR_TxConfirmation(TxPduId, result)`.
2. PduR finds path where CanIf is the source.
3. Forwards confirmation to Com or Dcm based on destination module.

### 7.4 Trigger Transmit Flow

1. Lower layer calls `PduR_TriggerTransmit(TxPduId, PduInfoPtr)`.
2. PduR routes to Com/Dcm `TriggerTransmit` to fill `PduInfoPtr`.
3. Returns result from upper layer.

### 7.5 FIFO Processing (PduR_MainFunction)

1. Iterate all routing paths.
2. For paths with deferred processing, pop FIFO entries.
3. Route popped PDUs to destinations.

---

## 8. Configuration Design

### 8.1 Pre-compile Configuration (PduR_Cfg.h)

| Macro | Description |
|-------|-------------|
| PDUR_NUMBER_OF_ROUTING_PATHS | Number of routing paths |
| PDUR_MAX_FIFO_DEPTH | Maximum FIFO depth |
| PDUR_MAX_DESTINATIONS_PER_PATH | Max destinations per path |
| PDUR_DEV_ERROR_DETECT | DET switch |
| PDUR_VERSION_INFO_API | Version info API switch |

### 8.2 Link-Time Configuration (PduR_Lcfg.c)

- `PduR_RoutingPaths[]`：源 PDU + 目标 PDU 列表 + 路径类型。
- Destination arrays：每个路径对应的 `PduR_DestPduConfigType` 数组。
- `PduR_Config`：顶层配置，聚合所有路径。

Example routing paths in current config:
- COM TX Engine Status -> CanIf
- COM TX Vehicle Speed -> CanIf
- COM TX Battery Status -> CanIf
- CanIf RX Engine Command -> COM
- CanIf RX Vehicle Command -> COM
- DCM TX Diag Response -> CanIf
- CanIf RX Diag Request -> DCM

---

## 9. Error Handling & Safety

- All public APIs validate init state and pointers when `PDUR_DEV_ERROR_DETECT == STD_ON`.
- Unknown PDU IDs or missing routing paths report `PDUR_E_ROUTING_PATH_NOT_FOUND`.
- FIFO overflow reports `PDUR_E_FIFO_FULL` and drops oldest/lowest-priority entries.

---

## 10. Memory & Performance

### 10.1 MemMap Sections

| Section | Usage |
|---------|-------|
| PDUR_START_SEC_VAR_CLEARED_UNSPECIFIED | Internal state |
| PDUR_START_SEC_CONFIG_DATA_UNSPECIFIED | Routing tables |
| PDUR_START_SEC_CODE | Code |

### 10.2 Resource Estimate

| Resource | Estimate | Notes |
|----------|----------|-------|
| RAM | Small | State + FIFO buffers |
| ROM | Depends on routing table size | Dominated by `PduR_Lcfg.c` |

---

## 11. Integration Guidelines

### 11.1 Initialization Sequence

1. Lower interfaces initialized (CanIf, etc.)
2. Upper modules initialized (Com, Dcm)
3. `PduR_Init(&PduR_Config)`

### 11.2 With COM and CAN Stack

```
Com_SendSignal -> Com_Transmit -> PduR_Transmit -> CanIf_Transmit -> Can_Write
Can_RxIndication -> CanIf_RxIndication -> PduR_RxIndication -> Com_RxIndication
```

### 11.3 With DCM and Diagnostic Stack

```
Dcm -> PduR_Transmit -> CanIf_Transmit -> CanTp (for multi-frame)
CanTp -> CanIf_RxIndication -> PduR_RxIndication -> Dcm_RxIndication
```

---

## 12. Testing Strategy

### 12.1 Unit Tests

- Routing table lookup
- Downward routing (Com -> CanIf)
- Upward routing (CanIf -> Com/Dcm)
- Tx confirmation routing
- Trigger transmit routing
- FIFO push/pop and overflow

### 12.2 Integration Tests

- End-to-end COM CAN transmit and receive
- DCM diagnostic request/response path
- Multi-destination routing (gateway)

---

## 13. Implementation Notes / TODO

- Current implementation supports 1:N routing paths but all configured paths are 1:1.
- `PduR_ChangeParameterRequest` is a stub.
- Cancel transmit/receive are partially implemented.
- Routing path group enable/disable uses simple boolean per path.
- Gateway operations are marked in config but not fully implemented.

---

## 14. References

- AUTOSAR_SWS_PDURouter.pdf 4.4.0
- `docs/modules/PDUR.md`
- `src/bsw/services/pdur/include/PduR.h`
- `src/bsw/services/pdur/src/PduR.c`
- `src/bsw/services/pdur/src/PduR_Lcfg.c`
