/**
 * @file test_ethsm.c
 * @brief EthSM Unit Tests — substantiated against the production EthSM
 *        state machine (src/bsw/ecual/ethSm/src/EthSM.c).
 * @version 2.0.0
 * @date 2026-09-17
 */

// @tests src/bsw/ecual/ethSm/src/EthSM.c  @tests src/bsw/ecual/ethSm/include/EthSM.h

#include "unity.h"
#include "EthSM.h"
#include "EthIf.h"
#include "ComM.h"

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint32 mock_DetCallCount = 0U;

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

/* Stub EthIf controller-mode + transceiver-link APIs */
static uint32 mock_EthIf_SetMode_Count = 0U;
static uint8 mock_EthIf_SetMode_LastCtrl = 0xFFU;
static uint8 mock_EthIf_SetMode_LastMode = 0xFFU;
static Std_ReturnType mock_EthIf_SetMode_Return = E_OK;

static uint32 mock_EthIf_GetLink_Count = 0U;
static uint8 mock_EthIf_GetLink_LastTrcv = 0xFFU;
static boolean mock_EthIf_LinkUp = TRUE;

Std_ReturnType EthIf_SetControllerMode(uint8 ControllerId, EthIf_ControllerMode Mode) {
    mock_EthIf_SetMode_Count++;
    mock_EthIf_SetMode_LastCtrl = ControllerId;
    mock_EthIf_SetMode_LastMode = (uint8)Mode;
    return mock_EthIf_SetMode_Return;
}

Std_ReturnType EthIf_GetTransceiverLinkState(uint8 TrcvIdx, EthIf_LinkStateType* LinkStatePtr) {
    mock_EthIf_GetLink_Count++;
    mock_EthIf_GetLink_LastTrcv = TrcvIdx;
    *LinkStatePtr = (mock_EthIf_LinkUp == TRUE) ? ETHIF_LINK_STATE_ACTIVE : ETHIF_LINK_STATE_DOWN;
    return E_OK;
}

/* Stub ComM mode-indication callback */
static uint32 mock_ComM_Indication_Count = 0U;
static uint8 mock_ComM_Indication_LastChannel = 0xFFU;
static uint8 mock_ComM_Indication_LastMode = 0xFFU;

void ComM_BusSM_ModeIndication(ComM_ChannelHandleType Channel, ComM_ModeType Mode) {
    mock_ComM_Indication_Count++;
    mock_ComM_Indication_LastChannel = (uint8)Channel;
    mock_ComM_Indication_LastMode = (uint8)Mode;
}

void setUp(void) {
    mock_Det_Reset();
    mock_EthIf_SetMode_Count = 0U;
    mock_EthIf_SetMode_LastCtrl = 0xFFU;
    mock_EthIf_SetMode_LastMode = 0xFFU;
    mock_EthIf_SetMode_Return = E_OK;
    mock_EthIf_GetLink_Count = 0U;
    mock_EthIf_GetLink_LastTrcv = 0xFFU;
    mock_EthIf_LinkUp = TRUE;
    mock_ComM_Indication_Count = 0U;
    mock_ComM_Indication_LastChannel = 0xFFU;
    mock_ComM_Indication_LastMode = 0xFFU;
}

void tearDown(void) {
    /* Leave module uninitialized for the next test (guarded: DeInit on an
       uninitialized module would report DET) */
    if (EthSM_GetInternalState(0U) != ETHSM_STATE_UNINIT) {
        EthSM_DeInit();
    }
}

/** @req SWS_EthSM_00001 */
void test_EthSM_Init_NullPtr_ShouldBeAccepted(void) {
    /* EthSM_Init has no null-pointer check; a null config is stored as-is */
    EthSM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_NO_COM, EthSM_GetInternalState(0U));
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_NO_COM, EthSM_GetInternalState(1U));
}

/** @req SWS_EthSM_00001 */
void test_EthSM_Init_DoubleInit_ShouldReportError(void) {
    EthSM_Init(NULL_PTR);
    EthSM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_EthSM_00002 */
void test_EthSM_DeInit_Uninit_ShouldReportError(void) {
    EthSM_DeInit();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_EthSM_00002 */
void test_EthSM_DeInit_AfterInit_ShouldShutdownAllNetworks(void) {
    EthSM_Init(NULL_PTR);
    EthSM_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_UNINIT, EthSM_GetInternalState(0U));
    /* One EthIf shutdown per configured network (ETHSM_MAX_NETWORKS = 2) */
    TEST_ASSERT_EQUAL_UINT32(2U, mock_EthIf_SetMode_Count);
    TEST_ASSERT_EQUAL_UINT8(ETHIF_MODE_DOWN, mock_EthIf_SetMode_LastMode);
}

/** @req SWS_EthSM_00003 */
void test_EthSM_GetVersionInfo_NullPtr_ShouldReportError(void) {
    EthSM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthSM_00003 */
void test_EthSM_GetVersionInfo_ValidPtr_ShouldFillFields(void) {
    Std_VersionInfoType version;
    EthSM_GetVersionInfo(&version);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_VENDOR_ID, version.vendorID);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_MODULE_ID, version.moduleID);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_SW_MAJOR_VERSION, version.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_SW_MINOR_VERSION, version.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_SW_PATCH_VERSION, version.sw_patch_version);
}

/** @req SWS_EthSM_00004 */
void test_EthSM_RequestComMode_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_SID_REQUESTCOMMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_EthSM_00004 */
void test_EthSM_RequestComMode_InvalidHandle_ShouldReportError(void) {
    Std_ReturnType ret;
    EthSM_Init(NULL_PTR);
    ret = EthSM_RequestComMode(0x05U, COMM_FULL_COMMUNICATION);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_E_INVALID_NETWORK_HANDLE, mock_DetLastErrorId);
}

/** @req SWS_EthSM_00004 */
void test_EthSM_RequestComMode_InvalidMode_ShouldReportError(void) {
    Std_ReturnType ret;
    EthSM_Init(NULL_PTR);
    ret = EthSM_RequestComMode(0U, 0x55U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_E_INVALID_PARAMETER, mock_DetLastErrorId);
}

/** @req SWS_EthSM_00004 */
void test_EthSM_RequestComMode_ValidFullCom_ShouldAccept(void) {
    Std_ReturnType ret;
    EthSM_Init(NULL_PTR);
    ret = EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Request alone must not change the state yet */
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_NO_COM, EthSM_GetInternalState(0U));
}

/** @req SWS_EthSM_00005 */
void test_EthSM_GetCurrentComMode_Uninit_ShouldReportError(void) {
    ComM_ModeType mode = COMM_FULL_COMMUNICATION;
    Std_ReturnType ret = EthSM_GetCurrentComMode(0U, &mode);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_SID_GETCURRENTCOMMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_E_NOT_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_EthSM_00005 */
void test_EthSM_GetCurrentComMode_InvalidHandle_ShouldReportError(void) {
    ComM_ModeType mode = COMM_FULL_COMMUNICATION;
    Std_ReturnType ret;
    EthSM_Init(NULL_PTR);
    ret = EthSM_GetCurrentComMode(0x05U, &mode);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_E_INVALID_NETWORK_HANDLE, mock_DetLastErrorId);
}

/** @req SWS_EthSM_00005 */
void test_EthSM_GetCurrentComMode_NullPtr_ShouldReportError(void) {
    Std_ReturnType ret;
    EthSM_Init(NULL_PTR);
    ret = EthSM_GetCurrentComMode(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthSM_00005 */
void test_EthSM_GetCurrentComMode_AfterInit_ShouldReturnNoCom(void) {
    ComM_ModeType mode = COMM_FULL_COMMUNICATION;
    Std_ReturnType ret;
    EthSM_Init(NULL_PTR);
    ret = EthSM_GetCurrentComMode(0U, &mode);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(COMM_NO_COMMUNICATION, mode);
}

/** @req SWS_EthSM_00007 */
void test_EthSM_MainFunction_Uninit_ShouldSilentlyReturn(void) {
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_EthIf_SetMode_Count);
}

/** @req SWS_EthSM_00007 */
void test_EthSM_Transition_NoComToWaitTrcvLink_OnFullComRequest(void) {
    ComM_ModeType mode = COMM_NO_COMMUNICATION;
    EthSM_Init(NULL_PTR);
    (void)EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_WAIT_TRCVLINK, EthSM_GetInternalState(0U));
    /* Controller 0 activated via EthIf */
    TEST_ASSERT_EQUAL_UINT32(1U, mock_EthIf_SetMode_Count);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_EthIf_SetMode_LastCtrl);
    TEST_ASSERT_EQUAL_UINT8(ETHIF_MODE_ACTIVE, mock_EthIf_SetMode_LastMode);
    /* Intermediate state reported to ComM as SILENT */
    TEST_ASSERT_EQUAL_UINT8(COMM_SILENT_COMMUNICATION, mock_ComM_Indication_LastMode);
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSM_GetCurrentComMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(COMM_SILENT_COMMUNICATION, mode);
}

/** @req SWS_EthSM_00007 */
void test_EthSM_Transition_WaitTrcvLinkToWaitOnline_OnLinkUp(void) {
    EthSM_Init(NULL_PTR);
    (void)EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_MainFunction();
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_WAIT_ONLINE, EthSM_GetInternalState(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_EthIf_GetLink_LastTrcv);
}

/** @req SWS_EthSM_00007 */
void test_EthSM_Transition_WaitOnlineToComReady_OnTcpIpOnline(void) {
    ComM_ModeType mode = COMM_NO_COMMUNICATION;
    EthSM_Init(NULL_PTR);
    (void)EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_MainFunction();
    EthSM_MainFunction();
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_ONLINE);
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_COM_READY, EthSM_GetInternalState(0U));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSM_GetCurrentComMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(COMM_FULL_COMMUNICATION, mode);
    TEST_ASSERT_EQUAL_UINT8(COMM_FULL_COMMUNICATION, mock_ComM_Indication_LastMode);
}

/** @req SWS_EthSM_00007 */
void test_EthSM_Transition_WaitOnlineToWaitTrcvLink_OnTcpIpOffline(void) {
    EthSM_Init(NULL_PTR);
    (void)EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_MainFunction();
    EthSM_MainFunction();
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_OFFLINE);
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_WAIT_TRCVLINK, EthSM_GetInternalState(0U));
}

/** @req SWS_EthSM_00007 */
void test_EthSM_Transition_ComReadyToOnHold_OnTcpIpOnHold(void) {
    EthSM_Init(NULL_PTR);
    (void)EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_MainFunction();
    EthSM_MainFunction();
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_ONLINE);
    EthSM_MainFunction();
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_ONHOLD);
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_ONHOLD, EthSM_GetInternalState(0U));
    TEST_ASSERT_EQUAL_UINT8(COMM_SILENT_COMMUNICATION, mock_ComM_Indication_LastMode);
}

/** @req SWS_EthSM_00007 */
void test_EthSM_Transition_OnHoldToComReady_OnTcpIpOnline(void) {
    EthSM_Init(NULL_PTR);
    (void)EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_MainFunction();
    EthSM_MainFunction();
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_ONLINE);
    EthSM_MainFunction();
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_ONHOLD);
    EthSM_MainFunction();
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_ONLINE);
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_COM_READY, EthSM_GetInternalState(0U));
}

/** @req SWS_EthSM_00007 */
void test_EthSM_Transition_ComReadyToNoCom_OnNoComRequest(void) {
    ComM_ModeType mode = COMM_FULL_COMMUNICATION;
    EthSM_Init(NULL_PTR);
    (void)EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_MainFunction();
    EthSM_MainFunction();
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_ONLINE);
    EthSM_MainFunction();
    (void)EthSM_RequestComMode(0U, COMM_NO_COMMUNICATION);
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_NO_COM, EthSM_GetInternalState(0U));
    TEST_ASSERT_EQUAL_UINT8(E_OK, EthSM_GetCurrentComMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(COMM_NO_COMMUNICATION, mode);
    /* Controller shut down via EthIf */
    TEST_ASSERT_EQUAL_UINT8(ETHIF_MODE_DOWN, mock_EthIf_SetMode_LastMode);
}

/** @req SWS_EthSM_00007 */
void test_EthSM_Transition_ComReadyToWaitTrcvLink_OnLinkDown(void) {
    EthSM_Init(NULL_PTR);
    (void)EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_MainFunction();
    EthSM_MainFunction();
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_ONLINE);
    EthSM_MainFunction();
    /* Link drops while communication is ready */
    mock_EthIf_LinkUp = FALSE;
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_WAIT_TRCVLINK, EthSM_GetInternalState(0U));
}

/** @req SWS_EthSM_00007 */
void test_EthSM_Timeout_WaitTrcvLink_ShouldReturnToNoCom(void) {
    uint8 i;
    EthSM_Init(NULL_PTR);
    (void)EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_WAIT_TRCVLINK, EthSM_GetInternalState(0U));
    mock_EthIf_LinkUp = FALSE;
    /* ETHSM_TIMEOUT_WAIT_TRCVLINK (100ms) / cycle (10ms) = 10 cycles */
    for (i = 0U; i < 10U; i++) {
        EthSM_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_NO_COM, EthSM_GetInternalState(0U));
}

/** @req SWS_EthSM_00007 */
void test_EthSM_Timeout_WaitOnline_ShouldReturnToNoCom(void) {
    uint16 i;
    EthSM_Init(NULL_PTR);
    (void)EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_MainFunction();
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_WAIT_ONLINE, EthSM_GetInternalState(0U));
    /* Never get a TcpIp indication: 5000ms / 10ms = 500 cycles */
    for (i = 0U; i < 500U; i++) {
        EthSM_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_NO_COM, EthSM_GetInternalState(0U));
}

/** @req SWS_EthSM_00007 */
void test_EthSM_Retry_SetControllerModeFailure_ShouldGiveUpAfterMaxRetries(void) {
    EthSM_Init(NULL_PTR);
    mock_EthIf_SetMode_Return = E_NOT_OK;
    (void)EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_MainFunction();
    EthSM_MainFunction();
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(3U, mock_EthIf_SetMode_Count);
    /* ETHSM_MAX_RETRIES (3) reached: request cleared, still in NO_COM */
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_NO_COM, EthSM_GetInternalState(0U));
    EthSM_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(3U, mock_EthIf_SetMode_Count);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_NO_COM, EthSM_GetInternalState(0U));
}

/** @req SWS_EthSM_00006 */
void test_EthSM_TcpIpModeIndication_Uninit_ShouldSilentlyIgnore(void) {
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_ONLINE);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_EthSM_00006 */
void test_EthSM_TcpIpModeIndication_InvalidHandle_ShouldSilentlyIgnore(void) {
    EthSM_Init(NULL_PTR);
    EthSM_TcpIpModeIndication(0x05U, TCPIP_STATE_ONLINE);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_NO_COM, EthSM_GetInternalState(0U));
}

/** @req SWS_EthSM_00101 */
void test_EthSM_GetInternalState_Uninit_ShouldReturnUninit(void) {
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_UNINIT, EthSM_GetInternalState(0U));
}

/** @req SWS_EthSM_00101 */
void test_EthSM_GetInternalState_InvalidHandle_ShouldReturnUninit(void) {
    EthSM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(ETHSM_STATE_UNINIT, EthSM_GetInternalState(0x05U));
}

/* ---------------------------------------------------------------------------
 * Runner
 * ------------------------------------------------------------------------- */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_EthSM_Init_NullPtr_ShouldBeAccepted);
    RUN_TEST(test_EthSM_Init_DoubleInit_ShouldReportError);
    RUN_TEST(test_EthSM_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_EthSM_DeInit_AfterInit_ShouldShutdownAllNetworks);
    RUN_TEST(test_EthSM_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_EthSM_GetVersionInfo_ValidPtr_ShouldFillFields);
    RUN_TEST(test_EthSM_RequestComMode_Uninit_ShouldReportError);
    RUN_TEST(test_EthSM_RequestComMode_InvalidHandle_ShouldReportError);
    RUN_TEST(test_EthSM_RequestComMode_InvalidMode_ShouldReportError);
    RUN_TEST(test_EthSM_RequestComMode_ValidFullCom_ShouldAccept);
    RUN_TEST(test_EthSM_GetCurrentComMode_Uninit_ShouldReportError);
    RUN_TEST(test_EthSM_GetCurrentComMode_InvalidHandle_ShouldReportError);
    RUN_TEST(test_EthSM_GetCurrentComMode_NullPtr_ShouldReportError);
    RUN_TEST(test_EthSM_GetCurrentComMode_AfterInit_ShouldReturnNoCom);
    RUN_TEST(test_EthSM_MainFunction_Uninit_ShouldSilentlyReturn);
    RUN_TEST(test_EthSM_Transition_NoComToWaitTrcvLink_OnFullComRequest);
    RUN_TEST(test_EthSM_Transition_WaitTrcvLinkToWaitOnline_OnLinkUp);
    RUN_TEST(test_EthSM_Transition_WaitOnlineToComReady_OnTcpIpOnline);
    RUN_TEST(test_EthSM_Transition_WaitOnlineToWaitTrcvLink_OnTcpIpOffline);
    RUN_TEST(test_EthSM_Transition_ComReadyToOnHold_OnTcpIpOnHold);
    RUN_TEST(test_EthSM_Transition_OnHoldToComReady_OnTcpIpOnline);
    RUN_TEST(test_EthSM_Transition_ComReadyToNoCom_OnNoComRequest);
    RUN_TEST(test_EthSM_Transition_ComReadyToWaitTrcvLink_OnLinkDown);
    RUN_TEST(test_EthSM_Timeout_WaitTrcvLink_ShouldReturnToNoCom);
    RUN_TEST(test_EthSM_Timeout_WaitOnline_ShouldReturnToNoCom);
    RUN_TEST(test_EthSM_Retry_SetControllerModeFailure_ShouldGiveUpAfterMaxRetries);
    RUN_TEST(test_EthSM_TcpIpModeIndication_Uninit_ShouldSilentlyIgnore);
    RUN_TEST(test_EthSM_TcpIpModeIndication_InvalidHandle_ShouldSilentlyIgnore);
    RUN_TEST(test_EthSM_GetInternalState_Uninit_ShouldReturnUninit);
    RUN_TEST(test_EthSM_GetInternalState_InvalidHandle_ShouldReturnUninit);

    return UNITY_END();
}
