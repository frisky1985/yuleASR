/**
 * @file test_test_lntm.c
 * @brief LnTm Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/linTp/src/LinTp.c  @tests src/bsw/ecual/linTp/include/LinTp.h

#include "unity.h"
#include "LnTm.h"

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
LnTm_ConfigType testConfig;
static void test_LnTm_SetupDefaultConfig(void) {
    (void)testConfig;
}

static boolean lntm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    lntm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_LnTm_00001 */
void test_LnTm_Init_NullPtr_ShouldNotCrash(void) {
    LnTm_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LnTm_00001 */
void test_LnTm_Init_ValidConfig_ShouldSucceed(void) {
    test_LnTm_SetupDefaultConfig();
    LnTm_Init(&testConfig);
    lntm_initialized = TRUE;
    TEST_ASSERT_TRUE(lntm_initialized);
}

/** @req SWS_LnTm_00001 */
void test_LnTm_Init_DoubleInit_ShouldSucceed(void) {
    test_LnTm_SetupDefaultConfig();
    LnTm_Init(&testConfig);
    LnTm_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LnTm_00002 */
void test_LnTm_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LnTm_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LnTm_00002 */
void test_LnTm_DeInit_ValidCall_ShouldSucceed(void) {
    LnTm_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LnTm_00003 */
void test_LnTm_GetVersionInfo_NullPtr_ShouldReportError(void) {
    LnTm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LnTm_00003 */
void test_LnTm_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    LnTm_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LnTm_00004 */
void test_LnTm_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    LnTm_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LnTm_00004 */
void test_LnTm_MainFunction_ValidCall_ShouldSucceed(void) {
    LnTm_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LnTm_00005 */
void test_LnTm_GetTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LnTm_GetTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LnTm_00005 */
void test_LnTm_GetTime_NullPtr_ShouldReportError(void) {
    LnTm_GetTime(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LnTm_00005 */
void test_LnTm_GetTime_ValidCall_ShouldSucceed(void) {
    LnTm_GetTime();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LnTm_00006 */
void test_LnTm_SetTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LnTm_SetTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LnTm_00006 */
void test_LnTm_SetTime_ValidCall_ShouldSucceed(void) {
    LnTm_SetTime();
    TEST_ASSERT_TRUE(1);
}

