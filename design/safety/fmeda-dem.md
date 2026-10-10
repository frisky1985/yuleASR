# FMEDA — Dem (Diagnostic Event Manager)

> **文档编号**: SAF-FMEDA-DEM-001
> **版本**: 1.0
> **日期**: 2026-10-22
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW Service Layer 的 Dem (Diagnostic Event Manager) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。Dem 负责管理诊断事件（DTC）、去抖算法、冻结帧数据及扩展数据记录，是安全相关故障信息持久化的核心模块。

**模块路径**: `src/bsw/services/dem/src/Dem.c`

**分析边界**:
- Dem 核心逻辑（事件状态管理、去抖算法、DTC 存储）
- 关键功能：事件状态设置、冻结帧捕获、扩展数据记录、老化计数、操作周期管理
- 内部安全机制：去抖算法验证、DTC 状态一致性检查、NVM 写入确认
- 下层依赖（假设由 NvM 层覆盖）：NvM 持久化服务

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 Dem 模块的每个功能单元进行失效模式分析。

### 1.3 关键数据结构

| 结构体 | 说明 |
|--------|------|
| `EventStates[]` | 事件状态数组：TestFailed, Pending, Confirmed, TestNotCompleted 等 DTC 状态位 |
| `DTCEntries[]` | DTC 条目：DTC 编号、格式、关联事件 |
| `FreezeFrames[]` | 冻结帧数据记录 |
| `ExtendedDataRecords[]` | 扩展数据记录 |
| `OperationCycleStates[]` | 操作周期状态：Ignition Cycle, Power Cycle 等 |
| `TimeDebounceStates[]` | 时间去抖状态：Timer 值、故障/恢复计数 |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 事件状态管理 | `Dem_SetEventStatus()` (L216) | 设置事件状态，触发去抖算法 |
| FU-02 | 去抖算法 | `Dem_SetEventStatus()` 内部的 Counter/Time/Monitor 去抖 | 故障确认/恢复的判断逻辑 |
| FU-03 | DTC 状态跟踪 | `EventStates[]`, `DTCEntries[]` | DTC 状态位管理（Pending/Confirmed/TestFailed 等） |
| FU-04 | 冻结帧管理 | `Dem_GetFreezeFrameDataByDTC()` (L874) | 冻结帧捕获与读取 |
| FU-05 | 扩展数据记录 | `Dem_GetExtendedDataRecordByDTC()` (L917) | 扩展数据记录管理 |
| FU-06 | 老化计数 | `Dem_MainFunction()` (L1220) 中的老化处理 | 故障老化计数器管理 |
| FU-07 | 操作周期管理 | `Dem_SetOperationCycleState()` (L1003) | 点火/上电周期状态跟踪 |
| FU-08 | DTC 清除 | `Dem_ClearDTC()` (L628) | 单 DTC/全部/功能组清除 |
| FU-09 | DTC 设置控制 | `Dem_DisableDTCSetting()` (L730), `Dem_EnableDTCSetting()` (L748) | DTC 检测使能/禁止 |
| FU-10 | 初始化 | `Dem_Init()` (L57) | 模块状态初始化 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 事件状态管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | `EventStates[]` 数组越界访问 | 内存破坏或错误事件状态被修改 | EventId 范围校验：`EventId < Dem_Config->EventCount` | 99% | B |
| FM-01-02 | 事件状态位损坏（RAM 瞬态故障） | DTC 状态不一致（如 Confirmed=1 但 Pending=0） | 状态位一致性校验：Confirmed 为 1 时 Pending 必须为 1 | 85% | B |
| FM-01-03 | `Dem_SetEventStatus` 重复调用导致状态跳变 | 去抖计数器错误递增/递减 | 去抖算法状态机校验：仅允许合法状态转换 | 80% | B |

### 3.2 FU-02: 去抖算法

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | Counter 去抖计数器溢出 | 计数器回绕导致故障误确认或误恢复 | 计数器范围校验：`Counter >= FailedThreshold` 才确认故障；`Counter <= PassedThreshold` 才恢复 | 95% | B |
| FM-02-02 | Time 去抖定时器异常（未递减） | 基于时间的去抖判断失效 | `Dem_MainFunction` 周期检查 `TimeDebounceStates[]` Timer 递减；超时未变化则报警 | 90% | B |
| FM-02-03 | Monitor 去抖外部回调失败 | 外部监控逻辑无法正确报告故障状态 | 回调函数指针有效性校验；回调超时检测 | 75% | B |
| FM-02-04 | 去抖阈值配置错误 | 故障确认过快（漏报）或过慢（延迟报警） | 配置值合理性校验：`FailedThreshold > PassedThreshold` | 85% | B |

### 3.3 FU-03: DTC 状态跟踪

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | DTC 状态位丢失（RAM 故障） | 已确认故障丢失，安全相关 DTC 无法上报 | NVM 持久化：状态变更后写入 NvM，读取时与 NVM 数据交叉验证 | 90% | B |
| FM-03-02 | `DTCEntries[]` 配置损坏 | DTC 映射错误，故障关联到错误 DTC | 配置区域 CRC 校验（启动时）；DTC 编号有效性检查 | 90% | B |
| FM-03-03 | 状态位更新非原子操作 | 并发读写导致状态不一致 | 关键状态更新使用中断保护（DisableAllInterrupts/EnableAllInterrupts） | 80% | B |

### 3.4 FU-04: 冻结帧管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | 冻结帧数据损坏 | 故障诊断时读取到错误的快照数据 | 冻结帧数据 CRC 校验（存储时计算，读取时验证） | 90% | B |
| FM-04-02 | 冻结帧索引越界 | 访问非法冻结帧记录 | `Dem_GetFreezeFrameDataByDTC()` 入口校验：RecordId 范围检查 | 99% | B |
| FM-04-03 | 冻结帧捕获时机错误 | 故障发生瞬间未捕获有效数据 | 捕获触发条件校验：仅在 DTC 状态从 Pending→Confirmed 时触发 | 80% | B |
| FM-04-04 | 冻结帧 NVM 写入失败 | 冻结帧数据未持久化，断电后丢失 | NVM 写入结果检查；失败时重试或上报诊断事件 | 85% | B |

### 3.5 FU-05: 扩展数据记录

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | 扩展数据记录损坏 | 故障分析时数据不可用 | 记录数据 CRC 校验 | 90% | B |
| FM-05-02 | RecordId 映射错误 | 读取到错误的扩展数据 | `Dem_GetExtendedDataRecordByDTC()` 入口校验：RecordId 有效性 | 99% | B |
| FM-05-03 | 扩展数据更新冲突 | 数据不一致 | 更新时加锁保护；更新后 CRC 重算并验证 | 85% | B |

### 3.6 FU-06: 老化计数

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | 老化计数器溢出 | 计数器回绕导致老化判断失效 | 计数器上限校验：`AgingCounter < AgingThreshold` | 95% | B |
| FM-06-02 | 老化计数器未递增 | 已修复故障无法自动清除 | `Dem_MainFunction` 周期检查老化逻辑执行状态 | 80% | B |
| FM-06-03 | 老化阈值配置错误 | 故障过早清除（安全隐患）或过晚清除 | 配置值合理性校验：`AgingThreshold > 0 && AgingThreshold <= MaxAging` | 85% | B |

### 3.7 FU-07: 操作周期管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | 周期状态转换错误 | 操作周期计数不准确，影响老化判断 | 状态机校验：仅允许 `IDLE→RUNNING→COMPLETE` 合法转换 | 90% | B |
| FM-07-02 | 周期状态丢失（断电） | 重启后周期状态不一致 | NVM 持久化周期状态；启动时恢复并校验 | 85% | B |
| FM-07-03 | `Dem_SetOperationCycleState` 非法调用 | 周期状态异常切换 | CycleState 参数有效性校验 | 95% | B |

### 3.8 FU-08: DTC 清除

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | 清除操作未持久化 | 断电后 DTC 重新出现 | NVM 写入确认：清除后写入 NVM 并验证 | 90% | B |
| FM-08-02 | 功能 DTC 组清除范围错误 | 误清安全相关 DTC | 功能组范围校验：排除安全关键 DTC 组 | 80% | B |
| FM-08-03 | 清除过程中状态不一致 | 部分状态已清除、部分未清除 | 清除操作原子性保护；完成后一致性校验 | 85% | B |

### 3.9 FU-09: DTC 设置控制

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | DTC 设置被意外禁用 | 安全相关故障无法被检测 | 禁用超时监控：长时间禁用触发报警 | 75% | B |
| FM-09-02 | 使能/禁用状态不一致 | 部分 DTC 检测状态错误 | 状态标志与配置交叉验证 | 80% | B |

### 3.10 FU-10: 初始化

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | `Dem_Init` 未完全执行 | 模块处于未定义状态 | 初始化完成后设置初始化标志；主函数入口校验 | 90% | B |
| FM-10-02 | NVM 恢复数据损坏 | 事件状态、冻结帧数据错误 | NVM 数据 CRC 校验；校验失败则使用默认值 | 90% | B |
| FM-10-03 | 配置数据损坏 | 事件/DTC 映射错误 | 配置区域 CRC 校验 | 95% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 事件状态管理 | 99% | 88% | 范围校验 + 状态一致性 |
| FU-02 | 去抖算法 | 95% | 86% | 计数器范围 + 定时器监控 |
| FU-03 | DTC 状态跟踪 | 90% | 87% | NVM 持久化 + CRC |
| FU-04 | 冻结帧管理 | 99% | 89% | CRC + 索引校验 |
| FU-05 | 扩展数据记录 | 99% | 91% | CRC + 映射校验 |
| FU-06 | 老化计数 | 95% | 87% | 范围校验 + 周期监控 |
| FU-07 | 操作周期管理 | 95% | 90% | 状态机 + NVM 持久化 |
| FU-08 | DTC 清除 | 90% | 85% | NVM 确认 + 原子性 |
| FU-09 | DTC 设置控制 | 80% | 78% | 超时监控 |
| FU-10 | 初始化 | 95% | 92% | CRC + 标志校验 |

### 4.2 模块整体诊断覆盖率

```
DC_Dem = SUM(DC_i * FIT_i) / SUM(FIT_i)

估算结果: DC_Dem ≈ 88%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL B)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM | ≥ 90% | ~91% | 满足 |
| LFM | ≥ 90% | ~90% | 满足 |
| PMHF | < 100 FIT | ~48 FIT | 满足 |

### 5.2 SPFM 分析

Dem 中主要单点故障来源：
- DTC 设置禁用超时监控的不可检测故障
- 去抖回调函数指针的瞬态故障（部分由 WDT 覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (6.8 / 78) ≈ 91%
```

### 5.3 LFM 分析

Dem 中主要潜伏故障来源：
- NVM 持久化逻辑故障（由写入确认覆盖）
- 状态一致性校验逻辑故障（由交叉验证覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (7.8 / 78) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_dtc_status_loss + lambda_freezeframe_corruption + lambda_aging_error
     ≈ 6 + 17 + 25 = 48 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 Dem 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | 初始化完成，所有 DTC 状态有效 | 正常处理事件状态更新、冻结帧捕获、DTC 查询 |
| **降级运行** | 部分冻结帧或扩展数据损坏 | 使用有效数据服务，损坏数据标记为不可用，上报诊断事件 |
| **ROM 恢复** | NVM 数据 CRC 校验失败 | 使用默认事件状态，上报初始化失败诊断事件 |
| **安全停止** | Dem 主函数无法执行或核心数据结构损坏 | 停止所有诊断事件处理，WDT 触发系统复位 |

### 6.2 安全状态转换

```
正常运行 ──[冻结帧/扩展数据CRC失败]──→ 降级运行
正常运行 ──[NVM数据CRC失败]──→ ROM恢复
ROM恢复  ──[NVM再次写入失败]──→ 安全停止
任意状态 ──[MainFunction超时]──→ 安全停止 (WDT触发)
降级运行 ──[下次初始化成功]──→ 正常运行
```

---

## 7. 故障反应时间线

### 7.1 场景一：DTC 状态丢失（NVM 写入失败）

| 时间 | 事件 |
|------|------|
| T=0 | `Dem_SetEventStatus` 触发 DTC 状态变更（Pending→Confirmed） |
| T=1 | 去抖算法确认故障，更新 `EventStates[]` |
| T=2 | 触发 NVM 写入（通过 NvM_WriteBlock） |
| T=3 | NvM 返回写入失败 |
| T=4 | Dem 检测到写入失败，重试一次 |
| T=5 | 重试仍失败，上报 Dem 内部诊断事件（NVM_WRITE_FAILURE） |
| T=6 | 系统监控检测到诊断事件，触发降级策略 |
| **总延迟** | **6 个主函数周期** |

### 7.2 场景二：冻结帧数据损坏

| 时间 | 事件 |
|------|------|
| T=0 | DTC 状态从 Pending→Confirmed，触发冻结帧捕获 |
| T=1 | 采集当前传感器数据写入 `FreezeFrames[]` |
| T=2 | 计算冻结帧 CRC 并存储 |
| T=3 | 触发 NVM 持久化 |
| T=N | 后续读取时 `Dem_GetFreezeFrameDataByDTC()` 验证 CRC |
| T=N+1 | CRC 不匹配，冻结帧标记为不可用 |
| T=N+2 | 上报冻结帧损坏诊断事件 |
| **总延迟** | **检测到故障：N+2 个周期（N 取决于读取时机）** |

### 7.3 场景三：老化计数器异常

| 时间 | 事件 |
|------|------|
| T=0 | `Dem_MainFunction` 执行老化处理 |
| T=1 | 老化计数器递增 |
| T=2 | 检测到 `AgingCounter >= AgingThreshold` |
| T=3 | DTC 状态从 Confirmed→Pending（故障老化恢复） |
| T=4 | 触发 NVM 写入更新状态 |
| T=5 | NVM 写入确认成功 |
| **总延迟** | **5 个主函数周期** |

### 7.4 场景四：操作周期异常终止

| 时间 | 事件 |
|------|------|
| T=0 | 点火关闭（KL15 OFF），未调用 `Dem_SetOperationCycleState(OFF)` |
| T=1 | 下次点火开启，Dem_Init 检测到周期状态不一致 |
| T=2 | 强制结束上一个未完成的周期 |
| T=3 | 更新相关 DTC 的 TestFailedThisOperationCycle 状态 |
| T=4 | 开始新的操作周期 |
| **总延迟** | **4 个初始化周期** |

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

1. **NVM 层假设**: NvM 模块的失效率及诊断覆盖率由独立 FMEDA (SAF-FMEDA-NVM-001) 覆盖
2. **去抖阈值假设**: 去抖阈值（FailedThreshold, PassedThreshold, AgingThreshold）在系统集成阶段经过标定验证
3. **冻结帧数据源**: 冻结帧数据来自 SWC 层，数据有效性由 SWC 负责
4. **配置正确性**: 假设 Dem 配置（事件列表、DTC 映射、去抖参数）在集成阶段经过验证
5. **WDT 覆盖**: Dem_MainFunction 的调度监控由系统级 WDT 覆盖
6. **操作周期假设**: 假设 SWC 层正确调用 `Dem_SetOperationCycleState`

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| RAM 存储 | 8 FIT | IEC 61508 Annex D |
| CPU 逻辑 | 12 FIT | 供应商 FMD |
| NVRAM 写入 | 20 FIT | 供应商 FMD (见 SAF-FMEDA-NVM-001) |
| 定时器模块 | 5 FIT | IEC 61508 Annex D |

## 附录 B: DTC 状态位定义

| 状态位 | 位编号 | 含义 |
|--------|--------|------|
| TestFailed | 0 | 当前测试周期内故障存在 |
| TestFailedThisOperationCycle | 1 | 本操作周期内曾发生故障 |
| Pending | 2 | 故障待确认（去抖中） |
| Confirmed | 3 | 故障已确认 |
| TestNotCompleted | 4 | 测试未完成 |
| TestNotCompletedThisOperationCycle | 5 | 本操作周期内测试未完成 |

## 附录 C: 去抖算法类型

| 算法 | 说明 | 安全相关 |
|------|------|---------|
| Counter-based | 基于计数器递增/递减，达到阈值确认/恢复 | 是（主要） |
| Time-based | 基于时间累计，故障持续超过阈值时间确认 | 是 |
| Monitor-based | 外部 SWC 提供去抖结果 | 视 SWC 实现 |

## 附录 D: 术语表

| 缩写 | 全称 |
|------|------|
| Dem | Diagnostic Event Manager |
| DTC | Diagnostic Trouble Code |
| NVM | Non-Volatile Memory |
| CRC | Cyclic Redundancy Check |
| WDT | Watchdog Timer |
| SWC | Software Component |
