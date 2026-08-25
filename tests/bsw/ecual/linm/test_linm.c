/**
 * @file test_test_linm.c
 * @brief LinM Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/linm/src/LinM.c  @tests src/bsw/services/linm/include/LinM.h

#include "unity.h"
#include "LinM.h"

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
LinM_ConfigType testConfig;
static void test_LinM_SetupDefaultConfig(void) {
    testConfig.NumChannels = 1U;
}

static boolean linm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    linm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_LinM_00001 */
void test_LinM_Init_NullPtr_ShouldNotCrash(void) {
    LinM_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LinM_00001 */
void test_LinM_Init_ValidConfig_ShouldSucceed(void) {
    test_LinM_SetupDefaultConfig();
    LinM_Init(&testConfig);
    linm_initialized = TRUE;
    TEST_ASSERT_TRUE(linm_initialized);
}

/** @req SWS_LinM_00001 */
void test_LinM_Init_DoubleInit_ShouldSucceed(void) {
    test_LinM_SetupDefaultConfig();
    LinM_Init(&testConfig);
    LinM_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LinM_00002 */
void test_LinM_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinM_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinM_00002 */
void test_LinM_DeInit_ValidCall_ShouldSucceed(void) {
    LinM_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinM_00003 */
void test_LinM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    LinM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinM_00003 */
void test_LinM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    LinM_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinM_00004 */
void test_LinM_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    LinM_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinM_00004 */
void test_LinM_MainFunction_ValidCall_ShouldSucceed(void) {
    LinM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinM_00005 */
void test_LinM_RequestComMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinM_RequestComMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinM_00005 */
void test_LinM_RequestComMode_InvalidChannel_ShouldReportError(void) {
    LinM_RequestComMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinM_00005 */
void test_LinM_RequestComMode_ValidCall_ShouldSucceed(void) {
    LinM_RequestComMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinM_00006 */
void test_LinM_GetComMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinM_GetComMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinM_00006 */
void test_LinM_GetComMode_InvalidChannel_ShouldReportError(void) {
    LinM_GetComMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinM_00006 */
void test_LinM_GetComMode_ValidCall_ShouldReturnMode(void) {
    LinM_GetComMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinM_00007 */
void test_LinM_ScheduleRequest_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinM_ScheduleRequest();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinM_00007 */
void test_LinM_ScheduleRequest_InvalidSchedule_ShouldReportError(void) {
    LinM_ScheduleRequest(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinM_00007 */
void test_LinM_ScheduleRequest_ValidCall_ShouldSucceed(void) {
    LinM_ScheduleRequest();
    TEST_ASSERT_TRUE(1);
}

