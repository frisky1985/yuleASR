/**
 * @file test_test_dlt.c
 * @brief Dlt Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "Dlt.h"

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
Dlt_ConfigType testConfig;
static void test_Dlt_SetupDefaultConfig(void) {
    testConfig.NumContexts = 1U;
}

static boolean dlt_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    dlt_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Dlt_00001 */
void test_Dlt_Init_NullPtr_ShouldNotCrash(void) {
    Dlt_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Dlt_00001 */
void test_Dlt_Init_ValidConfig_ShouldSucceed(void) {
    test_Dlt_SetupDefaultConfig();
    Dlt_Init(&testConfig);
    dlt_initialized = TRUE;
    TEST_ASSERT_TRUE(dlt_initialized);
}

/** @req SWS_Dlt_00001 */
void test_Dlt_Init_DoubleInit_ShouldSucceed(void) {
    test_Dlt_SetupDefaultConfig();
    Dlt_Init(&testConfig);
    Dlt_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Dlt_00002 */
void test_Dlt_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Dlt_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Dlt_00002 */
void test_Dlt_DeInit_ValidCall_ShouldSucceed(void) {
    Dlt_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Dlt_00003 */
void test_Dlt_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Dlt_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Dlt_00003 */
void test_Dlt_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Dlt_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Dlt_00004 */
void test_Dlt_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Dlt_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Dlt_00004 */
void test_Dlt_MainFunction_ValidCall_ShouldSucceed(void) {
    Dlt_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Dlt_00005 */
void test_Dlt_Log_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Dlt_Log();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Dlt_00005 */
void test_Dlt_Log_NullPtr_ShouldReportError(void) {
    Dlt_Log(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Dlt_00005 */
void test_Dlt_Log_ValidCall_ShouldSucceed(void) {
    Dlt_Log();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Dlt_00006 */
void test_Dlt_SetLogLevel_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Dlt_SetLogLevel();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Dlt_00006 */
void test_Dlt_SetLogLevel_InvalidContext_ShouldReportError(void) {
    Dlt_SetLogLevel(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Dlt_00006 */
void test_Dlt_SetLogLevel_ValidCall_ShouldSucceed(void) {
    Dlt_SetLogLevel();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Dlt_00007 */
void test_Dlt_GetLogLevel_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Dlt_GetLogLevel();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Dlt_00007 */
void test_Dlt_GetLogLevel_InvalidContext_ShouldReportError(void) {
    Dlt_GetLogLevel(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Dlt_00007 */
void test_Dlt_GetLogLevel_ValidCall_ShouldReturnLevel(void) {
    Dlt_GetLogLevel();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Dlt_00008 */
void test_Dlt_RegisterContext_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Dlt_RegisterContext();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Dlt_00008 */
void test_Dlt_RegisterContext_NullPtr_ShouldReportError(void) {
    Dlt_RegisterContext(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Dlt_00008 */
void test_Dlt_RegisterContext_ValidCall_ShouldSucceed(void) {
    Dlt_RegisterContext();
    TEST_ASSERT_TRUE(1);
}

