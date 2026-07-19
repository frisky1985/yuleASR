/**
 * @file CanTSyn.h
 * @brief CAN Time Synchronization - AutoSAR R22-11 Service Layer
 * @version 4.7.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: CAN Time Synchronization (CANTSYN)
 * Module ID: 0xA4U
 * Layer: Service Layer
 *
 * @details
 * The CanTSyn module provides services for time synchronization over CAN bus.
 * It supports both Time Master and Time Slave functionality for global time
 * distribution in distributed automotive systems.
 *
 * Key Features:
 * - Time Master: Transmits SYNC and FUP messages
 * - Time Slave: Receives SYNC/FUP and adjusts local time
 * - User Data (OCS) support for application-specific time data
 * - Microsecond-level synchronization precision
 * - Integration with StbM (Synchronized Time-base Manager)
 */

#ifndef CANTSYN_H
#define CANTSYN_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "CanTSyn_Cfg.h"
#include "ComStack_Types.h"
#include "StbM.h"
#include "CanIf.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define CANTSYN_VENDOR_ID                       (0x01U) /* YuleTech Vendor ID */
#define CANTSYN_MODULE_ID                       (0xA4U) /* CANTSYN Module ID */
#define CANTSYN_INSTANCE_ID                     (0x00U)

#define CANTSYN_AR_RELEASE_MAJOR_VERSION        (0x22U)
#define CANTSYN_AR_RELEASE_MINOR_VERSION        (0x11U)
#define CANTSYN_AR_RELEASE_REVISION_VERSION     (0x00U)

#define CANTSYN_SW_MAJOR_VERSION                (0x04U)
#define CANTSYN_SW_MINOR_VERSION                (0x07U)
#define CANTSYN_SW_PATCH_VERSION                (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define CANTSYN_SID_INIT                        (0x01U)
#define CANTSYN_SID_DEINIT                      (0x02U)
#define CANTSYN_SID_GETVERSIONINFO              (0x03U)
#define CANTSYN_SID_SETTRANSMISSIONMODE         (0x04U)
#define CANTSYN_SID_GETSYNCRECEIVED             (0x05U)
#define CANTSYN_SID_GETCURRENTTIME              (0x06U)
#define CANTSYN_SID_GETCURRENTVIRTUALTIME       (0x07U)
#define CANTSYN_SID_SETGLOBALTIME               (0x08U)
#define CANTSYN_SID_SETRATECORRECTION           (0x09U)
#define CANTSYN_SID_RXINDICATION                (0x0AU)
#define CANTSYN_SID_TXCONFIRMATION              (0x0BU)
#define CANTSYN_SID_MAINFUNCTION                (0x0CU)
#define CANTSYN_SID_SETUSERDATA                 (0x0DU)
#define CANTSYN_SID_GETUSERDATA                 (0x0EU)
#define CANTSYN_SID_TIMETXCONFIRMATION          (0x0FU)
#define CANTSYN_SID_TIMETXCONFIRMATIONSYNC      (0x10U)
#define CANTSYN_SID_TIMETXCONFIRMATIONFUP       (0x11U)
#define CANTSYN_SID_TIMETXCONFIRMATIONOCS       (0x12U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define CANTSYN_E_NO_ERROR                      (0x00U)
#define CANTSYN_E_PARAM_POINTER                 (0x01U)
#define CANTSYN_E_PARAM_CONFIG                  (0x02U)
#define CANTSYN_E_UNINIT                        (0x03U)
#define CANTSYN_E_ALREADY_INITIALIZED           (0x04U)
#define CANTSYN_E_INVALID_TIMEBASE_ID           (0x05U)
#define CANTSYN_E_INVALID_DOMAIN_ID             (0x06U)
#define CANTSYN_E_INVALID_PDU_ID                (0x07U)
#define CANTSYN_E_INVALID_CAN_ID                (0x08U)
#define CANTSYN_E_INVALID_DLC                   (0x09U)
#define CANTSYN_E_SYNC_LOST                     (0x0AU)
#define CANTSYN_E_TIME_NOT_AVAILABLE            (0x0BU)
#define CANTSYN_E_TRANSMISSION_FAILED           (0x0CU)

/*==================================================================================================
*                                    TRANSMISSION MODES
==================================================================================================*/
/** @brief Time transmission mode - Immediate (time in SYNC message) */
#define CANTSYN_TX_MODE_IMMEDIATE               (0x00U)

/** @brief Time transmission mode - Delayed (time in FUP message) */
#define CANTSYN_TX_MODE_DELAYED                 (0x01U)

/** @brief Time transmission mode - Off */
#define CANTSYN_TX_MODE_OFF                     (0x02U)

/*==================================================================================================
*                                    MESSAGE TYPES
==================================================================================================*/
/** @brief SYNC message type (Synchronization) */
#define CANTSYN_MSG_TYPE_SYNC                   (0x00U)

/** @brief FUP message type (Follow Up) */
#define CANTSYN_MSG_TYPE_FUP                    (0x01U)

/** @brief OCS message type (Optional Content SYNC - User Data) */
#define CANTSYN_MSG_TYPE_OCS                    (0x02U)

/** @brief Sync message with CRC */
#define CANTSYN_MSG_TYPE_SYNC_CRC               (0x04U)

/** @brief FUP message with CRC */
#define CANTSYN_MSG_TYPE_FUP_CRC                (0x05U)

/** @brief OCS message with CRC */
#define CANTSYN_MSG_TYPE_OCS_CRC                (0x06U)

/*==================================================================================================
*                                    SGW (SYNC Gate Way) VALUES
==================================================================================================*/
/** @brief SGW - Time Base is not provided by a synchronization gateway */
#define CANTSYN_SGW_NONE                        (0x00U)

/** @brief SGW - Time Base is provided by a synchronization gateway */
#define CANTSYN_SGW_ACTIVE                      (0x01U)

/*==================================================================================================
*                                    TIME DOMAIN VALUES
==================================================================================================*/
/** @brief Time Domain - Global Time */
#define CANTSYN_TIME_DOMAIN_GLOBAL              (0x00U)

/** @brief Time Domain - Offset Time */
#define CANTSYN_TIME_DOMAIN_OFFSET              (0x01U)

/*==================================================================================================
*                                    TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief CanTSyn time base configuration type
 */
typedef struct {
    uint8 timeBaseId;                       /**< Time base ID */
    uint8 domainId;                         /**< Time domain ID */
    uint8 masterConfig;                     /**< Master configuration (0: None, 1: Slave, 2: Master) */
    boolean IsTimeMaster;                   /**< TRUE if this time domain acts as master */
    PduIdType TxPduId;                      /**< TX PDU ID for SYNC messages */
    uint32 syncPeriodMs;                    /**< SYNC message period in ms */
    uint32 debounceTimeMs;                  /**< Debounce time in ms */
    uint32 syncTimeoutMs;                   /**< Sync timeout in ms */
    boolean crcSecured;                     /**< CRC secured mode */
    boolean useImmediateTransmission;       /**< Use immediate transmission mode */
    uint32 syncCanId;                   /**< CAN ID for SYNC messages */
    uint32 fupCanId;                    /**< CAN ID for FUP messages */
    uint32 ocsCanId;                    /**< CAN ID for OCS messages */
    PduIdType syncTxPduId;                  /**< Tx PDU ID for SYNC */
    PduIdType fupTxPduId;                   /**< Tx PDU ID for FUP */
    PduIdType ocsTxPduId;                   /**< Tx PDU ID for OCS */
    PduIdType syncRxPduId;                  /**< Rx PDU ID for SYNC */
    PduIdType fupRxPduId;                   /**< Rx PDU ID for FUP */
    PduIdType ocsRxPduId;                   /**< Rx PDU ID for OCS */
} CanTSyn_TimeBaseConfigType;

/**
 * @brief CanTSyn transmission mode configuration type
 */
typedef struct {
    uint8 timeBaseId;                       /**< Time base ID */
    uint8 txMode;                           /**< Transmission mode (Immediate/Delayed/Off) */
    uint8 debounceCounter;                  /**< Debounce counter value */
} CanTSyn_TransmissionModeType;

/**
 * @brief CanTSyn message data structure (SYNC/FUP/OCS)
 */
typedef struct {
    uint8 type;                             /**< Message type */
    uint8 sequenceCounter;                  /**< Sequence counter */
    uint8 messageCounter;                   /**< Message counter */
    uint8 sgw;                              /**< Synchronization Gateway info */
    uint8 ofs;                              /**< Offset info */
    StbM_TimeStampType timeStamp;           /**< Time stamp */
    StbM_UserDataType userData;             /**< User data (for OCS) */
    uint8 crc;                              /**< CRC (if enabled) */
} CanTSyn_MessageType;

/**
 * @brief CanTSyn time slave configuration type
 */
typedef struct {
    uint8 timeBaseId;                       /**< Time base ID */
    uint8 domainId;                         /**< Time domain ID */
    uint32 timeoutMs;                       /**< Timeout for sync in ms */
    uint8 syncLostThreshold;                /**< Number of missed SYNCs before sync lost */
    boolean crcCheck;                       /**< CRC check enabled */
} CanTSyn_SlaveConfigType;

/**
 * @brief CanTSyn time master configuration type
 */
typedef struct {
    uint8 timeBaseId;                       /**< Time base ID */
    uint8 domainId;                         /**< Time domain ID */
    uint32 syncPeriodMs;                    /**< SYNC period in ms */
    boolean immediateMode;                  /**< Immediate transmission mode */
    boolean crcSecured;                     /**< CRC secured mode */
    uint8 debounceValue;                    /**< Debounce counter value */
} CanTSyn_MasterConfigType;

/**
 * @brief CanTSyn global configuration type
 */
typedef struct {
    const CanTSyn_TimeBaseConfigType* timeBaseConfigs;  /**< Array of time base configs */
    const CanTSyn_SlaveConfigType* slaveConfigs;        /**< Array of slave configs */
    const CanTSyn_MasterConfigType* masterConfigs;      /**< Array of master configs */
    uint8 numTimeBases;                     /**< Number of time bases */
    uint8 numSlaves;                        /**< Number of slave configurations */
    uint8 numMasters;                       /**< Number of master configurations */
    boolean devErrorDetect;                 /**< Development error detection */
    boolean versionInfoApi;                 /**< Version info API enabled */
} CanTSyn_ConfigType;

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define CANTSYN_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const CanTSyn_ConfigType CanTSyn_Config;

#define CANTSYN_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define CANTSYN_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the CanTSyn module
 * @param ConfigPtr Pointer to configuration structure
 * @pre None
 * @post CanTSyn module initialized and ready for operation
 */
void CanTSyn_Init(const CanTSyn_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the CanTSyn module
 * @pre CanTSyn module must be initialized
 * @post CanTSyn module deinitialized
 */
void CanTSyn_DeInit(void);

/**
 * @brief Gets version information of the CanTSyn module
 * @param versioninfo Pointer to version info structure to store the information
 * @pre None
 * @post Version information stored in versioninfo
 */
#if (CANTSYN_VERSION_INFO_API == STD_ON)
void CanTSyn_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Sets the transmission mode for a time base
 * @param timeBaseId Time base ID
 * @param txMode Transmission mode (CANTSYN_TX_MODE_IMMEDIATE/DELAYED/OFF)
 * @return E_OK: Success, E_NOT_OK: Error
 * @pre CanTSyn module must be initialized
 */
Std_ReturnType CanTSyn_SetTransmissionMode(uint8 timeBaseId, uint8 txMode);

/**
 * @brief Gets the transmission mode for a time base
 * @param timeBaseId Time base ID
 * @param txModePtr Pointer to store transmission mode
 * @return E_OK: Success, E_NOT_OK: Error
 * @pre CanTSyn module must be initialized
 */
Std_ReturnType CanTSyn_GetTransmissionMode(uint8 timeBaseId, uint8* txModePtr);

/**
 * @brief Checks if valid SYNC has been received for a time base
 * @param timeBaseId Time base ID
 * @return TRUE: Valid SYNC received, FALSE: No valid SYNC
 * @pre CanTSyn module must be initialized
 */
boolean CanTSyn_GetSyncReceived(uint8 timeBaseId);

/**
 * @brief Gets current synchronized time for a time base
 * @param timeBaseId Time base ID
 * @param timeStampPtr Pointer to store time stamp
 * @param userDataPtr Pointer to store user data (can be NULL)
 * @return E_OK: Success, E_NOT_OK: Error
 * @pre CanTSyn module must be initialized
 */
Std_ReturnType CanTSyn_GetCurrentTime(uint8 timeBaseId, 
                                      StbM_TimeStampType* timeStampPtr,
                                      StbM_UserDataType* userDataPtr);

/**
 * @brief Gets current virtual time for a time base
 * @param timeBaseId Time base ID
 * @param virtualTimePtr Pointer to store virtual time
 * @return E_OK: Success, E_NOT_OK: Error
 * @pre CanTSyn module must be initialized
 */
Std_ReturnType CanTSyn_GetCurrentVirtualTime(uint8 timeBaseId, 
                                              StbM_VirtualLocalTimeType* virtualTimePtr);

/**
 * @brief Sets global time for a time base (called by application)
 * @param timeBaseId Time base ID
 * @param timeStampPtr Pointer to new time stamp
 * @param userDataPtr Pointer to user data (can be NULL)
 * @return E_OK: Success, E_NOT_OK: Error
 * @pre CanTSyn module must be initialized, timeBaseId must be configured as Master
 */
Std_ReturnType CanTSyn_SetGlobalTime(uint8 timeBaseId,
                                     const StbM_TimeStampType* timeStampPtr,
                                     const StbM_UserDataType* userDataPtr);

/**
 * @brief Sets rate correction for a time base
 * @param timeBaseId Time base ID
 * @param rateCorrection Rate correction value in parts per million (ppm)
 * @return E_OK: Success, E_NOT_OK: Error
 * @pre CanTSyn module must be initialized
 */
Std_ReturnType CanTSyn_SetRateCorrection(uint8 timeBaseId, sint32 rateCorrection);

/**
 * @brief Sets user data for a time base
 * @param timeBaseId Time base ID
 * @param userDataPtr Pointer to user data
 * @return E_OK: Success, E_NOT_OK: Error
 * @pre CanTSyn module must be initialized
 */
Std_ReturnType CanTSyn_SetUserData(uint8 timeBaseId, const StbM_UserDataType* userDataPtr);

/**
 * @brief Gets user data for a time base
 * @param timeBaseId Time base ID
 * @param userDataPtr Pointer to store user data
 * @return E_OK: Success, E_NOT_OK: Error
 * @pre CanTSyn module must be initialized
 */
Std_ReturnType CanTSyn_GetUserData(uint8 timeBaseId, StbM_UserDataType* userDataPtr);

/**
 * @brief Rx indication callback from CanIf
 * @param RxPduId Rx PDU ID
 * @param PduInfoPtr Pointer to PDU info containing received data
 * @pre CanTSyn module must be initialized
 * @post Received message processed
 */
void CanTSyn_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Tx confirmation callback from CanIf
 * @param TxPduId Tx PDU ID
 * @param result Result of transmission
 * @pre CanTSyn module must be initialized
 * @post Transmission confirmed, next message can be sent
 */
void CanTSyn_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/**
 * @brief Time Tx confirmation callback for SYNC messages
 * @param TxPduId Tx PDU ID
 * @param timeStampPtr Hardware timestamp of transmission
 * @pre CanTSyn module must be initialized
 */
void CanTSyn_TimeTxConfirmationSYNC(PduIdType TxPduId, const StbM_TimeStampType* timeStampPtr);

/**
 * @brief Time Tx confirmation callback for FUP messages
 * @param TxPduId Tx PDU ID
 * @param timeStampPtr Hardware timestamp of transmission
 * @pre CanTSyn module must be initialized
 */
void CanTSyn_TimeTxConfirmationFUP(PduIdType TxPduId, const StbM_TimeStampType* timeStampPtr);

/**
 * @brief Time Tx confirmation callback for OCS messages
 * @param TxPduId Tx PDU ID
 * @param timeStampPtr Hardware timestamp of transmission
 * @pre CanTSyn module must be initialized
 */
void CanTSyn_TimeTxConfirmationOCS(PduIdType TxPduId, const StbM_TimeStampType* timeStampPtr);

/**
 * @brief Main function for periodic processing
 * @details Handles SYNC/FUP transmission timing and timeout detection
 * @pre CanTSyn module must be initialized
 * @post Periodic processing completed
 */
void CanTSyn_MainFunction(void);

#define CANTSYN_STOP_SEC_CODE
#include "MemMap.h"

#endif /* CANTSYN_H */
