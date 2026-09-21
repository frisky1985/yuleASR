/**
 * @file test_bswm.c
 * @brief BswM (BSW Manager) Unit Tests — Substantiated
 * @req SWS_BswM
 *
 * Substantiation: every assertion checks observable behavior of the
 * production BswM implementation (src/bsw/services/bswm/src/BswM.c):
 * return values (E_OK/E_NOT_OK), internal mode state before/after
 * BswM_MainFunction, version info fields and DET mock call arguments.
 */

// @tests src/bsw/services/bswm/src/BswM.c  @tests src/bsw/services/bswm/include/BswM.h
#include "unity.h"
#include "BswM.h"

/* Service IDs and error codes from BswM.c */
#define BSWM_SID_INIT               0x00U
#define BSWM_SID_REQUEST_MODE       0x03U
#define BSWM_E_PARAM_POINTER        0x10U
#define BSWM_E_UNINIT               0x20U

static uint8 mock_DetCalls = 0U;
static uint16 mock_DetLastModuleId = 0U;
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)InstanceId;
    mock_DetLastModuleId = ModuleId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCalls++;
    return E_OK;
}

static BswM_ConfigType testConfig;

void setUp(void) {
    mock_DetCalls = 0U;
    mock_DetLastModuleId = 0U;
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    testConfig.NumModeRequestPorts = 0U;
    testConfig.NumRules = 0U;
    testConfig.NumActionLists = 0U;
    testConfig.ModeRequestPorts = NULL_PTR;
    testConfig.Rules = NULL_PTR;
    testConfig.ActionLists = NULL_PTR;
    /* Force a known UNINIT state before every test (BswM_State is file-static). */
    BswM_DeInit();
}

void tearDown(void) {}

/** @req SWS_BswM_00001 */
void test_BswM_Init_NullPtr_ShouldReportDet(void) {
    BswM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT16(BSWM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_PARAM_POINTER, mock_DetLastErrorId);
    /* Module must remain uninitialized: a mode request still fails. */
    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN));
}

/** @req SWS_BswM_00001 */
void test_BswM_Init_ValidConfig_ShouldSucceed(void) {
    BswM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    /* Fresh init resets both mode registers to OFF. */
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetRequestedMode());
}

/** @req SWS_BswM_00002 */
void test_BswM_DeInit_AfterInit_ShouldReturnToUninit(void) {
    BswM_Init(&testConfig);
    BswM_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_REQUEST_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_BswM_00010 */
void test_BswM_RequestMode_AfterInit_ShouldSucceed(void) {
    BswM_Init(&testConfig);
    Std_ReturnType ret = BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    /* Request is latched but not applied until BswM_MainFunction runs. */
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, BswM_GetRequestedMode());
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00011 */
void test_BswM_GetCurrentMode_AfterInit_ShouldReturnOff(void) {
    BswM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_BswM_00012 */
void test_BswM_GetRequestedMode_AfterRequest_ShouldReturnMode(void) {
    BswM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN));
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, BswM_GetRequestedMode());
}

/** @req SWS_BswM_00020 */
void test_BswM_MainFunction_AfterRequest_ShouldApplyMode(void) {
    BswM_Init(&testConfig);
    (void)BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, BswM_GetCurrentMode());
    /* Request mask is consumed: a second run must not change the mode. */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_RUN, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_BswM_00030 */
void test_BswM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    BswM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(BSWM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_BswM_00030 */
void test_BswM_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    BswM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT16(BSWM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(0xFFU, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_BswM_Init_DoubleInit_ShouldNotCrash(void) {
    BswM_Init(&testConfig);
    BswM_Init(&testConfig);
    /* Production code does not guard double init: no DET, state stays INIT. */
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

void test_BswM_DeInit_BeforeInit_ShouldNotCrash(void) {
    /* setUp() already forced UNINIT; DeInit on an uninitialized module is a
     * no-op in production and must not report a development error. */
    BswM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN));
}

void test_BswM_RequestMode_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SID_REQUEST_MODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(BSWM_E_UNINIT, mock_DetLastErrorId);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_BswM_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_BswM_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_BswM_DeInit_BeforeInit_ShouldNotCrash);
    RUN_TEST(test_BswM_DeInit_AfterInit_ShouldReturnToUninit);
    RUN_TEST(test_BswM_RequestMode_BeforeInit_ShouldFail);
    RUN_TEST(test_BswM_Init_DoubleInit_ShouldNotCrash);
    RUN_TEST(test_BswM_RequestMode_AfterInit_ShouldSucceed);
    RUN_TEST(test_BswM_GetCurrentMode_AfterInit_ShouldReturnOff);
    RUN_TEST(test_BswM_GetRequestedMode_AfterRequest_ShouldReturnMode);
    RUN_TEST(test_BswM_MainFunction_AfterRequest_ShouldApplyMode);
    RUN_TEST(test_BswM_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_BswM_GetVersionInfo_NullPtr_ShouldReportDet);

    return UnityEnd();
}
