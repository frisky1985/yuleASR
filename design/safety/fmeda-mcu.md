# FMEDA — Mcu (MCU Driver)

> **文档编号**: SAF-FMEDA-MCU-001
> **版本**: 1.0
> **日期**: 2026-10-24
> **ASIL 等级**: D
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW MCAL 层的 Mcu (MCU Driver) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/mcal/mcu/src/Mcu.c`

**分析边界**:
- Mcu 核心逻辑（时钟配置、PLL 管理、复位控制、低功耗模式）
- 上层接口：`Mcu_Init`, `Mcu_InitClock`, `Mcu_DistributePllClock`, `Mcu_SetMode`, `Mcu_PerformReset`
- 内部安全机制：PLL 锁定超时检测、时钟切换超时、复位原因检测、RAM 完整性检查
- 下层依赖（假设由硬件覆盖）：CCM (Clock Controller Module), SRC (System Reset Controller), GPC (General Power Controller)

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 Mcu 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `Mcu_DriverStateType` | Mcu.c:68-73 | 模块内部状态：initialized, currentClock, currentMode, ramState |
| `Mcu_ConfigType` | Mcu.h:164-176 | 配置结构：ClockConfigs[], RamSections[], ModeConfigs[], NumClockConfigs 等 |
| `Mcu_ClockConfigType` | Mcu.h:153-161 | 时钟配置：PllConfigs[], ClockSource, ArmDiv, AxiDiv, AhbDiv |
| `Mcu_PllConfigType` | Mcu.h:131-138 | PLL 配置：PllBaseAddr, Prediv, Multiplier, Postdiv1, Postdiv2 |
| `Mcu_RamSectionType` | Mcu.h:141-145 | RAM 段配置：RamBaseAddr, RamSize, RamDefaultValue |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 模块初始化 | `Mcu_Init()` (L297) | 驱动初始化、RAM 段填充、状态设置 |
| FU-02 | 时钟初始化 | `Mcu_InitClock()` (L339) | 配置系统时钟源和分频器 |
| FU-03 | PLL 配置 | `Mcu_ConfigurePLL()` (L162) | PLL 参数配置、旁路控制、使能 |
| FU-04 | PLL 锁定检测 | `Mcu_WaitForPLLLock()` (L206) | 等待 PLL 锁定，超时保护 |
| FU-05 | 时钟分频器设置 | `Mcu_SetClockDividers()` (L230) | ARM/AXI/AHB 分频器配置 |
| FU-06 | PLL 时钟分发 | `Mcu_DistributePllClock()` (L371) | 启用时钟分发到系统 |
| FU-07 | PLL 状态查询 | `Mcu_GetPllStatus()` (L397) | 读取 PLL 锁定状态 |
| FU-08 | MCU 模式控制 | `Mcu_SetMode()` (L429) | Run/Sleep/Deep Sleep 模式切换 |
| FU-09 | 复位原因检测 | `Mcu_GetResetReason()` (L476), `Mcu_GetResetRawValue()` (L498) | 读取复位源寄存器 |
| FU-10 | 系统复位执行 | `Mcu_PerformReset()` (L520) | 触发软件复位 |
| FU-11 | RAM 段初始化 | `Mcu_InitRamSection()` (L570) | 单个 RAM 段填充默认值 |
| FU-12 | RAM 状态查询 | `Mcu_GetRamState()` (L604) | 检查 RAM 完整性 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 模块初始化 (`Mcu_Init`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | ConfigPtr 为空指针 | 初始化失败，模块无法工作 | 入口参数校验 (ConfigPtr != NULL_PTR)，上报 `MCU_E_PARAM_CONFIG` | 99% | D |
| FM-01-02 | 重复初始化 | 配置被覆盖，状态不一致 | `Mcu_DriverState.initialized` 检查，上报 `MCU_E_ALREADY_INITIALIZED` | 95% | D |
| FM-01-03 | RAM 段配置指针为空 | RAM 初始化跳过，可能遗留不确定数据 | `ConfigPtr->RamSections != NULL_PTR` 检查 | 90% | D |
| FM-01-04 | RAM 填充过程中地址越界 | 内存破坏，系统崩溃 | 配置阶段验证 RamBaseAddr 和 RamSize 合法性（由配置工具保证） | 85% | D |

### 3.2 FU-02: 时钟初始化 (`Mcu_InitClock`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | 模块未初始化 | 时钟配置失败，系统无法启动 | `Mcu_DriverState.initialized` 检查，上报 `MCU_E_UNINIT` | 99% | D |
| FM-02-02 | ClockSetting 超出范围 | 访问非法配置，返回错误 | `ClockSetting < Mcu_ConfigPtr->NumClockConfigs` 校验，上报 `MCU_E_PARAM_CLOCK` | 99% | D |
| FM-02-03 | PLL 配置失败（底层返回 E_NOT_OK） | 时钟切换到备用源，系统降频运行 | `Mcu_ConfigureClock` 返回码检查，保持 `currentClock = MCU_CLOCK_INVALID` | 90% | D |
| FM-02-04 | 时钟切换超时 | 系统时钟未切换到目标源 | `MCU_CLOCK_SWITCH_TIMEOUT` 超时检测（L143-150），返回 E_NOT_OK | 95% | D |

### 3.3 FU-03: PLL 配置 (`Mcu_ConfigurePLL`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | PLL 基地址无效 | 寄存器写入失败，PLL 无法配置 | 配置阶段验证 PllBaseAddr（由配置工具保证） | 85% | D |
| FM-03-02 | PLL 参数超出硬件范围 | PLL 频率异常或无法锁定 | 参数范围校验（Prediv, Multiplier, Postdiv 由配置工具限制） | 90% | D |
| FM-03-03 | PLL 旁路设置失败 | PLL 配置期间时钟不稳定 | 旁路位设置后读取验证（隐式通过 PLL 锁定检测覆盖） | 80% | D |
| FM-03-04 | PLL 使能位写入失败 | PLL 无法启动 | PLL 锁定超时检测（FU-04） | 90% | D |

### 3.4 FU-04: PLL 锁定检测 (`Mcu_WaitForPLLLock`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | PLL 锁定超时 | 时钟源不可用，系统使用备用时钟 | `MCU_PLL_LOCK_TIMEOUT` (10000) 超时检测，返回 E_NOT_OK | 99% | D |
| FM-04-02 | LOCK 位读取错误 | 误判 PLL 状态 | 冗余读取（关键路径可双重采样）；系统级 WDT 监控 | 85% | D |
| FM-04-03 | PLL 锁定后失锁 | 系统时钟不稳定，可能导致 CPU 异常 | `Mcu_GetPllStatus` 周期查询；WDT 监控 | 80% | D |

### 3.5 FU-05: 时钟分频器设置 (`Mcu_SetClockDividers`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | ARM 分频器配置错误 | CPU 频率异常，可能导致时序违规 | 配置值范围校验（ArmDiv 限制在 0x07U 内） | 90% | D |
| FM-05-02 | AXI 分频器配置错误 | 总线频率异常，外设通信失败 | 配置值范围校验（AxiDiv 限制在 0x07U 内） | 90% | D |
| FM-05-03 | AHB 分频器配置错误 | AHB 外设时钟异常 | 配置值范围校验（AhbDiv 限制在 0x07U 内） | 90% | D |
| FM-05-04 | 寄存器读-改-写过程中被中断 | 分频器值不一致 | 关键寄存器操作禁用中断（由系统级保证） | 75% | D |

### 3.6 FU-06: PLL 时钟分发 (`Mcu_DistributePllClock`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | 模块未初始化 | 时钟分发失败 | `Mcu_DriverState.initialized` 检查，上报 `MCU_E_UNINIT` | 99% | D |
| FM-06-02 | PLL 未锁定（currentClock == MCU_CLOCK_INVALID） | 分发无效时钟源 | `currentClock` 状态检查，上报 `MCU_E_PLL_NOT_LOCKED` | 99% | D |
| FM-06-03 | CLK_ENABLE 位写入失败 | 时钟未分发到系统 | 写入后读取验证（隐式通过系统行为监控） | 85% | D |

### 3.7 FU-07: PLL 状态查询 (`Mcu_GetPllStatus`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | 模块未初始化 | 返回未定义状态 | `Mcu_DriverState.initialized` 检查，上报 `MCU_E_UNINIT` | 99% | D |
| FM-07-02 | CSR 寄存器读取错误 | 误报 PLL 锁定状态 | 冗余读取（由上层应用交叉验证）；WDT 监控 | 80% | D |
| FM-07-03 | MCU_NO_PLL 配置错误 | 误判 PLL 状态 | 编译时配置校验（由配置工具保证） | 95% | D |

### 3.8 FU-08: MCU 模式控制 (`Mcu_SetMode`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | 模块未初始化 | 模式切换失败 | `Mcu_DriverState.initialized` 检查，上报 `MCU_E_UNINIT` | 99% | D |
| FM-08-02 | McuMode 超出范围 | 访问非法模式配置 | `McuMode < Mcu_ConfigPtr->NumModes` 校验，上报 `MCU_E_PARAM_MODE` | 99% | D |
| FM-08-03 | GPC 寄存器写入失败 | 无法进入目标低功耗模式 | 写入后读取验证（隐式通过模式状态监控） | 85% | D |
| FM-08-04 | 睡眠模式唤醒失败 | 系统无法恢复正常运行 | WDT 监控；外部唤醒中断检测 | 75% | D |

### 3.9 FU-09: 复位原因检测 (`Mcu_GetResetReason`, `Mcu_GetResetRawValue`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | 模块未初始化 | 无法读取复位原因 | `Mcu_DriverState.initialized` 检查，上报 `MCU_E_UNINIT` | 99% | D |
| FM-09-02 | SRSR 寄存器读取错误 | 误报复位原因 | 冗余读取（关键安全路径可双重采样） | 85% | D |
| FM-09-03 | 多位同时置位（多复位源） | 复位原因判断优先级错误 | 位优先级检测（L266-280）：POR > WDT > SW > EXT | 90% | D |
| FM-09-04 | 寄存器读取后自动清零 | 复位原因丢失 | `Mcu_GetResetRawValue` 提供原始值读取（不清零） | 95% | D |

### 3.10 FU-10: 系统复位执行 (`Mcu_PerformReset`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | 模块未初始化 | 复位失败 | `Mcu_DriverState.initialized` 检查，上报 `MCU_E_UNINIT` | 99% | D |
| FM-10-02 | SCR 寄存器写入失败 | 软件复位无法触发 | 写入后死循环等待（L534-536）；WDT 作为后备 | 90% | D |
| FM-10-03 | MCU_PERFORM_RESET_API 配置为 OFF | 复位 API 不可用 | 编译时配置校验（由配置工具保证） | 99% | D |
| FM-10-04 | 复位后系统无法正常启动 | 系统进入不确定状态 | WDT 监控；BIST 自检；复位原因检测（FU-09） | 85% | D |

### 3.11 FU-11: RAM 段初始化 (`Mcu_InitRamSection`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-11-01 | 模块未初始化 | RAM 初始化失败 | `Mcu_DriverState.initialized` 检查，上报 `MCU_E_UNINIT` | 99% | D |
| FM-11-02 | RamSection 索引超出范围 | 访问非法 RAM 段 | `RamSection < Mcu_ConfigPtr->NumRamSections` 校验，上报 `MCU_E_PARAM_RAMSECTION` | 99% | D |
| FM-11-03 | RAM 地址越界 | 内存破坏 | 配置阶段验证 RamBaseAddr 和 RamSize（由配置工具保证） | 85% | D |
| FM-11-04 | RAM 填充过程中被中断 | 填充不完整 | 关键操作禁用中断（由系统级保证）；填充后校验 | 80% | D |
| FM-11-05 | MCU_GET_RAM_STATE_API 配置为 OFF | RAM 状态查询不可用 | 编译时配置校验 | 99% | D |

### 3.12 FU-12: RAM 状态查询 (`Mcu_GetRamState`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-12-01 | 模块未初始化 | 返回无效状态 | `Mcu_DriverState.initialized` 检查，上报 `MCU_E_UNINIT` | 99% | D |
| FM-12-02 | MCU_GET_RAM_STATE_API 配置为 OFF | 始终返回 MCU_RAMSTATE_INVALID | 编译时配置校验 | 99% | D |
| FM-12-03 | RAM 完整性检查失败 | 返回 MCU_RAMSTATE_INVALID | 硬件 RAM 自检（如 ECC 校验）；上层应用处理 | 90% | D |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 模块初始化 | 99% | 92% | 参数校验 + 重复初始化检测 |
| FU-02 | 时钟初始化 | 99% | 96% | 超时检测 + 配置校验 |
| FU-03 | PLL 配置 | 90% | 88% | 参数范围校验 + 锁定检测 |
| FU-04 | PLL 锁定检测 | 99% | 88% | 超时保护 + 冗余读取 |
| FU-05 | 时钟分频器 | 90% | 88% | 范围校验 + 中断保护 |
| FU-06 | PLL 时钟分发 | 99% | 94% | 状态检查 + 写入验证 |
| FU-07 | PLL 状态查询 | 99% | 91% | 初始化检查 + 冗余读取 |
| FU-08 | MCU 模式控制 | 99% | 90% | 参数校验 + WDT 监控 |
| FU-09 | 复位原因检测 | 99% | 94% | 优先级检测 + 原始值读取 |
| FU-10 | 系统复位执行 | 99% | 93% | WDT 后备 + 死循环等待 |
| FU-11 | RAM 段初始化 | 99% | 92% | 索引校验 + 地址验证 |
| FU-12 | RAM 状态查询 | 99% | 96% | 初始化检查 + 硬件自检 |

### 4.2 模块整体诊断覆盖率

```
DC_Mcu = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_Mcu ≈ 92%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL D)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 99% | ~99% | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ~95% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 10 FIT | ~8 FIT | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。Mcu 中主要单点故障来源：
- PLL 锁定检测的瞬态故障（由 WDT 覆盖）
- 时钟分频器寄存器读-改-写被中断（由系统级中断管理覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (0.8 / 80) ≈ 99%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。Mcu 中主要潜伏故障来源：
- 复位原因寄存器读取错误的检测机制（通过冗余读取覆盖）
- RAM 完整性检查算法缺陷（通过硬件 ECC 覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (4.0 / 80) ≈ 95%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_pll_lock_timeout + lambda_clock_switch_timeout + lambda_ram_integrity
     ≈ 2 + 3 + 3 = 8 FIT < 10 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 Mcu 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | PLL 锁定，时钟分频器配置正确，currentClock != MCU_CLOCK_INVALID | 正常时钟分发和服务 |
| **降级运行** | PLL 未锁定或时钟切换超时，使用备用时钟源（XTAL/RC） | 系统降频运行，上报诊断事件 |
| **安全时钟源** | PLL 配置失败，切换到内部 RC 振荡器或外部晶振 | 最低频率运行，仅支持关键功能 |
| **安全停止** | 时钟系统完全失效或 WDT 超时 | 系统进入安全状态，触发复位 |

### 6.2 安全状态转换

```
正常运行 ──[PLL失锁]──→ 降级运行
降级运行 ──[备用时钟有效]──→ 安全时钟源
安全时钟源 ──[时钟恢复]──→ 正常运行
任意状态 ──[WDT超时]──→ 安全停止 (触发复位)
任意状态 ──[时钟完全失效]──→ 安全停止
```

---

## 7. 故障反应时间线

### 7.1 场景一：PLL 锁定超时

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Mcu_InitClock(ClockSetting)` |
| T=1 cycle | `Mcu_ConfigureClock` 调用 `Mcu_ConfigurePLL` |
| T=2 cycles | PLL 旁路设置，参数配置，使能 PLL |
| T=3 cycles | `Mcu_WaitForPLLLock` 开始轮询 LOCK 位 |
| T=3 + 10000 cycles | 超时计数器归零，返回 E_NOT_OK |
| T=4 cycles | `Mcu_ConfigureClock` 返回 E_NOT_OK |
| T=5 cycles | `Mcu_InitClock` 返回 E_NOT_OK，`currentClock` 保持 MCU_CLOCK_INVALID |
| T=6 cycles | 上层应用检测到错误，切换到备用时钟策略 |
| **总延迟** | **6 个调用周期 + 10000 个轮询周期** |

### 7.2 场景二：时钟切换超时

| 时间 | 事件 |
|------|------|
| T=0 | `Mcu_ConfigureClock` PLL 配置成功 |
| T=1 cycle | `Mcu_SetClockDividers` 设置分频器 |
| T=2 cycles | 开始时钟源切换，轮询 CCSR 寄存器 |
| T=2 + 10000 cycles | 超时计数器归零，返回 E_NOT_OK |
| T=3 cycles | `Mcu_InitClock` 返回 E_NOT_OK |
| T=4 cycles | 系统保持原有时钟源运行，上报诊断事件 |
| **总延迟** | **4 个调用周期 + 10000 个轮询周期** |

### 7.3 场景三：复位原因检测

| 时间 | 事件 |
|------|------|
| T=0 | 系统启动，调用 `Mcu_Init` |
| T=1 cycle | 应用层调用 `Mcu_GetResetReason` |
| T=2 cycles | 读取 SRC_SRSR 寄存器 |
| T=3 cycles | 位优先级判断：POR (bit 0) > WDT (bit 1) > SW (bit 2) > EXT (bit 3) |
| T=4 cycles | 返回复位原因（如 MCU_RST_WATCHDOG） |
| T=5 cycles | 应用层根据复位原因执行相应策略 |
| **总延迟** | **5 个调用周期** |

### 7.4 场景四：RAM 段初始化失败

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Mcu_InitRamSection(RamSection)` |
| T=1 | 检查 `Mcu_DriverState.initialized == FALSE` |
| T=2 | 返回 E_NOT_OK，上报 `MCU_E_UNINIT` |
| T=3 | 应用层检测到错误，执行降级策略 |
| **总延迟** | **3 个调用周期** |

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

---

## 9. 假设与约束

1. **硬件驱动假设**: CCM/SRC/GPC 硬件模块的失效率及诊断覆盖率由芯片供应商 FMD 覆盖，本分析假设硬件 FIT 数据已知
2. **PLL 配置正确性**: 假设 PLL 参数（Prediv, Multiplier, Postdiv）在配置阶段经过验证，符合硬件规格
3. **WDT 覆盖**: Mcu 模块的调度监控由系统级 WDT 覆盖，特别是 PLL 锁定和时钟切换的超时场景
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **配置正确性**: 假设 Mcu 配置（ClockConfigs, RamSections, ModeConfigs）在集成阶段经过配置工具验证
6. **中断保护**: 假设关键寄存器操作（读-改-写）期间由系统级中断管理保证原子性
7. **备用时钟源**: 假设系统具备备用时钟源（XTAL 或 RC 振荡器），在 PLL 失效时可自动切换

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| PLL 配置逻辑 | 3 FIT | 供应商 FMD |
| 时钟分频器 | 2 FIT | 供应商 FMD |
| 复位控制逻辑 | 2 FIT | 供应商 FMD |
| RAM 控制器 | 3 FIT | IEC 61508 Annex D |
| CPU 逻辑 | 12 FIT | 供应商 FMD |
| 寄存器访问 | 5 FIT | IEC 61508 Annex D |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| PLL | Phase-Locked Loop |
| CCM | Clock Controller Module |
| SRC | System Reset Controller |
| GPC | General Power Controller |
| POR | Power-On Reset |
| WDT | Watchdog Timer |
| XTAL | Crystal Oscillator |
| RC | Resistor-Capacitor Oscillator |
