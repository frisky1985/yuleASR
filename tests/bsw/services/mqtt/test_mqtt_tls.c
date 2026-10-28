/**
 * @file test_mqtt_tls.c
 * @brief Mqtt_Tls Unit Tests
 *
 * Mqtt_Tls.c is compiled directly against the REAL mbedtls headers (true
 * struct layouts, constants and the inline mbedtls_ssl_get_version_number),
 * while every mbedtls function the module calls is replaced by the
 * configurable stubs in mbedtls_stubs.c. This drives both success and failure
 * paths deterministically without a live TLS peer.
 *
 * NOTE: a real TLS handshake cannot succeed without an actual peer; the
 * success path here is exercised through the handshake stub, which is
 * equivalent for coverage of the wrapper logic (the BIO callbacks are driven
 * by the stub handshake against the TcpIp stubs).
 */

// @tests src/bsw/services/mqtt/src/Mqtt_Tls.c  @tests src/bsw/services/mqtt/include/Mqtt_Tls.h

#include "unity.h"
#include "Mqtt_Tls.h"
#include "mbedtls/ssl.h"
#include <string.h>
#include <stdio.h>

/* ---- mbedtls stub knobs (defined in mbedtls_stubs.c) ---- */
extern int stub_x509_parse_mode;
extern int stub_cert_valid_from_year;
extern int stub_cert_valid_to_year;
extern int stub_x509_verify_ret;
extern unsigned int stub_x509_verify_flags;
extern int stub_ssl_setup_ret;
extern int stub_pk_parse_key_ret;
extern int stub_ssl_conf_own_cert_ret;
extern int stub_ssl_set_hostname_ret;
extern int stub_ssl_handshake_ret;
extern int stub_ssl_bio_trigger;
extern int stub_ssl_write_ret;
extern int stub_ssl_read_ret;
extern unsigned int stub_ssl_verify_flags;
extern const mbedtls_x509_crt* stub_ssl_peer_cert;
extern const char* stub_ssl_ciphersuite;
extern int stub_mem_init_ret;
extern int stub_strerror_empty;
extern int stub_ca_chain_calls;
extern int stub_own_cert_calls;
extern int stub_set_hostname_calls;
extern int stub_handshake_calls;
extern int stub_get_session_calls;
extern int stub_close_notify_calls;
extern int stub_parse_calls;
extern int stub_parse_der_calls;
extern int stub_ssl_write_calls;
extern int stub_ssl_read_calls;
extern int stub_bio_send_calls;
extern int stub_bio_recv_calls;
extern int stub_bio_recv_timeout_calls;
extern int stub_last_authmode;
extern mbedtls_ssl_context* stub_last_ssl;
extern void stub_peer_cert_setup(void);
extern void stub_peer_cert_clear(void);
extern void stub_all_reset(void);

/* ---- TcpIp stubs (defined in stubs.c; used by the BIO callbacks) ---- */
extern void mock_TcpIp_Reset(void);
extern int mock_TcpIp_Send_Fail;
extern int mock_TcpIp_Receive_Fail;
extern unsigned int mock_TcpIp_Send_Count;

static const uint8 fakeCaPem[] = "FAKE-CA-CERTIFICATE-PEM";
static const uint8 fakeCaDer[] = { 'F', 'A', 'K', 'E', 0x01U, 0x02U, 0x03U };
static const uint8 fakeClientPem[] = "FAKE-CLIENT-CERTIFICATE-PEM";
static const uint8 fakeKeyPem[] = "FAKE-PRIVATE-KEY-PEM";

static void resetAll(void) {
    stub_all_reset();
    mock_TcpIp_Reset();
}

void setUp(void) {
    resetAll();
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_Init());
}

void tearDown(void) {
    Mqtt_Tls_DeInit();
    resetAll();
}

/*============================================================================
 * Init / DeInit
 *============================================================================*/

void test_Tls_Init_ShouldSucceedAndBeIdempotent(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_Init());
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_Init());
}

void test_Tls_Init_MemInitFailure_ShouldFail(void) {
    Mqtt_Tls_DeInit();
    stub_mem_init_ret = 1;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_Init());
}

void test_Tls_DeInit_WithoutInit_ShouldBeHarmless(void) {
    Mqtt_Tls_DeInit();
    Mqtt_Tls_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_TcpIp_Send_Count);
}

/*============================================================================
 * CreateContext / DestroyContext
 *============================================================================*/

static Mqtt_TlsConfigType minimalConfig(void) {
    Mqtt_TlsConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.version = MQTT_TLS_VERSION_1_2;
    cfg.verifyMode = MQTT_TLS_VERIFY_NONE;
    return cfg;
}

/* Addressable holder so tests can pass a minimal config by pointer. */
static Mqtt_TlsConfigType g_minCfg;
static Mqtt_TlsConfigType* minCfgPtr(void) {
    g_minCfg = minimalConfig();
    return &g_minCfg;
}

void test_Tls_CreateContext_WithoutInit_ShouldFail(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_Tls_DeInit();
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), &ctx));
}

void test_Tls_CreateContext_NullArguments_ShouldFail(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_CreateContext(NULL_PTR, &ctx));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), NULL_PTR));
    TEST_ASSERT_TRUE(ctx == NULL_PTR);
}

void test_Tls_CreateContext_Minimal_ShouldSucceed(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), &ctx));
    TEST_ASSERT_TRUE(ctx != NULL_PTR);
}

void test_Tls_CreateContext_VerifyModes_ShouldConfigureAuthmode(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    cfg.verifyMode = MQTT_TLS_VERIFY_OPTIONAL;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    TEST_ASSERT_EQUAL_INT(MBEDTLS_SSL_VERIFY_OPTIONAL, stub_last_authmode);
    Mqtt_Tls_DestroyContext(ctx);

    stub_all_reset();
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_Init());
    cfg.verifyMode = MQTT_TLS_VERIFY_REQUIRED;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    TEST_ASSERT_EQUAL_INT(MBEDTLS_SSL_VERIFY_REQUIRED, stub_last_authmode);
    Mqtt_Tls_DestroyContext(ctx);
}

void test_Tls_CreateContext_WithCaPem_ShouldConfigureCaChain(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    cfg.trustStore.caCert = fakeCaPem;
    cfg.trustStore.caCertLength = sizeof(fakeCaPem);
    cfg.trustStore.caFormat = MQTT_CERT_FORMAT_PEM;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    TEST_ASSERT_EQUAL_INT(1, stub_parse_calls);
    TEST_ASSERT_EQUAL_INT(1, stub_ca_chain_calls);
    Mqtt_Tls_DestroyContext(ctx);
}

void test_Tls_CreateContext_WithCaDer_ShouldUseDerParser(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    cfg.trustStore.caCert = fakeCaDer;
    cfg.trustStore.caCertLength = sizeof(fakeCaDer);
    cfg.trustStore.caFormat = MQTT_CERT_FORMAT_DER;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    TEST_ASSERT_EQUAL_INT(1, stub_parse_der_calls);
    Mqtt_Tls_DestroyContext(ctx);
}

void test_Tls_CreateContext_CaParseFailure_ShouldFailAndReuseSlot(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    cfg.trustStore.caCert = fakeCaPem;
    cfg.trustStore.caCertLength = sizeof(fakeCaPem);
    stub_x509_parse_mode = 2;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    TEST_ASSERT_TRUE(ctx == NULL_PTR);
    /* Failed creation must not leak the slot. */
    stub_x509_parse_mode = 0;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    TEST_ASSERT_TRUE(ctx != NULL_PTR);
    Mqtt_Tls_DestroyContext(ctx);
}

void test_Tls_CreateContext_mTls_ShouldLoadCertAndKey(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    Mqtt_CertificateType clientCert;
    Mqtt_PrivateKeyType clientKey;
    clientCert.data = fakeClientPem;
    clientCert.length = sizeof(fakeClientPem);
    clientCert.format = MQTT_CERT_FORMAT_PEM;
    clientCert.cache = MQTT_CERT_CACHE_NONE;
    clientKey.data = fakeKeyPem;
    clientKey.length = sizeof(fakeKeyPem);
    clientKey.type = MQTT_KEY_TYPE_ECC;
    clientKey.format = MQTT_CERT_FORMAT_PEM;
    clientKey.password = "secret";
    cfg.trustStore.caCert = fakeCaPem;
    cfg.trustStore.caCertLength = sizeof(fakeCaPem);
    cfg.clientCert = &clientCert;
    cfg.clientKey = &clientKey;
    cfg.expectedHostname = "broker.example.com";
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    TEST_ASSERT_EQUAL_INT(1, stub_own_cert_calls);
    TEST_ASSERT_EQUAL_INT(1, stub_set_hostname_calls);
    Mqtt_Tls_DestroyContext(ctx);
}

void test_Tls_CreateContext_KeyParseFailure_ShouldFail(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    Mqtt_CertificateType clientCert;
    Mqtt_PrivateKeyType clientKey;
    clientCert.data = fakeClientPem;
    clientCert.length = sizeof(fakeClientPem);
    clientCert.format = MQTT_CERT_FORMAT_PEM;
    clientCert.cache = MQTT_CERT_CACHE_NONE;
    clientKey.data = fakeKeyPem;
    clientKey.length = sizeof(fakeKeyPem);
    clientKey.type = MQTT_KEY_TYPE_RSA;
    clientKey.format = MQTT_CERT_FORMAT_PEM;
    clientKey.password = NULL_PTR;
    cfg.clientCert = &clientCert;
    cfg.clientKey = &clientKey;
    stub_pk_parse_key_ret = -1;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    TEST_ASSERT_TRUE(ctx == NULL_PTR);
}

void test_Tls_CreateContext_OwnCertFailure_ShouldFail(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    Mqtt_CertificateType clientCert;
    Mqtt_PrivateKeyType clientKey;
    clientCert.data = fakeClientPem;
    clientCert.length = sizeof(fakeClientPem);
    clientCert.format = MQTT_CERT_FORMAT_PEM;
    clientCert.cache = MQTT_CERT_CACHE_NONE;
    clientKey.data = fakeKeyPem;
    clientKey.length = sizeof(fakeKeyPem);
    clientKey.type = MQTT_KEY_TYPE_RSA;
    clientKey.format = MQTT_CERT_FORMAT_PEM;
    clientKey.password = NULL_PTR;
    cfg.clientCert = &clientCert;
    cfg.clientKey = &clientKey;
    stub_ssl_conf_own_cert_ret = -1;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
}

void test_Tls_CreateContext_HostnameFailure_ShouldFail(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    cfg.expectedHostname = "broker.example.com";
    stub_ssl_set_hostname_ret = -1;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
}

void test_Tls_CreateContext_SetupFailure_ShouldFail(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    stub_ssl_setup_ret = -1;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), &ctx));
    TEST_ASSERT_TRUE(ctx == NULL_PTR);
}

void test_Tls_CreateContext_SlotExhaustion_ShouldReportBusy(void) {
    Mqtt_TlsContextType ctx[5];
    int i;
    for (i = 0; i < 4; i++) {
        ctx[i] = NULL_PTR;
        TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), &ctx[i]));
        TEST_ASSERT_TRUE(ctx[i] != NULL_PTR);
    }
    ctx[4] = NULL_PTR;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), &ctx[4]));
    /* Free one slot -> creation works again. */
    Mqtt_Tls_DestroyContext(ctx[2]);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), &ctx[4]));
    Mqtt_Tls_DestroyContext(ctx[4]);
}

void test_Tls_DestroyContext_ShouldReuseFreedSlot(void) {
    Mqtt_TlsContextType ctx1 = NULL_PTR;
    Mqtt_TlsContextType ctx2 = NULL_PTR;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), &ctx1));
    Mqtt_Tls_DestroyContext(ctx1);
    Mqtt_Tls_DestroyContext(NULL_PTR); /* invalid handle: must be harmless */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), &ctx2));
    TEST_ASSERT_EQUAL_PTR(ctx1, ctx2);
    Mqtt_Tls_DestroyContext(ctx2);
}

/*============================================================================
 * PerformHandshake
 *============================================================================*/

static int rec_hs_calls;
static int rec_hs_success;
static int rec_hs_validation_isValid; /* -1: callback got NULL */

static void handshakeCb(Mqtt_ConnectionIdType id, boolean success,
                        const Mqtt_CertValidationResultType* result) {
    (void)id;
    rec_hs_calls++;
    rec_hs_success = (success != FALSE) ? 1 : 0;
    if (result != NULL_PTR) {
        rec_hs_validation_isValid = (result->isValid != FALSE) ? 1 : 0;
    } else {
        rec_hs_validation_isValid = -1;
    }
}

static Mqtt_TlsContextType createdCtx(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), &ctx));
    return ctx;
}

void test_Tls_PerformHandshake_InvalidParams_ShouldFail(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_PerformHandshake(NULL_PTR, 7U, &handshakeCb));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK,
        Mqtt_Tls_PerformHandshake(createdCtx(), TCPIP_SOCKETID_INVALID, &handshakeCb));
}

void test_Tls_PerformHandshake_Success_ShouldNotifyCallback(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    rec_hs_calls = 0;
    stub_ssl_handshake_ret = 0;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_PerformHandshake(ctx, 7U, &handshakeCb));
    TEST_ASSERT_EQUAL_INT(1, stub_handshake_calls);
    TEST_ASSERT_EQUAL_INT(1, rec_hs_calls);
    TEST_ASSERT_EQUAL_INT(1, rec_hs_success);
    TEST_ASSERT_EQUAL_INT(1, rec_hs_validation_isValid);
}

void test_Tls_PerformHandshake_Success_WithSessionResumption(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    cfg.enableSessionResumption = TRUE;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    stub_ssl_handshake_ret = 0;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_PerformHandshake(ctx, 7U, NULL_PTR));
    TEST_ASSERT_EQUAL_INT(1, stub_get_session_calls);
    Mqtt_Tls_DestroyContext(ctx);
}

void test_Tls_PerformHandshake_Failure_ShouldNotifyAndRecordError(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    rec_hs_calls = 0;
    stub_ssl_handshake_ret = -1;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_PerformHandshake(ctx, 7U, &handshakeCb));
    TEST_ASSERT_EQUAL_INT(1, rec_hs_calls);
    TEST_ASSERT_EQUAL_INT(0, rec_hs_success);
    TEST_ASSERT_EQUAL_INT(-1, Mqtt_Tls_GetLastError(ctx));
}

void test_Tls_PerformHandshake_ShouldDriveBioCallbacks(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    stub_ssl_handshake_ret = -1;
    (void)Mqtt_Tls_PerformHandshake(ctx, 7U, NULL_PTR);
    /* The stub handshake invokes the registered BIO functions once each. */
    TEST_ASSERT_EQUAL_INT(1, stub_bio_recv_calls);
    TEST_ASSERT_EQUAL_INT(1, stub_bio_recv_timeout_calls);
    TEST_ASSERT_EQUAL_INT(1, stub_bio_send_calls);
}

/*============================================================================
 * Send / Receive / Close
 *============================================================================*/

void test_Tls_Send_ParameterChecks_ShouldFail(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    uint8 buf[4] = { 1U, 2U, 3U, 4U };
    uint32 sent = 0U;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_Send(NULL_PTR, buf, 4U, &sent));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_Send(ctx, NULL_PTR, 4U, &sent));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_Send(ctx, buf, 0U, &sent));
    /* Handshake not performed yet -> production rejects the send. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_Send(ctx, buf, 4U, &sent));
    TEST_ASSERT_EQUAL_UINT32(0U, sent);
}

void test_Tls_Send_AfterHandshake_ShouldSucceed(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    static const uint8 data[10] = { 1U };
    uint32 sent = 0U;
    stub_ssl_handshake_ret = 0;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_PerformHandshake(ctx, 7U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_Send(ctx, data, 10U, &sent));
    TEST_ASSERT_EQUAL_UINT32(10U, sent);
    TEST_ASSERT_EQUAL_INT(1, stub_ssl_write_calls);
}

void test_Tls_Send_WriteFailure_ShouldFail(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    uint8 data[4] = { 1U };
    uint32 sent = 0U;
    stub_ssl_handshake_ret = 0;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_PerformHandshake(ctx, 7U, NULL_PTR));
    stub_ssl_write_ret = -1;
    /* PRODUCT DEFECT (recorded, not fixed): Mqtt_Tls_Send evaluates
     * "if (ret > 0U)" with an int ret, so a negative mbedtls_ssl_write error
     * (-1) is converted to unsigned 0xFFFFFFFF and treated as success: the
     * wrapper returns MQTT_OK and reports a bogus sentLength. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_Send(ctx, data, 4U, &sent));
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFU, sent);
}

void test_Tls_Receive_ParameterChecks_ShouldFail(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    uint8 buf[16];
    uint32 received = 0U;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_Receive(NULL_PTR, buf, 16U, &received));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_Receive(ctx, NULL_PTR, 16U, &received));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_Receive(ctx, buf, 0U, &received));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_Receive(ctx, buf, 16U, NULL_PTR));
}

void test_Tls_Receive_AfterHandshake_ShouldSucceed(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    uint8 buf[16];
    uint32 received = 0U;
    stub_ssl_handshake_ret = 0;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_PerformHandshake(ctx, 7U, NULL_PTR));
    stub_ssl_read_ret = 5;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_Receive(ctx, buf, 16U, &received));
    TEST_ASSERT_EQUAL_UINT32(5U, received);
    TEST_ASSERT_EQUAL_INT(1, stub_ssl_read_calls);
}

void test_Tls_Receive_ReadFailure_ShouldFail(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    uint8 buf[16];
    uint32 received = 0xFFFFFFFFU;
    stub_ssl_handshake_ret = 0;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_PerformHandshake(ctx, 7U, NULL_PTR));
    stub_ssl_read_ret = -1;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_Receive(ctx, buf, 16U, &received));
    /* NOTE (actual behaviour): on a read failure the wrapper leaves
     * *receivedLength untouched instead of clearing it to zero. */
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFU, received);
}

void test_Tls_Close_WithoutHandshake_ShouldNotSendNotify(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_Close(ctx));
    TEST_ASSERT_EQUAL_INT(0, stub_close_notify_calls);
}

void test_Tls_Close_AfterHandshake_ShouldSendCloseNotifyOnce(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    stub_ssl_handshake_ret = 0;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_PerformHandshake(ctx, 7U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_Close(ctx));
    TEST_ASSERT_EQUAL_INT(1, stub_close_notify_calls);
    /* Closing twice must not double-send. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_Close(ctx));
    TEST_ASSERT_EQUAL_INT(1, stub_close_notify_calls);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_Close(NULL_PTR));
}

/*============================================================================
 * VerifyCertificate / GetPeerCertificateInfo
 *============================================================================*/

void test_Tls_VerifyCertificate_ParameterChecks_ShouldFail(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    Mqtt_CertValidationResultType result;
    memset(&result, 0, sizeof(result));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_VerifyCertificate(ctx, NULL_PTR, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK,
        Mqtt_Tls_VerifyCertificate(NULL_PTR, NULL_PTR, &result));
}

void test_Tls_VerifyCertificate_ValidPeer_ShouldSucceed(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    Mqtt_CertValidationResultType result;
    stub_peer_cert_setup();
    stub_x509_parse_mode = 1; /* template cert parses on demand */
    cfg.trustStore.caCert = fakeCaPem;
    cfg.trustStore.caCertLength = sizeof(fakeCaPem);
    cfg.trustStore.caFormat = MQTT_CERT_FORMAT_PEM;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    memset(&result, 0xAA, sizeof(result));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_VerifyCertificate(ctx, NULL_PTR, &result));
    TEST_ASSERT_EQUAL_UINT8(1U, (uint8)result.isValid);
    TEST_ASSERT_EQUAL_UINT8(1U, (uint8)result.isTrusted);
    TEST_ASSERT_EQUAL_UINT8(1U, (uint8)result.notExpired);
    /* NOTE (actual behaviour): the wrapper copies subject/issuer from the
     * SECOND chain node (subject.next->val / issuer.next->val), not the head:
     * subject = "YuleTech" (O), issuer = "Test Issuer CA". */
    TEST_ASSERT_EQUAL_STRING("YuleTech", result.subject);
    TEST_ASSERT_EQUAL_STRING("Test Issuer CA", result.issuer);
}

void test_Tls_VerifyCertificate_VerifyFlags_ShouldReportUntrusted(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    Mqtt_CertValidationResultType result;
    stub_peer_cert_setup();
    stub_x509_parse_mode = 1;
    cfg.trustStore.caCert = fakeCaPem;
    cfg.trustStore.caCertLength = sizeof(fakeCaPem);
    cfg.trustStore.caFormat = MQTT_CERT_FORMAT_PEM;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    stub_ssl_verify_flags = MBEDTLS_X509_BADCERT_NOT_TRUSTED;
    memset(&result, 0, sizeof(result));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_VerifyCertificate(ctx, NULL_PTR, &result));
    TEST_ASSERT_EQUAL_UINT8(0U, (uint8)result.isValid);
    TEST_ASSERT_EQUAL_UINT8(0U, (uint8)result.isTrusted);
}

void test_Tls_VerifyCertificate_NoPeerCert_ShouldReportInvalid(void) {
    /* NOTE (actual behaviour): with no peer certificate present the wrapper
     * skips the cert copy block entirely and leaves isValid/isTrusted driven
     * only by the verify-result flags (0 here) -> both stay TRUE. */
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_CertValidationResultType result;
    stub_peer_cert_clear();
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), &ctx));
    memset(&result, 0, sizeof(result));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_VerifyCertificate(ctx, NULL_PTR, &result));
    TEST_ASSERT_EQUAL_UINT8(1U, (uint8)result.isValid);
    TEST_ASSERT_EQUAL_UINT8(1U, (uint8)result.isTrusted);
    /* notExpired is only set when a peer certificate exists. */
    TEST_ASSERT_EQUAL_UINT8(0U, (uint8)result.notExpired);
}

void test_Tls_GetPeerCertificateInfo_ShouldExtractSubjectFields(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_CertValidationResultType result;
    stub_peer_cert_setup();
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(minCfgPtr(), &ctx));
    memset(&result, 0, sizeof(result));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_GetPeerCertificateInfo(ctx, &result));
    TEST_ASSERT_EQUAL_STRING("YuleTech", result.subject); /* second chain node (O) */
    TEST_ASSERT_EQUAL_STRING("Test Issuer CA", result.issuer);
    /* NOTE (actual behaviour): GetPeerCertificateInfo delegates to
     * Mqtt_Tls_VerifyCertificate which never fills serialNumber. */
    TEST_ASSERT_EQUAL_UINT8(0U, result.serialNumber[0]);
}

void test_Tls_GetPeerCertificateInfo_ParameterChecks_ShouldFail(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    Mqtt_CertValidationResultType result;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_GetPeerCertificateInfo(ctx, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_GetPeerCertificateInfo(NULL_PTR, &result));
}

/*============================================================================
 * Session save / restore
 *============================================================================*/

void test_Tls_SaveSession_ShouldCopySessionData(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    uint8 data[512];
    uint32 actual = 0U;
    cfg.enableSessionResumption = TRUE;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_SaveSession(ctx, data, sizeof(data), &actual));
    TEST_ASSERT_EQUAL_UINT32((uint32)sizeof(mbedtls_ssl_session), actual);
}

void test_Tls_SaveSession_ParameterChecks_ShouldFail(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    uint8 data[512];
    uint32 actual = 0U;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_SaveSession(NULL_PTR, data, sizeof(data), &actual));
    /* NOTE (actual behaviour): a NULL sessionData is also tolerated (MQTT_OK) --
     * actualSize is still written, only the memcpy is skipped. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_SaveSession(ctx, NULL_PTR, sizeof(data), &actual));
    TEST_ASSERT_EQUAL_UINT32((uint32)sizeof(mbedtls_ssl_session), actual);
    /* NOTE (actual behaviour): a NULL actualSize is tolerated (MQTT_OK);
     * only the sessionData copy is skipped. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_SaveSession(ctx, data, sizeof(data), NULL_PTR));
}

void test_Tls_RestoreSession_SizeChecks(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    uint8 data[512];
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_RestoreSession(ctx, NULL_PTR, 512U));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_RestoreSession(ctx, data, 1U));
    memset(data, 0, sizeof(data));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK,
        Mqtt_Tls_RestoreSession(ctx, data, (uint32)sizeof(mbedtls_ssl_session)));
}

/*============================================================================
 * Error handling / info queries
 *============================================================================*/

void test_Tls_GetLastError_InvalidContext_ShouldReturnInternalError(void) {
    TEST_ASSERT_EQUAL_INT(MBEDTLS_ERR_SSL_INTERNAL_ERROR, Mqtt_Tls_GetLastError(NULL_PTR));
}

void test_Tls_GetErrorString_ShouldFormatKnownAndUnknownErrors(void) {
    char buf[128];
    stub_strerror_empty = 0;
    memset(buf, 0, sizeof(buf));
    /* Production copies via mbedtls_strerror into a static buffer. */
    TEST_ASSERT_NOT_NULL(Mqtt_Tls_GetErrorString(-1));
    stub_strerror_empty = 1;
    TEST_ASSERT_EQUAL_STRING("Unknown error", Mqtt_Tls_GetErrorString(-1));
    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("Unknown error", Mqtt_Tls_GetErrorString(0));
    (void)buf;
}

void test_Tls_SetErrorCallback_InvalidContext_ShouldBeIgnored(void) {
    Mqtt_Tls_SetErrorCallback(NULL_PTR, NULL_PTR);
    Mqtt_Tls_SetErrorCallback(createdCtx(), NULL_PTR);
}

void test_Tls_GetVersion_ShouldReportConfiguredAndNegotiatedVersion(void) {
    Mqtt_TlsContextType ctx = NULL_PTR;
    Mqtt_TlsConfigType cfg = minimalConfig();
    TEST_ASSERT_EQUAL_UINT8(MQTT_TLS_VERSION_1_2, Mqtt_Tls_GetVersion(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_CreateContext(&cfg, &ctx));
    /* No negotiated version yet (context zeroed) -> config version. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_TLS_VERSION_1_2, Mqtt_Tls_GetVersion(ctx));
    /* Simulate negotiated TLS 1.3 through the stub-recorded ssl context. */
    stub_last_ssl->tls_version = MBEDTLS_SSL_VERSION_TLS1_3;
    TEST_ASSERT_EQUAL_UINT8(MQTT_TLS_VERSION_1_3, Mqtt_Tls_GetVersion(ctx));
    stub_last_ssl->tls_version = MBEDTLS_SSL_VERSION_TLS1_2;
    TEST_ASSERT_EQUAL_UINT8(MQTT_TLS_VERSION_1_2, Mqtt_Tls_GetVersion(ctx));
}

void test_Tls_GetCipherSuite_ShouldCopySuiteName(void) {
    Mqtt_TlsContextType ctx = createdCtx();
    char name[64];
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_GetCipherSuite(NULL_PTR, name, sizeof(name)));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_GetCipherSuite(ctx, NULL_PTR, sizeof(name)));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_Tls_GetCipherSuite(ctx, name, 0U));
    memset(name, 0, sizeof(name));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_GetCipherSuite(ctx, name, sizeof(name)));
    TEST_ASSERT_EQUAL_STRING("TLS-STUB-SUITE", name);
    /* NULL ciphersuite -> empty string result. */
    stub_ssl_ciphersuite = NULL_PTR;
    memset(name, 'X', sizeof(name));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_Tls_GetCipherSuite(ctx, name, sizeof(name)));
    TEST_ASSERT_EQUAL_UINT8(0U, name[0]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Tls_Init_ShouldSucceedAndBeIdempotent);
    RUN_TEST(test_Tls_Init_MemInitFailure_ShouldFail);
    RUN_TEST(test_Tls_DeInit_WithoutInit_ShouldBeHarmless);

    RUN_TEST(test_Tls_CreateContext_WithoutInit_ShouldFail);
    RUN_TEST(test_Tls_CreateContext_NullArguments_ShouldFail);
    RUN_TEST(test_Tls_CreateContext_Minimal_ShouldSucceed);
    RUN_TEST(test_Tls_CreateContext_VerifyModes_ShouldConfigureAuthmode);
    RUN_TEST(test_Tls_CreateContext_WithCaPem_ShouldConfigureCaChain);
    RUN_TEST(test_Tls_CreateContext_WithCaDer_ShouldUseDerParser);
    RUN_TEST(test_Tls_CreateContext_CaParseFailure_ShouldFailAndReuseSlot);
    RUN_TEST(test_Tls_CreateContext_mTls_ShouldLoadCertAndKey);
    RUN_TEST(test_Tls_CreateContext_KeyParseFailure_ShouldFail);
    RUN_TEST(test_Tls_CreateContext_OwnCertFailure_ShouldFail);
    RUN_TEST(test_Tls_CreateContext_HostnameFailure_ShouldFail);
    RUN_TEST(test_Tls_CreateContext_SetupFailure_ShouldFail);
    RUN_TEST(test_Tls_CreateContext_SlotExhaustion_ShouldReportBusy);
    RUN_TEST(test_Tls_DestroyContext_ShouldReuseFreedSlot);

    RUN_TEST(test_Tls_PerformHandshake_InvalidParams_ShouldFail);
    RUN_TEST(test_Tls_PerformHandshake_Success_ShouldNotifyCallback);
    RUN_TEST(test_Tls_PerformHandshake_Success_WithSessionResumption);
    RUN_TEST(test_Tls_PerformHandshake_Failure_ShouldNotifyAndRecordError);
    RUN_TEST(test_Tls_PerformHandshake_ShouldDriveBioCallbacks);

    RUN_TEST(test_Tls_Send_ParameterChecks_ShouldFail);
    RUN_TEST(test_Tls_Send_AfterHandshake_ShouldSucceed);
    RUN_TEST(test_Tls_Send_WriteFailure_ShouldFail);
    RUN_TEST(test_Tls_Receive_ParameterChecks_ShouldFail);
    RUN_TEST(test_Tls_Receive_AfterHandshake_ShouldSucceed);
    RUN_TEST(test_Tls_Receive_ReadFailure_ShouldFail);
    RUN_TEST(test_Tls_Close_WithoutHandshake_ShouldNotSendNotify);
    RUN_TEST(test_Tls_Close_AfterHandshake_ShouldSendCloseNotifyOnce);

    RUN_TEST(test_Tls_VerifyCertificate_ParameterChecks_ShouldFail);
    RUN_TEST(test_Tls_VerifyCertificate_ValidPeer_ShouldSucceed);
    RUN_TEST(test_Tls_VerifyCertificate_VerifyFlags_ShouldReportUntrusted);
    RUN_TEST(test_Tls_VerifyCertificate_NoPeerCert_ShouldReportInvalid);
    RUN_TEST(test_Tls_GetPeerCertificateInfo_ShouldExtractSubjectFields);
    RUN_TEST(test_Tls_GetPeerCertificateInfo_ParameterChecks_ShouldFail);

    RUN_TEST(test_Tls_SaveSession_ShouldCopySessionData);
    RUN_TEST(test_Tls_SaveSession_ParameterChecks_ShouldFail);
    RUN_TEST(test_Tls_RestoreSession_SizeChecks);

    RUN_TEST(test_Tls_GetLastError_InvalidContext_ShouldReturnInternalError);
    RUN_TEST(test_Tls_GetErrorString_ShouldFormatKnownAndUnknownErrors);
    RUN_TEST(test_Tls_SetErrorCallback_InvalidContext_ShouldBeIgnored);
    RUN_TEST(test_Tls_GetVersion_ShouldReportConfiguredAndNegotiatedVersion);
    RUN_TEST(test_Tls_GetCipherSuite_ShouldCopySuiteName);
    return UnityEnd();
}
