/**
 * @file test_frtp.c
 * @brief FrTp (FlexRay Transport Protocol) unit tests — coverage uplift.
 *
 * Compiles the real production sources (FrTp.c, FrTp_PrivUtil.c, FrTp_Tx.c,
 * FrTp_Rx.c, FrTp_TxSm.c) against host stubs and exercises:
 *  - lifecycle (Init/DeInit/GetVersionInfo/MainFunction) + DET paths
 *  - SF / multi-frame TX state machine incl. flow control (CTS/WAIT/OVFLW)
 *  - TX confirmation success / retry / abort paths
 *  - RX indication SF/FF/CF/FC incl. sequence error and completion
 *  - timeout handling via white-box timer injection into FrTp_Runtime
 *  - PDU encode/decode helpers, timer helpers, connection lookup helpers
 *
 * FrTp_Lcfg.c is intentionally NOT compiled: it duplicates the global
 * FrTp_Runtime definition (also in FrTp.c). The test provides its own
 * FrTp_ConnectionConfigs table instead.
 */

// @tests src/bsw/ecual/frtp/src/FrTp.c  @tests src/bsw/ecual/frtp/src/FrTp_PrivUtil.c
// @tests src/bsw/ecual/frtp/src/FrTp_Tx.c  @tests src/bsw/ecual/frtp/src/FrTp_Rx.c
// @tests src/bsw/ecual/frtp/src/FrTp_TxSm.c  @tests src/bsw/ecual/frtp/include/FrTp.h

#include <string.h>
#include "unity.h"
#include "FrTp.h"
#include "FrTp_Private.h"
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
*                                  PduR FrTp callback stubs
*================================================================================================*/
static uint16         test_PdurTxConfCalls;
static PduIdType      test_PdurTxConfLastId;
static Std_ReturnType test_PdurTxConfLastResult;
static uint16         test_PdurRxIndCalls;
static PduIdType      test_PdurRxIndLastId;
static Std_ReturnType test_PdurRxIndLastResult;

void PduR_FrTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    test_PdurTxConfCalls++;
    test_PdurTxConfLastId = TxPduId;
    test_PdurTxConfLastResult = result;
}

void PduR_FrTpRxIndication(PduIdType RxPduId, Std_ReturnType result)
{
    test_PdurRxIndCalls++;
    test_PdurRxIndLastId = RxPduId;
    test_PdurRxIndLastResult = result;
}

/*==================================================================================================
*                                  Test configuration
*================================================================================================*/
/* Replaces FrTp_Lcfg.c (excluded from this target to avoid the duplicate
 * FrTp_Runtime definition). Same values: 4 connections, tx/rx PDU ids 0..7. */
const FrTp_ConnectionConfigType FrTp_ConnectionConfigs[FRTP_MAX_CONNECTIONS] =
{
    { 0U, 0U, 1U, 254U, 3U, 100U, 100U, 100U, 100U, 100U, 100U, TRUE,  8U, 10U },
    { 1U, 2U, 3U, 254U, 3U, 100U, 100U, 100U, 100U, 100U, 100U, TRUE,  8U, 10U },
    { 2U, 4U, 5U, 254U, 3U, 100U, 100U, 100U, 100U, 100U, 100U, TRUE,  8U, 10U },
    { 3U, 6U, 7U, 254U, 3U, 100U, 100U, 100U, 100U, 100U, 100U, TRUE,  8U, 10U }
};

static const FrTp_ConfigType testConfig =
{
    FrTp_ConnectionConfigs,
    FRTP_MAX_CONNECTIONS,
    TRUE,   /* devErrorDetect  */
    TRUE    /* versionInfoApi  */
};

/* shorthand for white-box runtime inspection (FrTp_Runtime is a global) */
#define RT(i) (FrTp_Runtime.connections[(i)])

static uint8 testBigBuf[512];   /* multi-frame payload buffer */

/*==================================================================================================
*                                  Fixtures
*================================================================================================*/
void setUp(void)
{
    test_DetCalls = 0U;
    test_DetLastApiId = 0xFFU;
    test_DetLastErrorId = 0xFFU;
    test_PdurTxConfCalls = 0U;
    test_PdurTxConfLastId = 0xFFFFU;
    test_PdurTxConfLastResult = E_NOT_OK;
    test_PdurRxIndCalls = 0U;
    test_PdurRxIndLastId = 0xFFFFU;
    test_PdurRxIndLastResult = E_NOT_OK;
    (void)memset(testBigBuf, 0, sizeof(testBigBuf));

    /* start every test from a defined module state */
    if (FrTp_Runtime.initialized)
    {
        FrTp_DeInit();
    }
    test_DetCalls = 0U;
}

void tearDown(void)
{
    if (FrTp_Runtime.initialized)
    {
        FrTp_DeInit();
    }
}

/* helpers ------------------------------------------------------------------ */
static void test_init(void)
{
    FrTp_Init(&testConfig);
}

/* transmit an 8-byte single frame on connection 0 */
static void test_transmit_sf(void)
{
    PduInfoType pdu;
    uint8 data[8] = { 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U };

    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, FrTp_Transmit(0U, &pdu));
}

/* transmit a 300-byte payload -> FF carries 252 bytes, 48 remain */
static void test_transmit_ff300(void)
{
    PduInfoType pdu;
    uint8 i;

    for (i = 0U; i < 255U; i++)
    {
        testBigBuf[i] = i;
    }
    pdu.SduDataPtr = testBigBuf;
    pdu.SduLength = 300U;
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, FrTp_Transmit(0U, &pdu));
}

/* start an FF reception of 20 bytes total (16 carried in FF) on rx PDU 1 */
static void test_start_ff_reception(uint8* rxBuffer, PduLengthType rxBufferSize)
{
    PduInfoType pdu;
    uint8 frame[18];
    uint8 i;

    if (rxBuffer != NULL_PTR)
    {
        RT(0).rxBuffer = rxBuffer;
        RT(0).rxBufferSize = rxBufferSize;
    }
    frame[0] = 0x40U;   /* FF, length high = 0 */
    frame[1] = 20U;     /* length low = 20 */
    for (i = 2U; i < sizeof(frame); i++)
    {
        frame[i] = (uint8)(i + 0x40U);
    }
    pdu.SduDataPtr = frame;
    pdu.SduLength = sizeof(frame);
    pdu.MetaDataPtr = NULL_PTR;
    FrTp_RxIndication(1U, &pdu);
}

/*==================================================================================================
*                                  Lifecycle
*================================================================================================*/
void test_Init_NullPtr_ReportsDet(void)
{
    FrTp_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(FRTP_SID_INIT, test_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_PARAM_POINTER, test_DetLastErrorId);
}

void test_Init_TooManyConnections_ReportsDet(void)
{
    FrTp_ConfigType cfg = testConfig;
    cfg.numConnections = FRTP_MAX_CONNECTIONS + 1U;
    FrTp_Init(&cfg);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_PARAM_CONFIG, test_DetLastErrorId);
    TEST_ASSERT_EQUAL(FALSE, FrTp_Runtime.initialized);
}

void test_Init_NullConnections_ReportsDet(void)
{
    FrTp_ConfigType cfg = testConfig;
    cfg.connections = NULL_PTR;
    FrTp_Init(&cfg);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_PARAM_CONFIG, test_DetLastErrorId);
}

void test_Init_Valid_InitializesRuntime(void)
{
    test_init();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL(TRUE, FrTp_Runtime.initialized);
    TEST_ASSERT_EQUAL_UINT8(0U, FrTp_Runtime.activeConnections);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(3).state);
}

void test_Init_Twice_ReportsDet(void)
{
    test_init();
    FrTp_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_ALREADY_INITIALIZED, test_DetLastErrorId);
}

void test_DeInit_Uninit_ReportsDet(void)
{
    FrTp_DeInit();
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_UNINIT, test_DetLastErrorId);
}

void test_DeInit_AfterInit_ClearsState(void)
{
    test_init();
    test_transmit_sf();
    FrTp_DeInit();
    TEST_ASSERT_EQUAL(FALSE, FrTp_Runtime.initialized);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
    /* second DeInit reports uninit */
    FrTp_DeInit();
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_UNINIT, test_DetLastErrorId);
}

void test_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType info;

    (void)memset(&info, 0, sizeof(info));
    FrTp_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(FRTP_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(FRTP_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(FRTP_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(FRTP_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(FRTP_SW_PATCH_VERSION, info.sw_patch_version);
}

void test_GetVersionInfo_Null_ReportsDet(void)
{
    FrTp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_PARAM_POINTER, test_DetLastErrorId);
}

void test_MainFunction_Uninit_ReportsDet(void)
{
    FrTp_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(1U, test_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_UNINIT, test_DetLastErrorId);
}

/*==================================================================================================
*                                  Transmit (SF / FF)
*================================================================================================*/
void test_Transmit_Uninit_ReportsDet(void)
{
    PduInfoType pdu;
    uint8 data[4] = { 1U, 2U, 3U, 4U };

    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_UNINIT, test_DetLastErrorId);
}

void test_Transmit_NullPdu_ReportsDet(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_Transmit(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_PARAM_POINTER, test_DetLastErrorId);
}

void test_Transmit_UnknownPduId_ReportsDet(void)
{
    PduInfoType pdu;
    uint8 data[4] = { 1U, 2U, 3U, 4U };

    test_init();
    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_Transmit(50U, &pdu));
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_INVALID_PDU_SDU_ID, test_DetLastErrorId);
}

void test_Transmit_SingleFrame_EntersWaitConfirm(void)
{
    test_init();
    test_transmit_sf();
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_WAIT_CONFIRM, RT(0).state);
    TEST_ASSERT_EQUAL_UINT8(FRTP_FLAG_TX_ACTIVE, RT(0).flags);
    TEST_ASSERT_EQUAL_UINT8(8U, RT(0).blockSize);
    TEST_ASSERT_EQUAL_UINT8(10U, RT(0).stMin);
    TEST_ASSERT_EQUAL_UINT16(100U, RT(0).timer);
    TEST_ASSERT_EQUAL_UINT32(8U, RT(0).dataLength);
    TEST_ASSERT_EQUAL_UINT32(0U, RT(0).bytesTransferred);
}

void test_Transmit_ZeroLength_AcceptedAsSf(void)
{
    PduInfoType pdu;
    uint8 data[1] = { 0U };

    test_init();
    pdu.SduDataPtr = data;
    pdu.SduLength = 0U;
    pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, FrTp_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_WAIT_CONFIRM, RT(0).state);
}

void test_Transmit_BusyConnection_Rejected(void)
{
    test_init();
    test_transmit_sf();
    /* second transmit attempt with valid pdu must fail because busy */
    {
        PduInfoType pdu;
        uint8 data[4] = { 1U, 2U, 3U, 4U };
        pdu.SduDataPtr = data;
        pdu.SduLength = sizeof(data);
        pdu.MetaDataPtr = NULL_PTR;
        TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_Transmit(0U, &pdu));
    }
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);  /* busy is silent */
}

void test_Transmit_MultiFrame_EntersWaitFc(void)
{
    test_init();
    test_transmit_ff300();
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_WAIT_FC, RT(0).state);
    TEST_ASSERT_EQUAL_UINT8(FRTP_FLAG_TX_ACTIVE, RT(0).flags);
    /* FF carries maxPayload(254) - pci(2) = 252 bytes, 48 remain */
    TEST_ASSERT_EQUAL_UINT32(252U, RT(0).bytesTransferred);
    TEST_ASSERT_EQUAL_UINT32(48U, RT(0).bytesRemaining);
    TEST_ASSERT_EQUAL_UINT16(100U, RT(0).timer);   /* N_Bs */
}

/*==================================================================================================
*                                  TxConfirmation
*================================================================================================*/
void test_TxConfirmation_SfComplete_NotifiesPduR(void)
{
    test_init();
    test_transmit_sf();
    /* NOTE (observed behaviour): Transmit() sets bytesRemaining = SduLength,
     * so the first successful confirmation moves the connection into
     * TX_SENDING_CF instead of completing; the follow-up CF cycle completes
     * the transfer. Documented here, not "fixed" in production code. */
    FrTp_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_SENDING_CF, RT(0).state);
    TEST_ASSERT_EQUAL_UINT16(0U, test_PdurTxConfCalls);

    /* CF cycle: one MainFunction sends the follow-up CF, the next one
     * sees bytesRemaining == 0 and waits for the final confirmation */
    (void)FrTp_MainFunction();
    (void)FrTp_MainFunction();
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_WAIT_CONFIRM, RT(0).state);

    FrTp_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurTxConfCalls);
    TEST_ASSERT_EQUAL(0U, test_PdurTxConfLastId);
    TEST_ASSERT_EQUAL(E_OK, test_PdurTxConfLastResult);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_TxConfirmation_FailureRetriesThenAborts(void)
{
    test_init();
    test_transmit_sf();

    FrTp_TxConfirmation(0U, E_NOT_OK);   /* retry 1 */
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_WAIT_CONFIRM, RT(0).state);
    FrTp_TxConfirmation(0U, E_NOT_OK);   /* retry 2 */
    TEST_ASSERT_EQUAL_UINT16(0U, test_PdurTxConfCalls);
    FrTp_TxConfirmation(0U, E_NOT_OK);   /* retry 3 -> abort */
    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurTxConfCalls);
    TEST_ASSERT_EQUAL(E_NOT_OK, test_PdurTxConfLastResult);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_TxConfirmation_UnknownPdu_ReportsDet(void)
{
    test_init();
    FrTp_TxConfirmation(99U, E_OK);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_INVALID_PDU_SDU_ID, test_DetLastErrorId);
}

void test_TxConfirmation_MultiFrame_CtsToComplete(void)
{
    test_init();
    test_transmit_ff300();

    /* FC(CTS) -> sending CFs */
    FrTp_ProcessFlowControl(0U, FRTP_FC_STATUS_CTS, 8U, 10U);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_SENDING_CF, RT(0).state);
    TEST_ASSERT_EQUAL_UINT8(0U, RT(0).sequenceNumber);
    TEST_ASSERT_EQUAL_UINT8(8U, RT(0).blockSize);

    /* first MainFunction sends the remaining 48 bytes in one CF */
    FrTp_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, RT(0).bytesRemaining);
    TEST_ASSERT_EQUAL_UINT8(1U, RT(0).sequenceNumber);

    /* next MainFunction sees bytesRemaining==0 -> wait for final confirm */
    FrTp_MainFunction();
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_WAIT_CONFIRM, RT(0).state);

    FrTp_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurTxConfCalls);
    TEST_ASSERT_EQUAL(E_OK, test_PdurTxConfLastResult);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_TxConfirmation_MultiFrame_FailureAborts(void)
{
    test_init();
    test_transmit_ff300();
    FrTp_ProcessFlowControl(0U, FRTP_FC_STATUS_CTS, 8U, 10U);
    (void)FrTp_MainFunction();

    FrTp_TxConfirmation(0U, E_NOT_OK);
    FrTp_TxConfirmation(0U, E_NOT_OK);
    TEST_ASSERT_EQUAL_UINT16(0U, test_PdurTxConfCalls);
    FrTp_TxConfirmation(0U, E_NOT_OK);
    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurTxConfCalls);
    TEST_ASSERT_EQUAL(E_NOT_OK, test_PdurTxConfLastResult);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

/*==================================================================================================
*                                  Flow control handling
*================================================================================================*/
void test_ProcessFlowControl_Wait_RestartsTimer(void)
{
    test_init();
    test_transmit_ff300();
    RT(0).timer = 5U;   /* almost expired */

    FrTp_ProcessFlowControl(0U, FRTP_FC_STATUS_WAIT, 0U, 0U);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_WAIT_FC, RT(0).state);
    TEST_ASSERT_EQUAL_UINT16(100U, RT(0).timer);   /* restarted with N_Bs */
    TEST_ASSERT_EQUAL_UINT16(0U, test_PdurTxConfCalls);
}

void test_ProcessFlowControl_Ovflw_Aborts(void)
{
    test_init();
    test_transmit_ff300();
    FrTp_ProcessFlowControl(0U, FRTP_FC_STATUS_OVFLW, 0U, 0U);
    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurTxConfCalls);
    TEST_ASSERT_EQUAL(E_NOT_OK, test_PdurTxConfLastResult);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_ProcessFlowControl_InvalidStatus_Aborts(void)
{
    test_init();
    test_transmit_ff300();
    FrTp_ProcessFlowControl(0U, 7U, 0U, 0U);   /* invalid flow status */
    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurTxConfCalls);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_ProcessFlowControl_WrongState_Ignored(void)
{
    test_init();   /* idle */
    FrTp_ProcessFlowControl(0U, FRTP_FC_STATUS_CTS, 8U, 10U);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
    TEST_ASSERT_EQUAL_UINT16(0U, test_PdurTxConfCalls);
}

void test_ProcessFlowControl_InvalidIdx_NoCrash(void)
{
    FrTp_ProcessFlowControl(FRTP_MAX_CONNECTIONS, FRTP_FC_STATUS_CTS, 8U, 10U);
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
}

/*==================================================================================================
*                                  CancelTransmit / CancelReceive
*================================================================================================*/
void test_CancelTransmit_Uninit_ReportsDet(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_CancelTransmit(0U));
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_UNINIT, test_DetLastErrorId);
}

void test_CancelTransmit_ActiveTx_Aborts(void)
{
    test_init();
    test_transmit_sf();
    TEST_ASSERT_EQUAL(E_OK, FrTp_CancelTransmit(0U));
    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurTxConfCalls);
    TEST_ASSERT_EQUAL(E_NOT_OK, test_PdurTxConfLastResult);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
    /* idle again -> cannot cancel */
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_CancelTransmit(0U));
}

void test_CancelTransmit_UnknownOrIdle_ReturnsNotOk(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_CancelTransmit(42U));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_CancelTransmit(2U));   /* connection 1 idle */
}

void test_CancelTransmit_RxActive_ReturnsNotOk(void)
{
    test_init();
    test_start_ff_reception(NULL_PTR, 0U);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_RX_WAIT_CF, RT(0).state);
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_CancelTransmit(0U));   /* RX flag, not TX */
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_RX_WAIT_CF, RT(0).state);
}

void test_CancelReceive_Uninit_ReportsDet(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_CancelReceive(1U));
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_UNINIT, test_DetLastErrorId);
}

void test_CancelReceive_RxActive_Cancels(void)
{
    test_init();
    test_start_ff_reception(NULL_PTR, 0U);
    TEST_ASSERT_EQUAL(E_OK, FrTp_CancelReceive(1U));
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
    /* idle again */
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_CancelReceive(1U));
}

void test_CancelReceive_TxActive_ReturnsNotOk(void)
{
    test_init();
    test_transmit_sf();
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_CancelReceive(1U));
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_WAIT_CONFIRM, RT(0).state);
}

/*==================================================================================================
*                                  MainFunction timeouts
*================================================================================================*/
void test_MainFunction_WaitConfirmTimeout_AbortsAfterRetries(void)
{
    test_init();
    test_transmit_sf();

    /* inject near-expiry: UpdateTimers decrements by 5, then the state
     * machine sees an expired timer. The WAIT_CONFIRM timeout handler and
     * StateWaitConfirm each count one retry per expired cycle. */
    RT(0).timer = 5U;
    FrTp_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(2U, RT(0).retryCount);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_WAIT_CONFIRM, RT(0).state);

    RT(0).timer = 5U;
    FrTp_MainFunction();   /* retries reach 3 -> abort */
    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurTxConfCalls);
    TEST_ASSERT_EQUAL(E_NOT_OK, test_PdurTxConfLastResult);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_MainFunction_WaitFcTimeout_RetriesThenResets(void)
{
    test_init();
    test_transmit_ff300();

    RT(0).timer = 5U;
    FrTp_MainFunction();
    /* WAIT_FC timeout: retry 1, transition to TX_STARTING; then the TX
     * state machine sees the expired timer and counts retry 2 */
    TEST_ASSERT_EQUAL_UINT8(2U, RT(0).retryCount);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_STARTING, RT(0).state);

    /* second expired cycle: timeout switch default branch resets connection */
    FrTp_MainFunction();
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_MainFunction_RxWaitCfTimeout_Resets(void)
{
    test_init();
    test_start_ff_reception(NULL_PTR, 0U);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_RX_WAIT_CF, RT(0).state);

    RT(0).timer = 5U;
    FrTp_MainFunction();
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_MainFunction_RxSendFc_SendsCtsAndWaitsForCf(void)
{
    test_init();
    RT(0).state = FRTP_STATE_RX_SEND_FC;
    RT(0).blockSize = 8U;
    RT(0).stMin = 10U;

    FrTp_MainFunction();
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_RX_WAIT_CF, RT(0).state);
    TEST_ASSERT_EQUAL_UINT16(100U, RT(0).timer);   /* N_Cr started */
}

void test_MainFunction_IdleConnections_NoAction(void)
{
    test_init();
    FrTp_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, test_DetCalls);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(3).state);
}

/*==================================================================================================
*                                  RxIndication
*================================================================================================*/
void test_RxIndication_Uninit_ReportsDet(void)
{
    PduInfoType pdu;
    uint8 frame[2] = { 0x05U, 0xAAU };

    pdu.SduDataPtr = frame;
    pdu.SduLength = sizeof(frame);
    pdu.MetaDataPtr = NULL_PTR;
    FrTp_RxIndication(1U, &pdu);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_UNINIT, test_DetLastErrorId);
}

void test_RxIndication_NullPduOrData_ReportsDet(void)
{
    test_init();
    FrTp_RxIndication(1U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_PARAM_POINTER, test_DetLastErrorId);

    {
        PduInfoType pdu;
        pdu.SduDataPtr = NULL_PTR;
        pdu.SduLength = 8U;
        pdu.MetaDataPtr = NULL_PTR;
        FrTp_RxIndication(1U, &pdu);
        TEST_ASSERT_EQUAL_UINT16(2U, test_DetCalls);
    }
}

void test_RxIndication_UnknownPduId_ReportsDet(void)
{
    PduInfoType pdu;
    uint8 frame[2] = { 0x05U, 0xAAU };

    test_init();
    pdu.SduDataPtr = frame;
    pdu.SduLength = sizeof(frame);
    pdu.MetaDataPtr = NULL_PTR;
    FrTp_RxIndication(9U, &pdu);
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_INVALID_PDU_SDU_ID, test_DetLastErrorId);
}

void test_RxIndication_SingleFrame_Completes(void)
{
    PduInfoType pdu;
    uint8 frame[6] = { 0x05U, 0xA1U, 0xA2U, 0xA3U, 0xA4U, 0xA5U };

    test_init();
    pdu.SduDataPtr = frame;
    pdu.SduLength = sizeof(frame);
    pdu.MetaDataPtr = NULL_PTR;
    FrTp_RxIndication(1U, &pdu);
    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurRxIndCalls);
    TEST_ASSERT_EQUAL(0U, test_PdurRxIndLastId);   /* connection index */
    TEST_ASSERT_EQUAL(E_OK, test_PdurRxIndLastResult);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_RxIndication_SfWithPreSetBuffer_CopiesData(void)
{
    PduInfoType pdu;
    uint8 frame[5] = { 0x04U, 0xB1U, 0xB2U, 0xB3U, 0xB4U };
    uint8 rxBuffer[16];

    test_init();
    (void)memset(rxBuffer, 0, sizeof(rxBuffer));
    RT(0).rxBuffer = rxBuffer;
    RT(0).rxBufferSize = sizeof(rxBuffer);

    pdu.SduDataPtr = frame;
    pdu.SduLength = sizeof(frame);
    pdu.MetaDataPtr = NULL_PTR;
    FrTp_RxIndication(1U, &pdu);

    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurRxIndCalls);
    TEST_ASSERT_EQUAL(E_OK, test_PdurRxIndLastResult);
    TEST_ASSERT_EQUAL_HEX8(0xB1U, rxBuffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0xB4U, rxBuffer[3]);
    /* NOTE: the SF reception path only memcpy's into the buffer; it does
     * not update bytesTransferred (only CF processing does). */
    TEST_ASSERT_EQUAL_UINT32(0U, RT(0).bytesTransferred);
}

void test_RxIndication_FirstFrame_EntersWaitCf(void)
{
    test_init();
    test_start_ff_reception(NULL_PTR, 0U);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_RX_WAIT_CF, RT(0).state);
    TEST_ASSERT_EQUAL_UINT8(FRTP_FLAG_RX_ACTIVE, RT(0).flags);
    TEST_ASSERT_EQUAL_UINT32(20U, RT(0).dataLength);
    TEST_ASSERT_EQUAL_UINT16(100U, RT(0).timer);   /* N_Cr */
}

void test_RxIndication_FirstFrameWithBuffer_CompletesOnCf(void)
{
    PduInfoType pdu;
    uint8 cf[5];
    uint8 rxBuffer[32];

    test_init();
    (void)memset(rxBuffer, 0, sizeof(rxBuffer));
    test_start_ff_reception(rxBuffer, sizeof(rxBuffer));
    TEST_ASSERT_EQUAL_UINT32(16U, RT(0).bytesTransferred);
    TEST_ASSERT_EQUAL_UINT32(4U, RT(0).bytesRemaining);

    /* CF with sequence 0 carrying the remaining 4 bytes */
    cf[0] = 0x80U;   /* CF, seq 0 */
    cf[1] = 0xC1U;
    cf[2] = 0xC2U;
    cf[3] = 0xC3U;
    cf[4] = 0xC4U;
    pdu.SduDataPtr = cf;
    pdu.SduLength = sizeof(cf);
    pdu.MetaDataPtr = NULL_PTR;
    FrTp_RxIndication(1U, &pdu);

    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurRxIndCalls);
    TEST_ASSERT_EQUAL(E_OK, test_PdurRxIndLastResult);
    TEST_ASSERT_EQUAL_HEX8(0xC1U, rxBuffer[16]);
    TEST_ASSERT_EQUAL_HEX8(0xC4U, rxBuffer[19]);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_RxIndication_CfWrongSeq_Aborts(void)
{
    PduInfoType pdu;
    uint8 cf[5];
    uint8 rxBuffer[32];

    test_init();
    (void)memset(rxBuffer, 0, sizeof(rxBuffer));
    test_start_ff_reception(rxBuffer, sizeof(rxBuffer));

    cf[0] = 0x83U;   /* CF, seq 3 (expected 0) */
    cf[1] = 0xD1U;
    cf[2] = 0xD2U;
    cf[3] = 0xD3U;
    cf[4] = 0xD4U;
    pdu.SduDataPtr = cf;
    pdu.SduLength = sizeof(cf);
    pdu.MetaDataPtr = NULL_PTR;
    FrTp_RxIndication(1U, &pdu);

    TEST_ASSERT_EQUAL_UINT8(FRTP_E_INVALID_SEQUENCE_NUMBER, test_DetLastErrorId);
    TEST_ASSERT_EQUAL_UINT16(1U, test_PdurRxIndCalls);
    TEST_ASSERT_EQUAL(E_NOT_OK, test_PdurRxIndLastResult);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_RxIndication_CfUnexpected_Ignored(void)
{
    PduInfoType pdu;
    uint8 cf[2] = { 0x80U, 0x00U };

    test_init();   /* idle -> CF ignored */
    pdu.SduDataPtr = cf;
    pdu.SduLength = sizeof(cf);
    pdu.MetaDataPtr = NULL_PTR;
    FrTp_RxIndication(1U, &pdu);
    TEST_ASSERT_EQUAL_UINT16(0U, test_PdurRxIndCalls);
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_IDLE, RT(0).state);
}

void test_RxIndication_SecondFfWhileBusy_SendsOvflw(void)
{
    test_init();
    test_start_ff_reception(NULL_PTR, 0U);
    test_start_ff_reception(NULL_PTR, 0U);   /* busy -> OVFLW, keep state */
    TEST_ASSERT_EQUAL_INT(FRTP_STATE_RX_WAIT_CF, RT(0).state);
    TEST_ASSERT_EQUAL_UINT32(20U, RT(0).dataLength);
}

void test_RxIndication_FlowControl_DrivesTxStateMachine(void)
{
    PduInfoType pdu;
    uint8 fc[3];

    test_init();
    test_transmit_ff300();   /* TX_WAIT_FC on connection 0 */

    fc[0] = (uint8)(FRTP_PCI_TYPE_FC | FRTP_FC_STATUS_CTS);
    fc[1] = 8U;
    fc[2] = 10U;
    pdu.SduDataPtr = fc;
    pdu.SduLength = sizeof(fc);
    pdu.MetaDataPtr = NULL_PTR;
    FrTp_RxIndication(1U, &pdu);

    TEST_ASSERT_EQUAL_INT(FRTP_STATE_TX_SENDING_CF, RT(0).state);
}

/*==================================================================================================
*                                  ChangeParameter
*================================================================================================*/
void test_ChangeParameter_Uninit_ReportsDet(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_ChangeParameter(0U, TP_STMIN, 50U));
    TEST_ASSERT_EQUAL_UINT8(FRTP_E_UNINIT, test_DetLastErrorId);
}

void test_ChangeParameter_StMinAndBs_Accepted(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_OK, FrTp_ChangeParameter(0U, TP_STMIN, 50U));
    TEST_ASSERT_EQUAL_UINT8(50U, RT(0).stMin);
    TEST_ASSERT_EQUAL(E_OK, FrTp_ChangeParameter(0U, TP_BS, 200U));
    TEST_ASSERT_EQUAL_UINT8(200U, RT(0).blockSize);
    /* found by rx pdu id as well */
    TEST_ASSERT_EQUAL(E_OK, FrTp_ChangeParameter(1U, TP_STMIN, 60U));
    TEST_ASSERT_EQUAL_UINT8(60U, RT(0).stMin);
}

void test_ChangeParameter_ValueTooLarge_Rejected(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_ChangeParameter(0U, TP_STMIN, 0x100U));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_ChangeParameter(0U, TP_BS, 0x100U));
}

void test_ChangeParameter_BcUnsupported_Rejected(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_ChangeParameter(0U, TP_BC, 5U));
}

void test_ChangeParameter_UnknownPduId_Rejected(void)
{
    test_init();
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_ChangeParameter(99U, TP_STMIN, 5U));
}

/*==================================================================================================
*                                  PDU encode / decode helpers
*================================================================================================*/
void test_EncodeSfPdu(void)
{
    uint8 buffer[8];
    PduInfoType pdu;
    uint8 pciLength = 0U;
    uint8 sfLen;

    pdu.SduDataPtr = NULL_PTR;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;

    /* NULL args rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_EncodeSfPdu(NULL_PTR, &pdu, &pciLength));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_EncodeSfPdu(buffer, NULL_PTR, &pciLength));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_EncodeSfPdu(buffer, &pdu, NULL_PTR));
    /* length too large for SF */
    pdu.SduLength = 64U;
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_EncodeSfPdu(buffer, &pdu, &pciLength));
    /* valid */
    pdu.SduLength = 8U;
    TEST_ASSERT_EQUAL(E_OK, FrTp_EncodeSfPdu(buffer, &pdu, &pciLength));
    TEST_ASSERT_EQUAL_UINT8(1U, pciLength);
    TEST_ASSERT_EQUAL_HEX8(0x08U, buffer[0]);

    TEST_ASSERT_EQUAL(E_OK, FrTp_DecodeSfPdu(buffer, &sfLen));
    TEST_ASSERT_EQUAL_UINT8(8U, sfLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_DecodeSfPdu(NULL_PTR, &sfLen));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_DecodeSfPdu(buffer, NULL_PTR));
}

void test_EncodeFfPdu(void)
{
    uint8 buffer[8];
    PduInfoType pdu;
    uint8 pciLength = 0U;
    uint16 ffLen;

    pdu.SduDataPtr = NULL_PTR;
    pdu.SduLength = 300U;
    pdu.MetaDataPtr = NULL_PTR;

    /* short payloads belong in SF, not FF */
    pdu.SduLength = 63U;
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_EncodeFfPdu(buffer, &pdu, &pciLength));

    pdu.SduLength = 300U;   /* 0x12C */
    TEST_ASSERT_EQUAL(E_OK, FrTp_EncodeFfPdu(buffer, &pdu, &pciLength));
    TEST_ASSERT_EQUAL_UINT8(2U, pciLength);
    TEST_ASSERT_EQUAL_HEX8(0x41U, buffer[0]);   /* FF | (300 >> 8) */
    TEST_ASSERT_EQUAL_HEX8(0x2CU, buffer[1]);

    TEST_ASSERT_EQUAL(E_OK, FrTp_DecodeFfPdu(buffer, &ffLen));
    TEST_ASSERT_EQUAL_UINT16(300U, ffLen);
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_DecodeFfPdu(NULL_PTR, &ffLen));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_DecodeFfPdu(buffer, NULL_PTR));
}

void test_EncodeCfPdu(void)
{
    uint8 buffer[8];
    PduInfoType pdu;
    uint8 pciLength = 0U;
    uint8 seq;

    pdu.SduDataPtr = NULL_PTR;
    pdu.SduLength = 0U;
    pdu.MetaDataPtr = NULL_PTR;

    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_EncodeCfPdu(NULL_PTR, &pdu, 5U, &pciLength));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_EncodeCfPdu(buffer, NULL_PTR, 5U, &pciLength));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_EncodeCfPdu(buffer, &pdu, 5U, NULL_PTR));
    TEST_ASSERT_EQUAL(E_OK, FrTp_EncodeCfPdu(buffer, &pdu, 5U, &pciLength));
    TEST_ASSERT_EQUAL_UINT8(1U, pciLength);
    TEST_ASSERT_EQUAL_HEX8(0x85U, buffer[0]);   /* CF | seq 5 */

    TEST_ASSERT_EQUAL(E_OK, FrTp_DecodeCfPdu(buffer, &seq));
    TEST_ASSERT_EQUAL_UINT8(5U, seq);
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_DecodeCfPdu(NULL_PTR, &seq));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_DecodeCfPdu(buffer, NULL_PTR));
}

void test_EncodeFcPdu(void)
{
    uint8 buffer[8];
    uint8 pciLength = 0U;
    uint8 fs, bs, stmin;

    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_EncodeFcPdu(NULL_PTR, 0U, 0U, 0U, &pciLength));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_EncodeFcPdu(buffer, 0U, 0U, 0U, NULL_PTR));
    TEST_ASSERT_EQUAL(E_OK, FrTp_EncodeFcPdu(buffer, FRTP_FC_STATUS_CTS, 8U, 10U, &pciLength));
    TEST_ASSERT_EQUAL_UINT8(3U, pciLength);
    TEST_ASSERT_EQUAL_HEX8(0xC0U, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x08U, buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(0x0AU, buffer[2]);

    TEST_ASSERT_EQUAL(E_OK, FrTp_DecodeFcPdu(buffer, &fs, &bs, &stmin));
    TEST_ASSERT_EQUAL_UINT8(FRTP_FC_STATUS_CTS, fs);
    TEST_ASSERT_EQUAL_UINT8(8U, bs);
    TEST_ASSERT_EQUAL_UINT8(10U, stmin);
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_DecodeFcPdu(NULL_PTR, &fs, &bs, &stmin));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_DecodeFcPdu(buffer, NULL_PTR, &bs, &stmin));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_DecodeFcPdu(buffer, &fs, NULL_PTR, &stmin));
    TEST_ASSERT_EQUAL(E_NOT_OK, FrTp_DecodeFcPdu(buffer, &fs, &bs, NULL_PTR));
}

void test_DecodePduType_AllTypes(void)
{
    uint8 sf[1] = { 0x08U };
    uint8 ff[1] = { 0x41U };
    uint8 cf[1] = { 0x85U };
    uint8 fc[1] = { 0xC0U };

    TEST_ASSERT_EQUAL_INT(FRTP_PDU_SF, FrTp_DecodePduType(sf));
    TEST_ASSERT_EQUAL_INT(FRTP_PDU_FF, FrTp_DecodePduType(ff));
    TEST_ASSERT_EQUAL_INT(FRTP_PDU_CF, FrTp_DecodePduType(cf));
    TEST_ASSERT_EQUAL_INT(FRTP_PDU_FC, FrTp_DecodePduType(fc));
    TEST_ASSERT_EQUAL_INT(FRTP_PDU_SF, FrTp_DecodePduType(NULL_PTR));
}

/*==================================================================================================
*                                  Timer / connection helpers
*================================================================================================*/
void test_TimerHelpers(void)
{
    FrTp_ConnectionRuntimeType* conn = &RT(1);
    uint8 i;

    /* NULL guards */
    FrTp_StartTimer(NULL_PTR, 10U);
    FrTp_StopTimer(NULL_PTR);
    TEST_ASSERT_EQUAL(FALSE, FrTp_IsTimerRunning(NULL_PTR));
    TEST_ASSERT_EQUAL(FALSE, FrTp_IsTimerExpired(NULL_PTR));

    FrTp_StopTimer(conn);
    TEST_ASSERT_EQUAL(FALSE, FrTp_IsTimerRunning(conn));
    TEST_ASSERT_EQUAL(FALSE, FrTp_IsTimerExpired(conn));

    FrTp_StartTimer(conn, 100U);
    TEST_ASSERT_EQUAL(TRUE, FrTp_IsTimerRunning(conn));
    TEST_ASSERT_EQUAL_UINT16(100U, conn->timeoutValue);

    /* 19 updates leave 5 ms, the 20th expires the timer */
    for (i = 0U; i < 19U; i++)
    {
        FrTp_UpdateTimers();
    }
    TEST_ASSERT_EQUAL(FALSE, FrTp_IsTimerExpired(conn));
    FrTp_UpdateTimers();
    TEST_ASSERT_EQUAL(TRUE, FrTp_IsTimerExpired(conn));

    FrTp_StopTimer(conn);
    TEST_ASSERT_EQUAL(FALSE, FrTp_IsTimerRunning(conn));
}

void test_ConnectionLookupHelpers(void)
{
    TEST_ASSERT_EQUAL_UINT8(0U, FrTp_FindConnectionByTxPdu(0U));
    TEST_ASSERT_EQUAL_UINT8(1U, FrTp_FindConnectionByTxPdu(2U));
    TEST_ASSERT_EQUAL_UINT8(FRTP_MAX_CONNECTIONS, FrTp_FindConnectionByTxPdu(50U));
    TEST_ASSERT_EQUAL_UINT8(0U, FrTp_FindConnectionByRxPdu(1U));
    TEST_ASSERT_EQUAL_UINT8(3U, FrTp_FindConnectionByRxPdu(7U));
    TEST_ASSERT_EQUAL_UINT8(FRTP_MAX_CONNECTIONS, FrTp_FindConnectionByRxPdu(50U));

    TEST_ASSERT_NOT_NULL(FrTp_GetConnectionRuntime(0U));
    TEST_ASSERT_NULL(FrTp_GetConnectionRuntime(FRTP_MAX_CONNECTIONS));
    TEST_ASSERT_NOT_NULL(FrTp_GetConnectionConfig(3U));
    TEST_ASSERT_NULL(FrTp_GetConnectionConfig(FRTP_MAX_CONNECTIONS));

    /* invalid index writes are ignored */
    FrTp_SetConnectionState(FRTP_MAX_CONNECTIONS, FRTP_STATE_TX_SENDING_CF);
    TEST_ASSERT_EQUAL(FALSE, FrTp_IsConnectionIdle(FRTP_MAX_CONNECTIONS));
    TEST_ASSERT_EQUAL(TRUE, FrTp_IsConnectionIdle(0U));
}

void test_BufferRequestRelease(void)
{
    uint8 rxBuffer[16];

    test_init();
    TEST_ASSERT_EQUAL(BUFREQ_E_OK, FrTp_RequestTxBuffer(0U, 10U));

    FrTp_ReleaseTxBuffer(0U);
    TEST_ASSERT_NULL(RT(0).txPduInfo);

    TEST_ASSERT_EQUAL(BUFREQ_E_OK, FrTp_RequestRxBuffer(0U, 10U));
    TEST_ASSERT_EQUAL(TRUE, RT(0).rxBufferLocked);

    RT(0).rxBuffer = rxBuffer;
    RT(0).rxBufferSize = sizeof(rxBuffer);
    FrTp_ReleaseRxBuffer(0U);
    TEST_ASSERT_NULL(RT(0).rxBuffer);
    TEST_ASSERT_EQUAL_UINT32(0U, RT(0).rxBufferSize);
    TEST_ASSERT_EQUAL(FALSE, RT(0).rxBufferLocked);
}

/*==================================================================================================
*                                  main
*================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Init_NullPtr_ReportsDet);
    RUN_TEST(test_Init_TooManyConnections_ReportsDet);
    RUN_TEST(test_Init_NullConnections_ReportsDet);
    RUN_TEST(test_Init_Valid_InitializesRuntime);
    RUN_TEST(test_Init_Twice_ReportsDet);
    RUN_TEST(test_DeInit_Uninit_ReportsDet);
    RUN_TEST(test_DeInit_AfterInit_ClearsState);
    RUN_TEST(test_GetVersionInfo_Valid);
    RUN_TEST(test_GetVersionInfo_Null_ReportsDet);
    RUN_TEST(test_MainFunction_Uninit_ReportsDet);

    RUN_TEST(test_Transmit_Uninit_ReportsDet);
    RUN_TEST(test_Transmit_NullPdu_ReportsDet);
    RUN_TEST(test_Transmit_UnknownPduId_ReportsDet);
    RUN_TEST(test_Transmit_SingleFrame_EntersWaitConfirm);
    RUN_TEST(test_Transmit_ZeroLength_AcceptedAsSf);
    RUN_TEST(test_Transmit_BusyConnection_Rejected);
    RUN_TEST(test_Transmit_MultiFrame_EntersWaitFc);

    RUN_TEST(test_TxConfirmation_SfComplete_NotifiesPduR);
    RUN_TEST(test_TxConfirmation_FailureRetriesThenAborts);
    RUN_TEST(test_TxConfirmation_UnknownPdu_ReportsDet);
    RUN_TEST(test_TxConfirmation_MultiFrame_CtsToComplete);
    RUN_TEST(test_TxConfirmation_MultiFrame_FailureAborts);

    RUN_TEST(test_ProcessFlowControl_Wait_RestartsTimer);
    RUN_TEST(test_ProcessFlowControl_Ovflw_Aborts);
    RUN_TEST(test_ProcessFlowControl_InvalidStatus_Aborts);
    RUN_TEST(test_ProcessFlowControl_WrongState_Ignored);
    RUN_TEST(test_ProcessFlowControl_InvalidIdx_NoCrash);

    RUN_TEST(test_CancelTransmit_Uninit_ReportsDet);
    RUN_TEST(test_CancelTransmit_ActiveTx_Aborts);
    RUN_TEST(test_CancelTransmit_UnknownOrIdle_ReturnsNotOk);
    RUN_TEST(test_CancelTransmit_RxActive_ReturnsNotOk);
    RUN_TEST(test_CancelReceive_Uninit_ReportsDet);
    RUN_TEST(test_CancelReceive_RxActive_Cancels);
    RUN_TEST(test_CancelReceive_TxActive_ReturnsNotOk);

    RUN_TEST(test_MainFunction_WaitConfirmTimeout_AbortsAfterRetries);
    RUN_TEST(test_MainFunction_WaitFcTimeout_RetriesThenResets);
    RUN_TEST(test_MainFunction_RxWaitCfTimeout_Resets);
    RUN_TEST(test_MainFunction_RxSendFc_SendsCtsAndWaitsForCf);
    RUN_TEST(test_MainFunction_IdleConnections_NoAction);

    RUN_TEST(test_RxIndication_Uninit_ReportsDet);
    RUN_TEST(test_RxIndication_NullPduOrData_ReportsDet);
    RUN_TEST(test_RxIndication_UnknownPduId_ReportsDet);
    RUN_TEST(test_RxIndication_SingleFrame_Completes);
    RUN_TEST(test_RxIndication_SfWithPreSetBuffer_CopiesData);
    RUN_TEST(test_RxIndication_FirstFrame_EntersWaitCf);
    RUN_TEST(test_RxIndication_FirstFrameWithBuffer_CompletesOnCf);
    RUN_TEST(test_RxIndication_CfWrongSeq_Aborts);
    RUN_TEST(test_RxIndication_CfUnexpected_Ignored);
    RUN_TEST(test_RxIndication_SecondFfWhileBusy_SendsOvflw);
    RUN_TEST(test_RxIndication_FlowControl_DrivesTxStateMachine);

    RUN_TEST(test_ChangeParameter_Uninit_ReportsDet);
    RUN_TEST(test_ChangeParameter_StMinAndBs_Accepted);
    RUN_TEST(test_ChangeParameter_ValueTooLarge_Rejected);
    RUN_TEST(test_ChangeParameter_BcUnsupported_Rejected);
    RUN_TEST(test_ChangeParameter_UnknownPduId_Rejected);

    RUN_TEST(test_EncodeSfPdu);
    RUN_TEST(test_EncodeFfPdu);
    RUN_TEST(test_EncodeCfPdu);
    RUN_TEST(test_EncodeFcPdu);
    RUN_TEST(test_DecodePduType_AllTypes);

    RUN_TEST(test_TimerHelpers);
    RUN_TEST(test_ConnectionLookupHelpers);
    RUN_TEST(test_BufferRequestRelease);

    return UNITY_END();
}
