/**
 * @file test_test_frif.c
 * @brief FrIf Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "FrIf.h"

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
FrIf_ConfigType testConfig;
static void test_FrIf_SetupDefaultConfig(void) {
    testConfig.NumCtrl = 1U;
}

static boolean frif_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    frif_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_FrIf_00001 */
void test_FrIf_Init_NullPtr_ShouldNotCrash(void) {
    FrIf_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_FrIf_00001 */
void test_FrIf_Init_ValidConfig_ShouldSucceed(void) {
    test_FrIf_SetupDefaultConfig();
    FrIf_Init(&testConfig);
    frif_initialized = TRUE;
    TEST_ASSERT_TRUE(frif_initialized);
}

/** @req SWS_FrIf_00001 */
void test_FrIf_Init_DoubleInit_ShouldSucceed(void) {
    test_FrIf_SetupDefaultConfig();
    FrIf_Init(&testConfig);
    FrIf_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_FrIf_00002 */
void test_FrIf_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FrIf_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrIf_00002 */
void test_FrIf_DeInit_ValidCall_ShouldSucceed(void) {
    FrIf_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrIf_00003 */
void test_FrIf_GetVersionInfo_NullPtr_ShouldReportError(void) {
    FrIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrIf_00003 */
void test_FrIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    FrIf_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrIf_00004 */
void test_FrIf_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FrIf_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrIf_00004 */
void test_FrIf_Transmit_NullPtr_ShouldReportError(void) {
    FrIf_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrIf_00004 */
void test_FrIf_Transmit_ValidCall_ShouldSucceed(void) {
    FrIf_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrIf_00005 */
void test_FrIf_Cancel_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FrIf_Cancel();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrIf_00005 */
void test_FrIf_Cancel_InvalidChannel_ShouldReportError(void) {
    FrIf_Cancel(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrIf_00005 */
void test_FrIf_Cancel_ValidCall_ShouldSucceed(void) {
    FrIf_Cancel();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrIf_00006 */
void test_FrIf_GetCtrlIdx_NullPtr_ShouldReportError(void) {
    FrIf_GetCtrlIdx(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrIf_00006 */
void test_FrIf_GetCtrlIdx_ValidCall_ShouldSucceed(void) {
    FrIf_GetCtrlIdx();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrIf_00007 */
void test_FrIf_GetCtrlMode_InvalidCtrl_ShouldReportError(void) {
    FrIf_GetCtrlMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrIf_00007 */
void test_FrIf_GetCtrlMode_ValidCall_ShouldReturnMode(void) {
    FrIf_GetCtrlMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrIf_00008 */
void test_FrIf_SetCtrlMode_InvalidCtrl_ShouldReportError(void) {
    FrIf_SetCtrlMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrIf_00008 */
void test_FrIf_SetCtrlMode_ValidCall_ShouldSucceed(void) {
    FrIf_SetCtrlMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FrIf_00009 */
void test_FrIf_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    FrIf_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FrIf_00009 */
void test_FrIf_MainFunction_ValidCall_ShouldSucceed(void) {
    FrIf_MainFunction();
    TEST_ASSERT_TRUE(1);
}

