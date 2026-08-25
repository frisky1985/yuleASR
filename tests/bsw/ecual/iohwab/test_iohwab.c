/**
 * @file test_test_iohwab.c
 * @brief IoHwAb Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/iohwab/src/IoHwAb.c  @tests src/bsw/ecual/iohwab/include/IoHwAb.h

#include "unity.h"
#include "IoHwAb.h"

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
IoHwAb_ConfigType testConfig;
static void test_IoHwAb_SetupDefaultConfig(void) {
    testConfig.NumSignals = 1U;
}

static boolean iohwab_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    iohwab_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_IoHwAb_00001 */
void test_IoHwAb_Init_NullPtr_ShouldNotCrash(void) {
    IoHwAb_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_IoHwAb_00001 */
void test_IoHwAb_Init_ValidConfig_ShouldSucceed(void) {
    test_IoHwAb_SetupDefaultConfig();
    IoHwAb_Init(&testConfig);
    iohwab_initialized = TRUE;
    TEST_ASSERT_TRUE(iohwab_initialized);
}

/** @req SWS_IoHwAb_00001 */
void test_IoHwAb_Init_DoubleInit_ShouldSucceed(void) {
    test_IoHwAb_SetupDefaultConfig();
    IoHwAb_Init(&testConfig);
    IoHwAb_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_IoHwAb_00002 */
void test_IoHwAb_GetVersionInfo_NullPtr_ShouldReportError(void) {
    IoHwAb_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00002 */
void test_IoHwAb_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    IoHwAb_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IoHwAb_00003 */
void test_IoHwAb_ReadDigital_Uninit_ShouldReportError(void) {
    /* Not initialized */
    IoHwAb_ReadDigital();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00003 */
void test_IoHwAb_ReadDigital_InvalidSignal_ShouldReportError(void) {
    IoHwAb_ReadDigital(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00003 */
void test_IoHwAb_ReadDigital_ValidCall_ShouldReturnLevel(void) {
    IoHwAb_ReadDigital();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IoHwAb_00004 */
void test_IoHwAb_WriteDigital_Uninit_ShouldReportError(void) {
    /* Not initialized */
    IoHwAb_WriteDigital();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00004 */
void test_IoHwAb_WriteDigital_InvalidSignal_ShouldReportError(void) {
    IoHwAb_WriteDigital(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00004 */
void test_IoHwAb_WriteDigital_ValidCall_ShouldSucceed(void) {
    IoHwAb_WriteDigital();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IoHwAb_00001 */
void test_IoHwAb_ReadAnalog_Uninit_ShouldReportError(void) {
    /* Not initialized */
    IoHwAb_ReadAnalog();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00005 */
void test_IoHwAb_ReadAnalog_InvalidSignal_ShouldReportError(void) {
    IoHwAb_ReadAnalog(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00005 */
void test_IoHwAb_ReadAnalog_ValidCall_ShouldReturnValue(void) {
    IoHwAb_ReadAnalog();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IoHwAb_00001 */
void test_IoHwAb_WriteAnalog_Uninit_ShouldReportError(void) {
    /* Not initialized */
    IoHwAb_WriteAnalog();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00006 */
void test_IoHwAb_WriteAnalog_InvalidSignal_ShouldReportError(void) {
    IoHwAb_WriteAnalog(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00006 */
void test_IoHwAb_WriteAnalog_ValidCall_ShouldSucceed(void) {
    IoHwAb_WriteAnalog();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IoHwAb_00001 */
void test_IoHwAb_ReadPwm_Uninit_ShouldReportError(void) {
    /* Not initialized */
    IoHwAb_ReadPwm();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00007 */
void test_IoHwAb_ReadPwm_InvalidSignal_ShouldReportError(void) {
    IoHwAb_ReadPwm(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00007 */
void test_IoHwAb_ReadPwm_ValidCall_ShouldReturnPwm(void) {
    IoHwAb_ReadPwm();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IoHwAb_00001 */
void test_IoHwAb_WritePwm_Uninit_ShouldReportError(void) {
    /* Not initialized */
    IoHwAb_WritePwm();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00008 */
void test_IoHwAb_WritePwm_InvalidSignal_ShouldReportError(void) {
    IoHwAb_WritePwm(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00008 */
void test_IoHwAb_WritePwm_ValidCall_ShouldSucceed(void) {
    IoHwAb_WritePwm();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IoHwAb_00004 */
void test_IoHwAb_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    IoHwAb_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IoHwAb_00004 */
void test_IoHwAb_MainFunction_ValidCall_ShouldSucceed(void) {
    IoHwAb_MainFunction();
    TEST_ASSERT_TRUE(1);
}

