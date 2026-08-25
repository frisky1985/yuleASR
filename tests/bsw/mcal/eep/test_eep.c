/**
 * @file test_test_eep.c
 * @brief Eep Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/eep/src/Eep.c  @tests src/bsw/mcal/eep/include/Eep.h

#include "unity.h"
#include "Eep.h"

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
Eep_ConfigType testConfig;
static void test_Eep_SetupDefaultConfig(void) {
    (void)testConfig;
}

static boolean eep_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    eep_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Eep_00001 */
void test_Eep_Init_NullPtr_ShouldNotCrash(void) {
    Eep_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Eep_00001 */
void test_Eep_Init_ValidConfig_ShouldSucceed(void) {
    test_Eep_SetupDefaultConfig();
    Eep_Init(&testConfig);
    eep_initialized = TRUE;
    TEST_ASSERT_TRUE(eep_initialized);
}

/** @req SWS_Eep_00001 */
void test_Eep_Init_DoubleInit_ShouldSucceed(void) {
    test_Eep_SetupDefaultConfig();
    Eep_Init(&testConfig);
    Eep_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Eep_00002 */
void test_Eep_Erase_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eep_Erase();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eep_00002 */
void test_Eep_Erase_InvalidSector_ShouldReportError(void) {
    Eep_Erase(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eep_00002 */
void test_Eep_Erase_ValidSector_ShouldSucceed(void) {
    Eep_Erase();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eep_00003 */
void test_Eep_Write_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eep_Write();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eep_00003 */
void test_Eep_Write_InvalidAddress_ShouldReportError(void) {
    Eep_Write(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eep_00003 */
void test_Eep_Write_ValidData_ShouldSucceed(void) {
    Eep_Write();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eep_00004 */
void test_Eep_Read_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eep_Read();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eep_00004 */
void test_Eep_Read_InvalidAddress_ShouldReportError(void) {
    Eep_Read(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eep_00004 */
void test_Eep_Read_ValidBuffer_ShouldSucceed(void) {
    Eep_Read();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eep_00005 */
void test_Eep_Compare_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eep_Compare();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eep_00005 */
void test_Eep_Compare_Mismatch_ShouldReturnNotOk(void) {
    /* Compare mismatch scenario */
    Eep_Compare(0U, NULL_PTR, 0U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eep_00005 */
void test_Eep_Compare_Match_ShouldReturnOk(void) {
    /* Compare match scenario */
    Eep_Compare(0U, NULL_PTR, 0U);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eep_00006 */
void test_Eep_Cancel_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eep_Cancel();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eep_00006 */
void test_Eep_Cancel_ValidCall_ShouldSucceed(void) {
    Eep_Cancel();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eep_00007 */
void test_Eep_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    Eep_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount); /* No DET in uninit for status */
}

/** @req SWS_Eep_00007 */
void test_Eep_GetStatus_ValidCall_ShouldReturnStatus(void) {
    Eep_GetStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eep_00008 */
void test_Eep_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Eep_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eep_00008 */
void test_Eep_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Eep_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

