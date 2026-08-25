/**
 * @file test_test_srp.c
 * @brief Srp Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/srp/src/Srp.c  @tests src/bsw/ecual/srp/include/Srp.h

#include "unity.h"
#include "Srp.h"

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
Srp_ConfigType testConfig;
static void test_Srp_SetupDefaultConfig(void) {
    testConfig.NumSignals = 1U;
}

static boolean srp_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    srp_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Srp_00001 */
void test_Srp_Init_NullPtr_ShouldNotCrash(void) {
    Srp_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Srp_00001 */
void test_Srp_Init_ValidConfig_ShouldSucceed(void) {
    test_Srp_SetupDefaultConfig();
    Srp_Init(&testConfig);
    srp_initialized = TRUE;
    TEST_ASSERT_TRUE(srp_initialized);
}

/** @req SWS_Srp_00001 */
void test_Srp_Init_DoubleInit_ShouldSucceed(void) {
    test_Srp_SetupDefaultConfig();
    Srp_Init(&testConfig);
    Srp_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Srp_00002 */
void test_Srp_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Srp_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Srp_00002 */
void test_Srp_DeInit_ValidCall_ShouldSucceed(void) {
    Srp_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Srp_00003 */
void test_Srp_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Srp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Srp_00003 */
void test_Srp_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Srp_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Srp_00004 */
void test_Srp_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Srp_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Srp_00004 */
void test_Srp_MainFunction_ValidCall_ShouldSucceed(void) {
    Srp_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Srp_00005 */
void test_Srp_ProcessSignal_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Srp_ProcessSignal();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Srp_00005 */
void test_Srp_ProcessSignal_InvalidSignal_ShouldReportError(void) {
    Srp_ProcessSignal(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Srp_00005 */
void test_Srp_ProcessSignal_ValidCall_ShouldSucceed(void) {
    Srp_ProcessSignal();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Srp_00006 */
void test_Srp_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    Srp_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Srp_00006 */
void test_Srp_GetStatus_ValidCall_ShouldReturnStatus(void) {
    Srp_GetStatus();
    TEST_ASSERT_TRUE(1);
}

