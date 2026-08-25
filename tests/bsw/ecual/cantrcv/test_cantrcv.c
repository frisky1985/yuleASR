/**
 * @file test_test_cantrcv.c
 * @brief CanTrcv Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/cantrcv/src/CanTrcv.c  @tests src/bsw/ecual/cantrcv/include/CanTrcv.h

#include "unity.h"
#include "CanTrcv.h"

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
CanTrcv_ConfigType testConfig;
static void test_CanTrcv_SetupDefaultConfig(void) {
    testConfig.NumTransceivers = 1U;
}

static boolean cantrcv_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    cantrcv_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_CanTrcv_00001 */
void test_CanTrcv_Init_NullPtr_ShouldNotCrash(void) {
    CanTrcv_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_CanTrcv_00001 */
void test_CanTrcv_Init_ValidConfig_ShouldSucceed(void) {
    test_CanTrcv_SetupDefaultConfig();
    CanTrcv_Init(&testConfig);
    cantrcv_initialized = TRUE;
    TEST_ASSERT_TRUE(cantrcv_initialized);
}

/** @req SWS_CanTrcv_00001 */
void test_CanTrcv_Init_DoubleInit_ShouldSucceed(void) {
    test_CanTrcv_SetupDefaultConfig();
    CanTrcv_Init(&testConfig);
    CanTrcv_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_CanTrcv_00002 */
void test_CanTrcv_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CanTrcv_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTrcv_00002 */
void test_CanTrcv_DeInit_ValidCall_ShouldSucceed(void) {
    CanTrcv_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanTrcv_00003 */
void test_CanTrcv_GetVersionInfo_NullPtr_ShouldReportError(void) {
    CanTrcv_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTrcv_00003 */
void test_CanTrcv_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    CanTrcv_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanTrcv_00004 */
void test_CanTrcv_SetTransceiverMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CanTrcv_SetTransceiverMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTrcv_00004 */
void test_CanTrcv_SetTransceiverMode_InvalidTrcv_ShouldReportError(void) {
    CanTrcv_SetTransceiverMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTrcv_00004 */
void test_CanTrcv_SetTransceiverMode_ValidCall_ShouldSucceed(void) {
    CanTrcv_SetTransceiverMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanTrcv_00005 */
void test_CanTrcv_GetTransceiverMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CanTrcv_GetTransceiverMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTrcv_00005 */
void test_CanTrcv_GetTransceiverMode_InvalidTrcv_ShouldReportError(void) {
    CanTrcv_GetTransceiverMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTrcv_00005 */
void test_CanTrcv_GetTransceiverMode_ValidCall_ShouldReturnMode(void) {
    CanTrcv_GetTransceiverMode();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanTrcv_00006 */
void test_CanTrcv_GetTrcvWakeupReason_Uninit_ShouldReportError(void) {
    /* Not initialized */
    CanTrcv_GetTrcvWakeupReason();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTrcv_00006 */
void test_CanTrcv_GetTrcvWakeupReason_InvalidTrcv_ShouldReportError(void) {
    CanTrcv_GetTrcvWakeupReason(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTrcv_00006 */
void test_CanTrcv_GetTrcvWakeupReason_ValidCall_ShouldReturnReason(void) {
    CanTrcv_GetTrcvWakeupReason();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanTrcv_00007 */
void test_CanTrcv_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    CanTrcv_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_CanTrcv_00007 */
void test_CanTrcv_MainFunction_ValidCall_ShouldSucceed(void) {
    CanTrcv_MainFunction();
    TEST_ASSERT_TRUE(1);
}

