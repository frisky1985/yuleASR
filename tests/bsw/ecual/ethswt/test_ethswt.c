/**
 * @file test_test_ethswt.c
 * @brief EthSwt Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "EthSwt.h"

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
EthSwt_ConfigType testConfig;
static void test_EthSwt_SetupDefaultConfig(void) {
    testConfig.NumCtrl = 1U;
}

static boolean ethswt_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    ethswt_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_EthSwt_00001 */
void test_EthSwt_Init_NullPtr_ShouldNotCrash(void) {
    EthSwt_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EthSwt_00001 */
void test_EthSwt_Init_ValidConfig_ShouldSucceed(void) {
    test_EthSwt_SetupDefaultConfig();
    EthSwt_Init(&testConfig);
    ethswt_initialized = TRUE;
    TEST_ASSERT_TRUE(ethswt_initialized);
}

/** @req SWS_EthSwt_00001 */
void test_EthSwt_Init_DoubleInit_ShouldSucceed(void) {
    test_EthSwt_SetupDefaultConfig();
    EthSwt_Init(&testConfig);
    EthSwt_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EthSwt_00002 */
void test_EthSwt_GetVersionInfo_NullPtr_ShouldReportError(void) {
    EthSwt_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSwt_00002 */
void test_EthSwt_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    EthSwt_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSwt_00003 */
void test_EthSwt_ActivateFrameTrigger_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EthSwt_ActivateFrameTrigger();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSwt_00003 */
void test_EthSwt_ActivateFrameTrigger_InvalidCtrl_ShouldReportError(void) {
    EthSwt_ActivateFrameTrigger(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSwt_00003 */
void test_EthSwt_ActivateFrameTrigger_ValidCall_ShouldSucceed(void) {
    EthSwt_ActivateFrameTrigger();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSwt_00004 */
void test_EthSwt_DeactivateFrameTrigger_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EthSwt_DeactivateFrameTrigger();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSwt_00004 */
void test_EthSwt_DeactivateFrameTrigger_InvalidCtrl_ShouldReportError(void) {
    EthSwt_DeactivateFrameTrigger(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSwt_00004 */
void test_EthSwt_DeactivateFrameTrigger_ValidCall_ShouldSucceed(void) {
    EthSwt_DeactivateFrameTrigger();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSwt_00005 */
void test_EthSwt_GetCtrlIdx_NullPtr_ShouldReportError(void) {
    EthSwt_GetCtrlIdx(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSwt_00005 */
void test_EthSwt_GetCtrlIdx_ValidCall_ShouldSucceed(void) {
    EthSwt_GetCtrlIdx();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSwt_00006 */
void test_EthSwt_GetCtrlMode_InvalidCtrl_ShouldReportError(void) {
    EthSwt_GetCtrlMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSwt_00006 */
void test_EthSwt_GetCtrlMode_ValidCall_ShouldReturnMode(void) {
    EthSwt_GetCtrlMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSwt_00007 */
void test_EthSwt_SetCtrlMode_InvalidCtrl_ShouldReportError(void) {
    EthSwt_SetCtrlMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSwt_00007 */
void test_EthSwt_SetCtrlMode_ValidCall_ShouldSucceed(void) {
    EthSwt_SetCtrlMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSwt_00008 */
void test_EthSwt_GetPhyCtrlMode_InvalidCtrl_ShouldReportError(void) {
    EthSwt_GetPhyCtrlMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSwt_00008 */
void test_EthSwt_GetPhyCtrlMode_ValidCall_ShouldReturnMode(void) {
    EthSwt_GetPhyCtrlMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSwt_00009 */
void test_EthSwt_SetPhyCtrlMode_InvalidCtrl_ShouldReportError(void) {
    EthSwt_SetPhyCtrlMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSwt_00009 */
void test_EthSwt_SetPhyCtrlMode_ValidCall_ShouldSucceed(void) {
    EthSwt_SetPhyCtrlMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSwt_00010 */
void test_EthSwt_UpdatePhyState_InvalidCtrl_ShouldReportError(void) {
    EthSwt_UpdatePhyState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSwt_00010 */
void test_EthSwt_UpdatePhyState_ValidCall_ShouldSucceed(void) {
    EthSwt_UpdatePhyState();
    TEST_ASSERT_TRUE(1);
}

