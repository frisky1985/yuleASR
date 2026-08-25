# Icu Design Document

> **Module ID**: 0x16
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Icu  
> **Source Path**: `src/bsw/mcal/icu/`  
> **Reference Document**: `docs/modules/icu.md`  
> **Doc Version**: 1.0  
> **Status**: 已完成

---

## 1. 模块概述

ICU（输入捕获单元）是 AUTOSAR MCAL 层驱动模块。提供边沿检测、信号测量、中断通知和PWM提取服务。

在 AUTOSAR 分层架构中，ICU 位于微控制器驱动层（MCAL），直接操作芯片硬件寄存器，向上为 ECUAL 层和 Services 层提供标准化硬件抽象接口。主要交互模块包括：

- **上层**：EcuM（初始化管理）、SchM（调度）以及使用该驱动的 ECUAL/Services 模块（如 CanIf、LinIf、IoHwAb 等）。
- **同层**：Port、DIO、MCU、GPT 等其它 MCAL 驱动（部分模块存在硬件依赖或时序依赖）。
- **下层**：微控制器硬件寄存器、中断控制器、DMA（若支持）。
- **公共**：Det（开发错误检测，可选）、Dem（诊断事件，可选）。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS Icu | 4.4.0 | 模块软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | CanIf / LinIf / IoHwAb 等 | 使用 Icu 提供硬件服务 | |
| 下层 | 微控制器硬件 | 直接操作芯片寄存器 | |
| 同层 | Port / Mcu / Gpt 等 | 时序或引脚依赖 | |
| 公共 | Det, Dem | 错误追踪与诊断事件（可选） | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        ECUAL / Services / ASW       │
├─────────────────────────────────────┤
│           Icu Driver (MCAL)        │
├─────────────────────────────────────┤
│      Microcontroller Hardware       │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **公共头文件 `Icu.h`**：定义模块 API、服务 ID、错误码、数据类型。
- **配置文件 `Icu_Cfg.h`**：预编译配置开关、数量宏、外部配置表声明。
- **实现文件 `Icu.c`**：API 实现、硬件访问、状态管理、错误处理。
- **链接时配置 `Icu_Lcfg.c`**（若存在）：由配置工具生成的链接时配置数据。
- **中断/硬件抽象文件**（若存在）：处理中断分发或平台相关硬件访问。

### 3.3 文件结构

```
src/bsw/mcal/icu/
include
    ├── Icu.h
    ├── Icu_Cfg.h
    ├── Icu_Lcfg.h
    ├── Icu_Private.h
src
    ├── Icu.c
    ├── Icu_Irq.c
    ├── Icu_Lcfg.c
```

---

## 4. 状态机

Icu 定时/波形模块的通道状态机：

```
[CHANNEL_STOPPED] -- Start() --> [CHANNEL_RUNNING]
[CHANNEL_RUNNING] -- Stop()  --> [CHANNEL_STOPPED]
[CHANNEL_RUNNING] -- 中断/比较匹配 --> [通知回调]
```

全局模块状态在 `Init()` 后从 `UNINIT` 转换为 `INIT`。

---

## 5. 核心数据结构

关键类型定义如下：

```c
typedef struct {...} Icu_DutyCycleType;
```

```c
typedef struct {...} Icu_ChannelConfigType;
```

```c
typedef struct {...} Icu_ConfigType;
```

```c
typedef enum {...} Icu_StateType;
```

```c
typedef enum {...} Icu_SignalEdgeType;
```

```c
typedef enum {...} Icu_InputStateType;
```

```c
typedef enum {...} Icu_ActivationType;
```

```c
typedef enum {...} Icu_ModeType;
```

```c
typedef enum {...} Icu_MeasurementModeType;
```

```c
typedef enum {...} Icu_SignalMeasurementPropertyType;
```

```c
typedef enum {...} Icu_TimestampBufferType;
```

```c
typedef uint8 Icu_ChannelType;
```


---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 | SWS ID |
|-----|------|------|----------------|--------|
| Icu_Init | `void Icu_Init(const Icu_ConfigType* ConfigPtr)` | _Init | 见 Icu.h | SWS_Icu_00001 | SWS_Icu_00001 |
| Icu_DeInit | `void Icu_DeInit(void)` | _DeInit | 见 Icu.h | SWS_Icu_00002 | SWS_Icu_00002 |
| Icu_SetMode | `void Icu_SetMode(Icu_ModeType Mode)` | _SetMode | 见 Icu.h | SWS_Icu_00003 | SWS_Icu_00003 |
| Icu_DisableWakeup | `void Icu_DisableWakeup(Icu_ChannelType Channel)` | _DisableWakeup | 见 Icu.h | SWS_Icu_00004 | SWS_Icu_00004 |
| Icu_EnableWakeup | `void Icu_EnableWakeup(Icu_ChannelType Channel)` | _EnableWakeup | 见 Icu.h | SWS_Icu_00005 | SWS_Icu_00005 |
| Icu_CheckWakeup | `Std_ReturnType Icu_CheckWakeup(uint32 WakeupSource)` | _CheckWakeup | 见 Icu.h | SWS_Icu_00006 | SWS_Icu_00006 |
| Icu_SetActivationCondition | `void Icu_SetActivationCondition(Icu_ChannelType Channel, Icu_ActivationType Activation)` | _SetActivationCondition | 见 Icu.h | SWS_Icu_00007 | SWS_Icu_00007 |
| Icu_DisableNotification | `void Icu_DisableNotification(Icu_ChannelType Channel)` | _DisableNotification | 见 Icu.h | SWS_Icu_00008 | SWS_Icu_00008 |
| Icu_EnableNotification | `void Icu_EnableNotification(Icu_ChannelType Channel)` | _EnableNotification | 见 Icu.h | SWS_Icu_00009 | SWS_Icu_00009 |
| Icu_GetInputState | `Icu_InputStateType Icu_GetInputState(Icu_ChannelType Channel)` | _GetInputState | 见 Icu.h | SWS_Icu_00010 | SWS_Icu_00010 |
| Icu_StartTimestamp | `void Icu_StartTimestamp(Icu_ChannelType Channel, uint32* BufferPtr, uint16 BufferSize, uint16 NotifyInterval)` | _StartTimestamp | 见 Icu.h | SWS_Icu_00011 | SWS_Icu_00011 |
| Icu_StopTimestamp | `void Icu_StopTimestamp(Icu_ChannelType Channel)` | _StopTimestamp | 见 Icu.h | SWS_Icu_00012 | SWS_Icu_00012 |
| Icu_GetTimestampIndex | `Icu_IndexType Icu_GetTimestampIndex(Icu_ChannelType Channel)` | _GetTimestampIndex | 见 Icu.h | SWS_Icu_00013 | SWS_Icu_00013 |
| Icu_ResetEdgeCount | `void Icu_ResetEdgeCount(Icu_ChannelType Channel)` | _ResetEdgeCount | 见 Icu.h | SWS_Icu_00014 | SWS_Icu_00014 |
| Icu_EnableEdgeCount | `void Icu_EnableEdgeCount(Icu_ChannelType Channel)` | _EnableEdgeCount | 见 Icu.h | SWS_Icu_00015 | SWS_Icu_00015 |
| Icu_DisableEdgeCount | `void Icu_DisableEdgeCount(Icu_ChannelType Channel)` | _DisableEdgeCount | 见 Icu.h | SWS_Icu_00016 | SWS_Icu_00016 |
| Icu_GetEdgeNumbers | `uint16 Icu_GetEdgeNumbers(Icu_ChannelType Channel)` | _GetEdgeNumbers | 见 Icu.h | SWS_Icu_00017 | SWS_Icu_00017 |
| Icu_StartSignalMeasurement | `void Icu_StartSignalMeasurement(Icu_ChannelType Channel, Icu_SignalMeasurementPropertyType MeasureKind)` | _StartSignalMeasurement | 见 Icu.h | SWS_Icu_00018 | SWS_Icu_00018 |
| Icu_StopSignalMeasurement | `void Icu_StopSignalMeasurement(Icu_ChannelType Channel)` | _StopSignalMeasurement | 见 Icu.h | SWS_Icu_00019 | SWS_Icu_00019 |
| Icu_GetTimeElapsed | `uint16 Icu_GetTimeElapsed(Icu_ChannelType Channel)` | _GetTimeElapsed | 见 Icu.h | SWS_Icu_00020 | SWS_Icu_00020 |

### 6.2 回调函数

| 回调 | 说明 | |
|------|------|
| Icu_Notification / Icu_Cbk | 若配置启用，由中断或状态变化触发 | |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 | SWS ID |
|-----|-----|------------|--------|
| (0x00U) | INIT | 见实现 | |
| (0x01U) | DEINIT | 见实现 | |
| (0x02U) | SETMODE | 见实现 | |
| (0x03U) | DISABLEWAKEUP | 见实现 | |
| (0x04U) | ENABLEWAKEUP | 见实现 | |
| (0x05U) | CHECKWAKEUP | 见实现 | |
| (0x06U) | SETACTIVATIONCONDITION | 见实现 | |
| (0x07U) | DISABLENOTIFICATION | 见实现 | |
| (0x08U) | ENABLENOTIFICATION | 见实现 | |
| (0x09U) | GETINPUTSTATE | 见实现 | |
| (0x0AU) | STARTTIMESTAMP | 见实现 | |
| (0x0BU) | STOPTIMESTAMP | 见实现 | |
| (0x0CU) | GETTIMESTAMPINDEX | 见实现 | |
| (0x0DU) | RESETEDGECOUNT | 见实现 | |
| (0x0EU) | ENABLEEDGECOUNT | 见实现 | |
| (0x0FU) | DISABLEEDGECOUNT | 见实现 | |
| (0x10U) | GETEDGENUMBERS | 见实现 | |
| (0x11U) | STARTSIGNALMEASUREMENT | 见实现 | |
| (0x12U) | STOPSIGNALMEASUREMENT | 见实现 | |
| (0x13U) | GETTIMEELAPSED | 见实现 | |

开发错误码定义：

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| (0x0AU) | PARAM_CONFIG | 见 Icu.h | |
| (0x0BU) | UNINIT | 见 Icu.h | |
| (0x0CU) | PARAM_CHANNEL | 见 Icu.h | |
| (0x0DU) | PARAM_ACTIVATION | 见 Icu.h | |
| (0x0EU) | PARAM_BUFFER_SIZE | 见 Icu.h | |
| (0x0FU) | ALREADY_INITIALIZED | 见 Icu.h | |
| (0x10U) | PARAM_POINTER | 见 Icu.h | |
| (0x11U) | BUSY | 见 Icu.h | |
| (0x12U) | WAKEUP_NOT_ENABLED | 见 Icu.h | |
| (0x13U) | WAKEUP_ALREADY_ENABLED | 见 Icu.h | |
| (0x14U) | MEASUREMENT_NOT_RUNNING | 见 Icu.h | |
| (0x15U) | MEASUREMENT_RUNNING | 见 Icu.h | |

---

## 7. 处理流程

### 7.1 初始化流程

1. EcuM 在启动阶段调用 `Icu_Init(ConfigPtr)`。
2. 若 `DEV_ERROR_DETECT == STD_ON`，校验 `ConfigPtr` 非空及版本信息。
3. 初始化硬件寄存器、全局状态变量和运行时数据结构。
4. 设置模块初始化标志，模块进入可操作状态。

### 7.2 正常操作/数据处理流程

1. 上层通过标准 API 请求服务（如读取、写入、发送、接收）。
2. 模块校验初始化状态、参数范围和指针有效性（DET 开启时）。
3. 访问硬件寄存器或调用硬件抽象层完成请求。
4. 同步 API 直接返回结果；异步 API 更新作业状态并通过中断/轮询完成。
5. 若启用通知，调用配置的回调函数。

### 7.3 错误处理流程

1. API 入口进行参数和状态检查。
2. 检测到错误时，通过 `Det_ReportError()` 报告开发错误。
3. 函数通常提前返回安全默认值或错误状态，避免影响硬件。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 | |
|----|--------|------|
| ICU_DEV_ERROR_DETECT | STD_ON | 配置开关 | |
| ICU_VERSION_INFO_API | STD_ON | 配置开关 | |
| ICU_EDGE_COUNT_API | STD_ON | 配置开关 | |
| ICU_DE_INIT_API | STD_ON | 配置开关 | |
| ICU_SET_MODE_API | STD_ON | 配置开关 | |
| ICU_DISABLE_WAKEUP_API | STD_ON | 配置开关 | |
| ICU_ENABLE_WAKEUP_API | STD_ON | 配置开关 | |
| ICU_CHECK_WAKEUP_API | STD_ON | 配置开关 | |
| ICU_TIMESTAMP_API | STD_ON | 配置开关 | |
| ICU_SIGNAL_MEASUREMENT_API | STD_ON | 配置开关 | |
| ICU_WAKEUP_FUNCTIONALITY_API | STD_OFF | 配置开关 | |
| ICU_REPORT_WAKEUP_SOURCE | STD_OFF | 配置开关 | |
| ICU_NUM_CHANNELS | (8U) | 数量/ID 宏 | |
| ICU_MAX_EDGE_COUNT | (65535U) | 其它配置 | |
| ICU_MAIN_FUNCTION_PERIOD_MS | (1U) | 其它配置 | |
| ICU_CHANNEL_0 | (0U) | 其它配置 | |
| ICU_CHANNEL_1 | (1U) | 其它配置 | |
| ICU_CHANNEL_2 | (2U) | 其它配置 | |
| ICU_CHANNEL_3 | (3U) | 其它配置 | |
| ICU_CHANNEL_4 | (4U) | 其它配置 | |
| ICU_CHANNEL_5 | (5U) | 其它配置 | |

### 8.2 链接时配置

| 配置表 | 说明 | |
|--------|------|
| `Icu_Lcfg.c` | 由 yuleASR Configurator 生成的链接时配置数据 | |
| `Icu_Config` | 外部配置根结构体声明（位于 `Icu_Cfg.h`） | |

### 8.3 构建后配置

本模块当前未使用 Post-Build 配置；所有配置在编译/链接时确定。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| (0x0AU) | PARAM_CONFIG | 参数或状态错误 | |
| (0x0BU) | UNINIT | 参数或状态错误 | |
| (0x0CU) | PARAM_CHANNEL | 参数或状态错误 | |
| (0x0DU) | PARAM_ACTIVATION | 参数或状态错误 | |
| (0x0EU) | PARAM_BUFFER_SIZE | 参数或状态错误 | |
| (0x0FU) | ALREADY_INITIALIZED | 参数或状态错误 | |
| (0x10U) | PARAM_POINTER | 参数或状态错误 | |
| (0x11U) | BUSY | 参数或状态错误 | |
| (0x12U) | WAKEUP_NOT_ENABLED | 参数或状态错误 | |
| (0x13U) | WAKEUP_ALREADY_ENABLED | 参数或状态错误 | |
| (0x14U) | MEASUREMENT_NOT_RUNNING | 参数或状态错误 | |
| (0x15U) | MEASUREMENT_RUNNING | 参数或状态错误 | |

### 9.2 DEM 错误

本模块当前未定义专用 DEM 事件；相关硬件故障可通过下层或上层模块上报。

### 9.3 安全机制

- ASIL 等级：视具体安全项目分配，通常与使用该模块的上层安全相关功能一致。
- 参数校验：在 `DEV_ERROR_DETECT == STD_ON` 时执行空指针、越界、未初始化检查。
- 安全相关模块（如 Wdg、RamTst）需结合项目安全手册进行额外分析。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 | |
|------|------|
| ICU_START_SEC_CODE | MemMap 代码/数据段 | |
| ICU_START_SEC_VAR_CLEARED_UNSPECIFIED | MemMap 代码/数据段 | |
| ICU_STOP_SEC_CODE | MemMap 代码/数据段 | |
| ICU_STOP_SEC_VAR_CLEARED_UNSPECIFIED | MemMap 代码/数据段 | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | 依赖配置 | 运行时状态变量、通道/作业句柄 | |
| ROM | 依赖配置 | 代码段 + 链接时配置表 | |
| 堆栈 | 依赖调用深度 | 通常为浅层调用，中断处理另计 | |

---

## 11. 集成指南

- **与上层模块集成**：Icu 向上层提供标准 API；上层模块（如 CanIf、LinIf、IoHwAb）在初始化后调用 Icu 服务。
- **与下层硬件集成**：直接访问微控制器外设寄存器，具体寄存器映射与目标平台（NXP S32K312 / i.MX8M Mini 等）相关。
- **初始化顺序**：Icu 通常在 EcuM 的驱动初始化阶段调用；若依赖 Port/Mcu/Gpt，应确保这些模块先完成初始化。
- **中断配置**：若模块使用中断，需在 OS/启动代码中配置对应中断向量，并在 Icu_Irq.c（若存在）中实现 ISR。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 | |
|----------|----------|
| `test_icu.c` | 初始化、API 参数边界、错误处理、MemMap 段 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 与上层模块联调 | 验证 Icu 与 CanIf/LinIf/IoHwAb 等接口行为 | |
| 中断时序测试 | 验证异步操作完成与回调触发时序 | |
| 错误注入测试 | 验证 DET 报告和错误恢复行为 | |

---

## 13. 实现说明 / TODO

- 本设计文档基于当前 `src/bsw/mcal/icu/` 源码自动生成并人工校对。
- 若源码与 AUTOSAR SWS 存在偏差，以源码实现为准并在实现注释中说明。
- 平台相关寄存器定义可能分散在平台头文件或条件编译块中，集成时需结合具体目标芯片手册。

---

## 14. 参考资料

1. AUTOSAR_SWS_Icu.pdf
2. `docs/modules/icu.md`
3. `src/bsw/mcal/icu/`
4. `docs/design/modules/TEMPLATE.md`
