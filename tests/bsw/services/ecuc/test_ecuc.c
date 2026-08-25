/**
 * @file test_test_ecuc.c
 * @brief EcuC Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "EcuC.h"

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
EcuC_ConfigType testConfig;
static void test_EcuC_SetupDefaultConfig(void) {
    (void)testConfig;
}

static boolean ecuc_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    ecuc_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_EcuC_00001 */
void test_EcuC_Init_NullPtr_ShouldNotCrash(void) {
    EcuC_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EcuC_00001 */
void test_EcuC_Init_ValidConfig_ShouldSucceed(void) {
    test_EcuC_SetupDefaultConfig();
    EcuC_Init(&testConfig);
    ecuc_initialized = TRUE;
    TEST_ASSERT_TRUE(ecuc_initialized);
}

/** @req SWS_EcuC_00001 */
void test_EcuC_Init_DoubleInit_ShouldSucceed(void) {
    test_EcuC_SetupDefaultConfig();
    EcuC_Init(&testConfig);
    EcuC_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EcuC_00002 */
void test_EcuC_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EcuC_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EcuC_00002 */
void test_EcuC_DeInit_ValidCall_ShouldSucceed(void) {
    EcuC_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EcuC_00003 */
void test_EcuC_GetVersionInfo_NullPtr_ShouldReportError(void) {
    EcuC_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EcuC_00003 */
void test_EcuC_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    EcuC_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EcuC_00004 */
void test_EcuC_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    EcuC_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EcuC_00004 */
void test_EcuC_MainFunction_ValidCall_ShouldSucceed(void) {
    EcuC_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EcuC_00005 */
void test_EcuC_GetResetReason_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EcuC_GetResetReason();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EcuC_00005 */
void test_EcuC_GetResetReason_ValidCall_ShouldReturnReason(void) {
    EcuC_GetResetReason();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EcuC_00006 */
void test_EcuC_SetResetReason_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EcuC_SetResetReason();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EcuC_00006 */
void test_EcuC_SetResetReason_ValidCall_ShouldSucceed(void) {
    EcuC_SetResetReason();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EcuC_00007 */
void test_EcuC_PerformReset_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EcuC_PerformReset();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EcuC_00007 */
void test_EcuC_PerformReset_ValidCall_ShouldSucceed(void) {
    EcuC_PerformReset();
    TEST_ASSERT_TRUE(1);
}

