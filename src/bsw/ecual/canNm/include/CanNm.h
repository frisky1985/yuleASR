/**
 * @file CanNm.h
 * @brief CAN Network Management header file
 * @version 4.4.0
 *
 * AUTOSAR CAN Network Management Module
 * Following AUTOSAR_SWS_CANNetworkManagement specification version 4.4.0
 */

#ifndef CANNM_H
#define CANNM_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                           INCLUDES
 ==================================================================================================*/
#include "ComStack_Types.h"
#include "CanNm_Cfg.h"
#include "Std_Types.h"

/*==================================================================================================
 *                                     VERSION INFORMATION
 ==================================================================================================*/
#define CANNM_VENDOR_ID                     (uint16)0x0001U
#define CANNM_MODULE_ID                     (uint16)0x001FU

#define CANNM_SW_MAJOR_VERSION              (uint8)4U
#define CANNM_SW_MINOR_VERSION              (uint8)4U
#define CANNM_SW_PATCH_VERSION              (uint8)0U

#define CANNM_AR_MAJOR_VERSION              (uint8)4U
#define CANNM_AR_MINOR_VERSION              (uint8)4U
#define CANNM_AR_PATCH_VERSION              (uint8)0U

/*==================================================================================================
 *                                     SERVICE IDs (APIs)
 ==================================================================================================*/
#define CANNM_SID_INIT                      (uint8)0x00U
#define CANNM_SID_DEINIT                    (uint8)0x01U
#define CANNM_SID_PASSIVE_STARTUP           (uint8)0x01U
#define CANNM_SID_NETWORK_REQUEST           (uint8)0x02U
#define CANNM_SID_NETWORK_RELEASE           (uint8)0x03U
#define CANNM_SID_SET_USER_DATA             (uint8)0x04U
#define CANNM_SID_GET_USER_DATA             (uint8)0x05U
#define CANNM_SID_GET_NODE_ID               (uint8)0x06U
#define CANNM_SID_GET_LOCAL_NODE_ID         (uint8)0x07U
#define CANNM_SID_REPEAT_MESSAGE_REQUEST    (uint8)0x08U
#define CANNM_SID_GET_PDUDATA               (uint8)0x0AU
#define CANNM_SID_GET_STATE                 (uint8)0x0BU
#define CANNM_SID_DISABLE_COMMUNICATION     (uint8)0x0CU
#define CANNM_SID_ENABLE_COMMUNICATION      (uint8)0x0DU
#define CANNM_SID_GET_VERSION_INFO          (uint8)0x0FU
#define CANNM_SID_REQUEST_BUS_SYNCHRONIZATION   (uint8)0x10U
#define CANNM_SID_CHECK_REMOTE_SLEEP_INDICATION (uint8)0x11U
#define CANNM_SID_SET_SLEEP_READY_BIT       (uint8)0x12U
#define CANNM_SID_TX_CONFIRMATION           (uint8)0x40U
#define CANNM_SID_RX_INDICATION             (uint8)0x42U
#define CANNM_SID_MAIN_FUNCTION             (uint8)0x14U
#define CANNM_SID_CONFIRM_PN_AVAILABILITY   (uint8)0x15U
#define CANNM_SID_TRANSMIT                  (uint8)0x16U

/*==================================================================================================
 *                                     ERROR CODES
 ==================================================================================================*/
#define CANNM_E_NO_ERROR                    (uint8)0x00U
#define CANNM_E_NOT_INITIALIZED             (uint8)0x01U
#define CANNM_E_INVALID_CHANNEL             (uint8)0x02U
#define CANNM_E_INVALID_PDUID               (uint8)0x03U
#define CANNM_E_NETWORK_TIMEOUT             (uint8)0x04U
#define CANNM_E_INIT_FAILED                 (uint8)0x05U
#define CANNM_E_PARAM_POINTER               (uint8)0x06U
#define CANNM_E_ALREADY_INITIALIZED         (uint8)0x07U
#define CANNM_E_PDUR_TRIGGERTRANSMIT        (uint8)0x08U

/*==================================================================================================
 *                                    MACRO DEFINITIONS
 ==================================================================================================*/

/** @brief NM message byte definitions (AUTOSAR 4.4.0) */
#define CANNM_PDU_BYTE_0                    (uint8)0x00U
#define CANNM_PDU_BYTE_1                    (uint8)0x01U
#define CANNM_PDU_BYTE_N                    (uint8)0x07U

/** @brief Control Bit Vector (CBV) masks */
#define CANNM_CBV_REPEAT_MSG_REQUEST        (uint8)0x01U
#define CANNM_CBV_NM_COORD_SLEEP_BIT        (uint8)0x10U
#define CANNM_CBV_ACTIVE_WAKEUP_BIT         (uint8)0x04U
#define CANNM_CBV_RESERVED_MASK             (uint8)0xEAU

/** @brief Maximum number of channels */
#if (CANNM_NUMBER_OF_CHANNELS == 0U)
#error "CANNM_NUMBER_OF_CHANNELS must be greater than 0"
#endif

/*==================================================================================================
 *                                     TYPE DEFINITIONS
 ==================================================================================================*/

/**
 * @brief CAN NM Network states
 */
typedef enum
{
    CANNM_STATE_UNINIT = 0,       /**< Module is not initialized */
    CANNM_STATE_BUS_SLEEP_MODE,   /**< Bus Sleep Mode */
    CANNM_STATE_PREPARE_BUS_SLEEP_MODE, /**< Prepare Bus Sleep Mode */
    CANNM_STATE_READY_SLEEP_MODE, /**< Ready Sleep Mode */
    CANNM_STATE_NORMAL_OPERATION_MODE,  /**< Normal Operation Mode */
    CANNM_STATE_REPEAT_MESSAGE_MODE     /**< Repeat Message Mode */
} CanNm_StateType;

/**
 * @brief CAN NM mode type (mapped to ComM)
 */
typedef enum
{
    CANNM_MODE_BUS_SLEEP = 0,     /**< Bus Sleep */
    CANNM_MODE_PREPARE_BUS_SLEEP, /**< Prepare Bus Sleep */
    CANNM_MODE_SYNCHRONIZE,       /**< Synchronize */
    CANNM_MODE_NETWORK            /**< Network Mode */
} CanNm_ModeType;

/**
 * @brief CAN NM internal state type
 */
typedef enum
{
    CANNM_NETWORK_MODE_STATE_REPEAT_MESSAGE = 0,
    CANNM_NETWORK_MODE_STATE_NORMAL_OPERATION,
    CANNM_NETWORK_MODE_STATE_READY_SLEEP
} CanNm_NetworkModeStateType;

/**
 * @brief CAN NM channel configuration type
 */
typedef struct
{
    uint8                       ChannelId;              /**< Channel identifier */
    uint16                      NmTimeoutTime;          /**< NM timeout time in ms */
    uint16                      RepeatMessageTime;      /**< Repeat message time in ms */
    uint16                      WaitBusSleepTime;       /**< Wait bus sleep time in ms */
    uint16                      MessageCycleTime;       /**< Message cycle time in ms */
    uint16                      MessageCycleOffset;     /**< Message cycle offset in ms */
    uint16                      ImmediateNmCycleTime;   /**< Immediate NM cycle time in ms */
    uint8                       ImmediateNmTransmissions; /**< Number of immediate transmissions */
    uint8                       NidPosition;            /**< Node ID position in PDU */
    uint8                       CbvPosition;            /**< CBV position in PDU */
    boolean                     NodeDetectionEnabled;   /**< Node detection enabled */
    boolean                     NodeIdEnabled;          /**< Node ID enabled */
    boolean                     PassiveModeEnabled;     /**< Passive mode enabled */
    boolean                     RemoteSleepIndEnabled;  /**< Remote sleep indication enabled */
    boolean                     ActiveWakeupBitEnabled; /**< Active wakeup bit enabled */
    boolean                     ComControlEnabled;      /**< Communication control enabled */
    boolean                     CoordinatorSyncSupport; /**< Coordinator sync support */
    uint8                       PduLength;              /**< PDU length */
    uint8                       NodeId;                 /**< Node ID */
    PduIdType                   TxPduId;              /**< Transmit PDU ID */
    PduIdType                   RxPduId;              /**< Receive PDU ID */
} CanNm_ChannelConfigType;

/**
 * @brief CAN NM global configuration type
 */
typedef struct
{
    const CanNm_ChannelConfigType*  ChannelConfig;      /**< Channel configurations */
    uint8                           ChannelCount;       /**< Number of channels */
    boolean                         VersionInfoApi;     /**< Version info API enabled */
    boolean                         PassiveModeEnabled; /**< Passive mode enabled globally */
    boolean                         PnEnabled;          /**< Partial networking enabled */
    boolean                         StateReportEnabled; /**< State change report enabled */
} CanNm_ConfigType;

/**
 * @brief CAN NM internal channel state structure
 */
typedef struct
{
    CanNm_StateType             State;                  /**< Current NM state */
    uint8                       TxPduBuffer[8];         /**< Transmit PDU buffer */
    uint8                       RxPduBuffer[8];         /**< Receive PDU buffer */
    uint16                      NmTimeoutTimer;         /**< NM timeout timer */
    uint16                      MessageCycleTimer;      /**< Message cycle timer */
    uint16                      RepeatMessageTimer;     /**< Repeat message timer */
    uint16                      WaitBusSleepTimer;      /**< Wait bus sleep timer */
    uint16                      RemoteSleepIndTimer;    /**< Remote sleep indication timer */
    uint8                       ImmediateTxCounter;     /**< Immediate transmission counter */
    uint8                       RptMsgReqCounter;       /**< Repeat message request counter */
    boolean                     CommunicationEnabled;   /**< Communication control state */
    boolean                     NetworkRequested;       /**< Network request flag */
    boolean                     RptMsgRequestPending;   /**< Repeat message request pending */
    boolean                     RemoteSleepInd;         /**< Remote sleep indication */
    boolean                     RemoteSleepIndEnabled;  /**< Remote sleep indication enabled */
    boolean                     TxConfirmationPending;  /**< TX confirmation pending */
    boolean                     MsgTxEnabled;           /**< Message transmission enabled */
} CanNm_ChannelStateType;

/*==================================================================================================
 *                                       CONSTANTS
 ==================================================================================================*/

extern const CanNm_ConfigType CanNm_Config;

/*==================================================================================================
 *                                     GLOBAL FUNCTIONS
 ==================================================================================================*/

/**
 * @brief Initialize the CAN NM module
 * @param[in] ConfigPtr Pointer to configuration structure
 */
extern void CanNm_Init(const CanNm_ConfigType* ConfigPtr);

/**
 * @brief De-initialize the CAN NM module
 */
extern void CanNm_DeInit(void);

/**
 * @brief Passive startup of a CAN NM channel
 * @param[in] nmChannelHandle Network channel handle
 * @return E_OK: Successfully entered network mode
 *         E_NOT_OK: Entering network mode failed
 */
extern Std_ReturnType CanNm_PassiveStartUp(NetworkHandleType nmChannelHandle);

/**
 * @brief Request network mode on a channel
 * @param[in] nmChannelHandle Network channel handle
 * @return E_OK: Successfully entered network mode
 *         E_NOT_OK: Entering network mode failed
 */
extern Std_ReturnType CanNm_NetworkRequest(NetworkHandleType nmChannelHandle);

/**
 * @brief Release network mode on a channel
 * @param[in] nmChannelHandle Network channel handle
 * @return E_OK: Successfully released network mode
 *         E_NOT_OK: Releasing network mode failed
 */
extern Std_ReturnType CanNm_NetworkRelease(NetworkHandleType nmChannelHandle);

/**
 * @brief Main function for CAN NM (periodic processing)
 */
extern void CanNm_MainFunction(void);

/**
 * @brief Transmit function for CAN NM PDUs
 * @param[in] TxPduId PDU ID to transmit
 * @param[in] PduInfoPtr Pointer to PDU info structure
 * @return E_OK: Transmission successful
 *         E_NOT_OK: Transmission failed
 */
extern Std_ReturnType CanNm_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief RX indication callback from CAN Interface
 * @param[in] RxPduId Receive PDU ID
 * @param[in] PduInfoPtr Pointer to PDU info structure
 */
extern void CanNm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief TX confirmation callback from CAN Interface
 * @param[in] TxPduId Transmit PDU ID
 * @param[in] result Transmission result
 */
extern void CanNm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/**
 * @brief Trigger transmit callback from PDU Router
 * @param[in] TxPduId Transmit PDU ID
 * @param[in,out] PduInfoPtr Pointer to PDU info structure
 * @return E_OK: Data copied successfully
 *         E_NOT_OK: Data copy failed
 */
extern Std_ReturnType CanNm_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);

/**
 * @brief Confirm partial networking availability
 * @param[in] nmChannelHandle Network channel handle
 */
extern void CanNm_ConfirmPnAvailability(NetworkHandleType nmChannelHandle);

/**
 * @brief Get current state of the CAN NM
 * @param[in] nmChannelHandle Network channel handle
 * @param[out] nmStatePtr Pointer to store state
 * @param[out] nmModePtr Pointer to store mode
 * @return E_OK: State retrieved successfully
 *         E_NOT_OK: State retrieval failed
 */
extern Std_ReturnType CanNm_GetState(NetworkHandleType nmChannelHandle, 
                                      Nm_StateType* nmStatePtr, 
                                      Nm_ModeType* nmModePtr);

#if (CANNM_VERSION_INFO_API == STD_ON)
/**
 * @brief Get version information of CAN NM module
 * @param[out] versioninfo Pointer to version info structure
 */
extern void CanNm_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Set user data for NM PDU
 * @param[in] nmChannelHandle Network channel handle
 * @param[in] nmUserDataPtr Pointer to user data
 * @return E_OK: User data set successfully
 *         E_NOT_OK: Setting user data failed
 */
extern Std_ReturnType CanNm_SetUserData(NetworkHandleType nmChannelHandle, 
                                         const uint8* nmUserDataPtr);

/**
 * @brief Get user data from NM PDU
 * @param[in] nmChannelHandle Network channel handle
 * @param[out] nmUserDataPtr Pointer to store user data
 * @return E_OK: User data retrieved successfully
 *         E_NOT_OK: User data retrieval failed
 */
extern Std_ReturnType CanNm_GetUserData(NetworkHandleType nmChannelHandle, 
                                         uint8* nmUserDataPtr);

/**
 * @brief Set sleep ready bit in CBV
 * @param[in] nmChannelHandle Network channel handle
 * @param[in] nmSleepReadyBit Sleep ready bit value
 * @return E_OK: Sleep ready bit set successfully
 *         E_NOT_OK: Setting sleep ready bit failed
 */
extern Std_ReturnType CanNm_SetSleepReadyBit(NetworkHandleType nmChannelHandle, 
                                              boolean nmSleepReadyBit);

/**
 * @brief Disable NM PDU transmission
 * @param[in] nmChannelHandle Network channel handle
 * @return E_OK: Communication disabled successfully
 *         E_NOT_OK: Disabling communication failed
 */
extern Std_ReturnType CanNm_DisableCommunication(NetworkHandleType nmChannelHandle);

/**
 * @brief Enable NM PDU transmission
 * @param[in] nmChannelHandle Network channel handle
 * @return E_OK: Communication enabled successfully
 *         E_NOT_OK: Enabling communication failed
 */
extern Std_ReturnType CanNm_EnableCommunication(NetworkHandleType nmChannelHandle);

#ifdef __cplusplus
}
#endif

#endif /* CANNM_H */
