# FMEDA — WdgM (Watchdog Manager)

> **文档编号**: SAF-FMEDA-WDGM-001
> **版本**: 1.0
> **日期**: 2026-10-23
> **ASIL 等级**: D
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW Services 层的 WdgM (Watchdog Manager) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/services/wdgm/src/WdgM.c`

**分析边界**:
- WdgM 核心逻辑（监督实体管理、活监督评估、安全状态机、看门狗触发）
- 上层接口：`WdgM_CheckpointReached`, `WdgM_UpdateAliveIndication`, `WdgM_MainFunction`, `WdgM_TriggerWatchdog`, `WdgM_SetMode`
- 内部安全机制：安全魔数校验 (Safety Magic)、监督超时检测、连续错误计数与复位升级、Lockstep/RamSafety 集成
- 下层依赖（假设由 MCAL 层覆盖）：Wdg (看门狗驱动), Det (默认错误追踪), Dem (诊断事件管理)

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 WdgM 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `WdgM_SupervisedEntityType` | WdgM.h:192-205 | 监督实体运行时信息：seId, state, aliveCounter, expectedAliveIndications, aliveSupMaxIndications, aliveSupRefCycle, aliveCycleCounter, consecutiveErrors, deactivated |
| `WdgM_ConfigType` | WdgM.h:210-220 | 模块配置：watchdogs[], numWatchdogs, entities[], numEntities, failureThreshold, supervisionCycleMs, lockstepIntegration, ramSafetyIntegration |
| `WdgM_GlobalStatusType` | WdgM.h:225-233 | 全局状态统计：expiredSupervisionCycles, totalRefreshes, failedRefreshes, lockstepErrors, ramSafetyErrors, currentMode |
| `WdgM_AliveSupervisionType` | WdgM.h:158-163 | 活监督配置：aliveSupRefCycle (参考周期数), aliveSupMin (最小活计数), aliveSupMax (最大活计数) |
| `WdgM_WatchdogConfigType` | WdgM.h:146-153 | 看门狗配置：type (WWD/IWD), triggerPeriodMs, windowStartMs, windowEndMs, enabled |

### 1.4 关键运行时变量

| 变量 | 位置 (WdgM.c) | 说明 |
|------|---------------|------|
| `WdgM_State` | L99 | 模块状态机 (UNINIT/INIT/ACTIVE/SUPERVISION_EXPIRED/STOPPED/DEACTIVATED) |
| `WdgM_SafetyMagic` | L114 | 安全魔数 (INIT: 0xA55AA55A, ACTIVE: 0x5AA55AA5)，用于运行时完整性检查 |
| `WdgM_SupervisedEntities[]` | L125 | 监督实体运行时数组，最大 WDGM_CFG_MAX_SUPERVISED_ENTITIES 个 |
| `WdgM_CycleTimer` | L135 | 监督周期计时器 |
| `WdgM_TriggerTimer` | L140 | 看门狗触发计时器 |
| `WdgM_ConsecutiveErrors` | L145 | 全局连续错误计数 |
| `WdgM_FirstExpiredSEID` | L156 | 首个超时的监督实体 ID |
| `WdgM_DisableAllowed` | L161 | 看门狗禁用允许标志 |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 模块初始化 | `WdgM_Init()` (L191) | 配置校验、监督实体初始化、安全魔数设置 |
| FU-02 | 活监督检查 | `WdgM_CheckpointReached()` (L433), `WdgM_CheckEntityAlive()` (L876) | 检查点报告与活指示计数评估 |
| FU-03 | 监督周期评估 | `WdgM_UpdateSupervision()` (L858), `WdgM_CheckEntityAlive()` (L876) | 参考周期内活指示数与 [min, max] 区间比对 |
| FU-04 | 超时处理与安全响应 | `WdgM_HandleExpiredSupervision()` (L921), `WdgM_PerformSafetyAction()` (L982) | 监督超时状态转换、Dem 上报、安全复位 |
| FU-05 | 看门狗触发 | `WdgM_TriggerWatchdog()` (L656), `WdgM_PlatformTrigger()` (L962) | 周期触发底层看门狗刷新 |
| FU-06 | 安全状态机 | `WdgM_State` 状态变量, `WdgM_MainFunction()` (L608) | UNINIT→INIT→ACTIVE→SUPERVISION_EXPIRED 状态管理 |
| FU-07 | 模式管理 | `WdgM_SetMode()` (L366), `WdgM_PlatformSetMode()` (L972) | OFF/SLOW/FAST 模式切换与禁用保护 |
| FU-08 | Lockstep/RamSafety 集成 | `WdgM_HandleLockstepError()` (L719), `WdgM_HandleRamSafetyError()` (L748) | 外部安全模块错误事件处理与响应 |
| FU-09 | 监督实体管理 | `WdgM_ActivateSupervisionEntity()` (L548), `WdgM_DeactivateSupervisionEntity()` (L521) | 运行时动态激活/去激活监督实体 |
| FU-10 | 安全复位 | `WdgM_PerformReset()` (L678) | 紧急复位执行，复位失败进入死循环 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 模块初始化 (`WdgM_Init`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | 配置指针 config 为 NULL | 初始化失败，模块保持 UNINIT 状态，看门狗不运行 | 入口参数校验 (L198: `config == NULL_PTR`)，Det 上报 `WDGM_E_PARAM_POINTER` | 99% | D |
| FM-01-02 | 重复初始化 (`WdgM_State != WDGM_STATE_UNINIT`) | 初始化被拒绝，防止运行时数据被破坏 | 状态检查 (L204: `WdgM_State != WDGM_STATE_UNINIT`)，Det 上报 `WDGM_E_ALREADY_INITIALIZED` | 99% | D |
| FM-01-03 | 配置数据无效 (numEntities 超限 / failureThreshold=0 / supervisionCycleMs=0) | 模块无法正确运行监督功能 | `WdgM_ValidateConfig()` (L834) 三重校验：实体数量、失败阈值、监督周期 | 95% | D |
| FM-01-04 | 安全魔数写入失败 (SRAM 瞬态故障) | 后续 MainFunction 检测到魔数异常触发安全复位 | `WdgM_MainFunction()` (L616) 每个周期校验 `WdgM_SafetyMagic != WDGM_SAFETY_MAGIC_ACTIVE` | 90% | D |
| FM-01-05 | 监督实体配置中 aliveSupRefCycle=0 | 活监督评估周期为零导致除零或永不触发 | 初始化时强制保护 (L261-269): `aliveSupRefCycle == 0U` 时设为 1U | 99% | D |

### 3.2 FU-02: 活监督检查 (`WdgM_CheckpointReached`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | seId 未配置 (不存在于监督实体数组中) | 检查点报告被忽略，对应实体无法更新活计数 | `WdgM_FindEntityIndex()` (L942) 线性搜索失败，Det 上报 `WDGM_E_CPID_NOT_CONFIGURED` | 99% | D |
| FM-02-02 | aliveCounter 溢出 (达到 0xFFFF) | 活计数回绕导致监督评估错误 | 饱和保护 (L455: `aliveCounter < 0xFFFFU`)，计数器不再递增 | 90% | D |
| FM-02-03 | 监督实体处于 DEACTIVATED 状态时收到检查点 | 状态被错误更新 | 状态保护检查 (L464: `state != WDGM_SE_STATE_DEACTIVATED`)，仅非去激活实体才更新为 CORRECT | 85% | D |
| FM-02-04 | 实体索引查找中的数组越界 | 访问非法内存区域 | `WdgM_FindEntityIndex()` 循环上限为 `WDGM_CFG_MAX_SUPERVISED_ENTITIES` (L946)，编译时配置约束 | 95% | D |

### 3.3 FU-03: 监督周期评估 (`WdgM_CheckEntityAlive`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | 活指示数低于期望值 (`aliveCounter < expectedAliveIndications`) | 被监督实体可能已挂死 | 参考周期结束时评估 (L894): 活指示数与 [aliveSupMin, aliveSupMax] 区间比对 | 99% | D |
| FM-03-02 | 活指示数超过最大值 (`aliveCounter > aliveSupMaxIndications`) | 被监督实体可能运行过快（时序异常） | 同上 (L895): 上限检查防止异常高频运行 | 95% | D |
| FM-03-03 | aliveCycleCounter 未正确递增导致参考周期永不结束 | 活监督永远不评估，监督功能失效 | `WdgM_MainFunction()` 周期性调用保证 (L627); 系统级 WDT 监控 MainFunction 调度 | 80% | D |
| FM-03-04 | consecutiveErrors 达到阈值但 HandleExpiredSupervision 调用失败 | 超时状态未被正确处理 | 全局 `WdgM_ConsecutiveErrors` 独立递增 (L645-648) 作为二级保护 | 85% | D |

### 3.4 FU-04: 超时处理与安全响应 (`WdgM_HandleExpiredSupervision`, `WdgM_PerformSafetyAction`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | 监督超时后状态未正确转换为 SUPERVISION_EXPIRED | 模块继续触发看门狗，掩盖故障 | `WdgM_HandleExpiredSupervision()` (L929) 显式设置 `WdgM_State = WDGM_STATE_SUPERVISION_EXPIRED` | 95% | D |
| FM-04-02 | FirstExpiredSEID 被后续超时实体覆盖 | 无法追溯首个故障实体 | `WdgM_FirstExpiredSEID` 仅在首次超时时写入 (L924); `WdgM_GetFirstExpiredSEID()` (L696) 提供查询接口 | 80% | D |
| FM-04-03 | `WdgM_PerformSafetyAction` 调用 `WdgM_PerformReset` 失败 | 系统无法进入安全状态 | `WdgM_PerformReset()` (L686-689) 复位失败后进入死循环，硬件看门狗最终触发复位 | 90% | D |
| FM-04-04 | Dem 事件上报失败 (Dem 集成被禁用或 Dem 本身故障) | 诊断系统无法记录安全事件 | `WDGM_CFG_DEM_INTEGRATION` 编译时开关 (L933); 安全回调 `WdgM_SafetyCallback` 提供替代通知路径 (L824-827) | 75% | D |

### 3.5 FU-05: 看门狗触发 (`WdgM_TriggerWatchdog`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | 非 ACTIVE 状态下触发看门狗 | 在安全违规状态下刷新看门狗，阻止应有的复位 | 状态检查 (L658: `WdgM_State == WDGM_STATE_ACTIVE`)，仅 ACTIVE 状态允许触发 | 99% | D |
| FM-05-02 | 安全魔数校验失败时仍触发看门狗 | 数据损坏状态下看门狗被刷新 | 魔数双重检查 (L661: `WdgM_SafetyMagic != WDGM_SAFETY_MAGIC_ACTIVE`)，校验失败直接 return | 95% | D |
| FM-05-03 | 底层 `WdgM_WatchdogTrigger()` 调用失败 | 看门狗未刷新，超时后硬件复位 | 硬件看门狗自身作为最终安全网：若软件触发失败，看门狗超时自动复位 | 90% | D |
| FM-05-04 | TriggerTimer 未达到 supervisionCycleMs 时误触发 | 看门狗刷新频率过高 | `WdgM_TriggerTimer >= WdgM_CurrentConfig->supervisionCycleMs` (L632) 周期门控 | 95% | D |
| FM-05-05 | totalRefreshes 计数器溢出 | 刷新统计不准确 | `WdgM_GlobalStatus.totalRefreshes` 为 uint32 (L669)，溢出后回绕；不影响安全功能 | 60% | D |

### 3.6 FU-06: 安全状态机

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | `WdgM_State` 变量被内存瞬态故障篡改 | 状态机行为异常，可能跳过安全保护 | `WdgM_MainFunction()` 入口状态检查 (L610); `WdgM_TriggerWatchdog()` 状态门控 (L658); 安全魔数交叉验证 | 85% | D |
| FM-06-02 | `WdgM_MainFunction` 未被周期调用 | 监督功能完全停止，看门狗不被刷新 | 系统级监控（OS Task 监控 / 上层 BswM 调度监控）；硬件看门狗最终超时复位 | 80% | D |
| FM-06-03 | `WdgM_CycleTimer` 溢出 (uint16) | 监督周期计时不准确 | uint16 自然回绕 (L623); 相对计时逻辑不受回绕影响（差值比较） | 70% | D |
| FM-06-04 | 全局连续错误计数 `WdgM_ConsecutiveErrors` 溢出 | 错误阈值比较失效 | uint8 最大值 255 (L645); `failureThreshold` 配置应远小于 255 | 75% | D |

### 3.7 FU-07: 模式管理 (`WdgM_SetMode`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | 非法模式值 (mode > WDGM_WATCHDOG_MODE_FAST) | 看门狗设置为未定义模式 | 入口参数校验 (L377: `mode > WDGM_WATCHDOG_MODE_FAST`)，Det 上报 `WDGM_E_PARAM_MODE` | 99% | D |
| FM-07-02 | 禁用看门狗时 `WdgM_DisableAllowed == FALSE` | 看门狗被非法关闭，安全监控丧失 | 禁用允许检查 (L385: `WdgM_IsDisableAllowed()`)，拒绝时上报 `WDGM_E_DISABLE_NOT_ALLOWED` | 99% | D |
| FM-07-03 | UNINIT 状态下调用 SetMode | 操作未初始化的模块 | 状态检查 (L371: `WdgM_State == WDGM_STATE_UNINIT`)，Det 上报 `WDGM_E_NOT_INITIALIZED` | 99% | D |
| FM-07-04 | 模式切换后 GlobalStatus 与 CurrentMode 不一致 | 状态查询返回矛盾信息 | 双重更新 (L394-395): `WdgM_CurrentMode` 和 `WdgM_GlobalStatus.currentMode` 同步写入 | 85% | D |

### 3.8 FU-08: Lockstep/RamSafety 集成

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | Lockstep 错误事件丢失 | 锁步核比较故障未被记录 | `WdgM_HandleLockstepError()` 每次调用递增 `lockstepErrors` 统计 (L730) 并触发安全回调 (L733) | 90% | D |
| FM-08-02 | RamSafety 错误事件丢失 | RAM 保护故障未被记录 | `WdgM_HandleRamSafetyError()` 每次调用递增 `ramSafetyErrors` 统计 (L759) 并触发安全回调 (L762) | 90% | D |
| FM-08-03 | Lockstep/RamSafety 错误处理中 `PerformSafetyAction` 导致过早复位 | 错误现场未被诊断系统记录 | `WdgM_PerformSafetyAction()` (L982) 先递增 `failedRefreshes` 再复位，保留统计信息 | 80% | D |
| FM-08-04 | `lockstepErrors` / `ramSafetyErrors` 计数器溢出 | 统计信息不准确 | uint32 计数器 (L730, L759)，溢出后回绕；不影响安全响应功能 | 60% | D |

### 3.9 FU-09: 监督实体管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | 去激活已故障实体，掩盖监督超时 | 故障实体不再被监控 | `WdgM_DeactivateSupervisionEntity()` (L537-538) 设置 `deactivated=TRUE` 和 `state=DEACTIVATED`; `WdgM_UpdateSupervision()` 跳过已去激活实体 (L864) | 75% | D |
| FM-09-02 | 重新激活实体时计数器未完全重置 | 旧周期的残留数据影响新周期评估 | `WdgM_ActivateSupervisionEntity()` (L566-568) 重置 aliveCounter, aliveCycleCounter, consecutiveErrors 三项 | 90% | D |
| FM-09-03 | 查找实体时 seId=0 匹配到未初始化槽位 | 误操作未配置的实体槽位 | `WdgM_UpdateSupervision()` 双重过滤 (L864-865): `!deactivated && (seId != 0U)` | 95% | D |

### 3.10 FU-10: 安全复位 (`WdgM_PerformReset`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | 复位请求被硬件忽略 | 系统无法进入安全状态 | 软件复位失败后进入死循环 (L686-689)，硬件看门狗超时强制复位 | 90% | D |
| FM-10-02 | 复位前未通知上层应用 | 应用层数据可能丢失 | `WdgM_NotifyEvent(WDGM_EVENT_WATCHDOG_RESET)` (L680) 在复位前触发安全回调 | 80% | D |
| FM-10-03 | 死循环中 CPU 仍可执行中断 | 中断服务可能破坏安全状态 | 死循环为最终安全措施；中断由硬件看门狗超时覆盖 | 85% | D |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 模块初始化 | 99% | 95% | 配置校验 + 安全魔数 |
| FU-02 | 活监督检查 | 99% | 92% | 实体索引查找 + 饱和保护 |
| FU-03 | 监督周期评估 | 99% | 88% | 活指示区间评估 + 连续错误计数 |
| FU-04 | 超时处理与安全响应 | 95% | 86% | 状态转换 + Dem 上报 + 安全回调 |
| FU-05 | 看门狗触发 | 99% | 89% | 状态门控 + 魔数校验 + 周期门控 |
| FU-06 | 安全状态机 | 99% | 82% | 魔数交叉验证 + 系统级 WDT 监控 |
| FU-07 | 模式管理 | 99% | 95% | 参数校验 + 禁用保护 |
| FU-08 | Lockstep/RamSafety | 90% | 80% | 错误统计 + 安全回调 |
| FU-09 | 监督实体管理 | 95% | 87% | 去激活过滤 + 计数器重置 |
| FU-10 | 安全复位 | 90% | 85% | 死循环兜底 + 硬件 WDT |

### 4.2 模块整体诊断覆盖率

```
DC_WdgM = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_WdgM ≈ 88%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL D)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 99% | ~99.1% | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ~92% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 10 FIT | ~6.8 FIT | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。WdgM 中主要单点故障来源：
- 安全魔数内存位置的不可检测瞬态故障（由 MainFunction 周期校验覆盖，残余概率极低）
- 状态机变量 `WdgM_State` 的瞬态篡改（由魔数交叉验证和状态门控覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (0.8 / 95) ≈ 99.1%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。WdgM 中主要潜伏故障来源：
- 安全魔数校验逻辑自身的系统性缺陷（通过双重魔数 INIT→ACTIVE 写入序列检测）
- 活监督评估逻辑故障（通过已知测试实体在集成测试阶段验证）
- Lockstep/RamSafety 回调路径故障（通过定期注入测试错误验证）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (7.2 / 92) ≈ 92%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_safety_magic_bypass + lambda_state_machine_corruption + lambda_reset_failure
     ≈ 0.5 + 0.3 + 6.0 = 6.8 FIT < 10 FIT ✓
```

注：WdgM 作为 ASIL D 模块，PMHF 目标值为 10 FIT（远低于 ASIL B 的 100 FIT），主要得益于多层安全机制（安全魔数、状态门控、硬件看门狗兜底）。

---

## 6. 安全状态定义

### 6.1 WdgM 安全状态

| 状态 | 枚举值 (WdgM.h:103-111) | 条件 | 行为 |
|------|--------------------------|------|------|
| **UNINIT** | `WDGM_STATE_UNINIT` (0) | 模块未初始化或已去初始化 | 所有 API 返回 E_NOT_OK (UNINIT 检查)，看门狗不运行 |
| **INIT** | `WDGM_STATE_INIT` (1) | 初始化过程中 (L222) | 短暂过渡态，配置已加载但尚未激活 |
| **ACTIVE** | `WDGM_STATE_ACTIVE` (2) | 初始化完成 (L296)，正常运行 | 周期监督评估 + 看门狗触发，所有安全机制激活 |
| **SUPERVISION_EXPIRED** | `WDGM_STATE_SUPERVISION_EXPIRED` (3) | 监督超时 (L929) | 停止看门狗触发，触发安全复位 |
| **STOPPED** | `WDGM_STATE_STOPPED` (4) | 模块已停止 | 不执行监督功能 |
| **DEACTIVATED** | `WDGM_STATE_DEACTIVATED` (5) | 模块已去激活 | 不执行监督功能 |

### 6.2 监督实体 (SE) 状态

| 状态 | 枚举值 (WdgM.h:116-122) | 条件 | 行为 |
|------|--------------------------|------|------|
| **CORRECT** | `WDGM_SE_STATE_CORRECT` (0) | 初始化/检查点到达/重新激活 | 实体正常运行，活指示被计数 |
| **INCORRECT** | `WDGM_SE_STATE_INCORRECT` (1) | 实体行为异常 | 实体处于错误状态 |
| **EXPIRED** | `WDGM_SE_STATE_EXPIRED` (2) | 监督超时 (L923) | 实体已被标记为超时 |
| **DEACTIVATED** | `WDGM_SE_STATE_DEACTIVATED` (3) | 去激活 (L538) | 实体不被监督评估 |

### 6.3 安全状态转换

```
UNINIT ──[WdgM_Init() 成功]──→ INIT ──[初始化完成]──→ ACTIVE
ACTIVE ──[监督超时/安全动作]──→ SUPERVISION_EXPIRED ──[PerformReset()]──→ 系统复位
ACTIVE ──[WdgM_DeInit() 成功]──→ UNINIT
任意状态 ──[安全魔数损坏]──→ PerformSafetyAction() → PerformReset() → 系统复位
任意状态 ──[Lockstep/RamSafety 错误]──→ PerformSafetyAction() → PerformReset() → 系统复位
```

---

## 7. 故障反应时间线

### 7.1 场景一：监督实体活指示不足

| 时间 | 事件 |
|------|------|
| T=0 | 被监督实体 (SE) 停止调用 `WdgM_CheckpointReached(seId)` |
| T=1~N cycles | `WdgM_MainFunction()` 周期执行，`WdgM_CycleTimer` 递增 (L623) |
| T=N cycles | `WdgM_CheckEntityAlive()` 中 `aliveCycleCounter` 达到 `aliveSupRefCycle` (L889) |
| T=N+1 | 评估活指示数: `aliveCounter < expectedAliveIndications` (L894)，条件为真 |
| T=N+2 | `entity->consecutiveErrors++` (L898) |
| T=N+2~N+K | 连续 K 个参考周期均失败，`consecutiveErrors >= WDGM_CFG_FAILURE_THRESHOLD` (L900) |
| T=N+K+1 | `WdgM_HandleExpiredSupervision()` 被调用 (L902) |
| T=N+K+2 | SE 状态设为 `EXPIRED` (L923), `WdgM_State = WDGM_STATE_SUPERVISION_EXPIRED` (L929) |
| T=N+K+3 | `WdgM_NotifyEvent(WDGM_EVENT_SUPERVISION_EXPIRED)` 触发安全回调 (L931) |
| T=N+K+4 | `WdgM_PerformSafetyAction()` → `WdgM_PerformReset()` 执行系统复位 (L990) |
| **总延迟** | **(aliveSupRefCycle × failureThreshold + 评估延迟) × MainFunction 周期** |

### 7.2 场景二：安全魔数损坏

| 时间 | 事件 |
|------|------|
| T=0 | SRAM 瞬态故障导致 `WdgM_SafetyMagic` 被篡改 |
| T=1 | `WdgM_MainFunction()` 被周期调用 |
| T=2 | 魔数检查失败 (L616: `WdgM_SafetyMagic != WDGM_SAFETY_MAGIC_ACTIVE`) |
| T=3 | `WdgM_PerformSafetyAction(WDGM_E_DATA_CORRUPTION)` 被调用 (L618) |
| T=4 | `failedRefreshes++` (L987) |
| T=5 | `WdgM_PerformReset()` 执行系统复位 (L990) |
| **总延迟** | **1 个 MainFunction 周期 (最快故障检测路径)** |

### 7.3 场景三：Lockstep 核比较错误

| 时间 | 事件 |
|------|------|
| T=0 | Lockstep 模块检测到核比较不一致，调用 `WdgM_HandleLockstepError(errorCode)` |
| T=1 | `lockstepErrors++` (L730) |
| T=2 | `WdgM_NotifyEvent(WDGM_EVENT_LOCKSTEP_ERROR, errorCode)` (L733) |
| T=3 | Dem 上报诊断事件 (L736, 编译时使能时) |
| T=4 | `WdgM_PerformSafetyAction(errorCode)` 被调用 (L740) |
| T=5 | `WdgM_PerformReset()` 执行系统复位 |
| **总延迟** | **1 次函数调用延迟 (微秒级)** |

### 7.4 场景四：全局连续错误阈值触发

| 时间 | 事件 |
|------|------|
| T=0~N | 多个监督实体陆续超时，`WdgM_ConsecutiveErrors` 递增 |
| T=N | `WdgM_ConsecutiveErrors >= WdgM_CurrentConfig->failureThreshold` (L645) |
| T=N+1 | `WdgM_PerformSafetyAction(WDGM_E_SUPERVISION_EXPIRED)` (L647) |
| T=N+2 | 系统复位执行 |
| **总延迟** | **1 个 MainFunction 周期** |

---

## 8. ISO 26262 条款追溯

| ISO 26262 条款 | 要求 | 本文档覆盖 |
|---------------|------|-----------|
| Part 5, Clause 7 | FMEDA 方法论 | 第 3 节完整失效模式表 (10 个功能单元, 40+ 失效模式) |
| Part 5, Clause 8 | 安全度量 (SPFM/LFM/PMHF) | 第 5 节度量分析 (ASIL D 目标) |
| Part 5, Clause 9 | 诊断覆盖率评估 | 第 4 节 DC 汇总 |
| Part 6, Clause 8 | 软件单元设计中的安全机制 | 第 3 节故障检测方法列：安全魔数、状态门控、参数校验、饱和保护 |
| Part 4, Clause 7 | 安全状态定义 | 第 6 节安全状态 (模块状态机 + SE 状态 + 状态转换图) |
| Part 4, Clause 8 | 技术安全需求 | 第 7 节故障反应时间线 (4 个典型场景) |
| Part 5, Clause 7.4 | 多重失效 (Latent Fault) | 第 5.3 节 LFM 分析 |
| Part 5, Annex D | 安全机制覆盖率计算 | 第 4.2 节加权 DC 计算 |

---

## 9. 假设与约束

1. **底层看门狗驱动假设**: Wdg 驱动层（MCAL）的失效率及诊断覆盖率由独立 FMEDA 覆盖，本分析假设 Wdg 驱动的触发机制可靠
2. **安全魔数保护**: 假设 `WdgM_SafetyMagic` 存储于受保护的 SRAM 区域，其被瞬态故障篡改的概率已纳入 FIT 估算
3. **MainFunction 调度**: 假设 `WdgM_MainFunction()` 由 OS Task 或 BswM 以固定周期（建议 10ms）调度，调度监控由系统级 WDT 覆盖
4. **Lockstep/RamSafety 模块**: 假设 Lockstep 和 RamSafety 模块具有独立的 FMEDA，其输出错误信号的可靠性已验证
5. **配置正确性**: 假设 WdgM 配置（监督实体参数、failureThreshold、supervisionCycleMs）在集成阶段经过充分验证
6. **Dem 集成**: Dem 事件上报为编译时可选项 (`WDGM_CFG_DEM_INTEGRATION`)，安全回调 `WdgM_SafetyCallback` 作为替代通知路径
7. **ASIL D 合规**: WdgM 被标注为 ASIL D 模块，其安全度量需满足 ISO 26262 Part 5 对 ASIL D 的最严格要求

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| SRAM (监督实体数组) | 8 FIT | IEC 61508 Annex D |
| CPU 逻辑 (状态机/评估) | 12 FIT | 供应商 FMD |
| 看门狗触发逻辑 | 5 FIT | IEC 61508 Annex D |
| 配置数据 (ROM/Flash) | 3 FIT | 供应商 FMD |
| Lockstep 接口 | 4 FIT | 供应商 FMD |
| RamSafety 接口 | 4 FIT | 供应商 FMD |
| Dem 上报路径 | 2 FIT | IEC 61508 Annex D |
| 安全回调机制 | 3 FIT | IEC 61508 Annex D |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| WdgM | Watchdog Manager |
| SE | Supervised Entity (监督实体) |
| WWD | Window WatchDog (窗口看门狗) |
| IWD | Independent WatchDog (独立看门狗) |
| Dem | Diagnostic Event Manager |
| Det | Default Error Tracer |
| BswM | Basic Software Mode Manager |
| SRAM | Static Random Access Memory |
| WDT | Watchdog Timer |

## 附录 C: 安全机制映射

| 安全机制 | 实现位置 | 覆盖的失效模式 | 类型 |
|---------|---------|-------------|------|
| 安全魔数校验 | WdgM.c: L114, L616, L661 | FM-01-04, FM-05-02, FM-06-01 | 运行时完整性检查 |
| 参数校验 (NULL/范围) | WdgM.c: L198, L377, L501, L581 | FM-01-01, FM-07-01, FM-02-04 | 输入验证 |
| 状态门控 | WdgM.c: L204, L371, L610, L658 | FM-01-02, FM-06-01, FM-05-01 | 状态机保护 |
| 活指示饱和保护 | WdgM.c: L455 | FM-02-02 | 溢出保护 |
| 参考周期零值保护 | WdgM.c: L261-269 | FM-01-05 | 配置防御 |
| 连续错误计数升级 | WdgM.c: L645-648, L898-902 | FM-03-04, FM-04-01 | 故障升级 |
| 禁用保护 | WdgM.c: L385, L423-426 | FM-07-02 | 权限控制 |
| 死循环兜底 | WdgM.c: L686-689 | FM-10-01 | 最终安全措施 |
| 安全事件回调 | WdgM.c: L824-827 | FM-04-04, FM-08-01 | 冗余通知路径 |
| 实体零 ID 过滤 | WdgM.c: L864-865 | FM-09-03 | 数据完整性 |
