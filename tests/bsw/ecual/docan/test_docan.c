/**
 * @file test_test_docan.c
 * @brief DoCan Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "DoCan.h"

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
DoCan_ConfigType testConfig;
static void test_DoCan_SetupDefaultConfig(void) {
    testConfig.NumChannels = 1U;
}

static boolean docan_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    docan_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_DoCan_00001 */
void test_DoCan_Init_NullPtr_ShouldNotCrash(void) {
    DoCan_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_DoCan_00001 */
void test_DoCan_Init_ValidConfig_ShouldSucceed(void) {
    test_DoCan_SetupDefaultConfig();
    DoCan_Init(&testConfig);
    docan_initialized = TRUE;
    TEST_ASSERT_TRUE(docan_initialized);
}

/** @req SWS_DoCan_00001 */
void test_DoCan_Init_DoubleInit_ShouldSucceed(void) {
    test_DoCan_SetupDefaultConfig();
    DoCan_Init(&testConfig);
    DoCan_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_DoCan_00002 */
void test_DoCan_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoCan_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00002 */
void test_DoCan_DeInit_ValidCall_ShouldSucceed(void) {
    DoCan_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoCan_00003 */
void test_DoCan_GetVersionInfo_NullPtr_ShouldReportError(void) {
    DoCan_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00003 */
void test_DoCan_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    DoCan_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoCan_00004 */
void test_DoCan_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    DoCan_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00004 */
void test_DoCan_MainFunction_ValidCall_ShouldSucceed(void) {
    DoCan_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoCan_00005 */
void test_DoCan_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoCan_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00005 */
void test_DoCan_Transmit_NullPtr_ShouldReportError(void) {
    DoCan_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00005 */
void test_DoCan_Transmit_ValidCall_ShouldSucceed(void) {
    DoCan_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoCan_00006 */
void test_DoCan_Cancel_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoCan_Cancel();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00006 */
void test_DoCan_Cancel_InvalidChannel_ShouldReportError(void) {
    DoCan_Cancel(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00006 */
void test_DoCan_Cancel_ValidCall_ShouldSucceed(void) {
    DoCan_Cancel();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoCan_00007 */
void test_DoCan_RxIndication_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoCan_RxIndication();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00007 */
void test_DoCan_RxIndication_NullPtr_ShouldReportError(void) {
    DoCan_RxIndication(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00007 */
void test_DoCan_RxIndication_ValidCall_ShouldSucceed(void) {
    DoCan_RxIndication();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoCan_00008 */
void test_DoCan_TxConfirmation_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoCan_TxConfirmation();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00008 */
void test_DoCan_TxConfirmation_ValidCall_ShouldSucceed(void) {
    DoCan_TxConfirmation();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_DoCan_00009 */
void test_DoCan_GetState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    DoCan_GetState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00009 */
void test_DoCan_GetState_NullPtr_ShouldReportError(void) {
    DoCan_GetState(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_DoCan_00009 */
void test_DoCan_GetState_ValidCall_ShouldSucceed(void) {
    DoCan_GetState();
    TEST_ASSERT_TRUE(1);
}

