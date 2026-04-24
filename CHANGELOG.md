# 版本变更日志

本文件记录ETH-DDS Integration项目的所有重要变更。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，版本号遵循 [Semantic Versioning](https://semver.org/lang/zh-CN/)。

---

## [2.0.0] - 2026-04-24

### 新增

#### TSN模块
- 完整的TSN功能支持，包括：
  - gPTP（通用精确时间协议）-时钟同步
  - TAS（时间感知调度器）-门控调度
  - CBS（累积形束波形）-流量整形
  - Frame Preemption（帧抢占）-优先级传输
  - SRP（流预留协议）-资源预留

#### AUTOSAR模块
- AUTOSAR Classic RTE集成（rte_dds.c/h）
  - RTE接口实现
  - E2E（End-to-End）保护支持
  - ARXML配置文件解析
- AUTOSAR Adaptive ara::com绑定（ara_com_dds.c/h）
  - Service Discovery
  - Method Call和Event Subscription

#### DDS高级功能
- 内容过滤主题（Content Filtered Topic）
- 时间过滤（Time Filter）
- 所有权管理（Ownership）
- 数据持久化（Persistence）

#### DDS安全扩展
- 完整的DDS-Security实现：
  - 认证插件（Authentication）-PKI证书
  - 加密插件（Cryptography）-AES-256加密
  - 访问控制（Access Control）-Topic和Domain级别

#### 传输层增强
- UDP传输优化
- 共享内存传输（SHM）-Zero-Copy支持
- 传输管理器统一调度

#### 平台支持
- Linux平台完整支持（标准和PREEMPT_RT）
- FreeRTOS平台适配层
- 裸机平台支持

### 修复
- 修复了RTPS消息片段重组的边界情况处理
- 修复了TSN时钟同步在高负载下的稳定性问题
- 修复了AUTOSAR RTE数据类型映射的内存对齐问题
- 修复了某些平台上的DMA缓冲区同步问题

### 改进
- 重构了CMake构建系统，支持更多配置选项
- 优化了DDS发现机制的性能
- 改进了错误处理和日志系统
- 提升了以太网驱动的硬实时性能

---

## [1.1.0] - 2026-03-15

### 新增
- 添加了DDS Runtime监控功能
- 增加了更多示例程序
- 支持多平台交叉编译

### 修复
- 修复了内存泄漏问题
- 修复了某些编译器的警告

---

## [1.0.0] - 2026-02-01

### 新增
- 首个正式版本发布
- 基本DDS功能（DomainParticipant、Topic、Publisher、Subscriber）
- RTPS协议实现
- 基础Ethernet驱动支持
- UDP传输层实现
- 基础QoS支持
- Linux平台支持

### 特性
- 符合OMG DDS规范
- 低延迟设计
- 模块化架构

---

## [0.9.0] - 2026-01-10

### 新增
- Beta测试版本
- 核心架构实现
- 基础测试套件

---

## 版本符号说明

- **MAJOR**：不兼容的API修改
- **MINOR**：向后兼容的功能添加
- **PATCH**：向后兼容的问题修复

---

*本文档由ETH-DDS项目团队维护*
