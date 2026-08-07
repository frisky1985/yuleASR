/**
 * @file LinSM.c
 * @brief LIN State Manager implementation
 * @req SHALL_LINSM - LIN State Manager implementation
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: LIN State Manager (LinSM)
 * Layer: Service Layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "LinSM.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define LINSM_INVALID_CHANNEL               (0xFFU)
#define LINSM_INVALID_SCHEDULE              (0xFFU)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
/**
 * @brief LinSM Channel State structure
 * @req SHALL_LINSM - LinSM Channel State structure
 */
typedef struct {
    LinSM_StateType State;                  /*!< Current state */
    LinSM_ModeType ComMode;                 /*!< Communication mode */
    LinSM_ScheduleType CurrentSchedule;     /*!< Current schedule */
    LinSM_ScheduleType RequestedSchedule;   /*!< Requested schedule */
    LinSM_ScheduleStatusType ScheduleStatus; /*!< Schedule status */
    uint16 RequestTimer;                    /*!< Request timeout timer */
    uint8 ScheduleSwitchCount;              /*!< Schedule switch counter */
    boolean Initialized;                    /*!< Initialization flag */
    boolean WakeupPending;                  /*!< Wakeup pending flag */
    boolean GotosleepPending;               /*!< Go-to-sleep pending flag */
} LinSM_ChannelStateType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define LINSM_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC boolean LinSM_Initialized = FALSE;
STATIC const LinSM_ConfigType* LinSM_ConfigPtr = NULL_PTR;

#define LINSM_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

#define LINSM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC LinSM_ChannelStateType LinSM_ChannelStates[LINSM_NUMBER_OF_CHANNELS];

#define LINSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Version check */
#if defined(LINSM_AR_RELEASE_MAJOR_VERSION) && (LINSM_AR_RELEASE_MAJOR_VERSION != 4u)
#error "LinSM: AR major mismatch"
#endif
#if defined(LINSM_AR_RELEASE_MINOR_VERSION) && (LINSM_AR_RELEASE_MINOR_VERSION != 4u)
#error "LinSM: AR minor mismatch"
#endif

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType LinSM_ValidateChannel(LinSM_ChannelType Channel);
STATIC void LinSM_ProcessStateMachine(LinSM_ChannelType Channel);
STATIC void LinSM_HandleScheduleRequest(LinSM_ChannelType Channel);
STATIC void LinSM_HandleModeRequest(LinSM_ChannelType Channel);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
/**
 * @brief Validates the channel ID
 * @req SHALL_LINSM - Validates the channel ID
 */
STATIC Std_ReturnType LinSM_ValidateChannel(LinSM_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (Channel < LINSM_NUMBER_OF_CHANNELS) {
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Handles schedule request
 * @req SHALL_LINSM - Handles schedule request
 */
STATIC void LinSM_HandleScheduleRequest(LinSM_ChannelType Channel)
{
    LinSM_ChannelStateType* channelState = &LinSM_ChannelStates[Channel];
    const LinSM_ChannelConfigType* channelConfig ;
    
    if ((uint8_t)(channelState->ScheduleStatus) == LINSM_SCHEDULE_REQUESTED) {
        /* Check if request timeout expired */
        if (channelState->RequestTimer == 0U) {
            /* Timeout, schedule request failed */
            channelState->ScheduleStatus = LINSM_SCHEDULE_NULL;
        } else {
            channelState->RequestTimer--;
        }
    }
}

/**
 * @brief Handles mode request
 * @req SHALL_LINSM - Handles mode request
 */
STATIC void LinSM_HandleModeRequest(LinSM_ChannelType Channel)
{
    LinSM_ChannelStateType* channelState = &LinSM_ChannelStates[Channel];
    
    if (channelState->WakeupPending) {
        /* Process wakeup */
        if (channelState->State == LINSM_STATE_WAKEUP) {
            /* Wait for wakeup confirmation */
        }
    }
    
    if (channelState->GotosleepPending) {
        /* Process go-to-sleep */
        if (channelState->State == LINSM_STATE_GOTOSLEEP) {
            /* Wait for go-to-sleep confirmation */
        }
    }
    
    (void)Channel;
}

/**
 * @brief Processes the state machine
 * @req SHALL_LINSM - Processes the state machine
 */
STATIC void LinSM_ProcessStateMachine(LinSM_ChannelType Channel)
{
    LinSM_ChannelStateType* channelState = &LinSM_ChannelStates[Channel];
    
    switch (channelState->State) {
        case LINSM_STATE_UNINIT:
            /* Do nothing, wait for initialization */
            break;
            
        case LINSM_STATE_INIT:
            /* Initialization complete, transition to RUN */
            channelState->State = LINSM_STATE_RUN;
            channelState->ComMode = LINSM_FULL_COM;
            break;
            
        case LINSM_STATE_RUN:
            /* Normal operation */
            LinSM_HandleScheduleRequest(Channel);
            LinSM_HandleModeRequest(Channel);
            break;
            
        case LINSM_STATE_WAKEUP:
            /* Wakeup in progress */
            LinSM_HandleModeRequest(Channel);
            break;
            
        case LINSM_STATE_GOTOSLEEP:
            /* Go-to-sleep in progress */
            LinSM_HandleModeRequest(Channel);
            break;
            
        default:
            /* Invalid state */
            channelState->State = LINSM_STATE_UNINIT;
            break;
    }
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
/**
 * @brief Initializes the LIN State Manager module
 * @req SHALL_LINSM - Initializes the LIN State Manager module
 */
void LinSM_Init(const LinSM_ConfigType* ConfigPtr)
{
    uint8 i;
    const LinSM_ChannelConfigType* channelConfig;
    
    if (ConfigPtr == NULL_PTR) {
#if (LINSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_INIT, LINSM_E_INVALID_POINTER);
#endif
        return;
    }
    
    LinSM_ConfigPtr = ConfigPtr;
    
    /* Initialize channel states */
    for (i = 0U; i < LINSM_NUMBER_OF_CHANNELS; i++) {
        channelConfig = &ConfigPtr->ChannelConfig[i];
        
        LinSM_ChannelStates[i].State = LINSM_STATE_INIT;
        LinSM_ChannelStates[i].ComMode = LINSM_NO_COM;
        LinSM_ChannelStates[i].CurrentSchedule = channelConfig->InitialSchedule;
        LinSM_ChannelStates[i].RequestedSchedule = LINSM_SCHEDULE_NULL;
        LinSM_ChannelStates[i].ScheduleStatus = LINSM_SCHEDULE_NULL;
        LinSM_ChannelStates[i].RequestTimer = 0U;
        LinSM_ChannelStates[i].ScheduleSwitchCount = 0U;
        LinSM_ChannelStates[i].Initialized = TRUE;
        LinSM_ChannelStates[i].WakeupPending = FALSE;
        LinSM_ChannelStates[i].GotosleepPending = FALSE;
    }
    
    LinSM_Initialized = TRUE;
}

/**
 * @brief Deinitializes the LIN State Manager module
 * @req SHALL_LINSM - Deinitializes the LIN State Manager module
 */
void LinSM_DeInit(void)
{
    uint8 i;
    
    if (!LinSM_Initialized) {
#if (LINSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_DEINIT, LINSM_E_NOT_INITIALIZED);
#endif
        return;
    }
    
    /* Reset channel states */
    for (i = 0U; i < LINSM_NUMBER_OF_CHANNELS; i++) {
        LinSM_ChannelStates[i].State = LINSM_STATE_UNINIT;
        LinSM_ChannelStates[i].ComMode = LINSM_NO_COM;
        LinSM_ChannelStates[i].Initialized = FALSE;
    }
    
    LinSM_ConfigPtr = NULL_PTR;
    LinSM_Initialized = FALSE;
}

/**
 * @brief Gets version information
 * @req SHALL_LINSM - Gets version information
 */
#if (LINSM_VERSION_INFO_API == STD_ON)
void LinSM_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    if (VersionInfo == NULL_PTR) {
#if (LINSM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_GET_VERSION_INFO, LINSM_E_INVALID_POINTER);
#endif
        return;
    }
    
    VersionInfo->vendorID = LINSM_VENDOR_ID;
    VersionInfo->moduleID = LINSM_MODULE_ID;
    VersionInfo->sw_major_version = LINSM_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = LINSM_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = LINSM_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Requests schedule change
 * @req SHALL_LINSM - Requests schedule change
 */
Std_ReturnType LinSM_ScheduleRequest(LinSM_ChannelType Channel, LinSM_ScheduleType Schedule)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINSM_DEV_ERROR_DETECT == STD_ON)
    if (!LinSM_Initialized) {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_SCHEDULE_REQUEST, LINSM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (LinSM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_SCHEDULE_REQUEST, LINSM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
    if (Schedule >= LINSM_NUMBER_OF_SCHEDULES) {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_SCHEDULE_REQUEST, LINSM_E_INVALID_SCHEDULE);
        return E_NOT_OK;
    }
#endif
    
    if (LinSM_Initialized && 
        (LinSM_ValidateChannel(Channel) == E_OK) && 
        (Schedule < LINSM_NUMBER_OF_SCHEDULES)) {
        
        LinSM_ChannelStates[Channel].RequestedSchedule = Schedule;
        LinSM_ChannelStates[Channel].ScheduleStatus = LINSM_SCHEDULE_REQUESTED;
        LinSM_ChannelStates[Channel].RequestTimer = LINSM_REQUEST_TIMEOUT_COUNT;
        
        /* In real implementation, call LinM to change schedule */
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Gets the current schedule for a channel
 * @req SHALL_LINSM - Gets the current schedule for a channel
 */
Std_ReturnType LinSM_GetCurrentSchedule(LinSM_ChannelType Channel, LinSM_ScheduleType* Schedule)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINSM_DEV_ERROR_DETECT == STD_ON)
    if (!LinSM_Initialized) {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_GET_CURRENT_SCHEDULE, LINSM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (Schedule == NULL_PTR) {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_GET_CURRENT_SCHEDULE, LINSM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
    if (LinSM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_GET_CURRENT_SCHEDULE, LINSM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (LinSM_Initialized && (Schedule != NULL_PTR) && (LinSM_ValidateChannel(Channel) == E_OK)) {
        *Schedule = LinSM_ChannelStates[Channel].CurrentSchedule;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Requests communication mode change
 * @req SHALL_LINSM - Requests communication mode change
 */
Std_ReturnType LinSM_RequestComMode(LinSM_ChannelType Channel, LinSM_ModeType Mode)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINSM_DEV_ERROR_DETECT == STD_ON)
    if (!LinSM_Initialized) {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_REQUEST_COM_MODE, LINSM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (LinSM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_REQUEST_COM_MODE, LINSM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (LinSM_Initialized && (LinSM_ValidateChannel(Channel) == E_OK)) {
        switch (Mode) {
            case LINSM_FULL_COM:
                LinSM_ChannelStates[Channel].WakeupPending = TRUE;
                LinSM_ChannelStates[Channel].State = LINSM_STATE_WAKEUP;
                result = E_OK;
                break;
                
            case LINSM_NO_COM:
                LinSM_ChannelStates[Channel].GotosleepPending = TRUE;
                LinSM_ChannelStates[Channel].State = LINSM_STATE_GOTOSLEEP;
                result = E_OK;
                break;
                
            case LINSM_SILENT_COM:
                /* Silent communication not supported for LIN */
                result = E_NOT_OK;
                break;
                
            default:
                result = E_NOT_OK;
                break;
        }
    }
    
    return result;
}

/**
 * @brief Gets the current communication mode for a channel
 * @req SHALL_LINSM - Gets the current communication mode for a channel
 */
Std_ReturnType LinSM_GetCurrentComMode(LinSM_ChannelType Channel, LinSM_ModeType* Mode)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINSM_DEV_ERROR_DETECT == STD_ON)
    if (!LinSM_Initialized) {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_GET_CURRENT_COM_MODE, LINSM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (Mode == NULL_PTR) {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_GET_CURRENT_COM_MODE, LINSM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
    if (LinSM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_GET_CURRENT_COM_MODE, LINSM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (LinSM_Initialized && (Mode != NULL_PTR) && (LinSM_ValidateChannel(Channel) == E_OK)) {
        *Mode = LinSM_ChannelStates[Channel].ComMode;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Main function for LinSM (to be called periodically)
 * @req SHALL_LINSM - Main function for LinSM (to be called periodically)
 */
void LinSM_MainFunction(void)
{
    uint8 i;
    
    if (!LinSM_Initialized) {
        return;
    }
    
    for (i = 0U; i < LINSM_NUMBER_OF_CHANNELS; i++) {
        LinSM_ProcessStateMachine(i);
    }
}

/**
 * @brief Schedule confirmation callback from LinM
 * @req SHALL_LINSM - Schedule confirmation callback from LinM
 */
void LinSM_ScheduleConfirmation(LinSM_ChannelType Channel, LinSM_ScheduleType Schedule)
{
    if (LinSM_Initialized && (LinSM_ValidateChannel(Channel) == E_OK)) {
        LinSM_ChannelStates[Channel].CurrentSchedule = Schedule;
        LinSM_ChannelStates[Channel].ScheduleStatus = LINSM_SCHEDULE_NULL;
        LinSM_ChannelStates[Channel].RequestedSchedule = LINSM_SCHEDULE_NULL;
    }
}

/**
 * @brief Wakeup confirmation callback from LinM
 * @req SHALL_LINSM - Wakeup confirmation callback from LinM
 */
void LinSM_WakeUpConfirmation(LinSM_ChannelType Channel, boolean Success)
{
    if (LinSM_Initialized && (LinSM_ValidateChannel(Channel) == E_OK)) {
        if (Success) {
            LinSM_ChannelStates[Channel].ComMode = LINSM_FULL_COM;
            LinSM_ChannelStates[Channel].State = LINSM_STATE_RUN;
        } else {
            /* Wakeup failed, remain in NO_COM */
            LinSM_ChannelStates[Channel].ComMode = LINSM_NO_COM;
        }
        LinSM_ChannelStates[Channel].WakeupPending = FALSE;
    }
}

/**
 * @brief Go-to-sleep confirmation callback from LinM
 * @req SHALL_LINSM - Go-to-sleep confirmation callback from LinM
 */
void LinSM_GotoSleepConfirmation(LinSM_ChannelType Channel, boolean Success)
{
    if (LinSM_Initialized && (LinSM_ValidateChannel(Channel) == E_OK)) {
        if (Success) {
            LinSM_ChannelStates[Channel].ComMode = LINSM_NO_COM;
        } else {
            /* Go-to-sleep failed, return to RUN */
            LinSM_ChannelStates[Channel].State = LINSM_STATE_RUN;
        }
        LinSM_ChannelStates[Channel].GotosleepPending = FALSE;
    }
    
    (void)Success;
}
