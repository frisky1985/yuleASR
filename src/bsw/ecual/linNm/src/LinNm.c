/** * @file LinNm.c * @brief LIN Network Management module implementation following AutoSAR Classic Platform 4.x *
 * @version 1.0.0 * @date 2026-05-05 * @author Shanghai Yule Electronics Technology Co., Ltd. * @copyright Copyright (c)
 * 2026 Shanghai Yule Electronics Technology Co., Ltd. * * AutoSAR Standard: LIN Network Management (LINNM) * Layer: ECU
 * Abstraction Layer (ECUAL) */
/*==================================================================================================* INCLUDE
 * FILES==================================================================================================*/
#include "LinNm.h"
#include "LinNm_Cfg.h"
#include "LinIf.h"
#include "ComM.h"
#include "Det.h"
#include <string.h>
/*==================================================================================================* INTERNAL
 * DEFINITIONS==================================================================================================*/
#define LINNM_INITIALIZED (0x01U)
#define LINNM_UNINITIALIZED (0x00U)
#define LINNM_INVALID_CHANNEL (0xFFU)
/* State machine events */
typedef enum
{
    LINNM_EVENT_NONE = 0,
    LINNM_EVENT_NETWORK_REQUEST,
    LINNM_EVENT_NETWORK_RELEASE,
    LINNM_EVENT_TIMEOUT,
    LINNM_EVENT_MSG_RECEIVED,
    LINNM_EVENT_BUS_SYNCHRONIZATION,
    LINNM_EVENT_SLEEP_ACK,
    LINNM_EVENT_WAKEUP
} LinNm_EventType;
/*==================================================================================================* INTERNAL
 * VARIABLES==================================================================================================*/
/* Module initialization state */
static uint8 LinNm_ModuleState = LINNM_UNINITIALIZED;
/* Pointer to configuration */
static const LinNm_ConfigType* LinNm_ConfigPtr = NULL_PTR;
/*==================================================================================================* LOCAL FUNCTION
 * PROTOTYPES==================================================================================================*/
static inline uint8 LinNm_GetChannelIndex(NetworkHandleType nmChannelHandle);
static void LinNm_StateMachine(uint8 channelIndex, LinNm_EventType event);
static void LinNm_TransitionToState(uint8 channelIndex, LinNm_StateType newState);
static void LinNm_ProcessTimeouts(uint8 channelIndex);
static void LinNm_SendNmPdu(uint8 channelIndex, boolean activeWakeup);
static void LinNm_ReceiveNmPdu(uint8 channelIndex, const uint8* pduData);
static void LinNm_UpdateRemoteSleepIndication(uint8 channelIndex);
static void LinNm_NotifyStateChange(uint8 channelIndex, Nm_StateType prevState, Nm_StateType newState);
static void LinNm_NotifyModeChange(uint8 channelIndex, Nm_ModeType newMode);
static void LinNm_EnterBusSleep(uint8 channelIndex);
static void LinNm_LeaveBusSleep(uint8 channelIndex);
static boolean LinNm_IsMasterNode(uint8 channelIndex);
static void LinNm_StartTimeoutTimer(uint8 channelIndex, uint32 timeoutMs);
static void LinNm_StopTimeoutTimer(uint8 channelIndex);
/*==================================================================================================* HELPER
 * FUNCTIONS==================================================================================================*/
/** * @brief Get channel index from network handle */
static inline uint8 LinNm_GetChannelIndex(NetworkHandleType nmChannelHandle)
{
    uint8 i;
    if (LinNm_ConfigPtr == NULL_PTR)
    {
        return LINNM_INVALID_CHANNEL;
    }
    for (i = 0; i < LINNM_NUMBER_OF_CHANNELS; i++)
    {
        if (LinNm_ConfigPtr->ChannelConfig[i].NetworkHandle == nmChannelHandle)
        {
            return i;
        }
    }
    return LINNM_INVALID_CHANNEL;
}
/** * @brief Check if channel index is valid */
static inline boolean LinNm_IsChannelValid(uint8 channelIndex)
{
    return (channelIndex < LINNM_NUMBER_OF_CHANNELS);
}
/** * @brief Check if node is master */
static boolean LinNm_IsMasterNode(uint8 channelIndex)
{
    if (!LinNm_IsChannelValid(channelIndex))
    {
        return FALSE;
    }
    return (LinNm_ConfigPtr->ChannelConfig[channelIndex].NodeType == LINNM_NODE_TYPE_MASTER);
}
/** * @brief Start timeout timer */
static void LinNm_StartTimeoutTimer(uint8 channelIndex, uint32 timeoutMs)
{
    if (LinNm_IsChannelValid(channelIndex))
    {
        LinNm_ConfigPtr->ChannelRuntime[channelIndex].TimeoutTimer = timeoutMs;
    }
}
/** * @brief Stop timeout timer */
static void LinNm_StopTimeoutTimer(uint8 channelIndex)
{
    if (LinNm_IsChannelValid(channelIndex))
    {
        LinNm_ConfigPtr->ChannelRuntime[channelIndex].TimeoutTimer = 0U;
    }
}
/** * @brief Send NM PDU (triggered by master or during bus synchronization) */
static void LinNm_SendNmPdu(uint8 channelIndex, boolean activeWakeup)
{
    const LinNm_ChannelConfigType* chConfig;
    LinNm_ChannelRuntimeType* chRuntime;
    uint8 nmPdu[LINNM_PDU_SIZE];
    uint8 i;
    if (!LinNm_IsChannelValid(channelIndex))
    {
        return;
    }
    chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
    chRuntime = &LinNm_ConfigPtr->ChannelRuntime[channelIndex];
    /* Initialize PDU with zeros */
    memset(nmPdu, 0, LINNM_PDU_SIZE);
    /* Set Control Bit Vector (CBV) */
    nmPdu[LINNM_PDU_CBV_POS] = 0x00U;
    if (activeWakeup)
    {
        nmPdu[LINNM_PDU_CBV_POS] |= LINNM_CBV_ACTIVEWAKEUP_MASK;
    }
/* Set Node ID */
#if (LINNM_NODE_ID_ENABLED == STD_ON)
    nmPdu[LINNM_PDU_NODEID_POS] = chConfig->NodeId;
#endif
/* Set User Data */
#if (LINNM_USER_DATA_ENABLED == STD_ON)
    for (i = 0; i < chConfig->UserDataLength && i < LINNM_PDU_USER_DATA_SIZE; i++)
    {
        nmPdu[LINNM_PDU_USERDATA_START + i] = chRuntime->UserData[i];
    }
#endif
    /* Send PDU via LinIf - For LIN, the schedule table handles NM messages */
    /* In LIN, NM is handled via diagnostic frames or specific schedule tables */
    /* Here we inform LinIf to use appropriate schedule */
    if (chConfig->NodeType == LINNM_NODE_TYPE_MASTER)
    {
        /* Master: Request diagnostic or event-triggered schedule */
        (void)LinIf_ScheduleRequest(chConfig->LinIfChannelHandle, 1U);
        /* Diagnostic schedule */
    }
    /* Reset message cycle timer */
    chRuntime->MessageCycleTimer = chConfig->MsgCycleTimeMs;
}
/** * @brief Process received NM PDU */
static void LinNm_ReceiveNmPdu(uint8 channelIndex, const uint8* pduData)
{
    const LinNm_ChannelConfigType* chConfig;
    LinNm_ChannelRuntimeType* chRuntime;
    uint8 i;
    boolean activeWakeup = FALSE;
    if (!LinNm_IsChannelValid(channelIndex) || (pduData == NULL_PTR))
    {
        return;
    }
    chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
    chRuntime = &LinNm_ConfigPtr->ChannelRuntime[channelIndex];
    /* Check CBV for active wake-up */
    if ((pduData[LINNM_PDU_CBV_POS] & LINNM_CBV_ACTIVEWAKEUP_MASK) != 0U)
    {
        activeWakeup = TRUE;
    }
/* Extract user data */
#if (LINNM_USER_DATA_ENABLED == STD_ON)
    for (i = 0; i < chConfig->UserDataLength && i < LINNM_PDU_USER_DATA_SIZE; i++)
    {
        chRuntime->UserData[i] = pduData[LINNM_PDU_USERDATA_START + i];
    }
#endif
    /* If received message while in Prepare Bus Sleep or Bus Sleep, enter Network Mode */
    if ((chRuntime->State == LINNM_STATE_PREPARE_BUS_SLEEP) || (chRuntime->State == LINNM_STATE_BUS_SLEEP))
    {
        LinNm_StateMachine(channelIndex, LINNM_EVENT_MSG_RECEIVED);
    }
    /* Reset timeout timer on message reception */
    if (chRuntime->State != LINNM_STATE_BUS_SLEEP)
    {
        LinNm_StartTimeoutTimer(channelIndex, chConfig->TimeoutTimeMs);
    }
    /* Cancel remote sleep indication */
    if (chRuntime->RemoteSleepIndication)
    {
        chRuntime->RemoteSleepIndication = FALSE;
        chRuntime->RemoteSleepTimer = 0U;
#if (LINNM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
        LINNM_CALL_REMOTE_SLEEP_CANCELLATION(chConfig->NetworkHandle);
#endif
    }
}
/** * @brief Update remote sleep indication status */
static void LinNm_UpdateRemoteSleepIndication(uint8 channelIndex)
{
    LinNm_ChannelRuntimeType* chRuntime;
    const LinNm_ChannelConfigType* chConfig;
    if (!LinNm_IsChannelValid(channelIndex))
    {
        return;
    }
    chRuntime = &LinNm_ConfigPtr->ChannelRuntime[channelIndex];
    chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
#if (LINNM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
    if (chConfig->RemoteSleepIndEnabled)
    {
        /* Increment remote sleep timer */
        if (chRuntime->RemoteSleepTimer < chConfig->RemoteSleepIndTimeMs)
        {
            chRuntime->RemoteSleepTimer++;
        }
        /* Check if remote sleep indication time has elapsed */
        if ((chRuntime->RemoteSleepTimer >= chConfig->RemoteSleepIndTimeMs) && (!chRuntime->RemoteSleepIndication))
        {
            chRuntime->RemoteSleepIndication = TRUE;
            chRuntime->RemoteSleepIndStatus = TRUE;
            LINNM_CALL_REMOTE_SLEEP_INDICATION(chConfig->NetworkHandle);
        }
    }
#endif
}
/** * @brief Notify state change to Nm module */
static void LinNm_NotifyStateChange(uint8 channelIndex, Nm_StateType prevState, Nm_StateType newState)
{
    const LinNm_ChannelConfigType* chConfig;
    (void)prevState;
    if (!LinNm_IsChannelValid(channelIndex))
    {
        return;
    }
    chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
#if (LINNM_STATE_CHANGE_IND_ENABLED == STD_ON)
    LINNM_CALL_STATE_CHANGE_NOTIFICATION(chConfig->NetworkHandle, prevState, newState);
#endif
    /* Mark state change for ComM notification */
    LinNm_ConfigPtr->ChannelRuntime[channelIndex].StateChanged = TRUE;
}
/** * @brief Notify mode change to ComM module */
static void LinNm_NotifyModeChange(uint8 channelIndex, Nm_ModeType newMode)
{
    const LinNm_ChannelConfigType* chConfig;
    if (!LinNm_IsChannelValid(channelIndex))
    {
        return;
    }
    chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
    /* Notify ComM about mode change */
    switch (newMode)
    {
        case NM_MODE_BUS_SLEEP:
            ComM_Nm_BusSleepMode(chConfig->NetworkHandle);
            break;
        case NM_MODE_PREPARE_BUS_SLEEP:
            ComM_Nm_PrepareBusSleepMode(chConfig->NetworkHandle);
            break;
        case NM_MODE_SYNCHRONIZE: /* LIN does not have explicit synchronize mode, map to network mode */
            ComM_Nm_NetworkMode(chConfig->NetworkHandle);
            break;
        case NM_MODE_NETWORK:
            ComM_Nm_NetworkMode(chConfig->NetworkHandle);
            break;
        default: /* Do nothing */
            break;
    }
}
/** * @brief Enter bus sleep mode */
static void LinNm_EnterBusSleep(uint8 channelIndex)
{
    const LinNm_ChannelConfigType* chConfig;
    if (!LinNm_IsChannelValid(channelIndex))
    {
        return;
    }
    chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
    /* Request LinIf to enter sleep */
    (void)LinIf_GotoSleep(chConfig->LinIfChannelHandle);
    /* Notify ComM */
    ComM_Nm_BusSleepMode(chConfig->NetworkHandle);
}
/** * @brief Leave bus sleep mode (wake-up) */
static void LinNm_LeaveBusSleep(uint8 channelIndex)
{
    const LinNm_ChannelConfigType* chConfig;
    if (!LinNm_IsChannelValid(channelIndex))
    {
        return;
    }
    chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
    /* Request LinIf to wake up */
    (void)LinIf_WakeUp(chConfig->LinIfChannelHandle);
    /* Notify ComM */
    ComM_Nm_NetworkMode(chConfig->NetworkHandle);
}
/*==================================================================================================* STATE
 * MACHINE==================================================================================================*/
/** * @brief Transition to a new state */
static void LinNm_TransitionToState(uint8 channelIndex, LinNm_StateType newState)
{
    LinNm_ChannelRuntimeType* chRuntime;
    const LinNm_ChannelConfigType* chConfig;
    LinNm_StateType oldState;
    if (!LinNm_IsChannelValid(channelIndex))
    {
        return;
    }
    chRuntime = &LinNm_ConfigPtr->ChannelRuntime[channelIndex];
    chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
    oldState = chRuntime->State;
    if (oldState != newState)
    {
        /* Exit actions for old state */
        switch (oldState)
        {
            case LINNM_STATE_REPEAT_MESSAGE: /* Cancel repeat message timer */
                chRuntime->RepeatMessageCounter = 0U;
                break;
            default: /* No exit actions */
                break;
        }
        /* Enter new state */
        chRuntime->State = newState;
        /* Entry actions for new state */
        switch (newState)
        {
            case LINNM_STATE_BUS_SLEEP:
                chRuntime->Mode = LINNM_BUSNM_MODE_BUS_SLEEP;
                chRuntime->CommunicationEnabled = FALSE;
                chRuntime->RemoteSleepIndication = FALSE;
                chRuntime->RemoteSleepTimer = 0U;
                LinNm_StopTimeoutTimer(channelIndex);
                LinNm_EnterBusSleep(channelIndex);
                LinNm_NotifyModeChange(channelIndex, NM_MODE_BUS_SLEEP);
                break;
            case LINNM_STATE_PREPARE_BUS_SLEEP:
                chRuntime->Mode = LINNM_BUSNM_MODE_PREPARE_BUS_SLEEP;
                chRuntime->CommunicationEnabled = FALSE;
                LinNm_StartTimeoutTimer(channelIndex, chConfig->WaitBusSleepTimeMs);
                LinNm_NotifyModeChange(channelIndex, NM_MODE_PREPARE_BUS_SLEEP);
                break;
            case LINNM_STATE_READY_SLEEP:
                chRuntime->Mode = LINNM_BUSNM_MODE_NETWORK_MODE;
                chRuntime->CommunicationEnabled = TRUE;
                chRuntime->NetworkRequested = FALSE;
                LinNm_StartTimeoutTimer(channelIndex, chConfig->TimeoutTimeMs);
                LinNm_NotifyModeChange(channelIndex, NM_MODE_NETWORK);
                break;
            case LINNM_STATE_NORMAL_OPERATION:
                chRuntime->Mode = LINNM_BUSNM_MODE_NETWORK_MODE;
                chRuntime->CommunicationEnabled = TRUE;
                chRuntime->NetworkRequested = TRUE;
                LinNm_StartTimeoutTimer(channelIndex, chConfig->TimeoutTimeMs);
                LinNm_NotifyModeChange(channelIndex, NM_MODE_NETWORK);
                break;
            case LINNM_STATE_REPEAT_MESSAGE:
                chRuntime->Mode = LINNM_BUSNM_MODE_NETWORK_MODE;
                chRuntime->CommunicationEnabled = TRUE;
                chRuntime->RepeatMessageCounter = (uint8)(chConfig->TimeoutTimeMs / chConfig->MsgCycleTimeMs);
                LinNm_StartTimeoutTimer(channelIndex, chConfig->TimeoutTimeMs);
                LinNm_NotifyModeChange(channelIndex, NM_MODE_NETWORK);
                /* Send initial NM PDU for active wake-up */
                LinNm_SendNmPdu(channelIndex, TRUE);
                break;
            case LINNM_STATE_NETWORK_MODE:
                chRuntime->Mode = LINNM_BUSNM_MODE_NETWORK_MODE;
                chRuntime->CommunicationEnabled = TRUE;
                LinNm_StartTimeoutTimer(channelIndex, chConfig->TimeoutTimeMs);
                LinNm_NotifyModeChange(channelIndex, NM_MODE_NETWORK);
                break;
            default: /* No entry actions */
                break;
        }
        /* Notify state change */
        LinNm_NotifyStateChange(channelIndex, oldState, (Nm_StateType)newState);
    }
}
/** * @brief Main state machine processing */
static void LinNm_StateMachine(uint8 channelIndex, LinNm_EventType event)
{
    const LinNm_ChannelRuntimeType* chRuntime;
    if (!LinNm_IsChannelValid(channelIndex))
    {
        return;
    }
    chRuntime = &LinNm_ConfigPtr->ChannelRuntime[channelIndex];
    switch (chRuntime->State)
    {
        case LINNM_STATE_BUS_SLEEP:
            switch (event)
            {
                case LINNM_EVENT_NETWORK_REQUEST:
                case LINNM_EVENT_MSG_RECEIVED:
                case LINNM_EVENT_WAKEUP: /* Wake up the network */
                    LinNm_LeaveBusSleep(channelIndex);
                    LinNm_TransitionToState(channelIndex, LINNM_STATE_REPEAT_MESSAGE);
                    break;
                default: /* Stay in bus sleep */
                    break;
            }
            break;
        case LINNM_STATE_PREPARE_BUS_SLEEP:
            switch (event)
            {
                case LINNM_EVENT_NETWORK_REQUEST:
                case LINNM_EVENT_MSG_RECEIVED:
                case LINNM_EVENT_WAKEUP: /* Re-enter network mode */
                    LinNm_TransitionToState(channelIndex, LINNM_STATE_NORMAL_OPERATION);
                    break;
                case LINNM_EVENT_TIMEOUT: /* Enter bus sleep */
                    LinNm_TransitionToState(channelIndex, LINNM_STATE_BUS_SLEEP);
                    break;
                default: /* Stay in prepare bus sleep */
                    break;
            }
            break;
        case LINNM_STATE_READY_SLEEP:
            switch (event)
            {
                case LINNM_EVENT_NETWORK_REQUEST: /* Enter normal operation */
                    LinNm_TransitionToState(channelIndex, LINNM_STATE_NORMAL_OPERATION);
                    break;
                case LINNM_EVENT_MSG_RECEIVED: /* Reset timeout timer */
                    LinNm_StartTimeoutTimer(channelIndex, LinNm_ConfigPtr->ChannelConfig[channelIndex].TimeoutTimeMs);
                    break;
                case LINNM_EVENT_TIMEOUT: /* Enter prepare bus sleep */
                    LinNm_TransitionToState(channelIndex, LINNM_STATE_PREPARE_BUS_SLEEP);
                    break;
                default: /* Stay in ready sleep */
                    break;
            }
            break;
        case LINNM_STATE_NORMAL_OPERATION:
            switch (event)
            {
                case LINNM_EVENT_NETWORK_RELEASE: /* Enter ready sleep */
                    LinNm_TransitionToState(channelIndex, LINNM_STATE_READY_SLEEP);
                    break;
                case LINNM_EVENT_MSG_RECEIVED: /* Reset timeout timer */
                    LinNm_StartTimeoutTimer(channelIndex, LinNm_ConfigPtr->ChannelConfig[channelIndex].TimeoutTimeMs);
                    break;
                case LINNM_EVENT_TIMEOUT: /* Enter ready sleep */
                    LinNm_TransitionToState(channelIndex, LINNM_STATE_READY_SLEEP);
                    break;
                default: /* Stay in normal operation */
                    break;
            }
            break;
        case LINNM_STATE_REPEAT_MESSAGE:
            switch (event)
            {
                case LINNM_EVENT_NETWORK_RELEASE: /* Enter ready sleep */
                    LinNm_TransitionToState(channelIndex, LINNM_STATE_READY_SLEEP);
                    break;
                case LINNM_EVENT_MSG_RECEIVED:
                case LINNM_EVENT_TIMEOUT: /* Transition to normal operation */
                    LinNm_TransitionToState(channelIndex, LINNM_STATE_NORMAL_OPERATION);
                    break;
                default: /* Stay in repeat message, decrement counter in MainFunction */
                    break;
            }
            break;
        case LINNM_STATE_NETWORK_MODE:
            switch (event)
            {
                case LINNM_EVENT_NETWORK_RELEASE:
                    LinNm_TransitionToState(channelIndex, LINNM_STATE_READY_SLEEP);
                    break;
                case LINNM_EVENT_MSG_RECEIVED:
                    LinNm_StartTimeoutTimer(channelIndex, LinNm_ConfigPtr->ChannelConfig[channelIndex].TimeoutTimeMs);
                    break;
                case LINNM_EVENT_TIMEOUT:
                    LinNm_TransitionToState(channelIndex, LINNM_STATE_READY_SLEEP);
                    break;
                default: /* Stay in network mode */
                    break;
            }
            break;
        default: /* Invalid state, enter bus sleep */
            LinNm_TransitionToState(channelIndex, LINNM_STATE_BUS_SLEEP);
            break;
    }
}
/** * @brief Process timeout timers */
static void LinNm_ProcessTimeouts(uint8 channelIndex)
{
    LinNm_ChannelRuntimeType* chRuntime;
    if (!LinNm_IsChannelValid(channelIndex))
    {
        return;
    }
    chRuntime = &LinNm_ConfigPtr->ChannelRuntime[channelIndex];
    if (chRuntime->TimeoutTimer > 0U)
    {
        chRuntime->TimeoutTimer--;
        if (chRuntime->TimeoutTimer == 0U)
        {
            /* Timeout occurred */
            LinNm_StateMachine(channelIndex, LINNM_EVENT_TIMEOUT);
        }
    }
}
/*==================================================================================================* API
 * FUNCTIONS==================================================================================================*/
/** * @brief Returns the version information of the LinNm module. */
#if (LINNM_VERSION_INFO_API == STD_ON)
void LinNm_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#    if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_GETVERSIONINFO, LINNM_E_INVALID_POINTER);
        return;
    }
#    endif
    versioninfo->vendorID = LINNM_VENDOR_ID;
    versioninfo->moduleID = LINNM_MODULE_ID;
    versioninfo->sw_major_version = LINNM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = LINNM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = LINNM_SW_PATCH_VERSION;
}
#endif
/** * @brief Initializes the LIN NM module. */
void LinNm_Init(const LinNm_ConfigType* config)
{
    uint8 i;
    uint8 j;
#if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (config == NULL_PTR)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_INIT, LINNM_E_INVALID_POINTER);
        return;
    }
    if (LinNm_ModuleState == LINNM_INITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_INIT, LINNM_E_ALREADY_INITIALIZED);
        return;
    }
#endif
    LinNm_ConfigPtr = config;
    /* Initialize all channels */
    for (i = 0; i < LINNM_NUMBER_OF_CHANNELS; i++)
    {
        /* Initialize runtime data */
        LinNm_ConfigPtr->ChannelRuntime[i].State = LINNM_STATE_BUS_SLEEP;
        LinNm_ConfigPtr->ChannelRuntime[i].Mode = LINNM_BUSNM_MODE_BUS_SLEEP;
        LinNm_ConfigPtr->ChannelRuntime[i].CommunicationEnabled = FALSE;
        LinNm_ConfigPtr->ChannelRuntime[i].RemoteSleepIndication = FALSE;
        LinNm_ConfigPtr->ChannelRuntime[i].RemoteSleepIndStatus = FALSE;
        LinNm_ConfigPtr->ChannelRuntime[i].BusSynchronizationActive = FALSE;
        LinNm_ConfigPtr->ChannelRuntime[i].TimeoutTimer = 0U;
        LinNm_ConfigPtr->ChannelRuntime[i].RemoteSleepTimer = 0U;
        LinNm_ConfigPtr->ChannelRuntime[i].NetworkRequested = FALSE;
        LinNm_ConfigPtr->ChannelRuntime[i].StateChanged = FALSE;
        LinNm_ConfigPtr->ChannelRuntime[i].BusLoadReductionActive = FALSE;
        LinNm_ConfigPtr->ChannelRuntime[i].MessageCycleTimer = 0U;
        LinNm_ConfigPtr->ChannelRuntime[i].RepeatMessageCounter = 0U;
        LinNm_ConfigPtr->ChannelRuntime[i].UserDataLength = LinNm_ConfigPtr->ChannelConfig[i].UserDataLength;
        /* Clear user data */
        for (j = 0; j < 8; j++)
        {
            LinNm_ConfigPtr->ChannelRuntime[i].UserData[j] = 0U;
        }
    }
    LinNm_ModuleState = LINNM_INITIALIZED;
}
/** * @brief De-initializes the LIN NM module. */
void LinNm_DeInit(void)
{
    uint8 i;
#if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (LinNm_ModuleState == LINNM_UNINITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_DEINIT, LINNM_E_UNINIT);
        return;
    }
#endif
    /* Deinitialize all channels - enter bus sleep */
    for (i = 0; i < LINNM_NUMBER_OF_CHANNELS; i++)
    {
        LinNm_TransitionToState(i, LINNM_STATE_BUS_SLEEP);
    }
    LinNm_ConfigPtr = NULL_PTR;
    LinNm_ModuleState = LINNM_UNINITIALIZED;
}
/** * @brief Passive startup of the network management. */
Std_ReturnType LinNm_PassiveStartUp(NetworkHandleType nmChannelHandle)
{
    uint8 channelIndex;
    Std_ReturnType ret = E_NOT_OK;
#if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (LinNm_ModuleState == LINNM_UNINITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_PASSIVESTARTUP, LINNM_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    channelIndex = LinNm_GetChannelIndex(nmChannelHandle);
#if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (channelIndex == LINNM_INVALID_CHANNEL)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_PASSIVESTARTUP, LINNM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif
#if (LINNM_PASSIVE_MODE_ENABLED == STD_ON)
    if (LinNm_ConfigPtr->ChannelConfig[channelIndex].PassiveModeEnabled)
    {
        /* In passive mode, just transition to Ready Sleep */
        LinNm_TransitionToState(channelIndex, LINNM_STATE_READY_SLEEP);
        ret = E_OK;
    }
    else

#        endif
        {
            /* Active mode: enter Repeat Message state */
            LinNm_StateMachine(channelIndex, LINNM_EVENT_NETWORK_REQUEST);
            ret = E_OK;
        }
    return ret;
}
/** * @brief Request the network to enter network mode. */
Std_ReturnType LinNm_NetworkRequest(NetworkHandleType nmChannelHandle)
{
    uint8 channelIndex;
    Std_ReturnType ret = E_NOT_OK;
#    if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (LinNm_ModuleState == LINNM_UNINITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_NETWORKREQUEST, LINNM_E_UNINIT);
        return E_NOT_OK;
    }
#    endif
    channelIndex = LinNm_GetChannelIndex(nmChannelHandle);
#    if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (channelIndex == LINNM_INVALID_CHANNEL)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_NETWORKREQUEST, LINNM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#    endif
#    if (LINNM_PASSIVE_MODE_ENABLED == STD_ON)
    if (LinNm_ConfigPtr->ChannelConfig[channelIndex].PassiveModeEnabled)
    {
        /* Passive mode: no transmission, just enter Ready Sleep */
        LinNm_TransitionToState(channelIndex, LINNM_STATE_READY_SLEEP);
        ret = E_OK;
    }
    else

#        endif
        {
            /* Active mode */
            LinNm_ConfigPtr->ChannelRuntime[channelIndex].NetworkRequested = TRUE;
            /* Check if already in network mode */
            if (LinNm_ConfigPtr->ChannelRuntime[channelIndex].State == LINNM_STATE_NORMAL_OPERATION)
            {
                ret = E_OK;
                /* Already in network mode */
            }
            else
            {
                LinNm_StateMachine(channelIndex, LINNM_EVENT_NETWORK_REQUEST);
                ret = E_OK;
            }
        }
    return ret;
}
/** * @brief Request the network to be released to enter bus sleep mode. */
Std_ReturnType LinNm_NetworkRelease(NetworkHandleType nmChannelHandle)
{
    uint8 channelIndex;
    Std_ReturnType ret = E_NOT_OK;
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (LinNm_ModuleState == LINNM_UNINITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_NETWORKRELEASE, LINNM_E_UNINIT);
        return E_NOT_OK;
    }
#        endif
    channelIndex = LinNm_GetChannelIndex(nmChannelHandle);
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (channelIndex == LINNM_INVALID_CHANNEL)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_NETWORKRELEASE, LINNM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#        endif
    LinNm_ConfigPtr->ChannelRuntime[channelIndex].NetworkRequested = FALSE;
    LinNm_StateMachine(channelIndex, LINNM_EVENT_NETWORK_RELEASE);
    ret = E_OK;
    return ret;
}
/** * @brief Returns the state and the mode of the network management. */
Std_ReturnType LinNm_GetState(NetworkHandleType nmChannelHandle, Nm_StateType* nmStatePtr, Nm_ModeType* nmModePtr)
{
    uint8 channelIndex;
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (LinNm_ModuleState == LINNM_UNINITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_GETSTATE, LINNM_E_UNINIT);
        return E_NOT_OK;
    }
    if ((nmStatePtr == NULL_PTR) && (nmModePtr == NULL_PTR))
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_GETSTATE, LINNM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#        endif
    channelIndex = LinNm_GetChannelIndex(nmChannelHandle);
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (channelIndex == LINNM_INVALID_CHANNEL)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_GETSTATE, LINNM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#        endif
    if (nmStatePtr != NULL_PTR)
    {
        *nmStatePtr = (Nm_StateType)LinNm_ConfigPtr->ChannelRuntime[channelIndex].State;
    }
    if (nmModePtr != NULL_PTR)
    {
        *nmModePtr = (Nm_ModeType)LinNm_ConfigPtr->ChannelRuntime[channelIndex].Mode;
    }
    return E_OK;
}
/** * @brief Request bus synchronization. */
Std_ReturnType LinNm_RequestBusSynchronization(NetworkHandleType nmChannelHandle)
{
    uint8 channelIndex;
    Std_ReturnType ret = E_NOT_OK;
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (LinNm_ModuleState == LINNM_UNINITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_REQUESTBUSSYNCHRONIZATION, LINNM_E_UNINIT);
        return E_NOT_OK;
    }
#        endif
    channelIndex = LinNm_GetChannelIndex(nmChannelHandle);
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (channelIndex == LINNM_INVALID_CHANNEL)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_REQUESTBUSSYNCHRONIZATION, LINNM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#        endif
#        if (LINNM_BUS_SYNCHRONIZATION_ENABLED == STD_ON)
    const LinNm_ChannelConfigType* chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
    LinNm_ChannelRuntimeType* chRuntime = &LinNm_ConfigPtr->ChannelRuntime[channelIndex];
    if (chConfig->BusSynchronizationEnabled && chRuntime->CommunicationEnabled)
    {
        /* Set bus synchronization flag */
        chRuntime->BusSynchronizationActive = TRUE;
        /* Master node initiates synchronization by sending NM PDU */
        if (chConfig->NodeType == LINNM_NODE_TYPE_MASTER)
        {
            LinNm_SendNmPdu(channelIndex, FALSE);
        }
        LINNM_CALL_SYNC_POINT(chConfig->NetworkHandle);
        ret = E_OK;
    }
#        else
    (void)channelIndex;
/* Suppress unused parameter warning */
#        endif
    return ret;
}
/** * @brief Checks if remote sleep indication has taken place or not. */
Std_ReturnType LinNm_CheckRemoteSleepIndication(NetworkHandleType nmChannelHandle, boolean* nmRemoteSleepIndPtr)
{
    uint8 channelIndex;
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (LinNm_ModuleState == LINNM_UNINITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_CHECKREMOTESLEEPINDICATION, LINNM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmRemoteSleepIndPtr == NULL_PTR)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_CHECKREMOTESLEEPINDICATION, LINNM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#        endif
    channelIndex = LinNm_GetChannelIndex(nmChannelHandle);
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (channelIndex == LINNM_INVALID_CHANNEL)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_CHECKREMOTESLEEPINDICATION, LINNM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#        endif
#        if (LINNM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
    *nmRemoteSleepIndPtr = LinNm_ConfigPtr->ChannelRuntime[channelIndex].RemoteSleepIndStatus;
#        else
    *nmRemoteSleepIndPtr = FALSE;
#        endif
    return E_OK;
}
/** * @brief Main function for the LIN NM module. */
void LinNm_MainFunction(void)
{
    uint8 i;
    LinNm_ChannelRuntimeType* chRuntime;
    const LinNm_ChannelConfigType* chConfig;
    if (LinNm_ModuleState != LINNM_INITIALIZED)
    {
        return;
    }
    for (i = 0; i < LINNM_NUMBER_OF_CHANNELS; i++)
    {
        chRuntime = &LinNm_ConfigPtr->ChannelRuntime[i];
        chConfig = &LinNm_ConfigPtr->ChannelConfig[i];
        /* Process timeouts */
        LinNm_ProcessTimeouts(i);
/* Process remote sleep indication */
#        if (LINNM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
        if (chConfig->RemoteSleepIndEnabled && chRuntime->CommunicationEnabled)
        {
            LinNm_UpdateRemoteSleepIndication(i);
        }
#        endif
        /* Handle message cycle timer for active nodes */
        if ((chRuntime->State == LINNM_STATE_NORMAL_OPERATION) || (chRuntime->State == LINNM_STATE_REPEAT_MESSAGE))
        {
            if (chRuntime->MessageCycleTimer > 0U)
            {
                chRuntime->MessageCycleTimer--;
            }
            else
            {
                /* Time to send NM message */
                if (chConfig->NodeType == LINNM_NODE_TYPE_MASTER)
                {
                    LinNm_SendNmPdu(i, FALSE);
                }
                chRuntime->MessageCycleTimer = chConfig->MsgCycleTimeMs;
            }
            /* Handle repeat message counter */
            if (chRuntime->State == LINNM_STATE_REPEAT_MESSAGE)
            {
                if (chRuntime->RepeatMessageCounter > 0U)
                {
                    chRuntime->RepeatMessageCounter--;
                    if (chRuntime->RepeatMessageCounter == 0U)
                    {
                        /* Repeat message period expired */
                        LinNm_StateMachine(i, LINNM_EVENT_TIMEOUT);
                    }
                }
            }
        }
        /* Handle bus synchronization */
        if (chRuntime->BusSynchronizationActive)
        {
            /* Periodically perform synchronization */
            if (chConfig->NodeType == LINNM_NODE_TYPE_MASTER)
            {
                if (chRuntime->MessageCycleTimer == 0U)
                {
                    LINNM_CALL_SYNC_POINT(chConfig->NetworkHandle);
                }
            }
        }
    }
}
/*==================================================================================================* EXTENDED API
 * FUNCTIONS==================================================================================================*/
#        if (LINNM_COM_CONTROL_ENABLED == STD_ON)
/** * @brief Request communication mode change */
Std_ReturnType LinNm_RequestComMode(NetworkHandleType nmChannelHandle, ComM_ModeType nmComMode)
{
    uint8 channelIndex;
    Std_ReturnType ret = E_NOT_OK;
#            if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (LinNm_ModuleState == LINNM_UNINITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_COMCONTROL, LINNM_E_UNINIT);
        return E_NOT_OK;
    }
#            endif
    channelIndex = LinNm_GetChannelIndex(nmChannelHandle);
#            if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (channelIndex == LINNM_INVALID_CHANNEL)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_COMCONTROL, LINNM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#            endif
    const LinNm_ChannelConfigType* chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
    if (chConfig->ComControlEnabled)
    {
        switch (nmComMode)
        {
            case COMM_FULL_COMMUNICATION:
                LinNm_ConfigPtr->ChannelRuntime[channelIndex].CommunicationEnabled = TRUE;
                ret = E_OK;
                break;
            case COMM_SILENT_COMMUNICATION:
            case COMM_NO_COMMUNICATION:
                LinNm_ConfigPtr->ChannelRuntime[channelIndex].CommunicationEnabled = FALSE;
                ret = E_OK;
                break;
            default:
                ret = E_NOT_OK;
                break;
        }
    }
    return ret;
}
/** * @brief Get current communication mode */
Std_ReturnType LinNm_GetCurrentComMode(NetworkHandleType nmChannelHandle, ComM_ModeType* nmComModePtr)
{
    uint8 channelIndex;
#            if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (LinNm_ModuleState == LINNM_UNINITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_COMCONTROL, LINNM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmComModePtr == NULL_PTR)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_COMCONTROL, LINNM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#            endif
    channelIndex = LinNm_GetChannelIndex(nmChannelHandle);
#            if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (channelIndex == LINNM_INVALID_CHANNEL)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_COMCONTROL, LINNM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#            endif
    if (LinNm_ConfigPtr->ChannelRuntime[channelIndex].CommunicationEnabled)
    {
        *nmComModePtr = COMM_FULL_COMMUNICATION;
    }
    else
    {
        *nmComModePtr = COMM_NO_COMMUNICATION;
    }
    return E_OK;
}
#        endif
/** * @brief Set user data for NM PDU */
Std_ReturnType LinNm_SetUserData(NetworkHandleType nmChannelHandle, const uint8* nmUserDataPtr)
{
    uint8 channelIndex;
    uint8 i;
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (LinNm_ModuleState == LINNM_UNINITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_SETUSERDATA, LINNM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmUserDataPtr == NULL_PTR)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_SETUSERDATA, LINNM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#        endif
    channelIndex = LinNm_GetChannelIndex(nmChannelHandle);
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (channelIndex == LINNM_INVALID_CHANNEL)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_SETUSERDATA, LINNM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#        endif
#        if (LINNM_USER_DATA_ENABLED == STD_ON)
    const LinNm_ChannelConfigType* chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
    for (i = 0; i < chConfig->UserDataLength && i < LINNM_PDU_USER_DATA_SIZE; i++)
    {
        LinNm_ConfigPtr->ChannelRuntime[channelIndex].UserData[i] = nmUserDataPtr[i];
    }
    LinNm_ConfigPtr->ChannelRuntime[channelIndex].UserDataLength = chConfig->UserDataLength;
    return E_OK;
#        else
    return E_NOT_OK;
#        endif
}
/** * @brief Get user data from NM PDU */
Std_ReturnType LinNm_GetUserData(NetworkHandleType nmChannelHandle, uint8* nmUserDataPtr)
{
    uint8 channelIndex;
    uint8 i;
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (LinNm_ModuleState == LINNM_UNINITIALIZED)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_GETUSERDATA, LINNM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmUserDataPtr == NULL_PTR)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_GETUSERDATA, LINNM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
#        endif
    channelIndex = LinNm_GetChannelIndex(nmChannelHandle);
#        if (LINNM_DEV_ERROR_DETECT == STD_ON)
    if (channelIndex == LINNM_INVALID_CHANNEL)
    {
        Det_ReportError(LINNM_MODULE_ID, 0U, LINNM_SID_GETUSERDATA, LINNM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#        endif
#        if (LINNM_USER_DATA_ENABLED == STD_ON)
    const LinNm_ChannelConfigType* chConfig = &LinNm_ConfigPtr->ChannelConfig[channelIndex];
    for (i = 0; i < chConfig->UserDataLength && i < LINNM_PDU_USER_DATA_SIZE; i++)
    {
        nmUserDataPtr[i] = LinNm_ConfigPtr->ChannelRuntime[channelIndex].UserData[i];
    }
    return E_OK;
#        else
    return E_NOT_OK;
#        endif
}
/*==================================================================================================* CALLBACK
 * FUNCTIONS==================================================================================================*/
/** * @brief LinIf transmission confirmation callback */
void LinIf_TxConfirmation(uint8 Channel, uint8 LinTxPduId)
{
    uint8 i;
    (void)LinTxPduId;
    /* May be used for future extensions */
    if (LinNm_ModuleState != LINNM_INITIALIZED)
    {
        return;
    }
    /* Find matching channel */
    for (i = 0; i < LINNM_NUMBER_OF_CHANNELS; i++)
    {
        if (LinNm_ConfigPtr->ChannelConfig[i].LinIfChannelHandle == Channel)
        {
            /* Message transmitted successfully */
            /* Reset message cycle timer */
            LinNm_ConfigPtr->ChannelRuntime[i].MessageCycleTimer = LinNm_ConfigPtr->ChannelConfig[i].MsgCycleTimeMs;
            break;
        }
    }
}
/** * @brief LinIf schedule request confirmation callback */
void LinIf_ScheduleRequestConfirmation(uint8 Channel, uint8 ScheduleIndex)
{
    uint8 i;
    (void)ScheduleIndex;
    /* May be used for future extensions */
    if (LinNm_ModuleState != LINNM_INITIALIZED)
    {
        return;
    }
    /* Find matching channel */
    for (i = 0; i < LINNM_NUMBER_OF_CHANNELS; i++)
    {
        if (LinNm_ConfigPtr->ChannelConfig[i].LinIfChannelHandle == Channel)
        {
            /* Schedule switch confirmed - bus is synchronized */
            if (LinNm_ConfigPtr->ChannelRuntime[i].BusSynchronizationActive)
            {
                LINNM_CALL_SYNC_POINT(LinNm_ConfigPtr->ChannelConfig[i].NetworkHandle);
            }
            break;
        }
    }
}
