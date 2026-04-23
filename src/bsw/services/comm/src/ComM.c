/**
 * @file ComM.c
 * @brief Communication Manager Implementation
 */

#include "ComM.h"
#include "ComM_Cfg.h"
#include "Det.h"

/* Internal State */
static ComM_StateType ComM_State = COMM_STATE_UNINIT;
static ComM_ModeType ComM_RequestedMode[COMM_MAX_USERS];
static ComM_ModeType ComM_CurrentMode[COMM_MAX_CHANNELS];
static boolean ComM_CommunicationAllowedFlag[COMM_MAX_CHANNELS];
static boolean ComM_DiagnosticActive[COMM_MAX_CHANNELS];
static uint32 ComM_UserRequestMask[COMM_MAX_CHANNELS];

/* Version Info */
#define COMM_VENDOR_ID                      0x0001
#define COMM_INSTANCE_ID                    0x00

void ComM_Init(const ComM_ConfigType* ConfigPtr)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_INIT_SID, COMM_E_WRONG_PARAMETERS);
        return;
    }
#endif

    /* Initialize state */
    ComM_State = COMM_STATE_INIT;
    
    /* Initialize user request arrays */
    for (uint8 i = 0; i < COMM_MAX_USERS; i++)
    {
        ComM_RequestedMode[i] = COMM_NO_COMMUNICATION;
    }
    
    /* Initialize channel states */
    for (uint8 i = 0; i < COMM_MAX_CHANNELS; i++)
    {
        ComM_CurrentMode[i] = COMM_NO_COMMUNICATION;
        ComM_CommunicationAllowedFlag[i] = FALSE;
        ComM_DiagnosticActive[i] = FALSE;
        ComM_UserRequestMask[i] = 0;
    }
}

void ComM_DeInit(void)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (ComM_State == COMM_STATE_UNINIT)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_DEINIT_SID, COMM_E_NOT_INIT);
        return;
    }
#endif

    ComM_State = COMM_STATE_UNINIT;
}

void ComM_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
#if (COMM_VERSION_INFO_API == STD_ON)
    if (VersionInfo != NULL_PTR)
    {
        VersionInfo->vendorID = COMM_VENDOR_ID;
        VersionInfo->moduleID = COMM_MODULE_ID;
        VersionInfo->sw_major_version = COMM_SW_MAJOR_VERSION;
        VersionInfo->sw_minor_version = COMM_SW_MINOR_VERSION;
        VersionInfo->sw_patch_version = COMM_SW_PATCH_VERSION;
    }
#endif
}

Std_ReturnType ComM_RequestComMode(ComM_UserHandleType User, ComM_ModeType ComMode)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (ComM_State == COMM_STATE_UNINIT)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_REQUESTCOMODE_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    if (User >= COMM_MAX_USERS)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_REQUESTCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
    if (ComMode > COMM_FULL_COMMUNICATION)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_REQUESTCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
#endif

    /* Store the requested mode */
    ComM_RequestedMode[User] = ComMode;
    
    /* TODO: Notify BusSM of mode change request */
    /* TODO: Update channel state based on requests */
    
    return E_OK;
}

Std_ReturnType ComM_GetMaxComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (ComM_State == COMM_STATE_UNINIT)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETMAXCOMODE_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    if (User >= COMM_MAX_USERS)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETMAXCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
    if (ComModePtr == NULL_PTR)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETMAXCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
#endif

    /* Return highest requested mode across all users */
    ComM_ModeType maxMode = COMM_NO_COMMUNICATION;
    for (uint8 i = 0; i < COMM_MAX_USERS; i++)
    {
        if (ComM_RequestedMode[i] > maxMode)
        {
            maxMode = ComM_RequestedMode[i];
        }
    }
    *ComModePtr = maxMode;
    
    return E_OK;
}

Std_ReturnType ComM_GetRequestedComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (ComM_State == COMM_STATE_UNINIT)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETREQUESTEDCOMODE_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    if (User >= COMM_MAX_USERS)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETREQUESTEDCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
    if (ComModePtr == NULL_PTR)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETREQUESTEDCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
#endif

    *ComModePtr = ComM_RequestedMode[User];
    return E_OK;
}

Std_ReturnType ComM_GetCurrentComMode(ComM_UserHandleType User, ComM_ModeType* ComModePtr)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (ComM_State == COMM_STATE_UNINIT)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETCURRENTCOMODE_SID, COMM_E_NOT_INIT);
        return E_NOT_OK;
    }
    if (User >= COMM_MAX_USERS)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETCURRENTCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
    if (ComModePtr == NULL_PTR)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_GETCURRENTCOMODE_SID, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
#endif

    /* Return the highest current mode across all channels */
    ComM_ModeType maxMode = COMM_NO_COMMUNICATION;
    for (uint8 i = 0; i < COMM_MAX_CHANNELS; i++)
    {
        if (ComM_CurrentMode[i] > maxMode)
        {
            maxMode = ComM_CurrentMode[i];
        }
    }
    *ComModePtr = maxMode;
    
    return E_OK;
}

void ComM_CommunicationAllowed(ComM_ChannelHandleType Channel, boolean Allowed)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (ComM_State == COMM_STATE_UNINIT)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_COMMUNICATIONALLOWED_SID, COMM_E_NOT_INIT);
        return;
    }
    if (Channel >= COMM_MAX_CHANNELS)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_COMMUNICATIONALLOWED_SID, COMM_E_WRONG_PARAMETERS);
        return;
    }
#endif

    ComM_CommunicationAllowedFlag[Channel] = Allowed;
}

void ComM_MainFunction(void)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (ComM_State == COMM_STATE_UNINIT)
    {
        return;
    }
#endif

    /* Process mode changes for each channel */
    for (uint8 channel = 0; channel < COMM_MAX_CHANNELS; channel++)
    {
        /* Check if diagnostic session is active */
        if (ComM_DiagnosticActive[channel])
        {
            ComM_CurrentMode[channel] = COMM_FULL_COMMUNICATION;
            continue;
        }
        
        /* Check communication allowed flag */
        if (!ComM_CommunicationAllowedFlag[channel])
        {
            ComM_CurrentMode[channel] = COMM_NO_COMMUNICATION;
            continue;
        }
        
        /* TODO: Implement proper state machine */
        /* For now, just reflect the highest requested mode */
        ComM_ModeType maxRequested = COMM_NO_COMMUNICATION;
        for (uint8 user = 0; user < COMM_MAX_USERS; user++)
        {
            if (ComM_RequestedMode[user] > maxRequested)
            {
                maxRequested = ComM_RequestedMode[user];
            }
        }
        ComM_CurrentMode[channel] = maxRequested;
    }
}

/* DCM Interface */
Std_ReturnType ComM_DCM_ActiveDiagnostic(ComM_ChannelHandleType Channel)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (Channel >= COMM_MAX_CHANNELS)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, 0x80, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
#endif

    ComM_DiagnosticActive[Channel] = TRUE;
    return E_OK;
}

Std_ReturnType ComM_DCM_InactiveDiagnostic(ComM_ChannelHandleType Channel)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (Channel >= COMM_MAX_CHANNELS)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, 0x81, COMM_E_WRONG_PARAMETERS);
        return E_NOT_OK;
    }
#endif

    ComM_DiagnosticActive[Channel] = FALSE;
    return E_OK;
}

/* BusSM Interface */
void ComM_BusSM_ModeIndication(ComM_ChannelHandleType Channel, ComM_ModeType Mode)
{
#if (COMM_DEV_ERROR_DETECT == STD_ON)
    if (Channel >= COMM_MAX_CHANNELS)
    {
        Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, 0x82, COMM_E_WRONG_PARAMETERS);
        return;
    }
#endif

    ComM_CurrentMode[Channel] = Mode;
}

/* ECU Manager Interface */
Std_ReturnType ComM_EcuM_WakeUpIndication(void)
{
    /* Wake up indication received from ECU Manager */
    /* TODO: Implement wake up handling */
    return E_OK;
}

Std_ReturnType ComM_EcuM_RunRequestIndication(void)
{
    /* RUN request from ECU Manager */
    return E_OK;
}
