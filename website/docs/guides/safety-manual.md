---
title: SAFETY MANUAL - ISO 26262 ASIL-D
description: "本项目实现了多个AUTOSAR BSW模块，其中以下模块达到ASIL-D级别:"
sidebar_position: 29
---

# SAFETY MANUAL - ISO 26262 ASIL-D

## 项目概述

本项目实现了多个AUTOSAR BSW模块，其中以下模块达到ASIL-D级别:
- E2E (End-to-End Protection)
- OS Timing Protection
- NVM Redundant Storage

## 安全目标

| ASIL等级 | 单点故障覆盖率 | 双点故障覆盖率 | 诊断覆盖率 |
|*********|************|************|*********--|
| ASIL-D  | 99%        | 90%        | 100%      |

## 安全机制

### 1. 数据完整性 (E2E)

E2E模块提供以下安全机制:
- CRC保护: 检测数据损坏
- 计数器: 检测丢失或重复消息
- 数据ID: 验证消息源

### 2. 时间监控 (OS Timing)

OS Timing Protection提供:
- 执行时间监控: 防止任务超时
- 锁定时间监控: 防止资源锁死
- 间隔时间监控: 防止任务激活过快

### 3. 数据存储安全 (NVM)

NVM冗余存储提供:
- 双备份存储: 确保数据不丢失
- CRC校验: 检测数据损坏
- 自动恢复: 从故障中恢复

## 开发流程

1. 需求分析 (ISO 26262-3)
2. 设计实现 (ISO 26262-5)
3. 测试验证 (ISO 26262-6)
4. 安全分析 (ISO 26262-9)
