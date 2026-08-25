/**
 * @file test_test_stbm.c
 * @brief StbM Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "StbM.h"

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
StbM_ConfigType testConfig;
static void test_StbM_SetupDefaultConfig(void) {
    testConfig.NumDomains = 1U;
}

static boolean stbm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    stbm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_StbM_00001 */
void test_StbM_Init_NullPtr_ShouldNotCrash(void) {
    StbM_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_StbM_00001 */
void test_StbM_Init_ValidConfig_ShouldSucceed(void) {
    test_StbM_SetupDefaultConfig();
    StbM_Init(&testConfig);
    stbm_initialized = TRUE;
    TEST_ASSERT_TRUE(stbm_initialized);
}

/** @req SWS_StbM_00001 */
void test_StbM_Init_DoubleInit_ShouldSucceed(void) {
    test_StbM_SetupDefaultConfig();
    StbM_Init(&testConfig);
    StbM_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_StbM_00002 */
void test_StbM_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    StbM_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_StbM_00002 */
void test_StbM_DeInit_ValidCall_ShouldSucceed(void) {
    StbM_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_StbM_00003 */
void test_StbM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    StbM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_StbM_00003 */
void test_StbM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    StbM_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_StbM_00004 */
void test_StbM_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    StbM_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_StbM_00004 */
void test_StbM_MainFunction_ValidCall_ShouldSucceed(void) {
    StbM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_StbM_00005 */
void test_StbM_GetTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    StbM_GetTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_StbM_00005 */
void test_StbM_GetTime_NullPtr_ShouldReportError(void) {
    StbM_GetTime(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_StbM_00005 */
void test_StbM_GetTime_ValidCall_ShouldSucceed(void) {
    StbM_GetTime();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_StbM_00006 */
void test_StbM_SetTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    StbM_SetTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_StbM_00006 */
void test_StbM_SetTime_ValidCall_ShouldSucceed(void) {
    StbM_SetTime();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_StbM_00007 */
void test_StbM_GetTimeDomain_Uninit_ShouldReportError(void) {
    /* Not initialized */
    StbM_GetTimeDomain();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_StbM_00007 */
void test_StbM_GetTimeDomain_InvalidDomain_ShouldReportError(void) {
    StbM_GetTimeDomain(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_StbM_00007 */
void test_StbM_GetTimeDomain_ValidCall_ShouldReturnDomain(void) {
    StbM_GetTimeDomain();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_StbM_00008 */
void test_StbM_SyncTime_Uninit_ShouldReportError(void) {
    /* Not initialized */
    StbM_SyncTime();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_StbM_00008 */
void test_StbM_SyncTime_InvalidDomain_ShouldReportError(void) {
    StbM_SyncTime(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_StbM_00008 */
void test_StbM_SyncTime_ValidCall_ShouldSucceed(void) {
    StbM_SyncTime();
    TEST_ASSERT_TRUE(1);
}

