/**
 * @file CanIf.h
 * @brief CAN Interface module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: CAN Interface (CANIF)
 * Layer: ECU Abstraction Layer (ECUAL)
 */

#ifndef CANIF_H
#define CANIF_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "CanIf_Cfg.h"
#include "ComStack_Types.h"
#include "Can.h"

/* Forward declarations */
typedef uint32 EcuM_WakeupSourceType;

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define CANIF_VENDOR_ID                 (0x01U) /* YuleTech Vendor ID */
#define CANIF_MODULE_ID                 (0x3CU) /* CANIF Module ID */
#define CANIF_AR_RELEASE_MAJOR_VERSION  (0x04U)
#define CANIF_AR_RELEASE_MINOR_VERSION  (0x04U)
#define CANIF_AR_RELEASE_REVISION_VERSION (0x00U)
#define CANIF_SW_MAJOR_VERSION          (0x01U)
#define CANIF_SW_MINOR_VERSION          (0x00U)
#define CANIF_SW_PATCH_VERSION          (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define CANIF_SID_INIT                  (0x01U)
#define CANIF_SID_DEINIT                (0x02U)
#define CANIF_SID_SETCONTROLLERMODE     (0x03U)
#define CANIF_SID_GETCONTROLLERMODE     (0x04U)
#define CANIF_SID_GETCONTROLLERERRORSTATE (0x4BU)
#define CANIF_SID_TRANSMIT              (0x05U)
#define CANIF_SID_READTXPDUDATA         (0x06U)
#define CANIF_SID_READRXPDUDATA         (0x07U)
#define CANIF_SID_SETPDUMODE            (0x08U)
#define CANIF_SID_GETPDUMODE            (0x09U)
#define CANIF_SID_GETVERSIONINFO        (0x0BU)
#define CANIF_SID_SETDYNAMICTXID        (0x0CU)
#define CANIF_SID_SETTRCVMODE           (0x0DU)
#define CANIF_SID_GETTRCVMODE           (0x0EU)
#define CANIF_SID_GETTRCVWAKEUPREASON   (0x0FU)
#define CANIF_SID_SETTRCVWAKEUPMODE     (0x10U)
#define CANIF_SID_CHECKWAKEUP           (0x11U)
#define CANIF_SID_CHECKVALIDATION       (0x12U)
#define CANIF_SID_GETTXCONFIRMATIONSTATE (0x13U)
#define CANIF_SID_SETBAUDRATE           (0x27U)
#define CANIF_SID_GETBAUDRATE           (0x28U)
#define CANIF_SID_GETCONTROLLERRXERRORCOUNTER (0x4CU)
#define CANIF_SID_GETCONTROLLERTXERRORCOUNTER (0x4DU)
#define CANIF_SID_READTXNOTIFSTATUS     (0x4EU)
#define CANIF_SID_READRXNOTIFSTATUS     (0x4FU)
#define CANIF_SID_CONFIRMPNAVAILABILITY (0x50U)
#define CANIF_SID_CHECKTRCVWAKEFLAG     (0x51U)
#define CANIF_SID_CLEARTRCVWUFFLAG      (0x52U)
#define CANIF_SID_CHECKTRCVWAKEFLAGINDICATION (0x53U)
#define CANIF_SID_CLEARTRCVWUFFLAGINDICATION  (0x54U)
#define CANIF_SID_TRCVMODEINDICATION    (0x55U)
#define CANIF_SID_CONTROLLERMODEINDICATION (0x56U)
#define CANIF_SID_CANIDTOMETADATA       (0x57U)
#define CANIF_SID_METADATATOCANID       (0x58U)
#define CANIF_SID_SETPNWAKEUPFILTER     (0x59U)
#define CANIF_SID_TXQUEUEMAINFUNCTION   (0x5AU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#ifndef CANIF_E_PARAM_CANID
#define CANIF_E_PARAM_CANID             (0x01U)
#endif
#ifndef CANIF_E_PARAM_DLC
#define CANIF_E_PARAM_DLC               (0x02U)
#endif
#define CANIF_E_PARAM_CONTROLLER        (0x03U)
#define CANIF_E_PARAM_POINTER           (0x04U)
#define CANIF_E_PARAM_CONTROLLERMODE    (0x05U)
#define CANIF_E_PARAM_TRCVMODE          (0x06U)
#define CANIF_E_PARAM_TRCVWAKEUPMODE    (0x07U)
#define CANIF_E_PARAM_TRCV              (0x08U)
#define CANIF_E_PARAM_PDUMODE           (0x09U)
#define CANIF_E_PARAM_HTH               (0x0BU)
#define CANIF_E_PARAM_HRH               (0x0CU)
#define CANIF_E_PARAM_CANIDTYPE         (0x0DU)
#define CANIF_E_UNINIT                  (0x14U)
#define CANIF_E_INVALID_TXPDUID         (0x50U)
#define CANIF_E_INVALID_RXPDUID         (0x60U)
#define CANIF_E_STOPPED                 (0x70U)
#define CANIF_E_NOT_SLEEP               (0x71U)
#define CANIF_E_PARAM_WAKEUPSOURCE      (0x72U)
#define CANIF_E_INVALID_DATA_LENGTH     (0x73U)
#define CANIF_E_DATA_LENGTH_MISMATCH    (0x74U)
#define CANIF_E_PARAM_BAUDRATE          (0x75U)
#define CANIF_E_INVALID_DLC             (0x76U)
#define CANIF_E_PARAM_HOH               (0x77U)
#define CANIF_E_PARAM_LPDU              (0x78U)
#define CANIF_E_INVALID_LPDU_DATAPTR    (0x79U)
#define CANIF_E_PARAM_TRCVWAKEUPREASON  (0x7AU)
#define CANIF_E_PARAM_TRCVTYPE          (0x7BU)
#define CANIF_E_ALREADY_INITIALIZED     (0x7CU)

/*==================================================================================================
*                                    CANIF PARTIAL NETWORKING / RX DISPATCH / TX QUEUE CONFIG
==================================================================================================*/
/* Compile switch: use the Hoh-bucketed Rx dispatch lookup table built by
 * CanIf_Init (O(bucket) matching) instead of the linear scan over all Rx
 * L-PDUs. When disabled, or for hardware object handles outside the table
 * range, the linear scan is used as fallback. */
#ifndef CANIF_USE_RX_LOOKUP_TABLE
#define CANIF_USE_RX_LOOKUP_TABLE       (STD_ON)
#endif
/* Highest hardware object handle covered by the Rx dispatch lookup table */
#ifndef CANIF_RX_LOOKUP_MAX_HOH
#define CANIF_RX_LOOKUP_MAX_HOH         (16U)
#endif
/* Depth of the Tx retry queue buffering frames that Can_Write rejected with
 * CAN_BUSY; drained by CanIf_TxQueueMainFunction. */
#ifndef CANIF_TX_QUEUE_DEPTH
#define CANIF_TX_QUEUE_DEPTH            (16U)
#endif
/* Maximum payload length bufferable in one Tx queue entry (classic CAN) */
#ifndef CANIF_TX_QUEUE_DATA_LENGTH
#define CANIF_TX_QUEUE_DATA_LENGTH      (8U)
#endif

/*==================================================================================================
*                                    CANIF RETURN TYPE
==================================================================================================*/
typedef enum {
    CANIF_OK = 0,
    CANIF_NOT_OK,
    CANIF_BUSY,
    CANIF_WAKEUP_VALID,
    CANIF_WAKEUP_INVALID,
    CANIF_WAKEUP_NOT_SUPPORTED,
    CANIF_WAKEUP_CHECK_SAFETY_B_REQ
} CanIf_ReturnType;

/*==================================================================================================
*                                    CANIF NOTIF STATUS TYPE
==================================================================================================*/
typedef enum {
    CANIF_NO_NOTIFICATION = 0,
    CANIF_TX_RX_NOTIFICATION
} CanIf_NotifStatusType;

/*==================================================================================================
*                                    CANIF TX CONFIRMATION STATE TYPE
==================================================================================================*/
/** @brief Tx confirmation state of a Tx L-PDU */
typedef uint8 CanIf_TxConfirmationStateType;

#define CANIF_TXCONF_NONE       (0x00U) /* No transmit requested */
#define CANIF_TXCONF_PENDING    (0x01U) /* Transmit requested, confirmation outstanding */
#define CANIF_TXCONF_CONFIRMED  (0x02U) /* Transmit confirmed by CAN driver */

/*==================================================================================================
*                                    CANIF PDU MODE TYPE (enum values)
*==================================================================================================*/
#define CANIF_OFFLINE                   0U
#define CANIF_TX_OFFLINE                1
#define CANIF_TX_OFFLINE_ACTIVE         2
#define CANIF_ONLINE                    3

/*==================================================================================================
*                                    CANIF CONTROLLER MODE TYPE (enum values)
*==================================================================================================*/
#define CANIF_CS_UNINIT                 0
#define CANIF_CS_SLEEP                  1
#define CANIF_CS_STARTED                2U
#define CANIF_CS_STOPPED                3

/*==================================================================================================
*                                    CANIF TRANSCEIVER MODE TYPE
==================================================================================================*/
typedef enum {
    CANIF_TRCV_MODE_NORMAL = 0,
    CANIF_TRCV_MODE_STANDBY,
    CANIF_TRCV_MODE_SLEEP
} CanIf_TransceiverModeType;

/*==================================================================================================
*                                    CANIF TRANSCEIVER WAKEUP MODE TYPE
==================================================================================================*/
typedef enum {
    CANIF_TRCV_WU_ENABLE = 0,
    CANIF_TRCV_WU_DISABLE,
    CANIF_TRCV_WU_CLEAR
} CanIf_TrcvWakeupModeType;

/*==================================================================================================
*                                    CANIF TRANSCEIVER WAKEUP REASON TYPE
==================================================================================================*/
typedef enum {
    CANIF_TRCV_WU_ERROR = 0,
    CANIF_TRCV_WU_BY_BUS,
    CANIF_TRCV_WU_BY_PIN,
    CANIF_TRCV_WU_INTERNALLY,
    CANIF_TRCV_WU_NOT_SUPPORTED,
    CANIF_TRCV_WU_POWER_ON,
    CANIF_TRCV_WU_RESET,
    CANIF_TRCV_WU_BY_SYSERR
} CanIf_TrcvWakeupReasonType;

/*==================================================================================================
*                                    CANIF TX PDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType PduId;
    CanIf_CanIdType CanId;
    CanIf_CanIdTypeType CanIdType;
    CanIf_HohType Hth;
    uint8 ControllerId;
    uint8 Length;
    boolean TxConfirmation;
    boolean UserType;
    boolean FdFrame;
} CanIf_TxPduConfigType;

/*==================================================================================================
*                                    CANIF RX PDU CONFIG TYPE
==================================================================================================*/
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

/*==================================================================================================
*                                    CANIF PN WAKEUP FILTER TYPE
==================================================================================================*/
/** @brief Selective wake-up filter of a transceiver channel
 * @details A received frame passes the PN filter when
 *          (CanId AND CanIdMask) lies inside the inclusive range
 *          [(CanIdRangeLower AND CanIdMask), (CanIdRangeUpper AND CanIdMask)].
 *          The filter is armed per transceiver via CanIf_SetPnWakeupFilter.
 */
typedef struct {
    CanIf_CanIdType CanIdRangeLower;    /**< Inclusive lower bound of the wake-up CAN ID range */
    CanIf_CanIdType CanIdRangeUpper;    /**< Inclusive upper bound of the wake-up CAN ID range */
    CanIf_CanIdType CanIdMask;          /**< Mask applied to the range bounds and to received CAN IDs */
    boolean PnFilterEnabled;            /**< TRUE: filter active, non-matching frames are dropped */
} CanIf_PnWakeupFilterType;

/*==================================================================================================
*                                    CANIF HRH CONFIG TYPE
==================================================================================================*/
typedef struct {
    CanIf_HohType Hrh;
    uint8 ControllerId;
    boolean SoftwareFiltering;
} CanIf_HrhConfigType;

/*==================================================================================================
*                                    CANIF HTH CONFIG TYPE
==================================================================================================*/
typedef struct {
    CanIf_HohType Hth;
    uint8 ControllerId;
} CanIf_HthConfigType;

/*==================================================================================================
*                                    CANIF CONTROLLER CONFIG TYPE
==================================================================================================*/
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

/*==================================================================================================
*                                    CANIF CONFIG TYPE
==================================================================================================*/
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

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define CANIF_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const CanIf_ConfigType CanIf_Config;

#define CANIF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    CALLBACK FUNCTION TYPES
==================================================================================================*/
typedef void (*CanIf_TxConfirmationFncType)(PduIdType CanTxPduId);
typedef void (*CanIf_RxIndicationFncType)(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
typedef void (*CanIf_ControllerBusOffFncType)(uint8 ControllerId);
typedef void (*CanIf_ControllerModeIndicationFncType)(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define CANIF_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the CAN Interface
 * @param ConfigPtr Pointer to configuration structure
 */
void CanIf_Init(const CanIf_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the CAN Interface
 */
void CanIf_DeInit(void);

/**
 * @brief Sets the controller mode
 * @param ControllerId Controller to set
 * @param ControllerMode Mode to set
 * @return Result of operation
 */
Std_ReturnType CanIf_SetControllerMode(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);

/**
 * @brief Gets the controller mode
 * @param ControllerId Controller to get
 * @param ControllerModePtr Pointer to store mode
 * @return Result of operation
 */
Std_ReturnType CanIf_GetControllerMode(uint8 ControllerId, CanIf_ControllerModeType* ControllerModePtr);

/**
 * @brief Transmits a CAN PDU
 * @param TxPduId PDU to transmit
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Cancels a CAN transmit request
 * @param TxPduId PDU to cancel
 * @return Result of operation
 */
Std_ReturnType CanIf_CancelTransmit(PduIdType TxPduId);

/**
 * @brief Sets the PDU mode
 * @param ControllerId Controller to set
 * @param PduModeRequest Mode to set
 * @return Result of operation
 */
Std_ReturnType CanIf_SetPduMode(uint8 ControllerId, CanIf_PduModeType PduModeRequest);

/**
 * @brief Gets the PDU mode
 * @param ControllerId Controller to get
 * @param PduModePtr Pointer to store mode
 * @return Result of operation
 */
Std_ReturnType CanIf_GetPduMode(uint8 ControllerId, CanIf_PduModeType* PduModePtr);

/**
 * @brief Tx confirmation callback invoked by the CAN driver
 * @param CanTxPduId Transmitted PDU ID
 */
void CanIf_TxConfirmation(PduIdType CanTxPduId);

/**
 * @brief Gets the TX confirmation state of a Tx PDU
 * @param CanTxPduId PDU to query
 * @return Confirmation state (CANIF_TXCONF_*)
 */
CanIf_TxConfirmationStateType CanIf_GetTxConfirmationState(PduIdType CanTxPduId);

/**
 * @brief Bus-off notification callback invoked by the CAN driver
 * @param ControllerId Controller that went bus-off
 */
void CanIf_ControllerBusOff(uint8 ControllerId);

/**
 * @brief Rx indication callback invoked by the CAN driver
 * @param Mailbox Hardware object that received the frame
 * @param PduInfoPtr Received PDU data
 */
void CanIf_RxIndication(const Can_HwType* Mailbox, const PduInfoType* PduInfoPtr);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void CanIf_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Sets dynamic TX ID
 * @param CanTxPduId PDU to set
 * @param CanId CAN ID to set
 * @return Result of operation
 */
Std_ReturnType CanIf_SetDynamicTxId(PduIdType CanTxPduId, uint32 CanId);

/**
 * @brief Checks for wakeup events
 * @param WakeupSource Wakeup source to check
 * @return Result of operation
 */
Std_ReturnType CanIf_CheckWakeup(EcuM_WakeupSourceType WakeupSource);

/**
 * @brief Checks and consumes a pending wakeup validation
 * @param WakeupSource Wakeup source to validate
 * @return E_OK if a wakeup was validated since the last call, else E_NOT_OK
 */
Std_ReturnType CanIf_CheckValidation(EcuM_WakeupSourceType WakeupSource);

/**
 * @brief Sets transceiver mode
 * @param TransceiverId Transceiver to set
 * @param TransceiverMode Mode to set
 * @return Result of operation
 */
Std_ReturnType CanIf_SetTrcvMode(uint8 TransceiverId, CanIf_TransceiverModeType TransceiverMode);

/**
 * @brief Gets transceiver mode
 * @param TransceiverId Transceiver to get
 * @param TransceiverModePtr Pointer to store mode
 * @return Result of operation
 */
Std_ReturnType CanIf_GetTrcvMode(uint8 TransceiverId, CanIf_TransceiverModeType* TransceiverModePtr);

/**
 * @brief Gets transceiver wakeup reason
 * @param TransceiverId Transceiver to check
 * @param TrcvWuReasonPtr Pointer to store reason
 * @return Result of operation
 */
Std_ReturnType CanIf_GetTrcvWakeupReason(uint8 TransceiverId, CanIf_TrcvWakeupReasonType* TrcvWuReasonPtr);

/**
 * @brief Sets transceiver wakeup mode
 * @param TransceiverId Transceiver to set
 * @param TrcvWakeupMode Mode to set
 * @return Result of operation
 */
Std_ReturnType CanIf_SetTrcvWakeupMode(uint8 TransceiverId, CanIf_TrcvWakeupModeType TrcvWakeupMode);

/**
 * @brief Sets baudrate
 * @param ControllerId Controller to set
 * @param BaudRate Baudrate to set
 * @return Result of operation
 */
Std_ReturnType CanIf_SetBaudrate(uint8 ControllerId, uint16 BaudRate);

/**
 * @brief Gets baudrate
 * @param ControllerId Controller to get
 * @param BaudRatePtr Pointer to store baudrate
 * @return Result of operation
 */
Std_ReturnType CanIf_GetBaudrate(uint8 ControllerId, uint16* BaudRatePtr);

/**
 * @brief Gets the error state of a CAN controller (delegates to the Can driver)
 * @param ControllerId Controller to query
 * @param ErrorStatePtr Pointer to store the error state
 * @return E_OK on success, E_NOT_OK otherwise
 */
Std_ReturnType CanIf_GetControllerErrorState(uint8 ControllerId, Can_ErrorStateType* ErrorStatePtr);

/**
 * @brief Gets the receive error counter of a CAN controller (delegates to the Can driver)
 * @param ControllerId Controller to query
 * @param RxErrorCounterPtr Pointer to store the RX error counter
 * @return E_OK on success, E_NOT_OK otherwise
 */
Std_ReturnType CanIf_GetControllerRxErrorCounter(uint8 ControllerId, uint8* RxErrorCounterPtr);

/**
 * @brief Gets the transmit error counter of a CAN controller (delegates to the Can driver)
 * @param ControllerId Controller to query
 * @param TxErrorCounterPtr Pointer to store the TX error counter
 * @return E_OK on success, E_NOT_OK otherwise
 */
Std_ReturnType CanIf_GetControllerTxErrorCounter(uint8 ControllerId, uint8* TxErrorCounterPtr);

/**
 * @brief Reads and clears the TX notification status of a Tx L-PDU
 * @details Read-and-clear semantics: the status is returned once and reset to
 *          CANIF_NO_NOTIFICATION immediately. Only usable when
 *          ReadTxPduNotifyStatusApi is enabled in the configuration.
 * @param CanTxPduId Tx L-PDU to query
 * @return CANIF_TX_RX_NOTIFICATION if a notification is pending, else CANIF_NO_NOTIFICATION
 */
/**
 * @brief Read data from channel
 * @param[in] CanTxPduId Identifier
 * @return Operation status
 */
CanIf_NotifStatusType CanIf_ReadTxNotifStatus(PduIdType CanTxPduId);

/**
 * @brief Reads and clears the RX notification status of an Rx L-PDU
 * @details Read-and-clear semantics: the status is returned once and reset to
 *          CANIF_NO_NOTIFICATION immediately. Only usable when
 *          ReadRxPduNotifyStatusApi is enabled in the configuration.
 * @param CanRxPduId Rx L-PDU to query
 * @return CANIF_TX_RX_NOTIFICATION if a notification is pending, else CANIF_NO_NOTIFICATION
 */
/**
 * @brief Read data from channel
 * @param[in] CanRxPduId Identifier
 * @return Operation status
 */
CanIf_NotifStatusType CanIf_ReadRxNotifStatus(PduIdType CanRxPduId);

/**
 * @brief Trigger-transmit data request for a Tx L-PDU
 * @details For PDUs configured for trigger transmit (TxPduConfig.UserType), the
 *          data cached by the most recent CanIf_Transmit call is copied into the
 *          buffer provided by the caller. The copied length is
 *          min(requested length, configured length, cached length).
 * @param TxPduId Tx L-PDU to query
 * @param PduInfoPtr Buffer descriptor (SduDataPtr/SduLength) to fill
 * @return E_OK if data was provided, E_NOT_OK otherwise
 */
/**
 * @brief Trigger action
 * @param[in] TxPduId Identifier
 * @param[in] PduInfoPtr Pointer reference
 * @return Operation status
 */
Std_ReturnType CanIf_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

/**
 * @brief Confirms partial-networking availability for a transceiver channel
 * @param TransceiverId Transceiver to confirm
 * @return E_OK on success, E_NOT_OK otherwise
 */
Std_ReturnType CanIf_ConfirmPnAvailability(uint8 TransceiverId);

/**
 * @brief Requests a wake-flag check on the given transceiver
 * @param TransceiverId Transceiver to check
 * @return E_OK on success, E_NOT_OK otherwise
 */
Std_ReturnType CanIf_CheckTrcvWakeFlag(uint8 TransceiverId);

/**
 * @brief Requests the wake-up flag of the given transceiver to be cleared
 * @param TransceiverId Transceiver to clear
 * @return E_OK on success, E_NOT_OK otherwise
 */
Std_ReturnType CanIf_ClearTrcvWufFlag(uint8 TransceiverId);

/**
 * @brief Wake-flag check indication invoked by the CAN transceiver driver
 * @param TransceiverId Transceiver that completed the wake-flag check
 */
void CanIf_CheckTrcvWakeFlagIndication(uint8 TransceiverId);

/**
 * @brief Wake-up-flag cleared indication invoked by the CAN transceiver driver
 * @param TransceiverId Transceiver whose wake-up flag was cleared
 */
void CanIf_ClearTrcvWufFlagIndication(uint8 TransceiverId);

/**
 * @brief Transceiver mode indication invoked by the CAN transceiver driver
 * @param TransceiverId Transceiver that changed mode
 * @param TransceiverMode New transceiver mode
 */
void CanIf_TrcvModeIndication(uint8 TransceiverId, CanIf_TransceiverModeType TransceiverMode);

/**
 * @brief Controller mode indication invoked by the CAN driver
 * @param ControllerId Controller that changed mode
 * @param ControllerMode New controller mode
 */
void CanIf_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);

/**
 * @brief Converts a 29-bit CAN identifier into the 4-byte big-endian MetaData layout
 * @details MetaData[0] = CanId bits 28..21, MetaData[1] = bits 20..13,
 *          MetaData[2] = bits 12..5, MetaData[3] = bits 7..0.
 * @param CanId CAN identifier to convert
 * @param MetaDataPtr 4-byte MetaData buffer to fill
 * @return E_OK on success, E_NOT_OK on NULL pointer
 */
/**
 * @brief can id to meta data
 * @param[in] CanId Identifier
 * @param[in] MetaDataPtr Data buffer
 * @return Operation status
 */
Std_ReturnType CanIf_CanIdToMetaData(uint32 CanId, uint8* MetaDataPtr);

/**
 * @brief Converts the 4-byte big-endian MetaData layout back into a 29-bit CAN identifier
 * @param MetaDataPtr 4-byte MetaData buffer to read
 * @param CanIdPtr Storage for the reconstructed CAN identifier
 * @return E_OK on success, E_NOT_OK on NULL pointer
 */
Std_ReturnType CanIf_MetaDataToCanId(const uint8* MetaDataPtr, uint32* CanIdPtr);

/**
 * @brief Arms the selective wake-up (PN) filter of a transceiver channel
 * @details When enabled, CanIf_RxIndication drops every frame whose (masked)
 *          CAN ID lies outside the configured range. The filter is reset by
 *          CanIf_Init / CanIf_DeInit.
 * @param TransceiverId Transceiver whose filter is configured
 * @param PnFilter Filter definition (range/mask/enabled), copied by CanIf
 * @return E_OK on success, E_NOT_OK on invalid transceiver, NULL pointer or
 *         an inverted CAN ID range
 */
/**
 * @brief Set configuration value
 * @param[in] TransceiverId Identifier
 * @param[in] PnFilter PnFilter value
 * @return Operation status
 */
Std_ReturnType CanIf_SetPnWakeupFilter(uint8 TransceiverId, const CanIf_PnWakeupFilterType* PnFilter);

/**
 * @brief Drains the Tx retry queue (frames buffered on CAN_BUSY)
 * @details Called cyclically from the CanIf main function context. Every
 *          queued frame is retried via Can_Write; on success the Tx
 *          confirmation state is armed, on CAN_BUSY the remaining frames stay
 *          queued for the next cycle. The queue is flushed by CanIf_Init,
 *          CanIf_DeInit and CanIf_ControllerBusOff.
 */
/**
 * @brief Transmit data
 * @param[in] TransceiverId Identifier
 * @param[in] PnFilter PnFilter value
 */
void CanIf_TxQueueMainFunction(void);

#define CANIF_STOP_SEC_CODE
#include "MemMap.h"

#endif /* CANIF_H */
