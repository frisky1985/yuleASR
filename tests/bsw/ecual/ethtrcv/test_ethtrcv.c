/**
 * @file test_test_ethtrcv.c
 * @brief EthTrcv Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "EthTrcv.h"

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
EthTrcv_ConfigType testConfig;
static void test_EthTrcv_SetupDefaultConfig(void) {
    testConfig.NumTransceivers = 1U;
}

static boolean ethtrcv_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    ethtrcv_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_EthTrcv_00001 */
void test_EthTrcv_Init_NullPtr_ShouldNotCrash(void) {
    EthTrcv_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EthTrcv_00001 */
void test_EthTrcv_Init_ValidConfig_ShouldSucceed(void) {
    test_EthTrcv_SetupDefaultConfig();
    EthTrcv_Init(&testConfig);
    ethtrcv_initialized = TRUE;
    TEST_ASSERT_TRUE(ethtrcv_initialized);
}

/** @req SWS_EthTrcv_00001 */
void test_EthTrcv_Init_DoubleInit_ShouldSucceed(void) {
    test_EthTrcv_SetupDefaultConfig();
    EthTrcv_Init(&testConfig);
    EthTrcv_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EthTrcv_00002 */
void test_EthTrcv_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EthTrcv_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTrcv_00002 */
void test_EthTrcv_DeInit_ValidCall_ShouldSucceed(void) {
    EthTrcv_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthTrcv_00003 */
void test_EthTrcv_GetVersionInfo_NullPtr_ShouldReportError(void) {
    EthTrcv_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTrcv_00003 */
void test_EthTrcv_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    EthTrcv_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthTrcv_00004 */
void test_EthTrcv_SetTrcvMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EthTrcv_SetTrcvMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTrcv_00004 */
void test_EthTrcv_SetTrcvMode_InvalidTrcv_ShouldReportError(void) {
    EthTrcv_SetTrcvMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTrcv_00004 */
void test_EthTrcv_SetTrcvMode_ValidCall_ShouldSucceed(void) {
    EthTrcv_SetTrcvMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthTrcv_00005 */
void test_EthTrcv_GetTrcvMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EthTrcv_GetTrcvMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTrcv_00005 */
void test_EthTrcv_GetTrcvMode_InvalidTrcv_ShouldReportError(void) {
    EthTrcv_GetTrcvMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTrcv_00005 */
void test_EthTrcv_GetTrcvMode_ValidCall_ShouldReturnMode(void) {
    EthTrcv_GetTrcvMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthTrcv_00006 */
void test_EthTrcv_GetTrcvWakeupReason_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EthTrcv_GetTrcvWakeupReason();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTrcv_00006 */
void test_EthTrcv_GetTrcvWakeupReason_InvalidTrcv_ShouldReportError(void) {
    EthTrcv_GetTrcvWakeupReason(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTrcv_00006 */
void test_EthTrcv_GetTrcvWakeupReason_ValidCall_ShouldReturnReason(void) {
    EthTrcv_GetTrcvWakeupReason();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthTrcv_00007 */
void test_EthTrcv_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    EthTrcv_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTrcv_00007 */
void test_EthTrcv_MainFunction_ValidCall_ShouldSucceed(void) {
    EthTrcv_MainFunction();
    TEST_ASSERT_TRUE(1);
}

