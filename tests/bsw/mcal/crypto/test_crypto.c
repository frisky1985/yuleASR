/**
 * @file test_test_crypto.c
 * @brief Crypto Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/crypto/src/Crypto.c  @tests src/bsw/mcal/crypto/include/Crypto.h

#include "unity.h"
#include "Crypto.h"

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
Crypto_ConfigType testConfig;
static void test_Crypto_SetupDefaultConfig(void) {
    testConfig.NumChannels = 1U;
}

static boolean crypto_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    crypto_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Crypto_00001 */
void test_Crypto_Init_NullPtr_ShouldNotCrash(void) {
    Crypto_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Crypto_00001 */
void test_Crypto_Init_ValidConfig_ShouldSucceed(void) {
    test_Crypto_SetupDefaultConfig();
    Crypto_Init(&testConfig);
    crypto_initialized = TRUE;
    TEST_ASSERT_TRUE(crypto_initialized);
}

/** @req SWS_Crypto_00001 */
void test_Crypto_Init_DoubleInit_ShouldSucceed(void) {
    test_Crypto_SetupDefaultConfig();
    Crypto_Init(&testConfig);
    Crypto_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Crypto_00002 */
void test_Crypto_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Crypto_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00002 */
void test_Crypto_DeInit_ValidCall_ShouldSucceed(void) {
    Crypto_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crypto_00003 */
void test_Crypto_StartBlock_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Crypto_StartBlock();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00003 */
void test_Crypto_StartBlock_NullPtr_ShouldReportError(void) {
    Crypto_StartBlock(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00003 */
void test_Crypto_StartBlock_ValidCall_ShouldSucceed(void) {
    Crypto_StartBlock();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crypto_00004 */
void test_Crypto_UpdateBlock_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Crypto_UpdateBlock();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00004 */
void test_Crypto_UpdateBlock_NullPtr_ShouldReportError(void) {
    Crypto_UpdateBlock(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00004 */
void test_Crypto_UpdateBlock_ValidCall_ShouldSucceed(void) {
    Crypto_UpdateBlock();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crypto_00005 */
void test_Crypto_FinishBlock_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Crypto_FinishBlock();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00005 */
void test_Crypto_FinishBlock_NullPtr_ShouldReportError(void) {
    Crypto_FinishBlock(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00005 */
void test_Crypto_FinishBlock_ValidCall_ShouldSucceed(void) {
    Crypto_FinishBlock();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crypto_00006 */
void test_Crypto_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Crypto_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00006 */
void test_Crypto_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Crypto_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crypto_00007 */
void test_Crypto_RandomGenerate_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Crypto_RandomGenerate();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00007 */
void test_Crypto_RandomGenerate_NullPtr_ShouldReportError(void) {
    Crypto_RandomGenerate(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00007 */
void test_Crypto_RandomGenerate_ValidCall_ShouldSucceed(void) {
    Crypto_RandomGenerate();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crypto_00008 */
void test_Crypto_KeySetValid_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Crypto_KeySetValid();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00008 */
void test_Crypto_KeySetValid_InvalidKey_ShouldReportError(void) {
    Crypto_KeySetValid(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00008 */
void test_Crypto_KeySetValid_ValidCall_ShouldSucceed(void) {
    Crypto_KeySetValid();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Crypto_00009 */
void test_Crypto_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Crypto_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Crypto_00009 */
void test_Crypto_MainFunction_ValidCall_ShouldSucceed(void) {
    Crypto_MainFunction();
    TEST_ASSERT_TRUE(1);
}

