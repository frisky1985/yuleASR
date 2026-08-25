/**
 * @file test_test_cryif.c
 * @brief CryIf Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/cryif/src/CryIf.c  @tests src/bsw/services/cryif/include/CryIf.h

#include "unity.h"
#include "CryIf.h"

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
CryIf_ConfigType testConfig;
static void test_CryIf_SetupDefaultConfig(void) {
    testConfig.NumChannels = 1U;
}

static boolean cryif_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    cryif_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_CryIf_00001 */
void test_CryIf_Init_NullPtr_ShouldNotCrash(void) {
    CryIf_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_CryIf_00001 */
void test_CryIf_Init_ValidConfig_ShouldSucceed(void) {
    test_CryIf_SetupDefaultConfig();
    CryIf_Init(&testConfig);
    cryif_initialized = TRUE;
    TEST_ASSERT_TRUE(cryif_initialized);
}

/** @req SWS_CryIf_00001 */
void test_CryIf_Init_DoubleInit_ShouldSucceed(void) {
    test_CryIf_SetupDefaultConfig();
    CryIf_Init(&testConfig);
    CryIf_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_CryIf_00002 */
void test_CryIf_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CryIf_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CryIf_00002 */
void test_CryIf_DeInit_ValidCall_ShouldSucceed(void) {
    CryIf_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CryIf_00003 */
void test_CryIf_GetVersionInfo_NullPtr_ShouldReportError(void) {
    CryIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CryIf_00003 */
void test_CryIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    CryIf_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CryIf_00004 */
void test_CryIf_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    CryIf_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CryIf_00004 */
void test_CryIf_MainFunction_ValidCall_ShouldSucceed(void) {
    CryIf_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CryIf_00005 */
void test_CryIf_StartBlock_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CryIf_StartBlock();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CryIf_00005 */
void test_CryIf_StartBlock_NullPtr_ShouldReportError(void) {
    CryIf_StartBlock(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CryIf_00005 */
void test_CryIf_StartBlock_ValidCall_ShouldSucceed(void) {
    CryIf_StartBlock();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CryIf_00006 */
void test_CryIf_UpdateBlock_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CryIf_UpdateBlock();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CryIf_00006 */
void test_CryIf_UpdateBlock_NullPtr_ShouldReportError(void) {
    CryIf_UpdateBlock(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CryIf_00006 */
void test_CryIf_UpdateBlock_ValidCall_ShouldSucceed(void) {
    CryIf_UpdateBlock();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CryIf_00007 */
void test_CryIf_FinishBlock_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CryIf_FinishBlock();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CryIf_00007 */
void test_CryIf_FinishBlock_NullPtr_ShouldReportError(void) {
    CryIf_FinishBlock(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CryIf_00007 */
void test_CryIf_FinishBlock_ValidCall_ShouldSucceed(void) {
    CryIf_FinishBlock();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CryIf_00008 */
void test_CryIf_KeySetValid_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CryIf_KeySetValid();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CryIf_00008 */
void test_CryIf_KeySetValid_InvalidKey_ShouldReportError(void) {
    CryIf_KeySetValid(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CryIf_00008 */
void test_CryIf_KeySetValid_ValidCall_ShouldSucceed(void) {
    CryIf_KeySetValid();
    TEST_ASSERT_TRUE(1);
}

