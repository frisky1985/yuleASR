/**
 * @file test_j1939tp.c
 * @brief J1939Tp (src/bsw/services/j1939tp/src/J1939Tp.c) unit tests — coverage uplift.
 *
 * Compiles the real production source by #including it into this translation
 * unit (white-box, same pattern as tests/unit/services/test_PduR_branch.c) so
 * the static runtime state and the static TP.CM/TP.DT helpers become callable:
 *  - lifecycle (Init/DeInit/GetVersionInfo) + DET paths
 *  - Transmit: single frame (direct CanIf), multi-frame RTS session setup,
 *    invalid/oversize/unknown-SDU paths
 *  - MainFunction TX state machine: RTS_TX -> CTS_RX -> DT_TX -> EOM_ACK
 *  - RX side: RTS/BAM/CTS/ACK/ABORT TP.CM handling and TP.DT reassembly
 *  - CancelTransmit/CancelReceive/ChangeParameter/TxConfirmation
 *
 * NOTE (observed behaviour): J1939Tp_RxIndication hardcodes `pgn = 0U`, so the
 * TP.CM/TP.DT dispatch is unreachable through the public API — the tests call
 * the static processors directly. Documented here, not "fixed" in production.
 */

// @tests src/bsw/services/j1939tp/src/J1939Tp.c  @tests src/bsw/services/j1939tp/include/J1939Tp.h

#include <string.h>
#include "unity.h"
#include "J1939Tp.h"
#include "Det.h"

/*==================================================================================================
*                                  Det mock
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

/*==================================================================================================
*                                  CanIf stub
*================================================================================================*/
static Std_ReturnType test_CanIfRet;
static uint16         test_CanIfCalls;
static PduIdType      test_CanIfLastPduId;
static uint8          test_CanIfLastData[8];
static PduLengthType  test_CanIfLastLen;

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    test_CanIfCalls++;
    test_CanIfLastPduId = TxPduId;
    if ((PduInfoPtr != NULL_PTR) && (PduInfoPtr->SduDataPtr != NULL_PTR))
    {
        test_CanIfLastLen = PduInfoPtr->SduLength;
        if (PduInfoPtr->SduLength > sizeof(test_CanIfLastData))
        {
            test_CanIfLastLen = sizeof(test_CanIfLastData);
        }
        (void)memcpy(test_CanIfLastData, PduInfoPtr->SduDataPtr, test_CanIfLastLen);
    }
    return test_CanIfRet;
}

/*==================================================================================================
*                                  Test configuration
*================================================================================================*/
static const J1939Tp_ConnectionConfigType testConnections[2] =
{
    /* SduId, ComType,      BlockSize, T1,    T2,    T3,    T4,    TxPduId, TxDtPduId, RxPduId */
    { 0U,     J1939TP_CTS,  8U,        750U,  1250U, 1250U, 1050U, 100U,    101U,      102U },
    { 1U,     J1939TP_BAM,  0U,        0U,    0U,    0U,    0U,    103U,    104U,      105U }
};

static const J1939Tp_PgConfigType testPgConfigs[2] =
{
    /* PgId, PduId, DirectNPdu, PgIsVariable, PgLength, DirectSdu, MetaDataLength */
    { 0U,    200U,  FALSE,      FALSE,        8U,       0U,        8U },
    { 1U,    201U,  TRUE,       TRUE,         1785U,    0U,        8U }
};

static const J1939Tp_ConfigType testConfig =
{
    2U,   /* ConnectionCount */
    testConnections,
    2U,   /* PgCount */
    testPgConfigs
};

/* Include the production source (white-box): gives access to the static
 * module state, channel arrays, buffers and the static TP helpers. */
#include "src/bsw/services/j1939tp/src/J1939Tp.c"

/*==================================================================================================
*                                  Fixtures
*================================================================================================*/
void setUp(void)
{
    test_DetCalls = 0U;
    test_DetLastApiId = 0xFFU;
    test_DetLastErrorId = 0xFFU;
    test_CanIfRet = E_OK;
    test_CanIfCalls = 0U;
    test_CanIfLastPduId = 0xFFFFU;
    test_CanIfLastLen = 0U;
    (void)memset(test_CanIfLastData, 0, sizeof(test_CanIfLastData));

    /* reset the module to a defined state (white-box) */
    J1939Tp_ModuleState = J1939TP_STATE_UNINITED;
    J1939Tp_ConfigPtr = NULL_PTR;
    for (uint8 i = 0U; i < J1939TP_MAX_TX_CHANNELS; i++)
    {
        J1939Tp_ResetTxChannel(i);
    }
    for (uint8 i = 0U; i < J1939TP_MAX_RX_CHANNELS; i++)
    {
        J1939Tp_ResetRxChannel(i);
    }
    (void)memset(J1939Tp_TxBuffer, 0, sizeof(J1939Tp_TxBuffer));
    (void)memset(J1939Tp_RxBuffer, 0, sizeof(J1939Tp_RxBuffer));
    test_DetCalls = 0U;
}

void tearDown(void)
{
}

/* helpers ------------------------------------------------------------------ */
static void test_init(void)
{
    TEST_ASSERT_EQUAL(E_OK, J1939Tp_Init(&testConfig));
}

/* transmit a 20-byte payload -> CMDT session on TX channel 0 */
static void test_transmit_mf(void)
{
    PduInfoType pdu;
    uint8 data[20];

    for (uint8 i = 0U; i < sizeof(data); i++)
    {
        data[i] = (uint8)(i + 1U);
    }
    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, J1939Tp_Transmit(0U, &pdu));
}

/* run one MainFunction while TX channel 0 is in RTS_TX (sends RTS) */
static void test_send_rts(void)
{
    test_transmit_mf();
    J1939Tp_MainFunction();
}

/*==================================================================================================
*                                  Lifecycle
*================================================================================================*/
void test_Init_NullPtr_ReportsDet(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_Init(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_SID_INIT, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_E_PARAM_POINTER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_STATE_UNINITED, J1939Tp_ModuleState);
}

void test_Init_Valid_InitializesChannels(void)
{
    /* dirty the channels first to prove Init resets them */
    J1939Tp_TxChannels[0].State = J1939TP_STATE_DT_TX;
    J1939Tp_RxChannels[1].State = J1939TP_STATE_BAM_RX;

    test_init();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_STATE_INITED, J1939Tp_ModuleState);
    TEST_ASSERT_EQUAL(J1939Tp_ConfigPtr, &testConfig);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_TxChannels[0].State);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_RxChannels[1].State);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_TxChannels[0].TotalBytes);
    TEST_ASSERT_EQUAL(FALSE, J1939Tp_TxChannels[0].BcTimerActive);
}

void test_DeInit_AfterInit_ClearsState(void)
{
    test_init();
    J1939Tp_TxChannels[0].State = J1939TP_STATE_RTS_TX;
    J1939Tp_DeInit();
    TEST_ASSERT_EQUAL_UINT8(J1939TP_STATE_UNINITED, J1939Tp_ModuleState);
    TEST_ASSERT_NULL(J1939Tp_ConfigPtr);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_TxChannels[0].State);
}

void test_DeInit_Uninit_SilentNoOp(void)
{
    /* NOTE (observed behaviour): DeInit while uninit does not report DET */
    J1939Tp_DeInit();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

void test_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType info;

    (void)memset(&info, 0, sizeof(info));
    J1939Tp_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(J1939TP_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(J1939TP_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_SW_PATCH_VERSION, info.sw_patch_version);
}

void test_GetVersionInfo_Null_ReportsDet(void)
{
    J1939Tp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_SID_GET_VERSION_INFO, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_E_PARAM_POINTER, test_DetLastErrorId);
}

/*==================================================================================================
*                                  Transmit
*================================================================================================*/
void test_Transmit_Uninit_ReportsDet(void)
{
    PduInfoType pdu;
    uint8 data[4] = { 1U, 2U, 3U, 4U };

    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL_UINT8(J1939TP_SID_TRANSMIT, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_E_UNINIT, test_DetLastErrorId);
}

void test_Transmit_NullPdu_ReportsDet(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_Transmit(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(J1939TP_E_PARAM_POINTER, test_DetLastErrorId);
}

void test_Transmit_SingleFrame_DirectToCanIf(void)
{
    PduInfoType pdu;
    uint8 data[8] = { 0x10U, 0x20U, 0x30U, 0x40U, 0x50U, 0x60U, 0x70U, 0x80U };

    test_init();
    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, J1939Tp_Transmit(0U, &pdu));
    /* routed via the PG config PduId of PG 0 */
    TEST_ASSERT_EQUAL_UINT16(1U, test_CanIfCalls);
    TEST_ASSERT_EQUAL_UINT16(200U, test_CanIfLastPduId);
    /* SF leaves no TP session behind */
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_TxChannels[0].State);
}

void test_Transmit_SingleFrame_CanIfFailurePropagates(void)
{
    PduInfoType pdu;
    uint8 data[2] = { 0x11U, 0x22U };

    test_init();
    test_CanIfRet = E_NOT_OK;
    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_Transmit(0U, &pdu));
}

void test_Transmit_UnknownSduId_SilentReject(void)
{
    PduInfoType pdu;
    uint8 data[4] = { 1U, 2U, 3U, 4U };

    test_init();
    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    /* NOTE (observed behaviour): out-of-range TxSduId is rejected silently,
     * without a DET report */
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_Transmit(testConfig.PgCount, &pdu));
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT16(0U, test_CanIfCalls);
}

void test_Transmit_MultiFrame_StartsCmdtSession(void)
{
    test_init();
    test_transmit_mf();
    TEST_ASSERT_EQUAL_UINT16(0U, test_CanIfCalls);   /* nothing sent before MainFunction */
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_RTS_TX, J1939Tp_TxChannels[0].State);
    TEST_ASSERT_EQUAL_UINT16(20U, J1939Tp_TxChannels[0].TotalBytes);
    TEST_ASSERT_EQUAL_UINT16(3U, J1939Tp_TxChannels[0].PacketsToSend);   /* ceil(20/7) */
    TEST_ASSERT_EQUAL_UINT8(1U, J1939Tp_TxChannels[0].SeqNumber);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_TxChannels[0].SentBytes);
    TEST_ASSERT_EQUAL_UINT16(750U, J1939Tp_TxChannels[0].T1Timer);   /* conn 0 T1Timeout */
    TEST_ASSERT_EQUAL_UINT8(1U, J1939Tp_TxBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(20U, J1939Tp_TxBuffer[19]);
}

void test_Transmit_MultiFrame_NoMatchingConnection_SilentReject(void)
{
    PduInfoType pdu;
    uint8 data[20];

    test_init();
    (void)memset(data, 0x5A, sizeof(data));
    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    /* PG 1 exists (length valid) but no Connection has SduId == 2 */
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_Transmit(2U, &pdu));
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_TxChannels[0].State);
    TEST_ASSERT_EQUAL_UINT16(0U, test_CanIfCalls);
}

void test_Transmit_MultiFrame_AllChannelsBusy_SilentReject(void)
{
    PduInfoType pdu;
    uint8 data[20];

    test_init();
    for (uint8 i = 0U; i < J1939TP_MAX_TX_CHANNELS; i++)
    {
        J1939Tp_TxChannels[i].State = J1939TP_STATE_DT_TX;
    }
    (void)memset(data, 0x5A, sizeof(data));
    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

void test_Transmit_Oversize_SilentReject(void)
{
    PduInfoType pdu;
    static uint8 data[J1939TP_MAX_TP_SIZE + 1U];

    test_init();
    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);   /* > J1939TP_MAX_TP_SIZE and > 8 */
    pdu.MetaDataPtr = NULL_PTR;
    /* NOTE (observed behaviour): length > MAX_TP_SIZE is rejected silently */
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

/*==================================================================================================
*                                  MainFunction TX state machine
*================================================================================================*/
void test_MainFunction_Uninit_NoOp(void)
{
    J1939Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT16(0U, test_CanIfCalls);
}

void test_MainFunction_RtsTx_SendsTpCm(void)
{
    test_init();
    test_transmit_mf();
    J1939Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(1U, test_CanIfCalls);
    TEST_ASSERT_EQUAL_UINT16(100U, test_CanIfLastPduId);   /* conn 0 TP.CM PDU */
    TEST_ASSERT_EQUAL_UINT8(J1939TP_CM_RTS, test_CanIfLastData[0]);
    TEST_ASSERT_EQUAL_UINT8(20U, test_CanIfLastData[1]);   /* total length low */
    TEST_ASSERT_EQUAL_UINT8(0U, test_CanIfLastData[2]);    /* total length high */
    TEST_ASSERT_EQUAL_UINT8(3U, test_CanIfLastData[3]);    /* packet count */
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_CTS_RX, J1939Tp_TxChannels[0].State);
    TEST_ASSERT_EQUAL_UINT16(J1939TP_T1_TIMEOUT_DEFAULT, J1939Tp_TxChannels[0].T1Timer);
}

void test_MainFunction_RtsTx_CanIfFailure_StaysInRtsTx(void)
{
    test_init();
    test_transmit_mf();
    test_CanIfRet = E_NOT_OK;
    J1939Tp_MainFunction();
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_RTS_TX, J1939Tp_TxChannels[0].State);
    TEST_ASSERT_EQUAL_UINT16(1U, test_CanIfCalls);
}

void test_MainFunction_CtsRxDecrementsTimers(void)
{
    test_init();
    test_transmit_mf();
    J1939Tp_MainFunction();   /* RTS_TX -> CTS_RX, T1Timer reset to default */
    TEST_ASSERT_EQUAL_UINT16(J1939TP_T1_TIMEOUT_DEFAULT, J1939Tp_TxChannels[0].T1Timer);
    J1939Tp_MainFunction();   /* CTS_RX: timers tick down */
    TEST_ASSERT_EQUAL_UINT16(J1939TP_T1_TIMEOUT_DEFAULT - 1U, J1939Tp_TxChannels[0].T1Timer);
    J1939Tp_TxChannels[0].T3Timer = 5U;
    J1939Tp_TxChannels[0].T1Timer = 1U;
    J1939Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_TxChannels[0].T1Timer);
    TEST_ASSERT_EQUAL_UINT16(4U, J1939Tp_TxChannels[0].T3Timer);
}

void test_MainFunction_DtTx_SendsDtPackets(void)
{
    test_init();
    /* drive channel 0 into DT_TX with a small 10-byte payload (2 packets) */
    J1939Tp_TxChannels[0].State = J1939TP_STATE_DT_TX;
    J1939Tp_TxChannels[0].TotalBytes = 10U;
    J1939Tp_TxChannels[0].PacketsToSend = 2U;
    J1939Tp_TxChannels[0].PacketsSent = 0U;
    J1939Tp_TxChannels[0].SeqNumber = 1U;
    for (uint8 i = 0U; i < 10U; i++)
    {
        J1939Tp_TxBuffer[i] = (uint8)(i + 0x30U);
    }

    J1939Tp_MainFunction();   /* DT seq 1: bytes 0..6 */
    TEST_ASSERT_EQUAL_UINT16(1U, test_CanIfCalls);
    TEST_ASSERT_EQUAL_UINT16(101U, test_CanIfLastPduId);   /* conn 0 TP.DT PDU */
    TEST_ASSERT_EQUAL_UINT8(1U, test_CanIfLastData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x30U, test_CanIfLastData[1]);
    TEST_ASSERT_EQUAL_UINT8(0x36U, test_CanIfLastData[7]);
    TEST_ASSERT_EQUAL_UINT16(1U, J1939Tp_TxChannels[0].PacketsSent);
    TEST_ASSERT_EQUAL_UINT8(2U, J1939Tp_TxChannels[0].SeqNumber);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_DT_TX, J1939Tp_TxChannels[0].State);

    J1939Tp_MainFunction();   /* DT seq 2: bytes 7..9 + padding, then EOM_ACK */
    TEST_ASSERT_EQUAL_UINT16(2U, test_CanIfCalls);
    TEST_ASSERT_EQUAL_UINT8(2U, test_CanIfLastData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x37U, test_CanIfLastData[1]);
    TEST_ASSERT_EQUAL_UINT8(0x39U, test_CanIfLastData[3]);
    TEST_ASSERT_EQUAL_UINT8(0xFFU, test_CanIfLastData[4]);   /* padding */
    TEST_ASSERT_EQUAL_UINT8(0xFFU, test_CanIfLastData[7]);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_EOM_ACK, J1939Tp_TxChannels[0].State);
}

void test_MainFunction_DtTx_CanIfFailure_KeepsSending(void)
{
    test_init();
    J1939Tp_TxChannels[0].State = J1939TP_STATE_DT_TX;
    J1939Tp_TxChannels[0].TotalBytes = 10U;
    J1939Tp_TxChannels[0].PacketsToSend = 2U;
    J1939Tp_TxChannels[0].PacketsSent = 0U;
    J1939Tp_TxChannels[0].SeqNumber = 1U;

    test_CanIfRet = E_NOT_OK;
    J1939Tp_MainFunction();
    /* send failed: no increment, stays in DT_TX to retry */
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_TxChannels[0].PacketsSent);
    TEST_ASSERT_EQUAL_UINT8(1U, J1939Tp_TxChannels[0].SeqNumber);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_DT_TX, J1939Tp_TxChannels[0].State);
}

void test_MainFunction_IdleAndBamStates_NoAction(void)
{
    test_init();
    J1939Tp_TxChannels[0].State = J1939TP_STATE_BAM_TX;
    J1939Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, test_CanIfCalls);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_BAM_TX, J1939Tp_TxChannels[0].State);
    /* IDLE channels are untouched */
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_TxChannels[1].State);
}

/*==================================================================================================
*                                  MainFunction RX side timers
*================================================================================================*/
void test_MainFunction_RxTimersDecrement(void)
{
    test_init();
    J1939Tp_RxChannels[0].T1Timer = 3U;
    J1939Tp_RxChannels[0].T2Timer = 2U;
    J1939Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(2U, J1939Tp_RxChannels[0].T1Timer);
    TEST_ASSERT_EQUAL_UINT16(1U, J1939Tp_RxChannels[0].T2Timer);
    /* never below zero */
    J1939Tp_MainFunction();
    J1939Tp_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_RxChannels[0].T1Timer);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_RxChannels[0].T2Timer);
}

/*==================================================================================================
*                                  Cancel / ChangeParameter / TxConfirmation
*================================================================================================*/
void test_CancelTransmit_Uninit_ReportsDet(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_CancelTransmit(0U));
    TEST_ASSERT_EQUAL_UINT8(J1939TP_SID_CANCEL_TRANSMIT, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_E_UNINIT, test_DetLastErrorId);
}

void test_CancelTransmit_ActiveSession_Cancels(void)
{
    test_init();
    test_transmit_mf();
    TEST_ASSERT_EQUAL(E_OK, J1939Tp_CancelTransmit(0U));
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_TxChannels[0].State);
    /* all channels idle again -> cannot cancel */
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_CancelTransmit(0U));
}

void test_CancelReceive_Uninit_ReportsDet(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_CancelReceive(0U));
    TEST_ASSERT_EQUAL_UINT8(J1939TP_SID_CANCEL_RECEIVE, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_E_UNINIT, test_DetLastErrorId);
}

void test_CancelReceive_ActiveSession_Cancels(void)
{
    test_init();
    J1939Tp_RxChannels[0].State = J1939TP_STATE_DT_RX;
    TEST_ASSERT_EQUAL(E_OK, J1939Tp_CancelReceive(0U));
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_RxChannels[0].State);
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_CancelReceive(0U));
}

void test_ChangeParameter_PlaceholderAlwaysRejects(void)
{
    /* NOTE (observed behaviour): ChangeParameter is a placeholder that always
     * returns E_NOT_OK regardless of parameters */
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_ChangeParameter(0U, J1939TP_PARAM_T1, 100U));
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_ChangeParameter(0U, J1939TP_PARAM_N_Bs, 0xFFFFU));
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

void test_TxConfirmation_IsNoOp(void)
{
    test_init();
    test_transmit_mf();
    J1939Tp_TxConfirmation(101U, E_OK);
    J1939Tp_TxConfirmation(0xFFFFU, E_NOT_OK);
    /* NOTE (observed behaviour): TxConfirmation does not touch the session */
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_RTS_TX, J1939Tp_TxChannels[0].State);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

void test_PduR_J1939TpRxIndication_IsNoOp(void)
{
    PduInfoType pdu;
    uint8 data[2] = { 0xAAU, 0x55U };

    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    PduR_J1939TpRxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

/*==================================================================================================
*                                  RxIndication (public path)
*================================================================================================*/
void test_RxIndication_NullPdu_ReportsDet(void)
{
    test_init();
    J1939Tp_RxIndication(102U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_SID_RX_INDICATION, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(J1939TP_E_PARAM_POINTER, test_DetLastErrorId);
}

void test_RxIndication_NullData_Ignored(void)
{
    PduInfoType pdu;

    test_init();
    pdu.SduDataPtr = NULL_PTR;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    J1939Tp_RxIndication(102U, &pdu);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_RxChannels[0].State);
}

void test_RxIndication_MatchingPdu_NoTpProcessing(void)
{
    PduInfoType pdu;
    uint8 cm[8] = { J1939TP_CM_RTS, 20U, 0U, 3U, 0xFFU, 0x00U, 0xECU, 0x00U };

    test_init();
    pdu.SduDataPtr = cm;
    pdu.SduLength = sizeof(cm);
    pdu.MetaDataPtr = NULL_PTR;
    /* NOTE (observed behaviour): RxIndication hardcodes pgn = 0, so even a
     * well-formed TP.CM RTS on a matching PDU is not dispatched to the TP.CM
     * processor — the RX channel stays IDLE. */
    J1939Tp_RxIndication(102U, &pdu);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_RxChannels[0].State);
}

void test_RxIndication_UnknownPdu_Ignored(void)
{
    PduInfoType pdu;
    uint8 cm[8] = { J1939TP_CM_RTS, 20U, 0U, 3U, 0xFFU, 0x00U, 0xECU, 0x00U };

    test_init();
    pdu.SduDataPtr = cm;
    pdu.SduLength = sizeof(cm);
    pdu.MetaDataPtr = NULL_PTR;
    J1939Tp_RxIndication(0x1234U, &pdu);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

/*==================================================================================================
*                                  TP.CM RX processing (white-box statics)
*================================================================================================*/
void test_ProcessTpCmRx_Rts_EntersDtRx(void)
{
    PduInfoType pdu;
    uint8 cm[8] = { J1939TP_CM_RTS, 20U, 0U, 3U, 0xFFU, 0x00U, 0xECU, 0x00U };

    test_init();
    pdu.SduDataPtr = cm;
    pdu.SduLength = sizeof(cm);
    pdu.MetaDataPtr = NULL_PTR;
    J1939Tp_ProcessTpCmRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_DT_RX, J1939Tp_RxChannels[0].State);
    TEST_ASSERT_EQUAL_UINT16(20U, J1939Tp_RxChannels[0].TotalBytes);
    TEST_ASSERT_EQUAL_UINT16(3U, J1939Tp_RxChannels[0].PacketsToReceive);
    TEST_ASSERT_EQUAL_UINT32(0x00EC00U, J1939Tp_RxChannels[0].Pgn);
    TEST_ASSERT_EQUAL_UINT8(0U, J1939Tp_RxChannels[0].SeqNumber);
}

void test_ProcessTpCmRx_Rts_Oversize_StaysIdle(void)
{
    PduInfoType pdu;
    uint8 cm[8] = { J1939TP_CM_RTS, 0xFFU, 0xFFU, 255U, 0xFFU, 0x00U, 0xECU, 0x00U };

    test_init();
    pdu.SduDataPtr = cm;
    pdu.SduLength = sizeof(cm);
    pdu.MetaDataPtr = NULL_PTR;
    /* total 65535 > J1939TP_MAX_TP_SIZE (1785) -> session refused */
    J1939Tp_ProcessTpCmRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_RxChannels[0].State);
}

void test_ProcessTpCmRx_Rts_BusyChannel_Ignored(void)
{
    PduInfoType pdu;
    uint8 cm[8] = { J1939TP_CM_RTS, 20U, 0U, 3U, 0xFFU, 0x00U, 0xECU, 0x00U };

    test_init();
    J1939Tp_RxChannels[0].State = J1939TP_STATE_DT_RX;
    pdu.SduDataPtr = cm;
    pdu.SduLength = sizeof(cm);
    pdu.MetaDataPtr = NULL_PTR;
    /* RTS on an already-busy RX channel is ignored (only IDLE accepts) */
    J1939Tp_ProcessTpCmRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_RxChannels[0].TotalBytes);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_DT_RX, J1939Tp_RxChannels[0].State);
}

void test_ProcessTpCmRx_ShortPdu_And_UnknownControl_Ignored(void)
{
    PduInfoType pdu;
    uint8 shortPdu[4] = { J1939TP_CM_RTS, 20U, 0U, 3U };
    uint8 unknown[8] = { 0x7FU, 20U, 0U, 3U, 0xFFU, 0x00U, 0xECU, 0x00U };

    test_init();
    pdu.SduDataPtr = shortPdu;
    pdu.SduLength = sizeof(shortPdu);
    pdu.MetaDataPtr = NULL_PTR;
    J1939Tp_ProcessTpCmRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_RxChannels[0].State);

    pdu.SduDataPtr = unknown;
    pdu.SduLength = sizeof(unknown);
    J1939Tp_ProcessTpCmRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_RxChannels[0].State);

    /* NULL PDU handled defensively */
    J1939Tp_ProcessTpCmRx(NULL_PTR, 0U);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_RxChannels[0].State);
}

void test_ProcessTpCmRx_Cts_MovesTxToDtTx(void)
{
    PduInfoType pdu;
    uint8 cm[8] = { J1939TP_CM_CTS, 2U, 0xFFU, 1U, 0xFFU, 0x00U, 0xECU, 0x00U };

    test_init();
    test_transmit_mf();
    J1939Tp_MainFunction();   /* RTS_TX -> CTS_RX */
    pdu.SduDataPtr = cm;
    pdu.SduLength = sizeof(cm);
    pdu.MetaDataPtr = NULL_PTR;
    J1939Tp_ProcessTpCmRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_DT_TX, J1939Tp_TxChannels[0].State);

    /* CTS in the wrong state is ignored */
    J1939Tp_ProcessTpCmRx(&pdu, 1U);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_TxChannels[1].State);
}

void test_ProcessTpCmRx_EomAck_CompletesTxSession(void)
{
    PduInfoType pdu;
    uint8 cm[8] = { J1939TP_CM_ACK, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0xECU, 0x00U };

    test_init();
    test_transmit_mf();
    J1939Tp_MainFunction();                        /* -> CTS_RX */
    J1939Tp_TxChannels[0].State = J1939TP_STATE_EOM_ACK;
    pdu.SduDataPtr = cm;
    pdu.SduLength = sizeof(cm);
    pdu.MetaDataPtr = NULL_PTR;
    J1939Tp_ProcessTpCmRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_TxChannels[0].State);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_TxChannels[0].TotalBytes);

    /* EOM ACK in the wrong state is ignored */
    J1939Tp_ProcessTpCmRx(&pdu, 1U);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_TxChannels[1].State);
}

void test_ProcessTpCmRx_Bam_EntersBamRx(void)
{
    PduInfoType pdu;
    uint8 cm[8] = { J1939TP_CM_BAM, 30U, 0U, 5U, 0xFFU, 0x00U, 0xFEU, 0x00U };

    test_init();
    pdu.SduDataPtr = cm;
    pdu.SduLength = sizeof(cm);
    pdu.MetaDataPtr = NULL_PTR;
    /* NOTE (observed behaviour): unlike RTS, BAM is accepted on any RX
     * channel state (no IDLE guard) */
    J1939Tp_ProcessTpCmRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_BAM_RX, J1939Tp_RxChannels[0].State);
    TEST_ASSERT_EQUAL_UINT16(30U, J1939Tp_RxChannels[0].TotalBytes);
    TEST_ASSERT_EQUAL_UINT16(5U, J1939Tp_RxChannels[0].PacketsToReceive);
    TEST_ASSERT_EQUAL_UINT32(0x00FE00U, J1939Tp_RxChannels[0].Pgn);
}

void test_ProcessTpCmRx_Abort_ResetsSessions(void)
{
    PduInfoType pdu;
    uint8 cm[8] = { J1939TP_CM_ABORT, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU };

    test_init();
    test_transmit_mf();
    J1939Tp_RxChannels[0].State = J1939TP_STATE_DT_RX;
    J1939Tp_RxChannels[0].TotalBytes = 20U;
    pdu.SduDataPtr = cm;
    pdu.SduLength = sizeof(cm);
    pdu.MetaDataPtr = NULL_PTR;
    J1939Tp_ProcessTpCmRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_TxChannels[0].State);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_RxChannels[0].State);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_RxChannels[0].TotalBytes);
}

/*==================================================================================================
*                                  TP.DT RX processing (white-box statics)
*================================================================================================*/
void test_ProcessTpDtRx_ReassemblesAndCompletes(void)
{
    PduInfoType pdu;
    uint8 dt[8];
    uint8 expected[20];

    test_init();
    /* set up an active RX session: 20 bytes in 3 packets */
    J1939Tp_RxChannels[0].State = J1939TP_STATE_DT_RX;
    J1939Tp_RxChannels[0].TotalBytes = 20U;
    J1939Tp_RxChannels[0].PacketsToReceive = 3U;
    J1939Tp_RxChannels[0].SeqNumber = 0U;
    for (uint8 i = 0U; i < sizeof(expected); i++)
    {
        expected[i] = (uint8)(0xA0U + i);
    }

    for (uint8 seq = 1U; seq <= 2U; seq++)
    {
        dt[0] = seq;
        (void)memcpy(&dt[1], &expected[(seq - 1U) * 7U], 7U);
        pdu.SduDataPtr = dt;
        pdu.SduLength = sizeof(dt);
        pdu.MetaDataPtr = NULL_PTR;
        J1939Tp_ProcessTpDtRx(&pdu, 0U);
    }

    /* mid-session: 2 of 3 packets buffered, session still active */
    TEST_ASSERT_EQUAL_UINT16(14U, J1939Tp_RxChannels[0].ReceivedBytes);
    TEST_ASSERT_EQUAL_UINT16(2U, J1939Tp_RxChannels[0].PacketsReceived);
    TEST_ASSERT_EQUAL_UINT8(2U, J1939Tp_RxChannels[0].SeqNumber);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_DT_RX, J1939Tp_RxChannels[0].State);

    /* final packet */
    dt[0] = 3U;
    (void)memcpy(&dt[1], &expected[14U], 6U);
    dt[7] = 0xFFU;   /* not copied: offset 20 == TotalBytes */
    pdu.SduDataPtr = dt;
    pdu.SduLength = sizeof(dt);
    J1939Tp_ProcessTpDtRx(&pdu, 0U);

    /* complete -> forwarded to PduR via the built-in stub and the channel is
     * reset (ReceivedBytes/PacketsReceived cleared again); the buffer keeps
     * the reassembled payload */
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_IDLE, J1939Tp_RxChannels[0].State);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_RxChannels[0].ReceivedBytes);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, J1939Tp_RxBuffer, 20U);
}

void test_ProcessTpDtRx_WrongSequence_Ignored(void)
{
    PduInfoType pdu;
    uint8 dt[8] = { 5U, 1U, 2U, 3U, 4U, 5U, 6U, 7U };

    test_init();
    J1939Tp_RxChannels[0].State = J1939TP_STATE_DT_RX;
    J1939Tp_RxChannels[0].TotalBytes = 20U;
    J1939Tp_RxChannels[0].PacketsToReceive = 3U;
    J1939Tp_RxChannels[0].SeqNumber = 0U;
    pdu.SduDataPtr = dt;
    pdu.SduLength = sizeof(dt);
    pdu.MetaDataPtr = NULL_PTR;
    /* expected seq 1, got 5 -> dropped */
    J1939Tp_ProcessTpDtRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_RxChannels[0].ReceivedBytes);
    TEST_ASSERT_EQUAL_UINT8(0U, J1939Tp_RxChannels[0].SeqNumber);
    TEST_ASSERT_EQUAL_INT(J1939TP_STATE_DT_RX, J1939Tp_RxChannels[0].State);
}

void test_ProcessTpDtRx_WrongState_AndShortPdu_Ignored(void)
{
    PduInfoType pdu;
    uint8 dt[8] = { 1U, 1U, 2U, 3U, 4U, 5U, 6U, 7U };
    uint8 shortDt[3] = { 1U, 2U, 3U };

    test_init();
    /* IDLE channel: DT dropped */
    pdu.SduDataPtr = dt;
    pdu.SduLength = sizeof(dt);
    pdu.MetaDataPtr = NULL_PTR;
    J1939Tp_ProcessTpDtRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_RxChannels[0].ReceivedBytes);

    /* truncated DT PDU dropped */
    J1939Tp_RxChannels[0].State = J1939TP_STATE_DT_RX;
    J1939Tp_RxChannels[0].TotalBytes = 20U;
    J1939Tp_RxChannels[0].PacketsToReceive = 3U;
    pdu.SduDataPtr = shortDt;
    pdu.SduLength = sizeof(shortDt);
    J1939Tp_ProcessTpDtRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_RxChannels[0].ReceivedBytes);

    /* NULL PDU handled defensively */
    J1939Tp_ProcessTpDtRx(NULL_PTR, 0U);
    TEST_ASSERT_EQUAL_UINT16(0U, J1939Tp_RxChannels[0].ReceivedBytes);
}

void test_ProcessTpDtRx_BamRx_AcceptsData(void)
{
    PduInfoType pdu;
    uint8 dt[8] = { 1U, 0xB1U, 0xB2U, 0xB3U, 0xB4U, 0xB5U, 0xB6U, 0xB7U };

    test_init();
    J1939Tp_RxChannels[0].State = J1939TP_STATE_BAM_RX;
    J1939Tp_RxChannels[0].TotalBytes = 30U;
    J1939Tp_RxChannels[0].PacketsToReceive = 5U;
    J1939Tp_RxChannels[0].SeqNumber = 0U;
    pdu.SduDataPtr = dt;
    pdu.SduLength = sizeof(dt);
    pdu.MetaDataPtr = NULL_PTR;
    J1939Tp_ProcessTpDtRx(&pdu, 0U);
    TEST_ASSERT_EQUAL_UINT16(7U, J1939Tp_RxChannels[0].ReceivedBytes);
    TEST_ASSERT_EQUAL_UINT8(1U, J1939Tp_RxChannels[0].SeqNumber);
    TEST_ASSERT_EQUAL_UINT8(0xB1U, J1939Tp_RxBuffer[0]);
}

/*==================================================================================================
*                                  Send helpers (white-box statics)
*================================================================================================*/
void test_SendTpCm_InvalidChannel_ReturnsNotOk(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_SendTpCm(J1939TP_MAX_TX_CHANNELS, J1939TP_CM_RTS));
    TEST_ASSERT_EQUAL_UINT16(0U, test_CanIfCalls);
}

void test_SendTpDt_InvalidChannel_ReturnsNotOk(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, J1939Tp_SendTpDt(J1939TP_MAX_TX_CHANNELS));
    TEST_ASSERT_EQUAL_UINT16(0U, test_CanIfCalls);
}

void test_ResetChannels_GuardInvalidIndex(void)
{
    /* out-of-range indexes are ignored without crash */
    J1939Tp_ResetTxChannel(J1939TP_MAX_TX_CHANNELS);
    J1939Tp_ResetRxChannel(J1939TP_MAX_RX_CHANNELS);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

/*==================================================================================================
*                                  MAIN
*================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    /* Lifecycle */
    RUN_TEST(test_Init_NullPtr_ReportsDet);
    RUN_TEST(test_Init_Valid_InitializesChannels);
    RUN_TEST(test_DeInit_AfterInit_ClearsState);
    RUN_TEST(test_DeInit_Uninit_SilentNoOp);
    RUN_TEST(test_GetVersionInfo_Valid);
    RUN_TEST(test_GetVersionInfo_Null_ReportsDet);

    /* Transmit */
    RUN_TEST(test_Transmit_Uninit_ReportsDet);
    RUN_TEST(test_Transmit_NullPdu_ReportsDet);
    RUN_TEST(test_Transmit_SingleFrame_DirectToCanIf);
    RUN_TEST(test_Transmit_SingleFrame_CanIfFailurePropagates);
    RUN_TEST(test_Transmit_UnknownSduId_SilentReject);
    RUN_TEST(test_Transmit_MultiFrame_StartsCmdtSession);
    RUN_TEST(test_Transmit_MultiFrame_NoMatchingConnection_SilentReject);
    RUN_TEST(test_Transmit_MultiFrame_AllChannelsBusy_SilentReject);
    RUN_TEST(test_Transmit_Oversize_SilentReject);

    /* MainFunction TX state machine */
    RUN_TEST(test_MainFunction_Uninit_NoOp);
    RUN_TEST(test_MainFunction_RtsTx_SendsTpCm);
    RUN_TEST(test_MainFunction_RtsTx_CanIfFailure_StaysInRtsTx);
    RUN_TEST(test_MainFunction_CtsRxDecrementsTimers);
    RUN_TEST(test_MainFunction_DtTx_SendsDtPackets);
    RUN_TEST(test_MainFunction_DtTx_CanIfFailure_KeepsSending);
    RUN_TEST(test_MainFunction_IdleAndBamStates_NoAction);
    RUN_TEST(test_MainFunction_RxTimersDecrement);

    /* Cancel / misc */
    RUN_TEST(test_CancelTransmit_Uninit_ReportsDet);
    RUN_TEST(test_CancelTransmit_ActiveSession_Cancels);
    RUN_TEST(test_CancelReceive_Uninit_ReportsDet);
    RUN_TEST(test_CancelReceive_ActiveSession_Cancels);
    RUN_TEST(test_ChangeParameter_PlaceholderAlwaysRejects);
    RUN_TEST(test_TxConfirmation_IsNoOp);
    RUN_TEST(test_PduR_J1939TpRxIndication_IsNoOp);

    /* RxIndication public path */
    RUN_TEST(test_RxIndication_NullPdu_ReportsDet);
    RUN_TEST(test_RxIndication_NullData_Ignored);
    RUN_TEST(test_RxIndication_MatchingPdu_NoTpProcessing);
    RUN_TEST(test_RxIndication_UnknownPdu_Ignored);

    /* TP.CM / TP.DT white-box processing */
    RUN_TEST(test_ProcessTpCmRx_Rts_EntersDtRx);
    RUN_TEST(test_ProcessTpCmRx_Rts_Oversize_StaysIdle);
    RUN_TEST(test_ProcessTpCmRx_Rts_BusyChannel_Ignored);
    RUN_TEST(test_ProcessTpCmRx_ShortPdu_And_UnknownControl_Ignored);
    RUN_TEST(test_ProcessTpCmRx_Cts_MovesTxToDtTx);
    RUN_TEST(test_ProcessTpCmRx_EomAck_CompletesTxSession);
    RUN_TEST(test_ProcessTpCmRx_Bam_EntersBamRx);
    RUN_TEST(test_ProcessTpCmRx_Abort_ResetsSessions);

    RUN_TEST(test_ProcessTpDtRx_ReassemblesAndCompletes);
    RUN_TEST(test_ProcessTpDtRx_WrongSequence_Ignored);
    RUN_TEST(test_ProcessTpDtRx_WrongState_AndShortPdu_Ignored);
    RUN_TEST(test_ProcessTpDtRx_BamRx_AcceptsData);

    /* Send helpers */
    RUN_TEST(test_SendTpCm_InvalidChannel_ReturnsNotOk);
    RUN_TEST(test_SendTpDt_InvalidChannel_ReturnsNotOk);
    RUN_TEST(test_ResetChannels_GuardInvalidIndex);

    return UNITY_END();
}
