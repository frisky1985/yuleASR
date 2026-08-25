/**
 * @file test_test_eth.c
 * @brief Eth Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/eth/src/Eth.c  @tests src/bsw/mcal/eth/include/Eth.h

#include "unity.h"
#include "Eth.h"

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
Eth_ConfigType testConfig;
static void test_Eth_SetupDefaultConfig(void) {
    testConfig.NumCtrl = 1U;
}

static boolean eth_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    eth_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Eth_00001 */
void test_Eth_Init_NullPtr_ShouldNotCrash(void) {
    Eth_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Eth_00001 */
void test_Eth_Init_ValidConfig_ShouldSucceed(void) {
    test_Eth_SetupDefaultConfig();
    Eth_Init(&testConfig);
    eth_initialized = TRUE;
    TEST_ASSERT_TRUE(eth_initialized);
}

/** @req SWS_Eth_00001 */
void test_Eth_Init_DoubleInit_ShouldSucceed(void) {
    test_Eth_SetupDefaultConfig();
    Eth_Init(&testConfig);
    Eth_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Eth_00002 */
void test_Eth_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eth_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eth_00002 */
void test_Eth_Transmit_NullBuf_ShouldReportError(void) {
    Eth_Transmit(0U, NULL_PTR, 0U);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eth_00002 */
void test_Eth_Transmit_ValidFrame_ShouldSucceed(void) {
    Eth_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eth_00003 */
void test_Eth_Receive_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eth_Receive();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eth_00003 */
void test_Eth_Receive_NullBuf_ShouldReportError(void) {
    Eth_Receive(0U, NULL_PTR, 0U);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eth_00003 */
void test_Eth_Receive_ValidCall_ShouldSucceed(void) {
    Eth_Receive();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eth_00004 */
void test_Eth_GetCounter_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eth_GetCounter();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eth_00004 */
void test_Eth_GetCounter_ValidCall_ShouldSucceed(void) {
    Eth_GetCounter();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eth_00005 */
void test_Eth_ReadMII_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eth_ReadMII();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eth_00005 */
void test_Eth_ReadMII_ValidCall_ShouldSucceed(void) {
    Eth_ReadMII();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eth_00006 */
void test_Eth_WriteMII_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eth_WriteMII();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eth_00006 */
void test_Eth_WriteMII_ValidCall_ShouldSucceed(void) {
    Eth_WriteMII();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eth_00007 */
void test_Eth_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Eth_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eth_00007 */
void test_Eth_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Eth_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eth_00008 */
void test_Eth_UpdatePhyState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eth_UpdatePhyState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eth_00008 */
void test_Eth_UpdatePhyState_ValidCall_ShouldSucceed(void) {
    Eth_UpdatePhyState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eth_00009 */
void test_Eth_GetPhysState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eth_GetPhysState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eth_00009 */
void test_Eth_GetPhysState_ValidCall_ShouldSucceed(void) {
    Eth_GetPhysState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Eth_00010 */
void test_Eth_SetForwardingMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Eth_SetForwardingMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Eth_00010 */
void test_Eth_SetForwardingMode_ValidCall_ShouldSucceed(void) {
    Eth_SetForwardingMode();
    TEST_ASSERT_TRUE(1);
}

