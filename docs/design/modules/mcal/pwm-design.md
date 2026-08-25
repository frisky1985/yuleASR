# Pwm Design Document

> **Module ID**: 0x11  
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Pwm  
> **Source Path**: `src/bsw/mcal/Pwm/`  
> **Reference Document**: `docs/modules/Pwm.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

PWM Driver 属于 MCAL 层，用于产生脉冲宽度调制信号，支持：

- 多通道 PWM 输出（最多 8 通道）。
- 固定周期、可变周期与相位偏移通道类。
- 占空比与周期动态修改。
- 输出置 idle、通知回调与 power state 接口占位。

上层可由 ICU、OCU、电机控制等模块调用；下层直接访问 SoC PWM 外设寄存器（i.MX8M Mini PWM 或 S32K312 FTM）。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS Pwm | 4.4.0 | PWM Driver 软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | ICU、电机控制、ECUAL | PWM 信号消费方 | |
| 下层 | SoC PWM / FTM 外设 | i.MX8M Mini / S32K312 | |
| 同层 | Mcu、Port | 时钟与引脚配置 | |
| 公共 | Det | 开发错误检测（可选） | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│     RTE / Application / ECUAL       │
├─────────────────────────────────────┤
│           Pwm (MCAL)                │
├─────────────────────────────────────┤
│      SoC PWM / FTM Peripheral       │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **通道配置管理**：按 `Pwm_ChannelConfigType` 初始化各通道。
- **硬件寄存器访问**：i.MX8M Mini PWM 寄存器（CR、SR、IR、SAR、PR、CNR）。
- **占空比/周期计算**：将 0~32768 占空比映射为 SAR 采样值。
- **通知回调**：`NotificationFn` 由通道配置提供，当前代码未在 ISR 中调用。
- **Power State API**：接口已定义，但 `PWM_POWER_STATE_SUPPORTED` 为 STD_OFF。

### 3.3 文件结构

```
src/bsw/mcal/Pwm/
├── include/
│   ├── Pwm.h
│   └── Pwm_Cfg.h
└── src/
    ├── Pwm.c
    └── Pwm_Lcfg.c
```

---

## 4. 状态机

模块级状态由布尔变量 `Pwm_DriverInitialized` 维护：

```
[UNINIT] -- Pwm_Init() --> [INITIALIZED]
[INITIALIZED] -- Pwm_DeInit() --> [UNINIT]
```

各通道无显式状态机，运行结果通过占空比缓存 `Pwm_ChannelDutyCycle` 与硬件寄存器反映。

---

## 5. 核心数据结构

| 类型 | 说明 | |
|------|------|
| `Pwm_ChannelType` | `uint8`，通道标识 | |
| `Pwm_PeriodType` | `uint32`，周期 tick 数 | |
| `Pwm_DutyCycleType` | `uint16`，占空比（0~32768） | |
| `Pwm_OutputStateType` | `PWM_LOW` / `PWM_HIGH` | |
| `Pwm_EdgeNotificationType` | `PWM_RISING_EDGE`、`PWM_FALLING_EDGE`、`PWM_BOTH_EDGES` | |
| `Pwm_ChannelClassType` | `PWM_VARIABLE_PERIOD`、`PWM_FIXED_PERIOD`、`PWM_FIXED_PERIOD_SHIFTED` | |
| `Pwm_IdleStateType` | `PWM_IDLE_LOW` / `PWM_IDLE_HIGH` | |
| `Pwm_PolarityType` | `PWM_POLARITY_LOW` / `PWM_POLARITY_HIGH` | |
| `Pwm_PowerStateType` | 请求结果枚举（当前命名与标准相反） | |
| `Pwm_PowerStateRequestResultType` | 状态枚举（当前命名与标准相反） | |
| `Pwm_ChannelConfigType` | 单通道完整配置 | |
| `Pwm_ConfigType` | 全局配置：通道数组、数量、API 开关 | |

```c
typedef struct {
    Pwm_ChannelType ChannelId;
    uint32 BaseAddress;
    Pwm_ChannelClassType ChannelClass;
    Pwm_PeriodType DefaultPeriod;
    Pwm_DutyCycleType DefaultDutyCycle;
    Pwm_IdleStateType IdleState;
    Pwm_PolarityType Polarity;
    Pwm_ClockSourceType ClockSource;
    uint32 ClockPrescaler;
    boolean NotificationSupported;
    void (*NotificationFn)(void);
} Pwm_ChannelConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|----------------|
| `Pwm_Init` | `void Pwm_Init(const Pwm_ConfigType* ConfigPtr)` | 初始化 PWM 驱动 | 必须先调用 | |
| `Pwm_DeInit` | `void Pwm_DeInit(void)` | 反初始化 | 受 `PWM_DE_INIT_API` 控制 | |
| `Pwm_SetDutyCycle` | `void Pwm_SetDutyCycle(Pwm_ChannelType Channel, uint16 DutyCycle)` | 设置占空比 | 无单位，0~32768 | |
| `Pwm_SetPeriodAndDuty` | `void Pwm_SetPeriodAndDuty(Pwm_ChannelType Channel, Pwm_PeriodType Period, uint16 DutyCycle)` | 设置周期与占空比 | 固定周期通道报错 | |
| `Pwm_SetOutputToIdle` | `void Pwm_SetOutputToIdle(Pwm_ChannelType Channel)` | 输出置 idle（SAR=0） | 受 `PWM_SET_OUTPUT_TO_IDLE_API` 控制 | |
| `Pwm_GetOutputState` | `Pwm_OutputStateType Pwm_GetOutputState(Pwm_ChannelType Channel)` | 读当前输出状态 | 受 `PWM_GET_OUTPUT_STATE_API` 控制 | |
| `Pwm_DisableNotification` | `void Pwm_DisableNotification(Pwm_ChannelType Channel)` | 禁用通知中断 | 受 `PWM_NOTIFICATION_SUPPORTED` 控制 | |
| `Pwm_EnableNotification` | `void Pwm_EnableNotification(Pwm_ChannelType Channel, Pwm_EdgeNotificationType Notification)` | 使能边沿通知 | 受 `PWM_NOTIFICATION_SUPPORTED` 控制 | |
| `Pwm_GetVersionInfo` | `void Pwm_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 获取版本信息 | 受 `PWM_VERSION_INFO_API` 控制 | |
| `Pwm_SetPowerState` 等 | ... | Power state 占位 | 受 `PWM_POWER_STATE_SUPPORTED` 控制（当前 OFF） | |

### 6.2 回调函数

| 回调 | 说明 | |
|------|------|
| `NotificationFn` | 每个通道配置中可注册一个无参回调，当前实现未在 ISR 中触发 | |

### 6.3 Service ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | `Pwm_Init` | `PWM_E_PARAM_CONFIG`、`PWM_E_ALREADY_INITIALIZED` | |
| 0x01 | `Pwm_DeInit` | `PWM_E_UNINIT` | |
| 0x02 | `Pwm_SetDutyCycle` | `PWM_E_UNINIT`、`PWM_E_PARAM_CHANNEL` | |
| 0x03 | `Pwm_SetPeriodAndDuty` | `PWM_E_UNINIT`、`PWM_E_PARAM_CHANNEL`、`PWM_E_PERIOD_UNCHANGEABLE` | |
| 0x04 | `Pwm_SetOutputToIdle` | `PWM_E_UNINIT`、`PWM_E_PARAM_CHANNEL` | |
| 0x05 | `Pwm_GetOutputState` | `PWM_E_UNINIT`、`PWM_E_PARAM_CHANNEL` | |
| 0x06 | `Pwm_DisableNotification` | `PWM_E_UNINIT`、`PWM_E_PARAM_CHANNEL` | |
| 0x07 | `Pwm_EnableNotification` | `PWM_E_UNINIT`、`PWM_E_PARAM_CHANNEL` | |
| 0x08 | `Pwm_GetVersionInfo` | `PWM_E_PARAM_POINTER` | |

| 错误码 | 名称 | 说明 | |
|--------|------|------|
| 0x0A | `PWM_E_PARAM_CONFIG` | 配置指针为空 | |
| 0x0B | `PWM_E_UNINIT` | 模块未初始化 | |
| 0x0C | `PWM_E_PARAM_CHANNEL` | 通道号越界 | |
| 0x0D | `PWM_E_PERIOD_UNCHANGEABLE` | 固定周期通道禁止修改周期 | |
| 0x0E | `PWM_E_ALREADY_INITIALIZED` | 重复初始化 | |
| 0x0F | `PWM_E_PARAM_POINTER` | 空指针参数 | |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 `ConfigPtr` 与初始化状态。
2. 保存配置指针。
3. 对每个通道：
   - 通过 `Pwm_GetBaseAddr` 获取基址（通道 0~3 分别映射 PWM1~PWM4）。
   - 软件复位 CR 寄存器并等待复位完成（带超时计数）。
   - 写 `PR` 周期寄存器。
   - 计算 SAR = `DefaultDutyCycle * DefaultPeriod / 32768` 并写入。
   - 组合 CR：prescaler + EN，启动通道。
4. 置初始化标志。

### 7.2 占空比更新流程

1. 检查初始化与通道有效性。
2. 读当前 `PR` 周期。
3. 计算 SAR = `DutyCycle * Period / 32768`。
4. 写 SAR 并缓存占空比值。

### 7.3 周期+占空比更新流程

1. 检查通道为可变周期类。
2. 写 PR，再按新周期计算 SAR。

### 7.4 Idle 输出流程

直接写 SAR = 0，PWM 输出低电平。

### 7.5 通知使能流程

写 IR 寄存器：Rising -> `PWM_IR_FIE`，Falling -> `PWM_IR_CIE`，Both -> 两者。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 | |
|----|--------|------|
| `PWM_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 | |
| `PWM_VERSION_INFO_API` | STD_ON | 版本信息 API | |
| `PWM_DE_INIT_API` | STD_ON | 反初始化 API | |
| `PWM_SET_DUTY_CYCLE_API` | STD_ON | 占空比设置 API | |
| `PWM_SET_PERIOD_AND_DUTY_API` | STD_ON | 周期+占空比设置 API | |
| `PWM_SET_OUTPUT_TO_IDLE_API` | STD_ON | Idle 输出 API | |
| `PWM_GET_OUTPUT_STATE_API` | STD_ON | 输出状态读取 API | |
| `PWM_NOTIFICATION_SUPPORTED` | STD_ON | 通知 API | |
| `PWM_POWER_STATE_SUPPORTED` | STD_OFF | Power State API（当前未实现） | |
| `PWM_NUM_CHANNELS` | 8U | 通道数量 | |
| `PWM_DEFAULT_PERIOD` | 1000U | 默认周期 | |
| `PWM_DEFAULT_DUTY_CYCLE` | 16384U | 默认占空比（50%） | |
| `PWM_DUTY_CYCLE_RESOLUTION` | 32768U | 占空比分辨率 | |
| `PWM_CLOCK_FREQUENCY_HZ` | 24000000U | PWM 时钟频率 | |

### 8.2 链接时配置

`Pwm_Lcfg.c` 提供 `const Pwm_ConfigType Pwm_Config`，当前为占位结构 `{ 0U }`，需配置工具生成完整通道表。

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 在 `PWM_DEV_ERROR_DETECT == STD_ON` 时调用 `Det_ReportError`。

### 9.2 DEM 错误

本模块未使用 DEM。

### 9.3 安全机制

- 固定周期通道禁止通过 `Pwm_SetPeriodAndDuty` 修改周期。
- 初始化时软件复位带超时保护。
- 通知中断通过 IR 寄存器独立开关。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 | |
|------|------|
| `PWM_START_SEC_VAR_CLEARED_UNSPECIFIED` | `Pwm_DriverInitialized`、`Pwm_ConfigPtr`、`Pwm_ChannelDutyCycle` | |
| `PWM_START_SEC_CONFIG_DATA_UNSPECIFIED` | `Pwm_Config` | |
| `PWM_START_SEC_CODE` | 代码段 | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | ~20 B + 通道占空比数组 | 8 通道约 36 B | |
| ROM | 配置表 + 代码 | 与通道数量成正比 | |
| 堆栈 | 中等 | 初始化循环 | |

---

## 11. 集成指南

- 与 Mcu 集成：依赖 Mcu 使能 PWM/FTM 时钟。
- 与 Port 集成：PWM 输出引脚需由 Port 配置为对应复用模式。
- 初始化顺序：Mcu -> Port -> Pwm。
- 通知回调：通道配置中注册回调后，需确保对应 PWM 中断已路由到 ISR。
- 配置工具：`Pwm_Lcfg.c` 与 `Pwm_Cfg.h` 应由 yuleASR Configurator 生成。

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 覆盖内容 | |
|--------|----------|
| 初始化 | 空配置、重复初始化、正常初始化 | |
| 占空比/周期 | 可变与固定周期通道行为 | |
| Idle 输出 | SAR 是否为 0 | |
| 通知 | IR 寄存器设置 | |
| DET | 各错误码路径 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| PWM 波形输出 | 示波器验证频率与占空比 | |
| 固定周期保护 | 禁止修改周期 | |
| 通知中断 | 触发回调 | |

---

## 13. 实现说明 / TODO

- **Module ID 差异**：头文件中 `PWM_MODULE_ID` 定义为 `0x7B`（十进制 123），与 AUTOSAR 标准 PWM Module ID `0x11` 不一致。设计文档按项目约定使用 `0x11`，实际代码需统一。
- **类型命名倒置**：`Pwm_PowerStateType` 与 `Pwm_PowerStateRequestResultType` 的枚举内容互换，使用时容易混淆，建议重命名。
- **通知回调未触发**：`NotificationFn` 只在配置结构中保存，当前实现未在 ISR 中调用。
- **Power State 未实现**：`PWM_POWER_STATE_SUPPORTED == STD_OFF`，接口仅返回 `PWM_SERVICE_ACCEPTED`。
- **Lcfg 占位**：`Pwm_Lcfg.c` 中 `Pwm_Config` 为 `{ 0U }`，需配置工具生成。
- **S32K312 适配**：编译时通过 `S32K312` 宏切换 PWM 基址到 FTM。

---

## 14. 参考资料

1. AUTOSAR_SWS_Pwm.pdf
2. `docs/modules/Pwm.md`
3. `src/bsw/mcal/Pwm/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Pwm | — | PWM 模块级需求归属 |
| SWS_Pwm_00001 | `pwm_init_null` | 测试 pwm_init_null 覆盖: pwm_init_null 场景 |
| SWS_Pwm_00002 | `pwm_deinit` | 测试 pwm_deinit 覆盖: pwm_deinit 场景 |
| SWS_Pwm_00003 | `pwm_set_duty_cycle` | 测试 pwm_set_duty_cycle 覆盖: pwm_set_duty_cycle 场景 |
| SWS_Pwm_00004 | `pwm_set_period_and_duty_fixed` | 测试 pwm_set_period_and_duty_fixed 覆盖: pwm_set_period_and_duty_fixed 场景 |
| SWS_Pwm_00005 | `pwm_set_output_to_idle` | 测试 pwm_set_output_to_idle 覆盖: pwm_set_output_to_idle 场景 |
| SWS_Pwm_00006 | `pwm_get_output_state` | 测试 pwm_get_output_state 覆盖: pwm_get_output_state 场景 |
| SWS_Pwm_00007 | `pwm_disable_notification` | 测试 pwm_disable_notification 覆盖: pwm_disable_notification 场景 |
| SWS_Pwm_00008 | `pwm_enable_notification` | 测试 pwm_enable_notification 覆盖: pwm_enable_notification 场景 |
| SWS_Pwm_00009 | `pwm_get_version_info` | 测试 pwm_get_version_info 覆盖: pwm_get_version_info 场景 |
| SWS_Pwm_00201 | `init` | 测试 test_init 覆盖: init 场景 |
| SWS_Pwm_00202 | `deinit` | 测试 test_deinit 覆盖: deinit 场景 |
| SWS_Pwm_00203 | `set_duty_cycle` | 测试 test_set_duty_cycle 覆盖: set_duty_cycle 场景 |
| SWS_Pwm_00204 | `set_period_and_duty` | 测试 test_set_period_and_duty 覆盖: set_period_and_duty 场景 |
| SWS_Pwm_00205 | `set_output_to_idle` | 测试 test_set_output_to_idle 覆盖: set_output_to_idle 场景 |
| SWS_Pwm_00206 | `get_output_state` | 测试 test_get_output_state 覆盖: get_output_state 场景 |
| SWS_Pwm_00207 | `notification` | 测试 test_notification 覆盖: notification 场景 |
| SWS_Pwm_00208 | `version_info` | 测试 test_version_info 覆盖: version_info 场景 |
| SWS_Pwm_00209 | `power_state` | 测试 test_power_state 覆盖: power_state 场景 |
| SWS_Pwm_00210 | `multi_channel` | 测试 test_multi_channel 覆盖: multi_channel 场景 |
| SWS_Pwm_00211 | `boundary_conditions` | 测试 test_boundary_conditions 覆盖: boundary_conditions 场景 |
