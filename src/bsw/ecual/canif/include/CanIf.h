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
#define CANIF_SID_SETBAUDRATE           (0x27U)
#define CANIF_SID_GETBAUDRATE           (0x28U)
#define CANIF_SID_GETCONTROLLERRXERRORCOUNTER (0x4CU)
#define CANIF_SID_GETCONTROLLERTXERRORCOUNTER (0x4DU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define CANIF_E_PARAM_CANID             (0x01U)
#define CANIF_E_PARAM_DLC               (0x02U)
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
*                                    CANIF PDU MODE TYPE
==================================================================================================*/
typedef enum {
    CANIF_OFFLINE = 0,
    CANIF_TX_OFFLINE,
    CANIF_TX_OFFLINE_ACTIVE,
    CANIF_ONLINE
} CanIf_PduModeType;

/*==================================================================================================
*                                    CANIF CONTROLLER MODE TYPE
==================================================================================================*/
typedef enum {
    CANIF_CS_UNINIT = 0,
    CANIF_CS_SLEEP,
    CANIF_CS_STARTED,
    CANIF_CS_STOPPED
} CanIf_ControllerModeType;

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
    Can_IdType CanId;
    CanIf_CanIdTypeType CanIdType;
    Can_HwHandleType Hth;
    uint8 ControllerId;
    uint8 Length;
    boolean TxConfirmation;
    boolean UserType;
} CanIf_TxPduConfigType;

/*==================================================================================================
*                                    CANIF RX PDU CONFIG TYPE
==================================================================================================*/
typedef struct {
    PduIdType PduId;
    Can_IdType CanId;
    Can_IdType CanIdMask;
    CanIf_CanIdTypeType CanIdType;
    Can_HwHandleType Hrh;
    uint8 ControllerId;
    uint8 Length;
    boolean RxIndication;
} CanIf_RxPduConfigType;

/*==================================================================================================
*                                    CANIF HRH CONFIG TYPE
==================================================================================================*/
typedef struct {
    Can_HwHandleType Hrh;
    uint8 ControllerId;
    boolean SoftwareFiltering;
} CanIf_HrhConfigType;

/*==================================================================================================
*                                    CANIF HTH CONFIG TYPE
==================================================================================================*/
typedef struct {
    Can_HwHandleType Hth;
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
Std_ReturnType CanIf_SetDynamicTxId(PduIdType CanTxPduId, Can_IdType CanId);

/**
 * @brief Checks for wakeup events
 * @param WakeupSource Wakeup source to check
 * @return Result of operation
 */
Std_ReturnType CanIf_CheckWakeup(EcuM_WakeupSourceType WakeupSource);

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

#define CANIF_STOP_SEC_CODE
#include "MemMap.h"

#endif /* CANIF_H */
