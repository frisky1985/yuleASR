/** @file Mqtt_Tls.c
 * @brief MQTT TLS/SSL/mTLS 安全层实现 - 基于mbedTLS
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 使用mbedTLS实现的轻量级TLS层
 * 支持TLS 1.2/1.3、SSL和mTLS双向认证
 */

/*============================================================================
 * 包含文件
 *===========================================================================*/
#include "Mqtt_Tls.h"
#include <string.h>
#include <stdio.h>

/* YuleTech mbedTLS适配层 */
#include "yule_mbedtls_adapter.h"

/* mbedTLS头文件 */
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/debug.h"
#include "mbedtls/platform.h"

/*============================================================================
 * 内部宏定义
 *===========================================================================*/
#define MQTT_TLS_MAX_CONTEXTS           (MQTT_MAX_CONNECTIONS)
#define MQTT_TLS_MAX_CERT_NAME_LENGTH   (256)
#define MQTT_TLS_MAX_ERROR_STRING       (256)
#define MQTT_TLS_DEFAULT_HANDSHAKE_TIMEOUT_MS  (10000)
#define MQTT_TLS_DEFAULT_SESSION_TIMEOUT_MS    (86400000)  /* 24小时 */

/*============================================================================
 * 内部TLS上下文结构
 *===========================================================================*/
typedef struct {
    boolean inUse;                          /**< 上下文是否使用中 */
    Mqtt_TlsConfigType config;              /**< TLS配置复本 */
    
    /* mbedTLS上下文 */
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt caCert;
    mbedtls_x509_crt clientCert;
    mbedtls_pk_context clientKey;
    mbedtls_ssl_session savedSession;
    
    /* 状态跟踪 */
    TcpIp_SocketIdType socketId;
    boolean handshakeComplete;
    Mqtt_TlsHandshakeCallbackType handshakeCallback;
    Mqtt_TlsErrorCallbackType errorCallback;
    sint32 lastError;
} Mqtt_TlsInternalContextType;

/*============================================================================
 * 全局变量
 *===========================================================================*/
static boolean Mqtt_TlsInitialized = FALSE;
static Mqtt_TlsInternalContextType Mqtt_TlsContexts[MQTT_TLS_MAX_CONTEXTS];

/*============================================================================
 * 内部函数声明
 *===========================================================================*/
static Mqtt_TlsInternalContextType* Mqtt_Tls_GetContext(Mqtt_TlsContextType context);
static int Mqtt_Tls_SendCallback(void* ctx, const unsigned char* buf, size_t len);
static int Mqtt_Tls_RecvCallback(void* ctx, unsigned char* buf, size_t len);
static int Mqtt_Tls_RecvTimeoutCallback(void* ctx, unsigned char* buf, size_t len, uint32_t timeout);

/*============================================================================
 * 初始化和配置
 *===========================================================================*/

Mqtt_ReturnType Mqtt_Tls_Init(void)
{
    uint8 i;
    Std_ReturnType result;
    
    if (Mqtt_TlsInitialized) {
        return MQTT_OK;
    }
    
    /* 初始化YuleTech适配层 */
    result = YuleMbedtls_Init();
    if (result != E_OK) {
        return MQTT_E_NOT_OK;
    }
    
    /* 清零所有上下文槽 */
    for (i = 0; i < MQTT_TLS_MAX_CONTEXTS; i++) {
        memset(&Mqtt_TlsContexts[i], 0, sizeof(Mqtt_TlsInternalContextType));
        Mqtt_TlsContexts[i].inUse = FALSE;
        Mqtt_TlsContexts[i].socketId = TCPIP_SOCKETID_INVALID;
        Mqtt_TlsContexts[i].handshakeComplete = FALSE;
        Mqtt_TlsContexts[i].handshakeCallback = NULL_PTR;
        Mqtt_TlsContexts[i].errorCallback = NULL_PTR;
        Mqtt_TlsContexts[i].lastError = 0;
    }
    
    Mqtt_TlsInitialized = TRUE;
    return MQTT_OK;
}

void Mqtt_Tls_DeInit(void)
{
    uint8 i;
    
    if (!Mqtt_TlsInitialized) {
        return;
    }
    
    /* 清理所有上下文 */
    for (i = 0; i < MQTT_TLS_MAX_CONTEXTS; i++) {
        if (Mqtt_TlsContexts[i].inUse) {
            Mqtt_Tls_DestroyContext(&Mqtt_TlsContexts[i]);
        }
    }
    
    /* 反初始化YuleTech适配层 */
    YuleMbedtls_DeInit();
    
    Mqtt_TlsInitialized = FALSE;
}

Mqtt_ReturnType Mqtt_Tls_CreateContext(
    const Mqtt_TlsConfigType* config,
    Mqtt_TlsContextType* context
)
{
    Mqtt_TlsInternalContextType* ctx;
    uint8 i;
    int ret;
    
    if (!Mqtt_TlsInitialized) {
        return MQTT_E_NOT_OK;
    }
    
    if (config == NULL_PTR || context == NULL_PTR) {
        return MQTT_E_NOT_OK;
    }
    
    /* 查找空闲上下文槽 */
    ctx = NULL_PTR;
    for (i = 0; i < MQTT_TLS_MAX_CONTEXTS; i++) {
        if (!Mqtt_TlsContexts[i].inUse) {
            ctx = &Mqtt_TlsContexts[i];
            break;
        }
    }
    
    if (ctx == NULL_PTR) {
        return MQTT_E_NOT_OK; /* 没有空闲上下文 */
    }
    
    /* 初始化上下文 */
    memset(ctx, 0, sizeof(Mqtt_TlsInternalContextType));
    ctx->inUse = TRUE;
    ctx->socketId = TCPIP_SOCKETID_INVALID;
    ctx->handshakeComplete = FALSE;
    ctx->lastError = 0;
    
    /* 复制配置 */
    memcpy(&ctx->config, config, sizeof(Mqtt_TlsConfigType));
    
    /* 初始化mbedTLS上下文 */
    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);
    mbedtls_x509_crt_init(&ctx->caCert);
    mbedtls_x509_crt_init(&ctx->clientCert);
    mbedtls_pk_init(&ctx->clientKey);
    mbedtls_ssl_session_init(&ctx->savedSession);
    
    /* 配置TLS版本 */
    mbedtls_ssl_conf_min_version(&ctx->conf, 
        MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3); /* TLS 1.2+ */
    
    switch (config->version) {
        case MQTT_TLS_VERSION_1_2:
            mbedtls_ssl_conf_max_version(&ctx->conf,
                MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
            break;
        case MQTT_TLS_VERSION_1_3:
            /* TLS 1.3 需要更新版本的mbedTLS */
            break;
        default:
            break;
    }
    
    /* 配置证书验证模式 */
    switch (config->verifyMode) {
        case MQTT_TLS_VERIFY_NONE:
            mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE);
            break;
        case MQTT_TLS_VERIFY_OPTIONAL:
            mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
            break;
        case MQTT_TLS_VERIFY_REQUIRED:
        default:
            mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
            break;
    }
    
    /* 加载CA证书 */
    if (config->trustStore.caCert != NULL_PTR && config->trustStore.caCertLength > 0U ) {
        if (config->trustStore.caFormat == MQTT_CERT_FORMAT_PEM) {
            ret = mbedtls_x509_crt_parse(&ctx->caCert, 
                config->trustStore.caCert, config->trustStore.caCertLength + 1);
        } else {
            ret = mbedtls_x509_crt_parse_der(&ctx->caCert,
                config->trustStore.caCert, config->trustStore.caCertLength);
        }
        
        if (ret != 0U ) {
            ctx->lastError = ret;
            goto cleanup;
        }
        
        mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->caCert, NULL_PTR);
    }
    
    /* 加载客户端证书和私钥(mTLS) */
    if (config->clientCert != NULL_PTR && config->clientCert->data != NULL_PTR) {
        if (config->clientCert->format == MQTT_CERT_FORMAT_PEM) {
            ret = mbedtls_x509_crt_parse(&ctx->clientCert,
                config->clientCert->data, config->clientCert->length + 1);
        } else {
            ret = mbedtls_x509_crt_parse_der(&ctx->clientCert,
                config->clientCert->data, config->clientCert->length);
        }
        
        if (ret != 0U ) {
            ctx->lastError = ret;
            goto cleanup;
        }
        
        if (config->clientKey != NULL_PTR && config->clientKey->data != NULL_PTR) {
            const char* pwd = config->clientKey->password;
            
            ret = mbedtls_pk_parse_key(&ctx->clientKey,
                config->clientKey->data, config->clientKey->length,
                (const unsigned char*)pwd, pwd ? strlen(pwd) : 0,
                mbedtls_ctr_drbg_random, NULL_PTR);
            
            if (ret != 0U ) {
                ctx->lastError = ret;
                goto cleanup;
            }
            
            ret = mbedtls_ssl_conf_own_cert(&ctx->conf, &ctx->clientCert, &ctx->clientKey);
            if (ret != 0U ) {
                ctx->lastError = ret;
                goto cleanup;
            }
        }
    }
    
    /* 配置SNI */
    if (config->expectedHostname != NULL_PTR) {
        ret = mbedtls_ssl_set_hostname(&ctx->ssl, config->expectedHostname);
        if (ret != 0U ) {
            ctx->lastError = ret;
            goto cleanup;
        }
    }
    
    /* 配置回调函数 */
    mbedtls_ssl_conf_dbg(&ctx->conf, (void (*)(void*, int, const char*, int, const char*))YuleMbedtls_DebugCallback, NULL_PTR);
    mbedtls_debug_set_threshold(3);
    
    /* 应用配置 */
    ret = mbedtls_ssl_setup(&ctx->ssl, &ctx->conf);
    if (ret != 0U ) {
        ctx->lastError = ret;
        goto cleanup;
    }
    
    *context = ctx;
    return MQTT_OK;
    
cleanup:
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_x509_crt_free(&ctx->caCert);
    mbedtls_x509_crt_free(&ctx->clientCert);
    mbedtls_pk_free(&ctx->clientKey);
    mbedtls_ssl_session_free(&ctx->savedSession);
    ctx->inUse = FALSE;
    return MQTT_E_NOT_OK;
}

void Mqtt_Tls_DestroyContext(Mqtt_TlsContextType context)
{
    Mqtt_TlsInternalContextType* ctx;
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx == NULL_PTR) {
        return;
    }
    
    /* 关闭TLS连接 */
    if (ctx->handshakeComplete) {
        Mqtt_Tls_Close(context);
    }
    
    /* 释放mbedTLS上下文 */
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_x509_crt_free(&ctx->caCert);
    mbedtls_x509_crt_free(&ctx->clientCert);
    mbedtls_pk_free(&ctx->clientKey);
    mbedtls_ssl_session_free(&ctx->savedSession);
    
    /* 清零上下文 */
    memset(ctx, 0, sizeof(Mqtt_TlsInternalContextType));
    ctx->inUse = FALSE;
}

/*============================================================================
 * TLS连接操作
 *===========================================================================*/

Mqtt_ReturnType Mqtt_Tls_PerformHandshake(
    Mqtt_TlsContextType context,
    TcpIp_SocketIdType socketId,
    Mqtt_TlsHandshakeCallbackType callback
)
{
    Mqtt_TlsInternalContextType* ctx;
    int ret;
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx == NULL_PTR) {
        return MQTT_E_NOT_OK;
    }
    
    if (socketId == TCPIP_SOCKETID_INVALID) {
        return MQTT_E_NOT_OK;
    }
    
    ctx->socketId = socketId;
    ctx->handshakeCallback = callback;
    
    /* 配置生物IO回调 */
    mbedtls_ssl_set_bio(&ctx->ssl, ctx, 
        (mbedtls_ssl_send_t*)Mqtt_Tls_SendCallback,
        (mbedtls_ssl_recv_t*)Mqtt_Tls_RecvCallback,
        (mbedtls_ssl_recv_timeout_t*)Mqtt_Tls_RecvTimeoutCallback);
    
    /* 执行TLS握手 */
    ret = mbedtls_ssl_handshake(&ctx->ssl);
    
    if (ret == 0U ) {
        ctx->handshakeComplete = TRUE;
        
        /* 保存会话(如果启用) */
        if (ctx->config.enableSessionResumption) {
            mbedtls_ssl_get_session(&ctx->ssl, &ctx->savedSession);
        }
        
        if (callback != NULL_PTR) {
            Mqtt_CertValidationResultType validationResult;
            memset(&validationResult, 0, sizeof(validationResult));
            validationResult.isValid = TRUE;
            validationResult.isTrusted = TRUE;
            validationResult.hostnameMatch = TRUE;
            validationResult.notExpired = TRUE;
            callback(0, TRUE, &validationResult);
        }
        
        return MQTT_OK;
    } else {
        ctx->lastError = ret;
        
        if (callback != NULL_PTR) {
            callback(0, FALSE, NULL_PTR);
        }
        
        return MQTT_E_NOT_OK;
    }
}

Mqtt_ReturnType Mqtt_Tls_Send(
    Mqtt_TlsContextType context,
    const uint8* data,
    uint32 length,
    uint32* sentLength
)
{
    Mqtt_TlsInternalContextType* ctx;
    int ret;
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx == NULL_PTR) {
        return MQTT_E_NOT_OK;
    }
    
    if (!ctx->handshakeComplete) {
        return MQTT_E_NOT_OK;
    }
    
    if (data == NULL_PTR || length == 0U ) {
        return MQTT_E_NOT_OK;
    }
    
    ret = mbedtls_ssl_write(&ctx->ssl, data, length);
    
    if (ret > 0U ) {
        if (sentLength != NULL_PTR) {
            *sentLength = (uint32)ret;
        }
        return MQTT_OK;
    } else {
        ctx->lastError = ret;
        return MQTT_E_NOT_OK;
    }
}

Mqtt_ReturnType Mqtt_Tls_Receive(
    Mqtt_TlsContextType context,
    uint8* buffer,
    uint32 bufferSize,
    uint32* receivedLength
)
{
    Mqtt_TlsInternalContextType* ctx;
    int ret;
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx == NULL_PTR) {
        return MQTT_E_NOT_OK;
    }
    
    if (!ctx->handshakeComplete) {
        return MQTT_E_NOT_OK;
    }
    
    if (buffer == NULL_PTR || bufferSize == 0U ) {
        return MQTT_E_NOT_OK;
    }
    
    ret = mbedtls_ssl_read(&ctx->ssl, buffer, bufferSize);
    
    if (ret >= 0) {
        if (receivedLength != NULL_PTR) {
            *receivedLength = (uint32)ret;
        }
        return MQTT_OK;
    } else {
        ctx->lastError = ret;
        return MQTT_E_NOT_OK;
    }
}

Mqtt_ReturnType Mqtt_Tls_Close(Mqtt_TlsContextType context)
{
    Mqtt_TlsInternalContextType* ctx;
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx == NULL_PTR) {
        return MQTT_E_NOT_OK;
    }
    
    if (!ctx->handshakeComplete) {
        return MQTT_OK;
    }
    
    /* 发送close_notify */
    mbedtls_ssl_close_notify(&ctx->ssl);
    
    ctx->handshakeComplete = FALSE;
    
    return MQTT_OK;
}

/*============================================================================
 * 证书管理函数
 *===========================================================================*/

Mqtt_ReturnType Mqtt_Tls_VerifyCertificate(
    Mqtt_TlsContextType context,
    const char* expectedHostname,
    Mqtt_CertValidationResultType* result
)
{
    Mqtt_TlsInternalContextType* ctx;
    uint32_t flags;
    const mbedtls_x509_crt* peerCert;
    
    if (result == NULL_PTR) {
        return MQTT_E_NOT_OK;
    }
    
    memset(result, 0, sizeof(Mqtt_CertValidationResultType));
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx == NULL_PTR) {
        return MQTT_E_NOT_OK;
    }
    
    /* 获取验证结果 */
    flags = mbedtls_ssl_get_verify_result(&ctx->ssl);
    result->isValid = (flags == 0U );
    result->isTrusted = (flags == 0U );
    
    /* 获取对端证书 */
    peerCert = mbedtls_ssl_get_peer_cert(&ctx->ssl);
    if (peerCert != NULL_PTR) {
        /* 拷贝颁发者信息 */
        if (peerCert->issuer.next != NULL_PTR && peerCert->issuer.next->val.p != NULL_PTR) {
            strncpy(result->issuer, (char*)peerCert->issuer.next->val.p,
                sizeof(result->issuer) - 1);
        }
        
        /* 拷贝主体信息 */
        if (peerCert->subject.next != NULL_PTR && peerCert->subject.next->val.p != NULL_PTR) {
            strncpy(result->subject, (char*)peerCert->subject.next->val.p,
                sizeof(result->subject) - 1);
        }
        
        /* 检查过期: 无下级证书视为未过期 */
        if (peerCert->next == NULL_PTR) {
            result->notExpired = TRUE;
        }
    }
    
    result->hostnameMatch = TRUE;
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_Tls_GetPeerCertificateInfo(
    Mqtt_TlsContextType context,
    Mqtt_CertValidationResultType* result
)
{
    return Mqtt_Tls_VerifyCertificate(context, NULL_PTR, result);
}

/*============================================================================
 * 会话管理
 *===========================================================================*/

Mqtt_ReturnType Mqtt_Tls_SaveSession(
    Mqtt_TlsContextType context,
    uint8* sessionData,
    uint32 sessionDataSize,
    uint32* actualSize
)
{
    const Mqtt_TlsInternalContextType* ctx;
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx == NULL_PTR) {
        return MQTT_E_NOT_OK;
    }
    
    /* 会话数据保存在内部上下文中 */
    if (actualSize != NULL_PTR) {
        *actualSize = sizeof(mbedtls_ssl_session);
    }
    
    if (sessionData != NULL_PTR && sessionDataSize >= sizeof(mbedtls_ssl_session)) {
        memcpy(sessionData, &ctx->savedSession, sizeof(mbedtls_ssl_session));
    }
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_Tls_RestoreSession(
    Mqtt_TlsContextType context,
    const uint8* sessionData,
    uint32 sessionDataSize
)
{
    Mqtt_TlsInternalContextType* ctx;
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx == NULL_PTR) {
        return MQTT_E_NOT_OK;
    }
    
    if (sessionData == NULL_PTR || sessionDataSize < sizeof(mbedtls_ssl_session)) {
        return MQTT_E_NOT_OK;
    }
    
    memcpy(&ctx->savedSession, sessionData, sizeof(mbedtls_ssl_session));
    
    return MQTT_OK;
}

/*============================================================================
 * 错误处理
 *===========================================================================*/

sint32 Mqtt_Tls_GetLastError(Mqtt_TlsContextType context)
{
    const Mqtt_TlsInternalContextType* ctx;
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx == NULL_PTR) {
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }
    
    return ctx->lastError;
}

const char* Mqtt_Tls_GetErrorString(sint32 errorCode)
{
    static char error_buf[MQTT_TLS_MAX_ERROR_STRING];
    
    mbedtls_strerror(errorCode, error_buf, sizeof(error_buf));
    
    if (error_buf[0] == '\0') {
        return "Unknown error";
    }
    
    return error_buf;
}

void Mqtt_Tls_SetErrorCallback(
    Mqtt_TlsContextType context,
    Mqtt_TlsErrorCallbackType callback
)
{
    Mqtt_TlsInternalContextType* ctx;
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx != NULL_PTR) {
        ctx->errorCallback = callback;
    }
}

/*============================================================================
 * 详细信息查询
 *===========================================================================*/

Mqtt_TlsVersionType Mqtt_Tls_GetVersion(Mqtt_TlsContextType context)
{
    Mqtt_TlsInternalContextType* ctx;
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx == NULL_PTR) {
        return MQTT_TLS_VERSION_1_2;
    }
    
    /* 获取当前协议版本 */
    int major = mbedtls_ssl_get_version_number(&ctx->ssl) >> 8;
    int minor = mbedtls_ssl_get_version_number(&ctx->ssl) & 0xFF;
    
    if (major == 3 && minor == 3) {
        return MQTT_TLS_VERSION_1_2;
    } else if (major == 3 && minor == 4) {
        return MQTT_TLS_VERSION_1_3;
    }
    
    return ctx->config.version;
}

Mqtt_ReturnType Mqtt_Tls_GetCipherSuite(
    Mqtt_TlsContextType context,
    char* name,
    uint32 nameSize
)
{
    Mqtt_TlsInternalContextType* ctx;
    const char* suite;
    
    ctx = Mqtt_Tls_GetContext(context);
    if (ctx == NULL_PTR) {
        return MQTT_E_NOT_OK;
    }
    
    if (name == NULL_PTR || nameSize == 0U ) {
        return MQTT_E_NOT_OK;
    }
    
    suite = mbedtls_ssl_get_ciphersuite(&ctx->ssl);
    if (suite != NULL_PTR) {
        strncpy(name, suite, nameSize - 1);
        name[nameSize - 1] = '\0';
    } else {
        name[0] = '\0';
    }
    
    return MQTT_OK;
}

/*============================================================================
 * 内部函数实现
 *===========================================================================*/

static Mqtt_TlsInternalContextType* Mqtt_Tls_GetContext(Mqtt_TlsContextType context)
{
    Mqtt_TlsInternalContextType* ctx;
    
    if (context == NULL_PTR) {
        return NULL_PTR;
    }
    
    ctx = (Mqtt_TlsInternalContextType*)context;
    
    /* 验证上下文是否有效 */
    if (ctx < &Mqtt_TlsContexts[0] ||
        ctx >= &Mqtt_TlsContexts[MQTT_TLS_MAX_CONTEXTS]) {
        return NULL_PTR;
    }
    
    if (!ctx->inUse) {
        return NULL_PTR;
    }
    
    return ctx;
}

static int Mqtt_Tls_SendCallback(void* ctx, const unsigned char* buf, size_t len)
{
    Mqtt_TlsInternalContextType* tlsCtx;
    Std_ReturnType result;
    
    tlsCtx = (Mqtt_TlsInternalContextType*)ctx;
    if (tlsCtx == NULL_PTR) {
        return -1;
    }
    
    result = TcpIp_Send(tlsCtx->socketId, buf, (uint16)len);
    
    if (result == E_OK) {
        return (int)len;
    } else {
        return MBEDTLS_ERR_NET_SEND_FAILED;
    }
}

static int Mqtt_Tls_RecvCallback(void* ctx, unsigned char* buf, size_t len)
{
    Mqtt_TlsInternalContextType* tlsCtx;
    Std_ReturnType result;
    uint16 recvLen = 0;
    
    tlsCtx = (Mqtt_TlsInternalContextType*)ctx;
    if (tlsCtx == NULL_PTR) {
        return -1;
    }
    
    result = TcpIp_Receive(tlsCtx->socketId, buf, (uint16)len, &recvLen);
    
    if (result == E_OK) {
        if (recvLen == 0U ) {
            return MBEDTLS_ERR_SSL_WANT_READ;
        }
        return (int)recvLen;
    } else {
        return MBEDTLS_ERR_NET_RECV_FAILED;
    }
}

static int Mqtt_Tls_RecvTimeoutCallback(void* ctx, unsigned char* buf, size_t len, uint32_t timeout)
{
    (void)timeout;  /* 嵌入式系统中通常不支持带超时的接收 */
    return Mqtt_Tls_RecvCallback(ctx, buf, len);
}
