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

/************************************************************************************
 * File: LinSM.c
 * Description: LIN State Manager - Implementation
 * AUTOSAR Version: 4.4.0
 *
 * Module: LinSM (LIN State Manager)
 * Purpose: Implements LIN state machine and schedule management
 *
 * State Machine:
 *   NO_COM -> (Wake-up) -> FULL_COM
 *   FULL_COM -> (Sleep) -> NO_COM
 *
 * Schedule Table States:
 *   - NULL_SCHEDULE: No active schedule
 *   - SCHEDULE_REQUESTED: Schedule request pending
 *   - SCHEDULE_RUNNING: Schedule active
 *   - SCHEDULE_STOPPED: Schedule stopped
 ************************************************************************************/

#include "LinSM.h"
#include "LinIf.h"
#include "Det.h"

/*================================================================================
 * Internal Macros
 *===============================================================================*/
#define LINSM_STATE_UNINIT                  (0x00U)
#define LINSM_STATE_INIT                    (0x01U)

#define LINSM_SUBSTATE_NONE                 (0x00U)
#define LINSM_SUBSTATE_WAKEUP_PENDING       (0x01U)
#define LINSM_SUBSTATE_SLEEP_PENDING        (0x02U)
#define LINSM_SUBSTATE_SCHEDULE_PENDING     (0x03U)

#define LINSM_INITIALIZED_CHECK() \
    do { \
        if (LinSM_ModuleState != LINSM_STATE_INIT) { \
            (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_INIT, LINSM_E_UNINIT); \
            return; \
        } \
    } while(0)

#define LINSM_INITIALIZED_CHECK_RET() \
    do { \
        if (LinSM_ModuleState != LINSM_STATE_INIT) { \
            (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_INIT, LINSM_E_UNINIT); \
            return E_NOT_OK; \
        } \
    } while(0)

#define LINSM_CHANNEL_VALID_CHECK(channel) \
    do { \
        if ((channel) >= LINSM_CHANNEL_COUNT) { \
            (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_REQUESTCOMMODE, LINSM_E_NONEXISTENT_CHANNEL); \
            return E_NOT_OK; \
        } \
    } while(0)

#define LINSM_POINTER_VALID_CHECK(ptr) \
    do { \
        if ((ptr) == NULL_PTR) { \
            (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_GETCURRENTCOMMODE, LINSM_E_PARAMETER_POINTER); \
            return E_NOT_OK; \
        } \
    } while(0)

/*================================================================================
 * Internal Type Definitions
 *===============================================================================*/
typedef enum
{
    LINSM_SCHEDULE_STATE_NULL = 0,
    LINSM_SCHEDULE_STATE_REQUESTED,
    LINSM_SCHEDULE_STATE_RUNNING,
    LINSM_SCHEDULE_STATE_STOPPED
} LinSM_ScheduleStateType;

typedef struct
{
    LinSM_ModeType          ComMode;           /* Current communication mode */
    LinSM_ModeType          RequestedMode;     /* Requested communication mode */
    uint8                   Substate;          /* Internal substate */
    uint8                   CurrentSchedule;   /* Currently active schedule */
    uint8                   RequestedSchedule; /* Requested schedule table */
    LinSM_ScheduleStateType ScheduleState;     /* Schedule state */
    uint16                  TimeoutCounter;    /* Timeout counter */
    uint8                   RetryCounter;      /* Retry counter */
    boolean                 ModeChangePending; /* Mode change pending flag */
    boolean                 SchedulePending;   /* Schedule change pending flag */
} LinSM_ChannelStateType;

/*================================================================================
 * Internal Variables
 *===============================================================================*/
/* Module state */
static uint8 LinSM_ModuleState = LINSM_STATE_UNINIT;

/* Channel states */
static LinSM_ChannelStateType LinSM_ChannelStates[LINSM_CHANNEL_COUNT];

/* Configuration pointer */
static const LinSM_ConfigType *LinSM_ConfigPtr = NULL_PTR;

/*================================================================================
 * Static Function Declarations
 *===============================================================================*/
static void LinSM_HandleNoComState(uint8 Channel);
static void LinSM_HandleFullComState(uint8 Channel);
static void LinSM_ProcessScheduleRequest(uint8 Channel);
static void LinSM_ProcessModeRequest(uint8 Channel);
static void LinSM_CheckTimeouts(uint8 Channel);
static void LinSM_ReportModeChange(uint8 Channel, ComM_ModeType Mode);
static Std_ReturnType LinSM_ValidateSchedule(uint8 Channel, uint8 Schedule);

/*================================================================================
 * Service Functions Implementation
 *===============================================================================*/

void LinSM_Init(const LinSM_ConfigType *ConfigPtr)
{
    uint8 channelIdx;

    #if (LINSM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_INIT, LINSM_E_PARAMETER_POINTER);
        return;
    }

    if (ConfigPtr->ChannelCount > LINSM_CHANNEL_COUNT)
    {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_INIT, LINSM_E_INIT_FAILED);
        return;
    }
    #endif

    /* Initialize all channel states */
    for (channelIdx = 0U; channelIdx < LINSM_CHANNEL_COUNT; channelIdx++)
    {
        LinSM_ChannelStates[channelIdx].ComMode = LINSM_NO_COM;
        LinSM_ChannelStates[channelIdx].RequestedMode = LINSM_NO_COM;
        LinSM_ChannelStates[channelIdx].Substate = LINSM_SUBSTATE_NONE;
        LinSM_ChannelStates[channelIdx].CurrentSchedule = LINSM_SCHEDULE_NULL;
        LinSM_ChannelStates[channelIdx].RequestedSchedule = LINSM_SCHEDULE_NULL;
        LinSM_ChannelStates[channelIdx].ScheduleState = LINSM_SCHEDULE_STATE_NULL;
        LinSM_ChannelStates[channelIdx].TimeoutCounter = 0U;
        LinSM_ChannelStates[channelIdx].RetryCounter = 0U;
        LinSM_ChannelStates[channelIdx].ModeChangePending = FALSE;
        LinSM_ChannelStates[channelIdx].SchedulePending = FALSE;

        /* Report initial mode to ComM */
        LinSM_ReportModeChange(channelIdx, COMM_NO_COMMUNICATION);
    }

    /* Store configuration */
    LinSM_ConfigPtr = ConfigPtr;
    LinSM_ModuleState = LINSM_STATE_INIT;
}

void LinSM_DeInit(void)
{
    uint8 channelIdx;

    #if (LINSM_DEV_ERROR_DETECT == STD_ON)
    if (LinSM_ModuleState != LINSM_STATE_INIT)
    {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_DEINIT, LINSM_E_UNINIT);
        return;
    }
    #endif

    /* Reset all channels to NO_COM */
    for (channelIdx = 0U; channelIdx < LINSM_CHANNEL_COUNT; channelIdx++)
    {
        LinSM_ChannelStates[channelIdx].ComMode = LINSM_NO_COM;
        LinSM_ReportModeChange(channelIdx, COMM_NO_COMMUNICATION);
    }

    LinSM_ConfigPtr = NULL_PTR;
    LinSM_ModuleState = LINSM_STATE_UNINIT;
}

Std_ReturnType LinSM_RequestComMode(uint8 Channel, ComM_ModeType Mode)
{
    #if (LINSM_DEV_ERROR_DETECT == STD_ON)
    LINSM_INITIALIZED_CHECK_RET();
    LINSM_CHANNEL_VALID_CHECK(Channel);

    if ((Mode != COMM_NO_COMMUNICATION) && (Mode != COMM_FULL_COMMUNICATION))
    {
        (void)Det_ReportError(LINSM_MODULE_ID, Channel, LINSM_SID_REQUESTCOMMODE, LINSM_E_PARAMETER);
        return E_NOT_OK;
    }
    #endif

    /* Check if mode change is already pending */
    if (LinSM_ChannelStates[Channel].ModeChangePending == TRUE)
    {
        return E_NOT_OK;
    }

    /* Map ComM mode to LinSM mode */
    if (Mode == COMM_FULL_COMMUNICATION)
    {
        LinSM_ChannelStates[Channel].RequestedMode = LINSM_FULL_COM;
    }
    else
    {
        LinSM_ChannelStates[Channel].RequestedMode = LINSM_NO_COM;
    }

    LinSM_ChannelStates[Channel].ModeChangePending = TRUE;
    LinSM_ChannelStates[Channel].TimeoutCounter = 0U;
    LinSM_ChannelStates[Channel].RetryCounter = 0U;

    return E_OK;
}

Std_ReturnType LinSM_GetCurrentComMode(uint8 Channel, ComM_ModeType *Mode)
{
    #if (LINSM_DEV_ERROR_DETECT == STD_ON)
    LINSM_INITIALIZED_CHECK_RET();
    LINSM_CHANNEL_VALID_CHECK(Channel);
    LINSM_POINTER_VALID_CHECK(Mode);
    #endif

    /* Map LinSM mode to ComM mode */
    if (LinSM_ChannelStates[Channel].ComMode == LINSM_FULL_COM)
    {
        *Mode = COMM_FULL_COMMUNICATION;
    }
    else
    {
        *Mode = COMM_NO_COMMUNICATION;
    }

    return E_OK;
}

Std_ReturnType LinSM_ScheduleRequest(uint8 Channel, uint8 Schedule)
{
    #if (LINSM_DEV_ERROR_DETECT == STD_ON)
    Std_ReturnType retVal;

    LINSM_INITIALIZED_CHECK_RET();
    LINSM_CHANNEL_VALID_CHECK(Channel);

    retVal = LinSM_ValidateSchedule(Channel, Schedule);
    if (retVal != E_OK)
    {
        (void)Det_ReportError(LINSM_MODULE_ID, Channel, LINSM_SID_SCHEDULEREQUEST, LINSM_E_INVALID_SCHEDULE);
        return E_NOT_OK;
    }

    /* Schedule can only be changed in FULL_COM mode */
    if (LinSM_ChannelStates[Channel].ComMode != LINSM_FULL_COM)
    {
        return E_NOT_OK;
    }
    #else
    if (LinSM_ValidateSchedule(Channel, Schedule) != E_OK)
    {
        return E_NOT_OK;
    }
    #endif

    /* Check if schedule change is already pending */
    if (LinSM_ChannelStates[Channel].SchedulePending == TRUE)
    {
        return E_NOT_OK;
    }

    LinSM_ChannelStates[Channel].RequestedSchedule = Schedule;
    LinSM_ChannelStates[Channel].SchedulePending = TRUE;
    LinSM_ChannelStates[Channel].ScheduleState = LINSM_SCHEDULE_STATE_REQUESTED;
    LinSM_ChannelStates[Channel].TimeoutCounter = 0U;

    return E_OK;
}

#if (LINSM_VERSION_INFO_API == STD_ON)
void LinSM_GetVersionInfo(Std_VersionInfoType *VersionInfo)
{
    #if (LINSM_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfo == NULL_PTR)
    {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_GETVERSIONINFO, LINSM_E_PARAMETER_POINTER);
        return;
    }
    #endif

    VersionInfo->vendorID = LINSM_VENDOR_ID;
    VersionInfo->moduleID = LINSM_MODULE_ID;
    VersionInfo->sw_major_version = LINSM_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = LINSM_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = LINSM_SW_PATCH_VERSION;
}
#endif

void LinSM_MainFunction(void)
{
    uint8 channelIdx;

    if (LinSM_ModuleState != LINSM_STATE_INIT)
    {
        return;
    }

    for (channelIdx = 0U; channelIdx < LINSM_CHANNEL_COUNT; channelIdx++)
    {
        /* Process state machine based on current mode */
        if (LinSM_ChannelStates[channelIdx].ComMode == LINSM_NO_COM)
        {
            LinSM_HandleNoComState(channelIdx);
        }
        else
        {
            LinSM_HandleFullComState(channelIdx);
        }

        /* Check timeouts */
        LinSM_CheckTimeouts(channelIdx);
    }
}

/*================================================================================
 * Callback Functions
 *===============================================================================*/

void LinSM_WakeUpConfirmation(uint8 Channel, boolean Success)
{
    #if (LINSM_DEV_ERROR_DETECT == STD_ON)
    if (Channel >= LINSM_CHANNEL_COUNT)
    {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_WAKEUPCONFIRMATION, LINSM_E_NONEXISTENT_CHANNEL);
        return;
    }
    #endif

    if (Success == TRUE)
    {
        /* Wake-up successful - transition to FULL_COM */
        LinSM_ChannelStates[Channel].ComMode = LINSM_FULL_COM;
        LinSM_ChannelStates[Channel].Substate = LINSM_SUBSTATE_NONE;
        LinSM_ChannelStates[Channel].ModeChangePending = FALSE;

        /* Report mode change to ComM */
        LinSM_ReportModeChange(Channel, COMM_FULL_COMMUNICATION);

        #if (LINSM_WAKEUP_SUPPORT == STD_ON)
        /* Check if this is a wake-up event */
        if (LinSM_ConfigPtr != NULL_PTR)
        {
            const LinSM_ChannelConfigType *channelCfg = &LinSM_ConfigPtr->ChannelConfig[Channel];
            if (channelCfg->WakeupSupport == TRUE)
            {
                EcuM_SetWakeupEvent(channelCfg->WakeupSource);
            }
        }
        #endif
    }
    else
    {
        /* Wake-up failed - stay in NO_COM */
        LinSM_ChannelStates[Channel].Substate = LINSM_SUBSTATE_NONE;
        LinSM_ChannelStates[Channel].ModeChangePending = FALSE;
    }
}

void LinSM_GoToSleepConfirmation(uint8 Channel, boolean Success)
{
    #if (LINSM_DEV_ERROR_DETECT == STD_ON)
    if (Channel >= LINSM_CHANNEL_COUNT)
    {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_GOTOSLEEPCONFIRMATION, LINSM_E_NONEXISTENT_CHANNEL);
        return;
    }
    #endif

    if (Success == TRUE)
    {
        /* Sleep successful - transition to NO_COM */
        LinSM_ChannelStates[Channel].ComMode = LINSM_NO_COM;
        LinSM_ChannelStates[Channel].CurrentSchedule = LINSM_SCHEDULE_NULL;
        LinSM_ChannelStates[Channel].ScheduleState = LINSM_SCHEDULE_STATE_NULL;
        LinSM_ChannelStates[Channel].Substate = LINSM_SUBSTATE_NONE;
        LinSM_ChannelStates[Channel].ModeChangePending = FALSE;

        /* Report mode change to ComM */
        LinSM_ReportModeChange(Channel, COMM_NO_COMMUNICATION);
    }
    else
    {
        /* Sleep failed - stay in FULL_COM */
        LinSM_ChannelStates[Channel].Substate = LINSM_SUBSTATE_NONE;
        LinSM_ChannelStates[Channel].ModeChangePending = FALSE;
    }
}

void LinSM_ScheduleTableRequest(uint8 Channel, uint8 Schedule)
{
    #if (LINSM_DEV_ERROR_DETECT == STD_ON)
    if (Channel >= LINSM_CHANNEL_COUNT)
    {
        (void)Det_ReportError(LINSM_MODULE_ID, 0U, LINSM_SID_SCHEDULETABLEREQUEST, LINSM_E_NONEXISTENT_CHANNEL);
        return;
    }
    #endif

    /* Schedule request completed */
    if (LinSM_ChannelStates[Channel].SchedulePending == TRUE)
    {
        LinSM_ChannelStates[Channel].CurrentSchedule = Schedule;
        LinSM_ChannelStates[Channel].ScheduleState = LINSM_SCHEDULE_STATE_RUNNING;
        LinSM_ChannelStates[Channel].SchedulePending = FALSE;
    }
}

/*================================================================================
 * Static Functions Implementation
 *===============================================================================*/

static void LinSM_HandleNoComState(uint8 Channel)
{
    /* Check for mode change request to FULL_COM */
    if (LinSM_ChannelStates[Channel].ModeChangePending == TRUE)
    {
        if (LinSM_ChannelStates[Channel].RequestedMode == LINSM_FULL_COM)
        {
            LinSM_ProcessModeRequest(Channel);
        }
    }
}

static void LinSM_HandleFullComState(uint8 Channel)
{
    /* Check for mode change request to NO_COM */
    if (LinSM_ChannelStates[Channel].ModeChangePending == TRUE)
    {
        if (LinSM_ChannelStates[Channel].RequestedMode == LINSM_NO_COM)
        {
            LinSM_ProcessModeRequest(Channel);
            return;
        }
    }

    /* Process schedule requests */
    #if (LINSM_SCHEDULE_TABLE_SWITCHING == STD_ON)
    if (LinSM_ChannelStates[Channel].SchedulePending == TRUE)
    {
        LinSM_ProcessScheduleRequest(Channel);
    }
    #endif
}

static void LinSM_ProcessModeRequest(uint8 Channel)
{
    Std_ReturnType retVal;

    if (LinSM_ChannelStates[Channel].RequestedMode == LINSM_FULL_COM)
    {
        /* Request wake-up */
        if (LinSM_ChannelStates[Channel].Substate != LINSM_SUBSTATE_WAKEUP_PENDING)
        {
            retVal = LinIf_WakeUp(Channel);
            if (retVal == E_OK)
            {
                LinSM_ChannelStates[Channel].Substate = LINSM_SUBSTATE_WAKEUP_PENDING;
            }
        }
    }
    else
    {
        /* Request sleep */
        if (LinSM_ChannelStates[Channel].Substate != LINSM_SUBSTATE_SLEEP_PENDING)
        {
            retVal = LinIf_GotoSleep(Channel);
            if (retVal == E_OK)
            {
                LinSM_ChannelStates[Channel].Substate = LINSM_SUBSTATE_SLEEP_PENDING;
            }
        }
    }
}

static void LinSM_ProcessScheduleRequest(uint8 Channel)
{
    Std_ReturnType retVal;
    uint8 requestedSchedule;

    requestedSchedule = LinSM_ChannelStates[Channel].RequestedSchedule;

    /* Call LinIf to request schedule table */
    retVal = LinIf_ScheduleRequest(Channel, requestedSchedule);

    if (retVal == E_OK)
    {
        /* Schedule request accepted */
        LinSM_ChannelStates[Channel].ScheduleState = LINSM_SCHEDULE_STATE_REQUESTED;
    }
    else
    {
        /* Schedule request failed - will retry in next cycle */
        LinSM_ChannelStates[Channel].RetryCounter++;

        if (LinSM_ChannelStates[Channel].RetryCounter >= LINSM_MAX_RETRY_COUNT)
        {
            /* Max retries reached - abort schedule change */
            LinSM_ChannelStates[Channel].SchedulePending = FALSE;
            LinSM_ChannelStates[Channel].ScheduleState = LINSM_SCHEDULE_STATE_STOPPED;
            LinSM_ChannelStates[Channel].RetryCounter = 0U;
        }
    }
}

static void LinSM_CheckTimeouts(uint8 Channel)
{
    uint16 timeoutValue;

    if (LinSM_ChannelStates[Channel].ModeChangePending == TRUE)
    {
        /* Increment timeout counter */
        LinSM_ChannelStates[Channel].TimeoutCounter += LINSM_MAIN_FUNCTION_PERIOD_MS;

        /* Check appropriate timeout */
        if (LinSM_ChannelStates[Channel].Substate == LINSM_SUBSTATE_WAKEUP_PENDING)
        {
            timeoutValue = LINSM_WAKEUP_CONFIRMATION_TIMEOUT;
        }
        else if (LinSM_ChannelStates[Channel].Substate == LINSM_SUBSTATE_SLEEP_PENDING)
        {
            timeoutValue = LINSM_SLEEP_CONFIRMATION_TIMEOUT;
        }
        else
        {
            timeoutValue = LINSM_SCHEDULE_CONFIRMATION_TIMEOUT;
        }

        if (LinSM_ChannelStates[Channel].TimeoutCounter >= timeoutValue)
        {
            /* Timeout occurred - abort mode change */
            LinSM_ChannelStates[Channel].ModeChangePending = FALSE;
            LinSM_ChannelStates[Channel].Substate = LINSM_SUBSTATE_NONE;
            LinSM_ChannelStates[Channel].TimeoutCounter = 0U;
        }
    }

    /* Check schedule timeout */
    if (LinSM_ChannelStates[Channel].SchedulePending == TRUE)
    {
        LinSM_ChannelStates[Channel].TimeoutCounter += LINSM_MAIN_FUNCTION_PERIOD_MS;

        if (LinSM_ChannelStates[Channel].TimeoutCounter >= LINSM_SCHEDULE_CONFIRMATION_TIMEOUT)
        {
            /* Schedule request timeout */
            LinSM_ChannelStates[Channel].SchedulePending = FALSE;
            LinSM_ChannelStates[Channel].ScheduleState = LINSM_SCHEDULE_STATE_STOPPED;
            LinSM_ChannelStates[Channel].TimeoutCounter = 0U;
        }
    }
}

static void LinSM_ReportModeChange(uint8 Channel, ComM_ModeType Mode)
{
    if (LinSM_ConfigPtr != NULL_PTR)
    {
        uint8 commChannel = LinSM_ConfigPtr->ChannelConfig[Channel].ComMChannelId;
        ComM_BusSM_ModeIndication(commChannel, Mode);
    }
}

static Std_ReturnType LinSM_ValidateSchedule(uint8 Channel, uint8 Schedule)
{
    const LinSM_ChannelConfigType *channelCfg;
    uint8 scheduleIdx;

    if (LinSM_ConfigPtr == NULL_PTR)
    {
        return E_NOT_OK;
    }

    channelCfg = &LinSM_ConfigPtr->ChannelConfig[Channel];

    /* Check if schedule is valid for this channel */
    for (scheduleIdx = 0U; scheduleIdx < channelCfg->ScheduleCount; scheduleIdx++)
    {
        if (channelCfg->ScheduleIdList[scheduleIdx] == Schedule)
        {
            return E_OK;
        }
    }

    return E_NOT_OK;
}
