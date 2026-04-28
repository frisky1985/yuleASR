# Com (Communication) Module Specification

> **Module:** Com (Communication)  
> **Layer:** Service Layer  
> **Standard:** AutoSAR Classic Platform 4.x  
> **Platform:** NXP i.MX8M Mini  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

Com is a core Service Layer module in the AutoSAR BSW stack. It provides signal-based communication services for sending and receiving signals and signal groups between application software components (via RTE) and the underlying PDU-based transport layer (via PduR).

Com abstracts the PDU-based communication of the lower layers into a signal-based interface that is convenient for application developers.

### Key Responsibilities
- Pack/unpack signals into/from I-PDUs with configurable bit positions and sizes
- Support signal and signal group transmission and reception
- Support different transmission modes: DIRECT, PERIODIC, MIXED, NONE
- Support filter algorithms for signal transmission optimization
- Support endianness conversion (LITTLE_ENDIAN, BIG_ENDIAN, OPAQUE)
- Support update bits to track signal freshness
- Support I-PDU group control for enabling/disabling communication groups
- Support deadline monitoring (reception DM) for timeout detection
- Support notification callbacks (ComNotification) for signal and I-PDU events
- Support signal gateway routing between different I-PDUs

---

## 2. API List

### 2.1 Core Lifecycle APIs

| API | Description |
|-----|-------------|
| `Com_Init(const Com_ConfigType* config)` | Initializes the COM module with the given configuration |
| `Com_DeInit(void)` | De-initializes the COM module |
| `Com_GetStatus(void)` | Returns the current status of the COM module |
| `Com_GetConfigurationId(void)` | Returns the configuration ID |
| `Com_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Retrieves version information of the module |

### 2.2 Signal APIs

| API | Description |
|-----|-------------|
| `Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)` | Sends a signal by packing it into the associated I-PDU |
| `Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr)` | Receives a signal by unpacking it from the associated I-PDU |
| `Com_InvalidateSignal(Com_SignalIdType SignalId)` | Invalidates a signal by setting its value to the configured invalid value |

### 2.3 Signal Group APIs

| API | Description |
|-----|-------------|
| `Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId)` | Sends a signal group (shadow buffer to I-PDU) |
| `Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId)` | Receives a signal group (I-PDU to shadow buffer) |
| `Com_InvalidateSignalGroup(Com_SignalGroupIdType SignalGroupId)` | Invalidates a signal group |
| `Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)` | Updates a shadow signal in the shadow buffer |
| `Com_ReceiveShadowSignal(Com_SignalIdType SignalId, void* SignalDataPtr)` | Receives a shadow signal from the shadow buffer |

### 2.4 I-PDU Control APIs

| API | Description |
|-----|-------------|
| `Com_TriggerIPDUSend(PduIdType PduId)` | Triggers immediate transmission of an I-PDU |
| `Com_IpduGroupControl(Com_IpduGroupVector IpduGroupVector, boolean Initialize)` | Controls (starts/stops) I-PDU groups |
| `Com_ReceptionDMControl(Com_IpduGroupVector IpduGroupVector, boolean Enable)` | Enables or disables deadline monitoring for I-PDU groups |
| `Com_EnableReceptionDM(Com_IpduGroupVector IpduGroupVector)` | Enables deadline monitoring for specified I-PDU groups |
| `Com_DisableReceptionDM(Com_IpduGroupVector IpduGroupVector)` | Disables deadline monitoring for specified I-PDU groups |
| `Com_ClearIpduGroupVector(Com_IpduGroupVector ipduGroupVector)` | Clears the I-PDU group vector |
| `Com_SetIpduGroup(Com_IpduGroupVector ipduGroupVector, Com_IpduGroupIdType ipduGroupId)` | Sets a bit in the I-PDU group vector |
| `Com_ClearIpduGroup(Com_IpduGroupVector ipduGroupVector, Com_IpduGroupIdType ipduGroupId)` | Clears a bit in the I-PDU group vector |
| `Com_SwitchIpduTxMode(PduIdType PduId, ComTxModeModeType Mode)` | Switches the transmission mode of an I-PDU |

### 2.5 PduR Callback APIs

| API | Called By | Description |
|-----|-----------|-------------|
| `Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)` | PduR | Transmit confirmation callback |
| `Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | PduR | Receive indication callback |
| `Com_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)` | PduR | Trigger transmit callback |

### 2.6 Main Function APIs

| API | Description |
|-----|-------------|
| `Com_MainFunctionRx(void)` | Periodic main function for reception processing (deadline monitoring) |
| `Com_MainFunctionTx(void)` | Periodic main function for transmission processing (periodic and mixed modes) |
| `Com_MainFunctionRouteSignals(void)` | Periodic main function for signal gateway routing |

---

## 3. Data Types

### 3.1 Com_SignalIdType
```c
typedef uint16 Com_SignalIdType;
```
Type for signal identifiers.

### 3.2 Com_SignalGroupIdType
```c
typedef uint16 Com_SignalGroupIdType;
```
Type for signal group identifiers.

### 3.3 Com_IpduGroupIdType
```c
typedef uint16 Com_IpduGroupIdType;
```
Type for I-PDU group identifiers.

### 3.4 Com_IpduIdType
```c
typedef uint16 Com_IpduIdType;
```
Type for I-PDU identifiers.

### 3.5 Com_ConfigType
```c
typedef struct {
    const Com_SignalConfigType* Signals;
    uint16 NumSignals;
    const Com_IPduConfigType* IPdus;
    uint16 NumIPdus;
} Com_ConfigType;
```
Top-level configuration structure for the COM module.

### 3.6 Com_SignalConfigType
```c
typedef struct {
    Com_SignalIdType SignalId;
    uint16 BitPosition;
    uint8 BitSize;
    uint8 Endianness;
    uint8 TransferProperty;
    uint8 FilterAlgorithm;
    uint32 FilterMask;
    uint32 FilterX;
    uint16 SignalGroupRef;
} Com_SignalConfigType;
```
Configuration for a single signal.

### 3.7 Com_IPduConfigType
```c
typedef struct {
    PduIdType PduId;
    uint16 DataLength;
    boolean RepeatingEnabled;
    uint8 NumRepetitions;
    uint16 TimeBetweenRepetitions;
    uint16 TimePeriod;
} Com_IPduConfigType;
```
Configuration for a single I-PDU.

### 3.8 ComTxModeType
```c
typedef struct {
    ComTxModeModeType Mode;
    uint8 NumberOfRepetitions;
    uint16 RepetitionPeriodFactor;
    uint16 TimePeriodFactor;
} ComTxModeType;
```
Transmission mode configuration.

### 3.9 Com_IpduGroupVector
```c
typedef uint8 Com_IpduGroupVector[(COM_NUM_IPDU_GROUPS + 7U) / 8U];
```
Bit vector for I-PDU group control.

---

## 4. Error Handling (DET)

When `COM_DEV_ERROR_DETECT == STD_ON`, the following development errors are reported via `Det_ReportError`:

| Error Code | Value | Description |
|------------|-------|-------------|
| `COM_E_PARAM` | 0x01U | Generic parameter error |
| `COM_E_UNINIT` | 0x02U | Module not initialized |
| `COM_E_PARAM_POINTER` | 0x03U | Null pointer passed |
| `COM_E_PARAM_INIT` | 0x04U | Invalid initialization parameter |
| `COM_E_INIT_FAILED` | 0x05U | Initialization failed |
| `COM_E_INVALID_SIGNAL_ID` | 0x06U | Invalid signal ID |
| `COM_E_INVALID_SIGNAL_GROUP_ID` | 0x07U | Invalid signal group ID |
| `COM_E_INVALID_IPDU_ID` | 0x08U | Invalid I-PDU ID |

### DET Usage in APIs
- **Com_Init**: Reports `COM_E_PARAM_POINTER` if `config == NULL`
- **Com_DeInit**: Reports `COM_E_UNINIT` if module not initialized
- **Com_SendSignal**: Reports `COM_E_UNINIT` if not initialized, `COM_E_PARAM_POINTER` if `SignalDataPtr == NULL`, `COM_E_INVALID_SIGNAL_ID` if invalid SignalId
- **Com_ReceiveSignal**: Reports `COM_E_UNINIT` if not initialized, `COM_E_PARAM_POINTER` if `SignalDataPtr == NULL`, `COM_E_INVALID_SIGNAL_ID` if invalid SignalId
- **Com_SendSignalGroup**: Reports `COM_E_UNINIT` if not initialized, `COM_E_INVALID_SIGNAL_GROUP_ID` if invalid SignalGroupId
- **Com_TriggerIPDUSend**: Reports `COM_E_UNINIT` if not initialized, `COM_E_INVALID_IPDU_ID` if invalid PduId
- **Com_GetVersionInfo**: Reports `COM_E_PARAM_POINTER` if `versioninfo == NULL`

---

## 5. Configuration Parameters

### Pre-Compile Configuration (Com_Cfg.h)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `COM_DEV_ERROR_DETECT` | STD_ON | Enable/disable development error detection |
| `COM_VERSION_INFO_API` | STD_ON | Enable/disable version info API |
| `COM_NUM_SIGNALS` | 64U | Maximum number of signals |
| `COM_NUM_GROUP_SIGNALS` | 16U | Maximum number of signal groups |
| `COM_NUM_IPDUS` | 32U | Maximum number of I-PDUs |
| `COM_NUM_IPDU_GROUPS` | 4U | Maximum number of I-PDU groups |
| `COM_MAX_IPDU_BUFFER_SIZE` | 64U | Maximum I-PDU buffer size in bytes |
| `COM_MAIN_FUNCTION_PERIOD_MS` | 10U | Main function period in milliseconds |
| `COM_GATEWAY_SUPPORT` | STD_ON | Enable/disable signal gateway support |
| `COM_NUM_SIGNAL_GW_MAPPINGS` | 8U | Number of signal gateway mappings |

### Signal Configuration

Each signal defines:
- **SignalId**: Unique signal identifier
- **BitPosition**: Start bit position within the I-PDU
- **BitSize**: Signal size in bits
- **Endianness**: LITTLE_ENDIAN, BIG_ENDIAN, or OPAQUE
- **TransferProperty**: TRIGGERED, TRIGGERED_ON_CHANGE, TRIGGERED_ON_CHANGE_WITHOUT_REPETITION, or PENDING
- **FilterAlgorithm**: Signal filter algorithm
- **SignalGroupRef**: Reference to parent I-PDU/SignalGroup

---

## 6. Scenarios

### Scenario 1: Signal Send (Direct Transmission)

**Description:** Application sends an engine RPM signal which is packed into an I-PDU and transmitted via PduR.

**Flow:**
1. Application calls `Com_SendSignal(COM_SIGNAL_ENGINE_RPM, &rpmValue)`
2. Com validates module state and signal ID
3. Com looks up signal configuration (BitPosition=0, BitSize=16, Endianness=LITTLE_ENDIAN)
4. Com converts signal value to uint32 and applies filter algorithm
5. Com packs signal into I-PDU buffer at the configured bit position with endianness handling
6. Com sets the update bit for this signal (if configured)
7. Com marks the I-PDU as updated
8. Because TransferProperty is TRIGGERED, Com triggers transmission via `PduR_Transmit()`
9. PduR routes to CanIf for CAN bus transmission
10. Com returns `COM_SERVICE_OK`

**Expected Result:** Signal is successfully packed and I-PDU transmission is triggered.

### Scenario 2: Signal Receive (RxIndication)

**Description:** A CAN frame is received and the vehicle speed signal is extracted.

**Flow:**
1. CanIf receives a CAN frame and calls `PduR_CanIfRxIndication()`
2. PduR routes to `Com_RxIndication(PduId, &PduInfo)`
3. Com copies received data into the I-PDU buffer
4. Com marks the I-PDU as updated
5. Application calls `Com_ReceiveSignal(COM_SIGNAL_VEHICLE_SPEED, &speedValue)`
6. Com validates module state and signal ID
7. Com looks up signal configuration (BitPosition=16, BitSize=8)
8. Com unpacks signal from I-PDU buffer with endianness handling
9. Com converts uint32 value to the correct signal type
10. Com clears the signal update flag
11. Com returns `COM_SERVICE_OK` with speedValue populated

**Expected Result:** Signal is successfully unpacked from received I-PDU.

### Scenario 3: Signal Group Transmission

**Description:** Application updates multiple related signals in a signal group and sends them atomically.

**Flow:**
1. Application calls `Com_UpdateShadowSignal(COM_SIGNAL_ENGINE_RPM, &rpmValue)`
2. Com packs the signal into the shadow buffer
3. Application calls `Com_UpdateShadowSignal(COM_SIGNAL_ENGINE_TEMP, &tempValue)`
4. Com packs the second signal into the shadow buffer
5. Application calls `Com_SendSignalGroup(0)`
6. Com copies the entire shadow buffer to the I-PDU buffer
7. Com marks the I-PDU as updated
8. Com triggers transmission via `PduR_Transmit()`
9. All signals in the group are transmitted together atomically
10. Com returns `COM_SERVICE_OK`

**Expected Result:** Signal group is transmitted atomically with consistent signal values.

### Scenario 4: Periodic Transmission (Mixed Mode)

**Description:** An I-PDU is configured with MIXED transmission mode and is sent periodically with event-triggered retransmissions.

**Flow:**
1. `Com_Init()` is called with I-PDU configured: Mode=MIXED, TimePeriod=100ms, NumRepetitions=3
2. `Com_MainFunctionTx()` is called every 10ms by the OS scheduler
3. Com decrements the time counter for the I-PDU
4. When TimeCounter reaches 0, Com transmits the I-PDU via `PduR_Transmit()`
5. Com resets TimeCounter to TimePeriod
6. `Com_TxConfirmation()` is called by PduR with E_OK
7. Because RepeatingEnabled is true, Com schedules repetitions
8. Application calls `Com_SendSignal()` for a signal in this I-PDU
9. Com packs the signal and triggers immediate transmission (direct part of MIXED)
10. Com sets RepetitionCount and schedules repetition timer
11. Subsequent `Com_MainFunctionTx()` calls handle repetition transmission

**Expected Result:** I-PDU is transmitted periodically with event-triggered immediate sends and repetitions.

---

## 7. Dependencies

### Upper Layer Modules
- **RTE**: Run Time Environment for application signal access

### Lower Layer Modules
- **PduR**: PDU Router for I-PDU transmission and reception

### Common Modules
- **Det**: Development Error Tracer
- **MemMap**: Memory mapping for section placement
- **Os**: Operating System for main function scheduling

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-04-24 | YuleTech | Initial Com module specification with 4 scenarios |
