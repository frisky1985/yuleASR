/**
 * @file test_dcm.c
 * @brief Dcm (Diagnostic Communication Manager) Unit Tests — substantive assertions.
 * @req SWS_Dcm
 */
// @tests src/bsw/services/dcm/src/Dcm.c  @tests src/bsw/services/dcm/include/Dcm.h
#include "unity.h"
#include "Dcm.h"
#include "PduR.h"
#include "Dem.h"

/* ---- captured state from stubs.c (PduR_Transmit recorder, Dem_GetStatusOfDTC) ---- */
extern uint32 stub_PduR_Transmit_calls;
extern const uint8 *stub_PduR_lastPdu;
extern PduLengthType stub_PduR_lastLength;

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

static Dcm_ConfigType testConfig;
void setUp(void) {
    testConfig.NumDIDs = 0U;
    testConfig.NumRIDs = 0U;
    testConfig.DIDs = NULL_PTR;
    testConfig.RIDs = NULL_PTR;
    Dcm_Init(&testConfig);
    mock_DetCalls = 0;
    stub_PduR_Transmit_calls = 0U;
    stub_PduR_lastPdu = NULL_PTR;
}
void tearDown(void) {}

/** @req SWS_Dcm_00001 — Dcm_Init(NULL) must report DET DCM_E_PARAM_POINTER and stay uninitialized */
void test_Dcm_Init_NullPtr_ShouldReportDet(void) {
    /* setUp() initializes the module; deinitialize first so the failed
     * Dcm_Init(NULL) really leaves the module uninitialized */
    Dcm_DeInit();
    Dcm_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DCM_MODULE_ID, mock_lastModuleId);
    TEST_ASSERT_EQUAL(DCM_SID_INIT, mock_lastApiId);
    TEST_ASSERT_EQUAL(DCM_E_PARAM_POINTER, mock_lastErrorId);
    /* still uninitialized: GetSecurityLevel must fail and report DCM_E_UNINIT */
    uint8 level = 0xFFU;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dcm_GetSecurityLevel(&level));
    TEST_ASSERT_EQUAL(2, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x0DU, mock_lastApiId); /* Dcm.c reports GetSecurityLevel uninit with literal 0x0D */
    TEST_ASSERT_EQUAL(DCM_E_UNINIT, mock_lastErrorId);
}
/** @req SWS_Dcm_00001 — valid config: session is default, security is locked */
void test_Dcm_Init_ValidConfig_ShouldSetDefaults(void) {
    uint8 session = 0U;
    uint8 level = 0xFFU;
    TEST_ASSERT_EQUAL(E_OK, Dcm_GetSesCtrlType(&session));
    TEST_ASSERT_EQUAL_UINT8(DCM_DEFAULT_SESSION, session);
    TEST_ASSERT_EQUAL(E_OK, Dcm_GetSecurityLevel(&level));
    TEST_ASSERT_EQUAL_UINT8(DCM_SEC_LEV_LOCKED, level);
}
/** @req SWS_Dcm_00002 — after DeInit the module is uninitialized again */
void test_Dcm_DeInit_AfterInit_ShouldUninitialize(void) {
    Dcm_DeInit();
    uint8 session = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, Dcm_GetSesCtrlType(&session));
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL_HEX8(0x0CU, mock_lastApiId); /* Dcm.c reports GetSesCtrlType uninit with literal 0x0C */
    TEST_ASSERT_EQUAL(DCM_E_UNINIT, mock_lastErrorId);
}
/** @req SWS_Dcm_00003 — full version info contract */
void test_Dcm_GetVersionInfo_ValidPtr_ShouldReturnConfiguredValues(void) {
    Std_VersionInfoType info;
    info.vendorID = 0U; info.moduleID = 0U;
    info.sw_major_version = 0U; info.sw_minor_version = 0U; info.sw_patch_version = 0U;
    Dcm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(DCM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(DCM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(DCM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(DCM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(DCM_SW_PATCH_VERSION, info.sw_patch_version);
}
/** @req SWS_Dcm_00003 — NULL version pointer must report DET */
void test_Dcm_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Dcm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(DCM_SID_GETVERSIONINFO, mock_lastApiId);
    TEST_ASSERT_EQUAL(DCM_E_PARAM_POINTER, mock_lastErrorId);
}
/** @req SWS_Dcm_00003 — MainFunction must not raise DET when initialized */
void test_Dcm_MainFunction_AfterInit_ShouldRunSilently(void) {
    Dcm_MainFunction();
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}
/** @req SWS_Dcm_00008 — UDS 0x10 session switch drives state and emits positive response via PduR */
void test_Dcm_SessionControl_ShouldSwitchSessionAndTransmit(void) {
    uint8 diagData[2] = { 0x10U, DCM_EXTENDED_DIAGNOSTIC_SESSION };
    PduInfoType pdu;
    pdu.SduDataPtr = diagData;
    pdu.SduLength = 2U;
    Dcm_RxIndication(0U, &pdu);
    /* positive response goes through PduR_Transmit. NOTE: this SUT defines
     * DCM_E_POSITIVERESPONSE as 0x00 (Dcm.h line ~164), so the response SID
     * is the request SID (0x10) instead of the AUTOSAR-standard 0x50. */
    TEST_ASSERT_EQUAL(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_NOT_NULL(stub_PduR_lastPdu);
    TEST_ASSERT_EQUAL_HEX8(0x10U, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(DCM_EXTENDED_DIAGNOSTIC_SESSION, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_UINT32(6U, stub_PduR_lastLength); /* SID + session + P2(2) + P2*(2) */
    uint8 session = DCM_DEFAULT_SESSION;
    TEST_ASSERT_EQUAL(E_OK, Dcm_GetSesCtrlType(&session));
    TEST_ASSERT_EQUAL_UINT8(DCM_EXTENDED_DIAGNOSTIC_SESSION, session);
}
/** @req SWS_Dcm_00009 — ResetToDefaultSession restores default session and locked level */
void test_Dcm_ResetToDefaultSession_ShouldRestoreDefaults(void) {
    uint8 diagData[2] = { 0x10U, DCM_EXTENDED_DIAGNOSTIC_SESSION };
    PduInfoType pdu;
    pdu.SduDataPtr = diagData;
    pdu.SduLength = 2U;
    Dcm_RxIndication(0U, &pdu);
    Dcm_ResetToDefaultSession();
    uint8 session = 0U;
    uint8 level = 0xFFU;
    TEST_ASSERT_EQUAL(E_OK, Dcm_GetSesCtrlType(&session));
    TEST_ASSERT_EQUAL(E_OK, Dcm_GetSecurityLevel(&level));
    TEST_ASSERT_EQUAL_UINT8(DCM_DEFAULT_SESSION, session);
    TEST_ASSERT_EQUAL_UINT8(DCM_SEC_LEV_LOCKED, level);
}
/** @req SWS_Dcm_00005 — unknown service id must produce a negative response via PduR */
void test_Dcm_UnknownService_ShouldSendNegativeResponse(void) {
    uint8 diagData[1] = { 0x99U };
    PduInfoType pdu;
    pdu.SduDataPtr = diagData;
    pdu.SduLength = 1U;
    Dcm_RxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_NOT_NULL(stub_PduR_lastPdu);
    TEST_ASSERT_EQUAL_HEX8(0x7FU, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x99U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_HEX8(DCM_E_SERVICE_NOT_SUPPORTED, stub_PduR_lastPdu[2]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_Dcm_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_Dcm_Init_ValidConfig_ShouldSetDefaults);
    RUN_TEST(test_Dcm_DeInit_AfterInit_ShouldUninitialize);
    RUN_TEST(test_Dcm_GetVersionInfo_ValidPtr_ShouldReturnConfiguredValues);
    RUN_TEST(test_Dcm_GetVersionInfo_NullPtr_ShouldReportDet);
    RUN_TEST(test_Dcm_MainFunction_AfterInit_ShouldRunSilently);
    RUN_TEST(test_Dcm_SessionControl_ShouldSwitchSessionAndTransmit);
    RUN_TEST(test_Dcm_ResetToDefaultSession_ShouldRestoreDefaults);
    RUN_TEST(test_Dcm_UnknownService_ShouldSendNegativeResponse);
    return UnityEnd();
}
