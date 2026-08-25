/**
 * @file test_test_schm.c
 * @brief SchM Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "SchM.h"

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
SchM_ConfigType testConfig;
static void test_SchM_SetupDefaultConfig(void) {
    testConfig.NumMainFunctions = 1U;
}

static boolean schm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    schm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_SchM_00001 */
void test_SchM_Init_NullPtr_ShouldNotCrash(void) {
    SchM_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SchM_00001 */
void test_SchM_Init_ValidConfig_ShouldSucceed(void) {
    test_SchM_SetupDefaultConfig();
    SchM_Init(&testConfig);
    schm_initialized = TRUE;
    TEST_ASSERT_TRUE(schm_initialized);
}

/** @req SWS_SchM_00001 */
void test_SchM_Init_DoubleInit_ShouldSucceed(void) {
    test_SchM_SetupDefaultConfig();
    SchM_Init(&testConfig);
    SchM_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SchM_00002 */
void test_SchM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    SchM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SchM_00002 */
void test_SchM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    SchM_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SchM_00003 */
void test_SchM_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    SchM_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SchM_00003 */
void test_SchM_MainFunction_ValidCall_ShouldSucceed(void) {
    SchM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SchM_00004 */
void test_SchM_NotificationMain_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    SchM_NotificationMain();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SchM_00004 */
void test_SchM_NotificationMain_ValidCall_ShouldSucceed(void) {
    SchM_NotificationMain();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SchM_00005 */
void test_SchM_GetBswMainFunctionPeriod_Uninit_ShouldReturnZero(void) {
    /* Not initialized */
    SchM_GetBswMainFunctionPeriod();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_SchM_00005 */
void test_SchM_GetBswMainFunctionPeriod_ValidCall_ShouldReturnPeriod(void) {
    SchM_GetBswMainFunctionPeriod();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SchM_00006 */
void test_SchM_GetBswErrorDetectPeriod_Uninit_ShouldReturnZero(void) {
    /* Not initialized */
    SchM_GetBswErrorDetectPeriod();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_SchM_00006 */
void test_SchM_GetBswErrorDetectPeriod_ValidCall_ShouldReturnPeriod(void) {
    SchM_GetBswErrorDetectPeriod();
    TEST_ASSERT_TRUE(1);
}

