---
title: WdgM (看门狗管理模块) 集成指南
description: "> **AUTOSAR版本**: 4.7.0"
sidebar_position: 9
---

# WdgM (看门狗管理模块) 集成指南

> **AUTOSAR版本**: 4.7.0  
> **目标平台**: NXP S32K312  
> **安全等级**: ASIL-D  
> **版本**: 1.0.0

## 目录

1. [模块概述](#1-模块概述)
2. [架构设计](#2-架构设计)
3. [文件结构](#3-文件结构)
4. [配置说明](#4-配置说明)
5. [与Lockstep集成](#5-与lockstep集成)
6. [与RamSafety集成](#6-与ramsafety集成)
7. [与MCAL Wdg集成](#7-与mcal-wdg集成)
8. [使用示例](#8-使用示例)
9. [测试验证](#9-测试验证)
10. [问题排查](#10-问题排查)

***

## 1. 模块概述

### 1.1 功能描述

WdgM (看门狗管理模块) 是AUTOSAR基础软件(BSW)的服务层模块，提供：

- **活监督 (Alive Supervision)**: 监督定时器刷新次数
- **截止时间监督 (Deadline Supervision)**: 监督任务执行时间
- **逻辑监督 (Logical Supervision)**: 监督程序流执行顺序
- **看门狗触发管理**: 管理窗口看门狗(WWD)和独立看门狗(IWD)
- **安全事件处理**: 与Lockstep、RamSafety安全模块集成

### 1.2 特性支持

| 特性 | 支持状态 | 说明 |
|******|*********|******|
| 窗口看门狗 (WWD) | ✓ | 支持S32K312窗口看门狗 |
| 独立看门狗 (IWD) | ✓ | 支持S32K312独立看门狗 |
| 多监督实体 | ✓ | 最大8个监督实体 |
| 活监督 | ✓ | 定时刷新监督 |
| Lockstep集成 | ✓ | 锁步错误触发复位 |
| RamSafety集成 | ✓ | RAM错误触发复位 |
| ASIL-D设计 | ✓ | 符合ISO 26262要求 |

***

## 2. 架构设计

### 2.1 模块架构图

```
├─────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                    ASW (应用软件层)                                         │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  │
│  │  Engine  │  │  Comm    │  │  Diag    │  │  Safety  │  │ Storage │  │  Mode    │  │
│  │ Control  │  │ Manager │  │ Manager │  │ Monitor │  │ Manager │  │ Manager │  │
│  └───┼───┘  └───┼───┘  └───┼───┘  └───┼───┘  └───┼───┘  └───┼───┘  │
│       │          │          │          │          │          │          │  │
│       └───────────┼───────────┼───────────┼───────────┼───────────┼───────────┘  │
├─────────────────────────┼─────────────────────────────────────────────────────────────────────────────────┤
│                       RTE (运行时环境)                                                │
│                                   │                                                        │
├─────────────────────────┼─────────────────────────────────────────────────────────────────────────────────┤
│                       BSW (基础软件层)                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────────────────────────────┐  │
│  │                                WdgM                                                │  │
│  │  ┌───────────────────────────────────────────────────────────────────────────────────────────┐  │  │
│  │  │                    Supervised Entities (SE)                                 │  │  │
│  │  │  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  │  │  │
│  │  │  │ SE 0 │  │ SE 1 │  │ SE 2 │  │ SE 3 │  │ SE 4 │  │ SE 5 │  │ SE 6 │  │  │  │
│  │  │  │(Main)│  │(Comm)│  │(Diag)│  │(Store)│ │(Safety)│ │(LS) │  │(RAM) │  │  │  │
│  │  │  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘  │  │  │
│  │  └─────────────────────────────────────────────────────────────────────────────────────────────────┘  │  │
│  │  ┌───────────────────────────────────────────────────────────────────────────────────────────┐  │  │
│  │  │                        Watchdog Control                                    │  │  │
│  │  │  ┌────────────────────────────────────────────────────────────────────────────┐  │  │  │
│  │  │  │        Mode Control ─────┼─────────┼───────── Trigger Control        │  │  │
│  │  │  │                   │      │         ├─────── WWD (窗口看门狗)   │  │  │
│  │  │  │     OFF/SLOW/FAST  │      │         ├─────── IWD (独立看门狗)   │  │  │
│  │  │  └────────────────────────────────────────────────────────────────────────────┘  │  │  │
│  │  └───────────────────────────────────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────────────────────────────────┘  │
│           │                                    │                                    │
│           │                                    │                                    │
├───────────┼──────────────────── ────────────────────────────────────────────────────────┤
│           └─────────────────────┼──────────────────────────────────────────────────────┘  │
│                      Safety Integration                                        │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐       │
│  │    Lockstep     │  │   RamSafety     │  │      Dem       │  │      Det       │       │
│  │    (Lockstep)   │  │   (RAM Check)   │  │   (Diagnostic)  │  │  (Error Trace) │       │
│  │                 │  │                 │  │                 │  │                 │       │
│  │  Error Handler ─┾  │  Error Handler ─┾  │  Event Report ─┾  │  Error Report  │       │
│  │                 │  │                 │  │                 │  │                 │       │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘  └──────────────────┘       │
├─────────────────────────────────────────────────────────────────────────────────────────────────┤
│                       MCAL (微控制驱动层)                                             │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐       │
│  │      Wdg       │  │      Mcu       │  │      Gpt       │  │      Port      │       │
│  │   (Watchdog)   │  │    (MCU Ctrl)  │  │    (General    │  │  (Port I/O)    │       │
│  │                │  │                │  │   Purpose Timer)│  │               │       │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘  └──────────────────┘       │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 状态机

```
                    ┌───────────────────────────────────────────────────────┐
                    │                                                    │
                    │  WDGM_STATE_UNINIT (未初始化)                       │
                    │                                                    │
                    └───────────────────────────────────────────────────────┘
                                      │
                                      │ WdgM_Init()
                                      │ 初始化成功
                                      ▼
                    ┌───────────────────────────────────────────────────────┐
                    │                                                    │
                    │  WDGM_STATE_ACTIVE (活跃状态)                         │
                    │  ┌───────────────────────────────────────┐  │
          ────────┾───────────────────────────────────────────┘  │
         ▲         │  │  WdgM_MainFunction() ──┾  WdgM_TriggerWatchdog()  │  │
         │         │  │                              │  │
         │         │  │  ▲                             ▼  │
         │         │  └───┼───────────────────────────────────┘  │
         │         │     │  监督实体检查                           │
         │         │     │                                   │
         │         └─────┼─────────────────────────────────┘
         │               │
         │               │ 监督超时/安全错误
         │               ▼
         │             ┌───────────────────────────────────────────────────────┐
         │             │                                                    │
         │             │  WDGM_STATE_SUPERVISION_EXPIRED (监督超时)            │
         │             │                                                    │
         │             └───────────────────────────────────────────────────────┘
         │                           │
         │                           │ 错误阈值达到
         │                           ▼
         │             ┌───────────────────────────────────────────────────────┐
         │             │                                                    │
         ▲────────────┾  WDGM_STATE_STOPPED (已停止/复位)                  │
                       │                                                    │
                       │  WdgM_PerformReset()                               │
                       └───────────────────────────────────────────────────────┘
```

***

## 3. 文件结构

```
src/bsw/services/wdgm/
├─────────────────────────────────────────────────────────────────────────────────────────────────┐
│  目录结构                                                                       │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘
│
├─── include/
│   ├─── WdgM.h          - 模块主头文件，包含API声明和类型定义
│   └─── WdgM_Cfg.h      - 配置头文件，包含可配置选项
│
├─── src/
│   ├─── WdgM.c          - 模块主实现
│   └─── WdgM_Cfg.c      - 配置实现，包含默认配置表
│
└─── test/
    └─── WdgM_Test.c     - 单元测试套件

├─────────────────────────────────────────────────────────────────────────────────────────────────┐
│  关键文件说明                                                                       │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 3.1 WdgM.h (主头文件)

主要包含：
- 版本信息定义
- 错误码定义
- 数据类型定义 (`WdgM_StateType`, `WdgM_SEStateType` 等)
- API函数声明
- 外部变量声明

### 3.2 WdgM.c (主实现)

主要实现：
- `WdgM_Init()` - 初始化模块
- `WdgM_DeInit()` - 去初始化模块
- `WdgM_MainFunction()` - 主循环函数 (每10ms调用一次)
- `WdgM_CheckpointReached()` - 检查点报告
- `WdgM_TriggerWatchdog()` - 触发看门狗
- `WdgM_HandleLockstepError()` - 处理Lockstep错误
- `WdgM_HandleRamSafetyError()` - 处理RamSafety错误

***

## 4. 配置说明

### 4.1 基础配置

```c
/* 基础配置选项 */
#define WDGM_CFG_DEV_ERROR_DETECT           STD_ON      /* 开发错误检测 */
#define WDGM_CFG_VERSION_INFO_API           STD_ON      /* 版本信息API */
#define WDGM_CFG_WWD_ENABLE                 STD_ON      /* 使能窗口看门狗 */
#define WDGM_CFG_IWD_ENABLE                 STD_ON      /* 使能独立看门狗 */
```

### 4.2 监督参数配置

```c
/* 监督周期和阈值 */
#define WDGM_CFG_SUPERVISION_CYCLE_MS       10U         /* 监督周期 10ms */
#define WDGM_CFG_FAILURE_THRESHOLD          3U          /* 错误阈值 3次 */
#define WDGM_CFG_MAX_SUPERVISED_ENTITIES    8U          /* 最大8个监督实体 */
```

### 4.3 看门狗配置

```c
/* 窗口看门狗配置 */
#define WDGM_CFG_WWD_TRIGGER_PERIOD_MS      50U         /* 触发周期 50ms */
#define WDGM_CFG_WWD_WINDOW_START_PERCENT   50U         /* 窗口开始 50% */
#define WDGM_CFG_WWD_WINDOW_END_PERCENT     100U        /* 窗口结束 100% */
#define WDGM_CFG_WWD_TIMEOUT_MS             100U        /* 超时时间 100ms */

/* 独立看门狗配置 */
#define WDGM_CFG_IWD_TRIGGER_PERIOD_MS      100U        /* 触发周期 100ms */
#define WDGM_CFG_IWD_TIMEOUT_MS             200U        /* 超时时间 200ms */
```

### 4.4 安全模块集成配置

```c
/* 安全模块集成 */
#define WDGM_CFG_LOCKSTEP_INTEGRATION       STD_ON      /* Lockstep集成 */
#define WDGM_CFG_RAMSAFETY_INTEGRATION      STD_ON      /* RamSafety集成 */
#define WDGM_CFG_DEM_INTEGRATION            STD_ON      /* Dem集成 */
```

### 4.5 监督实体配置

```c
/* 监督实体ID定义 */
#define WDGM_SEID_MAIN_CYCLE                0x0001U     /* 主循环 */
#define WDGM_SEID_COMMUNICATION             0x0002U     /* 通信模块 */
#define WDGM_SEID_DIAGNOSTICS               0x0003U     /* 诊断模块 */
#define WDGM_SEID_STORAGE                   0x0004U     /* 存储模块 */
#define WDGM_SEID_SAFETY_MONITOR            0x0005U     /* 安全监控 */
#define WDGM_SEID_LOCKSTEP                  0x0006U     /* Lockstep */
#define WDGM_SEID_RAMSAFETY                 0x0007U     /* RamSafety */
```

***

## 5. 与Lockstep集成

### 5.1 集成架构

```
├─────────────────────────────────────────────────────────────────────────────────────────────────┐
│                        Lockstep - WdgM 集成流程                                │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘

     ┌─────────────────────┐
     │      Lockstep        │
     │      模块             │
     └─────────┼───────────┘
             │
             │ 1. 检测到锁步错误
             │    (LOCKSTEP_E_MISMATCH_DETECTED)
             ▼
     ┌─────────┼───────────┐
     │  Lockstep_EventCallback  │
     │  错误回调函数          │
     └─────────┼───────────┘
             │
             │ 2. 调用WdgM错误处理
             ▼
     ┌─────────┼───────────┐
     │  WdgM_HandleLockstepError  │
     │  (错误处理函数)          │
     └─────────┼───────────┘
             │
             │ 3. 更新统计
             │    执行安全响应
             ▼
     ┌─────────┼───────────┐
     │        WdgM              │
     │  WdgM_PerformReset()     │
     │  (系统复位)              │
     └─────────────────────┘
```

### 5.2 配置步骤

1. **在Lockstep配置中启用WdgM集成**

```c
/* Lockstep_Cfg.h */
#define LOCKSTEP_WDGM_INTEGRATION           STD_ON
```

2. **配置Lockstep错误回调**

```c
/* 在Lockstep_Cfg.c中 */
void Lockstep_EventCallback(
    Lockstep_EventType event,
    uint32 errorCode,
    const void* context)
{
    if (event == LOCKSTEP_EVENT_MISMATCH)
    {
        /* 调用WdgM处理锁步错误 */
        WdgM_HandleLockstepError(errorCode);
    }
}
```

3. **配置WdgM监督实体**

```c
/* WdgM_Cfg.c */
{
    .seId = WDGM_SEID_LOCKSTEP,
    .supervisionType = WDGM_SUPERVISION_ALIVE,
    .enabled = TRUE,
    .config.alive = {
        .aliveSupRefCycle = 10U,    /* 每100ms期望一次 */
        .aliveSupMin = 1U,
        .aliveSupMax = 2U
    }
}
```

### 5.3 代码示例

```c
/* Lockstep事件回调实现 - Lockstep_Cfg.c */
void Lockstep_EventCallback(
    Lockstep_EventType event,
    uint32 errorCode,
    const void* context)
{
    (void)context;
    
    switch (event)
    {
        case LOCKSTEP_EVENT_MISMATCH:
            /* 锁步不匹配 - 调用WdgM处理 */
            WdgM_HandleLockstepError(LOCKSTEP_E_MISMATCH_DETECTED);
            break;
            
        case LOCKSTEP_EVENT_BIST_FAILURE:
            /* BIST失败 - 调用WdgM处理 */
            WdgM_HandleLockstepError(LOCKSTEP_E_BIST_FAILURE);
            break;
            
        default:
            break;
    }
}
```

***

## 6. 与RamSafety集成

### 6.1 集成架构

```
├─────────────────────────────────────────────────────────────────────────────────────────────────┐
│                        RamSafety - WdgM 集成流程                               │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘

     ┌─────────────────────┐
     │     RamSafety       │
     │      模块             │
     └─────────┼───────────┘
             │
             │ 1. 检测到RAM错误
             │    (RAMSAFETY_E_ECC_ERROR)
             ▼
     ┌─────────┼───────────┐
     │  RamSafety_ErrorCallback │
     │  错误回调函数          │
     └─────────┼───────────┘
             │
             │ 2. 调用WdgM错误处理
             ▼
     ┌─────────┼───────────┐
     │  WdgM_HandleRamSafetyError │
     │  (错误处理函数)          │
     └─────────┼───────────┘
             │
             │ 3. 更新统计
             │    执行安全响应
             ▼
     ┌─────────┼───────────┐
     │        WdgM              │
     │  WdgM_PerformReset()     │
     │  (系统复位)              │
     └─────────────────────┘
```

### 6.2 配置步骤

1. **在RamSafety配置中启用WdgM集成**

```c
/* RamSafety_Cfg.h */
#define RAMSAFETY_WDGM_INTEGRATION          STD_ON
```

2. **配置RamSafety错误回调**

```c
/* RamSafety_Cfg.c */
void RamSafety_ErrorCallback(
    RamSafety_TestType testType,
    uint32 address,
    uint8 expected,
    uint8 actual)
{
    /* 调用WdgM处理RAM错误 */
    WdgM_HandleRamSafetyError(RAMSAFETY_E_TEST_FAILED);
}
```

3. **配置WdgM监督实体**

```c
/* WdgM_Cfg.c */
{
    .seId = WDGM_SEID_RAMSAFETY,
    .supervisionType = WDGM_SUPERVISION_ALIVE,
    .enabled = TRUE,
    .config.alive = {
        .aliveSupRefCycle = 10U,    /* 每100ms期望一次 */
        .aliveSupMin = 1U,
        .aliveSupMax = 2U
    }
}
```

### 6.3 代码示例

```c
/* RamSafety错误回调实现 - RamSafety_Cfg.c */
void RamSafety_ErrorCallback(
    RamSafety_TestType testType,
    uint32 address,
    uint8 expected,
    uint8 actual)
{
    (void)testType;
    (void)expected;
    (void)actual;
    
    /* RAM检查失败 - 调用WdgM处理 */
    WdgM_HandleRamSafetyError((uint32)address);
}
```

***

## 7. 与MCAL Wdg集成

### 7.1 平台抽象层

WdgM通过配置的回调函数与MCAL Wdg驱动交互:

```c
/* WdgM_Cfg.c - 平台抽象层实现 */
void WdgM_WatchdogTrigger(void)
{
#if (WDGM_CFG_WWD_ENABLE == STD_ON)
    /* 触发窗口看门狗 - 调用MCAL Wdg驱动 */
    Wdg_SetTriggerCondition(WDGM_CFG_WWD_TIMEOUT_MS);
#endif

#if (WDGM_CFG_IWD_ENABLE == STD_ON)
    /* 触发独立看门狗 */
    Wdg_17_Timer_SetTriggerCondition(WDGM_CFG_IWD_TIMEOUT_MS);
#endif
}

void WdgM_WatchdogSetMode(uint8 mode)
{
    WdgIf_ModeType wdgMode;
    
    switch (mode)
    {
        case WDGM_WATCHDOG_MODE_OFF:
            wdgMode = WDGIF_OFF_MODE;
            break;
        case WDGM_WATCHDOG_MODE_SLOW:
            wdgMode = WDGIF_SLOW_MODE;
            break;
        case WDGM_WATCHDOG_MODE_FAST:
            wdgMode = WDGIF_FAST_MODE;
            break;
        default:
            return;
    }
    
    /* 调用Wdg接口设置模式 */
    WdgIf_SetMode(0U, wdgMode);  /* DeviceIndex = 0 */
}
```

### 7.2 S32K312特定配置

```c
/* 针对S32K312的特定配置 */
#define S32K312_WWD_CLK_SOURCE          WDG_IP_CLK_BUS_CLK
#define S32K312_WWD_PRESCALER           WDG_IP_PRESCALER_1
#define S32K312_WWD_WINDOW_MODE         WDG_IP_WINDOW_MODE

#define S32K312_IWD_CLK_SOURCE          WDG_IP_CLK_LPO
#define S32K312_IWD_TIMEOUT_VALUE       0xFFFFU  /* 最大超时值 */
```

***

## 8. 使用示例

### 8.1 基本使用流程

```c
/* main.c - WdgM基本使用示例 */
#include "WdgM.h"
#include "WdgM_Cfg.h"

int main(void)
{
    /* 1. 初始化WdgM */
    if (E_OK != WdgM_Init(&WdgM_Config))
    {
        /* 初始化失败处理 */
        Error_Handler();
    }
    
    /* 2. 设置看门狗模式 */
    WdgM_SetMode(WDGM_WATCHDOG_MODE_SLOW);
    
    /* 3. 激活监督实体 */
    WdgM_ActivateSupervisionEntity(WDGM_SEID_MAIN_CYCLE);
    
    /* 4. 主循环 */
    while (1)
    {
        /* 执行应用逻辑 */
        Application_MainFunction();
        
        /* 报告检查点 */
        WdgM_CheckpointReached(WDGM_SEID_MAIN_CYCLE);
        
        /* 小延迟 */
        Delay(1ms);
    }
}

/* 10ms周期中断服务程序 */
void ISR_10ms(void)
{
    /* 调用WdgM主函数 */
    WdgM_MainFunction();
}
```

### 8.2 多监督实体示例

```c
/* 多模块监督示例 */
void Communication_MainFunction(void)
{
    /* 通信处理 */
    ProcessCanMessages();
    ProcessLinMessages();
    
    /* 报告检查点 */
    WdgM_CheckpointReached(WDGM_SEID_COMMUNICATION);
}

void Diagnostics_MainFunction(void)
{
    /* 诊断处理 */
    ProcessDiagnosticRequests();
    
    /* 报告检查点 */
    WdgM_CheckpointReached(WDGM_SEID_DIAGNOSTICS);
}

void SafetyMonitor_MainFunction(void)
{
    /* 安全监控 */
    CheckSafetyConditions();
    
    /* 报告检查点 */
    WdgM_CheckpointReached(WDGM_SEID_SAFETY_MONITOR);
}
```

### 8.3 安全回调示例

```c
/* 安全事件回调实现 */
void MySafetyEventCallback(uint8 eventType, uint32 errorCode, const void* context)
{
    (void)context;
    
    switch (eventType)
    {
        case 0x01U:  /* 监督超时 */
            LogError("Supervision expired, SE: %u", errorCode);
            break;
            
        case 0x02U:  /* Lockstep错误 */
            LogError("Lockstep error: 0x%08X", errorCode);
            break;
            
        case 0x03U:  /* RamSafety错误 */
            LogError("RAM safety error: 0x%08X", errorCode);
            break;
            
        case 0x04U:  /* 模式改变 */
            LogInfo("Mode changed to: %u", errorCode);
            break;
            
        case 0x05U:  /* 复位 */
            LogInfo("System reset triggered");
            break;
            
        default:
            LogWarning("Unknown safety event: %u", eventType);
            break;
    }
}

/* 初始化时注册回调 */
void Init_SafetySystem(void)
{
    WdgM_Init(&WdgM_Config);
    WdgM_RegisterSafetyCallback(MySafetyEventCallback, NULL);
}
```

***

## 9. 测试验证

### 9.1 测试套件概述

测试文件: `src/bsw/services/wdgm/test/WdgM_Test.c`

测试框架: Unity

测试用例总数: 43个

### 9.2 测试分类

| 测试类别 | 用例数量 | 说明 |
|*********|*********|******|
| 初始化/去初始化 | 4 | Init, DeInit, 参数验证 |
| 模式设置 | 4 | SetMode, GetMode, 无效参数 |
| 监督实体 | 5 | 激活/去激活, 状态查询 |
| 检查点报告 | 5 | CheckpointReached, 有效/无效 |
| 看门狗触发 | 4 | 正常触发, 主函数 |
| 安全事件 | 3 | 回调注册, Lockstep, RamSafety |
| 统计信息 | 4 | 状态获取, 超时SEID |
| 版本信息 | 2 | 版本获取, 空指针处理 |
| 配置验证 | 2 | 无效配置, 调试配置 |
| 主函数行为 | 2 | 循环测试, 未初始化调用 |
| 集成测试 | 5 | Lockstep, RamSafety, Dem, WWD, IWD |
| 边界条件 | 4 | 最大数量, 阈值, 周期 |

### 9.3 运行测试

```bash
# 编译测试程序
cd /home/admin/yuleASR_check
gcc -I src/bsw/services/wdgm/include \
    -I tests/unity \
    -c src/bsw/services/wdgm/test/WdgM_Test.c \
    -o build/test/wdgm_test.o

# 链接并运行测试
gcc build/test/wdgm_test.o \
    -o build/test/wdgm_test \
    -L build/lib -lwdgm -lunity

# 执行测试
./build/test/wdgm_test
```

### 9.4 预期测试结果

```
Unity test run 1 of 1
.......................43 Tests 0 Failures 0 Ignored
OK
```

***

## 10. 问题排查

### 10.1 常见问题

| 问题现象 | 可能原因 | 解决方法 |
|*********|*********|*********|
| WdgM_Init返回E_NOT_OK | 配置指针为空 | 检查配置指针 |
| 看门狗没有触发 | 状态不是ACTIVE | 检查状态转换 |
| 监督实体报告失败 | SEID不存在 | 检查配置表SEID |
| 安全回调没调用 | 未注册回调 | 调用RegisterSafetyCallback |
| 窗口看门狗复位 | 在窗口外触发 | 调整主函数调用周期 |

### 10.2 调试建议

1. **使用调试配置**

```c
/* 在开发阶段使用较松的配置 */
WdgM_Init(&WdgM_ConfigDebug);
```

2. **检查状态变化**

```c
/* 添加调试打印 */
WdgM_StateType state = WdgM_GetState();
printf("WdgM State: %d\n", state);
```

3. **验证监督实体配置**

```c
/* 检查监督实体状态 */
WdgM_SEStateType seState;
if (E_OK == WdgM_GetSEState(WDGM_SEID_MAIN_CYCLE, &seState))
{
    printf("SE State: %d\n", seState);
}
```

4. **检查统计信息**

```c
/* 获取全局状态 */
WdgM_GlobalStatusType status;
WdgM_GetGlobalStatus(&status);
printf("Total refreshes: %lu\n", status.totalRefreshes);
printf("Failed refreshes: %lu\n", status.failedRefreshes);
printf("Lockstep errors: %lu\n", status.lockstepErrors);
printf("RamSafety errors: %lu\n", status.ramSafetyErrors);
```

***

## 附录

### A. 参考资料

1. AUTOSAR SWS Watchdog Manager (AUTOSAR 4.7.0)
2. S32K3xx Reference Manual (NXP)
3. ISO 26262-6:2018 (汽车功能安全)

### B. 版本历史

| 版本 | 日期 | 说明 |
|******|******|******|
| 1.0.0 | 2024-04-30 | 初始版本 |

### C. 联系信息

**上海予乐电子科技有限公司**

- 项目: YuleTech AutoSAR BSW Platform
- 模块: WdgM (Watchdog Manager)
- 安全等级: ASIL-D
