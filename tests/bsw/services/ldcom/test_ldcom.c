/**
 * @file test_ldcom.c
 * @brief LdCom (Large Data Communication) Unit Tests — substantiated
 * @version 1.0.0
 * @date 2026-08-25
 *
 * NOTE: The production SUT (src/bsw/services/ldcom) does not provide
 * LdCom_GetVersionInfo/LdCom_Receive/LdCom_GetStatus. Those legacy test
 * names are remapped onto the real APIs as follows:
 *   Receive   -> LdCom_RxIndication
 *   GetStatus -> LdCom_GetSegmentStatus
 *   GetVersionInfo -> (no such API in the SUT) remapped to LdCom_CancelTransmit
 * The legacy config field NumChannels does not exist; LdCom_ConfigType
 * carries pduId/maxSegmentSize/interSegmentInterval/direction.
 *
 * Documented SUT stub behaviour (verified against LdCom.c, must not change):
 *   - LdCom_Init on an already-initialized module returns E_NOT_OK SILENTLY
 *     (no DET report — the already-init branch has no Det call).
 *   - LdCom_DeInit / LdCom_MainFunction never report to DET.
 *   - LdCom_CancelTransmit / LdCom_RxIndication / LdCom_GetSegmentStatus /
 *     LdCom_GetProgress perform no init/NULL checks and always return E_OK.
 *   - LdCom_TriggerTransmit always returns E_NOT_OK.
 */

// @tests src/bsw/services/ldcom/src/LdCom.c  @tests src/bsw/services/ldcom/include/LdCom.h

#include "unity.h"
#include "LdCom.h"
#include "Det.h"
#include <string.h>

/* Mock Det_ReportError — records ApiId/ErrorId/call count */
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

/* Test config — uses the real LdCom_ConfigType fields */
static LdCom_ConfigType testConfig;
static void test_LdCom_SetupDefaultConfig(void) {
    memset(&testConfig, 0, sizeof(testConfig));
    testConfig.pduId = 0U;
    testConfig.maxSegmentSize = 64U;
    testConfig.interSegmentInterval = 10U;
    testConfig.direction = LDCOM_DIR_TX;
}

static void test_LdCom_DoInit(void) {
    test_LdCom_SetupDefaultConfig();
    (void)LdCom_Init(&testConfig);
}

static uint8 testSduBuffer[8];
static PduInfoType testPduInfo = { testSduBuffer, NULL_PTR, 8U };

/* Force the module back to the uninitialized state regardless of the
 * previous test outcome, then clear the DET log (LdCom_DeInit is silent
 * even on an uninitialized module). */
void setUp(void) {
    LdCom_DeInit();
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_LdCom_00001 */
void test_LdCom_Init_NullConfig_ShouldReportError(void) {
    Std_ReturnType ret = LdCom_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetLastApiId);           /* ApiId 0 = Init */
    TEST_ASSERT_EQUAL_UINT8(DET_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LdCom_00001 */
void test_LdCom_Init_ValidConfig_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_LdCom_SetupDefaultConfig();
    ret = LdCom_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Initialization is observable: Transmit is accepted afterwards */
    TEST_ASSERT_EQUAL_UINT8(E_OK, LdCom_Transmit(0U, &testPduInfo));
}

/** @req SWS_LdCom_00001 — double init returns E_NOT_OK silently (no DET) */
void test_LdCom_Init_DoubleInit_ShouldReturnNotOkSilently(void) {
    Std_ReturnType ret;
    test_LdCom_DoInit();
    ret = LdCom_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* First initialization still stands: Transmit is accepted */
    TEST_ASSERT_EQUAL_UINT8(E_OK, LdCom_Transmit(0U, &testPduInfo));
}

/** @req SWS_LdCom_00002 — DeInit never reports to DET, even when uninitialized */
void test_LdCom_DeInit_Uninit_ShouldBeSilent(void) {
    LdCom_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* The module is (still) uninitialized: Transmit reports E_UNINIT */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LdCom_Transmit(0U, &testPduInfo));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetLastApiId);           /* ApiId 1 = Transmit */
    TEST_ASSERT_EQUAL_UINT8(DET_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LdCom_00002 */
void test_LdCom_DeInit_ValidCall_ShouldDeInitialize(void) {
    test_LdCom_DoInit();
    LdCom_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* After DeInit the module is uninitialized: Transmit is rejected */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, LdCom_Transmit(0U, &testPduInfo));
    TEST_ASSERT_EQUAL_UINT8(DET_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LdCom_00003 */
void test_LdCom_MainFunction_Uninit_ShouldBeSilent(void) {
    LdCom_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LdCom_00003 */
void test_LdCom_MainFunction_ValidCall_ShouldSucceed(void) {
    test_LdCom_DoInit();
    LdCom_MainFunction();
    LdCom_MainFunction();
    /* MainFunction is a no-op stub; the module stays fully operative */
    TEST_ASSERT_EQUAL_UINT8(E_OK, LdCom_Transmit(0U, &testPduInfo));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LdCom_00004 */
void test_LdCom_Transmit_Uninit_ShouldReportError(void) {
    Std_ReturnType ret;
    /* Not initialized */
    ret = LdCom_Transmit(0U, &testPduInfo);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetLastApiId);           /* ApiId 1 = Transmit */
    TEST_ASSERT_EQUAL_UINT8(DET_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LdCom_00004 */
void test_LdCom_Transmit_NullPduInfo_ShouldReportError(void) {
    Std_ReturnType ret;
    test_LdCom_DoInit();
    ret = LdCom_Transmit(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LdCom_00004 */
void test_LdCom_Transmit_ValidCall_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_LdCom_DoInit();
    ret = LdCom_Transmit(0U, &testPduInfo);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LdCom_00005 — CancelTransmit has no init check: E_OK even uninitialized */
void test_LdCom_CancelTransmit_ShouldReturnOk(void) {
    Std_ReturnType ret;
    /* Not initialized — documented stub behaviour */
    ret = LdCom_CancelTransmit(0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LdCom_00006 — RxIndication has no init check: E_OK even uninitialized */
void test_LdCom_RxIndication_Uninit_ShouldReturnOk(void) {
    Std_ReturnType ret;
    /* Not initialized — documented stub behaviour */
    ret = LdCom_RxIndication(0U, &testPduInfo);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LdCom_00006 — RxIndication has no NULL check either */
void test_LdCom_RxIndication_NullPduInfo_ShouldReturnOk(void) {
    Std_ReturnType ret;
    test_LdCom_DoInit();
    ret = LdCom_RxIndication(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LdCom_00007 */
void test_LdCom_GetSegmentStatus_ShouldReturnIdle(void) {
    Std_ReturnType ret;
    LdCom_SegmentStatusType status = LDCOM_SEG_IN_PROGRESS;
    test_LdCom_DoInit();
    ret = LdCom_GetSegmentStatus(0U, &status);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8((uint8)LDCOM_SEG_IDLE, (uint8)status);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LdCom_00007 — NULL status is tolerated (no write, no DET) */
void test_LdCom_GetSegmentStatus_NullStatus_ShouldReturnOk(void) {
    Std_ReturnType ret;
    test_LdCom_DoInit();
    ret = LdCom_GetSegmentStatus(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LdCom_00008 */
void test_LdCom_GetProgress_ShouldReturnZeros(void) {
    Std_ReturnType ret;
    uint16 bytesSent = 7U;
    uint16 totalBytes = 9U;
    test_LdCom_DoInit();
    ret = LdCom_GetProgress(0U, &bytesSent, &totalBytes);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT16(0U, bytesSent);
    TEST_ASSERT_EQUAL_UINT16(0U, totalBytes);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_LdCom_00009 — TriggerTransmit is unsupported: always E_NOT_OK */
void test_LdCom_TriggerTransmit_ShouldReturnNotOk(void) {
    Std_ReturnType ret;
    test_LdCom_DoInit();
    ret = LdCom_TriggerTransmit(0U, &testPduInfo);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_LdCom_Init_NullConfig_ShouldReportError);
    RUN_TEST(test_LdCom_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_LdCom_Init_DoubleInit_ShouldReturnNotOkSilently);
    RUN_TEST(test_LdCom_DeInit_Uninit_ShouldBeSilent);
    RUN_TEST(test_LdCom_DeInit_ValidCall_ShouldDeInitialize);
    RUN_TEST(test_LdCom_MainFunction_Uninit_ShouldBeSilent);
    RUN_TEST(test_LdCom_MainFunction_ValidCall_ShouldSucceed);
    RUN_TEST(test_LdCom_Transmit_Uninit_ShouldReportError);
    RUN_TEST(test_LdCom_Transmit_NullPduInfo_ShouldReportError);
    RUN_TEST(test_LdCom_Transmit_ValidCall_ShouldSucceed);
    RUN_TEST(test_LdCom_CancelTransmit_ShouldReturnOk);
    RUN_TEST(test_LdCom_RxIndication_Uninit_ShouldReturnOk);
    RUN_TEST(test_LdCom_RxIndication_NullPduInfo_ShouldReturnOk);
    RUN_TEST(test_LdCom_GetSegmentStatus_ShouldReturnIdle);
    RUN_TEST(test_LdCom_GetSegmentStatus_NullStatus_ShouldReturnOk);
    RUN_TEST(test_LdCom_GetProgress_ShouldReturnZeros);
    RUN_TEST(test_LdCom_TriggerTransmit_ShouldReturnNotOk);
    return UnityEnd();
}
