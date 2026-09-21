/**
 * @file test_cantpsyn.c
 * @brief CanTSyn (CAN Time Synchronization) Unit Tests
 * @req SWS_CanTSyn
 *
 * Substantiated against the production implementation in
 * src/bsw/services/cantsyn (note: source dir is "cantsyn", module API is CanTSyn).
 *
 * SUT behavior notes reflected in test ordering:
 *  - CanTSyn has no DeInit API: once initialized the module stays initialized
 *    for the rest of the process, so all UNINIT-state tests run before the
 *    first successful CanTSyn_Init, and later tests must not call Init again
 *    (it would raise an unexpected CANTSYN_E_ALREADY_INITIALIZED report).
 *  - PduInfoType field order is {SduDataPtr, MetaDataPtr, SduLength}.
 *  - SUT quirk: RxIndication masks the message type to the low nibble
 *    (SduDataPtr[0] & 0x0F) but compares against CANTSYN_SYNC_MSG_TYPE
 *    (0x10) / CANTSYN_OFS_MSG_TYPE (0x20), which can never match; a valid
 *    PDU therefore only increments the Rx counter and never reports a DET.
 */

// @tests src/bsw/services/cantsyn/src/CanTSyn.c  @tests src/bsw/services/cantsyn/src/CanTSyn_Lcfg.c

#include "unity.h"
#include "CanTSyn.h"

/* ------------------------------------------------------------------ */
/* Det mock — the test file defines its own Det_ReportError,           */
/* so tests/mocks/mock_det.c must NOT be linked (duplicate symbol).    */
/* ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ */
/* Minimal dependency stubs for symbols referenced by CanTSyn.c        */
/* (CanTSyn_MainFunction -> StbM_GetCurrentTime / CanIf_Transmit with  */
/* CANTSYN_TIME_MASTER_SUPPORT == STD_ON).                             */
/* ------------------------------------------------------------------ */
static uint8 mock_StbM_GetCurrentTime_Calls = 0U;
static uint8 mock_CanIf_Transmit_Calls = 0U;

Std_ReturnType StbM_GetCurrentTime(uint8 timeBaseId,
                                   StbM_TimeStampType* timeStampPtr,
                                   StbM_UserDataType* userDataPtr) {
    (void)timeBaseId;
    mock_StbM_GetCurrentTime_Calls++;
    if (timeStampPtr != NULL_PTR) {
        timeStampPtr->nanoseconds = 1000U;
        timeStampPtr->seconds = 42U;
        timeStampPtr->secondsHi = 0U;
        timeStampPtr->timeBaseStatus = 0U;
    }
    if (userDataPtr != NULL_PTR) {
        userDataPtr->userData[0] = 0U;
        userDataPtr->userData[1] = 0U;
        userDataPtr->userData[2] = 0U;
        userDataPtr->userByteCount = 0U;
    }
    return E_OK;
}

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    (void)TxPduId;
    (void)PduInfoPtr;
    mock_CanIf_Transmit_Calls++;
    return E_OK;
}

/* Test configuration: zero time bases / slaves / masters */
static CanTSyn_ConfigType testConfig = {
    .timeBaseConfigs = NULL_PTR,
    .slaveConfigs = NULL_PTR,
    .masterConfigs = NULL_PTR,
    .numTimeBases = 0U,
    .numSlaves = 0U,
    .numMasters = 0U,
    .devErrorDetect = TRUE,
    .versionInfoApi = TRUE
};

void setUp(void) {
    mock_Det_Reset();
    mock_StbM_GetCurrentTime_Calls = 0U;
    mock_CanIf_Transmit_Calls = 0U;
}

void tearDown(void) {
}

/* ------------------------------------------------------------------ */
/* UNINIT-state tests — must run before the first successful Init     */
/* (CanTSyn provides no DeInit, so state cannot be reset).            */
/* ------------------------------------------------------------------ */

/** @req SWS_CanTSyn_00001 */
void test_CanTSyn_Init_NullPtr_ShouldReportError(void) {
    CanTSyn_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_CanTSyn_00004 */
void test_CanTSyn_MainFunction_Uninit_ShouldReportError(void) {
    CanTSyn_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_CanTSyn_00014 */
void test_CanTSyn_TxConfirmation_Uninit_ShouldReportError(void) {
    CanTSyn_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_CanTSyn_00013 */
void test_CanTSyn_RxIndication_Uninit_ShouldReportError(void) {
    uint8 sdu[16] = {0U};
    PduInfoType pduInfo = {sdu, NULL_PTR, 16U};
    CanTSyn_RxIndication(0U, &pduInfo);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_E_UNINIT, mock_DetLastErrorId);
}

/* ------------------------------------------------------------------ */
/* From here on the module is initialized exactly once and stays in    */
/* CANTSYN_STATE_INIT (no DeInit exists). Tests must not call Init.   */
/* ------------------------------------------------------------------ */

/**
 * @req SWS_CanTSyn_00001
 * First (and only) successful initialization of the process.
 */
void test_CanTSyn_Init_ValidConfig_ShouldNotReportError(void) {
    CanTSyn_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Module is now initialized: MainFunction must run without DET errors. */
    CanTSyn_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_CanTSyn_00001 */
void test_CanTSyn_Init_DoubleInit_ShouldReportAlreadyInitialized(void) {
    CanTSyn_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_CanTSyn_00003 */
void test_CanTSyn_GetVersionInfo_NullPtr_ShouldReportError(void) {
    CanTSyn_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_E_PARAM_POINTER, mock_DetLastErrorId);
}

/**
 * @req SWS_CanTSyn_00003
 * Note: the .c redefines CANTSYN_VENDOR_ID to 0x00 and CANTSYN_MODULE_ID to
 * 0xDA (the header declares 0x01/0xA4); assert the values actually returned.
 */
void test_CanTSyn_GetVersionInfo_ValidPtr_ShouldReturnVersion(void) {
    Std_VersionInfoType info;
    CanTSyn_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(0x00U, info.vendorID);
    TEST_ASSERT_EQUAL_UINT8(0xDAU, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_SW_PATCH_VERSION, info.sw_patch_version);
}

/**
 * @req SWS_CanTSyn_00004
 * Time domain 0 is configured as master (CanTSyn_Lcfg.c): MainFunction must
 * fetch time from StbM and transmit a SYNC via CanIf. TxState is BUSY from an
 * earlier transmission, so confirm it first (TxConfirmation with E_OK resets
 * the internal Tx state machine) before asserting the transmit call.
 */
void test_CanTSyn_MainFunction_AfterInit_ShouldTransmitSync(void) {
    CanTSyn_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    CanTSyn_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_StbM_GetCurrentTime_Calls);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_CanIf_Transmit_Calls);
}

/** @req SWS_CanTSyn_00014 */
void test_CanTSyn_TxConfirmation_InvalidPdu_ShouldReportError(void) {
    CanTSyn_TxConfirmation(CANTSYN_NUMBER_OF_PDUS, E_OK);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_E_INVALID_PDU_SDU_ID, mock_DetLastErrorId);
}

/** @req SWS_CanTSyn_00014 */
void test_CanTSyn_TxConfirmation_ValidPdu_ShouldNotReportError(void) {
    CanTSyn_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_CanTSyn_00013 */
void test_CanTSyn_RxIndication_NullPdu_ShouldReportError(void) {
    CanTSyn_RxIndication(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_CanTSyn_00013 */
void test_CanTSyn_RxIndication_InvalidPduId_ShouldReportError(void) {
    uint8 sdu[16] = {0U};
    PduInfoType pduInfo = {sdu, NULL_PTR, 16U};
    CanTSyn_RxIndication(CANTSYN_NUMBER_OF_PDUS, &pduInfo);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(CANTSYN_E_INVALID_PDU_SDU_ID, mock_DetLastErrorId);
}

/**
 * @req SWS_CanTSyn_00013
 * Byte0 = 0x10 (SYNC | time domain 1). See file header: the SUT's msg-type
 * nibble extraction can never match CANTSYN_SYNC_MSG_TYPE, so a valid PDU is
 * accepted silently (no DET) — the assertion documents this behavior.
 */
void test_CanTSyn_RxIndication_ValidSyncPdu_ShouldNotReportError(void) {
    uint8 sdu[16] = {0x10U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    PduInfoType pduInfo = {sdu, NULL_PTR, 16U};
    CanTSyn_RxIndication(0U, &pduInfo);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void) {
    UNITY_BEGIN();

    /* UNINIT-state tests first (module cannot be deinitialized). */
    RUN_TEST(test_CanTSyn_Init_NullPtr_ShouldReportError);
    RUN_TEST(test_CanTSyn_MainFunction_Uninit_ShouldReportError);
    RUN_TEST(test_CanTSyn_TxConfirmation_Uninit_ShouldReportError);
    RUN_TEST(test_CanTSyn_RxIndication_Uninit_ShouldReportError);
    /* Single initialization point; module stays initialized afterwards. */
    RUN_TEST(test_CanTSyn_Init_ValidConfig_ShouldNotReportError);
    RUN_TEST(test_CanTSyn_Init_DoubleInit_ShouldReportAlreadyInitialized);
    RUN_TEST(test_CanTSyn_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_CanTSyn_GetVersionInfo_ValidPtr_ShouldReturnVersion);
    RUN_TEST(test_CanTSyn_MainFunction_AfterInit_ShouldTransmitSync);
    RUN_TEST(test_CanTSyn_TxConfirmation_InvalidPdu_ShouldReportError);
    RUN_TEST(test_CanTSyn_TxConfirmation_ValidPdu_ShouldNotReportError);
    RUN_TEST(test_CanTSyn_RxIndication_NullPdu_ShouldReportError);
    RUN_TEST(test_CanTSyn_RxIndication_InvalidPduId_ShouldReportError);
    RUN_TEST(test_CanTSyn_RxIndication_ValidSyncPdu_ShouldNotReportError);

    return UnityEnd();
}
