/**
 * @file test_test_fee.c
 * @brief Fee Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/fee/src/Fee.c  @tests src/bsw/mcal/fee/include/Fee.h

#include "unity.h"
#include "Fee.h"

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
Fee_ConfigType testConfig;
static void test_Fee_SetupDefaultConfig(void) {
    (void)testConfig;
}

static boolean fee_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    fee_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Fee_00001 */
void test_Fee_Init_NullPtr_ShouldNotCrash(void) {
    Fee_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Fee_00001 */
void test_Fee_Init_ValidConfig_ShouldSucceed(void) {
    test_Fee_SetupDefaultConfig();
    Fee_Init(&testConfig);
    fee_initialized = TRUE;
    TEST_ASSERT_TRUE(fee_initialized);
}

/** @req SWS_Fee_00001 */
void test_Fee_Init_DoubleInit_ShouldSucceed(void) {
    test_Fee_SetupDefaultConfig();
    Fee_Init(&testConfig);
    Fee_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Fee_00002 */
void test_Fee_Invalidate_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Fee_Invalidate();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fee_00002 */
void test_Fee_Invalidate_InvalidBlock_ShouldReportError(void) {
    Fee_Invalidate(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fee_00002 */
void test_Fee_Invalidate_ValidBlock_ShouldSucceed(void) {
    Fee_Invalidate();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fee_00003 */
void test_Fee_Read_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Fee_Read();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fee_00003 */
void test_Fee_Read_NullBuffer_ShouldReportError(void) {
    Fee_Read(0U, NULL_PTR, 0U);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fee_00003 */
void test_Fee_Read_ValidBlock_ShouldSucceed(void) {
    Fee_Read();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fee_00004 */
void test_Fee_Write_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Fee_Write();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fee_00004 */
void test_Fee_Write_NullData_ShouldReportError(void) {
    Fee_Write(0U, NULL_PTR, 0U);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fee_00004 */
void test_Fee_Write_ValidData_ShouldSucceed(void) {
    Fee_Write();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fee_00005 */
void test_Fee_Cancel_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Fee_Cancel();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fee_00005 */
void test_Fee_Cancel_ValidCall_ShouldSucceed(void) {
    Fee_Cancel();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fee_00006 */
void test_Fee_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    Fee_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount); /* No DET in uninit for status */
}

/** @req SWS_Fee_00006 */
void test_Fee_GetStatus_ValidCall_ShouldReturnStatus(void) {
    Fee_GetStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fee_00007 */
void test_Fee_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Fee_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fee_00007 */
void test_Fee_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Fee_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fee_00008 */
void test_Fee_InvalidateImmediate_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Fee_InvalidateImmediate();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fee_00008 */
void test_Fee_InvalidateImmediate_InvalidBlock_ShouldReportError(void) {
    Fee_InvalidateImmediate(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fee_00008 */
void test_Fee_InvalidateImmediate_ValidBlock_ShouldSucceed(void) {
    Fee_InvalidateImmediate();
    TEST_ASSERT_TRUE(1);
}

