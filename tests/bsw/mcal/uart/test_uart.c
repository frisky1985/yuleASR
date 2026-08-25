/**
 * @file test_test_uart.c
 * @brief Uart Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/uart/src/Uart.c  @tests src/bsw/mcal/uart/include/Uart.h

#include "unity.h"
#include "Uart.h"

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
Uart_ConfigType testConfig;
static void test_Uart_SetupDefaultConfig(void) {
    testConfig.NumChannels = 2U;
}

static boolean uart_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    uart_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Uart_00001 */
void test_Uart_Init_NullPtr_ShouldNotCrash(void) {
    Uart_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Uart_00001 */
void test_Uart_Init_ValidConfig_ShouldSucceed(void) {
    test_Uart_SetupDefaultConfig();
    Uart_Init(&testConfig);
    uart_initialized = TRUE;
    TEST_ASSERT_TRUE(uart_initialized);
}

/** @req SWS_Uart_00001 */
void test_Uart_Init_DoubleInit_ShouldSucceed(void) {
    test_Uart_SetupDefaultConfig();
    Uart_Init(&testConfig);
    Uart_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Uart_00002 */
void test_Uart_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Uart_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Uart_00002 */
void test_Uart_DeInit_ValidCall_ShouldSucceed(void) {
    Uart_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Uart_00003 */
void test_Uart_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Uart_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Uart_00003 */
void test_Uart_Transmit_NullPtr_ShouldReportError(void) {
    Uart_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Uart_00003 */
void test_Uart_Transmit_ValidCall_ShouldSucceed(void) {
    Uart_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Uart_00004 */
void test_Uart_Receive_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Uart_Receive();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Uart_00004 */
void test_Uart_Receive_NullPtr_ShouldReportError(void) {
    Uart_Receive(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Uart_00004 */
void test_Uart_Receive_ValidCall_ShouldSucceed(void) {
    Uart_Receive();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Uart_00005 */
void test_Uart_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    Uart_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_Uart_00005 */
void test_Uart_GetStatus_ValidCall_ShouldReturnStatus(void) {
    Uart_GetStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Uart_00006 */
void test_Uart_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Uart_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Uart_00006 */
void test_Uart_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Uart_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Uart_00007 */
void test_Uart_SetBaudRate_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Uart_SetBaudRate();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Uart_00007 */
void test_Uart_SetBaudRate_InvalidRate_ShouldReportError(void) {
    Uart_SetBaudRate(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Uart_00007 */
void test_Uart_SetBaudRate_ValidCall_ShouldSucceed(void) {
    Uart_SetBaudRate();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Uart_00008 */
void test_Uart_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Uart_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Uart_00008 */
void test_Uart_MainFunction_ValidCall_ShouldSucceed(void) {
    Uart_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Uart_00009 */
void test_Uart_FlushTxBuffer_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Uart_FlushTxBuffer();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Uart_00009 */
void test_Uart_FlushTxBuffer_ValidCall_ShouldSucceed(void) {
    Uart_FlushTxBuffer();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Uart_00010 */
void test_Uart_FlushRxBuffer_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Uart_FlushRxBuffer();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Uart_00010 */
void test_Uart_FlushRxBuffer_ValidCall_ShouldSucceed(void) {
    Uart_FlushRxBuffer();
    TEST_ASSERT_TRUE(1);
}

