/**
 * @file test_cantp.c
 * @brief CanTp (CAN Transport Protocol) Unit Tests
 * @req SWS_CanTp
 */

// @tests src/bsw/ecual/cantp/src/CanTp.c  @tests src/bsw/ecual/cantp/src/CanTp_Lcfg.c  @tests src/bsw/ecual/cantp/include/CanTp.h
#include "unity.h"
#include "CanTp.h"
#include "CanTp_Cfg.h"

/* Lower-layer stubs. Signatures match CanIf.h / PduR.h / Det.h. */
static uint8 mock_DetCalls = 0;
static uint8 mock_CanIfCalls = 0;
static PduIdType mock_CanIfPduId = 0xFFFFU;
static uint8 mock_CanIfFrame[CANTP_CAN_FRAME_LENGTH];

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    mock_CanIfCalls++;
    mock_CanIfPduId = TxPduId;
    for (uint8 i = 0U; i < CANTP_CAN_FRAME_LENGTH; i++) {
        mock_CanIfFrame[i] = (i < PduInfoPtr->SduLength) ? PduInfoPtr->SduDataPtr[i] : 0U;
    }
    return E_OK;
}

void PduR_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)RxPduId;(void)PduInfoPtr;
}

void PduR_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    (void)TxPduId;(void)result;
}

void setUp(void) {
    mock_DetCalls = 0;
    mock_CanIfCalls = 0;
    mock_CanIfPduId = 0xFFFFU;
    for (uint8 i = 0U; i < CANTP_CAN_FRAME_LENGTH; i++) { mock_CanIfFrame[i] = 0U; }
}
void tearDown(void) {}

/* NOTE: runner executes in declaration order below and CanTp keeps static
 * state across tests, so uninit-dependent tests are declared before any
 * successful CanTp_Init(). */

/** @req SWS_CanTp_00001 */
void test_CanTp_Init_NullPtr_ShouldReportDet(void) {
    CanTp_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);
}

/** @req SWS_CanTp_00003 */
void test_CanTp_Transmit_BeforeInit_ShouldFail(void) {
    PduInfoType pdu;
    uint8 data[3] = {0x11U, 0x22U, 0x33U};
    pdu.SduDataPtr = data; pdu.SduLength = 3U; pdu.MetaDataPtr = NULL_PTR;
    Std_ReturnType ret = CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1, mock_DetCalls); /* CANTP_E_UNINIT */
    TEST_ASSERT_EQUAL(0, mock_CanIfCalls);
}

/** @req SWS_CanTp_00001 */
void test_CanTp_Init_ValidConfig_ShouldSucceed(void) {
    PduInfoType pdu;
    uint8 data[3] = {0x11U, 0x22U, 0x33U};
    pdu.SduDataPtr = data; pdu.SduLength = 3U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);

    /* Single Frame: PCI 0x03, payload, padding with CANTP_PADDING_BYTE_VALUE */
    Std_ReturnType ret = CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
    TEST_ASSERT_EQUAL(CANTP_CANIF_TX_PDU_ID, mock_CanIfPduId);
    TEST_ASSERT_EQUAL_HEX8(0x03U, mock_CanIfFrame[0]);
    TEST_ASSERT_EQUAL_HEX8(0x11U, mock_CanIfFrame[1]);
    TEST_ASSERT_EQUAL_HEX8(0x22U, mock_CanIfFrame[2]);
    TEST_ASSERT_EQUAL_HEX8(0x33U, mock_CanIfFrame[3]);
    TEST_ASSERT_EQUAL_HEX8(CANTP_PADDING_BYTE_VALUE, mock_CanIfFrame[7]);
}

/** @req SWS_CanTp_00002 */
void test_CanTp_Shutdown_AfterInit_ShouldSucceed(void) {
    PduInfoType pdu;
    uint8 data[3] = {0x11U, 0x22U, 0x33U};
    pdu.SduDataPtr = data; pdu.SduLength = 3U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    CanTp_Shutdown();
    TEST_ASSERT_EQUAL(0, mock_DetCalls);

    /* After Shutdown the module is uninitialized again: Transmit must fail */
    Std_ReturnType ret = CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1, mock_DetCalls); /* CANTP_E_UNINIT */
    TEST_ASSERT_EQUAL(0, mock_CanIfCalls);
}

/** @req SWS_CanTp_00003 */
void test_CanTp_Transmit_NullPdu_ShouldFail(void) {
    CanTp_Init(&CanTp_Config);
    Std_ReturnType ret = CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1, mock_DetCalls); /* CANTP_E_PARAM_POINTER */
    TEST_ASSERT_EQUAL(0, mock_CanIfCalls);
}

/** @req SWS_CanTp_00004 */
void test_CanTp_CancelTransmit_ActiveTx_ShouldSucceed(void) {
    PduInfoType pdu;
    uint8 data[10] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 10U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));

    /* Multi-frame Tx is in CANTP_CH_TX_WAIT_FC: cancel must free the channel */
    TEST_ASSERT_EQUAL(E_OK, CanTp_CancelTransmit(CANTP_TX_DIAG_PHYSICAL));
    /* Channel already idle: second cancel finds nothing */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanTp_CancelTransmit(CANTP_TX_DIAG_PHYSICAL));
}

/** @req SWS_CanTp_00005 */
void test_CanTp_CancelReceive_NoActiveRx_ShouldFail(void) {
    CanTp_Init(&CanTp_Config);
    Std_ReturnType ret = CanTp_CancelReceive(CANTP_RX_DIAG_PHYSICAL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_CanTp_00006 */
void test_CanTp_ChangeParameter_ActiveNsdu_ShouldUpdate(void) {
    PduInfoType pdu;
    uint8 data[10] = {0U};
    uint16 value = 0U;
    pdu.SduDataPtr = data; pdu.SduLength = 10U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));

    TEST_ASSERT_EQUAL(E_OK, CanTp_ChangeParameter(CANTP_TX_DIAG_PHYSICAL, TP_BS, 10U));
    TEST_ASSERT_EQUAL(E_OK, CanTp_ReadParameter(CANTP_TX_DIAG_PHYSICAL, TP_BS, &value));
    TEST_ASSERT_EQUAL(10U, value);
}

/** @req SWS_CanTp_00007 */
void test_CanTp_ReadParameter_ActiveNsdu_ShouldReturnValue(void) {
    PduInfoType pdu;
    uint8 data[3] = {0U};
    uint16 value = 0U;
    pdu.SduDataPtr = data; pdu.SduLength = 3U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));

    TEST_ASSERT_EQUAL(E_OK, CanTp_ChangeParameter(CANTP_TX_DIAG_PHYSICAL, TP_STMIN, 5U));
    TEST_ASSERT_EQUAL(E_OK, CanTp_ReadParameter(CANTP_TX_DIAG_PHYSICAL, TP_STMIN, &value));
    TEST_ASSERT_EQUAL(5U, value);

    /* Null value pointer must be rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanTp_ReadParameter(CANTP_TX_DIAG_PHYSICAL, TP_STMIN, NULL_PTR));
}

/** @req SWS_CanTp_00008 */
void test_CanTp_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    CanTp_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(CANTP_VENDOR_ID, info.vendorID);
}

/** @req SWS_CanTp_00008 */
void test_CanTp_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    CanTp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_MainFunction_AfterInit_ShouldNotCrash(void) {
    CanTp_Init(&CanTp_Config);
    CanTp_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_MainFunction_NbsTimeout_ShouldResetChannel(void) {
    PduInfoType pdu;
    uint8 data[10] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 10U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);

    /* First Frame emitted, then CANTP_CH_TX_WAIT_FC with N_Bs = 75 ms */
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    TEST_ASSERT_EQUAL_HEX8(0x10U, mock_CanIfFrame[0]); /* FF PCI */
    TEST_ASSERT_EQUAL_HEX8(10U, mock_CanIfFrame[1]);   /* message length */
    TEST_ASSERT_EQUAL(E_OK, CanTp_CancelTransmit(CANTP_TX_DIAG_PHYSICAL)); /* channel active */

    /* Re-arm the multi-frame Tx and let N_Bs expire in MainFunction */
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    for (uint16 i = 0U; i < CANTP_NBS_DEFAULT; i++) {
        CanTp_MainFunction();
    }
    /* N_Bs timeout reset the channel: cancel finds no active Tx anymore */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanTp_CancelTransmit(CANTP_TX_DIAG_PHYSICAL));
}

void test_CanTp_Init_DoubleInit_ShouldReplaceConfig(void) {
    PduInfoType pdu;
    uint8 data[3] = {0x11U, 0x22U, 0x33U};
    pdu.SduDataPtr = data; pdu.SduLength = 3U; pdu.MetaDataPtr = NULL_PTR;

    /* Empty config first: no NSDUs resolvable */
    CanTp_ConfigType emptyConfig;
    emptyConfig.GeneralConfig = NULL_PTR;
    emptyConfig.ChannelConfigs = NULL_PTR;
    emptyConfig.NumChannels = 0U;
    CanTp_Init(&emptyConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));

    /* Second Init replaces the config: Tx must succeed now */
    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_CanTp_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_CanTp_Transmit_BeforeInit_ShouldFail);
    RUN_TEST(test_CanTp_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_CanTp_Shutdown_AfterInit_ShouldSucceed);
    RUN_TEST(test_CanTp_Transmit_NullPdu_ShouldFail);
    RUN_TEST(test_CanTp_CancelTransmit_ActiveTx_ShouldSucceed);
    RUN_TEST(test_CanTp_CancelReceive_NoActiveRx_ShouldFail);
    RUN_TEST(test_CanTp_ChangeParameter_ActiveNsdu_ShouldUpdate);
    RUN_TEST(test_CanTp_ReadParameter_ActiveNsdu_ShouldReturnValue);
    RUN_TEST(test_CanTp_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_CanTp_GetVersionInfo_NullPtr_ShouldReportDet);
    RUN_TEST(test_CanTp_MainFunction_AfterInit_ShouldNotCrash);
    RUN_TEST(test_CanTp_MainFunction_NbsTimeout_ShouldResetChannel);
    RUN_TEST(test_CanTp_Init_DoubleInit_ShouldReplaceConfig);

    return UnityEnd();
}
