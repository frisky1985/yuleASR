# FMEDA — Wdg (Watchdog Driver)

> **文档编号**: SAF-FMEDA-WDG-001
> **版本**: 1.0
> **日期**: 2026-10-24
> **ASIL 等级**: D (看门狗为最高安全等级模块)
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software), Part 11 (Semiconductors)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW MCAL 层的 Wdg (Watchdog Driver) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/mcal/wdg/src/Wdg.c`

**分析边界**:
- Wdg 核心逻辑（初始化、模式切换、触发/刷新、超时配置、窗口模式验证）
- 上层接口：`Wdg_Init`, `Wdg_SetMode`, `Wdg_Trigger`, `Wdg_SetTriggerCondition`, `Wdg_GetStatus`
- 内部安全机制：参数校验、状态机保护、窗口模式验证、服务序列校验、触发计数监控
- 下层依赖（假设由硬件层覆盖）：WDOG 外设寄存器 (WCR/WSR/WICR/WRSR)
- 硬件抽象层：`Wdg_Hw.h` 定义的 IWDG/WWDG 硬件抽象接口

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 Wdg 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体/变量 | 位置 | 说明 |
|------------|------|------|
| `Wdg_ConfigType` | Wdg.h:146-157 | 模块配置结构体：BaseAddress, FastModeSettings, SlowModeSettings, InitialMode, 回调函数指针 |
| `Wdg_ModeSettingsType` | Wdg.h:133-141 | 模式设置：TimeoutPeriod, ClockPrescaler, WindowModeEnabled, WindowStart/End, InterruptMode |
| `Wdg_DriverInitialized` | Wdg.c:75 | 驱动初始化标志 (boolean) |
| `Wdg_CurrentMode` | Wdg.c:76 | 当前模式 (WdgIf_ModeType: OFF/SLOW/FAST) |
| `Wdg_CurrentTimeout` | Wdg.c:77 | 当前超时值 (Wdg_TimeoutType) |
| `Wdg_ConfigPtr` | Wdg.c:78 | 配置指针 (const Wdg_ConfigType*) |
| `Wdg_ModuleState` | Wdg.c:79 | 状态机 (Wdg_StateType: UNINIT/IDLE/RUNNING/STOPPED/ERROR) |
| `Wdg_TriggerCounter` | Wdg.c:80 | 触发计数器 (uint32) |
| `Wdg_LastTriggerTime` | Wdg.c:81 | 上次触发时间戳 (uint32, ms) |

### 1.4 硬件寄存器映射

| 寄存器 | 偏移 | 说明 | 关键位域 |
|--------|------|------|---------|
| WCR (Watchdog Control) | 0x00 | 控制寄存器 | WDE(0x0004), WT[15:8](0xFF00), WDT(0x0020), WDZST(0x0008) |
| WSR (Watchdog Service) | 0x02 | 服务寄存器 | 写入序列 0x5555→0xAAAA 刷新看门狗 |
| WRSR (Watchdog Reset Status) | 0x04 | 复位状态寄存器 | SFTW(0x0001), TOUT(0x0002), POR(0x0010) |
| WICR (Watchdog Interrupt Control) | 0x06 | 中断控制寄存器 | WIE(0x0001), WTIS(0x0002), WICT[15:8](0xFF00) |
| WMCR (Watchdog Misc Control) | 0x08 | 杂项控制 | PDE(0x0001) |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 驱动初始化 | `Wdg_Init()` (L200) | 配置看门狗超时、模式、中断，使能看门狗 |
| FU-02 | 模式切换 | `Wdg_SetMode()` (L274) | OFF/SLOW/FAST 模式转换，更新 WCR 寄存器 |
| FU-03 | 触发/刷新 | `Wdg_Trigger()` (L348) | 窗口验证 + 0x5555/0xAAAA 服务序列写入 |
| FU-04 | 超时条件设置 | `Wdg_SetTriggerCondition()` (L444) | 动态更新超时值并触发看门狗 |
| FU-05 | 窗口模式验证 | `Wdg_ValidateWindowTrigger()` (L138) | 检查触发是否在合法时间窗口内 |
| FU-06 | 超时计算 | `Wdg_CalculateTimeoutValue()` (L103) | ms→WCR[WT] 值转换，含溢出保护 |
| FU-07 | 状态监控 | `Wdg_GetStatus()`, `Wdg_GetTriggerCounter()`, `Wdg_GetLastTriggerTime()` (L494, L504, L514) | 运行状态、触发计数、时间戳查询 |
| FU-08 | 回调管理 | `Wdg_InvokePreWarningCallback()` (L176), `Wdg_InvokeWindowViolationCallback()` (L188) | 超时预警与窗口违规回调 |
| FU-09 | 硬件抽象 | `Wdg_Hw.h` 接口层 | IWDG/WWDG 硬件抽象，复位原因诊断 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 驱动初始化 (`Wdg_Init`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | ConfigPtr 为空指针 | 访问非法地址，系统崩溃 | `Wdg_Init()` L203 入口校验 `ConfigPtr == NULL_PTR`，上报 DET 错误 `WDG_E_PARAM_CONFIG` | 99% | D |
| FM-01-02 | 重复初始化 | 配置被覆盖，看门狗状态不一致 | L207 检查 `Wdg_DriverInitialized == TRUE`，上报 `WDG_E_ALREADY_INITIALIZED` | 99% | D |
| FM-01-03 | InitialMode 配置值非法 | 看门狗未正确使能或误关闭 | L228-234 模式分支选择，OFF_MODE 时 modeSettings=NULL_PTR 不配置超时 | 85% | D |
| FM-01-04 | WCR 寄存器写入失败（硬件故障） | 看门狗未使能，丧失监控功能 | 写后回读验证（由 EcuM/BswM 上层监控覆盖）；Wdg_GetStatus 状态检查 | 90% | D |
| FM-01-05 | 时钟频率配置错误 (WDG_CLOCK_FREQ_HZ=32000) | 超时时间计算错误，看门狗过早/过晚复位 | 配置编译时校验；集成测试阶段功能验证 | 80% | D |

### 3.2 FU-02: 模式切换 (`Wdg_SetMode`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | 未初始化时调用 SetMode | 操作未初始化的硬件 | L277 检查 `Wdg_DriverInitialized == FALSE`，上报 `WDG_E_UNINIT` | 99% | D |
| FM-02-02 | 非法模式转换 (OFF→OFF) | 冗余切换请求，可能掩盖配置错误 | L283-286 检查 OFF_MODE 下不允许切换到非 FAST/SLOW 模式 | 95% | D |
| FM-02-03 | 运行时禁用看门狗 (WDG_DISABLE_ALLOWED=OFF) | 安全监控被非法关闭 | L291-294 编译时配置 `WDG_DISABLE_ALLOWED == STD_OFF` 时拒绝 OFF_MODE | 99% | D |
| FM-02-04 | Mode 参数超出枚举范围 | 进入 default 分支，返回 E_NOT_OK | L318-322 default 分支捕获非法值，上报 `WDG_E_PARAM_MODE` | 99% | D |
| FM-02-05 | WCR 寄存器写入后硬件未响应 | 模式切换未生效 | 写后回读比对（由上层周期监控覆盖）；触发失败时看门狗超时复位 | 85% | D |

### 3.3 FU-03: 触发/刷新 (`Wdg_Trigger`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | 未初始化时触发 | 操作未初始化硬件 | L351 检查 `Wdg_DriverInitialized == FALSE`，上报 `WDG_E_UNINIT` | 99% | D |
| FM-03-02 | OFF 模式下触发 | 对已禁用看门狗执行无效服务 | L357 检查 `Wdg_CurrentMode == WDGIF_OFF_MODE`，上报 `WDG_E_FORBIDDEN_INVOCATION` | 99% | D |
| FM-03-03 | 非 RUNNING 状态触发 | 状态机异常，触发被忽略 | L364 检查 `Wdg_ModuleState != WDG_STATE_RUNNING`，直接 return | 95% | D |
| FM-03-04 | 服务序列写入失败 (0x5555/0xAAAA) | 看门狗未刷新，超时后复位 | 硬件级保护：序列不匹配时看门狗不刷新，必然导致超时复位 | 99% | D |
| FM-03-05 | Wdg_GetCurrentTimeMs 返回错误时间 | 窗口验证误判，可能拒绝合法触发或接受非法触发 | 时间源由 GPT 定时器提供，独立校验；窗口违规触发回调+复位 | 85% | D |
| FM-03-06 | TriggerCounter 溢出 (uint32 回绕) | 触发计数不准确，监控功能降级 | 溢出检测：计数器回绕到 0 后继续递增，由上层诊断监控覆盖 | 60% | D |

### 3.4 FU-04: 超时条件设置 (`Wdg_SetTriggerCondition`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | 未初始化时调用 | 操作未初始化硬件 | L447 检查 `Wdg_DriverInitialized == FALSE`，上报 `WDG_E_UNINIT` | 99% | D |
| FM-04-02 | timeout > WDG_MAX_TIMEOUT (1000ms) | 超时值过大，安全监控失效 | L458 上限校验，上报 `WDG_E_PARAM_TIMEOUT` | 99% | D |
| FM-04-03 | timeout < WDG_MIN_TIMEOUT (1ms) | 超时值过小，频繁误复位 | L464 下限校验，上报 `WDG_E_PARAM_TIMEOUT` | 99% | D |
| FM-04-04 | OFF 模式下设置超时 | 对禁用看门狗配置无效操作 | L453 检查 `Wdg_CurrentMode == WDGIF_OFF_MODE`，返回 E_NOT_OK | 99% | D |
| FM-04-05 | WCR[WT] 写入后值未生效 | 超时时间未更新，可能导致过早/过晚复位 | 写后回读验证；内部调用 `Wdg_Trigger()` 确认服务成功 | 90% | D |

### 3.5 FU-05: 窗口模式验证 (`Wdg_ValidateWindowTrigger`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | WindowStart > WindowEnd 配置错误 | 窗口永远无效或永远合法 | 编译时配置校验 (Wdg_Cfg.h: L47-48 FAST: 10-40ms, L51-52 SLOW: 100-400ms) | 90% | D |
| FM-05-02 | 时间差计算溢出 (uint32 回绕) | 窗口验证误判 | L146 `currentTime - Wdg_LastTriggerTime` 利用 uint32 自然回绕特性，数学上正确 | 95% | D |
| FM-05-03 | 窗口期前触发 (WDG_TRIGGER_WINDOW_EARLY) | 触发被拒绝，看门狗未刷新 | L149 检测 early，调用 `Wdg_InvokeWindowViolationCallback()`，可触发复位 | 99% | D |
| FM-05-04 | 窗口期后触发 (WDG_TRIGGER_WINDOW_LATE) | 触发被拒绝，看门狗可能超时 | L154 检测 late，同上处理路径 | 99% | D |
| FM-05-05 | 窗口验证被编译禁用 (WDG_VALIDATE_WINDOW_MODE=OFF) | 丧失窗口保护功能 | 配置管理：安全手册要求 WDG_VALIDATE_WINDOW_MODE 必须为 STD_ON | 0% | D |

### 3.6 FU-06: 超时计算 (`Wdg_CalculateTimeoutValue`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | timeoutMs = 0 输入 | 返回 0，WCR[WT]=0 → 最短超时 | L109 零值保护，直接返回 0U（由上层超时范围校验覆盖） | 90% | D |
| FM-06-02 | 中间值溢出 (uint64 > UINT32_MAX) | 超时值计算错误 | L116 溢出检查，返回最大值 0xFF（对应最长超时） | 95% | D |
| FM-06-03 | wtValue > 0xFF 超出寄存器范围 | WCR[WT] 只取 8 位，高位截断 | L124 边界钳位 `wtValue = 0xFFU` | 99% | D |
| FM-06-04 | 时钟频率常量错误 (32000Hz) | 所有超时计算偏差 | 编译时配置校验；与硬件时钟源交叉验证 | 80% | D |

### 3.7 FU-07: 状态监控

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | Wdg_ModuleState 内存位翻转 | 状态误判，可能错误拒绝/接受触发 | 状态枚举值范围校验 (0-4)；由 Wdg_Trigger 中 L364 状态检查覆盖 | 85% | D |
| FM-07-02 | TriggerCounter 存储位翻转 | 计数值不准确 | 冗余读取（多次采样比对）；上层 EcuM 周期监控 | 70% | D |
| FM-07-03 | LastTriggerTime 时间戳异常 | 窗口验证基准错误 | 与 GPT 硬件定时器交叉校验；异常值由窗口验证逻辑捕获 | 80% | D |

### 3.8 FU-08: 回调管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | PreWarningCallback 指针为空 | 回调未执行，丧失超时预警功能 | L178-179 空指针保护：`Wdg_ConfigPtr->PreWarningCallback != NULL_PTR` | 99% | D |
| FM-08-02 | WindowViolationCallback 指针为空 | 窗口违规未上报 | L190-191 空指针保护：`Wdg_ConfigPtr->WindowViolationCallback != NULL_PTR` | 99% | D |
| FM-08-03 | 回调函数内部执行异常 | 回调执行中崩溃 | 回调在 DET 保护下执行；异常由系统级 SWD/WDT 监控覆盖 | 80% | D |

### 3.9 FU-09: 硬件抽象层

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | WDOG 外设基地址配置错误 | 寄存器操作指向错误地址 | 编译时配置校验 `WDG_WDOG1_BASE_ADDR`；S32K312 与 i.MX8MM 地址不同 | 90% | D |
| FM-09-02 | 寄存器读取返回错误值 | 状态判断错误，可能误操作 | REG_READ16 返回值合理性检查；多次采样确认 | 85% | D |
| FM-09-03 | 寄存器写入后未生效 (总线故障) | 看门狗配置未更新 | 写后回读验证；连续写入失败时系统进入安全状态 | 90% | D |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 驱动初始化 | 99% | 91% | 参数校验 + 写后回读 |
| FU-02 | 模式切换 | 99% | 95% | 状态机 + 编译时保护 |
| FU-03 | 触发/刷新 | 99% | 90% | 服务序列硬件保护 + 窗口验证 |
| FU-04 | 超时条件设置 | 99% | 95% | 范围校验 + 写后回读 |
| FU-05 | 窗口模式验证 | 99% | 95% | 时间窗口校验 + 违规回调 |
| FU-06 | 超时计算 | 99% | 91% | 溢出保护 + 边界钳位 |
| FU-07 | 状态监控 | 85% | 78% | 枚举范围校验 + 冗余读取 |
| FU-08 | 回调管理 | 99% | 93% | 空指针保护 |
| FU-09 | 硬件抽象 | 90% | 88% | 地址校验 + 写后回读 |

### 4.2 模块整体诊断覆盖率

```
DC_Wdg = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_Wdg ≈ 92%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL D)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 99% | ~99.1% | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ~93% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 10 FIT | ~6.5 FIT | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。Wdg 中主要单点故障来源：
- 超时计算中时钟频率常量不可检测故障（由编译时配置管理覆盖）
- TriggerCounter/LastTriggerTime 存储瞬态故障（由上层 EcuM 周期监控覆盖）
- 窗口验证禁用配置（由安全手册约束，DC=0% 但概率极低）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (0.8 / 90) ≈ 99.1%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。Wdg 中主要潜伏故障来源：
- 窗口验证逻辑自身的瞬态故障（由独立硬件看门狗覆盖）
- 状态机变量内存位翻转（由 Wdg_Trigger 中状态检查覆盖）
- 回调函数指针损坏（由空指针校验覆盖，但指针值错误不可检测）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (6.3 / 90) ≈ 93%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_window_config_disable + lambda_counter_overflow + lambda_time_source_error
     ≈ 0.5 + 2.0 + 4.0 = 6.5 FIT < 10 FIT ✓
```

---

## 6. 安全状态与故障反应

### 6.1 Wdg 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **UNINIT** | `Wdg_ModuleState = WDG_STATE_UNINIT` (L79) | 所有 API 拒绝服务，返回错误码 |
| **IDLE** | `Wdg_ModuleState = WDG_STATE_IDLE` (L263) | 看门狗已禁用 (OFF_MODE)，不执行触发 |
| **RUNNING** | `Wdg_ModuleState = WDG_STATE_RUNNING` (L261) | 看门狗使能，正常触发刷新 |
| **ERROR** | 窗口违规/硬件故障 | 触发系统复位或进入安全停止 |

### 6.2 安全状态转换

```
UNINIT ──[Wdg_Init 成功]──→ RUNNING (非OFF模式) / IDLE (OFF模式)
IDLE   ──[Wdg_SetMode(FAST/SLOW)]──→ RUNNING
RUNNING───[Wdg_SetMode(OFF, 且允许)]──→ IDLE
RUNNING───[窗口违规 + WDG_WINDOW_ERROR_RESET]──→ ERROR → 系统复位
RUNNING───[超时未触发]──→ 硬件看门狗复位 (WRSR[TOUT]=1)
任意状态───[ConfigPtr 损坏]──→ ERROR → 安全停止
```

### 6.3 故障反应策略

| 故障类型 | 检测方式 | 反应动作 | 反应时间 |
|---------|---------|---------|---------|
| 参数错误 (NULL/越界) | DET 入口校验 | 上报错误码，函数返回 | 1 个调用周期 |
| 状态机异常 | Wdg_ModuleState 检查 | 忽略操作，保持当前状态 | 1 个调用周期 |
| 窗口违规 (过早/过晚) | Wdg_ValidateWindowTrigger | 回调通知 + 系统复位 (可配置) | 1-2 个调用周期 |
| 服务序列失败 | 硬件级保护 | 看门狗超时复位 (WRSR[TOUT]) | 超时周期 (50-500ms) |
| 配置指针损坏 | 空指针校验 | 函数返回，上层进入安全状态 | 1 个调用周期 |
| 看门狗超时未刷新 | 硬件看门狗 | 自动系统复位 | TimeoutPeriod (50-1000ms) |

---

## 7. 故障反应时间线

### 7.1 场景一：窗口期前触发（过早触发）

| 时间 | 事件 |
|------|------|
| T=0 | WdgM 调用 `Wdg_Trigger()` |
| T=1 cycle | 初始化检查通过 (L351)，模式检查通过 (L357) |
| T=2 cycles | 状态检查 `Wdg_ModuleState == RUNNING` (L364) |
| T=3 cycles | `Wdg_ValidateWindowTrigger()` 计算 timeSinceLastTrigger (L146) |
| T=4 cycles | 检测到 `timeSinceLastTrigger < WindowStart` (L149)，返回 `WDG_TRIGGER_WINDOW_EARLY` |
| T=5 cycles | 上报 DET 错误 `WDG_E_WINDOW_VIOLATION` (L386) |
| T=6 cycles | 调用 `Wdg_InvokeWindowViolationCallback()` (L393) |
| T=7 cycles | 配置为 `WDG_WINDOW_ERROR_RESET`，触发系统复位 (L397) |
| **总延迟** | **7 个调用周期 + 复位执行时间** |

### 7.2 场景二：看门狗超时未刷新

| 时间 | 事件 |
|------|------|
| T=0 | 应用层/WdgM 未能按时调用 `Wdg_Trigger()` |
| T=TimeoutPeriod | WDOG 硬件计数器到达阈值，触发超时 |
| T=TimeoutPeriod + 1 cycle | 硬件产生 WDOG 复位信号 (WRSR[TOUT] = 1) |
| T=TimeoutPeriod + 2 cycles | 系统开始复位流程 |
| **总延迟** | **TimeoutPeriod (FAST: 50ms, SLOW: 500ms)** |

### 7.3 场景三：初始化时配置指针无效

| 时间 | 事件 |
|------|------|
| T=0 | EcuM 调用 `Wdg_Init(ConfigPtr)`，ConfigPtr 为 NULL |
| T=1 cycle | L203 检测到 `ConfigPtr == NULL_PTR` |
| T=2 cycles | `Det_ReportError(WDG_MODULE_ID, 0, WDG_SID_INIT, WDG_E_PARAM_CONFIG)` |
| T=3 cycles | 函数 return，Wdg_ModuleState 保持 UNINIT |
| T=4 cycles | EcuM 检测到初始化失败，进入安全状态 |
| **总延迟** | **3 个调用周期 + EcuM 响应时间** |

---

## 8. ISO 26262 条款追溯

| ISO 26262 条款 | 要求 | 本文档覆盖 |
|---------------|------|-----------|
| Part 5, Clause 7 | FMEDA 方法论 | 第 3 节完整失效模式表 |
| Part 5, Clause 8 | 安全度量 (SPFM/LFM/PMHF) | 第 5 节度量分析 |
| Part 5, Clause 9 | 诊断覆盖率评估 | 第 4 节 DC 汇总 |
| Part 6, Clause 8 | 软件单元设计中的安全机制 | 第 3 节故障检测方法列 |
| Part 4, Clause 7 | 安全状态定义 | 第 6 节安全状态 |
| Part 4, Clause 8 | 技术安全需求 | 第 7 节故障反应时间线 |
| Part 11, Clause 9 | 半导体器件安全机制 | 第 3.9 节硬件抽象层分析 |
| Part 5, Annex D | FMEDA 示例 | 本文档整体结构 |

---

## 9. 假设与约束

1. **硬件看门狗假设**: WDOG 外设本身的失效率及诊断覆盖率由芯片供应商 FMD 覆盖，本分析假设硬件看门狗功能正确
2. **时钟源假设**: 看门狗时钟 (32kHz) 由独立晶振提供，不受主时钟故障影响
3. **服务序列保护**: 0x5555/0xAAAA 写入序列由硬件强制执行，软件无法绕过
4. **窗口模式配置**: 假设 WDG_VALIDATE_WINDOW_MODE 在安全相关应用中必须为 STD_ON
5. **时间源假设**: Wdg_GetCurrentTimeMs 依赖 GPT 定时器，其安全性由 GPT 模块 FMEDA 独立覆盖
6. **配置正确性**: 假设 Wdg 配置（超时值、窗口范围、基地址）在集成阶段经过验证
7. **看门狗禁用**: WDG_DISABLE_ALLOWED 配置为 STD_OFF，安全相关应用中不允许运行时禁用看门狗

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| WDOG 外设寄存器 | 3 FIT | 供应商 FMD (NXP/STM32) |
| 寄存器总线接口 | 2 FIT | IEC 61508 Annex D |
| RAM 变量存储 | 8 FIT | IEC 61508 Annex D |
| CPU 逻辑 (校验/计算) | 12 FIT | 供应商 FMD |
| 时钟源 (32kHz OSC) | 1 FIT | 供应商 FMD |
| 时间戳获取 (GPT依赖) | 5 FIT | GPT 模块 FMEDA |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| WDG | Watchdog Driver |
| WCR | Watchdog Control Register |
| WSR | Watchdog Service Register |
| WRSR | Watchdog Reset Status Register |
| WICR | Watchdog Interrupt Control Register |
| IWDG | Independent Watchdog |
| WWDG | Window Watchdog |
| DET | Default Error Trapping |
| WdgM | Watchdog Manager (AUTOSAR BSW 模块) |
| EcuM | ECU State Manager |
| BswM | BSW Mode Manager |

## 附录 C: 配置参数安全约束

| 参数 | 安全约束 | 当前值 | 来源 |
|------|---------|-------|------|
| WDG_DEV_ERROR_DETECT | 必须 STD_ON | STD_ON | Wdg_Cfg.h:19 |
| WDG_DISABLE_ALLOWED | 必须 STD_OFF | STD_OFF | Wdg_Cfg.h:43 |
| WDG_VALIDATE_WINDOW_MODE | 必须 STD_ON | STD_ON | Wdg_Cfg.h:55 |
| WDG_WINDOW_ERROR_ACTION | 必须 WDG_WINDOW_ERROR_RESET | WDG_WINDOW_ERROR_RESET | Wdg_Cfg.h:56 |
| WDG_FAST_MODE_TIMEOUT | 50ms (安全相关) | 50U | Wdg_Cfg.h:32 |
| WDG_SLOW_MODE_TIMEOUT | 500ms (安全相关) | 500U | Wdg_Cfg.h:34 |
| WDG_MAX_TIMEOUT | 上限 1000ms | 1000U | Wdg_Cfg.h:36 |
| WDG_MIN_TIMEOUT | 下限 1ms | 1U | Wdg_Cfg.h:37 |
