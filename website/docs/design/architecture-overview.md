---
title: 系统架构概览
description: "yuleASR 是一个基于 AUTOSAR Classic Platform 的基础软件框架，支持多种嵌入式 MCU 平台。"
sidebar_position: 31
---

# 系统架构概览

## 概述

yuleASR 是一个基于 AUTOSAR Classic Platform 的基础软件框架，支持多种嵌入式 MCU 平台。

## 层次结构

```
┌──────────────────────────────────────────────────────────────────────┐
│                        应用层 (ASW)                            │
│                    RTE (Runtime Environment)                     │
├──────────────────────────────────────────────────────────────────────┤
│                      基础软件 (BSW)                           │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐     │
│  │   服务层     │  │   ECU 抽象层  │  │   微控制器驱动 │  │   复杂驱动  │     │
│  │  (Services)  │  │    (ECUAL)   │  │    (MCAL)    │  │   (CDD)    │     │
│  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘     │
├──────────────────────────────────────────────────────────────────────┤
│                      微控制器 (MCU)                            │
└──────────────────────────────────────────────────────────────────────┘
```

## 主要组件

### 服务层 (Services Layer)
- **DCM**: 诊断通信管理 (Diagnostic Communication Manager)
- **DEM**: 诊断事件管理 (Diagnostic Event Manager)
- **NVM**: 非易失性存储管理 (Non-Volatile Memory Manager)
- **COM**: 自动化通信管理 (Communication Manager)
- **PDUR**: PDU 路由器 (PDU Router)
- **CSM**: 加密服务管理 (Crypto Services Manager)
- **ECUM**: ECU 状态管理 (ECU State Manager)

### ECU 抽象层 (ECUAL)
- **CANIF**: CAN 接口 (CAN Interface)
- **CANTP**: CAN 传输协议 (CAN Transport Protocol)
- **SOAD**: 套接字适配器 (Socket Adapter)
- **DOIP**: 诊断 over IP (Diagnostic over IP)

### 微控制器驱动 (MCAL)
- **CAN**: CAN 驱动
- **SPI**: SPI 驱动
- **ADC**: ADC 驱动
- **GPT**: 通用定时器 (General Purpose Timer)
- **MCU**: 微控制器驱动

## 安全机制

- **WDGM**: 看门狗管理 (Watchdog Manager)
- **SECOC**: 安全车载通信 (Secure Onboard Communication)
- **E2E**: 端到端保护 (End-to-End Protection)
