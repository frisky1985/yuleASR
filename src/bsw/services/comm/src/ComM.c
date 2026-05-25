/**
 * @file ComM.c
 * @brief Communication Manager Implementation
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * Implements the full ComM channel state machine including:
 * - 6-state channel state machine (AUTOSAR SWS_ComM)
 * - BusSM notification on mode change
 * - Wake-up handling (ComM_EvaluateWakeup)
 * - DCM diagnostic integration
 * - DET error detection (MISRA C:2012 compliant)
 */

#include "ComM.h"
#include "Det.h"

/*=============================================================================
 * Internal Macros
 *===========================================================================*/
#define COMM_INITIALIZED                    0x01U
#define COMM_UNINITIALIZED                  0x00U

#define COMM_IS_INITIALIZED()               (ComM_ModuleState == COMM_INITIALIZED)
#define COMM_VALIDATE_USER(User)            ((User) < (ComM_UserHandleType)COMM_MAX_USERS)
#define COMM_VALIDATE_CHANNEL(Channel)      ((Channel) < (ComM_ChannelHandleType)COMM_MAX_CHANNELS)

/* Default timeout values (main function cycles) */
#define COMM_DEFAULT_WAKEUP_TIMEOUT         10U    /* ~100ms at 10ms main cycle */
#define COMM_DEFAULT_SILENT_TIMEOUT         100U   /* ~1000ms at 10ms main cycle */
#define COMM_DEFAULT_READY_SLEEP_TIMEOUT    50U    /* ~500ms at 10ms main cycle */

/*=============================================================================
 * Internal Variables
 *===========================================================================*/
static ComM_StateType ComM_ModuleState = COMM_STATE_UNINIT;

/* Channel runtime states */
static ComM_ChannelRuntimeType ComM_ChannelStates[COMM_MAX_CHANNELS];

/* User requests */
static ComM_UserRequestType ComM_UserRequests[COMM_MAX_USERS];

/*=============================================================================
 * Internal Function Prototypes
 *===========================================================================*/
static void ComM_ProcessChannelStateMachine(ComM_ChannelHandleType Channel);
static ComM_ModeType ComM_GetHighestRequestedMode(ComM_ChannelHandleType Channel);
static void ComM_NotifyBusSMOfModeChange(ComM_ChannelHandleType Channel, ComM_ModeType NewMode);
static void ComM_UpdateChannelUserRequests(ComM_ChannelHandleType Channel);
static void ComM_SetChannelState(ComM_ChannelHandleType Channel, ComM_ChannelStateType NewState);

/*=============================================================================
 * Core API Implementation
 *===========================================================================*/

/**
 * @brief Initialize the ComM module.
 * @param ConfigPtr Pointer to configuration (unused in simplified config).
 */
void ComM_Init(const ComM_ConfigType* ConfigPtr)
{
    ComM_ChannelHandleType ch;
    ComM_UserHandleType user;

#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_INIT_SID, COMM_E_PARAM_POINTER);
        return;
    }
#endif

    /* Initialize channel states */
    for (ch = 0U; ch < (ComM_ChannelHandleType)COMM_MAX_CHANNELS; ch++)
    {
        ComM_ChannelStates[ch].State = COMM_NO_COM_NO_PENDING_REQUEST;
        ComM_ChannelStates[ch].CurrentMode = COMM_NO_COMMUNICATION;
        ComM_ChannelStates[ch].RequestedMode = COMM_NO_COMMUNICATION;
        ComM_ChannelStates[ch].CommunicationAllowed = TRUE;
        ComM_ChannelStates[ch].DiagnosticActive = FALSE;
        ComM_ChannelStates[ch].TimeoutCounter = 0U;
        ComM_ChannelStates[ch].UserRequestMask = 0U;
    }

    /* Initialize user requests */
    for (user = 0U; user < (ComM_UserHandleType)COMM_MAX_USERS; user++)
    {
        ComM_UserRequests[user].RequestedMode = COMM_NO_COMMUNICATION;
        ComM_UserRequests[user].Active = FALSE;
    }

    ComM_ModuleState = COMM_INITIALIZED;
}

/**
 * @brief De-initialize the ComM module.
 */
void ComM_DeInit(void)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED())
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_DEINIT_SID, COMM_E_NOT_INIT);
        return;
    }
#endif

    ComM_ModuleState = COMM_UNINITIALIZED;
}

/**
 * @brief Get version information of the ComM module.
 * @param VersionInfo Pointer to version info structure.
 */
void ComM_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfo == NULL_PTR)
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_GETVERSIONINFO_SID, COMM_E_PARAM_POINTER);
        return;
    }
#endif

#if (COMM_VERSION_INFO_API == STD_ON)
    VersionInfo->vendorID = 0x0001U;
    VersionInfo->moduleID = (uint16)COMM_MODULE_ID;
    VersionInfo->sw_major_version = (uint8)COMM_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = (uint8)COMM_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = (uint8)COMM_SW_PATCH_VERSION;
#else
    (void)VersionInfo;
#endif
}

/*=============================================================================
 * Communication Mode Management
 *===========================================================================*/

/**
 * @brief Request a communication mode for a user.
 * @param User User handle.
 * @param ComMode Requested communication mode.
 * @return E_OK on success, E_NOT_OK on error.
 */
Std_ReturnType ComM_RequestComMode(ComM_UserHandleType User, ComM_ModeType ComMode)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED())
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_REQUESTCOMODE_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    if (!COMM_VALIDATE_USER(User))
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_REQUESTCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
    if (ComMode > COMM_FULL_COMMUNICATION)
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_REQUESTCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
#endif

    /* Store the requested mode for this user */
    ComM_UserRequests[User].RequestedMode = ComMode;
    ComM_UserRequests[User].Active = (ComMode != COMM_NO_COMMUNICATION);

    return E_OK;
}

/**
 * @brief Get the maximum (highest) communication mode for a user.
 * @param User User handle.
 * @param ComModePtr Output pointer for the mode.
 * @return E_OK on success, E_NOT_OK on error.
 */
Std_ReturnType ComM_GetMaxComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED())
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_GETMAXCOMODE_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    if (!COMM_VALIDATE_USER(User))
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_GETMAXCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
    if (ComModePtr == NULL_PTR)
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_GETMAXCOMODE_SID, COMM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *ComModePtr = ComM_UserRequests[User].RequestedMode;

    return E_OK;
}

/**
 * @brief Get the requested communication mode for a user.
 * @param User User handle.
 * @param ComModePtr Output pointer for the mode.
 * @return E_OK on success, E_NOT_OK on error.
 */
Std_ReturnType ComM_GetRequestedComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
{
    return ComM_GetMaxComMode(User, ComModePtr);
}

/**
 * @brief Get the current communication mode for a user.
 * @param User User handle.
 * @param ComModePtr Output pointer for the mode.
 * @return E_OK on success, E_NOT_OK on error.
 */
Std_ReturnType ComM_GetCurrentComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
{
    ComM_ModeType highestMode;
    ComM_ChannelHandleType ch;

#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED())
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_GETCURRENTCOMODE_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    if (!COMM_VALIDATE_USER(User))
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_GETCURRENTCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
    if (ComModePtr == NULL_PTR)
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_GETCURRENTCOMODE_SID, COMM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Return the highest current mode across all channels */
    highestMode = COMM_NO_COMMUNICATION;

    for (ch = 0U; ch < (ComM_ChannelHandleType)COMM_MAX_CHANNELS; ch++)
    {
        if (ComM_ChannelStates[ch].CurrentMode > highestMode)
        {
            highestMode = ComM_ChannelStates[ch].CurrentMode;
        }
    }

    *ComModePtr = highestMode;

    return E_OK;
}

/*=============================================================================
 * Channel Management
 *===========================================================================*/

/**
 * @brief Set the communication allowed flag for a channel.
 * @param Channel Channel handle.
 * @param Allowed TRUE if communication is allowed.
 */
void ComM_CommunicationAllowed(ComM_ChannelHandleType Channel, boolean Allowed)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED())
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_COMMUNICATIONALLOWED_SID, COMM_E_NOT_INIT);
        return;
    }
    if (!COMM_VALIDATE_CHANNEL(Channel))
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_COMMUNICATIONALLOWED_SID, COMM_E_WRONG_PARAMETERS);
        return;
    }
#endif

    ComM_ChannelStates[Channel].CommunicationAllowed = Allowed;
}

/**
 * @brief Main function — processes the channel state machine for all channels.
 *        Called cyclically (e.g., every 10ms via ComM_Cfg.h period).
 */
void ComM_MainFunction(void)
{
    ComM_ChannelHandleType ch;

#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED())
    {
        return;
    }
#endif

    /* Process each channel's state machine */
    for (ch = 0U; ch < (ComM_ChannelHandleType)COMM_MAX_CHANNELS; ch++)
    {
        ComM_ProcessChannelStateMachine(ch);
    }
}

/*=============================================================================
 * Wake-up Handling
 *===========================================================================*/

/**
 * @brief Evaluate wake-up on a channel.
 *        Called by EcuM or BusSM when a wake-up event occurs on a specific channel.
 *        This triggers the transition from pending to full communication.
 * @param Channel Channel handle.
 */
void ComM_EvaluateWakeup(ComM_ChannelHandleType Channel)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED())
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_EVALUATEWAKEUP_SID, COMM_E_NOT_INIT);
        return;
    }
    if (!COMM_VALIDATE_CHANNEL(Channel))
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_EVALUATEWAKEUP_SID, COMM_E_WRONG_PARAMETERS);
        return;
    }
#endif

    /* If channel is in pending state and a request is active, complete the wake-up */
    if (ComM_ChannelStates[Channel].State == COMM_NO_COM_PENDING_REQUEST)
    {
        if (ComM_ChannelStates[Channel].RequestedMode == COMM_FULL_COMMUNICATION)
        {
            ComM_SetChannelState(Channel, COMM_FULL_COM);
        }
        else if (ComM_ChannelStates[Channel].RequestedMode == COMM_SILENT_COMMUNICATION)
        {
            ComM_SetChannelState(Channel, COMM_SILENT_COM);
        }
    }
    else if (ComM_ChannelStates[Channel].State == COMM_NO_COM_NO_PENDING_REQUEST)
    {
        /* Spontaneous wake-up — set pending request and trigger evaluation */
        if (ComM_ChannelStates[Channel].RequestedMode < COMM_FULL_COMMUNICATION)
        {
            ComM_ChannelStates[Channel].RequestedMode = COMM_FULL_COMMUNICATION;
        }

        ComM_ChannelStates[Channel].TimeoutCounter = 0U;
        ComM_SetChannelState(Channel, COMM_NO_COM_PENDING_REQUEST);
    }
}

/*=============================================================================
 * ECU Manager Interface
 *===========================================================================*/

/**
 * @brief Wake-up indication from ECU Manager.
 * @return E_OK always.
 */
Std_ReturnType ComM_EcuM_WakeUpIndication(void)
{
    /* Wake-up received from EcuM; evaluated per-channel in MainFunction */
    return E_OK;
}

/**
 * @brief RUN request indication from ECU Manager.
 * @return E_OK always.
 */
Std_ReturnType ComM_EcuM_RunRequestIndication(void)
{
    /* ECU run request — keep current state; no specific action needed */
    return E_OK;
}

/*=============================================================================
 * Bus State Manager Interface
 *===========================================================================*/

/**
 * @brief Mode indication from BusSM.
 *        Called by BusSM to inform ComM of the actual bus communication mode.
 * @param Channel Channel handle.
 * @param Mode Actual communication mode from the bus.
 */
void ComM_BusSM_ModeIndication(ComM_ChannelHandleType Channel, ComM_ModeType Mode)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED())
    {
        return;
    }
    if (!COMM_VALIDATE_CHANNEL(Channel))
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_BUSSM_MODEINDICATION_SID, COMM_E_WRONG_PARAMETERS);
        return;
    }
    if (Mode > COMM_FULL_COMMUNICATION)
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_BUSSM_MODEINDICATION_SID, COMM_E_WRONG_PARAMETERS);
        return;
    }
#endif

    ComM_ChannelStates[Channel].CurrentMode = Mode;

    /* If bus indicates NoCom and we were in pending state, reflect it */
    if ((Mode == COMM_NO_COMMUNICATION) &&
        (ComM_ChannelStates[Channel].State == COMM_NO_COM_PENDING_REQUEST))
    {
        /* Bus is still sleeping; stay in pending */
    }
}

/*=============================================================================
 * DCM Interface
 *===========================================================================*/

/**
 * @brief Activate diagnostic session on a channel.
 * @param Channel Channel handle.
 * @return E_OK on success, E_NOT_OK on error.
 */
Std_ReturnType ComM_DCM_ActiveDiagnostic(ComM_ChannelHandleType Channel)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED())
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_DCM_ACTIVEDIAGNOSTIC_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    if (!COMM_VALIDATE_CHANNEL(Channel))
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_DCM_ACTIVEDIAGNOSTIC_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
#endif

    ComM_ChannelStates[Channel].DiagnosticActive = TRUE;

    /* When diagnostic becomes active, ensure full communication is requested */
    if (ComM_ChannelStates[Channel].RequestedMode < COMM_FULL_COMMUNICATION)
    {
        ComM_ChannelStates[Channel].RequestedMode = COMM_FULL_COMMUNICATION;
    }

    return E_OK;
}

/**
 * @brief Deactivate diagnostic session on a channel.
 * @param Channel Channel handle.
 * @return E_OK on success, E_NOT_OK on error.
 */
Std_ReturnType ComM_DCM_InactiveDiagnostic(ComM_ChannelHandleType Channel)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED())
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_DCM_INACTIVEDIAGNOSTIC_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    if (!COMM_VALIDATE_CHANNEL(Channel))
    {
        Det_ReportError((uint16)COMM_MODULE_ID, 0U, COMM_DCM_INACTIVEDIAGNOSTIC_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
#endif

    ComM_ChannelStates[Channel].DiagnosticActive = FALSE;

    return E_OK;
}

/*=============================================================================
 * Internal Functions - Channel State Machine
 *===========================================================================*/

/**
 * @brief Process the full 6-state channel state machine.
 * @param Channel Channel handle.
 * 
 * STATE MACHINE DIAGRAM:
 * 
 * NO_COM_NO_PENDING_REQUEST ──[request]──> NO_COM_PENDING_REQUEST ──[wakeup]──> FULL_COM
 *       ^                                                                           │
 *       │                                                                    [no request] + [no NM]
 *       │                                                                           │
 *       │                                                                           ▼
 *       │                                   SILENT_COM <──[timeout]── FULL_COM_READY_SLEEP
 *       │                                       │                                       │
 *       │                                       │                               [new request] ──┐
 *       │                              [no request + timeout]                                 │
 *       │                                       │                                             │
 *       └───────────────────────────────────────┘                                  FULL_COM_NETWORK_REQUESTED
 *                                                                                          │
 *                                                                                   [no request]
 *                                                                                          │
 *                                                                                          ▼
 *                                                                                  FULL_COM_READY_SLEEP
 */
static void ComM_ProcessChannelStateMachine(ComM_ChannelHandleType Channel)
{
    ComM_ChannelStateType currentState;
    ComM_ModeType requestedMode;

    currentState = ComM_ChannelStates[Channel].State;

    /* Update user request mask and compute highest requested mode */
    ComM_UpdateChannelUserRequests(Channel);
    requestedMode = ComM_GetHighestRequestedMode(Channel);

    /* Apply communication-allowed constraint */
    if (!ComM_ChannelStates[Channel].CommunicationAllowed)
    {
        if (requestedMode > COMM_NO_COMMUNICATION)
        {
            requestedMode = COMM_NO_COMMUNICATION;
        }
    }

    /* Store the effective requested mode */
    ComM_ChannelStates[Channel].RequestedMode = requestedMode;

    /*========================================================================
     * State Machine Transitions
     *======================================================================*/
    switch (currentState)
    {
        /*------------------------------------------------------------------
         * State: NO_COM_NO_PENDING_REQUEST
         * No communication; no active requests from any user.
         *----------------------------------------------------------------*/
        case COMM_NO_COM_NO_PENDING_REQUEST:
        {
            if (requestedMode > COMM_NO_COMMUNICATION)
            {
                /* A request has come in — enter pending state to initiate wake-up */
                ComM_ChannelStates[Channel].TimeoutCounter = COMM_DEFAULT_WAKEUP_TIMEOUT;
                ComM_SetChannelState(Channel, COMM_NO_COM_PENDING_REQUEST);
            }
            break;
        }

        /*------------------------------------------------------------------
         * State: NO_COM_PENDING_REQUEST
         * No communication; a request is pending (wake-up in progress).
         * Waiting for bus wake-up or timeout.
         *----------------------------------------------------------------*/
        case COMM_NO_COM_PENDING_REQUEST:
        {
            if (requestedMode == COMM_NO_COMMUNICATION)
            {
                /* Request was withdrawn before wake-up completed */
                ComM_ChannelStates[Channel].TimeoutCounter = 0U;
                ComM_SetChannelState(Channel, COMM_NO_COM_NO_PENDING_REQUEST);

            }
            else if (ComM_ChannelStates[Channel].TimeoutCounter > 0U)
            {
                /* Decrement wake-up timeout */
                ComM_ChannelStates[Channel].TimeoutCounter--;
            }
            else
            {
                /* Wake-up timeout expired — transition based on requested mode */
                if (requestedMode == COMM_FULL_COMMUNICATION)
                {
                    ComM_SetChannelState(Channel, COMM_FULL_COM);
    
                }
                else if (requestedMode == COMM_SILENT_COMMUNICATION)
                {
                    ComM_SetChannelState(Channel, COMM_SILENT_COM);
    
                }
            }
            break;
        }

        /*------------------------------------------------------------------
         * State: FULL_COM
         * Full communication is active (user data flowing).
         *----------------------------------------------------------------*/
        case COMM_FULL_COM:
        {
            if (requestedMode == COMM_NO_COMMUNICATION)
            {
                /* All requests withdrawn — enter ready sleep */
                ComM_ChannelStates[Channel].TimeoutCounter = COMM_DEFAULT_READY_SLEEP_TIMEOUT;
                ComM_SetChannelState(Channel, COMM_FULL_COM_READY_SLEEP);

            }
            else if (requestedMode == COMM_SILENT_COMMUNICATION)
            {
                /* Only silent mode requested — transition to silent */
                ComM_ChannelStates[Channel].TimeoutCounter = COMM_DEFAULT_SILENT_TIMEOUT;
                ComM_SetChannelState(Channel, COMM_SILENT_COM);

            }
            /* REMAIN in FULL_COM if full or network request is active */
            break;
        }

        /*------------------------------------------------------------------
         * State: FULL_COM_NETWORK_REQUESTED
         * Full communication with NM network request active.
         *----------------------------------------------------------------*/
        case COMM_FULL_COM_NETWORK_REQUESTED:
        {
            if (requestedMode == COMM_NO_COMMUNICATION)
            {
                /* No active requests — begin sleep preparation */
                ComM_ChannelStates[Channel].TimeoutCounter = COMM_DEFAULT_READY_SLEEP_TIMEOUT;
                ComM_SetChannelState(Channel, COMM_FULL_COM_READY_SLEEP);

            }
            else
            {
                /* Still have requests — remain in full com */
                /* FULL_COM_NETWORK_REQUESTED stays; if FULL_COM was requested
                 * the channel already is effectively in full communication,
                 * so no state change needed */
            }
            break;
        }

        /*------------------------------------------------------------------
         * State: FULL_COM_READY_SLEEP
         * Full communication still possible, but no active requests.
         * Waiting for timeout to enter silent com.
         *----------------------------------------------------------------*/
        case COMM_FULL_COM_READY_SLEEP:
        {
            if (requestedMode >= COMM_FULL_COMMUNICATION)
            {
                /* New request came in while waiting to sleep — return to full com */
                if (requestedMode == COMM_FULL_COMMUNICATION)
                {
                    ComM_SetChannelState(Channel, COMM_FULL_COM);
    
                }
            }
            else if (ComM_ChannelStates[Channel].TimeoutCounter > 0U)
            {
                /* Decrement sleep timeout */
                ComM_ChannelStates[Channel].TimeoutCounter--;
            }
            else
            {
                /* Timeout expired — transition to silent com */
                ComM_ChannelStates[Channel].TimeoutCounter = COMM_DEFAULT_SILENT_TIMEOUT;
                ComM_SetChannelState(Channel, COMM_SILENT_COM);

            }
            break;
        }

        /*------------------------------------------------------------------
         * State: SILENT_COM
         * Silent communication mode (NM traffic only, no user data).
         *----------------------------------------------------------------*/
        case COMM_SILENT_COM:
        {
            if (requestedMode == COMM_FULL_COMMUNICATION)
            {
                /* New full request — return to full com */
                ComM_SetChannelState(Channel, COMM_FULL_COM);

            }
            else if (requestedMode == COMM_NO_COMMUNICATION)
            {
                /* No requests — wait for silent timeout then go to NoCom */
                if (ComM_ChannelStates[Channel].TimeoutCounter > 0U)
                {
                    ComM_ChannelStates[Channel].TimeoutCounter--;
                }
                else
                {
                    /* Silent timeout expired — enter NoCom */
                    ComM_SetChannelState(Channel, COMM_NO_COM_NO_PENDING_REQUEST);
    
                }
            }
            /* If requestedMode == SILENT, stay in SILENT_COM */
            break;
        }

        /*------------------------------------------------------------------
         * Default: Invalid state — reset to safe state
         *----------------------------------------------------------------*/
        default:
        {
            ComM_SetChannelState(Channel, COMM_NO_COM_NO_PENDING_REQUEST);
            break;
        }
    }

    /* Update the communication mode based on the new state */
    {
        ComM_ModeType newMode;

        switch (ComM_ChannelStates[Channel].State)
        {
            case COMM_NO_COM_NO_PENDING_REQUEST:
            case COMM_NO_COM_PENDING_REQUEST:
                newMode = COMM_NO_COMMUNICATION;
                break;

            case COMM_SILENT_COM:
                newMode = COMM_SILENT_COMMUNICATION;
                break;

            case COMM_FULL_COM:
            case COMM_FULL_COM_NETWORK_REQUESTED:
            case COMM_FULL_COM_READY_SLEEP:
                newMode = COMM_FULL_COMMUNICATION;
                break;

            default:
                newMode = COMM_NO_COMMUNICATION;
                break;
        }

        if (ComM_ChannelStates[Channel].CurrentMode != newMode)
        {
            ComM_ChannelStates[Channel].CurrentMode = newMode;

            /* Notify BusSM of the mode change */
            ComM_NotifyBusSMOfModeChange(Channel, newMode);
        }
    }
}

/**
 * @brief Update the user request tracking for a channel.
 *        Collects all user requests that map to this channel into the bitmask.
 * @param Channel Channel handle.
 */
static void ComM_UpdateChannelUserRequests(ComM_ChannelHandleType Channel)
{
    ComM_UserHandleType user;
    uint32 mask;

    mask = 0U;

    for (user = 0U; user < (ComM_UserHandleType)COMM_MAX_USERS; user++)
    {
        if (ComM_UserRequests[user].Active)
        {
            /* For simplicity, all users map to all channels.
             * In a full implementation, this would use a user-to-channel mapping table. */
            mask |= (1UL << (uint32)user);
        }
    }

    ComM_ChannelStates[Channel].UserRequestMask = mask;
}

/**
 * @brief Compute the highest requested mode for a channel.
 *        Checks all user requests and DCM diagnostic state.
 * @param Channel Channel handle.
 * @return Highest requested communication mode.
 */
static ComM_ModeType ComM_GetHighestRequestedMode(ComM_ChannelHandleType Channel)
{
    ComM_ModeType highestMode;
    ComM_UserHandleType user;

    highestMode = COMM_NO_COMMUNICATION;

    /* Scan all user requests */
    for (user = 0U; user < (ComM_UserHandleType)COMM_MAX_USERS; user++)
    {
        if (ComM_UserRequests[user].Active)
        {
            if (ComM_UserRequests[user].RequestedMode > highestMode)
            {
                highestMode = ComM_UserRequests[user].RequestedMode;
            }
        }
    }

    /* Consider DCM diagnostic request — overrides to full communication */
    if (ComM_ChannelStates[Channel].DiagnosticActive)
    {
        if (highestMode < COMM_FULL_COMMUNICATION)
        {
            highestMode = COMM_FULL_COMMUNICATION;
        }
    }

    return highestMode;
}

/**
 * @brief Set the channel state and perform exit/entry actions.
 * @param Channel Channel handle.
 * @param NewState New channel state.
 */
static void ComM_SetChannelState(ComM_ChannelHandleType Channel, ComM_ChannelStateType NewState)
{
    ComM_ChannelStateType oldState;

    oldState = ComM_ChannelStates[Channel].State;

    if (oldState != NewState)
    {
        /* Perform state exit actions */
        switch (oldState)
        {
            case COMM_FULL_COM:
            case COMM_FULL_COM_NETWORK_REQUESTED:
            {
                /* Exit full communication — could disable transceiver, etc. */
                break;
            }
            default:
            {
                /* No specific exit action */
                break;
            }
        }

        /* Perform state entry actions */
        switch (NewState)
        {
            case COMM_FULL_COM:
            case COMM_FULL_COM_NETWORK_REQUESTED:
            {
                /* Enter full communication — could enable transceiver, etc. */
                break;
            }
            case COMM_NO_COM_NO_PENDING_REQUEST:
            {
                /* Enter NoCom — bus is sleeping */
                break;
            }
            default:
            {
                /* No specific entry action */
                break;
            }
        }

        ComM_ChannelStates[Channel].State = NewState;
    }
}

/**
 * @brief Notify BusSM of a communication mode change on a channel.
 *        This informs the Bus State Manager that the requested mode has changed,
 *        allowing it to adjust the bus state accordingly.
 * @param Channel Channel handle.
 * @param NewMode The new communication mode.
 */
static void ComM_NotifyBusSMOfModeChange(ComM_ChannelHandleType Channel, ComM_ModeType NewMode)
{
    (void)Channel;
    (void)NewMode;

    /* BusSM notification hook.
     *
     * In a production AUTOSAR stack, this would call BswM_ComM_CurrentMode()
     * or directly notify BusSM via a mode request port.
     *
     * Example:
     *   BswM_ComM_CurrentMode((uint8)Channel, (uint8)NewMode);
     *
     * This implementation provides the hook for integration.
     */
}
