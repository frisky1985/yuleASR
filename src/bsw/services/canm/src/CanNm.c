/**
 * @file CanNm.c
 * @brief CAN Network Management
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/**
     2| * @file CanNm.c
     3| * @brief CAN Network Management Module Implementation
     4| * @version 1.0.0
     5| * @date 2026-04-29
     6| * @author Shanghai Yule Electronics Technology Co., Ltd.
     7| * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
     8| *
     9| * Implements OSEK NM protocol state machine and timer management
    10| */
    11|
    12|/*==================================================================================================
    13|*                                          INCLUDE FILES
    14|==================================================================================================*/
    15|#include "CanNm.h"
    16|#include "Det.h"
    17|
    18|/*==================================================================================================
    19|*                                      LOCAL MACROS
    20|==================================================================================================*/
    21|#define CANNM_INITIALIZED                   (TRUE)
    22|#define CANNM_NOT_INITIALIZED               (FALSE)
    23|
    24|#define CANNM_CHANNEL_VALID(ch)             ((ch) < CANNM_NUMBER_OF_CHANNELS)
    25|#define CANNM_CHANNEL_PTR(ch)               (&CanNm_Channels[ch])
    26|
    27|#define CANNM_SET_CBV(pdu, bit)             ((pdu)[CANNM_PDU_BYTE_CBV] |= (bit))
    28|#define CANNM_CLEAR_CBV(pdu, bit)           ((pdu)[CANNM_PDU_BYTE_CBV] &= ~(bit))
    29|#define CANNM_IS_CBV_SET(pdu, bit)          (((pdu)[CANNM_PDU_BYTE_CBV] & (bit)) != 0)
    30|
    31|/*==================================================================================================
    32|*                                      LOCAL VARIABLES
    33|==================================================================================================*/
    34|#define CANNM_START_SEC_VAR_INIT_BOOLEAN
    35|#include "MemMap.h"
    36|
    37|static boolean CanNm_Initialized = CANNM_NOT_INITIALIZED;
    38|
    39|#define CANNM_STOP_SEC_VAR_INIT_BOOLEAN
    40|#include "MemMap.h"
    41|
    42|#define CANNM_START_SEC_VAR_NOINIT_UNSPECIFIED
    43|#include "MemMap.h"
    44|
    45|static CanNm_ChannelType CanNm_Channels[CANNM_NUMBER_OF_CHANNELS];
    46|static const CanNm_ConfigType *CanNm_ConfigPtr = NULL_PTR;
    47|
    48|/* Global PDU info for transmission */
    49|static PduInfoType CanNm_TxPduInfo[CANNM_NUMBER_OF_CHANNELS];
    50|
    51|#define CANNM_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    52|#include "MemMap.h"
    53|
    54|/*==================================================================================================
    55|*                                      LOCAL FUNCTION PROTOTYPES
    56|==================================================================================================*/
    57|static void CanNm_StateMachine(CanNm_ChannelHandleType channel);
    58|static void CanNm_Entry_BusSleep(CanNm_ChannelHandleType channel);
    59|static void CanNm_Entry_RepeatMessage(CanNm_ChannelHandleType channel);
    60|static void CanNm_Entry_NormalOperation(CanNm_ChannelHandleType channel);
    61|static void CanNm_Entry_ReadySleep(CanNm_ChannelHandleType channel);
    62|static void CanNm_Entry_PrepareBusSleep(CanNm_ChannelHandleType channel);
    63|static void CanNm_ProcessTimers(CanNm_ChannelHandleType channel);
    64|static void CanNm_TransmitMessage(CanNm_ChannelHandleType channel);
    65|static void CanNm_ProcessPduData(CanNm_ChannelHandleType channel, const uint8 *pduData);
    66|static void CanNm_ChangeState(CanNm_ChannelHandleType channel, 
    67|                               CanNm_StateType newState, 
    68|                               CanNm_ModeType newMode);
    69|
    70|/*==================================================================================================
    71|*                                      LOCAL FUNCTIONS
    72|==================================================================================================*/
    73|
    74|/**
    75| * @brief Change state and mode with callback notification
    76| */
    77|static void CanNm_ChangeState(CanNm_ChannelHandleType channel, 
    78|                               CanNm_StateType newState, 
    79|                               CanNm_ModeType newMode)
    80|{
    81|    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
    82|    CanNm_StateType oldState = chPtr->State;
    83|    
    84|    /* Store old state for notification */
    85|    Nm_StateType nmOldState = (Nm_StateType)oldState;
    86|    Nm_StateType nmNewState = (Nm_StateType)newState;
    87|    Nm_ModeType nmNewMode = (Nm_ModeType)newMode;
    88|    
    89|    /* Update state and mode */
    90|    chPtr->State = newState;
    91|    chPtr->Mode = newMode;
    92|    
    93|    /* State specific entry actions */
    94|    switch (newState) {
    95|        case CANNM_STATE_BUS_SLEEP:
    96|            CanNm_Entry_BusSleep(channel);
    97|            break;
    98|        case CANNM_STATE_REPEAT_MESSAGE:
    99|            CanNm_Entry_RepeatMessage(channel);
   100|            break;
   101|        case CANNM_STATE_NORMAL_OPERATION:
   102|            CanNm_Entry_NormalOperation(channel);
   103|            break;
   104|        case CANNM_STATE_READY_SLEEP:
   105|            CanNm_Entry_ReadySleep(channel);
   106|            break;
   107|        case CANNM_STATE_PREPARE_BUS_SLEEP:
   108|            CanNm_Entry_PrepareBusSleep(channel);
   109|            break;
   110|        default:
   111|            break;
   112|    }
   113|    
   114|    /* Notify upper layer of state change */
   115|#if (CANNM_STATE_CHANGE_NOTIFICATION_ENABLED == STD_ON)
   116|    Nm_StateChangeNotification(channel, nmOldState, nmNewState);
   117|#endif
   118|    
   119|    /* Mode entry notifications */
   120|    if (newMode == CANNM_MODE_BUS_SLEEP) {
   121|#if (CANNM_BUS_SLEEP_MODE_ENTRY_ENABLED == STD_ON)
   122|        Nm_BusSleepModeEntry(channel);
   123|#endif
   124|    } else if (newMode == CANNM_MODE_PREPARE_BUS_SLEEP) {
   125|#if (CANNM_PREPARE_BUS_SLEEP_MODE_ENTRY_ENABLED == STD_ON)
   126|        Nm_PrepareBusSleepModeEntry(channel);
   127|#endif
   128|    } else if (newMode == CANNM_MODE_NETWORK) {
   129|        if (oldState == CANNM_STATE_BUS_SLEEP || oldState == CANNM_STATE_PREPARE_BUS_SLEEP) {
   130|#if (CANNM_NETWORK_MODE_ENTRY_ENABLED == STD_ON)
   131|            Nm_NetworkModeEntry(channel);
   132|#endif
   133|        }
   134|    }
   135|}
   136|
   137|/**
   138| * @brief Bus Sleep state entry actions
   139| */
   140|static void CanNm_Entry_BusSleep(CanNm_ChannelHandleType channel)
   141|{
   142|    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
   143|    
   144|    /* Stop all timers */
   145|    chPtr->TimerNM = 0;
   146|    chPtr->TimerTimeout = 0;
   147|    chPtr->TimerWaitBusSleep = 0;
   148|    chPtr->TimerRepeatMessage = 0;
   149|    
   150|    /* Clear flags */
   151|    chPtr->RemoteSleepInd = FALSE;
   152|    chPtr->LocalSleepInd = FALSE;
   153|    
   154|    /* Set passive startup pending */
   155|    chPtr->NetworkRequested = FALSE;
   156|}
   157|
   158|/**
   159| * @brief Repeat Message state entry actions
   160| */
   161|static void CanNm_Entry_RepeatMessage(CanNm_ChannelHandleType channel)
   162|{
   163|    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
   164|    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
   165|    
   166|    /* Start repeat message timer */
   167|    chPtr->TimerRepeatMessage = cfgPtr->Timing->RepeatMessageTime;
   168|    
   169|    /* Start NM message timer */
   170|    chPtr->TimerNM = cfgPtr->Timing->MsgCycleTime;
   171|    
   172|    /* Initialize immediate transmission if enabled */
   173|#if (CANNM_IMMEDIATE_TRANSMISSION_ENABLED == STD_ON)
   174|    chPtr->TimerImmediate = cfgPtr->Timing->ImmediateNmCycleTime;
   175|    chPtr->ImmediateTxCounter = cfgPtr->Timing->ImmediateNmTransmissions;
   176|#endif
   177|    
   178|    /* Set active wakeup bit in CBV */
   179|    CANNM_SET_CBV(chPtr->TxPduData, CANNM_CBV_ACTIVE_WAKEUP);
   180|    
   181|    /* Clear timeout timer */
   182|    chPtr->TimerTimeout = 0;
   183|    chPtr->RemoteSleepInd = FALSE;
   184|    
   185|    /* Transmit first NM message */
   186|    CanNm_TransmitMessage(channel);
   187|}
   188|
   189|/**
   190| * @brief Normal Operation state entry actions
   191| */
   192|static void CanNm_Entry_NormalOperation(CanNm_ChannelHandleType channel)
   193|{
   194|    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
   195|    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
   196|    
   197|    /* Start NM message timer */
   198|    chPtr->TimerNM = cfgPtr->Timing->MsgCycleTime;
   199|    
   200|    /* Clear repeat message timer */
   201|    chPtr->TimerRepeatMessage = 0;
   202|    chPtr->TimerImmediate = 0;
   203|    chPtr->ImmediateTxCounter = 0;
   204|    
   205|    /* Clear active wakeup bit */
   206|    CANNM_CLEAR_CBV(chPtr->TxPduData, CANNM_CBV_ACTIVE_WAKEUP);
   207|    
   208|    /* Clear timeout timer */
   209|    chPtr->TimerTimeout = 0;
   210|}
   211|
   212|/**
   213| * @brief Ready Sleep state entry actions
   214| */
   215|static void CanNm_Entry_ReadySleep(CanNm_ChannelHandleType channel)
   216|{
   217|    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
   218|    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
   219|    
   220|    /* Stop transmission - set timeout timer */
   221|    chPtr->TimerNM = 0;
   222|    chPtr->TimerImmediate = 0;
   223|    chPtr->ImmediateTxCounter = 0;
   224|    
   225|    /* Start timeout timer for NM messages from other nodes */
   226|    chPtr->TimerTimeout = cfgPtr->Timing->TimeoutTime;
   227|    
   228|    /* Clear timers */
   229|    chPtr->TimerRepeatMessage = 0;
   230|}
   231|
   232|/**
   233| * @brief Prepare Bus Sleep state entry actions
   234| */
   235|static void CanNm_Entry_PrepareBusSleep(CanNm_ChannelHandleType channel)
   236|{
   237|    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
   238|    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
   239|    
   240|    /* Stop all timers */
   241|    chPtr->TimerNM = 0;
   242|    chPtr->TimerTimeout = 0;
   243|    chPtr->TimerRepeatMessage = 0;
   244|    chPtr->TimerImmediate = 0;
   245|    chPtr->ImmediateTxCounter = 0;
   246|    
   247|    /* Start wait bus sleep timer */
   248|    chPtr->TimerWaitBusSleep = cfgPtr->Timing->WaitBusSleepTime;
   249|    
   250|    /* Set local sleep indication */
   251|    chPtr->LocalSleepInd = TRUE;
   252|}
   253|
   254|/**
   255| * @brief Transmit NM message
   256| */
   257|static void CanNm_TransmitMessage(CanNm_ChannelHandleType channel)
   258|{
   259|    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
   260|    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
   261|    
   262|    /* Set source address */
   263|    chPtr->TxPduData[CANNM_PDU_BYTE_SRC_ADDR] = cfgPtr->NodeId;
   264|    
   265|    /* Setup PDU info */
   266|    CanNm_TxPduInfo[channel].SduDataPtr = chPtr->TxPduData;
   267|    CanNm_TxPduInfo[channel].SduLength = CANNM_PDU_LENGTH;
   268|    
   269|    /* Mark transmission pending - actual transmission in CanIf */
   270|    chPtr->TxConfPending = TRUE;
   271|    
   272|    /* Call Nm callback before transmission */
   273|    Nm_NetworkStartIndication(channel);
   274|    
   275|    /* Reset NM timer */
   276|    chPtr->TimerNM = cfgPtr->Timing->MsgCycleTime;
   277|}
   278|
   279|/**
   280| * @brief Process received PDU data
   281| */
   282|static void CanNm_ProcessPduData(CanNm_ChannelHandleType channel, const uint8 *pduData)
   283|{
   284|    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
   285|    
   286|    /* Copy received PDU data */
   287|    for (uint8 i = 0; i < CANNM_PDU_LENGTH; i++) {
   288|        chPtr->RxPduData[i] = pduData[i];
   289|    }
   290|    
   291|    /* Check for repeat message request */
   292|    if (CANNM_IS_CBV_SET(pduData, CANNM_CBV_REPEAT_MSG)) {
   293|        if (chPtr->State == CANNM_STATE_NORMAL_OPERATION || 
   294|            chPtr->State == CANNM_STATE_READY_SLEEP) {
   295|            /* Transition to Repeat Message state */
   296|            CanNm_ChangeState(channel, CANNM_STATE_REPEAT_MESSAGE, CANNM_MODE_NETWORK);
   297|        }
   298|    }
   299|    
   300|    /* Notify upper layer */
   301|#if (CANNM_PDU_RX_INDICATION_ENABLED == STD_ON)
   302|    Nm_RxIndication(channel, pduData);
   303|#endif
   304|    
   305|    /* Clear remote sleep indication on any reception */
   306|    if (chPtr->RemoteSleepInd) {
   307|        chPtr->RemoteSleepInd = FALSE;
   308|#if (CANNM_REMOTE_SLEEP_CALLBACK_ENABLED == STD_ON)
   309|        Nm_RemoteSleepCancellation(channel);
   310|#endif
   311|    }
   312|}
   313|
   314|/**
   315| * @brief Process timers for a channel
   316| */
   317|static void CanNm_ProcessTimers(CanNm_ChannelHandleType channel)
   318|{
   319|    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
   320|    const CanNm_ChannelConfigType *cfgPtr = &CanNm_ConfigPtr->ChannelConfig[channel];
   321|    uint16 period = CANNM_MAIN_FUNCTION_PERIOD;
   322|    
   323|    /* NM Message Timer (TTyp) */
   324|    if (chPtr->TimerNM > 0) {
   325|        if (chPtr->TimerNM > period) {
   326|            chPtr->TimerNM -= period;
   327|        } else {
   328|            chPtr->TimerNM = 0;
   329|            /* Timer expired - transmit NM message */
   330|            if (chPtr->State == CANNM_STATE_REPEAT_MESSAGE ||
   331|                chPtr->State == CANNM_STATE_NORMAL_OPERATION) {
   332|                CanNm_TransmitMessage(channel);
   333|            }
   334|        }
   335|    }
   336|    
   337|    /* Immediate Transmission Timer (TTx) */
   338|#if (CANNM_IMMEDIATE_TRANSMISSION_ENABLED == STD_ON)
   339|    if (chPtr->TimerImmediate > 0 && chPtr->ImmediateTxCounter > 0) {
   340|        if (chPtr->TimerImmediate > period) {
   341|            chPtr->TimerImmediate -= period;
   342|        } else {
   343|            chPtr->TimerImmediate = cfgPtr->Timing->ImmediateNmCycleTime;
   344|            chPtr->ImmediateTxCounter--;
   345|            /* Transmit immediate NM message */
   346|            CanNm_TransmitMessage(channel);
   347|        }
   348|    }
   349|#endif
   350|    
   351|    /* Timeout Timer (TMax/TError) */
   352|    if (chPtr->TimerTimeout > 0) {
   353|        if (chPtr->TimerTimeout > period) {
   354|            chPtr->TimerTimeout -= period;
   355|        } else {
   356|            chPtr->TimerTimeout = 0;
   357|            /* Timeout expired - transition to Prepare Bus Sleep */
   358|            if (chPtr->State == CANNM_STATE_READY_SLEEP) {
   359|                CanNm_ChangeState(channel, CANNM_STATE_PREPARE_BUS_SLEEP, 
   360|                                   CANNM_MODE_PREPARE_BUS_SLEEP);
   361|            }
   362|        }
   363|    }
   364|    
   365|    /* Repeat Message Timer */
   366|    if (chPtr->TimerRepeatMessage > 0) {
   367|        if (chPtr->TimerRepeatMessage > period) {
   368|            chPtr->TimerRepeatMessage -= period;
   369|        } else {
   370|            chPtr->TimerRepeatMessage = 0;
   371|            /* Timer expired - check network request */
   372|            if (chPtr->State == CANNM_STATE_REPEAT_MESSAGE) {
   373|                if (chPtr->NetworkRequested) {
   374|                    CanNm_ChangeState(channel, CANNM_STATE_NORMAL_OPERATION, 
   375|                                       CANNM_MODE_NETWORK);
   376|                } else {
   377|                    CanNm_ChangeState(channel, CANNM_STATE_READY_SLEEP, 
   378|                                       CANNM_MODE_NETWORK);
   379|                }
   380|            }
   381|        }
   382|    }
   383|    
   384|    /* Wait Bus Sleep Timer (TWbs) */
   385|    if (chPtr->TimerWaitBusSleep > 0) {
   386|        if (chPtr->TimerWaitBusSleep > period) {
   387|            chPtr->TimerWaitBusSleep -= period;
   388|        } else {
   389|            chPtr->TimerWaitBusSleep = 0;
   390|            /* Timer expired - transition to Bus Sleep */
   391|            if (chPtr->State == CANNM_STATE_PREPARE_BUS_SLEEP) {
   392|                CanNm_ChangeState(channel, CANNM_STATE_BUS_SLEEP, 
   393|                                   CANNM_MODE_BUS_SLEEP);
   394|            }
   395|        }
   396|    }
   397|}
   398|
   399|/**
   400| * @brief State machine processing
   401| */
   402|static void CanNm_StateMachine(CanNm_ChannelHandleType channel)
   403|{
   404|    CanNm_ChannelType *chPtr = CANNM_CHANNEL_PTR(channel);
   405|    
   406|    /* Process state-specific logic */
   407|    switch (chPtr->State) {
   408|        case CANNM_STATE_BUS_SLEEP:
   409|            /* In Bus Sleep, wait for network request or Rx indication */
   410|            if (chPtr->RxIndPending) {
   411|                chPtr->RxIndPending = FALSE;
   412|                /* Transition to Repeat Message on reception */
   413|                CanNm_ChangeState(channel, CANNM_STATE_REPEAT_MESSAGE, 
   414|                                   CANNM_MODE_NETWORK);
   415|            } else if (chPtr->NetworkRequested) {
   416|                /* Transition to Repeat Message on request */
   417|                CanNm_ChangeState(channel, CANNM_STATE_REPEAT_MESSAGE, 
   418|                                   CANNM_MODE_NETWORK);
   419|            }
   420|            break;
   421|            
   422|        case CANNM_STATE_PREPARE_BUS_SLEEP:
   423|            /* Wait for timer or network request */
   424|            if (chPtr->NetworkRequested || chPtr->RxIndPending) {
   425|                chPtr->RxIndPending = FALSE;
   426|                /* Abort and go back to Network mode */
   427|                CanNm_ChangeState(channel, CANNM_STATE_REPEAT_MESSAGE, 
   428|                                   CANNM_MODE_NETWORK);
   429|            }
   430|            break;
   431|            
   432|        case CANNM_STATE_READY_SLEEP:
   433|            /* Wait for timeout or network request */
   434|            if (chPtr->NetworkRequested) {
   435|                CanNm_ChangeState(channel, CANNM_STATE_NORMAL_OPERATION, 
   436|                                   CANNM_MODE_NETWORK);
   437|            }
   438|            break;
   439|            
   440|        case CANNM_STATE_NORMAL_OPERATION:
   441|            /* Check if network released */
   442|            if (!chPtr->NetworkRequested) {
   443|                CanNm_ChangeState(channel, CANNM_STATE_READY_SLEEP, 
   444|                                   CANNM_MODE_NETWORK);
   445|            }
   446|            break;
   447|            
   448|        case CANNM_STATE_REPEAT_MESSAGE:
   449|            /* Transitions handled by timer */
   450|            break;
   451|            
   452|        default:
   453|            break;
   454|    }
   455|}
   456|
   457|/*==================================================================================================
   458|*                                      GLOBAL FUNCTIONS
   459|==================================================================================================*/
   460|
   461|/**
   462| * @brief Initialize CAN NM module
   463| */
   464|void CanNm_Init(const CanNm_ConfigType *ConfigPtr)
   465|{
   466|    CanNm_ChannelHandleType channel;
   467|    const CanNm_ChannelConfigType *cfgPtr;
   468|    
   469|    /* Parameter check */
   470|#if (CANNM_DEV_ERROR_DETECT == STD_ON)
   471|    if (ConfigPtr == NULL_PTR) {
   472|        Det_ReportError(CANNM_MODULE_ID, 0, CANNM_SID_INIT, CANNM_E_INVALID_POINTER);
   473|        return;
   474|    }
   475|#endif
   476|    
   477|    CanNm_ConfigPtr = ConfigPtr;
   478|    
   479|    /* Initialize all channels */
   480|    for (channel = 0; channel < CANNM_NUMBER_OF_CHANNELS; channel++) {
   481|        cfgPtr = &ConfigPtr->ChannelConfig[channel];
   482|        
   483|        /* Initialize channel structure */
   484|        CanNm_Channels[channel].State = CANNM_STATE_BUS_SLEEP;
   485|        CanNm_Channels[channel].Mode = CANNM_MODE_BUS_SLEEP;
   486|        CanNm_Channels[channel].TimerNM = 0;
   487|        CanNm_Channels[channel].TimerTimeout = 0;
   488|        CanNm_Channels[channel].TimerWaitBusSleep = 0;
   489|        CanNm_Channels[channel].TimerRepeatMessage = 0;
   490|        CanNm_Channels[channel].TimerImmediate = 0;
   491|        CanNm_Channels[channel].ImmediateTxCounter = 0;
   492|        CanNm_Channels[channel].NetworkRequested = FALSE;
   493|        CanNm_Channels[channel].BusOff = FALSE;
   494|        CanNm_Channels[channel].RemoteSleepInd = FALSE;
   495|        CanNm_Channels[channel].LocalSleepInd = FALSE;
   496|        CanNm_Channels[channel].RxIndPending = FALSE;
   497|        CanNm_Channels[channel].TxConfPending = FALSE;
   498|        
   499|        /* Initialize PDU data */
   500|        for (uint8 i = 0; i < CANNM_PDU_LENGTH; i++) {
   501|