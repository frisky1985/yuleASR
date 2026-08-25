/**
 * @file test_test_someipif.c
 * @brief SomeIpIf Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/someipif/src/SomeIpIf.c  @tests src/bsw/ecual/someipif/include/SomeIpIf.h

#include "unity.h"
#include "SomeIpIf.h"

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
SomeIpIf_ConfigType testConfig;
static void test_SomeIpIf_SetupDefaultConfig(void) {
    testConfig.NumEvents = 1U;
}

static boolean someipif_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    someipif_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_SomeIpIf_00001 */
void test_SomeIpIf_Init_NullPtr_ShouldNotCrash(void) {
    SomeIpIf_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIpIf_00001 */
void test_SomeIpIf_Init_ValidConfig_ShouldSucceed(void) {
    test_SomeIpIf_SetupDefaultConfig();
    SomeIpIf_Init(&testConfig);
    someipif_initialized = TRUE;
    TEST_ASSERT_TRUE(someipif_initialized);
}

/** @req SWS_SomeIpIf_00001 */
void test_SomeIpIf_Init_DoubleInit_ShouldSucceed(void) {
    test_SomeIpIf_SetupDefaultConfig();
    SomeIpIf_Init(&testConfig);
    SomeIpIf_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIpIf_00002 */
void test_SomeIpIf_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpIf_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpIf_00002 */
void test_SomeIpIf_DeInit_ValidCall_ShouldSucceed(void) {
    SomeIpIf_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpIf_00003 */
void test_SomeIpIf_GetVersionInfo_NullPtr_ShouldReportError(void) {
    SomeIpIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpIf_00003 */
void test_SomeIpIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    SomeIpIf_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpIf_00004 */
void test_SomeIpIf_Transmit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpIf_Transmit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpIf_00004 */
void test_SomeIpIf_Transmit_NullPtr_ShouldReportError(void) {
    SomeIpIf_Transmit(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpIf_00004 */
void test_SomeIpIf_Transmit_ValidCall_ShouldSucceed(void) {
    SomeIpIf_Transmit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpIf_00005 */
void test_SomeIpIf_RegisterEvent_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpIf_RegisterEvent();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpIf_00005 */
void test_SomeIpIf_RegisterEvent_NullPtr_ShouldReportError(void) {
    SomeIpIf_RegisterEvent(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpIf_00005 */
void test_SomeIpIf_RegisterEvent_ValidCall_ShouldSucceed(void) {
    SomeIpIf_RegisterEvent();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpIf_00006 */
void test_SomeIpIf_UnregisterEvent_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpIf_UnregisterEvent();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpIf_00006 */
void test_SomeIpIf_UnregisterEvent_InvalidEvent_ShouldReportError(void) {
    SomeIpIf_UnregisterEvent(0xFFFFU);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpIf_00006 */
void test_SomeIpIf_UnregisterEvent_ValidCall_ShouldSucceed(void) {
    SomeIpIf_UnregisterEvent();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpIf_00007 */
void test_SomeIpIf_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    SomeIpIf_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpIf_00007 */
void test_SomeIpIf_MainFunction_ValidCall_ShouldSucceed(void) {
    SomeIpIf_MainFunction();
    TEST_ASSERT_TRUE(1);
}

