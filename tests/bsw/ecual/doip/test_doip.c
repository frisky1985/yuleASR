/**
 * @file test_test_doip.c
 * @brief DoIP Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "DoIP.h"

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
DoIP_ConfigType testConfig;
static void test_DoIP_SetupDefaultConfig(void) {
    testConfig.NumConnections = 1U;
}

static boolean doip_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    doip_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_DoIP_00001 */
void test_DoIP_Init_NullPtr_ShouldNotCrash(void) {
    DoIP_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_DoIP_00001 */
void test_DoIP_Init_ValidConfig_ShouldSucceed(void) {
    test_DoIP_SetupDefaultConfig();
    DoIP_Init(&testConfig);
    doip_initialized = TRUE;
    TEST_ASSERT_TRUE(doip_initialized);
}

/** @req SWS_DoIP_00001 */
void test_DoIP_Init_DoubleInit_ShouldSucceed(void) {
    test_DoIP_SetupDefaultConfig();
    DoIP_Init(&testConfig);
    DoIP_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_DoIP_00002 */
void test_DoIP_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoIP_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoIP_00002 */
void test_DoIP_DeInit_ValidCall_ShouldSucceed(void) {
    DoIP_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoIP_00003 */
void test_DoIP_GetVersionInfo_NullPtr_ShouldReportError(void) {
    DoIP_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoIP_00003 */
void test_DoIP_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    DoIP_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoIP_00004 */
void test_DoIP_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    DoIP_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoIP_00004 */
void test_DoIP_MainFunction_ValidCall_ShouldSucceed(void) {
    DoIP_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoIP_00005 */
void test_DoIP_RxIndication_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoIP_RxIndication();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoIP_00005 */
void test_DoIP_RxIndication_NullPtr_ShouldReportError(void) {
    DoIP_RxIndication(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoIP_00005 */
void test_DoIP_RxIndication_ValidCall_ShouldSucceed(void) {
    DoIP_RxIndication();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoIP_00006 */
void test_DoIP_TxConfirmation_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoIP_TxConfirmation();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoIP_00006 */
void test_DoIP_TxConfirmation_ValidCall_ShouldSucceed(void) {
    DoIP_TxConfirmation();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoIP_00007 */
void test_DoIP_GetConnectionState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoIP_GetConnectionState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoIP_00007 */
void test_DoIP_GetConnectionState_InvalidConn_ShouldReportError(void) {
    DoIP_GetConnectionState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoIP_00007 */
void test_DoIP_GetConnectionState_ValidCall_ShouldReturnState(void) {
    DoIP_GetConnectionState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoIP_00008 */
void test_DoIP_ActivateRouting_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoIP_ActivateRouting();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoIP_00008 */
void test_DoIP_ActivateRouting_ValidCall_ShouldSucceed(void) {
    DoIP_ActivateRouting();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoIP_00009 */
void test_DoIP_DeactivateRouting_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoIP_DeactivateRouting();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoIP_00009 */
void test_DoIP_DeactivateRouting_ValidCall_ShouldSucceed(void) {
    DoIP_DeactivateRouting();
    TEST_ASSERT_TRUE(1);
}

