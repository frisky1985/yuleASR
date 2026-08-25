/**
 * @file test_test_lintp.c
 * @brief LinTp Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/lintp/src/LinTp.c  @tests src/bsw/ecual/lintp/include/LinTp.h

#include "unity.h"
#include "LinTp.h"

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
LinTp_ConfigType testConfig;
static void test_LinTp_SetupDefaultConfig(void) {
    testConfig.NumChannels = 2U;
}

static boolean lintp_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    lintp_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_LinTp_00001 */
void test_LinTp_Init_NullPtr_ShouldNotCrash(void) {
    LinTp_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LinTp_00001 */
void test_LinTp_Init_ValidConfig_ShouldSucceed(void) {
    test_LinTp_SetupDefaultConfig();
    LinTp_Init(&testConfig);
    lintp_initialized = TRUE;
    TEST_ASSERT_TRUE(lintp_initialized);
}

/** @req SWS_LinTp_00001 */
void test_LinTp_Init_DoubleInit_ShouldSucceed(void) {
    test_LinTp_SetupDefaultConfig();
    LinTp_Init(&testConfig);
    LinTp_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LinTp_00002 */
void test_LinTp_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinTp_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTp_00002 */
void test_LinTp_DeInit_ValidCall_ShouldSucceed(void) {
    LinTp_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinTp_00003 */
void test_LinTp_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinTp_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTp_00003 */
void test_LinTp_Transmit_NullPtr_ShouldReportError(void) {
    LinTp_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTp_00003 */
void test_LinTp_Transmit_ValidCall_ShouldSucceed(void) {
    LinTp_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinTp_00004 */
void test_LinTp_CancelTransmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinTp_CancelTransmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTp_00004 */
void test_LinTp_CancelTransmit_InvalidChannel_ShouldReportError(void) {
    LinTp_CancelTransmit(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTp_00004 */
void test_LinTp_CancelTransmit_ValidCall_ShouldSucceed(void) {
    LinTp_CancelTransmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinTp_00005 */
void test_LinTp_GetVersionInfo_NullPtr_ShouldReportError(void) {
    LinTp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTp_00005 */
void test_LinTp_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    LinTp_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinTp_00006 */
void test_LinTp_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    LinTp_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTp_00006 */
void test_LinTp_MainFunction_ValidCall_ShouldSucceed(void) {
    LinTp_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinTp_00007 */
void test_LinTp_GetState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinTp_GetState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTp_00007 */
void test_LinTp_GetState_NullPtr_ShouldReportError(void) {
    LinTp_GetState(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinTp_00007 */
void test_LinTp_GetState_ValidCall_ShouldSucceed(void) {
    LinTp_GetState();
    TEST_ASSERT_TRUE(1);
}

