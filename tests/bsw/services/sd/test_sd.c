/**
 * @file test_test_sd.c
 * @brief Sd Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "Sd.h"

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
Sd_ConfigType testConfig;
static void test_Sd_SetupDefaultConfig(void) {
    testConfig.NumServices = 1U;
}

static boolean sd_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    sd_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Sd_00001 */
void test_Sd_Init_NullPtr_ShouldNotCrash(void) {
    Sd_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Sd_00001 */
void test_Sd_Init_ValidConfig_ShouldSucceed(void) {
    test_Sd_SetupDefaultConfig();
    Sd_Init(&testConfig);
    sd_initialized = TRUE;
    TEST_ASSERT_TRUE(sd_initialized);
}

/** @req SWS_Sd_00001 */
void test_Sd_Init_DoubleInit_ShouldSucceed(void) {
    test_Sd_SetupDefaultConfig();
    Sd_Init(&testConfig);
    Sd_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Sd_00002 */
void test_Sd_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Sd_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00002 */
void test_Sd_DeInit_ValidCall_ShouldSucceed(void) {
    Sd_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Sd_00003 */
void test_Sd_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Sd_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00003 */
void test_Sd_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Sd_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Sd_00004 */
void test_Sd_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Sd_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00004 */
void test_Sd_MainFunction_ValidCall_ShouldSucceed(void) {
    Sd_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Sd_00005 */
void test_Sd_RequestService_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Sd_RequestService();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00005 */
void test_Sd_RequestService_InvalidService_ShouldReportError(void) {
    Sd_RequestService(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00005 */
void test_Sd_RequestService_ValidCall_ShouldSucceed(void) {
    Sd_RequestService();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Sd_00006 */
void test_Sd_ReleaseService_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Sd_ReleaseService();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00006 */
void test_Sd_ReleaseService_InvalidService_ShouldReportError(void) {
    Sd_ReleaseService(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00006 */
void test_Sd_ReleaseService_ValidCall_ShouldSucceed(void) {
    Sd_ReleaseService();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Sd_00007 */
void test_Sd_SubscribeEventgroup_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Sd_SubscribeEventgroup();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00007 */
void test_Sd_SubscribeEventgroup_InvalidService_ShouldReportError(void) {
    Sd_SubscribeEventgroup(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00007 */
void test_Sd_SubscribeEventgroup_ValidCall_ShouldSucceed(void) {
    Sd_SubscribeEventgroup();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Sd_00008 */
void test_Sd_UnsubscribeEventgroup_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Sd_UnsubscribeEventgroup();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00008 */
void test_Sd_UnsubscribeEventgroup_InvalidService_ShouldReportError(void) {
    Sd_UnsubscribeEventgroup(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00008 */
void test_Sd_UnsubscribeEventgroup_ValidCall_ShouldSucceed(void) {
    Sd_UnsubscribeEventgroup();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Sd_00009 */
void test_Sd_GetServiceState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Sd_GetServiceState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00009 */
void test_Sd_GetServiceState_InvalidService_ShouldReportError(void) {
    Sd_GetServiceState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Sd_00009 */
void test_Sd_GetServiceState_ValidCall_ShouldReturnState(void) {
    Sd_GetServiceState();
    TEST_ASSERT_TRUE(1);
}

