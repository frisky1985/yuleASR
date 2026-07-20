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
 * @file CanNm.c
 * @brief CAN Network Management implementation file
 * @version 4.4.0
 *
 * AUTOSAR CAN Network Management Module Core Implementation
 * Following AUTOSAR_SWS_CANNetworkManagement specification version 4.4.0
 *
 * State Machine:
 * Bus Sleep Mode <-> Prepare Bus Sleep Mode <-> Network Mode
 *                                       |
 *                       Repeat Message State <-> Normal Operation State <-> Ready Sleep State
 */

/*==================================================================================================
 *                                           INCLUDES
 ==================================================================================================*/
#include "CanNm.h"
#include "CanIf.h"
#include "ComM.h"
#include "Nm.h"

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

#include <string.h>

/*==================================================================================================
 *                                      LOCAL DEFINES
 ==================================================================================================*/
#define CANNM_MODULE_INITIALIZED            (boolean)TRUE
#define CANNM_MODULE_NOT_INITIALIZED        (boolean)FALSE

#define CANNM_CBV_REPEAT_MSG_BIT            (uint8)0x01U
#define CANNM_CBV_ACTIVE_WAKEUP_BIT         (uint8)0x04U
#define CANNM_CBV_NM_COORD_SLEEP_BIT        (uint8)0x10U

#define CANNM_INVALID_CHANNEL               (uint8)0xFFU

/*==================================================================================================
 *                                      LOCAL TYPES
 ==================================================================================================*/

/*==================================================================================================
 *                               LOCAL VARIABLES (STATIC)
 ==================================================================================================*/
/**
 * @brief Module initialization state
 */
static boolean CanNm_ModuleInitialized = CANNM_MODULE_NOT_INITIALIZED;

/**
 * @brief Channel state storage for all configured channels
 */
static CanNm_ChannelStateType CanNm_ChannelStates[CANNM_NUMBER_OF_CHANNELS];

/**
 * @brief Pointer to configuration structure
 */
static const CanNm_ConfigType* CanNm_ConfigPtr = NULL;

/*==================================================================================================
 *                               LOCAL FUNCTION PROTOTYPES
 ==================================================================================================*/
static uint8 CanNm_GetChannelIndex(NetworkHandleType nmChannelHandle);
static void CanNm_ProcessStateMachine(uint8 ChannelIndex);
static void CanNm_TransmitMessage(uint8 ChannelIndex, boolean Immediate);
static void CanNm_UpdateTxPdu(uint8 ChannelIndex);
static boolean CanNm_ValidateChannel(uint8 ChannelIndex, uint8 ApiId);
static void CanNm_EnterBusSleepMode(uint8 ChannelIndex);
static void CanNm_EnterPrepareBusSleepMode(uint8 ChannelIndex);
static void CanNm_EnterRepeatMessageState(uint8 ChannelIndex, boolean ActiveWakeup);
static void CanNm_EnterNormalOperationState(uint8 ChannelIndex);
static void CanNm_EnterReadySleepState(uint8 ChannelIndex);
static void CanNm_ProcessNmTimeoutTimer(uint8 ChannelIndex);
static void CanNm_ProcessMessageCycleTimer(uint8 ChannelIndex);
static void CanNm_ProcessRepeatMessageTimer(uint8 ChannelIndex);
static void CanNm_ProcessWaitBusSleepTimer(uint8 ChannelIndex);
static void CanNm_ProcessRemoteSleepIndication(uint8 ChannelIndex);
static void CanNm_NotifyStateChange(uint8 ChannelIndex, Nm_StateType State, Nm_ModeType Mode);

/*==================================================================================================
 *                                   LOCAL FUNCTIONS
 ==================================================================================================*/

/**
 * @brief Get channel index from network handle
 */
static uint8 CanNm_GetChannelIndex(NetworkHandleType nmChannelHandle)
{
    uint8 ChannelIndex;
    
    for (ChannelIndex = 0U; ChannelIndex < CANNM_NUMBER_OF_CHANNELS; ChannelIndex++)
    {
        if (CanNm_ConfigPtr->ChannelConfig[ChannelIndex].ChannelId == nmChannelHandle)
        {
            return ChannelIndex;
        }
    }
    return CANNM_INVALID_CHANNEL;
}

/**
 * @brief Validate channel and report DET error if needed
 */
static boolean CanNm_ValidateChannel(uint8 ChannelIndex, uint8 ApiId)
{
    if (ChannelIndex >= CANNM_NUMBER_OF_CHANNELS)
    {
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANNM_MODULE_ID, 0U, ApiId, CANNM_E_INVALID_CHANNEL);
#endif
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Update TX PDU buffer with current state
 */
static void CanNm_UpdateTxPdu(uint8 ChannelIndex)
{
    const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
    uint8 CbvValue = 0U;

    /* Clear buffer */
    memset(ChState->TxPduBuffer, 0U, CANNM_PDU_LENGTH);

    /* Set Node ID if enabled */
#if (CANNM_NODE_ID_ENABLED == STD_ON)
    if (ChCfg->NidPosition < CANNM_PDU_LENGTH)
    {
        ChState->TxPduBuffer[ChCfg->NidPosition] = ChCfg->NodeId;
    }
#endif

    /* Set CBV if enabled */
    if (ChCfg->CbvPosition < CANNM_PDU_LENGTH)
    {
        /* Set repeat message request bit if pending */
        if (ChState->RptMsgRequestPending)
        {
            CbvValue |= CANNM_CBV_REPEAT_MSG_BIT;
            ChState->RptMsgRequestPending = FALSE;
        }

#if (CANNM_COORDINATOR_SUPPORT_ENABLED == STD_ON)
        /* Set NM coordinator sleep bit if in ready sleep */
        if (ChState->State == CANNM_STATE_READY_SLEEP_MODE)
        {
            CbvValue |= CANNM_CBV_NM_COORD_SLEEP_BIT;
        }
#endif

#if (CANNM_ACTIVE_WAKEUP_BIT_ENABLED == STD_ON)
        /* Active wakeup bit is set in NetworkRequest context */
#endif

        ChState->TxPduBuffer[ChCfg->CbvPosition] = CbvValue;
    }

    /* Set default user data (can be overridden by CanNm_SetUserData) */
#if (CANNM_USER_DATA_ENABLED == STD_ON)
    {
        uint8 UserDataStart = CANNM_USER_DATA_POSITION;
        uint8 i;
        
        for (i = UserDataStart; i < CANNM_PDU_LENGTH; i++)
        {
            if ((i != ChCfg->NidPosition) && (i != ChCfg->CbvPosition))
            {
                ChState->TxPduBuffer[i] = 0x00U;
            }
        }
    }
#endif
}

/**
 * @brief Transmit NM message
 */
static void CanNm_TransmitMessage(uint8 ChannelIndex, boolean Immediate)
{
    const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
    PduInfoType PduInfo;
    Std_ReturnType Status;

    /* Update PDU buffer */
    CanNm_UpdateTxPdu(ChannelIndex);

    /* Prepare PDU info */
    PduInfo.SduDataPtr = ChState->TxPduBuffer;
    PduInfo.SduLength = ChCfg->PduLength;
    PduInfo.MetaDataPtr = NULL;

    /* Transmit via CanIf */
    Status = CanIf_Transmit(ChCfg->TxPduId, &PduInfo);

    if (Status == E_OK)
    {
        ChState->TxConfirmationPending = TRUE;
        
        /* Reset NM timeout timer on successful transmission */
        ChState->NmTimeoutTimer = ChCfg->NmTimeoutTime;

        /* Handle immediate transmission counter */
        if (Immediate && (ChState->ImmediateTxCounter > 0U))
        {
            ChState->ImmediateTxCounter--;
        }
    }
}

/**
 * @brief Notify state change to upper layer
 */
static void CanNm_NotifyStateChange(uint8 ChannelIndex, Nm_StateType State, Nm_ModeType Mode)
{
    (void)ChannelIndex; /* Unused for now */
    
    /* Call Nm_StateChangeNotification */
#if (CANNM_STATE_CHANGE_IND_ENABLED == STD_ON)
    Nm_StateChangeNotification(
        CanNm_ConfigPtr->ChannelConfig[ChannelIndex].ChannelId, 
        State, 
        Mode
    );
#endif

    /* Notify ComM of mode change */
    ComM_Nm_NetworkMode(CanNm_ConfigPtr->ChannelConfig[ChannelIndex].ChannelId);
}

/**
 * @brief Enter Bus Sleep Mode
 */
static void CanNm_EnterBusSleepMode(uint8 ChannelIndex)
{
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
    const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];

    ChState->State = CANNM_STATE_BUS_SLEEP_MODE;
    ChState->NetworkRequested = FALSE;
    ChState->CommunicationEnabled = TRUE;
    ChState->NmTimeoutTimer = 0U;
    ChState->MessageCycleTimer = 0U;
    ChState->RepeatMessageTimer = 0U;
    ChState->WaitBusSleepTimer = 0U;
    ChState->ImmediateTxCounter = 0U;
    ChState->MsgTxEnabled = FALSE;

    /* Notify ComM */
    ComM_Nm_BusSleepMode(ChCfg->ChannelId);

    /* Notify state change */
    CanNm_NotifyStateChange(ChannelIndex, NM_STATE_BUS_SLEEP, NM_MODE_BUS_SLEEP);
}

/**
 * @brief Enter Prepare Bus Sleep Mode
 */
static void CanNm_EnterPrepareBusSleepMode(uint8 ChannelIndex)
{
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
    const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];

    ChState->State = CANNM_STATE_PREPARE_BUS_SLEEP_MODE;
    ChState->WaitBusSleepTimer = ChCfg->WaitBusSleepTime;
    ChState->MsgTxEnabled = FALSE;

    /* Notify ComM */
    ComM_Nm_PrepareBusSleepMode(ChCfg->ChannelId);

    /* Notify state change */
    CanNm_NotifyStateChange(ChannelIndex, NM_STATE_PREPARE_BUS_SLEEP, NM_MODE_PREPARE_BUS_SLEEP);
}

/**
 * @brief Enter Repeat Message State
 */
static void CanNm_EnterRepeatMessageState(uint8 ChannelIndex, boolean ActiveWakeup)
{
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
    const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];

    ChState->State = CANNM_STATE_REPEAT_MESSAGE_MODE;
    ChState->RepeatMessageTimer = ChCfg->RepeatMessageTime;
    ChState->NmTimeoutTimer = ChCfg->NmTimeoutTime;
    ChState->MsgTxEnabled = TRUE;

    /* Reset remote sleep indication */
    ChState->RemoteSleepInd = FALSE;
    ChState->RemoteSleepIndEnabled = TRUE;

#if (CANNM_IMMEDIATE_TRANSMIT_ENABLED == STD_ON)
    /* Setup immediate transmissions */
    ChState->ImmediateTxCounter = ChCfg->ImmediateNmTransmissions;
    ChState->MessageCycleTimer = 0U; /* Transmit immediately */
#else
    ChState->MessageCycleTimer = ChCfg->MessageCycleTime;
#endif

    /* Set active wakeup bit in first message if active wakeup */
    if (ActiveWakeup && (ChCfg->CbvPosition < CANNM_PDU_LENGTH))
    {
        ChState->TxPduBuffer[ChCfg->CbvPosition] |= CANNM_CBV_ACTIVE_WAKEUP_BIT;
    }

    /* Notify ComM */
    ComM_Nm_NetworkMode(ChCfg->ChannelId);

    /* Notify state change */
    CanNm_NotifyStateChange(ChannelIndex, NM_STATE_REPEAT_MESSAGE, NM_MODE_NETWORK);
}

/**
 * @brief Enter Normal Operation State
 */
static void CanNm_EnterNormalOperationState(uint8 ChannelIndex)
{
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
    const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];

    ChState->State = CANNM_STATE_NORMAL_OPERATION_MODE;
    ChState->MsgTxEnabled = TRUE;
    ChState->MessageCycleTimer = ChCfg->MessageCycleTime;

    /* Notify state change */
    CanNm_NotifyStateChange(ChannelIndex, NM_STATE_NORMAL_OPERATION, NM_MODE_NETWORK);
}

/**
 * @brief Enter Ready Sleep State
 */
static void CanNm_EnterReadySleepState(uint8 ChannelIndex)
{
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
    const CanNm_ChannelConfigType* ChCfg ;

    ChState->State = CANNM_STATE_READY_SLEEP_MODE;
    ChState->MsgTxEnabled = FALSE;

    /* Notify state change */
    CanNm_NotifyStateChange(ChannelIndex, NM_STATE_READY_SLEEP, NM_MODE_NETWORK);

    /* Enable remote sleep indication detection */
    ChState->RemoteSleepIndEnabled = TRUE;
}

/**
 * @brief Process NM timeout timer
 */
static void CanNm_ProcessNmTimeoutTimer(uint8 ChannelIndex)
{
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];

    if (ChState->NmTimeoutTimer > 0U)
    {
        if (ChState->NmTimeoutTimer > CANNM_MAIN_FUNCTION_PERIOD_MS)
        {
            ChState->NmTimeoutTimer -= CANNM_MAIN_FUNCTION_PERIOD_MS;
        }
        else
        {
            ChState->NmTimeoutTimer = 0U;
            
            /* NM timeout expired - transition to Prepare Bus Sleep */
            if (ChState->State != CANNM_STATE_BUS_SLEEP_MODE)
            {
                CanNm_EnterPrepareBusSleepMode(ChannelIndex);
            }
        }
    }
}

/**
 * @brief Process message cycle timer (periodic transmission)
 */
static void CanNm_ProcessMessageCycleTimer(uint8 ChannelIndex)
{
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
    const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];

    if ((!ChState->MsgTxEnabled) || (!ChState->CommunicationEnabled))
    {
        return;
    }

#if (CANNM_PASSIVE_MODE_ENABLED == STD_OFF)
    if (ChState->MessageCycleTimer > 0U)
    {
        if (ChState->MessageCycleTimer > CANNM_MAIN_FUNCTION_PERIOD_MS)
        {
            ChState->MessageCycleTimer -= CANNM_MAIN_FUNCTION_PERIOD_MS;
        }
        else
        {
            ChState->MessageCycleTimer = 0U;
            
            /* Timer expired - transmit message */
            CanNm_TransmitMessage(ChannelIndex, FALSE);
            
            /* Restart timer */
            ChState->MessageCycleTimer = ChCfg->MessageCycleTime;
        }
    }
    else
    {
        /* Immediate transmission or first transmission */
        boolean Immediate = (ChState->ImmediateTxCounter > 0U);
        CanNm_TransmitMessage(ChannelIndex, Immediate);
        
        if (Immediate && (ChState->ImmediateTxCounter > 0U))
        {
            ChState->MessageCycleTimer = ChCfg->ImmediateNmCycleTime;
        }
        else
        {
            ChState->MessageCycleTimer = ChCfg->MessageCycleTime;
        }
    }
#endif
}

/**
 * @brief Process repeat message timer
 */
static void CanNm_ProcessRepeatMessageTimer(uint8 ChannelIndex)
{
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];

    if ((ChState->State == CANNM_STATE_REPEAT_MESSAGE_MODE) && 
        (ChState->RepeatMessageTimer > 0U))
    {
        if (ChState->RepeatMessageTimer > CANNM_MAIN_FUNCTION_PERIOD_MS)
        {
            ChState->RepeatMessageTimer -= CANNM_MAIN_FUNCTION_PERIOD_MS;
        }
        else
        {
            ChState->RepeatMessageTimer = 0U;
            
            /* Transition to Normal Operation or Ready Sleep */
            if (ChState->NetworkRequested)
            {
                CanNm_EnterNormalOperationState(ChannelIndex);
            }
            else
            {
                CanNm_EnterReadySleepState(ChannelIndex);
            }
        }
    }
}

/**
 * @brief Process wait bus sleep timer
 */
static void CanNm_ProcessWaitBusSleepTimer(uint8 ChannelIndex)
{
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];

    if ((ChState->State == CANNM_STATE_PREPARE_BUS_SLEEP_MODE) && 
        (ChState->WaitBusSleepTimer > 0U))
    {
        if (ChState->WaitBusSleepTimer > CANNM_MAIN_FUNCTION_PERIOD_MS)
        {
            ChState->WaitBusSleepTimer -= CANNM_MAIN_FUNCTION_PERIOD_MS;
        }
        else
        {
            ChState->WaitBusSleepTimer = 0U;
            
            /* Transition to Bus Sleep */
            CanNm_EnterBusSleepMode(ChannelIndex);
        }
    }
}

/**
 * @brief Process remote sleep indication
 */
static void CanNm_ProcessRemoteSleepIndication(uint8 ChannelIndex)
{
#if (CANNM_REMOTE_SLEEP_IND_ENABLED == STD_ON)
    CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
    const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];

    if ((ChState->State == CANNM_STATE_NORMAL_OPERATION_MODE) && 
        ChState->RemoteSleepIndEnabled && 
        (!ChState->RemoteSleepInd))
    {
        if (ChState->RemoteSleepIndTimer > 0U)
        {
            if (ChState->RemoteSleepIndTimer > CANNM_MAIN_FUNCTION_PERIOD_MS)
            {
                ChState->RemoteSleepIndTimer -= CANNM_MAIN_FUNCTION_PERIOD_MS;
            }
            else
            {
                ChState->RemoteSleepIndTimer = 0U;
                ChState->RemoteSleepInd = TRUE;

#if (CANNM_REMOTE_SLEEP_IND_CALLBACK == STD_ON)
                Nm_RemoteSleepIndication(ChCfg->ChannelId);
#endif
            }
        }
    }
#endif
}

/**
 * @brief Process state machine for a channel
 */
static void CanNm_ProcessStateMachine(uint8 ChannelIndex)
{
    const CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];

    switch (ChState->State)
    {
        case CANNM_STATE_BUS_SLEEP_MODE:
            /* Wait for network request or wakeup indication */
            break;

        case CANNM_STATE_PREPARE_BUS_SLEEP_MODE:
            /* Process wait bus sleep timer */
            CanNm_ProcessWaitBusSleepTimer(ChannelIndex);
            break;

        case CANNM_STATE_READY_SLEEP_MODE:
            /* Process NM timeout timer */
            CanNm_ProcessNmTimeoutTimer(ChannelIndex);
            break;

        case CANNM_STATE_REPEAT_MESSAGE_MODE:
            /* Process timers */
            CanNm_ProcessNmTimeoutTimer(ChannelIndex);
            CanNm_ProcessRepeatMessageTimer(ChannelIndex);
            CanNm_ProcessMessageCycleTimer(ChannelIndex);
            break;

        case CANNM_STATE_NORMAL_OPERATION_MODE:
            /* Process timers */
            CanNm_ProcessNmTimeoutTimer(ChannelIndex);
            CanNm_ProcessMessageCycleTimer(ChannelIndex);
            CanNm_ProcessRemoteSleepIndication(ChannelIndex);
            break;

        default:
            /* Should not reach here */
            break;
    }
}

/*==================================================================================================
 *                                   GLOBAL FUNCTIONS
 ==================================================================================================*/

/**
 * @brief Initialize the CAN NM module
 */
void CanNm_Init(const CanNm_ConfigType* ConfigPtr)
{
    uint8 ChannelIndex;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    /* Check if already initialized */
    if (CanNm_ModuleInitialized == CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_INIT, CANNM_E_ALREADY_INITIALIZED);
        return;
    }

    /* Check NULL pointer if configuration is required */
    if (ConfigPtr == NULL)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_INIT, CANNM_E_PARAM_POINTER);
        return;
    }
#else
    (void)ConfigPtr; /* ConfigPtr might not be used in variant builds */
#endif

    /* Use the link-time configuration */
    CanNm_ConfigPtr = &CanNm_Config;

    /* Initialize all channels */
    for (ChannelIndex = 0U; ChannelIndex < CANNM_NUMBER_OF_CHANNELS; ChannelIndex++)
    {
        CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
        const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];

        /* Clear state structure */
        memset(ChState, 0U, sizeof(CanNm_ChannelStateType));

        /* Set initial state */
        ChState->State = CANNM_STATE_BUS_SLEEP_MODE;
        ChState->CommunicationEnabled = TRUE;
        ChState->RemoteSleepIndEnabled = ChCfg->RemoteSleepIndEnabled;

        /* Initialize TX PDU buffer */
        memset(ChState->TxPduBuffer, 0U, CANNM_PDU_LENGTH);
        memset(ChState->RxPduBuffer, 0U, CANNM_PDU_LENGTH);
    }

    CanNm_ModuleInitialized = CANNM_MODULE_INITIALIZED;
}

/**
 * @brief De-initialize the CAN NM module
 */
void CanNm_DeInit(void)
{
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_DEINIT, CANNM_E_NOT_INITIALIZED);
        return;
    }
#endif

    /* Reset all channels to bus sleep */
    {
        uint8 ChannelIndex;
        for (ChannelIndex = 0U; ChannelIndex < CANNM_NUMBER_OF_CHANNELS; ChannelIndex++)
        {
            CanNm_EnterBusSleepMode(ChannelIndex);
        }
    }

    CanNm_ModuleInitialized = CANNM_MODULE_NOT_INITIALIZED;
    CanNm_ConfigPtr = NULL;
}

/**
 * @brief Passive startup of a CAN NM channel
 */
Std_ReturnType CanNm_PassiveStartUp(NetworkHandleType nmChannelHandle)
{
    uint8 ChannelIndex;
    Std_ReturnType Result = E_NOT_OK;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_PASSIVE_STARTUP, CANNM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif

    ChannelIndex = CanNm_GetChannelIndex(nmChannelHandle);
    
    if (!CanNm_ValidateChannel(ChannelIndex, CANNM_SID_PASSIVE_STARTUP))
    {
        return E_NOT_OK;
    }

    {
        CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];

        /* Can only start from Bus Sleep or Prepare Bus Sleep */
        if ((ChState->State == CANNM_STATE_BUS_SLEEP_MODE) ||
            (ChState->State == CANNM_STATE_PREPARE_BUS_SLEEP_MODE))
        {
            ChState->NetworkRequested = FALSE;
            CanNm_EnterRepeatMessageState(ChannelIndex, FALSE);
            Result = E_OK;
        }
    }

    return Result;
}

/**
 * @brief Request network mode on a channel
 */
Std_ReturnType CanNm_NetworkRequest(NetworkHandleType nmChannelHandle)
{
    uint8 ChannelIndex;
    Std_ReturnType Result = E_NOT_OK;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_NETWORK_REQUEST, CANNM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif

    ChannelIndex = CanNm_GetChannelIndex(nmChannelHandle);
    
    if (!CanNm_ValidateChannel(ChannelIndex, CANNM_SID_NETWORK_REQUEST))
    {
        return E_NOT_OK;
    }

    {
        CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];

        ChState->NetworkRequested = TRUE;

        switch (ChState->State)
        {
            case CANNM_STATE_BUS_SLEEP_MODE:
            case CANNM_STATE_PREPARE_BUS_SLEEP_MODE:
                /* Enter network mode with active wakeup */
                CanNm_EnterRepeatMessageState(ChannelIndex, TRUE);
                Result = E_OK;
                break;

            case CANNM_STATE_READY_SLEEP_MODE:
                /* Transition to Normal Operation */
                CanNm_EnterNormalOperationState(ChannelIndex);
                Result = E_OK;
                break;

            case CANNM_STATE_REPEAT_MESSAGE_MODE:
            case CANNM_STATE_NORMAL_OPERATION_MODE:
                /* Already in network mode */
                Result = E_OK;
                break;

            default:
                break;
        }
    }

    return Result;
}

/**
 * @brief Release network mode on a channel
 */
Std_ReturnType CanNm_NetworkRelease(NetworkHandleType nmChannelHandle)
{
    uint8 ChannelIndex;
    Std_ReturnType Result = E_NOT_OK;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_NETWORK_RELEASE, CANNM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif

    ChannelIndex = CanNm_GetChannelIndex(nmChannelHandle);
    
    if (!CanNm_ValidateChannel(ChannelIndex, CANNM_SID_NETWORK_RELEASE))
    {
        return E_NOT_OK;
    }

    {
        CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];

        ChState->NetworkRequested = FALSE;

        switch (ChState->State)
        {
            case CANNM_STATE_NORMAL_OPERATION_MODE:
                /* Transition to Ready Sleep */
                CanNm_EnterReadySleepState(ChannelIndex);
                Result = E_OK;
                break;

            case CANNM_STATE_REPEAT_MESSAGE_MODE:
            case CANNM_STATE_READY_SLEEP_MODE:
                /* No state change needed */
                Result = E_OK;
                break;

            case CANNM_STATE_BUS_SLEEP_MODE:
            case CANNM_STATE_PREPARE_BUS_SLEEP_MODE:
                /* Already released */
                Result = E_OK;
                break;

            default:
                break;
        }
    }

    return Result;
}

/**
 * @brief Main function for CAN NM
 */
void CanNm_MainFunction(void)
{
    uint8 ChannelIndex;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_MAIN_FUNCTION, CANNM_E_NOT_INITIALIZED);
        return;
    }
#endif

    /* Process all channels */
    for (ChannelIndex = 0U; ChannelIndex < CANNM_NUMBER_OF_CHANNELS; ChannelIndex++)
    {
        CanNm_ProcessStateMachine(ChannelIndex);
    }
}

/**
 * @brief Transmit function (for external PDU requests)
 */
Std_ReturnType CanNm_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    uint8 ChannelIndex;
    Std_ReturnType Result = E_NOT_OK;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_TRANSMIT, CANNM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_TRANSMIT, CANNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Find channel matching the TxPduId */
    for (ChannelIndex = 0U; ChannelIndex < CANNM_NUMBER_OF_CHANNELS; ChannelIndex++)
    {
        if (CanNm_ConfigPtr->ChannelConfig[ChannelIndex].TxPduId == TxPduId)
        {
            Result = CanIf_Transmit(TxPduId, PduInfoPtr);
            break;
        }
    }

    return Result;
}

/**
 * @brief RX indication callback from CAN Interface
 */
void CanNm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    uint8 ChannelIndex;
    boolean ChannelFound = FALSE;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_RX_INDICATION, CANNM_E_NOT_INITIALIZED);
        return;
    }

    if (PduInfoPtr == NULL)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_RX_INDICATION, CANNM_E_PARAM_POINTER);
        return;
    }
#endif

    /* Find channel matching the RxPduId */
    for (ChannelIndex = 0U; ChannelIndex < CANNM_NUMBER_OF_CHANNELS; ChannelIndex++)
    {
        if (CanNm_ConfigPtr->ChannelConfig[ChannelIndex].RxPduId == RxPduId)
        {
            ChannelFound = TRUE;
            break;
        }
    }

    if (ChannelFound == 0U)     {
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_RX_INDICATION, CANNM_E_INVALID_PDUID);
#endif
        return;
    }

    {
        CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
        const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];

        /* Copy received PDU to buffer */
        if (PduInfoPtr->SduDataPtr != NULL)
        {
            memcpy(ChState->RxPduBuffer, PduInfoPtr->SduDataPtr, 
                   (PduInfoPtr->SduLength < CANNM_PDU_LENGTH) ? PduInfoPtr->SduLength : CANNM_PDU_LENGTH);
        }

        /* Reset NM timeout timer on any NM message reception */
        ChState->NmTimeoutTimer = ChCfg->NmTimeoutTime;

        /* Cancel remote sleep indication if active */
        if (ChState->RemoteSleepInd)
        {
            ChState->RemoteSleepInd = FALSE;
            ChState->RemoteSleepIndTimer = CANNM_REMOTE_SLEEP_IND_TIME;

#if (CANNM_REMOTE_SLEEP_IND_CALLBACK == STD_ON)
            Nm_RemoteSleepCancellation(ChCfg->ChannelId);
#endif
        }

        /* Check for repeat message request in received message */
        if (ChCfg->CbvPosition < CANNM_PDU_LENGTH)
        {
            uint8 RxCbv = ChState->RxPduBuffer[ChCfg->CbvPosition];
            
            if ((RxCbv & CANNM_CBV_REPEAT_MSG_BIT) != 0U)
            {
                /* Repeat message request received */
                switch (ChState->State)
                {
                    case CANNM_STATE_NORMAL_OPERATION_MODE:
                    case CANNM_STATE_READY_SLEEP_MODE:
                        /* Transition to Repeat Message */
                        CanNm_EnterRepeatMessageState(ChannelIndex, FALSE);
                        break;

                    default:
                        break;
                }
            }
        }

        /* Process wakeup from Bus Sleep or Prepare Bus Sleep */
        switch (ChState->State)
        {
            case CANNM_STATE_BUS_SLEEP_MODE:
            case CANNM_STATE_PREPARE_BUS_SLEEP_MODE:
                /* Wakeup detected - enter Repeat Message State */
                CanNm_EnterRepeatMessageState(ChannelIndex, FALSE);
                
                /* Notify ComM of wakeup */
                ComM_EcuM_WakeUpIndication(ChCfg->ChannelId);
                break;

            default:
                break;
        }
    }
}

/**
 * @brief TX confirmation callback from CAN Interface
 */
void CanNm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    uint8 ChannelIndex;
    boolean ChannelFound = FALSE;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_TX_CONFIRMATION, CANNM_E_NOT_INITIALIZED);
        return;
    }
#endif

    /* Find channel matching the TxPduId */
    for (ChannelIndex = 0U; ChannelIndex < CANNM_NUMBER_OF_CHANNELS; ChannelIndex++)
    {
        if (CanNm_ConfigPtr->ChannelConfig[ChannelIndex].TxPduId == TxPduId)
        {
            ChannelFound = TRUE;
            break;
        }
    }

    if (ChannelFound == 0U)     {
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_TX_CONFIRMATION, CANNM_E_INVALID_PDUID);
#endif
        return;
    }

    {
        CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
        
        ChState->TxConfirmationPending = FALSE;

        if (result != E_OK)
        {
            /* Transmission failed - could trigger error handling */
        }
    }
}

/**
 * @brief Trigger transmit callback from PDU Router
 */
Std_ReturnType CanNm_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    uint8 ChannelIndex;
    Std_ReturnType Result = E_NOT_OK;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_GET_PDUDATA, CANNM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_GET_PDUDATA, CANNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Find channel matching the TxPduId */
    for (ChannelIndex = 0U; ChannelIndex < CANNM_NUMBER_OF_CHANNELS; ChannelIndex++)
    {
        if (CanNm_ConfigPtr->ChannelConfig[ChannelIndex].TxPduId == TxPduId)
        {
            const CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
            
            if ((PduInfoPtr->SduDataPtr != NULL) && (PduInfoPtr->SduLength >= CANNM_PDU_LENGTH))
            {
                /* Update PDU and copy to output */
                CanNm_UpdateTxPdu(ChannelIndex);
                memcpy(PduInfoPtr->SduDataPtr, ChState->TxPduBuffer, CANNM_PDU_LENGTH);
                Result = E_OK;
            }
            break;
        }
    }

    return Result;
}

/**
 * @brief Confirm partial networking availability
 */
void CanNm_ConfirmPnAvailability(NetworkHandleType nmChannelHandle)
{
#if (CANNM_PN_ENABLED == STD_ON)
    uint8 ChannelIndex;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_CONFIRM_PN_AVAILABILITY, CANNM_E_NOT_INITIALIZED);
        return;
    }
#endif

    ChannelIndex = CanNm_GetChannelIndex(nmChannelHandle);
    
    if (CanNm_ValidateChannel(ChannelIndex, CANNM_SID_CONFIRM_PN_AVAILABILITY))
    {
        /* Partial networking confirmation handling */
        /* This would typically set a flag for PN message handling */
    }
#else
    (void)nmChannelHandle;
#endif
}

/**
 * @brief Get current state of the CAN NM
 */
Std_ReturnType CanNm_GetState(NetworkHandleType nmChannelHandle, 
                               Nm_StateType* nmStatePtr, 
                               Nm_ModeType* nmModePtr)
{
    uint8 ChannelIndex;
    Std_ReturnType Result = E_NOT_OK;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_GET_STATE, CANNM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if ((nmStatePtr == NULL) || (nmModePtr == NULL))
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_GET_STATE, CANNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    ChannelIndex = CanNm_GetChannelIndex(nmChannelHandle);
    
    if (CanNm_ValidateChannel(ChannelIndex, CANNM_SID_GET_STATE))
    {
        const CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];

        /* Map internal state to Nm_StateType */
        switch (ChState->State)
        {
            case CANNM_STATE_BUS_SLEEP_MODE:
                *nmStatePtr = NM_STATE_BUS_SLEEP;
                *nmModePtr = NM_MODE_BUS_SLEEP;
                break;

            case CANNM_STATE_PREPARE_BUS_SLEEP_MODE:
                *nmStatePtr = NM_STATE_PREPARE_BUS_SLEEP;
                *nmModePtr = NM_MODE_PREPARE_BUS_SLEEP;
                break;

            case CANNM_STATE_READY_SLEEP_MODE:
                *nmStatePtr = NM_STATE_READY_SLEEP;
                *nmModePtr = NM_MODE_NETWORK;
                break;

            case CANNM_STATE_NORMAL_OPERATION_MODE:
                *nmStatePtr = NM_STATE_NORMAL_OPERATION;
                *nmModePtr = NM_MODE_NETWORK;
                break;

            case CANNM_STATE_REPEAT_MESSAGE_MODE:
                *nmStatePtr = NM_STATE_REPEAT_MESSAGE;
                *nmModePtr = NM_MODE_NETWORK;
                break;

            default:
                *nmStatePtr = NM_STATE_UNINIT;
                *nmModePtr = NM_MODE_BUS_SLEEP;
                break;
        }
        
        Result = E_OK;
    }

    return Result;
}

#if (CANNM_VERSION_INFO_API == STD_ON)
/**
 * @brief Get version information of CAN NM module
 */
void CanNm_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_GET_VERSION_INFO, CANNM_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = CANNM_VENDOR_ID;
    versioninfo->moduleID = CANNM_MODULE_ID;
    versioninfo->sw_major_version = CANNM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CANNM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CANNM_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Set user data for NM PDU
 */
Std_ReturnType CanNm_SetUserData(NetworkHandleType nmChannelHandle, const uint8* nmUserDataPtr)
{
    uint8 ChannelIndex;
    Std_ReturnType Result = E_NOT_OK;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_SET_USER_DATA, CANNM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (nmUserDataPtr == NULL)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_SET_USER_DATA, CANNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    ChannelIndex = CanNm_GetChannelIndex(nmChannelHandle);
    
    if (CanNm_ValidateChannel(ChannelIndex, CANNM_SID_SET_USER_DATA))
    {
#if (CANNM_USER_DATA_ENABLED == STD_ON)
        CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
        const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];
        uint8 i;
        uint8 UserDataIdx = 0U;

        /* Copy user data to TX buffer, skipping NID and CBV positions */
        for (i = 0U; i < CANNM_PDU_LENGTH; i++)
        {
            if ((i != ChCfg->NidPosition) && (i != ChCfg->CbvPosition))
            {
                ChState->TxPduBuffer[i] = nmUserDataPtr[UserDataIdx];
                UserDataIdx++;
                if (UserDataIdx >= CANNM_USER_DATA_LENGTH)
                {
                    break;
                }
            }
        }
        Result = E_OK;
#endif
    }

    return Result;
}

/**
 * @brief Get user data from NM PDU
 */
Std_ReturnType CanNm_GetUserData(NetworkHandleType nmChannelHandle, uint8* nmUserDataPtr)
{
    uint8 ChannelIndex;
    Std_ReturnType Result = E_NOT_OK;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_GET_USER_DATA, CANNM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (nmUserDataPtr == NULL)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_GET_USER_DATA, CANNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    ChannelIndex = CanNm_GetChannelIndex(nmChannelHandle);
    
    if (CanNm_ValidateChannel(ChannelIndex, CANNM_SID_GET_USER_DATA))
    {
#if (CANNM_USER_DATA_ENABLED == STD_ON)
        CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
        const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];
        uint8 i;
        uint8 UserDataIdx = 0U;

        /* Copy user data from RX buffer, skipping NID and CBV positions */
        for (i = 0U; i < CANNM_PDU_LENGTH; i++)
        {
            if ((i != ChCfg->NidPosition) && (i != ChCfg->CbvPosition))
            {
                nmUserDataPtr[UserDataIdx] = ChState->RxPduBuffer[i];
                UserDataIdx++;
                if (UserDataIdx >= CANNM_USER_DATA_LENGTH)
                {
                    break;
                }
            }
        }
        Result = E_OK;
#endif
    }

    return Result;
}

/**
 * @brief Set sleep ready bit in CBV
 */
Std_ReturnType CanNm_SetSleepReadyBit(NetworkHandleType nmChannelHandle, boolean nmSleepReadyBit)
{
    uint8 ChannelIndex;
    Std_ReturnType Result = E_NOT_OK;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_SET_SLEEP_READY_BIT, CANNM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif

    ChannelIndex = CanNm_GetChannelIndex(nmChannelHandle);
    
    if (CanNm_ValidateChannel(ChannelIndex, CANNM_SID_SET_SLEEP_READY_BIT))
    {
        CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];
        const CanNm_ChannelConfigType* ChCfg = &CanNm_ConfigPtr->ChannelConfig[ChannelIndex];

#if (CANNM_COORDINATOR_SUPPORT_ENABLED == STD_ON)
        if (ChCfg->CbvPosition < CANNM_PDU_LENGTH)
        {
            if (nmSleepReadyBit)
            {
                ChState->TxPduBuffer[ChCfg->CbvPosition] |= CANNM_CBV_NM_COORD_SLEEP_BIT;
            }
            else
            {
                ChState->TxPduBuffer[ChCfg->CbvPosition] &= ~CANNM_CBV_NM_COORD_SLEEP_BIT;
            }
            Result = E_OK;
        }
#else
        (void)nmSleepReadyBit;
        (void)ChState;
        (void)ChCfg;
#endif
    }

    return Result;
}

#if (CANNM_COM_CONTROL_ENABLED == STD_ON)
/**
 * @brief Disable NM PDU transmission
 */
Std_ReturnType CanNm_DisableCommunication(NetworkHandleType nmChannelHandle)
{
    uint8 ChannelIndex;
    Std_ReturnType Result = E_NOT_OK;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_DISABLE_COMMUNICATION, CANNM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif

    ChannelIndex = CanNm_GetChannelIndex(nmChannelHandle);
    
    if (CanNm_ValidateChannel(ChannelIndex, CANNM_SID_DISABLE_COMMUNICATION))
    {
        CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];

        if ((ChState->State == CANNM_STATE_REPEAT_MESSAGE_MODE) ||
            (ChState->State == CANNM_STATE_NORMAL_OPERATION_MODE))
        {
            ChState->CommunicationEnabled = FALSE;
            ChState->MsgTxEnabled = FALSE;
            Result = E_OK;
        }
    }

    return Result;
}

/**
 * @brief Enable NM PDU transmission
 */
Std_ReturnType CanNm_EnableCommunication(NetworkHandleType nmChannelHandle)
{
    uint8 ChannelIndex;
    Std_ReturnType Result = E_NOT_OK;

#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (CanNm_ModuleInitialized != CANNM_MODULE_INITIALIZED)
    {
        Det_ReportError(CANNM_MODULE_ID, 0U, CANNM_SID_ENABLE_COMMUNICATION, CANNM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif

    ChannelIndex = CanNm_GetChannelIndex(nmChannelHandle);
    
    if (CanNm_ValidateChannel(ChannelIndex, CANNM_SID_ENABLE_COMMUNICATION))
    {
        CanNm_ChannelStateType* ChState = &CanNm_ChannelStates[ChannelIndex];

        ChState->CommunicationEnabled = TRUE;
        
        if ((ChState->State == CANNM_STATE_REPEAT_MESSAGE_MODE) ||
            (ChState->State == CANNM_STATE_NORMAL_OPERATION_MODE))
        {
            ChState->MsgTxEnabled = TRUE;
        }
        
        Result = E_OK;
    }

    return Result;
}
#endif /* CANNM_COM_CONTROL_ENABLED */
