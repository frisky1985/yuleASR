# Missing Modules Implementation Specification

> **Modules:** Eth (MCAL), Icu (MCAL), FrTp (ECUAL), Ocu (MCAL)
> **Layer:** MCAL (Eth, Icu, Ocu) / ECUAL (FrTp)
> **Standard:** AUTOSAR Classic Platform 4.x
> **Platform:** NXP S32K312
> **Safety:** ASIL-D
> **Author:** YuleTech

---

## 1. Module Overview

### 1.1 Pending Module List

| Module | Type   | Priority | Dependencies       | Estimated Effort |
|--------|--------|----------|--------------------|-----------------|
| Eth    | MCAL   | High     | Det, EcuM, Port    | 32h             |
| Icu    | MCAL   | Medium   | Det, Gpt, Port     | 30h             |
| FrTp   | ECUAL  | Medium   | FrIf, Det, PduR    | 36h             |
| Ocu    | MCAL   | Low      | Gpt, Port, Det     | 19h             |

### 1.2 Inter-Module Dependency Graph

```
Eth   <-- Det, EcuM, Port
Icu   <-- Det, Gpt, Port
FrTp  <-- FrIf, Det, PduR
Ocu   <-- Gpt, Port, Det
```

### 1.3 Coding Standards

- Language: C99
- Style: MISRA C:2012 compliant
- Safety level: ASIL-D compatible
- Comments: Doxygen style

### 1.4 Safety Requirements

- All public APIs must perform parameter validation
- Runtime checks for out-of-bounds and NULL pointer conditions
- Critical state variables must use `volatile` qualifier
- Interrupt service routines must be minimized in logic; deferred processing via flags

### 1.5 Test Requirements

- Unit test line coverage >= 80%
- MC/DC coverage >= 100% for safety-relevant code paths
- Boundary condition tests for all array/buffer accesses
- Fault injection tests for DET error paths

---

# Eth — Ethernet Driver (MCAL)

## 2. Module Overview

Eth is the MCAL-layer Ethernet hardware driver for the NXP S32K312. It provides direct access to the on-chip Ethernet MAC controller, managing descriptor rings, MII/RMII PHY register access, frame transmission and reception, and hardware timestamping. Eth sits below EthIf and does not implement protocol logic.

### Key Responsibilities

- Initialize and configure the Ethernet MAC hardware
- Manage transmit and receive buffer descriptor rings
- Provide MII/MDIO register read/write access to external PHY devices
- Deliver received frames to EthIf via callback
- Report transmit completion to EthIf via callback
- Report hardware errors to Det and EcuM

---

## 3. API List

### 3.1 Lifecycle APIs

| API | Description |
|-----|-------------|
| `void Eth_Init(const Eth_ConfigType* CfgPtr)` | Initializes the Ethernet driver and configures all controllers per `CfgPtr`. Reports `ETH_E_PARAM_POINTER` if `CfgPtr` is NULL. |
| `Std_ReturnType Eth_ControllerInit(uint8 CtrlIdx, uint8 CfgIdx)` | Initializes a specific controller instance identified by `CtrlIdx` using configuration set `CfgIdx`. Returns `E_NOT_OK` if index is out of range. |
| `void Eth_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)` | Fills `VersionInfoPtr` with the module vendor, module ID, and software version. |

### 3.2 Mode Management APIs

| API | Description |
|-----|-------------|
| `Std_ReturnType Eth_SetControllerMode(uint8 CtrlIdx, Eth_ModeType CtrlMode)` | Sets the controller to `ETH_MODE_ACTIVE` or `ETH_MODE_DOWN`. |
| `Std_ReturnType Eth_GetControllerMode(uint8 CtrlIdx, Eth_ModeType* CtrlModePtr)` | Reads the current mode of the controller into `CtrlModePtr`. |

### 3.3 PHY Interface APIs

| API | Description |
|-----|-------------|
| `Std_ReturnType Eth_WriteMII(uint8 CtrlIdx, uint8 TrcvIdx, uint8 RegIdx, uint16 RegVal)` | Writes `RegVal` to MII register `RegIdx` of transceiver `TrcvIdx` on controller `CtrlIdx`. |
| `Std_ReturnType Eth_ReadMII(uint8 CtrlIdx, uint8 TrcvIdx, uint8 RegIdx, uint16* RegValPtr)` | Reads a MII register and stores the result in `RegValPtr`. |
| `uint8 Eth_GetPhyAddress(uint8 CtrlIdx, uint8 TrcvIdx)` | Returns the 5-bit MDIO PHY address of transceiver `TrcvIdx`. |

### 3.4 Data Transfer APIs

| API | Description |
|-----|-------------|
| `BufReq_ReturnType Eth_ProvideTxBuffer(uint8 CtrlIdx, uint8* BufIdxPtr, Eth_BufDescType** BufPtr, uint16* LenBytePtr)` | Reserves a transmit buffer descriptor. Returns `BUFREQ_OK`, `BUFREQ_E_BUSY`, or `BUFREQ_E_OVFL`. |
| `Std_ReturnType Eth_Transmit(uint8 CtrlIdx, uint8 BufIdx, Eth_FrameType FrameType, boolean TxConfirmation, uint16 LenByte, const uint8* PhysAddrPtr)` | Triggers DMA transmission of the buffer identified by `BufIdx`. |
| `void Eth_Receive(uint8 CtrlIdx, uint8* DataPtr, boolean* IsDataAvailablePtr)` | Polls the receive descriptor ring; if a frame is available copies it into `DataPtr` and sets `*IsDataAvailablePtr = TRUE`. |
| `void Eth_MainFunction(void)` | Periodic task for transmit confirmation polling and error recovery. Must be called from the BSW Scheduler. |

### 3.5 Callback APIs (called by Eth, implemented by EthIf)

| API | Description |
|-----|-------------|
| `void EthIf_RxIndication(uint8 CtrlIdx, Eth_FrameType FrameType, boolean IsBroadcast, const uint8* PhysAddrPtr, const uint8* DataPtr, uint16 LenByte)` | Invoked by Eth when a frame is successfully received. |
| `void EthIf_TxConfirmation(uint8 CtrlIdx, uint8 BufIdx, Std_ReturnType Result)` | Invoked by Eth when a previously submitted transmit buffer has been sent. |

---

## 4. Data Types

### 4.1 Eth_ModeType
```c
typedef enum {
    ETH_MODE_DOWN   = 0U,
    ETH_MODE_ACTIVE = 1U
} Eth_ModeType;
```

### 4.2 Eth_StateType
```c
typedef enum {
    ETH_STATE_UNINIT = 0U,
    ETH_STATE_INIT   = 1U
} Eth_StateType;
```

### 4.3 Eth_FrameType
```c
typedef uint16 Eth_FrameType;  /* EtherType field value, e.g. 0x0800 = IPv4 */
```

### 4.4 Eth_BufDescType
```c
typedef struct {
    uint8*  DataPtr;        /* Pointer to the payload buffer */
    uint16  LenByte;        /* Length of the payload in bytes */
    boolean Locked;         /* TRUE when buffer is reserved for TX */
} Eth_BufDescType;
```

### 4.5 Eth_CtrlConfigType
```c
typedef struct {
    uint8          CtrlIdx;          /* Controller hardware index */
    uint32         MacAddress[2U];   /* 6-byte MAC address packed as two uint32 */
    uint32         MiiClockDiv;      /* MDIO clock divider */
    uint8          PhyAddress;       /* PHY address on MDIO bus */
    uint16         RxBufSize;        /* Receive buffer size in bytes */
    uint16         TxBufSize;        /* Transmit buffer size in bytes */
    uint8          RxBufCount;       /* Number of receive descriptors */
    uint8          TxBufCount;       /* Number of transmit descriptors */
} Eth_CtrlConfigType;
```

### 4.6 Eth_ConfigType
```c
typedef struct {
    const Eth_CtrlConfigType* Controllers;
    uint8                     NumControllers;
    boolean                   DevErrorDetect;
} Eth_ConfigType;
```

---

## 5. Error Handling (DET)

When `ETH_DEV_ERROR_DETECT == STD_ON`, the following errors are reported via `Det_ReportError(ETH_MODULE_ID, CtrlIdx, ApiId, ErrorId)`:

| Error Code | Value | Triggering Condition |
|------------|-------|----------------------|
| `ETH_E_PARAM_POINTER`   | 0x01U | NULL pointer passed to any API |
| `ETH_E_UNINIT`          | 0x02U | API called before `Eth_Init` |
| `ETH_E_INV_CTRL_IDX`    | 0x03U | `CtrlIdx` exceeds configured controller count |
| `ETH_E_INV_POINTER`     | 0x04U | Internal buffer pointer is invalid |
| `ETH_E_NO_ACCESS`       | 0x05U | Hardware register access failed |
| `ETH_E_INV_CONFIG`      | 0x06U | Configuration structure contains invalid values |
| `ETH_E_RX_FRAMES_LOST`  | 0x11U | Receive descriptor overflow detected |
| `ETH_E_TX_TIMEOUT`      | 0x12U | Transmit DMA did not complete within timeout |

---

## 6. Configuration Parameters

### Pre-Compile Configuration (Eth_Cfg.h)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `ETH_DEV_ERROR_DETECT`       | `STD_ON`  | Enable development error detection and DET reporting |
| `ETH_VERSION_INFO_API`       | `STD_ON`  | Enable `Eth_GetVersionInfo` API |
| `ETH_MAX_CTRLS_SUPPORTED`    | `1U`      | Maximum number of Ethernet controller instances |
| `ETH_TX_BUF_TOTAL`           | `8U`      | Total transmit buffer descriptors across all controllers |
| `ETH_RX_BUF_TOTAL`           | `8U`      | Total receive buffer descriptors across all controllers |
| `ETH_MII_TIMEOUT_CYCLES`     | `1000U`   | Maximum polling cycles for MII register access |
| `ETH_MAIN_FUNCTION_PERIOD`   | `5U`      | `Eth_MainFunction` call period in milliseconds |
| `ETH_ENABLE_HW_TIMESTAMP`    | `STD_OFF` | Enable hardware IEEE 1588 timestamping |

---

## 7. Scenarios

### Scenario 1: Normal Frame Transmission

**GIVEN** the Ethernet controller is initialized and in `ETH_MODE_ACTIVE`
**WHEN** EthIf calls `Eth_ProvideTxBuffer(0U, &BufIdx, &BufPtr, &Len)` and then `Eth_Transmit(0U, BufIdx, 0x0800U, TRUE, Len, &DestMac[0])`
**THEN** the MAC DMA descriptor is armed and the frame is transmitted; `EthIf_TxConfirmation(0U, BufIdx, E_OK)` is called on the next `Eth_MainFunction` cycle after the hardware sets the "TX done" flag.

### Scenario 2: PHY Link Configuration via MII

**GIVEN** the Ethernet driver is initialized
**WHEN** EthTrcv calls `Eth_WriteMII(0U, 0U, 0x00U, 0x1000U)` to set the PHY into 100BASE-TX full-duplex auto-negotiation mode
**THEN** `Eth_WriteMII` drives the MDIO bus, waits for the hardware busy flag to clear within `ETH_MII_TIMEOUT_CYCLES`, and returns `E_OK`; a subsequent `Eth_ReadMII` returns the written value.

### Scenario 3: Receive Frame Indication

**GIVEN** the Ethernet controller is active and a remote node sends a frame to the configured MAC address
**WHEN** the receive DMA descriptor is filled and the hardware sets the "RX complete" flag
**THEN** `Eth_Receive` (called from `Eth_MainFunction`) detects the complete descriptor, copies the payload, releases the descriptor back to hardware, and calls `EthIf_RxIndication` with the correct `FrameType`, source MAC address, data pointer, and length.

### Scenario 4: Uninitialized Module DET Report

**GIVEN** `Eth_Init` has not been called
**WHEN** any API such as `Eth_SetControllerMode(0U, ETH_MODE_ACTIVE)` is invoked
**THEN** the driver reports `ETH_E_UNINIT` to Det via `Det_ReportError` and returns `E_NOT_OK` without accessing any hardware register.

---

## 8. Dependencies

| Direction | Module | Role |
|-----------|--------|------|
| Upstream (caller) | EthIf | Calls Eth APIs; receives RxIndication and TxConfirmation callbacks |
| Upstream (caller) | EthTrcv | Uses Eth MII read/write for PHY configuration |
| Downstream | Port | Must configure RMII/RGMII pin muxing before `Eth_Init` |
| Downstream | EcuM | Provides power and clock management; calls `Eth_SetControllerMode` during run/stop transitions |
| Downstream | Det | Receives all development error reports |
| Downstream | MemMap | Provides section placement macros |

---

## 9. Version History

| Version | Date       | Author   | Changes                    |
|---------|------------|----------|----------------------------|
| 1.0.0   | 2026-08-22 | YuleTech | Initial Eth specification  |

---

---

# Icu — Input Capture Unit Driver (MCAL)

## 10. Module Overview

Icu is the MCAL-layer Input Capture Unit driver for the NXP S32K312. It abstracts the on-chip FTM (FlexTimer Module) and LPIT/LPTMR capture channels, providing measurement of signal edges, periods, duty cycles, and timestamps, as well as edge counting and configurable wakeup notifications. Icu operates below the ECUAL layer and provides a hardware-independent API to the Signal Measurement and Wakeup Handler modules.

### Key Responsibilities

- Initialize and configure capture channel hardware
- Detect rising, falling, or both edges on input signals
- Provide edge counting, timestamp capture, and signal measurement (duty cycle, period)
- Generate notification callbacks when configured thresholds are reached
- Support low-power wakeup via dedicated wakeup channels
- Report all parameter errors to Det

---

## 11. API List

### 11.1 Lifecycle APIs

| API | Description |
|-----|-------------|
| `void Icu_Init(const Icu_ConfigType* ConfigPtr)` | Initializes all configured ICU channels. Reports `ICU_E_PARAM_CONFIG` if `ConfigPtr` is NULL. |
| `void Icu_DeInit(void)` | Resets all channels to their power-on state and marks the module as uninitialized. |
| `void Icu_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Returns module version information. |

### 11.2 Mode Management APIs

| API | Description |
|-----|-------------|
| `void Icu_SetMode(Icu_ModeType Mode)` | Switches the driver between `ICU_MODE_NORMAL` and `ICU_MODE_SLEEP`. In sleep mode only wakeup-enabled channels remain active. |
| `void Icu_DisableWakeup(Icu_ChannelType Channel)` | Disables the wakeup capability of `Channel`. |
| `void Icu_EnableWakeup(Icu_ChannelType Channel)` | Enables the wakeup capability of `Channel`. |
| `void Icu_CheckWakeup(EcuM_WakeupSourceType WakeupSource)` | Validates and reports a wakeup event to EcuM. Called from EcuM wakeup validation. |

### 11.3 Edge Detection APIs

| API | Description |
|-----|-------------|
| `void Icu_SetActivationCondition(Icu_ChannelType Channel, Icu_ActivationType Activation)` | Sets the active edge: `ICU_RISING_EDGE`, `ICU_FALLING_EDGE`, or `ICU_BOTH_EDGES`. |
| `void Icu_EnableNotification(Icu_ChannelType Channel)` | Enables the configured notification callback for `Channel`. |
| `void Icu_DisableNotification(Icu_ChannelType Channel)` | Disables the notification callback for `Channel` without losing configuration. |
| `Icu_InputStateType Icu_GetInputState(Icu_ChannelType Channel)` | Returns `ICU_ACTIVE` if an edge has occurred since the last call, otherwise `ICU_IDLE`. |

### 11.4 Timestamp APIs

| API | Description |
|-----|-------------|
| `void Icu_StartTimestamp(Icu_ChannelType Channel, Icu_ValueType* BufferPtr, uint16 BufferSize, uint16 NotifyInterval)` | Begins timestamp capture into `BufferPtr`. `NotifyInterval` specifies how many timestamps trigger the notification callback. |
| `void Icu_StopTimestamp(Icu_ChannelType Channel)` | Stops timestamp capture. |
| `Icu_IndexType Icu_GetTimestampIndex(Icu_ChannelType Channel)` | Returns the index of the next free entry in the timestamp buffer. |

### 11.5 Edge Count APIs

| API | Description |
|-----|-------------|
| `void Icu_ResetEdgeCount(Icu_ChannelType Channel)` | Resets the edge counter of `Channel` to zero. |
| `void Icu_EnableEdgeCount(Icu_ChannelType Channel)` | Starts incrementing the edge counter on each configured edge. |
| `void Icu_DisableEdgeCount(Icu_ChannelType Channel)` | Stops edge counting, preserving the current count. |
| `Icu_EdgeNumberType Icu_GetEdgeNumbers(Icu_ChannelType Channel)` | Returns the current edge count value. |

### 11.6 Signal Measurement APIs

| API | Description |
|-----|-------------|
| `void Icu_StartSignalMeasurement(Icu_ChannelType Channel)` | Starts continuous duty cycle and period measurement on `Channel`. |
| `void Icu_StopSignalMeasurement(Icu_ChannelType Channel)` | Stops signal measurement. |
| `Icu_ValueType Icu_GetTimeElapsed(Icu_ChannelType Channel)` | Returns the elapsed time since the last active edge, in timer ticks. |
| `void Icu_GetDutyCycleValues(Icu_ChannelType Channel, Icu_DutyCycleType* DutyCycleValues)` | Fills `DutyCycleValues` with the most recent active time and period measurements. |

---

## 12. Data Types

### 12.1 Icu_ModeType
```c
typedef enum {
    ICU_MODE_NORMAL = 0U,
    ICU_MODE_SLEEP  = 1U
} Icu_ModeType;
```

### 12.2 Icu_ActivationType
```c
typedef enum {
    ICU_RISING_EDGE  = 0U,
    ICU_FALLING_EDGE = 1U,
    ICU_BOTH_EDGES   = 2U
} Icu_ActivationType;
```

### 12.3 Icu_InputStateType
```c
typedef enum {
    ICU_IDLE   = 0U,
    ICU_ACTIVE = 1U
} Icu_InputStateType;
```

### 12.4 Icu_MeasurementModeType
```c
typedef enum {
    ICU_MODE_SIGNAL_EDGE_DETECT   = 0U,
    ICU_MODE_TIMESTAMP            = 1U,
    ICU_MODE_EDGE_COUNTER         = 2U,
    ICU_MODE_SIGNAL_MEASUREMENT   = 3U
} Icu_MeasurementModeType;
```

### 12.5 Icu_DutyCycleType
```c
typedef struct {
    Icu_ValueType ActiveTime;   /* High-level duration in timer ticks */
    Icu_ValueType PeriodTime;   /* Full period duration in timer ticks */
} Icu_DutyCycleType;
```

### 12.6 Icu_ChannelConfigType
```c
typedef struct {
    Icu_ChannelType         ChannelId;
    Icu_MeasurementModeType MeasurementMode;
    Icu_ActivationType      DefaultActivation;
    boolean                 WakeupCapability;
    void                    (*NotificationPtr)(void);  /* User callback, may be NULL */
} Icu_ChannelConfigType;
```

### 12.7 Icu_ConfigType
```c
typedef struct {
    const Icu_ChannelConfigType* Channels;
    uint8                        NumChannels;
    boolean                      DevErrorDetect;
} Icu_ConfigType;
```

---

## 13. Error Handling (DET)

| Error Code | Value | Triggering Condition |
|------------|-------|----------------------|
| `ICU_E_PARAM_CONFIG`       | 0x0AU | `Icu_Init` called with NULL `ConfigPtr` |
| `ICU_E_PARAM_CHANNEL`      | 0x0BU | Channel number exceeds configured channel count |
| `ICU_E_PARAM_ACTIVATION`   | 0x0CU | Invalid `Icu_ActivationType` value |
| `ICU_E_PARAM_POINTER`      | 0x0DU | NULL pointer passed to buffer parameter |
| `ICU_E_PARAM_BUFFERSIZE`   | 0x0EU | `BufferSize` is zero in `Icu_StartTimestamp` |
| `ICU_E_UNINIT`             | 0x0FU | Any API called before `Icu_Init` |
| `ICU_E_NOT_STARTED`        | 0x10U | Stop API called when channel is not running |
| `ICU_E_BUSY_OPERATION`     | 0x11U | Start API called when channel is already running |
| `ICU_E_WRONG_WAKEUP`       | 0x12U | `Icu_CheckWakeup` called with non-wakeup source |

---

## 14. Configuration Parameters

### Pre-Compile Configuration (Icu_Cfg.h)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `ICU_DEV_ERROR_DETECT`      | `STD_ON` | Enable development error detection |
| `ICU_VERSION_INFO_API`      | `STD_ON` | Enable `Icu_GetVersionInfo` API |
| `ICU_WAKEUP_FUNCTIONALITY`  | `STD_ON` | Compile wakeup source support |
| `ICU_TIMESTAMP_API`         | `STD_ON` | Compile timestamp API group |
| `ICU_EDGE_COUNT_API`        | `STD_ON` | Compile edge count API group |
| `ICU_SIGNAL_MEASUREMENT_API`| `STD_ON` | Compile signal measurement API group |
| `ICU_GET_INPUT_STATE_API`   | `STD_ON` | Compile `Icu_GetInputState` API |
| `ICU_MAX_CHANNELS`          | `16U`    | Maximum number of configured ICU channels |
| `ICU_TIMESTAMP_BUFFER_SIZE` | `64U`    | Default timestamp buffer depth per channel |

---

## 15. Scenarios

### Scenario 1: PWM Duty Cycle Measurement

**GIVEN** Channel 0 is configured in `ICU_MODE_SIGNAL_MEASUREMENT` with `ICU_BOTH_EDGES` activation
**WHEN** `Icu_StartSignalMeasurement(0U)` is called and a PWM signal with 25% duty cycle at 1 kHz is applied
**THEN** `Icu_GetDutyCycleValues(0U, &DcValues)` returns `DcValues.ActiveTime` corresponding to 250 µs and `DcValues.PeriodTime` corresponding to 1000 µs in hardware timer ticks.

### Scenario 2: Edge Count for Wheel Speed

**GIVEN** Channel 3 is configured in `ICU_MODE_EDGE_COUNTER` with `ICU_RISING_EDGE` activation
**WHEN** `Icu_EnableEdgeCount(3U)` is called and 48 rising edges arrive over 100 ms
**THEN** `Icu_GetEdgeNumbers(3U)` returns 48U; after `Icu_ResetEdgeCount(3U)` the return value is 0U.

### Scenario 3: Wakeup from Sleep Mode

**GIVEN** the ECU is in sleep mode and Channel 2 has wakeup capability enabled via `Icu_EnableWakeup(2U)` and ICU mode is `ICU_MODE_SLEEP`
**WHEN** a rising edge is detected on Channel 2's input pin
**THEN** the Icu ISR records the wakeup event and calls `EcuM_SetWakeupEvent` with the associated wakeup source; upon ECU wakeup, `Icu_CheckWakeup` validates the source and `EcuM_ValidateWakeupEvent` is invoked.

### Scenario 4: Notification Callback on Timestamp Interval

**GIVEN** Channel 1 is configured in `ICU_MODE_TIMESTAMP` with `NotifyInterval = 4U` and a user callback `App_IcuTimestampCb` is registered
**WHEN** `Icu_StartTimestamp(1U, TimestampBuf, 32U, 4U)` is called and 4 edges arrive
**THEN** `App_IcuTimestampCb` is invoked exactly once; subsequent groups of 4 edges each trigger one callback invocation; `Icu_GetTimestampIndex` advances by 1 after each edge.

---

## 16. Dependencies

| Direction | Module | Role |
|-----------|--------|------|
| Upstream (caller) | WdgM / application | Calls Icu signal measurement and edge count APIs |
| Upstream (caller) | EcuM | Calls `Icu_CheckWakeup`; drives mode transitions |
| Downstream | Port | Must configure capture pins before `Icu_Init` |
| Downstream | Gpt | Shares timer hardware; must not conflict on FTM channel allocation |
| Downstream | Det | Receives all development error reports |
| Downstream | MemMap | Provides section placement macros |

---

## 17. Version History

| Version | Date       | Author   | Changes                    |
|---------|------------|----------|----------------------------|
| 1.0.0   | 2026-08-22 | YuleTech | Initial Icu specification  |

---

---

# FrTp — FlexRay Transport Protocol (ECUAL)

## 18. Module Overview

FrTp is the ECUAL-layer FlexRay Transport Protocol module defined in AUTOSAR SWS_FrTp. It provides segmentation and reassembly of long I-PDUs over the FlexRay bus, bridging the gap between the PDU Router (PduR) and the FlexRay Interface (FrIf). FrTp implements the AUTOSAR TP protocol for FlexRay, handling multi-frame transmission, flow control, connection management, and timeout supervision.

### Key Responsibilities

- Segment outgoing SDUs into FlexRay frames (Start Frame SF, First Frame FF, Consecutive Frames CF)
- Reassemble incoming frames back into SDUs for delivery to PduR
- Manage FlexRay TP connections: N_As, N_Bs, N_Cs, N_Cr timers
- Handle Flow Control (FC) frames: Wait (WT), Continue-to-Send (CTS), Overflow (OVF)
- Support transmit and receive cancellation
- Report all transport protocol events and errors to Det

---

## 19. API List

### 19.1 Lifecycle APIs

| API | Description |
|-----|-------------|
| `void FrTp_Init(const FrTp_ConfigType* CfgPtr)` | Initializes the FrTp module. Reports `FRTP_E_PARAM_POINTER` if `CfgPtr` is NULL. |
| `void FrTp_Shutdown(void)` | Closes all active connections and marks the module as uninitialized. |
| `void FrTp_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Returns module version information. |

### 19.2 Service APIs (called by PduR)

| API | Description |
|-----|-------------|
| `Std_ReturnType FrTp_Transmit(PduIdType FrTpTxSduId, const PduInfoType* FrTpTxInfoPtr)` | Initiates transport protocol transmission of SDU `FrTpTxSduId`. Returns `E_NOT_OK` if a connection is already active for this SDU. |
| `Std_ReturnType FrTp_CancelTransmit(PduIdType FrTpTxSduId)` | Cancels an ongoing transmit session. Returns `E_NOT_OK` if no active session exists. |
| `Std_ReturnType FrTp_CancelReceive(PduIdType FrTpRxSduId)` | Cancels an ongoing receive session. Returns `E_NOT_OK` if no active session exists. |
| `Std_ReturnType FrTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value)` | Modifies transport layer parameters (`TP_STMIN`, `TP_BS`) for connection `id`. |

### 19.3 Lower-Layer Callback APIs (called by FrIf)

| API | Description |
|-----|-------------|
| `void FrTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | Called by FrIf when a FlexRay frame carrying FrTp data is received. FrTp parses the frame type (SF/FF/CF/FC) and advances the reassembly state machine. |
| `void FrTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)` | Called by FrIf when a previously submitted frame has been accepted into the FlexRay schedule. |

### 19.4 Scheduled API

| API | Description |
|-----|-------------|
| `void FrTp_MainFunction(void)` | Periodic task for TP timer supervision (N_As, N_Bs, N_Cs, N_Cr) and retransmission handling. Must be called from the BSW Scheduler at the configured period. |

---

## 20. Data Types

### 20.1 FrTp_StateType
```c
typedef enum {
    FRTP_STATE_UNINIT = 0U,
    FRTP_STATE_INIT   = 1U
} FrTp_StateType;
```

### 20.2 FrTp_ChannelStateType
```c
typedef enum {
    FRTP_CH_IDLE         = 0U,
    FRTP_CH_TX_WAIT_FC   = 1U,
    FRTP_CH_TX_CF        = 2U,
    FRTP_CH_RX_WAIT_CF   = 3U,
    FRTP_CH_RX_COMPLETE  = 4U,
    FRTP_CH_ERROR        = 5U
} FrTp_ChannelStateType;
```

### 20.3 FrTp_FrameTypeType
```c
typedef enum {
    FRTP_FRAME_SF  = 0x00U,   /* Single Frame */
    FRTP_FRAME_FF  = 0x01U,   /* First Frame  */
    FRTP_FRAME_CF  = 0x02U,   /* Consecutive Frame */
    FRTP_FRAME_FC  = 0x03U    /* Flow Control */
} FrTp_FrameTypeType;
```

### 20.4 FrTp_FlowStatusType
```c
typedef enum {
    FRTP_FS_CTS  = 0x00U,   /* Continue to Send */
    FRTP_FS_WT   = 0x01U,   /* Wait             */
    FRTP_FS_OVF  = 0x02U    /* Overflow         */
} FrTp_FlowStatusType;
```

### 20.5 FrTp_TxNsduConfigType
```c
typedef struct {
    PduIdType  TxNSduId;        /* NSDU identifier used by PduR */
    PduIdType  FrIfTxPduId;     /* FrIf PDU to use for transmission */
    uint16     N_As_TimeoutMs;  /* Transmitter-side frame sending timeout */
    uint16     N_Bs_TimeoutMs;  /* Timeout waiting for FC after FF */
    uint16     N_Cs_TimeoutMs;  /* Inter-frame separation time enforcement */
    uint8      MaxBlockSize;    /* BS field value in FC frames (0 = no limit) */
} FrTp_TxNsduConfigType;
```

### 20.6 FrTp_RxNsduConfigType
```c
typedef struct {
    PduIdType  RxNSduId;        /* NSDU identifier used by PduR */
    PduIdType  FrIfRxPduId;     /* FrIf PDU assigned for reception */
    uint16     N_Cr_TimeoutMs;  /* Receiver-side consecutive frame timeout */
    uint8      STminMs;         /* Minimum separation time to request in FC */
    uint8      BlockSize;       /* Number of CF frames before next FC */
} FrTp_RxNsduConfigType;
```

### 20.7 FrTp_ConfigType
```c
typedef struct {
    const FrTp_TxNsduConfigType* TxNSdus;
    uint8                         NumTxNSdus;
    const FrTp_RxNsduConfigType* RxNSdus;
    uint8                         NumRxNSdus;
    boolean                       DevErrorDetect;
    uint16                        MainFunctionPeriodMs;
} FrTp_ConfigType;
```

---

## 21. Error Handling (DET)

| Error Code | Value | Triggering Condition |
|------------|-------|----------------------|
| `FRTP_E_PARAM_POINTER`      | 0x01U | NULL pointer passed to any API |
| `FRTP_E_UNINIT`             | 0x02U | API called before `FrTp_Init` |
| `FRTP_E_INVALID_PDU_SDU_ID` | 0x03U | Unknown `FrTpTxSduId` or `FrTpRxSduId` |
| `FRTP_E_OPER_NOT_SUPPORTED` | 0x04U | `FrTp_ChangeParameter` called with unsupported parameter |
| `FRTP_E_COM_ERROR`          | 0x05U | FrIf transmit returned `E_NOT_OK` unexpectedly |
| `FRTP_E_NO_BUFFER`          | 0x06U | PduR `CopyTxData` or `CopyRxData` returned no buffer |
| `FRTP_E_TIMEOUT_AS`         | 0x10U | N_As timer expired during multi-frame transmission |
| `FRTP_E_TIMEOUT_BS`         | 0x11U | N_Bs timer expired waiting for Flow Control |
| `FRTP_E_TIMEOUT_CR`         | 0x12U | N_Cr timer expired waiting for Consecutive Frame |
| `FRTP_E_INVALID_FS`         | 0x13U | Received FC frame contains invalid flow status value |

---

## 22. Configuration Parameters

### Pre-Compile Configuration (FrTp_Cfg.h)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `FRTP_DEV_ERROR_DETECT`       | `STD_ON` | Enable development error detection |
| `FRTP_VERSION_INFO_API`       | `STD_ON` | Enable `FrTp_GetVersionInfo` API |
| `FRTP_CANCEL_TRANSMIT_API`    | `STD_ON` | Compile `FrTp_CancelTransmit` API |
| `FRTP_CANCEL_RECEIVE_API`     | `STD_ON` | Compile `FrTp_CancelReceive` API |
| `FRTP_CHANGE_PARAMETER_API`   | `STD_ON` | Compile `FrTp_ChangeParameter` API |
| `FRTP_MAX_TX_NSDUS`           | `4U`     | Maximum number of transmit NSDUs |
| `FRTP_MAX_RX_NSDUS`           | `4U`     | Maximum number of receive NSDUs |
| `FRTP_MAX_PDU_LENGTH`         | `4095U`  | Maximum SDU length supported (12-bit FF_DL field) |
| `FRTP_MAIN_FUNCTION_PERIOD`   | `10U`    | `FrTp_MainFunction` call period in milliseconds |

---

## 23. Scenarios

### Scenario 1: Single-Frame UDS Request Transmission

**GIVEN** FrTp is initialized and NSDU 0 is configured with a FlexRay static segment PDU
**WHEN** PduR calls `FrTp_Transmit(0U, &PduInfo)` with `PduInfo.SduLength = 6U` (fits in a single frame)
**THEN** FrTp immediately constructs a Single Frame (SF) with the 6-byte payload and calls `FrIf_Transmit`; `FrTp_TxConfirmation` is called by FrIf, after which FrTp calls `PduR_FrTpTxConfirmation(0U, E_OK)`.

### Scenario 2: Multi-Frame Reception with Flow Control

**GIVEN** FrTp is initialized and NSDU 1 is configured to receive with `BlockSize = 2U`
**WHEN** a remote node sends a First Frame (FF) with `FF_DL = 100U` followed by Consecutive Frames CF[1], CF[2]
**THEN** upon receiving FF, FrTp calls `PduR_FrTpStartOfReception`, sends an FC CTS frame via FrIf, and begins buffering CFs; after receiving CF[1] and CF[2] (one block), FrTp sends another FC CTS; after all data is received, `PduR_FrTpRxIndication(1U, E_OK)` is called.

### Scenario 3: N_Bs Timer Expiry (Flow Control Timeout)

**GIVEN** FrTp has sent a First Frame for NSDU 2 and is waiting for a Flow Control response
**WHEN** no FC frame arrives within the configured `N_Bs_TimeoutMs` period
**THEN** `FrTp_MainFunction` detects the timeout, reports `FRTP_E_TIMEOUT_BS` to Det, calls `PduR_FrTpTxConfirmation(2U, E_NOT_OK)`, and resets the connection state to `FRTP_CH_IDLE`.

### Scenario 4: Transmit Cancellation

**GIVEN** NSDU 3 is in state `FRTP_CH_TX_WAIT_FC` after sending a First Frame
**WHEN** PduR calls `FrTp_CancelTransmit(3U)`
**THEN** FrTp aborts the session, calls `PduR_FrTpTxConfirmation(3U, E_NOT_OK)` to notify the upper layer, and resets the channel state to `FRTP_CH_IDLE`; subsequent calls to `FrTp_CancelTransmit(3U)` return `E_NOT_OK`.

---

## 24. Dependencies

| Direction | Module | Role |
|-----------|--------|------|
| Upstream (caller) | PduR | Calls `FrTp_Transmit`, `FrTp_CancelTransmit`, `FrTp_CancelReceive`; receives `PduR_FrTpTxConfirmation` and `PduR_FrTpRxIndication` |
| Downstream | FrIf | FrTp calls `FrIf_Transmit`; FrIf calls back `FrTp_RxIndication` and `FrTp_TxConfirmation` |
| Downstream | Det | Receives all development error reports |
| Downstream | MemMap | Provides section placement macros |

---

## 25. Version History

| Version | Date       | Author   | Changes                      |
|---------|------------|----------|------------------------------|
| 1.0.0   | 2026-08-22 | YuleTech | Initial FrTp specification   |

---

---

# Ocu — Output Compare Unit Driver (MCAL)

## 26. Module Overview

Ocu is the MCAL-layer Output Compare Unit driver for the NXP S32K312. It abstracts FTM Output Compare channels, providing configurable output pin state changes when the hardware counter reaches a programmed threshold value. Unlike Pwm (which continuously generates waveforms), Ocu fires a single compare event at an absolute or relative counter value and then requires the application to reprogram the threshold for the next event. This makes Ocu suitable for precise one-shot timing events and software-managed waveforms.

### Key Responsibilities

- Initialize output compare channels and configure default pin states
- Start and stop output compare channels independently
- Set absolute or relative thresholds for compare match events
- Drive output pins to a fixed state (`OCU_HIGH`, `OCU_LOW`) or toggle on match
- Generate user-configurable notification callbacks on compare match events
- Report parameter errors to Det

---

## 27. API List

### 27.1 Lifecycle APIs

| API | Description |
|-----|-------------|
| `void Ocu_Init(const Ocu_ConfigType* ConfigPtr)` | Initializes all configured OCU channels. Reports `OCU_E_PARAM_CONFIG` if `ConfigPtr` is NULL. |
| `void Ocu_DeInit(void)` | Stops all channels and resets hardware to default state. |
| `void Ocu_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Returns module version information. |

### 27.2 Channel Control APIs

| API | Description |
|-----|-------------|
| `void Ocu_StartChannel(Ocu_ChannelType ChannelNumber)` | Enables the compare interrupt and activates the output pin action for `ChannelNumber`. |
| `void Ocu_StopChannel(Ocu_ChannelType ChannelNumber)` | Disables the compare interrupt. The output pin retains its last state. |

### 27.3 Output Pin APIs

| API | Description |
|-----|-------------|
| `void Ocu_SetPinState(Ocu_ChannelType ChannelNumber, Ocu_PinStateType PinState)` | Forces the output pin to `OCU_HIGH` or `OCU_LOW` immediately, independent of compare events. |
| `void Ocu_SetPinAction(Ocu_ChannelType ChannelNumber, Ocu_PinActionType PinAction)` | Configures the pin action on the next compare match: `OCU_SET_HIGH`, `OCU_SET_LOW`, `OCU_TOGGLE`, or `OCU_DISABLE`. |

### 27.4 Threshold APIs

| API | Description |
|-----|-------------|
| `Ocu_ReturnType Ocu_SetAbsoluteThreshold(Ocu_ChannelType ChannelNumber, Ocu_ValueType ReferenceValue, Ocu_ValueType AbsoluteValue)` | Sets the compare register to `AbsoluteValue`. Returns `OCU_CM_IN_REF_INTERVAL` if the absolute value falls within `[ReferenceValue, counter]` (i.e., a compare event was likely missed), otherwise `OCU_CM_OUT_REF_INTERVAL`. |
| `Ocu_ReturnType Ocu_SetRelativeThreshold(Ocu_ChannelType ChannelNumber, Ocu_ValueType RelativeValue)` | Sets the compare register to `(current_counter + RelativeValue) mod counter_max`. Returns the same codes as the absolute variant. |
| `Ocu_ValueType Ocu_GetCounter(Ocu_ChannelType ChannelNumber)` | Returns the current free-running counter value associated with the channel's timer. |

### 27.5 Notification APIs

| API | Description |
|-----|-------------|
| `void Ocu_EnableNotification(Ocu_ChannelType ChannelNumber)` | Enables the compare match notification callback for `ChannelNumber`. |
| `void Ocu_DisableNotification(Ocu_ChannelType ChannelNumber)` | Disables the compare match notification callback without changing the threshold. |

---

## 28. Data Types

### 28.1 Ocu_PinStateType
```c
typedef enum {
    OCU_LOW  = 0U,
    OCU_HIGH = 1U
} Ocu_PinStateType;
```

### 28.2 Ocu_PinActionType
```c
typedef enum {
    OCU_SET_LOW  = 0U,
    OCU_SET_HIGH = 1U,
    OCU_TOGGLE   = 2U,
    OCU_DISABLE  = 3U
} Ocu_PinActionType;
```

### 28.3 Ocu_ReturnType
```c
typedef enum {
    OCU_CM_IN_REF_INTERVAL  = 0U,   /* Compare match likely missed */
    OCU_CM_OUT_REF_INTERVAL = 1U    /* Compare will fire in the future */
} Ocu_ReturnType;
```

### 28.4 Ocu_ChannelConfigType
```c
typedef struct {
    Ocu_ChannelType  ChannelId;
    Ocu_PinActionType DefaultPinAction;
    Ocu_PinStateType  DefaultPinState;
    boolean           NotificationEnabled;
    void             (*NotificationPtr)(void);  /* Compare match callback; may be NULL */
} Ocu_ChannelConfigType;
```

### 28.5 Ocu_ConfigType
```c
typedef struct {
    const Ocu_ChannelConfigType* Channels;
    uint8                        NumChannels;
    boolean                      DevErrorDetect;
} Ocu_ConfigType;
```

---

## 29. Error Handling (DET)

| Error Code | Value | Triggering Condition |
|------------|-------|----------------------|
| `OCU_E_PARAM_CONFIG`       | 0x0AU | `Ocu_Init` called with NULL `ConfigPtr` |
| `OCU_E_PARAM_CHANNEL`      | 0x0BU | Channel number exceeds configured channel count |
| `OCU_E_PARAM_POINTER`      | 0x0CU | NULL `versioninfo` pointer |
| `OCU_E_UNINIT`             | 0x0DU | Any API called before `Ocu_Init` |
| `OCU_E_BUSY`               | 0x0EU | `Ocu_StartChannel` called on an already-running channel |
| `OCU_E_NOT_RUNNING`        | 0x0FU | `Ocu_StopChannel` called on a stopped channel |
| `OCU_E_PARAM_INVALID_VALUE`| 0x10U | Threshold value exceeds the hardware counter maximum |

---

## 30. Configuration Parameters

### Pre-Compile Configuration (Ocu_Cfg.h)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `OCU_DEV_ERROR_DETECT`        | `STD_ON` | Enable development error detection |
| `OCU_VERSION_INFO_API`        | `STD_ON` | Enable `Ocu_GetVersionInfo` API |
| `OCU_SET_PIN_STATE_API`       | `STD_ON` | Compile `Ocu_SetPinState` API |
| `OCU_SET_PIN_ACTION_API`      | `STD_ON` | Compile `Ocu_SetPinAction` API |
| `OCU_GET_COUNTER_API`         | `STD_ON` | Compile `Ocu_GetCounter` API |
| `OCU_NOTIFICATION_API`        | `STD_ON` | Compile notification enable/disable APIs |
| `OCU_MAX_CHANNELS`            | `8U`     | Maximum number of configured OCU channels |
| `OCU_COUNTER_MAX_VALUE`       | `0xFFFFU`| Hardware counter wrap-around value |

---

## 31. Scenarios

### Scenario 1: One-Shot Compare Event

**GIVEN** Channel 0 is initialized with `DefaultPinAction = OCU_SET_HIGH` and `NotificationEnabled = TRUE`
**WHEN** `Ocu_StartChannel(0U)` is called and `Ocu_SetRelativeThreshold(0U, 1000U)` programs a match 1000 ticks in the future
**THEN** when the hardware counter reaches the compare value the output pin is driven high, the registered notification callback is invoked, and `Ocu_GetCounter(0U)` returns a value greater than or equal to the programmed threshold.

### Scenario 2: Periodic Toggle via Absolute Threshold

**GIVEN** Channel 1 is initialized with `DefaultPinAction = OCU_TOGGLE` and the counter runs at 1 MHz
**WHEN** on each notification callback the application calls `Ocu_SetAbsoluteThreshold(1U, currentCounter, currentCounter + 500U)` to schedule the next event 500 µs ahead
**THEN** `OCU_CM_OUT_REF_INTERVAL` is returned each time (because the next event is in the future), the pin toggles every 500 µs, and the notification fires at 2 kHz.

### Scenario 3: Force Output Pin to Defined State

**GIVEN** Channel 2 is running with `PinAction = OCU_TOGGLE`
**WHEN** `Ocu_SetPinState(2U, OCU_LOW)` is called while the channel is active
**THEN** the pin is driven immediately to the low state without waiting for a compare event; subsequent compare matches still follow the configured `OCU_TOGGLE` action from the forced state.

### Scenario 4: Threshold in Reference Interval (Missed Event)

**GIVEN** Channel 3 is started and the counter is at value 5000U
**WHEN** `Ocu_SetAbsoluteThreshold(3U, 4000U, 4500U)` is called (absolute value 4500 is between reference 4000 and current counter 5000)
**THEN** `Ocu_SetAbsoluteThreshold` returns `OCU_CM_IN_REF_INTERVAL`, indicating the event would have occurred in the past; the application must immediately reprogram a future threshold.

---

## 32. Dependencies

| Direction | Module | Role |
|-----------|--------|------|
| Upstream (caller) | Application / SWC | Programs compare thresholds and registers notifications for precise timing events |
| Downstream | Port | Must configure OCU output pins before `Ocu_Init` |
| Downstream | Gpt | Shares FTM timer hardware; channel allocation must not conflict |
| Downstream | Det | Receives all development error reports |
| Downstream | MemMap | Provides section placement macros |

---

## 33. Version History

| Version | Date       | Author   | Changes                    |
|---------|------------|----------|----------------------------|
| 1.0.0   | 2026-08-22 | YuleTech | Initial Ocu specification  |

---

## 34. Acceptance Criteria

### Functional Acceptance

- [ ] All APIs are implemented in accordance with AUTOSAR SWS for Eth, Icu, FrTp, and Ocu
- [ ] All configuration parameters are compile-time configurable via `_Cfg.h` headers
- [ ] All error conditions are detected and reported via DET when `DEV_ERROR_DETECT == STD_ON`

### Quality Acceptance

- [ ] Unit tests pass with >= 80% line coverage and 100% MC/DC on safety-relevant branches
- [ ] Static analysis (MISRA C:2012) generates zero mandatory-rule violations
- [ ] Code review completed and all findings resolved

### Documentation Acceptance

- [ ] API documentation complete (Doxygen headers for all public functions)
- [ ] Design documents updated to include these modules
- [ ] Configuration guide complete

---

*Version: 1.0.0 | Date: 2026-08-22 | Status: Approved*
