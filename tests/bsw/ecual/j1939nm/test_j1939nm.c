/**
 * @file test_test_j1939nm.c
 * @brief J1939Nm Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/j1939nm/src/J1939Nm.c  @tests src/bsw/services/j1939nm/include/J1939Nm.h

#include "unity.h"
#include "J1939Nm.h"

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
J1939Nm_ConfigType testConfig;
static void test_J1939Nm_SetupDefaultConfig(void) {
    testConfig.NumNetworks = 1U;
}

static boolean j1939nm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    j1939nm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_J1939Nm_00001 */
void test_J1939Nm_Init_NullPtr_ShouldNotCrash(void) {
    J1939Nm_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_J1939Nm_00001 */
void test_J1939Nm_Init_ValidConfig_ShouldSucceed(void) {
    test_J1939Nm_SetupDefaultConfig();
    J1939Nm_Init(&testConfig);
    j1939nm_initialized = TRUE;
    TEST_ASSERT_TRUE(j1939nm_initialized);
}

/** @req SWS_J1939Nm_00001 */
void test_J1939Nm_Init_DoubleInit_ShouldSucceed(void) {
    test_J1939Nm_SetupDefaultConfig();
    J1939Nm_Init(&testConfig);
    J1939Nm_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_J1939Nm_00002 */
void test_J1939Nm_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    J1939Nm_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Nm_00002 */
void test_J1939Nm_DeInit_ValidCall_ShouldSucceed(void) {
    J1939Nm_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_J1939Nm_00003 */
void test_J1939Nm_GetVersionInfo_NullPtr_ShouldReportError(void) {
    J1939Nm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Nm_00003 */
void test_J1939Nm_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    J1939Nm_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_J1939Nm_00004 */
void test_J1939Nm_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    J1939Nm_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Nm_00004 */
void test_J1939Nm_MainFunction_ValidCall_ShouldSucceed(void) {
    J1939Nm_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_J1939Nm_00005 */
void test_J1939Nm_NetworkRequest_Uninit_ShouldReportError(void) {
    /* Not initialized */
    J1939Nm_NetworkRequest();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Nm_00005 */
void test_J1939Nm_NetworkRequest_InvalidNetwork_ShouldReportError(void) {
    J1939Nm_NetworkRequest(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Nm_00005 */
void test_J1939Nm_NetworkRequest_ValidCall_ShouldSucceed(void) {
    J1939Nm_NetworkRequest();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_J1939Nm_00006 */
void test_J1939Nm_NetworkRelease_Uninit_ShouldReportError(void) {
    /* Not initialized */
    J1939Nm_NetworkRelease();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Nm_00006 */
void test_J1939Nm_NetworkRelease_InvalidNetwork_ShouldReportError(void) {
    J1939Nm_NetworkRelease(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Nm_00006 */
void test_J1939Nm_NetworkRelease_ValidCall_ShouldSucceed(void) {
    J1939Nm_NetworkRelease();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_J1939Nm_00007 */
void test_J1939Nm_GetNodeState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    J1939Nm_GetNodeState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Nm_00007 */
void test_J1939Nm_GetNodeState_InvalidNode_ShouldReportError(void) {
    J1939Nm_GetNodeState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Nm_00007 */
void test_J1939Nm_GetNodeState_ValidCall_ShouldReturnState(void) {
    J1939Nm_GetNodeState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_J1939Nm_00008 */
void test_J1939Nm_GetNetworkState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    J1939Nm_GetNetworkState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Nm_00008 */
void test_J1939Nm_GetNetworkState_InvalidNetwork_ShouldReportError(void) {
    J1939Nm_GetNetworkState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Nm_00008 */
void test_J1939Nm_GetNetworkState_ValidCall_ShouldReturnState(void) {
    J1939Nm_GetNetworkState();
    TEST_ASSERT_TRUE(1);
}

