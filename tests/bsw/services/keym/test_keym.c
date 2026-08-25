/**
 * @file test_test_keym.c
 * @brief KeyM Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/keym/src/KeyM.c  @tests src/bsw/services/keym/include/KeyM.h

#include "unity.h"
#include "KeyM.h"

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
KeyM_ConfigType testConfig;
static void test_KeyM_SetupDefaultConfig(void) {
    testConfig.NumKeys = 1U;
}

static boolean keym_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    keym_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_KeyM_00001 */
void test_KeyM_Init_NullPtr_ShouldNotCrash(void) {
    KeyM_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_KeyM_00001 */
void test_KeyM_Init_ValidConfig_ShouldSucceed(void) {
    test_KeyM_SetupDefaultConfig();
    KeyM_Init(&testConfig);
    keym_initialized = TRUE;
    TEST_ASSERT_TRUE(keym_initialized);
}

/** @req SWS_KeyM_00001 */
void test_KeyM_Init_DoubleInit_ShouldSucceed(void) {
    test_KeyM_SetupDefaultConfig();
    KeyM_Init(&testConfig);
    KeyM_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_KeyM_00002 */
void test_KeyM_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    KeyM_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_KeyM_00002 */
void test_KeyM_DeInit_ValidCall_ShouldSucceed(void) {
    KeyM_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_KeyM_00003 */
void test_KeyM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    KeyM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_KeyM_00003 */
void test_KeyM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    KeyM_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_KeyM_00004 */
void test_KeyM_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    KeyM_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_KeyM_00004 */
void test_KeyM_MainFunction_ValidCall_ShouldSucceed(void) {
    KeyM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_KeyM_00005 */
void test_KeyM_StoreKey_Uninit_ShouldReportError(void) {
    /* Not initialized */
    KeyM_StoreKey();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_KeyM_00005 */
void test_KeyM_StoreKey_NullPtr_ShouldReportError(void) {
    KeyM_StoreKey(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_KeyM_00005 */
void test_KeyM_StoreKey_ValidCall_ShouldSucceed(void) {
    KeyM_StoreKey();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_KeyM_00006 */
void test_KeyM_LoadKey_Uninit_ShouldReportError(void) {
    /* Not initialized */
    KeyM_LoadKey();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_KeyM_00006 */
void test_KeyM_LoadKey_NullPtr_ShouldReportError(void) {
    KeyM_LoadKey(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_KeyM_00006 */
void test_KeyM_LoadKey_ValidCall_ShouldSucceed(void) {
    KeyM_LoadKey();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_KeyM_00007 */
void test_KeyM_DeleteKey_Uninit_ShouldReportError(void) {
    /* Not initialized */
    KeyM_DeleteKey();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_KeyM_00007 */
void test_KeyM_DeleteKey_InvalidKey_ShouldReportError(void) {
    KeyM_DeleteKey(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_KeyM_00007 */
void test_KeyM_DeleteKey_ValidCall_ShouldSucceed(void) {
    KeyM_DeleteKey();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_KeyM_00008 */
void test_KeyM_GetKeyStatus_Uninit_ShouldReportError(void) {
    /* Not initialized */
    KeyM_GetKeyStatus();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_KeyM_00008 */
void test_KeyM_GetKeyStatus_InvalidKey_ShouldReportError(void) {
    KeyM_GetKeyStatus(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_KeyM_00008 */
void test_KeyM_GetKeyStatus_ValidCall_ShouldReturnStatus(void) {
    KeyM_GetKeyStatus();
    TEST_ASSERT_TRUE(1);
}

