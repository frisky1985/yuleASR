/**
 * @file CanSm.h
 * @brief CAN State Management module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: CAN State Management (CanSM)
 * Layer: Service Layer
 */

#ifndef CANSM_H
#define CANSM_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "CanSm_Cfg.h"
#include "ComM.h"
#include "ComStack_Types.h"
#include "CanIf.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define CANSM_VENDOR_ID                         (0x01U) /* YuleTech Vendor ID */
#define CANSM_MODULE_ID                         (0x08U) /* CANSM Module ID */
#define CANSM_INSTANCE_ID                       (0x00U)
#define CANSM_AR_RELEASE_MAJOR_VERSION          (0x04U)
#define CANSM_AR_RELEASE_MINOR_VERSION          (0x04U)
#define CANSM_AR_RELEASE_REVISION_VERSION       (0x00U)
#define CANSM_SW_MAJOR_VERSION                  (0x01U)
#define CANSM_SW_MINOR_VERSION                  (0x00U)
#define CANSM_SW_PATCH_VERSION                  (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define CANSM_SID_INIT                          (0x00U)
#define CANSM_SID_DEINIT                        (0x01U)
#define CANSM_SID_REQUESTCOMMODE                (0x02U)
#define CANSM_SID_GETCURRENTCOMMODE             (0x03U)
#define CANSM_SID_CONTROLLERBUSOFF              (0x04U)
#define CANSM_SID_MAINFUNCTION                  (0x05U)
#define CANSM_SID_CONTROLLERMODEINDICATION      (0x07U)
#define CANSM_SID_GETVERSIONINFO                (0x09U)
#define CANSM_SID_CONFIRMPNAVAILABILITY         (0x0AU)
#define CANSM_SID_CLEARTRCVWUFFLAGINDICATION    (0x0BU)
#define CANSM_SID_CONTROLLERERRORSSTATUSINDICATION (0x3CU)
#define CANSM_SID_SETECUPASSIVE                 (0x10U)
#define CANSM_SID_TXTIMEOUTEXCEPTION            (0x11U)
#define CANSM_SID_GETCURRENTINTERNALSTATE       (0x12U)
#define CANSM_SID_GETCBKSTATUS                  (0x13U)
#define CANSM_SID_SETBAUDRATE                   (0x14U)
#define CANSM_SID_GETBAUDRATE                   (0x15U)
#define CANSM_SID_STARTWAKEUPSOURCE             (0x16U)
#define CANSM_SID_STOPWAKEUPSOURCE              (0x17U)
#define CANSM_SID_SETNETWORKPASSIVE             (0x18U)
#define CANSM_SID_TRANSCEIVERMODEINDICATION     (0x19U)
#define CANSM_SID_CHECKTRCVWAKEFLAGINDICATION   (0x1AU)
#define CANSM_SID_SETPNREQUEST                  (0x1BU)
#define CANSM_SID_GETPNSTATE                    (0x1CU)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define CANSM_E_PARAM_POINTER                   (0x01U)
#define CANSM_E_PARAM_CONTROLLER                (0x02U)
#define CANSM_E_PARAM_INVALID_NETWORK_MODE      (0x03U)
#define CANSM_E_INVALID_COMM_REQUEST            (0x04U)
#define CANSM_E_MODE_REQUEST_TIMEOUT            (0x05U)
#define CANSM_E_UNEXPECTED_EXECUTION            (0x06U)
#define CANSM_E_NOT_INITIALIZED                 (0x07U)
#define CANSM_E_INVALID_BAUDRATE                (0x08U)
#define CANSM_E_BUSOFF_RECOVERY_ACTIVE          (0x09U)
#define CANSM_E_PARAM_NETWORK                   (0x0AU)

/*==================================================================================================
*                                    CANSM STATES (BSM - Bus State Machine)
==================================================================================================*/
/**
 * @brief CANSM Controller States
 * These represent the internal state machine states for each CAN controller
 */
typedef enum {
    /* Not Initialized State */
    CANSM_BSM_S_NOTINITIALIZED = 0,
    
    /* No Communication State */
    CANSM_BSM_S_NOCOM,
    
    /* Silent Communication State (Listen Only) */
    CANSM_BSM_S_SILENTCOM,
    
    /* Full Communication State */
    CANSM_BSM_S_FULLCOM,
    
    /* Silent Communication with BusOff Recovery */
    CANSM_BSM_S_SILENTCOM_BOR,
    
    /* Wait State for Mode Transitions */
    CANSM_BSM_S_WAIT_MODE_CHANGE,
    
    /* Check Wakeup State */
    CANSM_BSM_S_CHECKWAKEUP,
    
    /* Change Baudrate State */
    CANSM_BSM_S_CHANGEBAUDRATE
} CanSm_BsmStateType;

/**
 * @brief Partial Network Sleep Availability (PNSA) states of a network
 * @details CANSM_PNSA_NO_PN is the default after init and while the network
 *          has no communication. A pending PN request (CanSM_SetPnRequest) is
 *          promoted to CANSM_PNSA_PN_REQUESTED by CanSM_MainFunction once the
 *          network is in full communication. CanSM_ConfirmPnAvailability
 *          moves the state to CANSM_PNSA_PN_AVAILABLE.
 */
typedef enum {
    CANSM_PNSA_NO_PN = 0,           /**< No PN sleep availability (default) */
    CANSM_PNSA_PN_REQUESTED,        /**< PN requested, waiting for the transceiver confirmation */
    CANSM_PNSA_PN_AVAILABLE         /**< PN confirmed available by CanIf / the transceiver */
} CanSm_PnStateType;

/**
 * @brief CANSM Network Sub-states for BSM_S_NOCOM
 */
typedef enum {
    CANSM_S_NOCOM_NOP = 0,
    CANSM_S_RESTART_CC,
    CANSM_S_RESTART_CC_WAIT,
    CANSM_S_CC_STOPPED,
    CANSM_S_CC_STOPPED_WAIT,
    CANSM_S_CC_SLEEP,
    CANSM_S_CC_SLEEP_WAIT,
    CANSM_S_CC_OFFLINE
} CanSm_NoComSubStateType;

/**
 * @brief CANSM Network Sub-states for BSM_S_SILENTCOM
 */
typedef enum {
    CANSM_S_SILENTCOM_NOP = 0,
    CANSM_S_CC_ONLINE
} CanSm_SilentComSubStateType;

/**
 * @brief CANSM Network Sub-states for BSM_S_FULLCOM
 */
typedef enum {
    CANSM_S_FULLCOM_NOP = 0,
    CANSM_S_FC_CC_START,
    CANSM_S_FC_CC_START_WAIT,
    CANSM_S_FC_CC_ONLINE
} CanSm_FullComSubStateType;

/**
 * @brief CANSM Network Sub-states for BSM_S_SILENTCOM_BOR (BusOff Recovery)
 */
typedef enum {
    CANSM_S_BUSOFF_CHECK = 0,
    CANSM_S_BUSOFF_RECOVERY_L1,
    CANSM_S_BUSOFF_RECOVERY_L2,
    CANSM_S_BOR_RESTART_CC,
    CANSM_S_BOR_RESTART_CC_WAIT,
    CANSM_S_BOR_CC_STOPPED,
    CANSM_S_BOR_CC_STOPPED_WAIT
} CanSm_SilentComBorSubStateType;

/**
 * @brief CANSM Baudrate Configuration Type
 */
typedef struct {
    uint16 BaudRate;                /**< Baudrate in kbps */
    uint32 BaudRateConfig;          /**< Hardware-specific configuration */
} CanSm_BaudrateConfigType;

/**
 * @brief CANSM Network Configuration Type
 */
typedef struct {
    uint8 NetworkHandle;            /**< ComM Channel Handle */
    uint8 ControllerId;             /**< CAN Controller ID */
    uint8 NumBaudrates;             /**< Number of supported baudrates */
    const CanSm_BaudrateConfigType* BaudrateConfigs; /**< Baudrate configurations */
    uint16 MainFunctionPeriodMs;    /**< Main function period in milliseconds */
    uint16 BusOffRecoveryTimeMs;    /**< BusOff recovery timeout in milliseconds */
    uint8  BusOffThreshold;         /**< BusOff counter threshold before recovery */
    boolean WakeupSupport;          /**< Wakeup support enabled */
    boolean BusOffRecoveryEnabled;  /**< Automatic BusOff recovery enabled */
    boolean TransceiverSupport;     /**< Transceiver management support */
    uint8  TransceiverId;           /**< Transceiver ID (if supported) */
} CanSm_NetworkConfigType;

/**
 * @brief CANSM Configuration Type
 */
typedef struct {
    const CanSm_NetworkConfigType* Networks;    /**< Network configurations */
    uint8 NumNetworks;                          /**< Number of networks */
    boolean DevErrorDetect;                     /**< Development error detection */
    boolean VersionInfoApi;                     /**< Version info API enabled */
    boolean SetBaudrateApi;                     /**< Set baudrate API enabled */
} CanSm_ConfigType;

/*==================================================================================================
*                                    EXTERNAL DATA
==================================================================================================*/
#define CANSM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const CanSm_ConfigType CanSm_Config;

#define CANSM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    GLOBAL DATA
==================================================================================================*/
typedef uint8 CanSm_NetworkHandleType;

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define CANSM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the CAN State Management module
 * @param ConfigPtr Pointer to configuration structure (NULL_PTR selects the
 *        pre-compile default configuration object)
 * @details This function initializes all CAN networks to CANSM_BSM_S_NOCOM state
 */
void CanSM_Init(const CanSm_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the CAN State Management module
 * @details This function transitions all networks to CANSM_BSM_S_NOTINITIALIZED
 *          and clears all timers and counters
 */
void CanSM_DeInit(void);

/**
 * @brief Requests a communication mode change for a network
 * @param Network Network handle
 * @param ComM_Mode Requested communication mode
 * @return E_OK if request was accepted, E_NOT_OK otherwise
 * @details This is the main API used by ComM to request communication mode changes
 */
/**
 * @brief Request operation
 * @param[in] Network Network value
 * @param[in] ComM_Mode Operation mode
 * @return Operation status
 */
Std_ReturnType CanSM_RequestComMode(ComM_UserHandleType Network, ComM_ModeType ComM_Mode);

/**
 * @brief Gets the current communication mode of a network
 * @param Network Network handle
 * @param ComM_ModePtr Pointer to store the current communication mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanSM_GetCurrentComMode(ComM_UserHandleType Network, ComM_ModeType* ComM_ModePtr);

/**
 * @brief Main function for the CAN State Management module
 * @details This function must be called cyclically to process state machine transitions
 */
void CanSM_MainFunction(void);

/**
 * @brief BusOff indication callback from CanIf
 * @param ControllerId Controller that experienced BusOff
 * @details Called by CanIf when a BusOff event is detected
 */
void CanSM_ControllerBusOff(uint8 ControllerId);

/**
 * @brief Controller mode indication callback from CanIf
 * @param ControllerId Controller that changed mode
 * @param ControllerMode New controller mode
 * @details Called by CanIf to confirm controller mode changes
 */
void CanSM_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);

/**
 * @brief Confirms partial networking availability for a network
 * @param NetworkHandle Network handle
 * @return E_OK if the confirmation was accepted, E_NOT_OK otherwise
 * @details Only supported while the network is in full communication
 */
Std_ReturnType CanSM_ConfirmPnAvailability(NetworkHandleType NetworkHandle);

/**
 * @brief Sets the PN (partial networking) wake-up request of a network
 * @param NetworkHandle Network handle
 * @param PnRequest TRUE: PN selective wake-up requested, FALSE: cancel request
 * @return E_OK if the request was accepted, E_NOT_OK otherwise
 * @details The request is latched and consumed by CanSM_MainFunction: in full
 *          communication the PN state advances from CANSM_PNSA_NO_PN to
 *          CANSM_PNSA_PN_REQUESTED. Requires transceiver support in the
 *          network configuration.
 */
/**
 * @brief Set configuration value
 * @param[in] NetworkHandle NetworkHandle value
 * @param[in] PnRequest PnRequest value
 * @return Operation status
 */
Std_ReturnType CanSM_SetPnRequest(NetworkHandleType NetworkHandle, boolean PnRequest);

/**
 * @brief Gets the PN sleep availability state of a network
 * @param NetworkHandle Network handle
 * @param PnStatePtr Pointer to store the PNSA state
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanSM_GetPnState(NetworkHandleType NetworkHandle, CanSm_PnStateType* PnStatePtr);

/**
 * @brief Clears the transceiver wakeup-flag indication of a network
 * @param NetworkHandle Network handle
 * @return E_OK if the indication was cleared, E_NOT_OK otherwise
 * @details Only supported while the network is in full communication
 */
Std_ReturnType CanSM_ClearTrcvWufFlagIndication(NetworkHandleType NetworkHandle);

/**
 * @brief Gets version information
 * @param VersionInfo Pointer to version info structure
 */
void CanSM_GetVersionInfo(Std_VersionInfoType* VersionInfo);

/**
 * @brief Sets the baudrate for a network
 * @param Network Network handle
 * @param BaudRate New baudrate to set
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanSM_SetBaudrate(ComM_UserHandleType Network, uint16 BaudRate);

/**
 * @brief Gets the baudrate of a network
 * @param Network Network handle
 * @param BaudRatePtr Pointer to store the baudrate
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanSM_GetBaudrate(ComM_UserHandleType Network, uint16* BaudRatePtr);

/**
 * @brief Gets the current internal state of a network
 * @param Network Network handle
 * @param StatePtr Pointer to store the internal state
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanSM_GetCurrentInternalState(uint8 Network, CanSm_BsmStateType* StatePtr);

/**
 * @brief Starts the wakeup source of a network (wakeup validation)
 * @param Network Network handle
 * @return E_OK if the request was accepted, E_NOT_OK otherwise
 * @details Latches a wakeup source request which is consumed by the next
 *          CanSM_MainFunction cycle: the network enters the wakeup validation
 *          state (CANSM_BSM_S_CHECKWAKEUP, controller STOPPED + transceiver
 *          NORMAL when transceiver management is configured)
 */
/**
 * @brief Start the operation
 * @param[in] Network Network value
 * @return Operation status
 */
Std_ReturnType CanSM_StartWakeupSource(NetworkHandleType Network);

/**
 * @brief Stops the wakeup source of a network
 * @param Network Network handle
 * @return E_OK if the request was accepted, E_NOT_OK otherwise
 * @details Clears a pending wakeup source request; a network currently in
 *          wakeup validation (CANSM_BSM_S_CHECKWAKEUP) returns to the regular
 *          NOCOM flow
 */
/**
 * @brief Stop the operation
 * @param[in] Network Network value
 * @return Operation status
 */
Std_ReturnType CanSM_StopWakeupSource(NetworkHandleType Network);

/**
 * @brief Sets the ECU-wide passive mode
 * @param passive TRUE: ECU passive, FALSE: ECU active
 * @return E_OK always
 * @details While the ECU is passive, FULL_COMMUNICATION requests of a network
 *          are degraded to SILENT_COMMUNICATION unless the network was
 *          explicitly set non-passive via CanSM_SetNetworkPassive(Network, FALSE)
 */
/**
 * @brief Set configuration value
 * @param[in] passive passive value
 * @return Operation status
 */
Std_ReturnType CanSM_SetEcuPassive(boolean passive);

/**
 * @brief Sets the passive mode of a single network (overrides the ECU-wide mode)
 * @param NetworkHandle Network handle
 * @param passive TRUE: force passive (degrade FULL requests), FALSE: explicitly
 *        non-passive (takes precedence over an ECU-wide passive mode)
 * @return E_OK if the request was accepted, E_NOT_OK otherwise
 */
/**
 * @brief Set configuration value
 * @param[in] NetworkHandle NetworkHandle value
 * @param[in] passive passive value
 * @return Operation status
 */
Std_ReturnType CanSM_SetNetworkPassive(NetworkHandleType NetworkHandle, boolean passive);

/**
 * @brief TX timeout exception notification (CanIf timeout exception)
 * @param Network Network handle
 * @details Triggers the bus-off recovery entry semantics for the network:
 *          the network enters CANSM_BSM_S_SILENTCOM_BOR (initial sub-state),
 *          sharing the entry logic with CanSM_ControllerBusOff
 */
/**
 * @brief Transmit data
 * @param[in] Network Network value
 */
void CanSM_TxTimeoutException(NetworkHandleType Network);

/**
 * @brief Transceiver mode indication callback from CanIf
 * @param TransceiverId Transceiver that changed mode
 * @param TransceiverMode New transceiver mode
 * @details Updates the transceiver mode recorded in the CanSM runtime,
 *          matched against the TransceiverId of the network configuration
 */
/**
 * @brief transceiver mode indication
 * @param[in] TransceiverId Identifier
 * @param[in] TransceiverMode Operation mode
 */
void CanSM_TransceiverModeIndication(uint8 TransceiverId, CanIf_TransceiverModeType TransceiverMode);

/**
 * @brief Check transceiver wakeup flag indication callback from CanIf
 * @param TransceiverId Transceiver whose wakeup flag was checked
 * @details Clears the pending transceiver wakeup-flag check of the matching
 *          network ( wakeup validation completion marker )
 */
void CanSM_CheckTransceiverWakeFlagIndication(uint8 TransceiverId);

#define CANSM_STOP_SEC_CODE
#include "MemMap.h"

void CanSm_GetVersionInfo(Std_VersionInfoType* versioninfo);

#endif /* CANSM_H */
