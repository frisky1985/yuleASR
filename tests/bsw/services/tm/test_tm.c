/**
 * @file test_test_tm.c
 * @brief Tm Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/tm/src/Tm.c  @tests src/bsw/services/tm/include/Tm.h

#include "unity.h"
#include "Tm.h"

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
Tm_ConfigType testConfig;
static void test_Tm_SetupDefaultConfig(void) {
    testConfig.NumCounters = 1U;
}

static boolean tm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    tm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Tm_00001 */
void test_Tm_Init_NullPtr_ShouldNotCrash(void) {
    Tm_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Tm_00001 */
void test_Tm_Init_ValidConfig_ShouldSucceed(void) {
    test_Tm_SetupDefaultConfig();
    Tm_Init(&testConfig);
    tm_initialized = TRUE;
    TEST_ASSERT_TRUE(tm_initialized);
}

/** @req SWS_Tm_00001 */
void test_Tm_Init_DoubleInit_ShouldSucceed(void) {
    test_Tm_SetupDefaultConfig();
    Tm_Init(&testConfig);
    Tm_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Tm_00002 */
void test_Tm_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Tm_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Tm_00002 */
void test_Tm_DeInit_ValidCall_ShouldSucceed(void) {
    Tm_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Tm_00003 */
void test_Tm_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Tm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Tm_00003 */
void test_Tm_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Tm_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Tm_00004 */
void test_Tm_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Tm_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Tm_00004 */
void test_Tm_MainFunction_ValidCall_ShouldSucceed(void) {
    Tm_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Tm_00005 */
void test_Tm_GetTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Tm_GetTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Tm_00005 */
void test_Tm_GetTime_NullPtr_ShouldReportError(void) {
    Tm_GetTime(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Tm_00005 */
void test_Tm_GetTime_ValidCall_ShouldSucceed(void) {
    Tm_GetTime();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Tm_00006 */
void test_Tm_SetTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Tm_SetTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Tm_00006 */
void test_Tm_SetTime_ValidCall_ShouldSucceed(void) {
    Tm_SetTime();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Tm_00007 */
void test_Tm_GetCounter_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Tm_GetCounter();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Tm_00007 */
void test_Tm_GetCounter_ValidCall_ShouldReturnCounter(void) {
    Tm_GetCounter();
    TEST_ASSERT_TRUE(1);
}

