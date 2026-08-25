> **Module ID**: 0x29  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_DiagnosticCommunicationManager  
> **Source Path**: `src/bsw/services/dcm/`  
> **Reference Document**: `docs/modules/DCM.md`  
> **Doc Version**: 1.0  
> **Status**: Approved

# Dcm (Diagnostic Communication Manager) Design Document

## 1. Module Overview

The Diagnostic Communication Manager (Dcm) is a Service Layer module that implements the UDS (ISO 14229-1) and OBD-II diagnostic protocols on top of the PDU Router (`PduR`). It receives diagnostic requests, dispatches them to the appropriate service handler, enforces session and security access control, builds positive/negative responses, and returns them to the tester via `PduR` → `CanTp`/`CanIf`.

### 1.1 Position in the Stack

```
┌─────────────────────────────────────────────────────────┐
│  Diagnostic Tester (external)                           │
├─────────────────────────────────────────────────────────┤
│  CanTp / DoIP (Transport)                               │
├─────────────────────────────────────────────────────────┤
│  PduR (Services)                                        │
├─────────────────────────────────────────────────────────┤
│  Dcm (Services)                                         │
├─────────────────────────────────────────────────────────┤
│  Dem / NvM / BswM / Application (data sources)          │
└─────────────────────────────────────────────────────────┘
```

---

## 2. Standards & Dependencies

### 2.1 Standards

| Standard | Version | Description |
|----------|---------|-------------|
| AUTOSAR SWS Diagnostic Communication Manager | 4.4.0 | Dcm specification |
| ISO 14229-1 | 2020 | Unified Diagnostic Services (UDS) |
| ISO 15031-5 / SAE J1979 | — | OBD-II diagnostic services |

### 2.2 Dependencies

| Module | Direction | Purpose |
|--------|-----------|---------|
| PduR | Upper/Lower | Receives requests and sends responses |
| Dem | Peer | Reads DTC status for service 0x19, clears DTCs for 0x14 |
| Det | Common | Development error detection |
| NvM / Application | Peer | DID/RID data sources and write targets |

---

## 3. Architecture Design

### 3.1 Internal Components

| Component | Responsibility |
|-----------|----------------|
| Request Dispatcher | Parses incoming diagnostic request, extracts SID, and routes to service handler |
| Service Handlers | Implements UDS services 0x10–0x3E, 0x22/0x2E, 0x19/0x14, 0x31, 0x34–0x37 |
| Session Manager | Tracks current diagnostic session, S3 timeout, default-session fallback |
| Security Manager | Seed/key access control, attempt counter, delay timer |
| DID/RID Manager | Looks up configured Data Identifiers and Routine Identifiers |
| Response Builder | Constructs positive (SID+0x40) and negative (0x7F + SID + NRC) responses |
| Transfer Manager | Handles UDS RequestDownload / TransferData / RequestTransferExit |
| OBD Handler | OBD-II services 0x01, 0x03, 0x04, 0x09 (stub interface) |
| Timer Manager | Decrements P2, S3, and security delay timers in `Dcm_MainFunction` |

### 3.2 File Structure

```
src/bsw/services/dcm/
├── include/
│   ├── Dcm.h              # Public API, UDS SIDs, NRCs, session/security types
│   ├── Dcm_Cfg.h          # Pre-compile configuration counts and sizes
│   ├── Dcm_Obd.h          # OBD-II service interface
│   └── dcm_transfer.h     # UDS transfer services (0x34–0x37) types and callbacks
└── src/
    ├── Dcm.c              # Core DCM service dispatcher and handlers
    ├── Dcm_Obd.c          # OBD-II service implementations
    ├── dcm_transfer.c     # Transfer service helper functions
    └── Dcm_test.c         # Unit tests
```

---

## 4. State Machines

### 4.1 Module State

```
UNINIT ──Dcm_Init──► INIT ──Dcm_DeInit──► UNINIT
```

- `DCM_STATE_UNINIT`: module not initialized; most APIs report `DCM_E_UNINIT`.
- `DCM_STATE_INIT`: module ready; requests can be accepted.
- `DCM_STATE_BUSY`: reserved for future multi-request handling.

### 4.2 Protocol State (per protocol instance)

```
IDLE ──RxIndication──► RX_IN_PROGRESS ──ProcessRequest──► PROCESSING
                            │
                            ▼
               SendPositive/NegativeResponse
                            │
                            ▼
                    TX_IN_PROGRESS ──TxConfirmation──► IDLE
```

- `DCM_PROTOCOL_IDLE`: ready for new request.
- `DCM_PROTOCOL_RX_IN_PROGRESS`: request copied into Rx buffer.
- `DCM_PROTOCOL_PROCESSING`: request parsed and service handler invoked.
- `DCM_PROTOCOL_TX_IN_PROGRESS`: response dispatched, awaiting `PduR` confirmation.

### 4.3 Session State

```
DEFAULT_SESSION ◄────S3 timeout──── EXTENDED_DIAGNOSTIC_SESSION
       ▲                    ▲
       └────── ECU reset ───┘
```

- Default session after initialization and after S3 timeout.
- Security level returns to locked when session falls back to default.

---

## 5. Core Data Structures

### 5.1 Protocol Runtime State

```c
typedef struct
{
    uint8 State;
    uint8 CurrentSID;
    uint8 CurrentSubFunction;
    uint16 RxDataLength;
    uint16 TxDataLength;
    uint8 RxBuffer[DCM_RX_BUFFER_SIZE];
    uint8 TxBuffer[DCM_TX_BUFFER_SIZE];
    uint32 P2Timer;
    uint32 S3Timer;
    boolean ResponsePending;
} Dcm_ProtocolStateType;
```

### 5.2 Module Internal State

```c
typedef struct
{
    uint8 State;
    const Dcm_ConfigType* ConfigPtr;
    uint8 CurrentSession;
    uint8 CurrentSecurityLevel;
    uint8 SecurityAttempts;
    uint32 SecurityDelayTimer;
    boolean SecurityDelayActive;
    Dcm_ProtocolStateType ProtocolStates[DCM_NUM_PROTOCOLS];
    uint32 DownloadAddress;
    uint32 DownloadSize;
    uint32 TransferOffset;
    uint8 BlockSequenceCounter;
    boolean TransferActive;
} Dcm_InternalStateType;
```

### 5.3 DID / RID Configuration

```c
typedef struct {
    uint16 DID;
    uint16 DataLength;
    uint8 SessionType;
    uint8 SecurityLevel;
    Std_ReturnType (*ReadDataFnc)(uint8* Data);
    Std_ReturnType (*WriteDataFnc)(const uint8* Data, uint16 DataLength);
} Dcm_DIDConfigType;

typedef struct {
    uint16 RID;
    uint8 SessionType;
    uint8 SecurityLevel;
    Std_ReturnType (*StartFnc)(const uint8* RequestData, uint16 RequestDataLength,
                               uint8* ResponseData, uint16* ResponseDataLength);
    Std_ReturnType (*StopFnc)(const uint8* RequestData, uint16 RequestDataLength,
                              uint8* ResponseData, uint16* ResponseDataLength);
    Std_ReturnType (*RequestResultFnc)(uint8* ResponseData, uint16* ResponseDataLength);
} Dcm_RIDConfigType;
```

### 5.4 Root Configuration

```c
typedef struct {
    uint8 NumProtocols;
    uint8 NumConnections;
    uint8 NumRxPduIds;
    uint8 NumTxPduIds;
    uint8 NumSessions;
    uint8 NumSecurityLevels;
    uint8 NumServices;
    uint8 NumDIDs;
    uint8 NumRIDs;
    const Dcm_DIDConfigType* DIDs;
    const Dcm_RIDConfigType* RIDs;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean RespondAllRequest;
    boolean DcmTaskTime;
} Dcm_ConfigType;
```

---

## 6. API Design

### 6.1 Public Interfaces

| API | Signature | Function | SID | SWS 需求 |
|-----|-----------|----------|-----|---------|
| `Dcm_Init` | `void Dcm_Init(const Dcm_ConfigType* ConfigPtr)` | Initialize DCM | 0x01 | SWS_Dcm_00001 |
| `Dcm_DeInit` | `void Dcm_DeInit(void)` | Deinitialize DCM | 0x06 | SWS_Dcm_00002 |
| `Dcm_Start` | `void Dcm_Start(void)` | Start DCM | 0x02 | SWS_Dcm_00011 |
| `Dcm_Stop` | `void Dcm_Stop(void)` | Stop DCM | 0x03 | SWS_Dcm_00012 |
| `Dcm_GetVersionInfo` | `void Dcm_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Version info | 0x04 | SWS_Dcm_00010 |
| `Dcm_MainFunction` | `void Dcm_MainFunction(void)` | Periodic processing | 0x05 | SWS_Dcm_00003 |
| `Dcm_GetSecurityLevel` | `Std_ReturnType Dcm_GetSecurityLevel(uint8* SecLevel)` | Get security level | 0x0D | SWS_Dcm_00007 |
| `Dcm_GetSesCtrlType` | `Std_ReturnType Dcm_GetSesCtrlType(uint8* SesCtrlType)` | Get session | 0x0E | SWS_Dcm_00008 |
| `Dcm_GetActiveDiagnostic` | `boolean Dcm_GetActiveDiagnostic(void)` | Diagnostic active flag | 0x0F | SWS_Dcm_00013 |
| `Dcm_SetActiveDiagnostic` | `void Dcm_SetActiveDiagnostic(boolean active)` | Set active flag | 0x10 | SWS_Dcm_00014 |
| `Dcm_ResetToDefaultSession` | `void Dcm_ResetToDefaultSession(void)` | Force default session | 0x11 | SWS_Dcm_00009 |

### 6.2 PduR Callbacks

| Function | Trigger | Purpose |
|----------|---------|---------|
| `Dcm_RxIndication` | `PduR_RxIndication` | Accept diagnostic request and dispatch service handler |
| `Dcm_TxConfirmation` | `PduR_TxConfirmation` | Return protocol state to idle after response sent |
| `Dcm_TriggerTransmit` | `PduR_TriggerTransmit` | Provide already-built Tx buffer to PduR |

### 6.3 Service IDs and DET Error Codes

| SID | API | Main DET Codes |
|-----|-----|----------------|
| 0x01 | Init | `DCM_E_PARAM_POINTER` |
| 0x02 | Start | — |
| 0x03 | Stop | — |
| 0x04 | GetVersionInfo | `DCM_E_PARAM_POINTER` |
| 0x05 | MainFunction | — |
| 0x06 | DeInit | `DCM_E_UNINIT` |
| 0x0D | GetSecurityLevel | `DCM_E_UNINIT`, `DCM_E_PARAM_POINTER` |
| 0x0E | GetSesCtrlType | `DCM_E_UNINIT`, `DCM_E_PARAM_POINTER` |
| 0x40 | TxConfirmation | `DCM_E_UNINIT` |
| 0x42 | RxIndication | `DCM_E_UNINIT`, `DCM_E_PARAM_POINTER` |

Key DET error codes:

| Code | Name | Meaning |
|------|------|---------|
| 0x01 | `DCM_E_INTERFACE_TIMEOUT` | Lower-layer timeout |
| 0x02 | `DCM_E_INTERFACE_RETURN_VALUE` | Unexpected return value |
| 0x03 | `DCM_E_INTERFACE_BUFFER_OVERFLOW` | Buffer overflow |
| 0x05 | `DCM_E_UNINIT` | Module not initialized |
| 0x06 | `DCM_E_PARAM` | Invalid parameter |
| 0x07 | `DCM_E_PARAM_POINTER` | NULL pointer argument |
| 0x08 | `DCM_E_INIT_FAILED` | Initialization failed |
| 0x09 | `DCM_E_INVALID_VALUE` | Invalid value |

---

## 7. Processing Flows

### 7.1 Request Reception (`Dcm_RxIndication`)

1. Validate module state and PDU pointer (DET).
2. If `RxPduId < DCM_NUM_PROTOCOLS`, copy request into the protocol Rx buffer.
3. Set protocol state to `DCM_PROTOCOL_PROCESSING`.
4. Call `Dcm_ProcessRequest(RxPduId)`.

### 7.2 Service Dispatch (`Dcm_ProcessRequest`)

```c
serviceId = RxBuffer[0];
switch (serviceId) {
    case 0x10: Dcm_ProcessDiagnosticSessionControl(...); break;
    case 0x11: Dcm_ProcessEcuReset(...); break;
    case 0x27: Dcm_ProcessSecurityAccess(...); break;
    case 0x3E: Dcm_ProcessTesterPresent(...); break;
    case 0x22: Dcm_ProcessReadDataByIdentifier(...); break;
    case 0x2E: Dcm_ProcessWriteDataByIdentifier(...); break;
    case 0x19: Dcm_ProcessReadDTCInformation(...); break;
    case 0x14: Dcm_ProcessClearDiagnosticInformation(...); break;
    case 0x31: Dcm_ProcessRoutineControl(...); break;
    case 0x34: Dcm_ProcessRequestDownload(...); break;
    case 0x36: Dcm_ProcessTransferData(...); break;
    case 0x37: Dcm_ProcessRequestTransferExit(...); break;
    default: Dcm_SendNegativeResponse(..., DCM_E_SERVICE_NOT_SUPPORTED);
}
```

### 7.3 Positive Response Construction

```c
TxBuffer[0] = SID + 0x40;   /* positive response SID */
TxBuffer[1..n] = response data;
TxDataLength = n + 1;
PduR_Transmit(ProtocolId, &pduInfo);
State = DCM_PROTOCOL_TX_IN_PROGRESS;
```

### 7.4 Negative Response Construction

```c
TxBuffer[0] = 0x7F;         /* negative response service */
TxBuffer[1] = SID;
TxBuffer[2] = NRC;
TxDataLength = 3;
PduR_Transmit(ProtocolId, &pduInfo);
```

### 7.5 DID Read/Write Access Control

For each DID/RID request:

1. Parse identifier (big-endian, 2 bytes).
2. Find entry in `Dcm_Config.DIDs` / `Dcm_Config.RIDs`.
3. Verify `CurrentSecurityLevel >= config.SecurityLevel`.
4. Verify `config.SessionType == default` or `CurrentSession == config.SessionType`.
5. Call configured application callback (`ReadDataFnc` / `WriteDataFnc` / `StartFnc` …).
6. Build positive response or return NRC (`REQUEST_OUT_OF_RANGE`, `SECURITY_ACCESS_DENIED`, `CONDITIONS_NOT_CORRECT`).

### 7.6 Read DTC Information (0x19)

Supported sub-functions:

| SubFunction | Description |
|-------------|-------------|
| 0x01 / 0x12 | Report number of DTCs by status mask |
| 0x02 / 0x13 | Report DTCs by status mask |
| 0x06 | Report DTC extended data record by DTC number |
| 0x0A | Report supported DTCs |

The implementation iterates over `Dem_Config.DtcParameters`, calls `Dem_GetDTCStatus`, and filters by the provided status mask.

### 7.7 Transfer Services (0x34/0x36/0x37)

1. **Request Download (0x34)**: decode address/size format, store download parameters, set `TransferActive`, return `maxNumberOfBlockLength`.
2. **Transfer Data (0x36)**: validate block sequence counter, update `TransferOffset`, acknowledge block.
3. **Request Transfer Exit (0x37)**: clear `TransferActive` and reset block sequence counter.

### 7.8 Periodic Processing (`Dcm_MainFunction`)

For every configured protocol:

1. **S3 timer**: decrement; on expiry reset session to default and lock security.
2. **Security delay timer**: decrement; on expiry clear attempt counter.
3. **P2 timer**: decrement; on expiry send `NRC 0x78` if `ResponsePending` is set.

---

## 8. Configuration Design

### 8.1 Pre-compile Configuration (`Dcm_Cfg.h`)

| Macro | Value | Description |
|-------|-------|-------------|
| `DCM_DEV_ERROR_DETECT` | `STD_ON` | DET enabled |
| `DCM_VERSION_INFO_API` | `STD_ON` | Version info API |
| `DCM_NUM_PROTOCOLS` | 2 | Number of protocols |
| `DCM_NUM_CONNECTIONS` | 4 | Number of connections |
| `DCM_NUM_RX_PDU_IDS` | 4 | Rx PDU IDs |
| `DCM_NUM_TX_PDU_IDS` | 4 | Tx PDU IDs |
| `DCM_NUM_SESSIONS` | 4 | Supported sessions |
| `DCM_NUM_SECURITY_LEVELS` | 3 | Supported security levels |
| `DCM_NUM_DIDS` | 32 | Max DIDs |
| `DCM_NUM_RIDS` | 8 | Max RIDs |
| `DCM_RX_BUFFER_SIZE` | 256 | Per-protocol Rx buffer |
| `DCM_TX_BUFFER_SIZE` | 256 | Per-protocol Tx buffer |
| `DCM_RESPOND_ALL_REQUEST` | `STD_ON` | Respond to all requests |
| `DCM_OBD_SUPPORT` | `STD_ON` | OBD-II enabled |
| `DCM_ROUTINE_CONTROL_SUPPORT` | `STD_ON` | Routine control enabled |
| `DCM_DATA_TRANSFER_SUPPORT` | `STD_ON` | Data transfer enabled |
| `DCM_TRANSFER_BLOCK_SIZE` | 1024 | Default transfer block size |
| `DCM_SEED_SIZE` / `DCM_KEY_SIZE` | 4 | Security seed/key size |
| `DCM_MAX_SECURITY_ATTEMPTS` | 3 | Max failed security attempts |
| `DCM_SECURITY_DELAY_TIME_MS` | 10000 | Lockout delay after max attempts |
| `DCM_P2SERVER_MAX` | 50 ms | P2 server max |
| `DCM_P2STAR_SERVER_MAX` | 5000 ms | P2* server max |
| `DCM_S3SERVER` | 5000 ms | S3 session timeout |
| `DCM_MAIN_FUNCTION_PERIOD_MS` | 10 ms | MainFunction period |

### 8.2 Link-time Configuration

- `DIDs[]` — array of `Dcm_DIDConfigType` with read/write function pointers.
- `RIDs[]` — array of `Dcm_RIDConfigType` with start/stop/result function pointers.
- `Dcm_Config` — root configuration exported to `Dcm_Init`.

---

## 9. Error Handling & Safety

### 9.1 Negative Response Codes (NRC)

Key NRCs used by Dcm:

| Code | Name | Typical Use |
|------|------|-------------|
| 0x10 | `GENERALREJECT` | General rejection |
| 0x11 | `SERVICENOTSUPPORTED` | Unknown SID |
| 0x12 | `SUBFUNCTIONNOTSUPPORTED` | Unknown sub-function |
| 0x13 | `INCORRECTMESSAGELENGTHORINVALIDFORMAT` | Length/format error |
| 0x31 | `REQUESTOUTOFRANGE` | DID/RID not found |
| 0x33 | `SECURITYACCESSDENIED` | Security level too low |
| 0x35 | `INVALIDKEY` | Wrong security key |
| 0x36 | `EXCEEDNUMBEROFATTEMPTS` | Too many failed attempts |
| 0x37 | `REQUIREDTIMEDELAYNOTEXPIRED` | Delay still active |
| 0x78 | `RESPONSEPENDING` | Response will be sent later |
| 0x7E | `SUBFUNCTIONNOTSUPPORTEDINACTIVESESSION` | Wrong session |
| 0x7F | `SERVICENOTSUPPORTEDINACTIVESESSION` | Wrong session |

### 9.2 Security Mechanisms

- Seed/key access with configurable `DCM_SEED_SIZE` / `DCM_KEY_SIZE`.
- Attempt counter incremented on invalid key; lockout triggered after `DCM_MAX_SECURITY_ATTEMPTS`.
- Lockout timer decremented in `Dcm_MainFunction`.
- Security level reset when session returns to default.

### 9.3 Safety Notes

- All application callbacks are invoked only after session/security checks.
- Buffer lengths are bounded by `DCM_RX_BUFFER_SIZE` / `DCM_TX_BUFFER_SIZE`.
- Transfer services validate block sequence counters and transfer-active state.

---

## 10. Memory & Performance

### 10.1 MemMap Sections

| Section | Usage |
|---------|-------|
| `DCM_START_SEC_VAR_CLEARED_UNSPECIFIED` | `Dcm_InternalState` and protocol runtime |
| `DCM_START_SEC_CONFIG_DATA_UNSPECIFIED` | Link-time configuration tables |
| `DCM_START_SEC_CODE` | Module code |

### 10.2 Resource Estimation

| Resource | Estimate | Note |
|----------|----------|------|
| RAM | `DCM_NUM_PROTOCOLS * (DCM_RX_BUFFER_SIZE + DCM_TX_BUFFER_SIZE + overhead)` | Protocol buffers dominate |
| ROM | Code + DID/RID/config tables | Depends on configured services |
| Stack | Low | No recursion; local response buffers up to `DCM_TX_BUFFER_SIZE` |
| CPU | Periodic in `Dcm_MainFunction` every 10 ms | Linear in protocol count |

---

## 11. Integration Guide

### 11.1 Upper Tester Path

Tester → CanTp/DoIP → PduR → `Dcm_RxIndication`

### 11.2 Response Path

Dcm → `PduR_Transmit` → PduR → CanTp/DoIP → Tester
`Dcm_TxConfirmation` releases the protocol state.

### 11.3 Data Source Integration

- DIDs/RIDs link to application functions at configuration time.
- DTC information is fetched from `Dem_GetDTCStatus` using `Dem_Config.DtcParameters`.

### 11.4 Initialization Order

1. Initialize `PduR`, `Dem`, and transport layers (`CanTp` / `DoIP`).
2. Call `Dcm_Init(&Dcm_Config)`.
3. Call `Dcm_Start()` (if required by integration).
4. Cyclically call `Dcm_MainFunction()`.

---

## 12. Test Strategy

### 12.1 Unit Tests

| Test File | Coverage |
|-----------|----------|
| `Dcm_test.c` | Init/DeInit, version info, session/security timers |
| Service tests | Each UDS service handler, positive and negative paths |
| DID/RID tests | Access control, callback success/failure |
| Transfer tests | 0x34/0x36/0x37 sequence, BSC errors |

### 12.2 Integration Tests

| Scenario | Purpose |
|----------|---------|
| Full diagnostic session flow | Session control → security unlock → DID read/write |
| DTC read/clear | Verify Dem integration and status-mask filtering |
| Flash download | RequestDownload → multiple TransferData → RequestTransferExit |
| S3 timeout | Confirm return to default session and locked security |
| Security lockout | Verify delay timer after max failed attempts |

---

## 13. Implementation Notes / TODO

- **OBD-II**: `Dcm_Obd.h` defines service prototypes for 0x01/0x03/0x04/0x09; the current integration dispatches only UDS SIDs in `Dcm_ProcessRequest`. OBD requests should be routed to `Dcm_ObdServiceXX` when `DCM_OBD_SUPPORT` is enabled.
- **Request Upload (0x35)**: declared in `dcm_transfer.h` but not yet dispatched from the main service handler.
- **Seed/Key algorithm**: the current security access uses a fixed seed pattern (`0xA5 + i`) and identical key; a production system must replace this with a cryptographically secure algorithm via Csm/KeyM.
- **P2/P2* handling**: `P2Timer` is decremented, but the full P2* pending-response mechanism (`NRC 0x78` repeated) is simplified.
- **Compression/encryption**: `dcm_transfer.h` defines compression/encryption method IDs and callbacks; actual decompression/decryption is not implemented.
- **Memory write**: `Dcm_ProcessTransferData` updates counters only; the actual flash write callback (`Dcm_TransferCallbacks.WriteMemory`) is not yet invoked.
- **Multiple connections**: the code indexes protocol state by PDU ID; connection-to-protocol mapping may need extension for complex topologies.

---

## 14. References

1. AUTOSAR_SWS_DiagnosticCommunicationManager.pdf — AUTOSAR Classic Platform 4.4.0
2. ISO 14229-1:2020 — Unified Diagnostic Services (UDS)
3. ISO 15031-5 / SAE J1979 — OBD-II diagnostic services
4. `docs/modules/DCM.md`
5. `src/bsw/services/dcm/include/Dcm.h`
6. `src/bsw/services/dcm/include/Dcm_Cfg.h`
7. `src/bsw/services/dcm/include/dcm_transfer.h`
8. `src/bsw/services/dcm/src/Dcm.c`

## 需求追溯表

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Dcm | — | DCM 模块级需求归属 |
