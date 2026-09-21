/**
 * @file test_lntm.c
 * @brief LinTp (lntm service directory) unit tests — T-021 Batch 2
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/lntm/src/LinTp.c  @tests src/bsw/services/lntm/include/LinTp.h

#include "unity.h"
#include "LinTp.h"

/* ---------------------------------------------------------------------------
 * Inline Det_ReportError mock.
 * The production LinTp.c reports development errors via Det_ReportError when
 * LINTP_DEV_ERROR_DETECT == STD_ON (see config/input/services/LinTp_Cfg.h).
 * This file intentionally provides its own mock, so mock_det.c must NOT be
 * linked into this target (double-definition guard).
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

/* ---------------------------------------------------------------------------
 * Test configuration.
 * Layout follows LinTp_ConfigType / LinTp_ChannelConfigType /
 * LinTp_ConnectionConfigType in the production header LinTp.h.
 * ------------------------------------------------------------------------- */
static const LinTp_ConnectionConfigType testConnections[LINTP_NUMBER_OF_CONNECTIONS] = {
    { LINTP_CONNECTION_0, LINTP_NAD_DIAGNOSTIC, LINTP_DEFAULT_N_AS_MS, LINTP_DEFAULT_N_CR_MS, LINTP_DEFAULT_STMIN_MS },
    { LINTP_CONNECTION_1, LINTP_NAD_FUNCTIONAL, LINTP_DEFAULT_N_AS_MS, LINTP_DEFAULT_N_CR_MS, LINTP_DEFAULT_STMIN_MS },
};

static const LinTp_ChannelConfigType testChannelConfig[LINTP_NUMBER_OF_CHANNELS] = {
    { LINTP_CHANNEL_0, testConnections, LINTP_NUMBER_OF_CONNECTIONS,
      LINTP_DEFAULT_N_AS_MS, LINTP_DEFAULT_N_CR_MS, LINTP_DEFAULT_STMIN_MS,
      LINTP_MAX_MESSAGE_LENGTH },
};

static const LinTp_ConfigType testConfig = {
    testChannelConfig,
    LINTP_NUMBER_OF_CHANNELS,
    TRUE,   /* DevErrorDetect */
    TRUE    /* VersionInfoApi */
};

/* Tracks whether a previous test left the SUT initialized, so setUp() can
 * safely return the module to the uninitialized state. Calling LinTp_DeInit()
 * while uninitialized is itself a DET error, so it must only be called when
 * the module is known to be initialized. */
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
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_INVALID_POINTER, mock_DetLastErrorId);
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
void test_LinTp_DeInit_BeforeInit_ReportsNotInitialized(void) {
    LinTp_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(LINTP_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00002 */
void test_LinTp_DeInit_AfterInit_LeavesModuleUninitialized(void) {
    uint8 txData[3] = { 0x22U, 0xF1U, 0x90U };
    PduInfoType pduInfo = { txData, NULL_PTR, 3U };

    test_LinTp_InitSut();
    LinTp_DeInit();
    moduleInitialized = FALSE;

    /* After DeInit the module must behave as uninitialized again. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_Transmit(0x0200U, &pduInfo));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/* ---------------------------------------------------------------------------
 * LinTp_GetVersionInfo
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00003 */
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

/** @req SWS_LinTp_00003 */
void test_LinTp_GetVersionInfo_NullPtr_ReportsDet(void) {
    LinTp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(LINTP_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_GET_VERSION_INFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_INVALID_POINTER, mock_DetLastErrorId);
}

/* ---------------------------------------------------------------------------
 * LinTp_Transmit
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00005 */
void test_LinTp_Transmit_BeforeInit_ReportsUninit(void) {
    uint8 txData[1] = { 0x3EU };
    PduInfoType pduInfo = { txData, NULL_PTR, 1U };

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_Transmit(0x0200U, &pduInfo));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00005 */
void test_LinTp_Transmit_NullPduInfo_ReportsInvalidPointer(void) {
    test_LinTp_InitSut();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_Transmit(0x0200U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00005 */
void test_LinTp_Transmit_AfterInit_NoMatchingConnection_ReturnsNotOkSilently(void) {
    uint8 txData[3] = { 0x22U, 0xF1U, 0x90U };
    PduInfoType pduInfo = { txData, NULL_PTR, 3U };

    test_LinTp_InitSut();
    /* SUT quirk: after Init every connection keeps TxPduId == LINTP_INVALID_PDU
     * (0xFFFF), so no user-supplied PDU ID ever matches and Transmit always
     * returns E_NOT_OK without reporting a DET error. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_Transmit(0x0200U, &pduInfo));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * LinTp_CancelReceive
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00006 */
void test_LinTp_CancelReceive_BeforeInit_ReportsNotInitialized(void) {
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_CancelReceive(0x0200U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_CANCEL_RECEIVE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00006 */
void test_LinTp_CancelReceive_AfterInit_ReturnsOk(void) {
    test_LinTp_InitSut();
    TEST_ASSERT_EQUAL_UINT8(E_OK, LinTp_CancelReceive(0x0200U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * LinTp_CancelTransmit
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00007 */
void test_LinTp_CancelTransmit_BeforeInit_ReportsNotInitialized(void) {
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_CancelTransmit(0x0200U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_CANCEL_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00007 */
void test_LinTp_CancelTransmit_AfterInit_ReturnsOk(void) {
    test_LinTp_InitSut();
    TEST_ASSERT_EQUAL_UINT8(E_OK, LinTp_CancelTransmit(0x0200U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * LinTp_ChangeParameter
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00008 */
void test_LinTp_ChangeParameter_BeforeInit_ReportsNotInitialized(void) {
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_ChangeParameter(0x0200U, TP_STMIN, 5U));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(LINTP_SID_CHANGE_PARAMETER, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(LINTP_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_LinTp_00008 */
void test_LinTp_ChangeParameter_TpStmin_AfterInit_ReturnsOk(void) {
    test_LinTp_InitSut();
    TEST_ASSERT_EQUAL_UINT8(E_OK, LinTp_ChangeParameter(0x0200U, TP_STMIN, 5U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LinTp_00008 */
void test_LinTp_ChangeParameter_UnknownParameter_ReturnsNotOk(void) {
    test_LinTp_InitSut();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_ChangeParameter(0x0200U, (TPParameterType)0xEEU, 5U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * LinTp_ResetToDefaultParameters
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00009 */
void test_LinTp_ResetToDefaultParameters_BeforeInit_IsSilentNotOk(void) {
    /* SUT quirk: the uninitialized guard in ResetToDefaultParameters lives in a
     * LINTM_*-guarded block that compiles out (LINTM_DEV_ERROR_DETECT is never
     * defined), so the uninitialized call returns E_NOT_OK silently. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LinTp_ResetToDefaultParameters(0x0200U, TP_STMIN));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LinTp_00009 */
void test_LinTp_ResetToDefaultParameters_AfterInit_ReturnsOk(void) {
    test_LinTp_InitSut();
    TEST_ASSERT_EQUAL_UINT8(E_OK, LinTp_ResetToDefaultParameters(0x0200U, TP_STMIN));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * LinTp_MainFunction / RxIndication / TxConfirmation
 * ------------------------------------------------------------------------- */

/** @req SWS_LinTp_00004 */
void test_LinTp_MainFunction_BeforeInit_IsSilentNoDet(void) {
    /* The uninitialized guard in MainFunction is in a LINTM_*-guarded block
     * that compiles out, so an uninitialized call is a silent no-op. */
    LinTp_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LinTp_00004 */
void test_LinTp_MainFunction_AfterInit_RunsSilently(void) {
    test_LinTp_InitSut();
    LinTp_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LinTp_00010 */
void test_LinTp_RxIndication_BeforeInit_IsSilentNoDet(void) {
    uint8 rxFrame[LINTP_FRAME_SIZE] = { 0x02U, 0x22U, 0xF1U, 0x90U, 0xFFU, 0xFFU, 0xFFU, 0xFFU };
    PduInfoType pduInfo = { rxFrame, NULL_PTR, LINTP_FRAME_SIZE };

    LinTp_RxIndication(LINTP_PDU_RX_DIAGNOSTIC, &pduInfo);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LinTp_00011 */
void test_LinTp_TxConfirmation_UnknownPduId_IsIgnored(void) {
    test_LinTp_InitSut();
    /* No connection carries TxPduId 0x0200 (all stay LINTP_INVALID_PDU), so
     * the confirmation must be ignored without state change or DET error. */
    LinTp_TxConfirmation(0x0200U, E_OK);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void) {
    UNITY_BEGIN();

    /* LinTp_Init / LinTp_DeInit */
    RUN_TEST(test_LinTp_Init_NullPtr_ReportsDetWithExactIds);
    RUN_TEST(test_LinTp_Init_ValidConfig_DoesNotReportDet);
    RUN_TEST(test_LinTp_DeInit_BeforeInit_ReportsNotInitialized);
    RUN_TEST(test_LinTp_DeInit_AfterInit_LeavesModuleUninitialized);

    /* LinTp_GetVersionInfo */
    RUN_TEST(test_LinTp_GetVersionInfo_ValidPtr_FillsVersion);
    RUN_TEST(test_LinTp_GetVersionInfo_NullPtr_ReportsDet);

    /* LinTp_Transmit */
    RUN_TEST(test_LinTp_Transmit_BeforeInit_ReportsUninit);
    RUN_TEST(test_LinTp_Transmit_NullPduInfo_ReportsInvalidPointer);
    RUN_TEST(test_LinTp_Transmit_AfterInit_NoMatchingConnection_ReturnsNotOkSilently);

    /* LinTp_CancelReceive / LinTp_CancelTransmit */
    RUN_TEST(test_LinTp_CancelReceive_BeforeInit_ReportsNotInitialized);
    RUN_TEST(test_LinTp_CancelReceive_AfterInit_ReturnsOk);
    RUN_TEST(test_LinTp_CancelTransmit_BeforeInit_ReportsNotInitialized);
    RUN_TEST(test_LinTp_CancelTransmit_AfterInit_ReturnsOk);

    /* LinTp_ChangeParameter / LinTp_ResetToDefaultParameters */
    RUN_TEST(test_LinTp_ChangeParameter_BeforeInit_ReportsNotInitialized);
    RUN_TEST(test_LinTp_ChangeParameter_TpStmin_AfterInit_ReturnsOk);
    RUN_TEST(test_LinTp_ChangeParameter_UnknownParameter_ReturnsNotOk);
    RUN_TEST(test_LinTp_ResetToDefaultParameters_BeforeInit_IsSilentNotOk);
    RUN_TEST(test_LinTp_ResetToDefaultParameters_AfterInit_ReturnsOk);

    /* LinTp_MainFunction / callbacks */
    RUN_TEST(test_LinTp_MainFunction_BeforeInit_IsSilentNoDet);
    RUN_TEST(test_LinTp_MainFunction_AfterInit_RunsSilently);
    RUN_TEST(test_LinTp_RxIndication_BeforeInit_IsSilentNoDet);
    RUN_TEST(test_LinTp_TxConfirmation_UnknownPduId_IsIgnored);

    return UnityEnd();
}
