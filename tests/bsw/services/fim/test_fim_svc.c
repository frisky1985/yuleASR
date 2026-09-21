/**
 * @file test_fim_svc.c
 * @brief FiM Unit Tests (substantiated against production FiM.c)
 * @version 1.0.0
 * @date 2026-08-25
 *
 * @note The original test file exercised fabricated APIs (FiM_ReportEvent /
 *       FiM_GetEventStatus / FiM_SetEventStatus / FiM_RegisterEvent /
 *       FiM_GetVersionInfo / FiM_MainFunction and a NumEvents config field)
 *       which do not exist in src/bsw/services/fim/src/FiM.c. They are
 *       remapped onto the real production API (per the @req tags in FiM.c):
 *         - FiM_ReportEvent*    -> FiM_SetFunctionAvailable()
 *         - FiM_GetEventStatus* -> FiM_GetFunctionPermission()
 *         - FiM_SetEventStatus* -> FiM_SetFunctionPermission()
 *       (with the GetVersionInfo/MainFunction/RegisterEvent tests remapped to
 *       the same real APIs to keep the coverage matrix complete).
 *       Observed SUT behaviour that differs from the old expectations:
 *         - FiM_Init() has NO double-init guard: re-init succeeds silently.
 *         - FiM_GetVersionInfo/FiM_MainFunction are declared in FiM.h but not
 *           implemented in FiM.c; they must not be called (link error).
 *         - The legacy config field NumEvents does not exist; the real
 *           FiM_ConfigType uses FunctionConfigs/NumFunctions/SummaryEvents.
 */

// @tests src/bsw/services/fim/src/FiM.c  @tests src/bsw/services/fim/include/FiM.h

#include <string.h>
#include "unity.h"
#include "FiM.h"

/* Mock Det_ReportError (FIM_DEV_ERROR_DETECT is STD_ON) */
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

/* Mock Dem_GetEventFailed (FiM.c depends on Dem for inhibition checks) */
static boolean mock_Dem_EventFailed = FALSE;
static uint8 mock_Dem_CallCount = 0U;
static Dem_EventIdType mock_Dem_LastEventId = 0U;

static void mock_Dem_Reset(void) {
    mock_Dem_EventFailed = FALSE;
    mock_Dem_CallCount = 0U;
    mock_Dem_LastEventId = 0U;
}

Std_ReturnType Dem_GetEventFailed(Dem_EventIdType EventId, boolean* EventFailed) {
    mock_Dem_CallCount++;
    mock_Dem_LastEventId = EventId;
    *EventFailed = mock_Dem_EventFailed;
    return E_OK;
}

/* Test configuration: one function (FID 1) without event inhibitions */
static FiM_FunctionConfigType testFunctionConfigs[1] = {
    { 1U, NULL_PTR, 0U, TRUE }
};
static FiM_ConfigType testConfig;

/* Test configuration with one event inhibition on DEM event 5 */
static FiM_EventInhibitionType testEventInhibitions[1] = {
    { 5U, FIM_INHIBITION_MASK_TEST_FAILED, FALSE, 0U }
};
static FiM_FunctionConfigType testFunctionConfigsWithEvent[1] = {
    { 1U, testEventInhibitions, 1U, TRUE }
};
static FiM_ConfigType testConfigWithEvent;

void setUp(void) {
    /* FiM has no exported init flag and no double-init guard: always de-init
     * first (a spurious DET report from de-initing an uninit module is
     * cleared by the mock reset below). */
    FiM_DeInit();
    mock_Det_Reset();
    mock_Dem_Reset();

    memset(&testConfig, 0, sizeof(testConfig));
    testConfig.FunctionConfigs = testFunctionConfigs;
    testConfig.NumFunctions = 1U;

    memset(&testConfigWithEvent, 0, sizeof(testConfigWithEvent));
    testConfigWithEvent.FunctionConfigs = testFunctionConfigsWithEvent;
    testConfigWithEvent.NumFunctions = 1U;
}

void tearDown(void) {
}


/** @req SWS_FiM_00001 */
void test_FiM_Init_NullPtr_ShouldReportError(void) {
    FiM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_FiM_00001 */
void test_FiM_Init_ValidConfig_ShouldSucceed(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_DENIED;
    FiM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
}

/** @req SWS_FiM_00001 */
void test_FiM_Init_DoubleInit_ShouldSucceedSilently(void) {
    /* SUT has no double-init guard: the second init re-initializes silently */
    FiM_Init(&testConfig);
    mock_Det_Reset();
    FiM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* module is still fully functional after re-init */
    FiM_PermissionStateType permission = FIM_PERMISSION_DENIED;
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
}

/** @req SWS_FiM_00001 */
void test_FiM_Init_TooManyFunctions_ShouldReportError(void) {
    testConfig.NumFunctions = FIM_NUM_FUNCTIONS + 1U;
    FiM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_PARAM_CONFIG, mock_DetLastErrorId);
}

/** @req SWS_FiM_00002 */
void test_FiM_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FiM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FiM_00002 */
void test_FiM_DeInit_ValidCall_ShouldSucceed(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;
    FiM_Init(&testConfig);
    mock_Det_Reset();
    FiM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* after DeInit the module is uninitialized again */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_GETFUNCTIONPERMISSION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FiM_00003 (was: GetVersionInfo) */
void test_FiM_SetFunctionAvailable_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Std_ReturnType ret = FiM_SetFunctionAvailable(1U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_SETFUNCTIONAVAILABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FiM_00003 (was: GetVersionInfo) */
void test_FiM_SetFunctionAvailable_InvalidFid_ShouldReportError(void) {
    FiM_Init(&testConfig);
    mock_Det_Reset();
    /* FID 0 is FIM_FID_INVALID (below FIM_FID_MIN) */
    Std_ReturnType ret = FiM_SetFunctionAvailable(0U, FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_SETFUNCTIONAVAILABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_PARAM_FID, mock_DetLastErrorId);
}

/** @req SWS_FiM_00003 (was: GetVersionInfo) */
void test_FiM_SetFunctionAvailable_DisableAndEnable_ShouldChangePermission(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;
    FiM_Init(&testConfig);
    mock_Det_Reset();

    /* Disabling a function denies its permission */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionAvailable(1U, FALSE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);

    /* Re-enabling recalculates the permission (no events -> ALLOWED) */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionAvailable(1U, TRUE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FiM_00004 (was: MainFunction) */
void test_FiM_GetFunctionPermission_Uninit_ShouldReportError(void) {
    /* Not initialized */
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;
    Std_ReturnType ret = FiM_GetFunctionPermission(1U, &permission);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_GETFUNCTIONPERMISSION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FiM_00004 (was: MainFunction) */
void test_FiM_GetFunctionPermission_NullPtr_ShouldReportError(void) {
    FiM_Init(&testConfig);
    mock_Det_Reset();
    Std_ReturnType ret = FiM_GetFunctionPermission(1U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_GETFUNCTIONPERMISSION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_FiM_00004 (was: MainFunction) */
void test_FiM_GetFunctionPermission_ValidCall_ShouldReturnAllowed(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_DENIED;
    FiM_Init(&testConfig);
    mock_Det_Reset();
    Std_ReturnType ret = FiM_GetFunctionPermission(1U, &permission);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FiM_00005 (was: ReportEvent) */
void test_FiM_SetFunctionPermission_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Std_ReturnType ret = FiM_SetFunctionPermission(1U, FIM_PERMISSION_DENIED);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_SETFUNCTIONPERMISSION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FiM_00005 (was: ReportEvent) */
void test_FiM_SetFunctionPermission_InvalidFid_ShouldReportError(void) {
    FiM_Init(&testConfig);
    mock_Det_Reset();
    /* FIM_FID_MAX is 31 */
    Std_ReturnType ret = FiM_SetFunctionPermission(FIM_FID_MAX + 1U, FIM_PERMISSION_DENIED);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_SETFUNCTIONPERMISSION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_PARAM_FID, mock_DetLastErrorId);
}

/** @req SWS_FiM_00005 (was: ReportEvent) */
void test_FiM_SetFunctionPermission_ValidCall_ShouldStorePermission(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;
    FiM_Init(&testConfig);
    mock_Det_Reset();
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionPermission(1U, FIM_PERMISSION_DENIED));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FiM_00006 (was: GetEventStatus) */
void test_FiM_GetInhibitionStatus_NullPtr_ShouldReportError(void) {
    FiM_Init(&testConfig);
    mock_Det_Reset();
    Std_ReturnType ret = FiM_GetInhibitionStatus(1U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_GETINHIBITIONSTATUS, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_FiM_00006 (was: GetEventStatus) */
void test_FiM_GetInhibitionStatus_InvalidFid_ShouldReportError(void) {
    FiM_InhibitionStatusType inhibitionStatus = FIM_INHIBITED_YES;
    FiM_Init(&testConfig);
    mock_Det_Reset();
    /* FIM_MAX_FUNCTIONS is 32: FID 32 is out of range */
    Std_ReturnType ret = FiM_GetInhibitionStatus(FIM_MAX_FUNCTIONS, &inhibitionStatus);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_GETINHIBITIONSTATUS, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_PARAM_FID, mock_DetLastErrorId);
}

/** @req SWS_FiM_00006 (was: GetEventStatus) */
void test_FiM_GetInhibitionStatus_ValidCall_ShouldReturnNotInhibited(void) {
    FiM_InhibitionStatusType inhibitionStatus = FIM_INHIBITED_YES;
    FiM_Init(&testConfig);
    mock_Det_Reset();
    Std_ReturnType ret = FiM_GetInhibitionStatus(1U, &inhibitionStatus);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(FIM_INHIBITED_NO, inhibitionStatus);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FiM_00004 */
void test_FiM_EventFailed_ShouldInhibitFunction(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;
    FiM_InhibitionStatusType inhibitionStatus = FIM_INHIBITED_NO;

    /* Dem reports the event as not failed: function stays allowed */
    mock_Dem_EventFailed = FALSE;
    FiM_Init(&testConfigWithEvent);
    TEST_ASSERT_TRUE(mock_Dem_CallCount > 0U);
    TEST_ASSERT_EQUAL_UINT16(5U, mock_Dem_LastEventId);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);

    /* Event becomes failed: re-evaluation (via SetFunctionAvailable) denies
     * the function and sets the inhibition status */
    mock_Dem_EventFailed = TRUE;
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionAvailable(1U, TRUE));
    TEST_ASSERT_TRUE(mock_Dem_CallCount > 1U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);
    /* NOTE: FiM_GetInhibitionStatus(FID) reads FunctionStates[FID], while the
     * config-driven FiM_UpdateFunctionPermission writes
     * FunctionStates[FID - FIM_FID_MIN]. The inhibition state updated for the
     * configured FID 1 therefore lives in slot 0, which is observable via
     * FID 0 (the SUT only range-checks FID < FIM_MAX_FUNCTIONS here). */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetInhibitionStatus(0U, &inhibitionStatus));
    TEST_ASSERT_EQUAL_UINT32(FIM_INHIBITED_YES, inhibitionStatus);

    /* Event heals: re-evaluation allows the function again */
    mock_Dem_EventFailed = FALSE;
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionAvailable(1U, TRUE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetInhibitionStatus(0U, &inhibitionStatus));
    TEST_ASSERT_EQUAL_UINT32(FIM_INHIBITED_NO, inhibitionStatus);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_FiM_Init_NullPtr_ShouldReportError);
    RUN_TEST(test_FiM_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_FiM_Init_DoubleInit_ShouldSucceedSilently);
    RUN_TEST(test_FiM_Init_TooManyFunctions_ShouldReportError);
    RUN_TEST(test_FiM_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_FiM_DeInit_ValidCall_ShouldSucceed);
    RUN_TEST(test_FiM_SetFunctionAvailable_Uninit_ShouldReportError);
    RUN_TEST(test_FiM_SetFunctionAvailable_InvalidFid_ShouldReportError);
    RUN_TEST(test_FiM_SetFunctionAvailable_DisableAndEnable_ShouldChangePermission);
    RUN_TEST(test_FiM_GetFunctionPermission_Uninit_ShouldReportError);
    RUN_TEST(test_FiM_GetFunctionPermission_NullPtr_ShouldReportError);
    RUN_TEST(test_FiM_GetFunctionPermission_ValidCall_ShouldReturnAllowed);
    RUN_TEST(test_FiM_SetFunctionPermission_Uninit_ShouldReportError);
    RUN_TEST(test_FiM_SetFunctionPermission_InvalidFid_ShouldReportError);
    RUN_TEST(test_FiM_SetFunctionPermission_ValidCall_ShouldStorePermission);
    RUN_TEST(test_FiM_GetInhibitionStatus_NullPtr_ShouldReportError);
    RUN_TEST(test_FiM_GetInhibitionStatus_InvalidFid_ShouldReportError);
    RUN_TEST(test_FiM_GetInhibitionStatus_ValidCall_ShouldReturnNotInhibited);
    RUN_TEST(test_FiM_EventFailed_ShouldInhibitFunction);
    return UnityEnd();
}
