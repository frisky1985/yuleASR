# YuleTech Third Party Libraries

本目录包含YuleTech AUTOSAR BSW平台所使用的第三方库。

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

| 库 | 版本 | 说明 |
|------|-------|-------|
| mbedTLS | 2.28.8 LTS | 长期支持版本，稳定可靠 |
| Yule Adapter | 1.0 | 自研适配层 |

## 授权

- **mbedTLS**: Apache-2.0 OR GPL-2.0-or-later
- **Yule Adapter**: MIT
