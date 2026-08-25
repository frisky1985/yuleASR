/**
 * @file UdpNm.c
 * @brief UDP Network Management Implementation
 * @version 1.0.0
 * @date 2026-05-06
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: UDP Network Management (UdpNm)
 * Module ID: 0x33
 * Layer: Service Layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "UdpNm.h"
#include "UdpNm_Cfg.h"
#include "Det.h"
#include "Nm.h"
#include "MemMap.h"

/*==================================================================================================
*                                    FORWARD DECLARATIONS
*  Weak callback hooks referenced by UdpNm_Cfg.h macros before their definitions
*  Prevent implicit-declaration / conflicting-type errors with --coverage builds.
==================================================================================================*/
__attribute__((weak)) void Appl_UdpNm_StateChangeNotification(uint8 channel, UdpNm_StateType prev_state, UdpNm_StateType curr_state);
__attribute__((weak)) void Appl_UdpNm_RemoteSleepIndication(uint8 channel);
__attribute__((weak)) void Appl_UdpNm_RemoteSleepCancellation(uint8 channel);
__attribute__((weak)) void Appl_UdpNm_NetworkStartIndication(uint8 channel);
__attribute__((weak)) void Appl_UdpNm_NetworkModeEntry(uint8 channel);
__attribute__((weak)) void Appl_UdpNm_BusSleepModeEntry(uint8 channel);
__attribute__((weak)) void Appl_UdpNm_PrepareBusSleepModeEntry(uint8 channel);

/*==================================================================================================
*                                    LOCAL DEFINES
==================================================================================================*/
#define UDPNM_VENDOR_ID                         (0x01U)
#define UDPNM_INSTANCE_ID                       (0x00U)

/*==================================================================================================
*                                    LOCAL TYPEDEFS
==================================================================================================*/
/**
 * @brief UdpNm Internal Channel State Type
 */
typedef struct {
    UdpNm_StateType State;                  /**< Current state */
    UdpNm_ModeType Mode;                    /**< Current mode */
    UdpNm_TimerType TimerNM;                /**< NM message timer (TTyp) */
    UdpNm_TimerType TimerTimeout;           /**< Timeout timer (TMax/TError) */
    UdpNm_TimerType TimerWaitBusSleep;      /**< Wait bus sleep timer (TWbs) */
    UdpNm_TimerType TimerRepeatMessage;     /**< Repeat message timer */
    UdpNm_TimerType TimerImmediate;         /**< Immediate transmission timer */
    uint8 ImmediateTxCounter;               /**< Immediate transmission counter */
    boolean NetworkRequested;               /**< Network request flag */
    boolean CommunicationEnabled;           /**< Communication enabled flag */
    boolean RemoteSleepInd;                 /**< Remote sleep indication */
    boolean LocalSleepInd;                  /**< Local sleep indication */
    boolean SleepReadyBit;                  /**< Sleep ready bit */
    uint8 TxPduData[UDPNM_PDU_LENGTH];      /**< Tx PDU buffer */
    uint8 RxPduData[UDPNM_PDU_LENGTH];      /**< Rx PDU buffer */
    boolean RxIndPending;                   /**< Rx indication pending */
    boolean TxConfPending;                  /**< Tx confirmation pending */
    boolean Initialized;                    /**< Channel initialized flag */
} UdpNm_InternalChannelType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define UDPNM_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief Module initialized flag
 */
static boolean UdpNm_ModuleInitialized = FALSE;

/**
 * @brief Pointer to configuration
 */
static const UdpNm_ConfigType *UdpNm_ConfigPtr = NULL_PTR;

/**
 * @brief Internal channel states
 */
static UdpNm_InternalChannelType UdpNm_Channels[UDPNM_NUMBER_OF_CHANNELS];

#define UDPNM_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static void UdpNm_ProcessStateMachine(uint8 ChannelIdx);
static void UdpNm_TransmitMessage(uint8 ChannelIdx);
static void UdpNm_UpdateTimers(uint8 ChannelIdx);
static void UdpNm_ResetTimer(UdpNm_TimerType *Timer, uint16 Value);
static boolean UdpNm_IsTimerExpired(UdpNm_TimerType *Timer);
static void UdpNm_DecrementTimer(UdpNm_TimerType *Timer);
static void UdpNm_TransitionToState(uint8 ChannelIdx, UdpNm_StateType NewState);
static Std_ReturnType UdpNm_ValidateChannel(Nm_ChannelHandleType nmChannelHandle, uint8 ServiceId);
static void UdpNm_BuildPdu(uint8 ChannelIdx);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
#define UDPNM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Validates the channel handle
 * @param nmChannelHandle NM channel handle
 * @param ServiceId Service ID for DET reporting
 * @return E_OK if valid, E_NOT_OK otherwise
 */
static Std_ReturnType UdpNm_ValidateChannel(Nm_ChannelHandleType nmChannelHandle, uint8 ServiceId)
{
#if (UDPNM_DEV_ERROR_DETECT == STD_ON)
    if (UdpNm_ModuleInitialized == 0U)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, ServiceId, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }
    if (nmChannelHandle >= UDPNM_NUMBER_OF_CHANNELS)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, ServiceId, UDPNM_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#else
    (void)ServiceId;
#endif
    
    if (nmChannelHandle >= UDPNM_NUMBER_OF_CHANNELS)
    {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief Resets a timer to a specific value
 * @param Timer Pointer to timer
 * @param Value Value to set
 */
static void UdpNm_ResetTimer(UdpNm_TimerType *Timer, uint16 Value)
{
    *Timer = (UdpNm_TimerType)Value;
}

/**
 * @brief Checks if a timer has expired
 * @param Timer Pointer to timer
 * @return TRUE if expired, FALSE otherwise
 */
static boolean UdpNm_IsTimerExpired(UdpNm_TimerType *Timer)
{
    return (*Timer == 0U);
}

/**
 * @brief Decrements a timer by main function period
 * @param Timer Pointer to timer
 */
static void UdpNm_DecrementTimer(UdpNm_TimerType *Timer)
{
    if (*Timer >= UDPNM_MAIN_FUNCTION_PERIOD)
    {
        *Timer -= UDPNM_MAIN_FUNCTION_PERIOD;
    }
    else
    {
        *Timer = 0U;
    }
}

/**
 * @brief Updates all timers for a channel
 * @param ChannelIdx Channel index
 */
static void UdpNm_UpdateTimers(uint8 ChannelIdx)
{
    UdpNm_DecrementTimer(&UdpNm_Channels[ChannelIdx].TimerNM);
    UdpNm_DecrementTimer(&UdpNm_Channels[ChannelIdx].TimerTimeout);
    UdpNm_DecrementTimer(&UdpNm_Channels[ChannelIdx].TimerWaitBusSleep);
    UdpNm_DecrementTimer(&UdpNm_Channels[ChannelIdx].TimerRepeatMessage);
    UdpNm_DecrementTimer(&UdpNm_Channels[ChannelIdx].TimerImmediate);
}

/**
 * @brief Transitions a channel to a new state
 * @param ChannelIdx Channel index
 * @param NewState New state to transition to
 */
static void UdpNm_TransitionToState(uint8 ChannelIdx, UdpNm_StateType NewState)
{
    UdpNm_StateType PreviousState = UdpNm_Channels[ChannelIdx].State;
    
    if (PreviousState != NewState)
    {
        UdpNm_Channels[ChannelIdx].State = NewState;
        
        /* Update mode based on state */
        switch (NewState)
        {
            case UDPNM_STATE_BUS_SLEEP:
                UdpNm_Channels[ChannelIdx].Mode = UDPNM_MODE_BUS_SLEEP;
                UDPNM_BUS_SLEEP_MODE_ENTRY(ChannelIdx);
                break;
            case UDPNM_STATE_PREPARE_BUS_SLEEP:
                UdpNm_Channels[ChannelIdx].Mode = UDPNM_MODE_PREPARE_BUS_SLEEP;
                UDPNM_PREPARE_BUS_SLEEP_MODE_ENTRY(ChannelIdx);
                break;
            case UDPNM_STATE_REPEAT_MESSAGE:
            case UDPNM_STATE_NORMAL_OPERATION:
            case UDPNM_STATE_READY_SLEEP:
                UdpNm_Channels[ChannelIdx].Mode = UDPNM_MODE_NETWORK;
                break;
            default:
                /* No mode change for other states */
                break;
        }
        
        /* Notify state change */
        UDPNM_STATE_CHANGE_NOTIFICATION(ChannelIdx, PreviousState, NewState);
    }
}

/**
 * @brief Builds the NM PDU for transmission
 * @param ChannelIdx Channel index
 */
static void UdpNm_BuildPdu(uint8 ChannelIdx)
{
    const UdpNm_ChannelConfigType *ChannelConfig = &UdpNm_ConfigPtr->ChannelConfig[ChannelIdx];
    uint8 *PduData = UdpNm_Channels[ChannelIdx].TxPduData;
    
    /* Clear PDU data */
    for (uint8 i = 0U; i < UDPNM_PDU_LENGTH; i++)
    {
        PduData[i] = 0U;
    }
    
    /* Set Node ID */
#if (UDPNM_NODE_ID_ENABLED == STD_ON)
    if ((ChannelConfig->NodeIdEnabled) != 0U)
    {
        PduData[ChannelConfig->NodeIdPosition] = ChannelConfig->NodeId;
    }
#endif
    
    /* Set Control Bit Vector */
#if (UDPNM_CONTROL_BIT_VECTOR_ENABLED == STD_ON)
    if ((uint32_t)(ChannelConfig->ControlBitVectorPosition) < UDPNM_PDU_LENGTH)
    {
        uint8 CBV = 0U;
        
        /* Set active wakeup bit if network was requested */
        if ((UdpNm_Channels[ChannelIdx].NetworkRequested) != 0U)
        {
            CBV |= UDPNM_CBV_ACTIVE_WAKEUP;
        }
        
        /* Set sleep ready bit */
        if ((UdpNm_Channels[ChannelIdx].SleepReadyBit) != 0U)
        {
            CBV |= UDPNM_CBV_NM_COORD_SLEEP;
        }
        
        PduData[ChannelConfig->ControlBitVectorPosition] = CBV;
    }
#endif
    
    /* Copy user data */
#if (UDPNM_USER_DATA_ENABLED == STD_ON)
    if ((ChannelConfig->UserDataEnabled) != 0U)
    {
        /* User data is already in TxPduData buffer from SetUserData API */
    }
#endif
}

/**
 * @brief Transmits an NM message
 * @param ChannelIdx Channel index
 */
static void UdpNm_TransmitMessage(uint8 ChannelIdx)
{
    const UdpNm_ChannelConfigType *ChannelConfig = &UdpNm_ConfigPtr->ChannelConfig[ChannelIdx];
    
    /* Build PDU */
    UdpNm_BuildPdu(ChannelIdx);
    
    /* Transmit via SoAd */
    /* In real implementation, this would call SoAd APIs */
    /* For now, we just set the Tx confirmation pending flag */
    UdpNm_Channels[ChannelIdx].TxConfPending = TRUE;
    
    /* Reset NM timer */
    UdpNm_ResetTimer(&UdpNm_Channels[ChannelIdx].TimerNM, ChannelConfig->MsgCycleTime);
}

/**
 * @brief Processes the state machine for a channel
 * @param ChannelIdx Channel index
 */
static void UdpNm_ProcessStateMachine(uint8 ChannelIdx)
{
    const UdpNm_ChannelConfigType *ChannelConfig = &UdpNm_ConfigPtr->ChannelConfig[ChannelIdx];
    
    if (!UdpNm_Channels[ChannelIdx].CommunicationEnabled)
    {
        return;
    }
    
    switch (UdpNm_Channels[ChannelIdx].State)
    {
        case UDPNM_STATE_BUS_SLEEP:
            /* Wait for network request or Rx indication */
            if ((UdpNm_Channels[ChannelIdx].NetworkRequested) != 0U)
            {
                UdpNm_TransitionToState(ChannelIdx, UDPNM_STATE_REPEAT_MESSAGE);
                UdpNm_ResetTimer(&UdpNm_Channels[ChannelIdx].TimerRepeatMessage, 
                                 ChannelConfig->RepeatMessageTime);
                UdpNm_Channels[ChannelIdx].ImmediateTxCounter = ChannelConfig->ImmediateNmTransmissions;
            }
            else if ((UdpNm_Channels[ChannelIdx].RxIndPending) != 0U)
            {
                UdpNm_Channels[ChannelIdx].RxIndPending = FALSE;
                UDPNM_NETWORK_START_INDICATION(ChannelIdx);
                UdpNm_TransitionToState(ChannelIdx, UDPNM_STATE_REPEAT_MESSAGE);
                UdpNm_ResetTimer(&UdpNm_Channels[ChannelIdx].TimerRepeatMessage, 
                                 ChannelConfig->RepeatMessageTime);
            }
            break;
            
        case UDPNM_STATE_REPEAT_MESSAGE:
            /* Transmit NM messages with fast cycle time initially */
            if (UdpNm_Channels[ChannelIdx].ImmediateTxCounter > 0U)
            {
                if ((UdpNm_IsTimerExpired(&UdpNm_Channels[ChannelIdx].TimerImmediate)) != 0U)
                {
                    UdpNm_TransmitMessage(ChannelIdx);
                    UdpNm_Channels[ChannelIdx].ImmediateTxCounter--;
                    UdpNm_ResetTimer(&UdpNm_Channels[ChannelIdx].TimerImmediate, 
                                     ChannelConfig->ImmediateNmCycleTime);
                }
            }
            else if ((UdpNm_IsTimerExpired(&UdpNm_Channels[ChannelIdx].TimerNM)) != 0U)
            {
                UdpNm_TransmitMessage(ChannelIdx);
            }
            
            /* Check for repeat message timeout */
            if ((UdpNm_IsTimerExpired(&UdpNm_Channels[ChannelIdx].TimerRepeatMessage)) != 0U)
            {
                if ((UdpNm_Channels[ChannelIdx].NetworkRequested) != 0U)
                {
                    UdpNm_TransitionToState(ChannelIdx, UDPNM_STATE_NORMAL_OPERATION);
                }
                else
                {
                    UdpNm_TransitionToState(ChannelIdx, UDPNM_STATE_READY_SLEEP);
                }
            }
            break;
            
        case UDPNM_STATE_NORMAL_OPERATION:
            /* Transmit NM messages with normal cycle time */
            if ((UdpNm_IsTimerExpired(&UdpNm_Channels[ChannelIdx].TimerNM)) != 0U)
            {
                UdpNm_TransmitMessage(ChannelIdx);
            }
            
            /* Check for network release */
            if (!UdpNm_Channels[ChannelIdx].NetworkRequested)
            {
                UdpNm_TransitionToState(ChannelIdx, UDPNM_STATE_READY_SLEEP);
            }
            break;
            
        case UDPNM_STATE_READY_SLEEP:
            /* Continue to transmit NM messages */
            if ((UdpNm_IsTimerExpired(&UdpNm_Channels[ChannelIdx].TimerNM)) != 0U)
            {
                UdpNm_TransmitMessage(ChannelIdx);
            }
            
            /* Check timeout timer */
            if ((UdpNm_IsTimerExpired(&UdpNm_Channels[ChannelIdx].TimerTimeout)) != 0U)
            {
                /* No NM messages received from other nodes */
                UdpNm_TransitionToState(ChannelIdx, UDPNM_STATE_PREPARE_BUS_SLEEP);
                UdpNm_ResetTimer(&UdpNm_Channels[ChannelIdx].TimerWaitBusSleep, 
                                 ChannelConfig->WaitBusSleepTime);
            }
            
            /* Check for network request */
            if ((UdpNm_Channels[ChannelIdx].NetworkRequested) != 0U)
            {
                UdpNm_TransitionToState(ChannelIdx, UDPNM_STATE_NORMAL_OPERATION);
            }
            break;
            
        case UDPNM_STATE_PREPARE_BUS_SLEEP:
            /* Wait for bus sleep timeout */
            if ((UdpNm_IsTimerExpired(&UdpNm_Channels[ChannelIdx].TimerWaitBusSleep)) != 0U)
            {
                UdpNm_TransitionToState(ChannelIdx, UDPNM_STATE_BUS_SLEEP);
            }
            
            /* Check for network request */
            if ((UdpNm_Channels[ChannelIdx].NetworkRequested) != 0U)
            {
                UdpNm_TransitionToState(ChannelIdx, UDPNM_STATE_REPEAT_MESSAGE);
                UdpNm_ResetTimer(&UdpNm_Channels[ChannelIdx].TimerRepeatMessage, 
                                 ChannelConfig->RepeatMessageTime);
            }
            
            /* Check for Rx indication */
            if ((UdpNm_Channels[ChannelIdx].RxIndPending) != 0U)
            {
                UdpNm_Channels[ChannelIdx].RxIndPending = FALSE;
                UdpNm_TransitionToState(ChannelIdx, UDPNM_STATE_REPEAT_MESSAGE);
                UdpNm_ResetTimer(&UdpNm_Channels[ChannelIdx].TimerRepeatMessage, 
                                 ChannelConfig->RepeatMessageTime);
            }
            break;
            
        default:
            /* Invalid state - transition to bus sleep */
            UdpNm_TransitionToState(ChannelIdx, UDPNM_STATE_BUS_SLEEP);
            break;
    }
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the UDP Network Management module
 * @param ConfigPtr Pointer to configuration structure
 */
/** @req SWS_UdpNm_00001 */
void UdpNm_Init(const UdpNm_ConfigType *ConfigPtr)
{
#if (UDPNM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_INIT, UDPNM_E_INVALID_POINTER);
        return;
    }
#endif

    UdpNm_ConfigPtr = ConfigPtr;
    
    /* Initialize all channels */
    for (uint8 i = 0U; i < UDPNM_NUMBER_OF_CHANNELS; i++)
    {
        UdpNm_Channels[i].State = UDPNM_STATE_BUS_SLEEP;
        UdpNm_Channels[i].Mode = UDPNM_MODE_BUS_SLEEP;
        UdpNm_Channels[i].TimerNM = 0U;
        UdpNm_Channels[i].TimerTimeout = 0U;
        UdpNm_Channels[i].TimerWaitBusSleep = 0U;
        UdpNm_Channels[i].TimerRepeatMessage = 0U;
        UdpNm_Channels[i].TimerImmediate = 0U;
        UdpNm_Channels[i].ImmediateTxCounter = 0U;
        UdpNm_Channels[i].NetworkRequested = FALSE;
        UdpNm_Channels[i].CommunicationEnabled = TRUE;
        UdpNm_Channels[i].RemoteSleepInd = FALSE;
        UdpNm_Channels[i].LocalSleepInd = FALSE;
        UdpNm_Channels[i].SleepReadyBit = FALSE;
        UdpNm_Channels[i].RxIndPending = FALSE;
        UdpNm_Channels[i].TxConfPending = FALSE;
        UdpNm_Channels[i].Initialized = TRUE;
        
        /* Clear PDU data */
        for (uint8 j = 0U; j < UDPNM_PDU_LENGTH; j++)
        {
            UdpNm_Channels[i].TxPduData[j] = 0U;
            UdpNm_Channels[i].RxPduData[j] = 0U;
        }
    }
    
    UdpNm_ModuleInitialized = TRUE;
}

/**
 * @brief Deinitializes the UDP Network Management module
 */
/** @req SWS_UdpNm_00002 */
void UdpNm_DeInit(void)
{
#if (UDPNM_DEV_ERROR_DETECT == STD_ON)
    if (UdpNm_ModuleInitialized == 0U)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_DEINIT, UDPNM_E_UNINIT);
        return;
    }
#endif

    /* Deinitialize all channels */
    for (uint8 i = 0U; i < UDPNM_NUMBER_OF_CHANNELS; i++)
    {
        UdpNm_Channels[i].Initialized = FALSE;
        UdpNm_Channels[i].State = UDPNM_STATE_UNINIT;
    }
    
    UdpNm_ConfigPtr = NULL_PTR;
    UdpNm_ModuleInitialized = FALSE;
}

/**
 * @brief Passive startup of network management
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
/** @req SWS_UdpNm_00003 */
Std_ReturnType UdpNm_PassiveStartUp(Nm_ChannelHandleType nmChannelHandle)
{
    Std_ReturnType result = UdpNm_ValidateChannel(nmChannelHandle, UDPNM_SID_PASSIVESTARTUP);
    
    if (result == E_OK)
    {
        /* Passive startup - listen for NM messages without transmitting */
        /* Transition to Repeat Message state but don't set NetworkRequested */
        UdpNm_Channels[nmChannelHandle].State = UDPNM_STATE_REPEAT_MESSAGE;
        UdpNm_Channels[nmChannelHandle].Mode = UDPNM_MODE_NETWORK;
        UdpNm_ResetTimer(&UdpNm_Channels[nmChannelHandle].TimerRepeatMessage,
                         UdpNm_ConfigPtr->ChannelConfig[nmChannelHandle].RepeatMessageTime);
        
        UDPNM_NETWORK_MODE_ENTRY(nmChannelHandle);
    }
    
    return result;
}

/**
 * @brief Request the network
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
/** @req SWS_UdpNm_00004 */
Std_ReturnType UdpNm_NetworkRequest(Nm_ChannelHandleType nmChannelHandle)
{
    Std_ReturnType result = UdpNm_ValidateChannel(nmChannelHandle, UDPNM_SID_NETWORKREQUEST);
    
    if (result == E_OK)
    {
        UdpNm_Channels[nmChannelHandle].NetworkRequested = TRUE;
        
        /* Transition to Repeat Message state */
        UdpNm_TransitionToState(nmChannelHandle, UDPNM_STATE_REPEAT_MESSAGE);
        UdpNm_ResetTimer(&UdpNm_Channels[nmChannelHandle].TimerRepeatMessage,
                         UdpNm_ConfigPtr->ChannelConfig[nmChannelHandle].RepeatMessageTime);
        UdpNm_Channels[nmChannelHandle].ImmediateTxCounter = 
            UdpNm_ConfigPtr->ChannelConfig[nmChannelHandle].ImmediateNmTransmissions;
        UdpNm_ResetTimer(&UdpNm_Channels[nmChannelHandle].TimerImmediate,
                         UdpNm_ConfigPtr->ChannelConfig[nmChannelHandle].ImmediateNmCycleTime);
        
        UDPNM_NETWORK_MODE_ENTRY(nmChannelHandle);
    }
    
    return result;
}

/**
 * @brief Release the network
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
/** @req SWS_UdpNm_00005 */
Std_ReturnType UdpNm_NetworkRelease(Nm_ChannelHandleType nmChannelHandle)
{
    Std_ReturnType result = UdpNm_ValidateChannel(nmChannelHandle, UDPNM_SID_NETWORKRELEASE);
    
    if (result == E_OK)
    {
        UdpNm_Channels[nmChannelHandle].NetworkRequested = FALSE;
        
        /* Transition to Ready Sleep state */
        UdpNm_TransitionToState(nmChannelHandle, UDPNM_STATE_READY_SLEEP);
    }
    
    return result;
}

/**
 * @brief Disable NM PDU transmission
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
/** @req SWS_UdpNm_00006 */
Std_ReturnType UdpNm_DisableCommunication(Nm_ChannelHandleType nmChannelHandle)
{
    Std_ReturnType result = UdpNm_ValidateChannel(nmChannelHandle, UDPNM_SID_DISABLECOMMUNICATION);
    
    if (result == E_OK)
    {
        UdpNm_Channels[nmChannelHandle].CommunicationEnabled = FALSE;
    }
    
    return result;
}

/**
 * @brief Enable NM PDU transmission
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
/** @req SWS_UdpNm_00007 */
Std_ReturnType UdpNm_EnableCommunication(Nm_ChannelHandleType nmChannelHandle)
{
    Std_ReturnType result = UdpNm_ValidateChannel(nmChannelHandle, UDPNM_SID_ENABLECOMMUNICATION);
    
    if (result == E_OK)
    {
        UdpNm_Channels[nmChannelHandle].CommunicationEnabled = TRUE;
    }
    
    return result;
}

/**
 * @brief Get user data from last received NM message
 * @param nmChannelHandle NM channel handle
 * @param nmUserDataPtr Pointer to store user data
 * @return Result of operation
 */
/** @req SWS_UdpNm_00008 */
Std_ReturnType UdpNm_GetUserData(Nm_ChannelHandleType nmChannelHandle, uint8 *nmUserDataPtr)
{
    Std_ReturnType result = E_OK;
    
#if (UDPNM_DEV_ERROR_DETECT == STD_ON)
    if (UdpNm_ModuleInitialized == 0U)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_GETUSERDATA, UDPNM_E_UNINIT);
        result = E_NOT_OK;
    }
    else if (nmChannelHandle >= UDPNM_NUMBER_OF_CHANNELS)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_GETUSERDATA, UDPNM_E_INVALID_CHANNEL);
        result = E_NOT_OK;
    }
    else if (nmUserDataPtr == NULL_PTR)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_GETUSERDATA, UDPNM_E_INVALID_POINTER);
        result = E_NOT_OK;
    }
    else
#endif
    {
        const UdpNm_ChannelConfigType *ChannelConfig = &UdpNm_ConfigPtr->ChannelConfig[nmChannelHandle];
        
        /* Copy user data from Rx PDU */
        for (uint8 i = 0U; i < ChannelConfig->UserDataLength; i++)
        {
            nmUserDataPtr[i] = UdpNm_Channels[nmChannelHandle].RxPduData[ChannelConfig->UserDataOffset + i];
        }
    }
    
    return result;
}

/**
 * @brief Set user data for next NM message transmission
 * @param nmChannelHandle NM channel handle
 * @param nmUserDataPtr Pointer to user data
 * @return Result of operation
 */
/** @req SWS_UdpNm_00009 */
Std_ReturnType UdpNm_SetUserData(Nm_ChannelHandleType nmChannelHandle, const uint8 *nmUserDataPtr)
{
    Std_ReturnType result = E_OK;
    
#if (UDPNM_DEV_ERROR_DETECT == STD_ON)
    if (UdpNm_ModuleInitialized == 0U)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_SETUSERDATA, UDPNM_E_UNINIT);
        result = E_NOT_OK;
    }
    else if (nmChannelHandle >= UDPNM_NUMBER_OF_CHANNELS)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_SETUSERDATA, UDPNM_E_INVALID_CHANNEL);
        result = E_NOT_OK;
    }
    else if (nmUserDataPtr == NULL_PTR)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_SETUSERDATA, UDPNM_E_INVALID_POINTER);
        result = E_NOT_OK;
    }
    else
#endif
    {
        const UdpNm_ChannelConfigType *ChannelConfig = &UdpNm_ConfigPtr->ChannelConfig[nmChannelHandle];
        
        /* Copy user data to Tx PDU */
        for (uint8 i = 0U; i < ChannelConfig->UserDataLength; i++)
        {
            UdpNm_Channels[nmChannelHandle].TxPduData[ChannelConfig->UserDataOffset + i] = nmUserDataPtr[i];
        }
    }
    
    return result;
}

/**
 * @brief Get PDU data from last received NM message
 * @param nmChannelHandle NM channel handle
 * @param nmPduDataPtr Pointer to store PDU data
 * @return Result of operation
 */
/** @req SWS_UdpNm_00010 */
Std_ReturnType UdpNm_GetPduData(Nm_ChannelHandleType nmChannelHandle, uint8 *nmPduDataPtr)
{
    Std_ReturnType result = E_OK;
    
#if (UDPNM_DEV_ERROR_DETECT == STD_ON)
    if (UdpNm_ModuleInitialized == 0U)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_GETPDUDATA, UDPNM_E_UNINIT);
        result = E_NOT_OK;
    }
    else if (nmChannelHandle >= UDPNM_NUMBER_OF_CHANNELS)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_GETPDUDATA, UDPNM_E_INVALID_CHANNEL);
        result = E_NOT_OK;
    }
    else if (nmPduDataPtr == NULL_PTR)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_GETPDUDATA, UDPNM_E_INVALID_POINTER);
        result = E_NOT_OK;
    }
    else
#endif
    {
        /* Copy entire PDU data */
        for (uint8 i = 0U; i < UDPNM_PDU_LENGTH; i++)
        {
            nmPduDataPtr[i] = UdpNm_Channels[nmChannelHandle].RxPduData[i];
        }
    }
    
    return result;
}

/**
 * @brief Get current state and mode
 * @param nmChannelHandle NM channel handle
 * @param nmStatePtr Pointer to store state
 * @param nmModePtr Pointer to store mode
 * @return Result of operation
 */
Std_ReturnType UdpNm_GetState(Nm_ChannelHandleType nmChannelHandle, 
                               Nm_StateType *nmStatePtr, 
                               Nm_ModeType *nmModePtr)
{
    Std_ReturnType result = E_OK;
    
#if (UDPNM_DEV_ERROR_DETECT == STD_ON)
    if (UdpNm_ModuleInitialized == 0U)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_GETSTATE, UDPNM_E_UNINIT);
        result = E_NOT_OK;
    }
    else if (nmChannelHandle >= UDPNM_NUMBER_OF_CHANNELS)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_GETSTATE, UDPNM_E_INVALID_CHANNEL);
        result = E_NOT_OK;
    }
    else if ((nmStatePtr == NULL_PTR) || (nmModePtr == NULL_PTR))
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_GETSTATE, UDPNM_E_INVALID_POINTER);
        result = E_NOT_OK;
    }
    else
#endif
    {
        /* Map internal state/mode to Nm types */
        switch (UdpNm_Channels[nmChannelHandle].State)
        {
            case UDPNM_STATE_BUS_SLEEP:
                *nmStatePtr = NM_STATE_BUS_SLEEP;
                break;
            case UDPNM_STATE_PREPARE_BUS_SLEEP:
                *nmStatePtr = NM_STATE_PREPARE_BUS_SLEEP;
                break;
            case UDPNM_STATE_READY_SLEEP:
                *nmStatePtr = NM_STATE_READY_SLEEP;
                break;
            case UDPNM_STATE_NORMAL_OPERATION:
                *nmStatePtr = NM_STATE_NORMAL_OPERATION;
                break;
            case UDPNM_STATE_REPEAT_MESSAGE:
                *nmStatePtr = NM_STATE_REPEAT_MESSAGE;
                break;
            default:
                *nmStatePtr = NM_STATE_UNINIT;
                break;
        }
        
        switch (UdpNm_Channels[nmChannelHandle].Mode)
        {
            case UDPNM_MODE_BUS_SLEEP:
                *nmModePtr = NM_MODE_BUS_SLEEP;
                break;
            case UDPNM_MODE_PREPARE_BUS_SLEEP:
                *nmModePtr = NM_MODE_PREPARE_BUS_SLEEP;
                break;
            case UDPNM_MODE_NETWORK:
                *nmModePtr = NM_MODE_NETWORK;
                break;
            default:
                *nmModePtr = NM_MODE_BUS_SLEEP;
                break;
        }
    }
    
    return result;
}

/**
 * @brief Get version information
 * @param VersionInfoPtr Pointer to version info structure
 */
/** @req SWS_UdpNm_00012 */
void UdpNm_GetVersionInfo(Std_VersionInfoType *VersionInfoPtr)
{
#if (UDPNM_VERSION_INFO_API == STD_ON)
    if (VersionInfoPtr != NULL_PTR)
    {
        VersionInfoPtr->vendorID = UDPNM_VENDOR_ID;
        VersionInfoPtr->moduleID = UDPNM_MODULE_ID;
        VersionInfoPtr->sw_major_version = UDPNM_SW_MAJOR_VERSION;
        VersionInfoPtr->sw_minor_version = UDPNM_SW_MINOR_VERSION;
        VersionInfoPtr->sw_patch_version = UDPNM_SW_PATCH_VERSION;
    }
#endif
}

/**
 * @brief Request bus synchronization
 * @param nmChannelHandle NM channel handle
 * @return Result of operation
 */
/** @req SWS_UdpNm_00013 */
Std_ReturnType UdpNm_RequestBusSynchronization(Nm_ChannelHandleType nmChannelHandle)
{
#if (UDPNM_BUS_SYNCHRONIZATION_ENABLED == STD_ON)
    return UdpNm_ValidateChannel(nmChannelHandle, UDPNM_SID_REQUESTBUSSYNCHRONIZATION);
#else
    (void)nmChannelHandle;
    return E_NOT_OK;
#endif
}

/**
 * @brief Check remote sleep indication
 * @param nmChannelHandle NM channel handle
 * @param nmRemoteSleepIndPtr Pointer to store indication
 * @return Result of operation
 */
Std_ReturnType UdpNm_CheckRemoteSleepIndication(Nm_ChannelHandleType nmChannelHandle, 
                                                boolean *nmRemoteSleepIndPtr)
{
    Std_ReturnType result = E_OK;
    
#if (UDPNM_DEV_ERROR_DETECT == STD_ON)
    if (UdpNm_ModuleInitialized == 0U)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_CHECKREMOTESLEEPINDICATION, UDPNM_E_UNINIT);
        result = E_NOT_OK;
    }
    else if (nmChannelHandle >= UDPNM_NUMBER_OF_CHANNELS)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_CHECKREMOTESLEEPINDICATION, UDPNM_E_INVALID_CHANNEL);
        result = E_NOT_OK;
    }
    else if (nmRemoteSleepIndPtr == NULL_PTR)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_CHECKREMOTESLEEPINDICATION, UDPNM_E_INVALID_POINTER);
        result = E_NOT_OK;
    }
    else
#endif
    {
        *nmRemoteSleepIndPtr = UdpNm_Channels[nmChannelHandle].RemoteSleepInd;
    }
    
    return result;
}

/**
 * @brief Set sleep ready bit
 * @param nmChannelHandle NM channel handle
 * @param nmSleepReadyBit Sleep ready bit value
 * @return Result of operation
 */
Std_ReturnType UdpNm_SetSleepReadyBit(Nm_ChannelHandleType nmChannelHandle, 
                                       boolean nmSleepReadyBit)
{
    Std_ReturnType result = UdpNm_ValidateChannel(nmChannelHandle, UDPNM_SID_SETSLEEPREADYBIT);
    
    if (result == E_OK)
    {
        UdpNm_Channels[nmChannelHandle].SleepReadyBit = nmSleepReadyBit;
    }
    
    return result;
}

/**
 * @brief Transmit NM message
 * @param nmChannelHandle NM channel handle
 * @param PduInfoPtr Pointer to PDU info
 * @return Result of operation
 */
Std_ReturnType UdpNm_Transmit(Nm_ChannelHandleType nmChannelHandle, 
                               const PduInfoType *PduInfoPtr)
{
    Std_ReturnType result = E_OK;
    
#if (UDPNM_DEV_ERROR_DETECT == STD_ON)
    if (UdpNm_ModuleInitialized == 0U)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_TRANSMIT, UDPNM_E_UNINIT);
        result = E_NOT_OK;
    }
    else if (nmChannelHandle >= UDPNM_NUMBER_OF_CHANNELS)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_TRANSMIT, UDPNM_E_INVALID_CHANNEL);
        result = E_NOT_OK;
    }
    else if (PduInfoPtr == NULL_PTR)
    {
        Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID, UDPNM_SID_TRANSMIT, UDPNM_E_INVALID_POINTER);
        result = E_NOT_OK;
    }
    else
#endif
    {
        /* Copy PDU data to transmit buffer */
        if (PduInfoPtr->SduDataPtr != NULL_PTR)
        {
            for (uint8 i = 0U; i < UDPNM_PDU_LENGTH; i++)
            {
                if (i < PduInfoPtr->SduLength)
                {
                    UdpNm_Channels[nmChannelHandle].TxPduData[i] = PduInfoPtr->SduDataPtr[i];
                }
            }
        }
        
        UdpNm_TransmitMessage(nmChannelHandle);
    }
    
    return result;
}

/**
 * @brief Main function for periodic processing
 * Must be called cyclically with configured period
 */
/** @req SWS_UdpNm_00017 */
void UdpNm_MainFunction(void)
{
    if (UdpNm_ModuleInitialized == 0U)
    {
        return;
    }
    
    /* Process all channels */
    for (uint8 i = 0U; i < UDPNM_NUMBER_OF_CHANNELS; i++)
    {
        if ((UdpNm_Channels[i].Initialized) != 0U)
        {
            UdpNm_UpdateTimers(i);
            UdpNm_ProcessStateMachine(i);
        }
    }
}

/*==================================================================================================
*                                    CALLBACK FUNCTIONS
==================================================================================================*/

/**
 * @brief Tx confirmation callback from SoAd
 * @param UdpNmTxPduId PDU ID of transmitted NM message
 */
/** @req SWS_UdpNm_00100 */
void UdpNm_TxConfirmation(PduIdType UdpNmTxPduId)
{
    if (UdpNm_ModuleInitialized == 0U)
    {
        return;
    }
    
    /* Find channel by TxPduId */
    for (uint8 i = 0U; i < UDPNM_NUMBER_OF_CHANNELS; i++)
    {
        if (UdpNm_ConfigPtr->ChannelConfig[i].TxPduId == UdpNmTxPduId)
        {
            UdpNm_Channels[i].TxConfPending = FALSE;
            break;
        }
    }
}

/**
 * @brief Rx indication callback from SoAd
 * @param UdpNmRxPduId PDU ID of received NM message
 * @param PduInfoPtr Pointer to PDU info structure
 */
/** @req SWS_UdpNm_00101 */
void UdpNm_RxIndication(PduIdType UdpNmRxPduId, const PduInfoType *PduInfoPtr)
{
    if ((!UdpNm_ModuleInitialized) || (PduInfoPtr == NULL_PTR))
    {
        return;
    }
    
    /* Find channel by RxPduId */
    for (uint8 i = 0U; i < UDPNM_NUMBER_OF_CHANNELS; i++)
    {
        if (UdpNm_ConfigPtr->ChannelConfig[i].RxPduId == UdpNmRxPduId)
        {
            /* Copy received PDU data */
            if (PduInfoPtr->SduDataPtr != NULL_PTR)
            {
                for (uint8 j = 0U; j < UDPNM_PDU_LENGTH; j++)
                {
                    if (j < PduInfoPtr->SduLength)
                    {
                        UdpNm_Channels[i].RxPduData[j] = PduInfoPtr->SduDataPtr[j];
                    }
                    else
                    {
                        UdpNm_Channels[i].RxPduData[j] = 0U;
                    }
                }
            }
            
            /* Set Rx indication pending */
            UdpNm_Channels[i].RxIndPending = TRUE;
            
            /* Reset timeout timer */
            UdpNm_ResetTimer(&UdpNm_Channels[i].TimerTimeout, 
                            UdpNm_ConfigPtr->ChannelConfig[i].MsgTimeoutTime);
            
            break;
        }
    }
}

/**
 * @brief Remote sleep indication callback
 * @param nmChannelHandle NM channel handle
 */
/** @req SWS_UdpNm_00102 */
void UdpNm_RemoteSleepIndication(Nm_ChannelHandleType nmChannelHandle)
{
    if ((UdpNm_ModuleInitialized) && (nmChannelHandle < UDPNM_NUMBER_OF_CHANNELS))
    {
        UdpNm_Channels[nmChannelHandle].RemoteSleepInd = TRUE;
        UDPNM_REMOTE_SLEEP_INDICATION(nmChannelHandle);
    }
}

/**
 * @brief Remote sleep cancellation callback
 * @param nmChannelHandle NM channel handle
 */
/** @req SWS_UdpNm_00103 */
void UdpNm_RemoteSleepCancellation(Nm_ChannelHandleType nmChannelHandle)
{
    if ((UdpNm_ModuleInitialized) && (nmChannelHandle < UDPNM_NUMBER_OF_CHANNELS))
    {
        UdpNm_Channels[nmChannelHandle].RemoteSleepInd = FALSE;
        UDPNM_REMOTE_SLEEP_CANCELLATION(nmChannelHandle);
    }
}

#define UDPNM_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    WEAK CALLBACKS (Application)
==================================================================================================*/

__attribute__((weak)) void Appl_UdpNm_StateChangeNotification(uint8 channel, UdpNm_StateType prev_state, UdpNm_StateType curr_state)
{
    (void)channel;
    (void)prev_state;
    (void)curr_state;
}

__attribute__((weak)) void Appl_UdpNm_RemoteSleepIndication(uint8 channel)
{
    (void)channel;
}

__attribute__((weak)) void Appl_UdpNm_RemoteSleepCancellation(uint8 channel)
{
    (void)channel;
}

__attribute__((weak)) void Appl_UdpNm_NetworkStartIndication(uint8 channel)
{
    (void)channel;
}

__attribute__((weak)) void Appl_UdpNm_NetworkModeEntry(uint8 channel)
{
    (void)channel;
}

__attribute__((weak)) void Appl_UdpNm_BusSleepModeEntry(uint8 channel)
{
    (void)channel;
}

__attribute__((weak)) void Appl_UdpNm_PrepareBusSleepModeEntry(uint8 channel)
{
    (void)channel;
}
