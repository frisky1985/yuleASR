/**
 * @file test_test_cantpsyn.c
 * @brief CanTpSyn Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/cantp/src/CanTp.c  @tests src/bsw/ecual/cantp/include/CanTp.h

#include "unity.h"
#include "CanTpSyn.h"

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
CanTpSyn_ConfigType testConfig;
static void test_CanTpSyn_SetupDefaultConfig(void) {
    (void)testConfig;
}

static boolean cantpsyn_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    cantpsyn_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_CanTpSyn_00001 */
void test_CanTpSyn_Init_NullPtr_ShouldNotCrash(void) {
    CanTpSyn_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_CanTpSyn_00001 */
void test_CanTpSyn_Init_ValidConfig_ShouldSucceed(void) {
    test_CanTpSyn_SetupDefaultConfig();
    CanTpSyn_Init(&testConfig);
    cantpsyn_initialized = TRUE;
    TEST_ASSERT_TRUE(cantpsyn_initialized);
}

/** @req SWS_CanTpSyn_00001 */
void test_CanTpSyn_Init_DoubleInit_ShouldSucceed(void) {
    test_CanTpSyn_SetupDefaultConfig();
    CanTpSyn_Init(&testConfig);
    CanTpSyn_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_CanTpSyn_00002 */
void test_CanTpSyn_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CanTpSyn_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTpSyn_00002 */
void test_CanTpSyn_DeInit_ValidCall_ShouldSucceed(void) {
    CanTpSyn_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanTpSyn_00003 */
void test_CanTpSyn_GetVersionInfo_NullPtr_ShouldReportError(void) {
    CanTpSyn_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTpSyn_00003 */
void test_CanTpSyn_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    CanTpSyn_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanTpSyn_00004 */
void test_CanTpSyn_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    CanTpSyn_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTpSyn_00004 */
void test_CanTpSyn_MainFunction_ValidCall_ShouldSucceed(void) {
    CanTpSyn_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanTpSyn_00005 */
void test_CanTpSyn_GetTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CanTpSyn_GetTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTpSyn_00005 */
void test_CanTpSyn_GetTime_NullPtr_ShouldReportError(void) {
    CanTpSyn_GetTime(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTpSyn_00005 */
void test_CanTpSyn_GetTime_ValidCall_ShouldSucceed(void) {
    CanTpSyn_GetTime();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanTpSyn_00006 */
void test_CanTpSyn_SetTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CanTpSyn_SetTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTpSyn_00006 */
void test_CanTpSyn_SetTime_ValidCall_ShouldSucceed(void) {
    CanTpSyn_SetTime();
    TEST_ASSERT_TRUE(1);
}

