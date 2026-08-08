/**
 * @file LinNm.h
 * @brief LIN Network Management module API following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-05-05
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: LIN Network Management (LINNM)
 * Layer: ECU Abstraction Layer (ECUAL)
 * Module ID: 0x45
 */

#ifndef LINNM_H
#define LINNM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "LinNm_Cfg.h"
#include "ComStack_Types.h"
#include "Nm.h"
#include "ComM.h"
#include "Det.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define LINNM_VENDOR_ID                     (0x01U) /* YuleTech Vendor ID */
#define LINNM_MODULE_ID                     (0x45U) /* LINNM Module ID */
#define LINNM_AR_RELEASE_MAJOR_VERSION      (0x04U)
#define LINNM_AR_RELEASE_MINOR_VERSION      (0x04U)
#define LINNM_AR_RELEASE_REVISION_VERSION   (0x00U)
#define LINNM_SW_MAJOR_VERSION              (0x01U)
#define LINNM_SW_MINOR_VERSION              (0x00U)
#define LINNM_SW_PATCH_VERSION              (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define LINNM_SID_INIT                          (0x01U)
#define LINNM_SID_DEINIT                        (0x02U)
#define LINNM_SID_PASSIVESTARTUP                (0x03U)
#define LINNM_SID_NETWORKREQUEST                (0x04U)
#define LINNM_SID_NETWORKRELEASE                (0x05U)
#define LINNM_SID_GETSTATE                      (0x06U)
#define LINNM_SID_GETVERSIONINFO                (0x07U)
#define LINNM_SID_REQUESTBUSSYNCHRONIZATION     (0x08U)
#define LINNM_SID_CHECKREMOTESLEEPINDICATION    (0x09U)
#define LINNM_SID_MAINFUNCTION                  (0x0AU)
#define LINNM_SID_COMCONTROL                    (0x0BU)
#define LINNM_SID_TRANSMIT                      (0x0CU)
#define LINNM_SID_GETUSERDATA                   (0x0DU)
#define LINNM_SID_SETUSERDATA                   (0x0EU)
#define LINNM_SID_GETPDUDATA                    (0x0FU)
#define LINNM_SID_REPEATMESSAGEREQUEST          (0x10U)
#define LINNM_SID_GETNODEDATA                   (0x11U)
#define LINNM_SID_GETLOCALNODEDATA              (0x12U)
#define LINNM_SID_GETREMOTENODEDATA             (0x13U)
#define LINNM_SID_CBKTRCVPDU                    (0x14U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define LINNM_E_UNINIT                          (0x01U)
#define LINNM_E_ALREADY_INITIALIZED             (0x02U)
#define LINNM_E_INVALID_CHANNEL                 (0x03U)
#define LINNM_E_INVALID_POINTER                 (0x04U)
#define LINNM_E_NOT_IN_BUS_SLEEP                (0x05U)
#define LINNM_E_ALREADY_IN_NETWORK_MODE         (0x06U)
#define LINNM_E_INVALID_PARAMETER               (0x07U)
#define LINNM_E_INVALID_CONFIG                  (0x08U)
#define LINNM_E_COM_CONTROL_ERROR               (0x09U)
#define LINNM_E_PDUID_INVALID                   (0x0AU)
#define LINNM_E_COF_UNSUPPORTED                 (0x0BU)
#define LINNM_E_INVALID_NODE_TYPE               (0x0CU)
#define LINNM_E_INVALID_STATE                   (0x0DU)
#define LINNM_E_MAINFUNCTION_CALL               (0x0EU)

/*==================================================================================================
*                                    TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief LIN NM Node State Type
 * Represents the current state of the LIN NM state machine
 */
typedef enum {
    LINNM_STATE_BUS_SLEEP = 0,          /**< Bus sleep mode */
    LINNM_STATE_PREPARE_BUS_SLEEP,      /**< Prepare bus sleep mode */
    LINNM_STATE_READY_SLEEP,            /**< Ready sleep mode */
    LINNM_STATE_NORMAL_OPERATION,       /**< Normal operation mode */
    LINNM_STATE_REPEAT_MESSAGE,         /**< Repeat message state */
    LINNM_STATE_NETWORK_MODE            /**< Network mode (active) */
} LinNm_StateType;

/**
 * @brief LIN NM Mode Type (for ComM interface)
 */
typedef enum {
    LINNM_BUSNM_MODE_BUS_SLEEP = 0,     /**< Bus sleep mode */
    LINNM_BUSNM_MODE_PREPARE_BUS_SLEEP, /**< Prepare bus sleep mode */
    LINNM_BUSNM_MODE_SYNCHRONIZE,       /**< Synchronize mode */
    LINNM_BUSNM_MODE_NETWORK_MODE,      /**< Network mode */
    LINNM_BUSNM_MODE_BUS_SLEEP_MODE     /**< Bus sleep mode (alias) */
} LinNm_ModeType;

/**
 * @brief LIN NM Request Type
 */
typedef enum {
    LINNM_NETWORK_REQUEST_NONE = 0,     /**< No request pending */
    LINNM_NETWORK_REQUEST,              /**< Network mode request */
    LINNM_NETWORK_RELEASE               /**< Network release request */
} LinNm_NetworkRequestType;

/**
 * @brief LIN NM Node Type (Master/Slave)
 */
typedef enum {
    LINNM_NODE_TYPE_MASTER = 0,         /**< Master node */
    LINNM_NODE_TYPE_SLAVE               /**< Slave node */
} LinNm_NodeTypeType;

/**
 * @brief LIN NM Com Control Type
 */
/* Values from LinNm_Cfg.h */
typedef uint8 LinNm_ComControlType;

/**
 * @brief LIN NM Channel Configuration Type
 */
typedef struct {
    NetworkHandleType       NetworkHandle;              /**< Network handle for ComM */
    NetworkHandleType       LinIfChannelHandle;         /**< LinIf channel reference */
    uint8                   NodeId;                     /**< Node ID (for diagnostic) */
    LinNm_NodeTypeType      NodeType;                   /**< Master or Slave node */
    boolean                 PassiveModeEnabled;         /**< Passive mode enabled flag */
    boolean                 StateReportEnabled;         /**< State reporting enabled */
    uint16                  TimeoutTimeMs;              /**< NM timeout time (ms) */
    uint16                  WaitBusSleepTimeMs;         /**< Wait bus sleep time (ms) */
    uint16                  RemoteSleepIndTimeMs;       /**< Remote sleep indication time (ms) */
    uint8                   MsgCycleTimeMs;             /**< Message cycle time (ms) */
    uint8                   MsgReducedTimeMs;           /**< Reduced message cycle time (ms) */
    uint8                   MsgCycleOffsetMs;           /**< Message cycle offset (ms) */
    uint8                   UserDataLength;             /**< User data length */
    boolean                 BusSynchronizationEnabled;  /**< Bus synchronization enabled */
    boolean                 RemoteSleepIndEnabled;      /**< Remote sleep indication enabled */
    boolean                 ComControlEnabled;          /**< Communication control enabled */
    boolean                 CoordinatorSyncSupport;     /**< Coordinator synchronization support */
} LinNm_ChannelConfigType;

/**
 * @brief LIN NM General Configuration Type
 */
typedef struct {
    boolean                 BusSynchronizationEnabled;  /**< Global bus synchronization */
    boolean                 ComControlEnabled;          /**< Global communication control */
    boolean                 CoordinatorSyncEnabled;     /**< Coordinator sync enabled */
    boolean                 PassiveModeEnabled;         /**< Global passive mode */
    boolean                 RemoteSleepIndEnabled;      /**< Global remote sleep indication */
    boolean                 StateChangeIndEnabled;      /**< State change indication */
    boolean                 UserDataEnabled;            /**< User data support */
    uint8                   NodeIdEnabled;              /**< Node ID support */
    uint8                   NumOfChannels;              /**< Number of channels */
} LinNm_GeneralConfigType;

/**
 * @brief LIN NM Channel Runtime Type
 */
typedef struct {
    LinNm_StateType         State;                      /**< Current NM state */
    LinNm_ModeType          Mode;                       /**< Current NM mode */
    boolean                 CommunicationEnabled;       /**< Communication enabled flag */
    boolean                 RemoteSleepIndication;      /**< Remote sleep indication flag */
    boolean                 RemoteSleepIndStatus;       /**< Remote sleep ind status */
    boolean                 BusSynchronizationActive;   /**< Bus synchronization active */
    uint32                  TimeoutTimer;               /**< Timeout timer (ticks) */
    uint32                  RemoteSleepTimer;           /**< Remote sleep timer (ticks) */
    boolean                 NetworkRequested;           /**< Network requested flag */
    boolean                 StateChanged;               /**< State changed flag */
    uint8                   UserData[8];                /**< User data buffer */
    uint8                   UserDataLength;             /**< Actual user data length */
    boolean                 BusLoadReductionActive;     /**< Bus load reduction active */
    uint16                  MessageCycleTimer;          /**< Message cycle timer */
    uint8                   RepeatMessageCounter;       /**< Repeat message counter */
} LinNm_ChannelRuntimeType;

/**
 * @brief LIN NM Configuration Type
 */
typedef struct {
    const LinNm_GeneralConfigType*      GeneralConfig;  /**< General configuration */
    const LinNm_ChannelConfigType*      ChannelConfig;  /**< Channel configuration array */
    LinNm_ChannelRuntimeType*           ChannelRuntime; /**< Channel runtime array */
} LinNm_ConfigType;

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/

#if (LINNM_VERSION_INFO_API == STD_ON)
/**
 * @brief Returns the version information of the LinNm module.
 * @param versioninfo Pointer to version information structure
 */
void LinNm_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Initializes the LIN NM module.
 * @param config Pointer to configuration structure
 */
void LinNm_Init(const LinNm_ConfigType* config);

/**
 * @brief De-initializes the LIN NM module.
 */
void LinNm_DeInit(void);

/**
 * @brief Passive startup of the network management.
 * @param nmChannelHandle Identification of the NM-channel
 * @return E_OK: No error
 *         E_NOT_OK: Passive startup of network management has failed
 */
Std_ReturnType LinNm_PassiveStartUp(NetworkHandleType nmChannelHandle);

/**
 * @brief Request the network to enter network mode.
 * @param nmChannelHandle Identification of the NM-channel
 * @return E_OK: No error
 *         E_NOT_OK: Requesting network mode has failed
 */
Std_ReturnType LinNm_NetworkRequest(NetworkHandleType nmChannelHandle);

/**
 * @brief Request the network to be released to enter bus sleep mode.
 * @param nmChannelHandle Identification of the NM-channel
 * @return E_OK: No error
 *         E_NOT_OK: Releasing network mode has failed
 */
Std_ReturnType LinNm_NetworkRelease(NetworkHandleType nmChannelHandle);

/**
 * @brief Returns the state and the mode of the network management.
 * @param nmChannelHandle Identification of the NM-channel
 * @param nmStatePtr Pointer where state of NM shall be copied to
 * @param nmModePtr Pointer where mode of NM shall be copied to
 * @return E_OK: No error
 *         E_NOT_OK: Getting state and mode has failed
 */
Std_ReturnType LinNm_GetState(NetworkHandleType nmChannelHandle, 
                              Nm_StateType* nmStatePtr, 
                              Nm_ModeType* nmModePtr);

/**
 * @brief Request bus synchronization.
 * @param nmChannelHandle Identification of the NM-channel
 * @return E_OK: No error
 *         E_NOT_OK: Requesting bus synchronization has failed
 */
Std_ReturnType LinNm_RequestBusSynchronization(NetworkHandleType nmChannelHandle);

/**
 * @brief Checks if remote sleep indication has taken place or not.
 * @param nmChannelHandle Identification of the NM-channel
 * @param nmRemoteSleepIndPtr Pointer where remote sleep check result shall be copied to
 * @return E_OK: No error
 *         E_NOT_OK: Checking remote sleep indication has failed
 */
Std_ReturnType LinNm_CheckRemoteSleepIndication(NetworkHandleType nmChannelHandle, 
                                                 boolean* nmRemoteSleepIndPtr);

/**
 * @brief Main function for the LIN NM module.
 * This function performs the state machine processing and timeout handling.
 */
void LinNm_MainFunction(void);

/* Extended API Functions */

#if (LINNM_COM_CONTROL_ENABLED == STD_ON)
/**
 * @brief Sets the communication control state.
 * @param nmChannelHandle Identification of the NM-channel
 * @param nmComMode Requested communication mode
 * @return E_OK: No error
 *         E_NOT_OK: Setting communication mode has failed
 */
Std_ReturnType LinNm_RequestComMode(NetworkHandleType nmChannelHandle, 
                                     ComM_ModeType nmComMode);

/**
 * @brief Gets the current communication mode.
 * @param nmChannelHandle Identification of the NM-channel
 * @param nmComModePtr Pointer where current communication mode shall be stored
 * @return E_OK: No error
 *         E_NOT_OK: Getting communication mode has failed
 */
Std_ReturnType LinNm_GetCurrentComMode(NetworkHandleType nmChannelHandle, 
                                        ComM_ModeType* nmComModePtr);
#endif

/**
 * @brief Sets the user data for the NM PDU.
 * @param nmChannelHandle Identification of the NM-channel
 * @param nmUserDataPtr Pointer to user data (max 8 bytes)
 * @return E_OK: No error
 *         E_NOT_OK: Setting user data has failed
 */
Std_ReturnType LinNm_SetUserData(NetworkHandleType nmChannelHandle, 
                                  const uint8* nmUserDataPtr);

/**
 * @brief Gets the user data from the NM PDU.
 * @param nmChannelHandle Identification of the NM-channel
 * @param nmUserDataPtr Pointer where user data shall be copied to
 * @return E_OK: No error
 *         E_NOT_OK: Getting user data has failed
 */
Std_ReturnType LinNm_GetUserData(NetworkHandleType nmChannelHandle, 
                                  uint8* nmUserDataPtr);

/* LinIf Call-back Functions */

/**
 * @brief Function called by LinIf to indicate a transmission confirmation.
 * @param Channel LinIf channel handle
 * @param LinTxPduId PDU ID of the transmitted PDU
 */
void LinIf_TxConfirmation(uint8 Channel, uint8 LinTxPduId);

/**
 * @brief Function called by LinIf to indicate a schedule switch.
 * @param Channel LinIf channel handle
 * @param ScheduleIndex Index of the new active schedule
 */
void LinIf_ScheduleRequestConfirmation(uint8 Channel, uint8 ScheduleIndex);

/* ComM Call-back Functions */

/**
 * @brief Function called by ComM to indicate a mode change.
 * @param Channel Network channel
 * @param ComMode Communication mode
 */
void LinNm_ComM_BusSleepMode(NetworkHandleType Channel);
void LinNm_ComM_PrepareBusSleepMode(NetworkHandleType Channel);
void LinNm_ComM_NetworkMode(NetworkHandleType Channel);

/* Nm Call-back Functions (for communication with Nm module if used) */

/**
 * @brief Callback function to indicate state changes.
 * @param nmNetworkHandle Network handle
 * @param nmState New NM state
 */
void Nm_StateChangeNotification(NetworkHandleType nmNetworkHandle,
                                 Nm_StateType nmPreviousState,
                                 Nm_StateType nmCurrentState);

/**
 * @brief Callback function for remote sleep indication.
 * @param nmNetworkHandle Network handle
 */
void Nm_RemoteSleepIndication(NetworkHandleType nmNetworkHandle);

/**
 * @brief Callback function for remote sleep cancellation.
 * @param nmNetworkHandle Network handle
 */
void Nm_RemoteSleepCancellation(NetworkHandleType nmNetworkHandle);

/**
 * @brief Callback function for synchronization point.
 * @param nmNetworkHandle Network handle
 */
void Nm_SynchronizationPoint(NetworkHandleType nmNetworkHandle);

#endif /* LINNM_H */
