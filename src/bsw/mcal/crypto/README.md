# Crypto (硬件加密驱动) 模块

## 模块概述

Crypto 模块是 AUTOSAR MCAL (Microcontroller Driver Layer) 层的硬件加密驱动，提供对 MCU 硬件加密加速器的抽象接口。

## 功能特性

### 1. 硬件加密操作
- **AES 加密/解密**: 支持 ECB, CBC, CTR, GCM 模式
- **SHA-256 哈希**: 硬件加速 SHA-256 计算
- **HMAC**: 硬件 HMAC-SHA256 计算
- **RSA**: 硬件 RSA 签名/验证 (1024/2048/4096位)
- **真随机数生成器 (TRNG)**: 硬件熵源

### 2. 密钥管理
- 密钥元素存储 (Key Element Storage)
- 密钥导入/导出
- 密钥生成
- 密钥派生 (Key Derivation)
- 密钥复制
- 密钥验证

### 3. 作业管理
- 同步/异步操作支持
- 作业队列管理
- 作业取消
- 优先级支持

### 4. 与上层集成
- 与 CryIf (Crypto Interface) 集成
- 通过 Csm (Crypto Services Manager) 提供服务
- 回调函数支持异步通知

## 文件结构

```
src/bsw/mcal/crypto/
├── include/
│   ├── Crypto.h          # 公共头文件 (API声明)
│   ├── Crypto_Cfg.h      # 配置文件
│   └── Crypto_MemMap.h   # 内存映射
├── src/
│   ├── Crypto.c          # 主实现文件
│   └── Crypto_Cfg.c      # 配置实现
└── README.md             # 本文件
```

## API 分类

### 1. 生命周期管理
```c
void Crypto_Init(const Crypto_ConfigType* ConfigPtr);
void Crypto_DeInit(void);
void Crypto_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

### 2. 作业处理
```c
Std_ReturnType Crypto_ProcessJob(Crypto_DriverObjectIdType objectId, Crypto_JobType* job);
Std_ReturnType Crypto_CancelJob(Crypto_DriverObjectIdType objectId, const Crypto_JobType* job);
```

### 3. 密钥管理
```c
Std_ReturnType Crypto_KeyElementSet(Crypto_KeyIdType keyId, Crypto_KeyElementIdType elementId, 
                                     const uint8* keyPtr, uint32 keyLength);
Std_ReturnType Crypto_KeyElementGet(Crypto_KeyIdType keyId, Crypto_KeyElementIdType elementId,
                                     uint8* keyPtr, uint32* keyLengthPtr);
Std_ReturnType Crypto_KeyGenerate(Crypto_KeyIdType keyId);
Std_ReturnType Crypto_KeyDerive(Crypto_KeyIdType sourceKeyId, Crypto_KeyIdType targetKeyId);
Std_ReturnType Crypto_KeyValidSet(Crypto_KeyIdType keyId);
```

### 4. 硬件抽象接口
```c
Std_ReturnType Crypto_HwAesEncrypt(...);
Std_ReturnType Crypto_HwAesDecrypt(...);
Std_ReturnType Crypto_HwHashSha256(...);
Std_ReturnType Crypto_HwHmacGenerate(...);
Std_ReturnType Crypto_HwHmacVerify(...);
Std_ReturnType Crypto_HwRandomGenerate(uint8* resultPtr, uint32 resultLength);
```

## 配置选项

在 `Crypto_Cfg.h` 中可配置:

| 配置项 | 说明 | 默认值 |
|--------|------|--------|
| CRYPTO_DEV_ERROR_DETECT | 开发错误检测 | STD_ON |
| CRYPTO_VERSION_INFO_API | 版本信息 API | STD_ON |
| CRYPTO_HW_ACCELERATION_ENABLED | 硬件加速 | STD_ON |
| CRYPTO_AES_HW_SUPPORT | AES 硬件支持 | STD_ON |
| CRYPTO_SHA256_HW_SUPPORT | SHA-256 硬件支持 | STD_ON |
| CRYPTO_HMAC_HW_SUPPORT | HMAC 硬件支持 | STD_ON |
| CRYPTO_RSA_HW_SUPPORT | RSA 硬件支持 | STD_ON |
| CRYPTO_TRNG_HW_SUPPORT | TRNG 支持 | STD_ON |
| CRYPTO_ASYNC_OPERATION_SUPPORT | 异步操作 | STD_ON |

## 使用示例

### 1. 初始化 Crypto 驱动
```c
#include "Crypto.h"

void Crypto_InitExample(void)
{
    Crypto_Init(&Crypto_Config);
}
```

### 2. AES-256 CBC 加密
```c
Std_ReturnType AesEncryptExample(void)
{
    uint8 plaintext[] = "Hello, World!!!!";  /* 16 bytes */
    uint8 ciphertext[32];
    uint32 cipherLen = sizeof(ciphertext);
    uint8 iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    
    /* Set AES key */
    uint8 key[32] = { /* 256-bit key */ };
    Crypto_KeyElementSet(CRYPTO_KEY_ID_AES_MASTER, CRYPTO_KEY_ELEMENT_AES_KEY, key, 32);
    Crypto_KeyValidSet(CRYPTO_KEY_ID_AES_MASTER);
    
    /* Encrypt */
    Crypto_AlgorithmInfoType algo = {
        .family = CRYPTO_ALGOFAM_AES,
        .mode = CRYPTO_ALGOMODE_CBC,
        .keyLength = 256,
        .ivLength = 16,
        .authTagLength = 0
    };
    
    return Crypto_HwAesEncrypt(CRYPTO_CHANNEL_AES_0, CRYPTO_OPERATIONMODE_SINGLECALL,
                                &algo, CRYPTO_KEY_ID_AES_MASTER, iv,
                                plaintext, sizeof(plaintext),
                                ciphertext, &cipherLen);
}
```

### 3. SHA-256 哈希
```c
Std_ReturnType HashExample(void)
{
    uint8 data[] = "Hello, World!";
    uint8 hash[32];
    uint32 hashLen = sizeof(hash);
    
    return Crypto_HwHashSha256(CRYPTO_CHANNEL_HASH_0, CRYPTO_OPERATIONMODE_SINGLECALL,
                                data, sizeof(data) - 1, hash, &hashLen);
}
```

### 4. HMAC 生成
```c
Std_ReturnType HmacExample(void)
{
    uint8 data[] = "Message to authenticate";
    uint8 mac[32];
    uint32 macLen = sizeof(mac);
    
    /* Set HMAC key */
    uint8 hmacKey[32] = { /* HMAC key */ };
    Crypto_KeyElementSet(CRYPTO_KEY_ID_HMAC_MASTER, CRYPTO_KEY_ELEMENT_HMAC_KEY, hmacKey, 32);
    Crypto_KeyValidSet(CRYPTO_KEY_ID_HMAC_MASTER);
    
    return Crypto_HwHmacGenerate(CRYPTO_CHANNEL_HMAC_0, CRYPTO_OPERATIONMODE_SINGLECALL,
                                  CRYPTO_KEY_ID_HMAC_MASTER,
                                  data, sizeof(data) - 1, mac, &macLen);
}
```

### 5. 随机数生成
```c
Std_ReturnType RandomExample(void)
{
    uint8 random[32];
    return Crypto_HwRandomGenerate(random, sizeof(random));
}
```

## 硬件适配

本模块是硬件抽象层，具体硬件实现需要根据不同 MCU 进行适配:

### 支持的 MCU 系列
- STM32: 使用 CRYP 和 HASH 外设
- NXP: 使用 DCP (Data Co-Processor) 或 CAAM
- Infineon: 使用 HSM (Hardware Security Module)
- Renesas: 使用 RSIP (Renesas Security IP)

### 适配接口
需要修改的硬件相关代码位于 `Crypto.c` 中标记为 `TODO` 的部分:
1. `Crypto_HwInitialize()` - 初始化硬件时钟和寄存器
2. `Crypto_HwDeinitialize()` - 关闭硬件
3. `Crypto_HwWaitReady()` - 等待硬件就绪
4. `Crypto_HwRegisters[]` - 替换为实际硬件寄存器访问

## 注意事项

1. **安全性**: 密钥数据存储在安全 RAM 区域，禁止外设 DMA 访问
2. **中断**: 异步操作需要配置中断服务例程
3. **时序**: 某些操作 (如 RSA) 可能需要较长处理时间
4. **熵源**: TRNG 需要足够的熵才能生成安全的随机数

## 版本历史

| 版本 | 日期 | 修改内容 |
|------|------|----------|
| 1.0.0 | 2026-04-30 | 初始版本，实现完整 AUTOSAR Crypto 驱动接口 |

## 参考文档

- AUTOSAR_SWS_CryptoDriver.pdf (R22-11)
- AUTOSAR_SWS_CryptoInterface.pdf (R22-11)
- AUTOSAR_SWS_CryptoServicesManager.pdf (R22-11)
