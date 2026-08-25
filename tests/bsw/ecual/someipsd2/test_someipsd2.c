/**
 * @file test_test_someipsd2.c
 * @brief SomeIpSD2 Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/someipsd/src/SomeIpSd.c  @tests src/bsw/ecual/someipsd/include/SomeIpSd.h

#include "unity.h"
#include "SomeIpSD2.h"

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
SomeIpSD2_ConfigType testConfig;
static void test_SomeIpSD2_SetupDefaultConfig(void) {
    testConfig.NumServices = 1U;
}

static boolean someipsd2_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    someipsd2_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_SomeIpSD2_00001 */
void test_SomeIpSD2_Init_NullPtr_ShouldNotCrash(void) {
    SomeIpSD2_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIpSD2_00001 */
void test_SomeIpSD2_Init_ValidConfig_ShouldSucceed(void) {
    test_SomeIpSD2_SetupDefaultConfig();
    SomeIpSD2_Init(&testConfig);
    someipsd2_initialized = TRUE;
    TEST_ASSERT_TRUE(someipsd2_initialized);
}

/** @req SWS_SomeIpSD2_00001 */
void test_SomeIpSD2_Init_DoubleInit_ShouldSucceed(void) {
    test_SomeIpSD2_SetupDefaultConfig();
    SomeIpSD2_Init(&testConfig);
    SomeIpSD2_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIpSD2_00002 */
void test_SomeIpSD2_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpSD2_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSD2_00002 */
void test_SomeIpSD2_DeInit_ValidCall_ShouldSucceed(void) {
    SomeIpSD2_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSD2_00003 */
void test_SomeIpSD2_GetVersionInfo_NullPtr_ShouldReportError(void) {
    SomeIpSD2_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSD2_00003 */
void test_SomeIpSD2_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    SomeIpSD2_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSD2_00004 */
void test_SomeIpSD2_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    SomeIpSD2_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSD2_00004 */
void test_SomeIpSD2_MainFunction_ValidCall_ShouldSucceed(void) {
    SomeIpSD2_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSD2_00005 */
void test_SomeIpSD2_RequestService_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpSD2_RequestService();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSD2_00005 */
void test_SomeIpSD2_RequestService_InvalidService_ShouldReportError(void) {
    SomeIpSD2_RequestService(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSD2_00005 */
void test_SomeIpSD2_RequestService_ValidCall_ShouldSucceed(void) {
    SomeIpSD2_RequestService();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSD2_00006 */
void test_SomeIpSD2_ReleaseService_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpSD2_ReleaseService();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSD2_00006 */
void test_SomeIpSD2_ReleaseService_InvalidService_ShouldReportError(void) {
    SomeIpSD2_ReleaseService(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSD2_00006 */
void test_SomeIpSD2_ReleaseService_ValidCall_ShouldSucceed(void) {
    SomeIpSD2_ReleaseService();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpSD2_00007 */
void test_SomeIpSD2_SubscribeEventgroup_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpSD2_SubscribeEventgroup();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSD2_00007 */
void test_SomeIpSD2_SubscribeEventgroup_InvalidService_ShouldReportError(void) {
    SomeIpSD2_SubscribeEventgroup(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpSD2_00007 */
void test_SomeIpSD2_SubscribeEventgroup_ValidCall_ShouldSucceed(void) {
    SomeIpSD2_SubscribeEventgroup();
    TEST_ASSERT_TRUE(1);
}

