/**
 * @file test_dem.c
 * @brief Dem (Diagnostic Event Manager) Unit Tests — substantive assertions.
 * @req SWS_Dem
 */
// @tests src/bsw/services/dem/src/Dem.c  @tests src/bsw/services/dem/include/Dem.h
#include "unity.h"
#include "Dem.h"
#include "Dem_Int.h"

/* ---- DET recorder (test-local mock; tests/mocks/mock_det.c is NOT linked) ---- */
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

/* ---- configs ---- */
static Dem_ConfigType emptyConfig; /* zero-init: no events, no DTCs */

static const Dem_EventParameterType eventParams[1] = {
    {
        1U,             /* EventId */
        0x00123456U,    /* Dtc */
        0U,             /* EventPriority */
        TRUE,           /* EventAvailable */
        TRUE,           /* EventReporting */
        0U, 0U,         /* failure cycle counter / confirmation threshold */
        DEM_DEBOUNCE_ALGORITHM_NONE,
        FALSE, FALSE, FALSE,
        127, -128,      /* counter thresholds */
        0U, 0U          /* time thresholds */
    }
};
static const Dem_DtcParameterType dtcParams[1] = {
    {
        0x00123456U,    /* Dtc */
        0U,             /* DtcSeverity */
        0U,             /* DtcFunctionalUnit */
        DEM_DTC_ORIGIN_PRIMARY_MEMORY,
        TRUE, TRUE,     /* available, reporting */
        1U,             /* AgingThreshold */
        FALSE           /* MemoryEntryOverflow */
    }
};
static const Dem_ConfigType fullConfig = {
    eventParams, 1U,
    dtcParams, 1U,
    NULL_PTR, 0U,   /* freeze frames */
    NULL_PTR, 0U,   /* extended data records */
    NULL_PTR, 0U,   /* indicators */
    TRUE, TRUE, TRUE, FALSE,
    0xFFU,
    FALSE, FALSE, FALSE, FALSE,
    NULL_PTR, NULL_PTR, NULL_PTR
};

void setUp(void) {
    Dem_Init(&emptyConfig);
    mock_DetCalls = 0;
}
void tearDown(void) {}

/** @req SWS_Dem_00001 — Dem_Init(NULL) reports DET and leaves state UNINIT */
void test_Dem_Init_NullPtr_ShouldReportDet(void) {
    /* setUp() initializes the module; deinitialize first so the failed
     * Dem_Init(NULL) really leaves the module uninitialized */
    Dem_DeInit();
    Dem_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_MODULE_ID, mock_lastModuleId);
    TEST_ASSERT_EQUAL(DEM_SID_INIT, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_PARAM_POINTER, mock_lastErrorId);
    TEST_ASSERT_EQUAL(DEM_STATE_UNINIT, Dem_InternalState.State);
}
/** @req SWS_Dem_00001 — valid empty config moves state to INIT */
void test_Dem_Init_ValidConfig_ShouldInitializeState(void) {
    TEST_ASSERT_EQUAL(DEM_STATE_INIT, Dem_InternalState.State);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_TRUE(&emptyConfig == Dem_InternalState.ConfigPtr);
}
/** @req SWS_Dem_00002 — DeInit returns state to UNINIT; a second DeInit reports DET */
void test_Dem_DeInit_AfterInit_ShouldUninitialize(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(DEM_STATE_UNINIT, Dem_InternalState.State);
    mock_DetCalls = 0;
    Dem_DeInit();
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_SID_SHUTDOWN, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_UNINIT, mock_lastErrorId);
}
/** @req SWS_Dem_00004 — SetEventStatus before init fails with DET E_UNINIT */
void test_Dem_SetEventStatus_BeforeInit_ShouldReportUninit(void) {
    Dem_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetEventStatus(1U, DEM_EVENT_STATUS_PASSED));
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_SID_SETEVENTSTATUS, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_UNINIT, mock_lastErrorId);
}
/** @req SWS_Dem_00004 — out-of-range EventId reports DET E_PARAM_EVENT_ID */
void test_Dem_SetEventStatus_InvalidEventId_ShouldReportDet(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetEventStatus(0U, DEM_EVENT_STATUS_PASSED));
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_SID_SETEVENTSTATUS, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_PARAM_EVENT_ID, mock_lastErrorId);
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetEventStatus(200U, DEM_EVENT_STATUS_FAILED));
    TEST_ASSERT_EQUAL(2, mock_DetCalls);
}
/** @req SWS_Dem_00004 — valid Id unknown to the config fails with E_NOT_OK + DET */
void test_Dem_SetEventStatus_UnknownEvent_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED));
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_SID_SETEVENTSTATUS, mock_lastApiId);
    TEST_ASSERT_EQUAL(DEM_E_PARAM_EVENT_ID, mock_lastErrorId);
}
/** @req SWS_Dem_00004 — configured event: FAILED status is stored in module state */
void test_Dem_SetEventStatus_ConfiguredEvent_ShouldUpdateState(void) {
    Dem_Init(&fullConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_OK, Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED));
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, Dem_InternalState.EventStates[0].LastReportedStatus);
    TEST_ASSERT_EQUAL(DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD,
                      Dem_InternalState.EventStates[0].FaultDetectionCounter);
    TEST_ASSERT_EQUAL(TRUE, Dem_InternalState.EventStates[0].TestCompletedThisOperationCycle);
}
/** @req SWS_Dem_00006/07/08/09 — getters reflect the reported FAILED status */
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
    TEST_ASSERT_EQUAL_INT8(DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD, fdc);

    /* unknown event id -> E_NOT_OK without touching the output value */
    status = DEM_EVENT_STATUS_FAILED;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_GetEventStatus(0U, &status));
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, status);
}
/** @req SWS_Dem_00005 — ResetEventStatus clears completed/tested flags */
void test_Dem_ResetEventStatus_ShouldClearFlags(void) {
    Dem_Init(&fullConfig);
    TEST_ASSERT_EQUAL(E_OK, Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED));
    TEST_ASSERT_EQUAL(E_OK, Dem_ResetEventStatus(1U));
    boolean tested = TRUE;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventTested(1U, &tested));
    TEST_ASSERT_EQUAL(FALSE, tested);
}
/** @req SWS_Dem_00013 — ClearDTC: unknown DTC reports DET E_PARAM_DATA; GROUP_ALL succeeds */
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
/** @req SWS_Dem_00010/13 — failed DTC sets TF; ClearDTC resets status bits */
void test_Dem_ClearDTC_ConfiguredDtc_ShouldResetStatus(void) {
    Dem_UdsStatusByteType status = 0U;
    Dem_Init(&fullConfig);
    TEST_ASSERT_EQUAL(E_OK, Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED));
    TEST_ASSERT_EQUAL(E_OK, Dem_GetStatusOfDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status));
    TEST_ASSERT_EQUAL(DEM_UDS_STATUS_TF, (status & DEM_UDS_STATUS_TF));
    TEST_ASSERT_EQUAL(E_OK,
        Dem_ClearDTC(0x00123456U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY));
    TEST_ASSERT_EQUAL(E_OK, Dem_GetStatusOfDTC(0x00123456U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status));
    /* TF bit cleared, TNCSLC bit set after ClearDTC */
    TEST_ASSERT_EQUAL(0, (status & DEM_UDS_STATUS_TF));
    TEST_ASSERT_EQUAL(DEM_UDS_STATUS_TNCSLC, (status & DEM_UDS_STATUS_TNCSLC));
}
/** @req SWS_Dem_00010 — unknown DTC yields DEM_STATUS_WRONG_DTC and zero status */
void test_Dem_GetStatusOfDTC_UnknownDtc_ShouldReturnWrongDtc(void) {
    Dem_UdsStatusByteType status = 0xFFU;
    TEST_ASSERT_EQUAL(DEM_STATUS_WRONG_DTC,
        Dem_GetStatusOfDTC(0x00999999U, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status));
    TEST_ASSERT_EQUAL_UINT8(0U, status);
}
/** @req SWS_Dem_00029 — full version info contract; NULL reports DET */
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

int main(void)
{
    UNITY_BEGIN();
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
    return UnityEnd();
}
