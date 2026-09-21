/**
 * @file test_udpnm.c
 * @brief UdpNm (UDP Network Management) Unit Tests
 * @req SWS_UdpNm
 *
 * Real assertions against the fully implemented UdpNm SUT
 * (src/bsw/services/udpnm/src/UdpNm.c, UDPNM_DEV_ERROR_DETECT = STD_ON).
 * Development-error reports are captured by the local Det mock below.
 */

// @tests src/bsw/services/udpnm/src/UdpNm.c  @tests src/bsw/services/udpnm/include/UdpNm.h
#include "unity.h"
#include "UdpNm.h"

/* Mock Det_ReportError — captures report arguments (UDPNM_DEV_ERROR_DETECT = STD_ON) */
static uint16 mock_DetLastModuleId = 0xFFFFU;
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastModuleId = 0xFFFFU;
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)InstanceId;
    mock_DetLastModuleId = ModuleId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test configuration: all four configured channels */
static const UdpNm_ChannelConfigType testChannelConfig[UDPNM_NUMBER_OF_CHANNELS] = {
    { UDPNM_CHANNEL_0, UDPNM_CHANNEL_0_NODE_ID, UDPNM_CHANNEL_0_CLUSTER_ID,
      FALSE, TRUE, UDPNM_NODEDETECTION_ENABLED, TRUE, FALSE, TRUE, TRUE,
      UDPNM_USER_DATA_OFFSET, UDPNM_USER_DATA_LENGTH,
      UDPNM_PDU_POS_BYTE_0, UDPNM_PDU_POS_BYTE_1,
      UDPNM_MSG_CYCLE_TIME, UDPNM_MSG_TIMEOUT_TIME, UDPNM_REPEAT_MESSAGE_TIME,
      UDPNM_WAIT_BUS_SLEEP_TIME, UDPNM_TIMEOUT_TIME,
      UDPNM_IMMEDIATE_NM_CYCLE_TIME, UDPNM_IMMEDIATE_NM_TRANSMISIONS,
      UDPNM_CHANNEL_0_TX_PDU_ID, UDPNM_CHANNEL_0_RX_PDU_ID },
    { UDPNM_CHANNEL_1, UDPNM_CHANNEL_1_NODE_ID, UDPNM_CHANNEL_1_CLUSTER_ID,
      FALSE, TRUE, UDPNM_NODEDETECTION_ENABLED, TRUE, FALSE, TRUE, TRUE,
      UDPNM_USER_DATA_OFFSET, UDPNM_USER_DATA_LENGTH,
      UDPNM_PDU_POS_BYTE_0, UDPNM_PDU_POS_BYTE_1,
      UDPNM_MSG_CYCLE_TIME, UDPNM_MSG_TIMEOUT_TIME, UDPNM_REPEAT_MESSAGE_TIME,
      UDPNM_WAIT_BUS_SLEEP_TIME, UDPNM_TIMEOUT_TIME,
      UDPNM_IMMEDIATE_NM_CYCLE_TIME, UDPNM_IMMEDIATE_NM_TRANSMISIONS,
      UDPNM_CHANNEL_1_TX_PDU_ID, UDPNM_CHANNEL_1_RX_PDU_ID },
    { UDPNM_CHANNEL_2, UDPNM_CHANNEL_2_NODE_ID, UDPNM_CHANNEL_2_CLUSTER_ID,
      FALSE, TRUE, UDPNM_NODEDETECTION_ENABLED, TRUE, FALSE, TRUE, TRUE,
      UDPNM_USER_DATA_OFFSET, UDPNM_USER_DATA_LENGTH,
      UDPNM_PDU_POS_BYTE_0, UDPNM_PDU_POS_BYTE_1,
      UDPNM_MSG_CYCLE_TIME, UDPNM_MSG_TIMEOUT_TIME, UDPNM_REPEAT_MESSAGE_TIME,
      UDPNM_WAIT_BUS_SLEEP_TIME, UDPNM_TIMEOUT_TIME,
      UDPNM_IMMEDIATE_NM_CYCLE_TIME, UDPNM_IMMEDIATE_NM_TRANSMISIONS,
      UDPNM_CHANNEL_2_TX_PDU_ID, UDPNM_CHANNEL_2_RX_PDU_ID },
    { UDPNM_CHANNEL_3, UDPNM_CHANNEL_3_NODE_ID, UDPNM_CHANNEL_3_CLUSTER_ID,
      FALSE, TRUE, UDPNM_NODEDETECTION_ENABLED, TRUE, FALSE, TRUE, TRUE,
      UDPNM_USER_DATA_OFFSET, UDPNM_USER_DATA_LENGTH,
      UDPNM_PDU_POS_BYTE_0, UDPNM_PDU_POS_BYTE_1,
      UDPNM_MSG_CYCLE_TIME, UDPNM_MSG_TIMEOUT_TIME, UDPNM_REPEAT_MESSAGE_TIME,
      UDPNM_WAIT_BUS_SLEEP_TIME, UDPNM_TIMEOUT_TIME,
      UDPNM_IMMEDIATE_NM_CYCLE_TIME, UDPNM_IMMEDIATE_NM_TRANSMISIONS,
      UDPNM_CHANNEL_3_TX_PDU_ID, UDPNM_CHANNEL_3_RX_PDU_ID },
};
static const UdpNm_ConfigType testConfig = {
    testChannelConfig,          /* ChannelConfig */
    UDPNM_NUMBER_OF_CHANNELS,   /* NumberOfChannels */
    TRUE,                       /* DevErrorDetect */
    TRUE,                       /* VersionInfoApi */
    FALSE,                      /* BusLoadReductionEnabled */
    TRUE,                       /* ComControlEnabled */
    FALSE,                      /* PnEnabled */
    UDPNM_MAIN_FUNCTION_PERIOD  /* MainFunctionPeriod */
};

void setUp(void) {
    UdpNm_DeInit();
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_UdpNm_00001 */
void test_UdpNm_Init_NullPtr_ReportsDetWithExactIds(void) {
    UdpNm_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(UDPNM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_UdpNm_00001 */
void test_UdpNm_Init_ValidConfig_DoesNotReportDet(void) {
    UdpNm_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_UdpNm_00002 */
void test_UdpNm_DeInit_BeforeInit_ReportsUninit(void) {
    mock_Det_Reset();
    UdpNm_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_UdpNm_00002 */
void test_UdpNm_DeInit_AfterInit_LeavesModuleUninitialized(void) {
    Nm_StateType state = NM_STATE_NORMAL_OPERATION;
    Nm_ModeType mode = NM_MODE_NETWORK;
    UdpNm_Init(&testConfig);
    UdpNm_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(E_NOT_OK, UdpNm_GetState(UDPNM_CHANNEL_0, &state, &mode));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_UdpNm_00003 */
void test_UdpNm_PassiveStartUp_BeforeInit_ReportsUninit(void) {
    Std_ReturnType ret = UdpNm_PassiveStartUp(UDPNM_CHANNEL_0);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_SID_PASSIVESTARTUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_UdpNm_00003 */
void test_UdpNm_PassiveStartUp_InvalidChannel_ReportsInvalidChannel(void) {
    Std_ReturnType ret;
    UdpNm_Init(&testConfig);
    ret = UdpNm_PassiveStartUp(UDPNM_NUMBER_OF_CHANNELS);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_E_INVALID_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_UdpNm_00003 */
void test_UdpNm_PassiveStartUp_AfterInit_TransitionsToRepeatMessage(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Nm_ModeType mode = NM_MODE_BUS_SLEEP;
    Std_ReturnType ret;
    UdpNm_Init(&testConfig);
    ret = UdpNm_PassiveStartUp(UDPNM_CHANNEL_0);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(E_OK, UdpNm_GetState(UDPNM_CHANNEL_0, &state, &mode));
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, state);
    TEST_ASSERT_EQUAL_UINT8(NM_MODE_NETWORK, mode);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_UdpNm_00004 */
void test_UdpNm_NetworkRequest_AfterInit_GetStateRepeatMessage(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Nm_ModeType mode = NM_MODE_BUS_SLEEP;
    Std_ReturnType ret;
    UdpNm_Init(&testConfig);
    ret = UdpNm_NetworkRequest(UDPNM_CHANNEL_0);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(E_OK, UdpNm_GetState(UDPNM_CHANNEL_0, &state, &mode));
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, state);
    TEST_ASSERT_EQUAL_UINT8(NM_MODE_NETWORK, mode);
}

/** @req SWS_UdpNm_00005 */
void test_UdpNm_NetworkRelease_AfterInit_GetStateReadySleep(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Nm_ModeType mode = NM_MODE_BUS_SLEEP;
    Std_ReturnType ret;
    UdpNm_Init(&testConfig);
    (void)UdpNm_NetworkRequest(UDPNM_CHANNEL_0);
    ret = UdpNm_NetworkRelease(UDPNM_CHANNEL_0);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(E_OK, UdpNm_GetState(UDPNM_CHANNEL_0, &state, &mode));
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_READY_SLEEP, state);
}

/** @req SWS_UdpNm_00012 */
void test_UdpNm_GetVersionInfo_ValidPtr_FillsVersion(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    UdpNm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(UDPNM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(UDPNM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_UdpNm_00012 */
void test_UdpNm_GetVersionInfo_NullPtr_IsSilentNoDet(void) {
    /* SUT quirk: UdpNm_GetVersionInfo silently ignores a NULL pointer. */
    UdpNm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_UdpNm_00017 */
void test_UdpNm_MainFunction_BeforeInit_IsSilentNoDet(void) {
    /* SUT quirk: uninitialized UdpNm_MainFunction returns without a DET report. */
    UdpNm_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_UdpNm_0000E */
void test_UdpNm_SetSleepReadyBit_AfterInit_ReturnsOk(void) {
    Std_ReturnType ret;
    UdpNm_Init(&testConfig);
    ret = UdpNm_SetSleepReadyBit(UDPNM_CHANNEL_0, TRUE);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_UdpNm_0000A */
void test_UdpNm_GetState_NullPtr_ReportsInvalidPointer(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Std_ReturnType ret;
    UdpNm_Init(&testConfig);
    ret = UdpNm_GetState(UDPNM_CHANNEL_0, NULL_PTR, &state);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_SID_GETSTATE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_E_INVALID_POINTER, mock_DetLastErrorId);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_UdpNm_Init_NullPtr_ReportsDetWithExactIds);
    RUN_TEST(test_UdpNm_Init_ValidConfig_DoesNotReportDet);
    RUN_TEST(test_UdpNm_DeInit_BeforeInit_ReportsUninit);
    RUN_TEST(test_UdpNm_DeInit_AfterInit_LeavesModuleUninitialized);
    RUN_TEST(test_UdpNm_PassiveStartUp_BeforeInit_ReportsUninit);
    RUN_TEST(test_UdpNm_PassiveStartUp_InvalidChannel_ReportsInvalidChannel);
    RUN_TEST(test_UdpNm_PassiveStartUp_AfterInit_TransitionsToRepeatMessage);
    RUN_TEST(test_UdpNm_NetworkRequest_AfterInit_GetStateRepeatMessage);
    RUN_TEST(test_UdpNm_NetworkRelease_AfterInit_GetStateReadySleep);
    RUN_TEST(test_UdpNm_GetVersionInfo_ValidPtr_FillsVersion);
    RUN_TEST(test_UdpNm_GetVersionInfo_NullPtr_IsSilentNoDet);
    RUN_TEST(test_UdpNm_MainFunction_BeforeInit_IsSilentNoDet);
    RUN_TEST(test_UdpNm_SetSleepReadyBit_AfterInit_ReturnsOk);
    RUN_TEST(test_UdpNm_GetState_NullPtr_ReportsInvalidPointer);

    return UnityEnd();
}
