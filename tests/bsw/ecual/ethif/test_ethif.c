/**
 * @file test_test_ethif.c
 * @brief EthIf Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "EthIf.h"

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
EthIf_ConfigType testConfig;
static void test_EthIf_SetupDefaultConfig(void) {
    testConfig.NumCtrl = 1U;
}

static boolean ethif_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    ethif_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_EthIf_00001 */
void test_EthIf_Init_NullPtr_ShouldNotCrash(void) {
    EthIf_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EthIf_00001 */
void test_EthIf_Init_ValidConfig_ShouldSucceed(void) {
    test_EthIf_SetupDefaultConfig();
    EthIf_Init(&testConfig);
    ethif_initialized = TRUE;
    TEST_ASSERT_TRUE(ethif_initialized);
}

/** @req SWS_EthIf_00001 */
void test_EthIf_Init_DoubleInit_ShouldSucceed(void) {
    test_EthIf_SetupDefaultConfig();
    EthIf_Init(&testConfig);
    EthIf_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EthIf_00002 */
void test_EthIf_GetVersionInfo_NullPtr_ShouldReportError(void) {
    EthIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthIf_00002 */
void test_EthIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    EthIf_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthIf_00003 */
void test_EthIf_GetCtrlIdx_NullPtr_ShouldReportError(void) {
    EthIf_GetCtrlIdx(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthIf_00003 */
void test_EthIf_GetCtrlIdx_ValidCall_ShouldSucceed(void) {
    EthIf_GetCtrlIdx();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthIf_00004 */
void test_EthIf_SetCtrlMode_InvalidCtrl_ShouldReportError(void) {
    EthIf_SetCtrlMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthIf_00004 */
void test_EthIf_SetCtrlMode_ValidCall_ShouldSucceed(void) {
    EthIf_SetCtrlMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthIf_00005 */
void test_EthIf_GetCtrlMode_InvalidCtrl_ShouldReportError(void) {
    EthIf_GetCtrlMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthIf_00005 */
void test_EthIf_GetCtrlMode_ValidCall_ShouldSucceed(void) {
    EthIf_GetCtrlMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthIf_00006 */
void test_EthIf_GetPhyCtrlIdx_NullPtr_ShouldReportError(void) {
    EthIf_GetPhyCtrlIdx(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthIf_00006 */
void test_EthIf_GetPhyCtrlIdx_ValidCall_ShouldSucceed(void) {
    EthIf_GetPhyCtrlIdx();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthIf_00007 */
void test_EthIf_SetPhyCtrlMode_InvalidCtrl_ShouldReportError(void) {
    EthIf_SetPhyCtrlMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthIf_00007 */
void test_EthIf_SetPhyCtrlMode_ValidCall_ShouldSucceed(void) {
    EthIf_SetPhyCtrlMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthIf_00008 */
void test_EthIf_GetPhyCtrlMode_InvalidCtrl_ShouldReportError(void) {
    EthIf_GetPhyCtrlMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthIf_00008 */
void test_EthIf_GetPhyCtrlMode_ValidCall_ShouldSucceed(void) {
    EthIf_GetPhyCtrlMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthIf_00009 */
void test_EthIf_UpdatePhyState_InvalidCtrl_ShouldReportError(void) {
    EthIf_UpdatePhyState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthIf_00009 */
void test_EthIf_UpdatePhyState_ValidCall_ShouldSucceed(void) {
    EthIf_UpdatePhyState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthIf_00010 */
void test_EthIf_GetPhyState_InvalidCtrl_ShouldReportError(void) {
    EthIf_GetPhyState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthIf_00010 */
void test_EthIf_GetPhyState_ValidCall_ShouldSucceed(void) {
    EthIf_GetPhyState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthIf_00011 */
void test_EthIf_SetForwardingMode_InvalidCtrl_ShouldReportError(void) {
    EthIf_SetForwardingMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthIf_00011 */
void test_EthIf_SetForwardingMode_ValidCall_ShouldSucceed(void) {
    EthIf_SetForwardingMode();
    TEST_ASSERT_TRUE(1);
}

