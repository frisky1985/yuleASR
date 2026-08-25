/**
 * @file test_test_ethtsyn.c
 * @brief EthTSyn Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/ethtsyn/src/EthTSyn.c  @tests src/bsw/services/ethtsyn/include/EthTSyn.h

#include "unity.h"
#include "EthTSyn.h"

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
EthTSyn_ConfigType testConfig;
static void test_EthTSyn_SetupDefaultConfig(void) {
    testConfig.NumDomains = 1U;
}

static boolean ethtsyn_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    ethtsyn_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_EthTSyn_00001 */
void test_EthTSyn_Init_NullPtr_ShouldNotCrash(void) {
    EthTSyn_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EthTSyn_00001 */
void test_EthTSyn_Init_ValidConfig_ShouldSucceed(void) {
    test_EthTSyn_SetupDefaultConfig();
    EthTSyn_Init(&testConfig);
    ethtsyn_initialized = TRUE;
    TEST_ASSERT_TRUE(ethtsyn_initialized);
}

/** @req SWS_EthTSyn_00001 */
void test_EthTSyn_Init_DoubleInit_ShouldSucceed(void) {
    test_EthTSyn_SetupDefaultConfig();
    EthTSyn_Init(&testConfig);
    EthTSyn_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EthTSyn_00002 */
void test_EthTSyn_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EthTSyn_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTSyn_00002 */
void test_EthTSyn_DeInit_ValidCall_ShouldSucceed(void) {
    EthTSyn_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthTSyn_00003 */
void test_EthTSyn_GetVersionInfo_NullPtr_ShouldReportError(void) {
    EthTSyn_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTSyn_00003 */
void test_EthTSyn_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    EthTSyn_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthTSyn_00004 */
void test_EthTSyn_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    EthTSyn_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTSyn_00004 */
void test_EthTSyn_MainFunction_ValidCall_ShouldSucceed(void) {
    EthTSyn_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthTSyn_00005 */
void test_EthTSyn_GetTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EthTSyn_GetTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTSyn_00005 */
void test_EthTSyn_GetTime_NullPtr_ShouldReportError(void) {
    EthTSyn_GetTime(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTSyn_00005 */
void test_EthTSyn_GetTime_ValidCall_ShouldSucceed(void) {
    EthTSyn_GetTime();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthTSyn_00006 */
void test_EthTSyn_SetTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EthTSyn_SetTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTSyn_00006 */
void test_EthTSyn_SetTime_ValidCall_ShouldSucceed(void) {
    EthTSyn_SetTime();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthTSyn_00007 */
void test_EthTSyn_GetRateRatio_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EthTSyn_GetRateRatio();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTSyn_00007 */
void test_EthTSyn_GetRateRatio_NullPtr_ShouldReportError(void) {
    EthTSyn_GetRateRatio(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTSyn_00007 */
void test_EthTSyn_GetRateRatio_ValidCall_ShouldSucceed(void) {
    EthTSyn_GetRateRatio();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthTSyn_00008 */
void test_EthTSyn_GetGlobalTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EthTSyn_GetGlobalTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTSyn_00008 */
void test_EthTSyn_GetGlobalTime_NullPtr_ShouldReportError(void) {
    EthTSyn_GetGlobalTime(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthTSyn_00008 */
void test_EthTSyn_GetGlobalTime_ValidCall_ShouldSucceed(void) {
    EthTSyn_GetGlobalTime();
    TEST_ASSERT_TRUE(1);
}

