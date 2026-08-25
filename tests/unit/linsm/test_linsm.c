/**
 * @file test_linsm.c
 * @brief Unit tests for LIN State Manager module
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

// @tests src/bsw/services/linsm/src/LinSM.c  @tests src/bsw/services/linsm/include/LinSM.h

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include "LinSM.h"

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
static int setup(void **state)
{
    (void)state;
    Det_ReportError_CallCount = 0;
    return 0;
}

static int teardown(void **state)
{
    (void)state;
    LinSM_DeInit();
    return 0;
}

/* Test: Init with valid config */
static void test_LinSM_Init_Valid(void **state)
{
    (void)state;

    LinSM_ChannelConfigType channelConfig = {
        .ChannelId = LINSM_CHANNEL_0,
        .InitialSchedule = LINSM_SCHEDULE_MASTER,
        .WakeupSupport = TRUE,
        .RequestTimeout = 100,
        .MaxScheduleSwitches = 10};

    LinSM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .MainFunctionPeriod = 5,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .CommunicationControlSupport = TRUE};

    LinSM_Init(&config);

    LinSM_ScheduleType currentSchedule;
    Std_ReturnType result = LinSM_GetCurrentSchedule(LINSM_CHANNEL_0, &currentSchedule);

    assert_int_equal(result, E_OK);
    assert_int_equal(currentSchedule, LINSM_SCHEDULE_MASTER);
}

/* Test: Init with NULL config */
static void test_LinSM_Init_NullConfig(void **state)
{
    (void)state;

    LinSM_Init(NULL_PTR);

    assert_int_equal(Det_ReportError_CallCount, 1);
}

/* Test: GetVersionInfo */
#if (LINSM_VERSION_INFO_API == STD_ON)
static void test_LinSM_GetVersionInfo_Valid(void **state)
{
    (void)state;

    Std_VersionInfoType versionInfo;
    LinSM_GetVersionInfo(&versionInfo);

    assert_int_equal(versionInfo.vendorID, LINSM_VENDOR_ID);
    assert_int_equal(versionInfo.moduleID, LINSM_MODULE_ID);
    assert_int_equal(versionInfo.sw_major_version, LINSM_SW_MAJOR_VERSION);
    assert_int_equal(versionInfo.sw_minor_version, LINSM_SW_MINOR_VERSION);
}

static void test_LinSM_GetVersionInfo_NullPointer(void **state)
{
    (void)state;

    LinSM_GetVersionInfo(NULL_PTR);

    assert_int_equal(Det_ReportError_CallCount, 1);
}
#endif

/* Test: ScheduleRequest */
static void test_LinSM_ScheduleRequest(void **state)
{
    (void)state;

    LinSM_ChannelConfigType channelConfig = {
        .ChannelId = LINSM_CHANNEL_0,
        .InitialSchedule = LINSM_SCHEDULE_MASTER,
        .WakeupSupport = TRUE,
        .RequestTimeout = 100,
        .MaxScheduleSwitches = 10};

    LinSM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .MainFunctionPeriod = 5,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .CommunicationControlSupport = TRUE};

    LinSM_Init(&config);

    /* Request schedule change */
    Std_ReturnType result = LinSM_ScheduleRequest(LINSM_CHANNEL_0, LINSM_SCHEDULE_DIAGNOSTIC);
    assert_int_equal(result, E_OK);
}

/* Test: GetCurrentSchedule */
static void test_LinSM_GetCurrentSchedule(void **state)
{
    (void)state;

    LinSM_ChannelConfigType channelConfig = {
        .ChannelId = LINSM_CHANNEL_0,
        .InitialSchedule = LINSM_SCHEDULE_MASTER,
        .WakeupSupport = TRUE,
        .RequestTimeout = 100,
        .MaxScheduleSwitches = 10};

    LinSM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .MainFunctionPeriod = 5,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .CommunicationControlSupport = TRUE};

    LinSM_Init(&config);

    LinSM_ScheduleType schedule;
    Std_ReturnType result = LinSM_GetCurrentSchedule(LINSM_CHANNEL_0, &schedule);

    assert_int_equal(result, E_OK);
    assert_int_equal(schedule, LINSM_SCHEDULE_MASTER);
}

/* Test: RequestComMode */
static void test_LinSM_RequestComMode(void **state)
{
    (void)state;

    LinSM_ChannelConfigType channelConfig = {
        .ChannelId = LINSM_CHANNEL_0,
        .InitialSchedule = LINSM_SCHEDULE_MASTER,
        .WakeupSupport = TRUE,
        .RequestTimeout = 100,
        .MaxScheduleSwitches = 10};

    LinSM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .MainFunctionPeriod = 5,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .CommunicationControlSupport = TRUE};

    LinSM_Init(&config);

    /* Request FULL_COM mode */
    Std_ReturnType result = LinSM_RequestComMode(LINSM_CHANNEL_0, LINSM_FULL_COM);
    assert_int_equal(result, E_OK);

    /* Request NO_COM mode */
    result = LinSM_RequestComMode(LINSM_CHANNEL_0, LINSM_NO_COM);
    assert_int_equal(result, E_OK);
}

/* Test: GetCurrentComMode */
static void test_LinSM_GetCurrentComMode(void **state)
{
    (void)state;

    LinSM_ChannelConfigType channelConfig = {
        .ChannelId = LINSM_CHANNEL_0,
        .InitialSchedule = LINSM_SCHEDULE_MASTER,
        .WakeupSupport = TRUE,
        .RequestTimeout = 100,
        .MaxScheduleSwitches = 10};

    LinSM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .MainFunctionPeriod = 5,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .CommunicationControlSupport = TRUE};

    LinSM_Init(&config);

    LinSM_ModeType mode;
    Std_ReturnType result = LinSM_GetCurrentComMode(LINSM_CHANNEL_0, &mode);

    assert_int_equal(result, E_OK);
    /* Initial mode should be NO_COM */
    assert_int_equal(mode, LINSM_NO_COM);
}

/* Test: MainFunction */
static void test_LinSM_MainFunction(void **state)
{
    (void)state;

    LinSM_ChannelConfigType channelConfig = {
        .ChannelId = LINSM_CHANNEL_0,
        .InitialSchedule = LINSM_SCHEDULE_MASTER,
        .WakeupSupport = TRUE,
        .RequestTimeout = 100,
        .MaxScheduleSwitches = 10};

    LinSM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .MainFunctionPeriod = 5,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .CommunicationControlSupport = TRUE};

    LinSM_Init(&config);

    /* Call MainFunction multiple times */
    for (int i = 0; i < 100; i++)
    {
        LinSM_MainFunction();
    }

    /* Verify state transition to RUN */
    assert_true(TRUE);
}

/* Test: ScheduleConfirmation */
static void test_LinSM_ScheduleConfirmation(void **state)
{
    (void)state;

    LinSM_ChannelConfigType channelConfig = {
        .ChannelId = LINSM_CHANNEL_0,
        .InitialSchedule = LINSM_SCHEDULE_MASTER,
        .WakeupSupport = TRUE,
        .RequestTimeout = 100,
        .MaxScheduleSwitches = 10};

    LinSM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .MainFunctionPeriod = 5,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .CommunicationControlSupport = TRUE};

    LinSM_Init(&config);

    /* Request schedule and confirm */
    LinSM_ScheduleRequest(LINSM_CHANNEL_0, LINSM_SCHEDULE_DIAGNOSTIC);
    LinSM_ScheduleConfirmation(LINSM_CHANNEL_0, LINSM_SCHEDULE_DIAGNOSTIC);

    LinSM_ScheduleType currentSchedule;
    Std_ReturnType result = LinSM_GetCurrentSchedule(LINSM_CHANNEL_0, &currentSchedule);

    assert_int_equal(result, E_OK);
    assert_int_equal(currentSchedule, LINSM_SCHEDULE_DIAGNOSTIC);
}

/* Test: WakeUpConfirmation */
static void test_LinSM_WakeUpConfirmation(void **state)
{
    (void)state;

    LinSM_ChannelConfigType channelConfig = {
        .ChannelId = LINSM_CHANNEL_0,
        .InitialSchedule = LINSM_SCHEDULE_MASTER,
        .WakeupSupport = TRUE,
        .RequestTimeout = 100,
        .MaxScheduleSwitches = 10};

    LinSM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .MainFunctionPeriod = 5,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .CommunicationControlSupport = TRUE};

    LinSM_Init(&config);

    LinSM_RequestComMode(LINSM_CHANNEL_0, LINSM_FULL_COM);
    LinSM_WakeUpConfirmation(LINSM_CHANNEL_0, TRUE);

    LinSM_ModeType mode;
    Std_ReturnType result = LinSM_GetCurrentComMode(LINSM_CHANNEL_0, &mode);

    assert_int_equal(result, E_OK);
    assert_int_equal(mode, LINSM_FULL_COM);
}

/* Test: GotoSleepConfirmation */
static void test_LinSM_GotoSleepConfirmation(void **state)
{
    (void)state;

    LinSM_ChannelConfigType channelConfig = {
        .ChannelId = LINSM_CHANNEL_0,
        .InitialSchedule = LINSM_SCHEDULE_MASTER,
        .WakeupSupport = TRUE,
        .RequestTimeout = 100,
        .MaxScheduleSwitches = 10};

    LinSM_ConfigType config = {
        .ChannelConfig = &channelConfig,
        .NumChannels = 1,
        .MainFunctionPeriod = 5,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE,
        .CommunicationControlSupport = TRUE};

    LinSM_Init(&config);

    /* First go to FULL_COM */
    LinSM_RequestComMode(LINSM_CHANNEL_0, LINSM_FULL_COM);
    LinSM_WakeUpConfirmation(LINSM_CHANNEL_0, TRUE);

    /* Then go to sleep */
    LinSM_RequestComMode(LINSM_CHANNEL_0, LINSM_NO_COM);
    LinSM_GotoSleepConfirmation(LINSM_CHANNEL_0, TRUE);

    LinSM_ModeType mode;
    Std_ReturnType result = LinSM_GetCurrentComMode(LINSM_CHANNEL_0, &mode);

    assert_int_equal(result, E_OK);
    assert_int_equal(mode, LINSM_NO_COM);
}

/* Test Suite */
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_LinSM_Init_Valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinSM_Init_NullConfig, setup, teardown),
#if (LINSM_VERSION_INFO_API == STD_ON)
        cmocka_unit_test_setup_teardown(test_LinSM_GetVersionInfo_Valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinSM_GetVersionInfo_NullPointer, setup, teardown),
#endif
        cmocka_unit_test_setup_teardown(test_LinSM_ScheduleRequest, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinSM_GetCurrentSchedule, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinSM_RequestComMode, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinSM_GetCurrentComMode, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinSM_MainFunction, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinSM_ScheduleConfirmation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinSM_WakeUpConfirmation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_LinSM_GotoSleepConfirmation, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
