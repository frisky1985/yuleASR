/**
 * @file test_linm.c
 * @brief Unit tests for LIN Master Management module
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include "LinM.h"

/* Mock for Det_ReportError */
uint8 Det_ReportError_CallCount = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    Det_ReportError_CallCount++;
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}

/* Test Fixtures */
static int setup(void **state) {
    (void)state;
    Det_ReportError_CallCount = 0;
    return 0;
}

static int teardown(void **state) {
    (void)state;
    LinM_DeInit();
    return 0;
}

/* Test: Init with valid config */
static void test_LinM_Init_Valid(void **state) {
    (void)state;

    LinM_ScheduleEntryConfigType entries[] = {
        {.Delay = 10, .FrameIndex = 0, .FrameType = LINM_ENTRY_TYPE_UNCONDITIONAL},
        {.Delay = 10, .FrameIndex = 1, .FrameType = LINM_ENTRY_TYPE_UNCONDITIONAL}
    };

    LinM_ScheduleConfigType schedules[] = {
        {.ScheduleId = LINM_SCHEDULE_NULL, .Entries = NULL, .NumEntries = 0, .Priority = 0, .IsEventTriggered = FALSE},
        {.ScheduleId = LINM_SCHEDULE_MASTER, .Entries = entries, .NumEntries = 2, .Priority = 0, .IsEventTriggered = FALSE}
    };

    LinM_ChannelConfigType channelConfig = {
        .ChannelId = LINM_CHANNEL_0,
        .Schedules = schedules,
        .NumSchedules = 2,
        .NumFrames = 2,
        .ScheduleTimerBase = 10,
        .WakeupSupport = TRUE,
        .SleepSupport = TRUE
    };

    LinM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };

    LinM_Init(&config);

    LinM_ScheduleStatusType status;
    Std_ReturnType result = LinM_GetScheduleStatus(LINM_CHANNEL_0, &status);

    assert_int_equal(result, E_OK);
    assert_int_equal(status, LINM_SCHEDULE_IDLE);
}

/* Test: Init with NULL config */
static void test_LinM_Init_NullConfig(void **state) {
    (void)state;

    LinM_Init(NULL_PTR);

    assert_int_equal(Det_ReportError_CallCount, 1);
}

/* Test: GetVersionInfo */
#if (LINM_VERSION_INFO_API == STD_ON)
static void test_LinM_GetVersionInfo_Valid(void **state) {
    (void)state;

    Std_VersionInfoType versionInfo;
    LinM_GetVersionInfo(&versionInfo);

    assert_int_equal(versionInfo.vendorID, LINM_VENDOR_ID);
    assert_int_equal(versionInfo.moduleID, LINM_MODULE_ID);
    assert_int_equal(versionInfo.sw_major_version, LINM_SW_MAJOR_VERSION);
    assert_int_equal(versionInfo.sw_minor_version, LINM_SW_MINOR_VERSION);
}

static void test_LinM_GetVersionInfo_NullPointer(void **state) {
    (void)state;

    LinM_GetVersionInfo(NULL_PTR);

    assert_int_equal(Det_ReportError_CallCount, 1);
}
#endif

/* Test: Schedule operations */
static void test_LinM_ScheduleOperations(void **state) {
    (void)state;

    LinM_ScheduleEntryConfigType entries[] = {
        {.Delay = 10, .FrameIndex = 0, .FrameType = LINM_ENTRY_TYPE_UNCONDITIONAL},
        {.Delay = 10, .FrameIndex = 1, .FrameType = LINM_ENTRY_TYPE_UNCONDITIONAL}
    };

    LinM_ScheduleConfigType schedules[] = {
        {.ScheduleId = LINM_SCHEDULE_NULL, .Entries = NULL, .NumEntries = 0, .Priority = 0, .IsEventTriggered = FALSE},
        {.ScheduleId = LINM_SCHEDULE_MASTER, .Entries = entries, .NumEntries = 2, .Priority = 0, .IsEventTriggered = FALSE}
    };

    LinM_ChannelConfigType channelConfig = {
        .ChannelId = LINM_CHANNEL_0,
        .Schedules = schedules,
        .NumSchedules = 2,
        .NumFrames = 2,
        .ScheduleTimerBase = 10,
        .WakeupSupport = TRUE,
        .SleepSupport = TRUE
    };

    LinM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };

    LinM_Init(&config);

    /* Initialize schedule */
    Std_ReturnType result = LinM_InitSchedule(LINM_CHANNEL_0, LINM_SCHEDULE_MASTER);
    assert_int_equal(result, E_OK);

    /* Start schedule */
    result = LinM_StartSchedule(LINM_CHANNEL_0, LINM_SCHEDULE_MASTER);
    assert_int_equal(result, E_OK);

    /* Check status */
    LinM_ScheduleStatusType status;
    result = LinM_GetScheduleStatus(LINM_CHANNEL_0, &status);
    assert_int_equal(result, E_OK);
    assert_int_equal(status, LINM_SCHEDULE_RUNNING);

    /* Stop schedule */
    result = LinM_StopSchedule(LINM_CHANNEL_0);
    assert_int_equal(result, E_OK);

    /* Check status after stop */
    result = LinM_GetScheduleStatus(LINM_CHANNEL_0, &status);
    assert_int_equal(result, E_OK);
    assert_int_equal(status, LINM_SCHEDULE_IDLE);
}

/* Test: SetScheduleMode */
static void test_LinM_SetScheduleMode(void **state) {
    (void)state;

    LinM_ScheduleEntryConfigType entries[] = {
        {.Delay = 10, .FrameIndex = 0, .FrameType = LINM_ENTRY_TYPE_UNCONDITIONAL}
    };

    LinM_ScheduleConfigType schedules[] = {
        {.ScheduleId = LINM_SCHEDULE_MASTER, .Entries = entries, .NumEntries = 1, .Priority = 0, .IsEventTriggered = FALSE}
    };

    LinM_ChannelConfigType channelConfig = {
        .ChannelId = LINM_CHANNEL_0,
        .Schedules = schedules,
        .NumSchedules = 1,
        .NumFrames = 1,
        .ScheduleTimerBase = 10,
        .WakeupSupport = TRUE,
        .SleepSupport = TRUE
    };

    LinM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };

    LinM_Init(&config);

    Std_ReturnType result = LinM_SetScheduleMode(LINM_CHANNEL_0, LINM_SCHEDULE_MODE_STARTED);
    assert_int_equal(result, E_OK);

    LinM_ScheduleStatusType status;
    result = LinM_GetScheduleStatus(LINM_CHANNEL_0, &status);
    assert_int_equal(result, E_OK);
    assert_int_equal(status, LINM_SCHEDULE_RUNNING);
}

/* Test: WakeUp */
static void test_LinM_WakeUp(void **state) {
    (void)state;

    LinM_ChannelConfigType channelConfig = {
        .ChannelId = LINM_CHANNEL_0,
        .Schedules = NULL,
        .NumSchedules = 0,
        .NumFrames = 0,
        .ScheduleTimerBase = 10,
        .WakeupSupport = TRUE,
        .SleepSupport = TRUE
    };

    LinM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };

    LinM_Init(&config);

    Std_ReturnType result = LinM_WakeUp(LINM_CHANNEL_0);
    assert_int_equal(result, E_OK);
}

/* Test: GotoSleep */
static void test_LinM_GotoSleep(void **state) {
    (void)state;

    LinM_ChannelConfigType channelConfig = {
        .ChannelId = LINM_CHANNEL_0,
        .Schedules = NULL,
        .NumSchedules = 0,
        .NumFrames = 0,
        .ScheduleTimerBase = 10,
        .WakeupSupport = TRUE,
        .SleepSupport = TRUE
    };

    LinM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };

    LinM_Init(&config);

    Std_ReturnType result = LinM_GotoSleep(LINM_CHANNEL_0);
    assert_int_equal(result, E_OK);
}

/* Test: MainFunction */
static void test_LinM_MainFunction(void **state) {
    (void)state;

    LinM_ScheduleEntryConfigType entries[] = {
        {.Delay = 1, .FrameIndex = 0, .FrameType = LINM_ENTRY_TYPE_UNCONDITIONAL}
    };

    LinM_ScheduleConfigType schedules[] = {
        {.ScheduleId = LINM_SCHEDULE_MASTER, .Entries = entries, .NumEntries = 1, .Priority = 0, .IsEventTriggered = FALSE}
    };

    LinM_ChannelConfigType channelConfig = {
        .ChannelId = LINM_CHANNEL_0,
        .Schedules = schedules,
        .NumSchedules = 1,
        .NumFrames = 1,
        .ScheduleTimerBase = 5,
        .WakeupSupport = TRUE,
        .SleepSupport = TRUE
    };

    LinM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };

    LinM_Init(&config);
    LinM_StartSchedule(LINM_CHANNEL_0, LINM_SCHEDULE_MASTER);

    /* Call MainFunction multiple times */
    for (int i = 0; i < 100; i++) {
        LinM_MainFunction();
    }

    /* Verify no crash */
    assert_true(TRUE);
}

/* Test: GetSlaveResponse */
static void test_LinM_GetSlaveResponse(void **state) {
    (void)state;

    LinM_ChannelConfigType channelConfig = {
        .ChannelId = LINM_CHANNEL_0,
        .Schedules = NULL,
        .NumSchedules = 0,
        .NumFrames = 0,
        .ScheduleTimerBase = 10,
        .WakeupSupport = TRUE,
        .SleepSupport = TRUE
    };

    LinM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };

    LinM_Init(&config);

    LinM_SlaveResponseStatusType status;
    Std_ReturnType result = LinM_GetSlaveResponse(LINM_CHANNEL_0, &status);

    assert_int_equal(result, E_OK);
}

/* Test Suite */
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_LinM_Init_Valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinM_Init_NullConfig, setup, teardown),
#if (LINM_VERSION_INFO_API == STD_ON)
        cmocka_unit_test_setup_teardown(test_LinM_GetVersionInfo_Valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinM_GetVersionInfo_NullPointer, setup, teardown),
#endif
        cmocka_unit_test_setup_teardown(test_LinM_ScheduleOperations, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinM_SetScheduleMode, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinM_WakeUp, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinM_GotoSleep, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinM_MainFunction, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinM_GetSlaveResponse, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
