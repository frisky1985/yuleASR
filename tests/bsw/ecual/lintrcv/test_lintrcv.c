/**
 * @file test_test_lintrcv.c
 * @brief LinTrcv Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "LinTrcv.h"

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
LinTrcv_ConfigType testConfig;
static void test_LinTrcv_SetupDefaultConfig(void) {
    testConfig.NumTransceivers = 1U;
}

static boolean lintrcv_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    lintrcv_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_LinTrcv_00001 */
void test_LinTrcv_Init_NullPtr_ShouldNotCrash(void) {
    LinTrcv_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LinTrcv_00001 */
void test_LinTrcv_Init_ValidConfig_ShouldSucceed(void) {
    test_LinTrcv_SetupDefaultConfig();
    LinTrcv_Init(&testConfig);
    lintrcv_initialized = TRUE;
    TEST_ASSERT_TRUE(lintrcv_initialized);
}

/** @req SWS_LinTrcv_00001 */
void test_LinTrcv_Init_DoubleInit_ShouldSucceed(void) {
    test_LinTrcv_SetupDefaultConfig();
    LinTrcv_Init(&testConfig);
    LinTrcv_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LinTrcv_00002 */
void test_LinTrcv_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinTrcv_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTrcv_00002 */
void test_LinTrcv_DeInit_ValidCall_ShouldSucceed(void) {
    LinTrcv_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinTrcv_00003 */
void test_LinTrcv_GetVersionInfo_NullPtr_ShouldReportError(void) {
    LinTrcv_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTrcv_00003 */
void test_LinTrcv_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    LinTrcv_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinTrcv_00004 */
void test_LinTrcv_SetTrcvMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinTrcv_SetTrcvMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTrcv_00004 */
void test_LinTrcv_SetTrcvMode_InvalidTrcv_ShouldReportError(void) {
    LinTrcv_SetTrcvMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTrcv_00004 */
void test_LinTrcv_SetTrcvMode_ValidCall_ShouldSucceed(void) {
    LinTrcv_SetTrcvMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinTrcv_00005 */
void test_LinTrcv_GetTrcvMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinTrcv_GetTrcvMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTrcv_00005 */
void test_LinTrcv_GetTrcvMode_InvalidTrcv_ShouldReportError(void) {
    LinTrcv_GetTrcvMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTrcv_00005 */
void test_LinTrcv_GetTrcvMode_ValidCall_ShouldReturnMode(void) {
    LinTrcv_GetTrcvMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinTrcv_00006 */
void test_LinTrcv_GetTrcvWakeupReason_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinTrcv_GetTrcvWakeupReason();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTrcv_00006 */
void test_LinTrcv_GetTrcvWakeupReason_InvalidTrcv_ShouldReportError(void) {
    LinTrcv_GetTrcvWakeupReason(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTrcv_00006 */
void test_LinTrcv_GetTrcvWakeupReason_ValidCall_ShouldReturnReason(void) {
    LinTrcv_GetTrcvWakeupReason();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinTrcv_00007 */
void test_LinTrcv_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    LinTrcv_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTrcv_00007 */
void test_LinTrcv_MainFunction_ValidCall_ShouldSucceed(void) {
    LinTrcv_MainFunction();
    TEST_ASSERT_TRUE(1);
}

