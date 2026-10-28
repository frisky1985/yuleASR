/**
 * @file test_doip.c
 * @brief DoIP (Diagnostic over IP) unit tests — coverage uplift.
 *
 * Compiles the real production source src/bsw/ecual/doIP/src/DoIP.c against
 * host stubs (SoAd, SchM, Det, PduR_DoIP) and exercises:
 *  - lifecycle (Init/DeInit/GetVersionInfo) + DET guards
 *  - activation line handling and vehicle announcement timing
 *  - SoAdIfRxIndication protocol dispatch: header checks, generic NACKs,
 *    VID request, routing activation (success / denied / exhaustion),
 *    diagnostic message routing (ACK/NACK, local vs tester target)
 *  - alive check request/response and MainFunction timeout closure
 *  - Transmit forwarding and no-op callbacks
 */

// @tests src/bsw/ecual/doIP/src/DoIP.c  @tests src/bsw/ecual/doIP/include/DoIP.h
// @tests src/bsw/ecual/doIP/include/DoIP_Cfg.h

#include <string.h>
#include "unity.h"
#include "DoIP.h"
#include "Det.h"
#include "PduR_DoIP.h"

/*==================================================================================================
*                                  Stubs
*================================================================================================*/
static uint16 test_DetCalls;
static uint8  test_DetLastApiId;
static uint8  test_DetLastErrorId;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    test_DetCalls++;
    test_DetLastApiId = ApiId;
    test_DetLastErrorId = ErrorId;
    return E_OK;
}

static uint16       test_SoAdTxCount;
static PduIdType    test_SoAdTxLastPduId;
static uint16       test_SoAdTxLastLength;
static uint8        test_SoAdTxLastData[64];
static uint16       test_SoAdCloseCount;

Std_ReturnType SoAd_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    test_SoAdTxCount++;
    test_SoAdTxLastPduId = TxPduId;
    if ((PduInfoPtr != NULL_PTR) && (PduInfoPtr->SduDataPtr != NULL_PTR))
    {
        test_SoAdTxLastLength = (PduInfoPtr->SduLength < sizeof(test_SoAdTxLastData))
                                ? (uint16)PduInfoPtr->SduLength
                                : (uint16)sizeof(test_SoAdTxLastData);
        (void)memcpy(test_SoAdTxLastData, PduInfoPtr->SduDataPtr, test_SoAdTxLastLength);
    }
    return E_OK;
}

Std_ReturnType SoAd_CloseConnection(SoAd_SoConIdType SoConId)
{
    (void)SoConId;
    test_SoAdCloseCount++;
    return E_OK;
}

static uint16 test_PduRRxCount;
static PduIdType test_PduRRxLastId;
static uint16 test_PduRRxLastLength;
static uint8  test_PduRRxLastData[64];

void PduR_DoIPRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    test_PduRRxCount++;
    test_PduRRxLastId = RxPduId;
    if ((PduInfoPtr != NULL_PTR) && (PduInfoPtr->SduDataPtr != NULL_PTR))
    {
        test_PduRRxLastLength = (PduInfoPtr->SduLength < sizeof(test_PduRRxLastData))
                                ? (uint16)PduInfoPtr->SduLength
                                : (uint16)sizeof(test_PduRRxLastData);
        (void)memcpy(test_PduRRxLastData, PduInfoPtr->SduDataPtr, test_PduRRxLastLength);
    }
}

void test_DoipEnterCs(void) {}
void test_DoipExitCs(void) {}

/*==================================================================================================
*                                  Test configuration
*================================================================================================*/
static const DoIP_RoutingActivationConfigType testRoutingActivations[2] =
{
    { 0U,    3584U, TRUE,  TRUE,  FALSE, 0U, 100U },
    { 1U,    3585U, FALSE, FALSE, FALSE, 1U, 100U }
};

static const DoIP_SocketConfigType testSockets[1] =
{
    { 0U, TRUE, 13400U, { 192U, 168U, 0U, 1U }, 0U, { 0U, 0U, 0U, 0U }, 0U, 1U }
};

static const DoIP_TesterConfigType testTesters[1] =
{
    { 3584U, { 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U }, 0U, FALSE }
};

static const DoIP_VehicleAnnouncementConfigType testVehicleAnnouncement =
{
    { 'Y', 'U', 'L', 'E', 'A', 'S', 'R', '0', '0', '0', '0', '0', '0', '0', '0', '0', '1' },
    { 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U },
    { 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU },
    1U,      /* logical address */
    0x10U,   /* further action required */
    0x00U,   /* sync status: none -> payload stays 32 bytes */
    2U,      /* announcement interval */
    3U,      /* announcement count */
    FALSE
};

static const DoIP_ConfigType testConfig =
{
    testRoutingActivations,
    2U,
    testSockets,
    1U,
    testTesters,
    1U,
    &testVehicleAnnouncement
};

#define TEST_SA_DEFAULT   3584U   /* 0x0E00, config entry 0 */
#define TEST_SA_WWHOBD    3585U   /* 0x0E01, config entry 1 */
#define TEST_SA_GENERIC   0x1234U
#define TEST_TA_LOCAL     1U      /* DOIP_LOCAL_LOGICAL_ADDRESS */

/* ACK/NACK codes are private to DoIP.c — mirrored here for assertions */
#ifndef DOIP_DIAG_ACK_CODE_OK
#define DOIP_DIAG_ACK_CODE_OK             (0x00U)
#endif
#ifndef DOIP_DIAG_NACK_CODE_INVALID_SA
#define DOIP_DIAG_NACK_CODE_INVALID_SA    (0x02U)
#endif
#ifndef DOIP_DIAG_NACK_CODE_UNKNOWN_TA
#define DOIP_DIAG_NACK_CODE_UNKNOWN_TA    (0x03U)
#endif

/*==================================================================================================
*                                  Fixtures
*================================================================================================*/
void setUp(void)
{
    test_DetCalls = 0U;
    test_DetLastApiId = 0xFFU;
    test_DetLastErrorId = 0xFFU;
    test_SoAdTxCount = 0U;
    test_SoAdTxLastPduId = 0xFFFFU;
    test_SoAdTxLastLength = 0U;
    (void)memset(test_SoAdTxLastData, 0, sizeof(test_SoAdTxLastData));
    test_SoAdCloseCount = 0U;
    test_PduRRxCount = 0U;
    test_PduRRxLastId = 0xFFFFU;
    test_PduRRxLastLength = 0U;

    /* fresh uninit state per test */
    DoIP_DeInit();
    test_DetCalls = 0U;
}

void tearDown(void)
{
    DoIP_DeInit();
}

static void test_init(void)
{
    DoIP_Init(&testConfig);
}

/* write a DoIP header into buf */
static void write_hdr(uint8* buf, uint16 payloadType, uint32 payloadLength)
{
    buf[0] = DOIP_PROTOCOL_VERSION;
    buf[1] = DOIP_PROTOCOL_VERSION_INVERTED;
    buf[2] = (uint8)(payloadType >> 8);
    buf[3] = (uint8)(payloadType);
    buf[4] = (uint8)(payloadLength >> 24);
    buf[5] = (uint8)(payloadLength >> 16);
    buf[6] = (uint8)(payloadLength >> 8);
    buf[7] = (uint8)(payloadLength);
}

static void rx_frame(PduIdType rxPduId, const uint8* frame, uint16 length)
{
    PduInfoType pdu;

    pdu.SduDataPtr = (uint8*)frame;
    pdu.SduLength = length;
    pdu.MetaDataPtr = NULL_PTR;
    DoIP_SoAdIfRxIndication(rxPduId, &pdu);
}

/* routing activation request frame; returns success of processing */
static void send_ra_request(PduIdType rxPduId, uint16 sourceAddress, uint8 activationType)
{
    uint8 frame[8U + 7U];

    write_hdr(frame, DOIP_PAYLOAD_TYPE_ROUTING_ACTIVATION_REQUEST, 7U);
    frame[8]  = (uint8)(sourceAddress >> 8);
    frame[9]  = (uint8)(sourceAddress);
    frame[10] = activationType;
    frame[11] = 0U;   /* ISO reserved */
    frame[12] = 0U;
    frame[13] = 0U;
    frame[14] = 0U;
    rx_frame(rxPduId, frame, sizeof(frame));
}

/* diagnostic message frame: SA(2) + TA(2) + user data */
static void send_diag_message(PduIdType rxPduId, uint16 sourceAddress, uint16 targetAddress)
{
    uint8 frame[8U + 6U];

    write_hdr(frame, DOIP_PAYLOAD_TYPE_DIAGNOSTIC_MESSAGE, 6U);
    frame[8]  = (uint8)(sourceAddress >> 8);
    frame[9]  = (uint8)(sourceAddress);
    frame[10] = (uint8)(targetAddress >> 8);
    frame[11] = (uint8)(targetAddress);
    frame[12] = 0xDEU;   /* user data */
    frame[13] = 0xADU;
    rx_frame(rxPduId, frame, sizeof(frame));
}

/* activate a tester connection via routing activation */
static void activate_connection(PduIdType rxPduId, uint16 sourceAddress)
{
    send_ra_request(rxPduId, sourceAddress, DOIP_ROUTING_ACTIVATION_DEFAULT);
}

/*==================================================================================================
*                                  Lifecycle
*================================================================================================*/
void test_Init_NullPtr_ReportsDet(void)
{
    DoIP_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(DOIP_SID_INIT, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DOIP_E_INVALID_POINTER, test_DetLastErrorId);
}

void test_Init_Valid_Succeeds(void)
{
    test_init();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

void test_DeInit_Uninit_ReportsDet(void)
{
    DoIP_DeInit();
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(DOIP_E_UNINIT, test_DetLastErrorId);
}

void test_DeInit_AfterInit_AllowsReinit(void)
{
    test_init();
    DoIP_DeInit();
    /* uninit again -> DET on DeInit, APIs report DET too */
    DoIP_MainFunction();   /* silent no-op when uninit */
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);

    test_init();   /* re-init works */
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

void test_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType info;

    (void)memset(&info, 0, sizeof(info));
    DoIP_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(DOIP_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(DOIP_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(DOIP_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(DOIP_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(DOIP_SW_PATCH_VERSION, info.sw_patch_version);
}

void test_GetVersionInfo_Null_ReportsDet(void)
{
    DoIP_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(DOIP_E_INVALID_POINTER, test_DetLastErrorId);
}

/*==================================================================================================
*                                  Activation line / vehicle announcement
*================================================================================================*/
void test_ActivationLineActive_Uninit_ReportsDet(void)
{
    DoIP_ActivationLineSwitchActive();
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(DOIP_E_UNINIT, test_DetLastErrorId);
}

void test_ActivationLineActive_SendsAnnouncements(void)
{
    uint8 i;

    test_init();
    DoIP_ActivationLineSwitchActive();

    /* announcement count 3, interval 2: sends at MF #1, #4, #7 */
    for (i = 0U; i < 7U; i++)
    {
        DoIP_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT16(3U, test_SoAdTxCount);

    /* announcement finished -> no further transmissions */
    DoIP_MainFunction();
    DoIP_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(3U, test_SoAdTxCount);
}

void test_ActivationLineInactive_Uninit_ReportsDet(void)
{
    DoIP_ActivationLineSwitchInactive();
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(DOIP_E_UNINIT, test_DetLastErrorId);
}

void test_ActivationLineInactive_ClosesSockets(void)
{
    test_init();
    activate_connection(0U, TEST_SA_DEFAULT);
    DoIP_ActivationLineSwitchInactive();
    /* all four connection slots are closed (even idle ones) */
    TEST_ASSERT_EQUAL_UINT16(4U, test_SoAdCloseCount);
}

void test_SendVehicleAnnouncement_Content(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, DoIP_SendVehicleAnnouncement());

    TEST_ASSERT_EQUAL_UINT16(1U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL((PduIdType)DOIP_UDP_SOAD_TX_PDU_ID, test_SoAdTxLastPduId);
    TEST_ASSERT_EQUAL_UINT8(DOIP_PROTOCOL_VERSION, test_SoAdTxLastData[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, test_SoAdTxLastData[2]);   /* VID response type */
    TEST_ASSERT_EQUAL_HEX8(0x04U, test_SoAdTxLastData[3]);
    TEST_ASSERT_EQUAL('Y', test_SoAdTxLastData[8]);          /* VIN first char */
    TEST_ASSERT_EQUAL('1', test_SoAdTxLastData[24]);         /* VIN last char */
    TEST_ASSERT_EQUAL_HEX8(0x10U, test_SoAdTxLastData[39]);  /* further action req */
}

/*==================================================================================================
*                                  SoAdIfRxIndication guards
*================================================================================================*/
void test_SoAdIfRx_Uninit_ReportsDet(void)
{
    PduInfoType pdu;
    uint8 frame[8] = { 0U };

    pdu.SduDataPtr = frame;
    pdu.SduLength = sizeof(frame);
    pdu.MetaDataPtr = NULL_PTR;
    DoIP_SoAdIfRxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL_UINT8(DOIP_E_UNINIT, test_DetLastErrorId);
}

void test_SoAdIfRx_NullPdu_ReportsDet(void)
{
    test_init();
    DoIP_SoAdIfRxIndication(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(DOIP_E_INVALID_POINTER, test_DetLastErrorId);
}

void test_SoAdIfRx_ShortFrame_Silent(void)
{
    PduInfoType pdu;
    uint8 frame[4] = { DOIP_PROTOCOL_VERSION, DOIP_PROTOCOL_VERSION_INVERTED, 0U, 1U };

    test_init();
    pdu.SduDataPtr = frame;
    pdu.SduLength = sizeof(frame);
    pdu.MetaDataPtr = NULL_PTR;
    DoIP_SoAdIfRxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT16(0U, test_SoAdTxCount);
}

void test_SoAdIfRx_BadVersion_GenericNack(void)
{
    uint8 frame[9];

    test_init();
    write_hdr(frame, DOIP_PAYLOAD_TYPE_VID_REQUEST, 0U);
    frame[0] = 0x99U;   /* wrong version */
    rx_frame(0U, frame, sizeof(frame));

    TEST_ASSERT_EQUAL_UINT16(1U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL_HEX8(0x00U, test_SoAdTxLastData[2]);   /* generic nack */
    TEST_ASSERT_EQUAL_HEX8(DOIP_NACK_CODE_INCORRECT_PATTERN_FORMAT, test_SoAdTxLastData[8]);
}

void test_SoAdIfRx_PayloadLengthMismatch_GenericNack(void)
{
    uint8 frame[9];

    test_init();
    /* header claims 100 bytes payload, frame only carries 1 */
    write_hdr(frame, DOIP_PAYLOAD_TYPE_VID_REQUEST, 100U);
    rx_frame(0U, frame, sizeof(frame));

    TEST_ASSERT_EQUAL_UINT16(1U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL_HEX8(DOIP_NACK_CODE_INVALID_PAYLOAD_LENGTH, test_SoAdTxLastData[8]);
}

/*==================================================================================================
*                                  VID request
*================================================================================================*/
void test_SoAdIfRx_VidRequest_SendsVidResponse(void)
{
    uint8 frame[8];

    test_init();
    write_hdr(frame, DOIP_PAYLOAD_TYPE_VID_REQUEST, 0U);
    rx_frame(0U, frame, sizeof(frame));

    TEST_ASSERT_EQUAL_UINT16(1U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL_HEX8(0x00U, test_SoAdTxLastData[2]);
    TEST_ASSERT_EQUAL_HEX8(0x04U, test_SoAdTxLastData[3]);   /* VID response */
}

/*==================================================================================================
*                                  Routing activation
*================================================================================================*/
void test_SoAdIfRx_RaRequest_TooShort_GenericNack(void)
{
    uint8 frame[8U + 3U];

    test_init();
    write_hdr(frame, DOIP_PAYLOAD_TYPE_ROUTING_ACTIVATION_REQUEST, 3U);
    rx_frame(0U, frame, sizeof(frame));
    TEST_ASSERT_EQUAL_HEX8(DOIP_NACK_CODE_INVALID_PAYLOAD_LENGTH, test_SoAdTxLastData[8]);
}

void test_SoAdIfRx_RaRequest_UnknownType_Denied(void)
{
    test_init();
    send_ra_request(0U, TEST_SA_GENERIC, 0x7FU);

    TEST_ASSERT_EQUAL_UINT16(1U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL((PduIdType)0U, test_SoAdTxLastPduId);
    TEST_ASSERT_EQUAL_HEX8(0x00U, test_SoAdTxLastData[2]);   /* RA response */
    TEST_ASSERT_EQUAL_HEX8(0x06U, test_SoAdTxLastData[3]);
    TEST_ASSERT_EQUAL_HEX8(DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_UNSUPPORTED_RA,
                           test_SoAdTxLastData[12]);
}

void test_SoAdIfRx_RaRequest_ConfigMatch_Success(void)
{
    test_init();
    send_ra_request(0U, TEST_SA_DEFAULT, DOIP_ROUTING_ACTIVATION_DEFAULT);

    TEST_ASSERT_EQUAL_UINT16(1U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL_HEX8(DOIP_ROUTING_ACTIVATION_RES_CODE_SUCCESS,
                           test_SoAdTxLastData[12]);

    /* re-activation from the same tester reuses the connection slot */
    send_ra_request(0U, TEST_SA_DEFAULT, DOIP_ROUTING_ACTIVATION_DEFAULT);
    TEST_ASSERT_EQUAL_UINT16(2U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL_HEX8(DOIP_ROUTING_ACTIVATION_RES_CODE_SUCCESS,
                           test_SoAdTxLastData[12]);
}

void test_SoAdIfRx_RaRequest_StandardType_Success(void)
{
    test_init();
    /* tester not in config but standard DEFAULT activation type */
    send_ra_request(1U, TEST_SA_GENERIC, DOIP_ROUTING_ACTIVATION_DEFAULT);

    TEST_ASSERT_EQUAL_UINT16(1U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL((PduIdType)1U, test_SoAdTxLastPduId);
    TEST_ASSERT_EQUAL_HEX8(DOIP_ROUTING_ACTIVATION_RES_CODE_SUCCESS,
                           test_SoAdTxLastData[12]);
}

void test_RaRequest_MaxConnections_DeniedSaActive(void)
{
    uint8 i;

    test_init();
    /* fill all 4 connection slots */
    for (i = 0U; i < 4U; i++)
    {
        send_ra_request((PduIdType)i, (uint16)(0x1000U + i), DOIP_ROUTING_ACTIVATION_DEFAULT);
        TEST_ASSERT_EQUAL_HEX8(DOIP_ROUTING_ACTIVATION_RES_CODE_SUCCESS,
                               test_SoAdTxLastData[12]);
    }
    TEST_ASSERT_EQUAL_UINT16(4U, test_SoAdTxCount);

    /* 5th tester: no free slot -> denied */
    send_ra_request(0U, 0x2000U, DOIP_ROUTING_ACTIVATION_DEFAULT);
    TEST_ASSERT_EQUAL_UINT16(5U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL_HEX8(DOIP_ROUTING_ACTIVATION_RES_CODE_DENIED_SA_ACTIVE,
                           test_SoAdTxLastData[12]);
}

/*==================================================================================================
*                                  Diagnostic message routing
*================================================================================================*/
void test_SoAdIfRx_Diag_UnknownSource_NackInvalidSa(void)
{
    test_init();
    send_diag_message(0U, 0x9999U, TEST_TA_LOCAL);

    TEST_ASSERT_EQUAL_UINT16(1U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL_HEX8(0x80U, test_SoAdTxLastData[2]);   /* diag NACK */
    TEST_ASSERT_EQUAL_HEX8(0x03U, test_SoAdTxLastData[3]);
    TEST_ASSERT_EQUAL_HEX8(DOIP_DIAG_NACK_CODE_INVALID_SA, test_SoAdTxLastData[12]);
    TEST_ASSERT_EQUAL_UINT16(0U, test_PduRRxCount);
}

void test_SoAdIfRx_Diag_UnknownTarget_NackUnknownTa(void)
{
    test_init();
    activate_connection(0U, TEST_SA_DEFAULT);
    send_diag_message(0U, TEST_SA_DEFAULT, 0x9999U);

    /* 2 tx: RA response + diag NACK */
    TEST_ASSERT_EQUAL_UINT16(2U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL_HEX8(DOIP_DIAG_NACK_CODE_UNKNOWN_TA, test_SoAdTxLastData[12]);
    TEST_ASSERT_EQUAL_UINT16(0U, test_PduRRxCount);
}

void test_SoAdIfRx_Diag_TargetLocal_AckAndPduR(void)
{
    test_init();
    activate_connection(0U, TEST_SA_DEFAULT);
    send_diag_message(0U, TEST_SA_DEFAULT, TEST_TA_LOCAL);

    /* tx 2: RA response + diag ACK; PduR received the 2 user-data bytes */
    TEST_ASSERT_EQUAL_UINT16(2U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL_HEX8(0x80U, test_SoAdTxLastData[2]);
    TEST_ASSERT_EQUAL_HEX8(0x02U, test_SoAdTxLastData[3]);   /* diag ACK */
    TEST_ASSERT_EQUAL_HEX8(DOIP_DIAG_ACK_CODE_OK, test_SoAdTxLastData[12]);

    TEST_ASSERT_EQUAL_UINT16(1U, test_PduRRxCount);
    TEST_ASSERT_EQUAL((PduIdType)DOIP_PDUR_RX_PDU_ID_BASE, test_PduRRxLastId);
    TEST_ASSERT_EQUAL_UINT16(2U, test_PduRRxLastLength);
    TEST_ASSERT_EQUAL_HEX8(0xDEU, test_PduRRxLastData[0]);
}

void test_SoAdIfRx_Diag_TargetTester_ForwardsViaSoAd(void)
{
    test_init();
    activate_connection(0U, TEST_SA_DEFAULT);   /* conn 0 */
    activate_connection(1U, TEST_SA_WWHOBD);    /* conn 1 */

    send_diag_message(0U, TEST_SA_DEFAULT, TEST_SA_WWHOBD);

    /* tx 4: two RA responses + diag ACK + forward (last capture wins);
     * forward uses TCP TX base + target connection index */
    TEST_ASSERT_EQUAL_UINT16(4U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL((PduIdType)(1U + DOIP_TCP_SOAD_TX_PDU_ID_BASE), test_SoAdTxLastPduId);
    TEST_ASSERT_EQUAL_UINT16(0U, test_PduRRxCount);
}

/*==================================================================================================
*                                  Alive check
*================================================================================================*/
void test_SendAliveCheckRequest_SetsPendingAndSends(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, DoIP_SendAliveCheckRequest(0U));

    TEST_ASSERT_EQUAL_UINT16(1U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL((PduIdType)DOIP_TCP_SOAD_TX_PDU_ID_BASE, test_SoAdTxLastPduId);
    TEST_ASSERT_EQUAL_HEX8(0x00U, test_SoAdTxLastData[2]);
    TEST_ASSERT_EQUAL_HEX8(0x07U, test_SoAdTxLastData[3]);   /* alive check request */
}

void test_ProcessAliveCheckResponse_MismatchedSa_NoCrash(void)
{
    uint8 payload[2] = { 0x99U, 0x99U };

    test_init();
    DoIP_ProcessAliveCheckResponse(0U, payload);   /* no matching connection */
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

void test_MainFunction_AliveCheckTimeout_ClosesSocket(void)
{
    uint16 i;

    test_init();
    activate_connection(0U, TEST_SA_DEFAULT);

    /* MainFunction counts up to DOIP_ALIVE_CHECK_INTERVAL (15000 cycles) and
     * then sends an alive check request on the activated connection */
    for (i = 0U; i < 15000U; i++)
    {
        DoIP_MainFunction();
    }
    /* tx 2: routing activation response + alive check request */
    TEST_ASSERT_EQUAL_UINT16(2U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL_HEX8(0x07U, test_SoAdTxLastData[3]);

    /* next interval: the pending alive check was never answered -> close */
    for (i = 0U; i < 15000U; i++)
    {
        DoIP_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT16(1U, test_SoAdCloseCount);

    /* the connection is freed: a new routing activation succeeds again */
    send_ra_request(0U, TEST_SA_DEFAULT, DOIP_ROUTING_ACTIVATION_DEFAULT);
    TEST_ASSERT_EQUAL_HEX8(DOIP_ROUTING_ACTIVATION_RES_CODE_SUCCESS,
                           test_SoAdTxLastData[12]);
}

/*==================================================================================================
*                                  Transmit / passthrough callbacks
*================================================================================================*/
void test_Transmit_Uninit_ReportsDet(void)
{
    PduInfoType pdu;
    uint8 data[4] = { 1U, 2U, 3U, 4U };

    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_NOT_OK, DoIP_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL_UINT8(DOIP_E_UNINIT, test_DetLastErrorId);
}

void test_Transmit_ForwardsToSoAd(void)
{
    PduInfoType pdu;
    uint8 data[4] = { 1U, 2U, 3U, 4U };

    test_init();
    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, DoIP_Transmit(7U, &pdu));
    TEST_ASSERT_EQUAL_UINT16(1U, test_SoAdTxCount);
    TEST_ASSERT_EQUAL((PduIdType)7U, test_SoAdTxLastPduId);
}

void test_SoAdTriggerTransmit_ReturnsNotOk(void)
{
    PduInfoType pdu;
    uint8 data[4];

    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_NOT_OK, DoIP_SoAdTriggerTransmit(0U, &pdu));
}

void test_PassthroughCallbacks_NoCrash(void)
{
    test_init();
    DoIP_SoAdIfTxConfirmation(0U);
    DoIP_SoAdTpRxIndication(0U, E_OK);
    DoIP_SoAdTpTxConfirmation(0U, E_OK);
    DoIP_RxIndication(0U, NULL_PTR);
    DoIP_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

/*==================================================================================================
*                                  main
*================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Init_NullPtr_ReportsDet);
    RUN_TEST(test_Init_Valid_Succeeds);
    RUN_TEST(test_DeInit_Uninit_ReportsDet);
    RUN_TEST(test_DeInit_AfterInit_AllowsReinit);
    RUN_TEST(test_GetVersionInfo_Valid);
    RUN_TEST(test_GetVersionInfo_Null_ReportsDet);

    RUN_TEST(test_ActivationLineActive_Uninit_ReportsDet);
    RUN_TEST(test_ActivationLineActive_SendsAnnouncements);
    RUN_TEST(test_ActivationLineInactive_Uninit_ReportsDet);
    RUN_TEST(test_ActivationLineInactive_ClosesSockets);
    RUN_TEST(test_SendVehicleAnnouncement_Content);

    RUN_TEST(test_SoAdIfRx_Uninit_ReportsDet);
    RUN_TEST(test_SoAdIfRx_NullPdu_ReportsDet);
    RUN_TEST(test_SoAdIfRx_ShortFrame_Silent);
    RUN_TEST(test_SoAdIfRx_BadVersion_GenericNack);
    RUN_TEST(test_SoAdIfRx_PayloadLengthMismatch_GenericNack);

    RUN_TEST(test_SoAdIfRx_VidRequest_SendsVidResponse);

    RUN_TEST(test_SoAdIfRx_RaRequest_TooShort_GenericNack);
    RUN_TEST(test_SoAdIfRx_RaRequest_UnknownType_Denied);
    RUN_TEST(test_SoAdIfRx_RaRequest_ConfigMatch_Success);
    RUN_TEST(test_SoAdIfRx_RaRequest_StandardType_Success);
    RUN_TEST(test_RaRequest_MaxConnections_DeniedSaActive);

    RUN_TEST(test_SoAdIfRx_Diag_UnknownSource_NackInvalidSa);
    RUN_TEST(test_SoAdIfRx_Diag_UnknownTarget_NackUnknownTa);
    RUN_TEST(test_SoAdIfRx_Diag_TargetLocal_AckAndPduR);
    RUN_TEST(test_SoAdIfRx_Diag_TargetTester_ForwardsViaSoAd);

    RUN_TEST(test_SendAliveCheckRequest_SetsPendingAndSends);
    RUN_TEST(test_ProcessAliveCheckResponse_MismatchedSa_NoCrash);
    RUN_TEST(test_MainFunction_AliveCheckTimeout_ClosesSocket);

    RUN_TEST(test_Transmit_Uninit_ReportsDet);
    RUN_TEST(test_Transmit_ForwardsToSoAd);
    RUN_TEST(test_SoAdTriggerTransmit_ReturnsNotOk);
    RUN_TEST(test_PassthroughCallbacks_NoCrash);

    return UNITY_END();
}
