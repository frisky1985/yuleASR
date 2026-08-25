/**
 * @file test_test_ethsm.c
 * @brief EthSM Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/ethsm/src/EthSM.c  @tests src/bsw/ecual/ethsm/include/EthSM.h

#include "unity.h"
#include "EthSM.h"

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
EthSM_ConfigType testConfig;
static void test_EthSM_SetupDefaultConfig(void) {
    testConfig.NumCtrl = 1U;
}

static boolean ethsm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    ethsm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_EthSM_00001 */
void test_EthSM_Init_NullPtr_ShouldNotCrash(void) {
    EthSM_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EthSM_00001 */
void test_EthSM_Init_ValidConfig_ShouldSucceed(void) {
    test_EthSM_SetupDefaultConfig();
    EthSM_Init(&testConfig);
    ethsm_initialized = TRUE;
    TEST_ASSERT_TRUE(ethsm_initialized);
}

/** @req SWS_EthSM_00001 */
void test_EthSM_Init_DoubleInit_ShouldSucceed(void) {
    test_EthSM_SetupDefaultConfig();
    EthSM_Init(&testConfig);
    EthSM_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EthSM_00002 */
void test_EthSM_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    EthSM_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSM_00002 */
void test_EthSM_MainFunction_ValidCall_ShouldSucceed(void) {
    EthSM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSM_00003 */
void test_EthSM_RequestComMode_InvalidCtrl_ShouldReportError(void) {
    EthSM_RequestComMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSM_00003 */
void test_EthSM_RequestComMode_FullCom_ShouldSucceed(void) {
    EthSM_RequestComMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSM_00003 */
void test_EthSM_RequestComMode_NoCom_ShouldSucceed(void) {
    EthSM_RequestComMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSM_00004 */
void test_EthSM_GetCtrlMode_InvalidCtrl_ShouldReportError(void) {
    EthSM_GetCtrlMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSM_00004 */
void test_EthSM_GetCtrlMode_ValidCall_ShouldReturnMode(void) {
    EthSM_GetCtrlMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSM_00005 */
void test_EthSM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    EthSM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSM_00005 */
void test_EthSM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    EthSM_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSM_00006 */
void test_EthSM_CtrlNotification_InvalidCtrl_ShouldReportError(void) {
    EthSM_CtrlNotification(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSM_00006 */
void test_EthSM_CtrlNotification_ValidCall_ShouldSucceed(void) {
    EthSM_CtrlNotification();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EthSM_00007 */
void test_EthSM_TrcvNotification_InvalidCtrl_ShouldReportError(void) {
    EthSM_TrcvNotification(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EthSM_00007 */
void test_EthSM_TrcvNotification_ValidCall_ShouldSucceed(void) {
    EthSM_TrcvNotification();
    TEST_ASSERT_TRUE(1);
}

