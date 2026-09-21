/**
 * @file test_canm.c
 * @brief CanNm (CAN Network Management) Unit Tests
 * @req SWS_CanNm
 */

// @tests src/bsw/services/canm/src/CanNm.c  @tests src/bsw/services/canm/include/CanNm.h
#include "unity.h"
#include "CanNm.h"

/* Mock Det_ReportError — CanNm.c reports through this hook (CANNM_DEV_ERROR_DETECT = STD_ON) */
static uint16 mock_DetLastModuleId = 0xFFFFU;
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastModuleId = 0xFFFFU;
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)InstanceId;
    mock_DetLastModuleId = ModuleId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test config */
static CanNm_ConfigType testConfig;

void setUp(void) {
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_CanNm_00001 */
void test_CanNm_Init_NullPtr_ShouldReportDet(void) {
    CanNm_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(CANNM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(CANNM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_CanNm_00001 */
void test_CanNm_Init_ValidConfig_ShouldNotReportDet(void) {
    CanNm_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_CanNm_00001 */
void test_CanNm_Init_DoubleInit_ShouldNotReportDet(void) {
    CanNm_Init(&testConfig);
    CanNm_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_CanNm_00001 */
void test_CanNm_Init_ValidAfterNull_ShouldStillAcceptConfig(void) {
    CanNm_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    CanNm_Init(&testConfig);
    /* No additional DET report: Init accepts a valid configuration */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
}

/** @req SWS_CanNm_00001 */
void test_CanNm_Init_NullAfterValid_ShouldReportDetAgain(void) {
    CanNm_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    CanNm_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANNM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANNM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_CanNm_00001 */
void test_CanNm_Init_RepeatedValidInit_KeepsSilentDet(void) {
    CanNm_Init(&testConfig);
    CanNm_Init(&testConfig);
    CanNm_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_CanNm_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_CanNm_Init_ValidConfig_ShouldNotReportDet);
    RUN_TEST(test_CanNm_Init_DoubleInit_ShouldNotReportDet);
    RUN_TEST(test_CanNm_Init_ValidAfterNull_ShouldStillAcceptConfig);
    RUN_TEST(test_CanNm_Init_NullAfterValid_ShouldReportDetAgain);
    RUN_TEST(test_CanNm_Init_RepeatedValidInit_KeepsSilentDet);

    return UnityEnd();
}
