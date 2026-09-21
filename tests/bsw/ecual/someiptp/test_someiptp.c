/**
 * @file test_someiptp.c
 * @brief SomeIpTp Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/someiptp/src/SomeIpTp.c  @tests src/bsw/services/someiptp/include/SomeIpTp.h

#include "unity.h"
#include "SomeIpTp.h"
#include <string.h>

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* ---- SoAd mock (records the fragmented segments sent by the SUT) ---- */
static uint32 mock_SoAd_Transmit_Count = 0U;
static PduIdType mock_SoAd_LastPduId = 0xFFFFU;
static PduLengthType mock_SoAd_LastSduLength = 0U;
static uint8 mock_SoAd_LastFlagsByte = 0U;
static Std_ReturnType mock_SoAd_Transmit_Return = E_OK;

Std_ReturnType SoAd_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    mock_SoAd_Transmit_Count++;
    mock_SoAd_LastPduId = TxPduId;
    if (PduInfoPtr != NULL_PTR) {
        mock_SoAd_LastSduLength = PduInfoPtr->SduLength;
        mock_SoAd_LastFlagsByte = (PduInfoPtr->SduDataPtr != NULL_PTR)
                                  ? PduInfoPtr->SduDataPtr[0] : 0U;
    }
    return mock_SoAd_Transmit_Return;
}

/* ---- SomeIpXf mock (called when reassembly completes) ---- */
static uint32 mock_SomeIpXf_RxIndication_Count = 0U;
static PduLengthType mock_SomeIpXf_LastSduLength = 0U;
static const uint8* mock_SomeIpXf_LastSduData = NULL_PTR;

void SomeIpXf_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)RxPduId;
    mock_SomeIpXf_RxIndication_Count++;
    if (PduInfoPtr != NULL_PTR) {
        mock_SomeIpXf_LastSduData = PduInfoPtr->SduDataPtr;
        mock_SomeIpXf_LastSduLength = PduInfoPtr->SduLength;
    }
}

/* Test configuration: 2 channels, small segments (100 bytes) so that
 * fragmentation is exercised on the host without huge buffers. */
static const SomeIpTp_ChannelConfigType testChannelConfigs[2] =
{
    { SOMEIPTP_PDU_ID_CHANNEL_0_TX, SOMEIPTP_PDU_ID_CHANNEL_0_RX,
      SOMEIPTP_MAX_PDU_LENGTH, 100U, SOMEIPTP_TX_TIMEOUT_MS,
      SOMEIPTP_RX_TIMEOUT_MS, SOMEIPTP_MAX_RETRIES },
    { SOMEIPTP_PDU_ID_CHANNEL_1_TX, SOMEIPTP_PDU_ID_CHANNEL_1_RX,
      SOMEIPTP_MAX_PDU_LENGTH, 100U, SOMEIPTP_TX_TIMEOUT_MS,
      SOMEIPTP_RX_TIMEOUT_MS, SOMEIPTP_MAX_RETRIES }
};

static const SomeIpTp_ConfigType testConfig =
{
    testChannelConfigs,          /* ChannelConfigs */
    2U,                          /* NumChannels */
    TRUE,                        /* DevErrorDetect */
    TRUE,                        /* VersionInfoApi */
    SOMEIPTP_MAIN_FUNCTION_PERIOD_MS
};

void setUp(void) {
    mock_Det_Reset();
    mock_SoAd_Transmit_Count = 0U;
    mock_SoAd_LastPduId = 0xFFFFU;
    mock_SoAd_LastSduLength = 0U;
    mock_SoAd_LastFlagsByte = 0U;
    mock_SoAd_Transmit_Return = E_OK;
    mock_SomeIpXf_RxIndication_Count = 0U;
    mock_SomeIpXf_LastSduLength = 0U;
    mock_SomeIpXf_LastSduData = NULL_PTR;
    /* Force a clean uninitialized state for every test */
    SomeIpTp_DeInit();
}

void tearDown(void) {
    SomeIpTp_DeInit();
}


/** @req SWS_SomeIpTp_00001 */
void test_SomeIpTp_Init_NullPtr_ShouldReportError(void) {
    SomeIpTp_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_E_PARAM_POINTER, mock_DetLastErrorId);
    /* Uninitialized: Transmit must be rejected */
    uint8 data[8] = { 0 };
    PduInfoType pdu = { data, 8U, NULL_PTR };
    TEST_ASSERT_EQUAL(E_NOT_OK,
        SomeIpTp_Transmit(SOMEIPTP_PDU_ID_CHANNEL_0_TX, &pdu, NULL_PTR, NULL_PTR));
}

/** @req SWS_SomeIpTp_00001 */
void test_SomeIpTp_Init_ValidConfig_ShouldSucceed(void) {
    SomeIpTp_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Initialized: Transmit is accepted */
    uint8 data[8] = { 0 };
    PduInfoType pdu = { data, 8U, NULL_PTR };
    TEST_ASSERT_EQUAL(E_OK,
        SomeIpTp_Transmit(SOMEIPTP_PDU_ID_CHANNEL_0_TX, &pdu, NULL_PTR, NULL_PTR));
}

/** @req SWS_SomeIpTp_00001 */
void test_SomeIpTp_Init_DoubleInit_ShouldReportError(void) {
    SomeIpTp_Init(&testConfig);
    SomeIpTp_Init(&testConfig);
    /* SUT: second Init reports SOMEIPTP_E_ALREADY_INITIALIZED and returns */
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
    /* Module remains initialized and operational */
    uint8 data[8] = { 0 };
    PduInfoType pdu = { data, 8U, NULL_PTR };
    TEST_ASSERT_EQUAL(E_OK,
        SomeIpTp_Transmit(SOMEIPTP_PDU_ID_CHANNEL_1_TX, &pdu, NULL_PTR, NULL_PTR));
}

/** @req SWS_SomeIpTp_00002 */
void test_SomeIpTp_DeInit_Uninit_ShouldReportError(void) {
    SomeIpTp_DeInit();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_SomeIpTp_00002 */
void test_SomeIpTp_DeInit_ValidCall_ShouldResetState(void) {
    SomeIpTp_Init(&testConfig);
    SomeIpTp_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* After DeInit the module is uninitialized again: Transmit reports
     * SOMEIPTP_E_UNINIT via DET. */
    uint8 data[8] = { 0 };
    PduInfoType pdu = { data, 8U, NULL_PTR };
    Std_ReturnType ret = SomeIpTp_Transmit(SOMEIPTP_PDU_ID_CHANNEL_0_TX, &pdu, NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_SomeIpTp_00003 */
void test_SomeIpTp_GetVersionInfo_NullPtr_ShouldReportError(void) {
    SomeIpTp_Init(&testConfig);
    SomeIpTp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_SomeIpTp_00003 */
void test_SomeIpTp_GetVersionInfo_ValidPtr_ShouldReturnVersion(void) {
    Std_VersionInfoType vi;
    SomeIpTp_Init(&testConfig);
    SomeIpTp_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(SOMEIPTP_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT16(SOMEIPTP_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SW_PATCH_VERSION, vi.sw_patch_version);
}

/** @req SWS_SomeIpTp_00004 */
void test_SomeIpTp_Transmit_Uninit_ShouldReportError(void) {
    uint8 data[8] = { 0 };
    PduInfoType pdu = { data, 8U, NULL_PTR };
    Std_ReturnType ret = SomeIpTp_Transmit(SOMEIPTP_PDU_ID_CHANNEL_0_TX, &pdu, NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_SomeIpTp_00004 */
void test_SomeIpTp_Transmit_NullPduInfoPtr_ShouldReportError(void) {
    SomeIpTp_Init(&testConfig);
    Std_ReturnType ret = SomeIpTp_Transmit(SOMEIPTP_PDU_ID_CHANNEL_0_TX, NULL_PTR, NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_SomeIpTp_00004 */
void test_SomeIpTp_Transmit_InvalidPduId_ShouldReturnNotOk(void) {
    SomeIpTp_Init(&testConfig);
    uint8 data[8] = { 0 };
    PduInfoType pdu = { data, 8U, NULL_PTR };
    Std_ReturnType ret = SomeIpTp_Transmit(0xFFFFU, &pdu, NULL_PTR, NULL_PTR);
    /* Unknown PDU ID: no channel found; SUT returns E_NOT_OK without DET */
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpTp_00004 */
void test_SomeIpTp_Transmit_ValidCall_ShouldSendFirstFragment(void) {
    SomeIpTp_Init(&testConfig);
    uint8 testData[200];
    memset(testData, 0xAB, sizeof(testData));
    PduInfoType pdu = { testData, 200U, NULL_PTR };

    Std_ReturnType ret = SomeIpTp_Transmit(SOMEIPTP_PDU_ID_CHANNEL_0_TX, &pdu, NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* First segment (100 bytes payload + 4 byte TP header) sent via SoAd */
    TEST_ASSERT_EQUAL_UINT32(1U, mock_SoAd_Transmit_Count);
    TEST_ASSERT_EQUAL(SOMEIPTP_PDU_ID_CHANNEL_0_TX, mock_SoAd_LastPduId);
    TEST_ASSERT_EQUAL_UINT32(100U + 4U, mock_SoAd_LastSduLength);
    /* MoreSegments flag (0x40) must be set in the first TP header word */
    TEST_ASSERT_EQUAL_UINT8(0x40U, mock_SoAd_LastFlagsByte & 0xC0U);
}

/** @req SWS_SomeIpTp_00004 */
void test_SomeIpTp_Transmit_BusyChannel_ShouldReturnNotOk(void) {
    SomeIpTp_Init(&testConfig);
    uint8 testData[200];
    PduInfoType pdu = { testData, 200U, NULL_PTR };
    TEST_ASSERT_EQUAL(E_OK,
        SomeIpTp_Transmit(SOMEIPTP_PDU_ID_CHANNEL_0_TX, &pdu, NULL_PTR, NULL_PTR));
    /* Channel is now waiting for confirmation: second Transmit is refused */
    Std_ReturnType ret = SomeIpTp_Transmit(SOMEIPTP_PDU_ID_CHANNEL_0_TX, &pdu, NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpTp_00005 */
void test_SomeIpTp_RxIndication_CompleteSingleSegment_ShouldForwardToUpperLayer(void) {
    SomeIpTp_Init(&testConfig);
    /* Single segment, no more segments: 4 byte TP header + 50 byte payload */
    uint8 segment[54];
    segment[0] = 0x00U; segment[1] = 0x00U; segment[2] = 0x00U; segment[3] = 0x00U;
    memset(&segment[4], 0xCC, 50);
    PduInfoType pdu = { segment, 54U, NULL_PTR };

    SomeIpTp_RxIndication(SOMEIPTP_PDU_ID_CHANNEL_0_RX, &pdu);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Reassembly finished immediately: upper layer got the full message */
    TEST_ASSERT_EQUAL_UINT32(1U, mock_SomeIpXf_RxIndication_Count);
    TEST_ASSERT_EQUAL_UINT32(50U, mock_SomeIpXf_LastSduLength);
    TEST_ASSERT_NOT_NULL(mock_SomeIpXf_LastSduData);
    TEST_ASSERT_EQUAL_UINT8(0xCCU, mock_SomeIpXf_LastSduData[0]);
    TEST_ASSERT_EQUAL_UINT8(0xCCU, mock_SomeIpXf_LastSduData[49]);
}

/** @req SWS_SomeIpTp_00005 */
void test_SomeIpTp_RxIndication_OutOfSequence_ShouldReportReassemblyError(void) {
    SomeIpTp_Init(&testConfig);
    /* Offset != NextOffset(0): out-of-sequence segment resets the channel */
    uint8 segment[8];
    segment[0] = 0x00U; segment[1] = 0x00U; segment[2] = 0x01U; segment[3] = 0x00U; /* offset 256 */
    memset(&segment[4], 0xEE, 4);
    PduInfoType pdu = { segment, 8U, NULL_PTR };

    SomeIpTp_RxIndication(SOMEIPTP_PDU_ID_CHANNEL_0_RX, &pdu);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SID_RXINDICATION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_E_REASSEMBLY_ERROR, mock_DetLastErrorId);
    /* Nothing was forwarded to the upper layer */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_SomeIpXf_RxIndication_Count);
}

/** @req SWS_SomeIpTp_00007 */
void test_SomeIpTp_MainFunction_Timeout_ShouldReportAndReset(void) {
    SomeIpTp_Init(&testConfig);
    uint8 testData[200];
    PduInfoType pdu = { testData, 200U, NULL_PTR };
    TEST_ASSERT_EQUAL(E_OK,
        SomeIpTp_Transmit(SOMEIPTP_PDU_ID_CHANNEL_0_TX, &pdu, NULL_PTR, NULL_PTR));

    /* Drain the TX timeout: TxTimeout(1000ms) / MainFunctionPeriod(10ms)
     * = 100 ticks; one tick is already consumed by Transmit itself. */
    uint32 i;
    for (i = 0U; i < 100U; i++) {
        SomeIpTp_MainFunction();
    }

    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_SID_MAINFUNCTION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SOMEIPTP_E_TIMEOUT, mock_DetLastErrorId);
    /* Channel was reset: a new transmission is accepted immediately */
    TEST_ASSERT_EQUAL(E_OK,
        SomeIpTp_Transmit(SOMEIPTP_PDU_ID_CHANNEL_0_TX, &pdu, NULL_PTR, NULL_PTR));
}

/** @req SWS_SomeIpTp_00007 */
void test_SomeIpTp_MainFunction_Uninit_ShouldBeSafeNoOp(void) {
    /* SUT: MainFunction on uninitialized module processes timeout counters
     * of idle channels only: no DET report, no crash. */
    SomeIpTp_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}
