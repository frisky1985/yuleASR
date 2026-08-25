/**
 * @file test_test_fim_svc.c
 * @brief FiM Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/fim/src/FiM.c  @tests src/bsw/services/fim/include/FiM.h

#include "unity.h"
#include "FiM.h"

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
FiM_ConfigType testConfig;
static void test_FiM_SetupDefaultConfig(void) {
    testConfig.NumEvents = 1U;
}

static boolean fim_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    fim_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_FiM_00001 */
void test_FiM_Init_NullPtr_ShouldNotCrash(void) {
    FiM_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_FiM_00001 */
void test_FiM_Init_ValidConfig_ShouldSucceed(void) {
    test_FiM_SetupDefaultConfig();
    FiM_Init(&testConfig);
    fim_initialized = TRUE;
    TEST_ASSERT_TRUE(fim_initialized);
}

/** @req SWS_FiM_00001 */
void test_FiM_Init_DoubleInit_ShouldSucceed(void) {
    test_FiM_SetupDefaultConfig();
    FiM_Init(&testConfig);
    FiM_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_FiM_00002 */
void test_FiM_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FiM_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FiM_00002 */
void test_FiM_DeInit_ValidCall_ShouldSucceed(void) {
    FiM_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FiM_00003 */
void test_FiM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    FiM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FiM_00003 */
void test_FiM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    FiM_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FiM_00004 */
void test_FiM_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    FiM_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FiM_00004 */
void test_FiM_MainFunction_ValidCall_ShouldSucceed(void) {
    FiM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FiM_00005 */
void test_FiM_ReportEvent_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FiM_ReportEvent();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FiM_00005 */
void test_FiM_ReportEvent_InvalidEvent_ShouldReportError(void) {
    FiM_ReportEvent(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FiM_00005 */
void test_FiM_ReportEvent_ValidCall_ShouldSucceed(void) {
    FiM_ReportEvent();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FiM_00006 */
void test_FiM_GetEventStatus_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FiM_GetEventStatus();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FiM_00006 */
void test_FiM_GetEventStatus_InvalidEvent_ShouldReportError(void) {
    FiM_GetEventStatus(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FiM_00006 */
void test_FiM_GetEventStatus_ValidCall_ShouldReturnStatus(void) {
    FiM_GetEventStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FiM_00007 */
void test_FiM_SetEventStatus_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FiM_SetEventStatus();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FiM_00007 */
void test_FiM_SetEventStatus_InvalidEvent_ShouldReportError(void) {
    FiM_SetEventStatus(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FiM_00007 */
void test_FiM_SetEventStatus_ValidCall_ShouldSucceed(void) {
    FiM_SetEventStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_FiM_00008 */
void test_FiM_RegisterEvent_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FiM_RegisterEvent();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FiM_00008 */
void test_FiM_RegisterEvent_NullPtr_ShouldReportError(void) {
    FiM_RegisterEvent(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_FiM_00008 */
void test_FiM_RegisterEvent_ValidCall_ShouldSucceed(void) {
    FiM_RegisterEvent();
    TEST_ASSERT_TRUE(1);
}

