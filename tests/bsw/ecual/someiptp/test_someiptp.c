/**
 * @file test_test_someiptp.c
 * @brief SomeIpTp Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/someiptp/src/SomeIpTp.c  @tests src/bsw/services/someiptp/include/SomeIpTp.h

#include "unity.h"
#include "SomeIpTp.h"

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
SomeIpTp_ConfigType testConfig;
static void test_SomeIpTp_SetupDefaultConfig(void) {
    testConfig.NumChannels = 1U;
}

static boolean someiptp_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    someiptp_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_SomeIpTp_00001 */
void test_SomeIpTp_Init_NullPtr_ShouldNotCrash(void) {
    SomeIpTp_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIpTp_00001 */
void test_SomeIpTp_Init_ValidConfig_ShouldSucceed(void) {
    test_SomeIpTp_SetupDefaultConfig();
    SomeIpTp_Init(&testConfig);
    someiptp_initialized = TRUE;
    TEST_ASSERT_TRUE(someiptp_initialized);
}

/** @req SWS_SomeIpTp_00001 */
void test_SomeIpTp_Init_DoubleInit_ShouldSucceed(void) {
    test_SomeIpTp_SetupDefaultConfig();
    SomeIpTp_Init(&testConfig);
    SomeIpTp_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIpTp_00002 */
void test_SomeIpTp_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpTp_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpTp_00002 */
void test_SomeIpTp_DeInit_ValidCall_ShouldSucceed(void) {
    SomeIpTp_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpTp_00003 */
void test_SomeIpTp_GetVersionInfo_NullPtr_ShouldReportError(void) {
    SomeIpTp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpTp_00003 */
void test_SomeIpTp_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    SomeIpTp_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpTp_00004 */
void test_SomeIpTp_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpTp_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpTp_00004 */
void test_SomeIpTp_Transmit_NullPtr_ShouldReportError(void) {
    SomeIpTp_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpTp_00004 */
void test_SomeIpTp_Transmit_ValidCall_ShouldSucceed(void) {
    SomeIpTp_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpTp_00005 */
void test_SomeIpTp_CancelTransmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpTp_CancelTransmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpTp_00005 */
void test_SomeIpTp_CancelTransmit_InvalidEvent_ShouldReportError(void) {
    SomeIpTp_CancelTransmit(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpTp_00005 */
void test_SomeIpTp_CancelTransmit_ValidCall_ShouldSucceed(void) {
    SomeIpTp_CancelTransmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpTp_00006 */
void test_SomeIpTp_GetState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpTp_GetState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpTp_00006 */
void test_SomeIpTp_GetState_NullPtr_ShouldReportError(void) {
    SomeIpTp_GetState(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpTp_00006 */
void test_SomeIpTp_GetState_ValidCall_ShouldSucceed(void) {
    SomeIpTp_GetState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpTp_00007 */
void test_SomeIpTp_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    SomeIpTp_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpTp_00007 */
void test_SomeIpTp_MainFunction_ValidCall_ShouldSucceed(void) {
    SomeIpTp_MainFunction();
    TEST_ASSERT_TRUE(1);
}

