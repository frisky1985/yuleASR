/**
 * @file test_bswm_svc.c
 * @brief BswM Unit Tests — Substantiated (service behavior focus)
 * @version 1.0.0
 * @date 2026-08-25
 *
 * Substantiation: rewritten against the real BswM API (BswM.h). All
 * assertions verify return values, latched vs. applied mode state and
 * exact DET mock arguments (module/service/error IDs).
 */

// @tests src/bsw/services/bswm/src/BswM.c  @tests src/bsw/services/bswm/include/BswM.h

#include "unity.h"
#include "BswM.h"

/* Mock Det_ReportError — records full argument set for verification */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint16 mock_DetLastModuleId = 0U;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetLastModuleId = 0U;
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
static BswM_ConfigType testConfig;
static void test_BswM_SetupDefaultConfig(void) {
    testConfig.NumModeRequestPorts = 0U;
    testConfig.NumRules = 0U;
    testConfig.NumActionLists = 0U;
    testConfig.ModeRequestPorts = NULL_PTR;
    testConfig.Rules = NULL_PTR;
    testConfig.ActionLists = NULL_PTR;
}

void setUp(void) {
    mock_Det_Reset();
    test_BswM_SetupDefaultConfig();
    /* Force a known UNINIT state before every test (BswM_State is file-static). */
    BswM_DeInit();
}

void tearDown(void) {
}

/** @req SWS_BswM_00001 */
void test_BswM_Init_NullPtr_ShouldReportDet(void) {
    BswM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(BSWM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(0x00U, mock_DetLastApiId);   /* BSWM_SID_INIT */
    TEST_ASSERT_EQUAL_UINT8(0x10U, mock_DetLastErrorId); /* BSWM_E_PARAM_POINTER */
}

/** @req SWS_BswM_00001 */
void test_BswM_Init_ValidConfig_ShouldSucceed(void) {
    BswM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetRequestedMode());
}

/** @req SWS_BswM_00001 */
void test_BswM_Init_DoubleInit_ShouldResetRequestedMode(void) {
    BswM_Init(&testConfig);
    (void)BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN);
    /* Production Init is not guarded: re-init silently resets the state. */
    BswM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetRequestedMode());
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00002 */
void test_BswM_DeInit_Uninit_ShouldNotReportDet(void) {
    /* Production BswM_DeInit() has no DET guard — it silently resets the
     * (already uninitialized) state. Assert the actual behavior. */
    BswM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00002 */
void test_BswM_DeInit_ValidCall_ShouldReturnToUninit(void) {
    BswM_Init(&testConfig);
    BswM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(E_NOT_OK, BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x03U, mock_DetLastApiId);   /* BSWM_SID_REQUEST_MODE */
    TEST_ASSERT_EQUAL_UINT8(0x20U, mock_DetLastErrorId); /* BSWM_E_UNINIT */
}

/** @req SWS_BswM_00030 */
void test_BswM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    BswM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0xFFU, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(0x10U, mock_DetLastErrorId); /* BSWM_E_PARAM_POINTER */
}

/** @req SWS_BswM_00030 */
void test_BswM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    BswM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(BSWM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_BswM_00020 */
void test_BswM_MainFunction_Uninit_ShouldSilentlyReturn(void) {
    /* Production BswM_MainFunction() has no DET guard: it returns early. */
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00020 */
void test_BswM_MainFunction_ValidCall_ShouldApplyRequestedMode(void) {
    BswM_Init(&testConfig);
    (void)BswM_RequestMode(0U, BSWM_MODE_VALUE_SLEEP);
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SLEEP, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SLEEP, BswM_GetRequestedMode());
}

/** @req SWS_BswM_00010 */
void test_BswM_RequestMode_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0x03U, mock_DetLastApiId);   /* BSWM_SID_REQUEST_MODE */
    TEST_ASSERT_EQUAL_UINT8(0x20U, mock_DetLastErrorId); /* BSWM_E_UNINIT */
}

/** @req SWS_BswM_00010 */
void test_BswM_RequestMode_ValidCall_ShouldLatchMode(void) {
    BswM_Init(&testConfig);
    Std_ReturnType ret = BswM_RequestMode(0x55U, BSWM_MODE_VALUE_POST_RUN);
    /* SwCompositionId is ignored by production code; any value succeeds. */
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_POST_RUN, BswM_GetRequestedMode());
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00011 */
void test_BswM_GetCurrentMode_Uninit_ShouldReturnOffWithoutDet(void) {
    /* Production BswM_GetCurrentMode() never reports DET; the static state
     * is OFF before the first successful init. */
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_BswM_00011 */
void test_BswM_GetCurrentMode_AfterRequestAndMain_ShouldReturnAppliedMode(void) {
    BswM_Init(&testConfig);
    (void)BswM_RequestMode(0U, BSWM_MODE_VALUE_SHUTDOWN);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_OFF, BswM_GetCurrentMode());
    BswM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_SHUTDOWN, BswM_GetCurrentMode());
}

/** @req SWS_BswM_00012 */
void test_BswM_GetRequestedMode_AfterTwoRequests_ShouldReturnLatest(void) {
    BswM_Init(&testConfig);
    (void)BswM_RequestMode(0U, BSWM_MODE_VALUE_RUN);
    (void)BswM_RequestMode(0U, BSWM_MODE_VALUE_WAKEUP);
    TEST_ASSERT_EQUAL_UINT8(BSWM_MODE_VALUE_WAKEUP, BswM_GetRequestedMode());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_BswM_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_BswM_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_BswM_Init_DoubleInit_ShouldResetRequestedMode);
    RUN_TEST(test_BswM_DeInit_Uninit_ShouldNotReportDet);
    RUN_TEST(test_BswM_DeInit_ValidCall_ShouldReturnToUninit);
    RUN_TEST(test_BswM_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_BswM_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_BswM_MainFunction_Uninit_ShouldSilentlyReturn);
    RUN_TEST(test_BswM_MainFunction_ValidCall_ShouldApplyRequestedMode);
    RUN_TEST(test_BswM_RequestMode_Uninit_ShouldReportError);
    RUN_TEST(test_BswM_RequestMode_ValidCall_ShouldLatchMode);
    RUN_TEST(test_BswM_GetCurrentMode_Uninit_ShouldReturnOffWithoutDet);
    RUN_TEST(test_BswM_GetCurrentMode_AfterRequestAndMain_ShouldReturnAppliedMode);
    RUN_TEST(test_BswM_GetRequestedMode_AfterTwoRequests_ShouldReturnLatest);

    return UnityEnd();
}
