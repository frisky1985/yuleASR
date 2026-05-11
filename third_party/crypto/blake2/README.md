# BLAKE2 Hash Algorithm Library

## 概述

BLAKE2是高性能哈希算法，比SHA-256更快、比MD5更安全。这个库为YuleTech AutoSAR项目提供了BLAKE2算法的完整实现。

## 特性

- **BLAKE2b**: 64位平台优化，最大哈希长度512位(64字节)
- **BLAKE2s**: 32位平台优化，最大哈希长度256位(32字节)
- **密钥化哈希**: 支持带密钥的MAC生成
- **增量哈希**: 支持分块处理大数据
- **纯C语言**: 无依赖，可移植性强
- **MISRA-C合规**: 符合汽车软件开发标准

## 目录结构

```
blake2/
├── include/
│   └── blake2.h          # 公共头文件
├── src/
│   ├── blake2b.c         # BLAKE2b实现
│   ├── blake2s.c         # BLAKE2s实现
│   └── blake2_autosar.c  # AUTOSAR适配层
├── tests/
│   ├── test_blake2.c     # 单元测试
│   └── CMakeLists.txt    # 测试构建配置
├── CMakeLists.txt       # 构建配置
└── README.md            # 本文件
```

## 性能数据

### 与其他算法对比

| 算法 | 输出长度 | 速度(相对于SHA-256) | 安全性 |
|--------|---------|---------------------|--------|
| MD5 | 128-bit | ~150% | 弱 |
| SHA-1 | 160-bit | ~120% | 较弱 |
| SHA-256 | 256-bit | 100% (基准) | 强 |
| **BLAKE2s** | 256-bit | **~120%** | **强** |
| SHA-3-256 | 256-bit | ~30% | 强 |
| SHA-512 | 512-bit | ~90% | 很强 |
| **BLAKE2b** | 512-bit | **~130%** | **很强** |
| SHA-3-512 | 512-bit | ~35% | 很强 |

*注: 数据基于x86-64平台测试，实际性能因平台而异*

### 轮询次数与速度

- **BLAKE2b**: 12轮询操作
- **BLAKE2s**: 10轮询操作
- 每轮处理128字节(BLAKE2b)或64字节(BLAKE2s)

## API使用说明

### 基本哈希

```c
#include "blake2.h"

// BLAKE2b-512哈希
uint8 out[64];
uint8 data[] = "Hello, World!";
blake2b(out, data, sizeof(data)-1, NULL, 0, 64);

// BLAKE2s-256哈希
uint8 out[32];
blake2s(out, data, sizeof(data)-1, NULL, 0, 32);
```

### 带密钥的MAC

```c
uint8 key[32] = { /* 32字节密钥 */ };
uint8 data[] = "Message to authenticate";
uint8 mac[64];

// 使用BLAKE2b生成MAC
blake2b(mac, data, sizeof(data)-1, key, 32, 64);
```

### 增量哈希

```c
blake2b_state_t state;
uint8 out[64];

// 初始化
blake2b_init(&state, 64);

// 更新数据(可多次调用)
blake2b_update(&state, chunk1, len1);
blake2b_update(&state, chunk2, len2);

// 完成哈希
blake2b_final(&state, out, 64);
```

## AUTOSAR集成

### CSM配置示例

```c
#include "CryptoStack_Types.h"

// 配置BLAKE2b哈希服务
const Crypto_JobPrimitiveInfoType Blake2b_JobInfo = {
    .callbackId = 0,
    .algorithm = {
        .family = CRYPTO_ALGOFAM_BLAKE2B,
        .mode = CRYPTO_ALGOMODE_NOT_SET,
        .classType = CRYPTO_ALGOCLASS_HASH,
        .keyLength = 0,
        .curve = CRYPTO_ECC_CURVE_NONE,
        .secondaryFamily = CRYPTO_ALGOFAM_NOT_SET
    },
    .service = CRYPTO_SERVICE_HASH,
    .processingType = CRYPTO_PROCESSING_SYNC,
    .callbackUpdateNotification = FALSE
};
```

### 使用Crypto Driver API

```c
#include "Crypto.h"

// 单次调用哈希
uint8 digest[64];
Std_ReturnType result = Crypto_Blake2b(
    data, dataLength,    // 输入数据
    NULL, 0,             // 无密钥
    64,                  // 输出长度
    digest               // 输出缓冲区
);

// 增量哈希
Crypto_Blake2b_Start(jobId, NULL, 0, 64);
Crypto_Blake2b_Update(jobId, chunk1, len1);
Crypto_Blake2b_Update(jobId, chunk2, len2);
Crypto_Blake2b_Finish(jobId, digest, &digestLen);
```

## 构建

### 使用CMake

```bash
mkdir build && cd build
cmake .. -DBLAKE2_BUILD_TESTS=ON
make
make test
```

### 直接编译

```bash
gcc -c -Iinclude -Wall -O2 src/blake2b.c -o blake2b.o
gcc -c -Iinclude -Wall -O2 src/blake2s.c -o blake2s.o
ar rcs libblake2.a blake2b.o blake2s.o
```

## 测试

### 运行单元测试

```bash
cd build
./src/bsw/crypto_libs/blake2/tests/test_blake2
```

### 测试覆盖

- RFC 7693标准测试向量
- 空输入测试
- 带密钥哈希测试
- 增量哈希测试
- 错误处理测试
- 边界条件测试

## 安全说明

1. **密钥清除**: 库内部使用安全内存清除，确保密钥不会泄露
2. **时间一致性**: 实现采用常数时间操作，防止时间攻击
3. **内存安全**: 所有敏感数据都在使用后立即清除

## 参考资料

- RFC 7693: The BLAKE2 Cryptographic Hash and Message Authentication Code (MAC)
- [BLAKE2官方网站](https://www.blake2.net/)
- AUTOSAR Crypto Stack规范

## 版本历史

| 版本 | 日期 | 说明 |
|-------|------|------|
| 1.0.0 | 2026-05-01 | 初始发布 |

## 联系我们

上海予乐电子科技有限公司  
YuleTech AutoSAR Team
