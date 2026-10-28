/**
 * @file test_mqtt.c
 * @brief Mqtt Unit Tests
 * @version 1.1.0
 * @date 2026-10-26
 */

// @tests src/bsw/services/mqtt/src/Mqtt.c  @tests src/bsw/services/mqtt/include/Mqtt.h
// @tests src/bsw/services/mqtt/src/Mqtt_Lcfg.c  @tests src/bsw/services/mqtt/src/Mqtt_Tls.c
//
// NOTE (white-box): this target compiles a GENERATED copy of Mqtt.c
// (tests/bsw/services/mqtt/whitebox/src/bsw/services/mqtt/src/Mqtt.c) with the
// three static module variables demoted to external linkage plus a small
// test-only accessor appendix. This is required because the production state
// machine has NO path to MQTT_STATE_CONNECTED (no CONNACK processing exists),
// so all CONNECTED branches are unreachable through the public API. The
// production source is not modified.
//
// NOTE (product defect, recorded not fixed): Mqtt_Connect() leaves the
// connection in MQTT_STATE_CONNECTING and Mqtt_MainFunction() re-sends the
// CONNECT packet on every call while staying in CONNECTING. There is no code
// that parses a CONNACK or sets MQTT_STATE_CONNECTED/MQTT_STATE_MQTT_CONNECTING
// (the MQTT_CONNECTING / TCP_CONNECTING / TLS_HANDSHAKING states are also
// unreachable). Mqtt.c was therefore tested for the behaviour it actually has.

#include "unity.h"
#include "Mqtt.h"
#include <string.h>
#include <stdint.h>

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

/* TcpIp stubs (implemented in stubs.c) */
extern int mock_TcpIp_SocketCreate_Fail;
extern int mock_TcpIp_Send_Fail;
extern int mock_TcpIp_Receive_Fail;
extern uint32 mock_TcpIp_SocketCreate_Count;
extern uint32 mock_TcpIp_SocketClose_Count;
extern uint32 mock_TcpIp_Send_Count;
extern uint8 mock_TcpIp_SendLog[4096];
extern uint32 mock_TcpIp_SendLogLen;
extern void mock_TcpIp_Reset(void);
extern void mock_TcpIp_InjectRx(const uint8* data, uint32 len);

/* White-box accessors (generated appendix of the Mqtt.c test copy) */
extern void Mqtt_Test_SetState(uint8 id, uint8 newState);
extern void Mqtt_Test_SetPendingPing(uint8 id, uint8 v);
extern uint16 Mqtt_Test_GetPacketIdCounter(uint8 id);
extern void Mqtt_Test_SetUseTls(uint8 id, uint8 v);
extern void Mqtt_Test_SetTlsContext(uint8 id, void* ctx);

/* Mqtt_Lcfg.c exports (no extern declarations in Mqtt.h besides Mqtt_ConfigPtr) */
extern const Mqtt_ConfigType Mqtt_Config;
extern const Mqtt_ConnectionConfigType Mqtt_DefaultConnectionConfig;
extern void Mqtt_DefaultConnectionCallback(uint8 connectionId, uint8 newState);
extern void Mqtt_DefaultMessageCallback(uint8 connectionId, const char* topic,
                                        const uint8* payload, uint32 length);
extern void Mqtt_DefaultErrorCallback(uint8 connectionId, uint8 errorCode);

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

#if (MQTT_SUPPORT_TLS == STD_ON)
/* Minimal TLS config: no CA, no client cert, no hostname. With the real
 * mbedTLS linked (like service_mqtt) Mqtt_Tls_CreateContext succeeds. */
static Mqtt_TlsConfigType tlsMinimalConfig;

static const Mqtt_ConnectionConfigType testTlsConnConfig = {
    "test-broker",
    8883U,
    "test-client",
    60U,
    MQTT_CLEAN_SESSION_TRUE,
    MQTT_VERSION_311,
    NULL_PTR,
    NULL_PTR,
    5000U,
    5000U,
    5000U,
    FALSE,
    1000U,
#if (MQTT_SUPPORT_TLS == STD_ON)
    TRUE,                   /* useTls */
    &tlsMinimalConfig
#endif
};
#endif

/* Callback recorders */
static uint8 rec_pubCbConnId;
static uint16 rec_pubCbSubId;
static Mqtt_ReturnType rec_pubCbResult;
static uint8 rec_pubCbCalls;

static void recordPublishCallback(Mqtt_ConnectionIdType id, Mqtt_SubscriptionIdType subId,
                                  Mqtt_ReturnType result) {
    rec_pubCbConnId = (uint8)id;
    rec_pubCbSubId = subId;
    rec_pubCbResult = result;
    rec_pubCbCalls++;
}

static uint8 rec_connCbConnId;
static uint8 rec_connCbState;
static uint8 rec_connCbCalls;

static void recordConnCallback(Mqtt_ConnectionIdType id, Mqtt_ConnectionStateType newState,
                               Mqtt_ReturnType result) {
    (void)result;
    rec_connCbConnId = (uint8)id;
    rec_connCbState = (uint8)newState;
    rec_connCbCalls++;
}

/* Bring the static SUT state deterministically to uninitialized. */
static void ensure_uninit(void) {
    (void)Mqtt_DeInit();
    mock_Det_Reset();
    mock_TcpIp_Reset();
}

void setUp(void) {
    ensure_uninit();
}

void tearDown(void) {
}

/*============================================================================
 * Existing baseline cases (kept from v1.0)
 *============================================================================*/

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

/*============================================================================
 * New: parameter validation paths
 *============================================================================*/

void test_Mqtt_Connect_InvalidConnectionId_ShouldReportError(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Connect(MQTT_MAX_CONNECTIONS, &testConnConfig);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_CONNECTION, mock_DetLastErrorId);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_TcpIp_SocketCreate_Count);
}

void test_Mqtt_Connect_NullBrokerHost_ShouldReportError(void) {
    Mqtt_ConnectionConfigType cfg = testConnConfig;
    Mqtt_ReturnType ret;
    cfg.brokerHost = NULL_PTR;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Connect(0U, &cfg);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_Mqtt_Connect_NullClientId_ShouldReportError(void) {
    Mqtt_ConnectionConfigType cfg = testConnConfig;
    Mqtt_ReturnType ret;
    cfg.clientId = NULL_PTR;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Connect(0U, &cfg);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_POINTER, mock_DetLastErrorId);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_TcpIp_SocketCreate_Count);
}

void test_Mqtt_Connect_SocketCreateFailure_ShouldReturnConnFailed(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    mock_TcpIp_SocketCreate_Fail = 1;
    ret = Mqtt_Connect(0U, &testConnConfig);
    /* Production reports no DET here; state stays UNINIT. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_CONNECTION_FAILED, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_UNINIT, Mqtt_GetConnectionState(0U));
}

void test_Mqtt_Publish_TopicNull_ShouldReportError(void) {
    static const uint8 payload[2] = { 1U, 2U };
    Mqtt_PublishMessageType msg = { NULL_PTR, payload, 2U, MQTT_QOS_0, MQTT_RETAIN_FALSE };
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Publish(0U, &msg, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_TOPIC, mock_DetLastErrorId);
}

void test_Mqtt_Publish_InvalidConnectionId_ShouldReportError(void) {
    Mqtt_PublishMessageType msg = { "t", NULL_PTR, 0U, MQTT_QOS_0, MQTT_RETAIN_FALSE };
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Publish(MQTT_MAX_CONNECTIONS, &msg, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_CONNECTION, mock_DetLastErrorId);
}

void test_Mqtt_Disconnect_InvalidConnectionId_ShouldReportError(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Disconnect(MQTT_MAX_CONNECTIONS);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_CONNECTION, mock_DetLastErrorId);
}

void test_Mqtt_Disconnect_AlreadyDisconnected_ShouldBeIdempotent(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &testConnConfig));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Disconnect(0U));
    /* Second disconnect on DISCONNECTED state: OK, no extra socket close. */
    mock_TcpIp_SocketClose_Count = 0U;
    ret = Mqtt_Disconnect(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_TcpIp_SocketClose_Count);
}

void test_Mqtt_GetConnectionInfo_Errors(void) {
    Mqtt_ConnectionInfoType info;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_UNINIT, Mqtt_GetConnectionInfo(0U, &info));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_GetConnectionInfo(MQTT_MAX_CONNECTIONS, &info));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_GetConnectionInfo(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_GetConnectionInfo(0U, &info));
    TEST_ASSERT_EQUAL_UINT32(0U, info.disconnectCount);
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_UNINIT, info.state);
}

void test_Mqtt_SetConnectionCallback_InvalidId_ShouldBeIgnored(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    rec_connCbCalls = 0U;
    Mqtt_SetConnectionCallback(MQTT_MAX_CONNECTIONS, &recordConnCallback);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &testConnConfig));
    /* Callback was registered for an invalid id, so it must not fire. */
    TEST_ASSERT_EQUAL_UINT8(0U, rec_connCbCalls);
}

void test_Mqtt_Ping_Uninit_ShouldReturnUninit(void) {
    /* Mqtt_Ping returns error codes directly without DET reports. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_UNINIT, Mqtt_Ping(0U));
}

void test_Mqtt_Ping_InvalidId_ShouldReturnNotOk(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Ping(MQTT_MAX_CONNECTIONS));
}

void test_Mqtt_Ping_NotConnected_ShouldReturnNoConn(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &testConnConfig));
    /* CONNECTING is not CONNECTED -> no ping. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOCONN, Mqtt_Ping(0U));
}

/*============================================================================
 * New: state machine / packet encoding (CONNECTING is reachable via Connect)
 *============================================================================*/

void test_Mqtt_MainFunction_ConnectingState_SendsConnectPacket(void) {
    /* Expected MQTT CONNECT for testConnConfig:
     *   0x10, remainingLength=10+2+11=23
     *   00 04 'M' 'Q' 'T' 'T', level=4 (3.1.1), flags=0x02 (clean session),
     *   keepalive=60 (0x00 0x3C), clientId len 0x000B, "test-client" */
    static const uint8 expected[] = {
        0x10U, 23U,
        0x00U, 0x04U, (uint8)'M', (uint8)'Q', (uint8)'T', (uint8)'T',
        0x04U, 0x02U, 0x00U, 0x3CU,
        0x00U, 0x0BU, (uint8)'t', (uint8)'e', (uint8)'s', (uint8)'t', (uint8)'-',
        (uint8)'c', (uint8)'l', (uint8)'i', (uint8)'e', (uint8)'n', (uint8)'t'
    };
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &testConnConfig));
    Mqtt_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_TcpIp_Send_Count);
    TEST_ASSERT_EQUAL_UINT32(sizeof(expected), mock_TcpIp_SendLogLen);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, mock_TcpIp_SendLog, sizeof(expected));
    /* NOTE (actual behaviour): the state machine stays in CONNECTING and has
     * no CONNACK handling, so it re-sends the CONNECT packet on every cycle. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_CONNECTING, Mqtt_GetConnectionState(0U));
}

void test_Mqtt_MainFunction_ConnectingState_ResendsEveryCycle(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &testConnConfig));
    Mqtt_MainFunction();
    Mqtt_MainFunction();
    Mqtt_MainFunction();
    /* No state transition -> repeated CONNECT transmissions. */
    TEST_ASSERT_EQUAL_UINT32(3U, mock_TcpIp_Send_Count);
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_CONNECTING, Mqtt_GetConnectionState(0U));
}

void test_Mqtt_EncodeConnect_MultiByteRemainingLength(void) {
    /* clientId of 130 chars => remainingLength = 10 + 2 + 130 = 142 >= 128,
     * which must be encoded as two variable-length bytes (0x8E 0x01). */
    static char longId[131];
    Mqtt_ConnectionConfigType cfg = testConnConfig;
    uint8 i;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    for (i = 0; i < 130U; i++) {
        longId[i] = 'a';
    }
    longId[130] = '\0';
    cfg.clientId = longId;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &cfg));
    Mqtt_MainFunction();
    /* 1 (type) + 2 (multi-byte remaining length) + 142 = 145 */
    TEST_ASSERT_EQUAL_UINT32(145U, mock_TcpIp_SendLogLen);
    TEST_ASSERT_EQUAL_HEX8(0x10U, mock_TcpIp_SendLog[0]);
    TEST_ASSERT_EQUAL_HEX8(0x8EU, mock_TcpIp_SendLog[1]); /* (142 & 0x7F) | 0x80 */
    TEST_ASSERT_EQUAL_HEX8(0x01U, mock_TcpIp_SendLog[2]); /* 142 >> 7 */
}

void test_Mqtt_EncodeConnect_UsernamePasswordFlags(void) {
    /* username + password set => connectFlags = 0x02|0x80|0x40 = 0xC2 and the
     * remaining length grows by 2+1 (user) + 2+1 (pwd). */
    Mqtt_ConnectionConfigType cfg = testConnConfig;
    static const char user[] = "u";
    static const char pwd[] = "p";
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    cfg.username = user;
    cfg.password = pwd;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &cfg));
    Mqtt_MainFunction();
    /* remainingLength = 10 + 2+11 + 2+1 + 2+1 = 29 */
    TEST_ASSERT_EQUAL_HEX8(29U, mock_TcpIp_SendLog[1]);
    TEST_ASSERT_EQUAL_HEX8(0xC2U, mock_TcpIp_SendLog[9]);
    /* Layout: [0..1] fixed header, [2..7] "MQTT", [8] level, [9] flags,
     * [10..11] keepalive, [12..13] clientId length (11), [14..24] clientId,
     * then username field. */
    TEST_ASSERT_EQUAL_HEX8(0x00U, mock_TcpIp_SendLog[12]);
    TEST_ASSERT_EQUAL_HEX8(0x0BU, mock_TcpIp_SendLog[13]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8*)"test-client", &mock_TcpIp_SendLog[14], 11U);
    /* username field 0x0001 'u' at offset 25, password 0x0001 'p' at 28. */
    TEST_ASSERT_EQUAL_HEX8(0x00U, mock_TcpIp_SendLog[25]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, mock_TcpIp_SendLog[26]);
    TEST_ASSERT_EQUAL_HEX8((uint8)'u', mock_TcpIp_SendLog[27]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, mock_TcpIp_SendLog[28]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, mock_TcpIp_SendLog[29]);
    TEST_ASSERT_EQUAL_HEX8((uint8)'p', mock_TcpIp_SendLog[30]);
}

void test_Mqtt_EncodeConnect_Version50_Level5(void) {
    Mqtt_ConnectionConfigType cfg = testConnConfig;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    cfg.version = MQTT_VERSION_50;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &cfg));
    Mqtt_MainFunction();
    TEST_ASSERT_EQUAL_HEX8(5U, mock_TcpIp_SendLog[8]); /* protocol level */
}

void test_Mqtt_Connect_RegistersStateCallback(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    rec_connCbCalls = 0U;
    Mqtt_SetConnectionCallback(0U, &recordConnCallback);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &testConnConfig));
    /* Mqtt_UpdateState fires the callback on state change to CONNECTING. */
    TEST_ASSERT_EQUAL_UINT8(1U, rec_connCbCalls);
    TEST_ASSERT_EQUAL_UINT8(0U, rec_connCbConnId);
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_CONNECTING, rec_connCbState);
}

/*============================================================================
 * New: white-box CONNECTED paths (publish/subscribe/ping/disconnect/receive)
 *============================================================================*/

static void connectAndMarkConnected(uint8 id) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(id, &testConnConfig));
    /* Simulate broker CONNACK completion. */
    Mqtt_Test_SetState(id, (uint8)MQTT_STATE_CONNECTED);
}

void test_Mqtt_Publish_Qos0_WireFormat(void) {
    static const uint8 payload[4] = { 1U, 2U, 3U, 4U };
    Mqtt_PublishMessageType msg = { "t/topic", payload, 4U, MQTT_QOS_0, MQTT_RETAIN_FALSE };
    Mqtt_ConnectionInfoType info;
    connectAndMarkConnected(0U);
    rec_pubCbCalls = 0U;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Publish(0U, &msg, &recordPublishCallback));
    /* 0x30, remLen=2+7+4=13, topic len 0x0007, "t/topic", payload */
    TEST_ASSERT_EQUAL_HEX8(0x30U, mock_TcpIp_SendLog[0]);
    TEST_ASSERT_EQUAL_HEX8(13U, mock_TcpIp_SendLog[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, mock_TcpIp_SendLog[2]);
    TEST_ASSERT_EQUAL_HEX8(0x07U, mock_TcpIp_SendLog[3]);
    TEST_ASSERT_EQUAL_MEMORY("t/topic", &mock_TcpIp_SendLog[4], 7U);
    TEST_ASSERT_EQUAL_MEMORY(payload, &mock_TcpIp_SendLog[11], 4U);
    TEST_ASSERT_EQUAL_UINT32(15U, mock_TcpIp_SendLogLen);
    TEST_ASSERT_EQUAL_UINT8(1U, rec_pubCbCalls);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, rec_pubCbResult);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_GetConnectionInfo(0U, &info));
    TEST_ASSERT_EQUAL_UINT32(1U, info.messagesSent);
    TEST_ASSERT_EQUAL_UINT32(4U, info.bytesSent);
}

void test_Mqtt_Publish_Qos1_Retain_IncludesPacketId(void) {
    static const uint8 payload[4] = { 9U, 8U, 7U, 6U };
    Mqtt_PublishMessageType msg = { "t/topic", payload, 4U, MQTT_QOS_1, MQTT_RETAIN_TRUE };
    connectAndMarkConnected(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Publish(0U, &msg, NULL_PTR));
    /* fixed header = 0x30 | (QoS1 << 1) | retain = 0x33; packet id = 1 */
    TEST_ASSERT_EQUAL_HEX8(0x33U, mock_TcpIp_SendLog[0]);
    TEST_ASSERT_EQUAL_HEX8(15U, mock_TcpIp_SendLog[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, mock_TcpIp_SendLog[11]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, mock_TcpIp_SendLog[12]);
    TEST_ASSERT_EQUAL_UINT16(1U, Mqtt_Test_GetPacketIdCounter(0U));
}

void test_Mqtt_Publish_Qos2_Header(void) {
    static const uint8 payload[1] = { 0xA5U };
    Mqtt_PublishMessageType msg = { "x", payload, 1U, MQTT_QOS_2, MQTT_RETAIN_FALSE };
    connectAndMarkConnected(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Publish(0U, &msg, NULL_PTR));
    TEST_ASSERT_EQUAL_HEX8(0x34U, mock_TcpIp_SendLog[0]); /* QoS2, no retain */
}

void test_Mqtt_Publish_PacketId_Increments(void) {
    static const uint8 payload[1] = { 1U };
    Mqtt_PublishMessageType msg = { "t", payload, 1U, MQTT_QOS_1, MQTT_RETAIN_FALSE };
    connectAndMarkConnected(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Publish(0U, &msg, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Publish(0U, &msg, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT16(2U, Mqtt_Test_GetPacketIdCounter(0U));
}

void test_Mqtt_Publish_SendFailure_ShouldReturnPublishFailed(void) {
    static const uint8 payload[1] = { 1U };
    Mqtt_PublishMessageType msg = { "t", payload, 1U, MQTT_QOS_0, MQTT_RETAIN_FALSE };
    Mqtt_ConnectionInfoType info;
    connectAndMarkConnected(0U);
    mock_TcpIp_Send_Fail = 1;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PUBLISH_FAILED, Mqtt_Publish(0U, &msg, NULL_PTR));
    /* Statistics must not be updated on failure. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_GetConnectionInfo(0U, &info));
    TEST_ASSERT_EQUAL_UINT32(0U, info.messagesSent);
}

void test_Mqtt_Publish_TooLarge_ShouldReturnBufferOverflow(void) {
    static uint8 bigPayload[2040];
    Mqtt_PublishMessageType msg = { "t/topic", bigPayload, 2040U, MQTT_QOS_1, MQTT_RETAIN_FALSE };
    connectAndMarkConnected(0U);
    /* remainingLength = 2+7+2+2040 = 2051 -> remainingLength+5 > 2048 */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_BUFFER_OVERFLOW, Mqtt_Publish(0U, &msg, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_TcpIp_Send_Count);
}

void test_Mqtt_Subscribe_WireFormat(void) {
    Mqtt_SubscriptionType sub = { "t/#", MQTT_QOS_0, 1U };
    connectAndMarkConnected(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Subscribe(0U, &sub, NULL_PTR));
    /* 0x82, remLen=2+2+3+1=8, packet id 1, topic len 3, "t/#", qos 0 */
    TEST_ASSERT_EQUAL_HEX8(0x82U, mock_TcpIp_SendLog[0]);
    TEST_ASSERT_EQUAL_HEX8(8U, mock_TcpIp_SendLog[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, mock_TcpIp_SendLog[2]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, mock_TcpIp_SendLog[3]);
    TEST_ASSERT_EQUAL_MEMORY("t/#", &mock_TcpIp_SendLog[6], 3U);
    TEST_ASSERT_EQUAL_HEX8(0x00U, mock_TcpIp_SendLog[9]);
    TEST_ASSERT_EQUAL_UINT32(10U, mock_TcpIp_SendLogLen);
}

void test_Mqtt_Subscribe_SendFailure_ShouldRollBackSlot(void) {
    Mqtt_SubscriptionType sub = { "t/#", MQTT_QOS_0, 1U };
    connectAndMarkConnected(0U);
    mock_TcpIp_Send_Fail = 1;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_SUBSCRIBE_FAILED, Mqtt_Subscribe(0U, &sub, NULL_PTR));
    /* Failed subscribe must not leave a PENDING entry behind. */
    mock_TcpIp_Send_Fail = 0;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Unsubscribe(0U, "t/#"));
}

void test_Mqtt_Subscribe_TooLarge_ShouldReturnBufferOverflow(void) {
    static char longFilter[2041];
    Mqtt_SubscriptionType sub = { longFilter, MQTT_QOS_0, 1U };
    uint16 i;
    for (i = 0; i < 2040U; i++) {
        longFilter[i] = 'a';
    }
    longFilter[2040] = '\0';
    connectAndMarkConnected(0U);
    /* remainingLength = 2+2+2040+1 = 2045 -> +5 > 2048 */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_BUFFER_OVERFLOW, Mqtt_Subscribe(0U, &sub, NULL_PTR));
}

void test_Mqtt_Unsubscribe_Connected_ShouldRemoveSubscription(void) {
    Mqtt_SubscriptionType sub = { "t/#", MQTT_QOS_0, 1U };
    connectAndMarkConnected(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Subscribe(0U, &sub, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Unsubscribe(0U, "t/#"));
    /* Entry removed -> second unsubscribe reports not found.
     * NOTE (actual behaviour): Mqtt_Unsubscribe does NOT send an UNSUBSCRIBE
     * packet, it only clears the local subscription slot. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Unsubscribe(0U, "t/#"));
}

void test_Mqtt_Unsubscribe_Connected_UnknownTopic_ShouldReturnNotOk(void) {
    connectAndMarkConnected(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Unsubscribe(0U, "other"));
}

void test_Mqtt_Ping_Connected_ShouldSendPingReq(void) {
    connectAndMarkConnected(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Ping(0U));
    TEST_ASSERT_EQUAL_HEX8(0xC0U, mock_TcpIp_SendLog[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, mock_TcpIp_SendLog[1]);
    TEST_ASSERT_EQUAL_UINT32(2U, mock_TcpIp_SendLogLen);
}

void test_Mqtt_Ping_PendingPing_ShouldReturnBusy(void) {
    connectAndMarkConnected(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Ping(0U));
    /* PINGREQ without PINGRESP -> pendingPing is still set. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_BUSY, Mqtt_Ping(0U));
    /* White-box: clear the pending flag, ping works again. */
    Mqtt_Test_SetPendingPing(0U, 0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Ping(0U));
}

void test_Mqtt_MainFunction_Connected_ReceivesInjectedData(void) {
    /* MQTT CONNACK for reference: 0x20 0x02 0x00 0x00 */
    static const uint8 connack[4] = { 0x20U, 0x02U, 0x00U, 0x00U };
    Mqtt_ConnectionInfoType info;
    connectAndMarkConnected(0U);
    mock_TcpIp_InjectRx(connack, 4U);
    Mqtt_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_GetConnectionInfo(0U, &info));
    /* Mqtt_ReceivePacket accounts the received bytes; NOTE (actual behaviour):
     * no packet parsing happens (no message dispatch/callbacks implemented). */
    TEST_ASSERT_EQUAL_UINT32(4U, info.bytesReceived);
}

void test_Mqtt_MainFunction_Connected_ReceiveFailure_ShouldNotCountBytes(void) {
    Mqtt_ConnectionInfoType info;
    connectAndMarkConnected(0U);
    mock_TcpIp_Receive_Fail = 1;
    Mqtt_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_GetConnectionInfo(0U, &info));
    TEST_ASSERT_EQUAL_UINT32(0U, info.bytesReceived);
}

void test_Mqtt_Disconnect_Connected_ShouldSendDisconnectPacket(void) {
    connectAndMarkConnected(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Disconnect(0U));
    /* Last two bytes on the wire must be the DISCONNECT packet. */
    TEST_ASSERT_EQUAL_HEX8(0xE0U, mock_TcpIp_SendLog[mock_TcpIp_SendLogLen - 2U]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, mock_TcpIp_SendLog[mock_TcpIp_SendLogLen - 1U]);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_TcpIp_SocketClose_Count);
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_DISCONNECTED, Mqtt_GetConnectionState(0U));
}

void test_Mqtt_Disconnect_Connected_CountsDisconnect(void) {
    Mqtt_ConnectionInfoType info;
    connectAndMarkConnected(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Disconnect(0U));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_GetConnectionInfo(0U, &info));
    TEST_ASSERT_EQUAL_UINT32(1U, info.disconnectCount);
}

void test_Mqtt_Disconnect_Connected_FiresCallbackOnTransition(void) {
    connectAndMarkConnected(0U);
    rec_connCbCalls = 0U;
    Mqtt_SetConnectionCallback(0U, &recordConnCallback);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Disconnect(0U));
    TEST_ASSERT_EQUAL_UINT8(1U, rec_connCbCalls);
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_DISCONNECTED, rec_connCbState);
}

void test_Mqtt_DeInit_ConnectedConnections_ShouldCloseAllSockets(void) {
    connectAndMarkConnected(0U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(1U, &testConnConfig));
    Mqtt_Test_SetState(1U, (uint8)MQTT_STATE_CONNECTED);
    mock_TcpIp_SocketClose_Count = 0U;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_DeInit());
    TEST_ASSERT_EQUAL_UINT32(2U, mock_TcpIp_SocketClose_Count);
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_UNINIT, Mqtt_GetConnectionState(0U));
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_UNINIT, Mqtt_GetConnectionState(1U));
}

/*============================================================================
 * New: TLS connect path (real mbedTLS linked, minimal config)
 *============================================================================*/

#if (MQTT_SUPPORT_TLS == STD_ON)
void test_Mqtt_Connect_TlsWithoutConfig_ShouldReportError(void) {
    Mqtt_ConnectionConfigType cfg = testTlsConnConfig;
    Mqtt_ReturnType ret;
    cfg.tlsConfig = NULL_PTR;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Connect(0U, &cfg);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PARAM_CONFIG, mock_DetLastErrorId);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_TcpIp_SocketCreate_Count);
}

void test_Mqtt_Connect_Tls_MinimalRealMbedtls_ShouldFail(void) {
    Mqtt_ReturnType ret;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    ret = Mqtt_Connect(0U, &testTlsConnConfig);
    /* PRODUCT DEFECT (recorded, not fixed): Mqtt_Tls_CreateContext never calls
     * mbedtls_ssl_config_defaults, so mbedtls_ssl_setup fails with
     * MBEDTLS_ERR_SSL_BAD_CONFIG whenever the REAL mbedTLS 3.x is linked
     * (as in service_mqtt). Connect therefore reports CONNECTION_FAILED.
     * The same code succeeds against the stubbed mbedtls in mqtt_tls_test. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_CONNECTION_FAILED, ret);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_CONNECTION_FAILED, mock_DetLastErrorId);
    /* TCP socket is only created after the TLS context, so none was. */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_TcpIp_SocketCreate_Count);
    TEST_ASSERT_EQUAL_UINT8(MQTT_STATE_UNINIT, Mqtt_GetConnectionState(0U));
    (void)Mqtt_Disconnect(0U);
}

void test_Mqtt_Publish_OverTls_BeforeHandshake_ShouldFail(void) {
    static const uint8 payload[1] = { 1U };
    Mqtt_PublishMessageType msg = { "t", payload, 1U, MQTT_QOS_0, MQTT_RETAIN_FALSE };
    /* Plain connect succeeds, then white-box force the TLS send branch with an
     * out-of-range context handle (a real context can never be created with
     * the real mbedTLS linked, see the defect note above). Mqtt_Tls_Send
     * rejects it and Publish reports failure. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Connect(0U, &testConnConfig));
    Mqtt_Test_SetState(0U, (uint8)MQTT_STATE_CONNECTED);
    Mqtt_Test_SetUseTls(0U, 1U);
    Mqtt_Test_SetTlsContext(0U, (void*)(uintptr_t)0x1U);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_PUBLISH_FAILED, Mqtt_Publish(0U, &msg, NULL_PTR));
    Mqtt_Test_SetUseTls(0U, 0U);
    (void)Mqtt_Disconnect(0U);
}
#endif

/*============================================================================
 * New: Mqtt_Lcfg.c coverage (config table + default callbacks)
 *============================================================================*/

void test_Mqtt_Lcfg_ConfigTable_ShouldBeLinkedAndValid(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    /* NOTE (actual behaviour): Mqtt_Init stores the CALLER's config pointer,
     * not the Lcfg default table. */
    TEST_ASSERT_TRUE(Mqtt_ConfigPtr == &testConfig);
    TEST_ASSERT_EQUAL_UINT8(MQTT_MAX_CONNECTIONS, Mqtt_Config.maxConnections);
    TEST_ASSERT_EQUAL_UINT16(MQTT_SEND_BUFFER_SIZE, Mqtt_Config.sendBufferSize);
    TEST_ASSERT_EQUAL_UINT16(MQTT_RECV_BUFFER_SIZE, Mqtt_Config.recvBufferSize);
    TEST_ASSERT_EQUAL_UINT8(MQTT_MESSAGE_QUEUE_DEPTH, Mqtt_Config.messageQueueDepth);
    TEST_ASSERT_EQUAL_UINT16(1883U, Mqtt_DefaultConnectionConfig.brokerPort);
    TEST_ASSERT_TRUE(Mqtt_DefaultConnectionConfig.autoReconnect == TRUE);
}

void test_Mqtt_Lcfg_DefaultCallbacks_ShouldBeCallable(void) {
    static const uint8 payload[1] = { 0U };
    /* Weak default callbacks are empty; call them for line coverage. */
    Mqtt_DefaultConnectionCallback(0U, 1U);
    Mqtt_DefaultMessageCallback(0U, "t", payload, 1U);
    Mqtt_DefaultErrorCallback(0U, 2U);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

void test_Mqtt_DeInit_ShouldClearConfigPtr(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Init(&testConfig));
    TEST_ASSERT_TRUE(Mqtt_ConfigPtr != NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_DeInit());
    TEST_ASSERT_TRUE(Mqtt_ConfigPtr == NULL_PTR);
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

    RUN_TEST(test_Mqtt_Connect_InvalidConnectionId_ShouldReportError);
    RUN_TEST(test_Mqtt_Connect_NullBrokerHost_ShouldReportError);
    RUN_TEST(test_Mqtt_Connect_NullClientId_ShouldReportError);
    RUN_TEST(test_Mqtt_Connect_SocketCreateFailure_ShouldReturnConnFailed);
    RUN_TEST(test_Mqtt_Publish_TopicNull_ShouldReportError);
    RUN_TEST(test_Mqtt_Publish_InvalidConnectionId_ShouldReportError);
    RUN_TEST(test_Mqtt_Disconnect_InvalidConnectionId_ShouldReportError);
    RUN_TEST(test_Mqtt_Disconnect_AlreadyDisconnected_ShouldBeIdempotent);
    RUN_TEST(test_Mqtt_GetConnectionInfo_Errors);
    RUN_TEST(test_Mqtt_SetConnectionCallback_InvalidId_ShouldBeIgnored);
    RUN_TEST(test_Mqtt_Ping_Uninit_ShouldReturnUninit);
    RUN_TEST(test_Mqtt_Ping_InvalidId_ShouldReturnNotOk);
    RUN_TEST(test_Mqtt_Ping_NotConnected_ShouldReturnNoConn);

    RUN_TEST(test_Mqtt_MainFunction_ConnectingState_SendsConnectPacket);
    RUN_TEST(test_Mqtt_MainFunction_ConnectingState_ResendsEveryCycle);
    RUN_TEST(test_Mqtt_EncodeConnect_MultiByteRemainingLength);
    RUN_TEST(test_Mqtt_EncodeConnect_UsernamePasswordFlags);
    RUN_TEST(test_Mqtt_EncodeConnect_Version50_Level5);
    RUN_TEST(test_Mqtt_Connect_RegistersStateCallback);

    RUN_TEST(test_Mqtt_Publish_Qos0_WireFormat);
    RUN_TEST(test_Mqtt_Publish_Qos1_Retain_IncludesPacketId);
    RUN_TEST(test_Mqtt_Publish_Qos2_Header);
    RUN_TEST(test_Mqtt_Publish_PacketId_Increments);
    RUN_TEST(test_Mqtt_Publish_SendFailure_ShouldReturnPublishFailed);
    RUN_TEST(test_Mqtt_Publish_TooLarge_ShouldReturnBufferOverflow);
    RUN_TEST(test_Mqtt_Subscribe_WireFormat);
    RUN_TEST(test_Mqtt_Subscribe_SendFailure_ShouldRollBackSlot);
    RUN_TEST(test_Mqtt_Subscribe_TooLarge_ShouldReturnBufferOverflow);
    RUN_TEST(test_Mqtt_Unsubscribe_Connected_ShouldRemoveSubscription);
    RUN_TEST(test_Mqtt_Unsubscribe_Connected_UnknownTopic_ShouldReturnNotOk);
    RUN_TEST(test_Mqtt_Ping_Connected_ShouldSendPingReq);
    RUN_TEST(test_Mqtt_Ping_PendingPing_ShouldReturnBusy);
    RUN_TEST(test_Mqtt_MainFunction_Connected_ReceivesInjectedData);
    RUN_TEST(test_Mqtt_MainFunction_Connected_ReceiveFailure_ShouldNotCountBytes);
    RUN_TEST(test_Mqtt_Disconnect_Connected_ShouldSendDisconnectPacket);
    RUN_TEST(test_Mqtt_Disconnect_Connected_CountsDisconnect);
    RUN_TEST(test_Mqtt_Disconnect_Connected_FiresCallbackOnTransition);
    RUN_TEST(test_Mqtt_DeInit_ConnectedConnections_ShouldCloseAllSockets);

#if (MQTT_SUPPORT_TLS == STD_ON)
    RUN_TEST(test_Mqtt_Connect_TlsWithoutConfig_ShouldReportError);
    RUN_TEST(test_Mqtt_Connect_Tls_MinimalRealMbedtls_ShouldFail);
    RUN_TEST(test_Mqtt_Publish_OverTls_BeforeHandshake_ShouldFail);
#endif

    RUN_TEST(test_Mqtt_Lcfg_ConfigTable_ShouldBeLinkedAndValid);
    RUN_TEST(test_Mqtt_Lcfg_DefaultCallbacks_ShouldBeCallable);
    RUN_TEST(test_Mqtt_DeInit_ShouldClearConfigPtr);
    return UnityEnd();
}
