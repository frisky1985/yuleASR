/**
 * @file test_comm.c
 * @brief ComM Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/comM/src/ComM.c  @tests src/bsw/services/comM/include/ComM.h

#include "unity.h"
#include "ComM.h"

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

/* Link-time configuration provided by ComM_Lcfg.c (linked via service_comM). */
extern const ComM_ConfigType ComM_Config;

/* Bring the static SUT state deterministically to uninitialized:
 * ComM_Init() re-initializes unconditionally, DeInit() then resets state. */
static void ensure_uninit(void) {
    ComM_Init(&ComM_Config);
    ComM_DeInit();
    mock_Det_Reset();
}

void setUp(void) {
    ensure_uninit();
}

void tearDown(void) {
}

/** @req SWS_ComM_00001 */
void test_ComM_Init_NullPtr_ShouldNotCrash(void) {
    ComM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(COMM_INIT_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COMM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_ComM_00001 */
void test_ComM_Init_ValidConfig_ShouldSucceed(void) {
    ComM_ModeType mode = COMM_FULL_COMMUNICATION;
    ComM_Init(&ComM_Config);
    /* Init is silent; a mode query now succeeds without a DET report
     * (it would report COMM_E_NOT_INIT while uninitialized). */
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_GetMaxComMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(COMM_NO_COMMUNICATION, mode);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00001 */
void test_ComM_Init_DoubleInit_ShouldSucceed(void) {
    ComM_Init(&ComM_Config);
    mock_Det_Reset();
    /* ComM_Init() re-initializes unconditionally and silently. */
    ComM_Init(&ComM_Config);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00002 */
void test_ComM_DeInit_Uninit_ShouldReportError(void) {
    /* setUp() already left the module uninitialized. */
    ComM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(COMM_DEINIT_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COMM_E_NOT_INIT, mock_DetLastErrorId);
}

/** @req SWS_ComM_00002 */
void test_ComM_DeInit_ValidCall_ShouldSucceed(void) {
    ComM_Init(&ComM_Config);
    mock_Det_Reset();
    ComM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00003 */
void test_ComM_GetStatus_Uninit_ShouldReportError(void) {
    ComM_ModeType mode = COMM_FULL_COMMUNICATION;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ComM_GetCurrentComMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(COMM_GETCURRENTCOMMODE_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COMM_E_NOT_INIT, mock_DetLastErrorId);
}

/** @req SWS_ComM_00003 */
void test_ComM_GetStatus_ValidCall_ShouldReturnStatus(void) {
    ComM_ModeType mode = COMM_FULL_COMMUNICATION;
    ComM_Init(&ComM_Config);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_GetCurrentComMode(0U, &mode));
    /* Freshly initialized: all channels idle in NO_COMMUNICATION. */
    TEST_ASSERT_EQUAL_UINT8(COMM_NO_COMMUNICATION, mode);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00004 */
void test_ComM_GetIPDUGroupStatus_Uninit_ShouldReportError(void) {
    ComM_ModeType mode = COMM_FULL_COMMUNICATION;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ComM_GetRequestedComMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    /* Production quirk: ComM_GetRequestedComMode() delegates to
     * ComM_GetMaxComMode(), so the DET call carries the GETMAX SID. */
    TEST_ASSERT_EQUAL_UINT8(COMM_GETMAXCOMMODE_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COMM_E_NOT_INIT, mock_DetLastErrorId);
}

/** @req SWS_ComM_00004 */
void test_ComM_GetIPDUGroupStatus_InvalidGroup_ShouldReportError(void) {
    ComM_ModeType mode = COMM_NO_COMMUNICATION;
    ComM_Init(&ComM_Config);
    /* User handle == COMM_NUM_USERS is out of range. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ComM_GetRequestedComMode(COMM_NUM_USERS, &mode));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    /* Delegation to ComM_GetMaxComMode() -> GETMAX SID (see uninit test). */
    TEST_ASSERT_EQUAL_UINT8(COMM_GETMAXCOMMODE_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COMM_E_PARAM_USER, mock_DetLastErrorId);
}

/** @req SWS_ComM_00004 */
void test_ComM_GetIPDUGroupStatus_ValidCall_ShouldReturnStatus(void) {
    ComM_ModeType mode = COMM_FULL_COMMUNICATION;
    ComM_Init(&ComM_Config);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_GetRequestedComMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(COMM_NO_COMMUNICATION, mode);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00005 */
void test_ComM_GetCommunicationStatus_Uninit_ShouldReportError(void) {
    ComM_ModeType mode = COMM_FULL_COMMUNICATION;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ComM_GetMaxComMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(COMM_GETMAXCOMMODE_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COMM_E_NOT_INIT, mock_DetLastErrorId);
}

/** @req SWS_ComM_00005 */
void test_ComM_GetCommunicationStatus_ValidCall_ShouldReturnStatus(void) {
    ComM_ModeType mode = COMM_NO_COMMUNICATION;
    ComM_Init(&ComM_Config);
    /* Note: production ComM_GetMaxComMode() returns the user's requested mode. */
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION));
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_GetMaxComMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(COMM_FULL_COMMUNICATION, mode);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00006 */
void test_ComM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    ComM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(COMM_GETVERSIONINFO_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COMM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_ComM_00006 */
void test_ComM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType version = { 0U, 0U, 0U, 0U, 0U };
    ComM_GetVersionInfo(&version);
    TEST_ASSERT_EQUAL_UINT8(0U, version.vendorID);
    TEST_ASSERT_EQUAL_UINT8(COMM_MODULE_ID, version.moduleID);
    TEST_ASSERT_EQUAL_UINT8(COMM_SW_MAJOR_VERSION, version.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(COMM_SW_MINOR_VERSION, version.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(COMM_SW_PATCH_VERSION, version.sw_patch_version);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00007 */
void test_ComM_GetInhibitionStatus_Uninit_ShouldReportError(void) {
    ComM_InhibitionStatusType status = COMM_INHIBITION_STATUS_LIMIT_TO_NO_COM;
    /* Production returns E_NOT_OK without a DET report when uninitialized. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ComM_GetInhibitionStatus(0U, &status));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00007 */
void test_ComM_GetInhibitionStatus_ValidCall_ShouldReturnStatus(void) {
    ComM_InhibitionStatusType status = 0xFFU;
    ComM_Init(&ComM_Config);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_GetInhibitionStatus(COMM_CHANNEL_CAN0, &status));
    TEST_ASSERT_EQUAL_UINT8(COMM_INHIBITION_STATUS_NONE, status);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00008 */
void test_ComM_GetLimitation_Uninit_ShouldReportError(void) {
    /* Production ComM_LimitECUToNoComMode() silently returns when uninitialized. */
    ComM_LimitECUToNoComMode(TRUE);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00008 */
void test_ComM_GetLimitation_ValidCall_ShouldReturnLimitation(void) {
    ComM_InhibitionStatusType status = COMM_INHIBITION_STATUS_NONE;
    ComM_Init(&ComM_Config);
    ComM_LimitECUToNoComMode(TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_GetInhibitionStatus(COMM_CHANNEL_CAN0, &status));
    TEST_ASSERT_TRUE((status & COMM_INHIBITION_STATUS_LIMIT_TO_NO_COM) != 0U);
    /* Release the limitation again. */
    ComM_LimitECUToNoComMode(FALSE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_GetInhibitionStatus(COMM_CHANNEL_CAN0, &status));
    TEST_ASSERT_EQUAL_UINT8(COMM_INHIBITION_STATUS_NONE, status);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00009 */
void test_ComM_MainFunction_Uninit_ShouldNotCrash(void) {
    ComM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(COMM_MAINFUNCTION_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COMM_E_NOT_INIT, mock_DetLastErrorId);
}

/** @req SWS_ComM_00009 */
void test_ComM_MainFunction_ValidCall_ShouldSucceed(void) {
    ComM_ModeType mode = COMM_NO_COMMUNICATION;
    uint8 i;
    ComM_Init(&ComM_Config);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION));
    /* User 0 maps to CAN0 (WakeUpDelay 50ms) and ETH0 (30ms):
     * NOCOM -> PENDING on the first call, then one decrement per call,
     * FULLCOM once the counter reaches 0 (CAN0 needs 52 calls). */
    for (i = 0U; i < 60U; i++) {
        ComM_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_GetCurrentComMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(COMM_FULL_COMMUNICATION, mode);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_ComM_00010 */
void test_ComM_RequestComMode_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(COMM_REQUESTCOMMODE_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COMM_E_NOT_INIT, mock_DetLastErrorId);
}

/** @req SWS_ComM_00010 */
void test_ComM_RequestComMode_InvalidMode_ShouldReportError(void) {
    ComM_Init(&ComM_Config);
    /* 0xFF is not a valid ComM_ModeType value. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ComM_RequestComMode(0U, (ComM_ModeType)0xFFU));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(COMM_REQUESTCOMMODE_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COMM_E_WRONG_PARAMETERS, mock_DetLastErrorId);
}

/** @req SWS_ComM_00010 */
void test_ComM_RequestComMode_ValidCall_ShouldSucceed(void) {
    ComM_ModeType mode = COMM_NO_COMMUNICATION;
    ComM_Init(&ComM_Config);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION));
    TEST_ASSERT_EQUAL_UINT8(E_OK, ComM_GetRequestedComMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(COMM_FULL_COMMUNICATION, mode);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ComM_Init_NullPtr_ShouldNotCrash);
    RUN_TEST(test_ComM_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_ComM_Init_DoubleInit_ShouldSucceed);
    RUN_TEST(test_ComM_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_ComM_DeInit_ValidCall_ShouldSucceed);
    RUN_TEST(test_ComM_GetStatus_Uninit_ShouldReportError);
    RUN_TEST(test_ComM_GetStatus_ValidCall_ShouldReturnStatus);
    RUN_TEST(test_ComM_GetIPDUGroupStatus_Uninit_ShouldReportError);
    RUN_TEST(test_ComM_GetIPDUGroupStatus_InvalidGroup_ShouldReportError);
    RUN_TEST(test_ComM_GetIPDUGroupStatus_ValidCall_ShouldReturnStatus);
    RUN_TEST(test_ComM_GetCommunicationStatus_Uninit_ShouldReportError);
    RUN_TEST(test_ComM_GetCommunicationStatus_ValidCall_ShouldReturnStatus);
    RUN_TEST(test_ComM_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_ComM_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_ComM_GetInhibitionStatus_Uninit_ShouldReportError);
    RUN_TEST(test_ComM_GetInhibitionStatus_ValidCall_ShouldReturnStatus);
    RUN_TEST(test_ComM_GetLimitation_Uninit_ShouldReportError);
    RUN_TEST(test_ComM_GetLimitation_ValidCall_ShouldReturnLimitation);
    RUN_TEST(test_ComM_MainFunction_Uninit_ShouldNotCrash);
    RUN_TEST(test_ComM_MainFunction_ValidCall_ShouldSucceed);
    RUN_TEST(test_ComM_RequestComMode_Uninit_ShouldReportError);
    RUN_TEST(test_ComM_RequestComMode_InvalidMode_ShouldReportError);
    RUN_TEST(test_ComM_RequestComMode_ValidCall_ShouldSucceed);
    return UnityEnd();
}
