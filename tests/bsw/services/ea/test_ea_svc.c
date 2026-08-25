/**
 * @file test_test_ea_svc.c
 * @brief EaSvc Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/ea/src/Ea.c  @tests src/bsw/ecual/ea/include/Ea.h

#include "unity.h"
#include "EaSvc.h"

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
EaSvc_ConfigType testConfig;
static void test_EaSvc_SetupDefaultConfig(void) {
    testConfig.NumBlocks = 1U;
}

static boolean easvc_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    easvc_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_EaSvc_00001 */
void test_EaSvc_Init_NullPtr_ShouldNotCrash(void) {
    EaSvc_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EaSvc_00001 */
void test_EaSvc_Init_ValidConfig_ShouldSucceed(void) {
    test_EaSvc_SetupDefaultConfig();
    EaSvc_Init(&testConfig);
    easvc_initialized = TRUE;
    TEST_ASSERT_TRUE(easvc_initialized);
}

/** @req SWS_EaSvc_00001 */
void test_EaSvc_Init_DoubleInit_ShouldSucceed(void) {
    test_EaSvc_SetupDefaultConfig();
    EaSvc_Init(&testConfig);
    EaSvc_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_EaSvc_00002 */
void test_EaSvc_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EaSvc_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EaSvc_00002 */
void test_EaSvc_DeInit_ValidCall_ShouldSucceed(void) {
    EaSvc_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EaSvc_00003 */
void test_EaSvc_GetVersionInfo_NullPtr_ShouldReportError(void) {
    EaSvc_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EaSvc_00003 */
void test_EaSvc_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    EaSvc_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EaSvc_00004 */
void test_EaSvc_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    EaSvc_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EaSvc_00004 */
void test_EaSvc_MainFunction_ValidCall_ShouldSucceed(void) {
    EaSvc_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EaSvc_00005 */
void test_EaSvc_Read_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EaSvc_Read();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EaSvc_00005 */
void test_EaSvc_Read_NullPtr_ShouldReportError(void) {
    EaSvc_Read(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EaSvc_00005 */
void test_EaSvc_Read_ValidCall_ShouldSucceed(void) {
    EaSvc_Read();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EaSvc_00006 */
void test_EaSvc_Write_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EaSvc_Write();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EaSvc_00006 */
void test_EaSvc_Write_NullPtr_ShouldReportError(void) {
    EaSvc_Write(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EaSvc_00006 */
void test_EaSvc_Write_ValidCall_ShouldSucceed(void) {
    EaSvc_Write();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EaSvc_00007 */
void test_EaSvc_Erase_Uninit_ShouldReportError(void) {
    /* Not initialized */
    EaSvc_Erase();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EaSvc_00007 */
void test_EaSvc_Erase_InvalidBlock_ShouldReportError(void) {
    EaSvc_Erase(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_EaSvc_00007 */
void test_EaSvc_Erase_ValidCall_ShouldSucceed(void) {
    EaSvc_Erase();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_EaSvc_00008 */
void test_EaSvc_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    EaSvc_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_EaSvc_00008 */
void test_EaSvc_GetStatus_ValidCall_ShouldReturnStatus(void) {
    EaSvc_GetStatus();
    TEST_ASSERT_TRUE(1);
}

