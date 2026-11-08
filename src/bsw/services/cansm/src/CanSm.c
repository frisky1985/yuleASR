/**
 * @file CanSm.c
 * @brief CAN State Manager
 * @req SHALL_CANSM - CAN State Manager
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */
/**
 * @file CanSm.c
 * @brief CAN State Management module implementation following AutoSAR Classic Platform 4.x standard
 * @req SHALL_CANSM - CAN State Management module implementation following AutoSAR Classic Platform 4.x standard
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

/* Version check */
#if defined(CANSM_AR_RELEASE_MAJOR_VERSION) && (CANSM_AR_RELEASE_MAJOR_VERSION != 4u)
#error "CanSm: AR major mismatch"
#endif
#if defined(CANSM_AR_RELEASE_MINOR_VERSION) && (CANSM_AR_RELEASE_MINOR_VERSION != 4u)
#error "CanSm: AR minor mismatch"
#endif

/*==================================================================================================
*                                    LOCAL DEFINES
==================================================================================================*/
/**
 * @brief Local defines for state machine processing
 * @req SHALL_CANSM - Local defines for state machine processing
 */
#define CANSM_UNINIT                            (0U)
#define CANSM_INIT                              (1U)

/**
 * @brief Mode transition timeouts (in main function ticks)
 * @req SHALL_CANSM - Mode transition timeouts (in main function ticks)
 */
#define CANSM_NO_TRANSITION_TIMEOUT             (0xFFFFU)

/**
 * @brief Invalid network handle
 * @req SHALL_CANSM - Invalid network handle
 */
#define CANSM_INVALID_NETWORK_HANDLE            (0xFFU)

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
/**
 * @brief Per-network passive mode override (SetNetworkPassive)
 * @req SHALL_CANSM - Per-network passive mode override
 */
typedef enum {
    CANSM_NETPASSIVE_FOLLOW_ECU = 0,    /**< Follow the global ECU passive mode */
    CANSM_NETPASSIVE_FORCED,            /**< SetNetworkPassive(net, TRUE): force passive */
    CANSM_NETPASSIVE_OVERRIDDEN         /**< SetNetworkPassive(net, FALSE): explicitly non-passive */
} CanSm_NetworkPassiveOverrideType;

/**
 * @brief Internal state tracking for each network
 * @req SHALL_CANSM - Internal state tracking for each network
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
    CanIf_ControllerModeType CtrlMode;      /**< Last confirmed controller mode */
    CanIf_TransceiverModeType TrcvMode;     /**< Last indicated transceiver mode */
    boolean ModeChangePending;              /**< Mode change pending flag */
    boolean TrcvWufFlagIndication;          /**< Transceiver wakeup-flag check pending */
    boolean WakeupSourceRequested;          /**< Wakeup source request latched (consumed by MainFunction) */
    CanSm_NetworkPassiveOverrideType PassiveOverride; /**< Per-network passive override */
    CanSm_PnStateType PnState;              /**< Partial Network Sleep Availability state (PNSA) */
    boolean PnRequestPending;               /**< PN wake-up request latched (consumed by MainFunction) */
    boolean Initialized;                    /**< Network initialized flag */
} CanSm_NetworkStateType;

/**
 * @brief Module global state
 * @req SHALL_CANSM - Module global state
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
 * @req SHALL_CANSM - Baudrate configurations for each network
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
 * @req SHALL_CANSM - Module global state
 */
static CanSm_GlobalStateType CanSm_Global;

/**
 * @brief ECU-wide passive mode (CanSM_SetEcuPassive)
 * @req SHALL_CANSM - ECU-wide passive mode
 */
static boolean CanSm_EcuPassive = FALSE;

/**
 * @brief Network configurations
 * @req SHALL_CANSM - Network configurations
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
static Std_ReturnType CanSm_ProcessCheckWakeupState(uint8 NetworkIndex);
static Std_ReturnType CanSm_RequestControllerMode(uint8 NetworkIndex, CanIf_ControllerModeType Mode);
static boolean CanSm_IsNetworkValid(ComM_UserHandleType Network);
static uint8 CanSm_GetNetworkIndex(ComM_UserHandleType Network);
static void CanSm_HandleModeConfirmation(uint8 NetworkIndex, CanIf_ControllerModeType Mode);
static void CanSm_HandleBusOffRecovery(uint8 NetworkIndex);
static void CanSm_EnterBusOffRecovery(uint8 NetworkIndex);
static Std_ReturnType CanSm_EnterCheckWakeup(uint8 NetworkIndex);
static boolean CanSm_IsNetworkPassive(uint8 NetworkIndex);
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
 * @req SHALL_CANSM - Checks if network handle is valid
 */
static boolean CanSm_IsNetworkValid(ComM_UserHandleType Network)
{
    return (Network < CANSM_NUM_NETWORKS) ? TRUE : FALSE;
}

/**
 * @brief Gets network index from network handle
 * @req SHALL_CANSM - Gets network index from network handle
 */
static uint8 CanSm_GetNetworkIndex(ComM_UserHandleType Network)
{
    return CanSm_IsNetworkValid(Network) ? (uint8)Network : CANSM_INVALID_NETWORK_HANDLE;
}

/**
 * @brief Starts a timer for mode transition timeout
 * @req SHALL_CANSM - Starts a timer for mode transition timeout
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
 * @req SHALL_CANSM - Checks if timer has expired
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
 * @req SHALL_CANSM - Requests controller mode from CanIf
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
 * @req SHALL_CANSM - Handles mode confirmation from CanIf
 */
static void CanSm_HandleModeConfirmation(uint8 NetworkIndex, CanIf_ControllerModeType Mode)
{
    CanSm_Global.Networks[NetworkIndex].ModeChangePending = FALSE;
    CanSm_Global.Networks[NetworkIndex].ModeRequestTimer = 0U;
    CanSm_Global.Networks[NetworkIndex].CtrlMode = Mode;

    /* Update internal state based on confirmed mode */
    switch (Mode) {
        case CANIF_CS_STARTED:
            /* Controller is now started - can transition to FULLCOM */
            if ((CanSm_Global.Networks[NetworkIndex].BsmState == CANSM_BSM_S_NOCOM) ||
                (CanSm_Global.Networks[NetworkIndex].BsmState == CANSM_BSM_S_SILENTCOM) ||
                (CanSm_Global.Networks[NetworkIndex].BsmState == CANSM_BSM_S_SILENTCOM_BOR)) {
                (void)CanSm_TransitionToFullCom(NetworkIndex);
            }
            break;

        case CANIF_CS_STOPPED:
            /* Controller is stopped - transition to SILENTCOM or NOCOM */
            if (CanSm_Global.Networks[NetworkIndex].BsmState == CANSM_BSM_S_FULLCOM) {
                (void)CanSm_TransitionToSilentCom(NetworkIndex);
            }
            break;
            
        case CANIF_CS_SLEEP:
            /* Controller is in sleep - transition to NOCOM */
            if (CanSm_Global.Networks[NetworkIndex].BsmState != CANSM_BSM_S_NOCOM) {
                (void)CanSm_TransitionToNoCom(NetworkIndex);
            }
            break;
            
        case CANIF_CS_UNINIT:
        default:
            /* Do nothing */
            break;
    }
}

/**
 * @brief Enters the bus-off recovery state (shared entry logic)
 * @req SHALL_CANSM - Enters the bus-off recovery state
 * @details Shared by CanSM_ControllerBusOff (threshold exceeded) and
 *          CanSM_TxTimeoutException: enters CANSM_BSM_S_SILENTCOM_BOR at its
 *          initial sub-state CANSM_S_BUSOFF_CHECK and stops the controller
 */
static void CanSm_EnterBusOffRecovery(uint8 NetworkIndex)
{
    CanSm_NetworkStateType* netState;
    const CanSm_NetworkConfigType* netConfig;

    netState = &CanSm_Global.Networks[NetworkIndex];
    netConfig = &CanSm_NetworkConfigs[NetworkIndex];

    /* Transition to SILENTCOM_BOR state (initial sub-state) */
    netState->BsmState = CANSM_BSM_S_SILENTCOM_BOR;
    netState->SubState = CANSM_S_BUSOFF_CHECK;
    netState->BusOffRecoveryTimer = CANSM_BUSOFF_RECOVERY_L1_MS / netConfig->MainFunctionPeriodMs;

    /* Stop the controller */
    (void)CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STOPPED);
}

/**
 * @brief Handles BusOff recovery
 * @req SHALL_CANSM - Handles BusOff recovery
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
        CanSm_EnterBusOffRecovery(NetworkIndex);
    } else {
        /* Try immediate restart */
        (void)CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STOPPED);
        (void)CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STARTED);
    }
}

/**
 * @brief Checks the effective passive mode of a network
 * @req SHALL_CANSM - Checks the effective passive mode of a network
 * @details The per-network override (CanSM_SetNetworkPassive) takes precedence
 *          over the ECU-wide passive mode (CanSM_SetEcuPassive)
 */
static boolean CanSm_IsNetworkPassive(uint8 NetworkIndex)
{
    boolean passive;
    const CanSm_NetworkStateType* netState;

    netState = &CanSm_Global.Networks[NetworkIndex];

    switch (netState->PassiveOverride) {
        case CANSM_NETPASSIVE_FORCED:
            passive = TRUE;
            break;
        case CANSM_NETPASSIVE_OVERRIDDEN:
            passive = FALSE;
            break;
        case CANSM_NETPASSIVE_FOLLOW_ECU:
        default:
            passive = CanSm_EcuPassive;
            break;
    }

    return passive;
}

/**
 * @brief Enters the wakeup validation state (CANSM_BSM_S_CHECKWAKEUP)
 * @req SHALL_CANSM - Enters the wakeup validation state
 * @details Simplified AUTOSAR wakeup source sequence: controller STOPPED and,
 *          when transceiver management is configured, transceiver NORMAL with
 *          the transceiver wakeup-flag check marked pending
 */
static Std_ReturnType CanSm_EnterCheckWakeup(uint8 NetworkIndex)
{
    Std_ReturnType result;
    CanSm_NetworkStateType* netState;
    const CanSm_NetworkConfigType* netConfig;

    netState = &CanSm_Global.Networks[NetworkIndex];
    netConfig = &CanSm_NetworkConfigs[NetworkIndex];

    netState->BsmState = CANSM_BSM_S_CHECKWAKEUP;
    netState->SubState = 0U;

    /* Wakeup validation: controller STOPPED */
    result = CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STOPPED);

    if (netConfig->TransceiverSupport == TRUE) {
        /* Wakeup validation: transceiver NORMAL, wakeup-flag check pending */
        netState->TrcvWufFlagIndication = TRUE;
        (void)CanIf_SetTrcvMode(netConfig->TransceiverId, CANIF_TRCV_MODE_NORMAL);
    }

    return result;
}

/**
 * @brief Transitions network to NOCOM state
 * @req SHALL_CANSM - Transitions network to NOCOM state
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
        netState->SubState = CANSM_S_CC_SLEEP_WAIT;
        netState->CurrentComMMode = COMM_NO_COMMUNICATION;
    }
    
    return result;
}

/**
 * @brief Transitions network to SILENTCOM state
 * @req SHALL_CANSM - Transitions network to SILENTCOM state
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
 * @req SHALL_CANSM - Transitions network to FULLCOM state
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
 * @req SHALL_CANSM - Process NOCOM state
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
                if (CanSm_IsNetworkPassive(NetworkIndex) == TRUE) {
                    /* Passive network: degrade FULL request to SILENTCOM */
                    result = CanSm_TransitionToSilentCom(NetworkIndex);
                } else {
                    /* Need to go through controller start sequence */
                    result = CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STARTED);
                    if (result == E_OK) {
                        netState->SubState = CANSM_S_FC_CC_START_WAIT;
                    }
                }
            }
            break;
            
        case CANSM_S_FC_CC_START_WAIT:
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
            
        case CANSM_S_CC_SLEEP_WAIT:
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
 * @req SHALL_CANSM - Process SILENTCOM state
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
                if (CanSm_IsNetworkPassive(NetworkIndex) == FALSE) {
                    result = CanSm_TransitionToFullCom(NetworkIndex);
                }
                /* Passive network: stay in SILENTCOM (degraded FULL request) */
            }
            /* Stay in SILENTCOM otherwise (listen mode) */
            break;
            
        case CANSM_S_CC_ONLINE:
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
 * @req SHALL_CANSM - Process FULLCOM state
 */
static Std_ReturnType CanSm_ProcessFullComState(uint8 NetworkIndex)
{
    Std_ReturnType result = E_OK;
    const CanSm_NetworkStateType* netState;
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
            
        case CANSM_S_FC_CC_START_WAIT:
            /* Waiting for controller mode confirmation */
            if (CanSm_IsTimerExpired(NetworkIndex)) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
                Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                               CANSM_SID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
#endif
                result = E_NOT_OK;
            }
            break;
    }

    return result;
}

/**
 * @brief Process SILENTCOM_BOR state (bus-off recovery)
 * @req SHALL_CANSM - Process SILENTCOM_BOR state
 */
static Std_ReturnType CanSm_ProcessSilentComBorState(uint8 NetworkIndex)
{
    Std_ReturnType result = E_OK;
    CanSm_NetworkStateType* netState;
    const CanSm_NetworkConfigType* netConfig;

    netState = &CanSm_Global.Networks[NetworkIndex];
    netConfig = &CanSm_NetworkConfigs[NetworkIndex];

    switch (netState->SubState) {
        case CANSM_S_BUSOFF_CHECK:
            /* Wait for controller STOPPED confirmation, then start L1 recovery time */
            if (netState->ModeChangePending == FALSE) {
                netState->BusOffRecoveryTimer = CANSM_BUSOFF_RECOVERY_L1_MS / netConfig->MainFunctionPeriodMs;
                netState->SubState = CANSM_S_BUSOFF_RECOVERY_L1;
            } else if (CanSm_IsTimerExpired(NetworkIndex)) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
                Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                               CANSM_SID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
#endif
                result = E_NOT_OK;
            }
            break;

        case CANSM_S_BUSOFF_RECOVERY_L1:
            /* L1 bus-off recovery time elapsed */
            if (netState->BusOffRecoveryTimer > 0U) {
                netState->BusOffRecoveryTimer--;
            }
            if (netState->BusOffRecoveryTimer == 0U) {
                netState->BusOffRecoveryTimer = CANSM_BUSOFF_RECOVERY_L2_MS / netConfig->MainFunctionPeriodMs;
                netState->SubState = CANSM_S_BUSOFF_RECOVERY_L2;
            }
            break;

        case CANSM_S_BUSOFF_RECOVERY_L2:
            /* L2 bus-off recovery time elapsed */
            if (netState->BusOffRecoveryTimer > 0U) {
                netState->BusOffRecoveryTimer--;
            }
            if (netState->BusOffRecoveryTimer == 0U) {
                netState->SubState = CANSM_S_BOR_RESTART_CC;
            }
            break;

        case CANSM_S_BOR_RESTART_CC:
            /* Restart the controller after bus-off recovery */
            result = CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STARTED);
            if (result == E_OK) {
                netState->SubState = CANSM_S_BOR_RESTART_CC_WAIT;
            }
            break;

        case CANSM_S_BOR_RESTART_CC_WAIT:
            /* STARTED confirmation completes recovery (CanSm_HandleModeConfirmation) */
            if (CanSm_IsTimerExpired(NetworkIndex)) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
                Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                               CANSM_SID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
#endif
                result = E_NOT_OK;
            }
            break;

        case CANSM_S_BOR_CC_STOPPED:
        case CANSM_S_BOR_CC_STOPPED_WAIT:
        default:
            netState->SubState = CANSM_S_BUSOFF_CHECK;
            break;
    }

    return result;
}

/**
 * @brief Process CHECKWAKEUP state (wakeup validation)
 * @req SHALL_CANSM - Process CHECKWAKEUP state
 * @details Wakeup validation completes when the controller is confirmed
 *          STOPPED and, if transceiver management is configured, the
 *          transceiver is indicated NORMAL and its wakeup-flag check is
 *          no longer pending. The network then enters FULLCOM. The
 *          validation is left via CanSM_StopWakeupSource (back to NOCOM).
 */
static Std_ReturnType CanSm_ProcessCheckWakeupState(uint8 NetworkIndex)
{
    Std_ReturnType result = E_OK;
    const CanSm_NetworkStateType* netState;
    const CanSm_NetworkConfigType* netConfig;

    netState = &CanSm_Global.Networks[NetworkIndex];
    netConfig = &CanSm_NetworkConfigs[NetworkIndex];

    if (netState->ModeChangePending == TRUE) {
        /* Waiting for controller STOPPED confirmation */
        if (CanSm_IsTimerExpired(NetworkIndex)) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                           CANSM_SID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
#endif
            result = E_NOT_OK;
        }
    } else if ((netState->CtrlMode == CANIF_CS_STOPPED) &&
               ((netConfig->TransceiverSupport == FALSE) ||
                ((netState->TrcvMode == CANIF_TRCV_MODE_NORMAL) &&
                 (netState->TrcvWufFlagIndication == FALSE)))) {
        /* Wakeup validation completed */
        result = CanSm_TransitionToFullCom(NetworkIndex);
    } else {
        /* Remain in CHECKWAKEUP until validation completes or is stopped */
    }

    return result;
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the CAN State Management module
 * @req SHALL_CANSM - Initializes the CAN State Management module
 */
void CanSM_Init(const CanSm_ConfigType* ConfigPtr)
{
    uint8 networkIdx;
    CanSm_NetworkStateType* netState;

    /* Pre-compile configuration: NULL_PTR selects the default config object */
    if (NULL_PTR == ConfigPtr) {
        ConfigPtr = &CanSm_Config;
    }

    CanSm_Global.InitStatus = CANSM_INIT;
    CanSm_Global.NumNetworks = CANSM_NUM_NETWORKS;
    CanSm_Global.ConfigPtr = ConfigPtr;
    CanSm_EcuPassive = FALSE;

    for (networkIdx = 0U; (networkIdx < CANSM_NUM_NETWORKS) && (networkIdx < CANSM_MAX_NETWORKS); networkIdx++) {
        netState = &CanSm_Global.Networks[networkIdx];

        netState->BsmState = CANSM_BSM_S_NOCOM;
        netState->SubState = CANSM_S_NOCOM_NOP;
        netState->RequestedComMMode = COMM_NO_COMMUNICATION;
        netState->CurrentComMMode = COMM_NO_COMMUNICATION;
        netState->ModeRequestTimer = 0U;
        netState->BusOffRecoveryTimer = 0U;
        netState->BusOffCounter = 0U;
        netState->BusOffEventPending = FALSE;
        netState->CurrentBaudrate = CANSM_DEFAULT_BAUDRATE;
        netState->RequestedBaudrateIndex = 0U;
        netState->BaudrateChangePending = FALSE;
        netState->RequestedCtrlMode = CANIF_CS_UNINIT;
        netState->CtrlMode = CANIF_CS_UNINIT;
        netState->TrcvMode = CANIF_TRCV_MODE_SLEEP;
        netState->ModeChangePending = FALSE;
        netState->TrcvWufFlagIndication = FALSE;
        netState->WakeupSourceRequested = FALSE;
        netState->PassiveOverride = CANSM_NETPASSIVE_FOLLOW_ECU;
        netState->PnState = CANSM_PNSA_NO_PN;
        netState->PnRequestPending = FALSE;
        netState->Initialized = TRUE;
    }
}

/**
 * @brief Deinitializes the CAN State Management module
 * @req SHALL_CANSM - Deinitializes the CAN State Management module
 */
void CanSM_DeInit(void)
{
    uint8 networkIdx;
    CanSm_NetworkStateType* netState;

    for (networkIdx = 0U; networkIdx < CANSM_MAX_NETWORKS; networkIdx++) {
        netState = &CanSm_Global.Networks[networkIdx];

        netState->BsmState = CANSM_BSM_S_NOTINITIALIZED;
        netState->SubState = 0U;
        netState->RequestedComMMode = COMM_NO_COMMUNICATION;
        netState->CurrentComMMode = COMM_NO_COMMUNICATION;
        netState->ModeRequestTimer = 0U;
        netState->BusOffRecoveryTimer = 0U;
        netState->BusOffCounter = 0U;
        netState->BusOffEventPending = FALSE;
        netState->CurrentBaudrate = 0U;
        netState->RequestedBaudrateIndex = 0U;
        netState->BaudrateChangePending = FALSE;
        netState->RequestedCtrlMode = CANIF_CS_UNINIT;
        netState->CtrlMode = CANIF_CS_UNINIT;
        netState->TrcvMode = CANIF_TRCV_MODE_SLEEP;
        netState->ModeChangePending = FALSE;
        netState->TrcvWufFlagIndication = FALSE;
        netState->WakeupSourceRequested = FALSE;
        netState->PassiveOverride = CANSM_NETPASSIVE_FOLLOW_ECU;
        netState->PnState = CANSM_PNSA_NO_PN;
        netState->PnRequestPending = FALSE;
        netState->Initialized = FALSE;
    }

    CanSm_EcuPassive = FALSE;
    CanSm_Global.InitStatus = CANSM_UNINIT;
    CanSm_Global.NumNetworks = 0U;
    CanSm_Global.ConfigPtr = NULL_PTR;
}

/**
 * @brief Controller mode indication callback from CanIf
 * @req SHALL_CANSM - Controller mode indication callback from CanIf
 */
void CanSM_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_CONTROLLERMODEINDICATION, CANSM_E_NOT_INITIALIZED);
#endif
        return;
    }

    for (networkIdx = 0U; networkIdx < CANSM_NUM_NETWORKS; networkIdx++) {
        if (CanSm_NetworkConfigs[networkIdx].ControllerId == ControllerId) {
            CanSm_HandleModeConfirmation(networkIdx, ControllerMode);
            return;
        }
    }

#if (CANSM_DEV_ERROR_DETECT == STD_ON)
    Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                    CANSM_SID_CONTROLLERMODEINDICATION, CANSM_E_PARAM_CONTROLLER);
#endif
}

/**
 * @brief Requests a communication mode change for a network
 * @req SHALL_CANSM - Requests a communication mode change for a network
 */
Std_ReturnType CanSM_RequestComMode(ComM_UserHandleType Network, ComM_ModeType ComM_Mode)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_REQUESTCOMMODE, CANSM_E_NOT_INITIALIZED);
#endif
        return E_NOT_OK;
    }

    if (CanSm_IsNetworkValid(Network) == FALSE) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_REQUESTCOMMODE, CANSM_E_PARAM_NETWORK);
#endif
        return E_NOT_OK;
    }

    if ((ComM_Mode != COMM_NO_COMMUNICATION) &&
        (ComM_Mode != COMM_SILENT_COMMUNICATION) &&
        (ComM_Mode != COMM_FULL_COMMUNICATION)) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_REQUESTCOMMODE, CANSM_E_INVALID_COMM_REQUEST);
#endif
        return E_NOT_OK;
    }

    networkIdx = CanSm_GetNetworkIndex(Network);

    /* Latch the requested mode; it is consumed by CanSM_MainFunction.
     * A passive network degrades FULL_COMMUNICATION to SILENT_COMMUNICATION
     * at consumption time (see CanSm_ProcessNoComState /
     * CanSm_ProcessSilentComState). */
    CanSm_Global.Networks[networkIdx].RequestedComMMode = ComM_Mode;

    return E_OK;
}

/**
 * @brief BusOff indication callback from CanIf
 * @req SHALL_CANSM - BusOff indication callback from CanIf
 */
void CanSM_ControllerBusOff(uint8 ControllerId)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_CONTROLLERBUSOFF, CANSM_E_NOT_INITIALIZED);
#endif
        return;
    }

    for (networkIdx = 0U; networkIdx < CANSM_NUM_NETWORKS; networkIdx++) {
        if (CanSm_NetworkConfigs[networkIdx].ControllerId == ControllerId) {
            CanSm_HandleBusOffRecovery(networkIdx);
            return;
        }
    }

#if (CANSM_DEV_ERROR_DETECT == STD_ON)
    Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                    CANSM_SID_CONTROLLERBUSOFF, CANSM_E_PARAM_CONTROLLER);
#endif
}

/**
 * @brief Main function for the CAN State Management module
 * @req SHALL_CANSM - Main function for the CAN State Management module
 */
void CanSM_MainFunction(void)
{
    uint8 networkIdx;
    CanSm_NetworkStateType* netState;
    const CanSm_NetworkConfigType* netConfig;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
        return;
    }

    for (networkIdx = 0U; (networkIdx < CANSM_NUM_NETWORKS) && (networkIdx < CANSM_MAX_NETWORKS); networkIdx++) {
        netState = &CanSm_Global.Networks[networkIdx];
        netConfig = &CanSm_NetworkConfigs[networkIdx];

        /* Consume a latched wakeup source request: enter the wakeup
         * validation state (controller STOPPED + transceiver NORMAL) */
        if (netState->WakeupSourceRequested == TRUE) {
            netState->WakeupSourceRequested = FALSE;
            if ((netConfig->WakeupSupport == TRUE) &&
                ((netState->BsmState == CANSM_BSM_S_NOCOM) ||
                 (netState->BsmState == CANSM_BSM_S_SILENTCOM))) {
                (void)CanSm_EnterCheckWakeup(networkIdx);
            }
        }

        /* PN (Partial Networking) state machine processing:
         * - without communication the sleep availability is not applicable
         *   and resets to NO_PN (a latched PN request is kept),
         * - in full communication a latched PN request is promoted from
         *   NO_PN to PN_REQUESTED, from where CanSM_ConfirmPnAvailability
         *   advances it to PN_AVAILABLE. */
        if (netState->BsmState == CANSM_BSM_S_NOCOM) {
            netState->PnState = CANSM_PNSA_NO_PN;
        } else if ((netState->BsmState == CANSM_BSM_S_FULLCOM) &&
                   (netState->PnRequestPending == TRUE) &&
                   (netState->PnState == CANSM_PNSA_NO_PN)) {
            netState->PnState = CANSM_PNSA_PN_REQUESTED;
        } else {
            /* PN_REQUESTED / PN_AVAILABLE unchanged by the cyclic run */
        }

        switch (netState->BsmState) {
            case CANSM_BSM_S_NOCOM:
                (void)CanSm_ProcessNoComState(networkIdx);
                break;
            case CANSM_BSM_S_SILENTCOM:
                (void)CanSm_ProcessSilentComState(networkIdx);
                break;
            case CANSM_BSM_S_FULLCOM:
                (void)CanSm_ProcessFullComState(networkIdx);
                break;
            case CANSM_BSM_S_SILENTCOM_BOR:
                (void)CanSm_ProcessSilentComBorState(networkIdx);
                break;
            case CANSM_BSM_S_CHECKWAKEUP:
                (void)CanSm_ProcessCheckWakeupState(networkIdx);
                break;
            case CANSM_BSM_S_NOTINITIALIZED:
            case CANSM_BSM_S_WAIT_MODE_CHANGE:
            case CANSM_BSM_S_CHANGEBAUDRATE:
            default:
                /* No cyclic processing */
                break;
        }
    }
}

/**
 * @brief Starts the wakeup source of a network (wakeup validation)
 * @req SHALL_CANSM - Starts the wakeup source of a network
 */
Std_ReturnType CanSM_StartWakeupSource(NetworkHandleType Network)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_STARTWAKEUPSOURCE, CANSM_E_NOT_INITIALIZED);
#endif
        return E_NOT_OK;
    }

    if (CanSm_IsNetworkValid(Network) == FALSE) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_STARTWAKEUPSOURCE, CANSM_E_PARAM_NETWORK);
#endif
        return E_NOT_OK;
    }

    networkIdx = CanSm_GetNetworkIndex(Network);

    /* Latch the request; consumed by the next CanSM_MainFunction cycle */
    CanSm_Global.Networks[networkIdx].WakeupSourceRequested = TRUE;

    return E_OK;
}

/**
 * @brief Stops the wakeup source of a network
 * @req SHALL_CANSM - Stops the wakeup source of a network
 */
Std_ReturnType CanSM_StopWakeupSource(NetworkHandleType Network)
{
    uint8 networkIdx;
    CanSm_NetworkStateType* netState;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_STOPWAKEUPSOURCE, CANSM_E_NOT_INITIALIZED);
#endif
        return E_NOT_OK;
    }

    if (CanSm_IsNetworkValid(Network) == FALSE) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_STOPWAKEUPSOURCE, CANSM_E_PARAM_NETWORK);
#endif
        return E_NOT_OK;
    }

    networkIdx = CanSm_GetNetworkIndex(Network);
    netState = &CanSm_Global.Networks[networkIdx];

    /* Clear a pending (not yet consumed) wakeup source request */
    netState->WakeupSourceRequested = FALSE;

    /* A network in wakeup validation returns to the regular NOCOM flow */
    if (netState->BsmState == CANSM_BSM_S_CHECKWAKEUP) {
        (void)CanSm_TransitionToNoCom(networkIdx);
    }

    return E_OK;
}

/**
 * @brief Sets the ECU-wide passive mode
 * @req SHALL_CANSM - Sets the ECU-wide passive mode
 */
Std_ReturnType CanSM_SetEcuPassive(boolean passive)
{
    CanSm_EcuPassive = (passive == TRUE) ? TRUE : FALSE;

    return E_OK;
}

/**
 * @brief Sets the passive mode of a single network (overrides the ECU-wide mode)
 * @req SHALL_CANSM - Sets the passive mode of a single network
 */
Std_ReturnType CanSM_SetNetworkPassive(NetworkHandleType NetworkHandle, boolean passive)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_SETNETWORKPASSIVE, CANSM_E_NOT_INITIALIZED);
#endif
        return E_NOT_OK;
    }

    if (CanSm_IsNetworkValid(NetworkHandle) == FALSE) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_SETNETWORKPASSIVE, CANSM_E_PARAM_NETWORK);
#endif
        return E_NOT_OK;
    }

    networkIdx = CanSm_GetNetworkIndex(NetworkHandle);

    /* Per-network override takes precedence over the ECU-wide passive mode */
    CanSm_Global.Networks[networkIdx].PassiveOverride =
        (passive == TRUE) ? CANSM_NETPASSIVE_FORCED : CANSM_NETPASSIVE_OVERRIDDEN;

    return E_OK;
}

/**
 * @brief TX timeout exception notification (CanIf timeout exception)
 * @req SHALL_CANSM - TX timeout exception notification
 */
void CanSM_TxTimeoutException(NetworkHandleType Network)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_TXTIMEOUTEXCEPTION, CANSM_E_NOT_INITIALIZED);
#endif
        return;
    }

    if (CanSm_IsNetworkValid(Network) == FALSE) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_TXTIMEOUTEXCEPTION, CANSM_E_PARAM_NETWORK);
#endif
        return;
    }

    networkIdx = CanSm_GetNetworkIndex(Network);

    /* Bus-off recovery entry semantics, shared with CanSM_ControllerBusOff:
     * enter CANSM_BSM_S_SILENTCOM_BOR at its initial sub-state */
    CanSm_Global.Networks[networkIdx].BusOffEventPending = TRUE;
    CanSm_EnterBusOffRecovery(networkIdx);
}

/**
 * @brief Transceiver mode indication callback from CanIf
 * @req SHALL_CANSM - Transceiver mode indication callback from CanIf
 */
void CanSM_TransceiverModeIndication(uint8 TransceiverId, CanIf_TransceiverModeType TransceiverMode)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_TRANSCEIVERMODEINDICATION, CANSM_E_NOT_INITIALIZED);
#endif
        return;
    }

    for (networkIdx = 0U; networkIdx < CANSM_NUM_NETWORKS; networkIdx++) {
        if ((CanSm_NetworkConfigs[networkIdx].TransceiverSupport == TRUE) &&
            (CanSm_NetworkConfigs[networkIdx].TransceiverId == TransceiverId)) {
            /* Update the transceiver mode recorded in the runtime */
            CanSm_Global.Networks[networkIdx].TrcvMode = TransceiverMode;
            return;
        }
    }

    /* Transceiver IDs outside the network configuration are managed by
     * CanIf; the indication is ignored without a development error */
}

/**
 * @brief Check transceiver wakeup flag indication callback from CanIf
 * @req SHALL_CANSM - Check transceiver wakeup flag indication callback
 */
void CanSM_CheckTransceiverWakeFlagIndication(uint8 TransceiverId)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_CHECKTRCVWAKEFLAGINDICATION, CANSM_E_NOT_INITIALIZED);
#endif
        return;
    }

    for (networkIdx = 0U; networkIdx < CANSM_NUM_NETWORKS; networkIdx++) {
        if ((CanSm_NetworkConfigs[networkIdx].TransceiverSupport == TRUE) &&
            (CanSm_NetworkConfigs[networkIdx].TransceiverId == TransceiverId)) {
            /* Clear the pending wakeup-flag check of this network. The
             * runtime has no separate wake-flag-check pending flag; the
             * indication is treated as completion of the transceiver
             * wakeup-flag check (wakeup validation marker). */
            CanSm_Global.Networks[networkIdx].TrcvWufFlagIndication = FALSE;
            return;
        }
    }

    /* Unknown transceiver: ignored without a development error (see above) */
}

/**
 * @brief Gets the current internal state of a network
 * @req SHALL_CANSM - Gets the current internal state of a network
 */
Std_ReturnType CanSM_GetCurrentInternalState(uint8 Network, CanSm_BsmStateType* StatePtr)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_GETCURRENTINTERNALSTATE, CANSM_E_NOT_INITIALIZED);
#endif
        return E_NOT_OK;
    }

    if (NULL_PTR == StatePtr) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_GETCURRENTINTERNALSTATE, CANSM_E_PARAM_POINTER);
#endif
        return E_NOT_OK;
    }

    if (CanSm_IsNetworkValid(Network) == FALSE) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_GETCURRENTINTERNALSTATE, CANSM_E_PARAM_NETWORK);
#endif
        return E_NOT_OK;
    }

    networkIdx = CanSm_GetNetworkIndex(Network);
    *StatePtr = CanSm_Global.Networks[networkIdx].BsmState;

    return E_OK;
}

/**
 * @brief Confirms partial networking availability for a network
 * @req SHALL_CANSM - Confirms partial networking availability for a network
 */
Std_ReturnType CanSM_ConfirmPnAvailability(NetworkHandleType NetworkHandle)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_CONFIRMPNAVAILABILITY, CANSM_E_NOT_INITIALIZED);
#endif
        return E_NOT_OK;
    }

    if (CanSm_IsNetworkValid(NetworkHandle) == FALSE) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_CONFIRMPNAVAILABILITY, CANSM_E_PARAM_NETWORK);
#endif
        return E_NOT_OK;
    }

    networkIdx = CanSm_GetNetworkIndex(NetworkHandle);

    /* Partial networking confirmation is only supported in full communication */
    if (CanSm_Global.Networks[networkIdx].BsmState != CANSM_BSM_S_FULLCOM) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_CONFIRMPNAVAILABILITY, CANSM_E_PARAM_INVALID_NETWORK_MODE);
#endif
        return E_NOT_OK;
    }

    /* PN sleep-availability confirmation: PN_REQUESTED -> PN_AVAILABLE. A
     * confirmation without a prior request (NO_PN) is accepted implicitly,
     * keeping the API's E_OK contract for transceivers that signal PN
     * availability autonomously. */
    CanSm_Global.Networks[networkIdx].PnState = CANSM_PNSA_PN_AVAILABLE;

    /* CanIf provides no ConfirmPnAvailability service in this integration;
     * acknowledge the availability directly */
    return E_OK;
}

/**
 * @brief Sets the PN (partial networking) wake-up request of a network
 * @req SHALL_CANSM - Sets the PN wake-up request of a network
 * @details Latches the request; CanSM_MainFunction promotes the PN state from
 *          CANSM_PNSA_NO_PN to CANSM_PNSA_PN_REQUESTED while the network is
 *          in full communication. Requires transceiver support, since the
 *          selective wake-up filtering is a transceiver capability.
 */
Std_ReturnType CanSM_SetPnRequest(NetworkHandleType NetworkHandle, boolean PnRequest)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_SETPNREQUEST, CANSM_E_NOT_INITIALIZED);
#endif
        return E_NOT_OK;
    }

    if (CanSm_IsNetworkValid(NetworkHandle) == FALSE) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_SETPNREQUEST, CANSM_E_PARAM_NETWORK);
#endif
        return E_NOT_OK;
    }

    networkIdx = CanSm_GetNetworkIndex(NetworkHandle);

    if (CanSm_NetworkConfigs[networkIdx].TransceiverSupport != TRUE) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_SETPNREQUEST, CANSM_E_PARAM_INVALID_NETWORK_MODE);
#endif
        return E_NOT_OK;
    }

    CanSm_Global.Networks[networkIdx].PnRequestPending = PnRequest;

    return E_OK;
}

/**
 * @brief Gets the PN sleep availability state of a network
 * @req SHALL_CANSM - Gets the PN sleep availability state of a network
 */
Std_ReturnType CanSM_GetPnState(NetworkHandleType NetworkHandle, CanSm_PnStateType* PnStatePtr)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_GETPNSTATE, CANSM_E_NOT_INITIALIZED);
#endif
        return E_NOT_OK;
    }

    if (NULL_PTR == PnStatePtr) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_GETPNSTATE, CANSM_E_PARAM_POINTER);
#endif
        return E_NOT_OK;
    }

    if (CanSm_IsNetworkValid(NetworkHandle) == FALSE) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_GETPNSTATE, CANSM_E_PARAM_NETWORK);
#endif
        return E_NOT_OK;
    }

    networkIdx = CanSm_GetNetworkIndex(NetworkHandle);
    *PnStatePtr = CanSm_Global.Networks[networkIdx].PnState;

    return E_OK;
}

/**
 * @brief Clears the transceiver wakeup-flag indication of a network
 * @req SHALL_CANSM - Clears the transceiver wakeup-flag indication of a network
 */
Std_ReturnType CanSM_ClearTrcvWufFlagIndication(NetworkHandleType NetworkHandle)
{
    uint8 networkIdx;

    if (CanSm_Global.InitStatus != CANSM_INIT) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_CLEARTRCVWUFFLAGINDICATION, CANSM_E_NOT_INITIALIZED);
#endif
        return E_NOT_OK;
    }

    if (CanSm_IsNetworkValid(NetworkHandle) == FALSE) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_CLEARTRCVWUFFLAGINDICATION, CANSM_E_PARAM_NETWORK);
#endif
        return E_NOT_OK;
    }

    networkIdx = CanSm_GetNetworkIndex(NetworkHandle);

    /* Wakeup-flag handling is only supported in full communication */
    if (CanSm_Global.Networks[networkIdx].BsmState != CANSM_BSM_S_FULLCOM) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID,
                        CANSM_SID_CLEARTRCVWUFFLAGINDICATION, CANSM_E_PARAM_INVALID_NETWORK_MODE);
#endif
        return E_NOT_OK;
    }

    /* CanIf provides no ClearTrcvWufFlag service in this integration;
     * clear the internal wakeup-flag indication directly */
    CanSm_Global.Networks[networkIdx].TrcvWufFlagIndication = FALSE;

    return E_OK;
}

#if (CANSM_VERSION_INFO_API == STD_ON)
/** @req SWS_CanSM_00009 */
void CanSm_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == versioninfo) {
        Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, 0x02U, CANSM_E_PARAM_POINTER);
        return;
    }
#endif
    versioninfo->vendorID = CANSM_VENDOR_ID;
    versioninfo->moduleID = CANSM_MODULE_ID;
    versioninfo->sw_major_version = CANSM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CANSM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CANSM_SW_PATCH_VERSION;
}
#endif
