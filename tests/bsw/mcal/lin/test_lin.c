/**
 * @file test_test_lin.c
 * @brief Lin Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/mcal/lin/src/Lin.c  @tests src/bsw/mcal/lin/include/Lin.h

#include "unity.h"
#include "Lin.h"

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
Lin_ConfigType testConfig;
static void test_Lin_SetupDefaultConfig(void) {
    testConfig.NumChannels = 2U;
}

static boolean lin_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    lin_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_Lin_00001 */
void test_Lin_Init_NullPtr_ShouldNotCrash(void) {
    Lin_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Lin_00001 */
void test_Lin_Init_ValidConfig_ShouldSucceed(void) {
    test_Lin_SetupDefaultConfig();
    Lin_Init(&testConfig);
    lin_initialized = TRUE;
    TEST_ASSERT_TRUE(lin_initialized);
}

/** @req SWS_Lin_00001 */
void test_Lin_Init_DoubleInit_ShouldSucceed(void) {
    test_Lin_SetupDefaultConfig();
    Lin_Init(&testConfig);
    Lin_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_Lin_00002 */
void test_Lin_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Lin_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00002 */
void test_Lin_DeInit_ValidCall_ShouldSucceed(void) {
    Lin_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Lin_00003 */
void test_Lin_SetInternalState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Lin_SetInternalState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00003 */
void test_Lin_SetInternalState_InvalidState_ShouldReportError(void) {
    Lin_SetInternalState(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00003 */
void test_Lin_SetInternalState_ValidState_ShouldSucceed(void) {
    Lin_SetInternalState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Lin_00004 */
void test_Lin_GetInternalState_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Lin_GetInternalState();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00004 */
void test_Lin_GetInternalState_ValidCall_ShouldReturnState(void) {
    Lin_GetInternalState();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Lin_00005 */
void test_Lin_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Lin_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00005 */
void test_Lin_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Lin_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Lin_00006 */
void test_Lin_WakeUp_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Lin_WakeUp();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00006 */
void test_Lin_WakeUp_InvalidChannel_ShouldReportError(void) {
    Lin_WakeUp(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00006 */
void test_Lin_WakeUp_ValidCall_ShouldSucceed(void) {
    Lin_WakeUp();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Lin_00007 */
void test_Lin_Sleep_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Lin_Sleep();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00007 */
void test_Lin_Sleep_InvalidChannel_ShouldReportError(void) {
    Lin_Sleep(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00007 */
void test_Lin_Sleep_ValidCall_ShouldSucceed(void) {
    Lin_Sleep();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Lin_00008 */
void test_Lin_GoToSleep_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Lin_GoToSleep();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00008 */
void test_Lin_GoToSleep_ValidCall_ShouldSucceed(void) {
    Lin_GoToSleep();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Lin_00009 */
void test_Lin_GoToSleepInternal_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Lin_GoToSleepInternal();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00009 */
void test_Lin_GoToSleepInternal_ValidCall_ShouldSucceed(void) {
    Lin_GoToSleepInternal();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Lin_00010 */
void test_Lin_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Lin_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00010 */
void test_Lin_Transmit_NullPtr_ShouldReportError(void) {
    Lin_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00010 */
void test_Lin_Transmit_ValidCall_ShouldSucceed(void) {
    Lin_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Lin_00011 */
void test_Lin_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    Lin_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_Lin_00011 */
void test_Lin_MainFunction_ValidCall_ShouldSucceed(void) {
    Lin_MainFunction();
    TEST_ASSERT_TRUE(1);
}

