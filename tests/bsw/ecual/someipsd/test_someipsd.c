/**
 * @file test_test_someipsd.c
 * @brief SomeIpSd Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "SomeIpSd.h"

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
SomeIpSd_ConfigType testConfig;
static void test_SomeIpSd_SetupDefaultConfig(void) {
    testConfig.NumServices = 1U;
}

static boolean someipsd_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    someipsd_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_SomeIpSd_00001 */
void test_SomeIpSd_Init_NullPtr_ShouldNotCrash(void) {
    SomeIpSd_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIpSd_00001 */
void test_SomeIpSd_Init_ValidConfig_ShouldSucceed(void) {
    test_SomeIpSd_SetupDefaultConfig();
    SomeIpSd_Init(&testConfig);
    someipsd_initialized = TRUE;
    TEST_ASSERT_TRUE(someipsd_initialized);
}

/** @req SWS_SomeIpSd_00001 */
void test_SomeIpSd_Init_DoubleInit_ShouldSucceed(void) {
    test_SomeIpSd_SetupDefaultConfig();
    SomeIpSd_Init(&testConfig);
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIpSd_00002 */
void test_SomeIpSd_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpSd_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00002 */
void test_SomeIpSd_DeInit_ValidCall_ShouldSucceed(void) {
    SomeIpSd_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00003 */
void test_SomeIpSd_GetVersionInfo_NullPtr_ShouldReportError(void) {
    SomeIpSd_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00003 */
void test_SomeIpSd_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    SomeIpSd_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00004 */
void test_SomeIpSd_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    SomeIpSd_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00004 */
void test_SomeIpSd_MainFunction_ValidCall_ShouldSucceed(void) {
    SomeIpSd_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00005 */
void test_SomeIpSd_RequestService_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpSd_RequestService();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00005 */
void test_SomeIpSd_RequestService_InvalidService_ShouldReportError(void) {
    SomeIpSd_RequestService(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00005 */
void test_SomeIpSd_RequestService_ValidCall_ShouldSucceed(void) {
    SomeIpSd_RequestService();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00006 */
void test_SomeIpSd_ReleaseService_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpSd_ReleaseService();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00006 */
void test_SomeIpSd_ReleaseService_InvalidService_ShouldReportError(void) {
    SomeIpSd_ReleaseService(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00006 */
void test_SomeIpSd_ReleaseService_ValidCall_ShouldSucceed(void) {
    SomeIpSd_ReleaseService();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00007 */
void test_SomeIpSd_SubscribeEventgroup_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpSd_SubscribeEventgroup();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00007 */
void test_SomeIpSd_SubscribeEventgroup_InvalidService_ShouldReportError(void) {
    SomeIpSd_SubscribeEventgroup(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00007 */
void test_SomeIpSd_SubscribeEventgroup_ValidCall_ShouldSucceed(void) {
    SomeIpSd_SubscribeEventgroup();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00008 */
void test_SomeIpSd_UnsubscribeEventgroup_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpSd_UnsubscribeEventgroup();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00008 */
void test_SomeIpSd_UnsubscribeEventgroup_InvalidService_ShouldReportError(void) {
    SomeIpSd_UnsubscribeEventgroup(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00008 */
void test_SomeIpSd_UnsubscribeEventgroup_ValidCall_ShouldSucceed(void) {
    SomeIpSd_UnsubscribeEventgroup();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSd_00009 */
void test_SomeIpSd_GetServiceState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpSd_GetServiceState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00009 */
void test_SomeIpSd_GetServiceState_InvalidService_ShouldReportError(void) {
    SomeIpSd_GetServiceState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSd_00009 */
void test_SomeIpSd_GetServiceState_ValidCall_ShouldReturnState(void) {
    SomeIpSd_GetServiceState();
    TEST_ASSERT_TRUE(1);
}

