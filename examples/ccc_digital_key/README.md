# CCC数字钥匙应用示例

本示例展示了如何在YuleTech AutoSAR BSW平台上实现CCC (Car Connectivity Consortium) 数字钥匙协议。

## 概述

CCC数字钥匙是一种车辆访问技术，允许用户使用智能手机或其他移动设备作为数字钥匙来解锁和启动车辆。本示例实现了CCC Digital Key规范 3.0 中的核心功能。

## 功能特性

### 1. 密钥协商 (ECDH P-256)
- 使用椭圆曲线密钥交换算法安全地生成共享密钥
- 支持临时密钥对生成
- 符合NIST P-256标准

### 2. 身份认证 (ECDSA P-256)
- 基于X.509证书的身份验证
- 挑战-响应认证机制
- 证书链验证
- 证书有效期检查

### 3. 加密通信 (AES-128-GCM)
- 认证加密的安全通信
- 消息完整性保护
- 重放攻击防护

### 4. 密钥派生 (HKDF-SHA256)
- 基于HMAC的提取和扩展函数
- 从共享密钥派生会话密钥
- 支持多个密钥素材派生

## 文件结构

```
examples/ccc_digital_key/
├── include/
│   ├── CccTypes.h          # CCC专有类型定义
│   └── CccDigitalKey.h     # 主头文件，API声明
├── src/
│   ├── CccDigitalKey.c     # 核心实现
│   ├── CccKeyAgreement.c   # 密钥协商实现
│   ├── CccAuthentication.c # 身份认证实现
│   └── CccSecureChannel.c  # 安全通信实现
└── README.md             # 本文件
```

## 依赖

- YuleTech AutoSAR BSW平台
- CSM (Crypto Services Manager)
- Det (Development Error Tracer)

## 配置

在使用前，需要配置CSM模块的密钥和作业ID：

```c
Ccc_ConfigType cccConfig = {
    .deviceId = {
        .deviceId = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                     0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10},
        .role = CCC_ROLE_VEHICLE,
        .protocolVersion = 0x0300
    },
    .role = CCC_ROLE_VEHICLE,
    .keyStorageId = 0x01,
    .certStorageId = 0x02,
    .csmKeyId = 0x01,      /* CSM密钥ID */
    .csmJobId = 0x01,      /* CSM作业ID */
    .useSecureStorage = TRUE
};
```

## 使用示例

### 初始化

```c
#include "CccDigitalKey.h"

void main(void)
{
    Ccc_ReturnType result;
    
    /* 初始化CCC模块 */
    result = Ccc_Init(&cccConfig);
    if (result != CCC_E_OK) {
        /* 处理错误 */
    }
    
    /* ... 应用代码 ... */
    
    /* 去初始化 */
    Ccc_DeInit();
}
```

### 配对流程

```c
/* 开始配对 */
uint8 localPublicKey[CCC_ECC_P256_PUBLIC_KEY_SIZE];
uint32 publicKeyLength = sizeof(localPublicKey);
Ccc_DeviceIdType mobileDevice;

result = Ccc_PairingStart(&mobileDevice, localPublicKey, &publicKeyLength);
if (result != CCC_E_OK) {
    /* 处理错误 */
}

/* 发送localPublicKey到移动设备 */

/* 接收远程公钥和证书后完成配对 */
Ccc_CertificateType remoteCert;
/* ... 接收远程证书 ... */

result = Ccc_PairingComplete(remotePublicKey, remotePublicKeyLength, &remoteCert);
if (result != CCC_E_OK) {
    /* 处理错误 */
}
```

### 认证流程

```c
/* 开始认证 */
uint8 challenge[CCC_CHALLENGE_SIZE];
uint32 challengeLength = sizeof(challenge);

result = Ccc_AuthenticationStart(challenge, &challengeLength);
if (result != CCC_E_OK) {
    /* 处理错误 */
}

/* 发送挑战值到远程设备 */

/* 接收远程响应后完成认证 */
uint8 localSignature[128];
uint32 localSigLength = sizeof(localSignature);

result = Ccc_AuthenticationComplete(
    remoteChallenge,
    remoteSignature,
    remoteSignatureLength,
    localSignature,
    &localSigLength
);

if (result != CCC_E_OK) {
    /* 认证失败 */
}
```

### 建立安全会话

```c
/* 建立会话 */
result = Ccc_SessionEstablish(
    TRUE,  /* 作为发起方 */
    remotePublicKey,
    remotePublicKeyLength
);

if (result != CCC_E_OK) {
    /* 处理错误 */
}

/* 现在可以进行安全通信 */
```

### 安全通信

```c
/* 发送安全消息 */
uint8 plaintext[] = "Unlock command";
uint8 ciphertext[256];
uint32 ciphertextLength = sizeof(ciphertext);
uint8 authTag[CCC_AES_TAG_SIZE];

result = Ccc_EncryptMessage(
    plaintext,
    sizeof(plaintext),
    ciphertext,
    &ciphertextLength,
    authTag
);

if (result == CCC_E_OK) {
    /* 发送密文和认证标签 */
}

/* 接收并解密消息 */
uint8 decrypted[256];
uint32 decryptedLength = sizeof(decrypted);

result = Ccc_DecryptMessage(
    receivedCiphertext,
    receivedCiphertextLength,
    receivedAuthTag,
    decrypted,
    &decryptedLength
);

if (result == CCC_E_OK) {
    /* 处理解密后的数据 */
}
```

### 使用安全消息包

```c
/* 创建安全消息 */
Ccc_SecureMessageType message;
uint8 payload[] = {0x01, 0x02, 0x03};  /* 命令数据 */

result = Ccc_CreateSecureMessage(
    CCC_MSG_SECURE_MESSAGE,
    payload,
    sizeof(payload),
    &message
);

/* 解析安全消息 */
uint8 receivedPayload[256];
uint32 receivedPayloadLength = sizeof(receivedPayload);
Ccc_MessageType msgType;

result = Ccc_ParseSecureMessage(
    &receivedMessage,
    receivedPayload,
    &receivedPayloadLength,
    &msgType
);

if (result == CCC_E_OK) {
    /* 处理消息 */
}
```

## 安全考虑

### 密钥管理
- 所有敏感密钥数据在使用后立即清除
- 支持安全存储区存储长期密钥
- 密钥定期更新机制

### 重放防护
- 使用序列号防止重放攻击
- 支持32位重放防护窗口
- 检测到重放攻击时立即关闭会话

### 会话管理
- 会话有效期限制 (5分钟)
- 定期心跳检测
- 异常情况自动关闭会话

## API参考

### 核心功能
- `Ccc_Init()` - 初始化CCC模块
- `Ccc_DeInit()` - 去初始化CCC模块
- `Ccc_GetCurrentMode()` - 获取当前操作模式
- `Ccc_GetSessionState()` - 获取会话状态

### 配对功能
- `Ccc_PairingStart()` - 开始配对流程
- `Ccc_PairingComplete()` - 完成配对流程

### 认证功能
- `Ccc_AuthenticationStart()` - 开始认证流程
- `Ccc_AuthenticationComplete()` - 完成认证流程
- `Ccc_VerifyCertificate()` - 验证证书

### 会话管理
- `Ccc_SessionEstablish()` - 建立安全会话
- `Ccc_SessionClose()` - 关闭安全会话

### 安全通信
- `Ccc_EncryptMessage()` - 加密消息
- `Ccc_DecryptMessage()` - 解密消息
- `Ccc_CreateSecureMessage()` - 创建安全消息包
- `Ccc_ParseSecureMessage()` - 解析安全消息包

### 辅助功能
- `Ccc_GenerateRandom()` - 生成随机数
- `Ccc_CalculateHash()` - 计算哈希值
- `Ccc_SignData()` - 签名数据
- `Ccc_VerifySignature()` - 验证签名

## 错误处理

示例中定义了以下错误码：

| 错误码 | 说明 |
|---------|------|
| CCC_E_OK | 成功 |
| CCC_E_NOT_INITIALIZED | 未初始化 |
| CCC_E_CRYPTO_FAILURE | 加密操作失败 |
| CCC_E_KEY_INVALID | 密钥无效 |
| CCC_E_CERT_INVALID | 证书无效 |
| CCC_E_SIGNATURE_INVALID | 签名无效 |
| CCC_E_AUTHENTICATION_FAILED | 认证失败 |
| CCC_E_SESSION_NOT_ESTABLISHED | 会话未建立 |
| CCC_E_REPLAY_DETECTED | 检测到重放攻击 |
| CCC_E_MESSAGE_INVALID | 消息无效 |

## 构建

```bash
# 添加源文件到构建系统
# 在CMakeLists.txt中添加:
# target_sources(your_target PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/examples/ccc_digital_key/src/*.c
# )

# 添加头文件路径
# target_include_directories(your_target PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/examples/ccc_digital_key/include
# )

# 构建
mkdir build && cd build
cmake ..
make
```

## 测试

示例包含以下测试场景：

1. **配对测试** - 验证配对流程完整性
2. **认证测试** - 验证挑战-响应机制
3. **会话测试** - 验证密钥派生和会话管理
4. **加解密测试** - 验证AES-GCM加解密
5. **重放防护测试** - 验证重放攻击检测
6. **边界条件测试** - 验证错误处理

## 注意事项

1. 本示例仅用于演示目的，生产环境中需要更完善的错误处理
2. 所有加密操作都通过CSM API调用，确保了硬件独立性
3. 实际应用中需要实现完整的X.509证书解析
4. 密钥存储应使用硬件安全存储区域 (HSM/TEE)
5. 随机数生成应使用硬件随机数生成器

## 参考资料

- [CCC Digital Key Specification 3.0](https://carconnectivity.org/)
- [NIST SP 800-56A - Recommendation for Pair-Wise Key Establishment Schemes](https://csrc.nist.gov/publications/detail/sp/800-56a/rev-3/final)
- [NIST SP 800-38D - Recommendation for Block Cipher Modes of Operation: GCM](https://csrc.nist.gov/publications/detail/sp/800-38d/final)
- [RFC 5869 - HMAC-based Extract-and-Expand Key Derivation Function (HKDF)](https://tools.ietf.org/html/rfc5869)

## 授权

Copyright (c) 2024-2026 上海予乐电子科技有限公司
保留所有权利。
