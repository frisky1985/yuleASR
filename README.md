# ETH-DDS Integration Framework

[![Version](https://img.shields.io/badge/version-2.0.0-blue.svg)](VERSION)
[![License](https://img.shields.io/badge/license-Apache%202.0-green.svg)](LICENSE)
[![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

**ETH-DDS Integration Framework** 是一个面向汽车和工业自动化领域的高性能数据通信框架，实现了Ethernet驱动、DDS中间件、TSN时间敏感网络以及AUTOSAR标准的深度集成。

## 特性

- **多层次架构**: 支持从底层硬件驱动到高层应用的完整软件栈
- **DDS标准兼容**: 完整实现OMG DDS规范，支持RTPS协议
- **TSN支持**: 内建时间敏感网络功能，包括gPTP、TAS、CBS、Frame Preemption等
- **AUTOSAR集成**: 支持AUTOSAR Classic RTE和Adaptive ara::com
- **高性能**: 低延迟、高吞吐量设计，适合确定性传输场景
- **安全性**: DDS-Security扩展支持，包括认证、加密和访问控制
- **多平台**: 支持Linux、FreeRTOS和裸机平台

## 快速开始

### 构建要求

- CMake >= 3.14
- C编译器 (GCC >= 7.0, Clang >= 6.0, 或 MSVC >= 2017)
- C++编译器 (可选)

### 构建步骤

```bash
# 克隆代码
git clone https://github.com/eth-dds/eth-dds-integration.git
cd eth-dds-integration

# 创建构建目录
mkdir build && cd build

# 配置（启用所有功能）
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DENABLE_TSN=ON \
         -DENABLE_AUTOSAR_CLASSIC=ON \
         -DENABLE_AUTOSAR_ADAPTIVE=ON \
         -DENABLE_DDS_SECURITY=ON

# 编译
make -j$(nproc)

# 运行测试
make test

# 安装
sudo make install
```

### 基本用法

```c
#include "eth_dds.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    // 初始化配置
    eth_dds_config_t config = ETH_DDS_CONFIG_DEFAULT;
    config.flags = ETH_DDS_INIT_ALL;
    
    // 初始化库
    eth_dds_ret_t ret = eth_dds_init(&config);
    if (ETH_DDS_IS_ERROR(ret)) {
        fprintf(stderr, "初始化失败: %s\n", eth_dds_get_last_error());
        return 1;
    }
    
    printf("ETH-DDS 版本: %s\n", eth_dds_get_version(NULL, NULL, NULL));
    
    // 使用DDS功能...
    
    // 清理资源
    eth_dds_deinit();
    return 0;
}
```

## 文档

- [FEATURES.md](docs/FEATURES.md) - 完整功能清单
- [ARCHITECTURE.md](docs/ARCHITECTURE.md) - 架构设计文档
- [INTEGRATION_GUIDE.md](docs/INTEGRATION_GUIDE.md) - 集成指南
- [API Reference](docs/API.md) - API参考手册

### 设计文档

| 文档 | 内容 |
|--------|--------|
| [01-architecture-overview.md](01-architecture-overview.md) | 架构总览和分层设计 |
| [02-data-flow-design.md](02-data-flow-design.md) | 数据流设计和QoS策略 |
| [03-config-sync-mechanism.md](03-config-sync-mechanism.md) | 配置同步机制 |
| [04-deployment-architecture.md](04-deployment-architecture.md) | 部署架构和网络拓扑 |

## 项目结构

```
eth-dds-integration/
├── include/           # 公共头文件
│   └── eth_dds.h       # 统一API入口
├── src/               # 源代码
│   ├── common/         # 通用组件
│   ├── ethernet/       # 以太网驱动
│   ├── dds/            # DDS实现
│   ├── transport/      # 传输层
│   ├── tsn/            # TSN功能
│   ├── autosar/        # AUTOSAR集成
│   └── platform/       # 平台适配层
├── tests/             # 测试套件
│   ├── unit/           # 单元测试
│   ├── integration/    # 整合测试
│   ├── system/         # 系统测试
│   └── performance/    # 性能测试
├── examples/          # 示例程序
├── docs/              # 文档
├── scripts/           # 部署脚本
└── tools/             # 开发工具
```

## 功能模块

| 模块 | 描述 | 状态 |
|--------|--------|-------|
| **Common** | 通用类型、工具函数、日志系统 | ✅ 已完成 |
| **Ethernet** | MAC驱动、DMA管理、汽车以太网扩展 | ✅ 已完成 |
| **DDS Core** | RTPS协议、发现机制、状态管理 | ✅ 已完成 |
| **DDS Security** | 认证、加密、访问控制 | ✅ 已完成 |
| **DDS Advanced** | 所有权、持久化、内容过滤 | ✅ 已完成 |
| **Transport** | UDP/TCP/SHM 传输适配 | ✅ 已完成 |
| **TSN** | gPTP、TAS、CBS、Frame Preemption | ✅ 已完成 |
| **AUTOSAR Classic** | RTE集成、E2E保护、ARXML解析 | ✅ 已完成 |
| **AUTOSAR Adaptive** | ara::com DDS绑定 | ✅ 已完成 |

## 贡献

我们欢迎社区贡献！请参阅 [CONTRIBUTING.md](CONTRIBUTING.md) 了解如何参与项目。

## 许可

本项目采用 Apache License 2.0 开源许可。详见 [LICENSE](LICENSE) 文件。

## 联系我们

- 邮箱: support@eth-dds.org
- 问题跟踪: [GitHub Issues](https://github.com/eth-dds/eth-dds-integration/issues)
- 讨论: [GitHub Discussions](https://github.com/eth-dds/eth-dds-integration/discussions)

---

**版本**: 2.0.0 | **最后更新**: 2026-04-24
