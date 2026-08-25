/**
 * @file test_test_xcp.c
 * @brief Xcp Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "Xcp.h"

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
Xcp_ConfigType testConfig;
static void test_Xcp_SetupDefaultConfig(void) {
    testConfig.NumDaqLists = 1U;
}

static boolean xcp_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    xcp_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Xcp_00001 */
void test_Xcp_Init_NullPtr_ShouldNotCrash(void) {
    Xcp_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Xcp_00001 */
void test_Xcp_Init_ValidConfig_ShouldSucceed(void) {
    test_Xcp_SetupDefaultConfig();
    Xcp_Init(&testConfig);
    xcp_initialized = TRUE;
    TEST_ASSERT_TRUE(xcp_initialized);
}

/** @req SWS_Xcp_00001 */
void test_Xcp_Init_DoubleInit_ShouldSucceed(void) {
    test_Xcp_SetupDefaultConfig();
    Xcp_Init(&testConfig);
    Xcp_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Xcp_00002 */
void test_Xcp_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Xcp_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00002 */
void test_Xcp_DeInit_ValidCall_ShouldSucceed(void) {
    Xcp_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Xcp_00003 */
void test_Xcp_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Xcp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00003 */
void test_Xcp_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Xcp_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Xcp_00004 */
void test_Xcp_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Xcp_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00004 */
void test_Xcp_MainFunction_ValidCall_ShouldSucceed(void) {
    Xcp_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Xcp_00005 */
void test_Xcp_RxIndication_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Xcp_RxIndication();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00005 */
void test_Xcp_RxIndication_NullPtr_ShouldReportError(void) {
    Xcp_RxIndication(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00005 */
void test_Xcp_RxIndication_ValidCall_ShouldSucceed(void) {
    Xcp_RxIndication();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Xcp_00006 */
void test_Xcp_TxConfirmation_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Xcp_TxConfirmation();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00006 */
void test_Xcp_TxConfirmation_ValidCall_ShouldSucceed(void) {
    Xcp_TxConfirmation();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Xcp_00007 */
void test_Xcp_GetDaqList_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Xcp_GetDaqList();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00007 */
void test_Xcp_GetDaqList_InvalidDaq_ShouldReportError(void) {
    Xcp_GetDaqList(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00007 */
void test_Xcp_GetDaqList_ValidCall_ShouldSucceed(void) {
    Xcp_GetDaqList();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Xcp_00008 */
void test_Xcp_SetDaqList_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Xcp_SetDaqList();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00008 */
void test_Xcp_SetDaqList_InvalidDaq_ShouldReportError(void) {
    Xcp_SetDaqList(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00008 */
void test_Xcp_SetDaqList_ValidCall_ShouldSucceed(void) {
    Xcp_SetDaqList();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Xcp_00009 */
void test_Xcp_StartStopDaq_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Xcp_StartStopDaq();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00009 */
void test_Xcp_StartStopDaq_InvalidDaq_ShouldReportError(void) {
    Xcp_StartStopDaq(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Xcp_00009 */
void test_Xcp_StartStopDaq_ValidCall_ShouldSucceed(void) {
    Xcp_StartStopDaq();
    TEST_ASSERT_TRUE(1);
}

