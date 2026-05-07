# MQTT TLS 模块单元测试

## 概述

本目录包含 MQTT TLS 模块的单元测试，测试以下组件:
- `Mqtt_Tls.c` - TLS/SSL/mTLS 安全层
- `Mqtt_CertMgr.c` - 证书管理模块

## 测试内容

### Mqtt_Tls 测试
1. **初始化测试**
   - 正常初始化
   - 重复初始化

2. **上下文管理测试**
   - 创建有效上下文
   - 无效参数处理

3. **证书验证测试**
   - 有效证书验证
   - 错误处理

4. **版本和密码套件查询**
   - TLS版本获取
   - 密码套件查询

5. **错误处理**
   - 错误码翻译

### Mqtt_CertMgr 测试
1. **初始化测试**
   - 有效配置
   - 默认配置

2. **证书存储测试**
   - 添加证书
   - 更新证书
   - 删除证书

3. **证书查询测试**
   - 获取证书信息
   - 列表证书

4. **证书验证测试**
   - 单证书验证
   - 证书链验证

## 构建和运行

### 要求
- CMake 3.10+
- GCC 或 Clang
- Unity 测试框架

### 构建步骤

```bash
cd src/bsw/services/mqtt/test
chmod +x build.sh
./build.sh
```

或手动构建:

```bash
mkdir build
cd build
cmake ..
make
./mqtt_tls_tests
```

### 覆盖率报告

```bash
cd build
make coverage
```
覆盖率报告将生成在 `coverage_report` 目录。

## 测试结构

```
test/
├── CMakeLists.txt      # CMake配置
├── test_mqtt_tls.c     # 测试源文件
├── build.sh            # 构建脚本
├── README.md           # 本文件
└── stubs/              # 测试桩文件
    ├── Std_Types.h       # 标准类型桩
    ├── TcpIp.h           # TcpIp模块桩
    └── Mqtt.h            # MQTT模块桩
```

## 注意事项

- 当前实现为模拟实现，未真正集成mbedTLS
- 实际项目中需要安装mbedTLS库并取消相关注释
- 证书解析功能需要真实的X.509解析器
