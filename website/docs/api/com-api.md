---
title: COM 模块 API 参考
description: "> **遵循标准**: AUTOSAR SWS COM 4.4.0"
sidebar_position: 14
---

# COM 模块 API 参考

> **版本**: v1.0  
> **遵循标准**: AUTOSAR SWS COM 4.4.0  
> **安全等级**: ASIL-D  
> **更新日期**: 2024年

***

## 目录

1. [概述](#概述)
2. [核心 API](#核心-api)
3. [信号操作 API](#信号操作-api)
4. [信号组操作 API](#信号组操作-api)
5. [传输控制 API](#传输控制-api)
6. [I-PDU 组控制 API](#i-pdu-组控制-api)
7. [错误处理 API](#错误处理-api)
8. [回调函数](#回调函数)
9. [配置常量](#配置常量)
10. [错误码](#错误码)

***

## 概述

COM (Communication) 模块是 AUTOSAR 基础软件中的服务层组件，负责 ECU 间的信号级通信。它提供信号打包/解包、传输模式管理、超时监控等功能。

### 头文件引用

```c
#include "Com.h"           /* 主头文件 */
#include "Com_Types.h"     /* 类型定义 */
#include "Com_Cfg.h"       /* 配置常量 */
```

***

## 核心 API

### 1. Com_Init

**功能**: 初始化 COM 模块

**函数原型**:
```c
void Com_Init(const Com_ConfigType* config);
```

**参数**:
| 参数 | 类型 | 描述 |
|******|******|******|
| config | const Com_ConfigType* | 指向配置结构的指针 |

**返回值**: 无

**调用要求**:
- 必须在任何其他 COM API 之前调用
- 只能调用一次（除非先调用 Com_DeInit）

**示例**:
```c
#include "Com.h"

void EcuM_Init(void) {
    /* 初始化 COM 模块 */
    Com_Init(&ComConfig);
    
    /* 启动 I-PDU 组 */
    Com_IpduGroupStart(ComConf_ComIPduGroup_EngineGroup, TRUE);
}
```

**错误码**:
- COM_E_PARAM_POINTER: config 为 NULL
- COM_E_INIT_FAILED: 初始化失败
- COM_E_ALREADY_INITIALIZED: 已经初始化

***

### 2. Com_DeInit

**功能**: 反初始化 COM 模块

**函数原型**:
```c
void Com_DeInit(void);
```

**参数**: 无

**返回值**: 无

**示例**:
```c
void EcuM_Shutdown(void) {
    Com_DeInit();
}
```

***

### 3. Com_GetStatus

**功能**: 获取 COM 模块当前状态

**函数原型**:
```c
Com_StatusType Com_GetStatus(void);
```

**返回值**:
| 值 | 含义 |
|***--|******|
| COM_UNINIT (0) | 未初始化 |
| COM_READY (1) | 就绪状态 |

**示例**:
```c
if (Com_GetStatus() == COM_READY) {
    Com_SendSignal(ComConf_ComSignal_EngineSpeed, &speed);
}
```

***

### 4. Com_GetVersionInfo

**功能**: 获取版本信息

**函数原型**:
```c
void Com_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

**参数**:
| 参数 | 类型 | 描述 |
|******|******|******|
| versioninfo | Std_VersionInfoType* | 版本信息存储指针 |

**示例**:
```c
Std_VersionInfoType version;
Com_GetVersionInfo(&version);
printf("COM Version: %d.%d.%d\n", 
       version.vendorID, 
       version.moduleID, 
       version.instanceID);
```

***

## 信号操作 API

### 5. Com_SendSignal

**功能**: 发送信号

将信号数据复制到 I-PDU 缓冲区，并根据传输属性触发传输。

**函数原型**:
```c
uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);
```

**参数**:
| 参数 | 类型 | 描述 |
|******|******|******|
| SignalId | Com_SignalIdType | 信号标识符 |
| SignalDataPtr | const void* | 信号数据指针 |

**返回值**:
| 值 | 含义 |
|***--|******|
| E_OK (0) | 成功 |
| COM_SERVICE_NOT_AVAILABLE (0x80) | 服务不可用 |
| E_NOT_OK | 失败 |

**传输属性**:
| 属性 | 描述 |
|******|******|
| PENDING | 不触发传输 |
| TRIGGERED | 每次调用都触发传输 |
| TRIGGERED_ON_CHANGE | 数据变化时触发传输 |
| TRIGGERED_ON_CHANGE_WITHOUT_REPETITION | 变化时触发，不重复 |
| TRIGGERED_WITHOUT_REPETITION | 触发但不重复 |

**示例**:
```c
/* 发送发动机转速信号 */
uint16 engineSpeed = 3500;  /* RPM */
uint8 result = Com_SendSignal(ComConf_ComSignal_EngineSpeed, &engineSpeed);

if (result == E_OK) {
    /* 信号发送成功 */
} else {
    /* 处理错误 */
}
```

**线程安全**: 线程安全，可在中断上下文中调用

**性能**: O(1) - 常数时间复杂度

***

### 6. Com_ReceiveSignal

**功能**: 接收信号

从 I-PDU 缓冲区提取信号数据。

**函数原型**:
```c
uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr);
```

**参数**:
| 参数 | 类型 | 描述 |
|******|******|******|
| SignalId | Com_SignalIdType | 信号标识符 |
| SignalDataPtr | void* | 数据存储指针 |

**返回值**:
| 值 | 含义 |
|***--|******|
| E_OK | 成功 |
| E_NOT_OK | 失败 |

**示例**:
```c
uint16 vehicleSpeed;
uint8 result = Com_ReceiveSignal(ComConf_ComSignal_VehicleSpeed, &vehicleSpeed);

if (result == E_OK) {
    /* 使用车速数据 */
    ProcessVehicleSpeed(vehicleSpeed);
}
```

***

## 信号组操作 API

### 7. Com_SendSignalGroup

**功能**: 发送信号组

将影子缓冲区数据复制到 I-PDU 并触发传输。

**函数原型**:
```c
uint8 Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId);
```

**参数**:
| 参数 | 类型 | 描述 |
|******|******|******|
| SignalGroupId | Com_SignalGroupIdType | 信号组标识符 |

**返回值**:
| 值 | 含义 |
|***--|******|
| E_OK | 成功 |
| E_NOT_OK | 失败 |

**工作流程**:
```
1. 调用 Com_UpdateShadowSignal 更新信号到影子缓冲区
2. 调用 Com_SendSignalGroup 发送整个组
```

**示例**:
```c
/* 更新信号组中的各个信号 */
Com_UpdateShadowSignal(ComConf_ComSignal_EngineSpeed, &speed);
Com_UpdateShadowSignal(ComConf_ComSignal_CoolantTemp, &temp);
Com_UpdateShadowSignal(ComConf_ComSignal_ThrottlePosition, &throttle);

/* 发送整个信号组 */
Com_SendSignalGroup(ComConf_ComSignalGroup_EngineCoreInfo);
```

***

### 8. Com_ReceiveSignalGroup

**功能**: 接收信号组

从 I-PDU 复制数据到影子缓冲区。

**函数原型**:
```c
uint8 Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId);
```

***

### 9. Com_UpdateShadowSignal

**功能**: 更新影子信号

更新信号组影子缓冲区中的单个信号。

**函数原型**:
```c
uint8 Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);
```

**示例**:
```c
/* 更新发动机核心信息组中的信号 */
Com_UpdateShadowSignal(ComConf_ComSignal_EngineSpeed, &engineSpeed);
Com_UpdateShadowSignal(ComConf_ComSignal_EngineTorque, &engineTorque);

/* 发送信号组 */
Com_SendSignalGroup(ComConf_ComSignalGroup_EngineCoreInfo);
```

***

### 10. Com_SendSignalGroupArray / Com_ReceiveSignalGroupArray

**功能**: 通过数组发送/接收信号组

**函数原型**:
```c
uint8 Com_SendSignalGroupArray(Com_SignalGroupIdType SignalGroupId, const uint8* SignalGroupArrayPtr);
uint8 Com_ReceiveSignalGroupArray(Com_SignalGroupIdType SignalGroupId, uint8* SignalGroupArrayPtr);
```

**条件编译**: 需要 `COM_ENABLE_SIGNAL_GROUP_ARRAY_API = STD_ON`

***

## 传输控制 API

### 11. Com_MainFunctionRx

**功能**: 接收处理主函数

周期性调用以处理接收数据和超时监控。

**函数原型**:
```c
void Com_MainFunctionRx(void);
```

**调用周期**: 通常 10ms

**任务内容**:
- 处理接收到的 I-PDU
- 信号解包
- 超时监控

**示例配置 (OSEK OS)**:
```c
TASK(ComRxTask) {
    while(1) {
        Com_MainFunctionRx();
        WaitEvent(EVENT_10MS);
    }
}
```

***

### 12. Com_MainFunctionTx

**功能**: 发送处理主函数

周期性调用以处理传输请求和发送 I-PDU。

**函数原型**:
```c
void Com_MainFunctionTx(void);
```

**调用周期**: 通常 10ms

**任务内容**:
- 处理周期性传输
- 触发传输
- 重试逻辑
- 发送确认超时监控

***

### 13. Com_MainFunctionRouteSignals

**功能**: 信号路由主函数 (网关功能)

周期性调用以在不同 I-PDU 之间路由信号。

**函数原型**:
```c
void Com_MainFunctionRouteSignals(void);
```

***

### 14. Com_TriggerIPDUSend

**功能**: 触发 I-PDU 立即发送

无论配置的传输模式如何，都安排 I-PDU 立即传输。

**函数原型**:
```c
Std_ReturnType Com_TriggerIPDUSend(Com_IPduIdType PduId);
```

**参数**:
| 参数 | 类型 | 描述 |
|******|******|******|
| PduId | Com_IPduIdType | I-PDU 标识符 |

**返回值**:
| 值 | 含义 |
|***--|******|
| E_OK | 触发成功 |
| E_NOT_OK | 触发失败 |

**示例**:
```c
/* 紧急情况下立即发送诊断数据 */
void OnDiagnosticEvent(void) {
    Com_TriggerIPDUSend(ComConf_ComIPdu_EngineStatus);
}
```

***

### 15. Com_SwitchIpduTxMode

**功能**: 切换 I-PDU 传输模式

在 TRUE/FALSE 传输模式之间切换（如果配置了）。

**函数原型**:
```c
void Com_SwitchIpduTxMode(Com_IPduIdType PduId, boolean Mode);
```

**参数**:
| 参数 | 类型 | 描述 |
|******|******|******|
| PduId | Com_IPduIdType | I-PDU 标识符 |
| Mode | boolean | TRUE = ComTxModeTrue, FALSE = ComTxModeFalse |

**传输模式**:
| 模式 | 描述 |
|******|******|
| DIRECT | 直接传输 |
| PERIODIC | 周期性传输 |
| MIXED | 混合模式（直接+周期）|
| NONE | 不传输 |

***

### 16. Com_InvalidateSignal / Com_InvalidateSignalGroup

**功能**: 使信号/信号组无效

将信号设置为其配置的无效值并触发传输（如果配置）。

**函数原型**:
```c
void Com_InvalidateSignal(Com_SignalIdType SignalId);
void Com_InvalidateSignalGroup(Com_SignalGroupIdType SignalGroupId);
```

**示例**:
```c
/* 传感器故障时使信号无效 */
if (Sensor_IsFaulty()) {
    Com_InvalidateSignal(ComConf_ComSignal_CoolantTemp);
}
```

***

## I-PDU 组控制 API

### 17. Com_IpduGroupStart

**功能**: 启动 I-PDU 组

**函数原型**:
```c
void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean Initialize);
```

**参数**:
| 参数 | 类型 | 描述 |
|******|******|******|
| IpduGroupId | Com_IpduGroupIdType | I-PDU 组标识符 |
| Initialize | boolean | TRUE = 初始化信号为初始值 |

**预定义 I-PDU 组**:
| 标识符 | 描述 |
|******--|******|
| ComConf_ComIPduGroup_EngineGroup | 发动机相关 I-PDU |
| ComConf_ComIPduGroup_ChassisGroup | 底盘相关 I-PDU |
| ComConf_ComIPduGroup_BodyGroup | 车身相关 I-PDU |

**示例**:
```c
/* 启动发动机相关通信 */
Com_IpduGroupStart(ComConf_ComIPduGroup_EngineGroup, TRUE);
```

***

### 18. Com_IpduGroupStop

**功能**: 停止 I-PDU 组

**函数原型**:
```c
void Com_IpduGroupStop(Com_IpduGroupIdType IpduGroupId);
```

***

## 错误处理 API

### 19. Com_GetTxQueueFillLevel

**功能**: 获取发送队列填充级别

**函数原型**:
```c
uint8 Com_GetTxQueueFillLevel(void);
```

**返回值**: 待处理请求数量

**示例**:
```c
if (Com_GetTxQueueFillLevel() > 10) {
    /* 队列繁忙，可能需要流控 */
}
```

***

### 20. Com_ClearTxQueueForPdu

**功能**: 清除指定 I-PDU 的所有待处理传输请求

**函数原型**:
```c
void Com_ClearTxQueueForPdu(Com_IPduIdType PduId);
```

***

### 21. 错误处理扩展 API (Com_ErrorHandling.h)

#### Com_Eh_Init / Com_Eh_DeInit
```c
void Com_Eh_Init(void);
void Com_Eh_DeInit(void);
```

#### Com_Eh_ReportTxQueueOverflow
```c
Com_TxQueueOverflowStrategyType Com_Eh_ReportTxQueueOverflow(
    Com_IPduIdType PduId,
    Com_TxQueueOverflowStrategyType RequestedStrategy);
```

#### Com_Eh_GetErrorStats
```c
Std_ReturnType Com_Eh_GetErrorStats(Com_GlobalErrorStatsType* StatsPtr);
```

#### Com_Eh_ResetErrorStats
```c
void Com_Eh_ResetErrorStats(void);
```

#### Com_Eh_GetTxQueueStatus
```c
Std_ReturnType Com_Eh_GetTxQueueStatus(Com_TxQueueStatusType* StatusPtr);
```

#### Com_Eh_ValidateStatsIntegrity
```c
Std_ReturnType Com_Eh_ValidateStatsIntegrity(void);
```

**ASIL-D 安全检查**: 验证错误统计数据的完整性

***

## 回调函数

### PduR 到 COM 的接口

```c
/* 接收指示 */
void PduR_ComRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/* 发送确认 */
void PduR_ComTxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/* 触发传输 */
Std_ReturnType PduR_ComTriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);
```

### COM 确认接口

```c
void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
```

***

## 配置常量

### 模块版本

| 常量 | 值 | 描述 |
|******|***--|******|
| COM_VENDOR_ID | 0x0043u | 供应商 ID |
| COM_MODULE_ID | 0x001Eu | 模块 ID |
| COM_SW_MAJOR_VERSION | 0x01u | 主版本 |
| COM_SW_MINOR_VERSION | 0x00u | 次版本 |
| COM_SW_PATCH_VERSION | 0x00u | 补丁版本 |

### 最大数量限制

| 常量 | 值 | 描述 |
|******|***--|******|
| COM_MAX_SIGNALS | 128u | 最大信号数 |
| COM_MAX_SIGNAL_GROUPS | 32u | 最大信号组数 |
| COM_MAX_IPDUS | 64u | 最大 I-PDU 数 |
| COM_MAX_IPDU_GROUPS | 16u | 最大 I-PDU 组数 |
| COM_MAX_IPDU_LENGTH | 64u | 最大 I-PDU 长度 |
| COM_MAX_SHADOW_BUFFER_SIZE | 256u | 最大影子缓冲区大小 |

### 主函数周期

| 常量 | 默认值 | 描述 |
|******|******--|******|
| COM_MAIN_FUNCTION_RX_PERIOD | 10ms | Rx 主函数周期 |
| COM_MAIN_FUNCTION_TX_PERIOD | 10ms | Tx 主函数周期 |
| COM_MAIN_FUNCTION_SIGNAL_PERIOD | 10ms | 信号路由周期 |

***

## 错误码

### 标准错误码

| 错误码 | 值 | 描述 |
|******--|***--|******|
| COM_E_PARAM | 0x01u | 参数错误 |
| COM_E_PARAM_POINTER | 0x02u | 指针参数错误 |
| COM_E_UNINIT | 0x03u | 模块未初始化 |
| COM_E_INIT_FAILED | 0x04u | 初始化失败 |
| COM_E_PARAM_SIGNALID | 0x05u | 信号 ID 错误 |
| COM_E_PARAM_DATASERIESINDEX | 0x06u | 数据系列索引错误 |
| COM_E_PARAM_POINTER_TO_SIGNALGRP | 0x07u | 信号组指针错误 |
| COM_E_ALREADY_INITIALIZED | 0x08u | 已经初始化 |

### 错误处理扩展错误码

| 错误码 | 值 | 描述 |
|******--|***--|******|
| COM_E_TX_QUEUE_OVERFLOW | 0x40u | 发送队列溢出 |
| COM_E_TX_QUEUE_FULL | 0x41u | 发送队列满 |
| COM_E_INVALID_OVERFLOW_STRATEGY | 0x42u | 无效溢出策略 |
| COM_E_STATISTICS_CORRUPTION | 0x43u | 统计数据损坏 |
| COM_E_ERROR_COUNTER_OVERFLOW | 0x44u | 错误计数器溢出 |

### 服务 ID

| 服务 ID | 描述 |
|*********|******|
| 0x01 | Com_Init |
| 0x02 | Com_DeInit |
| 0x03 | Com_IpduGroupStart |
| 0x04 | Com_IpduGroupStop |
| 0x05 | Com_SendSignal |
| 0x06 | Com_ReceiveSignal |
| 0x07 | Com_SendSignalGroup |
| 0x08 | Com_ReceiveSignalGroup |
| 0x09 | Com_InvalidateSignal |
| 0x0A | Com_InvalidateSignalGroup |
| 0x0B | Com_TriggerIPDUSend |
| 0x0C | Com_TriggerIPDUSendWithMetaData |
| 0x0D | Com_MainFunctionRx |
| 0x0E | Com_MainFunctionTx |
| 0x0F | Com_MainFunctionRouteSignals |
| 0x10 | Com_GetVersionInfo |
| 0x11 | Com_SwitchIpduTxMode |
| 0x12 | Com_ReceiveSignalGroupArray |
| 0x13 | Com_SendSignalGroupArray |

***

## 数据类型

### 基本类型

```c
typedef uint16 Com_SignalIdType;        /* 信号 ID */
typedef uint16 Com_SignalGroupIdType;   /* 信号组 ID */
typedef uint16 Com_IPduIdType;          /* I-PDU ID */
typedef uint16 Com_IpduGroupIdType;     /* I-PDU 组 ID */
```

### 枚举类型

```c
/* 信号类型 */
typedef enum {
    COM_BOOLEAN, COM_UINT8, COM_UINT16, COM_UINT32, COM_UINT64,
    COM_SINT8, COM_SINT16, COM_SINT32, COM_SINT64,
    COM_FLOAT32, COM_FLOAT64,
    COM_UINT8_N, COM_UINT16_N, COM_UINT32_N, COM_UINT64_N
} Com_SignalTypeType;

/* 字节序 */
typedef enum {
    COM_LITTLE_ENDIAN,
    COM_BIG_ENDIAN,
    COM_OPAQUE
} Com_SignalEndiannessType;

/* 传输属性 */
typedef enum {
    COM_PENDING,
    COM_TRIGGERED,
    COM_TRIGGERED_ON_CHANGE,
    COM_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION,
    COM_TRIGGERED_WITHOUT_REPETITION
} Com_TransferPropertyType;

/* I-PDU 方向 */
typedef enum {
    COM_SEND,
    COM_RECEIVE
} Com_IPduDirectionType;

/* 传输模式 */
typedef enum {
    COM_DIRECT, COM_MIXED, COM_NONE, COM_PERIODIC
} Com_TransferModeType;

/* 模块状态 */
typedef enum {
    COM_UNINIT = 0,
    COM_READY = 1
} Com_StatusType;
```

***

## 相关文档

- [用户手册](../guides/com-user-manual.md) - 详细使用指南
- [故障排除指南](../guides/com-troubleshooting.md) - 常见问题解决
- [配置指南](../guides/com-config.md) - 配置参数说明

***

## 版本历史

| 版本 | 日期 | 描述 |
|******|******|******|
| v1.0 | 2024-04 | 初始版本 |
