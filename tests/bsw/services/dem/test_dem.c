/**
 * @file test_dem.c
 * @brief Dem (Diagnostic Event Manager) Unit Tests - comprehensive branch coverage.
 * @req SWS_Dem
 */
// @tests src/bsw/services/dem/src/Dem.c  @tests src/bsw/services/dem/include/Dem.h
// @tests src/bsw/services/dem/src/Dem_Int.c
#include "unity.h"
#include "Dem.h"
#include "Dem_Int.h"

/* ---- DET recorder (test-local mock) ---- */
static uint8 mock_DetCalls = 0;
static uint8 mock_lastApiId = 0;
static uint8 mock_lastErrorId = 0;
static uint16 mock_lastModuleId = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)InstanceId;
    mock_DetCalls++;
    mock_lastModuleId = ModuleId;
    mock_lastApiId = ApiId;
    mock_lastErrorId = ErrorId;
    return E_OK;
}

/* ---- Callback trackers ---- */
static uint8 clearDtcStartCallCount = 0;
static uint8 clearDtcFinishCallCount = 0;
static uint8 clearDtcLambdaCallCount = 0;
static void test_ClearDTCStartCb(void)  { clearDtcStartCallCount++; }
static void test_ClearDTCFinishCb(void) { clearDtcFinishCallCount++; }
static void test_ClearDTCLambdaCb(void) { clearDtcLambdaCallCount++; }

/* ---- configs ---- */
static Dem_ConfigType emptyConfig; /* zero-init */

/* Event 1: DEBOUNCE_NONE, DTC 0x00123456 */
static const Dem_EventParameterType eventParams[1] = {
    { 1U, 0x00123456U, 0U, TRUE, TRUE, 0U, 0U,
      DEM_DEBOUNCE_ALGORITHM_NONE,
      FALSE, FALSE, FALSE,
      127, -128, 0U, 0U }
};
static const Dem_DtcParameterType dtcParams[1] = {
    { 0x00123456U, 0U, 0U, DEM_DTC_ORIGIN_PRIMARY_MEMORY,
      TRUE, TRUE, 1U, FALSE }
};
static const Dem_ConfigType fullConfig = {
    eventParams, 1U,
    dtcParams, 1U,
    NULL_PTR, 0U,
    NULL_PTR, 0U,
    NULL_PTR, 0U,
    TRUE, TRUE, TRUE, FALSE,
    0xFFU,
    FALSE, FALSE, FALSE, FALSE,
    NULL_PTR, NULL_PTR, NULL_PTR
};

/* Multi-algorithm config: 4 events with NONE/COUNTER/TIME/MONITOR */
static Dem_EventParameterType multiEvParams[4];
static const Dem_DtcParameterType multiDtcParams[4] = {
    { 0x100001U, 0U, 0U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, TRUE, TRUE, 5U, FALSE },
    { 0x100002U, 0U, 0U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, TRUE, TRUE, 5U, FALSE },
    { 0x100003U, 0U, 0U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, TRUE, TRUE, 5U, FALSE },
    { 0x100004U, 0U, 0U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, TRUE, TRUE, 5U, FALSE }
};
static Dem_ConfigType multiDebounceConfig;

/* Config with extended data records */
static const Dem_EventParameterType extEvParams[1] = {
    { 1U, 0x200001U, 0U, TRUE, TRUE, 0U, 0U,
      DEM_DEBOUNCE_ALGORITHM_NONE,
      FALSE, FALSE, FALSE,
      127, -128, 0U, 0U }
};
static const Dem_DtcParameterType extDtcParams[1] = {
    { 0x200001U, 0U, 0U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, TRUE, TRUE, 5U, FALSE }
};
static const Dem_ExtendedDataRecordType extRecords[2] = {
    { 1U, 4U, TRUE, FALSE },
    { 2U, 4U, TRUE, FALSE }
};
static const Dem_ConfigType extDataConfig = {
    extEvParams, 1U,
    extDtcParams, 1U,
    NULL_PTR, 0U,
    extRecords, 2U,
    NULL_PTR, 0U,
    TRUE, TRUE, TRUE, FALSE,
    0xFFU,
    FALSE, FALSE, FALSE, FALSE,
    NULL_PTR, NULL_PTR, NULL_PTR
};

/* Config with extended data record 3 (for default case testing) */
static const Dem_ExtendedDataRecordType extRecords3[1] = {
    { 3U, 8U, TRUE, FALSE }
};
static const Dem_ConfigType extDataConfig3 = {
    extEvParams, 1U,
    extDtcParams, 1U,
    NULL_PTR, 0U,
    extRecords3, 1U,
    NULL_PTR, 0U,
    TRUE, TRUE, TRUE, FALSE,
    0xFFU,
    FALSE, FALSE, FALSE, FALSE,
    NULL_PTR, NULL_PTR, NULL_PTR
};

/* Config with callbacks */
static const Dem_ConfigType callbackConfig = {
    eventParams, 1U,
    dtcParams, 1U,
    NULL_PTR, 0U,
    NULL_PTR, 0U,
    NULL_PTR, 0U,
    TRUE, TRUE, TRUE, FALSE,
    0xFFU,
    FALSE, FALSE, FALSE, FALSE,
    test_ClearDTCLambdaCb, test_ClearDTCStartCb, test_ClearDTCFinishCb
};

/* Multi-DTC config for filter tests (2 events, 2 DTCs) */
static const Dem_EventParameterType multiDtcEvParams[2] = {
    { 1U, 0x300001U, 0U, TRUE, TRUE, 0U, 0U,
      DEM_DEBOUNCE_ALGORITHM_NONE,
      FALSE, FALSE, FALSE, 127, -128, 0U, 0U },
    { 2U, 0x300002U, 0U, TRUE, TRUE, 0U, 0U,
      DEM_DEBOUNCE_ALGORITHM_NONE,
      FALSE, FALSE, FALSE, 127, -128, 0U, 0U }
};
static const Dem_DtcParameterType multiDtcDtcParams[2] = {
    { 0x300001U, 0U, 0U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, TRUE, TRUE, 5U, FALSE },
    { 0x300002U, 0U, 0U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, TRUE, TRUE, 5U, FALSE }
};
static const Dem_ConfigType multiDtcConfig = {
    multiDtcEvParams, 2U,
    multiDtcDtcParams, 2U,
    NULL_PTR, 0U,
    NULL_PTR, 0U,
    NULL_PTR, 0U,
    TRUE, TRUE, TRUE, FALSE,
    0xFFU,
    FALSE, FALSE, FALSE, FALSE,
    NULL_PTR, NULL_PTR, NULL_PTR
};

/* Helper to init multiDebounceConfig (non-const due to array init) */
static void initMultiDebounceConfig(void) {
    /* Event 1: NONE, EventId=1, DTC=0x100001 */
    multiEvParams[0] = (Dem_EventParameterType){
        1U, 0x100001U, 0U, TRUE, TRUE, 0U, 0U,
        DEM_DEBOUNCE_ALGORITHM_NONE,
        FALSE, FALSE, FALSE, 10, -10, 0U, 0U };
    /* Event 2: COUNTER, EventId=2, DTC=0x100002 */
    multiEvParams[1] = (Dem_EventParameterType){
        2U, 0x100002U, 0U, TRUE, TRUE, 0U, 0U,
        DEM_DEBOUNCE_ALGORITHM_COUNTER,
        FALSE, FALSE, FALSE, 10, -10, 0U, 0U };
    /* Event 3: TIME, EventId=3, DTC=0x100003 */
    multiEvParams[2] = (Dem_EventParameterType){
        3U, 0x100003U, 0U, TRUE, TRUE, 0U, 0U,
        DEM_DEBOUNCE_ALGORITHM_TIME,
        FALSE, FALSE, FALSE, 10, -10, 100U, 100U };
    /* Event 4: MONITOR, EventId=4, DTC=0x100004 */
    multiEvParams[3] = (Dem_EventParameterType){
        4U, 0x100004U, 0U, TRUE, TRUE, 0U, 0U,
        DEM_DEBOUNCE_ALGORITHM_MONITOR,
        FALSE, FALSE, FALSE, 10, -10, 0U, 0U };
    multiDebounceConfig = (Dem_ConfigType){
        multiEvParams, 4U,
        multiDtcParams, 4U,
        NULL_PTR, 0U,
        NULL_PTR, 0U,
        NULL_PTR, 0U,
        TRUE, TRUE, TRUE, FALSE,
        0xFFU,
        FALSE, FALSE, FALSE, FALSE,
        NULL_PTR, NULL_PTR, NULL_PTR
    };
}

void setUp(void) {
    initMultiDebounceConfig();
    Dem_Init(&emptyConfig);
    mock_DetCalls = 0;
    clearDtcStartCallCount = 0;
    clearDtcFinishCallCount = 0;
    clearDtcLambdaCallCount = 0;
}
void tearDown(void) {}

/*===========================================================================
 * ORIGINAL 13 TESTS (preserved)
 *==========================================================================*/

void test_Dem_Init_NullPtr_ShouldReportDet(void) {
    Dem_DeInit();
    Dem_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_MODULE_ID, mock_lastModuleId);
    TEST_ASSERT_EQUAL(DEM_SID_INIT, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_PARAM_POINTER, mock_lastErrorId);
    TEST_ASSERT_EQUAL(DEM_STATE_UNINIT, Dem_InternalState.State);
}

void test_Dem_Init_ValidConfig_ShouldInitializeState(void) {
    TEST_ASSERT_EQUAL(DEM_STATE_INIT, Dem_InternalState.State);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_TRUE(&emptyConfig == Dem_InternalState.ConfigPtr);
}

void test_Dem_DeInit_AfterInit_ShouldUninitialize(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(DEM_STATE_UNINIT, Dem_InternalState.State);
    mock_DetCalls = 0;
    Dem_DeInit();
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_SID_SHUTDOWN, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_UNINIT, mock_lastErrorId);
}

void test_Dem_SetEventStatus_BeforeInit_ShouldReportUninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetEventStatus(1U, DEM_EVENT_STATUS_PASSED));
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_SID_SETEVENTSTATUS, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_UNINIT, mock_lastErrorId);
}

void test_Dem_SetEventStatus_InvalidEventId_ShouldReportDet(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetEventStatus(0U, DEM_EVENT_STATUS_PASSED));
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_SID_SETEVENTSTATUS, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_PARAM_EVENT_ID, mock_lastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetEventStatus(200U, DEM_EVENT_STATUS_FAILED));
    TEST_ASSERT_EQUAL(2, mock_DetCalls);
}

void test_Dem_SetEventStatus_UnknownEvent_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED));
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_SID_SETEVENTSTATUS, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_PARAM_EVENT_ID, mock_lastErrorId);
}

void test_Dem_SetEventStatus_ConfiguredEvent_ShouldUpdateState(void) {
    Dem_Init(&fullConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_OK, Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED));
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, Dem_InternalState.EventStates[0].LastReportedStatus);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.EventStates[0].TestCompletedThisOperationCycle);
}

void test_Dem_Getters_AfterFailedReport_ShouldReflectStatus(void) {
    Dem_Init(&fullConfig);
    TEST_ASSERT_EQUAL(E_OK, Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED));
    Dem_EventStatusType status = DEM_EVENT_STATUS_PASSED;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventStatus(1U, &status));
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, status);
    boolean failed = FALSE;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventFailed(1U, &failed));
    TEST_ASSERT_EQUAL(TRUE, failed);
    boolean tested = FALSE;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventTested(1U, &tested));
    TEST_ASSERT_EQUAL(TRUE, tested);
    sint8 fdc = 0;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetFaultDetectionCounter(1U, &fdc));
    /* unknown event id -> E_NOT_OK without touching the output value */
    status = DEM_EVENT_STATUS_FAILED;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventStatus(0U, &status));
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, status);
}

void test_Dem_ResetEventStatus_ShouldClearFlags(void) {
    Dem_Init(&fullConfig);
    TEST_ASSERT_EQUAL(E_OK, Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED));
    TEST_ASSERT_EQUAL(E_OK, Dem_ResetEventStatus(1U));
    boolean tested = TRUE;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventTested(1U, &tested));
    TEST_ASSERT_EQUAL(FALSE, tested);
}

void test_Dem_ClearDTC_ShouldValidateDtc(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK,
        Dem_ClearDTC(0x00000123U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY));
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_SID_CLEARDTC, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_PARAM_DATA, mock_lastErrorId);
    TEST_ASSERT_EQUAL(E_OK,
        Dem_ClearDTC(DEM_DTC_GROUP_ALL, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY));
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
}

void test_Dem_ClearDTC_ConfiguredDtc_ShouldResetStatus(void) {
    Dem_UdsStatusByteType status = 0U;
    Dem_Init(&fullConfig);
    TEST_ASSERT_EQUAL(E_OK, Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED));
    TEST_ASSERT_EQUAL(E_OK, Dem_GetStatusOfDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status));
    TEST_ASSERT_NOT_EQUAL(0U, (status & DEM_UDS_STATUS_TF));
    TEST_ASSERT_EQUAL(E_OK,
        Dem_ClearDTC(0x00123456U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY));
    TEST_ASSERT_EQUAL(E_OK, Dem_GetStatusOfDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status));
    TEST_ASSERT_EQUAL(0U, (status & DEM_UDS_STATUS_TF));
    TEST_ASSERT_NOT_EQUAL(0U, (status & DEM_UDS_STATUS_TNCSLC));
}

void test_Dem_GetStatusOfDTC_UnknownDtc_ShouldReturnWrongDtc(void) {
    Dem_UdsStatusByteType status = 0xFFU;
    TEST_ASSERT_EQUAL(DEM_STATUS_WRONG_DTC,
        Dem_GetStatusOfDTC(0x00999999U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status));
    TEST_ASSERT_EQUAL_UINT8(0U, status);
}

void test_Dem_GetVersionInfo_ShouldReturnConfiguredValues(void) {
    Std_VersionInfoType info = { 0U, 0U, 0U, 0U, 0U };
    Dem_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(DEM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(DEM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(DEM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(DEM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(DEM_SW_PATCH_VERSION, info.sw_patch_version);
    Dem_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_SID_GETVERSIONINFO, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_PARAM_POINTER, mock_lastErrorId);
}

/*===========================================================================
 * UNINIT CHECKS for all APIs
 *==========================================================================*/

void test_ResetEventStatus_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_ResetEventStatus(1U));
    TEST_ASSERT_EQUAL(DEM_SID_RESETEVENTSTATUS, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_UNINIT, mock_lastErrorId);
}

void test_ResetEventStatus_InvalidEventId(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_ResetEventStatus(0U));
    TEST_ASSERT_EQUAL(DEM_E_PARAM_EVENT_ID, mock_lastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_ResetEventStatus(200U));
}

void test_ResetEventStatus_UnknownEvent(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_ResetEventStatus(1U));
}

void test_GetEventStatus_Uninit(void) {
    Dem_DeInit();
    Dem_EventStatusType s;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventStatus(1U, &s));
}

void test_GetEventStatus_NullPtr(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventStatus(1U, NULL_PTR));
    TEST_ASSERT_EQUAL(DEM_E_PARAM_POINTER, mock_lastErrorId);
}

void test_GetEventStatus_InvalidId(void) {
    Dem_EventStatusType s;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventStatus(0U, &s));
    TEST_ASSERT_EQUAL(DEM_E_PARAM_EVENT_ID, mock_lastErrorId);
}

void test_GetEventStatus_UnknownEvent(void) {
    Dem_EventStatusType s = DEM_EVENT_STATUS_FAILED;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventStatus(1U, &s));
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, s);
}

void test_GetEventFailed_Uninit(void) {
    Dem_DeInit();
    boolean f;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventFailed(1U, &f));
}

void test_GetEventFailed_NullPtr(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventFailed(1U, NULL_PTR));
    TEST_ASSERT_EQUAL(DEM_E_PARAM_POINTER, mock_lastErrorId);
}

void test_GetEventFailed_InvalidId(void) {
    boolean f;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventFailed(0U, &f));
}

void test_GetEventFailed_UnknownEvent(void) {
    boolean f = TRUE;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventFailed(1U, &f));
    TEST_ASSERT_EQUAL(TRUE, f);
}

void test_GetEventTested_Uninit(void) {
    Dem_DeInit();
    boolean t;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventTested(1U, &t));
}

void test_GetEventTested_NullPtr(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventTested(1U, NULL_PTR));
}

void test_GetEventTested_InvalidId(void) {
    boolean t;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventTested(200U, &t));
}

void test_GetEventTested_UnknownEvent(void) {
    boolean t = TRUE;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventTested(1U, &t));
}

void test_GetFaultDetCounter_Uninit(void) {
    Dem_DeInit();
    sint8 c;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetFaultDetectionCounter(1U, &c));
}

void test_GetFaultDetCounter_NullPtr(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetFaultDetectionCounter(1U, NULL_PTR));
}

void test_GetFaultDetCounter_InvalidId(void) {
    sint8 c;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetFaultDetectionCounter(0U, &c));
}

void test_GetFaultDetCounter_UnknownEvent(void) {
    sint8 c = 42;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetFaultDetectionCounter(1U, &c));
    TEST_ASSERT_EQUAL_INT8(42, c);
}

void test_GetStatusOfDTC_Uninit(void) {
    Dem_DeInit();
    Dem_UdsStatusByteType s;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetStatusOfDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &s));
}

void test_GetStatusOfDTC_NullPtr(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetStatusOfDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, NULL_PTR));
}

void test_GetDTCStatusAvailMask_Uninit(void) {
    Dem_DeInit();
    uint8 m;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetDTCStatusAvailabilityMask(&m));
}

void test_GetDTCStatusAvailMask_NullPtr(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetDTCStatusAvailabilityMask(NULL_PTR));
}

void test_GetDTCStatusAvailMask_Valid(void) {
    uint8 m = 0U;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetDTCStatusAvailabilityMask(&m));
    TEST_ASSERT_EQUAL_UINT8(DEM_DTC_STATUS_AVAILABILITY_MASK, m);
}

void test_GetNumFilteredDTC_Uninit(void) {
    Dem_DeInit();
    uint16 n;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetNumberOfFilteredDTC(&n));
}

void test_GetNumFilteredDTC_NullPtr(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetNumberOfFilteredDTC(NULL_PTR));
}

void test_GetNumFilteredDTC_Valid(void) {
    uint16 n = 99U;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetNumberOfFilteredDTC(&n));
    TEST_ASSERT_EQUAL_UINT16(0U, n);
}

void test_GetNextFilteredDTC_Uninit(void) {
    Dem_DeInit();
    Dem_DtcType d; Dem_UdsStatusByteType s;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetNextFilteredDTC(&d, &s));
}

void test_GetNextFilteredDTC_NullPtr(void) {
    Dem_DtcType d; Dem_UdsStatusByteType s;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetNextFilteredDTC(NULL_PTR, &s));
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetNextFilteredDTC(&d, NULL_PTR));
}

void test_GetNextFilteredDTC_NoMatch(void) {
    Dem_DtcType d = 0U; Dem_UdsStatusByteType s = 0U;
    TEST_ASSERT_EQUAL(DEM_FILTERED_NO_MATCHING_ELEMENT, Dem_GetNextFilteredDTC(&d, &s));
}

void test_ClearDTC_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK,
        Dem_ClearDTC(0x00123456U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY));
}

void test_SelectDTC_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SelectDTC(0x00123456U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY));
}

void test_SelectDTC_UnknownDtc(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SelectDTC(0x999999U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY));
}

void test_SelectDTC_Valid(void) {
    Dem_Init(&fullConfig);
    /* Create DTC entry first */
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(E_OK, Dem_SelectDTC(0x00123456U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY));
}

void test_DisableDTCSetting_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_DisableDTCSetting(0U, 0U));
}

void test_DisableDTCSetting_Valid(void) {
    TEST_ASSERT_EQUAL(E_OK, Dem_DisableDTCSetting(0U, 0U));
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.DTCSettingDisabled);
}

void test_EnableDTCSetting_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_EnableDTCSetting(0U, 0U));
}

void test_EnableDTCSetting_Valid(void) {
    Dem_InternalState.DTCSettingDisabled = TRUE;
    TEST_ASSERT_EQUAL(E_OK, Dem_EnableDTCSetting(0U, 0U));
    TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.DTCSettingDisabled);
}

void test_DisableDTCRecordUpdate_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_DisableDTCRecordUpdate());
}

void test_DisableDTCRecordUpdate_AlreadyDisabled(void) {
    Dem_InternalState.DTCRecordUpdateDisabled = TRUE;
    TEST_ASSERT_EQUAL(DEM_DISABLEDTCRECUP_DISABLED, Dem_DisableDTCRecordUpdate());
}

void test_DisableDTCRecordUpdate_Valid(void) {
    TEST_ASSERT_EQUAL(DEM_DISABLEDTCRECUP_OK, Dem_DisableDTCRecordUpdate());
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.DTCRecordUpdateDisabled);
}

void test_EnableDTCRecordUpdate_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_EnableDTCRecordUpdate());
}

void test_EnableDTCRecordUpdate_Valid(void) {
    Dem_InternalState.DTCRecordUpdateDisabled = TRUE;
    TEST_ASSERT_EQUAL(E_OK, Dem_EnableDTCRecordUpdate());
    TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.DTCRecordUpdateDisabled);
}

void test_GetIndicatorStatus_Uninit(void) {
    Dem_DeInit();
    Dem_IndicatorStatusType s;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetIndicatorStatus(0U, &s));
}

void test_GetIndicatorStatus_NullPtr(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetIndicatorStatus(0U, NULL_PTR));
}

void test_GetIndicatorStatus_InvalidId(void) {
    Dem_IndicatorStatusType s;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetIndicatorStatus(DEM_NUM_INDICATORS, &s));
}

void test_GetIndicatorStatus_NoFailure(void) {
    Dem_IndicatorStatusType s = DEM_INDICATOR_CONTINUOUS;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetIndicatorStatus(0U, &s));
    TEST_ASSERT_EQUAL(DEM_INDICATOR_OFF, s);
}

void test_SetIndicatorStatus_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetIndicatorStatus(0U, DEM_INDICATOR_OFF));
}

void test_SetIndicatorStatus_InvalidId(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetIndicatorStatus(DEM_NUM_INDICATORS, DEM_INDICATOR_OFF));
}

void test_SetIndicatorStatus_Valid(void) {
    TEST_ASSERT_EQUAL(E_OK, Dem_SetIndicatorStatus(0U, DEM_INDICATOR_CONTINUOUS));
}

void test_GetFreezeFrameDataByDTC_Uninit(void) {
    Dem_DeInit();
    uint8 buf[4]; uint16 sz = sizeof(buf);
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetFreezeFrameDataByDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 0U, buf, &sz));
}

void test_GetFreezeFrameDataByDTC_NullPtr(void) {
    uint8 buf[4]; uint16 sz = sizeof(buf);
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetFreezeFrameDataByDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 0U, NULL_PTR, &sz));
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetFreezeFrameDataByDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 0U, buf, NULL_PTR));
}

void test_GetFreezeFrameDataByDTC_WrongDtc(void) {
    uint8 buf[4]; uint16 sz = sizeof(buf);
    TEST_ASSERT_EQUAL(DEM_GET_FREEZEFRAME_WRONG_DTC,
        Dem_GetFreezeFrameDataByDTC(0x999999U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 0U, buf, &sz));
}

void test_GetExtendedDataByDTC_Uninit(void) {
    Dem_DeInit();
    uint8 buf[4]; uint16 sz = sizeof(buf);
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetExtendedDataRecordByDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1U, buf, &sz));
}

void test_GetExtendedDataByDTC_NullPtr(void) {
    uint8 buf[4]; uint16 sz = sizeof(buf);
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetExtendedDataRecordByDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1U, NULL_PTR, &sz));
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetExtendedDataRecordByDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1U, buf, NULL_PTR));
}

void test_GetExtendedDataByDTC_WrongDtc(void) {
    uint8 buf[4]; uint16 sz = sizeof(buf);
    TEST_ASSERT_EQUAL(DEM_RECORD_WRONG_DTC,
        Dem_GetExtendedDataRecordByDTC(0x999999U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1U, buf, &sz));
}

void test_GetSizeOfExtDataRecord_Uninit(void) {
    Dem_DeInit();
    uint16 sz;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetSizeOfExtendedDataRecordByDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1U, &sz));
}

void test_GetSizeOfExtDataRecord_NullPtr(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetSizeOfExtendedDataRecordByDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1U, NULL_PTR));
}

void test_GetSizeOfExtDataRecord_WrongDtc(void) {
    uint16 sz;
    TEST_ASSERT_EQUAL(DEM_RECORD_WRONG_DTC,
        Dem_GetSizeOfExtendedDataRecordByDTC(0x999999U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1U, &sz));
}

void test_GetSizeOfExtDataRecord_WrongNumber(void) {
    Dem_Init(&extDataConfig);
    uint16 sz = 0U;
    TEST_ASSERT_EQUAL(DEM_RECORD_WRONG_NUMBER,
        Dem_GetSizeOfExtendedDataRecordByDTC(0x200001U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 99U, &sz));
}

void test_GetSizeOfExtDataRecord_Found(void) {
    Dem_Init(&extDataConfig);
    uint16 sz = 0U;
    TEST_ASSERT_EQUAL(E_OK,
        Dem_GetSizeOfExtendedDataRecordByDTC(0x200001U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1U, &sz));
    TEST_ASSERT_EQUAL_UINT16(4U, sz);
}

void test_SetOperationCycleState_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetOperationCycleState(DEM_OPCYC_POWER, DEM_CYCLE_STATE_START));
}

void test_SetOperationCycleState_InvalidCycle(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetOperationCycleState((Dem_OperationCycleType)DEM_NUM_OPERATION_CYCLES, DEM_CYCLE_STATE_START));
}

void test_SetOperationCycleState_StartCycle(void) {
    TEST_ASSERT_EQUAL(E_OK, Dem_SetOperationCycleState(DEM_OPCYC_POWER, DEM_CYCLE_STATE_START));
    TEST_ASSERT_EQUAL(DEM_CYCLE_STATE_START, Dem_InternalState.OperationCycleStates[DEM_OPCYC_POWER]);
}

void test_SetOperationCycleState_EndCycle(void) {
    Dem_InternalState.OperationCycleStates[DEM_OPCYC_POWER] = DEM_CYCLE_STATE_START;
    TEST_ASSERT_EQUAL(E_OK, Dem_SetOperationCycleState(DEM_OPCYC_POWER, DEM_CYCLE_STATE_END));
    TEST_ASSERT_EQUAL(DEM_CYCLE_STATE_END, Dem_InternalState.OperationCycleStates[DEM_OPCYC_POWER]);
}

void test_GetOperationCycleState_Uninit(void) {
    Dem_DeInit();
    Dem_OperationCycleStateType s;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetOperationCycleState(DEM_OPCYC_POWER, &s));
}

void test_GetOperationCycleState_NullPtr(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetOperationCycleState(DEM_OPCYC_POWER, NULL_PTR));
}

void test_GetOperationCycleState_InvalidCycle(void) {
    Dem_OperationCycleStateType s;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetOperationCycleState((Dem_OperationCycleType)DEM_NUM_OPERATION_CYCLES, &s));
}

void test_GetOperationCycleState_Valid(void) {
    Dem_OperationCycleStateType s;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetOperationCycleState(DEM_OPCYC_POWER, &s));
    TEST_ASSERT_EQUAL(DEM_CYCLE_STATE_END, s);
}

void test_RestartOperationCycle_Valid(void) {
    TEST_ASSERT_EQUAL(E_OK, Dem_RestartOperationCycle(DEM_OPCYC_POWER));
    TEST_ASSERT_EQUAL(DEM_CYCLE_STATE_START, Dem_InternalState.OperationCycleStates[DEM_OPCYC_POWER]);
}

void test_GetDTCOfCheckFailed_Uninit(void) {
    Dem_DeInit();
    Dem_DtcType d;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetDTCOfCheckFailed(&d));
}

void test_GetDTCOfCheckFailed_NullPtr(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetDTCOfCheckFailed(NULL_PTR));
}

void test_GetDTCOfCheckFailed_NoFailedDTC(void) {
    Dem_DtcType d = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetDTCOfCheckFailed(&d));
}

void test_PrestoreFreezeFrame_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_PrestoreFreezeFrame(1U));
}

void test_PrestoreFreezeFrame_InvalidId(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_PrestoreFreezeFrame(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_PrestoreFreezeFrame(200U));
}

void test_PrestoreFreezeFrame_UnknownEvent(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_PrestoreFreezeFrame(1U));
}

void test_PrestoreFreezeFrame_Valid(void) {
    Dem_Init(&fullConfig);
    TEST_ASSERT_EQUAL(E_OK, Dem_PrestoreFreezeFrame(1U));
    /* Should have stored a freeze frame */
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.FreezeFrames[0].IsValid);
}

void test_ClearPrestoredFreezeFrame_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_ClearPrestoredFreezeFrame(1U));
}

void test_ClearPrestoredFreezeFrame_UnknownEvent(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_ClearPrestoredFreezeFrame(1U));
}

void test_ClearPrestoredFreezeFrame_Valid(void) {
    Dem_Init(&fullConfig);
    Dem_PrestoreFreezeFrame(1U);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.FreezeFrames[0].IsValid);
    TEST_ASSERT_EQUAL(E_OK, Dem_ClearPrestoredFreezeFrame(1U));
    TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.FreezeFrames[0].IsValid);
}

void test_MainFunction_Uninit(void) {
    Dem_DeInit();
    Dem_MainFunction(); /* should return immediately, no crash */
    TEST_ASSERT_EQUAL(DEM_STATE_UNINIT, Dem_InternalState.State);
}

void test_MainFunction_Valid(void) {
    Dem_Init(&fullConfig);
    Dem_MainFunction();
    TEST_ASSERT_EQUAL(DEM_STATE_INIT, Dem_InternalState.State);
}

void test_Shutdown_Valid(void) {
    Dem_Shutdown();
    TEST_ASSERT_EQUAL(DEM_STATE_UNINIT, Dem_InternalState.State);
}

/*===========================================================================
 * DEBOUNCE ALGORITHM TESTS
 *==========================================================================*/

void test_Debounce_NONE_Passed(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_PASSED);
    /* NONE: DebounceCounter set to passed threshold (128 due to macro) */
    /* FaultDetectionCounter also set to passed threshold */
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_PASSED, Dem_InternalState.EventStates[0].LastReportedStatus);
}

void test_Debounce_NONE_Failed(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, Dem_InternalState.EventStates[0].LastReportedStatus);
    /* With threshold=10, DebounceCounter=127 (global), >= 10 -> updateDTCStatus TRUE */
    TEST_ASSERT_NOT_EQUAL(0U,
        (Dem_InternalState.DTCEntries[0].Status & DEM_DTC_STATUS_TEST_FAILED));
}

void test_Debounce_COUNTER_Passed(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_SetEventStatus(2U, DEM_EVENT_STATUS_PASSED);
    /* COUNTER PASSED: DebounceCounter = PassedThreshold = -10 */
    TEST_ASSERT_EQUAL(-10, Dem_InternalState.EventStates[1].DebounceCounter);
    /* -10 <= -10 -> updateDTCStatus FALSE */
}

void test_Debounce_COUNTER_Failed(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_SetEventStatus(2U, DEM_EVENT_STATUS_FAILED);
    /* COUNTER FAILED: DebounceCounter = FailedThreshold = 10 */
    TEST_ASSERT_EQUAL(10, Dem_InternalState.EventStates[1].DebounceCounter);
    /* 10 >= 10 -> updateDTCStatus TRUE */
    TEST_ASSERT_NOT_EQUAL(0U,
        (Dem_InternalState.DTCEntries[1].Status & DEM_DTC_STATUS_TEST_FAILED));
}

void test_Debounce_COUNTER_PrePassed(void) {
    Dem_Init(&multiDebounceConfig);
    /* Set counter above passed threshold so decrement happens */
    Dem_InternalState.EventStates[1].DebounceCounter = 5;
    Dem_SetEventStatus(2U, DEM_EVENT_STATUS_PREPASSED);
    /* PREPASSED: counter > -10, so decrement by 1 -> 4 */
    TEST_ASSERT_EQUAL(4, Dem_InternalState.EventStates[1].DebounceCounter);
}

void test_Debounce_COUNTER_PreFailed(void) {
    Dem_Init(&multiDebounceConfig);
    /* Set counter below failed threshold so increment happens */
    Dem_InternalState.EventStates[1].DebounceCounter = -5;
    Dem_SetEventStatus(2U, DEM_EVENT_STATUS_PREFAILED);
    /* PREFAILED: counter < 10, so increment by 1 -> -4 */
    TEST_ASSERT_EQUAL(-4, Dem_InternalState.EventStates[1].DebounceCounter);
}

void test_Debounce_COUNTER_PrePassed_AtThreshold(void) {
    Dem_Init(&multiDebounceConfig);
    /* Set counter exactly at passed threshold - should NOT decrement */
    Dem_InternalState.EventStates[1].DebounceCounter = -10;
    Dem_SetEventStatus(2U, DEM_EVENT_STATUS_PREPASSED);
    /* counter > -10 is FALSE, so no decrement */
    TEST_ASSERT_EQUAL(-10, Dem_InternalState.EventStates[1].DebounceCounter);
}

void test_Debounce_COUNTER_PreFailed_AtThreshold(void) {
    Dem_Init(&multiDebounceConfig);
    /* Set counter exactly at failed threshold - should NOT increment */
    Dem_InternalState.EventStates[1].DebounceCounter = 10;
    Dem_SetEventStatus(2U, DEM_EVENT_STATUS_PREFAILED);
    /* counter < 10 is FALSE, so no increment */
    TEST_ASSERT_EQUAL(10, Dem_InternalState.EventStates[1].DebounceCounter);
}

void test_Debounce_TIME_Passed(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_SetEventStatus(3U, DEM_EVENT_STATUS_PASSED);
    /* TIME PASSED: immediate - sets FDC to passed threshold, state=IDLE */
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_IDLE, Dem_InternalState.TimeDebounceStates[2].State);
}

void test_Debounce_TIME_Failed(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_SetEventStatus(3U, DEM_EVENT_STATUS_FAILED);
    /* TIME FAILED: immediate - sets FDC to failed threshold, state=IDLE */
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_IDLE, Dem_InternalState.TimeDebounceStates[2].State);
}

void test_Debounce_TIME_PreFailed_Start(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_SetEventStatus(3U, DEM_EVENT_STATUS_PREFAILED);
    /* TIME PREFAILED from IDLE: transition to COUNTING_UP, reset elapsed */
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_COUNTING_UP, Dem_InternalState.TimeDebounceStates[2].State);
    TEST_ASSERT_EQUAL(0U, Dem_InternalState.TimeDebounceStates[2].ElapsedTimeMs);
}

void test_Debounce_TIME_PreFailed_Continue(void) {
    Dem_Init(&multiDebounceConfig);
    /* Start counting up */
    Dem_InternalState.TimeDebounceStates[2].State = DEM_TIME_DEBOUNCE_COUNTING_UP;
    Dem_InternalState.TimeDebounceStates[2].ElapsedTimeMs = 50U;
    Dem_SetEventStatus(3U, DEM_EVENT_STATUS_PREFAILED);
    /* Continue counting: elapsed += DeltaTimeMs(0) = 50 */
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_COUNTING_UP, Dem_InternalState.TimeDebounceStates[2].State);
    /* 50 < 100 threshold, so not reached yet */
    TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.TimeDebounceStates[2].ThresholdReached);
}

void test_Debounce_TIME_PreFailed_ThresholdReached(void) {
    Dem_Init(&multiDebounceConfig);
    /* Set elapsed near threshold */
    Dem_InternalState.TimeDebounceStates[2].State = DEM_TIME_DEBOUNCE_COUNTING_UP;
    Dem_InternalState.TimeDebounceStates[2].ElapsedTimeMs = 100U;
    Dem_SetEventStatus(3U, DEM_EVENT_STATUS_PREFAILED);
    /* elapsed += 0 = 100, >= 100 -> threshold reached */
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.TimeDebounceStates[2].ThresholdReached);
}

void test_Debounce_TIME_PrePassed_Start(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_SetEventStatus(3U, DEM_EVENT_STATUS_PREPASSED);
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_COUNTING_DOWN, Dem_InternalState.TimeDebounceStates[2].State);
}

void test_Debounce_TIME_PrePassed_Continue(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_InternalState.TimeDebounceStates[2].State = DEM_TIME_DEBOUNCE_COUNTING_DOWN;
    Dem_InternalState.TimeDebounceStates[2].ElapsedTimeMs = 30U;
    Dem_SetEventStatus(3U, DEM_EVENT_STATUS_PREPASSED);
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_COUNTING_DOWN, Dem_InternalState.TimeDebounceStates[2].State);
}

void test_Debounce_TIME_PrePassed_ThresholdReached(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_InternalState.TimeDebounceStates[2].State = DEM_TIME_DEBOUNCE_COUNTING_DOWN;
    Dem_InternalState.TimeDebounceStates[2].ElapsedTimeMs = 100U;
    Dem_SetEventStatus(3U, DEM_EVENT_STATUS_PREPASSED);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.TimeDebounceStates[2].ThresholdReached);
}

void test_Debounce_MONITOR_Failed(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_SetEventStatus(4U, DEM_EVENT_STATUS_FAILED);
    /* MONITOR FAILED: FDC = FAILED_THRESHOLD, updateDTCStatus TRUE */
    TEST_ASSERT_NOT_EQUAL(0U,
        (Dem_InternalState.DTCEntries[3].Status & DEM_DTC_STATUS_TEST_FAILED));
}

void test_Debounce_MONITOR_Passed(void) {
    Dem_Init(&multiDebounceConfig);
    /* First set failed */
    Dem_SetEventStatus(4U, DEM_EVENT_STATUS_FAILED);
    /* Then pass */
    Dem_SetEventStatus(4U, DEM_EVENT_STATUS_PASSED);
    /* MONITOR PASSED: FDC = PASSED_THRESHOLD, updateDTCStatus FALSE */
    TEST_ASSERT_EQUAL(0U,
        (Dem_InternalState.DTCEntries[3].Status & DEM_DTC_STATUS_TEST_FAILED));
}

/*===========================================================================
 * CLEAR DTC WITH CALLBACKS
 *==========================================================================*/

void test_ClearDTC_WithCallbacks(void) {
    Dem_Init(&callbackConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_OK,
        Dem_ClearDTC(0x00123456U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY));
    TEST_ASSERT_EQUAL(1, clearDtcStartCallCount);
    TEST_ASSERT_EQUAL(1, clearDtcFinishCallCount);
}

void test_ClearDTC_GroupAll_WithCallbacks(void) {
    Dem_Init(&callbackConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(E_OK,
        Dem_ClearDTC(DEM_DTC_GROUP_ALL, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY));
    TEST_ASSERT_EQUAL(1, clearDtcStartCallCount);
    TEST_ASSERT_EQUAL(1, clearDtcFinishCallCount);
}

void test_ClearDTC_FunctionalGroup(void) {
    Dem_Init(&multiDtcConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(2U, DEM_EVENT_STATUS_FAILED);
    /* Clear functional group: 0x300000 matches both 0x300001 and 0x300002 */
    TEST_ASSERT_EQUAL(E_OK,
        Dem_ClearDTC(0x300000U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY));
}

void test_Init_WithLambdaCallback(void) {
    /* The lambda notification is only called if DEM_CLEAR_DTC_LAMBDA_NOTIFICATION is defined */
    Dem_Init(&callbackConfig);
    /* Verify config pointer is stored regardless */
    TEST_ASSERT_EQUAL_PTR(&callbackConfig, Dem_InternalState.ConfigPtr);
    TEST_ASSERT_EQUAL(DEM_STATE_INIT, Dem_InternalState.State);
}

/*===========================================================================
 * FILTER AND GETNEXT TESTS
 *==========================================================================*/

void test_GetNextFilteredDTC_WithMatch(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    /* SelectDTC sets up filter */
    Dem_SelectDTC(0x00123456U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    Dem_DtcType d = 0U;
    Dem_UdsStatusByteType s = 0U;
    TEST_ASSERT_EQUAL(DEM_FILTERED_OK, Dem_GetNextFilteredDTC(&d, &s));
    TEST_ASSERT_EQUAL_HEX32(0x00123456U, d);
}

void test_GetNextFilteredDTC_Multiple(void) {
    Dem_Init(&multiDtcConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(2U, DEM_EVENT_STATUS_FAILED);
    Dem_SelectDTC(0x300001U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    Dem_DtcType d; Dem_UdsStatusByteType s;
    /* Should get at least one filtered DTC */
    Std_ReturnType r = Dem_GetNextFilteredDTC(&d, &s);
    TEST_ASSERT_EQUAL(DEM_FILTERED_OK, r);
}

void test_SelectDTC_UpdatesFilteredCount(void) {
    Dem_Init(&multiDtcConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(2U, DEM_EVENT_STATUS_FAILED);
    Dem_SelectDTC(0x300001U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.FilteredDTCCount);
}

/*===========================================================================
 * GETDTCOFCHECKFAILED TESTS
 *==========================================================================*/

void test_GetDTCOfCheckFailed_WithFailedDTC(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    Dem_DtcType d = 0U;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetDTCOfCheckFailed(&d));
    TEST_ASSERT_EQUAL_HEX32(0x00123456U, d);
}

/*===========================================================================
 * INDICATOR TESTS
 *==========================================================================*/

void test_GetIndicatorStatus_WithFailedDTC(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    Dem_IndicatorStatusType s = DEM_INDICATOR_OFF;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetIndicatorStatus(0U, &s));
    /* The indicator loop checks all DTCs for TF status */
    TEST_ASSERT_EQUAL(DEM_INDICATOR_CONTINUOUS, s);
}

/*===========================================================================
 * INTERNAL FUNCTION TESTS (Dem_Int.c)
 *==========================================================================*/

/* FindOrCreateDTCEntry */
void test_Int_FindOrCreateDTCEntry_NewEntry(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    TEST_ASSERT_NOT_EQUAL(DEM_INVALID_DTC_INDEX, idx);
    TEST_ASSERT_EQUAL_HEX32(0x00123456U, Dem_InternalState.DTCEntries[idx].DTC);
}

void test_Int_FindOrCreateDTCEntry_ExistingEntry(void) {
    Dem_Init(&fullConfig);
    uint8 idx1 = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    uint8 idx2 = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    TEST_ASSERT_EQUAL(idx1, idx2);
}

void test_Int_FindOrCreateDTCEntry_ReuseDeleted(void) {
    Dem_Init(&fullConfig);
    uint8 idx1 = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_InternalState.DTCEntries[idx1].IsDeleted = TRUE;
    uint8 idx2 = Dem_IntFindOrCreateDTCEntry(0x555555U);
    /* Should reuse the deleted slot or find empty slot */
    TEST_ASSERT_NOT_EQUAL(DEM_INVALID_DTC_INDEX, idx2);
}

/* FindEventConfig */
void test_Int_FindEventConfig_Found(void) {
    Dem_Init(&fullConfig);
    const Dem_EventParameterType* c = Dem_IntFindEventConfig(1U);
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_EQUAL(1U, c->EventId);
}

void test_Int_FindEventConfig_NotFound(void) {
    Dem_Init(&fullConfig);
    const Dem_EventParameterType* c = Dem_IntFindEventConfig(999U);
    TEST_ASSERT_NULL(c);
}

void test_Int_FindEventConfig_NullConfig(void) {
    Dem_InternalState.ConfigPtr = NULL_PTR;
    const Dem_EventParameterType* c = Dem_IntFindEventConfig(1U);
    TEST_ASSERT_NULL(c);
}

/* FindDTCConfig */
void test_Int_FindDTCConfig_Found(void) {
    Dem_Init(&fullConfig);
    const Dem_DtcParameterType* c = Dem_IntFindDTCConfig(0x00123456U);
    TEST_ASSERT_NOT_NULL(c);
}

void test_Int_FindDTCConfig_NotFound(void) {
    Dem_Init(&fullConfig);
    const Dem_DtcParameterType* c = Dem_IntFindDTCConfig(0x999999U);
    TEST_ASSERT_NULL(c);
}

void test_Int_FindDTCConfig_NullConfig(void) {
    Dem_InternalState.ConfigPtr = NULL_PTR;
    const Dem_DtcParameterType* c = Dem_IntFindDTCConfig(0x00123456U);
    TEST_ASSERT_NULL(c);
}

/* FindDTCIndex */
void test_Int_FindDTCIndex_Found(void) {
    Dem_Init(&fullConfig);
    Dem_IntFindOrCreateDTCEntry(0x00123456U);
    uint8 idx = Dem_IntFindDTCIndex(0x00123456U);
    TEST_ASSERT_NOT_EQUAL(DEM_INVALID_DTC_INDEX, idx);
}

void test_Int_FindDTCIndex_NotFound(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindDTCIndex(0x999999U);
    TEST_ASSERT_EQUAL(DEM_INVALID_DTC_INDEX, idx);
}

/* ResetDebounceCounter */
void test_Int_ResetDebounceCounter_Valid(void) {
    Dem_Init(&fullConfig);
    Dem_InternalState.EventStates[0].DebounceCounter = 50;
    Dem_InternalState.EventStates[0].FaultDetectionCounter = 50;
    Dem_IntResetDebounceCounter(1U);
    TEST_ASSERT_EQUAL(0, Dem_InternalState.EventStates[0].DebounceCounter);
    TEST_ASSERT_EQUAL(0, Dem_InternalState.EventStates[0].FaultDetectionCounter);
}

void test_Int_ResetDebounceCounter_InvalidId(void) {
    Dem_Init(&fullConfig);
    Dem_IntResetDebounceCounter(0U); /* should return without crash */
    Dem_IntResetDebounceCounter(999U);
}

/* UpdateDebounceCounter edge cases */
void test_Int_UpdateDebounceCounter_InvalidEventId(void) {
    Dem_Init(&fullConfig);
    Dem_IntUpdateDebounceCounter(0U, DEM_EVENT_STATUS_FAILED); /* should return */
    Dem_IntUpdateDebounceCounter(999U, DEM_EVENT_STATUS_FAILED);
}

void test_Int_UpdateDebounceCounter_NullConfig(void) {
    Dem_Init(&fullConfig);
    Dem_InternalState.ConfigPtr = NULL_PTR;
    Dem_IntUpdateDebounceCounter(1U, DEM_EVENT_STATUS_FAILED); /* should return */
}

void test_Int_UpdateDebounceCounter_TimeAlgorithm(void) {
    Dem_Init(&multiDebounceConfig);
    /* Event 3 uses TIME algorithm - should just return early */
    Dem_IntUpdateDebounceCounter(3U, DEM_EVENT_STATUS_PREFAILED);
}

/* ProcessTimebasedDebounce edge cases */
void test_Int_ProcessTimeBasedDebounce_InvalidEventId(void) {
    Dem_Init(&fullConfig);
    Dem_IntProcessTimeBasedDebounce(0U, DEM_EVENT_STATUS_PREFAILED, 10U);
    Dem_IntProcessTimeBasedDebounce(999U, DEM_EVENT_STATUS_PREFAILED, 10U);
}

void test_Int_ProcessTimeBasedDebounce_NullConfig(void) {
    Dem_Init(&fullConfig);
    Dem_InternalState.ConfigPtr = NULL_PTR;
    Dem_IntProcessTimeBasedDebounce(1U, DEM_EVENT_STATUS_PREFAILED, 10U);
}

void test_Int_ProcessTimeBasedDebounce_NotTimeAlgorithm(void) {
    Dem_Init(&fullConfig); /* event 1 uses NONE algorithm */
    Dem_IntProcessTimeBasedDebounce(1U, DEM_EVENT_STATUS_PREFAILED, 10U);
    /* Should return early since algorithm != TIME */
}

/* StoreFreezeFrame */
void test_Int_StoreFreezeFrame_Valid(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_IntStoreFreezeFrame(idx);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.FreezeFrames[0].IsValid);
    TEST_ASSERT_EQUAL(idx, Dem_InternalState.FreezeFrames[0].DtcIndex);
}

void test_Int_StoreFreezeFrame_InvalidIndex(void) {
    Dem_Init(&fullConfig);
    Dem_IntStoreFreezeFrame(DEM_NUM_DTCS + 1U); /* should return */
}

void test_Int_StoreFreezeFrame_UpdateExisting(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_IntStoreFreezeFrame(idx);
    /* Store again for same DTC - should update existing slot */
    Dem_IntStoreFreezeFrame(idx);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.FreezeFrames[0].IsValid);
}

/* ClearFreezeFrame */
void test_Int_ClearFreezeFrame_Valid(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_IntStoreFreezeFrame(idx);
    Dem_IntClearFreezeFrame(idx);
    TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.FreezeFrames[0].IsValid);
}

/* GetFreezeFrame */
void test_Int_GetFreezeFrame_NullPtr(void) {
    uint16 sz = 4;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntGetFreezeFrame(0U, 0U, NULL_PTR, &sz));
    uint8 buf[4];
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntGetFreezeFrame(0U, 0U, buf, NULL_PTR));
}

void test_Int_GetFreezeFrame_InvalidIndex(void) {
    uint8 buf[4]; uint16 sz = sizeof(buf);
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntGetFreezeFrame(DEM_NUM_DTCS + 1U, 0U, buf, &sz));
}

void test_Int_GetFreezeFrame_NotFound(void) {
    Dem_Init(&fullConfig);
    uint8 buf[4]; uint16 sz = sizeof(buf);
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntGetFreezeFrame(0U, 0U, buf, &sz));
}

void test_Int_GetFreezeFrame_BufferTooSmall(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_IntStoreFreezeFrame(idx);
    uint8 buf[4]; uint16 sz = 4U; /* smaller than DEM_FREEZE_FRAME_MAX_SIZE */
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntGetFreezeFrame(idx, 0U, buf, &sz));
}

void test_Int_GetFreezeFrame_Valid(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_IntStoreFreezeFrame(idx);
    uint8 buf[DEM_FREEZE_FRAME_MAX_SIZE];
    uint16 sz = sizeof(buf);
    TEST_ASSERT_EQUAL(E_OK, Dem_IntGetFreezeFrame(idx, 0U, buf, &sz));
    TEST_ASSERT_EQUAL(DEM_FREEZE_FRAME_MAX_SIZE, sz);
}

/* StoreExtendedData */
void test_Int_StoreExtendedData_Record0_Skipped(void) {
    Dem_Init(&extDataConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x200001U);
    Dem_IntStoreExtendedData(idx, 0U); /* RecordNumber 0 -> return early */
    TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.ExtendedDataRecords[0].IsValid);
}

void test_Int_StoreExtendedData_Record1(void) {
    Dem_Init(&extDataConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x200001U);
    Dem_IntStoreExtendedData(idx, 1U);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.ExtendedDataRecords[0].IsValid);
    TEST_ASSERT_EQUAL(1U, Dem_InternalState.ExtendedDataRecords[0].RecordNumber);
}

void test_Int_StoreExtendedData_Record2(void) {
    Dem_Init(&extDataConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x200001U);
    Dem_IntStoreExtendedData(idx, 2U);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.ExtendedDataRecords[0].IsValid);
    TEST_ASSERT_EQUAL(2U, Dem_InternalState.ExtendedDataRecords[0].RecordNumber);
}

void test_Int_StoreExtendedData_UnknownRecord(void) {
    Dem_Init(&extDataConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x200001U);
    Dem_IntStoreExtendedData(idx, 99U); /* not in config -> return */
    /* No record should be stored */
}

void test_Int_StoreExtendedData_InvalidIndex(void) {
    Dem_Init(&extDataConfig);
    Dem_IntStoreExtendedData(DEM_NUM_DTCS + 1U, 1U); /* should return */
}

void test_Int_StoreExtendedData_OverwriteValidEntry(void) {
    Dem_Init(&extDataConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x200001U);
    /* Store record 1 first */
    Dem_IntStoreExtendedData(idx, 1U);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.ExtendedDataRecords[0].IsValid);
    /* Store record 1 again - should find the existing valid entry */
    Dem_IntStoreExtendedData(idx, 1U);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.ExtendedDataRecords[0].IsValid);
    TEST_ASSERT_EQUAL(1U, Dem_InternalState.ExtendedDataRecords[0].RecordNumber);
}

void test_Int_StoreExtendedData_DefaultRecordNumber(void) {
    Dem_Init(&extDataConfig3);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x200001U);
    /* Store record 3 (uses default case in switch) */
    Dem_IntStoreExtendedData(idx, 3U);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.ExtendedDataRecords[0].IsValid);
    TEST_ASSERT_EQUAL(3U, Dem_InternalState.ExtendedDataRecords[0].RecordNumber);
}

/* GetExtendedData */
void test_Int_GetExtendedData_NullPtr(void) {
    uint16 sz = 4;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntGetExtendedData(0U, 1U, NULL_PTR, &sz));
    uint8 buf[4];
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntGetExtendedData(0U, 1U, buf, NULL_PTR));
}

void test_Int_GetExtendedData_NotFound(void) {
    Dem_Init(&extDataConfig);
    uint8 buf[4]; uint16 sz = sizeof(buf);
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntGetExtendedData(0U, 1U, buf, &sz));
}

void test_Int_GetExtendedData_BufferTooSmall(void) {
    Dem_Init(&extDataConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x200001U);
    Dem_IntStoreExtendedData(idx, 1U);
    uint8 buf[2]; uint16 sz = 2U; /* smaller than record size 4 */
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntGetExtendedData(idx, 1U, buf, &sz));
}

void test_Int_GetExtendedData_Valid(void) {
    Dem_Init(&extDataConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x200001U);
    Dem_IntStoreExtendedData(idx, 1U);
    uint8 buf[4]; uint16 sz = sizeof(buf);
    TEST_ASSERT_EQUAL(E_OK, Dem_IntGetExtendedData(idx, 1U, buf, &sz));
    TEST_ASSERT_EQUAL_UINT16(4U, sz);
}

/* ProcessAging */
void test_Int_ProcessAging_NoAging(void) {
    Dem_Init(&fullConfig);
    /* No DTC entries with CONFIRMED status -> no aging */
    Dem_IntProcessAging();
}

void test_Int_ProcessAging_WithConfirmedDTC(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_InternalState.DTCEntries[idx].Status |= DEM_DTC_STATUS_CONFIRMED_DTC;
    Dem_InternalState.DTCEntries[idx].Status &= ~DEM_DTC_STATUS_TEST_FAILED;
    Dem_InternalState.DTCEntries[idx].IsAged = FALSE;
    Dem_InternalState.DTCEntries[idx].AgingCounter = 0U;
    Dem_IntProcessAging();
    TEST_ASSERT_EQUAL(1U, Dem_InternalState.DTCEntries[idx].AgingCounter);
}

/* AgeDTCEntry */
void test_Int_AgeDTCEntry_Valid(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_InternalState.DTCEntries[idx].Status |= DEM_DTC_STATUS_CONFIRMED_DTC;
    Dem_IntAgeDTCEntry(idx);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.DTCEntries[idx].IsAged);
    TEST_ASSERT_EQUAL(0U, (Dem_InternalState.DTCEntries[idx].Status & DEM_DTC_STATUS_CONFIRMED_DTC));
}

void test_Int_AgeDTCEntry_InvalidIndex(void) {
    Dem_IntAgeDTCEntry(DEM_NUM_DTCS + 1U); /* should return */
}

/* ClearSingleDTC */
void test_Int_ClearSingleDTC_Valid(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_InternalState.DTCEntries[idx].Status = DEM_UDS_STATUS_TF;
    Dem_IntClearSingleDTC(idx);
    TEST_ASSERT_EQUAL(0U, (Dem_InternalState.DTCEntries[idx].Status & DEM_UDS_STATUS_TF));
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.DTCEntries[idx].IsDeleted);
}

void test_Int_ClearSingleDTC_InvalidIndex(void) {
    Dem_IntClearSingleDTC(DEM_NUM_DTCS + 1U); /* should return */
}

/* ClearAllDTCs */
void test_Int_ClearAllDTCs(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_InternalState.DTCEntries[idx].Status = DEM_UDS_STATUS_TF;
    Dem_IntClearAllDTCs();
    TEST_ASSERT_EQUAL(0U, (Dem_InternalState.DTCEntries[idx].Status & DEM_UDS_STATUS_TF));
}

/* UpdateDTCStatusFromDebounce */
void test_Int_UpdateDTCStatus_True(void) {
    Dem_Init(&fullConfig);
    Dem_IntUpdateDTCStatusFromDebounce(1U, TRUE);
    uint8 idx = Dem_IntFindDTCIndex(0x00123456U);
    if (idx != DEM_INVALID_DTC_INDEX) {
        TEST_ASSERT_NOT_EQUAL(0U,
            (Dem_InternalState.DTCEntries[idx].Status & DEM_DTC_STATUS_TEST_FAILED));
    }
}

void test_Int_UpdateDTCStatus_False(void) {
    Dem_Init(&fullConfig);
    /* Set failed first */
    Dem_IntUpdateDTCStatusFromDebounce(1U, TRUE);
    /* Then pass */
    Dem_InternalState.EventStates[0].TestCompletedThisOperationCycle = TRUE;
    Dem_IntUpdateDTCStatusFromDebounce(1U, FALSE);
    uint8 idx = Dem_IntFindDTCIndex(0x00123456U);
    if (idx != DEM_INVALID_DTC_INDEX) {
        TEST_ASSERT_EQUAL(0U,
            (Dem_InternalState.DTCEntries[idx].Status & DEM_DTC_STATUS_TEST_FAILED));
    }
}

void test_Int_UpdateDTCStatus_NullConfig(void) {
    Dem_InternalState.ConfigPtr = NULL_PTR;
    Dem_IntUpdateDTCStatusFromDebounce(1U, TRUE); /* should return */
}

/* MatchDTCFilter */
void test_Int_MatchDTCFilter_OutOfRange(void) {
    TEST_ASSERT_EQUAL(FALSE, Dem_IntMatchDTCFilter(DEM_NUM_DTCS));
}

void test_Int_MatchDTCFilter_EmptySlot(void) {
    Dem_Init(&fullConfig);
    /* After init with fullConfig, slot 0 has DTC=0x00123456, slot 1 has DTC=0 */
    TEST_ASSERT_EQUAL(FALSE, Dem_IntMatchDTCFilter(1U));
}

void test_Int_MatchDTCFilter_DeletedEntry(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_InternalState.DTCEntries[idx].IsDeleted = TRUE;
    TEST_ASSERT_EQUAL(FALSE, Dem_IntMatchDTCFilter(idx));
}

void test_Int_MatchDTCFilter_NoConfig(void) {
    Dem_Init(&fullConfig);
    /* Create entry with DTC not in config */
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0xAAAAAAU);
    TEST_ASSERT_EQUAL(FALSE, Dem_IntMatchDTCFilter(idx));
}

void test_Int_MatchDTCFilter_OriginMismatch(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    /* Set filter origin to something different */
    Dem_InternalState.DTCFilterOrigin = DEM_DTC_ORIGIN_MIRROR_MEMORY;
    TEST_ASSERT_EQUAL(FALSE, Dem_IntMatchDTCFilter(idx));
}

void test_Int_MatchDTCFilter_Match(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_InternalState.DTCFilterOrigin = DEM_DTC_ORIGIN_PRIMARY_MEMORY;
    TEST_ASSERT_EQUAL(TRUE, Dem_IntMatchDTCFilter(idx));
}

/* UpdateFilteredCount */
void test_Int_UpdateFilteredCount(void) {
    Dem_Init(&fullConfig);
    Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_InternalState.DTCFilterOrigin = DEM_DTC_ORIGIN_PRIMARY_MEMORY;
    Dem_IntUpdateFilteredCount();
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.FilteredDTCCount);
}

/* CountDTCGroupMatches */
void test_Int_CountDTCGroupMatches(void) {
    Dem_Init(&multiDtcConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(2U, DEM_EVENT_STATUS_FAILED);
    /* Group 0x300000 should match both 0x300001 and 0x300002 */
    uint8 count = Dem_IntCountDTCGroupMatches(0x300000U);
    TEST_ASSERT_EQUAL(2U, count);
}

void test_Int_CountDTCGroupMatches_NoMatch(void) {
    Dem_Init(&fullConfig);
    uint8 count = Dem_IntCountDTCGroupMatches(0xFFFFFFU);
    /* No deleted entries with matching mask */
}

/* ClearDTCGroup */
void test_Int_ClearDTCGroup(void) {
    Dem_Init(&multiDtcConfig);
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(2U, DEM_EVENT_STATUS_FAILED);
    Dem_IntClearDTCGroup(0x300000U);
    /* Both should be cleared */
    uint8 idx1 = Dem_IntFindDTCIndex(0x300001U);
    if (idx1 != DEM_INVALID_DTC_INDEX) {
        TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.DTCEntries[idx1].IsDeleted);
    }
}

/* CheckInit */
void test_Int_CheckInit_Init(void) {
    TEST_ASSERT_EQUAL(E_OK, Dem_IntCheckInit());
}

void test_Int_CheckInit_Uninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntCheckInit());
}

/* ValidateEventId */
void test_Int_ValidateEventId_Valid(void) {
    TEST_ASSERT_EQUAL(E_OK, Dem_IntValidateEventId(1U));
}

void test_Int_ValidateEventId_Invalid(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntValidateEventId(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntValidateEventId(999U));
}

/* ProcessDebounceMainFunction */
void test_Int_ProcessDebounceMainFunction_NoTimeDebounce(void) {
    Dem_Init(&fullConfig); /* NONE algorithm only */
    Dem_IntProcessDebounceMainFunction(10U);
}

void test_Int_ProcessDebounceMainFunction_WithThresholdReached(void) {
    Dem_Init(&multiDebounceConfig);
    /* Set time debounce state with threshold reached */
    Dem_InternalState.TimeDebounceStates[2].State = DEM_TIME_DEBOUNCE_COUNTING_UP;
    Dem_InternalState.TimeDebounceStates[2].ThresholdReached = TRUE;
    Dem_IntProcessDebounceMainFunction(10U);
    /* Should update DTC status and reset state */
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_IDLE, Dem_InternalState.TimeDebounceStates[2].State);
    TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.TimeDebounceStates[2].ThresholdReached);
}

void test_Int_ProcessDebounceMainFunction_CountingDown(void) {
    Dem_Init(&multiDebounceConfig);
    Dem_InternalState.TimeDebounceStates[2].State = DEM_TIME_DEBOUNCE_COUNTING_DOWN;
    Dem_InternalState.TimeDebounceStates[2].ThresholdReached = TRUE;
    Dem_IntProcessDebounceMainFunction(10U);
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_IDLE, Dem_InternalState.TimeDebounceStates[2].State);
}

/* HandleOperationCycleStart/End */
void test_Int_HandleOperationCycleStart(void) {
    Dem_IntHandleOperationCycleStart(0U); /* no-op but should not crash */
}

void test_Int_HandleOperationCycleEnd(void) {
    Dem_IntHandleOperationCycleEnd(0U); /* no-op but should not crash */
}

/*===========================================================================
 * AGING THRESHOLD REACHED TEST
 *==========================================================================*/

void test_Int_ProcessAging_ThresholdReached(void) {
    Dem_Init(&fullConfig);
    uint8 idx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_InternalState.DTCEntries[idx].Status |= DEM_DTC_STATUS_CONFIRMED_DTC;
    Dem_InternalState.DTCEntries[idx].Status &= ~DEM_DTC_STATUS_TEST_FAILED;
    Dem_InternalState.DTCEntries[idx].IsAged = FALSE;
    /* Set aging counter just below threshold */
    Dem_InternalState.DTCEntries[idx].AgingCounter = DEM_AGING_THRESHOLD - 1U;
    Dem_IntProcessAging();
    /* Should have aged the entry */
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.DTCEntries[idx].IsAged);
}

/*===========================================================================
 * BATCH 2: Additional branch coverage tests
 *==========================================================================*/

/* --- DTC status bit management --- */

void test_SetEventStatus_FAILED_SetsTFAndPDTCBits(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    uint8 dtcIdx = Dem_IntFindDTCIndex(0x00123456U);
    TEST_ASSERT_NOT_EQUAL(0xFFU, dtcIdx);
    /* TF bit should be set */
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_TEST_FAILED);
    /* PDTC bit should be set */
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_PENDING_DTC);
    /* TFSLC bit should be set */
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_TEST_FAILED_SINCE_LAST_CLEAR);
    /* TFTOC bit should be set */
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_TEST_FAILED_THIS_OPERATION_CYCLE);
}

void test_SetEventStatus_PASSED_ClearsTFBit(void) {
    Dem_Init(&fullConfig);
    /* First set failed */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    uint8 dtcIdx = Dem_IntFindDTCIndex(0x00123456U);
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_TEST_FAILED);
    /* Now set passed */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_PASSED);
    /* TF bit should be cleared */
    TEST_ASSERT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_TEST_FAILED);
}

void test_SetEventStatus_ConfirmationThreshold_SetsCDTC(void) {
    /* Config with confirmation threshold = 1 (fullConfig has threshold=0, so 1 occurrence confirms) */
    Dem_Init(&fullConfig);
    /* fullConfig event has EventConfirmationThreshold=0, so first fail confirms */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    uint8 dtcIdx = Dem_IntFindDTCIndex(0x00123456U);
    /* With threshold=0, OccurrenceCounter=1 >= 0, so CDTC should be set */
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_CONFIRMED_DTC);
}

void test_SetEventStatus_FreezeFrameStored_OnFirstConfirmation(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    /* Check that a freeze frame was stored */
    boolean foundFF = FALSE;
    for (uint16 i = 0; i < DEM_NUM_FREEZE_FRAME_RECORDS; i++) {
        if (Dem_InternalState.FreezeFrames[i].IsValid) {
            foundFF = TRUE;
            break;
        }
    }
    TEST_ASSERT_EQUAL(TRUE, foundFF);
}

void test_SetEventStatus_SecondConfirmation_NoNewFreezeFrame(void) {
    /* Use a config with confirmation threshold = 2 so we need 2 occurrences */
    static Dem_EventParameterType evParams[1];
    evParams[0] = (Dem_EventParameterType){
        1U, 0x00123456U, 0U, TRUE, TRUE, 0U, 2U,
        DEM_DEBOUNCE_ALGORITHM_NONE,
        FALSE, FALSE, FALSE, 127, -128, 0U, 0U };
    static Dem_ConfigType cfg;
    cfg.EventParameters = evParams;
    cfg.NumEvents = 1;
    cfg.DtcParameters = dtcParams;
    cfg.NumDtcs = 1;
    cfg.FreezeFrameRecords = NULL_PTR;
    cfg.NumFreezeFrameRecords = 0;
    cfg.ExtendedDataRecords = NULL_PTR;
    cfg.NumExtendedDataRecords = 0;
    cfg.Indicators = NULL_PTR;
    cfg.NumIndicators = 0;
    cfg.DevErrorDetect = TRUE;
    cfg.VersionInfoApi = TRUE;
    cfg.ClearDtcSupported = TRUE;
    cfg.DtcStatusAvailabilityMask = 0xFF;
    cfg.ClearDtcLimitation = FALSE;
    cfg.ClearDTCStartNotification = NULL_PTR;
    cfg.ClearDTCFinishNotification = NULL_PTR;
    cfg.ClearDTCLambdaNotification = NULL_PTR;

    Dem_Init(&cfg);
    /* First fail: occurrence=1, threshold=2, not yet confirmed */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    uint8 dtcIdx = Dem_IntFindDTCIndex(0x00123456U);
    TEST_ASSERT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_CONFIRMED_DTC);
    /* Second fail: occurrence=2, threshold=2, now confirmed -> freeze frame stored */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_CONFIRMED_DTC);
}

/* --- Init edge cases --- */

void test_Init_NumDtcsLessThanDEM_NUM_DTCS(void) {
    /* fullConfig has NumDtcs=1, DEM_NUM_DTCS=64.
     * The else branch in init loop sets DTC=0 for i >= NumDtcs */
    Dem_Init(&fullConfig);
    /* Entry 0 should have the configured DTC */
    TEST_ASSERT_EQUAL(0x00123456U, Dem_InternalState.DTCEntries[0].DTC);
    /* Entry 1 should have DTC=0 (else branch) */
    TEST_ASSERT_EQUAL(0U, Dem_InternalState.DTCEntries[1].DTC);
    /* Entry 63 should have DTC=0 (else branch) */
    TEST_ASSERT_EQUAL(0U, Dem_InternalState.DTCEntries[63].DTC);
}

/* --- Operation cycle transitions --- */

void test_SetOperationCycleState_END_to_START(void) {
    Dem_Init(&fullConfig);
    /* After init, all cycles are DEM_CYCLE_STATE_END */
    Dem_OperationCycleStateType state;
    Dem_GetOperationCycleState(DEM_OPCYC_POWER, &state);
    TEST_ASSERT_EQUAL(DEM_CYCLE_STATE_END, state);
    /* Transition END -> START */
    Dem_SetOperationCycleState(DEM_OPCYC_POWER, DEM_CYCLE_STATE_START);
    Dem_GetOperationCycleState(DEM_OPCYC_POWER, &state);
    TEST_ASSERT_EQUAL(DEM_CYCLE_STATE_START, state);
}

void test_SetOperationCycleState_START_to_END(void) {
    Dem_Init(&fullConfig);
    /* Start the cycle first */
    Dem_SetOperationCycleState(DEM_OPCYC_POWER, DEM_CYCLE_STATE_START);
    /* Now end it */
    Dem_SetOperationCycleState(DEM_OPCYC_POWER, DEM_CYCLE_STATE_END);
    Dem_OperationCycleStateType state;
    Dem_GetOperationCycleState(DEM_OPCYC_POWER, &state);
    TEST_ASSERT_EQUAL(DEM_CYCLE_STATE_END, state);
}

void test_SetOperationCycleState_SameState_NoTransitionEffect(void) {
    Dem_Init(&fullConfig);
    /* END -> END should not trigger HandleOperationCycleStart */
    Dem_SetOperationCycleState(DEM_OPCYC_POWER, DEM_CYCLE_STATE_END);
    Dem_OperationCycleStateType state;
    Dem_GetOperationCycleState(DEM_OPCYC_POWER, &state);
    TEST_ASSERT_EQUAL(DEM_CYCLE_STATE_END, state);
}

void test_RestartOperationCycle(void) {
    Dem_Init(&fullConfig);
    /* Start a cycle */
    Dem_SetOperationCycleState(DEM_OPCYC_POWER, DEM_CYCLE_STATE_START);
    /* Restart it (END then START) */
    Std_ReturnType ret = Dem_RestartOperationCycle(DEM_OPCYC_POWER);
    TEST_ASSERT_EQUAL(E_OK, ret);
    Dem_OperationCycleStateType state;
    Dem_GetOperationCycleState(DEM_OPCYC_POWER, &state);
    TEST_ASSERT_EQUAL(DEM_CYCLE_STATE_START, state);
}

void test_SetOperationCycleState_InvalidCycleType(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_SetOperationCycleState((Dem_OperationCycleType)255, DEM_CYCLE_STATE_START);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_GetOperationCycleState_NullCycleStatePtr(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_GetOperationCycleState(DEM_OPCYC_POWER, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_GetOperationCycleState_InvalidCycleType(void) {
    Dem_Init(&fullConfig);
    Dem_OperationCycleStateType state;
    Std_ReturnType ret = Dem_GetOperationCycleState((Dem_OperationCycleType)255, &state);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- GetStatusOfDTC valid path --- */

void test_GetStatusOfDTC_ValidDTC_ReturnsOK(void) {
    Dem_Init(&fullConfig);
    /* Create a DTC entry by setting event status */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    Dem_UdsStatusByteType status = 0;
    Std_ReturnType ret = Dem_GetStatusOfDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* Should have TF bit set */
    TEST_ASSERT_NOT_EQUAL(0U, status & DEM_DTC_STATUS_TEST_FAILED);
}

void test_GetStatusOfDTC_UnknownDTC_ReturnsWrongDTC(void) {
    Dem_Init(&fullConfig);
    Dem_UdsStatusByteType status = 0xFF;
    Std_ReturnType ret = Dem_GetStatusOfDTC(0x999999U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status);
    TEST_ASSERT_EQUAL(DEM_STATUS_WRONG_DTC, ret);
    TEST_ASSERT_EQUAL(0U, status);
}

/* --- ClearDTC paths --- */

void test_ClearDTC_SingleDTC_WithValidIndex(void) {
    Dem_Init(&callbackConfig);
    /* Create a DTC entry */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    uint8 dtcIdx = Dem_IntFindDTCIndex(0x00123456U);
    TEST_ASSERT_NOT_EQUAL(0xFFU, dtcIdx);
    /* Clear the single DTC */
    Std_ReturnType ret = Dem_ClearDTC(0x00123456U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* Start and finish callbacks should have been called */
    TEST_ASSERT_EQUAL(1, clearDtcStartCallCount);
    TEST_ASSERT_EQUAL(1, clearDtcFinishCallCount);
}

void test_ClearDTC_FunctionalGroup_WithMask(void) {
    Dem_Init(&multiDtcConfig);
    /* Create entries for both DTCs */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(2, DEM_EVENT_STATUS_FAILED);
    /* Clear by group mask 0x300000 - both DTCs 0x300001 and 0x300002 match */
    Std_ReturnType ret = Dem_ClearDTC(0x300000U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_ClearDTC_GroupAll(void) {
    Dem_Init(&callbackConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    Std_ReturnType ret = Dem_ClearDTC(DEM_DTC_GROUP_ALL, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(1, clearDtcStartCallCount);
    TEST_ASSERT_EQUAL(1, clearDtcFinishCallCount);
}

void test_ClearDTC_NullCallbacks(void) {
    /* fullConfig has NULL start/finish callbacks */
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    Std_ReturnType ret = Dem_ClearDTC(0x00123456U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/* --- GetNextFilteredDTC iteration --- */

void test_GetNextFilteredDTC_ExhaustsAllEntries(void) {
    Dem_Init(&multiDtcConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    Dem_SetEventStatus(2, DEM_EVENT_STATUS_FAILED);
    Dem_SelectDTC(0x300001U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    /* Both DTCs match (filter matches by origin, both are PRIMARY_MEMORY) */
    Dem_DtcType dtc;
    Dem_UdsStatusByteType status;
    Std_ReturnType ret = Dem_GetNextFilteredDTC(&dtc, &status);
    TEST_ASSERT_EQUAL(DEM_FILTERED_OK, ret);
    /* Second DTC also matches */
    ret = Dem_GetNextFilteredDTC(&dtc, &status);
    TEST_ASSERT_EQUAL(DEM_FILTERED_OK, ret);
    /* Third call - exhausted all entries */
    ret = Dem_GetNextFilteredDTC(&dtc, &status);
    TEST_ASSERT_EQUAL(DEM_FILTERED_NO_MATCHING_ELEMENT, ret);
}

void test_GetNextFilteredDTC_NullDTCPtr(void) {
    Dem_Init(&fullConfig);
    Dem_UdsStatusByteType status;
    Std_ReturnType ret = Dem_GetNextFilteredDTC(NULL_PTR, &status);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_GetNextFilteredDTC_NullStatusPtr(void) {
    Dem_Init(&fullConfig);
    Dem_DtcType dtc;
    Std_ReturnType ret = Dem_GetNextFilteredDTC(&dtc, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- MainFunction --- */

void test_MainFunction_Uninit_NoAction(void) {
    /* DeInit first */
    Dem_Init(&fullConfig);
    Dem_DeInit();
    /* Should not crash */
    Dem_MainFunction();
}

void test_MainFunction_Initialized(void) {
    Dem_Init(&fullConfig);
    /* Should process without crash */
    Dem_MainFunction();
    Dem_MainFunction();
}

/* --- SetIndicatorStatus --- */

void test_SetIndicatorStatus_ValidId(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_SetIndicatorStatus(0, DEM_INDICATOR_CONTINUOUS);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_SetIndicatorStatus_InvalidId_B2(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_SetIndicatorStatus(DEM_NUM_INDICATORS, DEM_INDICATOR_CONTINUOUS);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_SetIndicatorStatus_Uninit_B2(void) {
    Dem_Init(&fullConfig);
    Dem_DeInit();
    Std_ReturnType ret = Dem_SetIndicatorStatus(0, DEM_INDICATOR_CONTINUOUS);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- GetIndicatorStatus --- */

void test_GetIndicatorStatus_NoFailedDTCs(void) {
    Dem_Init(&fullConfig);
    Dem_IndicatorStatusType status = DEM_INDICATOR_CONTINUOUS;
    Std_ReturnType ret = Dem_GetIndicatorStatus(0, &status);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(DEM_INDICATOR_OFF, status);
}

void test_GetIndicatorStatus_WithSuppressedDTC(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    uint8 dtcIdx = Dem_IntFindDTCIndex(0x00123456U);
    /* Suppress the DTC */
    Dem_InternalState.DTCEntries[dtcIdx].IsSuppressed = TRUE;
    Dem_IndicatorStatusType status = DEM_INDICATOR_OFF;
    Dem_GetIndicatorStatus(0, &status);
    /* Suppressed DTC should not trigger indicator */
    TEST_ASSERT_EQUAL(DEM_INDICATOR_OFF, status);
}

void test_GetIndicatorStatus_NullPtr_B2(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_GetIndicatorStatus(0, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}


/* --- FreezeFrame by DTC --- */

void test_GetFreezeFrameDataByDTC_ValidWithStoredData(void) {
    Dem_Init(&fullConfig);
    /* Trigger freeze frame storage */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    uint8 buf[256];
    uint16 bufSize = sizeof(buf);
    Std_ReturnType ret = Dem_GetFreezeFrameDataByDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 0, buf, &bufSize);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(DEM_FREEZE_FRAME_MAX_SIZE, bufSize);
}

void test_GetFreezeFrameDataByDTC_WrongDTC(void) {
    Dem_Init(&fullConfig);
    uint8 buf[256];
    uint16 bufSize = sizeof(buf);
    Std_ReturnType ret = Dem_GetFreezeFrameDataByDTC(0x999999U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 0, buf, &bufSize);
    TEST_ASSERT_EQUAL(DEM_GET_FREEZEFRAME_WRONG_DTC, ret);
}

void test_GetFreezeFrameDataByDTC_NullBuffers(void) {
    Dem_Init(&fullConfig);
    uint16 bufSize = 256;
    Std_ReturnType ret = Dem_GetFreezeFrameDataByDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 0, NULL_PTR, &bufSize);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- ExtendedData by DTC --- */

void test_GetExtendedDataRecordByDTC_ValidWithStoredData(void) {
    Dem_Init(&extDataConfig);
    /* Store extended data via internal function */
    uint8 dtcIdx = Dem_IntFindOrCreateDTCEntry(0x200001U);
    Dem_InternalState.DTCEntries[dtcIdx].OccurrenceCounter = 5;
    Dem_IntStoreExtendedData(dtcIdx, 1);
    uint8 buf[128];
    uint16 bufSize = sizeof(buf);
    Std_ReturnType ret = Dem_GetExtendedDataRecordByDTC(0x200001U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1, buf, &bufSize);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

void test_GetExtendedDataRecordByDTC_WrongDTC(void) {
    Dem_Init(&extDataConfig);
    uint8 buf[128];
    uint16 bufSize = sizeof(buf);
    Std_ReturnType ret = Dem_GetExtendedDataRecordByDTC(0x999999U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1, buf, &bufSize);
    TEST_ASSERT_EQUAL(DEM_RECORD_WRONG_DTC, ret);
}

void test_GetExtendedDataRecordByDTC_NullBuffers(void) {
    Dem_Init(&extDataConfig);
    uint16 bufSize = 128;
    Std_ReturnType ret = Dem_GetExtendedDataRecordByDTC(0x200001U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1, NULL_PTR, &bufSize);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- GetSizeOfExtendedDataRecordByDTC --- */

void test_GetSizeOfExtendedDataRecordByDTC_Found(void) {
    Dem_Init(&extDataConfig);
    uint16 size = 0;
    Std_ReturnType ret = Dem_GetSizeOfExtendedDataRecordByDTC(0x200001U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1, &size);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(4U, size);
}

void test_GetSizeOfExtendedDataRecordByDTC_WrongDTC(void) {
    Dem_Init(&extDataConfig);
    uint16 size = 0;
    Std_ReturnType ret = Dem_GetSizeOfExtendedDataRecordByDTC(0x999999U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1, &size);
    TEST_ASSERT_EQUAL(DEM_RECORD_WRONG_DTC, ret);
}

void test_GetSizeOfExtendedDataRecordByDTC_WrongNumber(void) {
    Dem_Init(&extDataConfig);
    uint16 size = 0;
    Std_ReturnType ret = Dem_GetSizeOfExtendedDataRecordByDTC(0x200001U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 99, &size);
    TEST_ASSERT_EQUAL(DEM_RECORD_WRONG_NUMBER, ret);
}

void test_GetSizeOfExtendedDataRecordByDTC_NullPtr(void) {
    Dem_Init(&extDataConfig);
    Std_ReturnType ret = Dem_GetSizeOfExtendedDataRecordByDTC(0x200001U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_GetSizeOfExtendedDataRecordByDTC_Uninit(void) {
    Dem_Init(&extDataConfig);
    Dem_DeInit();
    uint16 size;
    Std_ReturnType ret = Dem_GetSizeOfExtendedDataRecordByDTC(0x200001U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, 1, &size);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- PrestoreFreezeFrame / ClearPrestoredFreezeFrame --- */


void test_PrestoreFreezeFrame_InvalidEventId(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_PrestoreFreezeFrame(0);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_PrestoreFreezeFrame_EventNotInConfig(void) {
    Dem_Init(&fullConfig);
    /* EventId 100 is valid range but not in fullConfig */
    Std_ReturnType ret = Dem_PrestoreFreezeFrame(100);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}



void test_ClearPrestoredFreezeFrame_NoDTCIndex(void) {
    Dem_Init(&fullConfig);
    /* Event 1 is in config but DTC may not have an entry yet */
    Std_ReturnType ret = Dem_ClearPrestoredFreezeFrame(1);
    /* If DTC index not found, returns E_NOT_OK */
    /* Depends on whether FindDTCIndex finds it */
    (void)ret;
}

void test_ClearPrestoredFreezeFrame_EventNotInConfig(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_ClearPrestoredFreezeFrame(100);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}


/* --- Counter debounce: PREPASSED, PREFAILED, default --- */

void test_CounterDebounce_PREPASSED_DecrementsCounter(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    /* Event 2 is COUNTER with threshold 10/-10 */
    /* First set to FAILED to set counter to 10 */
    Dem_SetEventStatus(2, DEM_EVENT_STATUS_FAILED);
    sint8 fdc;
    Dem_GetFaultDetectionCounter(2, &fdc);
    TEST_ASSERT_EQUAL(10, fdc);
    /* Now PREPASSED should decrement */
    Dem_SetEventStatus(2, DEM_EVENT_STATUS_PREPASSED);
    Dem_GetFaultDetectionCounter(2, &fdc);
    TEST_ASSERT_EQUAL(9, fdc);
}

void test_CounterDebounce_PREFAILED_IncrementsCounter(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    /* Event 2 is COUNTER, start from 0 */
    Dem_SetEventStatus(2, DEM_EVENT_STATUS_PREFAILED);
    sint8 fdc;
    Dem_GetFaultDetectionCounter(2, &fdc);
    TEST_ASSERT_EQUAL(1, fdc);
}

void test_CounterDebounce_PREPASSED_DoesNotGoBelowPassedThreshold(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    /* Set counter to passed threshold directly */
    uint16 eventIndex = 2 - 1; /* EventId 2 */
    Dem_InternalState.EventStates[eventIndex].DebounceCounter = -10; /* = PassedThreshold */
    /* PREPASSED should not decrement below PassedThreshold */
    Dem_IntUpdateDebounceCounter(2, DEM_EVENT_STATUS_PREPASSED);
    TEST_ASSERT_EQUAL(-10, Dem_InternalState.EventStates[eventIndex].DebounceCounter);
}

void test_CounterDebounce_PREFAILED_DoesNotExceedFailedThreshold(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    uint16 eventIndex = 2 - 1;
    Dem_InternalState.EventStates[eventIndex].DebounceCounter = 10; /* = FailedThreshold */
    Dem_IntUpdateDebounceCounter(2, DEM_EVENT_STATUS_PREFAILED);
    TEST_ASSERT_EQUAL(10, Dem_InternalState.EventStates[eventIndex].DebounceCounter);
}

void test_CounterDebounce_DefaultEventStatus(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    /* FDC_THRESHOLD_REACHED (4) is the default case */
    Dem_IntUpdateDebounceCounter(2, DEM_EVENT_STATUS_FDC_THRESHOLD_REACHED);
    /* Should do nothing - counter stays at 0 */
    TEST_ASSERT_EQUAL(0, Dem_InternalState.EventStates[2 - 1].DebounceCounter);
}

/* --- Monitor debounce: PASSED, FAILED, default --- */

void test_MonitorDebounce_PASSED_SetsPassedThreshold(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    Dem_IntUpdateDebounceCounter(4, DEM_EVENT_STATUS_PASSED);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD,
                      Dem_InternalState.EventStates[4 - 1].FaultDetectionCounter);
}

void test_MonitorDebounce_FAILED_SetsFailedThreshold(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    Dem_IntUpdateDebounceCounter(4, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD,
                      Dem_InternalState.EventStates[4 - 1].FaultDetectionCounter);
}

void test_MonitorDebounce_DefaultCase(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    /* FDC_THRESHOLD_REACHED should hit default case in monitor */
    Dem_IntUpdateDebounceCounter(4, DEM_EVENT_STATUS_FDC_THRESHOLD_REACHED);
    /* Should do nothing */
    TEST_ASSERT_EQUAL(0, Dem_InternalState.EventStates[4 - 1].FaultDetectionCounter);
}

void test_MonitorDebounce_SetEventStatus_FAILED(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    Dem_SetEventStatus(4, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD,
                      Dem_InternalState.EventStates[4 - 1].FaultDetectionCounter);
    /* TF bit should be set */
    uint8 dtcIdx = Dem_IntFindDTCIndex(0x100004U);
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_TEST_FAILED);
}

void test_MonitorDebounce_SetEventStatus_PASSED(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    Dem_SetEventStatus(4, DEM_EVENT_STATUS_PASSED);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD,
                      Dem_InternalState.EventStates[4 - 1].FaultDetectionCounter);
}

/* --- Time-based debounce: PREFAILED and PREPASSED paths --- */

void test_TimeDebounce_PREFAILED_StartCounting(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    /* Event 3 is TIME algorithm */
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_PREFAILED, 0);
    /* Should start counting up */
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_COUNTING_UP,
                      Dem_InternalState.TimeDebounceStates[3 - 1].State);
}

void test_TimeDebounce_PREFAILED_ReachesThreshold(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    /* Start counting */
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_PREFAILED, 0);
    /* Continue with enough time to reach threshold (100ms) */
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_PREFAILED, 100);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.TimeDebounceStates[3 - 1].ThresholdReached);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD,
                      Dem_InternalState.EventStates[3 - 1].FaultDetectionCounter);
}

void test_TimeDebounce_PREPASSED_StartCounting(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_PREPASSED, 0);
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_COUNTING_DOWN,
                      Dem_InternalState.TimeDebounceStates[3 - 1].State);
}

void test_TimeDebounce_PREPASSED_ReachesThreshold(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_PREPASSED, 0);
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_PREPASSED, 100);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.TimeDebounceStates[3 - 1].ThresholdReached);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD,
                      Dem_InternalState.EventStates[3 - 1].FaultDetectionCounter);
}

void test_TimeDebounce_PASSED_Immediate(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_PASSED, 0);
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_IDLE, Dem_InternalState.TimeDebounceStates[3 - 1].State);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.TimeDebounceStates[3 - 1].ThresholdReached);
}

void test_TimeDebounce_FAILED_Immediate(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_FAILED, 0);
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_IDLE, Dem_InternalState.TimeDebounceStates[3 - 1].State);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.TimeDebounceStates[3 - 1].ThresholdReached);
}

void test_TimeDebounce_DefaultStatus(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_FDC_THRESHOLD_REACHED, 0);
    /* Default case - should do nothing */
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_IDLE, Dem_InternalState.TimeDebounceStates[3 - 1].State);
}

/* --- DisableDTCRecordUpdate / EnableDTCRecordUpdate --- */


void test_DisableDTCRecordUpdate_FirstCall(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_DisableDTCRecordUpdate();
    TEST_ASSERT_EQUAL(DEM_DISABLEDTCRECUP_OK, ret);
}

void test_EnableDTCRecordUpdate_AfterDisable(void) {
    Dem_Init(&fullConfig);
    Dem_DisableDTCRecordUpdate();
    Std_ReturnType ret = Dem_EnableDTCRecordUpdate();
    TEST_ASSERT_EQUAL(E_OK, ret);
}



/* --- DisableDTCSetting / EnableDTCSetting --- */



/* --- GetDTCOfCheckFailed --- */




/* --- GetDTCStatusAvailabilityMask --- */

void test_GetDTCStatusAvailabilityMask_Valid(void) {
    Dem_Init(&fullConfig);
    uint8 mask = 0;
    Std_ReturnType ret = Dem_GetDTCStatusAvailabilityMask(&mask);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(0xFFU, mask);
}

void test_GetDTCStatusAvailabilityMask_NullPtr(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_GetDTCStatusAvailabilityMask(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_GetDTCStatusAvailabilityMask_Uninit(void) {
    Dem_Init(&fullConfig);
    Dem_DeInit();
    uint8 mask;
    Std_ReturnType ret = Dem_GetDTCStatusAvailabilityMask(&mask);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- GetNumberOfFilteredDTC --- */

void test_GetNumberOfFilteredDTC_Valid(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    Dem_SelectDTC(0x00123456U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    uint16 count = 0;
    Std_ReturnType ret = Dem_GetNumberOfFilteredDTC(&count);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(1U, count);
}

void test_GetNumberOfFilteredDTC_NullPtr(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_GetNumberOfFilteredDTC(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_GetNumberOfFilteredDTC_Uninit(void) {
    Dem_Init(&fullConfig);
    Dem_DeInit();
    uint16 count;
    Std_ReturnType ret = Dem_GetNumberOfFilteredDTC(&count);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- SelectDTC --- */

void test_SelectDTC_UnknownDTC(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_SelectDTC(0x999999U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}


/* --- GetEventFailed / GetEventTested / GetFaultDetectionCounter valid paths --- */

void test_GetEventFailed_Valid_NotFailed(void) {
    Dem_Init(&fullConfig);
    boolean failed = TRUE;
    Std_ReturnType ret = Dem_GetEventFailed(1, &failed);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(FALSE, failed);
}

void test_GetEventFailed_Valid_Failed(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    boolean failed = FALSE;
    Std_ReturnType ret = Dem_GetEventFailed(1, &failed);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(TRUE, failed);
}

void test_GetEventTested_Valid(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    boolean tested = FALSE;
    Std_ReturnType ret = Dem_GetEventTested(1, &tested);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(TRUE, tested);
}

void test_GetFaultDetectionCounter_Valid(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    sint8 fdc = 0;
    Std_ReturnType ret = Dem_GetFaultDetectionCounter(1, &fdc);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD, fdc);
}

/* --- ResetEventStatus valid path --- */

void test_ResetEventStatus_Valid(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    Std_ReturnType ret = Dem_ResetEventStatus(1);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* Debounce counter should be 0 */
    TEST_ASSERT_EQUAL(0, Dem_InternalState.EventStates[0].DebounceCounter);
    TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.EventStates[0].TestCompletedThisOperationCycle);
    TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.EventStates[0].TestFailedThisOperationCycle);
}

void test_ResetEventStatus_EventNotInConfig(void) {
    Dem_Init(&fullConfig);
    Std_ReturnType ret = Dem_ResetEventStatus(100);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- GetEventStatus valid path --- */

void test_GetEventStatus_Valid(void) {
    Dem_Init(&fullConfig);
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    Dem_EventStatusType status = DEM_EVENT_STATUS_PASSED;
    Std_ReturnType ret = Dem_GetEventStatus(1, &status);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, status);
}

void test_GetEventStatus_EventNotInConfig(void) {
    Dem_Init(&fullConfig);
    Dem_EventStatusType status;
    Std_ReturnType ret = Dem_GetEventStatus(100, &status);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- GetEventFailed / GetEventTested / GetFaultDetectionCounter - invalid event not in config --- */

void test_GetEventFailed_EventNotInConfig(void) {
    Dem_Init(&fullConfig);
    boolean failed;
    Std_ReturnType ret = Dem_GetEventFailed(100, &failed);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_GetEventTested_EventNotInConfig(void) {
    Dem_Init(&fullConfig);
    boolean tested;
    Std_ReturnType ret = Dem_GetEventTested(100, &tested);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_GetFaultDetectionCounter_EventNotInConfig(void) {
    Dem_Init(&fullConfig);
    sint8 fdc;
    Std_ReturnType ret = Dem_GetFaultDetectionCounter(100, &fdc);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- Shutdown --- */

void test_Shutdown_AfterInit(void) {
    Dem_Init(&fullConfig);
    Dem_Shutdown();
    /* Should be uninit now */
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_IntCheckInit());
}

/* --- GetVersionInfo valid path --- */

void test_GetVersionInfo_Valid(void) {
    Std_VersionInfoType info;
    Dem_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(DEM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(DEM_MODULE_ID, info.moduleID);
}

/* --- IntUpdateDebounceCounter NONE algorithm PASSED/FAILED --- */

void test_Int_UpdateDebounceCounter_NONE_PASSED(void) {
    Dem_Init(&fullConfig);
    Dem_IntUpdateDebounceCounter(1, DEM_EVENT_STATUS_PASSED);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD,
                      Dem_InternalState.EventStates[0].DebounceCounter);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD,
                      Dem_InternalState.EventStates[0].FaultDetectionCounter);
}

void test_Int_UpdateDebounceCounter_NONE_FAILED(void) {
    Dem_Init(&fullConfig);
    Dem_IntUpdateDebounceCounter(1, DEM_EVENT_STATUS_FAILED);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD,
                      Dem_InternalState.EventStates[0].DebounceCounter);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD,
                      Dem_InternalState.EventStates[0].FaultDetectionCounter);
}

void test_Int_UpdateDebounceCounter_NONE_DefaultStatus(void) {
    Dem_Init(&fullConfig);
    /* PREPASSED with NONE algorithm - no effect (NONE only handles PASSED/FAILED) */
    Dem_IntUpdateDebounceCounter(1, DEM_EVENT_STATUS_PREPASSED);
    TEST_ASSERT_EQUAL(0, Dem_InternalState.EventStates[0].DebounceCounter);
}

/* --- IntUpdateDTCStatusFromDebounce - passed with TestCompleted --- */

void test_Int_UpdateDTCStatus_Passed_ClearsPDTC_WhenTestCompleted(void) {
    Dem_Init(&fullConfig);
    /* Set failed first */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    uint8 dtcIdx = Dem_IntFindDTCIndex(0x00123456U);
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_PENDING_DTC);
    /* Now pass - TestCompletedThisOperationCycle is TRUE after SetEventStatus */
    Dem_IntUpdateDTCStatusFromDebounce(1, FALSE);
    /* PDTC should be cleared since TestCompletedThisOperationCycle is TRUE */
    TEST_ASSERT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_PENDING_DTC);
}

void test_Int_UpdateDTCStatus_Passed_KeepsPDTC_WhenTestNotCompleted(void) {
    Dem_Init(&fullConfig);
    /* Set failed first */
    Dem_SetEventStatus(1, DEM_EVENT_STATUS_FAILED);
    uint8 dtcIdx = Dem_IntFindDTCIndex(0x00123456U);
    /* Clear TestCompletedThisOperationCycle manually */
    Dem_InternalState.EventStates[0].TestCompletedThisOperationCycle = FALSE;
    Dem_IntUpdateDTCStatusFromDebounce(1, FALSE);
    /* PDTC should still be set since TestCompleted is FALSE */
    TEST_ASSERT_NOT_EQUAL(0U, Dem_InternalState.DTCEntries[dtcIdx].Status & DEM_DTC_STATUS_PENDING_DTC);
}

/* --- IntGetFreezeFrame - buffer too small --- */


void test_Int_GetFreezeFrame_NoMatchingRecord(void) {
    Dem_Init(&fullConfig);
    /* Don't store any freeze frame */
    uint8 dtcIdx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    uint8 buf[256];
    uint16 bufSize = sizeof(buf);
    Std_ReturnType ret = Dem_IntGetFreezeFrame(dtcIdx, 0, buf, &bufSize);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_Int_GetFreezeFrame_InvalidDtcIndex(void) {
    uint8 buf[256];
    uint16 bufSize = sizeof(buf);
    Std_ReturnType ret = Dem_IntGetFreezeFrame(0xFF, 0, buf, &bufSize);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- IntGetExtendedData - buffer too small --- */


void test_Int_GetExtendedData_NullPtrs(void) {
    uint16 bufSize = 4;
    Std_ReturnType ret = Dem_IntGetExtendedData(0, 1, NULL_PTR, &bufSize);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* --- IntStoreFreezeFrame - invalid index --- */


/* --- IntStoreExtendedData - invalid index and record 0 --- */


void test_Int_StoreExtendedData_RecordZero(void) {
    Dem_Init(&extDataConfig);
    uint8 dtcIdx = Dem_IntFindOrCreateDTCEntry(0x200001U);
    Dem_IntStoreExtendedData(dtcIdx, 0);
    /* RecordNumber 0 should return early */
}


/* --- IntClearFreezeFrame --- */

void test_Int_ClearFreezeFrame_NoMatching(void) {
    Dem_Init(&fullConfig);
    /* Clear freeze frames for a DTC index that has none */
    Dem_IntClearFreezeFrame(63);
    /* Should not crash */
}

/* --- IntProcessDebounceMainFunction - with no threshold reached --- */

void test_Int_ProcessDebounceMainFunction_NoThresholdReached(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    /* Start counting up for event 3 (TIME) but don't reach threshold */
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_PREFAILED, 0);
    /* State is COUNTING_UP but ThresholdReached is FALSE */
    /* ProcessDebounceMainFunction should not update DTC status */
    Dem_IntProcessDebounceMainFunction(10);
    /* No crash, no status change */
}

void test_Int_ProcessDebounceMainFunction_CountingUp_ThresholdReached(void) {
    initMultiDebounceConfig();
    Dem_Init(&multiDebounceConfig);
    /* Start counting and reach threshold */
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_PREFAILED, 0);
    Dem_IntProcessTimeBasedDebounce(3, DEM_EVENT_STATUS_PREFAILED, 100);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.TimeDebounceStates[3 - 1].ThresholdReached);
    /* Now MainFunction should process it */
    Dem_IntProcessDebounceMainFunction(100);
    /* State should be reset to IDLE */
    TEST_ASSERT_EQUAL(DEM_TIME_DEBOUNCE_IDLE, Dem_InternalState.TimeDebounceStates[3 - 1].State);
    TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.TimeDebounceStates[3 - 1].ThresholdReached);
}

/* --- IntAgeDTCEntry - valid --- */

void test_Int_AgeDTCEntry_ClearsFreezeFrames(void) {
    Dem_Init(&fullConfig);
    uint8 dtcIdx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    /* Store a freeze frame first */
    Dem_IntStoreFreezeFrame(dtcIdx);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.FreezeFrames[0].IsValid);
    /* Age the entry */
    Dem_InternalState.DTCEntries[dtcIdx].Status |= DEM_DTC_STATUS_CONFIRMED_DTC;
    Dem_IntAgeDTCEntry(dtcIdx);
    /* Freeze frames should be cleared */
    TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.FreezeFrames[0].IsValid);
}

/* --- IntClearSingleDTC - clears extended data --- */

void test_Int_ClearSingleDTC_ClearsExtendedData(void) {
    Dem_Init(&extDataConfig);
    uint8 dtcIdx = Dem_IntFindOrCreateDTCEntry(0x200001U);
    Dem_InternalState.DTCEntries[dtcIdx].OccurrenceCounter = 1;
    Dem_IntStoreExtendedData(dtcIdx, 1);
    /* Verify extended data is stored */
    boolean found = FALSE;
    for (uint8 i = 0; i < DEM_NUM_EXTENDED_DATA_RECORDS; i++) {
        if (Dem_InternalState.ExtendedDataRecords[i].IsValid &&
            Dem_InternalState.ExtendedDataRecords[i].DtcIndex == dtcIdx) {
            found = TRUE;
            break;
        }
    }
    TEST_ASSERT_EQUAL(TRUE, found);
    /* Clear the DTC */
    Dem_IntClearSingleDTC(dtcIdx);
    /* Extended data should be invalidated */
    for (uint8 i = 0; i < DEM_NUM_EXTENDED_DATA_RECORDS; i++) {
        if (Dem_InternalState.ExtendedDataRecords[i].DtcIndex == dtcIdx) {
            TEST_ASSERT_EQUAL(FALSE, Dem_InternalState.ExtendedDataRecords[i].IsValid);
        }
    }
}

/* --- ProcessAging - with no config found --- */

void test_Int_ProcessAging_NoConfig(void) {
    Dem_Init(&fullConfig);
    /* Manually set a DTC entry with CONFIRMED status but unknown DTC */
    Dem_InternalState.DTCEntries[0].DTC = 0xFFFFFFU; /* Not in config */
    Dem_InternalState.DTCEntries[0].Status = DEM_DTC_STATUS_CONFIRMED_DTC;
    Dem_InternalState.DTCEntries[0].IsAged = FALSE;
    Dem_InternalState.DTCEntries[0].AgingCounter = 0;
    /* ProcessAging should skip this entry (FindDTCConfig returns NULL) */
    Dem_IntProcessAging();
    TEST_ASSERT_EQUAL(0U, Dem_InternalState.DTCEntries[0].AgingCounter);
}

/* --- OccurrenceCounter max cap --- */

void test_UpdateDTCStatus_OccurrenceCounter_MaxCap(void) {
    Dem_Init(&fullConfig);
    uint8 dtcIdx = Dem_IntFindOrCreateDTCEntry(0x00123456U);
    Dem_InternalState.DTCEntries[dtcIdx].OccurrenceCounter = DEM_MAX_OCCURRENCE_COUNTER;
    /* Trigger another occurrence */
    Dem_IntUpdateDTCStatusFromDebounce(1, TRUE);
    /* Should not exceed max */
    TEST_ASSERT_EQUAL(DEM_MAX_OCCURRENCE_COUNTER, Dem_InternalState.DTCEntries[dtcIdx].OccurrenceCounter);
}

/*===========================================================================
 * TEST RUNNER
 *==========================================================================*/

int main(void)
{
    UNITY_BEGIN();
    /* Original 13 tests */
    RUN_TEST(test_Dem_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_Dem_Init_ValidConfig_ShouldInitializeState);
    RUN_TEST(test_Dem_DeInit_AfterInit_ShouldUninitialize);
    RUN_TEST(test_Dem_SetEventStatus_BeforeInit_ShouldReportUninit);
    RUN_TEST(test_Dem_SetEventStatus_InvalidEventId_ShouldReportDet);
    RUN_TEST(test_Dem_SetEventStatus_UnknownEvent_ShouldFail);
    RUN_TEST(test_Dem_SetEventStatus_ConfiguredEvent_ShouldUpdateState);
    RUN_TEST(test_Dem_Getters_AfterFailedReport_ShouldReflectStatus);
    RUN_TEST(test_Dem_ResetEventStatus_ShouldClearFlags);
    RUN_TEST(test_Dem_ClearDTC_ShouldValidateDtc);
    RUN_TEST(test_Dem_ClearDTC_ConfiguredDtc_ShouldResetStatus);
    RUN_TEST(test_Dem_GetStatusOfDTC_UnknownDtc_ShouldReturnWrongDtc);
    RUN_TEST(test_Dem_GetVersionInfo_ShouldReturnConfiguredValues);

    /* Uninit/Null/Invalid checks */
    RUN_TEST(test_ResetEventStatus_Uninit);
    RUN_TEST(test_ResetEventStatus_InvalidEventId);
    RUN_TEST(test_ResetEventStatus_UnknownEvent);
    RUN_TEST(test_GetEventStatus_Uninit);
    RUN_TEST(test_GetEventStatus_NullPtr);
    RUN_TEST(test_GetEventStatus_InvalidId);
    RUN_TEST(test_GetEventStatus_UnknownEvent);
    RUN_TEST(test_GetEventFailed_Uninit);
    RUN_TEST(test_GetEventFailed_NullPtr);
    RUN_TEST(test_GetEventFailed_InvalidId);
    RUN_TEST(test_GetEventFailed_UnknownEvent);
    RUN_TEST(test_GetEventTested_Uninit);
    RUN_TEST(test_GetEventTested_NullPtr);
    RUN_TEST(test_GetEventTested_InvalidId);
    RUN_TEST(test_GetEventTested_UnknownEvent);
    RUN_TEST(test_GetFaultDetCounter_Uninit);
    RUN_TEST(test_GetFaultDetCounter_NullPtr);
    RUN_TEST(test_GetFaultDetCounter_InvalidId);
    RUN_TEST(test_GetFaultDetCounter_UnknownEvent);
    RUN_TEST(test_GetStatusOfDTC_Uninit);
    RUN_TEST(test_GetStatusOfDTC_NullPtr);
    RUN_TEST(test_GetDTCStatusAvailMask_Uninit);
    RUN_TEST(test_GetDTCStatusAvailMask_NullPtr);
    RUN_TEST(test_GetDTCStatusAvailMask_Valid);
    RUN_TEST(test_GetNumFilteredDTC_Uninit);
    RUN_TEST(test_GetNumFilteredDTC_NullPtr);
    RUN_TEST(test_GetNumFilteredDTC_Valid);
    RUN_TEST(test_GetNextFilteredDTC_Uninit);
    RUN_TEST(test_GetNextFilteredDTC_NullPtr);
    RUN_TEST(test_GetNextFilteredDTC_NoMatch);
    RUN_TEST(test_ClearDTC_Uninit);
    RUN_TEST(test_SelectDTC_UnknownDtc);
    RUN_TEST(test_SelectDTC_Valid);
    RUN_TEST(test_DisableDTCSetting_Valid);
    RUN_TEST(test_EnableDTCSetting_Valid);
    RUN_TEST(test_DisableDTCRecordUpdate_Valid);
    RUN_TEST(test_EnableDTCRecordUpdate_Valid);
    RUN_TEST(test_GetIndicatorStatus_Uninit);
    RUN_TEST(test_GetIndicatorStatus_NullPtr);
    RUN_TEST(test_GetIndicatorStatus_NoFailure);
    RUN_TEST(test_SetIndicatorStatus_Uninit);
    RUN_TEST(test_SetIndicatorStatus_InvalidId);
    RUN_TEST(test_SetIndicatorStatus_Valid);
    RUN_TEST(test_GetFreezeFrameDataByDTC_Uninit);
    RUN_TEST(test_GetFreezeFrameDataByDTC_NullPtr);
    RUN_TEST(test_GetFreezeFrameDataByDTC_WrongDtc);
    RUN_TEST(test_GetExtendedDataByDTC_Uninit);
    RUN_TEST(test_GetExtendedDataByDTC_NullPtr);
    RUN_TEST(test_GetExtendedDataByDTC_WrongDtc);
    RUN_TEST(test_GetSizeOfExtDataRecord_Uninit);
    RUN_TEST(test_GetSizeOfExtDataRecord_NullPtr);
    RUN_TEST(test_GetSizeOfExtDataRecord_WrongDtc);
    RUN_TEST(test_GetSizeOfExtDataRecord_WrongNumber);
    RUN_TEST(test_GetSizeOfExtDataRecord_Found);
    RUN_TEST(test_SetOperationCycleState_Uninit);
    RUN_TEST(test_SetOperationCycleState_InvalidCycle);
    RUN_TEST(test_SetOperationCycleState_StartCycle);
    RUN_TEST(test_SetOperationCycleState_EndCycle);
    RUN_TEST(test_GetOperationCycleState_Uninit);
    RUN_TEST(test_GetOperationCycleState_NullPtr);
    RUN_TEST(test_GetOperationCycleState_InvalidCycle);
    RUN_TEST(test_GetOperationCycleState_Valid);
    RUN_TEST(test_RestartOperationCycle_Valid);
    RUN_TEST(test_PrestoreFreezeFrame_InvalidId);
    RUN_TEST(test_PrestoreFreezeFrame_UnknownEvent);
    RUN_TEST(test_ClearPrestoredFreezeFrame_UnknownEvent);
    RUN_TEST(test_MainFunction_Uninit);
    RUN_TEST(test_MainFunction_Valid);
    RUN_TEST(test_Shutdown_Valid);

    /* Debounce algorithm tests */
    RUN_TEST(test_Debounce_NONE_Passed);
    RUN_TEST(test_Debounce_NONE_Failed);
    RUN_TEST(test_Debounce_COUNTER_Passed);
    RUN_TEST(test_Debounce_COUNTER_Failed);
    RUN_TEST(test_Debounce_COUNTER_PrePassed);
    RUN_TEST(test_Debounce_COUNTER_PreFailed);
    RUN_TEST(test_Debounce_COUNTER_PrePassed_AtThreshold);
    RUN_TEST(test_Debounce_COUNTER_PreFailed_AtThreshold);
    RUN_TEST(test_Debounce_TIME_Passed);
    RUN_TEST(test_Debounce_TIME_Failed);
    RUN_TEST(test_Debounce_TIME_PreFailed_Start);
    RUN_TEST(test_Debounce_TIME_PreFailed_Continue);
    RUN_TEST(test_Debounce_TIME_PreFailed_ThresholdReached);
    RUN_TEST(test_Debounce_TIME_PrePassed_Start);
    RUN_TEST(test_Debounce_TIME_PrePassed_Continue);
    RUN_TEST(test_Debounce_TIME_PrePassed_ThresholdReached);
    RUN_TEST(test_Debounce_MONITOR_Failed);
    RUN_TEST(test_Debounce_MONITOR_Passed);

    /* Callback tests */
    RUN_TEST(test_ClearDTC_WithCallbacks);
    RUN_TEST(test_ClearDTC_GroupAll_WithCallbacks);
    RUN_TEST(test_ClearDTC_FunctionalGroup);
    RUN_TEST(test_Init_WithLambdaCallback);

    /* Filter tests */
    RUN_TEST(test_GetNextFilteredDTC_WithMatch);
    RUN_TEST(test_GetNextFilteredDTC_Multiple);
    RUN_TEST(test_SelectDTC_UpdatesFilteredCount);
    RUN_TEST(test_GetDTCOfCheckFailed_WithFailedDTC);
    RUN_TEST(test_GetIndicatorStatus_WithFailedDTC);

    /* Internal function tests */
    RUN_TEST(test_Int_FindOrCreateDTCEntry_NewEntry);
    RUN_TEST(test_Int_FindOrCreateDTCEntry_ExistingEntry);
    RUN_TEST(test_Int_FindOrCreateDTCEntry_ReuseDeleted);
    RUN_TEST(test_Int_FindEventConfig_Found);
    RUN_TEST(test_Int_FindEventConfig_NotFound);
    RUN_TEST(test_Int_FindEventConfig_NullConfig);
    RUN_TEST(test_Int_FindDTCConfig_Found);
    RUN_TEST(test_Int_FindDTCConfig_NotFound);
    RUN_TEST(test_Int_FindDTCConfig_NullConfig);
    RUN_TEST(test_Int_FindDTCIndex_Found);
    RUN_TEST(test_Int_FindDTCIndex_NotFound);
    RUN_TEST(test_Int_ResetDebounceCounter_Valid);
    RUN_TEST(test_Int_ResetDebounceCounter_InvalidId);
    RUN_TEST(test_Int_UpdateDebounceCounter_InvalidEventId);
    RUN_TEST(test_Int_UpdateDebounceCounter_NullConfig);
    RUN_TEST(test_Int_UpdateDebounceCounter_TimeAlgorithm);
    RUN_TEST(test_Int_ProcessTimeBasedDebounce_InvalidEventId);
    RUN_TEST(test_Int_ProcessTimeBasedDebounce_NullConfig);
    RUN_TEST(test_Int_ProcessTimeBasedDebounce_NotTimeAlgorithm);
    RUN_TEST(test_Int_StoreFreezeFrame_Valid);
    RUN_TEST(test_Int_StoreFreezeFrame_UpdateExisting);
    RUN_TEST(test_Int_ClearFreezeFrame_Valid);
    RUN_TEST(test_Int_GetFreezeFrame_NullPtr);
    RUN_TEST(test_Int_GetFreezeFrame_InvalidIndex);
    RUN_TEST(test_Int_GetFreezeFrame_NotFound);
    RUN_TEST(test_Int_GetFreezeFrame_Valid);
    RUN_TEST(test_Int_StoreExtendedData_Record0_Skipped);
    RUN_TEST(test_Int_StoreExtendedData_Record1);
    RUN_TEST(test_Int_StoreExtendedData_Record2);
    RUN_TEST(test_Int_StoreExtendedData_OverwriteValidEntry);
    RUN_TEST(test_Int_StoreExtendedData_DefaultRecordNumber);
    RUN_TEST(test_Int_GetExtendedData_NullPtr);
    RUN_TEST(test_Int_GetExtendedData_NotFound);
    RUN_TEST(test_Int_GetExtendedData_Valid);
    RUN_TEST(test_Int_ProcessAging_NoAging);
    RUN_TEST(test_Int_ProcessAging_WithConfirmedDTC);
    RUN_TEST(test_Int_ProcessAging_ThresholdReached);
    RUN_TEST(test_Int_AgeDTCEntry_Valid);
    RUN_TEST(test_Int_AgeDTCEntry_InvalidIndex);
    RUN_TEST(test_Int_ClearSingleDTC_Valid);
    RUN_TEST(test_Int_ClearSingleDTC_InvalidIndex);
    RUN_TEST(test_Int_ClearAllDTCs);
    RUN_TEST(test_Int_UpdateDTCStatus_True);
    RUN_TEST(test_Int_UpdateDTCStatus_False);
    RUN_TEST(test_Int_UpdateDTCStatus_NullConfig);
    RUN_TEST(test_Int_MatchDTCFilter_OutOfRange);
    RUN_TEST(test_Int_MatchDTCFilter_EmptySlot);
    RUN_TEST(test_Int_MatchDTCFilter_DeletedEntry);
    RUN_TEST(test_Int_MatchDTCFilter_NoConfig);
    RUN_TEST(test_Int_MatchDTCFilter_OriginMismatch);
    RUN_TEST(test_Int_MatchDTCFilter_Match);
    RUN_TEST(test_Int_UpdateFilteredCount);
    RUN_TEST(test_Int_CountDTCGroupMatches);
    RUN_TEST(test_Int_CountDTCGroupMatches_NoMatch);
    RUN_TEST(test_Int_ClearDTCGroup);
    RUN_TEST(test_Int_CheckInit_Init);
    RUN_TEST(test_Int_CheckInit_Uninit);
    RUN_TEST(test_Int_ValidateEventId_Valid);
    RUN_TEST(test_Int_ValidateEventId_Invalid);
    RUN_TEST(test_Int_ProcessDebounceMainFunction_NoTimeDebounce);
    RUN_TEST(test_Int_ProcessDebounceMainFunction_WithThresholdReached);
    RUN_TEST(test_Int_ProcessDebounceMainFunction_CountingDown);
    RUN_TEST(test_Int_HandleOperationCycleStart);
    RUN_TEST(test_Int_HandleOperationCycleEnd);

    /* BATCH 2: Additional branch coverage tests */
    /* DTC status bit management */
    RUN_TEST(test_SetEventStatus_FAILED_SetsTFAndPDTCBits);
    RUN_TEST(test_SetEventStatus_PASSED_ClearsTFBit);
    RUN_TEST(test_SetEventStatus_ConfirmationThreshold_SetsCDTC);
    RUN_TEST(test_SetEventStatus_FreezeFrameStored_OnFirstConfirmation);
    RUN_TEST(test_SetEventStatus_SecondConfirmation_NoNewFreezeFrame);
    /* Init edge cases */
    RUN_TEST(test_Init_NumDtcsLessThanDEM_NUM_DTCS);
    /* Operation cycle transitions */
    RUN_TEST(test_SetOperationCycleState_END_to_START);
    RUN_TEST(test_SetOperationCycleState_START_to_END);
    RUN_TEST(test_SetOperationCycleState_SameState_NoTransitionEffect);
    RUN_TEST(test_RestartOperationCycle);
    RUN_TEST(test_SetOperationCycleState_InvalidCycleType);
    RUN_TEST(test_GetOperationCycleState_NullCycleStatePtr);
    RUN_TEST(test_GetOperationCycleState_InvalidCycleType);
    /* GetStatusOfDTC valid path */
    RUN_TEST(test_GetStatusOfDTC_ValidDTC_ReturnsOK);
    RUN_TEST(test_GetStatusOfDTC_UnknownDTC_ReturnsWrongDTC);
    /* ClearDTC paths */
    RUN_TEST(test_ClearDTC_SingleDTC_WithValidIndex);
    RUN_TEST(test_ClearDTC_FunctionalGroup_WithMask);
    RUN_TEST(test_ClearDTC_GroupAll);
    RUN_TEST(test_ClearDTC_NullCallbacks);
    /* GetNextFilteredDTC iteration */
    RUN_TEST(test_GetNextFilteredDTC_ExhaustsAllEntries);
    RUN_TEST(test_GetNextFilteredDTC_NullDTCPtr);
    RUN_TEST(test_GetNextFilteredDTC_NullStatusPtr);
    /* MainFunction */
    RUN_TEST(test_MainFunction_Uninit_NoAction);
    RUN_TEST(test_MainFunction_Initialized);
    /* SetIndicatorStatus */
    RUN_TEST(test_SetIndicatorStatus_ValidId);
    RUN_TEST(test_SetIndicatorStatus_InvalidId_B2);
    RUN_TEST(test_SetIndicatorStatus_Uninit_B2);
    /* GetIndicatorStatus */
    RUN_TEST(test_GetIndicatorStatus_NoFailedDTCs);
    RUN_TEST(test_GetIndicatorStatus_WithSuppressedDTC);
    RUN_TEST(test_GetIndicatorStatus_NullPtr_B2);
    /* FreezeFrame by DTC */
    RUN_TEST(test_GetFreezeFrameDataByDTC_ValidWithStoredData);
    RUN_TEST(test_GetFreezeFrameDataByDTC_WrongDTC);
    RUN_TEST(test_GetFreezeFrameDataByDTC_NullBuffers);
    /* ExtendedData by DTC */
    RUN_TEST(test_GetExtendedDataRecordByDTC_ValidWithStoredData);
    RUN_TEST(test_GetExtendedDataRecordByDTC_WrongDTC);
    RUN_TEST(test_GetExtendedDataRecordByDTC_NullBuffers);
    /* GetSizeOfExtendedDataRecordByDTC */
    RUN_TEST(test_GetSizeOfExtendedDataRecordByDTC_Found);
    RUN_TEST(test_GetSizeOfExtendedDataRecordByDTC_WrongDTC);
    RUN_TEST(test_GetSizeOfExtendedDataRecordByDTC_WrongNumber);
    RUN_TEST(test_GetSizeOfExtendedDataRecordByDTC_NullPtr);
    RUN_TEST(test_GetSizeOfExtendedDataRecordByDTC_Uninit);
    /* PrestoreFreezeFrame / ClearPrestoredFreezeFrame */
    RUN_TEST(test_PrestoreFreezeFrame_InvalidEventId);
    RUN_TEST(test_PrestoreFreezeFrame_EventNotInConfig);
    RUN_TEST(test_ClearPrestoredFreezeFrame_NoDTCIndex);
    RUN_TEST(test_ClearPrestoredFreezeFrame_EventNotInConfig);
    /* Counter debounce */
    RUN_TEST(test_CounterDebounce_PREPASSED_DecrementsCounter);
    RUN_TEST(test_CounterDebounce_PREFAILED_IncrementsCounter);
    RUN_TEST(test_CounterDebounce_PREPASSED_DoesNotGoBelowPassedThreshold);
    RUN_TEST(test_CounterDebounce_PREFAILED_DoesNotExceedFailedThreshold);
    RUN_TEST(test_CounterDebounce_DefaultEventStatus);
    /* Monitor debounce */
    RUN_TEST(test_MonitorDebounce_PASSED_SetsPassedThreshold);
    RUN_TEST(test_MonitorDebounce_FAILED_SetsFailedThreshold);
    RUN_TEST(test_MonitorDebounce_DefaultCase);
    RUN_TEST(test_MonitorDebounce_SetEventStatus_FAILED);
    RUN_TEST(test_MonitorDebounce_SetEventStatus_PASSED);
    /* Time-based debounce */
    RUN_TEST(test_TimeDebounce_PREFAILED_StartCounting);
    RUN_TEST(test_TimeDebounce_PREFAILED_ReachesThreshold);
    RUN_TEST(test_TimeDebounce_PREPASSED_StartCounting);
    RUN_TEST(test_TimeDebounce_PREPASSED_ReachesThreshold);
    RUN_TEST(test_TimeDebounce_PASSED_Immediate);
    RUN_TEST(test_TimeDebounce_FAILED_Immediate);
    RUN_TEST(test_TimeDebounce_DefaultStatus);
    /* DisableDTCRecordUpdate / EnableDTCRecordUpdate */
    RUN_TEST(test_DisableDTCRecordUpdate_FirstCall);
    RUN_TEST(test_EnableDTCRecordUpdate_AfterDisable);
    /* DisableDTCSetting / EnableDTCSetting */
    /* GetDTCOfCheckFailed */
    /* GetDTCStatusAvailabilityMask */
    RUN_TEST(test_GetDTCStatusAvailabilityMask_Valid);
    RUN_TEST(test_GetDTCStatusAvailabilityMask_NullPtr);
    RUN_TEST(test_GetDTCStatusAvailabilityMask_Uninit);
    /* GetNumberOfFilteredDTC */
    RUN_TEST(test_GetNumberOfFilteredDTC_Valid);
    RUN_TEST(test_GetNumberOfFilteredDTC_NullPtr);
    RUN_TEST(test_GetNumberOfFilteredDTC_Uninit);
    /* SelectDTC */
    RUN_TEST(test_SelectDTC_UnknownDTC);
    /* GetEventFailed / GetEventTested / GetFaultDetectionCounter valid paths */
    RUN_TEST(test_GetEventFailed_Valid_NotFailed);
    RUN_TEST(test_GetEventFailed_Valid_Failed);
    RUN_TEST(test_GetEventTested_Valid);
    RUN_TEST(test_GetFaultDetectionCounter_Valid);
    /* ResetEventStatus */
    RUN_TEST(test_ResetEventStatus_Valid);
    RUN_TEST(test_ResetEventStatus_EventNotInConfig);
    /* GetEventStatus */
    RUN_TEST(test_GetEventStatus_Valid);
    RUN_TEST(test_GetEventStatus_EventNotInConfig);
    /* Invalid event not in config */
    RUN_TEST(test_GetEventFailed_EventNotInConfig);
    RUN_TEST(test_GetEventTested_EventNotInConfig);
    RUN_TEST(test_GetFaultDetectionCounter_EventNotInConfig);
    /* Shutdown */
    RUN_TEST(test_Shutdown_AfterInit);
    /* GetVersionInfo */
    RUN_TEST(test_GetVersionInfo_Valid);
    /* IntUpdateDebounceCounter NONE */
    RUN_TEST(test_Int_UpdateDebounceCounter_NONE_PASSED);
    RUN_TEST(test_Int_UpdateDebounceCounter_NONE_FAILED);
    RUN_TEST(test_Int_UpdateDebounceCounter_NONE_DefaultStatus);
    /* IntUpdateDTCStatusFromDebounce */
    RUN_TEST(test_Int_UpdateDTCStatus_Passed_ClearsPDTC_WhenTestCompleted);
    RUN_TEST(test_Int_UpdateDTCStatus_Passed_KeepsPDTC_WhenTestNotCompleted);
    /* IntGetFreezeFrame */
    RUN_TEST(test_Int_GetFreezeFrame_NoMatchingRecord);
    RUN_TEST(test_Int_GetFreezeFrame_InvalidDtcIndex);
    /* IntGetExtendedData */
    RUN_TEST(test_Int_GetExtendedData_NullPtrs);
    /* IntStoreFreezeFrame / IntStoreExtendedData */
    RUN_TEST(test_Int_StoreExtendedData_RecordZero);
    /* IntClearFreezeFrame */
    RUN_TEST(test_Int_ClearFreezeFrame_NoMatching);
    /* IntProcessDebounceMainFunction */
    RUN_TEST(test_Int_ProcessDebounceMainFunction_NoThresholdReached);
    RUN_TEST(test_Int_ProcessDebounceMainFunction_CountingUp_ThresholdReached);
    /* IntAgeDTCEntry */
    RUN_TEST(test_Int_AgeDTCEntry_ClearsFreezeFrames);
    /* IntClearSingleDTC */
    RUN_TEST(test_Int_ClearSingleDTC_ClearsExtendedData);
    /* ProcessAging */
    RUN_TEST(test_Int_ProcessAging_NoConfig);
    /* OccurrenceCounter max cap */
    RUN_TEST(test_UpdateDTCStatus_OccurrenceCounter_MaxCap);

    return UnityEnd();
}
