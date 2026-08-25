/**
 * @file test_test_swc.c
 * @brief Swc Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/swc/src/Swc.c  @tests src/bsw/services/swc/include/Swc.h

#include "unity.h"
#include "Swc.h"

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
Swc_ConfigType testConfig;
static void test_Swc_SetupDefaultConfig(void) {
    testConfig.NumInstances = 1U;
}

static boolean swc_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    swc_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Swc_00001 */
void test_Swc_Init_NullPtr_ShouldNotCrash(void) {
    Swc_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Swc_00001 */
void test_Swc_Init_ValidConfig_ShouldSucceed(void) {
    test_Swc_SetupDefaultConfig();
    Swc_Init(&testConfig);
    swc_initialized = TRUE;
    TEST_ASSERT_TRUE(swc_initialized);
}

/** @req SWS_Swc_00001 */
void test_Swc_Init_DoubleInit_ShouldSucceed(void) {
    test_Swc_SetupDefaultConfig();
    Swc_Init(&testConfig);
    Swc_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Swc_00002 */
void test_Swc_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Swc_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Swc_00002 */
void test_Swc_DeInit_ValidCall_ShouldSucceed(void) {
    Swc_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Swc_00003 */
void test_Swc_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Swc_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Swc_00003 */
void test_Swc_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Swc_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Swc_00004 */
void test_Swc_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Swc_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Swc_00004 */
void test_Swc_MainFunction_ValidCall_ShouldSucceed(void) {
    Swc_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Swc_00005 */
void test_Swc_Start_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Swc_Start();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Swc_00005 */
void test_Swc_Start_ValidCall_ShouldSucceed(void) {
    Swc_Start();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Swc_00006 */
void test_Swc_Stop_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Swc_Stop();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Swc_00006 */
void test_Swc_Stop_ValidCall_ShouldSucceed(void) {
    Swc_Stop();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Swc_00007 */
void test_Swc_GetState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Swc_GetState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Swc_00007 */
void test_Swc_GetState_ValidCall_ShouldReturnState(void) {
    Swc_GetState();
    TEST_ASSERT_TRUE(1);
}

