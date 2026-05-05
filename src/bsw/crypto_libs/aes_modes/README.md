# AES算法模式库

YuleTech AutoSAR项目的完整AES算法实现，支持所有常用AES加密模式。

## 功能特性

- 完整的AES实现 (128/192/256位密钥)
- 支持所有标准模式：ECB、CBC、CFB、OFB、CTR、GCM、CCM
- PKCS#7填充支持
- 流式API支持 (Stream API)
- AUTOSAR标准API兼容
- 硬件加速支持 (S32K312 HSM)
- 高性能优化实现

## 支持的AES模式

| 模式 | 描述 | 安全性 | 推荐用途 |
|------|------|--------|---------|
| ECB  | 电子密码本 | 低 | 仅兼容性 |
| CBC  | 密码块链接 | 中 | 通用加密 |
| CFB  | 密码反馈 | 中 | 流密码传输 |
| OFB  | 输出反馈 | 中 | 并行加密 |
| CTR  | 计数器 | 高 | 高性能并行加密 |
| GCM  | Galois计数器 | 高 | 认证加密 (AEAD) |
| CCM  | 计数器+CBC-MAC | 高 | 认证加密 (AEAD) |

## 目录结构

```
aes_modes/
├── include/
│   └── aes_modes.h          # 主头文件
├── src/
│   ├── aes_core.c           # 核心AES算法
│   ├── aes_ecb.c            # ECB模式
│   ├── aes_cbc.c            # CBC模式
│   ├── aes_cfb.c            # CFB模式
│   ├── aes_ofb.c            # OFB模式
│   ├── aes_ctr.c            # CTR模式
│   ├── aes_gcm.c            # GCM模式 (AEAD)
│   ├── aes_ccm.c            # CCM模式 (AEAD)
│   └── aes_autosar.c        # AUTOSAR适配层
├── tests/
│   ├── test_aes_modes.c     # 测试套件
│   └── CMakeLists.txt
├── CMakeLists.txt
└── README.md
```

## API使用示例

### 基本加密/解密

```c
#include "aes_modes.h"

uint8 key[16] = {0x2b, 0x7e, 0x15, 0x16, ...};  // 128-bit key
uint8 iv[16] = {0x00, 0x01, 0x02, 0x03, ...};    // IV
uint8 plaintext[] = "Hello, World!!!!";          // 16 bytes
uint8 ciphertext[32];
uint8 decrypted[32];
uint32 cipherLen, plainLen;

Aes_ContextType ctx;

// CBC模式加密
Aes_Init(&ctx, key, 16);
Aes_CbcEncrypt(&ctx, iv, plaintext, 16, ciphertext, &cipherLen);

// CBC模式解密
Aes_CbcDecrypt(&ctx, iv, ciphertext, cipherLen, decrypted, &plainLen);

// 清理上下文
Aes_Clear(&ctx);
```

### GCM AEAD加密

```c
uint8 key[32] = {...};                           // 256-bit key
uint8 iv[12] = {...};                            // 96-bit IV
uint8 aad[] = "additional data";                 // 附加认证数据
uint8 plaintext[] = "secret message";
uint8 ciphertext[32];
uint8 tag[16];                                   // 认证标签

Aes_ContextType ctx;
Aes_Init(&ctx, key, 32);

// 加密并生成认证标签
Aes_GcmEncrypt(&ctx, iv, 12, aad, sizeof(aad)-1,
               plaintext, sizeof(plaintext)-1,
               ciphertext, tag, 16);

// 解密并验证
uint32 plainLen;
Crypto_VerifyResultType verifyResult;
Aes_GcmDecrypt(&ctx, iv, 12, aad, sizeof(aad)-1,
               ciphertext, sizeof(plaintext)-1,
               tag, 16, decrypted, &plainLen);
```

### 流式API

```c
// CBC流式加密
Aes_CbcEncryptStart(&ctx, iv);
Aes_CbcEncryptUpdate(&ctx, data1, len1, out1, &outLen1);
Aes_CbcEncryptUpdate(&ctx, data2, len2, out2, &outLen2);
Aes_CbcEncryptFinish(&ctx, lastData, lastLen, finalOut, &finalLen);
```

### AUTOSAR API

```c
// 通过Crypto Driver API调用
Crypto_JobType job;
// ... 配置作业结构体 ...
Crypto_ProcessJob(0, &job);
```

## 安装

```bash
mkdir build && cd build
cmake ..
make
make test
```

## 平台支持

- **CPU架构**: ARM Cortex-M7 (S32K312)
- **编译器**: GCC ARM工具链
- **标准**: AUTOSAR 4.7.0

## 参考标准

- FIPS-197: Advanced Encryption Standard (AES)
- NIST SP 800-38A: Recommendation for Block Cipher Modes of Operation
- NIST SP 800-38C: Recommendation for Block Cipher Modes of Operation: CCM Mode
- NIST SP 800-38D: Recommendation for Block Cipher Modes of Operation: GCM
- RFC 3610: Counter with CBC-MAC (CCM)

## 版本历史

### v1.0.0 (2026-05-01)
- 初始发布
- 完整支持AES-128/192/256
- 实现所有标准模式 (ECB, CBC, CFB, OFB, CTR, GCM, CCM)
- AUTOSAR适配层
- 硬件加速接口

## 版权

Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
