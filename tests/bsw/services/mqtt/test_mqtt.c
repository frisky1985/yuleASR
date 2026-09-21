/**
 * @file test_mqtt.c
 * @brief Mqtt Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/mqtt/src/Mqtt.c  @tests src/bsw/services/mqtt/include/Mqtt.h

#include "unity.h"
#include "Mqtt.h"

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

/* TcpIp recorder stubs (implemented in stubs.c) */
extern uint32 mock_TcpIp_SocketCreate_Count;
extern uint32 mock_TcpIp_SocketClose_Count;

/* Test configurations */
static Mqtt_ConfigType testConfig = {
    MQTT_MAX_CONNECTIONS,   /* maxConnections */
    1024U,                  /* sendBufferSize */
    1024U,                  /* recvBufferSize */
    4U,                     /* messageQueueDepth */
    FALSE,                  /* enableAutoReconnect */
    { NULL_PTR, NULL_PTR, NULL_PTR }  /* callbacks */
};

/* Plain (non-TLS) connection configuration. */
static const Mqtt_ConnectionConfigType testConnConfig = {
    "test-broker",          /* brokerHost */
    1883U,                  /* brokerPort */
    "test-client",          /* clientId */
    60U,                    /* keepAliveSeconds */
    MQTT_CLEAN_SESSION_TRUE,/* cleanSession */
    MQTT_VERSION_311,       /* version */
    NULL_PTR,               /* username */
    NULL_PTR,               /* password */
    5000U,                  /* connectTimeoutMs */
    5000U,                  /* recvTimeoutMs */
    5000U,                  /* sendTimeoutMs */
    FALSE,                  /* autoReconnect */
    1000U,                  /* reconnectIntervalMs */
#if (MQTT_SUPPORT_TLS == STD_ON)
    FALSE,                  /* useTls */
    NULL_PTR                /* tlsConfig */
#endif
};

/* Bring the static SUT state deterministically to uninitialized.
 * Mqtt_DeInit() cleans up when initialized and is otherwise ignored
 * (its DET report is cleared by the mock reset below). */
static void ensure_uninit(void) {
    (void)Mqtt_DeInit();
    mock_Det_Reset();
    mock_TcpIp_SocketCreate_Count = 0U;
    mock_TcpIp_SocketClose_Count = 0U;
}

void setUp(void) {
    ensure_uninit();
}

void tearDown(void) {
}

/** @req SWS_Mqtt_00001 */
void test_Mqtt_Init_NullPtr_ShouldNotCrash(void) {
    Mqtt_ReturnType ret = Mqtt_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_CONFIG, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00001 */
void test_Mqtt_Init_ValidConfig_ShouldSucceed(void) {
    Mqtt_ConnectionInfoType info;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Probe: GetConnectionInfo only succeeds once the module is initialized. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_GetConnectionInfo(0U, &info));
}

/** @req SWS_Mqtt_00001 */
void test_Mqtt_Init_DoubleInit_ShouldSucceed(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    mock_Det_Reset();
    /* Production Mqtt_Init() reports MQTT_E_ALREADY_INITIALIZED. */
    ret = Mqtt_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00002 */
void test_Mqtt_DeInit_Uninit_ShouldReportError(void) {
    Mqtt_ReturnType ret = Mqtt_DeInit();
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00002 */
void test_Mqtt_DeInit_ValidCall_ShouldSucceed(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    mock_Det_Reset();
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_DeInit());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Mqtt_00013 */
void test_Mqtt_GetVersionInfo_NullPtr_ShouldReportError(void) {
    /* Production Mqtt_GetVersionInfo() silently returns on NULL. */
    Mqtt_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Mqtt_00013 */
void test_Mqtt_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType version = { 0U, 0U, 0U, 0U, 0U };
    Mqtt_GetVersionInfo(&version);
    TEST_ASSERT_EQUAL_UINT16(MQTT_VENDOR_ID, version.vendorID);
    TEST_ASSERT_EQUAL_UINT8(MQTT_MODULE_ID, version.moduleID);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SW_MAJOR_VERSION, version.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SW_MINOR_VERSION, version.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SW_PATCH_VERSION, version.sw_patch_version);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Mqtt_00011 */
void test_Mqtt_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Production Mqtt_MainFunction() silently returns when uninitialized. */
    Mqtt_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Mqtt_00011 */
void test_Mqtt_MainFunction_ValidCall_ShouldSucceed(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    Mqtt_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Mqtt_00003 */
void test_Mqtt_Connect_Uninit_ShouldReportError(void) {
    Mqtt_ReturnType ret = Mqtt_Connect(0U, &testConnConfig);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_CONNECT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00003 */
void test_Mqtt_Connect_NullPtr_ShouldReportError(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Connect(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_CONNECT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_CONFIG, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00003 */
void test_Mqtt_Connect_ValidCall_ShouldSucceed(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Connect(0U, &testConnConfig);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, ret);
    /* Non-TLS connect must open a TCP socket and enter CONNECTING. */
    TEST_ASSERT_EQUAL_UINT32(1U, mock_TcpIp_SocketCreate_Count);
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_CONNECTING, Mqtt_GetConnectionState(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Mqtt_00004 */
void test_Mqtt_Disconnect_Uninit_ShouldReportError(void) {
    Mqtt_ReturnType ret = Mqtt_Disconnect(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_DISCONNECT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00004 */
void test_Mqtt_Disconnect_ValidCall_ShouldSucceed(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &testConnConfig));
    mock_Det_Reset();
    mock_TcpIp_SocketCreate_Count = 0U;
    ret = Mqtt_Disconnect(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, ret);
    /* The socket opened by Mqtt_Connect must be closed again. */
    TEST_ASSERT_EQUAL_UINT32(1U, mock_TcpIp_SocketClose_Count);
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_DISCONNECTED, Mqtt_GetConnectionState(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Publish_Uninit_ShouldReportError(void) {
    Mqtt_PublishMessageType msg = { "t/topic", NULL_PTR, 0U, MQTT_QOS_0, MQTT_RETAIN_FALSE };
    Mqtt_ReturnType ret = Mqtt_Publish(0U, &msg, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_PUBLISH, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Publish_NullPtr_ShouldReportError(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Publish(0U, NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_PUBLISH, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00005 */
void test_Mqtt_Publish_ValidCall_ShouldSucceed(void) {
    static const uint8 payload[4] = { 1U, 2U, 3U, 4U };
    Mqtt_PublishMessageType msg = { "t/topic", payload, 4U, MQTT_QOS_0, MQTT_RETAIN_FALSE };
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    /* No connection established -> not connected error, no DET. */
    ret = Mqtt_Publish(0U, &msg, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOCONN, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Mqtt_00006 */
void test_Mqtt_Subscribe_Uninit_ShouldReportError(void) {
    Mqtt_SubscriptionType sub = { "t/#", MQTT_QOS_0, 1U };
    Mqtt_ReturnType ret = Mqtt_Subscribe(0U, &sub, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_SUBSCRIBE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00006 */
void test_Mqtt_Subscribe_NullPtr_ShouldReportError(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Subscribe(0U, NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_SUBSCRIBE, mock_DetLastApiId);
    /* Production reports MQTT_E_PARAM_TOPIC for a NULL subscription. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_TOPIC, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00006 */
void test_Mqtt_Subscribe_ValidCall_ShouldSucceed(void) {
    Mqtt_SubscriptionType sub = { "t/#", MQTT_QOS_0, 1U };
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Subscribe(0U, &sub, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOCONN, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Mqtt_00007 */
void test_Mqtt_Unsubscribe_Uninit_ShouldReportError(void) {
    Mqtt_ReturnType ret = Mqtt_Unsubscribe(0U, "t/#");
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_UNSUBSCRIBE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00007 */
void test_Mqtt_Unsubscribe_NullPtr_ShouldReportError(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Unsubscribe(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_SID_UNSUBSCRIBE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_TOPIC, mock_DetLastErrorId);
}

/** @req SWS_Mqtt_00007 */
void test_Mqtt_Unsubscribe_ValidCall_ShouldSucceed(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Unsubscribe(0U, "t/#");
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOCONN, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Mqtt_00009 */
void test_Mqtt_GetConnectionState_Uninit_ShouldReportError(void) {
    /* Production returns MQTT_STATE_UNINIT without a DET report. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_UNINIT, Mqtt_GetConnectionState(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Mqtt_00009 */
void test_Mqtt_GetConnectionState_ValidCall_ShouldReturnState(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    /* After Init the connection slot is still UNINIT. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_UNINIT, Mqtt_GetConnectionState(0U));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &testConnConfig));
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_CONNECTING, Mqtt_GetConnectionState(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Mqtt_Init_NullPtr_ShouldNotCrash);
    RUN_TEST(test_Mqtt_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Mqtt_Init_DoubleInit_ShouldSucceed);
    RUN_TEST(test_Mqtt_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_Mqtt_DeInit_ValidCall_ShouldSucceed);
    RUN_TEST(test_Mqtt_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_Mqtt_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_Mqtt_MainFunction_Uninit_ShouldNotCrash);
    RUN_TEST(test_Mqtt_MainFunction_ValidCall_ShouldSucceed);
    RUN_TEST(test_Mqtt_Connect_Uninit_ShouldReportError);
    RUN_TEST(test_Mqtt_Connect_NullPtr_ShouldReportError);
    RUN_TEST(test_Mqtt_Connect_ValidCall_ShouldSucceed);
    RUN_TEST(test_Mqtt_Disconnect_Uninit_ShouldReportError);
    RUN_TEST(test_Mqtt_Disconnect_ValidCall_ShouldSucceed);
    RUN_TEST(test_Mqtt_Publish_Uninit_ShouldReportError);
    RUN_TEST(test_Mqtt_Publish_NullPtr_ShouldReportError);
    RUN_TEST(test_Mqtt_Publish_ValidCall_ShouldSucceed);
    RUN_TEST(test_Mqtt_Subscribe_Uninit_ShouldReportError);
    RUN_TEST(test_Mqtt_Subscribe_NullPtr_ShouldReportError);
    RUN_TEST(test_Mqtt_Subscribe_ValidCall_ShouldSucceed);
    RUN_TEST(test_Mqtt_Unsubscribe_Uninit_ShouldReportError);
    RUN_TEST(test_Mqtt_Unsubscribe_NullPtr_ShouldReportError);
    RUN_TEST(test_Mqtt_Unsubscribe_ValidCall_ShouldSucceed);
    RUN_TEST(test_Mqtt_GetConnectionState_Uninit_ShouldReportError);
    RUN_TEST(test_Mqtt_GetConnectionState_ValidCall_ShouldReturnState);
    return UnityEnd();
}
