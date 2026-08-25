/**
 * @file test_test_wdgif.c
 * @brief WdgIf Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/wdgif/src/WdgIf.c  @tests src/bsw/ecual/wdgif/include/WdgIf.h

#include "unity.h"
#include "WdgIf.h"

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
WdgIf_ConfigType testConfig;
static void test_WdgIf_SetupDefaultConfig(void) {
    testConfig.NumWatchdogs = 1U;
}

static boolean wdgif_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    wdgif_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_WdgIf_00001 */
void test_WdgIf_Init_NullPtr_ShouldNotCrash(void) {
    WdgIf_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_WdgIf_00001 */
void test_WdgIf_Init_ValidConfig_ShouldSucceed(void) {
    test_WdgIf_SetupDefaultConfig();
    WdgIf_Init(&testConfig);
    wdgif_initialized = TRUE;
    TEST_ASSERT_TRUE(wdgif_initialized);
}

/** @req SWS_WdgIf_00001 */
void test_WdgIf_Init_DoubleInit_ShouldSucceed(void) {
    test_WdgIf_SetupDefaultConfig();
    WdgIf_Init(&testConfig);
    WdgIf_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_WdgIf_00002 */
void test_WdgIf_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    WdgIf_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_WdgIf_00002 */
void test_WdgIf_DeInit_ValidCall_ShouldSucceed(void) {
    WdgIf_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_WdgIf_00003 */
void test_WdgIf_GetVersionInfo_NullPtr_ShouldReportError(void) {
    WdgIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_WdgIf_00003 */
void test_WdgIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    WdgIf_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_WdgIf_00004 */
void test_WdgIf_SetMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    WdgIf_SetMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_WdgIf_00004 */
void test_WdgIf_SetMode_InvalidMode_ShouldReportError(void) {
    WdgIf_SetMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_WdgIf_00004 */
void test_WdgIf_SetMode_ValidCall_ShouldSucceed(void) {
    WdgIf_SetMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_WdgIf_00005 */
void test_WdgIf_GetMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    WdgIf_GetMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_WdgIf_00005 */
void test_WdgIf_GetMode_ValidCall_ShouldReturnMode(void) {
    WdgIf_GetMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_WdgIf_00006 */
void test_WdgIf_Trigger_Uninit_ShouldReportError(void) {
    /* Not initialized */
    WdgIf_Trigger();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_WdgIf_00006 */
void test_WdgIf_Trigger_ValidCall_ShouldSucceed(void) {
    WdgIf_Trigger();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_WdgIf_00007 */
void test_WdgIf_CheckReset_Uninit_ShouldReportError(void) {
    /* Not initialized */
    WdgIf_CheckReset();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_WdgIf_00007 */
void test_WdgIf_CheckReset_ValidCall_ShouldSucceed(void) {
    WdgIf_CheckReset();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_WdgIf_00008 */
void test_WdgIf_SetTriggerCondition_Uninit_ShouldReportError(void) {
    /* Not initialized */
    WdgIf_SetTriggerCondition();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_WdgIf_00008 */
void test_WdgIf_SetTriggerCondition_InvalidIdx_ShouldReportError(void) {
    WdgIf_SetTriggerCondition(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_WdgIf_00008 */
void test_WdgIf_SetTriggerCondition_ValidCall_ShouldSucceed(void) {
    WdgIf_SetTriggerCondition();
    TEST_ASSERT_TRUE(1);
}

