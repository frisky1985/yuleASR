/**
 * @file CanNm.c
 * @brief CAN Network Management Module Implementation
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * Implements OSEK NM protocol state machine and timer management
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "CanNm.h"
#include "Det.h"

/*==================================================================================================
*                                      LOCAL MACROS
==================================================================================================*/
#define CANNM_INITIALIZED                   (TRUE)
#define CANNM_NOT_INITIALIZED               (FALSE)

#define CANNM_CHANNEL_VALID(ch)             ((ch) < CANNM_NUMBER_OF_CHANNELS)
#define CANNM_CHANNEL_PTR(ch)               (&CanNm_Channels[ch])

#define CANNM_SET_CBV(pdu, bit)             ((pdu)[CANNM_PDU_BYTE_CBV] |= (bit))
#define CANNM_CLEAR_CBV(pdu, bit)           ((pdu)[CANNM_PDU_BYTE_CBV] &= ~(bit))
#define CANNM_IS_CBV_SET(pdu, bit)          (((pdu)[CANNM_PDU_BYTE_CBV] & (bit)) != 0)

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
#define CANNM_START_SEC_VAR_INIT_BOOLEAN
#include "MemMap.h"

static boolean CanNm_Initialized = CANNM_NOT_INITIALIZED;

#define CANNM_STOP_SEC_VAR_INIT_BOOLEAN
#include "MemMap.h"

#define CANNM_START_SEC_VAR_NOINIT_UNSPECIFIED
#include "MemMap.h"

static CanNm_ChannelType CanNm_Channels[CANNM_NUMBER_OF_CHANNELS];
static const CanNm_ConfigType *CanNm_ConfigPtr = NULL_PTR;

/* Global PDU info for transmission */
static PduInfoType CanNm_TxPduInfo[CANNM_NUMBER_OF_CHANNELS];

#define CANNM_STOP_SEC_VAR_NOINIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                      LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static void CanNm_StateMachine(CanNm_ChannelHandleType channel);
static void CanNm_Entry_BusSleep(CanNm_ChannelHandleType channel);
static void CanNm_Entry_RepeatMessage(CanNm_ChannelHandleType channel);
static void CanNm_Entry_NormalOperation(CanNm_ChannelHandleType channel);
static void CanNm_Entry_ReadySleep(CanNm_ChannelHandleType channel);
static void CanNm_Entry_PrepareBusSleep(CanNm_ChannelHandleType channel);
static void CanNm_ProcessTimers(CanNm_ChannelHandleType channel);
static void CanNm_TransmitMessage(CanNm_ChannelHandleType channel);
static void CanNm_ProcessPduData(CanNm_ChannelHandleType channel, const uint8 *pduData);
static void CanNm_ChangeState(CanNm_ChannelHandleType channel, 
                               CanNm_StateType newState, 
                               CanNm_ModeType newMode);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Change state and mode with callback notification
 */
static void CanNm_ChangeState(CanNm_ChannelHandleType channel, 
                               CanNm_StateType newState, 
                               CanNm_ModeType newMode)
{
    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
    CanNm_StateType oldState = chPtr->State;
    
    /* Store old state for notification */
    Nm_StateType nmOldState = (Nm_StateType)oldState;
    Nm_StateType nmNewState = (Nm_StateType)newState;
    Nm_ModeType nmNewMode = (Nm_ModeType)newMode;
    
    /* Update state and mode */
    chPtr->State = newState;
    chPtr->Mode = newMode;
    
    /* State specific entry actions */
    switch (newState) {
        case CANNM_STATE_BUS_SLEEP:
            CanNm_Entry_BusSleep(channel);
            break;
        case CANNM_STATE_REPEAT_MESSAGE:
            CanNm_Entry_RepeatMessage(channel);
            break;
        case CANNM_STATE_NORMAL_OPERATION:
            CanNm_Entry_NormalOperation(channel);
            break;
        case CANNM_STATE_READY_SLEEP:
            CanNm_Entry_ReadySleep(channel);
            break;
        case CANNM_STATE_PREPARE_BUS_SLEEP:
            CanNm_Entry_PrepareBusSleep(channel);
            break;
        default:
            break;
    }
    
    /* Notify upper layer of state change */
#if (CANNM_STATE_CHANGE_NOTIFICATION_ENABLED == STD_ON)
    Nm_StateChangeNotification(channel, nmOldState, nmNewState);
#endif
    
    /* Mode entry notifications */
    if (newMode == CANNM_MODE_BUS_SLEEP) {
#if (CANNM_BUS_SLEEP_MODE_ENTRY_ENABLED == STD_ON)
        Nm_BusSleepModeEntry(channel);
#endif
    } else if (newMode == CANNM_MODE_PREPARE_BUS_SLEEP) {
#if (CANNM_PREPARE_BUS_SLEEP_MODE_ENTRY_ENABLED == STD_ON)
        Nm_PrepareBusSleepModeEntry(channel);
#endif
    } else if (newMode == CANNM_MODE_NETWORK) {
        if (oldState == CANNM_STATE_BUS_SLEEP || oldState == CANNM_STATE_PREPARE_BUS_SLEEP) {
#if (CANNM_NETWORK_MODE_ENTRY_ENABLED == STD_ON)
            Nm_NetworkModeEntry(channel);
#endif
        }
    }
}

/**
 * @brief Bus Sleep state entry actions
 */
static void CanNm_Entry_BusSleep(CanNm_ChannelHandleType channel)
{
    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
    
    /* Stop all timers */
    chPtr->TimerNM = 0;
    chPtr->TimerTimeout = 0;
    chPtr->TimerWaitBusSleep = 0;
    chPtr->TimerRepeatMessage = 0;
    
    /* Clear flags */
    chPtr->RemoteSleepInd = FALSE;
    chPtr->LocalSleepInd = FALSE;
    
    /* Set passive startup pending */
    chPtr->NetworkRequested = FALSE;
}

/**
 * @brief Repeat Message state entry actions
 */
static void CanNm_Entry_RepeatMessage(CanNm_ChannelHandleType channel)
{
    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
    
    /* Start repeat message timer */
    chPtr->TimerRepeatMessage = cfgPtr->Timing->RepeatMessageTime;
    
    /* Start NM message timer */
    chPtr->TimerNM = cfgPtr->Timing->MsgCycleTime;
    
    /* Initialize immediate transmission if enabled */
#if (CANNM_IMMEDIATE_TRANSMISSION_ENABLED == STD_ON)
    chPtr->TimerImmediate = cfgPtr->Timing->ImmediateNmCycleTime;
    chPtr->ImmediateTxCounter = cfgPtr->Timing->ImmediateNmTransmissions;
#endif
    
    /* Set active wakeup bit in CBV */
    CANNM_SET_CBV(chPtr->TxPduData, CANNM_CBV_ACTIVE_WAKEUP);
    
    /* Clear timeout timer */
    chPtr->TimerTimeout = 0;
    chPtr->RemoteSleepInd = FALSE;
    
    /* Transmit first NM message */
    CanNm_TransmitMessage(channel);
}

/**
 * @brief Normal Operation state entry actions
 */
static void CanNm_Entry_NormalOperation(CanNm_ChannelHandleType channel)
{
    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
    
    /* Start NM message timer */
    chPtr->TimerNM = cfgPtr->Timing->MsgCycleTime;
    
    /* Clear repeat message timer */
    chPtr->TimerRepeatMessage = 0;
    chPtr->TimerImmediate = 0;
    chPtr->ImmediateTxCounter = 0;
    
    /* Clear active wakeup bit */
    CANNM_CLEAR_CBV(chPtr->TxPduData, CANNM_CBV_ACTIVE_WAKEUP);
    
    /* Clear timeout timer */
    chPtr->TimerTimeout = 0;
}

/**
 * @brief Ready Sleep state entry actions
 */
static void CanNm_Entry_ReadySleep(CanNm_ChannelHandleType channel)
{
    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
    
    /* Stop transmission - set timeout timer */
    chPtr->TimerNM = 0;
    chPtr->TimerImmediate = 0;
    chPtr->ImmediateTxCounter = 0;
    
    /* Start timeout timer for NM messages from other nodes */
    chPtr->TimerTimeout = cfgPtr->Timing->TimeoutTime;
    
    /* Clear timers */
    chPtr->TimerRepeatMessage = 0;
}

/**
 * @brief Prepare Bus Sleep state entry actions
 */
static void CanNm_Entry_PrepareBusSleep(CanNm_ChannelHandleType channel)
{
    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
    
    /* Stop all timers */
    chPtr->TimerNM = 0;
    chPtr->TimerTimeout = 0;
    chPtr->TimerRepeatMessage = 0;
    chPtr->TimerImmediate = 0;
    chPtr->ImmediateTxCounter = 0;
    
    /* Start wait bus sleep timer */
    chPtr->TimerWaitBusSleep = cfgPtr->Timing->WaitBusSleepTime;
    
    /* Set local sleep indication */
    chPtr->LocalSleepInd = TRUE;
}

/**
 * @brief Transmit NM message
 */
static void CanNm_TransmitMessage(CanNm_ChannelHandleType channel)
{
    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
    
    /* Set source address */
    chPtr->TxPduData[CANNM_PDU_BYTE_SRC_ADDR] = cfgPtr->NodeId;
    
    /* Setup PDU info */
    CanNm_TxPduInfo[channel].SduDataPtr = chPtr->TxPduData;
    CanNm_TxPduInfo[channel].SduLength = CANNM_PDU_LENGTH;
    
    /* Mark transmission pending - actual transmission in CanIf */
    chPtr->TxConfPending = TRUE;
    
    /* Call Nm callback before transmission */
    Nm_NetworkStartIndication(channel);
    
    /* Reset NM timer */
    chPtr->TimerNM = cfgPtr->Timing->MsgCycleTime;
}

/**
 * @brief Process received PDU data
 */
static void CanNm_ProcessPduData(CanNm_ChannelHandleType channel, const uint8 *pduData)
{
    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
    
    /* Copy received PDU data */
    for (uint8 i = 0; i < CANNM_PDU_LENGTH; i++) {
        chPtr->RxPduData[i] = pduData[i];
    }
    
    /* Check for repeat message request */
    if (CANNM_IS_CBV_SET(pduData, CANNM_CBV_REPEAT_MSG)) {
        if (chPtr->State == CANNM_STATE_NORMAL_OPERATION || 
            chPtr->State == CANNM_STATE_READY_SLEEP) {
            /* Transition to Repeat Message state */
            CanNm_ChangeState(channel, CANNM_STATE_REPEAT_MESSAGE, CANNM_MODE_NETWORK);
        }
    }
    
    /* Notify upper layer */
#if (CANNM_PDU_RX_INDICATION_ENABLED == STD_ON)
    Nm_RxIndication(channel, pduData);
#endif
    
    /* Clear remote sleep indication on any reception */
    if (chPtr->RemoteSleepInd) {
        chPtr->RemoteSleepInd = FALSE;
#if (CANNM_REMOTE_SLEEP_CALLBACK_ENABLED == STD_ON)
        Nm_RemoteSleepCancellation(channel);
#endif
    }
}

/**
 * @brief Process timers for a channel
 */
static void CanNm_ProcessTimers(CanNm_ChannelHandleType channel)
{
    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
    uint16 period = CANNM_MAIN_FUNCTION_PERIOD;
    
    /* NM Message Timer (TTyp) */
    if (chPtr->TimerNM > 0) {
        if (chPtr->TimerNM > period) {
            chPtr->TimerNM -= period;
        } else {
            chPtr->TimerNM = 0;
            /* Timer expired - transmit NM message */
            if (chPtr->State == CANNM_STATE_REPEAT_MESSAGE ||
                chPtr->State == CANNM_STATE_NORMAL_OPERATION) {
                CanNm_TransmitMessage(channel);
            }
        }
    }
    
    /* Immediate Transmission Timer (TTx) */
#if (CANNM_IMMEDIATE_TRANSMISSION_ENABLED == STD_ON)
    if (chPtr->TimerImmediate > 0 && chPtr->ImmediateTxCounter > 0) {
        if (chPtr->TimerImmediate > period) {
            chPtr->TimerImmediate -= period;
        } else {
            chPtr->TimerImmediate = cfgPtr->Timing->ImmediateNmCycleTime;
            chPtr->ImmediateTxCounter--;
            /* Transmit immediate NM message */
            CanNm_TransmitMessage(channel);
        }
    }
#endif
    
    /* Timeout Timer (TMax/TError) */
    if (chPtr->TimerTimeout > 0) {
        if (chPtr->TimerTimeout > period) {
            chPtr->TimerTimeout -= period;
        } else {
            chPtr->TimerTimeout = 0;
            /* Timeout expired - transition to Prepare Bus Sleep */
            if (chPtr->State == CANNM_STATE_READY_SLEEP) {
                CanNm_ChangeState(channel, CANNM_STATE_PREPARE_BUS_SLEEP, 
                                   CANNM_MODE_PREPARE_BUS_SLEEP);
            }
        }
    }
    
    /* Repeat Message Timer */
    if (chPtr->TimerRepeatMessage > 0) {
        if (chPtr->TimerRepeatMessage > period) {
            chPtr->TimerRepeatMessage -= period;
        } else {
            chPtr->TimerRepeatMessage = 0;
            /* Timer expired - check network request */
            if (chPtr->State == CANNM_STATE_REPEAT_MESSAGE) {
                if (chPtr->NetworkRequested) {
                    CanNm_ChangeState(channel, CANNM_STATE_NORMAL_OPERATION, 
                                       CANNM_MODE_NETWORK);
                } else {
                    CanNm_ChangeState(channel, CANNM_STATE_READY_SLEEP, 
                                       CANNM_MODE_NETWORK);
                }
            }
        }
    }
    
    /* Wait Bus Sleep Timer (TWbs) */
    if (chPtr->TimerWaitBusSleep > 0) {
        if (chPtr->TimerWaitBusSleep > period) {
            chPtr->TimerWaitBusSleep -= period;
        } else {
            chPtr->TimerWaitBusSleep = 0;
            /* Timer expired - transition to Bus Sleep */
            if (chPtr->State == CANNM_STATE_PREPARE_BUS_SLEEP) {
                CanNm_ChangeState(channel, CANNM_STATE_BUS_SLEEP, 
                                   CANNM_MODE_BUS_SLEEP);
            }
        }
    }
}

/**
 * @brief State machine processing
 */
static void CanNm_StateMachine(CanNm_ChannelHandleType channel)
{
    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
    
    /* Process state-specific logic */
    switch (chPtr->State) {
        case CANNM_STATE_BUS_SLEEP:
            /* In Bus Sleep, wait for network request or Rx indication */
            if (chPtr->RxIndPending) {
                chPtr->RxIndPending = FALSE;
                /* Transition to Repeat Message on reception */
                CanNm_ChangeState(channel, CANNM_STATE_REPEAT_MESSAGE, 
                                   CANNM_MODE_NETWORK);
            } else if (chPtr->NetworkRequested) {
                /* Transition to Repeat Message on request */
                CanNm_ChangeState(channel, CANNM_STATE_REPEAT_MESSAGE, 
                                   CANNM_MODE_NETWORK);
            }
            break;
            
        case CANNM_STATE_PREPARE_BUS_SLEEP:
            /* Wait for timer or network request */
            if (chPtr->NetworkRequested || chPtr->RxIndPending) {
                chPtr->RxIndPending = FALSE;
                /* Abort and go back to Network mode */
                CanNm_ChangeState(channel, CANNM_STATE_REPEAT_MESSAGE, 
                                   CANNM_MODE_NETWORK);
            }
            break;
            
        case CANNM_STATE_READY_SLEEP:
            /* Wait for timeout or network request */
            if (chPtr->NetworkRequested) {
                CanNm_ChangeState(channel, CANNM_STATE_NORMAL_OPERATION, 
                                   CANNM_MODE_NETWORK);
            }
            break;
            
        case CANNM_STATE_NORMAL_OPERATION:
            /* Check if network released */
            if (!chPtr->NetworkRequested) {
                CanNm_ChangeState(channel, CANNM_STATE_READY_SLEEP, 
                                   CANNM_MODE_NETWORK);
            }
            break;
            
        case CANNM_STATE_REPEAT_MESSAGE:
            /* Transitions handled by timer */
            break;
            
        default:
            break;
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize CAN NM module
 */
void CanNm_Init(const CanNm_ConfigType *ConfigPtr)
{
    CanNm_ChannelHandleType channel;
    const CanNm_ChannelConfigType *cfgPtr;
    
    /* Parameter check */
#if (CANNM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(CANNM_MODULE_ID, 0, CANNM_SID_INIT, CANNM_E_INVALID_POINTER);
        return;
    }
#endif
    
    CanNm_ConfigPtr = ConfigPtr;
    
    /* Initialize all channels */
    for (channel = 0; channel < CANNM_NUMBER_OF_CHANNELS; channel++) {
        cfgPtr = &ConfigPtr->ChannelConfig[channel];
        
        /* Initialize channel structure */
        CanNm_Channels[channel].State = CANNM_STATE_BUS_SLEEP;
        CanNm_Channels[channel].Mode = CANNM_MODE_BUS_SLEEP;
        CanNm_Channels[channel].TimerNM = 0;
        CanNm_Channels[channel].TimerTimeout = 0;
        CanNm_Channels[channel].TimerWaitBusSleep = 0;
        CanNm_Channels[channel].TimerRepeatMessage = 0;
        CanNm_Channels[channel].TimerImmediate = 0;
        CanNm_Channels[channel].ImmediateTxCounter = 0;
        CanNm_Channels[channel].NetworkRequested = FALSE;
        CanNm_Channels[channel].BusOff = FALSE;
        CanNm_Channels[channel].RemoteSleepInd = FALSE;
        CanNm_Channels[channel].LocalSleepInd = FALSE;
        CanNm_Channels[channel].RxIndPending = FALSE;
        CanNm_Channels[channel].TxConfPending = FALSE;
        
        /* Initialize PDU data */
        for (uint8 i = 0; i < CANNM_PDU_LENGTH; i++) {
            CanNm_Channels[channel].TxPduData[i] = 0U;
            CanNm_Channels[channel].RxPduData[i] = 0U;
        }
    }
}
