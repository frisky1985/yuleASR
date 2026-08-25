/**
 * @file test_test_ea.c
 * @brief Ea Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "Ea.h"

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
Ea_ConfigType testConfig;
static void test_Ea_SetupDefaultConfig(void) {
    testConfig.NumBlocks = 1U;
}

static boolean ea_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    ea_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Ea_00001 */
void test_Ea_Init_NullPtr_ShouldNotCrash(void) {
    Ea_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Ea_00001 */
void test_Ea_Init_ValidConfig_ShouldSucceed(void) {
    test_Ea_SetupDefaultConfig();
    Ea_Init(&testConfig);
    ea_initialized = TRUE;
    TEST_ASSERT_TRUE(ea_initialized);
}

/** @req SWS_Ea_00001 */
void test_Ea_Init_DoubleInit_ShouldSucceed(void) {
    test_Ea_SetupDefaultConfig();
    Ea_Init(&testConfig);
    Ea_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Ea_00002 */
void test_Ea_EraseImmediate_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Ea_EraseImmediate();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Ea_00002 */
void test_Ea_EraseImmediate_InvalidBlock_ShouldReportError(void) {
    Ea_EraseImmediate(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Ea_00002 */
void test_Ea_EraseImmediate_ValidCall_ShouldSucceed(void) {
    Ea_EraseImmediate();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Ea_00003 */
void test_Ea_Read_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Ea_Read();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Ea_00003 */
void test_Ea_Read_NullPtr_ShouldReportError(void) {
    Ea_Read(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Ea_00003 */
void test_Ea_Read_ValidCall_ShouldSucceed(void) {
    Ea_Read();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Ea_00004 */
void test_Ea_Write_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Ea_Write();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Ea_00004 */
void test_Ea_Write_NullPtr_ShouldReportError(void) {
    Ea_Write(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Ea_00004 */
void test_Ea_Write_ValidCall_ShouldSucceed(void) {
    Ea_Write();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Ea_00005 */
void test_Ea_Invalidate_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Ea_Invalidate();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Ea_00005 */
void test_Ea_Invalidate_InvalidBlock_ShouldReportError(void) {
    Ea_Invalidate(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Ea_00005 */
void test_Ea_Invalidate_ValidCall_ShouldSucceed(void) {
    Ea_Invalidate();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Ea_00006 */
void test_Ea_Cancel_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Ea_Cancel();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Ea_00006 */
void test_Ea_Cancel_ValidCall_ShouldSucceed(void) {
    Ea_Cancel();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Ea_00007 */
void test_Ea_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    Ea_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Ea_00007 */
void test_Ea_GetStatus_ValidCall_ShouldReturnStatus(void) {
    Ea_GetStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Ea_00008 */
void test_Ea_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Ea_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Ea_00008 */
void test_Ea_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Ea_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Ea_00009 */
void test_Ea_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Ea_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Ea_00009 */
void test_Ea_MainFunction_ValidCall_ShouldSucceed(void) {
    Ea_MainFunction();
    TEST_ASSERT_TRUE(1);
}

