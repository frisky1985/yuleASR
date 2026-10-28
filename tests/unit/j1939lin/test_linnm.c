/**
 * @file test_linnm.c
 * @brief LinM unit tests (src/bsw/services/linm/src/LinM.c) — coverage uplift.
 *
 * The task refers to the module under src/bsw/services/linm/ as "LinNm"; the
 * actual module there is LinM (LIN Master Management, header LinM.h). This
 * file compiles the real production source by #including it into this
 * translation unit (white-box, same pattern as tests/unit/services/
 * test_PduR_branch.c) so the static channel-state array and the static
 * schedule helpers become accessible:
 *  - lifecycle (Init/DeInit/GetVersionInfo) + DET paths for every public API
 *  - schedule control: Init/Start/Stop/SetMode/GetStatus incl. DET errors
 *  - schedule engine: timer ticking, entry execution for all frame types
 *    (unconditional/event-triggered/diagnostic/sporadic/slave-to-slave/empty
 *    + unknown type), wrap-around in STARTED mode and single-pass ONCE mode
 *  - wakeup / go-to-sleep timers and slave response status
 *
 * NOTE (observed behaviour): LinM_SendFrameHeader is a stub that only returns
 * E_OK (no Lin driver call) — the tests therefore verify scheduling/timing
 * behaviour, not bus output.
 */

// @tests src/bsw/services/linm/src/LinM.c  @tests src/bsw/services/linm/include/LinM.h

#include <string.h>
#include "unity.h"
#include "LinM.h"
#include "Det.h"

/*==================================================================================================
*                                  Det mock
*================================================================================================*/
static uint16 test_DetCalls;
static uint8  test_DetLastApiId;
static uint8  test_DetLastErrorId;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    test_DetCalls++;
    test_DetLastApiId = ApiId;
    test_DetLastErrorId = ErrorId;
    return E_OK;
}

/*==================================================================================================
*                                  Test configuration
*================================================================================================*/
/* schedule 0 (LINM_SCHEDULE_NULL): covers every entry type + wrap-around */
static const LinM_ScheduleEntryConfigType testSched0Entries[6] =
{
    { 2U,   0U, LINM_ENTRY_TYPE_UNCONDITIONAL },
    { 1U,   1U, LINM_ENTRY_TYPE_EVENT_TRIGGERED },
    { 0U,   2U, LINM_ENTRY_TYPE_DIAGNOSTIC },
    { 0U,   3U, LINM_ENTRY_TYPE_SPORADIC },
    { 0U,   4U, LINM_ENTRY_TYPE_SLAVE_TO_SLAVE },
    { 0U, 0xFFU, LINM_ENTRY_TYPE_EMPTY }
};

/* schedule 1: two delayed entries, used for the ONCE single-pass test */
static const LinM_ScheduleEntryConfigType testSched1Entries[2] =
{
    { 1U, 10U, LINM_ENTRY_TYPE_UNCONDITIONAL },
    { 1U, 11U, LINM_ENTRY_TYPE_UNCONDITIONAL }
};

/* schedule 2: unknown frame type hits the default branch of ExecuteEntry */
static const LinM_ScheduleEntryConfigType testSched2Entries[1] =
{
    { 0U, 20U, 0x7FU }
};

static const LinM_ScheduleConfigType testSchedules[3] =
{
    { 0U, testSched0Entries, 6U, 0U, FALSE },
    { 1U, testSched1Entries, 2U, 1U, FALSE },
    { 2U, testSched2Entries, 1U, 2U, TRUE }
};

static const LinM_ChannelConfigType testChannelConfig =
{
    0U,             /* ChannelId */
    testSchedules,
    3U,             /* NumSchedules */
    12U,            /* NumFrames */
    5U,             /* ScheduleTimerBase */
    TRUE,           /* WakeupSupport */
    TRUE            /* SleepSupport */
};

static const LinM_ConfigType testLinMConfig =
{
    &testChannelConfig,
    1U,     /* NumChannels */
    TRUE,   /* DevErrorDetect */
    TRUE    /* VersionInfoApi */
};

/* Include the production source (white-box). */
#include "src/bsw/services/linm/src/LinM.c"

/*==================================================================================================
*                                  Fixtures
*================================================================================================*/
void setUp(void)
{
    test_DetCalls = 0U;
    test_DetLastApiId = 0xFFU;
    test_DetLastErrorId = 0xFFU;
    LinM_Initialized = FALSE;
    LinM_ConfigPtr = NULL_PTR;
    (void)memset(LinM_ChannelStates, 0, sizeof(LinM_ChannelStates));
}

void tearDown(void)
{
}

static void test_init(void)
{
    LinM_Init(&testLinMConfig);
}

static void test_start_sched0(void)
{
    LinM_Init(&testLinMConfig);
    TEST_ASSERT_EQUAL(E_OK, LinM_StartSchedule(0U, 0U));
}

/*==================================================================================================
*                                  Lifecycle
*================================================================================================*/
void test_Init_NullPtr_ReportsDet(void)
{
    /* NOTE (observed behaviour): Init reports DET via Det_ReportError but has
     * no return value; the module stays uninitialized. */
    LinM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(LINM_SID_INIT, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_POINTER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL(FALSE, LinM_Initialized);
    TEST_ASSERT_NULL(LinM_ConfigPtr);
}

void test_Init_Valid_ResetsChannelState(void)
{
    /* dirty state first to prove Init resets everything */
    LinM_ChannelStates[0].ScheduleStatus = LINM_SCHEDULE_RUNNING;
    LinM_ChannelStates[0].ScheduleMode = LINM_SCHEDULE_MODE_ONCE;
    LinM_ChannelStates[0].CurrentSchedule = 2U;
    LinM_ChannelStates[0].CurrentEntry = 4U;
    LinM_ChannelStates[0].ScheduleTimer = 99U;

    test_init();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL(TRUE, LinM_Initialized);
    TEST_ASSERT_EQUAL(LinM_ConfigPtr, &testLinMConfig);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_IDLE, LinM_ChannelStates[0].ScheduleStatus);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_MODE_STOPPED, LinM_ChannelStates[0].ScheduleMode);
    /* NOTE (observed behaviour): CurrentSchedule is set to LINM_SCHEDULE_NULL
     * which is defined as 0 — i.e. the first configured schedule */
    TEST_ASSERT_EQUAL_UINT8(LINM_SCHEDULE_NULL, LinM_ChannelStates[0].CurrentSchedule);
    TEST_ASSERT_EQUAL_UINT8(0U, LinM_ChannelStates[0].CurrentEntry);
    TEST_ASSERT_EQUAL_INT(LINM_SLAVE_RESPONSE_INVALID, LinM_ChannelStates[0].SlaveResponse);
    TEST_ASSERT_EQUAL_UINT16(0U, LinM_ChannelStates[0].ScheduleTimer);
    TEST_ASSERT_EQUAL_UINT16(0U, LinM_ChannelStates[0].WakeupTimer);
    TEST_ASSERT_EQUAL_UINT16(0U, LinM_ChannelStates[0].SleepTimer);
    TEST_ASSERT_EQUAL(TRUE, LinM_ChannelStates[0].Initialized);
}

void test_DeInit_AfterInit_ClearsState(void)
{
    test_start_sched0();
    LinM_DeInit();
    TEST_ASSERT_EQUAL(FALSE, LinM_Initialized);
    TEST_ASSERT_NULL(LinM_ConfigPtr);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_IDLE, LinM_ChannelStates[0].ScheduleStatus);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_MODE_STOPPED, LinM_ChannelStates[0].ScheduleMode);
    TEST_ASSERT_EQUAL_UINT8(LINM_SCHEDULE_NULL, LinM_ChannelStates[0].CurrentSchedule);
    TEST_ASSERT_EQUAL(FALSE, LinM_ChannelStates[0].Initialized);
}

void test_DeInit_Uninit_ReportsDet(void)
{
    LinM_DeInit();
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(LINM_SID_DEINIT, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINM_E_NOT_INITIALIZED, test_DetLastErrorId);
}

void test_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType info;

    (void)memset(&info, 0, sizeof(info));
    LinM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(LINM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(LINM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(LINM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(LINM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(LINM_SW_PATCH_VERSION, info.sw_patch_version);
}

void test_GetVersionInfo_Null_ReportsDet(void)
{
    LinM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(LINM_SID_GET_VERSION_INFO, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_POINTER, test_DetLastErrorId);
}

/*==================================================================================================
*                                  Schedule control
*================================================================================================*/
void test_InitSchedule_Errors(void)
{
    /* uninit */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_InitSchedule(0U, 0U));
    TEST_ASSERT_EQUAL_UINT8(LINM_SID_INIT_SCHEDULE, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_init();
    /* invalid channel */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_InitSchedule(LINM_NUMBER_OF_CHANNELS, 0U));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_PARAMETER, test_DetLastErrorId);
    /* invalid schedule (only 3 configured) */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_InitSchedule(0U, 3U));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_SCHEDULE, test_DetLastErrorId);
    TEST_ASSERT_EQUAL_UINT16(3U, test_DetCalls);
}

void test_InitSchedule_Valid(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, LinM_InitSchedule(0U, 2U));
    TEST_ASSERT_EQUAL_UINT8(2U, LinM_ChannelStates[0].CurrentSchedule);
    TEST_ASSERT_EQUAL_UINT8(0U, LinM_ChannelStates[0].CurrentEntry);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_IDLE, LinM_ChannelStates[0].ScheduleStatus);
}

void test_StartSchedule_Errors(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_StartSchedule(0U, 0U));   /* uninit */
    TEST_ASSERT_EQUAL_UINT8(LINM_SID_START_SCHEDULE, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_StartSchedule(LINM_NUMBER_OF_CHANNELS, 0U));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_PARAMETER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_StartSchedule(0U, 3U));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_SCHEDULE, test_DetLastErrorId);
    TEST_ASSERT_EQUAL_UINT16(3U, test_DetCalls);
}

void test_StartSchedule_Valid_ExecutesFirstEntry(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, LinM_StartSchedule(0U, 0U));
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_RUNNING, LinM_ChannelStates[0].ScheduleStatus);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_MODE_STARTED, LinM_ChannelStates[0].ScheduleMode);
    TEST_ASSERT_EQUAL_UINT8(0U, LinM_ChannelStates[0].CurrentSchedule);
    /* first entry executed immediately -> timer loaded with its delay */
    TEST_ASSERT_EQUAL_UINT16(2U, LinM_ChannelStates[0].ScheduleTimer);
}

void test_StopSchedule_Errors_AndValid(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_StopSchedule(0U));   /* uninit */
    TEST_ASSERT_EQUAL_UINT8(LINM_SID_STOP_SCHEDULE, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_start_sched0();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_StopSchedule(LINM_NUMBER_OF_CHANNELS));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_PARAMETER, test_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_OK, LinM_StopSchedule(0U));
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_IDLE, LinM_ChannelStates[0].ScheduleStatus);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_MODE_STOPPED, LinM_ChannelStates[0].ScheduleMode);
    TEST_ASSERT_EQUAL_UINT8(0U, LinM_ChannelStates[0].CurrentEntry);
}

void test_SetScheduleMode_Errors(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_SetScheduleMode(0U, LINM_SCHEDULE_MODE_STARTED));   /* uninit */
    TEST_ASSERT_EQUAL_UINT8(LINM_SID_SET_SCHEDULE_MODE, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_SetScheduleMode(LINM_NUMBER_OF_CHANNELS, LINM_SCHEDULE_MODE_STARTED));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_PARAMETER, test_DetLastErrorId);
}

void test_SetScheduleMode_StartedAndStopped(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, LinM_SetScheduleMode(0U, LINM_SCHEDULE_MODE_STARTED));
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_RUNNING, LinM_ChannelStates[0].ScheduleStatus);
    TEST_ASSERT_EQUAL(E_OK, LinM_SetScheduleMode(0U, LINM_SCHEDULE_MODE_STOPPED));
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_IDLE, LinM_ChannelStates[0].ScheduleStatus);
}

void test_SetScheduleMode_Other_OnlyModeChanges(void)
{
    test_start_sched0();
    /* NOTE (observed behaviour): CONTINUE / ONCE only update the mode, the
     * run status is left untouched (stays RUNNING) */
    TEST_ASSERT_EQUAL(E_OK, LinM_SetScheduleMode(0U, LINM_SCHEDULE_MODE_CONTINUE));
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_MODE_CONTINUE, LinM_ChannelStates[0].ScheduleMode);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_RUNNING, LinM_ChannelStates[0].ScheduleStatus);

    TEST_ASSERT_EQUAL(E_OK, LinM_SetScheduleMode(0U, LINM_SCHEDULE_MODE_ONCE));
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_MODE_ONCE, LinM_ChannelStates[0].ScheduleMode);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_RUNNING, LinM_ChannelStates[0].ScheduleStatus);
}

void test_GetScheduleStatus_Errors(void)
{
    LinM_ScheduleStatusType status = LINM_SCHEDULE_RUNNING;

    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_GetScheduleStatus(0U, &status));   /* uninit */
    TEST_ASSERT_EQUAL_UINT8(LINM_SID_GET_SCHEDULE_STATUS, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_GetScheduleStatus(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_POINTER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_GetScheduleStatus(LINM_NUMBER_OF_CHANNELS, &status));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_PARAMETER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL_UINT16(3U, test_DetCalls);
}

void test_GetScheduleStatus_Valid(void)
{
    LinM_ScheduleStatusType status = LINM_SCHEDULE_RUNNING;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, LinM_GetScheduleStatus(0U, &status));
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_IDLE, status);
}

/*==================================================================================================
*                                  Schedule engine (MainFunction)
*================================================================================================*/
void test_MainFunction_Uninit_NoOp(void)
{
    LinM_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

void test_MainFunction_NotRunning_NoOp(void)
{
    test_init();
    LinM_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, LinM_ChannelStates[0].ScheduleTimer);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_IDLE, LinM_ChannelStates[0].ScheduleStatus);
}

void test_MainFunction_TicksScheduleTimer(void)
{
    test_start_sched0();
    LinM_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(1U, LinM_ChannelStates[0].ScheduleTimer);
    LinM_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, LinM_ChannelStates[0].ScheduleTimer);
    TEST_ASSERT_EQUAL_UINT8(0U, LinM_ChannelStates[0].CurrentEntry);   /* timer only */
}

void test_MainFunction_ExecutesEntriesAndAdvances(void)
{
    test_start_sched0();   /* entry 0 executed, timer = 2 */
    LinM_MainFunction();   /* tick 2->1 */
    LinM_MainFunction();   /* tick 1->0 */
    LinM_MainFunction();   /* timer 0 -> process entry 0, advance to 1 (timer = delay(0) = 2) */
    TEST_ASSERT_EQUAL_UINT8(1U, LinM_ChannelStates[0].CurrentEntry);
    TEST_ASSERT_EQUAL_UINT16(2U, LinM_ChannelStates[0].ScheduleTimer);

    LinM_MainFunction();   /* tick 2->1 */
    LinM_MainFunction();   /* tick 1->0 */
    LinM_MainFunction();   /* process entry 1 (delay 1) -> entry 2, timer = 1 */
    TEST_ASSERT_EQUAL_UINT8(2U, LinM_ChannelStates[0].CurrentEntry);
    TEST_ASSERT_EQUAL_UINT16(1U, LinM_ChannelStates[0].ScheduleTimer);
}

void test_MainFunction_WrapsAroundInStartedMode(void)
{
    test_start_sched0();
    /* 6 entries with delays 2,1,0,0,0,0: 11 calls finish one full pass plus
     * the wrap-around processing of entry 0 */
    for (uint8 i = 0U; i < 11U; i++)
    {
        LinM_MainFunction();
    }
    /* schedule wrapped and restarted from entry 0, still RUNNING */
    TEST_ASSERT_EQUAL_UINT8(0U, LinM_ChannelStates[0].CurrentEntry);
    TEST_ASSERT_EQUAL_UINT16(2U, LinM_ChannelStates[0].ScheduleTimer);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_RUNNING, LinM_ChannelStates[0].ScheduleStatus);

    LinM_MainFunction();   /* keep running: timer ticks down */
    TEST_ASSERT_EQUAL_UINT16(1U, LinM_ChannelStates[0].ScheduleTimer);
}

void test_MainFunction_OnceMode_StopsAfterSinglePass(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, LinM_StartSchedule(0U, 1U));   /* entry 0 executed, timer = 1 */
    TEST_ASSERT_EQUAL(E_OK, LinM_SetScheduleMode(0U, LINM_SCHEDULE_MODE_ONCE));

    LinM_MainFunction();   /* tick 1->0 */
    LinM_MainFunction();   /* process entry 0 -> entry 1 (timer = delay = 1) */
    TEST_ASSERT_EQUAL_UINT8(1U, LinM_ChannelStates[0].CurrentEntry);
    LinM_MainFunction();   /* tick 1->0 */
    LinM_MainFunction();   /* process entry 1 -> schedule complete -> IDLE */
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_IDLE, LinM_ChannelStates[0].ScheduleStatus);
    TEST_ASSERT_EQUAL_UINT8(0U, LinM_ChannelStates[0].CurrentEntry);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_MODE_ONCE, LinM_ChannelStates[0].ScheduleMode);
}

void test_MainFunction_UnknownFrameType_DefaultBranch(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, LinM_StartSchedule(0U, 2U));   /* unknown type 0x7F: no action */
    LinM_MainFunction();   /* timer 0 -> process + wrap (STARTED) -> entry 0 again */
    TEST_ASSERT_EQUAL_UINT8(0U, LinM_ChannelStates[0].CurrentEntry);
    TEST_ASSERT_EQUAL_UINT16(0U, LinM_ChannelStates[0].ScheduleTimer);
    TEST_ASSERT_EQUAL_INT(LINM_SCHEDULE_RUNNING, LinM_ChannelStates[0].ScheduleStatus);
}

void test_MainFunction_InvalidCurrentSchedule_GuardReturn(void)
{
    test_start_sched0();
    LinM_ChannelStates[0].CurrentSchedule = 5U;   /* >= NumSchedules */
    LinM_MainFunction();
    /* guard hit: nothing processed, no crash */
    TEST_ASSERT_EQUAL_UINT16(2U, LinM_ChannelStates[0].ScheduleTimer);
}

/*==================================================================================================
*                                  Wakeup / sleep / slave response
*================================================================================================*/
void test_WakeUp_Errors_AndValid(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_WakeUp(0U));   /* uninit */
    TEST_ASSERT_EQUAL_UINT8(LINM_SID_WAKEUP, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_WakeUp(LINM_NUMBER_OF_CHANNELS));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_PARAMETER, test_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_OK, LinM_WakeUp(0U));
    /* 50 ms / 5 ms period */
    TEST_ASSERT_EQUAL_UINT16(10U, LinM_ChannelStates[0].WakeupTimer);
}

void test_GotoSleep_Errors_AndValid(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_GotoSleep(0U));   /* uninit */
    TEST_ASSERT_EQUAL_UINT8(LINM_SID_GOTOSLEEP, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_GotoSleep(LINM_NUMBER_OF_CHANNELS));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_PARAMETER, test_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_OK, LinM_GotoSleep(0U));
    /* 100 ms / 5 ms period */
    TEST_ASSERT_EQUAL_UINT16(20U, LinM_ChannelStates[0].SleepTimer);
}

void test_GetSlaveResponse_Errors(void)
{
    LinM_SlaveResponseStatusType response = LINM_SLAVE_RESPONSE_OK;

    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_GetSlaveResponse(0U, &response));   /* uninit */
    TEST_ASSERT_EQUAL_UINT8(LINM_SID_GET_SLAVE_RESPONSE, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINM_E_NOT_INITIALIZED, test_DetLastErrorId);

    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_GetSlaveResponse(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_POINTER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_GetSlaveResponse(LINM_NUMBER_OF_CHANNELS, &response));
    TEST_ASSERT_EQUAL_UINT8(LINM_E_INVALID_PARAMETER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL_UINT16(3U, test_DetCalls);
}

void test_GetSlaveResponse_Valid(void)
{
    LinM_SlaveResponseStatusType response = LINM_SLAVE_RESPONSE_OK;

    test_init();
    TEST_ASSERT_EQUAL(E_OK, LinM_GetSlaveResponse(0U, &response));
    TEST_ASSERT_EQUAL_INT(LINM_SLAVE_RESPONSE_INVALID, response);

    /* status comes straight from the channel state (white-box) */
    LinM_ChannelStates[0].SlaveResponse = LINM_SLAVE_RESPONSE_TIMEOUT;
    TEST_ASSERT_EQUAL(E_OK, LinM_GetSlaveResponse(0U, &response));
    TEST_ASSERT_EQUAL_INT(LINM_SLAVE_RESPONSE_TIMEOUT, response);
}

/*==================================================================================================
*                                  White-box helpers
*================================================================================================*/
void test_ValidateChannel_AndValidateSchedule(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, LinM_ValidateChannel(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_ValidateChannel(LINM_NUMBER_OF_CHANNELS));
    TEST_ASSERT_EQUAL(E_OK, LinM_ValidateSchedule(0U, 2U));
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_ValidateSchedule(0U, 3U));   /* only 3 schedules */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinM_ValidateSchedule(LINM_NUMBER_OF_CHANNELS, 0U));
}

void test_SendFrameHeader_AlwaysOk(void)
{
    /* NOTE (observed behaviour): SendFrameHeader is a stub that always
     * returns E_OK without touching a LIN driver */
    TEST_ASSERT_EQUAL(E_OK, LinM_SendFrameHeader(0U, 0U));
    TEST_ASSERT_EQUAL(E_OK, LinM_SendFrameHeader(0U, 0xFFU));
}

void test_ExecuteEntry_NullPtr_NoOp(void)
{
    test_init();
    LinM_ExecuteEntry(0U, NULL_PTR);
    /* nothing happened, channel state untouched */
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, LinM_ChannelStates[0].CurrentEntry);
}

/*==================================================================================================
*                                  MAIN
*================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    /* Lifecycle */
    RUN_TEST(test_Init_NullPtr_ReportsDet);
    RUN_TEST(test_Init_Valid_ResetsChannelState);
    RUN_TEST(test_DeInit_AfterInit_ClearsState);
    RUN_TEST(test_DeInit_Uninit_ReportsDet);
    RUN_TEST(test_GetVersionInfo_Valid);
    RUN_TEST(test_GetVersionInfo_Null_ReportsDet);

    /* Schedule control */
    RUN_TEST(test_InitSchedule_Errors);
    RUN_TEST(test_InitSchedule_Valid);
    RUN_TEST(test_StartSchedule_Errors);
    RUN_TEST(test_StartSchedule_Valid_ExecutesFirstEntry);
    RUN_TEST(test_StopSchedule_Errors_AndValid);
    RUN_TEST(test_SetScheduleMode_Errors);
    RUN_TEST(test_SetScheduleMode_StartedAndStopped);
    RUN_TEST(test_SetScheduleMode_Other_OnlyModeChanges);
    RUN_TEST(test_GetScheduleStatus_Errors);
    RUN_TEST(test_GetScheduleStatus_Valid);

    /* Schedule engine */
    RUN_TEST(test_MainFunction_Uninit_NoOp);
    RUN_TEST(test_MainFunction_NotRunning_NoOp);
    RUN_TEST(test_MainFunction_TicksScheduleTimer);
    RUN_TEST(test_MainFunction_ExecutesEntriesAndAdvances);
    RUN_TEST(test_MainFunction_WrapsAroundInStartedMode);
    RUN_TEST(test_MainFunction_OnceMode_StopsAfterSinglePass);
    RUN_TEST(test_MainFunction_UnknownFrameType_DefaultBranch);
    RUN_TEST(test_MainFunction_InvalidCurrentSchedule_GuardReturn);

    /* Wakeup / sleep / slave response */
    RUN_TEST(test_WakeUp_Errors_AndValid);
    RUN_TEST(test_GotoSleep_Errors_AndValid);
    RUN_TEST(test_GetSlaveResponse_Errors);
    RUN_TEST(test_GetSlaveResponse_Valid);

    /* White-box helpers */
    RUN_TEST(test_ValidateChannel_AndValidateSchedule);
    RUN_TEST(test_SendFrameHeader_AlwaysOk);
    RUN_TEST(test_ExecuteEntry_NullPtr_NoOp);

    return UNITY_END();
}
