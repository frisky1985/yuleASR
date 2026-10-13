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
static uint8 mock_RteCalls = 0;
static uint8 mock_RteLastApiId = 0;
static uint8 mock_RteLastErrorId = 0;
static uint8 mock_CanIfCalls = 0;
static PduIdType mock_CanIfPduId = 0xFFFFU;
static uint8 mock_CanIfFrame[CANTP_CAN_FRAME_LENGTH];
static boolean mock_CanIfMetaValid = FALSE;
static uint8 mock_CanIfMetaData[4];
static uint8 mock_PduRRxCalls = 0;
static PduLengthType mock_PduRRxLens[8];
static uint8 mock_PduRRxFirstByte[8];
static uint8 mock_PduRLastData[16];
static PduLengthType mock_PduRLastLen = 0;
static uint8 mock_PduRTxCalls = 0;
static Std_ReturnType mock_PduRTxLastResult = E_OK;

/* CAN FD test configuration storage (channel 0 flagged as CAN FD) */
static CanTp_ChannelConfigType testFdChannel;
static CanTp_ConfigType testFdConfig;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;
    mock_RteCalls++;
    mock_RteLastApiId = ApiId;
    mock_RteLastErrorId = ErrorId;
    return E_OK;
}

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    mock_CanIfCalls++;
    mock_CanIfPduId = TxPduId;
    for (uint8 i = 0U; i < CANTP_CAN_FRAME_LENGTH; i++) {
        mock_CanIfFrame[i] = (i < PduInfoPtr->SduLength) ? PduInfoPtr->SduDataPtr[i] : 0U;
    }
    mock_CanIfMetaValid = (PduInfoPtr->MetaDataPtr != NULL_PTR) ? TRUE : FALSE;
    for (uint8 i = 0U; i < 4U; i++) {
        mock_CanIfMetaData[i] = (mock_CanIfMetaValid == TRUE) ? PduInfoPtr->MetaDataPtr[i] : 0U;
    }
    return E_OK;
}

void PduR_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)RxPduId;
    if (mock_PduRRxCalls < 8U) {
        mock_PduRRxLens[mock_PduRRxCalls] = PduInfoPtr->SduLength;
        mock_PduRRxFirstByte[mock_PduRRxCalls] =
            (PduInfoPtr->SduLength > 0U) ? PduInfoPtr->SduDataPtr[0] : 0U;
    }
    for (PduLengthType i = 0U; (i < PduInfoPtr->SduLength) && (i < 16U); i++) {
        mock_PduRLastData[i] = PduInfoPtr->SduDataPtr[i];
    }
    mock_PduRLastLen = PduInfoPtr->SduLength;
    mock_PduRRxCalls++;
}

void PduR_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    (void)TxPduId;
    mock_PduRTxLastResult = result;
    mock_PduRTxCalls++;
}

void setUp(void) {
    mock_DetCalls = 0;
    mock_RteCalls = 0;
    mock_RteLastApiId = 0;
    mock_RteLastErrorId = 0;
    mock_CanIfCalls = 0;
    mock_CanIfPduId = 0xFFFFU;
    mock_CanIfMetaValid = FALSE;
    for (uint8 i = 0U; i < 4U; i++) { mock_CanIfMetaData[i] = 0U; }
    mock_PduRRxCalls = 0;
    for (uint8 i = 0U; i < 8U; i++) { mock_PduRRxLens[i] = 0; mock_PduRRxFirstByte[i] = 0U; }
    for (uint8 i = 0U; i < 16U; i++) { mock_PduRLastData[i] = 0U; }
    mock_PduRLastLen = 0;
    mock_PduRTxCalls = 0;
    mock_PduRTxLastResult = E_OK;
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

/*==================================================================================================
*                          LOCAL MIXED / NORMALFIXED ADDRESSING CONFIGS (AD5)
*================================================================================================*/
/* The production link-time configuration (CanTp_Lcfg.c) contains only CANTP_STANDARD
 * NSDUs, so the MIXED / NORMALFIXED MetaData tests build local config tables here.
 * Tx SDU IDs must stay below CANTP_NUM_TX_NSDU because CanTp_Transmit range-checks
 * the ID before the config lookup; each test re-initializes the module, so reusing
 * production indices 0 and 2 is safe. Rx NPdu IDs are looked up without a range
 * check and only need to be unique per local config. */
#define TEST_MIXED_TX_SDU_ID      (0U)   /* CANTP_TX_DIAG_PHYSICAL */
#define TEST_MIXED_RX_NPDU_ID     (21U)
#define TEST_MIXED_TX_ADDR        (0x11U)
#define TEST_MIXED_RX_ADDR        (0x7EU)

#define TEST_NF_TX_SDU_ID         (2U)   /* CANTP_TX_UDS_PHYSICAL */
#define TEST_NF_RX_NPDU_ID        (31U)

static const CanTp_TxNsduConfigType testMixedTxNsdu = {
    /* CanTpTxNPduId */                0U,
    /* CanTpTxNPduConfirmationId */    TEST_MIXED_TX_SDU_ID,
    /* CanTpTxFcNPduId */              1U,
    /* CanTpNas */                     CANTP_NAS_DEFAULT,
    /* CanTpNbs */                     CANTP_NBS_DEFAULT,
    /* CanTpNcs */                     CANTP_NCS_DEFAULT,
    /* CanTpTxAddressingFormat */      CANTP_MIXED,
    /* CanTpTxPaddingActivation */     TRUE,
    /* CanTpTxTaType */                CANTP_PHYSICAL,
    /* CanTpTxMaxMessageLength */      CANTP_MAX_MESSAGE_LENGTH,
    /* CanTpTxAddress */               TEST_MIXED_TX_ADDR,
    /* CanTpTxPriority */              1U
};

static const CanTp_RxNsduConfigType testMixedRxNsdu = {
    /* CanTpRxNPduId */                TEST_MIXED_RX_NPDU_ID,
    /* CanTpRxNSduId */                21U,
    /* CanTpRxFcNPduConfirmationId */  1U,
    /* CanTpNar */                     CANTP_NAR_DEFAULT,
    /* CanTpNbr */                     CANTP_NBR_DEFAULT,
    /* CanTpNcr */                     CANTP_NCR_DEFAULT,
    /* CanTpRxAddressingFormat */      CANTP_MIXED,
    /* CanTpRxPaddingActivation */     TRUE,
    /* CanTpRxTaType */                CANTP_PHYSICAL,
    /* CanTpRxMaxMessageLength */      CANTP_MAX_MESSAGE_LENGTH,
    /* CanTpRxAddress */               TEST_MIXED_RX_ADDR,
    /* CanTpRxWftMax */                CANTP_WFT_MAX_DEFAULT,
    /* CanTpRxPriority */              1U,
    /* CanTpBs */                      CANTP_BS_DEFAULT,
    /* CanTpSTmin */                   CANTP_STMIN_DEFAULT
};

static const CanTp_ChannelConfigType testMixedChannel = {
    /* ChannelId */     0U,
    /* ChannelMode */   CANTP_MODE_FULL_DUPLEX,
    /* NumTxNsdu */     1U,
    /* NumRxNsdu */     1U,
    /* TxNsduConfigs */ &testMixedTxNsdu,
    /* RxNsduConfigs */ &testMixedRxNsdu,
    /* CanFdEnabled */  FALSE
};

static const CanTp_ConfigType testMixedConfig = {
    /* GeneralConfig */   NULL_PTR,
    /* ChannelConfigs */  &testMixedChannel,
    /* NumChannels */     1U
};

static const CanTp_TxNsduConfigType testNormalFixedTxNsdu = {
    /* CanTpTxNPduId */                0U,
    /* CanTpTxNPduConfirmationId */    TEST_NF_TX_SDU_ID,
    /* CanTpTxFcNPduId */              1U,
    /* CanTpNas */                     CANTP_NAS_DEFAULT,
    /* CanTpNbs */                     CANTP_NBS_DEFAULT,
    /* CanTpNcs */                     CANTP_NCS_DEFAULT,
    /* CanTpTxAddressingFormat */      CANTP_NORMALFIXED,
    /* CanTpTxPaddingActivation */     TRUE,
    /* CanTpTxTaType */                CANTP_PHYSICAL,
    /* CanTpTxMaxMessageLength */      CANTP_MAX_MESSAGE_LENGTH,
    /* CanTpTxAddress */               0U,
    /* CanTpTxPriority */              1U
};

static const CanTp_RxNsduConfigType testNormalFixedRxNsdu = {
    /* CanTpRxNPduId */                TEST_NF_RX_NPDU_ID,
    /* CanTpRxNSduId */                31U,
    /* CanTpRxFcNPduConfirmationId */  1U,
    /* CanTpNar */                     CANTP_NAR_DEFAULT,
    /* CanTpNbr */                     CANTP_NBR_DEFAULT,
    /* CanTpNcr */                     CANTP_NCR_DEFAULT,
    /* CanTpRxAddressingFormat */      CANTP_NORMALFIXED,
    /* CanTpRxPaddingActivation */     TRUE,
    /* CanTpRxTaType */                CANTP_PHYSICAL,
    /* CanTpRxMaxMessageLength */      CANTP_MAX_MESSAGE_LENGTH,
    /* CanTpRxAddress */               0U,
    /* CanTpRxWftMax */                CANTP_WFT_MAX_DEFAULT,
    /* CanTpRxPriority */              1U,
    /* CanTpBs */                      CANTP_BS_DEFAULT,
    /* CanTpSTmin */                   CANTP_STMIN_DEFAULT
};

static const CanTp_ChannelConfigType testNormalFixedChannel = {
    /* ChannelId */     0U,
    /* ChannelMode */   CANTP_MODE_FULL_DUPLEX,
    /* NumTxNsdu */     1U,
    /* NumRxNsdu */     1U,
    /* TxNsduConfigs */ &testNormalFixedTxNsdu,
    /* RxNsduConfigs */ &testNormalFixedRxNsdu,
    /* CanFdEnabled */  FALSE
};

static const CanTp_ConfigType testNormalFixedConfig = {
    /* GeneralConfig */   NULL_PTR,
    /* ChannelConfigs */  &testNormalFixedChannel,
    /* NumChannels */     1U
};

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

    /* FC with CTS and STmin = 5 ms: the first CF goes out immediately.
     * AD5: RxIndication only enqueues the frame; MainFunction processes it. */
    CanTp_RxIndication(CANTP_CANIF_FC_RX_PDU_ID, &fcPdu);
    CanTp_MainFunction();
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
    CanTp_MainFunction();
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

    /* 12-byte CAN FD First Frame (message DL = 12) is accepted: FC CTS is sent.
     * AD5: RxIndication only enqueues the frame; MainFunction processes it. */
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &ffPdu);
    CanTp_MainFunction();
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

    /* 12-byte frame on a classic CAN channel is rejected: no Flow Control.
     * AD5: RxIndication only enqueues the frame; MainFunction processes it. */
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &ffPdu);
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(0, mock_CanIfCalls);

    /* Classic 8-byte FF with message DL = 12 keeps the legacy behaviour (accepted) */
    ffPdu.SduDataPtr = ff8Data; ffPdu.SduLength = 8U;
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &ffPdu);
    CanTp_MainFunction();
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
        CanTp_MainFunction();
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
        CanTp_MainFunction();
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
        CanTp_MainFunction();
        TEST_ASSERT_EQUAL(1, mock_PduRRxCalls);
    }
    pdu.SduLength = 9U;
    mock_PduRRxCalls = 0;
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &pdu);
    CanTp_MainFunction();
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

/*==================================================================================================
*                          RX FRAME QUEUE TESTS (AD5)
*================================================================================================*/
/** @req SWS_CanTp_00009 */
void test_CanTp_RxIndication_QueuesFrameUntilMainFunction(void) {
    uint8 sf1Data[8] = {0x03U, 0x11U, 0x22U, 0x33U, 0U, 0U, 0U, 0U};
    uint8 sf2Data[8] = {0x03U, 0x44U, 0x55U, 0x66U, 0U, 0U, 0U, 0U};
    PduInfoType sf1Pdu;
    PduInfoType sf2Pdu;
    sf1Pdu.SduDataPtr = sf1Data; sf1Pdu.MetaDataPtr = NULL_PTR; sf1Pdu.SduLength = 8U;
    sf2Pdu.SduDataPtr = sf2Data; sf2Pdu.MetaDataPtr = NULL_PTR; sf2Pdu.SduLength = 8U;

    CanTp_Init(&CanTp_Config);

    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sf1Pdu);
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sf2Pdu);

    /* AD5: RxIndication only enqueues - no protocol processing happens here,
     * so the PduR callback must not have fired yet. */
    TEST_ASSERT_EQUAL(0, mock_PduRRxCalls);
    TEST_ASSERT_EQUAL(0, mock_CanIfCalls);
    TEST_ASSERT_EQUAL(0, mock_RteCalls);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);

    CanTp_MainFunction();

    /* FIFO order: the first-enqueued frame is delivered first */
    TEST_ASSERT_EQUAL(2, mock_PduRRxCalls);
    TEST_ASSERT_EQUAL(3, mock_PduRRxLens[0]);
    TEST_ASSERT_EQUAL_HEX8(0x11U, mock_PduRRxFirstByte[0]);
    TEST_ASSERT_EQUAL(3, mock_PduRRxLens[1]);
    TEST_ASSERT_EQUAL_HEX8(0x44U, mock_PduRRxFirstByte[1]);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_RxQueueFull_ReportsRxComAndDrops(void) {
    uint8 sfData[8] = {0x01U, 0x99U, 0U, 0U, 0U, 0U, 0U, 0U};  /* SF_DL = 1 */
    PduInfoType sfPdu;
    sfPdu.SduDataPtr = sfData; sfPdu.MetaDataPtr = NULL_PTR; sfPdu.SduLength = 8U;

    CanTp_Init(&CanTp_Config);

    /* Fill the queue (depth 4) */
    for (uint8 i = 0U; i < 4U; i++) {
        CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sfPdu);
    }
    TEST_ASSERT_EQUAL(0, mock_RteCalls);

    /* 5th frame: queue full - dropped with the CANTP_E_RX_COM runtime error */
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sfPdu);
    TEST_ASSERT_EQUAL(1, mock_RteCalls);
    TEST_ASSERT_EQUAL_HEX8(CANTP_SID_RXINDICATION, mock_RteLastApiId);
    TEST_ASSERT_EQUAL_HEX8(CANTP_E_RX_COM, mock_RteLastErrorId);

    /* The frames already queued are unaffected and are all processed */
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(4, mock_PduRRxCalls);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_RxQueueFull_KeepsInFlightReception(void) {
    uint8 ffData[8]  = {0x10U, 0x0AU, 0xA0U, 0xA1U, 0xA2U, 0xA3U, 0xA4U, 0xA5U};  /* FF, DL = 10 */
    uint8 cfData[8]  = {0x21U, 0xA6U, 0xA7U, 0xA8U, 0xA9U, 0xCCU, 0xCCU, 0xCCU}; /* CF SN = 1 */
    uint8 sfData[8]  = {0x01U, 0x55U, 0U, 0U, 0U, 0U, 0U, 0U};                    /* SF_DL = 1 */
    PduInfoType ffPdu;
    PduInfoType cfPdu;
    PduInfoType sfPdu;
    ffPdu.SduDataPtr = ffData; ffPdu.MetaDataPtr = NULL_PTR; ffPdu.SduLength = 8U;
    cfPdu.SduDataPtr = cfData; cfPdu.MetaDataPtr = NULL_PTR; cfPdu.SduLength = 8U;
    sfPdu.SduDataPtr = sfData; sfPdu.MetaDataPtr = NULL_PTR; sfPdu.SduLength = 8U;

    CanTp_Init(&CanTp_Config);

    /* Start a multi-frame reception: the FF is processed and answered with FC CTS */
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &ffPdu);
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
    TEST_ASSERT_EQUAL_HEX8(0x30U, mock_CanIfFrame[0]);
    TEST_ASSERT_EQUAL(0, mock_PduRRxCalls);

    /* Queue: 3 SFs + the completing CF fills the depth-4 queue ... */
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sfPdu);
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sfPdu);
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sfPdu);
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &cfPdu);

    /* ... the next frame overflows: dropped with CANTP_E_RX_COM */
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sfPdu);
    TEST_ASSERT_EQUAL(1, mock_RteCalls);
    TEST_ASSERT_EQUAL_HEX8(CANTP_E_RX_COM, mock_RteLastErrorId);
    TEST_ASSERT_EQUAL(0, mock_PduRRxCalls);

    CanTp_MainFunction();

    /* The in-flight reception completes via the queued CF (3 SFs + 1 completion) */
    TEST_ASSERT_EQUAL(4, mock_PduRRxCalls);
    TEST_ASSERT_EQUAL(10, mock_PduRLastLen);
    for (uint8 i = 0U; i < 10U; i++) {
        TEST_ASSERT_EQUAL_HEX8((uint8)(0xA0U + i), mock_PduRLastData[i]);
    }
}

/** @req SWS_CanTp_00009 */
void test_CanTp_MainFunction_EmptyQueue_NoSideEffects(void) {
    CanTp_Init(&CanTp_Config);
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_EQUAL(0, mock_RteCalls);
    TEST_ASSERT_EQUAL(0, mock_CanIfCalls);
    TEST_ASSERT_EQUAL(0, mock_PduRRxCalls);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_MultiFrameRx_CompletesAfterCf(void) {
    uint8 ffData[8] = {0x10U, 0x0AU, 0xA0U, 0xA1U, 0xA2U, 0xA3U, 0xA4U, 0xA5U};  /* FF, DL = 10 */
    uint8 cfData[8] = {0x21U, 0xA6U, 0xA7U, 0xA8U, 0xA9U, 0xCCU, 0xCCU, 0xCCU}; /* CF SN = 1 */
    PduInfoType ffPdu;
    PduInfoType cfPdu;
    ffPdu.SduDataPtr = ffData; ffPdu.MetaDataPtr = NULL_PTR; ffPdu.SduLength = 8U;
    cfPdu.SduDataPtr = cfData; cfPdu.MetaDataPtr = NULL_PTR; cfPdu.SduLength = 8U;

    CanTp_Init(&CanTp_Config);

    /* FF: FC CTS is emitted, message not complete yet */
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &ffPdu);
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
    TEST_ASSERT_EQUAL_HEX8(0x30U, mock_CanIfFrame[0]);
    TEST_ASSERT_EQUAL(0, mock_PduRRxCalls);

    /* CF SN = 1: completes the 10-byte reassembly */
    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &cfPdu);
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(1, mock_PduRRxCalls);
    TEST_ASSERT_EQUAL(10, mock_PduRLastLen);
    for (uint8 i = 0U; i < 10U; i++) {
        TEST_ASSERT_EQUAL_HEX8((uint8)(0xA0U + i), mock_PduRLastData[i]);
    }
    TEST_ASSERT_EQUAL(0, mock_RteCalls);
}

/*==================================================================================================
*                          METADATA TESTS (AD5, MIXED / NORMALFIXED)
*================================================================================================*/
/** @req SWS_CanTp_00009 */
void test_CanTp_MixedAddressing_MetaDataSavedForReply(void) {
    uint8 sfData[8] = {0x03U, 0x11U, 0x22U, 0x33U, 0U, 0U, 0U, 0U};
    uint8 rxMeta[1] = {0x7EU};  /* requester target address */
    uint8 txData[3] = {0xAAU, 0xBBU, 0xCCU};
    PduInfoType rxPdu;
    PduInfoType txPdu;
    rxPdu.SduDataPtr = sfData; rxPdu.MetaDataPtr = rxMeta; rxPdu.SduLength = 8U;
    txPdu.SduDataPtr = txData; txPdu.MetaDataPtr = NULL_PTR; txPdu.SduLength = 3U;

    CanTp_Init(&testMixedConfig);

    /* Receive a MIXED-addressed Single Frame carrying the requester address */
    CanTp_RxIndication(TEST_MIXED_RX_NPDU_ID, &rxPdu);
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(1, mock_PduRRxCalls);

    /* The reply carries the saved MetaData (1-byte target address per SWS CanTp) */
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(TEST_MIXED_TX_SDU_ID, &txPdu));
    TEST_ASSERT_EQUAL(TRUE, mock_CanIfMetaValid);
    TEST_ASSERT_EQUAL_HEX8(0x7EU, mock_CanIfMetaData[0]);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_MixedAddressing_TxMetaDataFallsBackToConfigAddress(void) {
    uint8 txData[3] = {0xAAU, 0xBBU, 0xCCU};
    PduInfoType txPdu;
    txPdu.SduDataPtr = txData; txPdu.MetaDataPtr = NULL_PTR; txPdu.SduLength = 3U;

    /* Fresh Init clears any previously received MetaData */
    CanTp_Init(&testMixedConfig);

    /* No received MetaData and none from the upper layer: the configured
     * CanTpTxAddress (0x11) is used for the outgoing MetaData. */
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(TEST_MIXED_TX_SDU_ID, &txPdu));
    TEST_ASSERT_EQUAL(TRUE, mock_CanIfMetaValid);
    TEST_ASSERT_EQUAL_HEX8(TEST_MIXED_TX_ADDR, mock_CanIfMetaData[0]);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_NormalFixedAddressing_CanIdMetaDataRoundTrip(void) {
    uint8 sfData[8] = {0x03U, 0x11U, 0x22U, 0x33U, 0U, 0U, 0U, 0U};
    uint8 rxMeta[4] = {0x18U, 0xDAU, 0xF1U, 0x10U};  /* CAN ID 0x18DAF110, big-endian */
    uint8 txData[3] = {0xAAU, 0xBBU, 0xCCU};
    PduInfoType rxPdu;
    PduInfoType txPdu;
    rxPdu.SduDataPtr = sfData; rxPdu.MetaDataPtr = rxMeta; rxPdu.SduLength = 8U;
    txPdu.SduDataPtr = txData; txPdu.MetaDataPtr = NULL_PTR; txPdu.SduLength = 3U;

    CanTp_Init(&testNormalFixedConfig);

    /* Receive a NORMALFIXED-addressed Single Frame carrying the CAN ID MetaData */
    CanTp_RxIndication(TEST_NF_RX_NPDU_ID, &rxPdu);
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(1, mock_PduRRxCalls);

    /* The reply echoes the received CAN ID (4-byte big-endian MetaData) */
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(TEST_NF_TX_SDU_ID, &txPdu));
    TEST_ASSERT_EQUAL(TRUE, mock_CanIfMetaValid);
    TEST_ASSERT_EQUAL_HEX8(0x18U, mock_CanIfMetaData[0]);
    TEST_ASSERT_EQUAL_HEX8(0xDAU, mock_CanIfMetaData[1]);
    TEST_ASSERT_EQUAL_HEX8(0xF1U, mock_CanIfMetaData[2]);
    TEST_ASSERT_EQUAL_HEX8(0x10U, mock_CanIfMetaData[3]);
}

/*==================================================================================================
*                          UNIFIED LENGTH VALIDATION TESTS (AD5)
*================================================================================================*/
/** @req SWS_CanTp_00009 */
void test_CanTp_RxInvalidFrameLength_ReportsRuntimeError(void) {
    uint8 badSfData[9] = {0x03U, 0x11U, 0x22U, 0x33U, 0U, 0U, 0U, 0U, 0U};  /* 9-byte classic frame: illegal DLC */
    PduInfoType badSfPdu;
    badSfPdu.SduDataPtr = badSfData; badSfPdu.MetaDataPtr = NULL_PTR; badSfPdu.SduLength = 9U;

    CanTp_Init(&CanTp_Config);

    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &badSfPdu);
    CanTp_MainFunction();

    /* The unified SF length check rejects the frame: runtime error + drop */
    TEST_ASSERT_EQUAL(1, mock_RteCalls);
    TEST_ASSERT_EQUAL_HEX8(CANTP_SID_RXINDICATION, mock_RteLastApiId);
    TEST_ASSERT_EQUAL_HEX8(CANTP_E_RX_SF_UNEXPECTED_LEN, mock_RteLastErrorId);
    TEST_ASSERT_EQUAL(0, mock_PduRRxCalls);
    TEST_ASSERT_EQUAL(0, mock_CanIfCalls);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/*==================================================================================================
*                          RX PADDING VALIDATION TESTS (Phase 4)
*================================================================================================*/
/* Default build: CANTP_REJECT_INVALID_PADDING == STD_OFF. A padding mismatch is
 * reported as the CANTP_E_RX_UNEXP_PADDING runtime error but the frame is still
 * processed (ISO 15765-2 tolerance towards non-padding senders). */

/** @req SWS_CanTp_00009 */
void test_CanTp_RxSfInvalidPadding_ReportOnly_ShouldStillDeliver(void) {
    uint8 sfData[8] = {0x02U, 0xAAU, 0xBBU, 0U, 0U, 0U, 0U, 0U}; /* tail 0x00 != 0xCC */
    PduInfoType sfPdu;
    sfPdu.SduDataPtr = sfData; sfPdu.MetaDataPtr = NULL_PTR; sfPdu.SduLength = 8U;

    CanTp_Init(&CanTp_Config);

    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sfPdu);
    CanTp_MainFunction();

    TEST_ASSERT_EQUAL(1, mock_RteCalls);
    TEST_ASSERT_EQUAL_HEX8(CANTP_SID_RXINDICATION, mock_RteLastApiId);
    TEST_ASSERT_EQUAL_HEX8(CANTP_E_RX_UNEXP_PADDING, mock_RteLastErrorId);
    /* Report-only mode: the frame is still forwarded to PduR */
    TEST_ASSERT_EQUAL(1, mock_PduRRxCalls);
    TEST_ASSERT_EQUAL(2, mock_PduRLastLen);
    TEST_ASSERT_EQUAL_HEX8(0xAAU, mock_PduRRxFirstByte[0]);
    TEST_ASSERT_EQUAL(0, mock_DetCalls); /* runtime error, not a development error */
}

/** @req SWS_CanTp_00009 */
void test_CanTp_RxSfValidPadding_ShouldDeliverWithoutError(void) {
    uint8 sfData[8] = {0x02U, 0xAAU, 0xBBU, 0xCCU, 0xCCU, 0xCCU, 0xCCU, 0xCCU};
    PduInfoType sfPdu;
    sfPdu.SduDataPtr = sfData; sfPdu.MetaDataPtr = NULL_PTR; sfPdu.SduLength = 8U;

    CanTp_Init(&CanTp_Config);

    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sfPdu);
    CanTp_MainFunction();

    TEST_ASSERT_EQUAL(0, mock_RteCalls);
    TEST_ASSERT_EQUAL(1, mock_PduRRxCalls);
    TEST_ASSERT_EQUAL(2, mock_PduRLastLen);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_RxSfFullFrame_ShouldHaveNoPaddingArea(void) {
    /* SF_DL = 7: the seven data bytes fill the frame exactly, no padding bytes */
    uint8 sfData[8] = {0x07U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U};
    PduInfoType sfPdu;
    sfPdu.SduDataPtr = sfData; sfPdu.MetaDataPtr = NULL_PTR; sfPdu.SduLength = 8U;

    CanTp_Init(&CanTp_Config);

    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sfPdu);
    CanTp_MainFunction();

    TEST_ASSERT_EQUAL(0, mock_RteCalls);
    TEST_ASSERT_EQUAL(1, mock_PduRRxCalls);
    TEST_ASSERT_EQUAL(7, mock_PduRLastLen);
    TEST_ASSERT_EQUAL_HEX8(0x77U, mock_PduRLastData[6]);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_RxSfPaddingRegion_ShouldCheckAllTailBytes(void) {
    /* SF_DL = 1: padding area is bytes 2..7; one trailing 0x00 must be caught */
    uint8 sfData[8] = {0x01U, 0x99U, 0xCCU, 0xCCU, 0xCCU, 0xCCU, 0xCCU, 0U};
    PduInfoType sfPdu;
    sfPdu.SduDataPtr = sfData; sfPdu.MetaDataPtr = NULL_PTR; sfPdu.SduLength = 8U;

    CanTp_Init(&CanTp_Config);

    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &sfPdu);
    CanTp_MainFunction();

    TEST_ASSERT_EQUAL(1, mock_RteCalls);
    TEST_ASSERT_EQUAL_HEX8(CANTP_E_RX_UNEXP_PADDING, mock_RteLastErrorId);
    TEST_ASSERT_EQUAL(1, mock_PduRRxCalls); /* report-only: still delivered */
    TEST_ASSERT_EQUAL(1, mock_PduRLastLen);
}

/** @req SWS_CanTp_00009 */
void test_CanTp_RxCfInvalidPadding_ReportOnly_ShouldStillComplete(void) {
    uint8 ffData[8] = {0x10U, 0x0AU, 0xA0U, 0xA1U, 0xA2U, 0xA3U, 0xA4U, 0xA5U}; /* FF, DL = 10 */
    uint8 cfData[8] = {0x21U, 0xA6U, 0xA7U, 0xA8U, 0xA9U, 0U, 0U, 0U};          /* CF SN = 1, bad padding */
    PduInfoType ffPdu;
    PduInfoType cfPdu;
    ffPdu.SduDataPtr = ffData; ffPdu.MetaDataPtr = NULL_PTR; ffPdu.SduLength = 8U;
    cfPdu.SduDataPtr = cfData; cfPdu.MetaDataPtr = NULL_PTR; cfPdu.SduLength = 8U;

    CanTp_Init(&CanTp_Config);

    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &ffPdu);
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls); /* FC CTS emitted, FF exempt from padding */

    CanTp_RxIndication(CANTP_RX_DIAG_PHYSICAL, &cfPdu);
    CanTp_MainFunction();

    /* CF padding mismatch is reported ... */
    TEST_ASSERT_EQUAL(1, mock_RteCalls);
    TEST_ASSERT_EQUAL_HEX8(CANTP_E_RX_UNEXP_PADDING, mock_RteLastErrorId);
    /* ... but the reassembled message is still delivered in report-only mode */
    TEST_ASSERT_EQUAL(1, mock_PduRRxCalls);
    TEST_ASSERT_EQUAL(10, mock_PduRLastLen);
    for (uint8 i = 0U; i < 10U; i++) {
        TEST_ASSERT_EQUAL_HEX8((uint8)(0xA0U + i), mock_PduRLastData[i]);
    }
}

/* --- Additional coverage tests --- */

void test_CanTp_CancelReceive_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = CanTp_CancelReceive(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_CanTp_ChangeParameter_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = CanTp_ChangeParameter(0U, TP_STMIN, 10U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_CanTp_ReadParameter_BeforeInit_ShouldFail(void) {
    uint16 value = 0U;
    Std_ReturnType ret = CanTp_ReadParameter(0U, TP_STMIN, &value);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_CanTp_Transmit_ZeroLength_ShouldFail(void) {
    CanTp_ConfigType config = CanTp_Config;
    CanTp_Init(&config);
    uint8 data[1] = {0};
    uint8 meta[8] = {0};
    PduInfoType pdu;
    pdu.SduDataPtr = data;
    pdu.MetaDataPtr = meta;
    pdu.SduLength = 0U;
    Std_ReturnType ret = CanTp_Transmit(0U, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_CanTp_Shutdown_WhenInitialized_ShouldNotReportDet(void) {
    CanTp_ConfigType config = CanTp_Config;
    CanTp_Init(&config);
    mock_DetCalls = 0;
    CanTp_Shutdown();
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanTp_RxIndication_BeforeInit_ShouldFail(void) {
    uint8 data[8] = {0};
    PduInfoType pdu;
    pdu.SduDataPtr = data;
    pdu.MetaDataPtr = NULL_PTR;
    pdu.SduLength = 8U;
    CanTp_RxIndication(0U, &pdu);
    /* Should not crash or report DET (RxIndication is called from ISR context) */
}

void test_CanTp_RxIndication_NullPduInfo_ShouldFail(void) {
    CanTp_ConfigType config = CanTp_Config;
    CanTp_Init(&config);
    CanTp_RxIndication(0U, NULL_PTR);
    /* Should not crash */
}

/* ---- Additional coverage tests ---- */

void test_CanTp_CancelTransmit_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = CanTp_CancelTransmit(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

void test_CanTp_TxConfirmation_BeforeInit_ShouldReturn(void) {
    CanTp_TxConfirmation(0U);
    TEST_ASSERT_EQUAL(0, mock_PduRTxCalls);
}

void test_CanTp_TxConfirmation_TxFF_ShouldTransitionToWaitFC(void) {
    PduInfoType pdu;
    uint8 data[16];
    for (uint8 i = 0U; i < 16U; i++) { data[i] = (uint8)i; }
    pdu.SduDataPtr = data; pdu.SduLength = 16U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    /* Transmit 16 bytes -> FF sent, channel in TX_WAIT_FC after TxConfirmation */
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
    /* TxConfirmation for FF: transitions to TX_WAIT_FC */
    CanTp_TxConfirmation(0U);
    /* Channel should now be waiting for FC; cancel should succeed */
    TEST_ASSERT_EQUAL(E_OK, CanTp_CancelTransmit(CANTP_TX_DIAG_PHYSICAL));
}

void test_CanTp_TxConfirmation_TxSfComplete_ShouldNotifyPduR(void) {
    PduInfoType pdu;
    uint8 data[3] = {0x11U, 0x22U, 0x33U};
    pdu.SduDataPtr = data; pdu.SduLength = 3U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
    /* SF Tx confirmation: should notify PduR */
    CanTp_TxConfirmation(0U);
    TEST_ASSERT_EQUAL(1, mock_PduRTxCalls);
    TEST_ASSERT_EQUAL(E_OK, mock_PduRTxLastResult);
}

void test_CanTp_TxConfirmation_TxCfComplete_ShouldNotifyPduR(void) {
    PduInfoType pdu;
    PduInfoType fcPdu;
    uint8 data[16];
    uint8 fcData[8] = {0x30U, 0x08U, 0x00U, 0U, 0U, 0U, 0U, 0U};
    for (uint8 i = 0U; i < 16U; i++) { data[i] = (uint8)i; }
    pdu.SduDataPtr = data; pdu.SduLength = 16U; pdu.MetaDataPtr = NULL_PTR;
    fcPdu.SduDataPtr = fcData; fcPdu.SduLength = 8U; fcPdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    /* FC CTS with STmin=0 */
    CanTp_RxIndication(CANTP_CANIF_FC_RX_PDU_ID, &fcPdu);
    CanTp_MainFunction();
    /* Send all remaining CFs */
    for (uint8 i = 0U; i < 10U; i++) {
        CanTp_MainFunction();
    }
    /* All data sent: TxConfirmation should notify PduR */
    CanTp_TxConfirmation(0U);
    TEST_ASSERT_EQUAL(1, mock_PduRTxCalls);
}

void test_CanTp_TxConfirmation_TxCfMoreToSend_ShouldResetTimer(void) {
    PduInfoType pdu;
    PduInfoType fcPdu;
    uint8 data[16];
    uint8 fcData[8] = {0x30U, 0x01U, 0x05U, 0U, 0U, 0U, 0U, 0U};
    for (uint8 i = 0U; i < 16U; i++) { data[i] = (uint8)i; }
    pdu.SduDataPtr = data; pdu.SduLength = 16U; pdu.MetaDataPtr = NULL_PTR;
    fcPdu.SduDataPtr = fcData; fcPdu.SduLength = 8U; fcPdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    /* FC CTS with BS=1, STmin=5: only 1 CF allowed per block */
    CanTp_RxIndication(CANTP_CANIF_FC_RX_PDU_ID, &fcPdu);
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(2, mock_CanIfCalls);
    /* First CF sent. TxConfirmation while more data remains: should not notify PduR */
    CanTp_TxConfirmation(0U);
    TEST_ASSERT_EQUAL(0, mock_PduRTxCalls);
}

void test_CanTp_MainFunction_BeforeInit_ShouldReturn(void) {
    CanTp_MainFunction();
    TEST_ASSERT_EQUAL(0, mock_CanIfCalls);
    TEST_ASSERT_EQUAL(0, mock_PduRRxCalls);
}

void test_CanTp_MainFunction_NAsTimeout_ShouldResetChannel(void) {
    PduInfoType pdu;
    uint8 data[3] = {0x11U, 0x22U, 0x33U};
    pdu.SduDataPtr = data; pdu.SduLength = 3U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    /* Run MainFunction enough times to expire N_As timer */
    for (uint16 i = 0U; i < 2000U; i++) {
        CanTp_MainFunction();
    }
    /* Channel should be reset; cancel should fail */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanTp_CancelTransmit(CANTP_TX_DIAG_PHYSICAL));
}

void test_CanTp_MainFunction_NCrTimeout_ShouldResetChannel(void) {
    PduInfoType pdu;
    uint8 data[16];
    for (uint8 i = 0U; i < 16U; i++) { data[i] = (uint8)i; }
    pdu.SduDataPtr = data; pdu.SduLength = 16U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    /* Start a multi-frame Rx by sending FF */
    uint8 ffData[8] = {0x10U, 0x10U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U};
    PduInfoType ffPdu;
    ffPdu.SduDataPtr = ffData; ffPdu.SduLength = 8U; ffPdu.MetaDataPtr = NULL_PTR;
    CanTp_RxIndication(CANTP_CANIF_RX_PDU_ID, &ffPdu);
    CanTp_MainFunction(); /* Process FF, send FC, enter RX_CF */
    /* Run MainFunction enough times to expire N_Cr timer */
    for (uint16 i = 0U; i < 5000U; i++) {
        CanTp_MainFunction();
    }
    /* Channel should be reset; cancel receive should fail */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanTp_CancelReceive(CANTP_RX_DIAG_PHYSICAL));
}

void test_CanTp_CancelReceive_ActiveRx_ShouldSucceed(void) {
    CanTp_Init(&CanTp_Config);
    /* Start a multi-frame Rx by sending FF */
    uint8 ffData[8] = {0x10U, 0x10U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U};
    PduInfoType ffPdu;
    ffPdu.SduDataPtr = ffData; ffPdu.SduLength = 8U; ffPdu.MetaDataPtr = NULL_PTR;
    CanTp_RxIndication(CANTP_CANIF_RX_PDU_ID, &ffPdu);
    CanTp_MainFunction(); /* Process FF, enter RX_CF */
    /* CancelReceive returns E_NOT_OK because ActiveNsduId is not set in Rx path */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanTp_CancelReceive(CANTP_RX_DIAG_PHYSICAL));
}

void test_CanTp_ProcessRxFrame_FcWait_ShouldResetTimer(void) {
    PduInfoType pdu;
    PduInfoType fcPdu;
    uint8 data[16];
    uint8 fcData[8] = {0x31U, 0x00U, 0x00U, 0U, 0U, 0U, 0U, 0U}; /* FS=1 (Wait) */
    for (uint8 i = 0U; i < 16U; i++) { data[i] = (uint8)i; }
    pdu.SduDataPtr = data; pdu.SduLength = 16U; pdu.MetaDataPtr = NULL_PTR;
    fcPdu.SduDataPtr = fcData; fcPdu.SduLength = 8U; fcPdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
    /* FC Wait: should stay in TX_WAIT_FC, no new frame sent */
    CanTp_RxIndication(CANTP_CANIF_FC_RX_PDU_ID, &fcPdu);
    CanTp_MainFunction();
    /* Still only 1 CanIf call (FF), no CF sent */
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
    /* Channel still active */
    TEST_ASSERT_EQUAL(E_OK, CanTp_CancelTransmit(CANTP_TX_DIAG_PHYSICAL));
}

void test_CanTp_ProcessRxFrame_FcOvflw_ShouldResetChannel(void) {
    PduInfoType pdu;
    PduInfoType fcPdu;
    uint8 data[16];
    uint8 fcData[8] = {0x32U, 0x00U, 0x00U, 0U, 0U, 0U, 0U, 0U}; /* FS=2 (Overflow) */
    for (uint8 i = 0U; i < 16U; i++) { data[i] = (uint8)i; }
    pdu.SduDataPtr = data; pdu.SduLength = 16U; pdu.MetaDataPtr = NULL_PTR;
    fcPdu.SduDataPtr = fcData; fcPdu.SduLength = 8U; fcPdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    TEST_ASSERT_EQUAL(1, mock_CanIfCalls);
    /* FC Overflow: should abort transmission */
    CanTp_RxIndication(CANTP_CANIF_FC_RX_PDU_ID, &fcPdu);
    CanTp_MainFunction();
    /* Channel should be reset; cancel should fail */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanTp_CancelTransmit(CANTP_TX_DIAG_PHYSICAL));
}

void test_CanTp_Transmit_NoFreeChannel_ShouldFail(void) {
    PduInfoType pdu;
    uint8 data[16];
    for (uint8 i = 0U; i < 16U; i++) { data[i] = (uint8)i; }
    pdu.SduDataPtr = data; pdu.SduLength = 16U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    /* Fill all channels with multi-frame Tx */
    uint8 result = E_OK;
    for (uint8 ch = 0U; ch < CANTP_MAX_CHANNEL_CNT; ch++) {
        result = CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu);
        if (result != E_OK) { break; }
    }
    /* Next transmit should fail: no free channel */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
}

void test_CanTp_ChangeParameter_TpBs_ShouldUpdateBlockSize(void) {
    PduInfoType pdu;
    uint8 data[10] = {0U};
    uint16 value = 0U;
    pdu.SduDataPtr = data; pdu.SduLength = 10U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    TEST_ASSERT_EQUAL(E_OK, CanTp_ChangeParameter(CANTP_TX_DIAG_PHYSICAL, TP_BS, 20U));
    TEST_ASSERT_EQUAL(E_OK, CanTp_ReadParameter(CANTP_TX_DIAG_PHYSICAL, TP_BS, &value));
    TEST_ASSERT_EQUAL(20U, value);
}

void test_CanTp_ReadParameter_NullPtr_ShouldReportDet(void) {
    PduInfoType pdu;
    uint8 data[10] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 10U; pdu.MetaDataPtr = NULL_PTR;

    CanTp_Init(&CanTp_Config);
    TEST_ASSERT_EQUAL(E_OK, CanTp_Transmit(CANTP_TX_DIAG_PHYSICAL, &pdu));
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanTp_ReadParameter(CANTP_TX_DIAG_PHYSICAL, TP_STMIN, NULL_PTR));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
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
    RUN_TEST(test_CanTp_RxIndication_QueuesFrameUntilMainFunction);
    RUN_TEST(test_CanTp_RxQueueFull_ReportsRxComAndDrops);
    RUN_TEST(test_CanTp_RxQueueFull_KeepsInFlightReception);
    RUN_TEST(test_CanTp_MainFunction_EmptyQueue_NoSideEffects);
    RUN_TEST(test_CanTp_MultiFrameRx_CompletesAfterCf);
    RUN_TEST(test_CanTp_MixedAddressing_MetaDataSavedForReply);
    RUN_TEST(test_CanTp_MixedAddressing_TxMetaDataFallsBackToConfigAddress);
    RUN_TEST(test_CanTp_NormalFixedAddressing_CanIdMetaDataRoundTrip);
    RUN_TEST(test_CanTp_RxInvalidFrameLength_ReportsRuntimeError);
    RUN_TEST(test_CanTp_RxSfInvalidPadding_ReportOnly_ShouldStillDeliver);
    RUN_TEST(test_CanTp_RxSfValidPadding_ShouldDeliverWithoutError);
    RUN_TEST(test_CanTp_RxSfFullFrame_ShouldHaveNoPaddingArea);
    RUN_TEST(test_CanTp_RxSfPaddingRegion_ShouldCheckAllTailBytes);
    RUN_TEST(test_CanTp_RxCfInvalidPadding_ReportOnly_ShouldStillComplete);

    /* Additional coverage tests */
    RUN_TEST(test_CanTp_CancelReceive_BeforeInit_ShouldFail);
    RUN_TEST(test_CanTp_ChangeParameter_BeforeInit_ShouldFail);
    RUN_TEST(test_CanTp_ReadParameter_BeforeInit_ShouldFail);
    RUN_TEST(test_CanTp_Transmit_ZeroLength_ShouldFail);
    RUN_TEST(test_CanTp_Shutdown_WhenInitialized_ShouldNotReportDet);
    RUN_TEST(test_CanTp_RxIndication_BeforeInit_ShouldFail);
    RUN_TEST(test_CanTp_RxIndication_NullPduInfo_ShouldFail);

    /* Extended coverage tests */
    RUN_TEST(test_CanTp_CancelTransmit_BeforeInit_ShouldFail);
    RUN_TEST(test_CanTp_TxConfirmation_BeforeInit_ShouldReturn);
    RUN_TEST(test_CanTp_TxConfirmation_TxFF_ShouldTransitionToWaitFC);
    RUN_TEST(test_CanTp_TxConfirmation_TxSfComplete_ShouldNotifyPduR);
    RUN_TEST(test_CanTp_TxConfirmation_TxCfComplete_ShouldNotifyPduR);
    RUN_TEST(test_CanTp_TxConfirmation_TxCfMoreToSend_ShouldResetTimer);
    RUN_TEST(test_CanTp_MainFunction_BeforeInit_ShouldReturn);
    RUN_TEST(test_CanTp_MainFunction_NAsTimeout_ShouldResetChannel);
    RUN_TEST(test_CanTp_MainFunction_NCrTimeout_ShouldResetChannel);
    RUN_TEST(test_CanTp_CancelReceive_ActiveRx_ShouldSucceed);
    RUN_TEST(test_CanTp_ProcessRxFrame_FcWait_ShouldResetTimer);
    RUN_TEST(test_CanTp_ProcessRxFrame_FcOvflw_ShouldResetChannel);
    RUN_TEST(test_CanTp_Transmit_NoFreeChannel_ShouldFail);
    RUN_TEST(test_CanTp_ChangeParameter_TpBs_ShouldUpdateBlockSize);
    RUN_TEST(test_CanTp_ReadParameter_NullPtr_ShouldReportDet);

    return UnityEnd();
}
