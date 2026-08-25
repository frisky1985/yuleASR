# Wdg Design Document

> **Module ID**: 0x0D  
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Wdg  
> **Source Path**: `src/bsw/mcal/Wdg/`  
> **Reference Document**: `docs/modules/Wdg.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

WDG Driver 是 MCAL 层看门狗驱动，用于监控程序执行流程并在超时未喂狗时触发复位。本实现支持：

- OFF / SLOW / FAST 三种模式。
- 窗口模式：在指定时间窗口内喂狗才合法。
- 超时前预警中断与回调。
- 窗口违规回调与可配置错误动作。
- 喂狗计数器与上次喂狗时间戳。

上层由 WdgIf / WdgM 调用；下层直接访问 SoC WDOG 外设寄存器。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS Wdg | 4.4.0 | Watchdog Driver 软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | WdgIf、WdgM、EcuM | 喂狗策略与监控 | |
| 下层 | SoC WDOG 外设 | i.MX8M Mini / S32K312 WDOG | |
| 同层 | Mcu、Port | 时钟与复位引脚 | |
| 公共 | Det | 开发错误检测（可选） | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│      WdgM / WdgIf / EcuM            │
├─────────────────────────────────────┤
│           Wdg (MCAL)                │
├─────────────────────────────────────┤
│      SoC WDOG Peripheral            │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **模式管理**：支持 OFF / SLOW / FAST 模式切换。
- **超时计算**：根据 `timeoutMs` 与 32 kHz WDOG 时钟计算 WCR[WT] 值。
- **窗口验证**：喂狗时检查当前时间与上次喂狗时间差是否在窗口内。
- **预警回调**：接近超时时调用 `PreWarningCallback`。
- **窗口违规回调**：违反窗口规则时调用 `WindowViolationCallback`。
- **喂狗序列**：向 WSR 依次写入 0x5555、0xAAAA。

### 3.3 文件结构

```
src/bsw/mcal/Wdg/
├── include/
│   ├── Wdg.h
│   ├── Wdg_Cfg.h
│   └── Wdg_Hw.h
└── src/
    ├── Wdg.c
    └── Wdg_Hw.c
```

---

## 4. 状态机

```
[UNINIT] -- Wdg_Init() --> [IDLE] (OFF mode) or [RUNNING]
[RUNNING] -- Wdg_SetMode(OFF) --> [IDLE]
[IDLE]    -- Wdg_SetMode(SLOW/FAST) --> [RUNNING]
[RUNNING] -- 窗口违规/错误 --> [ERROR] (可选)
[RUNNING] -- reset --> [UNINIT]
```

---

## 5. 核心数据结构

| 类型 | 说明 | |
|------|------|
| `WdgIf_ModeType` | `WDGIF_OFF_MODE` / `SLOW_MODE` / `FAST_MODE` | |
| `Wdg_StateType` | `UNINIT` / `IDLE` / `RUNNING` / `STOPPED` / `ERROR` | |
| `Wdg_TriggerResultType` | 喂狗结果：`OK` / `WINDOW_EARLY` / `WINDOW_LATE` 等 | |
| `Wdg_TimeoutType` | `uint16`，超时值（ms） | |
| `Wdg_ClockPrescalerType` | 看门狗时钟预分频枚举 | |
| `Wdg_ModeSettingsType` | 单模式配置：超时、预分频、窗口、中断、预警时间 | |
| `Wdg_ConfigType` | 全局配置：基址、快慢模式、初始模式、回调 | |
| `Wdg_PreWarningCallbackType` | 预警回调类型 | |
| `Wdg_WindowViolationCallbackType` | 窗口违规回调类型 | |

```c
typedef struct {
    Wdg_TimeoutType TimeoutPeriod;
    Wdg_ClockPrescalerType ClockPrescaler;
    boolean WindowModeEnabled;
    Wdg_TimeoutType WindowStart;
    Wdg_TimeoutType WindowEnd;
    boolean InterruptMode;
    uint16 TimeoutPreWarningUs;
} Wdg_ModeSettingsType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|----------------|
| `Wdg_Init` | `void Wdg_Init(const Wdg_ConfigType* ConfigPtr)` | 初始化看门狗 | | |
| `Wdg_SetMode` | `Std_ReturnType Wdg_SetMode(WdgIf_ModeType Mode)` | 设置工作模式 | | |
| `Wdg_Trigger` | `void Wdg_Trigger(void)` | 喂狗 | | |
| `Wdg_GetVersionInfo` | `void Wdg_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 获取版本信息 | | |
| `Wdg_SetTriggerCondition` | `Std_ReturnType Wdg_SetTriggerCondition(uint16 timeout)` | 设置触发条件超时 | | |
| `Wdg_GetStatus` | `Wdg_StateType Wdg_GetStatus(void)` | 获取模块状态 | | |
| `Wdg_GetTriggerCounter` | `uint32 Wdg_GetTriggerCounter(void)` | 获取喂狗次数 | | |
| `Wdg_GetLastTriggerTime` | `uint32 Wdg_GetLastTriggerTime(void)` | 获取上次喂狗时间戳 | | |

### 6.2 回调函数

| 回调 | 说明 | |
|------|------|
| `PreWarningCallback` | 超时前预警回调，参数为剩余时间（us） | |
| `WindowViolationCallback` | 窗口违规回调 | |

回调在 `Wdg_ConfigType` 中配置。

### 6.3 Service ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | `Wdg_Init` | `WDG_E_PARAM_CONFIG`、`WDG_E_ALREADY_INITIALIZED` | |
| 0x01 | `Wdg_SetMode` | `WDG_E_UNINIT`、`WDG_E_PARAM_MODE`、`WDG_E_DISABLE_NOT_ALLOWED`、`WDG_E_MODE_TRANSITION` | |
| 0x02 | `Wdg_Trigger` | `WDG_E_UNINIT`、`WDG_E_FORBIDDEN_INVOCATION`、`WDG_E_WINDOW_VIOLATION` | |
| 0x03 | `Wdg_GetVersionInfo` | `WDG_E_PARAM_POINTER` | |
| 0x04 | `Wdg_SetTriggerCondition` | `WDG_E_UNINIT`、`WDG_E_PARAM_TIMEOUT` | |

| 错误码 | 名称 | 说明 | |
|--------|------|------|
| 0x10 | `WDG_E_DRIVER_STATE` | 驱动状态错误 | |
| 0x11 | `WDG_E_PARAM_MODE` | 无效模式 | |
| 0x12 | `WDG_E_PARAM_POINTER` | 空指针 | |
| 0x13 | `WDG_E_PARAM_CONFIG` | 无效配置 | |
| 0x14 | `WDG_E_PARAM_TIMEOUT` | 超时参数越界 | |
| 0x15 | `WDG_E_DISABLE_NOT_ALLOWED` | 禁止关闭看门狗 | |
| 0x16 | `WDG_E_FORBIDDEN_INVOCATION` | OFF 模式下调用 Trigger | |
| 0x17 | `WDG_E_ALREADY_INITIALIZED` | 重复初始化 | |
| 0x18 | `WDG_E_UNINIT` | 未初始化 | |
| 0x19 | `WDG_E_WINDOW_VIOLATION` | 喂狗不在窗口内 | |
| 0x1A | `WDG_E_SEQUENCE_ERROR` | 服务序列错误 | |
| 0x1B | `WDG_E_MODE_TRANSITION` | 模式转换错误 | |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 `ConfigPtr` 非空、未初始化。
2. 保存配置指针。
3. 读取 WDOG 基址，禁用 WDE。
4. 根据初始模式选择 `FastModeSettings` 或 `SlowModeSettings`：
   - 计算 WT 值并写入 WCR。
   - 若中断模式使能，配置 WICR。
5. 若非 OFF 模式，置 WDE + WDT，状态为 RUNNING；否则为 IDLE。
6. 重置触发计数器与时间戳。

### 7.2 模式切换流程

1. 检查初始化状态。
2. 若 `WDG_DISABLE_ALLOWED == STD_OFF` 且目标为 OFF，报错。
3. 根据目标模式写 WCR 使能/禁用 WDE，更新 WT。
4. 更新当前模式与状态。

### 7.3 喂狗流程

1. 检查初始化、非 OFF 模式、RUNNING 状态。
2. 获取当前模式对应的 `Wdg_ModeSettingsType`。
3. 若窗口模式使能，验证 `timeSinceLastTrigger` 在 `[WindowStart, WindowEnd]` 内。
   -  Early/Late 调用窗口违规回调并 DET 报错。
   -  若配置 `WDG_WINDOW_ERROR_ACTION == WDG_WINDOW_ERROR_RESET`，可触发系统复位（当前占位）。
4. 若预警使能且接近超时，调用 `PreWarningCallback`。
5. 向 WSR 依次写入 0x5555、0xAAAA。
6. 更新触发计数器与时间戳。

### 7.4 触发条件设置流程

1. 检查初始化与非 OFF 模式。
2. 校验 timeout 在 `[WDG_MIN_TIMEOUT, WDG_MAX_TIMEOUT]`。
3. 重新计算 WT 并写入 WCR。
4. 调用 `Wdg_Trigger` 刷新计数器。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 | |
|----|--------|------|
| `WDG_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 | |
| `WDG_VERSION_INFO_API` | STD_ON | 版本信息 API | |
| `WDG_DISABLE_ALLOWED` | STD_OFF | 是否允许关闭看门狗 | |
| `WDG_INITIAL_MODE` | `WDGIF_FAST_MODE` | 初始模式 | |
| `WDG_TIMEOUT_PRE_WARNING` | STD_ON | 超时前预警使能 | |
| `WDG_VALIDATE_WINDOW_MODE` | STD_ON | 窗口模式验证 | |
| `WDG_WINDOW_ERROR_ACTION` | `WDG_WINDOW_ERROR_RESET` | 窗口违规动作 | |
| `WDG_DEFAULT_TIMEOUT` | 100U | 默认超时（ms） | |
| `WDG_FAST_MODE_TIMEOUT` | 50U | Fast 模式超时 | |
| `WDG_SLOW_MODE_TIMEOUT` | 500U | Slow 模式超时 | |
| `WDG_MIN_TIMEOUT` | 1U | 最小超时 | |
| `WDG_MAX_TIMEOUT` | 1000U | 最大超时 | |
| `WDG_FAST_MODE_PRESCALER` | `WDG_PRESCALER_64` | Fast 模式预分频 | |
| `WDG_SLOW_MODE_PRESCALER` | `WDG_PRESCALER_128` | Slow 模式预分频 | |
| `WDG_BASE_ADDRESS` | 807927808U | WDOG 基址 | |
| `WDG_CLOCK_FREQUENCY_HZ` | 32000U | WDOG 时钟 | |

### 8.2 窗口模式配置

| 宏 | 值 | 说明 | |
|----|----|------|
| `WDG_FAST_MODE_WINDOW_ENABLED` | STD_ON | Fast 窗口使能 | |
| `WDG_FAST_MODE_WINDOW_START` | 10U | Fast 窗口起始（ms） | |
| `WDG_FAST_MODE_WINDOW_END` | 40U | Fast 窗口结束（ms） | |
| `WDG_SLOW_MODE_WINDOW_ENABLED` | STD_ON | Slow 窗口使能 | |
| `WDG_SLOW_MODE_WINDOW_START` | 100U | Slow 窗口起始 | |
| `WDG_SLOW_MODE_WINDOW_END` | 400U | Slow 窗口结束 | |

### 8.3 链接时配置

当前 `Wdg.c` 直接接收 `Wdg_ConfigType*`，未使用独立 Lcfg 文件。`Wdg_Hw.h/c` 提供额外硬件抽象，但 `Wdg.c` 未调用 `Wdg_Hw` 接口。建议统一或拆分。

---

## 9. 错误处理与安全

### 9.1 DET 错误

在 `WDG_DEV_ERROR_DETECT == STD_ON` 时：

- `Wdg_Init` 空配置 -> `WDG_E_PARAM_CONFIG`
- 重复初始化 -> `WDG_E_ALREADY_INITIALIZED`
- 未初始化调用 -> `WDG_E_UNINIT`
- 无效模式 -> `WDG_E_PARAM_MODE`
- OFF 模式触发 -> `WDG_E_FORBIDDEN_INVOCATION`
- 窗口违规 -> `WDG_E_WINDOW_VIOLATION`
- timeout 越界 -> `WDG_E_PARAM_TIMEOUT`
- 禁用不允许 -> `WDG_E_DISABLE_NOT_ALLOWED`

### 9.2 DEM 错误

本模块未使用 DEM。

### 9.3 安全机制

- 看门狗在初始化后默认 RUNNING，防止系统死锁。
- 窗口模式防止错误地过早/过晚喂狗。
- `WDG_DISABLE_ALLOWED` 控制是否允许关闭看门狗。
- `Wdg_SetTriggerCondition` 动态调整超时并立即喂狗。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 | |
|------|------|
| `WDG_START_SEC_VAR_CLEARED_UNSPECIFIED` | `Wdg_DriverInitialized`、`Wdg_CurrentMode`、`Wdg_TriggerCounter` 等 | |
| `WDG_START_SEC_CONFIG_DATA_UNSPECIFIED` | `Wdg_Config` | |
| `WDG_START_SEC_CODE` | 代码段 | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | ~32 B | 运行态变量 | |
| ROM | 配置表 + 代码 | 较小 | |
| 堆栈 | 低 | 无递归 | |

---

## 11. 集成指南

- 与 WdgIf/WdgM 集成：上层按策略调用 `Wdg_SetTriggerCondition` 与 `Wdg_Trigger`。
- 与 Mcu 集成：WDOG 时钟由 Mcu 配置。
- 与 Port 集成：若使用外部看门狗，需配置对应 GPIO。
- 与 EcuM 集成：在启动早期初始化 Wdg，确保运行态受监控。
- 中断路由：若使能预警中断，需将 WDOG 中断向量连接到 `Wdg_Hw_IRQHandler`。
- 初始化顺序：Mcu -> Port -> Wdg（越早越好）。

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 覆盖内容 | |
|--------|----------|
| 初始化 | 空配置、重复初始化、各初始模式 | |
| 模式切换 | OFF/SLOW/FAST、禁用保护 | |
| 喂狗 | 正常喂狗、窗口 Early/Late | |
| 触发条件 | 超时边界、越界处理 | |
| DET | 各错误码路径 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 超时复位 | 不喂狗时系统是否复位 | |
| 窗口违规 | 过早/过晚喂狗是否触发回调/复位 | |
| 模式切换 | Fast/SLOW 切换后超时是否生效 | |
| 预警中断 | 接近超时前是否调用回调 | |

---

## 13. 实现说明 / TODO

- **Module ID 差异**：头文件中 `WDG_MODULE_ID` 定义为 `0x10`（十进制 16），与 AUTOSAR 标准 WDG Module ID `0x0D` 不一致。设计文档按项目约定使用 `0x0D`，实际代码需统一。
- **时间戳未实现**：`Wdg_GetCurrentTimeMs` 固定返回 0，导致窗口验证与预警回调无法按真实时间工作。
- **窗口违规复位占位**：`WDG_WINDOW_ERROR_ACTION == WDG_WINDOW_ERROR_RESET` 分支为空，需接入平台复位函数。
- **Wdg_Hw 未使用**：`Wdg_Hw.h/c` 提供 IWDG/WWDG 抽象，但 `Wdg.c` 直接访问 WDOG 寄存器，未调用 `Wdg_Hw` 接口，建议统一 HW 抽象层。
- **缺少 DeInit API**：`Wdg.h` 未声明 `Wdg_DeInit`。
- **SetMode 状态转换检查不完整**：从 RUNNING 到 OFF 的合法性检查较简单。
- **独立 Lcfg**：建议生成 `Wdg_Lcfg.c` 存放 `Wdg_Config` 实例。

---

## 14. 参考资料

1. AUTOSAR_SWS_Wdg.pdf
2. `docs/modules/Wdg.md`
3. `src/bsw/mcal/Wdg/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Wdg | — | WDG 模块级需求归属 |
| SWS_Wdg_00001 | `wdg_init_already_initialized` | 测试 wdg_init_already_initialized 覆盖: wdg_init_already_initialized 场景 |
| SWS_Wdg_00002 | `wdg_set_mode_off_allowed` | 测试 wdg_set_mode_off_allowed 覆盖: wdg_set_mode_off_allowed 场景 |
| SWS_Wdg_00003 | `wdg_trigger_not_initialized` | 测试 wdg_trigger_not_initialized 覆盖: wdg_trigger_not_initialized 场景 |
| SWS_Wdg_00004 | `wdg_get_version_info_null` | 测试 wdg_get_version_info_null 覆盖: wdg_get_version_info_null 场景 |
| SWS_Wdg_00005 | `wdg_set_trigger_condition_invalid` | 测试 wdg_set_trigger_condition_invalid 覆盖: wdg_set_trigger_condition_invalid 场景 |
| SWS_Wdg_00006 | `Wdg_GetStatus` | 测试 test_Wdg_GetStatus_AfterInit_ShouldReturnState 覆盖: Wdg_GetStatus_AfterInit_ShouldReturnState 场景 |
| SWS_Wdg_00007 | `Wdg_GetTriggerCounter` | 测试 test_Wdg_GetTriggerCounter_AfterInit_ShouldReturnZero 覆盖: Wdg_GetTriggerCounter_AfterInit_ShouldReturnZero 场景 |
| SWS_Wdg_00008 | `Wdg_GetLastTriggerTime` | 测试 test_Wdg_GetLastTriggerTime_AfterInit_ShouldReturnZero 覆盖: Wdg_GetLastTriggerTime_AfterInit_ShouldReturnZero 场景 |
| SWS_Wdg_00201 | `wdg_DeInit_should_cleanup_successfully` | 测试 test_wdg_DeInit_should_cleanup_successfully 覆盖: wdg_DeInit_should_cleanup_successfully 场景 |
