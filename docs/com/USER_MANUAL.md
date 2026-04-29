# COM 模块 用户手册

> **版本**: v1.0  
> **使用对象**: 软件开发工程师1、系统集成工程师1  
> **更新日期**: 2024年

---

## 目录

1. [产品概述](#产品概述)
2. [快速入门](#快速入门)
3. [核心概念](#核心概念)
4. [配置指南](#配置指南)
5. [编程示例](#编程示例)
6. [高级功能](#高级功能)
7. [调试技巧](#调试技巧)
8. [性能优化](#性能优化)
9. [常见问题](#常见问题)
10. [附录](#附录)

---

## 产品概述

### 什么是 COM 模块？

COM (Communication) 是 AUTOSAR 基础软件中的服务层模块，负责管理 ECU 间的信号级通信。它是车辆网络通信的核心组件，处理以下功能：

- **信号打包/解包**: 将应用层数据转换为网络消息
- **传输模式管理**: 支持周期性、事件触发、混合模式
- **超时监控**: 检测通信故障和数据超时
- **信号路由**: 在不同通信接口之间转发数据
- **数据完整性**: 保证数据一致性和可靠性

### 系统架构

```
├───────────────────────────────────────────────────────────────┐
│                    应用层 (ASW)                      │
│  EngineControl    VehicleDynamics    DiagnosticManager    │
└───────────────────────────────────────────────────────────────┘
                          │
                          ▼
├───────────────────────────────────────────────────────────────┐
│                RTE (Runtime Environment)                 │
│              Com_SendSignal / Com_ReceiveSignal           │
└───────────────────────────────────────────────────────────────┘
                          │
                          ▼
├───────────────────────────────────────────────────────────────┐
│                  服务层 - COM 模块                    │
│  • 信号打包/解包      • 传输模式管理     • 超时监控     │
│  • 队列管理        • 错误处理        • I-PDU 组    │
└───────────────────────────────────────────────────────────────┘
                          │
                          ▼
├───────────────────────────────────────────────────────────────┐
│               ECU 抽象层 - PduR 模块                    │
│              PduR_ComTransmit / PduR_ComRxIndication      │
└───────────────────────────────────────────────────────────────┘
                          │
                          ▼
├───────────────────────────────────────────────────────────────┐
│                驱动层 - CanIf / EthIf / LinIf        │
│              硬件抽象接口层                            │
└───────────────────────────────────────────────────────────────┘
```

### 功能特性

| 特性 | 描述 |
|------|------|
| 信号支持 | 支持 15 种数据类型，包括整数、浮点、数组类型 |
| 传输模式 | 周期性、事件触发、混合模式、TMC 模式切换 |
| 安全性 | ASIL-D 安全等级，冗余检查、校验和 |
| 可靠性 | 发送确认、超时监控、重试机制 |
| 错误处理 | 队列溢出检测、统计收集、诊断日志 |
| 扩展性 | 支持 I-PDU 组管理，灵活的配置策略 |

---

## 快速入门

### 1. 环境准备

#### 必备头文件
```c
#include "Com.h"
#include "Com_Types.h"
#include "Com_Cfg.h"
```

#### 必要的依赖模块
- **PduR**: PDU 路由模块
- **Det**: 默认错误追踪器 (可选，但推荐在开发阶段使用)
- **EcuM**: ECU 状态管理（初始化/Shutdown）

### 2. 最简单示例

```c
#include "Com.h"
#include "Os.h"  /* 操作系统 API */

/* 全局变量 */
static uint16 g_engineSpeed = 0;
static uint8 g_engineTemp = 0;

/**
 * @brief ECU 初始化
 */
void App_Init(void) {
    /* 1. 初始化 COM 模块 */
    Com_Init(&ComConfig);
    
    /* 2. 启动需要的 I-PDU 组 */
    Com_IpduGroupStart(ComConf_ComIPduGroup_EngineGroup, TRUE);
    Com_IpduGroupStart(ComConf_ComIPduGroup_ChassisGroup, TRUE);
}

/**
 * @brief 定期任务：发送数据
 * 周期: 10ms
 */
TASK(SendTask) {
    /* 读取传感器数据 */
    g_engineSpeed = Sensor_ReadEngineSpeed();
    g_engineTemp = Sensor_ReadEngineTemp();
    
    /* 发送信号 */
    Com_SendSignal(ComConf_ComSignal_EngineSpeed, &g_engineSpeed);
    Com_SendSignal(ComConf_ComSignal_CoolantTemp, &g_engineTemp);
    
    TerminateTask();
}

/**
 * @brief 定期任务：COM 发送处理
 * 周期: 10ms
 */
TASK(ComTxTask) {
    Com_MainFunctionTx();
    TerminateTask();
}

/**
 * @brief 定期任务：COM 接收处理
 * 周期: 10ms
 */
TASK(ComRxTask) {
    Com_MainFunctionRx();
    TerminateTask();
}
```

### 3. 接收数据示例

```c
/**
 * @brief 接收并处理车速信号
 */
void ProcessVehicleSpeed(void) {
    uint16 vehicleSpeed;
    uint8 result;
    
    /* 接收信号 */
    result = Com_ReceiveSignal(ComConf_ComSignal_VehicleSpeed, &vehicleSpeed);
    
    if (result == E_OK) {
        /* 成功接收数据 */
        if (vehicleSpeed > 120) {
            /* 超速警告 */
            Display_ShowOverspeedWarning();
        }
    } else {
        /* 处理错误 */
        HandleReceiveError();
    }
}
```

---

## 核心概念

### 信号 (Signal)

信号是 COM 中的基本数据单元，代表一个物理量。

```
信号定义示例:
┌──────────────────────────────────────────────┐
│  信号名称: EngineSpeed                      │
│  信号 ID: ComConf_ComSignal_EngineSpeed         │
│  数据类型: uint16                               │
│  字节序: BIG_ENDIAN                         │
│  起始位置: 第 0 字节                             │
│  长度: 16 位                                │
│  传输属性: TRIGGERED_ON_CHANGE               │
└──────────────────────────────────────────────┘
```

#### 信号类型

| 类型 | 描述 | C 类型 | 范围 |
|------|------|--------|------|
| COM_BOOLEAN | 布尔值 | boolean | 0/1 |
| COM_UINT8 | 无符号8位 | uint8 | 0-255 |
| COM_UINT16 | 无符号16位 | uint16 | 0-65535 |
| COM_UINT32 | 无符号32位 | uint32 | 0-4294967295 |
| COM_SINT8 | 有符号8位 | sint8 | -128~127 |
| COM_SINT16 | 有符号16位 | sint16 | -32768~32767 |
| COM_FLOAT32 | 32位浮点 | float32 | IEEE 754 |
| COM_UINT8_N | 字节数组 | uint8[] | 可配置 |

#### 传输属性

| 属性 | 描述 | 使用场景 |
|------|------|---------|
| PENDING | 仅更新缓冲区，不触发传输 | 非紧急数据 |
| TRIGGERED | 每次调用都触发传输 | 紧急数据 |
| TRIGGERED_ON_CHANGE | 数据变化时触发 | 节省带宽 |
| TRIGGERED_ON_CHANGE_WITHOUT_REPETITION | 变化时触发，无重复 | 稳定数据 |
| TRIGGERED_WITHOUT_REPETITION | 触发，无重复 | 一次性事件 |

### I-PDU (Interaction Layer PDU)

I-PDU 是 COM 与 PduR 之间交换的数据单元。

```
I-PDU 结构示例 (EngineData - 8字节):
┌─────────────────────────────────────────────────────────────────┐
│ 位置  │  内容              │  信号名称           │
├────────┬─────────────────────┬───────────────────┤
│ 0-15 │ EngineSpeed (uint16)│ 发动机转速         │
├────────┼─────────────────────┼───────────────────┤
│ 16-23 │ CoolantTemp (uint8) │ 冷却液温度         │
├────────┼─────────────────────┼───────────────────┤
│ 24-31 │ ThrottlePos (uint8) │ 节气门位置         │
├────────┼─────────────────────┼───────────────────┤
│ 32-63 │ EngineTorque (uint32)│ 发动机扭矩         │
└────────┴─────────────────────┴───────────────────┘
```

### 信号组 (Signal Group)

信号组用于保证多个相关信号的一致性。

```
信号组示例: EngineCoreInfo
┌─────────────────────────────────────────┐
│  信号组: EngineCoreInfo                      │
│  ┌─────────────────────────────────┐ │
│  │  • EngineSpeed                        │ │
│  │  • CoolantTemp                        │ │
│  │  • ThrottlePosition                   │ │
│  │  • EngineTorque                       │ │
│  └─────────────────────────────────┘ │
│                                                │
│  影子缓冲区 (Shadow Buffer) 确保数据一致性     │
└─────────────────────────────────────────┘

工作流程:
1. Com_UpdateShadowSignal(信号1, 数据1)
2. Com_UpdateShadowSignal(信号2, 数据2)
3. Com_UpdateShadowSignal(信号3, 数据3)
4. Com_SendSignalGroup(信号组ID) → 一次性发送所有数据
```

### 传输模式

#### 周期性传输 (PERIODIC)

```
时间轴:
┌────────┬────────┬────────┬────────┬────────┐
│   TX  │   TX  │   TX  │   TX  │   TX  │
└────────┴────────┴────────┴────────┴────────┘
0ms    100ms  200ms  300ms  400ms  500ms

CycleTime = 100ms
```

#### 直接传输 (DIRECT)

```
事件触发:

              信号发送                    信号发送
┌────────────────┬────────┬────────────────┬────────┐
│               │  TX  │               │  TX  │
└────────────────┴────────┴────────────────┴────────┘
             50ms              150ms

NumRepetitions = 0（无重复）
```

#### 混合传输 (MIXED)

```
周期性 + 事件触发:

┌────────┬────────┬────────┬────────┬────────┐
│   TX  │ TX  │   TX  │   TX  │   TX  │
└────────┴────────┴────────┴────────┴────────┘
0ms    100ms 120ms 200ms  300ms  400ms
       ↑
    事件触发带重复

CycleTime = 100ms, RepetitionPeriod = 20ms, NumRepetitions = 1
```

---

## 配置指南

### I-PDU 组配置

```c
/* Com_Cfg.h 中的组定义 */
#define ComConf_ComIPduGroup_EngineGroup    0u
#define ComConf_ComIPduGroup_ChassisGroup   1u
#define ComConf_ComIPduGroup_BodyGroup      2u
```

### 配置选项

| 选项 | 默认值 | 描述 |
|------|--------|------|
| COM_DEV_ERROR_DETECT | STD_ON | 开发错误检测 |
| COM_VERSION_INFO_API | STD_ON | 版本信息 API |
| COM_ENABLE_SIGNAL_GROUP_ARRAY_API | STD_ON | 信号组数组 API |
| COM_OPTIMIZE_FOR_SPEED | STD_ON | 速度优化 |
| COM_ERROR_HANDLING_ENABLE | STD_ON | 错误处理 |

---

## 编程示例

### 完整应用示例

```c
/**
 * @file EngineControlApp.c
 * @brief 发动机控制应用示例
 */

#include "Com.h"
#include "Os.h"

/* 信号定义 */
typedef struct {
    uint16 engineSpeed;      /* RPM */
    uint8 coolantTemp;       /* °C */
    uint8 throttlePosition;  /* % */
    uint32 engineTorque;     /* Nm */
} EngineDataType;

/* 全局数据 */
static EngineDataType g_engineData;
static boolean g_engineRunning = FALSE;

/**
 * @brief 应用初始化
 */
void EngineControl_Init(void) {
    /* COM 初始化在 EcuM 中完成 */
    
    /* 初始化数据 */
    g_engineData.engineSpeed = 0;
    g_engineData.coolantTemp = 20;
    g_engineData.throttlePosition = 0;
    g_engineData.engineTorque = 0;
}

/**
 * @brief 10ms 定期任务：读取传感器并发送
 */
TASK(EngineControl_Task) {
    /* 读取传感器数据 */
    g_engineData.engineSpeed = Sensor_ReadRPM();
    g_engineData.coolantTemp = Sensor_ReadCoolantTemp();
    g_engineData.throttlePosition = Sensor_ReadThrottle();
    g_engineData.engineTorque = CalculateTorque();
    
    /* 发送信号 */
    if (Com_GetStatus() == COM_READY) {
        Com_SendSignal(ComConf_ComSignal_EngineSpeed, &g_engineData.engineSpeed);
        Com_SendSignal(ComConf_ComSignal_CoolantTemp, &g_engineData.coolantTemp);
        Com_SendSignal(ComConf_ComSignal_ThrottlePosition, &g_engineData.throttlePosition);
        Com_SendSignal(ComConf_ComSignal_EngineTorque, &g_engineData.engineTorque);
    }
    
    TerminateTask();
}

/**
 * @brief 接收车速并调整发动机输出
 */
void EngineControl_ProcessVehicleSpeed(void) {
    uint16 vehicleSpeed;
    uint8 result;
    
    result = Com_ReceiveSignal(ComConf_ComSignal_VehicleSpeed, &vehicleSpeed);
    
    if (result == E_OK) {
        /* 根据车速调整发动机管理 */
        if (vehicleSpeed > 120) {
            /* 高速限制 */
            SetEngineLimitMode(TRUE);
        } else {
            SetEngineLimitMode(FALSE);
        }
    }
}

/**
 * @brief 紧急停机处理
 */
void EngineControl_EmergencyShutdown(void) {
    /* 立即发送停机状态 */
    uint8 engineState = 0; /* STOPPED */
    Com_SendSignal(ComConf_ComSignal_EngineState, &engineState);
    
    /* 强制触发发送 */
    Com_TriggerIPDUSend(ComConf_ComIPdu_EngineStatus);
}
```

### 信号组使用示例

```c
/**
 * @brief 使用信号组发送复杂数据
 */
void SendEngineDiagnostics(void) {
    /* 更新影子缓冲区中的各信号 */
    uint16 rpm = 3500;
    uint8 temp = 90;
    uint16 torque = 200;
    
    Com_UpdateShadowSignal(ComConf_ComSignal_EngineSpeed, &rpm);
    Com_UpdateShadowSignal(ComConf_ComSignal_CoolantTemp, &temp);
    Com_UpdateShadowSignal(ComConf_ComSignal_EngineTorque, &torque);
    
    /* 一次性发送所有数据（确保一致性）*/
    Com_SendSignalGroup(ComConf_ComSignalGroup_EngineCoreInfo);
}

/**
 * @brief 使用信号组接收数据
 */
void ReceiveVehicleDynamics(void) {
    uint8 result;
    
    /* 接收到影子缓冲区 */
    result = Com_ReceiveSignalGroup(ComConf_ComSignalGroup_VehicleDynamics);
    
    if (result == E_OK) {
        /* 读取各信号值 */
        uint16 speed;
        uint16 ws_fl, ws_fr;
        uint8 gear;
        
        Com_ReceiveSignal(ComConf_ComSignal_VehicleSpeed, &speed);
        Com_ReceiveSignal(ComConf_ComSignal_WheelSpeed_FL, &ws_fl);
        Com_ReceiveSignal(ComConf_ComSignal_WheelSpeed_FR, &ws_fr);
        Com_ReceiveSignal(ComConf_ComSignal_GearPosition, &gear);
        
        /* 处理数据 */
        ProcessVehicleDynamics(speed, ws_fl, ws_fr, gear);
    }
}
```

---

## 高级功能

### 传输确认与重试

```c
/**
 * @brief 发送确认处理
 */
void TxConfirmationCallback(void) {
    /* 发送成功 */
    LogEvent(EVENT_TX_SUCCESS);
}

void TxErrorCallback(void) {
    /* 发送失败 */
    LogEvent(EVENT_TX_ERROR);
    
    /* COM 模块自动重试 */
}
```

### 错误统计监控

```c
/**
 * @brief 定期检查错误统计
 */
void CheckErrorStatistics(void) {
    Com_GlobalErrorStatsType stats;
    
    if (Com_Eh_GetErrorStats(&stats) == E_OK) {
        /* 检查队列溢出次数 */
        if (stats.TxQueueOverflowCount > 10) {
            /* 队列频繁溢出，可能需要调整 */
            Diagnostics_ReportHighLoad();
        }
        
        /* 检查错误率 */
        if (!Com_Eh_IsErrorRateAcceptable()) {
            /* 错误率过高 */
            EnterSafeMode();
        }
    }
}
```

### TMC (传输模式条件) 切换

```c
/**
 * @brief 根据当前状态切换传输模式
 */
void UpdateTransmissionMode(void) {
    if (IsDiagnosticMode()) {
        /* 切换到高频率模式 */
        Com_SwitchIpduTxMode(ComConf_ComIPdu_EngineData, TRUE);
    } else {
        /* 正常模式 */
        Com_SwitchIpduTxMode(ComConf_ComIPdu_EngineData, FALSE);
    }
}
```

---

## 调试技巧

### 使用 DET 调试

```c
/* 在开发阶段启用 COM_DEV_ERROR_DETECT */
#define COM_DEV_ERROR_DETECT    STD_ON

/* 错误日志示例 */
void Det_ReportError(uint16 ModuleId, uint8 InstanceId, 
                     uint8 ApiId, uint8 ErrorId) {
    printf("DET Error: Module=%d, API=%d, Error=%d\n", 
           ModuleId, ApiId, ErrorId);
}
```

### 监控队列状态

```c
/**
 * @brief 调试输出队列状态
 */
void DebugPrintQueueStatus(void) {
    Com_TxQueueStatusType status;
    
    if (Com_Eh_GetTxQueueStatus(&status) == E_OK) {
        printf("Queue Status:\n");
        printf("  Fill Level: %d/%d\n", status.FillLevel, status.MaxFillLevel);
        printf("  Is Full: %s\n", status.IsFull ? "Yes" : "No");
        printf("  Strategy: %d\n", status.CurrentStrategy);
    }
}
```

---

## 性能优化

### 1. 合理使用传输属性

| 数据类型 | 推荐属性 | 原因 |
|---------|---------|------|
| 紧急故障码 | TRIGGERED | 必须立即发送 |
| 实时车速 | TRIGGERED_ON_CHANGE | 节省带宽 |
| 定期更新数据 | PENDING | 由周期触发 |

### 2. 信号分组优化

- 将相关信号放入同一组
- 减少单独信号发送调用
- 确保数据一致性

### 3. 队列大小设置

```c
/* 根据实际需求设置队列大小 */
#define COM_MAX_TX_REQUESTS    32u  /* 常规场景 */
/* 或 */
#define COM_MAX_TX_REQUESTS    64u  /* 高负载场景 */
```

---

## 常见问题

### 问题 1: 数据收不到

**可能原因**:
- I-PDU 组未启动
- 信号方向配置错误
- PduR 路由配置错误

**解决方法**:
```c
/* 确保组已启动 */
Com_IpduGroupStart(ComConf_ComIPduGroup_EngineGroup, TRUE);

/* 检查模块状态 */
if (Com_GetStatus() != COM_READY) {
    /* 需要重新初始化 */
}
```

### 问题 2: 发送失败

**可能原因**:
- 队列满
- 模块未初始化
- 传输模式配置错误

**解决方法**:
```c
/* 检查队列状态 */
if (Com_GetTxQueueFillLevel() >= COM_MAX_TX_REQUESTS - 1) {
    /* 队列即将满，处理溢出 */
}
```

### 问题 3: 数据不一致

**可能原因**:
- 字节序配置错误
- 位置偏移计算错误
- 数据类型不匹配

**解决方法**:
- 验证配置工具输出
- 检查 DBC/ARXML 配置
- 确认字节序一致

---

## 附录

### 快速参考卡

```c
/* 必备头文件 */
#include "Com.h"
#include "Com_Types.h"

/* 初始化 */
Com_Init(&ComConfig);
Com_IpduGroupStart(ComConf_ComIPduGroup_EngineGroup, TRUE);

/* 发送 */
Com_SendSignal(ComConf_ComSignal_EngineSpeed, &speed);

/* 接收 */
Com_ReceiveSignal(ComConf_ComSignal_VehicleSpeed, &speed);

/* 主函数 */
Com_MainFunctionTx();
Com_MainFunctionRx();

/* 错误检查 */
if (Com_GetStatus() == COM_READY) { /* ... */ }
```

### 相关文档

- [API 参考](./API_REFERENCE.md)
- [故障排除指南](./TROUBLESHOOTING.md)
- [AUTOSAR SWS COM 4.4.0 规范](https://www.autosar.org)

---

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| v1.0 | 2024-04 | 初始版本 |
