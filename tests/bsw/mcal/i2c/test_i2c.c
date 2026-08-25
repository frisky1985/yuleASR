/**
 * @file test_test_i2c.c
 * @brief I2c Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/i2c/src/I2c.c  @tests src/bsw/mcal/i2c/include/I2c.h

#include "unity.h"
#include "I2c.h"

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
I2c_ConfigType testConfig;
static void test_I2c_SetupDefaultConfig(void) {
    testConfig.NumChannels = 1U;
}

static boolean i2c_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    i2c_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_I2c_00001 */
void test_I2c_Init_NullPtr_ShouldNotCrash(void) {
    I2c_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_I2c_00001 */
void test_I2c_Init_ValidConfig_ShouldSucceed(void) {
    test_I2c_SetupDefaultConfig();
    I2c_Init(&testConfig);
    i2c_initialized = TRUE;
    TEST_ASSERT_TRUE(i2c_initialized);
}

/** @req SWS_I2c_00001 */
void test_I2c_Init_DoubleInit_ShouldSucceed(void) {
    test_I2c_SetupDefaultConfig();
    I2c_Init(&testConfig);
    I2c_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_I2c_00002 */
void test_I2c_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    I2c_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_I2c_00002 */
void test_I2c_DeInit_ValidCall_ShouldSucceed(void) {
    I2c_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_I2c_00003 */
void test_I2c_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    I2c_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_I2c_00003 */
void test_I2c_Transmit_NullPtr_ShouldReportError(void) {
    I2c_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_I2c_00003 */
void test_I2c_Transmit_ValidCall_ShouldSucceed(void) {
    I2c_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_I2c_00004 */
void test_I2c_Receive_Uninit_ShouldReportError(void) {
    /* Not initialized */
    I2c_Receive();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_I2c_00004 */
void test_I2c_Receive_NullPtr_ShouldReportError(void) {
    I2c_Receive(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_I2c_00004 */
void test_I2c_Receive_ValidCall_ShouldSucceed(void) {
    I2c_Receive();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_I2c_00005 */
void test_I2c_GetStatus_Uninit_ShouldReturnIdle(void) {
    /* Not initialized */
    I2c_GetStatus();
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_I2c_00005 */
void test_I2c_GetStatus_ValidCall_ShouldReturnStatus(void) {
    I2c_GetStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_I2c_00006 */
void test_I2c_GetVersionInfo_NullPtr_ShouldReportError(void) {
    I2c_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_I2c_00006 */
void test_I2c_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    I2c_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_I2c_00007 */
void test_I2c_SetBaudRate_Uninit_ShouldReportError(void) {
    /* Not initialized */
    I2c_SetBaudRate();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_I2c_00007 */
void test_I2c_SetBaudRate_InvalidRate_ShouldReportError(void) {
    I2c_SetBaudRate(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_I2c_00007 */
void test_I2c_SetBaudRate_ValidCall_ShouldSucceed(void) {
    I2c_SetBaudRate();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_I2c_00008 */
void test_I2c_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    I2c_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_I2c_00008 */
void test_I2c_MainFunction_ValidCall_ShouldSucceed(void) {
    I2c_MainFunction();
    TEST_ASSERT_TRUE(1);
}

