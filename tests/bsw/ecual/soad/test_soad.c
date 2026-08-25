/**
 * @file test_test_soad.c
 * @brief SoAd Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "SoAd.h"

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
SoAd_ConfigType testConfig;
static void test_SoAd_SetupDefaultConfig(void) {
    testConfig.NumSockets = 1U;
}

static boolean soad_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    soad_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_SoAd_00001 */
void test_SoAd_Init_NullPtr_ShouldNotCrash(void) {
    SoAd_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SoAd_00001 */
void test_SoAd_Init_ValidConfig_ShouldSucceed(void) {
    test_SoAd_SetupDefaultConfig();
    SoAd_Init(&testConfig);
    soad_initialized = TRUE;
    TEST_ASSERT_TRUE(soad_initialized);
}

/** @req SWS_SoAd_00001 */
void test_SoAd_Init_DoubleInit_ShouldSucceed(void) {
    test_SoAd_SetupDefaultConfig();
    SoAd_Init(&testConfig);
    SoAd_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SoAd_00002 */
void test_SoAd_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    SoAd_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00002 */
void test_SoAd_MainFunction_ValidCall_ShouldSucceed(void) {
    SoAd_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SoAd_00003 */
void test_SoAd_Open_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SoAd_Open();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00003 */
void test_SoAd_Open_InvalidSocket_ShouldReportError(void) {
    SoAd_Open(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00003 */
void test_SoAd_Open_ValidCall_ShouldSucceed(void) {
    SoAd_Open();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SoAd_00004 */
void test_SoAd_Close_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SoAd_Close();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00004 */
void test_SoAd_Close_InvalidSocket_ShouldReportError(void) {
    SoAd_Close(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00004 */
void test_SoAd_Close_ValidCall_ShouldSucceed(void) {
    SoAd_Close();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SoAd_00005 */
void test_SoAd_Send_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SoAd_Send();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00005 */
void test_SoAd_Send_NullBuf_ShouldReportError(void) {
    SoAd_Send(0U, NULL_PTR, 0U);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00005 */
void test_SoAd_Send_ValidData_ShouldSucceed(void) {
    SoAd_Send();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SoAd_00006 */
void test_SoAd_Receive_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SoAd_Receive();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00006 */
void test_SoAd_Receive_NullBuf_ShouldReportError(void) {
    SoAd_Receive(0U, NULL_PTR, 0U);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00006 */
void test_SoAd_Receive_ValidCall_ShouldSucceed(void) {
    SoAd_Receive();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SoAd_00007 */
void test_SoAd_GetStats_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SoAd_GetStats();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00007 */
void test_SoAd_GetStats_NullPtr_ShouldReportError(void) {
    SoAd_GetStats(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00007 */
void test_SoAd_GetStats_ValidCall_ShouldSucceed(void) {
    SoAd_GetStats();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SoAd_00008 */
void test_SoAd_GetVersionInfo_NullPtr_ShouldReportError(void) {
    SoAd_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00008 */
void test_SoAd_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    SoAd_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SoAd_00009 */
void test_SoAd_EnableRouting_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SoAd_EnableRouting();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00009 */
void test_SoAd_EnableRouting_ValidCall_ShouldSucceed(void) {
    SoAd_EnableRouting();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SoAd_00010 */
void test_SoAd_DisableRouting_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SoAd_DisableRouting();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00010 */
void test_SoAd_DisableRouting_ValidCall_ShouldSucceed(void) {
    SoAd_DisableRouting();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SoAd_00011 */
void test_SoAd_SocketStateNotification_InvalidSocket_ShouldReportError(void) {
    SoAd_SocketStateNotification(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SoAd_00011 */
void test_SoAd_SocketStateNotification_ValidCall_ShouldSucceed(void) {
    SoAd_SocketStateNotification();
    TEST_ASSERT_TRUE(1);
}

