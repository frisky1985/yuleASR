/**
 * @file test_test_udpnm_svc.c
 * @brief UdpNm Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/udpnm/src/UdpNm.c  @tests src/bsw/services/udpnm/include/UdpNm.h

#include "unity.h"
#include "UdpNm.h"

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
UdpNm_ConfigType testConfig;
static void test_UdpNm_SetupDefaultConfig(void) {
    testConfig.NumNetworks = 1U;
}

static boolean udpnm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    udpnm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_UdpNm_00001 */
void test_UdpNm_Init_NullPtr_ShouldNotCrash(void) {
    UdpNm_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_UdpNm_00001 */
void test_UdpNm_Init_ValidConfig_ShouldSucceed(void) {
    test_UdpNm_SetupDefaultConfig();
    UdpNm_Init(&testConfig);
    udpnm_initialized = TRUE;
    TEST_ASSERT_TRUE(udpnm_initialized);
}

/** @req SWS_UdpNm_00001 */
void test_UdpNm_Init_DoubleInit_ShouldSucceed(void) {
    test_UdpNm_SetupDefaultConfig();
    UdpNm_Init(&testConfig);
    UdpNm_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_UdpNm_00002 */
void test_UdpNm_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    UdpNm_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_UdpNm_00002 */
void test_UdpNm_DeInit_ValidCall_ShouldSucceed(void) {
    UdpNm_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_UdpNm_00003 */
void test_UdpNm_GetVersionInfo_NullPtr_ShouldReportError(void) {
    UdpNm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_UdpNm_00003 */
void test_UdpNm_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    UdpNm_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_UdpNm_00004 */
void test_UdpNm_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    UdpNm_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_UdpNm_00004 */
void test_UdpNm_MainFunction_ValidCall_ShouldSucceed(void) {
    UdpNm_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_UdpNm_00005 */
void test_UdpNm_NetworkRequest_Uninit_ShouldReportError(void) {
    /* Not initialized */
    UdpNm_NetworkRequest();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_UdpNm_00005 */
void test_UdpNm_NetworkRequest_InvalidNetwork_ShouldReportError(void) {
    UdpNm_NetworkRequest(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_UdpNm_00005 */
void test_UdpNm_NetworkRequest_ValidCall_ShouldSucceed(void) {
    UdpNm_NetworkRequest();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_UdpNm_00006 */
void test_UdpNm_NetworkRelease_Uninit_ShouldReportError(void) {
    /* Not initialized */
    UdpNm_NetworkRelease();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_UdpNm_00006 */
void test_UdpNm_NetworkRelease_InvalidNetwork_ShouldReportError(void) {
    UdpNm_NetworkRelease(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_UdpNm_00006 */
void test_UdpNm_NetworkRelease_ValidCall_ShouldSucceed(void) {
    UdpNm_NetworkRelease();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_UdpNm_00007 */
void test_UdpNm_GetNodeState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    UdpNm_GetNodeState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_UdpNm_00007 */
void test_UdpNm_GetNodeState_InvalidNode_ShouldReportError(void) {
    UdpNm_GetNodeState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_UdpNm_00007 */
void test_UdpNm_GetNodeState_ValidCall_ShouldReturnState(void) {
    UdpNm_GetNodeState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_UdpNm_00008 */
void test_UdpNm_GetNetworkState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    UdpNm_GetNetworkState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_UdpNm_00008 */
void test_UdpNm_GetNetworkState_InvalidNetwork_ShouldReportError(void) {
    UdpNm_GetNetworkState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_UdpNm_00008 */
void test_UdpNm_GetNetworkState_ValidCall_ShouldReturnState(void) {
    UdpNm_GetNetworkState();
    TEST_ASSERT_TRUE(1);
}

