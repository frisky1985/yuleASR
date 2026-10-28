/**
 * @file test_dcm_uds_matrix.c
 * @brief Dcm UDS diagnostic test matrix (P1 Phase 8) — parameterized coverage
 *        of the main UDS services with positive and negative (NRC) paths.
 *
 * Covered SIDs:
 *   0x10 DiagnosticSessionControl (default / programming / extended / safety)
 *   0x11 ECUReset (hard / keyOffOn / soft + invalid type)
 *   0x14 ClearDiagnosticInformation
 *   0x19 ReadDTCInformation (reportNumberOfDTC, reportDTCByStatusMask,
 *                           reportDTCSnapshotRecord fallback, bad length)
 *   0x22 ReadDataByIdentifier
 *   0x23 ReadMemoryByAddress (unsupported -> NRC 0x11)
 *   0x27 SecurityAccess (requestSeed / sendKey / wrong key / attempt lockout)
 *   0x2E WriteDataByIdentifier (write + read-back, missing callbacks)
 *   0x2F IOControlByIdentifier (unsupported -> NRC 0x11)
 *   0x31 RoutineControl (start / stop / requestResults + error paths)
 *   0x34 RequestDownload / 0x36 TransferData / 0x37 RequestTransferExit
 *   0x3E TesterPresent (0x00 echo, 0x80 suppress, invalid sub)
 *
 * NOTE on this SUT: Dcm.h defines DCM_E_POSITIVERESPONSE as 0x00, therefore
 * a positive response repeats the request SID (0x10 instead of 0x50 etc.).
 * Negative responses are [0x7F, SID, NRC].
 *
 * Self-contained: embeds the PduR_Transmit recorder, the Det_ReportError
 * recorder and minimal Dem stubs (service_dem is NOT linked, mirroring the
 * tests/bsw/services/dcm pattern).
 *
 * @tests src/bsw/services/dcm/src/Dcm.c
 */
#include <string.h>
#include "unity.h"
#include "Dcm.h"
#include "PduR.h"
#include "Dem.h"

/*==============================================================================
 *                              STUBS
 *============================================================================*/

/* ---- PduR_Transmit recorder (single definition in this target) ---- */
uint32 stub_PduR_Transmit_calls = 0U;
const uint8 *stub_PduR_lastPdu = NULL_PTR;
PduIdType stub_PduR_lastTxPduId = 0U;
PduLengthType stub_PduR_lastLength = 0U;

Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    stub_PduR_Transmit_calls++;
    stub_PduR_lastTxPduId = TxPduId;
    if (PduInfoPtr != NULL_PTR)
    {
        stub_PduR_lastPdu = PduInfoPtr->SduDataPtr;
        stub_PduR_lastLength = PduInfoPtr->SduLength;
    }
    return E_OK;
}

/* ---- DET recorder (test-local; tests/mocks/mock_det.c NOT linked) ---- */
static uint8 mock_DetCalls = 0;
static uint8 mock_lastApiId = 0;
static uint8 mock_lastErrorId = 0;
static uint16 mock_lastModuleId = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)InstanceId;
    mock_DetCalls++;
    mock_lastModuleId = ModuleId;
    mock_lastApiId = ApiId;
    mock_lastErrorId = ErrorId;
    return E_OK;
}

/* ---- Dem stubs: configurable per-DTC status map ----
 * Dcm_ProcessReadDTCInformation iterates Dem_Config.DtcParameters[i].Dtc
 * (i < DEM_NUM_DTCS) and asks Dem_GetDTCStatus() for each entry. */
#define MATRIX_DTC_A                     (0x123456U)
#define MATRIX_DTC_B                     (0x456789U)
#define MATRIX_DTC_STATUS_A              (DEM_UDS_STATUS_TF | DEM_UDS_STATUS_CDTC) /* 0x09 */

static uint8 stub_Dem_statusA = MATRIX_DTC_STATUS_A;
static uint8 stub_Dem_statusB = 0U;

Std_ReturnType Dem_GetStatusOfDTC(Dem_DtcType DTC, Dem_DTCOriginType DTCOrigin, Dem_UdsStatusByteType* DTCStatus)
{
    (void)DTCOrigin;
    if (DTCStatus != NULL_PTR)
    {
        *DTCStatus = 0U;
    }
    if (DTC == MATRIX_DTC_A)
    {
        if (DTCStatus != NULL_PTR) { *DTCStatus = stub_Dem_statusA; }
        return E_OK;
    }
    if (DTC == MATRIX_DTC_B)
    {
        if (DTCStatus != NULL_PTR) { *DTCStatus = stub_Dem_statusB; }
        return E_OK;
    }
    /* DTC 0 fillers and unknown DTCs: not present in the event memory */
    return E_NOT_OK;
}

static const Dem_DtcParameterType matrix_DtcParameters[DEM_NUM_DTCS] = {
    { .Dtc = MATRIX_DTC_A },
    { .Dtc = MATRIX_DTC_B },
    /* remaining entries: Dtc = 0 (filler, rejected by the stub above) */
};
const Dem_ConfigType Dem_Config = {
    .DtcParameters = matrix_DtcParameters,
    .NumDtcs = DEM_NUM_DTCS
};

/*==============================================================================
 *                              TEST CONFIGURATION
 *============================================================================*/

#define MATRIX_DID_READABLE              (0xF190U)  /* default session, unlocked */
#define MATRIX_DID_WRITABLE              (0xF191U)  /* default session, unlocked */
#define MATRIX_DID_WRITE_FAILS           (0xF192U)  /* WriteDataFnc returns E_NOT_OK */
#define MATRIX_DID_NO_READ_FNC           (0x1234U)
#define MATRIX_DID_NO_WRITE_FNC          (0x1235U)
#define MATRIX_DID_EXTENDED_ONLY         (0x0202U)  /* extended session required */
#define MATRIX_DID_SECURED               (0x3610U)  /* security level 1 required */
#define MATRIX_DID_UNKNOWN               (0xDEADU)

#define MATRIX_RID_START_OK              (0xEF00U)
#define MATRIX_RID_START_FAILS           (0x0200U)
#define MATRIX_RID_EXTENDED_ONLY         (0xEF10U)  /* now has callbacks to exercise session-match path */
#define MATRIX_RID_STOP_FAILS            (0xEF11U)  /* StopFnc returns E_NOT_OK */
#define MATRIX_RID_RESULT_FAILS          (0xEF12U)  /* RequestResultFnc returns E_NOT_OK */
#define MATRIX_RID_SECURED               (0xEF20U)
#define MATRIX_RID_UNKNOWN               (0x4242U)

static uint8  matrix_F190_Data[4] = { 0x11U, 0x22U, 0x33U, 0x44U };
static uint16 matrix_F191_WriteLen = 0U;
static uint8  matrix_RidStartResp = 0x5AU;
static uint8  matrix_RidResultResp = 0xABU;
static uint16 matrix_RidStartCalls = 0U;
static uint16 matrix_RidStopCalls = 0U;
static uint16 matrix_RidResultCalls = 0U;

static Std_ReturnType matrix_Read_F190(uint8* Data)
{
    if (Data != NULL_PTR) { (void)memcpy(Data, matrix_F190_Data, sizeof(matrix_F190_Data)); }
    return E_OK;
}
static Std_ReturnType matrix_Write_F191(const uint8* Data, uint16 DataLength)
{
    matrix_F191_WriteLen = DataLength;
    (void)Data;
    return E_OK;
}
static Std_ReturnType matrix_Rid_Start(const uint8* RequestData, uint16 RequestDataLength,
                                       uint8* ResponseData, uint16* ResponseDataLength)
{
    (void)RequestData; (void)RequestDataLength;
    matrix_RidStartCalls++;
    if (ResponseData != NULL_PTR) { ResponseData[0] = matrix_RidStartResp; }
    if (ResponseDataLength != NULL_PTR) { *ResponseDataLength = 1U; }
    return E_OK;
}
static Std_ReturnType matrix_Rid_Stop(const uint8* RequestData, uint16 RequestDataLength,
                                      uint8* ResponseData, uint16* ResponseDataLength)
{
    (void)RequestData; (void)RequestDataLength; (void)ResponseData;
    matrix_RidStopCalls++;
    if (ResponseDataLength != NULL_PTR) { *ResponseDataLength = 0U; }
    return E_OK;
}
static Std_ReturnType matrix_Rid_Result(uint8* ResponseData, uint16* ResponseDataLength)
{
    matrix_RidResultCalls++;
    if (ResponseData != NULL_PTR) { ResponseData[0] = matrix_RidResultResp; }
    if (ResponseDataLength != NULL_PTR) { *ResponseDataLength = 1U; }
    return E_OK;
}
static Std_ReturnType matrix_Rid_Start_Fails(const uint8* RequestData, uint16 RequestDataLength,
                                             uint8* ResponseData, uint16* ResponseDataLength)
{
    (void)RequestData; (void)RequestDataLength; (void)ResponseData; (void)ResponseDataLength;
    return E_NOT_OK;
}
static Std_ReturnType matrix_Write_Fails(const uint8* Data, uint16 DataLength)
{
    (void)Data; (void)DataLength;
    return E_NOT_OK;
}
static Std_ReturnType matrix_Rid_Start_Ext(const uint8* RequestData, uint16 RequestDataLength,
                                           uint8* ResponseData, uint16* ResponseDataLength)
{
    (void)RequestData; (void)RequestDataLength;
    if (ResponseData != NULL_PTR) { ResponseData[0] = 0x77U; }
    if (ResponseDataLength != NULL_PTR) { *ResponseDataLength = 1U; }
    return E_OK;
}
static Std_ReturnType matrix_Rid_Stop_Fails(const uint8* RequestData, uint16 RequestDataLength,
                                            uint8* ResponseData, uint16* ResponseDataLength)
{
    (void)RequestData; (void)RequestDataLength; (void)ResponseData; (void)ResponseDataLength;
    return E_NOT_OK;
}
static Std_ReturnType matrix_Rid_Result_Fails(uint8* ResponseData, uint16* ResponseDataLength)
{
    (void)ResponseData; (void)ResponseDataLength;
    return E_NOT_OK;
}

static const Dcm_DIDConfigType matrix_DIDs[] = {
    { MATRIX_DID_READABLE,      4U,  DCM_DEFAULT_SESSION, DCM_SEC_LEV_LOCKED, matrix_Read_F190,     NULL_PTR },
    { MATRIX_DID_WRITABLE,      4U,  DCM_DEFAULT_SESSION, DCM_SEC_LEV_LOCKED, NULL_PTR,             matrix_Write_F191 },
    { MATRIX_DID_WRITE_FAILS,   2U,  DCM_DEFAULT_SESSION, DCM_SEC_LEV_LOCKED, NULL_PTR,             matrix_Write_Fails },
    { MATRIX_DID_NO_READ_FNC,   2U,  DCM_DEFAULT_SESSION, DCM_SEC_LEV_LOCKED, NULL_PTR,             NULL_PTR },
    { MATRIX_DID_NO_WRITE_FNC,  2U,  DCM_DEFAULT_SESSION, DCM_SEC_LEV_LOCKED, NULL_PTR,             NULL_PTR },
    { MATRIX_DID_EXTENDED_ONLY, 2U,  DCM_EXTENDED_DIAGNOSTIC_SESSION, DCM_SEC_LEV_LOCKED, NULL_PTR, NULL_PTR },
    { MATRIX_DID_SECURED,       2U,  DCM_DEFAULT_SESSION, 1U /* level 1 */,   NULL_PTR,             NULL_PTR }
};
static const Dcm_RIDConfigType matrix_RIDs[] = {
    { MATRIX_RID_START_OK,      DCM_DEFAULT_SESSION, DCM_SEC_LEV_LOCKED, matrix_Rid_Start,       matrix_Rid_Stop, matrix_Rid_Result },
    { MATRIX_RID_START_FAILS,   DCM_DEFAULT_SESSION, DCM_SEC_LEV_LOCKED, matrix_Rid_Start_Fails, NULL_PTR,        NULL_PTR },
    { MATRIX_RID_EXTENDED_ONLY, DCM_EXTENDED_DIAGNOSTIC_SESSION, DCM_SEC_LEV_LOCKED, matrix_Rid_Start_Ext, NULL_PTR, NULL_PTR },
    { MATRIX_RID_STOP_FAILS,    DCM_DEFAULT_SESSION, DCM_SEC_LEV_LOCKED, matrix_Rid_Start,       matrix_Rid_Stop_Fails, matrix_Rid_Result },
    { MATRIX_RID_RESULT_FAILS,  DCM_DEFAULT_SESSION, DCM_SEC_LEV_LOCKED, matrix_Rid_Start,       matrix_Rid_Stop, matrix_Rid_Result_Fails },
    { MATRIX_RID_SECURED,       DCM_DEFAULT_SESSION, 1U /* level 1 */,   NULL_PTR,               NULL_PTR,        NULL_PTR }
};
static Dcm_ConfigType matrix_Config;

/*==============================================================================
 *                              FIXTURE / HELPERS
 *============================================================================*/

void setUp(void)
{
    matrix_Config.NumProtocols = 1U;
    matrix_Config.NumDIDs = (uint8)(sizeof(matrix_DIDs) / sizeof(matrix_DIDs[0]));
    matrix_Config.NumRIDs = (uint8)(sizeof(matrix_RIDs) / sizeof(matrix_RIDs[0]));
    matrix_Config.DIDs = matrix_DIDs;
    matrix_Config.RIDs = matrix_RIDs;

    stub_Dem_statusA = MATRIX_DTC_STATUS_A;
    stub_Dem_statusB = 0U;
    matrix_RidStartCalls = 0U;
    matrix_RidStopCalls = 0U;
    matrix_RidResultCalls = 0U;
    mock_DetCalls = 0;
    stub_PduR_Transmit_calls = 0U;
    stub_PduR_lastPdu = NULL_PTR;

    Dcm_Init(&matrix_Config);
}
void tearDown(void) {}

static uint32 pdu_calls_before; /* snapshot taken by dcm_send() */

/* Feed a raw UDS request into the DCM (protocol/pdu id 0) */
static void dcm_send(const uint8* req, uint16 len)
{
    PduInfoType pdu;
    pdu.SduDataPtr = (uint8*)req;
    pdu.SduLength = len;
    pdu.MetaDataPtr = NULL_PTR;
    pdu_calls_before = stub_PduR_Transmit_calls;
    Dcm_RxIndication(0U, &pdu);
}

/* Assert the last dcm_send emitted exactly one negative response
 * [0x7F, SID, NRC] (delta-based, so multi-request tests stay valid) */
static void expect_negative(uint8 sid, uint8 nrc)
{
    TEST_ASSERT_EQUAL_UINT32(pdu_calls_before + 1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_NOT_NULL(stub_PduR_lastPdu);
    TEST_ASSERT_EQUAL_HEX8(0x7FU, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(sid, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_HEX8(nrc, stub_PduR_lastPdu[2]);
    TEST_ASSERT_EQUAL_UINT32(3U, stub_PduR_lastLength);
}

/*==============================================================================
 *                       0x10 DIAGNOSTIC SESSION CONTROL
 *============================================================================*/

void test_Uds10_DefaultSession_PositiveResponse(void)
{
    uint8 req[2] = { DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_DEFAULT_SESSION };
    uint8 session = 0U;
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(DCM_DEFAULT_SESSION, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_UINT32(6U, stub_PduR_lastLength); /* SID+sub+P2(2)+P2*(2) */
    TEST_ASSERT_EQUAL(E_OK, Dcm_GetSesCtrlType(&session));
    TEST_ASSERT_EQUAL_UINT8(DCM_DEFAULT_SESSION, session);
}

void test_Uds10_ProgrammingSession_PositiveResponse(void)
{
    uint8 req[2] = { DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_PROGRAMMING_SESSION };
    uint8 session = 0U;
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_HEX8(DCM_PROGRAMMING_SESSION, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL(E_OK, Dcm_GetSesCtrlType(&session));
    TEST_ASSERT_EQUAL_UINT8(DCM_PROGRAMMING_SESSION, session);
}

void test_Uds10_ExtendedSession_PositiveResponse(void)
{
    uint8 req[2] = { DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_EXTENDED_DIAGNOSTIC_SESSION };
    uint8 session = 0U;
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_HEX8(DCM_EXTENDED_DIAGNOSTIC_SESSION, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL(E_OK, Dcm_GetSesCtrlType(&session));
    TEST_ASSERT_EQUAL_UINT8(DCM_EXTENDED_DIAGNOSTIC_SESSION, session);
}

void test_Uds10_SafetySession_Accepted(void)
{
    uint8 req[2] = { DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_SAFETY_SYSTEM_DIAGNOSTIC_SESSION };
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(DCM_SAFETY_SYSTEM_DIAGNOSTIC_SESSION, stub_PduR_lastPdu[1]);
}

void test_Uds10_InvalidSession_NrcSubFunctionNotSupported(void)
{
    uint8 req[2] = { DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, 0x7FU };
    dcm_send(req, 2U);
    expect_negative(DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_E_SUBFUNCTION_NOT_SUPPORTED);
}

void test_Uds10_EmptyRequest_NrcIncorrectMessageLength(void)
{
    uint8 req[1] = { DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL };
    dcm_send(req, 1U);
    expect_negative(DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_E_INCORRECT_MESSAGE_LENGTH);
}

/*==============================================================================
 *                              0x11 ECU RESET
 *============================================================================*/

void test_Uds11_HardReset_PositiveResponse(void)
{
    uint8 req[2] = { DCM_UDS_SID_ECU_RESET, 0x01U };
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_ECU_RESET, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_UINT32(2U, stub_PduR_lastLength);
}

void test_Uds11_KeyOffOnReset_PositiveResponse(void)
{
    uint8 req[2] = { DCM_UDS_SID_ECU_RESET, 0x02U };
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_ECU_RESET, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x02U, stub_PduR_lastPdu[1]);
}

void test_Uds11_SoftReset_PositiveResponse(void)
{
    uint8 req[2] = { DCM_UDS_SID_ECU_RESET, 0x03U };
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_ECU_RESET, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x03U, stub_PduR_lastPdu[1]);
}

void test_Uds11_EnableRapidPowerShutdown_PositiveResponse(void)
{
    uint8 req[2] = { DCM_UDS_SID_ECU_RESET, 0x04U };
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_ECU_RESET, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x04U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_UINT32(2U, stub_PduR_lastLength);
}

void test_Uds11_DisableRapidPowerShutdown_PositiveResponse(void)
{
    uint8 req[2] = { DCM_UDS_SID_ECU_RESET, 0x05U };
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_HEX8(0x05U, stub_PduR_lastPdu[1]);
}

void test_Uds11_InvalidResetType_NrcSubFunctionNotSupported(void)
{
    uint8 req[2] = { DCM_UDS_SID_ECU_RESET, 0x42U };
    dcm_send(req, 2U);
    expect_negative(DCM_UDS_SID_ECU_RESET, DCM_E_SUBFUNCTION_NOT_SUPPORTED);
}

void test_Uds11_EmptyRequest_NrcIncorrectMessageLength(void)
{
    uint8 req[1] = { DCM_UDS_SID_ECU_RESET };
    dcm_send(req, 1U);
    expect_negative(DCM_UDS_SID_ECU_RESET, DCM_E_INCORRECT_MESSAGE_LENGTH);
}

/*==============================================================================
 *                    0x14 CLEAR DIAGNOSTIC INFORMATION
 *============================================================================*/

void test_Uds14_ClearAllDTCs_PositiveResponse(void)
{
    uint8 req[4] = { DCM_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION, 0xFFU, 0xFFU, 0xFFU };
    dcm_send(req, 4U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_lastLength); /* SID only */
}

void test_Uds14_ShortRequest_NrcIncorrectMessageLength(void)
{
    uint8 req[3] = { DCM_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION, 0x12U, 0x34U };
    dcm_send(req, 3U);
    expect_negative(DCM_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION, DCM_E_INCORRECT_MESSAGE_LENGTH);
}

/*==============================================================================
 *                     0x19 READ DTC INFORMATION
 *============================================================================*/

void test_Uds19_ReportNumberOfDTC_CountsMatchingStatus(void)
{
    uint8 req[3] = { DCM_UDS_SID_READ_DTC_INFORMATION, 0x01U, 0x09U /* TF|CDTC */ };
    dcm_send(req, 3U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_READ_DTC_INFORMATION, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, stub_PduR_lastPdu[1]);
    /* availability mask: DEM_DTC_STATUS_AVAILABILITY_MASK == 0xFF */
    TEST_ASSERT_EQUAL_HEX8(DEM_DTC_STATUS_AVAILABILITY_MASK, stub_PduR_lastPdu[2]);
    /* layout: [0x59, sub, availMask, format, cntHi, cntLo]
     * exactly one DTC (MATRIX_DTC_A) matches mask 0x09 */
    TEST_ASSERT_EQUAL_HEX8(0x00U, stub_PduR_lastPdu[3]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, stub_PduR_lastPdu[4]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, stub_PduR_lastPdu[5]);
    TEST_ASSERT_EQUAL_UINT32(6U, stub_PduR_lastLength);
}

void test_Uds19_ReportDTCByStatusMask_ReturnsMatchingDtc(void)
{
    uint8 req[3] = { DCM_UDS_SID_READ_DTC_INFORMATION, 0x02U, 0x08U /* CDTC */ };
    dcm_send(req, 3U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(0x02U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_HEX8(DEM_DTC_STATUS_AVAILABILITY_MASK, stub_PduR_lastPdu[2]);
    /* record: DTC(3) + status */
    TEST_ASSERT_EQUAL_HEX8(0x12U, stub_PduR_lastPdu[3]);
    TEST_ASSERT_EQUAL_HEX8(0x34U, stub_PduR_lastPdu[4]);
    TEST_ASSERT_EQUAL_HEX8(0x56U, stub_PduR_lastPdu[5]);
    TEST_ASSERT_EQUAL_HEX8(MATRIX_DTC_STATUS_A, stub_PduR_lastPdu[6]);
    TEST_ASSERT_EQUAL_UINT32(7U, stub_PduR_lastLength);
}

void test_Uds19_ReportDTCByStatusMask_StatusChange_IsReflected(void)
{
    uint8 req[3] = { DCM_UDS_SID_READ_DTC_INFORMATION, 0x02U, 0xFFU };
    /* B now confirmed too -> two records with mask 0xFF */
    stub_Dem_statusB = DEM_UDS_STATUS_CDTC;
    dcm_send(req, 3U);
    TEST_ASSERT_EQUAL_UINT32(7U + 4U, stub_PduR_lastLength);
    TEST_ASSERT_EQUAL_HEX8(0x45U, stub_PduR_lastPdu[7]);
    TEST_ASSERT_EQUAL_HEX8(0x67U, stub_PduR_lastPdu[8]);
    TEST_ASSERT_EQUAL_HEX8(0x89U, stub_PduR_lastPdu[9]);
    TEST_ASSERT_EQUAL_HEX8(DEM_UDS_STATUS_CDTC, stub_PduR_lastPdu[10]);
}

void test_Uds19_ReportSnapshotRecord_FallbackResponse(void)
{
    /* sub 0x04 falls into the simplified fallback: [sub, availMask, 0, 0, 0] */
    uint8 req[6] = { DCM_UDS_SID_READ_DTC_INFORMATION, 0x04U, 0x12U, 0x34U, 0x56U, 0xFFU };
    dcm_send(req, 6U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_READ_DTC_INFORMATION, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x04U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_HEX8(DEM_DTC_STATUS_AVAILABILITY_MASK, stub_PduR_lastPdu[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, stub_PduR_lastPdu[3]);
    TEST_ASSERT_EQUAL_UINT32(6U, stub_PduR_lastLength); /* SID+sub+avail+3 zeros */
}

void test_Uds19_ReportSupportedDTC_ReturnsAllConfigured(void)
{
    /* sub 0x0A reports every configured DTC — regardless of status */
    uint8 req[2] = { DCM_UDS_SID_READ_DTC_INFORMATION, 0x0AU };
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_READ_DTC_INFORMATION, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x0AU, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_HEX8(DEM_DTC_STATUS_AVAILABILITY_MASK, stub_PduR_lastPdu[2]);
    /* Should contain MATRIX_DTC_A (0x123456) and MATRIX_DTC_B (0x456789) */
    TEST_ASSERT_EQUAL_HEX8(0x12U, stub_PduR_lastPdu[3]);
    TEST_ASSERT_EQUAL_HEX8(0x34U, stub_PduR_lastPdu[4]);
    TEST_ASSERT_EQUAL_HEX8(0x56U, stub_PduR_lastPdu[5]);
    TEST_ASSERT_EQUAL_HEX8(MATRIX_DTC_STATUS_A, stub_PduR_lastPdu[6]);
    TEST_ASSERT_EQUAL_HEX8(0x45U, stub_PduR_lastPdu[7]);
    TEST_ASSERT_EQUAL_HEX8(0x67U, stub_PduR_lastPdu[8]);
    TEST_ASSERT_EQUAL_HEX8(0x89U, stub_PduR_lastPdu[9]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, stub_PduR_lastPdu[10]); /* status B = 0 */
    TEST_ASSERT_EQUAL_UINT32(11U, stub_PduR_lastLength);
}

void test_Uds19_EmptyRequest_NrcIncorrectMessageLength(void)
{
    uint8 req[1] = { DCM_UDS_SID_READ_DTC_INFORMATION };
    dcm_send(req, 1U);
    expect_negative(DCM_UDS_SID_READ_DTC_INFORMATION, DCM_E_INCORRECT_MESSAGE_LENGTH);
}

/*==============================================================================
 *                     0x22 / 0x2E DATA BY IDENTIFIER
 *============================================================================*/

void test_Uds22_ReadKnownDid_ReturnsData(void)
{
    uint8 req[3] = { DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, 0xF1U, 0x90U };
    dcm_send(req, 3U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0xF1U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_HEX8(0x90U, stub_PduR_lastPdu[2]);
    TEST_ASSERT_EQUAL_HEX8(0x11U, stub_PduR_lastPdu[3]);
    TEST_ASSERT_EQUAL_HEX8(0x44U, stub_PduR_lastPdu[6]);
    TEST_ASSERT_EQUAL_UINT32(7U, stub_PduR_lastLength); /* SID + DID(2) + data(4) */
}

void test_Uds22_UnknownDid_NrcRequestOutOfRange(void)
{
    uint8 req[3] = { DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, 0xDEU, 0xADU };
    dcm_send(req, 3U);
    expect_negative(DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, DCM_E_REQUEST_OUT_OF_RANGE);
}

void test_Uds22_ReadFncMissing_NrcConditionsNotCorrect(void)
{
    uint8 req[3] = { DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, 0x12U, 0x34U };
    dcm_send(req, 3U);
    expect_negative(DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, DCM_E_CONDITIONS_NOT_CORRECT);
}

void test_Uds22_ExtendedSessionDid_InDefaultSession_NrcServiceNotSupported(void)
{
    uint8 req[3] = { DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, 0x02U, 0x02U };
    dcm_send(req, 3U);
    expect_negative(DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, DCM_E_SERVICE_NOT_SUPPORTED);
}

void test_Uds22_ExtendedSessionDid_InExtendedSession_Succeeds(void)
{
    uint8 req[2] = { DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_EXTENDED_DIAGNOSTIC_SESSION };
    dcm_send(req, 2U);
    {
        uint8 reqDid[3] = { DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, 0x02U, 0x02U };
        dcm_send(reqDid, 3U);
        /* DID has no read callback -> conditions not correct, but the session
         * gate (NRC 0x11) has been passed, proving extended session access */
        expect_negative(DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, DCM_E_CONDITIONS_NOT_CORRECT);
    }
}

void test_Uds22_SecuredDid_LockedLevel_NrcSecurityAccessDenied(void)
{
    uint8 req[3] = { DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, 0x36U, 0x10U };
    dcm_send(req, 3U);
    expect_negative(DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, DCM_E_SECURITY_ACCESS_DENIED);
}

void test_Uds22_EmptyRequest_NrcIncorrectMessageLength(void)
{
    uint8 req[1] = { DCM_UDS_SID_READ_DATA_BY_IDENTIFIER };
    dcm_send(req, 1U);
    expect_negative(DCM_UDS_SID_READ_DATA_BY_IDENTIFIER, DCM_E_INCORRECT_MESSAGE_LENGTH);
}

void test_Uds2E_WriteKnownDid_PositiveResponseEchoesDid(void)
{
    uint8 req[6] = { DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER, 0xF1U, 0x91U, 0xAAU, 0xBBU, 0xCCU };
    dcm_send(req, 6U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0xF1U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_HEX8(0x91U, stub_PduR_lastPdu[2]);
    TEST_ASSERT_EQUAL_UINT32(3U, stub_PduR_lastLength);
    TEST_ASSERT_EQUAL_UINT16(3U, matrix_F191_WriteLen); /* callback got 3 data bytes */
}

void test_Uds2E_WriteFncMissing_NrcRequestOutOfRange(void)
{
    uint8 req[4] = { DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER, 0x12U, 0x35U, 0x00U };
    dcm_send(req, 4U);
    expect_negative(DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER, DCM_E_REQUEST_OUT_OF_RANGE);
}

void test_Uds2E_UnknownDid_NrcRequestOutOfRange(void)
{
    uint8 req[4] = { DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER, 0xDEU, 0xADU, 0x00U };
    dcm_send(req, 4U);
    expect_negative(DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER, DCM_E_REQUEST_OUT_OF_RANGE);
}

void test_Uds2E_WriteFncFails_NrcConditionsNotCorrect(void)
{
    /* DID 0xF192 has a WriteDataFnc that returns E_NOT_OK */
    uint8 req[4] = { DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER, 0xF1U, 0x92U, 0x00U };
    dcm_send(req, 4U);
    expect_negative(DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER, DCM_E_CONDITIONS_NOT_CORRECT);
}

void test_Uds2E_EmptyRequest_NrcIncorrectMessageLength(void)
{
    uint8 req[1] = { DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER };
    dcm_send(req, 1U);
    expect_negative(DCM_UDS_SID_WRITE_DATA_BY_IDENTIFIER, DCM_E_INCORRECT_MESSAGE_LENGTH);
}

/*==============================================================================
 *                0x23 / 0x2F SERVICES NOT SUPPORTED BY THIS DCM
 *============================================================================*/

void test_Uds23_ReadMemoryByAddress_NrcServiceNotSupported(void)
{
    uint8 req[4] = { DCM_UDS_SID_READ_MEMORY_BY_ADDRESS, 0x24U, 0x00U, 0x10U };
    dcm_send(req, 4U);
    expect_negative(DCM_UDS_SID_READ_MEMORY_BY_ADDRESS, DCM_E_SERVICE_NOT_SUPPORTED);
}

void test_Uds2F_IOControlByIdentifier_NrcServiceNotSupported(void)
{
    uint8 req[6] = { DCM_UDS_SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER, 0x12U, 0x34U, 0x03U, 0x01U, 0x40U };
    dcm_send(req, 6U);
    expect_negative(DCM_UDS_SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER, DCM_E_SERVICE_NOT_SUPPORTED);
}

/*==============================================================================
 *                        0x27 SECURITY ACCESS
 *============================================================================*/

void test_Uds27_RequestSeed_ReturnsSeedPattern(void)
{
    uint8 req[2] = { DCM_UDS_SID_SECURITY_ACCESS, 0x01U };
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_SECURITY_ACCESS, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, stub_PduR_lastPdu[1]);
    /* deterministic seed {0xA5, 0xA6, 0xA7, 0xA8} */
    TEST_ASSERT_EQUAL_HEX8(0xA5U, stub_PduR_lastPdu[2]);
    TEST_ASSERT_EQUAL_HEX8(0xA6U, stub_PduR_lastPdu[3]);
    TEST_ASSERT_EQUAL_HEX8(0xA7U, stub_PduR_lastPdu[4]);
    TEST_ASSERT_EQUAL_HEX8(0xA8U, stub_PduR_lastPdu[5]);
    TEST_ASSERT_EQUAL_UINT32(6U, stub_PduR_lastLength); /* SID + sub + seed(4) */
}

void test_Uds27_SendKey_CorrectKey_PositiveResponse(void)
{
    uint8 reqSeed[2] = { DCM_UDS_SID_SECURITY_ACCESS, 0x01U };
    /* sendKey is selected by bit 6 (0x40) of the sub-function, not by
     * odd/even parity: 0x42 = sendKey for level 0 */
    uint8 reqKey[6]  = { DCM_UDS_SID_SECURITY_ACCESS, 0x42U, 0xA5U, 0xA6U, 0xA7U, 0xA8U };
    dcm_send(reqSeed, 2U);
    dcm_send(reqKey, 6U);
    TEST_ASSERT_EQUAL_UINT32(2U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_SECURITY_ACCESS, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x42U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_UINT32(2U, stub_PduR_lastLength);
}

void test_Uds27_SendKey_WrongKey_NrcInvalidKey(void)
{
    uint8 reqKey[6] = { DCM_UDS_SID_SECURITY_ACCESS, 0x42U, 0x00U, 0x00U, 0x00U, 0x00U };
    dcm_send(reqKey, 6U);
    expect_negative(DCM_UDS_SID_SECURITY_ACCESS, DCM_E_INVALID_KEY);
}

void test_Uds27_ThreeFailedAttempts_LocksSeedRequest(void)
{
    uint8 reqKey[6] = { DCM_UDS_SID_SECURITY_ACCESS, 0x42U, 0x00U, 0x00U, 0x00U, 0x00U };
    uint8 reqSeed[2] = { DCM_UDS_SID_SECURITY_ACCESS, 0x01U };
    uint8 i;
    for (i = 0U; i < DCM_MAX_SECURITY_ATTEMPTS; i++)
    {
        dcm_send(reqKey, 6U); /* NRC 0x35 each time */
    }
    /* after DCM_MAX_SECURITY_ATTEMPTS failures the seed request is refused */
    dcm_send(reqSeed, 2U);
    expect_negative(DCM_UDS_SID_SECURITY_ACCESS, DCM_E_EXCEED_NUMBER_OF_ATTEMPTS);
}

void test_Uds27_Lockout_ActivatesTimeDelay(void)
{
    uint8 reqKey[6] = { DCM_UDS_SID_SECURITY_ACCESS, 0x42U, 0x00U, 0x00U, 0x00U, 0x00U };
    uint8 reqSeed[2] = { DCM_UDS_SID_SECURITY_ACCESS, 0x01U };
    uint8 i;
    for (i = 0U; i < DCM_MAX_SECURITY_ATTEMPTS; i++)
    {
        dcm_send(reqKey, 6U);
    }
    (void)dcm_send(reqSeed, 2U);            /* -> NRC 0x36, starts delay */
    (void)dcm_send(reqSeed, 2U);            /* delay active -> NRC 0x37 */
    expect_negative(DCM_UDS_SID_SECURITY_ACCESS, DCM_E_REQUIRED_TIME_DELAY_NOT_EXPIRED);
}

void test_Uds27_EmptyRequest_NrcIncorrectMessageLength(void)
{
    uint8 req[1] = { DCM_UDS_SID_SECURITY_ACCESS };
    dcm_send(req, 1U);
    expect_negative(DCM_UDS_SID_SECURITY_ACCESS, DCM_E_INCORRECT_MESSAGE_LENGTH);
}

/*==============================================================================
 *                       0x31 ROUTINE CONTROL
 *============================================================================*/

void test_Uds31_StartRoutine_PositiveResponseWithRoutineResult(void)
{
    uint8 req[4] = { DCM_UDS_SID_ROUTINE_CONTROL, 0x01U, 0xEFU, 0x00U };
    dcm_send(req, 4U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX16(1U, matrix_RidStartCalls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_ROUTINE_CONTROL, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_HEX8(0xEFU, stub_PduR_lastPdu[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, stub_PduR_lastPdu[3]);
    TEST_ASSERT_EQUAL_HEX8(matrix_RidStartResp, stub_PduR_lastPdu[4]);
    TEST_ASSERT_EQUAL_UINT32(5U, stub_PduR_lastLength); /* SID + sub + RID(2) + result(1) */
}

void test_Uds31_StopRoutine_PositiveResponse(void)
{
    uint8 req[4] = { DCM_UDS_SID_ROUTINE_CONTROL, 0x02U, 0xEFU, 0x00U };
    dcm_send(req, 4U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX16(1U, matrix_RidStopCalls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_ROUTINE_CONTROL, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x02U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_UINT32(4U, stub_PduR_lastLength); /* no routine result payload */
}

void test_Uds31_RequestRoutineResults_PositiveResponse(void)
{
    uint8 req[4] = { DCM_UDS_SID_ROUTINE_CONTROL, 0x03U, 0xEFU, 0x00U };
    dcm_send(req, 4U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX16(1U, matrix_RidResultCalls);
    TEST_ASSERT_EQUAL_HEX8(0x03U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_HEX8(matrix_RidResultResp, stub_PduR_lastPdu[4]);
    TEST_ASSERT_EQUAL_UINT32(5U, stub_PduR_lastLength);
}

void test_Uds31_UnknownRid_NrcRequestOutOfRange(void)
{
    uint8 req[4] = { DCM_UDS_SID_ROUTINE_CONTROL, 0x01U, 0x42U, 0x42U };
    dcm_send(req, 4U);
    expect_negative(DCM_UDS_SID_ROUTINE_CONTROL, DCM_E_REQUEST_OUT_OF_RANGE);
}

void test_Uds31_InvalidSubFunction_NrcSubFunctionNotSupported(void)
{
    uint8 req[4] = { DCM_UDS_SID_ROUTINE_CONTROL, 0x7EU, 0xEFU, 0x00U };
    dcm_send(req, 4U);
    expect_negative(DCM_UDS_SID_ROUTINE_CONTROL, DCM_E_SUBFUNCTION_NOT_SUPPORTED);
}

void test_Uds31_StartFncFails_NrcConditionsNotCorrect(void)
{
    uint8 req[4] = { DCM_UDS_SID_ROUTINE_CONTROL, 0x01U, 0x02U, 0x00U };
    dcm_send(req, 4U);
    expect_negative(DCM_UDS_SID_ROUTINE_CONTROL, DCM_E_CONDITIONS_NOT_CORRECT);
}

void test_Uds31_SecuredRid_LockedLevel_NrcSecurityAccessDenied(void)
{
    uint8 req[4] = { DCM_UDS_SID_ROUTINE_CONTROL, 0x01U, 0xEFU, 0x20U };
    dcm_send(req, 4U);
    expect_negative(DCM_UDS_SID_ROUTINE_CONTROL, DCM_E_SECURITY_ACCESS_DENIED);
}

void test_Uds31_ExtendedSessionRid_InDefaultSession_NrcServiceNotSupported(void)
{
    /* MATRIX_RID_EXTENDED_ONLY (0xEF10) requires extended session */
    uint8 req[4] = { DCM_UDS_SID_ROUTINE_CONTROL, 0x01U, 0xEFU, 0x10U };
    dcm_send(req, 4U);
    expect_negative(DCM_UDS_SID_ROUTINE_CONTROL, DCM_E_SERVICE_NOT_SUPPORTED);
}

void test_Uds31_ExtendedSessionRid_InExtendedSession_StartSucceeds(void)
{
    /* First switch to extended session */
    uint8 reqSes[2] = { DCM_UDS_SID_DIAGNOSTIC_SESSION_CONTROL, DCM_EXTENDED_DIAGNOSTIC_SESSION };
    uint8 req[4];
    dcm_send(reqSes, 2U);
    TEST_ASSERT_EQUAL_HEX8(DCM_EXTENDED_DIAGNOSTIC_SESSION, stub_PduR_lastPdu[1]);

    /* Now start the extended-session-only RID (0xEF10) */
    req[0] = DCM_UDS_SID_ROUTINE_CONTROL;
    req[1] = 0x01U;
    req[2] = (uint8)(MATRIX_RID_EXTENDED_ONLY >> 8);
    req[3] = (uint8)(MATRIX_RID_EXTENDED_ONLY & 0xFFU);
    dcm_send(req, 4U);
    TEST_ASSERT_EQUAL_UINT32(2U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_ROUTINE_CONTROL, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x77U, stub_PduR_lastPdu[4]); /* StartFnc response */
    TEST_ASSERT_EQUAL_UINT32(5U, stub_PduR_lastLength);

    /* Reset back to default for subsequent tests */
    Dcm_ResetToDefaultSession();
}

void test_Uds31_StopFncFails_NrcConditionsNotCorrect(void)
{
    /* RID 0xEF11 has StopFnc that returns E_NOT_OK */
    uint8 req[4] = { DCM_UDS_SID_ROUTINE_CONTROL, 0x02U, 0xEFU, 0x11U };
    dcm_send(req, 4U);
    expect_negative(DCM_UDS_SID_ROUTINE_CONTROL, DCM_E_CONDITIONS_NOT_CORRECT);
}

void test_Uds31_ResultFncFails_NrcConditionsNotCorrect(void)
{
    /* RID 0xEF12 has RequestResultFnc that returns E_NOT_OK */
    uint8 req[4] = { DCM_UDS_SID_ROUTINE_CONTROL, 0x03U, 0xEFU, 0x12U };
    dcm_send(req, 4U);
    expect_negative(DCM_UDS_SID_ROUTINE_CONTROL, DCM_E_CONDITIONS_NOT_CORRECT);
}

void test_Uds31_EmptyRequest_NrcIncorrectMessageLength(void)
{
    uint8 req[1] = { DCM_UDS_SID_ROUTINE_CONTROL };
    dcm_send(req, 1U);
    expect_negative(DCM_UDS_SID_ROUTINE_CONTROL, DCM_E_INCORRECT_MESSAGE_LENGTH);
}

/*==============================================================================
 *            0x34 / 0x36 / 0x37 DOWNLOAD SEQUENCE (STATE MACHINE)
 *============================================================================*/

void test_Uds36_TransferWithoutDownload_NrcRequestSequenceError(void)
{
    uint8 req[2] = { DCM_UDS_SID_TRANSFER_DATA, 0x01U };
    dcm_send(req, 2U);
    expect_negative(DCM_UDS_SID_TRANSFER_DATA, DCM_E_REQUESTSEQUENCEERROR);
}

void test_Uds37_ExitWithoutDownload_NrcRequestSequenceError(void)
{
    uint8 req[1] = { DCM_UDS_SID_REQUEST_TRANSFER_EXIT };
    dcm_send(req, 1U);
    expect_negative(DCM_UDS_SID_REQUEST_TRANSFER_EXIT, DCM_E_REQUESTSEQUENCEERROR);
}

void test_Uds34_RequestDownload_PositiveResponseWithBlockLength(void)
{
    /* dataFormat=0x00, addrAndLenFormat=0x11 (1-byte addr, 1-byte size),
     * address=0x20, size=0x04 */
    uint8 req[5] = { DCM_UDS_SID_REQUEST_DOWNLOAD, 0x00U, 0x11U, 0x20U, 0x04U };
    dcm_send(req, 5U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_REQUEST_DOWNLOAD, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x20U, stub_PduR_lastPdu[1]); /* lengthFormatIdentifier */
    /* maxNumberOfBlockLength = DCM_TRANSFER_BLOCK_SIZE (1024 = 0x0400) */
    TEST_ASSERT_EQUAL_HEX8((uint8)(DCM_TRANSFER_BLOCK_SIZE >> 8), stub_PduR_lastPdu[3]);
    TEST_ASSERT_EQUAL_HEX8((uint8)(DCM_TRANSFER_BLOCK_SIZE & 0xFFU), stub_PduR_lastPdu[4]);
    TEST_ASSERT_EQUAL_UINT32(5U, stub_PduR_lastLength);
}

void test_Uds34_ShortRequest_NrcIncorrectMessageLength(void)
{
    uint8 req[3] = { DCM_UDS_SID_REQUEST_DOWNLOAD, 0x00U, 0x11U };
    dcm_send(req, 3U);
    expect_negative(DCM_UDS_SID_REQUEST_DOWNLOAD, DCM_E_INCORRECT_MESSAGE_LENGTH);
}

void test_Uds36_FirstBlockMustBeOne_NrcWrongBlockSequenceCounter(void)
{
    uint8 reqDl[5] = { DCM_UDS_SID_REQUEST_DOWNLOAD, 0x00U, 0x11U, 0x20U, 0x04U };
    uint8 req[2]   = { DCM_UDS_SID_TRANSFER_DATA, 0x02U };
    dcm_send(reqDl, 5U);
    dcm_send(req, 2U);
    expect_negative(DCM_UDS_SID_TRANSFER_DATA, DCM_E_WRONGBLOCKSEQUENCECOUNTER);
}

void test_Uds36_SequentialBlocks_AcknowledgedWithBsc(void)
{
    uint8 reqDl[5] = { DCM_UDS_SID_REQUEST_DOWNLOAD, 0x00U, 0x11U, 0x20U, 0x04U };
    uint8 b1[3] = { DCM_UDS_SID_TRANSFER_DATA, 0x01U, 0xAAU };
    uint8 b2[3] = { DCM_UDS_SID_TRANSFER_DATA, 0x02U, 0xBBU };
    dcm_send(reqDl, 5U);
    dcm_send(b1, 3U);
    TEST_ASSERT_EQUAL_UINT32(2U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_TRANSFER_DATA, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_UINT32(2U, stub_PduR_lastLength);
    dcm_send(b2, 3U);
    TEST_ASSERT_EQUAL_HEX8(0x02U, stub_PduR_lastPdu[1]);
}

void test_Uds37_AfterDownload_FinishesTransfer(void)
{
    uint8 reqDl[5] = { DCM_UDS_SID_REQUEST_DOWNLOAD, 0x00U, 0x11U, 0x20U, 0x04U };
    uint8 exit[1]  = { DCM_UDS_SID_REQUEST_TRANSFER_EXIT };
    uint8 req[2]   = { DCM_UDS_SID_TRANSFER_DATA, 0x01U };
    dcm_send(reqDl, 5U);
    dcm_send(exit, 1U);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_REQUEST_TRANSFER_EXIT, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_lastLength);
    /* transfer is closed now: another TransferData is a sequence error */
    dcm_send(req, 2U);
    expect_negative(DCM_UDS_SID_TRANSFER_DATA, DCM_E_REQUESTSEQUENCEERROR);
}

/*==============================================================================
 *                            0x3E TESTER PRESENT
 *============================================================================*/

void test_Uds3E_ZeroSubFunction_PositiveResponseEcho(void)
{
    uint8 req[2] = { DCM_UDS_SID_TESTER_PRESENT, 0x00U };
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(DCM_UDS_SID_TESTER_PRESENT, stub_PduR_lastPdu[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_UINT32(2U, stub_PduR_lastLength);
}

void test_Uds3E_SuppressBit_NoResponseSent(void)
{
    uint8 req[2] = { DCM_UDS_SID_TESTER_PRESENT, 0x80U };
    dcm_send(req, 2U);
    TEST_ASSERT_EQUAL_UINT32(0U, stub_PduR_Transmit_calls);
}

void test_Uds3E_InvalidSubFunction_NrcSubFunctionNotSupported(void)
{
    uint8 req[2] = { DCM_UDS_SID_TESTER_PRESENT, 0x7FU };
    dcm_send(req, 2U);
    expect_negative(DCM_UDS_SID_TESTER_PRESENT, DCM_E_SUBFUNCTION_NOT_SUPPORTED);
}

void test_Uds3E_EmptyRequest_NrcIncorrectMessageLength(void)
{
    uint8 req[1] = { DCM_UDS_SID_TESTER_PRESENT };
    dcm_send(req, 1U);
    expect_negative(DCM_UDS_SID_TESTER_PRESENT, DCM_E_INCORRECT_MESSAGE_LENGTH);
}

/*==============================================================================
 *                            INTER-SERVICE STATE
 *============================================================================*/

void test_Uds_SessionSwitch_ResetsSecurityLock(void)
{
    /* lockout from wrong keys must not survive a fresh session switch back
     * to default via Dcm_ResetToDefaultSession + re-init in setUp */
    uint8 reqKey[6] = { DCM_UDS_SID_SECURITY_ACCESS, 0x42U, 0x00U, 0x00U, 0x00U, 0x00U };
    uint8 reqSeed[2] = { DCM_UDS_SID_SECURITY_ACCESS, 0x01U };
    dcm_send(reqKey, 6U);
    Dcm_ResetToDefaultSession();
    Dcm_Init(&matrix_Config); /* re-init clears attempts/delay */
    stub_PduR_Transmit_calls = 0U; /* isolate the post-reinit seed response */
    dcm_send(reqSeed, 2U);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_PduR_Transmit_calls);
    TEST_ASSERT_EQUAL_HEX8(0x01U, stub_PduR_lastPdu[1]);
    TEST_ASSERT_EQUAL_HEX8(0xA5U, stub_PduR_lastPdu[2]);
}

void test_Uds_UnknownService_NrcServiceNotSupported(void)
{
    uint8 req[1] = { 0x99U };
    dcm_send(req, 1U);
    expect_negative(0x99U, DCM_E_SERVICE_NOT_SUPPORTED);
}

/*==============================================================================
 *                                  MAIN
 *============================================================================*/

int main(void)
{
    UNITY_BEGIN();
    /* 0x10 */
    RUN_TEST(test_Uds10_DefaultSession_PositiveResponse);
    RUN_TEST(test_Uds10_ProgrammingSession_PositiveResponse);
    RUN_TEST(test_Uds10_ExtendedSession_PositiveResponse);
    RUN_TEST(test_Uds10_SafetySession_Accepted);
    RUN_TEST(test_Uds10_InvalidSession_NrcSubFunctionNotSupported);
    RUN_TEST(test_Uds10_EmptyRequest_NrcIncorrectMessageLength);
    /* 0x11 */
    RUN_TEST(test_Uds11_HardReset_PositiveResponse);
    RUN_TEST(test_Uds11_KeyOffOnReset_PositiveResponse);
    RUN_TEST(test_Uds11_SoftReset_PositiveResponse);
    RUN_TEST(test_Uds11_EnableRapidPowerShutdown_PositiveResponse);
    RUN_TEST(test_Uds11_DisableRapidPowerShutdown_PositiveResponse);
    RUN_TEST(test_Uds11_InvalidResetType_NrcSubFunctionNotSupported);
    RUN_TEST(test_Uds11_EmptyRequest_NrcIncorrectMessageLength);
    /* 0x14 */
    RUN_TEST(test_Uds14_ClearAllDTCs_PositiveResponse);
    RUN_TEST(test_Uds14_ShortRequest_NrcIncorrectMessageLength);
    /* 0x19 */
    RUN_TEST(test_Uds19_ReportNumberOfDTC_CountsMatchingStatus);
    RUN_TEST(test_Uds19_ReportDTCByStatusMask_ReturnsMatchingDtc);
    RUN_TEST(test_Uds19_ReportDTCByStatusMask_StatusChange_IsReflected);
    RUN_TEST(test_Uds19_ReportSnapshotRecord_FallbackResponse);
    RUN_TEST(test_Uds19_ReportSupportedDTC_ReturnsAllConfigured);
    RUN_TEST(test_Uds19_EmptyRequest_NrcIncorrectMessageLength);
    /* 0x22 */
    RUN_TEST(test_Uds22_ReadKnownDid_ReturnsData);
    RUN_TEST(test_Uds22_UnknownDid_NrcRequestOutOfRange);
    RUN_TEST(test_Uds22_ReadFncMissing_NrcConditionsNotCorrect);
    RUN_TEST(test_Uds22_ExtendedSessionDid_InDefaultSession_NrcServiceNotSupported);
    RUN_TEST(test_Uds22_ExtendedSessionDid_InExtendedSession_Succeeds);
    RUN_TEST(test_Uds22_SecuredDid_LockedLevel_NrcSecurityAccessDenied);
    RUN_TEST(test_Uds22_EmptyRequest_NrcIncorrectMessageLength);
    /* 0x2E */
    RUN_TEST(test_Uds2E_WriteKnownDid_PositiveResponseEchoesDid);
    RUN_TEST(test_Uds2E_WriteFncMissing_NrcRequestOutOfRange);
    RUN_TEST(test_Uds2E_WriteFncFails_NrcConditionsNotCorrect);
    RUN_TEST(test_Uds2E_UnknownDid_NrcRequestOutOfRange);
    RUN_TEST(test_Uds2E_EmptyRequest_NrcIncorrectMessageLength);
    /* 0x23 / 0x2F */
    RUN_TEST(test_Uds23_ReadMemoryByAddress_NrcServiceNotSupported);
    RUN_TEST(test_Uds2F_IOControlByIdentifier_NrcServiceNotSupported);
    /* 0x27 */
    RUN_TEST(test_Uds27_RequestSeed_ReturnsSeedPattern);
    RUN_TEST(test_Uds27_SendKey_CorrectKey_PositiveResponse);
    RUN_TEST(test_Uds27_SendKey_WrongKey_NrcInvalidKey);
    RUN_TEST(test_Uds27_ThreeFailedAttempts_LocksSeedRequest);
    RUN_TEST(test_Uds27_Lockout_ActivatesTimeDelay);
    RUN_TEST(test_Uds27_EmptyRequest_NrcIncorrectMessageLength);
    /* 0x31 */
    RUN_TEST(test_Uds31_StartRoutine_PositiveResponseWithRoutineResult);
    RUN_TEST(test_Uds31_StopRoutine_PositiveResponse);
    RUN_TEST(test_Uds31_RequestRoutineResults_PositiveResponse);
    RUN_TEST(test_Uds31_UnknownRid_NrcRequestOutOfRange);
    RUN_TEST(test_Uds31_InvalidSubFunction_NrcSubFunctionNotSupported);
    RUN_TEST(test_Uds31_StartFncFails_NrcConditionsNotCorrect);
    RUN_TEST(test_Uds31_ExtendedSessionRid_InDefaultSession_NrcServiceNotSupported);
    RUN_TEST(test_Uds31_ExtendedSessionRid_InExtendedSession_StartSucceeds);
    RUN_TEST(test_Uds31_StopFncFails_NrcConditionsNotCorrect);
    RUN_TEST(test_Uds31_ResultFncFails_NrcConditionsNotCorrect);
    RUN_TEST(test_Uds31_SecuredRid_LockedLevel_NrcSecurityAccessDenied);
    RUN_TEST(test_Uds31_EmptyRequest_NrcIncorrectMessageLength);
    /* 0x34/0x36/0x37 */
    RUN_TEST(test_Uds36_TransferWithoutDownload_NrcRequestSequenceError);
    RUN_TEST(test_Uds37_ExitWithoutDownload_NrcRequestSequenceError);
    RUN_TEST(test_Uds34_RequestDownload_PositiveResponseWithBlockLength);
    RUN_TEST(test_Uds34_ShortRequest_NrcIncorrectMessageLength);
    RUN_TEST(test_Uds36_FirstBlockMustBeOne_NrcWrongBlockSequenceCounter);
    RUN_TEST(test_Uds36_SequentialBlocks_AcknowledgedWithBsc);
    RUN_TEST(test_Uds37_AfterDownload_FinishesTransfer);
    /* 0x3E */
    RUN_TEST(test_Uds3E_ZeroSubFunction_PositiveResponseEcho);
    RUN_TEST(test_Uds3E_SuppressBit_NoResponseSent);
    RUN_TEST(test_Uds3E_InvalidSubFunction_NrcSubFunctionNotSupported);
    RUN_TEST(test_Uds3E_EmptyRequest_NrcIncorrectMessageLength);
    /* cross-service */
    RUN_TEST(test_Uds_SessionSwitch_ResetsSecurityLock);
    RUN_TEST(test_Uds_UnknownService_NrcServiceNotSupported);
    return UnityEnd();
}
