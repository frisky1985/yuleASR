/**
 * @file Nm.c
 * @brief Network Management Implementation
 */

#include "Nm.h"
#include "Nm_Cfg.h"
#include "Det.h"

/* Internal State */
static Nm_StateType Nm_ChannelState[NM_MAX_CHANNELS];
static Nm_ModeType Nm_ChannelMode[NM_MAX_CHANNELS];
static boolean Nm_ChannelInitialized[NM_MAX_CHANNELS];
static boolean Nm_NetworkRequested[NM_MAX_CHANNELS];
static boolean Nm_CommunicationEnabled[NM_MAX_CHANNELS];
static boolean Nm_RemoteSleepInd[NM_MAX_CHANNELS];
static uint8 Nm_UserData[NM_MAX_CHANNELS][8];
static boolean Nm_ModuleInitialized = FALSE;

/* Version Info */
#define NM_VENDOR_ID                      0x0001
#define NM_INSTANCE_ID                    0x00

void Nm_Init(const Nm_ConfigType* ConfigPtr)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_INIT_SID, NM_E_INVALID_POINTER);
        return;
    }
#endif

    /* Initialize all channels */
    for (uint8 i = 0; i < NM_MAX_CHANNELS; i++)
    {
        Nm_ChannelState[i] = NM_STATE_BUS_SLEEP;
        Nm_ChannelMode[i] = NM_MODE_BUS_SLEEP;
        Nm_ChannelInitialized[i] = TRUE;
        Nm_NetworkRequested[i] = FALSE;
        Nm_CommunicationEnabled[i] = TRUE;
        Nm_RemoteSleepInd[i] = FALSE;
        
        /* Clear user data */
        for (uint8 j = 0; j < 8; j++)
        {
            Nm_UserData[i][j] = 0;
        }
    }
    
    Nm_ModuleInitialized = TRUE;
}

void Nm_DeInit(void)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_DEINIT_SID, NM_E_UNINIT);
        return;
    }
#endif

    /* Deinitialize all channels */
    for (uint8 i = 0; i < NM_MAX_CHANNELS; i++)
    {
        Nm_ChannelInitialized[i] = FALSE;
    }
    
    Nm_ModuleInitialized = FALSE;
}

void Nm_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
#if (NM_VERSION_INFO_API == STD_ON)
    if (VersionInfo != NULL_PTR)
    {
        VersionInfo->vendorID = NM_VENDOR_ID;
        VersionInfo->moduleID = NM_MODULE_ID;
        VersionInfo->sw_major_version = NM_SW_MAJOR_VERSION;
        VersionInfo->sw_minor_version = NM_SW_MINOR_VERSION;
        VersionInfo->sw_patch_version = NM_SW_PATCH_VERSION;
    }
#endif
}

Std_ReturnType Nm_PassiveStartUp(Nm_ChannelHandleType nmChannelHandle)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_PASSIVESSTARTUP_SID, NM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmChannelHandle >= NM_MAX_CHANNELS)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_PASSIVESSTARTUP_SID, NM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif

    /* Passive startup - wait for network request from other nodes */
    Nm_ChannelState[nmChannelHandle] = NM_STATE_REPEAT_MESSAGE;
    Nm_ChannelMode[nmChannelHandle] = NM_MODE_NETWORK;
    
    return E_OK;
}

Std_ReturnType Nm_NetworkRequest(Nm_ChannelHandleType nmChannelHandle)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_NETWORKREQUEST_SID, NM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmChannelHandle >= NM_MAX_CHANNELS)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_NETWORKREQUEST_SID, NM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif

    Nm_NetworkRequested[nmChannelHandle] = TRUE;
    
    /* Transition to Repeat Message state */
    Nm_StateType previousState = Nm_ChannelState[nmChannelHandle];
    Nm_ChannelState[nmChannelHandle] = NM_STATE_REPEAT_MESSAGE;
    Nm_ChannelMode[nmChannelHandle] = NM_MODE_NETWORK;
    
    /* Notify state change */
    Nm_StateChangeNotification(nmChannelHandle, previousState, NM_STATE_REPEAT_MESSAGE);
    
    return E_OK;
}

Std_ReturnType Nm_NetworkRelease(Nm_ChannelHandleType nmChannelHandle)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_NETWORKRELEASE_SID, NM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmChannelHandle >= NM_MAX_CHANNELS)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_NETWORKRELEASE_SID, NM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif

    Nm_NetworkRequested[nmChannelHandle] = FALSE;
    
    /* Transition to Ready Sleep state */
    Nm_StateType previousState = Nm_ChannelState[nmChannelHandle];
    Nm_ChannelState[nmChannelHandle] = NM_STATE_READY_SLEEP;
    
    /* Notify state change */
    Nm_StateChangeNotification(nmChannelHandle, previousState, NM_STATE_READY_SLEEP);
    
    return E_OK;
}

Std_ReturnType Nm_DisableCommunication(Nm_ChannelHandleType nmChannelHandle)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_DISABLECOMMUNICATION_SID, NM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmChannelHandle >= NM_MAX_CHANNELS)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_DISABLECOMMUNICATION_SID, NM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif

    Nm_CommunicationEnabled[nmChannelHandle] = FALSE;
    return E_OK;
}

Std_ReturnType Nm_EnableCommunication(Nm_ChannelHandleType nmChannelHandle)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_ENABLECOMMUNICATION_SID, NM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmChannelHandle >= NM_MAX_CHANNELS)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_ENABLECOMMUNICATION_SID, NM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif

    Nm_CommunicationEnabled[nmChannelHandle] = TRUE;
    return E_OK;
}

Std_ReturnType Nm_GetState(Nm_ChannelHandleType nmChannelHandle, Nm_StateType* nmStatePtr)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_GETSTATE_SID, NM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmChannelHandle >= NM_MAX_CHANNELS)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_GETSTATE_SID, NM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    if (nmStatePtr == NULL_PTR)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_GETSTATE_SID, NM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#endif

    *nmStatePtr = Nm_ChannelState[nmChannelHandle];
    return E_OK;
}

Std_ReturnType Nm_GetMode(Nm_ChannelHandleType nmChannelHandle, Nm_ModeType* nmModePtr)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, 0x90, NM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmChannelHandle >= NM_MAX_CHANNELS)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, 0x90, NM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    if (nmModePtr == NULL_PTR)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, 0x90, NM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#endif

    *nmModePtr = Nm_ChannelMode[nmChannelHandle];
    return E_OK;
}

Std_ReturnType Nm_GetLocalNodeIdentifier(Nm_ChannelHandleType nmChannelHandle, Nm_NodeIdType* nmNodeIdPtr)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_GETLOCALNODEIDENTIFIER_SID, NM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmNodeIdPtr == NULL_PTR)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_GETLOCALNODEIDENTIFIER_SID, NM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#endif

    (void)nmChannelHandle;
    *nmNodeIdPtr = NM_NODE_ID;
    return E_OK;
}

Std_ReturnType Nm_GetUserData(Nm_ChannelHandleType nmChannelHandle, uint8* nmUserDataPtr)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_GETUSERDATA_SID, NM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmChannelHandle >= NM_MAX_CHANNELS)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_GETUSERDATA_SID, NM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    if (nmUserDataPtr == NULL_PTR)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, NM_GETUSERDATA_SID, NM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Copy user data */
    for (uint8 i = 0; i < 8; i++)
    {
        nmUserDataPtr[i] = Nm_UserData[nmChannelHandle][i];
    }
    
    return E_OK;
}

Std_ReturnType Nm_SetUserData(Nm_ChannelHandleType nmChannelHandle, const uint8* nmUserDataPtr)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, 0x91, NM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmChannelHandle >= NM_MAX_CHANNELS)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, 0x91, NM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
    if (nmUserDataPtr == NULL_PTR)
    {
        Det_ReportError(NM_MODULE_ID, NM_INSTANCE_ID, 0x91, NM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Copy user data */
    for (uint8 i = 0; i < 8; i++)
    {
        Nm_UserData[nmChannelHandle][i] = nmUserDataPtr[i];
    }
    
    return E_OK;
}

Std_ReturnType Nm_CheckRemoteSleepIndication(Nm_ChannelHandleType nmChannelHandle, boolean* nmRemoteSleepIndPtr)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        return E_NOT_OK;
    }
    if (nmChannelHandle >= NM_MAX_CHANNELS)
    {
        return E_NOT_OK;
    }
    if (nmRemoteSleepIndPtr == NULL_PTR)
    {
        return E_NOT_OK;
    }
#endif

    *nmRemoteSleepIndPtr = Nm_RemoteSleepInd[nmChannelHandle];
    return E_OK;
}

void Nm_MainFunction(void)
{
#if (NM_DEV_ERROR_DETECT == STD_ON)
    if (!Nm_ModuleInitialized)
    {
        return;
    }
#endif

    /* Process state machine for each channel */
    for (uint8 i = 0; i < NM_MAX_CHANNELS; i++)
    {
        if (!Nm_CommunicationEnabled[i])
        {
            continue;
        }

        /* Simple state machine handling */
        switch (Nm_ChannelState[i])
        {
            case NM_STATE_REPEAT_MESSAGE:
                /* After repeat message time, transition to Normal Operation */
                /* TODO: Implement timer handling */
                if (Nm_NetworkRequested[i])
                {
                    Nm_StateType prevState = Nm_ChannelState[i];
                    Nm_ChannelState[i] = NM_STATE_NORMAL_OPERATION;
                    Nm_StateChangeNotification(i, prevState, NM_STATE_NORMAL_OPERATION);
                }
                break;
                
            case NM_STATE_READY_SLEEP:
                /* If all nodes indicate remote sleep, prepare for bus sleep */
                if (!Nm_NetworkRequested[i] && Nm_RemoteSleepInd[i])
                {
                    Nm_StateType prevState = Nm_ChannelState[i];
                    Nm_ChannelState[i] = NM_STATE_PREPARE_BUS_SLEEP;
                    Nm_ChannelMode[i] = NM_MODE_PREPARE_BUS_SLEEP;
                    Nm_StateChangeNotification(i, prevState, NM_STATE_PREPARE_BUS_SLEEP);
                }
                break;
                
            default:
                break;
        }
    }
}

/* Callback Functions */
void Nm_StateChangeNotification(Nm_ChannelHandleType nmNetworkHandle, Nm_StateType nmPreviousState, Nm_StateType nmCurrentState)
{
#if (NM_STATE_CHANGE_IND_ENABLED == STD_ON)
    /* Notify application about state change */
    NM_STATE_CHANGE_NOTIFICATION(nmNetworkHandle, nmPreviousState, nmCurrentState);
#endif
    (void)nmNetworkHandle;
    (void)nmPreviousState;
    (void)nmCurrentState;
}

void Nm_RemoteSleepIndication(Nm_ChannelHandleType nmNetworkHandle)
{
#if (NM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
    if (nmNetworkHandle < NM_MAX_CHANNELS)
    {
        Nm_RemoteSleepInd[nmNetworkHandle] = TRUE;
        NM_REMOTE_SLEEP_INDICATION(nmNetworkHandle);
    }
#endif
}

void Nm_RemoteSleepCancellation(Nm_ChannelHandleType nmNetworkHandle)
{
#if (NM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
    if (nmNetworkHandle < NM_MAX_CHANNELS)
    {
        Nm_RemoteSleepInd[nmNetworkHandle] = FALSE;
        NM_REMOTE_SLEEP_CANCELLATION(nmNetworkHandle);
    }
#endif
    (void)nmNetworkHandle;
}

void Nm_NetworkStartIndication(Nm_ChannelHandleType nmNetworkHandle)
{
    /* Called when network startup is indicated by BusNm */
    (void)nmNetworkHandle;
}

void Nm_RxIndication(Nm_ChannelHandleType nmNetworkHandle, const uint8* nmPduDataPtr)
{
    /* Called when NM message is received */
    (void)nmNetworkHandle;
    (void)nmPduDataPtr;
}

/* Application callbacks - to be implemented by application */
__attribute__((weak)) void Appl_Nm_StateChangeNotification(Nm_ChannelHandleType ch, Nm_StateType prev, Nm_StateType curr)
{
    (void)ch;
    (void)prev;
    (void)curr;
}

__attribute__((weak)) void Appl_Nm_RemoteSleepIndication(Nm_ChannelHandleType ch)
{
    (void)ch;
}

__attribute__((weak)) void Appl_Nm_RemoteSleepCancellation(Nm_ChannelHandleType ch)
{
    (void)ch;
}
