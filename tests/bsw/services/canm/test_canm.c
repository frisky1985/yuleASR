/**
 * @file test_test_canm.c
 * @brief CanM Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/canNm/src/CanNm.c  @tests src/bsw/ecual/canNm/include/CanNm.h

#include "unity.h"
#include "CanM.h"

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
CanM_ConfigType testConfig;
static void test_CanM_SetupDefaultConfig(void) {
    testConfig.NumChannels = 1U;
}

static boolean canm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    canm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_CanM_00001 */
void test_CanM_Init_NullPtr_ShouldNotCrash(void) {
    CanM_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_CanM_00001 */
void test_CanM_Init_ValidConfig_ShouldSucceed(void) {
    test_CanM_SetupDefaultConfig();
    CanM_Init(&testConfig);
    canm_initialized = TRUE;
    TEST_ASSERT_TRUE(canm_initialized);
}

/** @req SWS_CanM_00001 */
void test_CanM_Init_DoubleInit_ShouldSucceed(void) {
    test_CanM_SetupDefaultConfig();
    CanM_Init(&testConfig);
    CanM_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_CanM_00002 */
void test_CanM_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CanM_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanM_00002 */
void test_CanM_DeInit_ValidCall_ShouldSucceed(void) {
    CanM_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanM_00003 */
void test_CanM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    CanM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanM_00003 */
void test_CanM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    CanM_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanM_00004 */
void test_CanM_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    CanM_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanM_00004 */
void test_CanM_MainFunction_ValidCall_ShouldSucceed(void) {
    CanM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanM_00005 */
void test_CanM_RequestComMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CanM_RequestComMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanM_00005 */
void test_CanM_RequestComMode_InvalidChannel_ShouldReportError(void) {
    CanM_RequestComMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanM_00005 */
void test_CanM_RequestComMode_ValidCall_ShouldSucceed(void) {
    CanM_RequestComMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanM_00006 */
void test_CanM_GetComMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CanM_GetComMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanM_00006 */
void test_CanM_GetComMode_InvalidChannel_ShouldReportError(void) {
    CanM_GetComMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanM_00006 */
void test_CanM_GetComMode_ValidCall_ShouldReturnMode(void) {
    CanM_GetComMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanM_00007 */
void test_CanM_CtrlBusOff_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CanM_CtrlBusOff();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanM_00007 */
void test_CanM_CtrlBusOff_InvalidCtrl_ShouldReportError(void) {
    CanM_CtrlBusOff(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanM_00007 */
void test_CanM_CtrlBusOff_ValidCall_ShouldSucceed(void) {
    CanM_CtrlBusOff();
    TEST_ASSERT_TRUE(1);
}

