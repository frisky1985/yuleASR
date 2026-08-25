# Gpt Design Document

> **Module ID**: 0x0E  
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_GPTDriver  
> **Source Path**: `src/bsw/mcal/gpt/`  
> **Reference Document**: `docs/modules/GPT.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Gpt（General Purpose Timer Driver）位于 MCAL 层，负责通用定时器硬件的初始化、启动、停止、时间读取、通知使能/禁止以及预定义定时器（Predefined Timer）服务。Gpt 向上层（Os、EcuM、Wdg、各 SWC 等）提供硬件无关的定时功能，支持一次性（One-Shot）与连续（Continuous）通道模式，以及 NORMAL/SLEEP 两种模块运行模式。

主要上下游模块：
- 上层：Os（Tick 源）、EcuM（Sleep/Wakeup）、Wdg（看门狗喂狗周期）、用户 SWC
- 下层：MCU 时钟、中断控制器、GPT/PIT 硬件
- 公共：Det（开发错误检测）

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS GPT Driver | 4.4.0 | GPT 驱动软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | Os / EcuM / Wdg / SWC | 使用定时服务 | |
| 下层 | MCU / Port / Interrupt | 时钟、引脚、中断路由 | |
| 同层 | - | - | |
| 公共 | Det | 开发错误检测（可选） | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│           Upper Layers              │
│    Os / EcuM / Wdg / Application    │
├─────────────────────────────────────┤
│           Gpt (MCAL)                │
├─────────────────────────────────────┤
│   MCU Clock / Interrupt / GPT-PIT   │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **初始化与反初始化子组件**：`Gpt_Init` / `Gpt_DeInit`，完成通道复位、分频配置、中断关闭。
- **通道控制子组件**：`Gpt_StartTimer` / `Gpt_StopTimer`、使能/禁止通知。
- **时间读取子组件**：`Gpt_GetTimeElapsed` / `Gpt_GetTimeRemaining` / `Gpt_GetPredefTimerValue`。
- **模式管理子组件**：`Gpt_SetMode`、Sleep/Wakeup 控制（可选）。
- **硬件抽象子组件**：`Gpt_GetBaseAddr`、`Gpt_GetChannelOffset`、`Gpt_EnableClock` / `Gpt_DisableClock`。

### 3.3 文件结构

```
src/bsw/mcal/gpt/
├── include/
│   ├── Gpt.h          # 公共 API 与类型定义
│   └── Gpt_Cfg.h      # 预编译配置宏
└── src/
    ├── Gpt.c          # 主实现
    └── Gpt_Lcfg.c     # 链接时配置
```

---

## 4. 状态机

### 4.1 模块模式

```
[GPT_MODE_NORMAL] -- Gpt_SetMode(SLEEP) --> [GPT_MODE_SLEEP]
[GPT_MODE_SLEEP]  -- Gpt_SetMode(NORMAL) --> [GPT_MODE_NORMAL]
```

进入 SLEEP 模式时，当前实现会停止所有运行中的通道。

### 4.2 通道运行状态

```
[STOPPED] -- Gpt_StartTimer --> [RUNNING]
[RUNNING] -- Gpt_StopTimer  --> [STOPPED]
[RUNNING] -- 到达目标值（One-Shot）--> [STOPPED]
[RUNNING] -- 到达目标值（Continuous）--> [RUNNING]（自动重载/自由运行）
```

---

## 5. 核心数据结构

| 类型 | 说明 | |
|------|------|
| `Gpt_ChannelType` | 通道索引类型，`uint8` | |
| `Gpt_ValueType` | 定时器计数值类型，`uint32` | |
| `Gpt_ModeType` | 模块模式：`GPT_MODE_NORMAL` / `GPT_MODE_SLEEP` | |
| `Gpt_PredefTimerType` | 预定义定时器：`1US_16BIT` / `1US_24BIT` / `1US_32BIT` / `100US_32BIT` | |
| `Gpt_ChannelModeType` | 通道模式：`GPT_CH_MODE_CONTINUOUS` / `GPT_CH_MODE_ONESHOT` | |
| `Gpt_ClockPrescalerType` | 时钟预分频：`1` / `2` / `4` / ... / `128` | |
| `Gpt_ChannelConfigType` | 单通道配置：ChannelId、BaseAddress、模式、分频、最大 Tick 值、时钟频率、Wakeup、通知开关、通知函数 | |
| `Gpt_ConfigType` | 模块总配置：通道数组、数量、DevErrorDetect、VersionInfoApi、Wakeup、各 API 开关、默认模式、预定义定时器开关 | |

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 | SWS ID |
|-----|------|------|----------------|--------|
| Gpt_Init | `void Gpt_Init(const Gpt_ConfigType* ConfigPtr)` | 初始化 GPT 模块 | 配置所有通道 | SWS_Gpt_00001 | SWS_Gpt_00001 |
| Gpt_DeInit | `void Gpt_DeInit(void)` | 反初始化 | 有通道运行时拒绝 | SWS_Gpt_00002 | SWS_Gpt_00002 |
| Gpt_GetTimeElapsed | `Gpt_ValueType Gpt_GetTimeElapsed(Gpt_ChannelType Channel)` | 获取已运行时间 | 返回 CNT 寄存器值 | SWS_Gpt_00003 | SWS_Gpt_00003 |
| Gpt_GetTimeRemaining | `Gpt_ValueType Gpt_GetTimeRemaining(Gpt_ChannelType Channel)` | 获取剩余时间 | 仅在运行通道有效 | SWS_Gpt_00004 | SWS_Gpt_00004 |
| Gpt_StartTimer | `void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value)` | 启动定时器 | Value 范围 [1, MaxTickValue] | SWS_Gpt_00005 | SWS_Gpt_00005 |
| Gpt_StopTimer | `void Gpt_StopTimer(Gpt_ChannelType Channel)` | 停止定时器 | 禁止中断 | SWS_Gpt_00006 | SWS_Gpt_00006 |
| Gpt_EnableNotification | `void Gpt_EnableNotification(Gpt_ChannelType Channel)` | 使能通道通知中断 | - | SWS_Gpt_00007 | SWS_Gpt_00007 |
| Gpt_DisableNotification | `void Gpt_DisableNotification(Gpt_ChannelType Channel)` | 禁止通道通知中断 | - | SWS_Gpt_00008 | SWS_Gpt_00008 |
| Gpt_GetVersionInfo | `void Gpt_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 版本信息 | - | SWS_Gpt_00009 | SWS_Gpt_00009 |
| Gpt_SetMode | `void Gpt_SetMode(Gpt_ModeType Mode)` | 设置 NORMAL/SLEEP 模式 | SLEEP 停止所有通道 | SWS_Gpt_00010 | SWS_Gpt_00010 |
| Gpt_DisableWakeup | `void Gpt_DisableWakeup(Gpt_ChannelType Channel)` | 禁止通道唤醒 | 受 `GPT_WAKEUP_FUNCTIONALITY_API` 控制 | SWS_Gpt_00011 | SWS_Gpt_00011 |
| Gpt_EnableWakeup | `void Gpt_EnableWakeup(Gpt_ChannelType Channel)` | 使能通道唤醒 | 同上 | SWS_Gpt_00012 | SWS_Gpt_00012 |
| Gpt_CheckWakeup | `Std_ReturnType Gpt_CheckWakeup(Gpt_ChannelType Channel)` | 检查唤醒源 | 同上 | SWS_Gpt_00013 | SWS_Gpt_00013 |
| Gpt_GetPredefTimerValue | `Std_ReturnType Gpt_GetPredefTimerValue(Gpt_PredefTimerType PredefTimer, uint32* TimeValuePtr)` | 读取预定义定时器 | 受各使能宏控制 | SWS_Gpt_00014 | SWS_Gpt_00014 |

### 6.2 回调函数

| 回调 | 说明 | |
|------|------|
| `Gpt_ChannelConfigType.NotificationFn` | 通道到达目标值时调用的通知函数 | |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 | SWS ID |
|-----|-----|------------|--------|
| 0x00 | Gpt_Init | `GPT_E_PARAM_POINTER` / `GPT_E_ALREADY_INITIALIZED` | SWS_Gpt_00001 | SWS_Gpt_00015 |
| 0x01 | Gpt_DeInit | `GPT_E_UNINIT` | SWS_Gpt_00002 | SWS_Gpt_00016 |
| 0x02 | Gpt_GetTimeElapsed | `GPT_E_UNINIT` / `GPT_E_PARAM_CHANNEL` | SWS_Gpt_00003 | SWS_Gpt_00017 |
| 0x03 | Gpt_GetTimeRemaining | `GPT_E_UNINIT` / `GPT_E_PARAM_CHANNEL` | SWS_Gpt_00004 | SWS_Gpt_00018 |
| 0x04 | Gpt_StartTimer | `GPT_E_UNINIT` / `GPT_E_PARAM_CHANNEL` / `GPT_E_PARAM_VALUE` / `GPT_E_CHANNEL_BUSY` | SWS_Gpt_00005 | SWS_Gpt_00019 |
| 0x05 | Gpt_StopTimer | `GPT_E_UNINIT` / `GPT_E_PARAM_CHANNEL` | SWS_Gpt_00006 | SWS_Gpt_00020 |
| 0x06 | Gpt_EnableNotification | `GPT_E_UNINIT` / `GPT_E_PARAM_CHANNEL` | SWS_Gpt_00007 | SWS_Gpt_00021 |
| 0x07 | Gpt_DisableNotification | `GPT_E_UNINIT` / `GPT_E_PARAM_CHANNEL` | SWS_Gpt_00008 | SWS_Gpt_00022 |
| 0x08 | Gpt_GetVersionInfo | `GPT_E_PARAM_POINTER` | SWS_Gpt_00009 | SWS_Gpt_00023 |
| 0x09 | Gpt_SetMode | `GPT_E_UNINIT` / `GPT_E_PARAM_MODE` | SWS_Gpt_00010 | SWS_Gpt_00024 |
| 0x0A | Gpt_DisableWakeup | `GPT_E_UNINIT` / `GPT_E_PARAM_CHANNEL` | SWS_Gpt_00011 | SWS_Gpt_00025 |
| 0x0B | Gpt_EnableWakeup | `GPT_E_UNINIT` / `GPT_E_PARAM_CHANNEL` | SWS_Gpt_00012 | SWS_Gpt_00026 |
| 0x0C | Gpt_CheckWakeup | `GPT_E_UNINIT` / `GPT_E_PARAM_CHANNEL` | SWS_Gpt_00013 | SWS_Gpt_00027 |
| 0x0D | Gpt_GetPredefTimerValue | `GPT_E_UNINIT` / `GPT_E_PARAM_POINTER` / `GPT_E_PARAM_PREDEF_TIMER` | SWS_Gpt_00014 | SWS_Gpt_00028 |

---

## 7. 处理流程

### 7.1 初始化流程

1. `Gpt_Init` 检查配置指针与重复初始化。
2. 保存配置指针。
3. 遍历每个通道：
   - 根据 ChannelId 获取硬件 BaseAddress（通道 0-3 使用 GPT1，4-7 使用 GPT2；S32K312 上均映射到 PIT_BASE）。
   - 使能时钟。
   - 软件复位控制寄存器并等待复位完成（带超时保护）。
   - 根据分频配置计算并写入 Prescaler 寄存器。
   - 配置自由运行模式、时钟源，清除状态，关闭中断。
   - 初始化运行标志与目标/已运行值为 0。
4. 设置模块模式为默认模式，`Gpt_DriverInitialized = TRUE`。

### 7.2 启动定时器流程

1. `Gpt_StartTimer` 校验初始化、通道号、目标值范围、通道未运行。
2. 记录目标值，设置运行标志。
3. 根据通道偏移写入对应 OCR 寄存器。
4. 若通知使能，则设置对应 OF 中断使能位。
5. 置位 GPT_CR_EN 启动定时器。

### 7.3 停止定时器流程

1. `Gpt_StopTimer` 校验初始化与通道号。
2. 清除对应 OF 中断使能位。
3. 清除 GPT_CR_EN 停止定时器。
4. 清除运行标志。

### 7.4 时间读取流程

- `Gpt_GetTimeElapsed` 直接读取 GPT_CNT 寄存器返回当前计数值。
- `Gpt_GetTimeRemaining` 在通道运行状态下返回 `TargetValue - Cnt`，否则返回 0。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 | |
|----|--------|------|
| `GPT_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 | |
| `GPT_VERSION_INFO_API` | STD_ON | 版本信息 API | |
| `GPT_NUM_CHANNELS` | 8U | 通道数量 | |
| `GPT_MAIN_FUNCTION_PERIOD_MS` | 1U | 主函数周期 | |
| `GPT_DEINIT_API` | STD_ON | DeInit API 开关 | |
| `GPT_TIME_ELAPSED_API` | STD_ON | TimeElapsed API 开关 | |
| `GPT_TIME_REMAINING_API` | STD_ON | TimeRemaining API 开关 | |
| `GPT_ENABLE_DISABLE_NOTIFICATION_API` | STD_ON | 通知使能/禁止 API 开关 | |
| `GPT_WAKEUP_FUNCTIONALITY_API` | STD_OFF | 唤醒功能 API 开关 | |
| `GPT_REPORT_WAKEUP_SOURCE` | STD_OFF | 唤醒源报告 | |
| `GPT_PREDEF_TIMER_1US_16BIT_ENABLE` | STD_ON | 1us 16bit 预定义定时器 | |
| `GPT_PREDEF_TIMER_1US_24BIT_ENABLE` | STD_OFF | 1us 24bit 预定义定时器 | |
| `GPT_PREDEF_TIMER_1US_32BIT_ENABLE` | STD_ON | 1us 32bit 预定义定时器 | |
| `GPT_PREDEF_TIMER_100US_32BIT_ENABLE` | STD_ON | 100us 32bit 预定义定时器 | |
| `GPT_DEFAULT_MODE` | `GPT_MODE_NORMAL` | 默认模块模式 | |
| `GPT_CLOCK_FREQUENCY_HZ` | 24000000U | 定时器时钟频率 | |
| `GPT_MAX_TICK_VALUE` | 4294967295U | 最大计数值 | |

### 8.2 链接时配置

| 配置表 | 说明 | |
|--------|------|
| `Gpt_Lcfg.c` | `Gpt_ChannelConfigType[]`、`Gpt_Config` | |

### 8.3 构建后配置

当前实现未使用 Post-Build 配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| 0x0A | `GPT_E_PARAM_CHANNEL` | 通道号越界 | |
| 0x0B | `GPT_E_PARAM_VALUE` | 目标值非法（0 或超过 MaxTickValue） | |
| 0x0C | `GPT_E_PARAM_POINTER` | 空指针 | |
| 0x0D | `GPT_E_PARAM_MODE` | 模式参数非法 | |
| 0x0E | `GPT_E_PARAM_PREDEF_TIMER` | 预定义定时器参数非法 | |
| 0x0F | `GPT_E_ALREADY_INITIALIZED` | 重复初始化 | |
| 0x10 | `GPT_E_CHANNEL_BUSY` | 通道已在运行 | |
| 0x11 | `GPT_E_UNINIT` | 模块未初始化 | |
| 0x12 | `GPT_E_INIT_FAILED` | 初始化失败 | |
| 0x13 | `GPT_E_PARAM_CONFIG` | 配置参数无效 | |

### 9.2 DEM 错误

当前实现未定义 Dem 事件。

### 9.3 安全机制

- 通道号、目标值范围、初始化状态校验。
- 软件复位超时保护。
- DeInit 时检查是否有通道仍在运行，防止异常关闭。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 | |
|------|------|
| `GPT_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据段 | |
| `GPT_STOP_SEC_CONFIG_DATA_UNSPECIFIED` | - | |
| `GPT_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化变量 | |
| `GPT_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | - | |
| `GPT_START_SEC_CODE` | 代码段 | |
| `GPT_STOP_SEC_CODE` | - | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | `GPT_NUM_CHANNELS * (2 * 4 + 1)` + 状态变量 | 目标值、已运行值、运行标志数组 | |
| ROM | 代码 + 通道配置表 | - | |
| 堆栈 | 浅 | API 调用嵌套较浅 | |

---

## 11. 集成指南

- 与上层集成：Os 可将某个 Gpt 通道配置为 System Timer Tick 源；EcuM 通过 `Gpt_SetMode(SLEEP)` 进入低功耗。
- 与下层集成：需根据目标芯片确认 `GPT1_BASE_ADDR` / `GPT2_BASE_ADDR`；S32K312 使用 PIT，默认基地址在 `S32K312.h` 中定义。
- 初始化顺序：MCU → Gpt（在 Os/EcuM 启动之前）。
- 中断路由：将各通道比较匹配中断挂接到中断向量表，并在 ISR 中调用对应 `NotificationFn`。
- 注意：当前 `Gpt_EnableClock` / `Gpt_DisableClock` 为空实现，需根据 MCU 时钟树补全。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 | |
|----------|----------|
| `test_Gpt.c` | 初始化、启动/停止、时间读取、通知使能/禁止、模式切换、预定义定时器、错误注入 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 定时中断测试 | 验证 One-Shot 与 Continuous 模式通知触发 | |
| SLEEP/WAKEUP | 验证 SLEEP 模式停止通道，WAKEUP 后恢复 | |
| Predef Timer | 验证不同位宽预定义定时器返回值 | |

---

## 13. 实现说明 / TODO

- 源码头文件 `Gpt.h` 中 `GPT_MODULE_ID` 定义为 `0x79`；本设计文档按任务要求使用 `0x0E`，二者不一致，需统一。
- 当前 `Gpt_EnableClock` / `Gpt_DisableClock` 为空函数，需根据目标 MCU 实现时钟门控。
- `Gpt_StopTimer` 在禁止中断时仅清除 `GPT_IR_OF1IE`，未按通道偏移清除，建议修正为 `~(GPT_IR_OF1IE << chOffset)`。
- 当前未实现 One-Shot 模式到达目标值后自动停止硬件的逻辑，依赖上层在通知中调用 `Gpt_StopTimer`。
- 预定义定时器 `Gpt_GetPredefTimerValue` 直接读取 GPT1 CNT，未按位宽与分辨率截断，需根据 `PredefTimer` 参数处理。
- S32K312 分支将 GPT1/GPT2 均映射到 PIT_BASE，需确认 PIT 通道与 GPT 通道的对应关系。
- 未实现 `Gpt_CheckWakeup` 的真实唤醒源检测。

---

## 14. 参考资料

1. AUTOSAR_SWS_GPTDriver.pdf
2. `docs/modules/GPT.md`
3. `src/bsw/mcal/gpt/`

## 需求追溯表

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Gpt | — | GPT 模块级需求归属 |
