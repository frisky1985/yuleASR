# DCM (Diagnostic Communication Manager) Module Specification

> **Module:** DCM (Diagnostic Communication Manager)
> **Layer:** Service Layer
> **Standard:** AutoSAR Classic Platform 4.x
> **Platform:** NXP i.MX8M Mini
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

The Diagnostic Communication Manager (DCM) is a Service Layer module that implements UDS (Unified Diagnostic Services, ISO 14229-1) and OBD-II diagnostic protocols. It acts as the interface between the diagnostic tester (external tool) and the ECU internal software components.

### Key Responsibilities

- **UDS Protocol Processing:** Receive, parse, and respond to UDS diagnostic requests
- **Diagnostic Session Management:** Manage diagnostic sessions (Default, Extended, Programming, Safety)
- **Security Access Control:** Implement Seed/Key mechanism for protected services
- **Diagnostic Service Dispatching:** Route service requests to appropriate internal handlers
- **DID/RID Management:** Handle Read/Write Data By Identifier and Routine Control services
- **DTC Information Access:** Interface with DEM to read and clear diagnostic trouble codes
- **Data Transfer:** Support software download/upload via Request Download/Transfer Data/Transfer Exit

---

## 2. UDS Service List

| SID | Service Name | Description |
|-----|--------------|-------------|
| 0x10 | Diagnostic Session Control | Switch between diagnostic sessions |
| 0x11 | ECU Reset | Perform hard/soft/key-off reset |
| 0x19 | Read DTC Information | Query DTC status, freeze frames, extended data |
| 0x22 | Read Data By Identifier | Read ECU data by 2-byte DID |
| 0x2E | Write Data By Identifier | Write ECU data by 2-byte DID |
| 0x31 | Routine Control | Start/stop/request result of routines |
| 0x34 | Request Download | Initiate software download session |
| 0x36 | Transfer Data | Transfer data blocks during download/upload |
| 0x37 | Request Transfer Exit | Terminate data transfer session |
| 0x3E | Tester Present | Keep diagnostic session alive |

---

## 3. API List

### 3.1 Core Lifecycle APIs

| API | Description |
|-----|-------------|
| `Dcm_Init(const Dcm_ConfigType* ConfigPtr)` | Initializes the DCM module |
| `Dcm_DeInit(void)` | Deinitializes the DCM module |
| `Dcm_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Retrieves version information |
| `Dcm_MainFunction(void)` | Periodic main function for timeouts and session management |

### 3.2 Diagnostic Processing APIs

| API | Description |
|-----|-------------|
| `Dcm_ProcessIncomingRequest(uint8 ProtocolId)` | Process a pending incoming diagnostic request |
| `Dcm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)` | Transmit confirmation callback from PduR |
| `Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | Receive indication callback from PduR |
| `Dcm_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)` | Trigger transmit callback from PduR |

### 3.3 Session and Security APIs

| API | Description |
|-----|-------------|
| `Dcm_GetSecurityLevel(uint8* SecLevel)` | Get current security level |
| `Dcm_GetSesCtrlType(uint8* SesCtrlType)` | Get current session control type |
| `Dcm_ResetToDefaultSession(void)` | Reset to default diagnostic session |
| `Dcm_GetActiveDiagnostic(void)` | Get active diagnostic flag |
| `Dcm_SetActiveDiagnostic(boolean active)` | Set active diagnostic flag |

---

## 4. Data Types

### 4.1 Dcm_SesCtrlType
```c
typedef enum {
    DCM_DEFAULT_SESSION = 0x01,
    DCM_PROGRAMMING_SESSION = 0x02,
    DCM_EXTENDED_DIAGNOSTIC_SESSION = 0x03,
    DCM_SAFETY_SYSTEM_DIAGNOSTIC_SESSION = 0x04
} Dcm_SesCtrlType;
```

### 4.2 Dcm_SecLevelType
```c
typedef uint8 Dcm_SecLevelType;
```

### 4.3 Dcm_ProtocolType
```c
typedef enum {
    DCM_OBD_ON_CAN = 0,
    DCM_OBD_ON_FLEXRAY,
    DCM_OBD_ON_IP,
    DCM_UDS_ON_CAN,
    DCM_UDS_ON_FLEXRAY,
    DCM_UDS_ON_IP,
    DCM_NO_PROTOCOL
} Dcm_ProtocolType;
```

### 4.4 Dcm_NegativeResponseCodeType
```c
typedef enum {
    DCM_E_POSITIVERESPONSE = 0x00,
    DCM_E_SERVICENOTSUPPORTED = 0x11,
    DCM_E_SUBFUNCTIONNOTSUPPORTED = 0x12,
    DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT = 0x13,
    DCM_E_REQUESTOUTOFRANGE = 0x31,
    DCM_E_SECURITYACCESSDENIED = 0x33,
    DCM_E_INVALIDKEY = 0x35,
    DCM_E_EXCEEDNUMBEROFATTEMPTS = 0x36,
    DCM_E_REQUIREDTIMEDELAYNOTEXPIRED = 0x37,
    /* ... additional NRCs ... */
} Dcm_NegativeResponseCodeType;
```

### 4.5 Dcm_DIDConfigType
```c
typedef struct {
    uint16 DID;
    uint16 DataLength;
    uint8 SessionType;
    uint8 SecurityLevel;
    Std_ReturnType (*ReadDataFnc)(uint8* Data);
    Std_ReturnType (*WriteDataFnc)(const uint8* Data, uint16 DataLength);
} Dcm_DIDConfigType;
```

### 4.6 Dcm_RIDConfigType
```c
typedef struct {
    uint16 RID;
    uint8 SessionType;
    uint8 SecurityLevel;
    Std_ReturnType (*StartFnc)(const uint8* RequestData, uint16 RequestDataLength, uint8* ResponseData, uint16* ResponseDataLength);
    Std_ReturnType (*StopFnc)(const uint8* RequestData, uint16 RequestDataLength, uint8* ResponseData, uint16* ResponseDataLength);
    Std_ReturnType (*RequestResultFnc)(uint8* ResponseData, uint16* ResponseDataLength);
} Dcm_RIDConfigType;
```

### 4.7 Dcm_ConfigType
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

## 5. Error Handling (DET)

When `DCM_DEV_ERROR_DETECT == STD_ON`, the following development errors are reported via `Det_ReportError`:

| Error Code | Value | Description |
|------------|-------|-------------|
| `DCM_E_UNINIT` | 0x05U | Module not initialized |
| `DCM_E_PARAM` | 0x06U | Invalid parameter |
| `DCM_E_PARAM_POINTER` | 0x07U | Null pointer passed |
| `DCM_E_INIT_FAILED` | 0x08U | Initialization failed |
| `DCM_E_INVALID_VALUE` | 0x09U | Invalid value |
| `DCM_E_INTERFACE_TIMEOUT` | 0x01U | Interface timeout |
| `DCM_E_INTERFACE_RETURN_VALUE` | 0x02U | Interface return value error |
| `DCM_E_INTERFACE_BUFFER_OVERFLOW` | 0x03U | Buffer overflow |

### DET Usage in APIs
- **Dcm_Init**: Reports `DCM_E_PARAM_POINTER` if `ConfigPtr == NULL`
- **Dcm_DeInit**: Reports `DCM_E_UNINIT` if module not initialized
- **Dcm_RxIndication**: Reports `DCM_E_UNINIT` if not initialized, `DCM_E_PARAM_POINTER` if `PduInfoPtr == NULL`
- **Dcm_TxConfirmation**: Reports `DCM_E_UNINIT` if not initialized
- **Dcm_TriggerTransmit**: Reports `DCM_E_UNINIT` if not initialized, `DCM_E_PARAM_POINTER` if `PduInfoPtr == NULL`

---

## 6. Scenarios

### Scenario 1: Default Session Diagnostics

**Description:** A diagnostic tester reads the ECU part number (DID 0xF18C) in the default session.

**Flow:**
1. Tester sends `0x22 0xF1 0x8C` (ReadDataByIdentifier for DID 0xF18C)
2. PduR routes the request to `Dcm_RxIndication`
3. DCM validates the request: service supported, session valid (default session allows 0x22)
4. DCM calls the registered read function for DID 0xF18C
5. DCM builds positive response `0x62 0xF1 0x8C <data...>`
6. DCM calls `PduR_Transmit` to send the response
7. `Dcm_TxConfirmation` is called when transmission completes

**Expected Result:** Positive response with DID data is returned.

### Scenario 2: Extended Session Security Access

**Description:** Tester switches to extended session and performs security access to unlock level 1.

**Flow:**
1. Tester sends `0x10 0x03` (DiagnosticSessionControl -> Extended)
2. DCM switches to extended session, responds `0x50 0x03 <P2 timing>`
3. Tester sends `0x27 0x01` (SecurityAccess -> RequestSeed for level 1)
4. DCM generates seed, responds `0x67 0x01 <seed[4]>`
5. Tester calculates key from seed and sends `0x27 0x02 <key[4]>`
6. DCM validates key, unlocks security level 1, responds `0x67 0x02`
7. Tester can now access security-protected DIDs and RIDs

**Expected Result:** Security level 1 is unlocked successfully.

### Scenario 3: Read DTC Information

**Description:** Tester queries confirmed DTCs stored in the ECU.

**Flow:**
1. Tester sends `0x19 0x02 0x08` (ReadDTCInformation -> reportDTCByStatusMask, ConfirmedDTC)
2. DCM validates the request
3. DCM iterates through configured DTCs via DEM interface
4. DCM collects DTCs with status bit ConfirmedDTC (0x08) set
5. DCM builds response: `0x59 0x02 <availabilityMask> <DTC1[3]> <status1> <DTC2[3]> <status2>...`
6. DCM sends response via PduR

**Expected Result:** Response contains all confirmed DTCs.

### Scenario 4: Software Flash Programming

**Description:** Tester downloads new software to the ECU using the data transfer services.

**Flow:**
1. Tester sends `0x10 0x02` (DiagnosticSessionControl -> Programming)
2. DCM switches to programming session
3. Tester sends `0x34 0x00 0x44 <addr[4]> <size[4]>` (RequestDownload)
4. DCM validates address and size, responds `0x74 <lengthFormat> <maxBlockLength[2]>`
5. Tester sends `0x36 0x01 <data[...]>` (TransferData, block 1)
6. DCM validates block sequence counter, writes data to flash area, responds `0x76 0x01`
7. Steps 5-6 repeat until all data is transferred
8. Tester sends `0x37` (RequestTransferExit)
9. DCM finalizes flash programming, responds `0x77`
10. Tester sends `0x11 0x01` (ECUReset -> HardReset) to boot new software

**Expected Result:** Software is successfully downloaded and ECU resets.

---

## 7. Dependencies

### Upper Layer Modules
- **Application / RTE**: Diagnostic applications and routines

### Lower Layer Modules
- **PduR**: PDU routing for diagnostic message transport
- **CanIf / EthIf / LinIf**: Communication interface modules

### Peer Service Layer Modules
- **Dem**: Diagnostic Event Manager for DTC information
- **NvM**: Non-Volatile Memory for persistent diagnostic data (future)

### Common Modules
- **Det**: Development Error Tracer
- **MemMap**: Memory mapping for section placement

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-04-24 | YuleTech | Initial DCM specification with UDS services and scenarios |
