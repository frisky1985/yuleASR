/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*******************************************************************************
* File: CanSm.c
* Description: Core implementation of CAN State Manager (CanSm)
*              AUTOSAR SWS CANStateManager 4.4.0 compliant
* Features:
*   - State machine (NO_COM -> SILENT_COM -> FULL_COM)
*   - Bus-off detection and recovery (T_RESTART, T_RECOVERY)
*   - Mode change notifications to ComM
*   - Transceiver control via CanIf
*   - Controller mode management
*******************************************************************************/

/*******************************************************************************
* Includes
*******************************************************************************/
#include "CanSm.h"
#include "CanIf.h"
#include "EcuM.h"

#if (CANSM_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*******************************************************************************
* Macros for Development Error Checking
*******************************************************************************/
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
#define CANSM_REPORT_ERROR(ApiId, ErrorId) \
    Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, (ApiId), (ErrorId))

#define CANSM_CHECK_INITIALIZED(ApiId) \
    do { \
        if (CanSm_InitStatus != TRUE) { \
            CANSM_REPORT_ERROR((ApiId), CANSM_E_UNINIT); \
            return; \
        } \
    } while(0)

#define CANSM_CHECK_INITIALIZED_RET(ApiId, RetVal) \
    do { \
        if (CanSm_InitStatus != TRUE) { \
            CANSM_REPORT_ERROR((ApiId), CANSM_E_UNINIT); \
            return (RetVal); \
        } \
    } while(0)

#define CANSM_CHECK_NETWORK(Network, ApiId, RetVal) \
    do { \
        if ((Network) >= CANSM_NETWORK_COUNT) { \
            CANSM_REPORT_ERROR((ApiId), CANSM_E_PARAM_INVALID_NETWORK); \
            return (RetVal); \
        } \
    } while(0)

#define CANSM_CHECK_POINTER(Pointer, ApiId, RetVal) \
    do { \
        if ((Pointer) == NULL_PTR) { \
            CANSM_REPORT_ERROR((ApiId), CANSM_E_PARAM_POINTER); \
            return (RetVal); \
        } \
    } while(0)
#else
#define CANSM_REPORT_ERROR(ApiId, ErrorId)
#define CANSM_CHECK_INITIALIZED(ApiId)
#define CANSM_CHECK_INITIALIZED_RET(ApiId, RetVal)
#define CANSM_CHECK_NETWORK(Network, ApiId, RetVal)
#define CANSM_CHECK_POINTER(Pointer, ApiId, RetVal)
#endif

/*******************************************************************************
* Internal State Variables
*******************************************************************************/
#define CANSM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Initialization status */
static boolean CanSm_InitStatus = FALSE;

/* Pointer to current configuration */
static const CanSm_ConfigType* CanSm_CurrentConfig = NULL_PTR;

/* Runtime data for all networks */
CanSm_NetworkRuntimeType CanSm_NetworkRuntime[CANSM_NETWORK_COUNT];

#define CANSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*******************************************************************************
* Static Function Prototypes
*******************************************************************************/
#define CANSM_START_SEC_CODE
#include "MemMap.h"

static void CanSm_ProcessNetwork(CanSm_NetworkHandleType network);
static void CanSm_ProcessBusOffRecovery(CanSm_NetworkHandleType network);
static void CanSm_StateTransition(
    CanSm_NetworkHandleType network,
    CanSm_NetworkStateType newState
);
static Std_ReturnType CanSm_SetControllerMode(
    CanSm_NetworkHandleType network,
    CanIf_ControllerModeType mode
);
#if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
static Std_ReturnType CanSm_SetTransceiverMode(
    CanSm_NetworkHandleType network,
    CanIf_TransceiverModeType mode
);
#endif
static void CanSm_NotifyComM(CanSm_NetworkHandleType network);
static void CanSm_StopNetwork(CanSm_NetworkHandleType network);
static void CanSm_StartNetworkSilent(CanSm_NetworkHandleType network);
static void CanSm_StartNetworkFull(CanSm_NetworkHandleType network);
static void CanSm_RestartControllerAfterBusOff(CanSm_NetworkHandleType network);

/*******************************************************************************
* Name: CanSm_Init
* Description: Initializes the CanSm module
*******************************************************************************/
void CanSm_Init(const CanSm_ConfigType* ConfigPtr)
{
    CanSm_NetworkHandleType netIdx;
    const CanSm_NetworkConfigType* netConfig;

    #if (CANSM_CONFIGURATION_VARIANT == CANSM_CONFIG_VARIANT_PRECOMPILE)
    /* In pre-compile variant, ConfigPtr is ignored */
    (void)ConfigPtr;
    CanSm_CurrentConfig = &CanSm_Config;
    #else
    /* In link-time/post-build variant, ConfigPtr is used */
    if (ConfigPtr == NULL_PTR) {
        #if (CANSM_DEV_ERROR_DETECT == STD_ON)
        CANSM_REPORT_ERROR(CANSM_SID_INIT, CANSM_E_PARAM_POINTER);
        #endif
        return;
    }
    CanSm_CurrentConfig = ConfigPtr;
    #endif

    /* Initialize all network runtime data */
    for (netIdx = 0U; netIdx < CANSM_NETWORK_COUNT; netIdx++) {
        netConfig = &CanSm_CurrentConfig->Networks[netIdx];

        CanSm_NetworkRuntime[netIdx].CurrentState = CANSM_NO_COM;
        CanSm_NetworkRuntime[netIdx].PreviousState = CANSM_NO_COM;
        CanSm_NetworkRuntime[netIdx].RequestedMode = CANSM_REQ_NONE;
        CanSm_NetworkRuntime[netIdx].ComMode = COMM_NO_COMMUNICATION;
        CanSm_NetworkRuntime[netIdx].BusOffDetected = FALSE;
        CanSm_NetworkRuntime[netIdx].BusOffState = CANSM_BOR_IDLE;
        CanSm_NetworkRuntime[netIdx].BusOffRetryCount = 0U;
        CanSm_NetworkRuntime[netIdx].BusOffTimer = 0.0f;
        CanSm_NetworkRuntime[netIdx].WakeupPending = FALSE;
        CanSm_NetworkRuntime[netIdx].ModeChangePending = FALSE;
        CanSm_NetworkRuntime[netIdx].RequestedControllerMode = CANIF_CS_STOPPED;
        CanSm_NetworkRuntime[netIdx].IndicatedControllerMode = CANIF_CS_STOPPED;
        #if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
        CanSm_NetworkRuntime[netIdx].RequestedTransceiverMode = CANIF_TRCV_MODE_STANDBY;
        CanSm_NetworkRuntime[netIdx].IndicatedTransceiverMode = CANIF_TRCV_MODE_STANDBY;
        #endif
    }

    CanSm_InitStatus = TRUE;
}

/*******************************************************************************
* Name: CanSm_DeInit
* Description: De-initializes the CanSm module
*******************************************************************************/
void CanSm_DeInit(void)
{
    CanSm_NetworkHandleType netIdx;

    CANSM_CHECK_INITIALIZED(CANSM_SID_DEINIT);

    /* Stop all networks */
    for (netIdx = 0U; netIdx < CANSM_NETWORK_COUNT; netIdx++) {
        CanSm_StopNetwork(netIdx);
    }

    CanSm_InitStatus = FALSE;
    CanSm_CurrentConfig = NULL_PTR;
}

/*******************************************************************************
* Name: CanSm_StartWakeUpSource
* Description: Starts the wake-up source
*******************************************************************************/
#if (CANSM_WAKEUP_SUPPORT == STD_ON)
Std_ReturnType CanSm_StartWakeUpSource(CanSm_NetworkHandleType network)
{
    CANSM_CHECK_INITIALIZED_RET(CANSM_SID_STARTWAKEUPSOURCE, E_NOT_OK);
    CANSM_CHECK_NETWORK(network, CANSM_SID_STARTWAKEUPSOURCE, E_NOT_OK);

    const CanSm_NetworkConfigType* netConfig = &CanSm_CurrentConfig->Networks[network];

    /* Set wakeup pending flag */
    CanSm_NetworkRuntime[network].WakeupPending = TRUE;

    /* Request wake-up mode for transceiver if supported */
    #if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
    if (netConfig->TransceiverCount > 0U) {
        (void)CanSm_SetTransceiverMode(network, CANIF_TRCV_MODE_NORMAL);
    }
    #endif

    /* Start the network in silent mode for monitoring */
    CanSm_NetworkRuntime[network].RequestedMode = CANSM_REQ_SILENT_COM;

    return E_OK;
}
#endif

/*******************************************************************************
* Name: CanSm_StopWakeUpSource
* Description: Stops the wake-up source
*******************************************************************************/
#if (CANSM_WAKEUP_SUPPORT == STD_ON)
Std_ReturnType CanSm_StopWakeUpSource(CanSm_NetworkHandleType network)
{
    CANSM_CHECK_INITIALIZED_RET(CANSM_SID_STOPWAKEUPSOURCE, E_NOT_OK);
    CANSM_CHECK_NETWORK(network, CANSM_SID_STOPWAKEUPSOURCE, E_NOT_OK);

    CanSm_NetworkRuntime[network].WakeupPending = FALSE;

    /* Go back to no communication */
    return CanSm_RequestComMode(network, COMM_NO_COMMUNICATION);
}
#endif

/*******************************************************************************
* Name: CanSm_RequestComMode
* Description: Requests a communication mode change from ComM
*******************************************************************************/
Std_ReturnType CanSm_RequestComMode(
    CanSm_NetworkHandleType network,
    ComM_ModeType ComM_Mode
)
{
    CANSM_CHECK_INITIALIZED_RET(CANSM_SID_REQUESTCOMMODE, E_NOT_OK);
    CANSM_CHECK_NETWORK(network, CANSM_SID_REQUESTCOMMODE, E_NOT_OK);

    /* Validate ComM_Mode */
    if ((ComM_Mode != COMM_NO_COMMUNICATION) &&
        (ComM_Mode != COMM_SILENT_COMMUNICATION) &&
        (ComM_Mode != COMM_FULL_COMMUNICATION)) {
        #if (CANSM_DEV_ERROR_DETECT == STD_ON)
        CANSM_REPORT_ERROR(CANSM_SID_REQUESTCOMMODE, CANSM_E_PARAM_INVALID_MODE);
        #endif
        return E_NOT_OK;
    }

    /* Convert ComM mode to internal request */
    switch (ComM_Mode) {
        case COMM_NO_COMMUNICATION:
            CanSm_NetworkRuntime[network].RequestedMode = CANSM_REQ_NO_COM;
            break;
        case COMM_SILENT_COMMUNICATION:
            CanSm_NetworkRuntime[network].RequestedMode = CANSM_REQ_SILENT_COM;
            break;
        case COMM_FULL_COMMUNICATION:
            CanSm_NetworkRuntime[network].RequestedMode = CANSM_REQ_FULL_COM;
            break;
        default:
            return E_NOT_OK;
    }

    return E_OK;
}

/*******************************************************************************
* Name: CanSm_GetCurrentComMode
* Description: Gets the current communication mode for ComM
*******************************************************************************/
Std_ReturnType CanSm_GetCurrentComMode(
    CanSm_NetworkHandleType network,
    ComM_ModeType* ComM_ModePtr
)
{
    CANSM_CHECK_INITIALIZED_RET(CANSM_SID_GETCURRENTCOMMODE, E_NOT_OK);
    CANSM_CHECK_NETWORK(network, CANSM_SID_GETCURRENTCOMMODE, E_NOT_OK);
    CANSM_CHECK_POINTER(ComM_ModePtr, CANSM_SID_GETCURRENTCOMMODE, E_NOT_OK);

    *ComM_ModePtr = CanSm_NetworkRuntime[network].ComMode;

    return E_OK;
}

/*******************************************************************************
* Name: CanSm_TxTimeoutException
* Description: Handles Tx timeout exception from CanIf
*******************************************************************************/
#if (CANSM_TX_TIMEOUT_EXCEPTION == STD_ON)
void CanSm_TxTimeoutException(CanSm_NetworkHandleType network)
{
    CANSM_CHECK_INITIALIZED(CANSM_SID_TXTIMEOUTEXCEPTION);
    CANSM_CHECK_NETWORK(network, CANSM_SID_TXTIMEOUTEXCEPTION,);

    /* In Tx timeout case, treat as bus-off equivalent */
    /* Request mode change to SILENT_COM to stop transmissions */
    if (CanSm_NetworkRuntime[network].CurrentState == CANSM_FULL_COM) {
        CanSm_NetworkRuntime[network].RequestedMode = CANSM_REQ_SILENT_COM;
    }
}
#endif

/*******************************************************************************
* Name: CanSm_MainFunction
* Description: Main function called periodically
*******************************************************************************/
void CanSm_MainFunction(void)
{
    CanSm_NetworkHandleType netIdx;

    if (CanSm_InitStatus != TRUE) {
        return;
    }

    /* Process each configured network */
    for (netIdx = 0U; netIdx < CanSm_CurrentConfig->NetworkCount; netIdx++) {
        CanSm_ProcessNetwork(netIdx);
    }
}

/*******************************************************************************
* Name: CanSm_GetVersionInfo
* Description: Returns version information of CanSm
*******************************************************************************/
#if (CANSM_VERSION_INFO_API == STD_ON)
void CanSm_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    CANSM_CHECK_POINTER(VersionInfo, CANSM_SID_GETVERSIONINFO,);

    VersionInfo->vendorID = 0U;  /* Vendor specific */
    VersionInfo->moduleID = CANSM_MODULE_ID;
    VersionInfo->sw_major_version = CANSM_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = CANSM_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = CANSM_SW_PATCH_VERSION;
}
#endif

/*******************************************************************************
* Name: CanSm_ControllerBusOff
* Description: Callback from CanIf indicating bus-off condition
*******************************************************************************/
void CanSm_ControllerBusOff(uint8 ControllerId)
{
    CanSm_NetworkHandleType netIdx;
    const CanSm_NetworkConfigType* netConfig;
    boolean found = FALSE;

    CANSM_CHECK_INITIALIZED(CANSM_SID_CONTROLLERBUSOFF);

    /* Find network containing this controller */
    for (netIdx = 0U; netIdx < CanSm_CurrentConfig->NetworkCount; netIdx++) {
        netConfig = &CanSm_CurrentConfig->Networks[netIdx];
        for (uint8 ctrlIdx = 0U; ctrlIdx < netConfig->ControllerCount; ctrlIdx++) {
            if (netConfig->ControllerRefs[ctrlIdx].ControllerId == ControllerId) {
                found = TRUE;
                break;
            }
        }
        if (found) {
            break;
        }
    }

    if (!found) {
        #if (CANSM_DEV_ERROR_DETECT == STD_ON)
        CANSM_REPORT_ERROR(CANSM_SID_CONTROLLERBUSOFF, CANSM_E_PARAM_INVALID_CONTROLLER);
        #endif
        return;
    }

    /* Mark bus-off detected */
    CanSm_NetworkRuntime[netIdx].BusOffDetected = TRUE;
    CanSm_NetworkRuntime[netIdx].BusOffState = CANSM_BOR_WAIT_RESTART;
    CanSm_NetworkRuntime[netIdx].BusOffTimer = 0.0f;

    /* Notify ComM about bus-off condition */
    if (CanSm_NetworkRuntime[netIdx].CurrentState == CANSM_FULL_COM) {
        CanSm_StateTransition(netIdx, CANSM_FULL_COM_BUS_OFF);
    } else if (CanSm_NetworkRuntime[netIdx].CurrentState == CANSM_SILENT_COM) {
        CanSm_StateTransition(netIdx, CANSM_SILENT_COM_BUS_OFF);
    }
}

/*******************************************************************************
* Name: CanSm_ControllerModeIndication
* Description: Callback from CanIf indicating mode change complete
*******************************************************************************/
void CanSm_ControllerModeIndication(
    uint8 ControllerId,
    CanIf_ControllerModeType ControllerMode
)
{
    CanSm_NetworkHandleType netIdx;
    const CanSm_NetworkConfigType* netConfig;
    boolean found = FALSE;

    CANSM_CHECK_INITIALIZED(CANSM_SID_CONTROLLERMODEINDICATION);

    /* Find network containing this controller */
    for (netIdx = 0U; netIdx < CanSm_CurrentConfig->NetworkCount; netIdx++) {
        netConfig = &CanSm_CurrentConfig->Networks[netIdx];
        for (uint8 ctrlIdx = 0U; ctrlIdx < netConfig->ControllerCount; ctrlIdx++) {
            if (netConfig->ControllerRefs[ctrlIdx].ControllerId == ControllerId) {
                found = TRUE;
                break;
            }
        }
        if (found) {
            break;
        }
    }

    if (!found) {
        return;
    }

    CanSm_NetworkRuntime[netIdx].IndicatedControllerMode = ControllerMode;
    CanSm_NetworkRuntime[netIdx].ModeChangePending = FALSE;
}

/*******************************************************************************
* Name: CanSm_TransceiverModeIndication
* Description: Callback from CanIf indicating transceiver mode change
*******************************************************************************/
#if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
void CanSm_TransceiverModeIndication(
    uint8 TransceiverId,
    CanIf_TransceiverModeType TransceiverMode
)
{
    CanSm_NetworkHandleType netIdx;
    const CanSm_NetworkConfigType* netConfig;
    boolean found = FALSE;

    CANSM_CHECK_INITIALIZED(CANSM_SID_TRCVMODEINDICATION);

    /* Find network containing this transceiver */
    for (netIdx = 0U; netIdx < CanSm_CurrentConfig->NetworkCount; netIdx++) {
        netConfig = &CanSm_CurrentConfig->Networks[netIdx];
        for (uint8 trcvIdx = 0U; trcvIdx < netConfig->TransceiverCount; trcvIdx++) {
            if (netConfig->TransceiverRefs[trcvIdx].TransceiverId == TransceiverId) {
                found = TRUE;
                break;
            }
        }
        if (found) {
            break;
        }
    }

    if (!found) {
        return;
    }

    CanSm_NetworkRuntime[netIdx].IndicatedTransceiverMode = TransceiverMode;
}
#endif

/*******************************************************************************
* Name: CanSm_CheckWakeup
* Description: Check for wake-up event
*******************************************************************************/
#if (CANSM_WAKEUP_SUPPORT == STD_ON)
void CanSm_CheckWakeup(uint8 TransceiverId)
{
    (void)TransceiverId;
    /* Implementation depends on hardware-specific wake-up checking */
}
#endif

/*******************************************************************************
* Static Functions Implementation
*******************************************************************************/

/*******************************************************************************
* Name: CanSm_ProcessNetwork
* Description: Process state machine for a single network
*******************************************************************************/
static void CanSm_ProcessNetwork(CanSm_NetworkHandleType network)
{
    CanSm_NetworkRuntimeType* runtime = &CanSm_NetworkRuntime[network];
    const CanSm_NetworkConfigType* netConfig = &CanSm_CurrentConfig->Networks[network];

    /* Update bus-off timer */
    if (runtime->BusOffState != CANSM_BOR_IDLE) {
        runtime->BusOffTimer += CANSM_MAIN_FUNCTION_PERIOD;
    }

    /* Process bus-off recovery if active */
    if (runtime->BusOffDetected) {
        CanSm_ProcessBusOffRecovery(network);
        return;
    }

    /* Process mode requests */
    switch (runtime->RequestedMode) {
        case CANSM_REQ_NO_COM:
            if (runtime->CurrentState != CANSM_NO_COM) {
                CanSm_StopNetwork(network);
                if (!runtime->ModeChangePending) {
                    CanSm_StateTransition(network, CANSM_NO_COM);
                }
            }
            runtime->RequestedMode = CANSM_REQ_NONE;
            break;

        case CANSM_REQ_SILENT_COM:
            if (runtime->CurrentState == CANSM_NO_COM) {
                CanSm_StartNetworkSilent(network);
                if (!runtime->ModeChangePending) {
                    CanSm_StateTransition(network, CANSM_SILENT_COM);
                }
            } else if (runtime->CurrentState == CANSM_FULL_COM) {
                /* Transition from FULL_COM to SILENT_COM */
                (void)CanSm_SetControllerMode(network, CANIF_CS_STARTED);
                if (!runtime->ModeChangePending) {
                    CanSm_StateTransition(network, CANSM_SILENT_COM);
                }
            }
            runtime->RequestedMode = CANSM_REQ_NONE;
            break;

        case CANSM_REQ_FULL_COM:
            if ((runtime->CurrentState == CANSM_NO_COM) ||
                (runtime->CurrentState == CANSM_SILENT_COM)) {
                CanSm_StartNetworkFull(network);
                if (!runtime->ModeChangePending) {
                    CanSm_StateTransition(network, CANSM_FULL_COM);
                }
            }
            runtime->RequestedMode = CANSM_REQ_NONE;
            break;

        case CANSM_REQ_NONE:
        default:
            /* No request to process */
            break;
    }

    /* Check if mode change has completed */
    if (runtime->ModeChangePending) {
        if (runtime->IndicatedControllerMode == runtime->RequestedControllerMode) {
            runtime->ModeChangePending = FALSE;
            /* State transition already happened when request was made */
        }
    }

    /* Check if bus-off occurred while in operation */
    if (runtime->BusOffState == CANSM_BOR_IDLE) {
        /* Check controller state - if bus-off detected by polling */
        CanIf_ControllerModeType currentMode;
        if (CanIf_GetControllerMode(
                netConfig->ControllerRefs[0].ControllerId,
                &currentMode) == E_OK) {
            if (currentMode == CANIF_CS_STOPPED) {
                /* Controller stopped unexpectedly - could be bus-off */
                /* This would typically be handled via the callback */
            }
        }
    }
}

/*******************************************************************************
* Name: CanSm_ProcessBusOffRecovery
* Description: Process bus-off recovery state machine
*******************************************************************************/
static void CanSm_ProcessBusOffRecovery(CanSm_NetworkHandleType network)
{
    CanSm_NetworkRuntimeType* runtime = &CanSm_NetworkRuntime[network];
    const CanSm_NetworkConfigType* netConfig = &CanSm_CurrentConfig->Networks[network];

    switch (runtime->BusOffState) {
        case CANSM_BOR_WAIT_RESTART:
            /* Wait T_RESTART time before restarting controller */
            if (runtime->BusOffTimer >= netConfig->TRestart) {
                /* Reset the controller */
                CanSm_RestartControllerAfterBusOff(network);
                runtime->BusOffState = CANSM_BOR_WAIT_RECOVERY;
                runtime->BusOffTimer = 0.0f;
            }
            break;

        case CANSM_BOR_WAIT_RECOVERY:
            /* Wait T_RECOVERY time to ensure bus is stable */
            if (runtime->BusOffTimer >= netConfig->TRecovery) {
                /* Check if controller is started successfully */
                if (runtime->IndicatedControllerMode == CANIF_CS_STARTED) {
                    /* Recovery successful */
                    runtime->BusOffDetected = FALSE;
                    runtime->BusOffState = CANSM_BOR_IDLE;
                    runtime->BusOffRetryCount = 0U;
                    runtime->BusOffTimer = 0.0f;

                    /* Return to appropriate state */
                    if (runtime->CurrentState == CANSM_FULL_COM_BUS_OFF) {
                        CanSm_StateTransition(network, CANSM_FULL_COM);
                    } else if (runtime->CurrentState == CANSM_SILENT_COM_BUS_OFF) {
                        CanSm_StateTransition(network, CANSM_SILENT_COM);
                    }
                } else {
                    /* Recovery failed - check retry count */
                    runtime->BusOffRetryCount++;
                    if (runtime->BusOffRetryCount >= netConfig->BusOffMaxRetries) {
                        /* Max retries exceeded - declare failure */
                        runtime->BusOffState = CANSM_BOR_FAILED;
                        /* Notify ComM about failure */
                        ComM_BusSM_ModeIndication(
                            network,
                            COMM_NO_COMMUNICATION
                        );
                    } else {
                        /* Try again - go back to WAIT_RESTART */
                        runtime->BusOffState = CANSM_BOR_WAIT_RESTART;
                        runtime->BusOffTimer = 0.0f;
                    }
                }
            }
            break;

        case CANSM_BOR_FAILED:
            /* Recovery failed - stay in failed state */
            /* Application needs to handle this (e.g., restart network) */
            break;

        case CANSM_BOR_IDLE:
        default:
            /* Should not reach here when BusOffDetected is TRUE */
            break;
    }
}

/*******************************************************************************
* Name: CanSm_StateTransition
* Description: Handle state transition and notify ComM
*******************************************************************************/
static void CanSm_StateTransition(
    CanSm_NetworkHandleType network,
    CanSm_NetworkStateType newState
)
{
    CanSm_NetworkRuntimeType* runtime = &CanSm_NetworkRuntime[network];
    ComM_ModeType commMode;

    /* Save previous state */
    runtime->PreviousState = runtime->CurrentState;
    runtime->CurrentState = newState;

    /* Map internal state to ComM mode */
    switch (newState) {
        case CANSM_NO_COM:
            commMode = COMM_NO_COMMUNICATION;
            break;
        case CANSM_SILENT_COM:
        case CANSM_SILENT_COM_BUS_OFF:
            commMode = COMM_SILENT_COMMUNICATION;
            break;
        case CANSM_FULL_COM:
        case CANSM_FULL_COM_BUS_OFF:
            commMode = COMM_FULL_COMMUNICATION;
            break;
        default:
            commMode = COMM_NO_COMMUNICATION;
            break;
    }

    runtime->ComMode = commMode;

    /* Notify ComM about mode change */
    CanSm_NotifyComM(network);
}

/*******************************************************************************
* Name: CanSm_NotifyComM
* Description: Notify ComM about mode change
*******************************************************************************/
static void CanSm_NotifyComM(CanSm_NetworkHandleType network)
{
    /* Call ComM_BusSM_ModeIndication to inform ComM */
    ComM_BusSM_ModeIndication(network, CanSm_NetworkRuntime[network].ComMode);
}

/*******************************************************************************
* Name: CanSm_SetControllerMode
* Description: Set controller mode via CanIf
*******************************************************************************/
static Std_ReturnType CanSm_SetControllerMode(
    CanSm_NetworkHandleType network,
    CanIf_ControllerModeType mode
)
{
    const CanSm_NetworkConfigType* netConfig = &CanSm_CurrentConfig->Networks[network];
    Std_ReturnType result = E_OK;

    CanSm_NetworkRuntime[network].RequestedControllerMode = mode;

    /* Set mode for all controllers in this network */
    for (uint8 ctrlIdx = 0U; ctrlIdx < netConfig->ControllerCount; ctrlIdx++) {
        result = CanIf_SetControllerMode(
            netConfig->ControllerRefs[ctrlIdx].ControllerId,
            mode
        );
        if (result != E_OK) {
            break;
        }
    }

    if (result == E_OK) {
        CanSm_NetworkRuntime[network].ModeChangePending = TRUE;
    }

    return result;
}

/*******************************************************************************
* Name: CanSm_SetTransceiverMode
* Description: Set transceiver mode via CanIf
*******************************************************************************/
#if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
static Std_ReturnType CanSm_SetTransceiverMode(
    CanSm_NetworkHandleType network,
    CanIf_TransceiverModeType mode
)
{
    const CanSm_NetworkConfigType* netConfig = &CanSm_CurrentConfig->Networks[network];
    Std_ReturnType result = E_OK;

    CanSm_NetworkRuntime[network].RequestedTransceiverMode = mode;

    /* Set mode for all transceivers in this network */
    for (uint8 trcvIdx = 0U; trcvIdx < netConfig->TransceiverCount; trcvIdx++) {
        result = CanIf_SetTrcvMode(
            netConfig->TransceiverRefs[trcvIdx].TransceiverId,
            mode
        );
        if (result != E_OK) {
            break;
        }
    }

    return result;
}
#endif

/*******************************************************************************
* Name: CanSm_StopNetwork
* Description: Stop all communication on a network (NO_COM state)
*******************************************************************************/
static void CanSm_StopNetwork(CanSm_NetworkHandleType network)
{
    const CanSm_NetworkConfigType* netConfig = &CanSm_CurrentConfig->Networks[network];

    /* Stop controllers */
    (void)CanSm_SetControllerMode(network, CANIF_CS_STOPPED);

    #if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
    /* Put transceivers in standby */
    if (netConfig->TransceiverCount > 0U) {
        (void)CanSm_SetTransceiverMode(network, CANIF_TRCV_MODE_STANDBY);
    }
    #endif
}

/*******************************************************************************
* Name: CanSm_StartNetworkSilent
* Description: Start network in silent mode (SILENT_COM state)
*******************************************************************************/
static void CanSm_StartNetworkSilent(CanSm_NetworkHandleType network)
{
    const CanSm_NetworkConfigType* netConfig = &CanSm_CurrentConfig->Networks[network];

    #if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
    /* Wake up transceivers if needed */
    if (netConfig->TransceiverCount > 0U) {
        (void)CanSm_SetTransceiverMode(network, CANIF_TRCV_MODE_NORMAL);
    }
    #endif

    /* Start controllers in mode that allows receiving but not transmitting */
    /* CANIF_CS_STARTED allows both - this is handled by higher layers */
    /* SILENT_COM is achieved by not having ComM in FULL_COM */
    (void)CanSm_SetControllerMode(network, CANIF_CS_STARTED);
}

/*******************************************************************************
* Name: CanSm_StartNetworkFull
* Description: Start network in full communication mode (FULL_COM state)
*******************************************************************************/
static void CanSm_StartNetworkFull(CanSm_NetworkHandleType network)
{
    const CanSm_NetworkConfigType* netConfig = &CanSm_CurrentConfig->Networks[network];

    #if (CANSM_TRANSCEIVER_SWITCH_OFF == STD_ON)
    /* Ensure transceivers are in normal mode */
    if (netConfig->TransceiverCount > 0U) {
        (void)CanSm_SetTransceiverMode(network, CANIF_TRCV_MODE_NORMAL);
    }
    #endif

    /* Start controllers for full communication */
    (void)CanSm_SetControllerMode(network, CANIF_CS_STARTED);
}

/*******************************************************************************
* Name: CanSm_RestartControllerAfterBusOff
* Description: Restart controller after bus-off detected
*******************************************************************************/
static void CanSm_RestartControllerAfterBusOff(CanSm_NetworkHandleType network)
{
    const CanSm_NetworkConfigType* netConfig = &CanSm_CurrentConfig->Networks[network];

    /* Stop then start the controller to recover from bus-off */
    for (uint8 ctrlIdx = 0U; ctrlIdx < netConfig->ControllerCount; ctrlIdx++) {
        /* First stop the controller */
        (void)CanIf_SetControllerMode(
            netConfig->ControllerRefs[ctrlIdx].ControllerId,
            CANIF_CS_STOPPED
        );
    }

    /* Small delay would be hardware-specific - here we rely on T_RESTART timing */

    /* Restart controllers */
    for (uint8 ctrlIdx = 0U; ctrlIdx < netConfig->ControllerCount; ctrlIdx++) {
        (void)CanIf_SetControllerMode(
            netConfig->ControllerRefs[ctrlIdx].ControllerId,
            CANIF_CS_STARTED
        );
    }

    CanSm_NetworkRuntime[network].ModeChangePending = TRUE;
}

#define CANSM_STOP_SEC_CODE
#include "MemMap.h"
