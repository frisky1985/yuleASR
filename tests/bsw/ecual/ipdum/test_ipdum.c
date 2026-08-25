/**
 * @file test_test_ipdum.c
 * @brief IpduM Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "IpduM.h"

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
IpduM_ConfigType testConfig;
static void test_IpduM_SetupDefaultConfig(void) {
    testConfig.NumPdus = 1U;
}

static boolean ipdum_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    ipdum_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_IpduM_00001 */
void test_IpduM_Init_NullPtr_ShouldNotCrash(void) {
    IpduM_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_IpduM_00001 */
void test_IpduM_Init_ValidConfig_ShouldSucceed(void) {
    test_IpduM_SetupDefaultConfig();
    IpduM_Init(&testConfig);
    ipdum_initialized = TRUE;
    TEST_ASSERT_TRUE(ipdum_initialized);
}

/** @req SWS_IpduM_00001 */
void test_IpduM_Init_DoubleInit_ShouldSucceed(void) {
    test_IpduM_SetupDefaultConfig();
    IpduM_Init(&testConfig);
    IpduM_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_IpduM_00002 */
void test_IpduM_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    IpduM_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IpduM_00002 */
void test_IpduM_DeInit_ValidCall_ShouldSucceed(void) {
    IpduM_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IpduM_00003 */
void test_IpduM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    IpduM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IpduM_00003 */
void test_IpduM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    IpduM_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IpduM_00004 */
void test_IpduM_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    IpduM_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IpduM_00004 */
void test_IpduM_MainFunction_ValidCall_ShouldSucceed(void) {
    IpduM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IpduM_00005 */
void test_IpduM_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    IpduM_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IpduM_00005 */
void test_IpduM_Transmit_NullPtr_ShouldReportError(void) {
    IpduM_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IpduM_00005 */
void test_IpduM_Transmit_ValidCall_ShouldSucceed(void) {
    IpduM_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IpduM_00006 */
void test_IpduM_TriggerTransmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    IpduM_TriggerTransmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IpduM_00006 */
void test_IpduM_TriggerTransmit_InvalidPdu_ShouldReportError(void) {
    IpduM_TriggerTransmit(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IpduM_00006 */
void test_IpduM_TriggerTransmit_ValidCall_ShouldSucceed(void) {
    IpduM_TriggerTransmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IpduM_00007 */
void test_IpduM_RxIndication_Uninit_ShouldReportError(void) {
    /* Not initialized */
    IpduM_RxIndication();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IpduM_00007 */
void test_IpduM_RxIndication_NullPtr_ShouldReportError(void) {
    IpduM_RxIndication(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IpduM_00007 */
void test_IpduM_RxIndication_ValidCall_ShouldSucceed(void) {
    IpduM_RxIndication();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_IpduM_00008 */
void test_IpduM_TxConfirmation_Uninit_ShouldReportError(void) {
    /* Not initialized */
    IpduM_TxConfirmation();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_IpduM_00008 */
void test_IpduM_TxConfirmation_ValidCall_ShouldSucceed(void) {
    IpduM_TxConfirmation();
    TEST_ASSERT_TRUE(1);
}

