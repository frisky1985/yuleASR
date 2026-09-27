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
static uint8 mock_PduRRxCalls = 0;

/* CAN FD test configuration storage (channel 0 flagged as CAN FD) */
static CanTp_ChannelConfigType testFdChannel;
static CanTp_ConfigType testFdConfig;

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
    mock_PduRRxCalls++;
}

void PduR_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    (void)TxPduId;(void)result;
}

void setUp(void) {
    mock_DetCalls = 0;
    mock_CanIfCalls = 0;
    mock_CanIfPduId = 0xFFFFU;
    mock_PduRRxCalls = 0;
    for (uint8 i = 0U; i < CANTP_CAN_FRAME_LENGTH; i++) { mock_CanIfFrame[i] = 0U; }
}
void tearDown(void) {}

/* Build an FD-capable config: channel 0 of the production config with CanFdEnabled set. */
static void test_SetupFdConfig(void) {
    testFdChannel = CanTp_Config.ChannelConfigs[0];
    testFdChannel.CanFdEnabled = TRUE;
    testFdConfig.GeneralConfig = NULL_PTR;
    testFdConfig.ChannelConfigs = &testFdChannel;
    testFdConfig.NumChannels = 1U;
}

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

/** @req SWS_CanTp_00009 */
void test_CanTp_Stmin_GatesNextCf(void) {
    PduInfoType pdu;
    PduInfoType fcPdu;
    uint8 data[16];
    uint8 fcData[8] = {0x30U, 0x08U, 0x05U, 0U, 0U, 0U, 0U, 0U};
    for (uint8 i = 0U; i < 16U; i++) { data[i] = (uint8)i; }
    pdu.SduDataPtr = data; pdu.SduLength = 16U; pdu.MetaDataPtr = NULL_PTR;
    fcPdu.SduDataPtr = fcData; fcPdu.SduLength = 8U; fcPdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);

    /* 16-byte message: First Frame is sent, then N_Bs wait */
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);

    /* FC with CTS and STmin = 5 ms: the first CF goes out immediately */
    CanTp_RxIndication(CANTP_CANIF_FC_RX_PDU_ID, &fcPdu);
    TEST_ASSERT_EQUAL(2, mock_CanIfCalls);
    TEST_ASSERT_EQUAL_HEX8(0x21U, mock_CanIfFrame[0]);

    /* The next CF must not be sent before the 5 ms separation has elapsed */
    for (uint8 i = 0U; i < 4U; i++) {
        CanTp_MainFunction();
    }
    TEST_ASSERT_EQUAL(2, mock_CanIfCalls);

    /* Exactly on the 5th ms the separation expires: second CF is sent */
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(3, mock_CanIfCalls);
    TEST_ASSERT_EQUAL_HEX8(0x22U, mock_CanIfFrame[0]);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_StminZero_SendsImmediately(void) {
    PduInfoType pdu;
    PduInfoType fcPdu;
    uint8 data[16];
    uint8 fcData[8] = {0x30U, 0x08U, 0x00U, 0U, 0U, 0U, 0U, 0U};
    for (uint8 i = 0U; i < 16U; i++) { data[i] = (uint8)i; }
    pdu.SduDataPtr = data; pdu.SduLength = 16U; pdu.MetaDataPtr = NULL_PTR;
    fcPdu.SduDataPtr = fcData; fcPdu.SduLength = 8U; fcPdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);

    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    CanTp_RxIndication(CANTP_CANIF_FC_RX_PDU_ID, &fcPdu);
    TEST_ASSERT_EQUAL(2, mock_CanIfCalls);
    TEST_ASSERT_EQUAL_HEX8(0x21U, mock_CanIfFrame[0]);

    /* STmin = 0: the next CF is sent without any separation delay */
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(3, mock_CanIfCalls);
    TEST_ASSERT_EQUAL_HEX8(0x22U, mock_CanIfFrame[0]);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_FdFirstFrame_AcceptedOnFdChannel(void) {
    uint8 ffData[12] = {0x10U, 0x0CU, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU, 0x01U, 0x02U, 0x03U, 0x04U};
    PduInfoType ffPdu;
    ffPdu.SduDataPtr = ffData; ffPdu.SduLength = 12U; ffPdu.MetaDataPtr = NULL_PTR;

    test_SetupFdConfig();
    CanTp_Init(&testFdConfig);

    /* 12-byte CAN FD First Frame (message DL = 12) is accepted: FC CTS is sent */
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &ffPdu);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
    TEST_ASSERT_EQUAL(CANTP_CANIF_FC_TX_PDU_ID, mock_CanIfPduId);
    TEST_ASSERT_EQUAL_HEX8(0x30U, mock_CanIfFrame[0]);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_FdFirstFrame_RejectedOnClassicChannel(void) {
    uint8 ffData[12] = {0x10U, 0x0CU, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU, 0x01U, 0x02U, 0x03U, 0x04U};
    uint8 ff8Data[8] = {0x10U, 0x0CU, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU};
    PduInfoType ffPdu;
    ffPdu.SduDataPtr = ffData; ffPdu.SduLength = 12U; ffPdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);

    /* 12-byte frame on a classic CAN channel is rejected: no Flow Control */
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &ffPdu);
    TEST_ASSERT_EQUAL(0, mock_CanIfCalls);

    /* Classic 8-byte FF with message DL = 12 keeps the legacy behaviour (accepted) */
    ffPdu.SduDataPtr = ff8Data; ffPdu.SduLength = 8U;
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &ffPdu);
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
    TEST_ASSERT_EQUAL_HEX8(0x30U, mock_CanIfFrame[0]);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_FrameLengthBoundaries(void) {
    static const uint16 illegalFdLengths[] = {9U, 11U, 13U};
    static const uint16 legalFdLengths[] = {12U, 16U, 20U, 24U, 32U, 48U, 64U};
    uint8 frame[64];
    uint8 sfData[8] = {0x03U, 0x11U, 0x22U, 0x33U, 0U, 0U, 0U, 0U};
    PduInfoType pdu;
    pdu.SduDataPtr = frame; pdu.SduLength = 0U; pdu.MetaDataPtr = NULL_PTR;

    test_SetupFdConfig();

    /* CAN FD: lengths outside the DLC payload set are rejected without any activity */
    for (uint8 i = 0U; i < (uint8)(sizeof(illegalFdLengths) / sizeof(illegalFdLengths[0])); i++) {
        for (uint8 j = 0U; j < 64U; j++) { frame[j] = 0U; }
        frame[0] = 0x10U; frame[1] = 0x0CU;  /* FF with message DL = 12 */
        pdu.SduLength = illegalFdLengths[i];
        mock_CanIfCalls = 0;

        CanTp_Init(&testFdConfig);
        CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &pdu);
        TEST_ASSERT_EQUAL(0, mock_CanIfCalls);
    }

    /* CAN FD: all legal DLC payload lengths are accepted (FC CTS is sent) */
    for (uint8 i = 0U; i < (uint8)(sizeof(legalFdLengths) / sizeof(legalFdLengths[0])); i++) {
        for (uint8 j = 0U; j < 64U; j++) { frame[j] = 0U; }
        frame[0] = 0x10U; frame[1] = 0x0CU;
        pdu.SduLength = legalFdLengths[i];
        mock_CanIfCalls = 0;

        CanTp_Init(&testFdConfig);
        CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &pdu);
        TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
        TEST_ASSERT_EQUAL_HEX8(0x30U, mock_CanIfFrame[0]);
    }

    /* Classic CAN: frame lengths 1..8 are legal, 9 bytes are rejected */
    CanTp_Init(&CanTp_Config);
    pdu.SduDataPtr = sfData;
    for (uint8 i = 1U; i <= 8U; i++) {
        pdu.SduLength = i;
        mock_PduRRxCalls = 0;
        CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &pdu);
        TEST_ASSERT_EQUAL(1, mock_PduRRxCalls);
    }
    pdu.SduLength = 9U;
    mock_PduRRxCalls = 0;
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &pdu);
    TEST_ASSERT_EQUAL(0, mock_PduRRxCalls);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanTp_00003 */
void test_CanTp_Transmit_LengthAboveMax_ShouldReportDet(void) {
    PduInfoType pdu;
    uint8 data = 0U;
    pdu.SduDataPtr = &data;
    pdu.SduLength = CANTP_CANFD_MAX_MESSAGE_LENGTH + 1U;
    pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    Std_ReturnType ret = CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(1, mock_DetCalls);  /* CANTP_E_INVALID_TX_LENGTH */
    TEST_ASSERT_EQUAL(0, mock_CanIfCalls);
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
    RUN_TEST(test_CanTp_Stmin_GatesNextCf);
    RUN_TEST(test_CanTp_StminZero_SendsImmediately);
    RUN_TEST(test_CanTp_FdFirstFrame_AcceptedOnFdChannel);
    RUN_TEST(test_CanTp_FdFirstFrame_RejectedOnClassicChannel);
    RUN_TEST(test_CanTp_FrameLengthBoundaries);
    RUN_TEST(test_CanTp_Transmit_LengthAboveMax_ShouldReportDet);

    return UnityEnd();
}
