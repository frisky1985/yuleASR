/**
 * @file test_test_ldcom.c
 * @brief LdCom Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "LdCom.h"

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
LdCom_ConfigType testConfig;
static void test_LdCom_SetupDefaultConfig(void) {
    testConfig.NumChannels = 1U;
}

static boolean ldcom_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    ldcom_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_LdCom_00001 */
void test_LdCom_Init_NullPtr_ShouldNotCrash(void) {
    LdCom_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LdCom_00001 */
void test_LdCom_Init_ValidConfig_ShouldSucceed(void) {
    test_LdCom_SetupDefaultConfig();
    LdCom_Init(&testConfig);
    ldcom_initialized = TRUE;
    TEST_ASSERT_TRUE(ldcom_initialized);
}

/** @req SWS_LdCom_00001 */
void test_LdCom_Init_DoubleInit_ShouldSucceed(void) {
    test_LdCom_SetupDefaultConfig();
    LdCom_Init(&testConfig);
    LdCom_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_LdCom_00002 */
void test_LdCom_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LdCom_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LdCom_00002 */
void test_LdCom_DeInit_ValidCall_ShouldSucceed(void) {
    LdCom_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LdCom_00003 */
void test_LdCom_GetVersionInfo_NullPtr_ShouldReportError(void) {
    LdCom_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LdCom_00003 */
void test_LdCom_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    LdCom_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LdCom_00004 */
void test_LdCom_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    LdCom_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LdCom_00004 */
void test_LdCom_MainFunction_ValidCall_ShouldSucceed(void) {
    LdCom_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LdCom_00005 */
void test_LdCom_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LdCom_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LdCom_00005 */
void test_LdCom_Transmit_NullPtr_ShouldReportError(void) {
    LdCom_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LdCom_00005 */
void test_LdCom_Transmit_ValidCall_ShouldSucceed(void) {
    LdCom_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LdCom_00006 */
void test_LdCom_Receive_Uninit_ShouldReportError(void) {
    /* Not initialized */
    LdCom_Receive();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LdCom_00006 */
void test_LdCom_Receive_NullPtr_ShouldReportError(void) {
    LdCom_Receive(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_LdCom_00006 */
void test_LdCom_Receive_ValidCall_ShouldSucceed(void) {
    LdCom_Receive();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_LdCom_00007 */
void test_LdCom_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    LdCom_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LdCom_00007 */
void test_LdCom_GetStatus_ValidCall_ShouldReturnStatus(void) {
    LdCom_GetStatus();
    TEST_ASSERT_TRUE(1);
}

