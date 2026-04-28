# DoIp (Diagnostic over IP) Module Specification

> **Module:** DoIp (Diagnostic over IP)  
> **Layer:** Service Layer  
> **Standard:** AutoSAR Classic Platform 4.x  
> **Platform:** NXP i.MX8M Mini  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

DoIp is a Service Layer module that implements the ISO 13400-2 (DoIP) protocol for diagnostic communication over Ethernet/IP. It acts as a transport adapter between the DCM (Diagnostic Communication Manager) and the lower-layer Ethernet stack (SoAd / TcpIp), enabling UDS diagnostic sessions over IP networks.

### Key Responsibilities
- **Vehicle Discovery:** Respond to vehicle identification requests (VIN, EID, GID)
- **Routing Activation:** Manage diagnostic routing activation between tester and ECU
- **Diagnostic Message Transport:** Encapsulate UDS diagnostic requests/responses in DoIP frames
- **Alive Check:** Maintain connection liveness via periodic alive check messages
- **Connection Management:** Handle TCP/UDP socket lifecycle for diagnostic sessions

### DoIP Protocol Stack
```
DCM (UDS/OBD)
    |
DoIp (DoIP framing: Generic Header + Payload)
    |
PduR / SoAd (Socket Adapter)
    |
TcpIp / Ethernet
```

---

## 2. API List

### 2.1 Core Lifecycle APIs

| API | Description |
|-----|-------------|
| `DoIp_Init(const DoIp_ConfigType* ConfigPtr)` | Initializes the DoIp module with the given configuration |
| `DoIp_DeInit(void)` | Deinitializes the DoIp module |
| `DoIp_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Retrieves version information of the module |

### 2.2 DCM Interface APIs (Upper Layer)

| API | Called By | Description |
|-----|-----------|-------------|
| `DoIp_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)` | Dcm | Transmits a diagnostic message encapsulated in DoIP frame |
| `DoIp_IfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | SoAd | Receives a raw DoIP frame from lower layer, parses and routes to DCM |
| `DoIp_ActivateRouting(uint16 SourceAddress, uint16 TargetAddress, uint8 ActivationType)` | Dcm / Tester | Activates a diagnostic routing path |
| `DoIp_CloseConnection(uint16 ConnectionId)` | Dcm | Closes an active diagnostic connection |

### 2.3 Lower-Layer Callback APIs

| API | Called By | Description |
|-----|-----------|-------------|
| `DoIp_SoAdRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | SoAd | Raw DoIP frame received from socket adapter |
| `DoIp_SoAdTxConfirmation(PduIdType TxPduId, Std_ReturnType result)` | SoAd | Transmit confirmation from socket adapter |

### 2.4 Periodic Processing APIs

| API | Description |
|-----|-------------|
| `DoIp_MainFunction(void)` | Periodic main function for timeout handling, alive check, and state machine maintenance |

---

## 3. Data Types

### 3.1 DoIp_StateType
```c
typedef enum {
    DOIP_STATE_UNINIT = 0,
    DOIP_STATE_INIT,
    DOIP_STATE_ACTIVE
} DoIp_StateType;
```

### 3.2 DoIp_ConnectionStateType
```c
typedef enum {
    DOIP_CONNECTION_CLOSED = 0,
    DOIP_CONNECTION_PENDING,
    DOIP_CONNECTION_ESTABLISHED,
    DOIP_CONNECTION_REGISTERED
} DoIp_ConnectionStateType;
```

### 3.3 DoIp_RoutingActivationType
```c
typedef enum {
    DOIP_ROUTING_ACTIVATION_DEFAULT = 0x00,
    DOIP_ROUTING_ACTIVATION_WWH_OBD = 0x01,
    DOIP_ROUTING_ACTIVATION_CENTRAL_SECURITY = 0xE0
} DoIp_RoutingActivationType;
```

### 3.4 DoIp_PayloadType
```c
typedef enum {
    DOIP_PAYLOAD_VEHICLE_IDENTIFICATION_REQ        = 0x0001,
    DOIP_PAYLOAD_VEHICLE_IDENTIFICATION_RES        = 0x0002,
    DOIP_PAYLOAD_ROUTING_ACTIVATION_REQ            = 0x0005,
    DOIP_PAYLOAD_ROUTING_ACTIVATION_RES            = 0x0006,
    DOIP_PAYLOAD_ALIVE_CHECK_REQ                   = 0x0007,
    DOIP_PAYLOAD_ALIVE_CHECK_RES                   = 0x0008,
    DOIP_PAYLOAD_DIAGNOSTIC_MESSAGE                = 0x8001,
    DOIP_PAYLOAD_DIAGNOSTIC_MESSAGE_POS_ACK        = 0x8002,
    DOIP_PAYLOAD_DIAGNOSTIC_MESSAGE_NEG_ACK        = 0x8003
} DoIp_PayloadType;
```

### 3.5 DoIp_GenericHeaderType
```c
typedef struct {
    uint8  ProtocolVersion;
    uint8  InverseProtocolVersion;
    uint16 PayloadType;
    uint32 PayloadLength;
} DoIp_GenericHeaderType;
```

### 3.6 DoIp_ConnectionConfigType
```c
typedef struct {
    uint16 ConnectionId;
    uint16 SourceAddress;
    uint16 TargetAddress;
    uint16 TesterLogicalAddress;
    uint16 AliveCheckTimeoutMs;
    uint16 GeneralInactivityTimeoutMs;
    boolean AliveCheckEnabled;
} DoIp_ConnectionConfigType;
```

### 3.7 DoIp_RoutingActivationConfigType
```c
typedef struct {
    uint8  ActivationType;
    uint16 SourceAddress;
    uint16 TargetAddress;
    uint8  NumAuthReqBytes;
    uint8  NumConfirmReqBytes;
    boolean AuthenticationRequired;
    boolean ConfirmationRequired;
} DoIp_RoutingActivationConfigType;
```

### 3.8 DoIp_ConfigType
```c
typedef struct {
    const DoIp_ConnectionConfigType* Connections;
    uint8 NumConnections;
    const DoIp_RoutingActivationConfigType* RoutingActivations;
    uint8 NumRoutingActivations;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    uint8 DoIpVehicleAnnouncementCount;
    uint16 DoIpVehicleAnnouncementInterval;
} DoIp_ConfigType;
```

---

## 4. DoIP Frame Format

### 4.1 Generic DoIP Header (8 bytes)

| Field | Size | Description |
|-------|------|-------------|
| Protocol Version | 1 byte | DoIP protocol version (0x02 for DoIP 2.x) |
| Inverse Protocol Version | 1 byte | Bitwise inverse of protocol version (0xFD) |
| Payload Type | 2 bytes | Type of DoIP payload (big-endian) |
| Payload Length | 4 bytes | Length of payload in bytes (big-endian) |

### 4.2 Diagnostic Message Payload (0x8001)

| Field | Size | Description |
|-------|------|-------------|
| Source Address | 2 bytes | Logical address of the sender |
| Target Address | 2 bytes | Logical address of the receiver |
| User Data | N bytes | UDS diagnostic data |

### 4.3 Routing Activation Request Payload (0x0005)

| Field | Size | Description |
|-------|------|-------------|
| Source Address | 2 bytes | Tester's logical address |
| Activation Type | 1 byte | Routing activation type code |
| Reserved | 4 bytes | ISO reserved (0x00) |
| OEM-specific | N bytes | Optional OEM payload |

### 4.4 Routing Activation Response Payload (0x0006)

| Field | Size | Description |
|-------|------|-------------|
| Tester Logical Address | 2 bytes | Echo of tester's address |
| Target Address | 2 bytes | ECU logical address |
| Response Code | 1 byte | Activation result (0x00 = accepted) |
| Reserved | 4 bytes | ISO reserved (0x00) |

---

## 5. Error Handling (DET)

When `DOIP_DEV_ERROR_DETECT == STD_ON`, the following development errors are reported via `Det_ReportError`:

| Error Code | Value | Description |
|------------|-------|-------------|
| `DOIP_E_PARAM_POINTER` | 0x01U | Null pointer passed |
| `DOIP_E_PARAM_CONFIG` | 0x02U | Invalid configuration |
| `DOIP_E_UNINIT` | 0x03U | Module not initialized |
| `DOIP_E_INVALID_PDU_ID` | 0x04U | Invalid PDU identifier |
| `DOIP_E_INVALID_CONNECTION` | 0x05U | Invalid connection identifier |
| `DOIP_E_INVALID_ROUTING_ACTIVATION` | 0x06U | Invalid routing activation type |
| `DOIP_E_INVALID_PARAMETER` | 0x07U | General invalid parameter |

### DET Usage in APIs
- **DoIp_Init**: Reports `DOIP_E_PARAM_POINTER` if `ConfigPtr == NULL`
- **DoIp_DeInit**: Reports `DOIP_E_UNINIT` if module not initialized
- **DoIp_IfTransmit**: Reports `DOIP_E_UNINIT` if not initialized, `DOIP_E_PARAM_POINTER` if `PduInfoPtr == NULL`
- **DoIp_IfRxIndication**: Reports `DOIP_E_UNINIT` if not initialized, `DOIP_E_PARAM_POINTER` if `PduInfoPtr == NULL`
- **DoIp_ActivateRouting**: Reports `DOIP_E_UNINIT` if not initialized

---

## 6. Configuration Parameters

### Pre-Compile Configuration (DoIp_Cfg.h)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `DOIP_DEV_ERROR_DETECT` | STD_ON | Enable/disable development error detection |
| `DOIP_VERSION_INFO_API` | STD_ON | Enable/disable version info API |
| `DOIP_MAX_CONNECTIONS` | 4U | Maximum number of simultaneous connections |
| `DOIP_MAX_ROUTING_ACTIVATIONS` | 8U | Maximum number of routing activation entries |
| `DOIP_MAX_DIAG_REQUEST_LENGTH` | 4096U | Maximum diagnostic request length |
| `DOIP_MAX_DIAG_RESPONSE_LENGTH` | 4096U | Maximum diagnostic response length |
| `DOIP_MAIN_FUNCTION_PERIOD_MS` | 10U | Main function period in milliseconds |

---

## 7. Scenarios

### Scenario 1: Routing Activation

**Description:** A diagnostic tester connects to the vehicle via Ethernet and requests routing activation to establish a diagnostic session.

**Flow:**
1. Tester sends DoIP Routing Activation Request (Payload Type 0x0005) via TCP
2. SoAd receives the frame and calls `DoIp_SoAdRxIndication()`
3. DoIp parses the Generic Header and extracts Payload Type 0x0005
4. DoIp validates the source address and activation type
5. DoIp calls `DoIp_ActivateRouting(SourceAddress, TargetAddress, ActivationType)` internally
6. DoIp sends Routing Activation Response (Payload Type 0x0006) with response code 0x00 (accepted)
7. Connection state transitions to `DOIP_CONNECTION_REGISTERED`

**Expected Result:** Routing is successfully activated and diagnostic messages can flow.

---

### Scenario 2: Diagnostic Request Sending (UDS over IP)

**Description:** DCM sends a UDS diagnostic request via DoIp to the tester.

**Flow:**
1. DCM prepares a UDS request (e.g., Read DTC Information 0x19)
2. DCM calls `DoIp_IfTransmit(DOIP_DCM_TX_DIAG_REQUEST, &PduInfo)`
3. DoIp checks module is initialized and PDU info is valid
4. DoIp constructs DoIP Generic Header:
   - Protocol Version = 0x02, Inverse = 0xFD
   - Payload Type = 0x8001 (Diagnostic Message)
   - Payload Length = 2 (SA) + 2 (TA) + UDS data length
5. DoIp prepends Source Address and Target Address
6. DoIp forwards the complete frame to SoAd for TCP transmission
7. SoAd returns `E_OK`
8. DoIp returns `E_OK` to DCM

**Expected Result:** UDS request is encapsulated in DoIP frame and transmitted via Ethernet.

---

### Scenario 3: Vehicle Identification Response

**Description:** A tester broadcasts a Vehicle Identification Request, and the ECU responds with its identity.

**Flow:**
1. Tester sends Vehicle Identification Request (Payload Type 0x0001) via UDP multicast
2. SoAd receives the request and calls `DoIp_SoAdRxIndication()`
3. DoIp parses the Generic Header and identifies Payload Type 0x0001
4. DoIp constructs Vehicle Identification Response (Payload Type 0x0002):
   - VIN (17 bytes)
   - Logical Address (2 bytes)
   - EID (6 bytes)
   - GID (6 bytes)
   - Further Action Required (1 byte)
5. DoIp sends the response via SoAd UDP unicast

**Expected Result:** Tester receives the ECU's vehicle identification information.

---

### Scenario 4: Uninitialized Module Error Detection

**Description:** API is called before DoIp_Init.

**Flow:**
1. Application calls `DoIp_IfTransmit(0, &PduInfo)` before `DoIp_Init`
2. DoIp detects `State != DOIP_STATE_INIT`
3. DoIp reports `DOIP_E_UNINIT` to DET
4. DoIp returns `E_NOT_OK`

**Expected Result:** Error is reported and API returns `E_NOT_OK`.

---

## 8. Dependencies

### Upper Layer Modules
- **Dcm**: Diagnostic Communication Manager for UDS protocol handling

### Lower Layer Modules
- **SoAd**: Socket Adapter for TCP/UDP communication over Ethernet
- **PduR**: PDU Router (optional path for DoIP frames)

### Common Modules
- **Det**: Development Error Tracer
- **MemMap**: Memory mapping for section placement

---

## 9. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-04-24 | YuleTech | Initial DoIp specification |
