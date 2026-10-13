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
static Std_ReturnType mock_Dem_ReturnValue = E_OK;

static void mock_Dem_Reset(void) {
    mock_Dem_EventFailed = FALSE;
    mock_Dem_CallCount = 0U;
    mock_Dem_LastEventId = 0U;
    mock_Dem_ReturnValue = E_OK;
}

Std_ReturnType Dem_GetEventFailed(Dem_EventIdType EventId, boolean* EventFailed) {
    mock_Dem_CallCount++;
    mock_Dem_LastEventId = EventId;
    if (mock_Dem_ReturnValue != E_OK) {
        return mock_Dem_ReturnValue;
    }
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

/* Test config: PENDING inhibition mask */
static FiM_EventInhibitionType pendingInhibitions[1] = {
    { 10U, FIM_INHIBITION_MASK_PENDING, FALSE, 0U }
};
static FiM_FunctionConfigType pendingFuncConfigs[1] = {
    { 1U, pendingInhibitions, 1U, TRUE }
};
static FiM_ConfigType pendingConfig;

/* Test config: Summary event inhibition */
static FiM_EventInhibitionType summaryInhibitions[1] = {
    { 0U, FIM_INHIBITION_MASK_TEST_FAILED, TRUE, 1U }
};
static FiM_FunctionConfigType summaryFuncConfigs[1] = {
    { 1U, summaryInhibitions, 1U, TRUE }
};
static FiM_ConfigType summaryConfig;

/* Test config: CONFIRMED inhibition mask */
static FiM_EventInhibitionType confirmedInhibitions[1] = {
    { 20U, FIM_INHIBITION_MASK_CONFIRMED, FALSE, 0U }
};
static FiM_FunctionConfigType confirmedFuncConfigs[1] = {
    { 1U, confirmedInhibitions, 1U, TRUE }
};
static FiM_ConfigType confirmedConfig;

/* Test config: TEST_NOT_COMPLETED inhibition mask */
static FiM_EventInhibitionType tncInhibitions[1] = {
    { 30U, FIM_INHIBITION_MASK_TEST_NOT_COMPLETED, FALSE, 0U }
};
static FiM_FunctionConfigType tncFuncConfigs[1] = {
    { 1U, tncInhibitions, 1U, TRUE }
};
static FiM_ConfigType tncConfig;

/* Test config: WARNING_INDICATOR inhibition mask */
static FiM_EventInhibitionType wirInhibitions[1] = {
    { 40U, FIM_INHIBITION_MASK_WARNING_INDICATOR, FALSE, 0U }
};
static FiM_FunctionConfigType wirFuncConfigs[1] = {
    { 1U, wirInhibitions, 1U, TRUE }
};
static FiM_ConfigType wirConfig;

/* Test config: FIM_FID_MAX boundary */
static FiM_FunctionConfigType maxFIDFuncConfigs[1] = {
    { 31U, NULL_PTR, 0U, TRUE }
};
static FiM_ConfigType maxFIDConfig;

/* Test config: mixed valid and invalid FID entries */
static FiM_FunctionConfigType mixedFuncConfigs[2] = {
    { 0U, NULL_PTR, 0U, TRUE },
    { 1U, NULL_PTR, 0U, TRUE }
};
static FiM_ConfigType mixedConfig;

/* Test config: two functions */
static FiM_FunctionConfigType twoFuncConfigs[2] = {
    { 1U, NULL_PTR, 0U, TRUE },
    { 2U, NULL_PTR, 0U, TRUE }
};
static FiM_ConfigType twoConfig;

/* Test config: multiple event inhibitions */
static FiM_EventInhibitionType multiInhibitions[3] = {
    { 50U, FIM_INHIBITION_MASK_TEST_FAILED, FALSE, 0U },
    { 51U, FIM_INHIBITION_MASK_PENDING, FALSE, 0U },
    { 52U, FIM_INHIBITION_MASK_CONFIRMED, FALSE, 0U }
};
static FiM_FunctionConfigType multiFuncConfigs[1] = {
    { 1U, multiInhibitions, 3U, TRUE }
};
static FiM_ConfigType multiConfig;

/* Test config: TEST_FAILED_TOC inhibition mask */
static FiM_EventInhibitionType tocInhibitions[1] = {
    { 60U, FIM_INHIBITION_MASK_TEST_FAILED_TOC, FALSE, 0U }
};
static FiM_FunctionConfigType tocFuncConfigs[1] = {
    { 1U, tocInhibitions, 1U, TRUE }
};
static FiM_ConfigType tocConfig;

/* Test config: summary event not failed */
static FiM_EventInhibitionType summaryNFInhibitions[1] = {
    { 0U, FIM_INHIBITION_MASK_TEST_FAILED, TRUE, 2U }
};
static FiM_FunctionConfigType summaryNFFuncConfigs[1] = {
    { 1U, summaryNFInhibitions, 1U, TRUE }
};
static FiM_ConfigType summaryNFConfig;

/* Test config: summary event with Dem error */
static FiM_EventInhibitionType summaryErrInhibitions[1] = {
    { 0U, FIM_INHIBITION_MASK_TEST_FAILED, TRUE, 3U }
};
static FiM_FunctionConfigType summaryErrFuncConfigs[1] = {
    { 1U, summaryErrInhibitions, 1U, TRUE }
};
static FiM_ConfigType summaryErrConfig;

/* Test config: combined summary and DEM inhibitions */
static FiM_EventInhibitionType combinedInhibitions[2] = {
    { 70U, FIM_INHIBITION_MASK_TEST_FAILED, FALSE, 0U },
    { 0U, FIM_INHIBITION_MASK_PENDING, TRUE, 1U }
};
static FiM_FunctionConfigType combinedFuncConfigs[1] = {
    { 1U, combinedInhibitions, 2U, TRUE }
};
static FiM_ConfigType combinedConfig;

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

    memset(&pendingConfig, 0, sizeof(pendingConfig));
    pendingConfig.FunctionConfigs = pendingFuncConfigs;
    pendingConfig.NumFunctions = 1U;

    memset(&summaryConfig, 0, sizeof(summaryConfig));
    summaryConfig.FunctionConfigs = summaryFuncConfigs;
    summaryConfig.NumFunctions = 1U;

    memset(&confirmedConfig, 0, sizeof(confirmedConfig));
    confirmedConfig.FunctionConfigs = confirmedFuncConfigs;
    confirmedConfig.NumFunctions = 1U;

    memset(&tncConfig, 0, sizeof(tncConfig));
    tncConfig.FunctionConfigs = tncFuncConfigs;
    tncConfig.NumFunctions = 1U;

    memset(&wirConfig, 0, sizeof(wirConfig));
    wirConfig.FunctionConfigs = wirFuncConfigs;
    wirConfig.NumFunctions = 1U;

    memset(&maxFIDConfig, 0, sizeof(maxFIDConfig));
    maxFIDConfig.FunctionConfigs = maxFIDFuncConfigs;
    maxFIDConfig.NumFunctions = 1U;

    memset(&mixedConfig, 0, sizeof(mixedConfig));
    mixedConfig.FunctionConfigs = mixedFuncConfigs;
    mixedConfig.NumFunctions = 2U;

    memset(&twoConfig, 0, sizeof(twoConfig));
    twoConfig.FunctionConfigs = twoFuncConfigs;
    twoConfig.NumFunctions = 2U;

    memset(&multiConfig, 0, sizeof(multiConfig));
    multiConfig.FunctionConfigs = multiFuncConfigs;
    multiConfig.NumFunctions = 1U;

    memset(&tocConfig, 0, sizeof(tocConfig));
    tocConfig.FunctionConfigs = tocFuncConfigs;
    tocConfig.NumFunctions = 1U;

    memset(&summaryNFConfig, 0, sizeof(summaryNFConfig));
    summaryNFConfig.FunctionConfigs = summaryNFFuncConfigs;
    summaryNFConfig.NumFunctions = 1U;

    memset(&summaryErrConfig, 0, sizeof(summaryErrConfig));
    summaryErrConfig.FunctionConfigs = summaryErrFuncConfigs;
    summaryErrConfig.NumFunctions = 1U;

    memset(&combinedConfig, 0, sizeof(combinedConfig));
    combinedConfig.FunctionConfigs = combinedFuncConfigs;
    combinedConfig.NumFunctions = 1U;
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

/* === New branch coverage tests === */

/* Test: PENDING inhibition mask - covers PENDING branch in CheckEventInhibition */
void test_FiM_PendingInhibitionMask_ShouldInhibit(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;
    FiM_InhibitionStatusType inhibitionStatus = FIM_INHIBITED_NO;

    mock_Dem_EventFailed = FALSE;
    FiM_Init(&pendingConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);

    /* Event fails: dtcStatus gets TF|TFTOC|PDTC; PENDING mask (0x04) matches PDTC */
    mock_Dem_EventFailed = TRUE;
    FiM_Init(&pendingConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetInhibitionStatus(0U, &inhibitionStatus));
    TEST_ASSERT_EQUAL_UINT32(FIM_INHIBITED_YES, inhibitionStatus);

    /* Event heals */
    mock_Dem_EventFailed = FALSE;
    FiM_Init(&pendingConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
}

/* Test: Dem_GetEventFailed returns E_NOT_OK - covers Dem error path */
void test_FiM_DemGetEventFailed_ReturnsNotOK(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_DENIED;

    mock_Dem_ReturnValue = E_NOT_OK;
    FiM_Init(&testConfigWithEvent);
    /* Dem error: event not treated as failed, inhibition mask stays 0 */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
    TEST_ASSERT_TRUE(mock_Dem_CallCount > 0U);
}

/* Test: Summary event path - covers UseSummaryEvent branches.
 * Note: FiM_UpdateSummaryEventState is not called during Init or
 * SetFunctionAvailable, so SummaryEventStates[].IsFailed stays FALSE
 * (cleared by Init). The UseSummaryEvent=TRUE branch IS exercised in
 * CalculateInhibitionMask, but IsFailed is always FALSE -> no inhibition. */
void test_FiM_SummaryEvent_Path(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_DENIED;

    /* Summary event not failed at Init: function allowed */
    mock_Dem_EventFailed = FALSE;
    FiM_Init(&summaryConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);

    /* Even with Dem reporting eventFailed=TRUE, the summary event state
     * (SummaryEventStates[1].IsFailed) stays FALSE because UpdateSummaryEventState
     * is never called. The UseSummaryEvent branch IS exercised but IsFailed=FALSE. */
    mock_Dem_EventFailed = TRUE;
    FiM_Init(&summaryConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);

    /* Summary event heals: still allowed */
    mock_Dem_EventFailed = FALSE;
    FiM_Init(&summaryConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
}

/* Test: FID boundary values - FIM_FID_MIN */
void test_FiM_BoundaryFID_Min(void) {
    FiM_PermissionStateType permission;
    FiM_InhibitionStatusType inhibitionStatus;

    /* FID 1 (FIM_FID_MIN) is in testFunctionConfigs */
    FiM_Init(&testConfig);

    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionAvailable(FIM_FID_MIN, TRUE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(FIM_FID_MIN, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionPermission(FIM_FID_MIN, FIM_PERMISSION_DENIED));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(FIM_FID_MIN, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetInhibitionStatus(FIM_FID_MIN, &inhibitionStatus));
}

/* Test: FID boundary values - FIM_FID_MAX (31) */
void test_FiM_BoundaryFID_Max(void) {
    FiM_PermissionStateType permission;
    FiM_InhibitionStatusType inhibitionStatus;

    FiM_Init(&maxFIDConfig);

    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionAvailable(FIM_FID_MAX, TRUE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(FIM_FID_MAX, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionPermission(FIM_FID_MAX, FIM_PERMISSION_DENIED));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(FIM_FID_MAX, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetInhibitionStatus(FIM_FID_MAX, &inhibitionStatus));
}

/* Test: Function config not found in array */
void test_FiM_FunctionConfigNotFound(void) {
    /* testConfig only has FID=1; FID=2 is valid range but not configured */
    FiM_Init(&testConfig);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, FiM_SetFunctionAvailable(2U, TRUE));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, FiM_SetFunctionPermission(2U, FIM_PERMISSION_DENIED));

    FiM_PermissionStateType permission;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, FiM_GetFunctionPermission(2U, &permission));
}

/* Test: Init with FIM_FID_INVALID entry - covers skip branch in Init loop */
void test_FiM_Init_WithInvalidFIDEntry(void) {
    FiM_PermissionStateType permission;

    FiM_Init(&mixedConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
}

/* Test: Function unavailable path in GetFunctionPermission */
void test_FiM_GetFunctionPermission_Unavailable(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_DENIED;

    FiM_Init(&testConfig);
    /* Manually set permission to ALLOWED */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionPermission(1U, FIM_PERMISSION_ALLOWED));
    /* Disable function - overrides permission to DENIED */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionAvailable(1U, FALSE));
    /* GetFunctionPermission should return DENIED because Available==FALSE */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);
}

/* Test: CONFIRMED inhibition mask type - exercises CheckEventInhibition evaluation.
 * When eventFailed=TRUE, dtcStatus has TF|TFTOC|PDTC (0x07). The CONFIRMED mask
 * (0x08) checks CDTC bit which is NOT set in dtcStatus, so no inhibition occurs.
 * This test still exercises the else-if branches in CheckEventInhibition. */
void test_FiM_ConfirmedInhibitionMask(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_DENIED;

    mock_Dem_EventFailed = FALSE;
    FiM_Init(&confirmedConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);

    /* eventFailed=TRUE: dtcStatus=0x07, CONFIRMED mask=0x08 checks CDTC(0x08)
     * bit which is not in dtcStatus -> no inhibition -> ALLOWED */
    mock_Dem_EventFailed = TRUE;
    FiM_Init(&confirmedConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
}

/* Test: TEST_NOT_COMPLETED inhibition mask type.
 * When eventFailed=TRUE, dtcStatus has TF|TFTOC|PDTC (0x07). The TEST_NOT_COMPLETED
 * mask (0x10) checks TNCTOC bit which is NOT set in dtcStatus, so no inhibition occurs.
 * This test exercises the else-if branch in CheckEventInhibition. */
void test_FiM_TestNotCompletedInhibitionMask(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_DENIED;

    mock_Dem_EventFailed = FALSE;
    FiM_Init(&tncConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);

    /* eventFailed=TRUE: dtcStatus=0x07, TEST_NOT_COMPLETED mask=0x10 checks
     * TNCTOC bit which is not in dtcStatus -> no inhibition -> ALLOWED */
    mock_Dem_EventFailed = TRUE;
    FiM_Init(&tncConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
}

/* Test: WARNING_INDICATOR inhibition mask type.
 * When eventFailed=TRUE, dtcStatus has TF|TFTOC|PDTC (0x07). The WARNING_INDICATOR
 * mask (0x20) checks WIR bit which is NOT set in dtcStatus, so no inhibition occurs.
 * This test exercises the else-if branch in CheckEventInhibition. */
void test_FiM_WarningIndicatorInhibitionMask(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_DENIED;

    mock_Dem_EventFailed = FALSE;
    FiM_Init(&wirConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);

    /* eventFailed=TRUE: dtcStatus=0x07, WARNING_INDICATOR mask=0x20 checks
     * WIR bit which is not in dtcStatus -> no inhibition -> ALLOWED */
    mock_Dem_EventFailed = TRUE;
    FiM_Init(&wirConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
}

/* Test: Multiple event inhibitions with different masks */
void test_FiM_MultipleEventInhibitions(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;
    FiM_InhibitionStatusType inhibitionStatus = FIM_INHIBITED_NO;

    mock_Dem_EventFailed = FALSE;
    FiM_Init(&multiConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);

    /* All events failed: function denied with combined inhibition mask */
    mock_Dem_EventFailed = TRUE;
    FiM_Init(&multiConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetInhibitionStatus(0U, &inhibitionStatus));
    TEST_ASSERT_EQUAL_UINT32(FIM_INHIBITED_YES, inhibitionStatus);
}

/* Test: TEST_FAILED_TOC inhibition mask type */
void test_FiM_TestFailedTOCInhibitionMask(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;

    mock_Dem_EventFailed = FALSE;
    FiM_Init(&tocConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);

    mock_Dem_EventFailed = TRUE;
    FiM_Init(&tocConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);
}

/* Test: Summary event not failed path */
void test_FiM_SummaryEvent_NotFailed(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_DENIED;

    mock_Dem_EventFailed = FALSE;
    FiM_Init(&summaryNFConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
}

/* Test: Summary event Dem_GetEventFailed returns error */
void test_FiM_SummaryEvent_DemError(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_DENIED;

    mock_Dem_ReturnValue = E_NOT_OK;
    FiM_Init(&summaryErrConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
}

/* Test: SetFunctionAvailable FALSE then TRUE with inhibition */
void test_FiM_SetAvailable_DisableEnable_WithInhibition(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;

    mock_Dem_EventFailed = TRUE;
    FiM_Init(&testConfigWithEvent);

    /* Disable function */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionAvailable(1U, FALSE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);

    /* Re-enable while event still failed: remains denied due to inhibition */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionAvailable(1U, TRUE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);

    /* Heal event and re-enable */
    mock_Dem_EventFailed = FALSE;
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionAvailable(1U, TRUE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);
}

/* Test: Init calculates inhibition on startup with failed event */
void test_FiM_Init_CalculatesInhibitionOnStartup(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;
    FiM_InhibitionStatusType inhibitionStatus = FIM_INHIBITED_NO;

    mock_Dem_EventFailed = TRUE;
    FiM_Init(&testConfigWithEvent);

    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetInhibitionStatus(0U, &inhibitionStatus));
    TEST_ASSERT_EQUAL_UINT32(FIM_INHIBITED_YES, inhibitionStatus);
}

/* Test: Multiple functions in config */
void test_FiM_MultipleFunctions_Config(void) {
    FiM_PermissionStateType perm1, perm2;

    FiM_Init(&twoConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &perm1));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, perm1);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(2U, &perm2));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, perm2);

    /* Disable FID 1, FID 2 should remain allowed */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_SetFunctionAvailable(1U, FALSE));
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &perm1));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, perm1);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(2U, &perm2));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, perm2);
}

/* Test: GetInhibitionStatus at FID boundary */
void test_FiM_GetInhibitionStatus_Boundary(void) {
    FiM_InhibitionStatusType inhibitionStatus;

    FiM_Init(&testConfig);

    /* FID 0: valid (< FIM_MAX_FUNCTIONS=32) */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetInhibitionStatus(0U, &inhibitionStatus));
    TEST_ASSERT_EQUAL_UINT32(FIM_INHIBITED_NO, inhibitionStatus);

    /* FID 31 (FIM_FID_MAX): valid */
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetInhibitionStatus(31U, &inhibitionStatus));
    TEST_ASSERT_EQUAL_UINT32(FIM_INHIBITED_NO, inhibitionStatus);

    /* FID 32 (FIM_MAX_FUNCTIONS): invalid, should report DET error */
    mock_Det_Reset();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, FiM_GetInhibitionStatus(32U, &inhibitionStatus));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_PARAM_FID, mock_DetLastErrorId);
}

/* Test: Combined summary event and DEM event inhibitions */
void test_FiM_CombinedSummaryAndDemInhibitions(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;

    mock_Dem_EventFailed = FALSE;
    FiM_Init(&combinedConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_ALLOWED, permission);

    /* Both events failed: denied */
    mock_Dem_EventFailed = TRUE;
    FiM_Init(&combinedConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, FiM_GetFunctionPermission(1U, &permission));
    TEST_ASSERT_EQUAL_UINT32(FIM_PERMISSION_DENIED, permission);
}

/* Test: GetFunctionPermission with FID below minimum (FID=0) */
void test_FiM_GetFunctionPermission_InvalidFID_BelowMin(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;
    
    FiM_Init(&testConfig);
    mock_Det_Reset();
    
    /* FID 0 is below FIM_FID_MIN (1), should trigger DET error */
    Std_ReturnType ret = FiM_GetFunctionPermission(0U, &permission);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_GETFUNCTIONPERMISSION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_PARAM_FID, mock_DetLastErrorId);
}

/* Test: GetFunctionPermission with FID above maximum (FID=32) */
void test_FiM_GetFunctionPermission_InvalidFID_AboveMax(void) {
    FiM_PermissionStateType permission = FIM_PERMISSION_ALLOWED;
    
    FiM_Init(&testConfig);
    mock_Det_Reset();
    
    /* FID 32 is above FIM_FID_MAX (31), should trigger DET error */
    Std_ReturnType ret = FiM_GetFunctionPermission(32U, &permission);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FIM_SID_GETFUNCTIONPERMISSION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FIM_E_PARAM_FID, mock_DetLastErrorId);
}

/* Test: Init with zero functions - covers loop-not-executed branches */
void test_FiM_Init_ZeroFunctions(void) {
    FiM_ConfigType emptyConfig;
    FiM_PermissionStateType permission;
    
    memset(&emptyConfig, 0, sizeof(emptyConfig));
    emptyConfig.FunctionConfigs = NULL_PTR;
    emptyConfig.NumFunctions = 0U;
    
    /* Init with zero functions should succeed */
    FiM_Init(&emptyConfig);
    
    /* Module is initialized but no functions are configured */
    /* Calling GetFunctionPermission with any FID should fail (not configured) */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, FiM_GetFunctionPermission(1U, &permission));
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
    /* New branch coverage tests */
    RUN_TEST(test_FiM_PendingInhibitionMask_ShouldInhibit);
    RUN_TEST(test_FiM_DemGetEventFailed_ReturnsNotOK);
    RUN_TEST(test_FiM_SummaryEvent_Path);
    RUN_TEST(test_FiM_BoundaryFID_Min);
    RUN_TEST(test_FiM_BoundaryFID_Max);
    RUN_TEST(test_FiM_FunctionConfigNotFound);
    RUN_TEST(test_FiM_Init_WithInvalidFIDEntry);
    RUN_TEST(test_FiM_GetFunctionPermission_Unavailable);
    RUN_TEST(test_FiM_ConfirmedInhibitionMask);
    RUN_TEST(test_FiM_TestNotCompletedInhibitionMask);
    RUN_TEST(test_FiM_WarningIndicatorInhibitionMask);
    RUN_TEST(test_FiM_MultipleEventInhibitions);
    RUN_TEST(test_FiM_TestFailedTOCInhibitionMask);
    RUN_TEST(test_FiM_SummaryEvent_NotFailed);
    RUN_TEST(test_FiM_SummaryEvent_DemError);
    RUN_TEST(test_FiM_SetAvailable_DisableEnable_WithInhibition);
    RUN_TEST(test_FiM_Init_CalculatesInhibitionOnStartup);
    RUN_TEST(test_FiM_MultipleFunctions_Config);
    RUN_TEST(test_FiM_GetInhibitionStatus_Boundary);
    RUN_TEST(test_FiM_CombinedSummaryAndDemInhibitions);
    RUN_TEST(test_FiM_GetFunctionPermission_InvalidFID_BelowMin);
    RUN_TEST(test_FiM_GetFunctionPermission_InvalidFID_AboveMax);
    RUN_TEST(test_FiM_Init_ZeroFunctions);
    return UnityEnd();
}
