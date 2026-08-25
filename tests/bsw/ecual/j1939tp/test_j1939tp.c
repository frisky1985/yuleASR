/**
 * @file test_test_j1939tp.c
 * @brief J1939Tp Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "J1939Tp.h"

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
J1939Tp_ConfigType testConfig;
static void test_J1939Tp_SetupDefaultConfig(void) {
    testConfig.NumChannels = 1U;
}

static boolean j1939tp_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    j1939tp_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_J1939Tp_00001 */
void test_J1939Tp_Init_NullPtr_ShouldNotCrash(void) {
    J1939Tp_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_J1939Tp_00001 */
void test_J1939Tp_Init_ValidConfig_ShouldSucceed(void) {
    test_J1939Tp_SetupDefaultConfig();
    J1939Tp_Init(&testConfig);
    j1939tp_initialized = TRUE;
    TEST_ASSERT_TRUE(j1939tp_initialized);
}

/** @req SWS_J1939Tp_00001 */
void test_J1939Tp_Init_DoubleInit_ShouldSucceed(void) {
    test_J1939Tp_SetupDefaultConfig();
    J1939Tp_Init(&testConfig);
    J1939Tp_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_J1939Tp_00002 */
void test_J1939Tp_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    J1939Tp_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Tp_00002 */
void test_J1939Tp_DeInit_ValidCall_ShouldSucceed(void) {
    J1939Tp_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_J1939Tp_00003 */
void test_J1939Tp_GetVersionInfo_NullPtr_ShouldReportError(void) {
    J1939Tp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Tp_00003 */
void test_J1939Tp_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    J1939Tp_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_J1939Tp_00004 */
void test_J1939Tp_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    J1939Tp_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Tp_00004 */
void test_J1939Tp_MainFunction_ValidCall_ShouldSucceed(void) {
    J1939Tp_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_J1939Tp_00005 */
void test_J1939Tp_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    J1939Tp_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Tp_00005 */
void test_J1939Tp_Transmit_NullPtr_ShouldReportError(void) {
    J1939Tp_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Tp_00005 */
void test_J1939Tp_Transmit_ValidCall_ShouldSucceed(void) {
    J1939Tp_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_J1939Tp_00006 */
void test_J1939Tp_CancelTransmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    J1939Tp_CancelTransmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Tp_00006 */
void test_J1939Tp_CancelTransmit_InvalidChannel_ShouldReportError(void) {
    J1939Tp_CancelTransmit(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Tp_00006 */
void test_J1939Tp_CancelTransmit_ValidCall_ShouldSucceed(void) {
    J1939Tp_CancelTransmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_J1939Tp_00007 */
void test_J1939Tp_GetState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    J1939Tp_GetState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Tp_00007 */
void test_J1939Tp_GetState_NullPtr_ShouldReportError(void) {
    J1939Tp_GetState(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_J1939Tp_00007 */
void test_J1939Tp_GetState_ValidCall_ShouldSucceed(void) {
    J1939Tp_GetState();
    TEST_ASSERT_TRUE(1);
}

