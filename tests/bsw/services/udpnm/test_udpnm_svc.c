/**
 * @file test_udpnm_svc.c
 * @brief UdpNm service/callback-path Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 *
 * Real assertions against the UdpNm SUT callback and service paths:
 * UdpNm_RxIndication / UdpNm_TxConfirmation / remote-sleep callbacks /
 * communication control / UdpNm_Transmit guards.
 */

// @tests src/bsw/services/udpnm/src/UdpNm.c  @tests src/bsw/services/udpnm/include/UdpNm.h

#include "unity.h"
#include "UdpNm.h"

/* Mock Det_ReportError — captures report arguments (UDPNM_DEV_ERROR_DETECT = STD_ON) */
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

/* Number of MainFunction calls needed to expire the 128 ms repeat-message
 * timer at the configured 10 ms main function period. */
#define TEST_UDPNM_RMT_TICKS ((UDPNM_REPEAT_MESSAGE_TIME / UDPNM_MAIN_FUNCTION_PERIOD) + 1U)

void setUp(void) {
    UdpNm_DeInit();
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_UdpNm_00101 SWS_UdpNm_00010 */
void test_UdpNm_RxIndication_ThenGetPduData_ReturnsReceivedPdu(void) {
    uint8 rxData[UDPNM_PDU_LENGTH] = {0x01U, 0x02U, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU};
    uint8 pdu[UDPNM_PDU_LENGTH] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    PduInfoType pduInfo;
    UdpNm_Init(&testConfig);
    pduInfo.SduDataPtr = rxData;
    pduInfo.MetaDataPtr = NULL_PTR;
    pduInfo.SduLength = UDPNM_PDU_LENGTH;
    UdpNm_RxIndication(UDPNM_CHANNEL_0_RX_PDU_ID, &pduInfo);
    TEST_ASSERT_EQUAL(E_OK, UdpNm_GetPduData(UDPNM_CHANNEL_0, pdu));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(rxData, pdu, UDPNM_PDU_LENGTH);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_UdpNm_00101 SWS_UdpNm_00008 */
void test_UdpNm_RxIndication_ThenGetUserData_ReturnsUserDataPortion(void) {
    uint8 rxData[UDPNM_PDU_LENGTH] = {0x01U, 0x02U, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU};
    uint8 userData[UDPNM_USER_DATA_LENGTH] = {0U, 0U, 0U, 0U, 0U, 0U};
    PduInfoType pduInfo;
    UdpNm_Init(&testConfig);
    pduInfo.SduDataPtr = rxData;
    pduInfo.MetaDataPtr = NULL_PTR;
    pduInfo.SduLength = UDPNM_PDU_LENGTH;
    UdpNm_RxIndication(UDPNM_CHANNEL_0_RX_PDU_ID, &pduInfo);
    TEST_ASSERT_EQUAL(E_OK, UdpNm_GetUserData(UDPNM_CHANNEL_0, userData));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&rxData[UDPNM_USER_DATA_OFFSET], userData, UDPNM_USER_DATA_LENGTH);
}

/** @req SWS_UdpNm_00101 */
void test_UdpNm_RxIndication_SleepingChannel_MainFunctionWakesToRepeatMessage(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Nm_ModeType mode = NM_MODE_BUS_SLEEP;
    uint8 rxData[UDPNM_PDU_LENGTH] = {0x09U, 0x00U, 1U, 2U, 3U, 4U, 5U, 6U};
    PduInfoType pduInfo;
    UdpNm_Init(&testConfig);
    pduInfo.SduDataPtr = rxData;
    pduInfo.MetaDataPtr = NULL_PTR;
    pduInfo.SduLength = UDPNM_PDU_LENGTH;
    UdpNm_RxIndication(UDPNM_CHANNEL_0_RX_PDU_ID, &pduInfo);
    UdpNm_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, UdpNm_GetState(UDPNM_CHANNEL_0, &state, &mode));
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, state);
    TEST_ASSERT_EQUAL_UINT8(NM_MODE_NETWORK, mode);
}

/** @req SWS_UdpNm_00102 SWS_UdpNm_00103 SWS_UdpNm_0000D */
void test_UdpNm_RemoteSleepIndication_ReflectedInCheckApi(void) {
    boolean remoteSleep = FALSE;
    UdpNm_Init(&testConfig);
    UdpNm_RemoteSleepIndication(UDPNM_CHANNEL_0);
    TEST_ASSERT_EQUAL(E_OK, UdpNm_CheckRemoteSleepIndication(UDPNM_CHANNEL_0, &remoteSleep));
    TEST_ASSERT_TRUE(remoteSleep);
    UdpNm_RemoteSleepCancellation(UDPNM_CHANNEL_0);
    TEST_ASSERT_EQUAL(E_OK, UdpNm_CheckRemoteSleepIndication(UDPNM_CHANNEL_0, &remoteSleep));
    TEST_ASSERT_FALSE(remoteSleep);
}

/** @req SWS_UdpNm_00006 SWS_UdpNm_00017 */
void test_UdpNm_DisableCommunication_StallsRepeatMessageTimeout(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Nm_ModeType mode = NM_MODE_BUS_SLEEP;
    uint8 ticks;
    UdpNm_Init(&testConfig);
    (void)UdpNm_NetworkRequest(UDPNM_CHANNEL_0);
    TEST_ASSERT_EQUAL(E_OK, UdpNm_DisableCommunication(UDPNM_CHANNEL_0));
    for (ticks = 0U; ticks < (uint8)TEST_UDPNM_RMT_TICKS; ticks++) {
        UdpNm_MainFunction();
    }
    TEST_ASSERT_EQUAL(E_OK, UdpNm_GetState(UDPNM_CHANNEL_0, &state, &mode));
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_REPEAT_MESSAGE, state);
}

/** @req SWS_UdpNm_00007 SWS_UdpNm_00017 */
void test_UdpNm_EnableCommunication_ResumesStateMachine(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Nm_ModeType mode = NM_MODE_BUS_SLEEP;
    uint8 ticks;
    UdpNm_Init(&testConfig);
    (void)UdpNm_NetworkRequest(UDPNM_CHANNEL_0);
    (void)UdpNm_DisableCommunication(UDPNM_CHANNEL_0);
    for (ticks = 0U; ticks < (uint8)TEST_UDPNM_RMT_TICKS; ticks++) {
        UdpNm_MainFunction();
    }
    TEST_ASSERT_EQUAL(E_OK, UdpNm_EnableCommunication(UDPNM_CHANNEL_0));
    UdpNm_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, UdpNm_GetState(UDPNM_CHANNEL_0, &state, &mode));
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_NORMAL_OPERATION, state);
}

/** @req SWS_UdpNm_00017 SWS_UdpNm_00004 */
void test_UdpNm_RepeatMessageTimeout_YieldsNormalOperationWhenRequested(void) {
    Nm_StateType state = NM_STATE_UNINIT;
    Nm_ModeType mode = NM_MODE_BUS_SLEEP;
    uint8 ticks;
    UdpNm_Init(&testConfig);
    (void)UdpNm_NetworkRequest(UDPNM_CHANNEL_0);
    for (ticks = 0U; ticks < (uint8)TEST_UDPNM_RMT_TICKS; ticks++) {
        UdpNm_MainFunction();
    }
    TEST_ASSERT_EQUAL(E_OK, UdpNm_GetState(UDPNM_CHANNEL_0, &state, &mode));
    TEST_ASSERT_EQUAL_UINT8(NM_STATE_NORMAL_OPERATION, state);
}

/** @req SWS_UdpNm_00011 */
void test_UdpNm_Transmit_BeforeInit_ReportsUninit(void) {
    uint8 txData[UDPNM_PDU_LENGTH] = {0U};
    PduInfoType pduInfo;
    Std_ReturnType ret;
    pduInfo.SduDataPtr = txData;
    pduInfo.MetaDataPtr = NULL_PTR;
    pduInfo.SduLength = UDPNM_PDU_LENGTH;
    ret = UdpNm_Transmit(UDPNM_CHANNEL_0, &pduInfo);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_UdpNm_00011 */
void test_UdpNm_Transmit_NullPduInfo_ReportsInvalidPointer(void) {
    Std_ReturnType ret;
    UdpNm_Init(&testConfig);
    ret = UdpNm_Transmit(UDPNM_CHANNEL_0, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_E_INVALID_POINTER, mock_DetLastErrorId);
}

/** @req SWS_UdpNm_00011 */
void test_UdpNm_Transmit_ValidPdu_ReturnsOk(void) {
    uint8 txData[UDPNM_PDU_LENGTH] = {0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U};
    PduInfoType pduInfo;
    Std_ReturnType ret;
    UdpNm_Init(&testConfig);
    pduInfo.SduDataPtr = txData;
    pduInfo.MetaDataPtr = NULL_PTR;
    pduInfo.SduLength = UDPNM_PDU_LENGTH;
    ret = UdpNm_Transmit(UDPNM_CHANNEL_0, &pduInfo);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_UdpNm_00100 */
void test_UdpNm_TxConfirmation_UnknownPduId_IsIgnored(void) {
    /* No matching channel: the SUT must silently ignore the confirmation. */
    UdpNm_Init(&testConfig);
    UdpNm_TxConfirmation(0xFFFFU);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_UdpNm_00008 */
void test_UdpNm_GetUserData_BeforeInit_ReportsUninit(void) {
    uint8 userData[UDPNM_USER_DATA_LENGTH];
    Std_ReturnType ret = UdpNm_GetUserData(UDPNM_CHANNEL_0, userData);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_SID_GETUSERDATA, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(UDPNM_E_UNINIT, mock_DetLastErrorId);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_UdpNm_RxIndication_ThenGetPduData_ReturnsReceivedPdu);
    RUN_TEST(test_UdpNm_RxIndication_ThenGetUserData_ReturnsUserDataPortion);
    RUN_TEST(test_UdpNm_RxIndication_SleepingChannel_MainFunctionWakesToRepeatMessage);
    RUN_TEST(test_UdpNm_RemoteSleepIndication_ReflectedInCheckApi);
    RUN_TEST(test_UdpNm_DisableCommunication_StallsRepeatMessageTimeout);
    RUN_TEST(test_UdpNm_EnableCommunication_ResumesStateMachine);
    RUN_TEST(test_UdpNm_RepeatMessageTimeout_YieldsNormalOperationWhenRequested);
    RUN_TEST(test_UdpNm_Transmit_BeforeInit_ReportsUninit);
    RUN_TEST(test_UdpNm_Transmit_NullPduInfo_ReportsInvalidPointer);
    RUN_TEST(test_UdpNm_Transmit_ValidPdu_ReturnsOk);
    RUN_TEST(test_UdpNm_TxConfirmation_UnknownPduId_IsIgnored);
    RUN_TEST(test_UdpNm_GetUserData_BeforeInit_ReportsUninit);

    return UnityEnd();
}
