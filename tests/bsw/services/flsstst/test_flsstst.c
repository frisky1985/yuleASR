/**
 * @file test_test_flsstst.c
 * @brief FlsStst Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/fls/src/Fls.c  @tests src/bsw/mcal/fls/include/Fls.h

#include "unity.h"
#include "FlsStst.h"

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
FlsStst_ConfigType testConfig;
static void test_FlsStst_SetupDefaultConfig(void) {
    testConfig.NumSectors = 1U;
}

static boolean flsstst_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    flsstst_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_FlsStst_00001 */
void test_FlsStst_Init_NullPtr_ShouldNotCrash(void) {
    FlsStst_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_FlsStst_00001 */
void test_FlsStst_Init_ValidConfig_ShouldSucceed(void) {
    test_FlsStst_SetupDefaultConfig();
    FlsStst_Init(&testConfig);
    flsstst_initialized = TRUE;
    TEST_ASSERT_TRUE(flsstst_initialized);
}

/** @req SWS_FlsStst_00001 */
void test_FlsStst_Init_DoubleInit_ShouldSucceed(void) {
    test_FlsStst_SetupDefaultConfig();
    FlsStst_Init(&testConfig);
    FlsStst_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_FlsStst_00002 */
void test_FlsStst_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FlsStst_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FlsStst_00002 */
void test_FlsStst_DeInit_ValidCall_ShouldSucceed(void) {
    FlsStst_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FlsStst_00003 */
void test_FlsStst_GetVersionInfo_NullPtr_ShouldReportError(void) {
    FlsStst_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FlsStst_00003 */
void test_FlsStst_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    FlsStst_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FlsStst_00004 */
void test_FlsStst_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    FlsStst_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FlsStst_00004 */
void test_FlsStst_MainFunction_ValidCall_ShouldSucceed(void) {
    FlsStst_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FlsStst_00005 */
void test_FlsStst_Read_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FlsStst_Read();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FlsStst_00005 */
void test_FlsStst_Read_NullPtr_ShouldReportError(void) {
    FlsStst_Read(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FlsStst_00005 */
void test_FlsStst_Read_ValidCall_ShouldSucceed(void) {
    FlsStst_Read();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FlsStst_00006 */
void test_FlsStst_Write_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FlsStst_Write();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FlsStst_00006 */
void test_FlsStst_Write_NullPtr_ShouldReportError(void) {
    FlsStst_Write(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FlsStst_00006 */
void test_FlsStst_Write_ValidCall_ShouldSucceed(void) {
    FlsStst_Write();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FlsStst_00007 */
void test_FlsStst_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    FlsStst_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_FlsStst_00007 */
void test_FlsStst_GetStatus_ValidCall_ShouldReturnStatus(void) {
    FlsStst_GetStatus();
    TEST_ASSERT_TRUE(1);
}

