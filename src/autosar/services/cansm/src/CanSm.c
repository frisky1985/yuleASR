/**
 * @file CanSm.c
 * @brief CAN State Manager
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/**
     2| * @file CanSm.c
     3| * @brief CAN State Management module implementation following AutoSAR Classic Platform 4.x standard
     4| * @version 1.0.0
     5| * @date 2026-04-30
     6| * @author Shanghai Yule Electronics Technology Co., Ltd.
     7| * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
     8| *
     9| * AutoSAR Standard: CAN State Management (CanSM)
    10| * Layer: Service Layer
    11| */
    12|
    13|/*==================================================================================================
    14|*                                          INCLUDE FILES
    15|==================================================================================================*/
    16|#include "CanSm.h"
    17|#include "Det.h"
    18|
    19|/*==================================================================================================
    20|*                                    LOCAL DEFINES
    21|==================================================================================================*/
    22|/**
    23| * @brief Local defines for state machine processing
    24| */
    25|#define CANSM_UNINIT                            (0U)
    26|#define CANSM_INIT                              (1U)
    27|
    28|/**
    29| * @brief Mode transition timeouts (in main function ticks)
    30| */
    31|#define CANSM_NO_TRANSITION_TIMEOUT             (0xFFFFU)
    32|
    33|/**
    34| * @brief Invalid network handle
    35| */
    36|#define CANSM_INVALID_NETWORK_HANDLE            (0xFFU)
    37|
    38|/*==================================================================================================
    39|*                                    LOCAL TYPES
    40|==================================================================================================*/
    41|/**
    42| * @brief Internal state tracking for each network
    43| */
    44|typedef struct {
    45|    CanSm_BsmStateType BsmState;           /**< Current BSM state */
    46|    uint8 SubState;                         /**< Current sub-state */
    47|    ComM_ModeType RequestedComMMode;        /**< Requested ComM mode */
    48|    ComM_ModeType CurrentComMMode;          /**< Current ComM mode */
    49|    uint16 ModeRequestTimer;                /**< Mode request timeout timer */
    50|    uint16 BusOffRecoveryTimer;             /**< BusOff recovery timer */
    51|    uint8 BusOffCounter;                    /**< BusOff event counter */
    52|    boolean BusOffEventPending;             /**< BusOff event pending flag */
    53|    uint16 CurrentBaudrate;                 /**< Current baudrate */
    54|    uint8 RequestedBaudrateIndex;           /**< Requested baudrate index */
    55|    boolean BaudrateChangePending;          /**< Baudrate change pending */
    56|    CanIf_ControllerModeType RequestedCtrlMode; /**< Requested controller mode */
    57|    boolean ModeChangePending;              /**< Mode change pending flag */
    58|    boolean Initialized;                    /**< Network initialized flag */
    59|} CanSm_NetworkStateType;
    60|
    61|/**
    62| * @brief Module global state
    63| */
    64|typedef struct {
    65|    uint8 InitStatus;                       /**< Module initialization status */
    66|    CanSm_NetworkStateType Networks[CANSM_MAX_NETWORKS]; /**< Per-network states */
    67|    uint8 NumNetworks;                      /**< Number of configured networks */
    68|    const CanSm_ConfigType* ConfigPtr;      /**< Pointer to configuration */
    69|} CanSm_GlobalStateType;
    70|
    71|/*==================================================================================================
    72|*                                    LOCAL CONSTANTS
    73|==================================================================================================*/
    74|/**
    75| * @brief Baudrate configurations for each network
    76| */
    77|static const CanSm_BaudrateConfigType CanSm_BaudrateConfigs_Network0[] = {
    78|    { CANSM_BAUDRATE_125K,  0x0001U },
    79|    { CANSM_BAUDRATE_250K,  0x0002U },
    80|    { CANSM_BAUDRATE_500K,  0x0003U },
    81|    { CANSM_BAUDRATE_1000K, 0x0004U }
    82|};
    83|
    84|static const CanSm_BaudrateConfigType CanSm_BaudrateConfigs_Network1[] = {
    85|    { CANSM_BAUDRATE_125K,  0x0001U },
    86|    { CANSM_BAUDRATE_250K,  0x0002U },
    87|    { CANSM_BAUDRATE_500K,  0x0003U },
    88|    { CANSM_BAUDRATE_1000K, 0x0004U }
    89|};
    90|
    91|/*==================================================================================================
    92|*                                    LOCAL DATA
    93|==================================================================================================*/
    94|/**
    95| * @brief Module global state
    96| */
    97|static CanSm_GlobalStateType CanSm_Global;
    98|
    99|/**
   100| * @brief Network configurations
   101| */
   102|static const CanSm_NetworkConfigType CanSm_NetworkConfigs[CANSM_NUM_NETWORKS] = {
   103|    {   /* Network 0 - CAN0 */
   104|        .NetworkHandle = CANSM_NETWORK_CAN0,
   105|        .ControllerId = CANSM_CONTROLLER_CAN0,
   106|        .NumBaudrates = 4U,
   107|        .BaudrateConfigs = CanSm_BaudrateConfigs_Network0,
   108|        .MainFunctionPeriodMs = CANSM_NETWORK0_MAIN_FUNCTION_PERIOD_MS,
   109|        .BusOffRecoveryTimeMs = CANSM_NETWORK0_BUSOFF_RECOVERY_TIME_MS,
   110|        .BusOffThreshold = CANSM_BUSOFF_THRESHOLD,
   111|        .WakeupSupport = CANSM_NETWORK0_WAKEUP_SUPPORT,
   112|        .BusOffRecoveryEnabled = CANSM_NETWORK0_BUSOFF_RECOVERY_ENABLED,
   113|        .TransceiverSupport = CANSM_TRANSCEIVER_SUPPORT,
   114|        .TransceiverId = CANSM_TRANSCEIVER_CAN0
   115|    },
   116|    {   /* Network 1 - CAN1 */
   117|        .NetworkHandle = CANSM_NETWORK_CAN1,
   118|        .ControllerId = CANSM_CONTROLLER_CAN1,
   119|        .NumBaudrates = 4U,
   120|        .BaudrateConfigs = CanSm_BaudrateConfigs_Network1,
   121|        .MainFunctionPeriodMs = CANSM_NETWORK1_MAIN_FUNCTION_PERIOD_MS,
   122|        .BusOffRecoveryTimeMs = CANSM_NETWORK1_BUSOFF_RECOVERY_TIME_MS,
   123|        .BusOffThreshold = CANSM_BUSOFF_THRESHOLD,
   124|        .WakeupSupport = CANSM_NETWORK1_WAKEUP_SUPPORT,
   125|        .BusOffRecoveryEnabled = CANSM_NETWORK1_BUSOFF_RECOVERY_ENABLED,
   126|        .TransceiverSupport = CANSM_TRANSCEIVER_SUPPORT,
   127|        .TransceiverId = CANSM_TRANSCEIVER_CAN1
   128|    }
   129|};
   130|
   131|/*==================================================================================================
   132|*                                    LOCAL FUNCTION PROTOTYPES
   133|==================================================================================================*/
   134|static Std_ReturnType CanSm_ProcessNoComState(uint8 NetworkIndex);
   135|static Std_ReturnType CanSm_ProcessSilentComState(uint8 NetworkIndex);
   136|static Std_ReturnType CanSm_ProcessFullComState(uint8 NetworkIndex);
   137|static Std_ReturnType CanSm_ProcessSilentComBorState(uint8 NetworkIndex);
   138|static Std_ReturnType CanSm_RequestControllerMode(uint8 NetworkIndex, CanIf_ControllerModeType Mode);
   139|static boolean CanSm_IsNetworkValid(ComM_UserHandleType Network);
   140|static uint8 CanSm_GetNetworkIndex(ComM_UserHandleType Network);
   141|static void CanSm_HandleModeConfirmation(uint8 NetworkIndex, CanIf_ControllerModeType Mode);
   142|static void CanSm_HandleBusOffRecovery(uint8 NetworkIndex);
   143|static Std_ReturnType CanSm_TransitionToNoCom(uint8 NetworkIndex);
   144|static Std_ReturnType CanSm_TransitionToSilentCom(uint8 NetworkIndex);
   145|static Std_ReturnType CanSm_TransitionToFullCom(uint8 NetworkIndex);
   146|static void CanSm_StartTimer(uint8 NetworkIndex, uint16 TimeoutMs);
   147|static boolean CanSm_IsTimerExpired(uint8 NetworkIndex);
   148|
   149|/*==================================================================================================
   150|*                                    LOCAL FUNCTIONS
   151|==================================================================================================*/
   152|
   153|/**
   154| * @brief Checks if network handle is valid
   155| */
   156|static boolean CanSm_IsNetworkValid(ComM_UserHandleType Network)
   157|{
   158|    return (Network < CANSM_NUM_NETWORKS) ? TRUE : FALSE;
   159|}
   160|
   161|/**
   162| * @brief Gets network index from network handle
   163| */
   164|static uint8 CanSm_GetNetworkIndex(ComM_UserHandleType Network)
   165|{
   166|    return CanSm_IsNetworkValid(Network) ? (uint8)Network : CANSM_INVALID_NETWORK_HANDLE;
   167|}
   168|
   169|/**
   170| * @brief Starts a timer for mode transition timeout
   171| */
   172|static void CanSm_StartTimer(uint8 NetworkIndex, uint16 TimeoutMs)
   173|{
   174|    uint16 ticks;
   175|    const CanSm_NetworkConfigType* netConfig;
   176|    
   177|    netConfig = &CanSm_NetworkConfigs[NetworkIndex];
   178|    
   179|    /* Calculate ticks based on main function period */
   180|    ticks = (TimeoutMs + netConfig->MainFunctionPeriodMs - 1U) / netConfig->MainFunctionPeriodMs;
   181|    
   182|    CanSm_Global.Networks[NetworkIndex].ModeRequestTimer = ticks;
   183|}
   184|
   185|/**
   186| * @brief Checks if timer has expired
   187| */
   188|static boolean CanSm_IsTimerExpired(uint8 NetworkIndex)
   189|{
   190|    boolean expired = FALSE;
   191|    
   192|    if (CanSm_Global.Networks[NetworkIndex].ModeRequestTimer > 0U) {
   193|        CanSm_Global.Networks[NetworkIndex].ModeRequestTimer--;
   194|        if (CanSm_Global.Networks[NetworkIndex].ModeRequestTimer == 0U) {
   195|            expired = TRUE;
   196|        }
   197|    }
   198|    
   199|    return expired;
   200|}
   201|
   202|/**
   203| * @brief Requests controller mode from CanIf
   204| */
   205|static Std_ReturnType CanSm_RequestControllerMode(uint8 NetworkIndex, CanIf_ControllerModeType Mode)
   206|{
   207|    Std_ReturnType result;
   208|    const CanSm_NetworkConfigType* netConfig;
   209|    
   210|    netConfig = &CanSm_NetworkConfigs[NetworkIndex];
   211|    
   212|    result = CanIf_SetControllerMode(netConfig->ControllerId, Mode);
   213|    
   214|    if (result == E_OK) {
   215|        CanSm_Global.Networks[NetworkIndex].RequestedCtrlMode = Mode;
   216|        CanSm_Global.Networks[NetworkIndex].ModeChangePending = TRUE;
   217|        CanSm_StartTimer(NetworkIndex, CANSM_MODE_CHANGE_REQUEST_TIMEOUT_MS);
   218|    }
   219|    
   220|    return result;
   221|}
   222|
   223|/**
   224| * @brief Handles mode confirmation from CanIf
   225| */
   226|static void CanSm_HandleModeConfirmation(uint8 NetworkIndex, CanIf_ControllerModeType Mode)
   227|{
   228|    CanSm_Global.Networks[NetworkIndex].ModeChangePending = FALSE;
   229|    CanSm_Global.Networks[NetworkIndex].ModeRequestTimer = 0U;
   230|    
   231|    /* Update internal state based on confirmed mode */
   232|    switch (Mode) {
   233|        case CANIF_CS_STARTED:
   234|            /* Controller is now started - can transition to FULLCOM */
   235|            if (CanSm_Global.Networks[NetworkIndex].BsmState == CANSM_BSM_S_NOCOM) {
   236|                CanSm_TransitionToFullCom(NetworkIndex);
   237|            }
   238|            break;
   239|            
   240|        case CANIF_CS_STOPPED:
   241|            /* Controller is stopped - transition to SILENTCOM or NOCOM */
   242|            if (CanSm_Global.Networks[NetworkIndex].BsmState == CANSM_BSM_S_FULLCOM) {
   243|                CanSm_TransitionToSilentCom(NetworkIndex);
   244|            }
   245|            break;
   246|            
   247|        case CANIF_CS_SLEEP:
   248|            /* Controller is in sleep - transition to NOCOM */
   249|            if (CanSm_Global.Networks[NetworkIndex].BsmState != CANSM_BSM_S_NOCOM) {
   250|                CanSm_TransitionToNoCom(NetworkIndex);
   251|            }
   252|            break;
   253|            
   254|        case CANIF_CS_UNINIT:
   255|        default:
   256|            /* Do nothing */
   257|            break;
   258|    }
   259|}
   260|
   261|/**
   262| * @brief Handles BusOff recovery
   263| */
   264|static void CanSm_HandleBusOffRecovery(uint8 NetworkIndex)
   265|{
   266|    CanSm_NetworkStateType* netState;
   267|    const CanSm_NetworkConfigType* netConfig;
   268|    
   269|    netState = &CanSm_Global.Networks[NetworkIndex];
   270|    netConfig = &CanSm_NetworkConfigs[NetworkIndex];
   271|    
   272|    if (!netConfig->BusOffRecoveryEnabled) {
   273|        return;
   274|    }
   275|    
   276|    /* Increment BusOff counter */
   277|    netState->BusOffCounter++;
   278|    netState->BusOffEventPending = TRUE;
   279|    
   280|    /* Check if threshold exceeded */
   281|    if (netState->BusOffCounter >= netConfig->BusOffThreshold) {
   282|        /* Transition to SILENTCOM_BOR state */
   283|        netState->BsmState = CANSM_BSM_S_SILENTCOM_BOR;
   284|        netState->SubState = CANSM_S_BUSOFF_CHECK;
   285|        netState->BusOffRecoveryTimer = CANSM_BUSOFF_RECOVERY_L1_MS / netConfig->MainFunctionPeriodMs;
   286|        
   287|        /* Stop the controller */
   288|        (void)CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STOPPED);
   289|    } else {
   290|        /* Try immediate restart */
   291|        (void)CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STOPPED);
   292|        (void)CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STARTED);
   293|    }
   294|}
   295|
   296|/**
   297| * @brief Transitions network to NOCOM state
   298| */
   299|static Std_ReturnType CanSm_TransitionToNoCom(uint8 NetworkIndex)
   300|{
   301|    Std_ReturnType result = E_NOT_OK;
   302|    CanSm_NetworkStateType* netState;
   303|    
   304|    netState = &CanSm_Global.Networks[NetworkIndex];
   305|    
   306|    /* Reset BusOff counter */
   307|    netState->BusOffCounter = 0U;
   308|    netState->BusOffEventPending = FALSE;
   309|    
   310|    /* Set PDU mode to OFFLINE */
   311|    (void)CanIf_SetPduMode(CanSm_NetworkConfigs[NetworkIndex].ControllerId, CANIF_OFFLINE);
   312|    
   313|    /* Request controller sleep */
   314|    result = CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_SLEEP);
   315|    
   316|    if (result == E_OK) {
   317|        netState->BsmState = CANSM_BSM_S_NOCOM;
   318|        netState->SubState = CANSM_S_CC_SLEEP_WAIT;
   319|        netState->CurrentComMMode = COMM_NO_COMMUNICATION;
   320|    }
   321|    
   322|    return result;
   323|}
   324|
   325|/**
   326| * @brief Transitions network to SILENTCOM state
   327| */
   328|static Std_ReturnType CanSm_TransitionToSilentCom(uint8 NetworkIndex)
   329|{
   330|    Std_ReturnType result = E_NOT_OK;
   331|    CanSm_NetworkStateType* netState;
   332|    
   333|    netState = &CanSm_Global.Networks[NetworkIndex];
   334|    
   335|    /* Set PDU mode to TX_OFFLINE (listen only) */
   336|    (void)CanIf_SetPduMode(CanSm_NetworkConfigs[NetworkIndex].ControllerId, CANIF_TX_OFFLINE);
   337|    
   338|    /* Request controller stop (listen only mode) */
   339|    result = CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STOPPED);
   340|    
   341|    if (result == E_OK) {
   342|        netState->BsmState = CANSM_BSM_S_SILENTCOM;
   343|        netState->SubState = CANSM_S_SILENTCOM_NOP;
   344|        netState->CurrentComMMode = COMM_SILENT_COMMUNICATION;
   345|    }
   346|    
   347|    return result;
   348|}
   349|
   350|/**
   351| * @brief Transitions network to FULLCOM state
   352| */
   353|static Std_ReturnType CanSm_TransitionToFullCom(uint8 NetworkIndex)
   354|{
   355|    Std_ReturnType result = E_NOT_OK;
   356|    CanSm_NetworkStateType* netState;
   357|    
   358|    netState = &CanSm_Global.Networks[NetworkIndex];
   359|    
   360|    /* Set PDU mode to ONLINE */
   361|    (void)CanIf_SetPduMode(CanSm_NetworkConfigs[NetworkIndex].ControllerId, CANIF_ONLINE);
   362|    
   363|    /* Request controller start */
   364|    result = CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STARTED);
   365|    
   366|    if (result == E_OK) {
   367|        netState->BsmState = CANSM_BSM_S_FULLCOM;
   368|        netState->SubState = CANSM_S_FULLCOM_NOP;
   369|        netState->CurrentComMMode = COMM_FULL_COMMUNICATION;
   370|        
   371|        /* Clear BusOff counter */
   372|        netState->BusOffCounter = 0U;
   373|    }
   374|    
   375|    return result;
   376|}
   377|
   378|/**
   379| * @brief Process NOCOM state
   380| */
   381|static Std_ReturnType CanSm_ProcessNoComState(uint8 NetworkIndex)
   382|{
   383|    Std_ReturnType result = E_OK;
   384|    CanSm_NetworkStateType* netState;
   385|    ComM_ModeType requestedMode;
   386|    
   387|    netState = &CanSm_Global.Networks[NetworkIndex];
   388|    requestedMode = netState->RequestedComMMode;
   389|    
   390|    switch (netState->SubState) {
   391|        case CANSM_S_NOCOM_NOP:
   392|            /* Check if mode change requested */
   393|            if (requestedMode == COMM_SILENT_COMMUNICATION) {
   394|                result = CanSm_TransitionToSilentCom(NetworkIndex);
   395|            } else if (requestedMode == COMM_FULL_COMMUNICATION) {
   396|                /* Need to go through controller start sequence */
   397|                result = CanSm_RequestControllerMode(NetworkIndex, CANIF_CS_STARTED);
   398|                if (result == E_OK) {
   399|                    netState->SubState = CANSM_S_CC_START_WAIT;
   400|                }
   401|            }
   402|            break;
   403|            
   404|        case CANSM_S_CC_START_WAIT:
   405|            /* Waiting for controller mode confirmation */
   406|            if (CanSm_IsTimerExpired(NetworkIndex)) {
   407|                /* Timeout - retry or error */
   408|#if (CANSM_DEV_ERROR_DETECT == STD_ON)
   409|                Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, 
   410|                               CANSM_SID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
   411|#endif
   412|                result = E_NOT_OK;
   413|            }
   414|            break;
   415|            
   416|        case CANSM_S_CC_SLEEP_WAIT:
   417|            /* Waiting for sleep mode confirmation */
   418|            if (CanSm_IsTimerExpired(NetworkIndex)) {
   419|                /* Timeout - stay in NOCOM */
   420|                netState->SubState = CANSM_S_NOCOM_NOP;
   421|            }
   422|            break;
   423|            
   424|        default:
   425|            netState->SubState = CANSM_S_NOCOM_NOP;
   426|            break;
   427|    }
   428|    
   429|    return result;
   430|}
   431|
   432|/**
   433| * @brief Process SILENTCOM state
   434| */
   435|static Std_ReturnType CanSm_ProcessSilentComState(uint8 NetworkIndex)
   436|{
   437|    Std_ReturnType result = E_OK;
   438|    CanSm_NetworkStateType* netState;
   439|    ComM_ModeType requestedMode;
   440|    
   441|    netState = &CanSm_Global.Networks[NetworkIndex];
   442|    requestedMode = netState->RequestedComMMode;
   443|    
   444|    switch (netState->SubState) {
   445|        case CANSM_S_SILENTCOM_NOP:
   446|            /* Check if mode change requested */
   447|            if (requestedMode == COMM_NO_COMMUNICATION) {
   448|                result = CanSm_TransitionToNoCom(NetworkIndex);
   449|            } else if (requestedMode == COMM_FULL_COMMUNICATION) {
   450|                result = CanSm_TransitionToFullCom(NetworkIndex);
   451|            }
   452|            /* Stay in SILENTCOM otherwise (listen mode) */
   453|            break;
   454|            
   455|        case CANSM_S_CC_ONLINE:
   456|            /* Handle any ongoing transitions */
   457|            break;
   458|            
   459|        default:
   460|            netState->SubState = CANSM_S_SILENTCOM_NOP;
   461|            break;
   462|    }
   463|    
   464|    return result;
   465|}
   466|
   467|/**
   468| * @brief Process FULLCOM state
   469| */
   470|static Std_ReturnType CanSm_ProcessFullComState(uint8 NetworkIndex)
   471|{
   472|    Std_ReturnType result = E_OK;
   473|    CanSm_NetworkStateType* netState;
   474|    ComM_ModeType requestedMode;
   475|    
   476|    netState = &CanSm_Global.Networks[NetworkIndex];
   477|    requestedMode = netState->RequestedComMMode;
   478|    
   479|    switch (netState->SubState) {
   480|        case CANSM_S_FULLCOM_NOP:
   481|            /* Check if mode change requested */
   482|            if (requestedMode == COMM_NO_COMMUNICATION) {
   483|                result = CanSm_TransitionToNoCom(NetworkIndex);
   484|            } else if (requestedMode == COMM_SILENT_COMMUNICATION) {
   485|                result = CanSm_TransitionToSilentCom(NetworkIndex);
   486|            }
   487|            /* Stay in FULLCOM otherwise */
   488|            break;
   489|            
   490|        case CANSM_S_CC_START_WAIT:
   491|            /* Waiting for controller mode confirmation */
   492|            if (CanSm_IsTimerExpired(NetworkIndex)) {
   493|#if (CANSM_DEV_ERROR_DETECT == STD_ON)
   494|                Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, 
   495|                               CANSM_SID_MAINFUNCTION, CANSM_E_MODE_REQUEST_TIMEOUT);
   496|#endif
   497|                result = E_NOT_OK;
   498|            }
   499|            break;
   500|            
   501|