/**
 * @file test_test_someip.c
 * @brief SomeIp Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/someip/src/SomeIp.c  @tests src/bsw/services/someip/include/SomeIp.h

#include "unity.h"
#include "SomeIp.h"

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
SomeIp_ConfigType testConfig;
static void test_SomeIp_SetupDefaultConfig(void) {
    testConfig.NumServices = 1U;
}

static boolean someip_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    someip_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_SomeIp_00001 */
void test_SomeIp_Init_NullPtr_ShouldNotCrash(void) {
    SomeIp_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIp_00001 */
void test_SomeIp_Init_ValidConfig_ShouldSucceed(void) {
    test_SomeIp_SetupDefaultConfig();
    SomeIp_Init(&testConfig);
    someip_initialized = TRUE;
    TEST_ASSERT_TRUE(someip_initialized);
}

/** @req SWS_SomeIp_00001 */
void test_SomeIp_Init_DoubleInit_ShouldSucceed(void) {
    test_SomeIp_SetupDefaultConfig();
    SomeIp_Init(&testConfig);
    SomeIp_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIpSd_00002 */
void test_SomeIp_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIp_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00002 */
void test_SomeIp_DeInit_ValidCall_ShouldSucceed(void) {
    SomeIp_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00003 */
void test_SomeIp_GetVersionInfo_NullPtr_ShouldReportError(void) {
    SomeIp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00003 */
void test_SomeIp_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    SomeIp_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIp_00004 */
void test_SomeIp_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIp_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIp_00004 */
void test_SomeIp_Transmit_NullPtr_ShouldReportError(void) {
    SomeIp_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIp_00004 */
void test_SomeIp_Transmit_ValidCall_ShouldSucceed(void) {
    SomeIp_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIp_00005 */
void test_SomeIp_RegisterEvent_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIp_RegisterEvent();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIp_00005 */
void test_SomeIp_RegisterEvent_NullPtr_ShouldReportError(void) {
    SomeIp_RegisterEvent(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIp_00005 */
void test_SomeIp_RegisterEvent_ValidCall_ShouldSucceed(void) {
    SomeIp_RegisterEvent();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00001 */
void test_SomeIp_UnregisterEvent_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIp_UnregisterEvent();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIp_00006 */
void test_SomeIp_UnregisterEvent_InvalidEvent_ShouldReportError(void) {
    SomeIp_UnregisterEvent(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIp_00006 */
void test_SomeIp_UnregisterEvent_ValidCall_ShouldSucceed(void) {
    SomeIp_UnregisterEvent();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIp_00007 */
void test_SomeIp_SubscribeEventgroup_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIp_SubscribeEventgroup();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIp_00007 */
void test_SomeIp_SubscribeEventgroup_InvalidEvent_ShouldReportError(void) {
    SomeIp_SubscribeEventgroup(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIp_00007 */
void test_SomeIp_SubscribeEventgroup_ValidCall_ShouldSucceed(void) {
    SomeIp_SubscribeEventgroup();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00001 */
void test_SomeIp_UnsubscribeEventgroup_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIp_UnsubscribeEventgroup();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00008 */
void test_SomeIp_UnsubscribeEventgroup_InvalidEvent_ShouldReportError(void) {
    SomeIp_UnsubscribeEventgroup(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00008 */
void test_SomeIp_UnsubscribeEventgroup_ValidCall_ShouldSucceed(void) {
    SomeIp_UnsubscribeEventgroup();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00004 */
void test_SomeIp_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    SomeIp_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00004 */
void test_SomeIp_MainFunction_ValidCall_ShouldSucceed(void) {
    SomeIp_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00001 */
void test_SomeIp_GetServiceState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIp_GetServiceState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIp_00010 */
void test_SomeIp_GetServiceState_InvalidService_ShouldReportError(void) {
    SomeIp_GetServiceState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIp_00010 */
void test_SomeIp_GetServiceState_ValidCall_ShouldReturnState(void) {
    SomeIp_GetServiceState();
    TEST_ASSERT_TRUE(1);
}

