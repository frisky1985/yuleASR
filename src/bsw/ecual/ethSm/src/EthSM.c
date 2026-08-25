/**
 * @file EthSM.c
 * @brief Ethernet State Manager (EthSM) Implementation
 * @version 1.0.0
 * @date 2026-05-05
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Ethernet State Manager (ETHSM)
 * Layer: ECU Abstraction Layer (ECUAL)
 * AUTOSAR Version: 4.4.0
 *
 * State Machine:
 *   UNINIT -> NO_COM (on Init)
 *   NO_COM -> WAIT_TRCVLINK (on FullComm requested)
 *   WAIT_TRCVLINK -> WAIT_ONLINE (on Link Up)
 *   WAIT_TRCVLINK -> NO_COM (on timeout or NoComm requested)
 *   WAIT_ONLINE -> COM_READY (on TcpIp Online)
 *   WAIT_ONLINE -> NO_COM (on timeout or NoComm requested)
 *   COM_READY -> ONHOLD (on TcpIp OnHold)
 *   COM_READY -> NO_COM (on NoComm requested)
 *   ONHOLD -> COM_READY (on TcpIp Online)
 *   ONHOLD -> NO_COM (on NoComm requested)
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "EthSM.h"
#include "EthSM_Cfg.h"
#include "EthIf.h"
#include "ComM.h"
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
*                                    INTERNAL DEFINES
==================================================================================================*/
#define ETHSM_INITIALIZED                   (0xA5U)
#define ETHSM_NOT_INITIALIZED               (0x00U)

/*==================================================================================================
*                                    INTERNAL TYPES
==================================================================================================*/
/**
 * @brief Internal network state structure
 */
typedef struct {
    EthSM_StateType currentState;           /**< Current state machine state */
    ComM_ModeType requestedComMode;         /**< Requested communication mode */
    ComM_ModeType currentComMode;           /**< Current communication mode */
    TcpIp_StateType tcpIpState;             /**< Current TcpIp state */
    uint16 timeoutCounter;                  /**< Timeout counter (in MainFunction cycles) */
    uint8 retryCounter;                     /**< Retry counter for failed operations */
    boolean linkStateUp;                    /**< Transceiver link state */
    boolean initDone;                       /**< Network initialized flag */
} EthSM_NetworkStateType;

/*==================================================================================================
*                                    INTERNAL VARIABLES
==================================================================================================*/
/**
 * @brief Module initialization state
 */
static uint8 EthSM_InitState = ETHSM_NOT_INITIALIZED;

/**
 * @brief Network state array
 */
static EthSM_NetworkStateType EthSM_NetworkState[ETHSM_MAX_NETWORKS];

/**
 * @brief Configuration pointer
 */
static const EthSM_ConfigType* EthSM_ConfigPtr = NULL_PTR;

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static void EthSM_ProcessState_NO_COM(uint8 networkIdx);
static void EthSM_ProcessState_WAIT_TRCVLINK(uint8 networkIdx);
static void EthSM_ProcessState_WAIT_ONLINE(uint8 networkIdx);
static void EthSM_ProcessState_ONHOLD(uint8 networkIdx);
static void EthSM_ProcessState_COM_READY(uint8 networkIdx);
static void EthSM_TransitionToState(uint8 networkIdx, EthSM_StateType newState);
static void EthSM_NotifyComM(uint8 networkIdx, ComM_ModeType mode);
static boolean EthSM_CheckLinkState(uint8 networkIdx);
static boolean EthSM_IsValidNetwork(EthSM_NetworkHandleType networkHandle);
static uint8 EthSM_GetNetworkIndex(EthSM_NetworkHandleType networkHandle);

/*==================================================================================================
*                                    LOCAL FUNCTION IMPLEMENTATIONS
==================================================================================================*/

/**
 * @brief Checks if network handle is valid
 */
static boolean EthSM_IsValidNetwork(EthSM_NetworkHandleType networkHandle)
{
    uint8 idx;
    boolean valid = FALSE;

    for (idx = 0U; idx < ETHSM_MAX_NETWORKS; idx++)
    {
        if (networkHandle == idx)
        {
            valid = TRUE;
            break;
        }
    }
    return valid;
}

/**
 * @brief Gets network index from handle
 */
static uint8 EthSM_GetNetworkIndex(EthSM_NetworkHandleType networkHandle)
{
    return (uint8)networkHandle;
}

/**
 * @brief Checks transceiver link state via EthIf
 */
static boolean EthSM_CheckLinkState(uint8 networkIdx)
{
    boolean linkUp = FALSE;
    uint8 trcvIdx = 0U;
    Std_ReturnType result;
    EthIf_LinkStateType linkState;

    /* Get transceiver index for this network */
    if (networkIdx == ETHSM_NETWORK_0)
    {
        trcvIdx = ETHSM_TRCV_IDX_NETWORK_0;
    }
    else if (networkIdx == ETHSM_NETWORK_1)
    {
        trcvIdx = ETHSM_TRCV_IDX_NETWORK_1;
    }
    else
    {
        trcvIdx = networkIdx;
    }

    /* Query link state from EthIf */
#if defined(ETHIF_VERSION_INFO_API)
    result = EthIf_GetTransceiverLinkState(trcvIdx, &linkState);
    if (result == E_OK)
    {
        linkUp = (linkState == ETHIF_LINK_STATE_ACTIVE) ? TRUE : FALSE;
    }
#else
    /* If EthIf API not available, assume link is up for simulation */
    linkUp = TRUE;
#endif

    return linkUp;
}

/**
 * @brief Notifies ComM about mode change
 */
static void EthSM_NotifyComM(uint8 networkIdx, ComM_ModeType mode)
{
#if (ETHSM_STATE_CHANGE_CALLBACK == STD_ON)
    ComM_ChannelHandleType channelHandle;

    /* Map network index to ComM channel handle */
    channelHandle = (ComM_ChannelHandleType)networkIdx;

    /* Notify ComM about mode change */
    ComM_BusSM_ModeIndication(channelHandle, mode);
#else
    (void)networkIdx;
    (void)mode;
#endif
}

/**
 * @brief Performs state transition and notifies ComM
 */
static void EthSM_TransitionToState(uint8 networkIdx, EthSM_StateType newState)
{
    EthSM_NetworkStateType* netState;
    ComM_ModeType newComMode;

    netState = &EthSM_NetworkState[networkIdx];

    /* Update state */
    netState->currentState = newState;

    /* Reset timeout counter */
    netState->timeoutCounter = 0U;

    /* Map internal state to ComM mode */
    switch (newState)
    {
        case ETHSM_STATE_NO_COM:
            newComMode = COMM_NO_COMMUNICATION;
            break;

        case ETHSM_STATE_WAIT_TRCVLINK:
            /* Intermediate state - report as Silent (bus active but no full comm) */
            newComMode = COMM_SILENT_COMMUNICATION;
            break;

        case ETHSM_STATE_WAIT_ONLINE:
            /* Intermediate state - report as Silent */
            newComMode = COMM_SILENT_COMMUNICATION;
            break;

        case ETHSM_STATE_ONHOLD:
            /* Communication on hold - report as Silent */
            newComMode = COMM_SILENT_COMMUNICATION;
            break;

        case ETHSM_STATE_COM_READY:
            newComMode = COMM_FULL_COMMUNICATION;
            break;

        default:
            newComMode = COMM_NO_COMMUNICATION;
            break;
    }

    /* Update current mode */
    netState->currentComMode = newComMode;

    /* Notify ComM */
    EthSM_NotifyComM(networkIdx, newComMode);
}

/**
 * @brief Processes NO_COM state
 */
static void EthSM_ProcessState_NO_COM(uint8 networkIdx)
{
    EthSM_NetworkStateType* netState;
    uint8 ctrlIdx = 0U;
    Std_ReturnType result;

    netState = &EthSM_NetworkState[networkIdx];

    /* Check if Full Communication is requested */
    if (netState->requestedComMode == COMM_FULL_COMMUNICATION)
    {
        /* Get controller index */
        if (networkIdx == ETHSM_NETWORK_0)
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_0;
        }
        else
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_1;
        }

        /* Initialize Ethernet Controller via EthIf */
#if defined(ETHIF_VERSION_INFO_API)
        result = EthIf_SetControllerMode(ctrlIdx, ETHIF_MODE_ACTIVE);
#else
        result = E_OK; /* For simulation */
#endif

        if (result == E_OK)
        {
            /* Transition to WAIT_TRCVLINK */
            EthSM_TransitionToState(networkIdx, ETHSM_STATE_WAIT_TRCVLINK);
        }
        else
        {
            /* Retry mechanism */
            netState->retryCounter++;
            if (netState->retryCounter >= ETHSM_MAX_RETRIES)
            {
                /* Max retries exceeded, stay in NO_COM */
                netState->retryCounter = 0U;
                netState->requestedComMode = COMM_NO_COMMUNICATION;
            }
        }
    }
    else
    {
        /* Stay in NO_COM, reset retry counter */
        netState->retryCounter = 0U;
    }
}

/**
 * @brief Processes WAIT_TRCVLINK state
 */
static void EthSM_ProcessState_WAIT_TRCVLINK(uint8 networkIdx)
{
    EthSM_NetworkStateType* netState;
    boolean linkUp;
    uint8 ctrlIdx = 0U;
    Std_ReturnType result;

    netState = &EthSM_NetworkState[networkIdx];

    /* Check if No Communication is requested */
    if (netState->requestedComMode == COMM_NO_COMMUNICATION)
    {
        /* Shut down controller */
        if (networkIdx == ETHSM_NETWORK_0)
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_0;
        }
        else
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_1;
        }

#if defined(ETHIF_VERSION_INFO_API)
        (void)EthIf_SetControllerMode(ctrlIdx, ETHIF_MODE_DOWN);
#endif

        /* Transition back to NO_COM */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_NO_COM);
        return;
    }

    /* Check transceiver link state */
    linkUp = EthSM_CheckLinkState(networkIdx);

    if (linkUp == TRUE)
    {
        /* Link is up, request TcpIp to go online */
        netState->tcpIpState = TCPIP_STATE_STARTUP;

        /* Transition to WAIT_ONLINE */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_WAIT_ONLINE);
    }
    else
    {
        /* Increment timeout counter */
        netState->timeoutCounter += ETHSM_MAIN_FUNCTION_CYCLE_MS;

        /* Check timeout */
        if (netState->timeoutCounter >= ETHSM_TIMEOUT_WAIT_TRCVLINK)
        {
            /* Timeout - shut down and go back to NO_COM */
            if (networkIdx == ETHSM_NETWORK_0)
            {
                ctrlIdx = ETHSM_CTRL_IDX_NETWORK_0;
            }
            else
            {
                ctrlIdx = ETHSM_CTRL_IDX_NETWORK_1;
            }

#if defined(ETHIF_VERSION_INFO_API)
            (void)EthIf_SetControllerMode(ctrlIdx, ETHIF_MODE_DOWN);
#endif

            EthSM_TransitionToState(networkIdx, ETHSM_STATE_NO_COM);
            netState->requestedComMode = COMM_NO_COMMUNICATION;
        }
    }

    /* Store link state */
    netState->linkStateUp = linkUp;
}

/**
 * @brief Processes WAIT_ONLINE state
 */
static void EthSM_ProcessState_WAIT_ONLINE(uint8 networkIdx)
{
    EthSM_NetworkStateType* netState;
    uint8 ctrlIdx = 0U;

    netState = &EthSM_NetworkState[networkIdx];

    /* Check if No Communication is requested */
    if (netState->requestedComMode == COMM_NO_COMMUNICATION)
    {
        /* Shut down controller */
        if (networkIdx == ETHSM_NETWORK_0)
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_0;
        }
        else
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_1;
        }

#if defined(ETHIF_VERSION_INFO_API)
        (void)EthIf_SetControllerMode(ctrlIdx, ETHIF_MODE_DOWN);
#endif

        /* Transition back to NO_COM */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_NO_COM);
        return;
    }

    /* Check TcpIp state (updated via EthSM_TcpIpModeIndication) */
    if (netState->tcpIpState == TCPIP_STATE_ONLINE)
    {
        /* TcpIp is online, transition to COM_READY */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_COM_READY);
    }
    else if (netState->tcpIpState == TCPIP_STATE_OFFLINE)
    {
        /* TcpIp went offline unexpectedly, go back to WAIT_TRCVLINK */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_WAIT_TRCVLINK);
    }
    else
    {
        /* Still waiting for TcpIp */
        netState->timeoutCounter += ETHSM_MAIN_FUNCTION_CYCLE_MS;

        /* Check timeout */
        if (netState->timeoutCounter >= ETHSM_TIMEOUT_WAIT_ONLINE)
        {
            /* Timeout - shut down and go back to NO_COM */
            if (networkIdx == ETHSM_NETWORK_0)
            {
                ctrlIdx = ETHSM_CTRL_IDX_NETWORK_0;
            }
            else
            {
                ctrlIdx = ETHSM_CTRL_IDX_NETWORK_1;
            }

#if defined(ETHIF_VERSION_INFO_API)
            (void)EthIf_SetControllerMode(ctrlIdx, ETHIF_MODE_DOWN);
#endif

            EthSM_TransitionToState(networkIdx, ETHSM_STATE_NO_COM);
            netState->requestedComMode = COMM_NO_COMMUNICATION;
        }
    }
}

/**
 * @brief Processes ONHOLD state
 */
static void EthSM_ProcessState_ONHOLD(uint8 networkIdx)
{
    const EthSM_NetworkStateType* netState;
    uint8 ctrlIdx = 0U;

    netState = &EthSM_NetworkState[networkIdx];

    /* Check if No Communication is requested */
    if (netState->requestedComMode == COMM_NO_COMMUNICATION)
    {
        /* Shut down controller */
        if (networkIdx == ETHSM_NETWORK_0)
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_0;
        }
        else
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_1;
        }

#if defined(ETHIF_VERSION_INFO_API)
        (void)EthIf_SetControllerMode(ctrlIdx, ETHIF_MODE_DOWN);
#endif

        /* Transition back to NO_COM */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_NO_COM);
        return;
    }

    /* Check TcpIp state */
    if (netState->tcpIpState == TCPIP_STATE_ONLINE)
    {
        /* TcpIp is back online, transition to COM_READY */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_COM_READY);
    }
    else if (netState->tcpIpState == TCPIP_STATE_OFFLINE)
    {
        /* TcpIp went offline, go back to WAIT_TRCVLINK */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_WAIT_TRCVLINK);
    }
    /* else: stay in ONHOLD */
}

/**
 * @brief Processes COM_READY state
 */
static void EthSM_ProcessState_COM_READY(uint8 networkIdx)
{
    EthSM_NetworkStateType* netState;
    boolean linkUp;
    uint8 ctrlIdx = 0U;

    netState = &EthSM_NetworkState[networkIdx];

    /* Check if No Communication is requested */
    if (netState->requestedComMode == COMM_NO_COMMUNICATION)
    {
        /* Shut down controller */
        if (networkIdx == ETHSM_NETWORK_0)
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_0;
        }
        else
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_1;
        }

#if defined(ETHIF_VERSION_INFO_API)
        (void)EthIf_SetControllerMode(ctrlIdx, ETHIF_MODE_DOWN);
#endif

        /* Transition back to NO_COM */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_NO_COM);
        return;
    }

    /* Check TcpIp state */
    if (netState->tcpIpState == TCPIP_STATE_ONHOLD)
    {
        /* TcpIp went on hold, transition to ONHOLD */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_ONHOLD);
    }
    else if (netState->tcpIpState == TCPIP_STATE_OFFLINE)
    {
        /* TcpIp went offline, go back to WAIT_TRCVLINK */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_WAIT_TRCVLINK);
    }

    /* Monitor link state (optional - for robustness) */
    linkUp = EthSM_CheckLinkState(networkIdx);
    if (linkUp == FALSE)
    {
        /* Link went down, go back to WAIT_TRCVLINK */
        EthSM_TransitionToState(networkIdx, ETHSM_STATE_WAIT_TRCVLINK);
    }

    netState->linkStateUp = linkUp;
}

/*==================================================================================================
*                                    GLOBAL FUNCTION IMPLEMENTATIONS
==================================================================================================*/

/**
 * @brief Initializes the Ethernet State Manager
 */
/** @req SWS_EthSM_00001 */
void EthSM_Init(const EthSM_ConfigType* ConfigPtr)
{
    uint8 idx;

    /* Check if already initialized */
    if (EthSM_InitState == ETHSM_INITIALIZED)
    {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_INIT, ETHSM_E_ALREADY_INITIALIZED);
#endif
        return;
    }

    /* Store configuration pointer */
    EthSM_ConfigPtr = ConfigPtr;

    /* Initialize all network states */
    for (idx = 0U; idx < ETHSM_MAX_NETWORKS; idx++)
    {
        EthSM_NetworkState[idx].currentState = ETHSM_STATE_NO_COM;
        EthSM_NetworkState[idx].requestedComMode = COMM_NO_COMMUNICATION;
        EthSM_NetworkState[idx].currentComMode = COMM_NO_COMMUNICATION;
        EthSM_NetworkState[idx].tcpIpState = TCPIP_STATE_OFFLINE;
        EthSM_NetworkState[idx].timeoutCounter = 0U;
        EthSM_NetworkState[idx].retryCounter = 0U;
        EthSM_NetworkState[idx].linkStateUp = FALSE;
        EthSM_NetworkState[idx].initDone = TRUE;
    }

    /* Mark as initialized */
    EthSM_InitState = ETHSM_INITIALIZED;
}

/**
 * @brief Deinitializes the Ethernet State Manager
 */
/** @req SWS_EthSM_00002 */
void EthSM_DeInit(void)
{
    uint8 idx;
    uint8 ctrlIdx = 0U;

    /* Check if initialized */
    if (EthSM_InitState != ETHSM_INITIALIZED)
    {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_DEINIT, ETHSM_E_NOT_INITIALIZED);
#endif
        return;
    }

    /* Shutdown all networks */
    for (idx = 0U; idx < ETHSM_MAX_NETWORKS; idx++)
    {
        /* Shut down controller */
        if (idx == ETHSM_NETWORK_0)
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_0;
        }
        else
        {
            ctrlIdx = ETHSM_CTRL_IDX_NETWORK_1;
        }

#if defined(ETHIF_VERSION_INFO_API)
        (void)EthIf_SetControllerMode(ctrlIdx, ETHIF_MODE_DOWN);
#endif

        /* Reset state */
        EthSM_NetworkState[idx].currentState = ETHSM_STATE_UNINIT;
        EthSM_NetworkState[idx].requestedComMode = COMM_NO_COMMUNICATION;
        EthSM_NetworkState[idx].currentComMode = COMM_NO_COMMUNICATION;
        EthSM_NetworkState[idx].initDone = FALSE;
    }

    /* Clear configuration pointer */
    EthSM_ConfigPtr = NULL_PTR;

    /* Mark as uninitialized */
    EthSM_InitState = ETHSM_NOT_INITIALIZED;
}

/**
 * @brief Returns version information
 */
#if (ETHSM_VERSION_INFO_API == STD_ON)
/** @req SWS_EthSM_00003 */
void EthSM_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    if (VersionInfo == NULL_PTR)
    {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_GETVERSIONINFO, ETHSM_E_INVALID_POINTER);
#endif
        return;
    }

    VersionInfo->vendorID = ETHSM_VENDOR_ID;
    VersionInfo->moduleID = ETHSM_MODULE_ID;
    VersionInfo->sw_major_version = ETHSM_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = ETHSM_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = ETHSM_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Requests a communication mode change
 */
/** @req SWS_EthSM_00004 */
Std_ReturnType EthSM_RequestComMode(
    EthSM_NetworkHandleType NetworkHandle,
    ComM_ModeType ComMode)
{
    uint8 networkIdx;
    Std_ReturnType result = E_NOT_OK;

    /* Check initialization */
    if (EthSM_InitState != ETHSM_INITIALIZED)
    {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_REQUESTCOMMODE, ETHSM_E_NOT_INITIALIZED);
#endif
        return E_NOT_OK;
    }

    /* Validate network handle */
    if (EthSM_IsValidNetwork(NetworkHandle) == FALSE)
    {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_REQUESTCOMMODE, ETHSM_E_INVALID_NETWORK_HANDLE);
#endif
        return E_NOT_OK;
    }

    /* Validate mode */
    if ((ComMode != COMM_NO_COMMUNICATION) &&
        (ComMode != COMM_SILENT_COMMUNICATION) &&
        (ComMode != COMM_FULL_COMMUNICATION))
    {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_REQUESTCOMMODE, ETHSM_E_INVALID_PARAMETER);
#endif
        return E_NOT_OK;
    }

    networkIdx = EthSM_GetNetworkIndex(NetworkHandle);

    /* Store requested mode */
    EthSM_NetworkState[networkIdx].requestedComMode = ComMode;

    /* Reset retry counter on new request */
    EthSM_NetworkState[networkIdx].retryCounter = 0U;

    result = E_OK;

    return result;
}

/**
 * @brief Gets the current communication mode
 */
/** @req SWS_EthSM_00005 */
Std_ReturnType EthSM_GetCurrentComMode(
    EthSM_NetworkHandleType NetworkHandle,
    ComM_ModeType* ComMode)
{
    uint8 networkIdx;

    /* Check initialization */
    if (EthSM_InitState != ETHSM_INITIALIZED)
    {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_GETCURRENTCOMMODE, ETHSM_E_NOT_INITIALIZED);
#endif
        return E_NOT_OK;
    }

    /* Validate network handle */
    if (EthSM_IsValidNetwork(NetworkHandle) == FALSE)
    {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_GETCURRENTCOMMODE, ETHSM_E_INVALID_NETWORK_HANDLE);
#endif
        return E_NOT_OK;
    }

    /* Validate pointer */
    if (ComMode == NULL_PTR)
    {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_GETCURRENTCOMMODE, ETHSM_E_INVALID_POINTER);
#endif
        return E_NOT_OK;
    }

    networkIdx = EthSM_GetNetworkIndex(NetworkHandle);

    /* Return current mode */
    *ComMode = EthSM_NetworkState[networkIdx].currentComMode;

    return E_OK;
}

/**
 * @brief TcpIp mode indication callback
 */
/** @req SWS_EthSM_00006 */
void EthSM_TcpIpModeIndication(
    EthSM_NetworkHandleType NetworkHandle,
    TcpIp_StateType TcpIpMode)
{
    uint8 networkIdx;

    /* Check initialization */
    if (EthSM_InitState != ETHSM_INITIALIZED)
    {
        return; /* Silent fail for callbacks */
    }

    /* Validate network handle */
    if (EthSM_IsValidNetwork(NetworkHandle) == FALSE)
    {
        return;
    }

    /* Validate TcpIp mode */
    if (TcpIpMode > TCPIP_STATE_ONHOLD)
    {
        return;
    }

    networkIdx = EthSM_GetNetworkIndex(NetworkHandle);

    /* Update TcpIp state */
    EthSM_NetworkState[networkIdx].tcpIpState = TcpIpMode;
}

/**
 * @brief Main function for cyclic processing
 */
/** @req SWS_EthSM_00007 */
void EthSM_MainFunction(void)
{
    uint8 idx;

    /* Check initialization */
    if (EthSM_InitState != ETHSM_INITIALIZED)
    {
        return;
    }

    /* Process all networks */
    for (idx = 0U; idx < ETHSM_MAX_NETWORKS; idx++)
    {
        /* Process based on current state */
        switch (EthSM_NetworkState[idx].currentState)
        {
            case ETHSM_STATE_NO_COM:
                EthSM_ProcessState_NO_COM(idx);
                break;

            case ETHSM_STATE_WAIT_TRCVLINK:
                EthSM_ProcessState_WAIT_TRCVLINK(idx);
                break;

            case ETHSM_STATE_WAIT_ONLINE:
                EthSM_ProcessState_WAIT_ONLINE(idx);
                break;

            case ETHSM_STATE_ONHOLD:
                EthSM_ProcessState_ONHOLD(idx);
                break;

            case ETHSM_STATE_COM_READY:
                EthSM_ProcessState_COM_READY(idx);
                break;

            default:
                /* Invalid state - reset to NO_COM */
                EthSM_NetworkState[idx].currentState = ETHSM_STATE_NO_COM;
                break;
        }
    }
}

/**
 * @brief Gets internal state (for debugging)
 */
/** @req SWS_EthSM_00101 */
EthSM_StateType EthSM_GetInternalState(EthSM_NetworkHandleType NetworkHandle)
{
    uint8 networkIdx;

    if (EthSM_InitState != ETHSM_INITIALIZED)
    {
        return ETHSM_STATE_UNINIT;
    }

    if (EthSM_IsValidNetwork(NetworkHandle) == FALSE)
    {
        return ETHSM_STATE_UNINIT;
    }

    networkIdx = EthSM_GetNetworkIndex(NetworkHandle);

    return EthSM_NetworkState[networkIdx].currentState;
}
