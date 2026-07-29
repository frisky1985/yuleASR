# YuleTech Third Party Libraries

本目录包含YuleTech AUTOSAR BSW平台所使用的第三方库。

## 目录结构

```
third_party/
├── crypto/               # 加密库
│   ├── mbedtls/       # mbedTLS (AutoSAR适配版)
│   ├── aes_modes/     # AES模式 (CBC/CTR/GCM/CCM等)
│   ├── blake2/        # Blake2哈希算法
│   └── hash/          # SHA系列哈希算法
├── test_frameworks/    # 测试框架
│   ├── unity/         # Unity单元测试框架
│   └── gtest/         # Google Test框架
├── network/            # 网络协议栈 (预留)
│   ├── lwip/          # lwIP协议栈
│   └── tls/           # TLS/SSL协议
├── rtos/               # 实时操作系统 (预留)
│   └── freertos/      # FreeRTOS
├── mbedtls/           # 完整mbedTLS库 (Git Submodule)
└── yule-mbedtls-adapter/  # YuleTech mbedTLS适配层
```

## 快速开始

```bash
# 克隆仓库（包含submodule）
git clone --recurse-submodules https://github.com/frisky1985/yuleASR.git

# 如果已经克隆，但缺少submodule
git submodule update --init --recursive

# 更新submodule到指定版本
cd third_party/mbedtls
git checkout v2.28.8
cd ../..
git add third_party/mbedtls
git commit -m "更新mbedtls版本"
```

## 库列表

### Crypto - 加密库

#### 1. AES Modes (AES模式库)

**位置**: `third_party/crypto/aes_modes/`

**描述**: AES加密模式实现，支持多种工作模式

**文件**:
- `include/aes_modes.h` - 头文件
- `src/aes_*.c` - 各种模式实现 (CBC, CTR, GCM, CCM, ECB, CFB, OFB)
- `src/aes_core.c` - 核心AES实现
- `src/aes_autosar.c` - AutoSAR适配层
- `README.md` - 说明文档

**特性**:
- 支持CBC, CTR, GCM, CCM, ECB, CFB, OFB等模式
- 符合AutoSAR Crypto模块规范
- 带有完整单元测试

#### 2. Blake2 (Blake2哈希库)

**位置**: `third_party/crypto/blake2/`

**描述**: Blake2哈希算法实现

**文件**:
- `include/blake2.h` - 头文件
- `src/blake2b.c` - Blake2b实现
- `src/blake2s.c` - Blake2s实现
- `src/blake2_autosar.c` - AutoSAR适配层
- `README.md` - 说明文档

**特性**:
- 支持Blake2b和Blake2s两种变体
- 高性能哈希算法
- 符合AutoSAR Crypto模块规范

#### 3. Hash (SHA哈希库)

**位置**: `third_party/crypto/hash/`

**描述**: SHA系列哈希算法实现

**文件**:
- `include/hash_algos.h` - 头文件
- `src/sha1.c` - SHA-1实现
- `src/sha224.c`, `src/sha384.c`, `src/sha512.c` - SHA-2实现
- `src/sha3_*.c` - SHA-3实现
- `src/hash_autosar.c` - AutoSAR适配层

**特性**:
- 支持SHA-1, SHA-224, SHA-256, SHA-384, SHA-512
- 支持SHA3-224, SHA3-256, SHA3-384, SHA3-512
- 符合AutoSAR Crypto模块规范

#### 4. mbedTLS (AutoSAR适配版)

**位置**: `third_party/crypto/mbedtls/`

**描述**: 专为AutoSAR优化的轻量级mbedTLS配置

**文件**:
- `include/mbedtls_config.h` - mbedTLS配置
- `include/mbedtls_wrapper.h` - 定制包装器
- `src/mbedtls_wrapper.c` - 实现
- `src/mbedtls_hardware.c` - 硬件适配

### Test Frameworks - 测试框架

#### 5. Unity (单元测试框架)

**位置**: `third_party/test_frameworks/unity/`

**来源**: `src/micro-dds/tests/unity/`

**描述**: 轻量级C语言单元测试框架

**文件**:
- `unity.h` - 头文件
- `unity.c` - 实现

**特性**:
- 适合嵌入式系统测试
- 轻量级，代码小
- 常用于micro-DDS等模块测试

### mbedTLS (v2.28.8 LTS)

**描述**: 轻量级TLS/SSL库，用于MQTT模块的TLS/mTLS安全通信

**管理方式**: Git Submodule

**位置**: `third_party/mbedtls/` -> https://github.com/Mbed-TLS/mbedtls.git

**版本**: v2.28.8 (`5a764e5555`)

**配置**:
- 使用自定义配置: `mbedtls/configs/config-yule-autosar.h`
- 针对AUTOSAR嵌入式系统优化
- 静态内存分配（适配MemMap）

**特性**:
- TLS 1.2支持
- 基于ECC的证书（更小代码体积）
- X.509证书解析
- mTLS双向认证
- 会话恢复

### YuleTech mbedTLS Adapter

**描述**: AUTOSAR平台适配层，连接mbedTLS与AUTOSAR BSW

**文件**:
- `yule-mbedtls-adapter/include/yule_mbedtls_adapter.h` - 头文件
- `yule-mbedtls-adapter/src/yule_mbedtls_adapter.c` - 实现
- `yule-mbedtls-adapter/CMakeLists.txt` - 构建配置

**适配功能**:
- 内存分配适配 (MemMap段映射)
- 随机数生成适配 (Trng模块)
- 时间戳服务 (StbM模块)
- 错误日志输出 (Det模块)

## 使用说明

### 在CMake项目中使用

```cmake
# 启用TLS支持
set(MQTT_SUPPORT_TLS ON)

# 使用子模块mbedTLS（默认）
set(MQTT_MBEDTLS_FROM_SUBMODULE ON)

add_subdirectory(third_party/yule-mbedtls-adapter)
add_subdirectory(src/bsw/services/mqtt)
```

### 手动构建mbedTLS

```bash
cd third_party/mbedtls
make CFLAGS="-DMBEDTLS_CONFIG_FILE='<config-yule-autosar.h>' \
           -I./configs \
           -Os \
           -ffunction-sections \
           -fdata-sections"
```

## 版本信息

| 库 | 版本 | 说明 | 位置 |
|:------|:-------:|:-------|:------|
| AES Modes | 1.0 | AES加密模式库 | third_party/crypto/aes_modes/ |
| Blake2 | 1.0 | Blake2哈希算法 | third_party/crypto/blake2/ |
| Hash | 1.0 | SHA系列哈希 | third_party/crypto/hash/ |
| mbedTLS (适配版) | 定制 | AutoSAR适配版本 | third_party/crypto/mbedtls/ |
| Unity | 2.x | 单元测试框架 | third_party/test_frameworks/unity/ |
| mbedTLS | 2.28.8 LTS | 完整库 (Git Submodule) | third_party/mbedtls/ |
| Yule Adapter | 1.0 | 自研适配层 | third_party/yule-mbedtls-adapter/ |

## 授权

| 库 | 授权 |
|:------|:------|
| AES Modes | MIT |
| Blake2 | MIT/CC0 |
| Hash | MIT |
| Unity | MIT |
| mbedTLS | Apache-2.0 OR GPL-2.0-or-later |
| Yule Adapter | MIT |

---

*YuleTech AutoSAR BSW Platform - Third Party Libraries*
