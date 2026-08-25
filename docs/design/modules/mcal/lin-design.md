# Lin Design Document

> **Module ID**: 0x52
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Lin  
> **Source Path**: `src/bsw/mcal/lin/`  
> **Reference Document**: `docs/modules/lin.md`  
> **Doc Version**: 1.0  
> **Status**: 已完成

---

## 1. 模块概述

LIN（LIN总线驱动）是 AUTOSAR MCAL 层驱动模块。提供LIN主/从控制器初始化、帧收发、调度和诊断服务。

在 AUTOSAR 分层架构中，LIN 位于微控制器驱动层（MCAL），直接操作芯片硬件寄存器，向上为 ECUAL 层和 Services 层提供标准化硬件抽象接口。主要交互模块包括：

- **上层**：EcuM（初始化管理）、SchM（调度）以及使用该驱动的 ECUAL/Services 模块（如 CanIf、LinIf、IoHwAb 等）。
- **同层**：Port、DIO、MCU、GPT 等其它 MCAL 驱动（部分模块存在硬件依赖或时序依赖）。
- **下层**：微控制器硬件寄存器、中断控制器、DMA（若支持）。
- **公共**：Det（开发错误检测，可选）、Dem（诊断事件，可选）。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS Lin | 4.4.0 | 模块软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | CanIf / LinIf / IoHwAb 等 | 使用 Lin 提供硬件服务 | |
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
│           Lin Driver (MCAL)        │
├─────────────────────────────────────┤
│      Microcontroller Hardware       │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **公共头文件 `Lin.h`**：定义模块 API、服务 ID、错误码、数据类型。
- **配置文件 `Lin_Cfg.h`**：预编译配置开关、数量宏、外部配置表声明。
- **实现文件 `Lin.c`**：API 实现、硬件访问、状态管理、错误处理。
- **链接时配置 `Lin_Lcfg.c`**（若存在）：由配置工具生成的链接时配置数据。
- **中断/硬件抽象文件**（若存在）：处理中断分发或平台相关硬件访问。

### 3.3 文件结构

```
src/bsw/mcal/lin/
include
    ├── Lin.h
    ├── LinMaster.h
    ├── LinMaster_Cfg.h
    ├── LinMaster_Diagnostic.h
    ├── LinMaster_Hal.h
    ├── LinMaster_Schedule.h
    ├── LinMaster_Tp.h
    ├── LinMaster_Types.h
    ├── LinSlave.h
    ├── LinSlave_Cfg.h
    ├── LinSlave_CfgTable.h
    ├── LinSlave_Checksum.h
    ├── LinSlave_Hal.h
    ├── LinSlave_Pid.h
    ├── LinSlave_Tp.h
    ├── LinSlave_Types.h
    ├── LinSlave_Uds.h
    ├── Lin_Cfg.h
    ├── Std_Types.h
src
    ├── Lin.c
    ├── LinMaster.c
    ├── LinMaster_Diagnostic.c
    ├── LinMaster_Hal.c
    ├── LinMaster_Schedule.c
    ├── LinMaster_Tp.c
    ├── LinSlave.c
    ├── LinSlave_CfgTable.c
    ├── LinSlave_Checksum.c
    ├── LinSlave_Hal.c
    ├── LinSlave_Pid.c
    ├── LinSlave_Tp.c
    ├── LinSlave_Uds.c
```

---

## 4. 状态机

Lin 控制器具有典型的初始化/运行/停止状态机：

```
[UNINIT] -- Init() --> [INIT]
[INIT]   -- Start() / Enable --> [RUNNING]
[RUNNING]-- Stop() / Disable --> [STOPPED]
[STOPPED]-- Start() --> [RUNNING]
```

部分模块还维护通道/报文对象状态（如发送空闲、发送请求、发送完成、接收就绪等），具体见实现中的状态变量或配置结构。

---

## 5. 核心数据结构

关键类型定义如下：

```c
typedef struct {...} Lin_PduType;
```

```c
typedef struct {...} Lin_ChannelConfigType;
```

```c
typedef struct {...} Lin_ConfigType;
```

```c
typedef uint8 Lin_StatusType;
```

```c
typedef uint8 Lin_FrameTypeType;
```

```c
typedef uint8 Lin_FrameResponseType;
```

```c
typedef uint8 Lin_FrameCheckSumType;
```

```c
typedef uint8 Lin_FramePidType;
```

```c
typedef uint8 Lin_ChannelType;
```


---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 | SWS ID |
|-----|------|------|----------------|--------|
| Lin_Init | `extern void Lin_Init(const Lin_ConfigType* Config)` | _Init | 见 Lin.h | SWS_Lin_00001 | SWS_Lin_00001 |
| Lin_DeInit | `extern void Lin_DeInit(void)` | _DeInit | 见 Lin.h | SWS_Lin_00002 | SWS_Lin_00002 |
| Lin_GetVersionInfo | `extern void Lin_GetVersionInfo(Std_VersionInfoType* versioninfo)` | _GetVersionInfo | 见 Lin.h | SWS_Lin_00003 | SWS_Lin_00003 |
| Lin_SendFrame | `extern Std_ReturnType Lin_SendFrame(Lin_ChannelType Channel, const Lin_PduType* PduInfoPtr)` | _SendFrame | 见 Lin.h | SWS_Lin_00004 | SWS_Lin_00004 |
| Lin_SendResponse | `extern Std_ReturnType Lin_SendResponse(Lin_ChannelType Channel, const Lin_PduType* PduInfoPtr)` | _SendResponse | 见 Lin.h | SWS_Lin_00005 | SWS_Lin_00005 |
| Lin_DisableResponse | `extern Std_ReturnType Lin_DisableResponse(Lin_ChannelType Channel)` | _DisableResponse | 见 Lin.h | SWS_Lin_00006 | SWS_Lin_00006 |
| Lin_WakeUp | `extern Std_ReturnType Lin_WakeUp(Lin_ChannelType Channel)` | _WakeUp | 见 Lin.h | SWS_Lin_00007 | SWS_Lin_00007 |
| Lin_WakeUpInternal | `extern Std_ReturnType Lin_WakeUpInternal(Lin_ChannelType Channel)` | _WakeUpInternal | 见 Lin.h | SWS_Lin_00008 | SWS_Lin_00008 |
| Lin_CheckWakeup | `extern Std_ReturnType Lin_CheckWakeup(Lin_ChannelType Channel)` | _CheckWakeup | 见 Lin.h | SWS_Lin_00009 | SWS_Lin_00009 |
| Lin_GetStatus | `extern Lin_StatusType Lin_GetStatus(Lin_ChannelType Channel, uint8** Lin_SduPtr)` | _GetStatus | 见 Lin.h | SWS_Lin_00010 | SWS_Lin_00010 |
| Lin_GoToSleep | `extern Std_ReturnType Lin_GoToSleep(Lin_ChannelType Channel)` | _GoToSleep | 见 Lin.h | SWS_Lin_00011 | SWS_Lin_00011 |
| Lin_GoToSleepInternal | `extern Std_ReturnType Lin_GoToSleepInternal(Lin_ChannelType Channel)` | _GoToSleepInternal | 见 Lin.h | SWS_Lin_00012 | SWS_Lin_00012 |
| Lin_WakeUpConfirmation | `extern void Lin_WakeUpConfirmation(Lin_ChannelType Channel)` | _WakeUpConfirmation | 见 Lin.h | SWS_Lin_00013 | SWS_Lin_00013 |
| Lin_WakeUpFrameIndication | `extern void Lin_WakeUpFrameIndication(void)` | _WakeUpFrameIndication | 见 Lin.h | SWS_Lin_00014 | SWS_Lin_00014 |
| Lin_IsrTx | `extern void Lin_IsrTx(Lin_ChannelType Channel)` | _IsrTx | 见 Lin.h | SWS_Lin_00015 | SWS_Lin_00015 |
| Lin_IsrRx | `extern void Lin_IsrRx(Lin_ChannelType Channel)` | _IsrRx | 见 Lin.h | SWS_Lin_00016 | SWS_Lin_00016 |
| Lin_IsrErr | `extern void Lin_IsrErr(Lin_ChannelType Channel)` | _IsrErr | 见 Lin.h | SWS_Lin_00017 | SWS_Lin_00017 |

### 6.2 回调函数

| 回调 | 说明 | |
|------|------|
| Lin_Notification / Lin_Cbk | 若配置启用，由中断或状态变化触发 | |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 | SWS ID |
|-----|-----|------------|--------|
| — | — | — | |

开发错误码定义：

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| 0x00 | UNINIT | 见 Lin.h | |
| 0x01 | INVALID_CHANNEL | 见 Lin.h | |
| 0x02 | INVALID_POINTER | 见 Lin.h | |
| 0x03 | STATE_TRANSITION | 见 Lin.h | |
| 0x04 | PARAM_VALUE | 见 Lin.h | |
| 0x05 | PARAM_POINTER | 见 Lin.h | |

---

## 7. 处理流程

### 7.1 初始化流程

1. EcuM 在启动阶段调用 `Lin_Init(ConfigPtr)`。
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
| LIN_DEV_ERROR_DETECT | STD_ON | 配置开关 | |
| LIN_VERSION_INFO_API | STD_ON | 配置开关 | |
| LIN_WAKEUP_SUPPORT | STD_ON | 配置开关 | |
| LIN_CH0_WAKEUP_SUPPORT | STD_ON | 配置开关 | |
| LIN_CH1_WAKEUP_SUPPORT | STD_ON | 配置开关 | |
| LIN_BAUDRATE_9600 | (9600U) | 其它配置 | |
| LIN_BAUDRATE_19200 | (19200U) | 其它配置 | |
| LIN_CH0_BAUDRATE | (19200U) | 其它配置 | |
| LIN_CH1_BAUDRATE | (19200U) | 其它配置 | |
| LIN_TIMEOUT | (100U) | 其它配置 | |
| LIN_WAKEUP_TIMEOUT | (50U) | 其它配置 | |
| LIN_MAX_CHANNELS | (2U) | 其它配置 | |
| LIN_CHANNEL_0 | (0U) | 其它配置 | |

### 8.2 链接时配置

| 配置表 | 说明 | |
|--------|------|
| `Lin_Lcfg.c` | 由 yuleASR Configurator 生成的链接时配置数据 | |
| `Lin_Config` | 外部配置根结构体声明（位于 `Lin_Cfg.h`） | |

### 8.3 构建后配置

本模块当前未使用 Post-Build 配置；所有配置在编译/链接时确定。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| 0x00 | UNINIT | 参数或状态错误 | |
| 0x01 | INVALID_CHANNEL | 参数或状态错误 | |
| 0x02 | INVALID_POINTER | 参数或状态错误 | |
| 0x03 | STATE_TRANSITION | 参数或状态错误 | |
| 0x04 | PARAM_VALUE | 参数或状态错误 | |
| 0x05 | PARAM_POINTER | 参数或状态错误 | |

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
| LIN_START_SEC_CODE | MemMap 代码/数据段 | |
| LIN_STOP_SEC_CODE | MemMap 代码/数据段 | |
| LIN_START_SEC_VAR_CLEARED_UNSPECIFIED | MemMap 代码/数据段 | |
| LIN_STOP_SEC_VAR_CLEARED_UNSPECIFIED | MemMap 代码/数据段 | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | 依赖配置 | 运行时状态变量、通道/作业句柄 | |
| ROM | 依赖配置 | 代码段 + 链接时配置表 | |
| 堆栈 | 依赖调用深度 | 通常为浅层调用，中断处理另计 | |

---

## 11. 集成指南

- **与上层模块集成**：Lin 向上层提供标准 API；上层模块（如 CanIf、LinIf、IoHwAb）在初始化后调用 Lin 服务。
- **与下层硬件集成**：直接访问微控制器外设寄存器，具体寄存器映射与目标平台（NXP S32K312 / i.MX8M Mini 等）相关。
- **初始化顺序**：Lin 通常在 EcuM 的驱动初始化阶段调用；若依赖 Port/Mcu/Gpt，应确保这些模块先完成初始化。
- **中断配置**：若模块使用中断，需在 OS/启动代码中配置对应中断向量，并在 Lin_Irq.c（若存在）中实现 ISR。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 | |
|----------|----------|
| `test_lin.c` | 初始化、API 参数边界、错误处理、MemMap 段 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 与上层模块联调 | 验证 Lin 与 CanIf/LinIf/IoHwAb 等接口行为 | |
| 中断时序测试 | 验证异步操作完成与回调触发时序 | |
| 错误注入测试 | 验证 DET 报告和错误恢复行为 | |

---

## 13. 实现说明 / TODO

- 本设计文档基于当前 `src/bsw/mcal/lin/` 源码自动生成并人工校对。
- 若源码与 AUTOSAR SWS 存在偏差，以源码实现为准并在实现注释中说明。
- 平台相关寄存器定义可能分散在平台头文件或条件编译块中，集成时需结合具体目标芯片手册。

---

## 14. 参考资料

1. AUTOSAR_SWS_Lin.pdf
2. `docs/modules/lin.md`
3. `src/bsw/mcal/lin/`
4. `docs/design/modules/TEMPLATE.md`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Lin_00209 | `checksum_types` | 测试 test_checksum_types 覆盖: checksum_types 场景 |
| SWS_Lin_00211 | `state_transitions` | 测试 test_state_transitions 覆盖: state_transitions 场景 |
