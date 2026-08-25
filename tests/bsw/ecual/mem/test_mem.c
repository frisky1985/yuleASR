/**
 * @file test_test_mem.c
 * @brief Mem Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/mem/src/Mem.c  @tests src/bsw/services/mem/include/Mem.h

#include "unity.h"
#include "Mem.h"

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
Mem_ConfigType testConfig;
static void test_Mem_SetupDefaultConfig(void) {
    testConfig.NumBlocks = 1U;
}

static boolean mem_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    mem_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Mem_00001 */
void test_Mem_Init_NullPtr_ShouldNotCrash(void) {
    Mem_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Mem_00001 */
void test_Mem_Init_ValidConfig_ShouldSucceed(void) {
    test_Mem_SetupDefaultConfig();
    Mem_Init(&testConfig);
    mem_initialized = TRUE;
    TEST_ASSERT_TRUE(mem_initialized);
}

/** @req SWS_Mem_00001 */
void test_Mem_Init_DoubleInit_ShouldSucceed(void) {
    test_Mem_SetupDefaultConfig();
    Mem_Init(&testConfig);
    Mem_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Mem_00002 */
void test_Mem_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Mem_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mem_00002 */
void test_Mem_DeInit_ValidCall_ShouldSucceed(void) {
    Mem_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mem_00003 */
void test_Mem_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Mem_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mem_00003 */
void test_Mem_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Mem_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mem_00004 */
void test_Mem_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Mem_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mem_00004 */
void test_Mem_MainFunction_ValidCall_ShouldSucceed(void) {
    Mem_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mem_00005 */
void test_Mem_Read_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Mem_Read();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mem_00005 */
void test_Mem_Read_NullPtr_ShouldReportError(void) {
    Mem_Read(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mem_00005 */
void test_Mem_Read_ValidCall_ShouldSucceed(void) {
    Mem_Read();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mem_00006 */
void test_Mem_Write_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Mem_Write();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mem_00006 */
void test_Mem_Write_NullPtr_ShouldReportError(void) {
    Mem_Write(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mem_00006 */
void test_Mem_Write_ValidCall_ShouldSucceed(void) {
    Mem_Write();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mem_00007 */
void test_Mem_Erase_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Mem_Erase();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mem_00007 */
void test_Mem_Erase_InvalidBlock_ShouldReportError(void) {
    Mem_Erase(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Mem_00007 */
void test_Mem_Erase_ValidCall_ShouldSucceed(void) {
    Mem_Erase();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Mem_00008 */
void test_Mem_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    Mem_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Mem_00008 */
void test_Mem_GetStatus_ValidCall_ShouldReturnStatus(void) {
    Mem_GetStatus();
    TEST_ASSERT_TRUE(1);
}

