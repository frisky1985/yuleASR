# Ocu Design Document

> **Module ID**: 0x17
> **AUTOSAR Layer**: MCAL
> **AUTOSAR Version**: Classic Platform 4.4.0
> **SWS Reference**: AUTOSAR_SWS_OCU
> **Source Path**: `src/bsw/mcal/ocu/`
> **Reference Document**: `docs/modules/OCU.md`
> **Doc Version**: 1.0
> **Status**: Draft

---

## 1. 模块概述

Ocu (Output Compare Unit) Driver 位于 MCAL 层，用于在计数器达到预设阈值时触发输出引脚动作（置高、置低、翻转、保持）并可选产生通知中断。本实现面向通用 32-bit 输出比较硬件模型，支持绝对阈值与相对阈值设置，适用于需要精确时序控制、PWM 生成、事件触发等场景。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS OCU | 4.4.0 | OCU 驱动软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |
| MCU Timer / Output Compare Reference Manual | - | 输出比较硬件单元 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | Pwm、IoHwAb、CDD | 时序控制、PWM 生成 | |
| 下层 | 输出比较硬件寄存器 | 直接寄存器操作 | |
| 同层 | Port, Mcu, Gpt | 引脚复用、模块时钟、时基 | |
| 公共 | Det | 开发错误检测 | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   Upper Layers (ECUAL / Services)   │
├─────────────────────────────────────┤
│         Ocu Driver (MCAL)           │
├─────────────────────────────────────┤
│   Port / Mcu / Gpt (MCAL)           │
│   Output Compare Hardware           │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Ocu.c**：核心 API 实现，包括初始化、通道启停、阈值设置、通知管理。
- **Ocu_Irq.c**：中断处理与硬件抽象层（HAL）实现，包含 ISR 入口与寄存器访问函数。
- **Ocu.h**：公共接口、类型、错误码、Service ID。
- **Ocu_Cfg.h / Ocu_Lcfg.h / Ocu_Private.h**：配置宏、链接时配置、内部类型与宏。

### 3.3 文件结构

```
src/bsw/mcal/ocu/
├── include/
│   ├── Ocu.h
│   ├── Ocu_Cfg.h
│   ├── Ocu_Lcfg.h
│   └── Ocu_Private.h
└── src/
    ├── Ocu.c
    └── Ocu_Irq.c
```

---

## 4. 状态机

### 4.1 模块状态

```
[OCU_UNINIT] -- Ocu_Init() --> [OCU_INITIALIZED]
[OCU_INITIALIZED] -- Ocu_DeInit() --> [OCU_UNINIT]
```

### 4.2 通道状态

```
[OCU_STOPPED] -- Ocu_StartChannel() --> [OCU_RUNNING]
[OCU_RUNNING] -- Ocu_StopChannel() --> [OCU_STOPPED]
```

---

## 5. 核心数据结构

```c
/* 通道配置 */
typedef struct {
    Ocu_ChannelType         ChannelId;
    Ocu_OutputPinStateType  DefaultPinState;  /* OCU_HIGH / OCU_LOW */
    Ocu_ValueType           DefaultThreshold; /* 默认比较阈值 */
    Ocu_NotificationType    Notification;     /* 通知回调 */
    boolean                 RunningInBackground;
    uint32                  BaseAddress;      /* 硬件寄存器基地址 */
} Ocu_ChannelConfigType;

/* 全局配置 */
typedef struct {
    const Ocu_ChannelConfigType* Channels;
    uint8                   NumChannels;
    boolean                 DevErrorDetect;
    boolean                 VersionInfoApi;
    boolean                 DeInitApi;
    boolean                 PinStateApi;
    boolean                 SetPinActionApi;
    boolean                 SetThresholdApi;
    boolean                 NotificationSupported;
    Ocu_ValueType           MaxCounterValue;
} Ocu_ConfigType;

/* 通道运行时状态 */
typedef struct {
    Ocu_StateType           State;              /* OCU_STOPPED / OCU_RUNNING */
    Ocu_OutputPinStateType  CurrentPinState;
    Ocu_ValueType           CompareValue;
    Ocu_PinActionType       PinAction;          /* SET_HIGH / SET_LOW / TOGGLE / HOLD */
    boolean                 IsRunning;
    boolean                 NotificationEnabled;
} Ocu_ChannelStateType;

/* 硬件寄存器抽象 */
typedef struct {
    volatile uint32 Control;
    volatile uint32 Status;
    volatile uint32 Counter;
    volatile uint32 Compare;
    volatile uint32 Action;
    volatile uint32 PinCtrl;
} Ocu_HwRegisterType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 | SWS ID |
|-----|------|------|----------------|--------|
| Ocu_Init | `void Ocu_Init(const Ocu_ConfigType* ConfigPtr)` | 初始化 OCU 驱动 | Pre-compile 模式下忽略入参 | SWS_Ocu_00001 | SWS_Ocu_00001 |
| Ocu_DeInit | `void Ocu_DeInit(void)` | 反初始化 | 停止所有通道 | SWS_Ocu_00002 | SWS_Ocu_00002 |
| Ocu_StartChannel | `void Ocu_StartChannel(Ocu_ChannelType Channel)` | 启动通道 | 使能计数器 | SWS_Ocu_00003 | SWS_Ocu_00003 |
| Ocu_StopChannel | `void Ocu_StopChannel(Ocu_ChannelType Channel)` | 停止通道 | 禁用计数器 | SWS_Ocu_00004 | SWS_Ocu_00004 |
| Ocu_SetPinState | `void Ocu_SetPinState(Channel, Ocu_OutputPinStateType PinState)` | 直接设置引脚电平 | 仅停止时可用 | SWS_Ocu_00005 | SWS_Ocu_00005 |
| Ocu_SetPinAction | `void Ocu_SetPinAction(Channel, Ocu_PinActionType PinAction)` | 设置比较匹配动作 | | SWS_Ocu_00006 | SWS_Ocu_00006 |
| Ocu_SetAbsoluteThreshold | `Std_ReturnType Ocu_SetAbsoluteThreshold(Channel, ReferenceValue, AbsoluteValue)` | 设置绝对比较阈值 | | SWS_Ocu_00007 | SWS_Ocu_00007 |
| Ocu_SetRelativeThreshold | `Std_ReturnType Ocu_SetRelativeThreshold(Channel, RelativeValue)` | 设置相对比较阈值 | 相对当前计数器值 | SWS_Ocu_00008 | SWS_Ocu_00008 |
| Ocu_GetCounter | `Ocu_ValueType Ocu_GetCounter(Channel)` | 获取当前计数值 | | SWS_Ocu_00009 | SWS_Ocu_00009 |
| Ocu_DisableNotification | `void Ocu_DisableNotification(Channel)` | 关闭通知 | | SWS_Ocu_00010 | SWS_Ocu_00010 |
| Ocu_EnableNotification | `void Ocu_EnableNotification(Channel)` | 使能通知 | | SWS_Ocu_00011 | SWS_Ocu_00011 |
| Ocu_GetVersionInfo | `void Ocu_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 获取版本信息 | | SWS_Ocu_00012 | SWS_Ocu_00012 |

### 6.2 回调函数

| 回调 | 说明 | |
|------|------|
| Notification | 每个通道独立配置的比较匹配通知回调 | |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 | SWS ID |
|-----|-----|------------|--------|
| 0x00 | Ocu_Init | OCU_E_PARAM_POINTER, OCU_E_ALREADY_INITIALIZED | SWS_Ocu_00001 | SWS_Ocu_00013 |
| 0x01 | Ocu_DeInit | OCU_E_UNINIT | SWS_Ocu_00002 | SWS_Ocu_00014 |
| 0x02 | Ocu_StartChannel | OCU_E_UNINIT, OCU_E_PARAM_CHANNEL, OCU_E_CHANNEL_BUSY | SWS_Ocu_00003 | SWS_Ocu_00015 |
| 0x03 | Ocu_StopChannel | OCU_E_UNINIT, OCU_E_PARAM_CHANNEL | SWS_Ocu_00004 | SWS_Ocu_00016 |
| 0x04 | Ocu_SetPinState | OCU_E_UNINIT, OCU_E_PARAM_CHANNEL, OCU_E_PARAM_PIN_STATE, OCU_E_PARAM_INVALID_STATE | SWS_Ocu_00005 | SWS_Ocu_00017 |
| 0x05 | Ocu_SetPinAction | OCU_E_UNINIT, OCU_E_PARAM_CHANNEL, OCU_E_PARAM_ACTION | SWS_Ocu_00006 | SWS_Ocu_00018 |
| 0x06 | Ocu_SetAbsoluteThreshold | OCU_E_UNINIT, OCU_E_PARAM_CHANNEL, OCU_E_PARAM_REF_VALUE, OCU_E_PARAM_THRESHOLD_VALUE | SWS_Ocu_00007 | SWS_Ocu_00019 |
| 0x07 | Ocu_SetRelativeThreshold | OCU_E_UNINIT, OCU_E_PARAM_CHANNEL, OCU_E_PARAM_THRESHOLD_VALUE | SWS_Ocu_00008 | SWS_Ocu_00020 |
| 0x08 | Ocu_GetCounter | OCU_E_UNINIT, OCU_E_PARAM_CHANNEL | SWS_Ocu_00009 | SWS_Ocu_00021 |
| 0x09 | Ocu_DisableNotification | OCU_E_UNINIT, OCU_E_PARAM_CHANNEL | SWS_Ocu_00010 | SWS_Ocu_00022 |
| 0x0A | Ocu_EnableNotification | OCU_E_UNINIT, OCU_E_PARAM_CHANNEL | SWS_Ocu_00011 | SWS_Ocu_00023 |
| 0x0B | Ocu_GetVersionInfo | OCU_E_PARAM_POINTER | SWS_Ocu_00012 | SWS_Ocu_00024 |

| 错误码 | 名称 | 含义 | |
|--------|------|------|--------|
| 0x01 | OCU_E_PARAM_POINTER | 空指针 | |
| 0x02 | OCU_E_PARAM_CONFIG | 配置错误 | |
| 0x03 | OCU_E_UNINIT | 未初始化 | |
| 0x04 | OCU_E_ALREADY_INITIALIZED | 重复初始化 | |
| 0x05 | OCU_E_PARAM_CHANNEL | 通道非法 | |
| 0x06 | OCU_E_PARAM_INVALID_STATE | 状态非法 | |
| 0x07 | OCU_E_PARAM_ACTION | 动作非法 | |
| 0x08 | OCU_E_PARAM_PIN_STATE | 引脚状态非法 | |
| 0x09 | OCU_E_CHANNEL_BUSY | 通道忙 | |
| 0x0A | OCU_E_PARAM_REF_VALUE | 参考值非法 | |
| 0x0B | OCU_E_PARAM_THRESHOLD_VALUE | 阈值非法 | |
| 0x0C | OCU_E_INIT_FAILED | 初始化失败 | |
| 0x0D | OCU_E_NO_TICKS_PER_CHANNEL | 通道无时钟 | |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查是否重复初始化。
2. Pre-compile 模式下直接使用 `&Ocu_Config`，否则使用传入配置。
3. 调用 `Ocu_ValidateInitConfig` 校验配置指针。
4. 遍历所有通道，调用 `Ocu_ChannelInit`：
   - 初始化运行时状态（STOPPED、默认引脚状态、默认阈值、TOGGLE 动作）。
   - 调用 `Ocu_HwInitChannel` 复位硬件寄存器。
   - 设置初始引脚状态与比较值。
5. 设置模块状态为 `OCU_INITIALIZED`。

### 7.2 通道启动/停止流程

- **Start**：校验初始化与通道号；若未运行，调用 `Ocu_HwStartChannel` 置位 Control 寄存器 ENABLE 位，状态更新为 RUNNING。
- **Stop**：校验初始化与通道号；若运行中，调用 `Ocu_HwStopChannel` 清除 ENABLE 位，状态更新为 STOPPED。

### 7.3 阈值设置流程

- **Absolute**：校验参考值与绝对值均小于 `MaxCounterValue`，调用 `Ocu_HwSetCompareValue` 写入 Compare 寄存器。
- **Relative**：读取当前 Counter，计算 `newValue = current + RelativeValue`，超出最大值时回绕，再写入 Compare 寄存器。

### 7.4 比较匹配中断处理流程

1. ISR 读取 Status 寄存器。
2. 若 Match 标志置位，清除标志并调用 `Ocu_ProcessCompareMatch`：
   - 根据当前 `PinAction` 计算新引脚状态。
   - 若状态变化，调用 `Ocu_HwSetPinState` 更新硬件。
3. 若通知使能且回调非空，调用 `Notification`。
4. 若 Overflow 标志置位，清除标志（可用于 PWM 周期完成扩展逻辑）。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 | |
|----|--------|------|
| OCU_DEV_ERROR_DETECT | STD_ON | 开发错误检测 | |
| OCU_VERSION_INFO_API | STD_ON | 版本信息 API | |
| OCU_NUM_CHANNELS | 4U | 逻辑通道数 | |
| OCU_MAX_COUNTER_VALUE | 4294967295U | 32-bit 最大计数值 | |
| OCU_CONFIGURATION_VARIANT | OCU_VARIANT_PRE_COMPILE | 配置变体 | |
| OCU_DE_INIT_API | STD_ON | DeInit API | |
| OCU_SET_PIN_STATE_API | STD_ON | SetPinState API | |
| OCU_SET_PIN_ACTION_API | STD_ON | SetPinAction API | |
| OCU_SET_THRESHOLD_API | STD_ON | 阈值设置 API | |
| OCU_NOTIFICATION_SUPPORTED | STD_ON | 通知支持 | |
| OCU_CHANNEL_0_ENABLE ~ OCU_CHANNEL_3_ENABLE | STD_ON | 通道使能 | |
| OCU_CHANNEL_x_DEFAULT_PIN_STATE | OCU_LOW | 默认引脚状态 | |
| OCU_CHANNEL_x_DEFAULT_THRESHOLD | 65536U | 默认阈值 | |
| OCU_CHANNEL_x_NOTIFICATION | NULL_PTR | 通道通知回调 | |
| OCU_CHANNEL_x_BACKGROUND_MODE | STD_OFF | 后台运行模式 | |
| OCU_CHANNEL_x_BASE_ADDRESS | 0x40000000 区域 | 硬件寄存器基地址 | |
| OCU_REG_CTRL_OFFSET | 0U | Control 寄存器偏移 | |
| OCU_REG_STATUS_OFFSET | 4U | Status 寄存器偏移 | |
| OCU_REG_COUNTER_OFFSET | 8U | Counter 寄存器偏移 | |
| OCU_REG_COMPARE_OFFSET | 12U | Compare 寄存器偏移 | |
| OCU_REG_ACTION_OFFSET | 16U | Action 寄存器偏移 | |
| OCU_REG_PIN_CTRL_OFFSET | 20U | PinCtrl 寄存器偏移 | |
| OCU_CTRL_ENABLE_BIT | 1U | 通道使能位 | |
| OCU_CTRL_INTERRUPT_BIT | 2U | 中断使能位 | |
| OCU_STATUS_MATCH_BIT | 1U | 比较匹配标志 | |
| OCU_STATUS_OVERFLOW_BIT | 2U | 溢出标志 | |

### 8.2 链接时配置

`Ocu.c` 内部直接定义 `Ocu_ChannelConfig[]` 与 `Ocu_Config`，实现 Pre-compile + Link-time 配置。

### 8.3 构建后配置

当前未支持 Post-build 配置变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误

`OCU_DEV_ERROR_DETECT == STD_ON` 时，所有 API 校验初始化状态、通道号、参数范围，并通过 `OCU_REPORT_ERROR`（内部调用 `Det_ReportError`）上报。

### 9.2 DEM 错误

未定义 DEM 事件。

### 9.3 安全机制

- ASIL D 声明：头文件声明为 ASIL-D 兼容，但实际实现需通过安全分析验证。
- 通道忙时再次启动返回 `OCU_E_CHANNEL_BUSY`。
- 运行中禁止直接 `SetPinState`，避免与比较匹配动作冲突。
- 阈值校验防止写入超出 `MaxCounterValue` 的值。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 | |
|------|------|
| OCU_START_SEC_CONFIG_DATA_UNSPECIFIED | 配置数据段（`Ocu_Config`、`Ocu_ChannelConfig`） | |
| OCU_STOP_SEC_CONFIG_DATA_UNSPECIFIED | 配置数据段结束 | |
| OCU_START_SEC_VAR_INIT_UNSPECIFIED | 已初始化变量（`Ocu_ModuleState`） | |
| OCU_STOP_SEC_VAR_INIT_UNSPECIFIED | 已初始化变量结束 | |
| OCU_START_SEC_VAR_NO_INIT_UNSPECIFIED | 未初始化变量（`Ocu_ChannelState`、`Ocu_CurrentConfig`） | |
| OCU_STOP_SEC_VAR_NO_INIT_UNSPECIFIED | 未初始化变量结束 | |
| OCU_START_SEC_CODE | 代码段 | |
| OCU_STOP_SEC_CODE | 代码段结束 | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | ~200 bytes | 运行时状态 + 硬件寄存器抽象 | |
| ROM | ~8 KB | 代码 + 配置 | |
| 中断延迟 | 低 | 单次比较匹配处理 | |

---

## 11. 集成指南

- 与上层集成：Pwm 模块可通过 OCU 实现可变占空比输出；IoHwAb 可利用 `Ocu_SetPinState` / `SetPinAction` 实现复杂时序。
- 与下层集成：依赖 Gpt 或独立计数器提供时基；依赖 Port 配置输出引脚；依赖 Mcu 使能模块时钟。
- 初始化顺序：`Mcu_Init` -> `Gpt_Init`（如需要） -> `Port_Init` -> `Ocu_Init`。
- 中断绑定：将硬件比较匹配中断连接到 `Ocu_Channelx_IrqHandler`。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 | |
|----------|----------|
| test_ocu.c | Init/DeInit、Start/Stop、阈值设置、PinAction、通知、错误码 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 比较匹配翻转 | 验证 TOGGLE 动作与通知 | |
| 相对阈值 PWM | 验证连续相对阈值设置生成 PWM | |
| 溢出处理 | 验证 Counter 达到最大值后的行为 | |
| 运行中 SetPinState 保护 | 验证 `OCU_E_PARAM_INVALID_STATE` | |

---

## 13. 实现说明 / TODO

- **Module ID 偏差**：本设计文档按 AUTOSAR 标准使用 Module ID `0x17`，当前 `Ocu.h` 中 `OCU_MODULE_ID` 为 `0x7A`（十进制 122），建议统一。
- **硬件抽象占位**：`Ocu_Irq.c` 中 `Ocu_HwGetRegisterBase` 返回静态 `Ocu_HwRegisters[]` 指针，未映射到真实 MCU 寄存器地址，需根据目标平台替换。
- **HAL 与 IRQ 文件合并**：当前 `Ocu_Irq.c` 同时包含中断处理与全部 HAL 函数，建议按职责拆分为 `Ocu_Hw.c` / `Ocu_Irq.c`，提高可移植性。
- **状态命名区分**：`Ocu.h` 中 `Ocu_StateType` 仅包含 STOPPED/RUNNING，用于通道状态；模块状态 `OCU_UNINIT` / `OCU_INITIALIZED` 定义在 `Ocu_Private.h`，建议文档与命名保持一致。
- **SetRelativeThreshold 回绕公式**：当前使用 `newValue - MaxCounterValue - 1U`，建议与项目计数器溢出语义核对。
- **Background Mode**：配置中保留 `RunningInBackground` 字段，当前实现未使用。
- **MISRA 与 ASIL-D**：代码声明 MISRA C:2012 兼容与 ASIL-D，需进一步做静态分析与安全测试验证。

---

## 14. 参考资料

1. AUTOSAR_SWS_OCU.pdf
2. `docs/modules/OCU.md`
3. `src/bsw/mcal/ocu/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Ocu_00228 | `PWM_50Percent_Duty` | 测试 Test_PWM_50Percent_Duty 覆盖: PWM_50Percent_Duty 场景 |
| SWS_Ocu_00229 | `PWM_VariableDutyCycle` | 测试 Test_PWM_VariableDutyCycle 覆盖: PWM_VariableDutyCycle 场景 |
| SWS_Ocu_00230 | `PWM_MultipleChannels` | 测试 Test_PWM_MultipleChannels 覆盖: PWM_MultipleChannels 场景 |
| SWS_Ocu_00231 | `OutputCompare_SingleShot` | 测试 Test_OutputCompare_SingleShot 覆盖: OutputCompare_SingleShot 场景 |
| SWS_Ocu_00232 | `OutputCompare_TimedPulse` | 测试 Test_OutputCompare_TimedPulse 覆盖: OutputCompare_TimedPulse 场景 |
| SWS_Ocu_00233 | `Edge_MaxChannels` | 测试 Test_Edge_MaxChannels 覆盖: Edge_MaxChannels 场景 |
| SWS_Ocu_00234 | `Edge_MaxThreshold` | 测试 Test_Edge_MaxThreshold 覆盖: Edge_MaxThreshold 场景 |
| SWS_Ocu_00235 | `Edge_ZeroThreshold` | 测试 Test_Edge_ZeroThreshold 覆盖: Edge_ZeroThreshold 场景 |
| SWS_Ocu_00237 | `Sequence_MultipleOperations` | 测试 Test_Sequence_MultipleOperations 覆盖: Sequence_MultipleOperations 场景 |
