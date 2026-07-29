/**
 * @file CanSm.c
 * @brief CAN State Manager
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

/**
 * @file CanSm.c
 * @brief CAN State Management module implementation following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: CAN State Management (CanSM)
 * Layer: Service Layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "CanSm.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL DEFINES
==================================================================================================*/
/**
 * @brief Local defines for state machine processing
 */
#define CANSM_UNINIT                            (0U)
#define CANSM_INIT                              (1U)

/**
 * @brief Mode transition timeouts (in main function ticks)
 */
#define CANSM_NO_TRANSITION_TIMEOUT             (0xFFFFU)

/**
 * @brief Invalid network handle
 */
#define CANSM_INVALID_NETWORK_HANDLE            (0xFFU)

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
/**
 * @brief Internal state tracking for each network
 */
typedef struct {
    CanSm_BsmStateType BsmState;           /**< Current BSM state */
    uint8 SubState;                         /**< Current sub-state */
    ComM_ModeType RequestedComMMode;        /**< Requested ComM mode */
    ComM_ModeType CurrentComMMode;          /**< Current ComM mode */
    uint16 ModeRequestTimer;                /**< Mode request timeout timer */
    uint16 BusOffRecoveryTimer;             /**< BusOff recovery timer */
    uint8 BusOffCounter;                    /**< BusOff event counter */
    boolean BusOffEventPending;             /**< BusOff event pending flag */
    uint16 CurrentBaudrate;                 /**< Current baudrate */
    uint8 RequestedBaudrateIndex;           /**< Requested baudrate index */
    boolean BaudrateChangePending;          /**< Baudrate change pending */
    CanIf_ControllerModeType RequestedCtrlMode; /**< Requested controller mode */
    boolean ModeChangePending;              /**< Mode change pending flag */
    boolean Initialized;                    /**< Network initialized flag */
} CanSm_NetworkStateType;

/**
 * @brief Module global state
 */
typedef struct {
    uint8 InitStatus;                       /**< Module initialization status */
    CanSm_NetworkStateType Networks[CANSM_MAX_NETWORKS]; /**< Per-network states */
    uint8 NumNetworks;                      /**< Number of configured networks */
    const CanSm_ConfigType* ConfigPtr;      /**< Pointer to configuration */
} CanSm_GlobalStateType;

/*==================================================================================================
*                                    LOCAL CONSTANTS
==================================================================================================*/
/**
 * @brief Baudrate configurations for each network
 */
static const CanSm_BaudrateConfigType CanSm_BaudrateConfigs_Network0[] = {
    { CANSM_BAUDRATE_125K,  0x0001U },
    { CANSM_BAUDRATE_250K,  0x0002U },
    { CANSM_BAUDRATE_500K,  0x0003U },
    { CANSM_BAUDRATE_1000K, 0x0004U }
};

static const CanSm_BaudrateConfigType CanSm_BaudrateConfigs_Network1[] = {
    { CANSM_BAUDRATE_125K,  0x0001U },
    { CANSM_BAUDRATE_250K,  0x0002U },
    { CANSM_BAUDRATE_500K,  0x0003U },
    { CANSM_BAUDRATE_1000K, 0x0004U }
};

/*==================================================================================================
*                                    LOCAL DATA
==================================================================================================*/
/**
 * @brief Module global state
 */
static CanSm_GlobalStateType CanSm_Global;

/**
 * @brief Network configurations
 */
static const CanSm_NetworkConfigType CanSm_NetworkConfigs[CANSM_NUM_NETWORKS] = {
    {   /* Network 0 - CAN0 */
        .NetworkHandle = CANSM_NETWORK_CAN0,
        .ControllerId = CANSM_CONTROLLER_CAN0,
        .NumBaudrates = 4U,
        .BaudrateConfigs = CanSm_BaudrateConfigs_Network0,
        .MainFunctionPeriodMs = CANSM_NETWORK0_MAIN_FUNCTION_PERIOD_MS,
        .BusOffRecoveryTimeMs = CANSM_NETWORK0_BUSOFF_RECOVERY_TIME_MS,
        .BusOffThreshold = CANSM_BUSOFF_THRESHOLD,
        .WakeupSupport = CANSM_NETWORK0_WAKEUP_SUPPORT,
        .BusOffRecoveryEnabled = CANSM_NETWORK0_BUSOFF_RECOVERY_ENABLED,
        .TransceiverSupport = CANSM_TRANSCEIVER_SUPPORT,
        .TransceiverId = CANSM_TRANSCEIVER_CAN0
    },
    {   /* Network 1 - CAN1 */
        .NetworkHandle = CANSM_NETWORK_CAN1,
        .ControllerId = CANSM_CONTROLLER_CAN1,
        .NumBaudrates = 4U,
        .BaudrateConfigs = CanSm_BaudrateConfigs_Network1,
        .MainFunctionPeriodMs = CANSM_NETWORK1_MAIN_FUNCTION_PERIOD_MS,
        .BusOffRecoveryTimeMs = CANSM_NETWORK1_BUSOFF_RECOVERY_TIME_MS,
        .BusOffThreshold = CANSM_BUSOFF_THRESHOLD,
        .WakeupSupport = CANSM_NETWORK1_WAKEUP_SUPPORT,
        .BusOffRecoveryEnabled = CANSM_NETWORK1_BUSOFF_RECOVERY_ENABLED,
        .TransceiverSupport = CANSM_TRANSCEIVER_SUPPORT,
        .TransceiverId = CANSM_TRANSCEIVER_CAN1
    }
};

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static Std_ReturnType CanSm_ProcessNoComState(uint8 NetworkIndex);
static Std_ReturnType CanSm_ProcessSilentComState(uint8 NetworkIndex);
static Std_ReturnType CanSm_ProcessFullComState(uint8 NetworkIndex);
static Std_ReturnType CanSm_ProcessSilentComBorState(uint8 NetworkIndex);
static Std_ReturnType CanSm_RequestControllerMode(uint8 NetworkIndex, CanIf_ControllerModeType Mode);
static boolean CanSm_IsNetworkValid(ComM_UserHandleType Network);
static uint8 CanSm_GetNetworkIndex(ComM_UserHandleType Network);
static void CanSm_HandleModeConfirmation(uint8 NetworkIndex, CanIf_ControllerModeType Mode);
static void CanSm_HandleBusOffRecovery(uint8 NetworkIndex);
static Std_ReturnType CanSm_TransitionToNoCom(uint8 NetworkIndex);
static Std_ReturnType CanSm_TransitionToSilentCom(uint8 NetworkIndex);
static Std_ReturnType CanSm_TransitionToFullCom(uint8 NetworkIndex);
static void CanSm_StartTimer(uint8 NetworkIndex, uint16 TimeoutMs);
static boolean CanSm_IsTimerExpired(uint8 NetworkIndex);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Checks if network handle is valid
 */
static boolean CanSm_IsNetworkValid(ComM_UserHandleType Network)
{
    return (Network < CANSM_NUM_NETWORKS) ? TRUE : FALSE;
}

/**
 * @brief Gets network index from network handle
 */
static uint8 CanSm_GetNetworkIndex(ComM_UserHandleType Network)
{
    return CanSm_IsNetworkValid(Network) ? (uint8)Network : CANSM_INVALID_NETWORK_HANDLE;
}

/**
 * @brief Starts a timer for mode transition timeout
 */
static void CanSm_StartTimer(uint8 NetworkIndex, uint16 TimeoutMs)
{
    uint16 ticks;
    const CanSm_NetworkConfigType* netConfig;
    
    netConfig = &CanSm_NetworkConfigs[NetworkIndex];
    
    /* Calculate ticks based on main function period */
    ticks = (TimeoutMs + netConfig->MainFunctionPeriodMs - 1U) / netConfig->MainFunctionPeriodMs;
    
    CanSm_Global.Networks[NetworkIndex].ModeRequestTimer = ticks;
}

/**
 * @brief Checks if timer has expired
 */
static boolean CanSm_IsTimerExpired(uint8 NetworkIndex)
{
    boolean expired = FALSE;
    
    if (CanSm_Global.Networks[NetworkIndex].ModeRequestTimer > 0U) {
        CanSm_Global.Networks[NetworkIndex].ModeRequestTimer--;
        if (CanSm_Global.Networks[NetworkIndex].ModeRequestTimer == 0U) {
            expired = TRUE;
        }
    }
    
    return expired;
}

/**
 * @brief Requests controller mode from CanIf
 */
static Std_ReturnType CanSm_RequestControllerMode(uint8 NetworkIndex, CanIf_ControllerModeType Mode)
{
    Std_ReturnType result;
    const CanSm_NetworkConfigType* netConfig;
    
    netConfig = &CanSm_NetworkConfigs[NetworkIndex];
    
    result = CanIf_SetControllerMode(netConfig->ControllerId, Mode);
    
    if (result == E_OK) {
        CanSm_Global.Networks[NetworkIndex].RequestedCtrlMode = Mode;
        CanSm_Global.Networks[NetworkIndex].ModeChangePending = TRUE;
        CanSm_StartTimer(NetworkIndex, CANSM_MODE_CHANGE_REQUEST_TIMEOUT_MS);
    }
    
    return result;
}

/**
 * @brief Handles mode confirmation from CanIf
 */
static void CanSm_HandleModeConfirmation(uint8 NetworkIndex, CanIf_ControllerModeType Mode)
{
    CanSm_Global.Networks[NetworkIndex].ModeChangePending = FALSE;
    CanSm_Global.Networks[NetworkIndex].ModeRequestTimer = 0U;
    
    /* Update internal state based on confirmed mode */
    switch (Mode) {
        case CANIF_CS_STARTED:
            /* Controller is now started - can transition to FULLCOM */
            if (CanSm_Global.Networks[NetworkIndex].BsmState == CANSM_BSM_S_NOCOM) {
                CanSm_TransitionToFullCom(NetworkIndex);
            }
            break;
            
        case CANIF_CS_STOPPED:
            /* Controller is stopped - transition to SILENTCOM or NOCOM */
            if (CanSm_Global.Networks[NetworkIndex].BsmState == CANSM_BSM_S_FULLCOM) {
                CanSm_TransitionToSilentCom(NetworkIndex);
            }
            break;
            
        case CANIF_CS_SLEEP:
            /* Controller is in sleep - transition to NOCOM */
            if (CanSm_Global.Networks[NetworkIndex].BsmState != CANSM_BSM_S_NOCOM) {
                CanSm_TransitionToNoCom(NetworkIndex);
            }
            break;
            
        case CANIF_CS_UNINIT:
        default:
            /* Do nothing */
            break;
    }
}

/**
 * @brief Handles BusOff recovery
 */
static void CanSm_HandleBusOffRecovery(uint8 NetworkIndex)
{
    CanSm_NetworkStateType* netState;
    const CanSm_NetworkConfigType* netConfig;
    
    netState = &CanSm_Global.Networks[NetworkIndex];
    netConfig = &CanSm_NetworkConfigs[NetworkIndex];
    
    if (!netConfig->BusOffRecoveryEnabled) {
        return;
    }
    
    /* Increment BusOff counter */
    netState->BusOffCounter++;
    netState->BusOffEventPending = TRUE;
    
    /* Check if threshold exceeded */
    if (netState->BusOffCounter >= netConfig->BusOffThreshold) {
        /* Transition to SILENTCOM_BOR state */
        netState->BsmState = CANSM_BSM_S_SILENTCOM_BOR;
        netState->SubState = CANSM_S_BUSOFF_CHECK;
        netState->BusOffRecoveryTimer = CANSM_BUSOFF_RECOVERY_L1_MS / netConfig->MainFunctionPeriodMs;
        
        /* Stop the controller */
        (void)CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STOPPED);
    } else {
        /* Try immediate restart */
        (void)CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STOPPED);
        (void)CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STARTED);
    }
}

/**
 * @brief Transitions network to NOCOM state
 */
static Std_ReturnType CanSm_TransitionToNoCom(uint8 NetworkIndex)
{
    Std_ReturnType result = E_NOT_OK;
    CanSm_NetworkStateType* netState;
    
    netState = &CanSm_Global.Networks[NetworkIndex];
    
    /* Reset BusOff counter */
    netState->BusOffCounter = 0U;
    netState->BusOffEventPending = FALSE;
    
    /* Set PDU mode to OFFLINE */
    (void)CanIf_SetPduMode(CanSm_NetworkConfigs[NetworkIndex].ControllerId, CANIF_OFFLINE);
    
    /* Request controller sleep */
    result = CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_SLEEP);
    
    if (result == E_OK) {
        netState->BsmState = CANSM_BSM_S_NOCOM;
        netState->SubState = CANSM_NOCOM_S_CC_SLEEP_WAIT;
        netState->CurrentComMMode = COMM_NO_COMMUNICATION;
    }
    
    return result;
}

/**
 * @brief Transitions network to SILENTCOM state
 */
static Std_ReturnType CanSm_TransitionToSilentCom(uint8 NetworkIndex)
{
    Std_ReturnType result = E_NOT_OK;
    CanSm_NetworkStateType* netState;
    
    netState = &CanSm_Global.Networks[NetworkIndex];
    
    /* Set PDU mode to TX_OFFLINE (listen only) */
    (void)CanIf_SetPduMode(CanSm_NetworkConfigs[NetworkIndex].ControllerId, CANIF_TX_OFFLINE);
    
    /* Request controller stop (listen only mode) */
    result = CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STOPPED);
    
    if (result == E_OK) {
        netState->BsmState = CANSM_BSM_S_SILENTCOM;
        netState->SubState = CANSM_S_SILENTCOM_NOP;
        netState->CurrentComMMode = COMM_SILENT_COMMUNICATION;
    }
    
    return result;
}

/**
 * @brief Transitions network to FULLCOM state
 */
static Std_ReturnType CanSm_TransitionToFullCom(uint8 NetworkIndex)
{
    Std_ReturnType result = E_NOT_OK;
    CanSm_NetworkStateType* netState;
    
    netState = &CanSm_Global.Networks[NetworkIndex];
    
    /* Set PDU mode to ONLINE */
    (void)CanIf_SetPduMode(CanSm_NetworkConfigs[NetworkIndex].ControllerId, CANIF_ONLINE);
    
    /* Request controller start */
    result = CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STARTED);
    
    if (result == E_OK) {
        netState->BsmState = CANSM_BSM_S_FULLCOM;
        netState->SubState = CANSM_S_FULLCOM_NOP;
        netState->CurrentComMMode = COMM_FULL_COMMUNICATION;
        
        /* Clear BusOff counter */
        netState->BusOffCounter = 0U;
    }
    
    return result;
}

/**
 * @brief Process NOCOM state
 */
static Std_ReturnType CanSm_ProcessNoComState(uint8 NetworkIndex)
{
    Std_ReturnType result = E_OK;
    CanSm_NetworkStateType* netState;
    ComM_ModeType requestedMode;
    
    netState = &CanSm_Global.Networks[NetworkIndex];
    requestedMode = netState->RequestedComMMode;
    
    switch (netState->SubState) {
        case CANSM_S_NOCOM_NOP:
            /* Check if mode change requested */
            if (requestedMode == COMM_SILENT_COMMUNICATION) {
                result = CanSm_TransitionToSilentCom(NetworkIndex);
            } else if (requestedMode == COMM_FULL_COMMUNICATION) {
                /* Need to go through controller start sequence */
                result = CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STARTED);
                if (result == E_OK) {
                    netState->SubState = CANSM_FULLCOM_S_CC_START_WAIT;
                }
            }
            break;
            
        case CANSM_FULLCOM_S_CC_START_WAIT:
            /* Waiting for controller mode confirmation */
            if (CanSm_IsTimerExpired(NetworkIndex)) {
                /* Timeout - retry or error */
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
                Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, 
                               CANSM_SID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
#endif
                result = E_NOT_OK;
            }
            break;
            
        case CANSM_NOCOM_S_CC_SLEEP_WAIT:
            /* Waiting for sleep mode confirmation */
            if (CanSm_IsTimerExpired(NetworkIndex)) {
                /* Timeout - stay in NOCOM */
                netState->SubState = CANSM_S_NOCOM_NOP;
            }
            break;
            
        default:
            netState->SubState = CANSM_S_NOCOM_NOP;
            break;
    }
    
    return result;
}

/**
 * @brief Process SILENTCOM state
 */
static Std_ReturnType CanSm_ProcessSilentComState(uint8 NetworkIndex)
{
    Std_ReturnType result = E_OK;
    CanSm_NetworkStateType* netState;
    ComM_ModeType requestedMode;
    
    netState = &CanSm_Global.Networks[NetworkIndex];
    requestedMode = netState->RequestedComMMode;
    
    switch (netState->SubState) {
        case CANSM_S_SILENTCOM_NOP:
            /* Check if mode change requested */
            if (requestedMode == COMM_NO_COMMUNICATION) {
                result = CanSm_TransitionToNoCom(NetworkIndex);
            } else if (requestedMode == COMM_FULL_COMMUNICATION) {
                result = CanSm_TransitionToFullCom(NetworkIndex);
            }
            /* Stay in SILENTCOM otherwise (listen mode) */
            break;
            
        case CANSM_SILENTCOM_S_CC_ONLINE:
            /* Handle any ongoing transitions */
            break;
            
        default:
            netState->SubState = CANSM_S_SILENTCOM_NOP;
            break;
    }
    
    return result;
}

/**
 * @brief Process FULLCOM state
 */
static Std_ReturnType CanSm_ProcessFullComState(uint8 NetworkIndex)
{
    Std_ReturnType result = E_OK;
    CanSm_NetworkStateType* netState;
    ComM_ModeType requestedMode;
    
    netState = &CanSm_Global.Networks[NetworkIndex];
    requestedMode = netState->RequestedComMMode;
    
    switch (netState->SubState) {
        case CANSM_S_FULLCOM_NOP:
            /* Check if mode change requested */
            if (requestedMode == COMM_NO_COMMUNICATION) {
                result = CanSm_TransitionToNoCom(NetworkIndex);
            } else if (requestedMode == COMM_SILENT_COMMUNICATION) {
                result = CanSm_TransitionToSilentCom(NetworkIndex);
            }
            /* Stay in FULLCOM otherwise */
            break;
            
        case CANSM_FULLCOM_S_CC_START_WAIT:
            /* Waiting for controller mode confirmation */
            if (CanSm_IsTimerExpired(NetworkIndex)) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
                Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, 
                               CANSM_SID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
#endif
                result = E_NOT_OK;
            }
            break;
            

        case CANSM_FULLCOM_S_CC_START:
            /* CC is online, move to normal operation */
            netState->SubState = CANSM_FULLCOM_S_CC_ONLINE;
            break;

        case CANSM_FULLCOM_S_CC_ONLINE:
            /* Check for mode changes */
            if (requestedMode == COMM_NO_COMMUNICATION) {
                result = CanSm_TransitionToNoCom(NetworkIndex);
            } else if (requestedMode == COMM_SILENT_COMMUNICATION) {
                result = CanSm_TransitionToSilentCom(NetworkIndex);
            }
            break;

        default:
            break;
    }
    return result;
}
