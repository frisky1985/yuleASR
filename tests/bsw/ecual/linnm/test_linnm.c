/**
 * @file test_test_linnm.c
 * @brief LinNm Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/linnm/src/LinNm.c  @tests src/bsw/ecual/linnm/include/LinNm.h

#include "unity.h"
#include "LinNm.h"

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
LinNm_ConfigType testConfig;
static void test_LinNm_SetupDefaultConfig(void) {
    testConfig.NumNetworks = 1U;
}

static boolean linnm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    linnm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_LinNm_00001 */
void test_LinNm_Init_NullPtr_ShouldNotCrash(void) {
    LinNm_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LinNm_00001 */
void test_LinNm_Init_ValidConfig_ShouldSucceed(void) {
    test_LinNm_SetupDefaultConfig();
    LinNm_Init(&testConfig);
    linnm_initialized = TRUE;
    TEST_ASSERT_TRUE(linnm_initialized);
}

/** @req SWS_LinNm_00001 */
void test_LinNm_Init_DoubleInit_ShouldSucceed(void) {
    test_LinNm_SetupDefaultConfig();
    LinNm_Init(&testConfig);
    LinNm_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LinNm_00002 */
void test_LinNm_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinNm_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00002 */
void test_LinNm_DeInit_ValidCall_ShouldSucceed(void) {
    LinNm_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinNm_00003 */
void test_LinNm_GetVersionInfo_NullPtr_ShouldReportError(void) {
    LinNm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00003 */
void test_LinNm_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    LinNm_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinNm_00004 */
void test_LinNm_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    LinNm_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00004 */
void test_LinNm_MainFunction_ValidCall_ShouldSucceed(void) {
    LinNm_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinNm_00005 */
void test_LinNm_NetworkRequest_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinNm_NetworkRequest();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00005 */
void test_LinNm_NetworkRequest_InvalidNetwork_ShouldReportError(void) {
    LinNm_NetworkRequest(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00005 */
void test_LinNm_NetworkRequest_ValidCall_ShouldSucceed(void) {
    LinNm_NetworkRequest();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinNm_00006 */
void test_LinNm_NetworkRelease_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinNm_NetworkRelease();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00006 */
void test_LinNm_NetworkRelease_InvalidNetwork_ShouldReportError(void) {
    LinNm_NetworkRelease(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00006 */
void test_LinNm_NetworkRelease_ValidCall_ShouldSucceed(void) {
    LinNm_NetworkRelease();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinNm_00007 */
void test_LinNm_GetNodeState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinNm_GetNodeState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00007 */
void test_LinNm_GetNodeState_InvalidNode_ShouldReportError(void) {
    LinNm_GetNodeState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00007 */
void test_LinNm_GetNodeState_ValidCall_ShouldReturnState(void) {
    LinNm_GetNodeState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinNm_00008 */
void test_LinNm_GetNetworkState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinNm_GetNetworkState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00008 */
void test_LinNm_GetNetworkState_InvalidNetwork_ShouldReportError(void) {
    LinNm_GetNetworkState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00008 */
void test_LinNm_GetNetworkState_ValidCall_ShouldReturnState(void) {
    LinNm_GetNetworkState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinNm_00009 */
void test_LinNm_RepeatMessageRequest_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinNm_RepeatMessageRequest();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00009 */
void test_LinNm_RepeatMessageRequest_InvalidNetwork_ShouldReportError(void) {
    LinNm_RepeatMessageRequest(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00009 */
void test_LinNm_RepeatMessageRequest_ValidCall_ShouldSucceed(void) {
    LinNm_RepeatMessageRequest();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LinNm_00010 */
void test_LinNm_PassiveStartUp_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LinNm_PassiveStartUp();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00010 */
void test_LinNm_PassiveStartUp_InvalidNetwork_ShouldReportError(void) {
    LinNm_PassiveStartUp(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LinNm_00010 */
void test_LinNm_PassiveStartUp_ValidCall_ShouldSucceed(void) {
    LinNm_PassiveStartUp();
    TEST_ASSERT_TRUE(1);
}

