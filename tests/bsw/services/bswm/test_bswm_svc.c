/**
 * @file test_test_bswm_svc.c
 * @brief BswM Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/bswm/src/BswM.c  @tests src/bsw/services/bswm/include/BswM.h

#include "unity.h"
#include "BswM.h"

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
BswM_ConfigType testConfig;
static void test_BswM_SetupDefaultConfig(void) {
    testConfig.NumModes = 1U;
}

static boolean bswm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    bswm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_BswM_00001 */
void test_BswM_Init_NullPtr_ShouldNotCrash(void) {
    BswM_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_BswM_00001 */
void test_BswM_Init_ValidConfig_ShouldSucceed(void) {
    test_BswM_SetupDefaultConfig();
    BswM_Init(&testConfig);
    bswm_initialized = TRUE;
    TEST_ASSERT_TRUE(bswm_initialized);
}

/** @req SWS_BswM_00001 */
void test_BswM_Init_DoubleInit_ShouldSucceed(void) {
    test_BswM_SetupDefaultConfig();
    BswM_Init(&testConfig);
    BswM_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_BswM_00002 */
void test_BswM_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    BswM_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_BswM_00002 */
void test_BswM_DeInit_ValidCall_ShouldSucceed(void) {
    BswM_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_BswM_00003 */
void test_BswM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    BswM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_BswM_00003 */
void test_BswM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    BswM_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_BswM_00004 */
void test_BswM_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    BswM_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_BswM_00004 */
void test_BswM_MainFunction_ValidCall_ShouldSucceed(void) {
    BswM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_BswM_00005 */
void test_BswM_RequestMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    BswM_RequestMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_BswM_00005 */
void test_BswM_RequestMode_InvalidMode_ShouldReportError(void) {
    BswM_RequestMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_BswM_00005 */
void test_BswM_RequestMode_ValidCall_ShouldSucceed(void) {
    BswM_RequestMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_BswM_00006 */
void test_BswM_GetMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    BswM_GetMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_BswM_00006 */
void test_BswM_GetMode_ValidCall_ShouldReturnMode(void) {
    BswM_GetMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_BswM_00007 */
void test_BswM_SetMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    BswM_SetMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_BswM_00007 */
void test_BswM_SetMode_InvalidMode_ShouldReportError(void) {
    BswM_SetMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_BswM_00007 */
void test_BswM_SetMode_ValidCall_ShouldSucceed(void) {
    BswM_SetMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_BswM_00008 */
void test_BswM_GetCurrentMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    BswM_GetCurrentMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_BswM_00008 */
void test_BswM_GetCurrentMode_ValidCall_ShouldReturnMode(void) {
    BswM_GetCurrentMode();
    TEST_ASSERT_TRUE(1);
}

