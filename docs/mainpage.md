ETH-DDS Integration Framework 文档主页 {#mainpage}
=========================================

[English Version](index_en.md)

欢迎使用 ETH-DDS Integration Framework
======================================

ETH-DDS Integration Framework 是一个面向汽车电子领域的DDS中间件解决方案，提供完整的AUTOSAR集成、实时以太网传输和安全通信功能。

## 核心特性

- **DDS核心协议**: 完整的OMG DDS标准实现，支持发布/订阅、QoS、内容过滤等
- **RTPS协议**: 实时发布订阅协议实现，支持自动发现和可靠传输
- **安全扩展**: 基于DDS-Security规范的认证、加密、访问控制
- **AUTOSAR集成**: 支持Classic和Adaptive AUTOSAR RTE集成
- **诊断栈**: 完整的UDS诊断协议栈（DCM/DEM/DoIP）
- **SecOC安全**: 安全车载通信，支持MAC生成/验证、密钥管理
- **TSN支持**: 时间敏感网络，支持802.1Qbv、CBS、SRP等
- **OTA更新**: 空中软件更新，支持安全下载和回滚
- **ROS2桥接**: ROS2 RMW层集成，支持机器人应用

## 架构概览

```
┌─────────────────────────────────────────────────────────────────┐
│                     应用层 (Application)                      │
│  AUTOSAR Classic RTE │ AUTOSAR Adaptive │ ROS2 │ Custom Apps  │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────┐
│                    DDS中间件层 (DDS Middleware)              │
│  DDS Core │ RTPS Protocol │ DDS-Security │ Advanced Features  │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────┐
│                    传输层 (Transport)                         │
│  UDP/TCP │ Shared Memory │ TSN │ Transport Manager            │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────┐
│                    平台适配层 (Platform)                      │
│  Linux │ FreeRTOS │ Bare-metal │ Hardware Abstraction          │
└─────────────────────────────────────────────────────────────────┘
```

## 快速开始

- @subpage quick_start "快速入门指南" - 环境搭建和第一个DDS应用
- @subpage concepts "概念指南" - DDS、AUTOSAR、安全概念
- @subpage api_reference "API参考" - 按模块组织的API文档
- @subpage configuration "配置指南" - XML/JSON配置和工具使用
- @subpage debugging "调试指南" - 日志、跟踪、常见问题
- @subpage deployment "部署指南" - 交叉编译、烧录
- @subpage security_guide "安全指南" - 密钥管理、证书配置

## 模块文档

### DDS核心模块
- @ref dds_core "DDS Core" - 核心DDS功能
- @ref rtps "RTPS Protocol" - 实时发布订阅协议
- @ref dds_security "DDS-Security" - 安全扩展

### 诊断与安全
- @ref diagnostics "Diagnostics Stack" - UDS诊断（DCM/DEM/DoIP）
- @ref secoc "SecOC Stack" - 安全车载通信
- @ref crypto "Crypto Stack" - 加密服务（CSM/CryIf/KeyM）

### 网络与传输
- @ref transport "Transport Layer" - 传输抽象层
- @ref tsn "TSN Stack" - 时间敏感网络
- @ref ethernet "Ethernet Driver" - 以太网驱动

### 其他模块
- @ref ota "OTA Manager" - 空中软件更新
- @ref nvm "NVM Stack" - 非易失性存储管理
- @ref ros2_bridge "ROS2 Bridge" - ROS2集成

## 示例代码

```c
// 简单的DDS发布者示例
#include "dds/dds.h"

int main(int argc, char **argv)
{
    dds_entity_t participant;
    dds_entity_t topic;
    dds_entity_t writer;
    
    // 创建参与者
    participant = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);
    
    // 创建主题
    topic = dds_create_topic(participant, &HelloWorld_Type_desc, 
                              "HelloWorld", NULL, NULL);
    
    // 创建发布者
    writer = dds_create_writer(participant, topic, NULL, NULL);
    
    // 发布数据
    HelloWorld msg = { .message = "Hello, DDS!" };
    dds_write(writer, &msg);
    
    return 0;
}
```

## 版本信息

- **版本**: 2.0.0
- **最后更新**: 2026-04-26
- **DDS标准**: OMG DDS v1.4
- **DDS-Security**: OMG DDS-Security v1.1

## 支持与交流

- 问题反馈: [GitHub Issues](https://github.com/your-org/eth-dds-integration/issues)
- 文档: [完整用户手册](user-guide/)
- 架构设计: [架构文档](architecture/)

## 许可

Copyright (c) 2026 ETH-DDS Team. All rights reserved.

---

*本软件按"原样"提供，不附带任何明示或暗示的保证。*