/**
 * @file test_lntm.c
 * @brief LinTp unit tests — converged to canonical ecual/linTp
 * @version 2.0.0
 * @date 2026-10-09
 *
 * After P0-NEW-1 duplicate-module convergence, this test targets the
 * canonical ecual/linTp implementation (services/lntm is shimmed).
 */

/* @tests src/bsw/ecual/linTp/src/LinTp.c  @tests src/bsw/ecual/linTp/include/LinTp.h */

#include "unity.h"
#include "LinTp.h"
#include "PduR_LinTp.h"

/* ---------------------------------------------------------------------------
 * Inline Det_ReportError mock.
 * The production LinTp.c reports development errors via Det_ReportError when
 * LINTP_DEV_ERROR_DETECT == STD_ON. This file provides its own mock, so
 * mock_det.c must NOT be linked into this target.
 * ------------------------------------------------------------------------- */
static uint16 mock_DetLastModuleId  = 0xFFFFU;
static uint8  mock_DetLastApiId     = 0xFFU;
static uint8  mock_DetLastErrorId   = 0xFFU;
static uint8  mock_DetCallCount     = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastModuleId  = 0xFFFFU;
    mock_DetLastApiId     = 0xFFU;
    mock_DetLastErrorId   = 0xFFU;
    mock_DetCallCount     = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)InstanceId;
    mock_DetLastModuleId = ModuleId;
    mock_DetLastApiId    = ApiId;
    mock_DetLastErrorId  = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}

/* ---------------------------------------------------------------------------
 * Inline PduR_LinTp mocks.
 * The canonical ecual/linTp calls into PduR for buffer management and
 * confirmation/indication. These stubs satisfy the linker.
 * ------------------------------------------------------------------------- */
void PduR_LinTpRxIndication(PduIdType RxPduId, Std_ReturnType result) {
    (void)RxPduId;
    (void)result;
}

void PduR_LinTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    (void)TxPduId;
    (void)result;
}

BufReq_ReturnType PduR_LinTpStartOfReception(PduIdType RxPduId,
                                              const PduInfoType* PduInfoPtr,
                                              PduLengthType TpSduLength,
                                              PduLengthType* BufferSizePtr) {
    (void)RxPduId;
    (void)PduInfoPtr;
    (void)TpSduLength;
    if (BufferSizePtr != NULL_PTR) {
        *BufferSizePtr = 256U;
    }
    return BUFREQ_E_OK;
}

BufReq_ReturnType PduR_LinTpCopyRxData(PduIdType RxPduId,
                                        const PduInfoType* PduInfoPtr,
                                        PduLengthType* BufferSizePtr) {
    (void)RxPduId;
    (void)PduInfoPtr;
    if (BufferSizePtr != NULL_PTR) {
        *BufferSizePtr = 256U;
    }
    return BUFREQ_E_OK;
}

BufReq_ReturnType PduR_LinTpCopyTxData(PduIdType TxPduId,
                                        const PduInfoType* PduInfoPtr,
                                        const RetryInfoType* RetryInfoPtr,
                                        PduLengthType* AvailableDataPtr) {
    (void)TxPduId;
    (void)PduInfoPtr;
    (void)RetryInfoPtr;
    if (AvailableDataPtr != NULL_PTR) {
        *AvailableDataPtr = 256U;
    }
    return BUFREQ_E_OK;
}

/* ---------------------------------------------------------------------------
 * Test configuration matching ecual/linTp LinTp_ConfigType layout.
 * ------------------------------------------------------------------------- */
static const LinTp_ChannelConfigType testChannelConfig[LINTP_MAX_CHANNEL_COUNT] = {
    {
        .ChannelId = 0U,
        .LinIfChannelId = 0U,
        .N_As = 1000U,
        .N_Cs = 1000U,
        .N_Cr = 1000U,
        .DefaultBs = 8U,
        .DefaultStMin = 20U,
        .TransmitCancellation = TRUE
    },
    {
        .ChannelId = 1U,
        .LinIfChannelId = 1U,
        .N_As = 1000U,
        .N_Cs = 1000U,
        .N_Cr = 1000U,
        .DefaultBs = 8U,
        .DefaultStMin = 20U,
        .TransmitCancellation = TRUE
    }
};

static const LinTp_NsduConfigType testNsduConfig[LINTP_NSDU_COUNT] = {
    { .NsduId = 0U, .ChannelId = 0U, .PduRNSduId = 0U, .Address = 0x01U, .IsTx = TRUE  },
    { .NsduId = 1U, .ChannelId = 0U, .PduRNSduId = 1U, .Address = 0x01U, .IsTx = FALSE },
    { .NsduId = 2U, .ChannelId = 1U, .PduRNSduId = 2U, .Address = 0x02U, .IsTx = TRUE  },
    { .NsduId = 3U, .ChannelId = 1U, .PduRNSduId = 3U, .Address = 0x02U, .IsTx = FALSE }
};

static const LinTp_ConfigType testConfig = {
    .ChannelConfig = testChannelConfig,
    .NsduConfig    = testNsduConfig,
    .ChannelCount  = LINTP_MAX_CHANNEL_COUNT,
    .NsduCount     = LINTP_NSDU_COUNT
};

static boolean moduleInitialized = FALSE;

static void test_LinTp_InitSut(void) {
    LinTp_Init(&testConfig);
    moduleInitialized = TRUE;
}

void setUp(void) {
    if (moduleInitialized) {
        LinTp_DeInit();
        moduleInitialized = FALSE;
    }
    mock_Det_Reset();
}

void tearDown(void) {
}

/* ---------------------------------------------------------------------------
 * LinTp_Init
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00001 */
void test_LinTp_Init_NullPtr_ReportsDetWithExactIds(void) {
    LinTp_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(LINTP_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00001 */
void test_LinTp_Init_ValidConfig_DoesNotReportDet(void) {
    test_LinTp_InitSut();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * LinTp_DeInit
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00002 */
void test_LinTp_DeInit_BeforeInit_ReportsUninit(void) {
    LinTp_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(LINTP_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00002 */
void test_LinTp_DeInit_AfterInit_LeavesModuleUninitialized(void) {
    uint8 txData[3] = { 0x22U, 0xF1U, 0x90U };
    PduInfoType pduInfo = { txData, NULL_PTR, 3U };

    test_LinTp_InitSut();
    LinTp_DeInit();
    moduleInitialized = FALSE;

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_Transmit(0U, &pduInfo));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_UNINIT, mock_DetLastErrorId);
}

/* ---------------------------------------------------------------------------
 * LinTp_GetVersionInfo
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00007 */
void test_LinTp_GetVersionInfo_ValidPtr_FillsVersion(void) {
    Std_VersionInfoType version = { 0U, 0U, 0U, 0U, 0U };

    LinTp_GetVersionInfo(&version);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(LINTP_VENDOR_ID, version.vendorID);
    TEST_ASSERT_EQUAL_UINT16(LINTP_MODULE_ID, version.moduleID);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SW_MAJOR_VERSION, version.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SW_MINOR_VERSION, version.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SW_PATCH_VERSION, version.sw_patch_version);
}

/** @req SWS_LinTp_00007 */
void test_LinTp_GetVersionInfo_NullPtr_ReportsDet(void) {
    LinTp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(LINTP_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_GET_VERSION_INFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_PARAM_POINTER, mock_DetLastErrorId);
}

/* ---------------------------------------------------------------------------
 * LinTp_Transmit
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00003 */
void test_LinTp_Transmit_BeforeInit_ReportsUninit(void) {
    uint8 txData[1] = { 0x3EU };
    PduInfoType pduInfo = { txData, NULL_PTR, 1U };

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_Transmit(0U, &pduInfo));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00003 */
void test_LinTp_Transmit_NullPduInfo_ReportsInvalidPointer(void) {
    test_LinTp_InitSut();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_Transmit(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00003 */
void test_LinTp_Transmit_ValidTxNsdu_ReturnsOk(void) {
    uint8 txData[3] = { 0x22U, 0xF1U, 0x90U };
    PduInfoType pduInfo = { txData, NULL_PTR, 3U };

    test_LinTp_InitSut();
    /* N-SDU 0 is a Tx connection on channel 0, channel is idle */
    TEST_ASSERT_EQUAL_UINT8(E_OK, LinTp_Transmit(0U, &pduInfo));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * LinTp_CancelReceive
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00005 */
void test_LinTp_CancelReceive_BeforeInit_ReportsUninit(void) {
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_CancelReceive(0U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_CANCEL_RECEIVE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00005 */
void test_LinTp_CancelReceive_AfterInit_NoActiveReception_ReturnsNotOk(void) {
    test_LinTp_InitSut();
    /* No active reception on channel 0 — returns E_NOT_OK silently */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_CancelReceive(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * LinTp_CancelTransmit
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00004 */
void test_LinTp_CancelTransmit_BeforeInit_ReportsUninit(void) {
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_CancelTransmit(0U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_CANCEL_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00004 */
void test_LinTp_CancelTransmit_AfterInit_NoActiveTransmission_ReturnsNotOk(void) {
    test_LinTp_InitSut();
    /* No active transmission on channel 0 — returns E_NOT_OK silently */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_CancelTransmit(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * LinTp_ChangeParameter
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00006 */
void test_LinTp_ChangeParameter_BeforeInit_ReportsUninit(void) {
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_ChangeParameter(0U, LINTP_PARAMETER_STMIN, 5U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_CHANGE_PARAMETER, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00006 */
void test_LinTp_ChangeParameter_Stmin_AfterInit_ReturnsOk(void) {
    test_LinTp_InitSut();
    /* Channel 0 is idle, valid PduId, valid parameter */
    TEST_ASSERT_EQUAL_UINT8(E_OK, LinTp_ChangeParameter(0U, LINTP_PARAMETER_STMIN, 5U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LinTp_00006 */
void test_LinTp_ChangeParameter_InvalidParameter_ReportsDet(void) {
    test_LinTp_InitSut();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_ChangeParameter(0U, (LinTp_ParameterType)0xEEU, 5U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_CHANGE_PARAMETER, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_INVALID_PARAMETER, mock_DetLastErrorId);
}

/* ---------------------------------------------------------------------------
 * LinTp_MainFunction
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00008 */
void test_LinTp_MainFunction_BeforeInit_IsSilentNoDet(void) {
    /* Uninitialized guard is a silent early return (no DET) */
    LinTp_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LinTp_00008 */
void test_LinTp_MainFunction_AfterInit_RunsSilently(void) {
    test_LinTp_InitSut();
    LinTp_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * LinTp_TxConfirmation
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00011 */
void test_LinTp_TxConfirmation_AfterInit_RunsWithoutDet(void) {
    test_LinTp_InitSut();
    LinTp_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void) {
    UNITY_BEGIN();

    /* LinTp_Init / LinTp_DeInit */
    RUN_TEST(test_LinTp_Init_NullPtr_ReportsDetWithExactIds);
    RUN_TEST(test_LinTp_Init_ValidConfig_DoesNotReportDet);
    RUN_TEST(test_LinTp_DeInit_BeforeInit_ReportsUninit);
    RUN_TEST(test_LinTp_DeInit_AfterInit_LeavesModuleUninitialized);

    /* LinTp_GetVersionInfo */
    RUN_TEST(test_LinTp_GetVersionInfo_ValidPtr_FillsVersion);
    RUN_TEST(test_LinTp_GetVersionInfo_NullPtr_ReportsDet);

    /* LinTp_Transmit */
    RUN_TEST(test_LinTp_Transmit_BeforeInit_ReportsUninit);
    RUN_TEST(test_LinTp_Transmit_NullPduInfo_ReportsInvalidPointer);
    RUN_TEST(test_LinTp_Transmit_ValidTxNsdu_ReturnsOk);

    /* LinTp_CancelReceive / LinTp_CancelTransmit */
    RUN_TEST(test_LinTp_CancelReceive_BeforeInit_ReportsUninit);
    RUN_TEST(test_LinTp_CancelReceive_AfterInit_NoActiveReception_ReturnsNotOk);
    RUN_TEST(test_LinTp_CancelTransmit_BeforeInit_ReportsUninit);
    RUN_TEST(test_LinTp_CancelTransmit_AfterInit_NoActiveTransmission_ReturnsNotOk);

    /* LinTp_ChangeParameter */
    RUN_TEST(test_LinTp_ChangeParameter_BeforeInit_ReportsUninit);
    RUN_TEST(test_LinTp_ChangeParameter_Stmin_AfterInit_ReturnsOk);
    RUN_TEST(test_LinTp_ChangeParameter_InvalidParameter_ReportsDet);

    /* LinTp_MainFunction / callbacks */
    RUN_TEST(test_LinTp_MainFunction_BeforeInit_IsSilentNoDet);
    RUN_TEST(test_LinTp_MainFunction_AfterInit_RunsSilently);
    RUN_TEST(test_LinTp_TxConfirmation_AfterInit_RunsWithoutDet);

    return UnityEnd();
}
