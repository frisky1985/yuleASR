# ETH-DDS Integration 技术规格书

## 1. 项目概要

| 项目属性 | 说明 |
|---------|------|
| **项目名称** | ETH-DDS Integration |
| **技术方向** | 嵌入式以太网 + DDS中间件 |
| **开发语言** | C (C11标准) |
| **支持平台** | Linux, FreeRTOS, 裸机 |

## 2. 系统架构

### 2.1 模块分层

```
┌────────────────────────────────────────┐
│           应用层 (Application)           │
│     设备管理 | 数据服务 | 配置管理      │
├────────────────────────────────────────┤
│           DDS中间件层 (DDS)              │
│     Domain | Pub/Sub | QoS | Discovery   │
├────────────────────────────────────────┤
│           传输层 (Transport)            │
│         UDP | TCP | Shared Memory        │
├────────────────────────────────────────┤
│           以太网驱动层 (Ethernet)       │
│      MAC Driver | PHY | DMA | lwIP       │
├────────────────────────────────────────┤
│           平台抽象层 (Platform)        │
│        Linux | FreeRTOS | Baremetal      │
└────────────────────────────────────────┘
```

### 2.2 核心API

| 模块 | 主要API | 功能描述 |
|-----|----------|---------|
| eth_driver | eth_init, eth_send, eth_receive | 以太网驱动控制 |
| dds_core | dds_create_participant, dds_create_topic, dds_write | DDS核心功能 |
| transport | transport_create, transport_send, transport_receive | 传输层抽象 |

## 3. 技术栈选型

### 3.1 推荐方案

| 组件 | 推荐技术 | 备选方案 |
|------|---------|----------|
| 嵌入式平台 | STM32H7 | Raspberry Pi 4, x86嵌入式 |
| 操作系统 | FreeRTOS | Linux (Yocto/Buildroot) |
| 以太网协议栈 | lwIP | Linux内核网络栈 |
| DDS实现 | Fast DDS | Cyclone DDS, 自研简化版 |
| 序列化 | CDR (DDS标准) | Protobuf |

## 4. QoS策略配置

### 4.1 预设配置

| 配置名 | Reliability | Durability | Deadline | 应用场景 |
|-------|-------------|------------|----------|----------|
| REALTIME | BEST_EFFORT | VOLATILE | 50ms | 实时数据传输 |
| RELIABLE | RELIABLE | VOLATILE | 100ms | 控制命令 |
| PERSISTENT | RELIABLE | TRANSIENT_LOCAL | 1s | 状态同步 |

## 5. 性能指标

| 指标 | 目标值 | 测试方法 |
|-----|--------|----------|
| 端到端延迟 | < 1ms (本地) | 时间戳差值 |
| 网络延迟 | < 5ms (同网段) | ping + DDS开销 |
| 吞吐量 | > 100K msg/s | 压力测试 |
| CPU占用率 | < 30% (1KHz) | top/ps监控 |
| 内存占用 | < 10MB | valgrind/自带统计 |

## 6. 开发计划

### 6.1 里程碑

- **M1**: 以太网驱动完成，ping测试通过 (4周)
- **M2**: DDS发布订阅功能验证 (6周)
- **M3**: 完整应用功能验证 (4周)
- **M4**: 生产就绪版本发布 (4周)

### 6.2 风险管理

| 风险 | 概率 | 影响 | 缓解措施 |
|-----|------|------|----------|
| 实时性不达标 | 中 | 高 | CPU绑定, 优先级队列 |
| 内存泄漏 | 低 | 高 | 静态分析, 内存池 |
| 网络不稳定 | 中 | 中 | 自动重连, 重试机制 |

## 7. 文档清单

- [x] 架构设计文档
- [x] API接口规范
- [ ] 实现详细设计
- [ ] 测试用例文档
- [ ] 部署指南

---
**版本**: v1.0 | **日期**: 2026-04-24
