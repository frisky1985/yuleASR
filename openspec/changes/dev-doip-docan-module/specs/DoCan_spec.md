# DoCan (Diagnostic over CAN) Module Specification

> **Module:** DoCan (Diagnostic over CAN)  
> **Layer:** Service Layer  
> **Standard:** AutoSAR Classic Platform 4.x  
> **Platform:** NXP i.MX8M Mini  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

DoCan is a Service Layer module that acts as a diagnostic transport adapter between the DCM (Diagnostic Communication Manager) and the CanTp (CAN Transport Protocol) module. It encapsulates diagnostic-specific CAN transport logic, providing a clean abstraction for UDS diagnostic communication over CAN.

DoCan simplifies the interface between DCM and CanTp by handling:
- Mapping of DCM PDU IDs to CanTp N-SDU IDs
- Diagnostic session management for CAN transport
- TX confirmation routing from CanTp back to DCM
- RX indication routing from CanTp to DCM

### Diagnostic CAN Stack
```
DCM (UDS/OBD)
    |
DoCan (Diagnostic CAN Adapter)
    |
CanTp (ISO 15765-2 Transport Protocol)
    |
CanIf (CAN Interface)
    |
CAN Controller / CAN Transceiver
```

---

## 2. API List

### 2.1 Core Lifecycle APIs

| API | Description |
|-----|-------------|
| `DoCan_Init(const DoCan_ConfigType* ConfigPtr)` | Initializes the DoCan module with the given configuration |
| `DoCan_DeInit(void)` | Deinitializes the DoCan module |
| `DoCan_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Retrieves version information of the module |

### 2.2 DCM Interface APIs (Upper Layer)

| API | Called By | Description |
|-----|-----------|-------------|
| `DoCan_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)` | Dcm | Transmits a diagnostic message via CanTp |
| `DoCan_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | CanTp | Routes a received diagnostic message to DCM |
| `DoCan_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)` | CanTp | Routes a transmit confirmation to DCM |

### 2.3 Periodic Processing APIs

| API | Description |
|-----|-------------|
| `DoCan_MainFunction(void)` | Periodic main function for timeout handling and state maintenance |

---

## 3. Data Types

### 3.1 DoCan_StateType
```c
typedef enum {
    DOCAN_STATE_UNINIT = 0,
    DOCAN_STATE_INIT
} DoCan_StateType;
```

### 3.2 DoCan_ChannelType
```c
typedef enum {
    DOCAN_CHANNEL_PHYSICAL = 0,
    DOCAN_CHANNEL_FUNCTIONAL
} DoCan_ChannelType;
```

### 3.3 DoCan_ChannelStateType
```c
typedef enum {
    DOCAN_CHANNEL_IDLE = 0,
    DOCAN_CHANNEL_TX_IN_PROGRESS,
    DOCAN_CHANNEL_RX_IN_PROGRESS
} DoCan_ChannelStateType;
```

### 3.4 DoCan_PduMappingType
```c
typedef struct {
    PduIdType DoCanPduId;       /* DCM-facing PDU ID */
    PduIdType CanTpPduId;       /* CanTp N-SDU ID */
    DoCan_ChannelType ChannelType;
    boolean TxConfirmationEnabled;
    boolean RxIndicationEnabled;
} DoCan_PduMappingType;
```

### 3.5 DoCan_ChannelConfigType
```c
typedef struct {
    uint8 ChannelId;
    DoCan_ChannelType ChannelType;
    PduIdType TxPduId;
    PduIdType RxPduId;
    uint16 TimeoutMs;
} DoCan_ChannelConfigType;
```

### 3.6 DoCan_ConfigType
```c
typedef struct {
    const DoCan_PduMappingType* PduMappings;
    uint8 NumPduMappings;
    const DoCan_ChannelConfigType* ChannelConfigs;
    uint8 NumChannels;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} DoCan_ConfigType;
```

---

## 4. Error Handling (DET)

When `DOCAN_DEV_ERROR_DETECT == STD_ON`, the following development errors are reported via `Det_ReportError`:

| Error Code | Value | Description |
|------------|-------|-------------|
| `DOCAN_E_PARAM_POINTER` | 0x01U | Null pointer passed |
| `DOCAN_E_PARAM_CONFIG` | 0x02U | Invalid configuration |
| `DOCAN_E_UNINIT` | 0x03U | Module not initialized |
| `DOCAN_E_INVALID_PDU_ID` | 0x04U | Invalid PDU identifier |
| `DOCAN_E_INVALID_CHANNEL` | 0x05U | Invalid channel identifier |
| `DOCAN_E_TX_FAILED` | 0x06U | Transmission failed |

### DET Usage in APIs
- **DoCan_Init**: Reports `DOCAN_E_PARAM_POINTER` if `ConfigPtr == NULL`
- **DoCan_DeInit**: Reports `DOCAN_E_UNINIT` if module not initialized
- **DoCan_Transmit**: Reports `DOCAN_E_UNINIT` if not initialized, `DOCAN_E_PARAM_POINTER` if `PduInfoPtr == NULL`
- **DoCan_RxIndication**: Reports `DOCAN_E_UNINIT` if not initialized, `DOCAN_E_PARAM_POINTER` if `PduInfoPtr == NULL`
- **DoCan_TxConfirmation**: Reports `DOCAN_E_UNINIT` if not initialized

---

## 5. Configuration Parameters

### Pre-Compile Configuration (DoCan_Cfg.h)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `DOCAN_DEV_ERROR_DETECT` | STD_ON | Enable/disable development error detection |
| `DOCAN_VERSION_INFO_API` | STD_ON | Enable/disable version info API |
| `DOCAN_MAX_CHANNELS` | 4U | Maximum number of diagnostic CAN channels |
| `DOCAN_MAX_PDU_MAPPINGS` | 8U | Maximum number of PDU mappings |
| `DOCAN_MAIN_FUNCTION_PERIOD_MS` | 10U | Main function period in milliseconds |

### PDU IDs (Example)

| Define | Value | Description |
|--------|-------|-------------|
| `DOCAN_DCM_TX_DIAG_PHYSICAL` | 0U | DCM TX physical addressing |
| `DOCAN_DCM_TX_DIAG_FUNCTIONAL` | 1U | DCM TX functional addressing |
| `DOCAN_DCM_RX_DIAG_PHYSICAL` | 2U | DCM RX physical addressing |
| `DOCAN_DCM_RX_DIAG_FUNCTIONAL` | 3U | DCM RX functional addressing |

---

## 6. Scenarios

### Scenario 1: Diagnostic Request Sending (Physical Addressing)

**Description:** DCM sends a UDS diagnostic request (physical addressing) via DoCan to CanTp.

**Flow:**
1. DCM prepares a UDS request (e.g., Security Access 0x27)
2. DCM calls `DoCan_Transmit(DOCAN_DCM_TX_DIAG_PHYSICAL, &PduInfo)`
3. DoCan checks module is initialized and PDU info is valid
4. DoCan looks up PDU mapping for `DOCAN_DCM_TX_DIAG_PHYSICAL`
5. DoCan finds corresponding CanTp N-SDU ID: `CANTP_TX_DIAG_PHYSICAL`
6. DoCan calls `CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &PduInfo)`
7. CanTp returns `E_OK`
8. DoCan returns `E_OK` to DCM

**Expected Result:** UDS request is forwarded to CanTp for ISO-TP transmission.

---

### Scenario 2: Diagnostic Response Reception

**Description:** A diagnostic response is received from the CAN bus and routed to DCM.

**Flow:**
1. CAN frame is received and passed up through CanIf to CanTp
2. CanTp completes reassembly of the diagnostic message
3. CanTp calls `DoCan_RxIndication(CANTP_RX_DIAG_PHYSICAL, &PduInfo)`
4. DoCan checks module is initialized
5. DoCan looks up PDU mapping for the CanTp RX PDU ID
6. DoCan finds corresponding DCM PDU ID: `DOCAN_DCM_RX_DIAG_PHYSICAL`
7. DoCan calls `Dcm_RxIndication(DOCAN_DCM_RX_DIAG_PHYSICAL, &PduInfo)`

**Expected Result:** Diagnostic response is successfully forwarded to DCM.

---

### Scenario 3: Transmit Confirmation

**Description:** CanTp confirms that a diagnostic request has been successfully transmitted on the CAN bus.

**Flow:**
1. CanTp completes transmission of all CAN frames for a diagnostic message
2. CanTp calls `DoCan_TxConfirmation(CANTP_TX_DIAG_PHYSICAL, E_OK)`
3. DoCan checks module is initialized
4. DoCan looks up PDU mapping for the CanTp TX PDU ID
5. DoCan finds corresponding DCM PDU ID: `DOCAN_DCM_TX_DIAG_PHYSICAL`
6. DoCan calls `Dcm_TxConfirmation(DOCAN_DCM_TX_DIAG_PHYSICAL, E_OK)`

**Expected Result:** DCM is notified that the diagnostic request was successfully transmitted.

---

### Scenario 4: Uninitialized Module Error Detection

**Description:** API is called before DoCan_Init.

**Flow:**
1. Application calls `DoCan_Transmit(0, &PduInfo)` before `DoCan_Init`
2. DoCan detects `State != DOCAN_STATE_INIT`
3. DoCan reports `DOCAN_E_UNINIT` to DET
4. DoCan returns `E_NOT_OK`

**Expected Result:** Error is reported and API returns `E_NOT_OK`.

---

## 7. Dependencies

### Upper Layer Modules
- **Dcm**: Diagnostic Communication Manager for UDS protocol handling

### Lower Layer Modules
- **CanTp**: CAN Transport Protocol module (ISO 15765-2)

### Common Modules
- **Det**: Development Error Tracer
- **MemMap**: Memory mapping for section placement

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-04-24 | YuleTech | Initial DoCan specification |
