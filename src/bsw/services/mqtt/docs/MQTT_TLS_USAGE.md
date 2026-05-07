# MQTT TLS/SSL/mTLS 使用指南

本文档说明如何在MQTT模块中使用TLS/SSL/mTLS安全连接。

## 目录

1. [概述](#概述)
2. [配置选项](#配置选项)
3. [快速开始](#快速开始)
4. [TLS连接示例](#tls连接示例)
5. [mTLS双向认证示例](#mtls双向认证示例)
6. [证书管理](#证书管理)
7. [常见问题](#常见问题)

## 概述

MQTT模块支持以下安全协议:

| 协议 | 版本 | 推荐程度 | 说明 |
|-------|-------|-----------|------|
| TLS | 1.2 | ⭐⭐⭐⭐⭐ | 推荐使用 |
| TLS | 1.3 | ⭐⭐⭐⭐⭐ | 最佳安全性 |
| SSL | 3.0 | ❌ | 已弃用 |

### 功能特性

- ✅ **TLS 1.2/1.3**加密连接
- ✅ **mTLS**双向认证(服务器+客户端证书)
- ✅ **SNI** (服务器名称指示)
- ✅ 证书**过期检查**
- ✅ **会话恢复**功能
- ✅ 多种证书格式(PEM/DER)

## 配置选项

### Mqtt_Cfg.h 配置

```c
/* 启用TLS支持 */
#define MQTT_SUPPORT_TLS          (STD_ON)

/* 启用mTLS双向认证 */
#define MQTT_SUPPORT_MTLS         (STD_ON)

/* 默认TLS版本 */
#define MQTT_DEFAULT_TLS_VERSION  (MQTT_TLS_VERSION_1_2)

/* TLS缓冲区大小 */
#define MQTT_TLS_SEND_BUFFER_SIZE     (4096U)
#define MQTT_TLS_RECV_BUFFER_SIZE     (4096U)

/* 握手超时时间 */
#define MQTT_TLS_HANDSHAKE_TIMEOUT_MS (10000U)
```

## 快速开始

### 1. 基本TLS连接(只验证服务器)

```c
#include "Mqtt.h"
#include "Mqtt_Tls.h"

/* CA证书数据(以PEM格式) */
static const uint8_t caCertData[] = 
"-----BEGIN CERTIFICATE-----\n"
"MIIDXTCCAkWgAwIBAgIJAKoK/heBjcOuMA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV\n"
"...\n"
"-----END CERTIFICATE-----\n";

void Mqtt_ConnectWithTls(void)
{
    Mqtt_ReturnType result;
    Mqtt_ConnectionIdType connId;
    
    /* TLS配置 */
    Mqtt_TlsConfigType tlsConfig;
    memset(&tlsConfig, 0, sizeof(tlsConfig));
    
    /* 设置TLS版本 */
    tlsConfig.version = MQTT_TLS_VERSION_1_2;
    
    /* 配置可信证书库 */
    tlsConfig.trustStore.caCert = caCertData;
    tlsConfig.trustStore.caCertLength = sizeof(caCertData) - 1;
    tlsConfig.trustStore.caFormat = MQTT_CERT_FORMAT_PEM;
    tlsConfig.trustStore.useSystemStore = FALSE;
    
    /* 配置证书验证 */
    tlsConfig.verifyMode = MQTT_TLS_VERIFY_REQUIRED;
    tlsConfig.expectedHostname = "mqtt.example.com";
    tlsConfig.checkCertificateExpiry = TRUE;
    
    /* 设置安全级别 */
    tlsConfig.securityLevel = MQTT_TLS_SECURITY_HIGH;
    
    /* MQTT连接配置 */
    Mqtt_ConnectionConfigType connConfig;
    memset(&connConfig, 0, sizeof(connConfig));
    connConfig.brokerHost = "mqtt.example.com";
    connConfig.brokerPort = 8883;  /* 标准TLS端口 */
    connConfig.clientId = "MyClient";
    connConfig.keepAliveSeconds = 60;
    connConfig.cleanSession = MQTT_CLEAN_SESSION_YES;
    connConfig.version = MQTT_VERSION_311;
    connConfig.connectTimeoutMs = 10000;
    connConfig.autoReconnect = TRUE;
    
    /* 启用TLS */
    connConfig.useTls = TRUE;
    connConfig.tlsConfig = &tlsConfig;
    
    /* 建立连接 */
    result = Mqtt_Connect(&connId, &connConfig);
    if (result == MQTT_OK) {
        printf("TLS连接建立成功!\n");
    } else {
        printf("TLS连接失败: %d\n", result);
    }
}
```

## TLS连接示例

### 使用证书管理模块

```c
#include "Mqtt.h"
#include "Mqtt_CertMgr.h"

void SetupTlsWithCertManager(void)
{
    /* 初始化证书管理器 */
    Mqtt_CertMgrConfigType certMgrConfig = {
        .autoReload = TRUE,
        .checkIntervalMs = 60000,
        .strictValidation = TRUE,
        .expiryWarningDays = 30
    };
    Mqtt_CertMgr_Init(&certMgrConfig);
    
    /* 导入CA证书 */
    Mqtt_CertMgr_ImportFromPem(caCertPem, "ca-root", MQTT_CERT_TYPE_CA_ROOT);
    
    /* TLS配置 */
    Mqtt_TlsConfigType tlsConfig;
    Mqtt_TrustStoreType trustStore;
    
    /* 从证书管理器加载CA */
    Mqtt_CertMgr_LoadTrustStore("ca-root", &trustStore);
    
    memcpy(&tlsConfig.trustStore, &trustStore, sizeof(trustStore));
    tlsConfig.version = MQTT_TLS_VERSION_1_2;
    tlsConfig.verifyMode = MQTT_TLS_VERIFY_REQUIRED;
    
    /* 使用TLS连接... */
}
```

### 使用系统证书库

```c
void ConnectUsingSystemStore(void)
{
    Mqtt_TlsConfigType tlsConfig;
    memset(&tlsConfig, 0, sizeof(tlsConfig));
    
    /* 使用系统证书库(如操作系统的CA证书存储) */
    tlsConfig.trustStore.useSystemStore = TRUE;
    tlsConfig.trustStore.caCert = NULL;
    
    tlsConfig.version = MQTT_TLS_VERSION_1_2;
    tlsConfig.verifyMode = MQTT_TLS_VERIFY_REQUIRED;
    tlsConfig.expectedHostname = "mqtt.example.com";
    
    /* MQTT配置... */
    connConfig.useTls = TRUE;
    connConfig.tlsConfig = &tlsConfig;
}
```

## mTLS双向认证示例

mTLS (双向TLS) 要求客户端和服务器都提供证书进行身份验证。

```c
/* 客户端证书和私钥 */
static const uint8_t clientCertData[] = 
"-----BEGIN CERTIFICATE-----\n"
"MIIDXTCCAkWgAwIBAgIJAJC1HiIAZAiUMA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV\n"
"...\n"
"-----END CERTIFICATE-----\n";

static const uint8_t clientKeyData[] = 
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpQIBAAKCAQEA0Z3VS5JJcds3xfn/ygWyF8PbnGy0AHB7MqK8k7f5l2EckKlw\n"
"...\n"
"-----END RSA PRIVATE KEY-----\n";

/* CA证书 */
static const uint8_t caCertData[] = 
"-----BEGIN CERTIFICATE-----\n"
"...\n"
"-----END CERTIFICATE-----\n";

void Mqtt_ConnectWithMtls(void)
{
    Mqtt_ReturnType result;
    Mqtt_ConnectionIdType connId;
    
    /* 配置客户端证书 */
    Mqtt_CertificateType clientCert = {
        .data = clientCertData,
        .length = sizeof(clientCertData) - 1,
        .format = MQTT_CERT_FORMAT_PEM,
        .cache = MQTT_CERT_CACHE_MEMORY
    };
    
    /* 配置客户端私钥 */
    Mqtt_PrivateKeyType clientKey = {
        .data = clientKeyData,
        .length = sizeof(clientKeyData) - 1,
        .type = MQTT_KEY_TYPE_RSA,
        .format = MQTT_CERT_FORMAT_PEM,
        .password = NULL  /* 如果私钥有密码，在此设置 */
    };
    
    /* TLS配置 */
    Mqtt_TlsConfigType tlsConfig;
    memset(&tlsConfig, 0, sizeof(tlsConfig));
    
    tlsConfig.version = MQTT_TLS_VERSION_1_2;
    
    /* CA证书 */
    tlsConfig.trustStore.caCert = caCertData;
    tlsConfig.trustStore.caCertLength = sizeof(caCertData) - 1;
    tlsConfig.trustStore.caFormat = MQTT_CERT_FORMAT_PEM;
    
    /* mTLS配置: 客户端证书和私钥 */
    tlsConfig.clientCert = &clientCert;
    tlsConfig.clientKey = &clientKey;
    
    /* 必须验证服务器证书 */
    tlsConfig.verifyMode = MQTT_TLS_VERIFY_REQUIRED;
    tlsConfig.expectedHostname = "mqtt.example.com";
    
    /* 安全级别 */
    tlsConfig.securityLevel = MQTT_TLS_SECURITY_VERY_HIGH;
    tlsConfig.cipherSuites = MQTT_TLS_CIPHER_SUITE_PFS_ONLY;
    
    /* MQTT配置 */
    Mqtt_ConnectionConfigType connConfig;
    memset(&connConfig, 0, sizeof(connConfig));
    connConfig.brokerHost = "mqtt.example.com";
    connConfig.brokerPort = 8883;
    connConfig.clientId = "SecureClient";
    connConfig.keepAliveSeconds = 60;
    connConfig.cleanSession = MQTT_CLEAN_SESSION_YES;
    connConfig.useTls = TRUE;
    connConfig.tlsConfig = &tlsConfig;
    
    /* 建立连接 */
    result = Mqtt_Connect(&connId, &connConfig);
    if (result == MQTT_OK) {
        printf("mTLS连接成功! 双向认证完成\n");
    }
}
```

## 证书管理

### 证书过期预警

```c
/* 证书过期回调 */
void OnCertExpiryWarning(const char* alias, uint32 daysUntilExpiry)
{
    printf("警告: 证书 '%s' 即将在 %d 天后过期\n", 
           alias, daysUntilExpiry);
    
    /* 触发证书更新流程 */
    if (daysUntilExpiry < 7) {
        /* 紧急: 立即更新证书 */
        TriggerCertRenewal(alias);
    }
}

void SetupCertExpiryMonitoring(void)
{
    /* 设置过期回调 */
    Mqtt_CertMgr_SetExpiryCallback(OnCertExpiryWarning);
    
    /* 在主循环中调用 */
    while (1) {
        Mqtt_CertMgr_MainFunction();  /* 检查证书状态 */
        Mqtt_MainFunction();
        Delay(100);
    }
}
```

### 动态证书更新

```c
/* 证书重新加载回调 */
void OnCertReloaded(const char* alias, Mqtt_ReturnType result)
{
    if (result == MQTT_OK) {
        printf("证书 '%s' 重新加载成功\n", alias);
        
        /* 重新连接以使用新证书 */
        Mqtt_Reconnect(connId);
    } else {
        printf("证书 '%s' 重新加载失败\n", alias);
    }
}

void UpdateCertificate(const char* alias, const uint8* newCertData, uint32 len)
{
    Mqtt_CertMgr_SetReloadCallback(OnCertReloaded);
    
    /* 更新证书 */
    Mqtt_CertMgr_UpdateCert(alias, newCertData, len);
}
```

## 常见问题

### Q: TLS握手失败

```c
/* 检查错误代码 */
sint32 errorCode = Mqtt_Tls_GetLastError(conn->tlsContext);
const char* errorStr = Mqtt_Tls_GetErrorString(errorCode);
printf("TLS错误: %s\n", errorStr);

/* 常见原因:
 * - 证书格式错误
 * - 证书已过期
 * - 主机名不匹配
 * - CA证书不可信
 */
```

### Q: 如何检查当前TLS版本

```c
Mqtt_TlsVersionType version = Mqtt_Tls_GetVersion(tlsContext);
switch (version) {
    case MQTT_TLS_VERSION_1_2:
        printf("使用TLS 1.2\n");
        break;
    case MQTT_TLS_VERSION_1_3:
        printf("使用TLS 1.3\n");
        break;
    default:
        printf("未知TLS版本\n");
}

/* 查看当前密码套件 */
char cipherName[64];
Mqtt_Tls_GetCipherSuite(tlsContext, cipherName, sizeof(cipherName));
printf("密码套件: %s\n", cipherName);
```

### Q: 如何验证证书

```c
Mqtt_CertValidationResultType result;
Mqtt_ReturnType status = Mqtt_Tls_VerifyCertificate(tlsContext, 
                                                       "mqtt.example.com",
                                                       &result);
if (status == MQTT_OK) {
    printf("证书有效: %s\n", result.isValid ? "是" : "否");
    printf("发行者: %s\n", result.issuer);
    printf("主体: %s\n", result.subject);
    printf("过期倒计: %d 天\n", result.daysUntilExpiry);
}
```

### Q: 不同场景的端口选择

| 场景 | 协议 | 端口 |
|------|-------|------|
| 明文MQTT | TCP | 1883 |
| TLS加密MQTT | TLS | 8883 |
| WebSocket | WS | 80 |
| WebSocket + TLS | WSS | 443 |

## 安全建议

1. **始终使用TLS 1.2或更高版本**
2. **启用证书验证** (`MQTT_TLS_VERIFY_REQUIRED`)
3. **验证主机名** (`expectedHostname`)
4. **检查证书过期**
5. **使用前向保密密码套件** (`MQTT_TLS_CIPHER_SUITE_PFS_ONLY`)
6. **定期更新证书**
7. **使用强密码的私钥** (RSA 2048位或ECC P-256以上)

## API参考

详见头文件:
- `Mqtt_Tls.h` - TLS核心API
- `Mqtt_CertMgr.h` - 证书管理API
