# PduR (Protocol Data Unit Router) Module Specification

> **Module:** PduR (PDU Router)  
> **Layer:** Service Layer  
> **Standard:** AutoSAR Classic Platform 4.x  
> **Platform:** NXP i.MX8M Mini  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

PduR is a core Service Layer module in the AutoSAR BSW stack. It is responsible for routing I-PDUs (Interaction Layer Protocol Data Units) between upper-layer modules (e.g., Com, Dcm) and lower-layer communication interface modules (e.g., CanIf, EthIf, LinIf).

PduR does **not** process bus protocols; it only performs routing decisions and dispatches PDUs to the appropriate destination module based on statically configured routing tables.

### Key Responsibilities
- Route transmit requests from upper layers (Com, Dcm) to lower layers (CanIf, LinIf, etc.)
- Route receive indications from lower layers to upper layers
- Route transmit confirmations from lower layers back to upper layers
- Support multicast (one source to multiple destinations)
- Support FIFO-based deferred routing
- Support routing path group enable/disable

---

## 2. API List

### 2.1 Core Lifecycle APIs

| API | Description |
|-----|-------------|
| `PduR_Init(const PduR_ConfigType* ConfigPtr)` | Initializes the PduR module with the given configuration |
| `PduR_DeInit(void)` | Deinitializes the PduR module |
| `PduR_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Retrieves version information of the module |

### 2.2 Upper-Layer Transmit APIs

These APIs are called by upper-layer modules to send PDUs downward.

| API | Called By | Description |
|-----|-----------|-------------|
| `PduR_ComTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)` | Com | Routes a COM transmit request to the configured lower layer |
| `PduR_DcmTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)` | Dcm | Routes a DCM transmit request to the configured lower layer |

### 2.3 Lower-Layer Callback APIs

These APIs are called by lower-layer modules to report events upward.

| API | Called By | Description |
|-----|-----------|-------------|
| `PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | CanIf | Routes a CAN receive indication to the upper layer (Com or Dcm) |
| `PduR_CanIfTxConfirmation(PduIdType TxPduId, Std_ReturnType result)` | CanIf | Routes a CAN transmit confirmation to the upper layer |
| `PduR_CanIfTriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)` | CanIf | Requests PDU data from upper layer for trigger transmit |

### 2.4 Generic Internal APIs

| API | Description |
|-----|-------------|
| `PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)` | Generic transmit handler (mapped by upper-layer macros) |
| `PduR_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | Generic RxIndication handler |
| `PduR_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)` | Generic TxConfirmation handler |
| `PduR_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)` | Generic TriggerTransmit handler |
| `PduR_CancelTransmitRequest(PduIdType TxPduId)` | Cancels a pending transmit request |
| `PduR_CancelReceiveRequest(PduIdType RxPduId)` | Cancels a pending receive request |
| `PduR_ChangeParameterRequest(PduIdType id, TPParameterType parameter, uint16 value)` | Changes transport protocol parameters |
| `PduR_EnableRouting(uint8 id)` | Enables a routing path group |
| `PduR_DisableRouting(uint8 id)` | Disables a routing path group |
| `PduR_MainFunction(void)` | Periodic main function for FIFO processing |

---

## 3. Data Types

### 3.1 PduR_StateType
```c
typedef enum {
    PDUR_STATE_UNINIT = 0,
    PDUR_STATE_INIT
} PduR_StateType;
```

### 3.2 PduR_ReturnType
```c
typedef enum {
    PDUR_OK = 0,
    PDUR_NOT_OK,
    PDUR_BUSY,
    PDUR_E_SDU_MISMATCH
} PduR_ReturnType;
```

### 3.3 PduR_RoutingPathType
```c
typedef enum {
    PDUR_ROUTING_PATH_DIRECT = 0,
    PDUR_ROUTING_PATH_FIFO,
    PDUR_ROUTING_PATH_GATEWAY
} PduR_RoutingPathType;
```

### 3.4 PduR_DestPduProcessingType
```c
typedef enum {
    PDUR_DESTPDU_PROCESSING_IMMEDIATE = 0,
    PDUR_DESTPDU_PROCESSING_DEFERRED
} PduR_DestPduProcessingType;
```

### 3.5 PduR_SrcPduConfigType
```c
typedef struct {
    PduIdType SourcePduId;
    uint8 SourceModule;
    PduLengthType SduLength;
} PduR_SrcPduConfigType;
```

### 3.6 PduR_DestPduConfigType
```c
typedef struct {
    PduIdType DestPduId;
    uint8 DestModule;
    PduR_DestPduProcessingType Processing;
    uint8 FifoDepth;
} PduR_DestPduConfigType;
```

### 3.7 PduR_RoutingPathConfigType
```c
typedef struct {
    PduR_SrcPduConfigType SrcPdu;
    const PduR_DestPduConfigType* DestPdus;
    uint8 NumDestPdus;
    PduR_RoutingPathType PathType;
    boolean GatewayOperation;
} PduR_RoutingPathConfigType;
```

### 3.8 PduR_ConfigType
```c
typedef struct {
    const PduR_RoutingPathConfigType* RoutingPaths;
    uint8 NumRoutingPaths;
    const PduR_RoutingPathGroupConfigType* RoutingPathGroups;
    uint8 NumRoutingPathGroups;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} PduR_ConfigType;
```

---

## 4. Error Handling (DET)

When `PDUR_DEV_ERROR_DETECT == STD_ON`, the following development errors are reported via `Det_ReportError`:

| Error Code | Value | Description |
|------------|-------|-------------|
| `PDUR_E_PARAM_POINTER` | 0x01U | Null pointer passed |
| `PDUR_E_PARAM_CONFIG` | 0x02U | Invalid configuration |
| `PDUR_E_INVALID_REQUEST` | 0x03U | Invalid request |
| `PDUR_E_PDU_ID_INVALID` | 0x04U | Invalid PDU ID |
| `PDUR_E_ROUTING_PATH_GROUP_INVALID` | 0x05U | Invalid routing path group |
| `PDUR_E_PARAM_INVALID` | 0x06U | Invalid parameter |
| `PDUR_E_UNINIT` | 0x07U | Module not initialized |
| `PDUR_E_INVALID_BUFFER_LENGTH` | 0x08U | Invalid buffer length |
| `PDUR_E_BUFFER_REQUEST_SDU_FAILED` | 0x09U | Buffer request failed |
| `PDUR_E_BUFFER_REQUEST_SDU_AVAILABLE` | 0x0AU | Buffer request available |
| `PDUR_E_LOIF_TXCONF_WITHOUT_REQ` | 0x20U | TxConfirmation without request |
| `PDUR_E_LOIF_RXIND_WITHOUT_REQ` | 0x21U | RxIndication without request |
| `PDUR_E_LOIF_TRIGGERTRANSMIT_WITHOUT_REQ` | 0x22U | TriggerTransmit without request |

### DET Usage in APIs
- **PduR_Init**: Reports `PDUR_E_PARAM_POINTER` if `ConfigPtr == NULL`
- **PduR_DeInit**: Reports `PDUR_E_UNINIT` if module not initialized
- **PduR_Transmit / PduR_ComTransmit**: Reports `PDUR_E_UNINIT` if not initialized, `PDUR_E_PARAM_POINTER` if `PduInfoPtr == NULL`
- **PduR_RxIndication / PduR_CanIfRxIndication**: Reports `PDUR_E_UNINIT` if not initialized, `PDUR_E_PARAM_POINTER` if `PduInfoPtr == NULL`
- **PduR_TxConfirmation / PduR_CanIfTxConfirmation**: Reports `PDUR_E_UNINIT` if not initialized

---

## 5. Configuration Parameters

### Pre-Compile Configuration (PduR_Cfg.h)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `PDUR_DEV_ERROR_DETECT` | STD_ON | Enable/disable development error detection |
| `PDUR_VERSION_INFO_API` | STD_ON | Enable/disable version info API |
| `PDUR_NUMBER_OF_ROUTING_PATHS` | 16U | Maximum number of routing paths |
| `PDUR_NUMBER_OF_ROUTING_PATH_GROUPS` | 4U | Maximum number of routing path groups |
| `PDUR_MAX_DESTINATIONS_PER_PATH` | 4U | Maximum destinations per routing path |
| `PDUR_FIFO_DEPTH` | 8U | FIFO depth for deferred routing |
| `PDUR_MAIN_FUNCTION_PERIOD_MS` | 10U | Main function period in milliseconds |

### Routing Path Configuration (PduR_Lcfg.c)

Each routing path defines:
- **Source PDU ID**: ID of the PDU at the source module
- **Source Module**: Module ID of the source (e.g., PDUR_MODULE_COM, PDUR_MODULE_CANIF)
- **Destination PDUs**: Array of destination configurations
- **Path Type**: DIRECT, FIFO, or GATEWAY

---

## 6. Scenarios

### Scenario 1: COM to CAN Transmit Routing

**Description:** Engine status signal from COM is routed to CanIf for transmission on the CAN bus.

**Flow:**
1. COM calls `PduR_ComTransmit(PDUR_COM_TX_ENGINE_STATUS, &PduInfo)`
2. PduR looks up routing path for `PDUR_COM_TX_ENGINE_STATUS` with source module `PDUR_MODULE_COM`
3. PduR finds destination module `PDUR_MODULE_CANIF` with `DestPduId = 0`
4. PduR calls `CanIf_Transmit(0, &PduInfo)`
5. CanIf returns `E_OK` or `E_NOT_OK`
6. PduR returns result to COM

**Expected Result:** PDU is successfully forwarded to CanIf_Transmit.

### Scenario 2: CAN Receive to COM Routing

**Description:** Engine command received from CAN bus is routed to COM.

**Flow:**
1. CanIf receives a CAN frame and calls `PduR_CanIfRxIndication(PDUR_COM_RX_ENGINE_CMD, &PduInfo)`
2. PduR looks up routing path for `PDUR_COM_RX_ENGINE_CMD` with source module `PDUR_MODULE_CANIF`
3. PduR finds destination module `PDUR_MODULE_COM` with `DestPduId = PDUR_COM_RX_ENGINE_CMD`
4. PduR calls `Com_RxIndication(PDUR_COM_RX_ENGINE_CMD, &PduInfo)`

**Expected Result:** PDU is successfully forwarded to Com_RxIndication.

### Scenario 3: Transmit Confirmation Routing

**Description:** CAN transmit confirmation is routed back to COM.

**Flow:**
1. CanIf completes transmission and calls `PduR_CanIfTxConfirmation(PDUR_COM_TX_ENGINE_STATUS, E_OK)`
2. PduR looks up routing path for the PDU with source module `PDUR_MODULE_CANIF`
3. PduR finds destination module `PDUR_MODULE_COM`
4. PduR calls `Com_TxConfirmation(PDUR_COM_TX_ENGINE_STATUS, E_OK)`

**Expected Result:** Confirmation is successfully forwarded to Com_TxConfirmation.

### Scenario 4: Uninitialized Module Error Detection

**Description:** API is called before PduR_Init.

**Flow:**
1. Application calls `PduR_ComTransmit(0, &PduInfo)` before `PduR_Init`
2. PduR detects `State != PDUR_STATE_INIT`
3. PduR reports `PDUR_E_UNINIT` to DET
4. PduR returns `E_NOT_OK`

**Expected Result:** Error is reported and API returns `E_NOT_OK`.

---

## 7. Dependencies

### Upper Layer Modules
- **Com**: Communication module for signal-based communication
- **Dcm**: Diagnostic Communication Manager for UDS/OBD diagnostics

### Lower Layer Modules
- **CanIf**: CAN Interface module
- **LinIf**: LIN Interface module (future)
- **EthIf**: Ethernet Interface module (future)

### Common Modules
- **Det**: Development Error Tracer
- **MemMap**: Memory mapping for section placement

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-04-23 | YuleTech | Initial PduR specification |
