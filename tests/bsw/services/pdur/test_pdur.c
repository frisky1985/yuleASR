/**
 * @file test_pdur.c
 * @brief PduR (PDU Router) Unit Tests
 * @req SWS_PduR
 */

// @tests src/bsw/services/pdur/src/PduR.c  @tests src/bsw/services/pdur/include/PduR.h
#include "unity.h"
#include "PduR.h"

/* ------------------------------------------------------------------ */
/* DET mock                                                            */
/* ------------------------------------------------------------------ */
static uint8 mock_DetCalls = 0;
static uint8 mock_DetLastApiId = 0;
static uint8 mock_DetLastErrorId = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId; (void)InstanceId;
    mock_DetCalls++;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    return E_OK;
}

/* PduR.c reports PDUR_E_ROUTING_PATH_NOT_FOUND through a file-local alias (0x05U);
   it intentionally shadows PDUR_E_ROUTING_PATH_GROUP_INVALID in PduR.h. */
#define TEST_PDUR_E_ROUTING_PATH_NOT_FOUND   (0x05U)

/* ------------------------------------------------------------------ */
/* Static test configurations                                          */
/*                                                                     */
/* path 0: Com (SourcePduId 0) -> CanIf (DestPduId 3)                  */
/* path 1: CanIf (SourcePduId 1) -> Com (DestPduId 5)                  */
/* ------------------------------------------------------------------ */
static const PduR_DestPduConfigType testDestCanIf =
    { 3U, PDUR_MODULE_CANIF, PDUR_DESTPDU_PROCESSING_IMMEDIATE, 0U };
static const PduR_DestPduConfigType testDestCom =
    { 5U, PDUR_MODULE_COM, PDUR_DESTPDU_PROCESSING_IMMEDIATE, 0U };

static const PduR_RoutingPathConfigType testPaths[2] = {
    { { 0U, PDUR_MODULE_COM,   8U }, &testDestCanIf, 1U, PDUR_ROUTING_PATH_DIRECT, FALSE },
    { { 1U, PDUR_MODULE_CANIF, 8U }, &testDestCom,   1U, PDUR_ROUTING_PATH_DIRECT, FALSE }
};

static const PduR_ConfigType testConfig =
    { testPaths, 2U, NULL_PTR, 0U, TRUE, TRUE };
static const PduR_ConfigType emptyConfig =
    { NULL_PTR, 0U, NULL_PTR, 0U, TRUE, TRUE };

/* ------------------------------------------------------------------ */
/* Call recorders provided by stubs.c                                  */
/* ------------------------------------------------------------------ */
extern uint32 mock_CanIf_Transmit_Count;
extern PduIdType mock_CanIf_Transmit_LastPduId;
extern PduLengthType mock_CanIf_Transmit_LastSduLength;
extern uint32 mock_CanIf_CancelTransmit_Count;
extern PduIdType mock_CanIf_CancelTransmit_LastPduId;
extern uint32 mock_Com_RxIndication_Count;
extern PduIdType mock_Com_RxIndication_LastPduId;
extern uint32 mock_Com_TxConfirmation_Count;
extern PduIdType mock_Com_TxConfirmation_LastPduId;
extern Std_ReturnType mock_Com_TxConfirmation_LastResult;
extern uint32 mock_Com_TriggerTransmit_Count;
extern PduIdType mock_Com_TriggerTransmit_LastPduId;
extern uint32 mock_Dcm_RxIndication_Count;
extern uint32 mock_Dcm_TxConfirmation_Count;
extern uint32 mock_Dcm_TriggerTransmit_Count;

static void mock_reset(void) {
    mock_DetCalls = 0;
    mock_DetLastApiId = 0;
    mock_DetLastErrorId = 0;
    mock_CanIf_Transmit_Count = 0U;
    mock_CanIf_Transmit_LastPduId = 0;
    mock_CanIf_Transmit_LastSduLength = 0;
    mock_CanIf_CancelTransmit_Count = 0U;
    mock_CanIf_CancelTransmit_LastPduId = 0;
    mock_Com_RxIndication_Count = 0U;
    mock_Com_RxIndication_LastPduId = 0;
    mock_Com_TxConfirmation_Count = 0U;
    mock_Com_TxConfirmation_LastPduId = 0;
    mock_Com_TxConfirmation_LastResult = E_NOT_OK;
    mock_Com_TriggerTransmit_Count = 0U;
    mock_Com_TriggerTransmit_LastPduId = 0;
    mock_Dcm_RxIndication_Count = 0U;
    mock_Dcm_TxConfirmation_Count = 0U;
    mock_Dcm_TriggerTransmit_Count = 0U;
}

/* PduR_DeInit() on an uninitialized module only raises a DET that is
   cleared by the mock reset, so every test starts from a clean state
   regardless of execution order. */
static void ensure_uninit(void) {
    PduR_DeInit();
    mock_reset();
}

void setUp(void) { ensure_uninit(); }
void tearDown(void) {}

static void make_pdu8(PduInfoType* pdu, uint8* data) {
    pdu->SduDataPtr = data;
    pdu->SduLength = 8U;
    pdu->MetaDataPtr = NULL_PTR;
}

/** @req SWS_PduR_00001 */
void test_PduR_Init_NullPtr_ShouldReportDet(void) {
    PduR_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(PDUR_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(PDUR_E_PARAM_POINTER, mock_DetLastErrorId);

    /* Init must have been ignored: the module still behaves as uninitialized. */
    PduInfoType pdu; uint8 data[8] = {0u}; make_pdu8(&pdu, data);
    Std_ReturnType ret = PduR_Transmit(0U, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(2, mock_DetCalls);
    TEST_ASSERT_EQUAL(PDUR_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(PDUR_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_PduR_00001 */
void test_PduR_Init_ValidConfig_ShouldEnterInitState(void) {
    PduR_Init(&emptyConfig);
    /* No DET from Init itself. */
    TEST_ASSERT_EQUAL(0, mock_DetCalls);

    /* With an empty config every Tx request hits "routing path not found"
       (not "uninit"), which proves the module accepted the configuration. */
    PduInfoType pdu; uint8 data[8] = {0u}; make_pdu8(&pdu, data);
    Std_ReturnType ret = PduR_Transmit(0U, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(PDUR_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(TEST_PDUR_E_ROUTING_PATH_NOT_FOUND, mock_DetLastErrorId);
}

/** @req SWS_PduR_00002 */
void test_PduR_DeInit_AfterInit_ShouldReturnToUninit(void) {
    PduR_Init(&testConfig);
    PduR_DeInit();
    /* DeInit of an initialized module must be silent. */
    TEST_ASSERT_EQUAL(0, mock_DetCalls);

    /* After DeInit the module reports E_UNINIT again. */
    PduInfoType pdu; uint8 data[8] = {0u}; make_pdu8(&pdu, data);
    Std_ReturnType ret = PduR_Transmit(0U, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(PDUR_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(PDUR_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_PduR_00003 */
void test_PduR_Transmit_BeforeInit_ShouldFail(void) {
    PduInfoType pdu; uint8 data[8] = {0u}; make_pdu8(&pdu, data);
    Std_ReturnType ret = PduR_Transmit(0U, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(PDUR_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(PDUR_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_PduR_00003 */
void test_PduR_Transmit_NullPdu_ShouldFail(void) {
    PduR_Init(&testConfig);
    Std_ReturnType ret = PduR_Transmit(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(PDUR_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(PDUR_E_PARAM_POINTER, mock_DetLastErrorId);
    TEST_ASSERT_EQUAL(0U, mock_CanIf_Transmit_Count);
}

/** @req SWS_PduR_00004 */
void test_PduR_RxIndication_ShouldRouteToCom(void) {
    PduR_Init(&testConfig);
    PduInfoType pdu; uint8 data[8] = {0x99u}; make_pdu8(&pdu, data);
    PduR_RxIndication(1U, &pdu);
    /* Path 1 (CanIf source) must be routed to Com with DestPduId 5. */
    TEST_ASSERT_EQUAL(1U, mock_Com_RxIndication_Count);
    TEST_ASSERT_EQUAL(5, mock_Com_RxIndication_LastPduId);
    TEST_ASSERT_EQUAL(0U, mock_Dcm_RxIndication_Count);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_PduR_00005 */
void test_PduR_TxConfirmation_ShouldForwardToCom(void) {
    PduR_Init(&testConfig);
    PduR_TxConfirmation(1U, E_OK);
    /* Path 1 (CanIf source) confirmation must reach Com with DestPduId 5. */
    TEST_ASSERT_EQUAL(1U, mock_Com_TxConfirmation_Count);
    TEST_ASSERT_EQUAL(5, mock_Com_TxConfirmation_LastPduId);
    TEST_ASSERT_EQUAL(E_OK, mock_Com_TxConfirmation_LastResult);
    TEST_ASSERT_EQUAL(0U, mock_Dcm_TxConfirmation_Count);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_PduR_00006 */
void test_PduR_TriggerTransmit_ShouldForwardToCom(void) {
    PduR_Init(&testConfig);
    PduInfoType pdu; uint8 data[8] = {0u}; make_pdu8(&pdu, data);
    Std_ReturnType ret = PduR_TriggerTransmit(1U, &pdu);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_Com_TriggerTransmit_Count);
    TEST_ASSERT_EQUAL(5, mock_Com_TriggerTransmit_LastPduId);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_PduR_00007 */
void test_PduR_CancelTransmitRequest_ShouldCallCanIf(void) {
    PduR_Init(&testConfig);
    Std_ReturnType ret = PduR_CancelTransmitRequest(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* Path 0 (Com source) cancels via its CanIf destination (DestPduId 3). */
    TEST_ASSERT_EQUAL(1U, mock_CanIf_CancelTransmit_Count);
    TEST_ASSERT_EQUAL(3, mock_CanIf_CancelTransmit_LastPduId);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_PduR_00008 */
void test_PduR_CancelReceiveRequest_ShouldAlwaysFailSilently(void) {
    PduR_Init(&testConfig);
    Std_ReturnType ret = PduR_CancelReceiveRequest(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_PduR_00009 */
void test_PduR_EnableRouting_ShouldNotReportDet(void) {
    PduR_Init(&testConfig);
    PduR_EnableRouting(PDUR_ROUTING_PATH_GROUP_0);
    PduR_EnableRouting((uint8)(PDUR_NUMBER_OF_ROUTING_PATH_GROUPS - 1U));
    /* Out-of-range ids are guarded and must be ignored silently as well. */
    PduR_EnableRouting((uint8)PDUR_NUMBER_OF_ROUTING_PATH_GROUPS);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_PduR_00010 */
void test_PduR_DisableRouting_ShouldNotReportDet(void) {
    PduR_Init(&testConfig);
    PduR_DisableRouting(PDUR_ROUTING_PATH_GROUP_0);
    PduR_DisableRouting((uint8)(PDUR_NUMBER_OF_ROUTING_PATH_GROUPS - 1U));
    PduR_DisableRouting((uint8)PDUR_NUMBER_OF_ROUTING_PATH_GROUPS);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_PduR_00011 */
void test_PduR_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    PduR_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(PDUR_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(PDUR_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(PDUR_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(PDUR_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(PDUR_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_PduR_00011 */
void test_PduR_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    PduR_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
    TEST_ASSERT_EQUAL(PDUR_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(PDUR_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_PduR_Init_DoubleInit_ShouldStayInitialized(void) {
    PduR_Init(&testConfig);
    PduR_Init(&testConfig);
    /* Re-initialization is accepted without a DET. */
    TEST_ASSERT_EQUAL(0, mock_DetCalls);

    /* Routing still works afterwards: path 0 (Com source) goes to CanIf. */
    PduInfoType pdu; uint8 data[8] = {0u}; make_pdu8(&pdu, data);
    Std_ReturnType ret = PduR_Transmit(0U, &pdu);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(1U, mock_CanIf_Transmit_Count);
    TEST_ASSERT_EQUAL(3, mock_CanIf_Transmit_LastPduId);
    TEST_ASSERT_EQUAL(8, mock_CanIf_Transmit_LastSduLength);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_PduR_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_PduR_Init_ValidConfig_ShouldEnterInitState);
    RUN_TEST(test_PduR_DeInit_AfterInit_ShouldReturnToUninit);
    RUN_TEST(test_PduR_Transmit_BeforeInit_ShouldFail);
    RUN_TEST(test_PduR_Transmit_NullPdu_ShouldFail);
    RUN_TEST(test_PduR_RxIndication_ShouldRouteToCom);
    RUN_TEST(test_PduR_TxConfirmation_ShouldForwardToCom);
    RUN_TEST(test_PduR_TriggerTransmit_ShouldForwardToCom);
    RUN_TEST(test_PduR_CancelTransmitRequest_ShouldCallCanIf);
    RUN_TEST(test_PduR_CancelReceiveRequest_ShouldAlwaysFailSilently);
    RUN_TEST(test_PduR_EnableRouting_ShouldNotReportDet);
    RUN_TEST(test_PduR_DisableRouting_ShouldNotReportDet);
    RUN_TEST(test_PduR_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_PduR_GetVersionInfo_NullPtr_ShouldReportDet);
    RUN_TEST(test_PduR_Init_DoubleInit_ShouldStayInitialized);
    return UnityEnd();
}
