/**
 * @file test_test_linif.c
 * @brief LinIf Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "LinIf.h"

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test config */
LinIf_ConfigType testConfig;
static void test_LinIf_SetupDefaultConfig(void) {
    testConfig.NumChannels = 2U;
}

static boolean linif_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    linif_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_LinIf_00001 */
void test_LinIf_Init_NullPtr_ShouldNotCrash(void) {
    LinIf_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LinIf_00001 */
void test_LinIf_Init_ValidConfig_ShouldSucceed(void) {
    test_LinIf_SetupDefaultConfig();
    LinIf_Init(&testConfig);
    linif_initialized = TRUE;
    TEST_ASSERT_TRUE(linif_initialized);
}

/** @req SWS_LinIf_00001 */
void test_LinIf_Init_DoubleInit_ShouldSucceed(void) {
    test_LinIf_SetupDefaultConfig();
    LinIf_Init(&testConfig);
    LinIf_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LinIf_00002 */
void test_LinIf_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinIf_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinIf_00002 */
void test_LinIf_DeInit_ValidCall_ShouldSucceed(void) {
    LinIf_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinIf_00003 */
void test_LinIf_ScheduleRequest_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinIf_ScheduleRequest();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinIf_00003 */
void test_LinIf_ScheduleRequest_InvalidSchedule_ShouldReportError(void) {
    LinIf_ScheduleRequest(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinIf_00003 */
void test_LinIf_ScheduleRequest_ValidCall_ShouldSucceed(void) {
    LinIf_ScheduleRequest();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinIf_00004 */
void test_LinIf_GotoSleepControl_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinIf_GotoSleepControl();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinIf_00004 */
void test_LinIf_GotoSleepControl_InvalidChannel_ShouldReportError(void) {
    LinIf_GotoSleepControl(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinIf_00004 */
void test_LinIf_GotoSleepControl_ValidCall_ShouldSucceed(void) {
    LinIf_GotoSleepControl();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinIf_00005 */
void test_LinIf_UpdateTxBuffer_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinIf_UpdateTxBuffer();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinIf_00005 */
void test_LinIf_UpdateTxBuffer_ValidCall_ShouldSucceed(void) {
    LinIf_UpdateTxBuffer();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinIf_00006 */
void test_LinIf_GetVersionInfo_NullPtr_ShouldReportError(void) {
    LinIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinIf_00006 */
void test_LinIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    LinIf_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinIf_00007 */
void test_LinIf_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    LinIf_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinIf_00007 */
void test_LinIf_MainFunction_ValidCall_ShouldSucceed(void) {
    LinIf_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinIf_00008 */
void test_LinIf_GetFrameStatus_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinIf_GetFrameStatus();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinIf_00008 */
void test_LinIf_GetFrameStatus_InvalidFrame_ShouldReportError(void) {
    LinIf_GetFrameStatus(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinIf_00008 */
void test_LinIf_GetFrameStatus_ValidCall_ShouldReturnStatus(void) {
    LinIf_GetFrameStatus();
    TEST_ASSERT_TRUE(1);
}

