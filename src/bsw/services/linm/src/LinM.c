/**
 * @file LinM.c
 * @brief LIN Master Management module implementation
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: LIN Master Management (LinM)
 * Layer: Service Layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "LinM.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define LINM_INVALID_CHANNEL                (0xFFU)
#define LINM_INVALID_SCHEDULE               (0xFFU)
#define LINM_INVALID_ENTRY                  (0xFFU)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
/**
 * @brief LIN Master Channel State structure
 */
typedef struct {
    LinM_ScheduleStatusType ScheduleStatus;     /*!< Schedule status */
    LinM_ScheduleModeType ScheduleMode;         /*!< Schedule mode */
    LinM_ScheduleType CurrentSchedule;          /*!< Current schedule */
    LinM_ScheduleEntryType CurrentEntry;        /*!< Current entry */
    LinM_SlaveResponseStatusType SlaveResponse; /*!< Slave response status */
    uint16 ScheduleTimer;                       /*!< Schedule timer */
    uint16 WakeupTimer;                         /*!< Wakeup timer */
    uint16 SleepTimer;                          /*!< Sleep timer */
    boolean Initialized;                        /*!< Initialization flag */
} LinM_ChannelStateType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define LINM_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC boolean LinM_Initialized = FALSE;
STATIC const LinM_ConfigType* LinM_ConfigPtr = NULL_PTR;

#define LINM_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

#define LINM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC LinM_ChannelStateType LinM_ChannelStates[LINM_NUMBER_OF_CHANNELS];

#define LINM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType LinM_ValidateChannel(LinM_ChannelType Channel);
STATIC Std_ReturnType LinM_ValidateSchedule(LinM_ChannelType Channel, LinM_ScheduleType Schedule);
STATIC void LinM_ProcessSchedule(LinM_ChannelType Channel);
STATIC void LinM_ProcessEntry(LinM_ChannelType Channel);
STATIC Std_ReturnType LinM_SendFrameHeader(LinM_ChannelType Channel, uint8 FrameIndex);
STATIC void LinM_ExecuteEntry(LinM_ChannelType Channel, const LinM_ScheduleEntryConfigType* Entry);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
/**
 * @brief Validates the channel ID
 */
STATIC Std_ReturnType LinM_ValidateChannel(LinM_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (Channel < LINM_NUMBER_OF_CHANNELS) {
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Validates the schedule ID
 */
STATIC Std_ReturnType LinM_ValidateSchedule(LinM_ChannelType Channel, LinM_ScheduleType Schedule)
{
    Std_ReturnType result = E_NOT_OK;
    
    if ((Channel < LINM_NUMBER_OF_CHANNELS) && 
        (Schedule < LinM_ConfigPtr->ChannelConfig[Channel].NumSchedules)) {
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Sends frame header on LIN bus
 */
STATIC Std_ReturnType LinM_SendFrameHeader(LinM_ChannelType Channel, uint8 FrameIndex)
{
    Std_ReturnType result = E_OK;
    
    /* In real implementation, this would call Lin_SendHeader */
    (void)Channel;
    (void)FrameIndex;
    
    return result;
}

/**
 * @brief Executes a schedule entry
 */
STATIC void LinM_ExecuteEntry(LinM_ChannelType Channel, const LinM_ScheduleEntryConfigType* Entry)
{
    if (Entry != NULL_PTR) {
        /* Process entry based on type */
        switch (Entry->FrameType) {
            case LINM_ENTRY_TYPE_UNCONDITIONAL:
                /* Send unconditional frame */
                (void)LinM_SendFrameHeader(Channel, Entry->FrameIndex);
                break;
                
            case LINM_ENTRY_TYPE_EVENT_TRIGGERED:
                /* Send event triggered frame */
                (void)LinM_SendFrameHeader(Channel, Entry->FrameIndex);
                break;
                
            case LINM_ENTRY_TYPE_DIAGNOSTIC:
                /* Send diagnostic frame */
                (void)LinM_SendFrameHeader(Channel, Entry->FrameIndex);
                break;
                
            case LINM_ENTRY_TYPE_SPORADIC:
                /* Send sporadic frame */
                (void)LinM_SendFrameHeader(Channel, Entry->FrameIndex);
                break;
                
            case LINM_ENTRY_TYPE_SLAVE_TO_SLAVE:
                /* Send slave-to-slave frame header */
                (void)LinM_SendFrameHeader(Channel, Entry->FrameIndex);
                break;
                
            case LINM_ENTRY_TYPE_EMPTY:
            default:
                /* Empty entry, do nothing */
                break;
        }
    }
}

/**
 * @brief Processes the current entry in a schedule
 */
STATIC void LinM_ProcessEntry(LinM_ChannelType Channel)
{
    LinM_ChannelStateType* channelState = &LinM_ChannelStates[Channel];
    const LinM_ChannelConfigType* channelConfig = &LinM_ConfigPtr->ChannelConfig[Channel];
    const LinM_ScheduleConfigType* schedule = &channelConfig->Schedules[channelState->CurrentSchedule];
    const LinM_ScheduleEntryConfigType* entry;
    
    if (channelState->CurrentEntry < schedule->NumEntries) {
        entry = &schedule->Entries[channelState->CurrentEntry];
        
        /* Execute the entry */
        LinM_ExecuteEntry(Channel, entry);
        
        /* Set timer for next entry */
        channelState->ScheduleTimer = entry->Delay;
    }
}

/**
 * @brief Processes schedule execution
 */
STATIC void LinM_ProcessSchedule(LinM_ChannelType Channel)
{
    LinM_ChannelStateType* channelState = &LinM_ChannelStates[Channel];
    const LinM_ChannelConfigType* channelConfig = &LinM_ConfigPtr->ChannelConfig[Channel];
    const LinM_ScheduleConfigType* schedule;
    
    if (channelState->ScheduleStatus != LINM_SCHEDULE_RUNNING) {
        return;
    }
    
    if (channelState->CurrentSchedule >= channelConfig->NumSchedules) {
        return;
    }
    
    schedule = &channelConfig->Schedules[channelState->CurrentSchedule];
    
    /* Process schedule timer */
    if (channelState->ScheduleTimer > 0U) {
        channelState->ScheduleTimer--;
    } else {
        /* Timer expired, process next entry */
        LinM_ProcessEntry(Channel);
        
        /* Move to next entry */
        channelState->CurrentEntry++;
        
        if (channelState->CurrentEntry >= schedule->NumEntries) {
            /* Schedule complete */
            if (channelState->ScheduleMode == LINM_SCHEDULE_MODE_ONCE) {
                /* Stop schedule after one run */
                channelState->ScheduleStatus = LINM_SCHEDULE_IDLE;
                channelState->CurrentEntry = 0U;
            } else {
                /* Continue to next iteration */
                channelState->CurrentEntry = 0U;
                LinM_ProcessEntry(Channel);
            }
        }
    }
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
/**
 * @brief Initializes the LIN Master Management module
 */
void LinM_Init(const LinM_ConfigType* ConfigPtr)
{
    uint8 i;
    
    if (ConfigPtr == NULL_PTR) {
#if (LINM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_INIT, LINM_E_INVALID_POINTER);
#endif
        return;
    }
    
    LinM_ConfigPtr = ConfigPtr;
    
    /* Initialize channel states */
    for (i = 0U; i < LINM_NUMBER_OF_CHANNELS; i++) {
        LinM_ChannelStates[i].ScheduleStatus = LINM_SCHEDULE_IDLE;
        LinM_ChannelStates[i].ScheduleMode = LINM_SCHEDULE_MODE_STOPPED;
        LinM_ChannelStates[i].CurrentSchedule = LINM_SCHEDULE_NULL;
        LinM_ChannelStates[i].CurrentEntry = 0U;
        LinM_ChannelStates[i].SlaveResponse = LINM_SLAVE_RESPONSE_INVALID;
        LinM_ChannelStates[i].ScheduleTimer = 0U;
        LinM_ChannelStates[i].WakeupTimer = 0U;
        LinM_ChannelStates[i].SleepTimer = 0U;
        LinM_ChannelStates[i].Initialized = TRUE;
    }
    
    LinM_Initialized = TRUE;
}

/**
 * @brief Deinitializes the LIN Master Management module
 */
void LinM_DeInit(void)
{
    uint8 i;
    
    if (!LinM_Initialized) {
#if (LINM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_DEINIT, LINM_E_NOT_INITIALIZED);
#endif
        return;
    }
    
    /* Stop all schedules and reset channel states */
    for (i = 0U; i < LINM_NUMBER_OF_CHANNELS; i++) {
        LinM_ChannelStates[i].ScheduleStatus = LINM_SCHEDULE_IDLE;
        LinM_ChannelStates[i].ScheduleMode = LINM_SCHEDULE_MODE_STOPPED;
        LinM_ChannelStates[i].CurrentSchedule = LINM_SCHEDULE_NULL;
        LinM_ChannelStates[i].CurrentEntry = 0U;
        LinM_ChannelStates[i].Initialized = FALSE;
    }
    
    LinM_ConfigPtr = NULL_PTR;
    LinM_Initialized = FALSE;
}

/**
 * @brief Gets version information
 */
#if (LINM_VERSION_INFO_API == STD_ON)
void LinM_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    if (VersionInfo == NULL_PTR) {
#if (LINM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_GET_VERSION_INFO, LINM_E_INVALID_POINTER);
#endif
        return;
    }
    
    VersionInfo->vendorID = LINM_VENDOR_ID;
    VersionInfo->moduleID = LINM_MODULE_ID;
    VersionInfo->sw_major_version = LINM_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = LINM_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = LINM_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Initializes a schedule for a channel
 */
Std_ReturnType LinM_InitSchedule(LinM_ChannelType Channel, LinM_ScheduleType Schedule)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINM_DEV_ERROR_DETECT == STD_ON)
    if (!LinM_Initialized) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_INIT_SCHEDULE, LINM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (LinM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_INIT_SCHEDULE, LINM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
    if (LinM_ValidateSchedule(Channel, Schedule) != E_OK) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_INIT_SCHEDULE, LINM_E_INVALID_SCHEDULE);
        return E_NOT_OK;
    }
#endif
    
    if (LinM_Initialized && 
        (LinM_ValidateChannel(Channel) == E_OK) && 
        (LinM_ValidateSchedule(Channel, Schedule) == E_OK)) {
        LinM_ChannelStates[Channel].CurrentSchedule = Schedule;
        LinM_ChannelStates[Channel].CurrentEntry = 0U;
        LinM_ChannelStates[Channel].ScheduleStatus = LINM_SCHEDULE_IDLE;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Starts a schedule for a channel
 */
Std_ReturnType LinM_StartSchedule(LinM_ChannelType Channel, LinM_ScheduleType Schedule)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINM_DEV_ERROR_DETECT == STD_ON)
    if (!LinM_Initialized) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_START_SCHEDULE, LINM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (LinM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_START_SCHEDULE, LINM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
    if (LinM_ValidateSchedule(Channel, Schedule) != E_OK) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_START_SCHEDULE, LINM_E_INVALID_SCHEDULE);
        return E_NOT_OK;
    }
#endif
    
    if (LinM_Initialized && 
        (LinM_ValidateChannel(Channel) == E_OK) && 
        (LinM_ValidateSchedule(Channel, Schedule) == E_OK)) {
        LinM_ChannelStates[Channel].CurrentSchedule = Schedule;
        LinM_ChannelStates[Channel].CurrentEntry = 0U;
        LinM_ChannelStates[Channel].ScheduleStatus = LINM_SCHEDULE_RUNNING;
        LinM_ChannelStates[Channel].ScheduleMode = LINM_SCHEDULE_MODE_STARTED;
        
        /* Start processing first entry */
        LinM_ProcessEntry(Channel);
        
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Stops a schedule for a channel
 */
Std_ReturnType LinM_StopSchedule(LinM_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINM_DEV_ERROR_DETECT == STD_ON)
    if (!LinM_Initialized) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_STOP_SCHEDULE, LINM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (LinM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_STOP_SCHEDULE, LINM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (LinM_Initialized && (LinM_ValidateChannel(Channel) == E_OK)) {
        LinM_ChannelStates[Channel].ScheduleStatus = LINM_SCHEDULE_IDLE;
        LinM_ChannelStates[Channel].ScheduleMode = LINM_SCHEDULE_MODE_STOPPED;
        LinM_ChannelStates[Channel].CurrentEntry = 0U;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Sets schedule mode for a channel
 */
Std_ReturnType LinM_SetScheduleMode(LinM_ChannelType Channel, LinM_ScheduleModeType Mode)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINM_DEV_ERROR_DETECT == STD_ON)
    if (!LinM_Initialized) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_SET_SCHEDULE_MODE, LINM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (LinM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_SET_SCHEDULE_MODE, LINM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (LinM_Initialized && (LinM_ValidateChannel(Channel) == E_OK)) {
        LinM_ChannelStates[Channel].ScheduleMode = Mode;
        
        if (Mode == LINM_SCHEDULE_MODE_STARTED) {
            LinM_ChannelStates[Channel].ScheduleStatus = LINM_SCHEDULE_RUNNING;
        } else if (Mode == LINM_SCHEDULE_MODE_STOPPED) {
            LinM_ChannelStates[Channel].ScheduleStatus = LINM_SCHEDULE_IDLE;
        }
        
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Gets schedule status for a channel
 */
Std_ReturnType LinM_GetScheduleStatus(LinM_ChannelType Channel, LinM_ScheduleStatusType* Status)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINM_DEV_ERROR_DETECT == STD_ON)
    if (!LinM_Initialized) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_GET_SCHEDULE_STATUS, LINM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (Status == NULL_PTR) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_GET_SCHEDULE_STATUS, LINM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
    if (LinM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_GET_SCHEDULE_STATUS, LINM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (LinM_Initialized && (Status != NULL_PTR) && (LinM_ValidateChannel(Channel) == E_OK)) {
        *Status = LinM_ChannelStates[Channel].ScheduleStatus;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Main function for LinM (to be called periodically)
 */
void LinM_MainFunction(void)
{
    uint8 i;
    
    if (!LinM_Initialized) {
        return;
    }
    
    for (i = 0U; i < LINM_NUMBER_OF_CHANNELS; i++) {
        LinM_ProcessSchedule(i);
    }
}

/**
 * @brief Sends wakeup signal on LIN bus
 */
Std_ReturnType LinM_WakeUp(LinM_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINM_DEV_ERROR_DETECT == STD_ON)
    if (!LinM_Initialized) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_WAKEUP, LINM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (LinM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_WAKEUP, LINM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (LinM_Initialized && (LinM_ValidateChannel(Channel) == E_OK)) {
        /* In real implementation, call Lin_WakeUp */
        LinM_ChannelStates[Channel].WakeupTimer = 
            LINM_WAKEUP_TIMEOUT_MS / LINM_MAIN_FUNCTION_PERIOD_MS;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Sends go-to-sleep command on LIN bus
 */
Std_ReturnType LinM_GotoSleep(LinM_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINM_DEV_ERROR_DETECT == STD_ON)
    if (!LinM_Initialized) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_GOTOSLEEP, LINM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (LinM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_GOTOSLEEP, LINM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (LinM_Initialized && (LinM_ValidateChannel(Channel) == E_OK)) {
        /* In real implementation, call Lin_GoToSleep */
        LinM_ChannelStates[Channel].SleepTimer = 
            LINM_SLEEP_TIMEOUT_MS / LINM_MAIN_FUNCTION_PERIOD_MS;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Gets slave response status
 */
Std_ReturnType LinM_GetSlaveResponse(LinM_ChannelType Channel, LinM_SlaveResponseStatusType* Status)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LINM_DEV_ERROR_DETECT == STD_ON)
    if (!LinM_Initialized) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_GET_SLAVE_RESPONSE, LINM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (Status == NULL_PTR) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_GET_SLAVE_RESPONSE, LINM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
    if (LinM_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(LINM_MODULE_ID, 0U, LINM_SID_GET_SLAVE_RESPONSE, LINM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (LinM_Initialized && (Status != NULL_PTR) && (LinM_ValidateChannel(Channel) == E_OK)) {
        *Status = LinM_ChannelStates[Channel].SlaveResponse;
        result = E_OK;
    }
    
    return result;
}
