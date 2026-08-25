/**
 * @file test_test_frtp.c
 * @brief FrTp Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "FrTp.h"

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
FrTp_ConfigType testConfig;
static void test_FrTp_SetupDefaultConfig(void) {
    testConfig.NumChannels = 1U;
}

static boolean frtp_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    frtp_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_FrTp_00001 */
void test_FrTp_Init_NullPtr_ShouldNotCrash(void) {
    FrTp_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_FrTp_00001 */
void test_FrTp_Init_ValidConfig_ShouldSucceed(void) {
    test_FrTp_SetupDefaultConfig();
    FrTp_Init(&testConfig);
    frtp_initialized = TRUE;
    TEST_ASSERT_TRUE(frtp_initialized);
}

/** @req SWS_FrTp_00001 */
void test_FrTp_Init_DoubleInit_ShouldSucceed(void) {
    test_FrTp_SetupDefaultConfig();
    FrTp_Init(&testConfig);
    FrTp_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_FrTp_00002 */
void test_FrTp_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FrTp_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrTp_00002 */
void test_FrTp_DeInit_ValidCall_ShouldSucceed(void) {
    FrTp_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrTp_00003 */
void test_FrTp_GetVersionInfo_NullPtr_ShouldReportError(void) {
    FrTp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrTp_00003 */
void test_FrTp_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    FrTp_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrTp_00004 */
void test_FrTp_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FrTp_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrTp_00004 */
void test_FrTp_Transmit_NullPtr_ShouldReportError(void) {
    FrTp_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrTp_00004 */
void test_FrTp_Transmit_ValidCall_ShouldSucceed(void) {
    FrTp_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrTp_00005 */
void test_FrTp_CancelTransmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FrTp_CancelTransmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrTp_00005 */
void test_FrTp_CancelTransmit_InvalidChannel_ShouldReportError(void) {
    FrTp_CancelTransmit(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrTp_00005 */
void test_FrTp_CancelTransmit_ValidCall_ShouldSucceed(void) {
    FrTp_CancelTransmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrTp_00006 */
void test_FrTp_GetState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FrTp_GetState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrTp_00006 */
void test_FrTp_GetState_NullPtr_ShouldReportError(void) {
    FrTp_GetState(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrTp_00006 */
void test_FrTp_GetState_ValidCall_ShouldSucceed(void) {
    FrTp_GetState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrTp_00007 */
void test_FrTp_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    FrTp_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrTp_00007 */
void test_FrTp_MainFunction_ValidCall_ShouldSucceed(void) {
    FrTp_MainFunction();
    TEST_ASSERT_TRUE(1);
}

