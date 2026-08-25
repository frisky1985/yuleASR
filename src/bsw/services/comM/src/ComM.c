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

/**
 * @file ComM.c
 * @brief AUTOSAR Communication Manager Core Implementation
 * @version 1.0.0
 * 
 * Core implementation of Communication Manager including:
 * - Communication mode management (NO_COM, SILENT_COM, FULL_COM)
 * - Channel state machine (NoCom, SilentCom, FullCom, Pending)
 * - User request management
 * - Partial Network Cluster (PNC) support
 * - Bus wake-up handling
 * - DCM passive mode support
 * - EcuM integration for wake-up
 */

#include "ComM.h"
#include "ComM_Cfg.h"
#include "Det.h"

/*=============================================================================
 * Internal Macros
 *===========================================================================*/
#define COMM_INITIALIZED                    0x01U
#define COMM_UNINITIALIZED                  0x00

#define COMM_IS_INITIALIZED()               (ComM_ModuleState == COMM_INITIALIZED)

#define COMM_VALIDATE_USER(User)            ((User) < COMM_NUM_USERS)
#define COMM_VALIDATE_CHANNEL(Channel)      ((Channel) < COMM_NUM_CHANNELS)
#define COMM_VALIDATE_PNC(Pnc)              ((Pnc) < COMM_NUM_PNCS)

#define COMM_SET_CHANNEL_STATE(ch, st)      (ComM_ChannelStates[(ch)].State = (st))
#define COMM_GET_CHANNEL_STATE(ch)          (ComM_ChannelStates[(ch)].State)

/*=============================================================================
 * Internal Variables
 *===========================================================================*/
static uint8 ComM_ModuleState = COMM_UNINITIALIZED;
static const ComM_ConfigType* ComM_ConfigPtr = NULL_PTR;

/* Channel States */
static ComM_ChannelStateStrType ComM_ChannelStates[COMM_NUM_CHANNELS];

/* User Requests */
static ComM_UserRequestType ComM_UserRequests[COMM_NUM_USERS];

/* PNC States */
#if (COMM_PNC_SUPPORT == STD_ON)
static ComM_PncStateType ComM_PncStates[COMM_NUM_PNCS];
#endif

/* Inhibition Status */
static boolean ComM_EcuLimitToNoCom = FALSE;

/*=============================================================================
 * Internal Function Prototypes
 *===========================================================================*/
static void ComM_ProcessChannelStateMachine(ComM_ChannelHandleType Channel);
static void ComM_ProcessChannelTransitions(ComM_ChannelHandleType Channel);
static void ComM_UpdateChannelMode(ComM_ChannelHandleType Channel);
static ComM_ModeType ComM_GetHighestRequestedMode(ComM_ChannelHandleType Channel);
static void ComM_ExecuteChannelEntryAction(ComM_ChannelHandleType Channel);
static void ComM_ExecuteChannelExitAction(ComM_ChannelHandleType Channel);

#if (COMM_PNC_SUPPORT == STD_ON)
static void ComM_ProcessPncStateMachine(ComM_PncHandleType Pnc);
static void ComM_UpdatePncRequestStatus(ComM_PncHandleType Pnc);
static void ComM_HandlePncChannelRequests(ComM_PncHandleType Pnc);
#endif

#if (COMM_DCM_SUPPORT == STD_ON)
static void ComM_UpdateDcmChannelRequests(ComM_ChannelHandleType Channel);
#endif

/** @req SWS_ComM_00001 */
/*=============================================================================
 * Core API Implementation
 *===========================================================================*/
void ComM_Init(const ComM_ConfigType* ConfigPtr)
{
    ComM_ChannelHandleType ch;
    ComM_UserHandleType user;
    
#if (COMM_PNC_SUPPORT == STD_ON)
    ComM_PncHandleType pnc;
#endif

#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_INIT_SID, COMM_E_PARAM_POINTER);
        return;
    }
#endif

    ComM_ConfigPtr = ConfigPtr;

    /* Initialize channel states */
    for (ch = 0U; ch < COMM_NUM_CHANNELS; ch++) {
        ComM_ChannelStates[ch].State = COMM_CHANNEL_STATE_NOCOM;
        ComM_ChannelStates[ch].CurrentMode = COMM_NO_COMMUNICATION;
        ComM_ChannelStates[ch].RequestedMode = COMM_NO_COMMUNICATION;
        ComM_ChannelStates[ch].CommunicationAllowed = TRUE;
        ComM_ChannelStates[ch].WakeUpInhibition = FALSE;
        ComM_ChannelStates[ch].LimitToNoCom = FALSE;
        ComM_ChannelStates[ch].DcmActive = FALSE;
        ComM_ChannelStates[ch].PassiveDiagnostic = FALSE;
        ComM_ChannelStates[ch].TimeoutCounter = 0U;
        ComM_ChannelStates[ch].WakeUpRetryCounter = 0U;
        ComM_ChannelStates[ch].UserRequestCount = 0U;
    }

    /* Initialize user requests */
    for (user = 0U; user < COMM_NUM_USERS; user++) {
        ComM_UserRequests[user].RequestedMode = COMM_NO_COMMUNICATION;
        ComM_UserRequests[user].Active = FALSE;
    }

#if (COMM_PNC_SUPPORT == STD_ON)
    /* Initialize PNC states */
    for (pnc = 0U; pnc < COMM_NUM_PNCS; pnc++) {
        ComM_PncStates[pnc].Mode = COMM_PNC_NO_COMMUNICATION;
        ComM_PncStates[pnc].RequestActive = FALSE;
        ComM_PncStates[pnc].TimeoutCounter = 0U;
        ComM_PncStates[pnc].ActiveRequestCount = 0U;
    }
#endif

    ComM_EcuLimitToNoCom = FALSE;
    ComM_ModuleState = COMM_INITIALIZED;
}

/** @req SWS_ComM_00002 */
void ComM_DeInit(void)
{
    ComM_ChannelHandleType ch;

#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED()) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_DEINIT_SID, COMM_E_NOT_INIT);
        return;
    }
#endif

    /* Reset all channels to NoCom */
    for (ch = 0U; ch < COMM_NUM_CHANNELS; ch++) {
        ComM_ChannelStates[ch].State = COMM_CHANNEL_STATE_NOCOM;
        ComM_ChannelStates[ch].CurrentMode = COMM_NO_COMMUNICATION;
        ComM_ChannelStates[ch].RequestedMode = COMM_NO_COMMUNICATION;
    }

    ComM_ConfigPtr = NULL_PTR;
    ComM_ModuleState = COMM_UNINITIALIZED;
}

/** @req SWS_ComM_00003 */
void ComM_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfo == NULL_PTR) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_GETVERSIONINFO_SID, COMM_E_PARAM_POINTER);
        return;
    }
#endif

#if (COMM_VERSION_INFO_API == STD_ON)
    VersionInfo->vendorID = 0x00U;
    VersionInfo->moduleID = COMM_MODULE_ID;
    VersionInfo->sw_major_version = COMM_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = COMM_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = COMM_SW_PATCH_VERSION;
#else
    (void)VersionInfo;
#endif
}

/** @req SWS_ComM_00005 */
/*=============================================================================
 * Communication Mode Management
 *===========================================================================*/
Std_ReturnType ComM_RequestComMode(ComM_UserHandleType User, ComM_ModeType ComMode)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED()) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_REQUESTCOMMODE_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    
    if (!COMM_VALIDATE_USER(User)) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_REQUESTCOMMODE_SID, COMM_E_PARAM_USER);
        return E_NOT_OK;
    }
    
    if ((ComMode != COMM_NO_COMMUNICATION) && 
        (ComMode != COMM_SILENT_COMMUNICATION) && 
        (ComMode != COMM_FULL_COMMUNICATION)) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_REQUESTCOMMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
#endif

    ComM_UserRequests[User].RequestedMode = ComMode;
    ComM_UserRequests[User].Active = (ComMode != COMM_NO_COMMUNICATION);

    return E_OK;
}

/** @req SWS_ComM_00006 */
Std_ReturnType ComM_GetMaxComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED()) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_GETMAXCOMMODE_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    
    if (!COMM_VALIDATE_USER(User)) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_GETMAXCOMMODE_SID, COMM_E_PARAM_USER);
        return E_NOT_OK;
    }
    
    if (ComModePtr == NULL_PTR) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_GETMAXCOMMODE_SID, COMM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *ComModePtr = ComM_UserRequests[User].RequestedMode;
    return E_OK;
}

/** @req SWS_ComM_00007 */
Std_ReturnType ComM_GetRequestedComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
{
    return ComM_GetMaxComMode(User, ComModePtr);
}

/** @req SWS_ComM_00008 */
Std_ReturnType ComM_GetCurrentComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
{
    const ComM_UserConfigType* userConfig;
    ComM_ModeType highestMode = COMM_NO_COMMUNICATION;
    uint8 i;

#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED()) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_GETCURRENTCOMMODE_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    
    if (!COMM_VALIDATE_USER(User)) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_GETCURRENTCOMMODE_SID, COMM_E_PARAM_USER);
        return E_NOT_OK;
    }
    
    if (ComModePtr == NULL_PTR) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_GETCURRENTCOMMODE_SID, COMM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    userConfig = &ComM_UserConfig[User];
    
    /* Find the highest mode among user's channels */
    for (i = 0U; i < userConfig->NumChannels; i++) {
        ComM_ChannelHandleType channel = userConfig->ChannelMap[i];
        if (ComM_ChannelStates[channel].CurrentMode > highestMode) {
            highestMode = ComM_ChannelStates[channel].CurrentMode;
        }
    }

    *ComModePtr = highestMode;
    return E_OK;
}

/** @req SWS_ComM_00009 */
/*=============================================================================
 * Channel Management
 *===========================================================================*/
void ComM_CommunicationAllowed(ComM_ChannelHandleType Channel, boolean Allowed)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED()) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_COMMUNICATIONALLOWED_SID, COMM_E_NOT_INIT);
        return;
    }
    
    if (!COMM_VALIDATE_CHANNEL(Channel)) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_COMMUNICATIONALLOWED_SID, COMM_E_PARAM_CHANNEL);
        return;
    }
#endif

    ComM_ChannelStates[Channel].CommunicationAllowed = Allowed;
}

/** @req SWS_ComM_00004 */
void ComM_MainFunction(void)
{
    ComM_ChannelHandleType ch;

#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED()) {
        Det_ReportError(COMM_MODULE_ID, 0U, COMM_MAINFUNCTION_SID, COMM_E_NOT_INIT);
        return;
    }
#endif

    /* Process each channel state machine */
    for (ch = 0U; ch < COMM_NUM_CHANNELS; ch++) {
        ComM_ProcessChannelStateMachine(ch);
    }

#if (COMM_PNC_SUPPORT == STD_ON)
    ComM_MainFunctionPnc();
#endif
}

/*=============================================================================
 * PNC Management
 *===========================================================================*/
#if (COMM_PNC_SUPPORT == STD_ON)
/** @req SWS_ComM_00010 */
void ComM_MainFunctionPnc(void)
{
    ComM_PncHandleType pnc;

    for (pnc = 0U; pnc < COMM_NUM_PNCS; pnc++) {
        ComM_ProcessPncStateMachine(pnc);
    }
}

/** @req SWS_ComM_00011 */
Std_ReturnType ComM_RequestPncMode(ComM_PncHandleType Pnc, ComM_PncModeType PncMode)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED()) {
        return E_NOT_OK;
    }
    
    if (!COMM_VALIDATE_PNC(Pnc)) {
        return E_NOT_OK;
    }
#endif

    ComM_PncStates[Pnc].Mode = PncMode;
    ComM_PncStates[Pnc].RequestActive = (PncMode == COMM_PNC_REQUESTED);
    
    return E_OK;
}

/** @req SWS_ComM_00012 */
Std_ReturnType ComM_GetPncMode(ComM_PncHandleType Pnc, ComM_PncModeType* PncModePtr)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED() || (PncModePtr == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (!COMM_VALIDATE_PNC(Pnc)) {
        return E_NOT_OK;
    }
#endif

    *PncModePtr = ComM_PncStates[Pnc].Mode;
    return E_OK;
}
#endif /* COMM_PNC_SUPPORT */

/*=============================================================================
 * ECU State Manager Integration
 *===========================================================================*/
#if (COMM_ECUM_SUPPORT == STD_ON)
/** @req SWS_ComM_00013 */
void ComM_EcuM_WakeUpIndication(ComM_EcuM_WakeUpType WakeupType)
{
    ComM_ChannelHandleType ch;

#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED()) {
        return;
    }
#endif

    (void)WakeupType;

    /* Handle wake-up for all channels with wake-up support */
    for (ch = 0U; ch < COMM_NUM_CHANNELS; ch++) {
        if ((ComM_ConfigPtr->ChannelConfigs[ch].WakeUpSupport) != 0U) {
            if (!ComM_ChannelStates[ch].WakeUpInhibition) {
                ComM_ChannelStates[ch].RequestedMode = COMM_FULL_COMMUNICATION;
            }
        }
    }
}

/** @req SWS_ComM_00014 */
void ComM_EcuM_BusWakeUpIndication(ComM_ChannelHandleType Channel)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED() || !COMM_VALIDATE_CHANNEL(Channel)) {
        return;
    }
#endif

    if ((ComM_ConfigPtr->ChannelConfigs[Channel].WakeUpSupport) != 0U) {
        if (!ComM_ChannelStates[Channel].WakeUpInhibition) {
            ComM_ChannelStates[Channel].RequestedMode = COMM_FULL_COMMUNICATION;
        }
    }
}

/** @req SWS_ComM_00015 */
void ComM_EcuM_RunRequestIndication(boolean Requested)
{
    (void)Requested;
    /* ECU Run request handling - can be used for power management */
}
#endif /* COMM_ECUM_SUPPORT */

/** @req SWS_ComM_00016 */
/*=============================================================================
 * Bus State Manager Interface
 *===========================================================================*/
void ComM_BusSM_ModeIndication(ComM_ChannelHandleType Channel, ComM_ModeType Mode)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED() || !COMM_VALIDATE_CHANNEL(Channel)) {
        return;
    }
#endif

    ComM_ChannelStates[Channel].CurrentMode = Mode;

    /* Update state based on mode */
    switch (Mode) {
        case COMM_NO_COMMUNICATION:
            COMM_SET_CHANNEL_STATE(Channel, COMM_CHANNEL_STATE_NOCOM);
            break;
        case COMM_SILENT_COMMUNICATION:
            COMM_SET_CHANNEL_STATE(Channel, COMM_CHANNEL_STATE_SILENTCOM);
            break;
        case COMM_FULL_COMMUNICATION:
            COMM_SET_CHANNEL_STATE(Channel, COMM_CHANNEL_STATE_FULLCOM);
            break;
        default:
            /* Invalid mode - remain in current state */
            break;
    }
}

/** @req SWS_ComM_00017 */
void ComM_BusSM_BusSleepMode(ComM_ChannelHandleType Channel)
{
    ComM_BusSM_ModeIndication(Channel, COMM_NO_COMMUNICATION);
}

/** @req SWS_ComM_00018 */
void ComM_BusSM_NetworkMode(ComM_ChannelHandleType Channel)
{
    ComM_BusSM_ModeIndication(Channel, COMM_FULL_COMMUNICATION);
}

/** @req SWS_ComM_00019 */
void ComM_BusSM_PrepareBusSleepMode(ComM_ChannelHandleType Channel)
{
    ComM_BusSM_ModeIndication(Channel, COMM_SILENT_COMMUNICATION);
}

/*=============================================================================
 * DCM Integration
 *===========================================================================*/
#if (COMM_DCM_SUPPORT == STD_ON)
/** @req SWS_ComM_00020 */
Std_ReturnType ComM_DCM_ActiveDiagnostic(ComM_ChannelHandleType Channel)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED() || !COMM_VALIDATE_CHANNEL(Channel)) {
        return E_NOT_OK;
    }
#endif

    ComM_ChannelStates[Channel].DcmActive = TRUE;
    ComM_ChannelStates[Channel].PassiveDiagnostic = FALSE;
    
    /* Request full communication for diagnostic */
    ComM_ChannelStates[Channel].RequestedMode = COMM_FULL_COMMUNICATION;
    
    return E_OK;
}

/** @req SWS_ComM_00021 */
Std_ReturnType ComM_DCM_InactiveDiagnostic(ComM_ChannelHandleType Channel)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED() || !COMM_VALIDATE_CHANNEL(Channel)) {
        return E_NOT_OK;
    }
#endif

    ComM_ChannelStates[Channel].DcmActive = FALSE;
    ComM_ChannelStates[Channel].PassiveDiagnostic = FALSE;
    
    return E_OK;
}

/** @req SWS_ComM_00022 */
Std_ReturnType ComM_DCM_PassiveDiagnostic(ComM_ChannelHandleType Channel, boolean Active)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED() || !COMM_VALIDATE_CHANNEL(Channel)) {
        return E_NOT_OK;
    }
#endif

    ComM_ChannelStates[Channel].PassiveDiagnostic = Active;
    
    if ((Active) != 0U) {
        /* In passive mode, allow silent communication for response */
        if (ComM_ChannelStates[Channel].RequestedMode < COMM_SILENT_COMMUNICATION) {
            ComM_ChannelStates[Channel].RequestedMode = COMM_SILENT_COMMUNICATION;
        }
    }
    
    return E_OK;
}
#endif /* COMM_DCM_SUPPORT */

/** @req SWS_ComM_00023 */
/*=============================================================================
 * ECNM Integration
 *===========================================================================*/
void ComM_ECNM_NetworkMode(ComM_ChannelHandleType Channel)
{
    ComM_BusSM_NetworkMode(Channel);
}

/** @req SWS_ComM_00024 */
void ComM_ECNM_PrepareBusSleepMode(ComM_ChannelHandleType Channel)
{
    ComM_BusSM_PrepareBusSleepMode(Channel);
}

/** @req SWS_ComM_00025 */
void ComM_ECNM_BusSleepMode(ComM_ChannelHandleType Channel)
{
    ComM_BusSM_BusSleepMode(Channel);
}

/*=============================================================================
 * NVM Integration
 *===========================================================================*/
#if (COMM_NVM_STORAGE_ENABLED == STD_ON)
/** @req SWS_ComM_00026 */
void ComM_Nvm_StartUpError(void)
{
    /* Handle NVM startup errors - may inhibit certain features */
}

/** @req SWS_ComM_00027 */
void ComM_Nvm_StoreInhibitionStatus(void)
{
    /* Store inhibition status to NVM */
}
#endif

/** @req SWS_ComM_00028 */
/*=============================================================================
 * Diagnostic Support
 *===========================================================================*/
Std_ReturnType ComM_GetInhibitionStatus(ComM_ChannelHandleType Channel, ComM_InhibitionStatusType* StatusPtr)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED() || (StatusPtr == NULL_PTR) || !COMM_VALIDATE_CHANNEL(Channel)) {
        return E_NOT_OK;
    }
#endif

    *StatusPtr = COMM_INHIBITION_STATUS_NONE;
    
    if ((ComM_ChannelStates[Channel].WakeUpInhibition) != 0U) {
        *StatusPtr |= COMM_INHIBITION_STATUS_WAKEUP;
    }
    
    if (ComM_ChannelStates[Channel].LimitToNoCom || ComM_EcuLimitToNoCom) {
        *StatusPtr |= COMM_INHIBITION_STATUS_LIMIT_TO_NO_COM;
    }
    
    return E_OK;
}

/** @req SWS_ComM_00029 */
void ComM_LimitChannelToNoComMode(ComM_ChannelHandleType Channel, boolean Status)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED() || !COMM_VALIDATE_CHANNEL(Channel)) {
        return;
    }
#endif

    ComM_ChannelStates[Channel].LimitToNoCom = Status;
    
    if ((Status) != 0U) {
        /* Release any full communication requests */
        ComM_ChannelStates[Channel].RequestedMode = COMM_NO_COMMUNICATION;
    }
}

/** @req SWS_ComM_00030 */
void ComM_LimitECUToNoComMode(boolean Status)
{
    ComM_ChannelHandleType ch;

#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED()) {
        return;
    }
#endif

    ComM_EcuLimitToNoCom = Status;
    
    if ((Status) != 0U) {
        /* Limit all channels to NoCom */
        for (ch = 0U; ch < COMM_NUM_CHANNELS; ch++) {
            ComM_ChannelStates[ch].RequestedMode = COMM_NO_COMMUNICATION;
        }
    }
}

/** @req SWS_ComM_00031 */
void ComM_PreventWakeUp(ComM_ChannelHandleType Channel, boolean Status)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (!COMM_IS_INITIALIZED() || !COMM_VALIDATE_CHANNEL(Channel)) {
        return;
    }
#endif

    ComM_ChannelStates[Channel].WakeUpInhibition = Status;
}

/*=============================================================================
 * Internal Functions - Channel State Machine
 *===========================================================================*/
static void ComM_ProcessChannelStateMachine(ComM_ChannelHandleType Channel)
{
    ComM_ChannelStateType currentState;
    ComM_ModeType requestedMode;
    
    currentState = COMM_GET_CHANNEL_STATE(Channel);
    requestedMode = ComM_GetHighestRequestedMode(Channel);
    
    /* Apply ECU-wide limit to NoCom */
    if (ComM_EcuLimitToNoCom || ComM_ChannelStates[Channel].LimitToNoCom) {
        if (requestedMode > COMM_NO_COMMUNICATION) {
            requestedMode = COMM_NO_COMMUNICATION;
        }
    }
    
    /* Apply communication allowed constraint */
    if (!ComM_ChannelStates[Channel].CommunicationAllowed) {
        if (requestedMode > COMM_NO_COMMUNICATION) {
            requestedMode = COMM_NO_COMMUNICATION;
        }
    }

    ComM_ChannelStates[Channel].RequestedMode = requestedMode;

    /* State machine processing */
    switch (currentState) {
        case COMM_CHANNEL_STATE_NOCOM:
            if (requestedMode == COMM_FULL_COMMUNICATION) {
                COMM_SET_CHANNEL_STATE(Channel, COMM_CHANNEL_STATE_PENDING);
                ComM_ChannelStates[Channel].TimeoutCounter = 
                    ComM_ConfigPtr->ChannelConfigs[Channel].WakeUpDelay;
            }
            break;
            
        case COMM_CHANNEL_STATE_PENDING:
            if (ComM_ChannelStates[Channel].TimeoutCounter > 0U) {
                ComM_ChannelStates[Channel].TimeoutCounter--;
            } else {
                /* Wake-up complete */
                ComM_ExecuteChannelEntryAction(Channel);
                COMM_SET_CHANNEL_STATE(Channel, COMM_CHANNEL_STATE_FULLCOM);
            }
            
            if (requestedMode == COMM_NO_COMMUNICATION) {
                COMM_SET_CHANNEL_STATE(Channel, COMM_CHANNEL_STATE_NOCOM);
            }
            break;
            
        case COMM_CHANNEL_STATE_FULLCOM:
            if (requestedMode == COMM_NO_COMMUNICATION) {
                COMM_SET_CHANNEL_STATE(Channel, COMM_CHANNEL_STATE_SILENTCOM);
                ComM_ChannelStates[Channel].TimeoutCounter = 
                    ComM_ConfigPtr->ChannelConfigs[Channel].SilentTimeout;
            } else if (requestedMode == COMM_SILENT_COMMUNICATION) {
                COMM_SET_CHANNEL_STATE(Channel, COMM_CHANNEL_STATE_SILENTCOM);
                ComM_ChannelStates[Channel].TimeoutCounter = 
                    ComM_ConfigPtr->ChannelConfigs[Channel].SilentTimeout;
            }
            break;
            
        case COMM_CHANNEL_STATE_SILENTCOM:
            if (requestedMode == COMM_FULL_COMMUNICATION) {
                COMM_SET_CHANNEL_STATE(Channel, COMM_CHANNEL_STATE_FULLCOM);
            } else if (requestedMode == COMM_NO_COMMUNICATION) {
                if (ComM_ChannelStates[Channel].TimeoutCounter > 0U) {
                    ComM_ChannelStates[Channel].TimeoutCounter--;
                } else {
                    ComM_ExecuteChannelExitAction(Channel);
                    COMM_SET_CHANNEL_STATE(Channel, COMM_CHANNEL_STATE_NOCOM);
                }
            }
            break;
            
        default:
            /* Invalid state - reset to NoCom */
            COMM_SET_CHANNEL_STATE(Channel, COMM_CHANNEL_STATE_NOCOM);
            break;
    }
    
    ComM_UpdateChannelMode(Channel);
}

static void ComM_ProcessChannelTransitions(ComM_ChannelHandleType Channel)
{
    (void)Channel;
    /* Handle specific transitions if needed */
}

static void ComM_UpdateChannelMode(ComM_ChannelHandleType Channel)
{
    ComM_ChannelStateType state = COMM_GET_CHANNEL_STATE(Channel);
    ComM_ModeType newMode;
    
    switch (state) {
        case COMM_CHANNEL_STATE_NOCOM:
        case COMM_CHANNEL_STATE_PENDING:
            newMode = COMM_NO_COMMUNICATION;
            break;
        case COMM_CHANNEL_STATE_SILENTCOM:
            newMode = COMM_SILENT_COMMUNICATION;
            break;
        case COMM_CHANNEL_STATE_FULLCOM:
            newMode = COMM_FULL_COMMUNICATION;
            break;
        default:
            newMode = COMM_NO_COMMUNICATION;
            break;
    }
    
    if (ComM_ChannelStates[Channel].CurrentMode != newMode) {
        ComM_ChannelStates[Channel].CurrentMode = newMode;
        /* Notify BusSM of mode change */
        /* BusSM_ModeIndication(Channel, newMode); */
    }
}

static ComM_ModeType ComM_GetHighestRequestedMode(ComM_ChannelHandleType Channel)
{
    ComM_ModeType highestMode = COMM_NO_COMMUNICATION;
    ComM_UserHandleType user;
    
    /* Check all user requests for this channel */
    for (user = 0U; user < COMM_NUM_USERS; user++) {
        const ComM_UserConfigType* userConfig = &ComM_UserConfig[user];
        uint8 i;
        
        for (i = 0U; i < userConfig->NumChannels; i++) {
            if (userConfig->ChannelMap[i] == Channel) {
                if (ComM_UserRequests[user].RequestedMode > highestMode) {
                    highestMode = ComM_UserRequests[user].RequestedMode;
                }
                break;
            }
        }
    }
    
#if (COMM_DCM_SUPPORT == STD_ON)
    /* Consider DCM requests */
    if ((ComM_ChannelStates[Channel].DcmActive) != 0U) {
        if (highestMode < COMM_FULL_COMMUNICATION) {
            highestMode = COMM_FULL_COMMUNICATION;
        }
    }
    
    if ((ComM_ChannelStates[Channel].PassiveDiagnostic) != 0U) {
        if (highestMode < COMM_SILENT_COMMUNICATION) {
            highestMode = COMM_SILENT_COMMUNICATION;
        }
    }
#endif
    
    return highestMode;
}

static void ComM_ExecuteChannelEntryAction(ComM_ChannelHandleType Channel)
{
    /* Execute entry actions for FullCom state */
    (void)Channel;
    /* Could trigger NM message, enable transceiver, etc. */
}

static void ComM_ExecuteChannelExitAction(ComM_ChannelHandleType Channel)
{
    /* Execute exit actions for leaving FullCom */
    (void)Channel;
    /* Could disable transceiver, send NM sleep indication, etc. */
}

/*=============================================================================
 * Internal Functions - PNC State Machine
 *===========================================================================*/
#if (COMM_PNC_SUPPORT == STD_ON)
static void ComM_ProcessPncStateMachine(ComM_PncHandleType Pnc)
{
    const ComM_PncConfigType* pncConfig = &ComM_PncConfig[Pnc];
    ComM_PncStateType* pncState = &ComM_PncStates[Pnc];
    
    ComM_UpdatePncRequestStatus(Pnc);
    
    switch (pncState->Mode) {
        case COMM_PNC_NO_COMMUNICATION:
            if ((pncState->RequestActive) != 0U) {
                pncState->Mode = COMM_PNC_REQUESTED;
                ComM_HandlePncChannelRequests(Pnc);
            }
            break;
            
        case COMM_PNC_REQUESTED:
            if (!pncState->RequestActive) {
                pncState->Mode = COMM_PNC_READY_SLEEP;
            }
            break;
            
        case COMM_PNC_READY_SLEEP:
            if ((pncState->RequestActive) != 0U) {
                pncState->Mode = COMM_PNC_REQUESTED;
            } else {
                /* Wait for all requests to complete */
                if (pncState->ActiveRequestCount == 0U) {
                    pncState->Mode = COMM_PNC_PREPARE_SLEEP;
                    pncState->TimeoutCounter = pncConfig->PrepareSleepTimeout;
                }
            }
            break;
            
        case COMM_PNC_PREPARE_SLEEP:
            if ((pncState->RequestActive) != 0U) {
                pncState->Mode = COMM_PNC_REQUESTED;
            } else {
                if (pncState->TimeoutCounter > 0U) {
                    pncState->TimeoutCounter--;
                } else {
                    pncState->Mode = COMM_PNC_NO_COMMUNICATION;
                }
            }
            break;
            
        default:
            pncState->Mode = COMM_PNC_NO_COMMUNICATION;
            break;
    }
}

static void ComM_UpdatePncRequestStatus(ComM_PncHandleType Pnc)
{
    ComM_UserHandleType user;
    boolean requestActive = FALSE;
    
    /* Check if any user has requested this PNC */
    for (user = 0U; user < COMM_NUM_USERS; user++) {
        const ComM_UserConfigType* userConfig = &ComM_UserConfig[user];
        uint8 i;
        
        for (i = 0U; i < userConfig->NumPncs; i++) {
            if (userConfig->PncMap[i] == Pnc) {
                if ((ComM_UserRequests[user].Active) != 0U) {
                    requestActive = TRUE;
                    break;
                }
            }
        }
        
        if ((requestActive) != 0U) {
            break;
        }
    }
    
    ComM_PncStates[Pnc].RequestActive = requestActive;
}

static void ComM_HandlePncChannelRequests(ComM_PncHandleType Pnc)
{
    const ComM_PncConfigType* pncConfig = &ComM_PncConfig[Pnc];
    uint8 i;
    
    /* Request appropriate modes on associated channels */
    for (i = 0U; i < pncConfig->NumChannels; i++) {
        ComM_ChannelHandleType channel = pncConfig->ChannelMap[i].ChannelId;
        
        if ((pncConfig->ChannelMap[i].IsRequester) != 0U) {
            if (ComM_ChannelStates[channel].RequestedMode < COMM_FULL_COMMUNICATION) {
                ComM_ChannelStates[channel].RequestedMode = COMM_FULL_COMMUNICATION;
            }
        }
    }
}
#endif /* COMM_PNC_SUPPORT */

/** @req SWS_ComM_00032 */
/*==================================================================================================
*                          NM NOTIFICATIONS (T3, 2026-08-08)
*
* Called by the Nm stack (LinNm/CanNm/...) when a network enters
* BUS_SLEEP / PREPARE_BUS_SLEEP / NETWORK mode. No-op stubs: the ComM
* channel state machine is driven by ComM_RequestComMode; integrating
* Nm-initiated mode changes is follow-up work. Providing the symbols is
* required for the previously-empty-compiled modules to link.
==================================================================================================*/
void ComM_Nm_NetworkMode(uint8 NetworkHandle)
{
    (void)NetworkHandle;
}

/** @req SWS_ComM_00033 */
void ComM_Nm_PrepareBusSleepMode(uint8 NetworkHandle)
{
    (void)NetworkHandle;
}

/** @req SWS_ComM_00034 */
void ComM_Nm_BusSleepMode(uint8 NetworkHandle)
{
    (void)NetworkHandle;
}
