/**
 * @file test_linsm.c
 * @brief LinSM (LIN State Manager) Unit Tests
 * @req SWS_LinSM
 *
 * Real assertions against the fully implemented LinSM SUT
 * (src/bsw/services/linsm/src/LinSM.c, LINSM_DEV_ERROR_DETECT = STD_ON).
 * Development-error reports are captured by the local Det mock below.
 */

// @tests src/bsw/services/linsm/src/LinSM.c  @tests src/bsw/services/linsm/include/LinSM.h
#include "unity.h"
#include "LinSM.h"

/* Mock Det_ReportError — captures report arguments (LINSM_DEV_ERROR_DETECT = STD_ON) */
static uint16 mock_DetLastModuleId = 0xFFFFU;
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastModuleId = 0xFFFFU;
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)InstanceId;
    mock_DetLastModuleId = ModuleId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test configuration: one channel, initial schedule = master schedule */
static const LinSM_ChannelConfigType testChannelConfig[LINSM_NUMBER_OF_CHANNELS] = {
    { 0U, LINSM_SCHEDULE_MASTER, FALSE, 100U, 5U }
};
static const LinSM_ConfigType testConfig = {
    testChannelConfig,          /* ChannelConfig */
    LINSM_NUMBER_OF_CHANNELS,   /* NumChannels */
    10U,                        /* MainFunctionPeriod */
    TRUE,                       /* DevErrorDetect */
    TRUE,                       /* VersionInfoApi */
    FALSE                       /* CommunicationControlSupport */
};

void setUp(void) {
    /* Return the SUT to a clean, uninitialized state (DeInit is a guarded
     * no-op when not initialized). Any DET report it may emit is cleared. */
    LinSM_DeInit();
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_LinSM_00001 */
void test_LinSM_Init_NullPtr_ReportsDetWithExactIds(void) {
    LinSM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(LINSM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(LINSM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINSM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinSM_00001 */
void test_LinSM_Init_ValidConfig_DoesNotReportDet(void) {
    LinSM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LinSM_00003 */
void test_LinSM_GetVersionInfo_ValidPtr_FillsVersion(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    LinSM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(LINSM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(LINSM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(LINSM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LinSM_00003 */
void test_LinSM_GetVersionInfo_NullPtr_ReportsDet(void) {
    LinSM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINSM_SID_GET_VERSION_INFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINSM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinSM_00002 */
void test_LinSM_DeInit_BeforeInit_ReportsNotInitialized(void) {
    mock_Det_Reset();
    LinSM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINSM_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_LinSM_00002 */
void test_LinSM_DeInit_AfterInit_LeavesModuleUninitialized(void) {
    LinSM_ModeType mode = LINSM_FULL_COM;
    LinSM_Init(&testConfig);
    LinSM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    Std_ReturnType ret = LinSM_GetCurrentComMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_LinSM_00007 */
void test_LinSM_GetCurrentComMode_AfterInit_ReturnsNoCom(void) {
    LinSM_ModeType mode = LINSM_FULL_COM;
    LinSM_Init(&testConfig);
    Std_ReturnType ret = LinSM_GetCurrentComMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(LINSM_NO_COM, mode);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LinSM_00008 */
void test_LinSM_MainFunction_TransitionsInitToRun_FullCom(void) {
    LinSM_ModeType mode = LINSM_NO_COM;
    LinSM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, LinSM_GetCurrentComMode(0U, &mode));
    TEST_ASSERT_EQUAL(LINSM_NO_COM, mode);
    LinSM_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, LinSM_GetCurrentComMode(0U, &mode));
    TEST_ASSERT_EQUAL(LINSM_FULL_COM, mode);
}

/** @req SWS_LinSM_00004 */
void test_LinSM_ScheduleRequest_BeforeInit_ReportsNotInitialized(void) {
    Std_ReturnType ret = LinSM_ScheduleRequest(0U, LINSM_SCHEDULE_MASTER);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINSM_SID_SCHEDULE_REQUEST, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_LinSM_00004 */
void test_LinSM_ScheduleRequest_ValidSchedule_ReturnsOk(void) {
    Std_ReturnType ret;
    LinSM_Init(&testConfig);
    ret = LinSM_ScheduleRequest(0U, LINSM_SCHEDULE_DIAGNOSTIC);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LinSM_00004 */
void test_LinSM_ScheduleRequest_InvalidChannel_ReportsInvalidParameter(void) {
    Std_ReturnType ret;
    LinSM_Init(&testConfig);
    ret = LinSM_ScheduleRequest((LinSM_ChannelType)LINSM_NUMBER_OF_CHANNELS, LINSM_SCHEDULE_MASTER);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINSM_E_INVALID_PARAMETER, mock_DetLastErrorId);
}

/** @req SWS_LinSM_00004 */
void test_LinSM_ScheduleRequest_InvalidSchedule_ReportsInvalidSchedule(void) {
    Std_ReturnType ret;
    LinSM_Init(&testConfig);
    ret = LinSM_ScheduleRequest(0U, (LinSM_ScheduleType)LINSM_NUMBER_OF_SCHEDULES);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINSM_E_INVALID_SCHEDULE, mock_DetLastErrorId);
}

/** @req SWS_LinSM_00005 */
void test_LinSM_GetCurrentSchedule_NullPtr_ReportsInvalidPointer(void) {
    Std_ReturnType ret;
    LinSM_Init(&testConfig);
    ret = LinSM_GetCurrentSchedule(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINSM_SID_GET_CURRENT_SCHEDULE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINSM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinSM_00005 */
void test_LinSM_GetCurrentSchedule_AfterInit_ReturnsInitialSchedule(void) {
    LinSM_ScheduleType schedule = 0xFFU;
    Std_ReturnType ret;
    LinSM_Init(&testConfig);
    ret = LinSM_GetCurrentSchedule(0U, &schedule);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(LINSM_SCHEDULE_MASTER, schedule);
}

/** @req SWS_LinSM_00009 */
void test_LinSM_ScheduleConfirmation_UpdatesCurrentSchedule(void) {
    LinSM_ScheduleType schedule = 0xFFU;
    Std_ReturnType ret;
    LinSM_Init(&testConfig);
    (void)LinSM_ScheduleRequest(0U, LINSM_SCHEDULE_DIAGNOSTIC);
    LinSM_ScheduleConfirmation(0U, LINSM_SCHEDULE_DIAGNOSTIC);
    ret = LinSM_GetCurrentSchedule(0U, &schedule);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(LINSM_SCHEDULE_DIAGNOSTIC, schedule);
}

/** @req SWS_LinSM_00006 */
void test_LinSM_RequestComMode_SilentCom_NotSupported(void) {
    Std_ReturnType ret;
    LinSM_Init(&testConfig);
    ret = LinSM_RequestComMode(0U, LINSM_SILENT_COM);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_LinSM_00006 */
void test_LinSM_RequestComMode_FullCom_WakeUpConfirmYieldsFullCom(void) {
    LinSM_ModeType mode = LINSM_NO_COM;
    Std_ReturnType ret;
    LinSM_Init(&testConfig);
    ret = LinSM_RequestComMode(0U, LINSM_FULL_COM);
    TEST_ASSERT_EQUAL(E_OK, ret);
    LinSM_WakeUpConfirmation(0U, TRUE);
    TEST_ASSERT_EQUAL(E_OK, LinSM_GetCurrentComMode(0U, &mode));
    TEST_ASSERT_EQUAL(LINSM_FULL_COM, mode);
}

/** @req SWS_LinSM_00006 */
void test_LinSM_RequestComMode_NoCom_GotoSleepConfirmYieldsNoCom(void) {
    LinSM_ModeType mode = LINSM_FULL_COM;
    Std_ReturnType ret;
    LinSM_Init(&testConfig);
    LinSM_MainFunction(); /* INIT -> RUN: ComMode becomes FULL_COM */
    ret = LinSM_RequestComMode(0U, LINSM_NO_COM);
    TEST_ASSERT_EQUAL(E_OK, ret);
    LinSM_GotoSleepConfirmation(0U, TRUE);
    TEST_ASSERT_EQUAL(E_OK, LinSM_GetCurrentComMode(0U, &mode));
    TEST_ASSERT_EQUAL(LINSM_NO_COM, mode);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_LinSM_Init_NullPtr_ReportsDetWithExactIds);
    RUN_TEST(test_LinSM_Init_ValidConfig_DoesNotReportDet);
    RUN_TEST(test_LinSM_GetVersionInfo_ValidPtr_FillsVersion);
    RUN_TEST(test_LinSM_GetVersionInfo_NullPtr_ReportsDet);
    RUN_TEST(test_LinSM_DeInit_BeforeInit_ReportsNotInitialized);
    RUN_TEST(test_LinSM_DeInit_AfterInit_LeavesModuleUninitialized);
    RUN_TEST(test_LinSM_GetCurrentComMode_AfterInit_ReturnsNoCom);
    RUN_TEST(test_LinSM_MainFunction_TransitionsInitToRun_FullCom);
    RUN_TEST(test_LinSM_ScheduleRequest_BeforeInit_ReportsNotInitialized);
    RUN_TEST(test_LinSM_ScheduleRequest_ValidSchedule_ReturnsOk);
    RUN_TEST(test_LinSM_ScheduleRequest_InvalidChannel_ReportsInvalidParameter);
    RUN_TEST(test_LinSM_ScheduleRequest_InvalidSchedule_ReportsInvalidSchedule);
    RUN_TEST(test_LinSM_GetCurrentSchedule_NullPtr_ReportsInvalidPointer);
    RUN_TEST(test_LinSM_GetCurrentSchedule_AfterInit_ReturnsInitialSchedule);
    RUN_TEST(test_LinSM_ScheduleConfirmation_UpdatesCurrentSchedule);
    RUN_TEST(test_LinSM_RequestComMode_SilentCom_NotSupported);
    RUN_TEST(test_LinSM_RequestComMode_FullCom_WakeUpConfirmYieldsFullCom);
    RUN_TEST(test_LinSM_RequestComMode_NoCom_GotoSleepConfirmYieldsNoCom);

    return UnityEnd();
}
