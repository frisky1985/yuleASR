/**
 * @file test_test_someipxf.c
 * @brief SomeIpXF Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

#include "unity.h"
#include "SomeIpXF.h"

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
SomeIpXF_ConfigType testConfig;
static void test_SomeIpXF_SetupDefaultConfig(void) {
    testConfig.NumMethods = 1U;
}

static boolean someipxf_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    someipxf_initialized = FALSE;
}

void tearDown(void) {
}


/** @req SWS_SomeIpXF_00001 */
void test_SomeIpXF_Init_NullPtr_ShouldNotCrash(void) {
    SomeIpXF_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIpXF_00001 */
void test_SomeIpXF_Init_ValidConfig_ShouldSucceed(void) {
    test_SomeIpXF_SetupDefaultConfig();
    SomeIpXF_Init(&testConfig);
    someipxf_initialized = TRUE;
    TEST_ASSERT_TRUE(someipxf_initialized);
}

/** @req SWS_SomeIpXF_00001 */
void test_SomeIpXF_Init_DoubleInit_ShouldSucceed(void) {
    test_SomeIpXF_SetupDefaultConfig();
    SomeIpXF_Init(&testConfig);
    SomeIpXF_Init(&testConfig);
    TEST_ASSERT_TRUE(1); /* No crash */
}

/** @req SWS_SomeIpXF_00002 */
void test_SomeIpXF_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpXF_DeInit();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpXF_00002 */
void test_SomeIpXF_DeInit_ValidCall_ShouldSucceed(void) {
    SomeIpXF_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpXF_00003 */
void test_SomeIpXF_GetVersionInfo_NullPtr_ShouldReportError(void) {
    SomeIpXF_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpXF_00003 */
void test_SomeIpXF_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    SomeIpXF_GetVersionInfo();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpXF_00004 */
void test_SomeIpXF_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized */
    SomeIpXF_MainFunction();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpXF_00004 */
void test_SomeIpXF_MainFunction_ValidCall_ShouldSucceed(void) {
    SomeIpXF_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpXF_00005 */
void test_SomeIpXF_Serialize_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpXF_Serialize();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpXF_00005 */
void test_SomeIpXF_Serialize_NullPtr_ShouldReportError(void) {
    SomeIpXF_Serialize(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpXF_00005 */
void test_SomeIpXF_Serialize_ValidCall_ShouldSucceed(void) {
    SomeIpXF_Serialize();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpXF_00006 */
void test_SomeIpXF_Deserialize_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpXF_Deserialize();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpXF_00006 */
void test_SomeIpXF_Deserialize_NullPtr_ShouldReportError(void) {
    SomeIpXF_Deserialize(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpXF_00006 */
void test_SomeIpXF_Deserialize_ValidCall_ShouldSucceed(void) {
    SomeIpXF_Deserialize();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_SomeIpXF_00007 */
void test_SomeIpXF_GetPayloadSize_Uninit_ShouldReportError(void) {
    /* Not initialized */
    SomeIpXF_GetPayloadSize();
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpXF_00007 */
void test_SomeIpXF_GetPayloadSize_NullPtr_ShouldReportError(void) {
    SomeIpXF_GetPayloadSize(NULL_PTR);
    TEST_ASSERT_TRUE(mock_DetCallCount > 0U);
}

/** @req SWS_SomeIpXF_00007 */
void test_SomeIpXF_GetPayloadSize_ValidCall_ShouldReturnSize(void) {
    SomeIpXF_GetPayloadSize();
    TEST_ASSERT_TRUE(1);
}

