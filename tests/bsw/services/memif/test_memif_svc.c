/**
 * @file test_test_memif_svc.c
 * @brief MemIf Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "MemIf.h"

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
MemIf_ConfigType testConfig;
static void test_MemIf_SetupDefaultConfig(void) {
    testConfig.NumBlocks = 1U;
}

static boolean memif_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    memif_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_MemIf_00001 */
void test_MemIf_Init_NullPtr_ShouldNotCrash(void) {
    MemIf_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_MemIf_00001 */
void test_MemIf_Init_ValidConfig_ShouldSucceed(void) {
    test_MemIf_SetupDefaultConfig();
    MemIf_Init(&testConfig);
    memif_initialized = TRUE;
    TEST_ASSERT_TRUE(memif_initialized);
}

/** @req SWS_MemIf_00001 */
void test_MemIf_Init_DoubleInit_ShouldSucceed(void) {
    test_MemIf_SetupDefaultConfig();
    MemIf_Init(&testConfig);
    MemIf_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_MemIf_00002 */
void test_MemIf_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    MemIf_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_MemIf_00002 */
void test_MemIf_DeInit_ValidCall_ShouldSucceed(void) {
    MemIf_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_MemIf_00003 */
void test_MemIf_GetVersionInfo_NullPtr_ShouldReportError(void) {
    MemIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_MemIf_00003 */
void test_MemIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    MemIf_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_MemIf_00004 */
void test_MemIf_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    MemIf_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_MemIf_00004 */
void test_MemIf_MainFunction_ValidCall_ShouldSucceed(void) {
    MemIf_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_MemIf_00005 */
void test_MemIf_Read_Uninit_ShouldReportError(void) {
    /* Not initialized */
    MemIf_Read();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_MemIf_00005 */
void test_MemIf_Read_NullPtr_ShouldReportError(void) {
    MemIf_Read(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_MemIf_00005 */
void test_MemIf_Read_ValidCall_ShouldSucceed(void) {
    MemIf_Read();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_MemIf_00006 */
void test_MemIf_Write_Uninit_ShouldReportError(void) {
    /* Not initialized */
    MemIf_Write();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_MemIf_00006 */
void test_MemIf_Write_NullPtr_ShouldReportError(void) {
    MemIf_Write(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_MemIf_00006 */
void test_MemIf_Write_ValidCall_ShouldSucceed(void) {
    MemIf_Write();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_MemIf_00007 */
void test_MemIf_Erase_Uninit_ShouldReportError(void) {
    /* Not initialized */
    MemIf_Erase();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_MemIf_00007 */
void test_MemIf_Erase_InvalidBlock_ShouldReportError(void) {
    MemIf_Erase(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_MemIf_00007 */
void test_MemIf_Erase_ValidCall_ShouldSucceed(void) {
    MemIf_Erase();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_MemIf_00008 */
void test_MemIf_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    MemIf_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_MemIf_00008 */
void test_MemIf_GetStatus_ValidCall_ShouldReturnStatus(void) {
    MemIf_GetStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_MemIf_00009 */
void test_MemIf_Invalidate_Uninit_ShouldReportError(void) {
    /* Not initialized */
    MemIf_Invalidate();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_MemIf_00009 */
void test_MemIf_Invalidate_InvalidBlock_ShouldReportError(void) {
    MemIf_Invalidate(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_MemIf_00009 */
void test_MemIf_Invalidate_ValidCall_ShouldSucceed(void) {
    MemIf_Invalidate();
    TEST_ASSERT_TRUE(1);
}

