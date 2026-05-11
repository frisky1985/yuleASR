/** @file Mqtt_Tls.h
 * @brief MQTT TLS/SSL/mTLS 安全层头文件
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 提供TLS 1.2/1.3、SSL和mTLS双向认证支持
 * 基于mbedTLS或其他轻量级TLS库实现
 */

#ifndef MQTT_TLS_H
#define MQTT_TLS_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 包含文件
 *===========================================================================*/
#include "Mqtt.h"
#include "TcpIp.h"

/*============================================================================
 * TLS版本定义
 *===========================================================================*/
typedef enum {
    MQTT_TLS_VERSION_1_0 = 0,  /**< TLS 1.0 (不推荐) */
    MQTT_TLS_VERSION_1_1,      /**< TLS 1.1 (u4e0d推荐) */
    MQTT_TLS_VERSION_1_2,      /**< TLS 1.2 (推荐) */
    MQTT_TLS_VERSION_1_3       /**< TLS 1.3 (最佳) */
} Mqtt_TlsVersionType;

typedef enum {
    MQTT_SSL_VERSION_3_0 = 0   /**< SSL 3.0 (已弃用) */
} Mqtt_SslVersionType;

/*============================================================================
 * 安全级别定义
 *===========================================================================*/
typedef enum {
    MQTT_TLS_SECURITY_LOW = 0,      /**< 低安全 (兼容性优先) */
    MQTT_TLS_SECURITY_MEDIUM,       /**< 中等安全 */
    MQTT_TLS_SECURITY_HIGH,         /**< 高安全 (推荐) */
    MQTT_TLS_SECURITY_VERY_HIGH     /**< 最高安全 */
} Mqtt_TlsSecurityLevelType;

/*============================================================================
 * 证书验证模式
 *===========================================================================*/
typedef enum {
    MQTT_TLS_VERIFY_NONE = 0,       /**< 不验证服务器证书 (不安全) */
    MQTT_TLS_VERIFY_OPTIONAL,       /**< 可选验证 */
    MQTT_TLS_VERIFY_REQUIRED        /**< 必须验证 (推荐) */
} Mqtt_TlsVerifyModeType;

/*============================================================================
 * 密码套件组合
 *===========================================================================*/
#define MQTT_TLS_CIPHER_SUITE_ALL           (0xFFFFFFFFU)
#define MQTT_TLS_CIPHER_SUITE_SECURE_ONLY   (0x00000001U)
#define MQTT_TLS_CIPHER_SUITE_FAST_ONLY     (0x00000002U)
#define MQTT_TLS_CIPHER_SUITE_PFS_ONLY      (0x00000004U)  /**< 前向保密 */

/*============================================================================
 * 证书类型定义
 *===========================================================================*/

/**
 * @brief 证书格式
 */
typedef enum {
    MQTT_CERT_FORMAT_PEM = 0,       /**< PEM格式 (Base64) */
    MQTT_CERT_FORMAT_DER,           /**< DER格式 (二进制) */
    MQTT_CERT_FORMAT_PFX,           /**< PKCS#12格式 */
    MQTT_CERT_FORMAT_P7B            /**< PKCS#7格式 */
} Mqtt_CertFormatType;

/**
 * @brief 私钥类型
 */
typedef enum {
    MQTT_KEY_TYPE_RSA = 0,          /**< RSA私钥 */
    MQTT_KEY_TYPE_ECC,              /**< 椭圆曲线私钥 */
    MQTT_KEY_TYPE_ED25519           /**< Ed25519私钥 */
} Mqtt_KeyTypeType;

/**
 * @brief 证书缓存策略
 */
typedef enum {
    MQTT_CERT_CACHE_NONE = 0,       /**< 不缓存 */
    MQTT_CERT_CACHE_MEMORY,         /**< 内存缓存 */
    MQTT_CERT_CACHE_NVM             /**< 非易失性存储缓存 */
} Mqtt_CertCacheType;

/**
 * @brief 证书信息结构
 */
typedef struct {
    const uint8* data;              /**< 证书数据 */
    uint32 length;                  /**< 证书长度 */
    Mqtt_CertFormatType format;     /**< 证书格式 */
    Mqtt_CertCacheType cache;       /**< 缓存策略 */
} Mqtt_CertificateType;

/**
 * @brief 私钥信息结构
 */
typedef struct {
    const uint8* data;              /**< 私钥数据 */
    uint32 length;                  /**< 私钥长度 */
    Mqtt_KeyTypeType type;          /**< 私钥类型 */
    Mqtt_CertFormatType format;     /**< 私钥格式 */
    const char* password;           /**< 私钥密码(如有) */
} Mqtt_PrivateKeyType;

/**
 * @brief 可信证书库结构
 */
typedef struct {
    const uint8* caCert;            /**< CA证书数据 */
    uint32 caCertLength;            /**< CA证书长度 */
    Mqtt_CertFormatType caFormat;   /**< CA证书格式 */
    const char* caPath;             /**< CA证书目录(可为NULL) */
    boolean useSystemStore;         /**< 使用系统证书库 */
} Mqtt_TrustStoreType;

/*============================================================================
 * TLS配置结构
 *===========================================================================*/

/**
 * @brief TLS配置结构
 */
typedef struct {
    /* 协议版本 */
    Mqtt_TlsVersionType version;            /**< TLS版本 */
    boolean allowSslFallback;               /**< 允许SSL回退(不推荐) */
    
    /* 证书配置 */
    Mqtt_TrustStoreType trustStore;         /**< 可信证书库 */
    Mqtt_CertificateType* clientCert;       /**< 客户端证书(mTLS时必需) */
    Mqtt_PrivateKeyType* clientKey;         /**< 客户端私钥(mTLS时必需) */
    
    /* 验证设置 */
    Mqtt_TlsVerifyModeType verifyMode;      /**< 证书验证模式 */
    const char* expectedHostname;           /**< 期望的服务器主机名(SNI) */
    boolean checkCertificateExpiry;         /**< 检查证书过期 */
    
    /* 安全级别 */
    Mqtt_TlsSecurityLevelType securityLevel; /**< 安全级别 */
    uint32 cipherSuites;                     /**< 密码套件组合 */
    boolean useSecureRenegotiation;          /**< 安全重协商 */
    
    /* 连接设置 */
    uint32 handshakeTimeoutMs;               /**< 握手超时(毫秒) */
    uint32 sessionTimeoutMs;                 /**< 会话超时(毫秒) */
    boolean enableSessionResumption;         /**< 启用会话恢复 */
    
    /* 内存管理 */
    void* tlsContextBuffer;                  /**< TLS上下文缓冲区(静态分配) */
    uint32 tlsContextBufferSize;             /**< 缓冲区大小 */
    
} Mqtt_TlsConfigType;

/*============================================================================
 * 证书验证结果
 *===========================================================================*/
typedef struct {
    boolean isValid;                        /**< 证书有效 */
    boolean isTrusted;                      /**< 证书可信 */
    boolean hostnameMatch;                  /**< 主机名匹配 */
    boolean notExpired;                     /**< 未过期 */
    uint32 daysUntilExpiry;                 /**< 过期倒计时(天) */
    char issuer[128];                       /**< 颁发者 */
    char subject[128];                      /**< 主体 */
    char serialNumber[64];                  /**< 序列号 */
} Mqtt_CertValidationResultType;

/*============================================================================
 * TLS上下文类型
 *===========================================================================*/
typedef void* Mqtt_TlsContextType;

/*============================================================================
 * 回调函数类型
 *===========================================================================*/
typedef void (*Mqtt_TlsHandshakeCallbackType)(
    Mqtt_ConnectionIdType connectionId,
    boolean success,
    const Mqtt_CertValidationResultType* validationResult
);

typedef void (*Mqtt_TlsErrorCallbackType)(
    Mqtt_ConnectionIdType connectionId,
    sint32 errorCode,
    const char* errorMessage
);

/*============================================================================
 * TLS初始化和配置
 *===========================================================================*/

/**
 * @brief 初始化TLS子系统
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_Init(void);

/**
 * @brief 反初始化TLS子系统
 */
extern void Mqtt_Tls_DeInit(void);

/**
 * @brief 创建TLS上下文
 * @param config TLS配置
 * @param context 输出参数: TLS上下文
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_CreateContext(
    const Mqtt_TlsConfigType* config,
    Mqtt_TlsContextType* context
);

/**
 * @brief 销毁TLS上下文
 * @param context TLS上下文
 */
extern void Mqtt_Tls_DestroyContext(Mqtt_TlsContextType context);

/*============================================================================
 * TLS连接操作
 *===========================================================================*/

/**
 * @brief 执行TLS握手
 * @param context TLS上下文
 * @param socketId 底层TCP套接字
 * @param callback 握手完成回调(可为NULL)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_PerformHandshake(
    Mqtt_TlsContextType context,
    TcpIp_SocketIdType socketId,
    Mqtt_TlsHandshakeCallbackType callback
);

/**
 * @brief 通过TLS发送数据
 * @param context TLS上下文
 * @param data 数据缓冲区
 * @param length 数据长度
 * @param sentLength 实际发送长度(输出)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_Send(
    Mqtt_TlsContextType context,
    const uint8* data,
    uint32 length,
    uint32* sentLength
);

/**
 * @brief 通过TLS接收数据
 * @param context TLS上下文
 * @param buffer 接收缓冲区
 * @param bufferSize 缓冲区大小
 * @param receivedLength 实际接收长度(输出)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_Receive(
    Mqtt_TlsContextType context,
    uint8* buffer,
    uint32 bufferSize,
    uint32* receivedLength
);

/**
 * @brief 关闭TLS连接(发送close_notify)
 * @param context TLS上下文
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_Close(Mqtt_TlsContextType context);

/*============================================================================
 * 证书管理函数
 *===========================================================================*/

/**
 * @brief 加载证书到内存
 * @param cert 证书结构
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_LoadCertificate(
    const Mqtt_CertificateType* cert
);

/**
 * @brief 加载私钥到内存
 * @param key 私钥结构
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_LoadPrivateKey(
    const Mqtt_PrivateKeyType* key
);

/**
 * @brief 加载CA证书库
 * @param trustStore 可信证书库配置
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_LoadTrustStore(
    const Mqtt_TrustStoreType* trustStore
);

/**
 * @brief 验证对端证书
 * @param context TLS上下文
 * @param expectedHostname 期望主机名(可为NULL)
 * @param result 验证结果(输出)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_VerifyCertificate(
    Mqtt_TlsContextType context,
    const char* expectedHostname,
    Mqtt_CertValidationResultType* result
);

/**
 * @brief 获取对端证书信息
 * @param context TLS上下文
 * @param result 证书验证结果(输出)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_GetPeerCertificateInfo(
    Mqtt_TlsContextType context,
    Mqtt_CertValidationResultType* result
);

/*============================================================================
 * 会话管理
 *===========================================================================*/

/**
 * @brief 保存TLS会话
 * @param context TLS上下文
 * @param sessionData 会话数据缓冲区
 * @param sessionDataSize 缓冲区大小
 * @param actualSize 实际会话数据大小(输出)
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_SaveSession(
    Mqtt_TlsContextType context,
    uint8* sessionData,
    uint32 sessionDataSize,
    uint32* actualSize
);

/**
 * @brief 恢复TLS会话
 * @param context TLS上下文
 * @param sessionData 会话数据
 * @param sessionDataSize 会话数据大小
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_RestoreSession(
    Mqtt_TlsContextType context,
    const uint8* sessionData,
    uint32 sessionDataSize
);

/*============================================================================
 * 错误处理
 *===========================================================================*/

/**
 * @brief 获取最近的TLS错误码
 * @param context TLS上下文
 * @return 错误码
 */
extern sint32 Mqtt_Tls_GetLastError(Mqtt_TlsContextType context);

/**
 * @brief 获取错误信息字符串
 * @param errorCode 错误码
 * @return 错误信息字符串
 */
extern const char* Mqtt_Tls_GetErrorString(sint32 errorCode);

/**
 * @brief 设置TLS错误回调
 * @param context TLS上下文
 * @param callback 错误回调函数
 */
extern void Mqtt_Tls_SetErrorCallback(
    Mqtt_TlsContextType context,
    Mqtt_TlsErrorCallbackType callback
);

/*============================================================================
 * 详细信息查询
 *===========================================================================*/

/**
 * @brief 获取当前TLS版本
 * @param context TLS上下文
 * @return TLS版本
 */
extern Mqtt_TlsVersionType Mqtt_Tls_GetVersion(Mqtt_TlsContextType context);

/**
 * @brief 获取当前密码套件名称
 * @param context TLS上下文
 * @param name 密码套件名称缓冲区
 * @param nameSize 缓冲区大小
 * @return 操作结果
 */
extern Mqtt_ReturnType Mqtt_Tls_GetCipherSuite(
    Mqtt_TlsContextType context,
    char* name,
    uint32 nameSize
);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_TLS_H */
