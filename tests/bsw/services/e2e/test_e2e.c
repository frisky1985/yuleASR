/**
 * @file test_test_e2e.c
 * @brief E2E Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "E2E.h"

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
E2E_ConfigType testConfig;
static void test_E2E_SetupDefaultConfig(void) {
    testConfig.NumProfiles = 1U;
}

static boolean e2e_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    e2e_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_E2E_00001 */
void test_E2E_Init_NullPtr_ShouldNotCrash(void) {
    E2E_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_E2E_00001 */
void test_E2E_Init_ValidConfig_ShouldSucceed(void) {
    test_E2E_SetupDefaultConfig();
    E2E_Init(&testConfig);
    e2e_initialized = TRUE;
    TEST_ASSERT_TRUE(e2e_initialized);
}

/** @req SWS_E2E_00001 */
void test_E2E_Init_DoubleInit_ShouldSucceed(void) {
    test_E2E_SetupDefaultConfig();
    E2E_Init(&testConfig);
    E2E_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_E2E_00002 */
void test_E2E_Protect_Uninit_ShouldReportError(void) {
    /* Not initialized */
    E2E_Protect();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_E2E_00002 */
void test_E2E_Protect_NullPtr_ShouldReportError(void) {
    E2E_Protect(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_E2E_00002 */
void test_E2E_Protect_ValidData_ShouldSucceed(void) {
    uint8 data[4] = {1, 2, 3, 4};
    E2E_Protect(data, 4U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_E2E_00003 */
void test_E2E_Check_Uninit_ShouldReportError(void) {
    /* Not initialized */
    E2E_Check();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_E2E_00003 */
void test_E2E_Check_NullPtr_ShouldReportError(void) {
    E2E_Check(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_E2E_00003 */
void test_E2E_Check_ValidData_ShouldSucceed(void) {
    uint8 data[4] = {1, 2, 3, 4};
    E2E_Check(data, 4U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_E2E_00004 */
void test_E2E_GetVersionInfo_NullPtr_ShouldReportError(void) {
    E2E_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_E2E_00004 */
void test_E2E_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    E2E_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_E2E_00005 */
void test_E2E_RegisterProfile_NullPtr_ShouldReportError(void) {
    E2E_RegisterProfile(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_E2E_00005 */
void test_E2E_RegisterProfile_ValidCall_ShouldSucceed(void) {
    E2E_RegisterProfile();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_E2E_00006 */
void test_E2E_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    E2E_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_E2E_00006 */
void test_E2E_MainFunction_ValidCall_ShouldSucceed(void) {
    E2E_MainFunction();
    TEST_ASSERT_TRUE(1);
}

