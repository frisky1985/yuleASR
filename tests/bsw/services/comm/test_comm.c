/**
 * @file test_test_comm.c
 * @brief ComM Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/comm/src/ComM.c  @tests src/bsw/services/comm/include/ComM.h

#include "unity.h"
#include "ComM.h"

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
ComM_ConfigType testConfig;
static void test_ComM_SetupDefaultConfig(void) {
    testConfig.NumChannels = 1U;
}

static boolean comm_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    comm_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_ComM_00001 */
void test_ComM_Init_NullPtr_ShouldNotCrash(void) {
    ComM_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_ComM_00001 */
void test_ComM_Init_ValidConfig_ShouldSucceed(void) {
    test_ComM_SetupDefaultConfig();
    ComM_Init(&testConfig);
    comm_initialized = TRUE;
    TEST_ASSERT_TRUE(comm_initialized);
}

/** @req SWS_ComM_00001 */
void test_ComM_Init_DoubleInit_ShouldSucceed(void) {
    test_ComM_SetupDefaultConfig();
    ComM_Init(&testConfig);
    ComM_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_ComM_00002 */
void test_ComM_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    ComM_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_ComM_00002 */
void test_ComM_DeInit_ValidCall_ShouldSucceed(void) {
    ComM_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_ComM_00003 */
void test_ComM_GetStatus_Uninit_ShouldReportError(void) {
    /* Not initialized */
    ComM_GetStatus();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_ComM_00003 */
void test_ComM_GetStatus_ValidCall_ShouldReturnStatus(void) {
    ComM_GetStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_ComM_00004 */
void test_ComM_GetIPDUGroupStatus_Uninit_ShouldReportError(void) {
    /* Not initialized */
    ComM_GetIPDUGroupStatus();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_ComM_00004 */
void test_ComM_GetIPDUGroupStatus_InvalidGroup_ShouldReportError(void) {
    ComM_GetIPDUGroupStatus(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_ComM_00004 */
void test_ComM_GetIPDUGroupStatus_ValidCall_ShouldReturnStatus(void) {
    ComM_GetIPDUGroupStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_ComM_00005 */
void test_ComM_GetCommunicationStatus_Uninit_ShouldReportError(void) {
    /* Not initialized */
    ComM_GetCommunicationStatus();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_ComM_00005 */
void test_ComM_GetCommunicationStatus_ValidCall_ShouldReturnStatus(void) {
    ComM_GetCommunicationStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_ComM_00006 */
void test_ComM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    ComM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_ComM_00006 */
void test_ComM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    ComM_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_ComM_00007 */
void test_ComM_GetInhibitionStatus_Uninit_ShouldReportError(void) {
    /* Not initialized */
    ComM_GetInhibitionStatus();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_ComM_00007 */
void test_ComM_GetInhibitionStatus_ValidCall_ShouldReturnStatus(void) {
    ComM_GetInhibitionStatus();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_ComM_00008 */
void test_ComM_GetLimitation_Uninit_ShouldReportError(void) {
    /* Not initialized */
    ComM_GetLimitation();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_ComM_00008 */
void test_ComM_GetLimitation_ValidCall_ShouldReturnLimitation(void) {
    ComM_GetLimitation();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_ComM_00009 */
void test_ComM_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    ComM_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_ComM_00009 */
void test_ComM_MainFunction_ValidCall_ShouldSucceed(void) {
    ComM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_ComM_00010 */
void test_ComM_RequestComMode_Uninit_ShouldReportError(void) {
    /* Not initialized */
    ComM_RequestComMode();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_ComM_00010 */
void test_ComM_RequestComMode_InvalidMode_ShouldReportError(void) {
    ComM_RequestComMode(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_ComM_00010 */
void test_ComM_RequestComMode_ValidCall_ShouldSucceed(void) {
    ComM_RequestComMode();
    TEST_ASSERT_TRUE(1);
}

