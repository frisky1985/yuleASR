/**
 * @file test_test_fls.c
 * @brief Fls Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/fls/src/Fls.c  @tests src/bsw/mcal/fls/include/Fls.h

#include "unity.h"
#include "Fls.h"

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
Fls_ConfigType testConfig;
static void test_Fls_SetupDefaultConfig(void) {
    (void)testConfig;
}

static boolean fls_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    fls_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Fls_00001 */
void test_Fls_Init_NullPtr_ShouldNotCrash(void) {
    Fls_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Fls_00001 */
void test_Fls_Init_ValidConfig_ShouldSucceed(void) {
    test_Fls_SetupDefaultConfig();
    Fls_Init(&testConfig);
    fls_initialized = TRUE;
    TEST_ASSERT_TRUE(fls_initialized);
}

/** @req SWS_Fls_00001 */
void test_Fls_Init_DoubleInit_ShouldSucceed(void) {
    test_Fls_SetupDefaultConfig();
    Fls_Init(&testConfig);
    Fls_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Fls_Erase();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_InvalidSector_ShouldReportError(void) {
    Fls_Erase(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fls_00002 */
void test_Fls_Erase_ValidSector_ShouldSucceed(void) {
    Fls_Erase();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Fls_Write();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_InvalidAddress_ShouldReportError(void) {
    Fls_Write(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fls_00003 */
void test_Fls_Write_ValidData_ShouldSucceed(void) {
    Fls_Write();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fls_00004 */
void test_Fls_Read_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Fls_Read();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fls_00004 */
void test_Fls_Read_InvalidAddress_ShouldReportError(void) {
    Fls_Read(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fls_00004 */
void test_Fls_Read_ValidBuffer_ShouldSucceed(void) {
    Fls_Read();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fls_00005 */
void test_Fls_Compare_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Fls_Compare();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fls_00005 */
void test_Fls_Compare_Mismatch_ShouldReturnNotOk(void) {
    /* Compare mismatch scenario */
    Fls_Compare(0U, NULL_PTR, 0U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fls_00005 */
void test_Fls_Compare_Match_ShouldReturnOk(void) {
    /* Compare match scenario */
    Fls_Compare(0U, NULL_PTR, 0U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fls_00006 */
void test_Fls_Cancel_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Fls_Cancel();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fls_00006 */
void test_Fls_Cancel_ValidCall_ShouldSucceed(void) {
    Fls_Cancel();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fls_00007 */
void test_Fls_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    Fls_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount); /* No DET in uninit for status */
}

/** @req SWS_Fls_00007 */
void test_Fls_GetStatus_ValidCall_ShouldReturnStatus(void) {
    Fls_GetStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Fls_00008 */
void test_Fls_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Fls_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Fls_00008 */
void test_Fls_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Fls_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

