# Mcu Design Document

> **Module ID**: 0x2B
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Mcu  
> **Source Path**: `src/bsw/mcal/mcu/`  
> **Reference Document**: `docs/modules/mcu.md`  
> **Doc Version**: 1.0  
> **Status**: 已完成

---

## 1. 模块概述

MCU（微控制器驱动）是 AUTOSAR MCAL 层驱动模块。提供时钟、复位、低功耗模式和芯片标识等微控制器核心服务。

在 AUTOSAR 分层架构中，MCU 位于微控制器驱动层（MCAL），直接操作芯片硬件寄存器，向上为 ECUAL 层和 Services 层提供标准化硬件抽象接口。主要交互模块包括：

- **上层**：EcuM（初始化管理）、SchM（调度）以及使用该驱动的 ECUAL/Services 模块（如 CanIf、LinIf、IoHwAb 等）。
- **同层**：Port、DIO、MCU、GPT 等其它 MCAL 驱动（部分模块存在硬件依赖或时序依赖）。
- **下层**：微控制器硬件寄存器、中断控制器、DMA（若支持）。
- **公共**：Det（开发错误检测，可选）、Dem（诊断事件，可选）。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS Mcu | 4.4.0 | 模块软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | CanIf / LinIf / IoHwAb 等 | 使用 Mcu 提供硬件服务 | |
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
│           Mcu Driver (MCAL)        │
├─────────────────────────────────────┤
│      Microcontroller Hardware       │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **公共头文件 `Mcu.h`**：定义模块 API、服务 ID、错误码、数据类型。
- **配置文件 `Mcu_Cfg.h`**：预编译配置开关、数量宏、外部配置表声明。
- **实现文件 `Mcu.c`**：API 实现、硬件访问、状态管理、错误处理。
- **链接时配置 `Mcu_Lcfg.c`**（若存在）：由配置工具生成的链接时配置数据。
- **中断/硬件抽象文件**（若存在）：处理中断分发或平台相关硬件访问。

### 3.3 文件结构

```
src/bsw/mcal/mcu/
include
    ├── Mcu.h
    ├── Mcu_Cfg.h
src
    ├── Mcal.c
    ├── Mcu.c
    ├── Mcu_Lcfg.c
```

---

## 4. 状态机

Mcu 模块在初始化后从 `UNINIT` 状态转换为 `INIT` 状态；各通道/实例在启用后进入运行状态。

```
[UNINIT] -- Init() --> [INIT]
[INIT]   -- Enable/Start --> [RUNNING]
[RUNNING]-- Disable/Stop  --> [STOPPED]
```

对于本模块，状态机相对简单，未实现复杂分层状态转换时，本节仍保留以说明基本生命周期。

---

## 5. 核心数据结构

关键类型定义如下：

```c
typedef struct {...} Mcu_PllConfigType;
```

```c
typedef struct {...} Mcu_RamSectionType;
```

```c
typedef struct {...} Mcu_ModeConfigType;
```

```c
typedef struct {...} Mcu_ClockConfigType;
```

```c
typedef struct {...} Mcu_ConfigType;
```

```c
typedef enum {...} Mcu_RamStateType;
```

```c
typedef enum {...} Mcu_StateType;
```

```c
typedef enum {...} Mcu_PllStatusType;
```

```c
typedef enum {...} Mcu_ResetType;
```

```c
typedef uint32 Mcu_ClockType;
```

```c
typedef uint32 Mcu_RawResetType;
```

```c
typedef uint8 Mcu_ModeType;
```


---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 | SWS ID |
|-----|------|------|----------------|--------|
| Mcu_Init | `Std_ReturnType Mcu_Init(const Mcu_ConfigType* ConfigPtr)` | _Init | 见 Mcu.h | SWS_Mcu_00001 | SWS_Mcu_00001 |
| Mcu_InitClock | `Std_ReturnType Mcu_InitClock(Mcu_ClockType ClockSetting)` | _InitClock | 见 Mcu.h | SWS_Mcu_00002 | SWS_Mcu_00002 |
| Mcu_DistributePllClock | `void Mcu_DistributePllClock(void)` | _DistributePllClock | 见 Mcu.h | SWS_Mcu_00003 | SWS_Mcu_00003 |
| Mcu_GetPllStatus | `Mcu_PllStatusType Mcu_GetPllStatus(void)` | _GetPllStatus | 见 Mcu.h | SWS_Mcu_00004 | SWS_Mcu_00004 |
| Mcu_SetMode | `void Mcu_SetMode(Mcu_ModeType McuMode)` | _SetMode | 见 Mcu.h | SWS_Mcu_00005 | SWS_Mcu_00005 |
| Mcu_GetResetReason | `Mcu_ResetType Mcu_GetResetReason(void)` | _GetResetReason | 见 Mcu.h | SWS_Mcu_00006 | SWS_Mcu_00006 |
| Mcu_GetResetRawValue | `Mcu_RawResetType Mcu_GetResetRawValue(void)` | _GetResetRawValue | 见 Mcu.h | SWS_Mcu_00007 | SWS_Mcu_00007 |
| Mcu_PerformReset | `void Mcu_PerformReset(void)` | _PerformReset | 见 Mcu.h | SWS_Mcu_00008 | SWS_Mcu_00008 |
| Mcu_GetVersionInfo | `void Mcu_GetVersionInfo(Std_VersionInfoType* versioninfo)` | _GetVersionInfo | 见 Mcu.h | SWS_Mcu_00009 | SWS_Mcu_00009 |
| Mcu_GetRamState | `Mcu_RamStateType Mcu_GetRamState(void)` | _GetRamState | 见 Mcu.h | SWS_Mcu_00011 | SWS_Mcu_00010 |
| Mcu_InitRamSection | `Std_ReturnType Mcu_InitRamSection(uint8 RamSection)` | _InitRamSection | 见 Mcu.h | SWS_Mcu_00010 | SWS_Mcu_00011 |

### 6.2 回调函数

| 回调 | 说明 | |
|------|------|
| Mcu_Notification / Mcu_Cbk | 若配置启用，由中断或状态变化触发 | |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 | SWS ID |
|-----|-----|------------|--------|
| MCU_API_ID_INIT | INIT | 见实现 | |
| MCU_API_ID_INIT_CLOCK | INIT_CLOCK | 见实现 | |
| MCU_API_ID_DISTRIBUTE_PLL_CLOCK | DISTRIBUTE_PLL_CLOCK | 见实现 | |
| MCU_API_ID_GET_PLL_STATUS | GET_PLL_STATUS | 见实现 | |
| MCU_API_ID_SET_MODE | SET_MODE | 见实现 | |
| MCU_API_ID_GET_RESET_REASON | GET_RESET_REASON | 见实现 | |
| MCU_API_ID_GET_RESET_RAW_VALUE | GET_RESET_RAW_VALUE | 见实现 | |
| MCU_API_ID_PERFORM_RESET | PERFORM_RESET | 见实现 | |
| MCU_API_ID_GET_VERSION_INFO | GET_VERSION_INFO | 见实现 | |
| MCU_API_ID_GET_RAM_STATE | GET_RAM_STATE | 见实现 | |
| MCU_API_ID_INIT_RAM_SECTION | INIT_RAM_SECTION | 见实现 | |

开发错误码定义：

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| 0x0AU | PARAM_CONFIG | 见 Mcu.h | |
| 0x0BU | PARAM_CLOCK | 见 Mcu.h | |
| 0x0CU | PARAM_MODE | 见 Mcu.h | |
| 0x0DU | PLL_NOT_LOCKED | 见 Mcu.h | |
| 0x0EU | UNINIT | 见 Mcu.h | |
| 0x0FU | PARAM_POINTER | 见 Mcu.h | |
| 0x10U | INIT_FAILED | 见 Mcu.h | |
| 0x11U | ALREADY_INITIALIZED | 见 Mcu.h | |
| 0x12U | PARAM_RAMSECTION | 见 Mcu.h | |

---

## 7. 处理流程

### 7.1 初始化流程

1. EcuM 在启动阶段调用 `Mcu_Init(ConfigPtr)`。
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
| MCU_DEV_ERROR_DETECT | STD_ON | 配置开关 | |
| MCU_VERSION_INFO_API | STD_ON | 配置开关 | |
| MCU_GET_RAM_STATE_API | STD_OFF | 配置开关 | |
| MCU_PERFORM_RESET_API | STD_ON | 配置开关 | |
| MCU_INIT_CLOCK_API | STD_ON | 配置开关 | |
| MCU_NO_PLL | STD_OFF | 配置开关 | |
| MCU_NUM_CLOCK_CONFIGS | (1U) | 数量/ID 宏 | |
| MCU_NUM_RAM_SECTIONS | (1U) | 数量/ID 宏 | |
| MCU_NUM_MODES | (4U) | 数量/ID 宏 | |
| MCU_TIMEOUT_US | (10000U) | 其它配置 | |
| MCU_XTAL_FREQUENCY_HZ | (24000000U) | 其它配置 | |
| MCU_SYSTEM_CLOCK_HZ | (1000000000U) | 其它配置 | |
| MCU_BUS_CLOCK_HZ | (500000000U) | 其它配置 | |
| MCU_FLASH_CLOCK_HZ | (100000000U) | 其它配置 | |
| MCU_MODE_NORMAL | (0U) | 其它配置 | |
| MCU_MODE_RUN | (0U) | 其它配置 | |
| MCU_MODE_SLEEP | (1U) | 其它配置 | |

### 8.2 链接时配置

| 配置表 | 说明 | |
|--------|------|
| `Mcu_Lcfg.c` | 由 yuleASR Configurator 生成的链接时配置数据 | |
| `Mcu_Config` | 外部配置根结构体声明（位于 `Mcu_Cfg.h`） | |

### 8.3 构建后配置

本模块当前未使用 Post-Build 配置；所有配置在编译/链接时确定。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| 0x0AU | PARAM_CONFIG | 参数或状态错误 | |
| 0x0BU | PARAM_CLOCK | 参数或状态错误 | |
| 0x0CU | PARAM_MODE | 参数或状态错误 | |
| 0x0DU | PLL_NOT_LOCKED | 参数或状态错误 | |
| 0x0EU | UNINIT | 参数或状态错误 | |
| 0x0FU | PARAM_POINTER | 参数或状态错误 | |
| 0x10U | INIT_FAILED | 参数或状态错误 | |
| 0x11U | ALREADY_INITIALIZED | 参数或状态错误 | |
| 0x12U | PARAM_RAMSECTION | 参数或状态错误 | |

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
| MCU_START_SEC_CODE | MemMap 代码/数据段 | |
| MCU_START_SEC_VAR_CLEARED_UNSPECIFIED | MemMap 代码/数据段 | |
| MCU_STOP_SEC_CODE | MemMap 代码/数据段 | |
| MCU_STOP_SEC_VAR_CLEARED_UNSPECIFIED | MemMap 代码/数据段 | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | 依赖配置 | 运行时状态变量、通道/作业句柄 | |
| ROM | 依赖配置 | 代码段 + 链接时配置表 | |
| 堆栈 | 依赖调用深度 | 通常为浅层调用，中断处理另计 | |

---

## 11. 集成指南

- **与上层模块集成**：Mcu 向上层提供标准 API；上层模块（如 CanIf、LinIf、IoHwAb）在初始化后调用 Mcu 服务。
- **与下层硬件集成**：直接访问微控制器外设寄存器，具体寄存器映射与目标平台（NXP S32K312 / i.MX8M Mini 等）相关。
- **初始化顺序**：Mcu 通常在 EcuM 的驱动初始化阶段调用；若依赖 Port/Mcu/Gpt，应确保这些模块先完成初始化。
- **中断配置**：若模块使用中断，需在 OS/启动代码中配置对应中断向量，并在 Mcu_Irq.c（若存在）中实现 ISR。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 | |
|----------|----------|
| `test_mcu.c` | 初始化、API 参数边界、错误处理、MemMap 段 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 与上层模块联调 | 验证 Mcu 与 CanIf/LinIf/IoHwAb 等接口行为 | |
| 中断时序测试 | 验证异步操作完成与回调触发时序 | |
| 错误注入测试 | 验证 DET 报告和错误恢复行为 | |

---

## 13. 实现说明 / TODO

- 本设计文档基于当前 `src/bsw/mcal/mcu/` 源码自动生成并人工校对。
- 若源码与 AUTOSAR SWS 存在偏差，以源码实现为准并在实现注释中说明。
- 平台相关寄存器定义可能分散在平台头文件或条件编译块中，集成时需结合具体目标芯片手册。

---

## 14. 参考资料

1. AUTOSAR_SWS_Mcu.pdf
2. `docs/modules/mcu.md`
3. `src/bsw/mcal/mcu/`
4. `docs/design/modules/TEMPLATE.md`

## 需求追溯表

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Mcu | — | MCU 模块级需求归属 |
