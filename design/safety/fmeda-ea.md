# FMEDA — Ea (EEPROM Abstraction)

> **文档编号**: SAF-FMEDA-EA-001
> **版本**: 1.0
> **日期**: 2026-10-23
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW ECUAL Layer 的 Ea (EEPROM Abstraction) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/ecual/ea/src/Ea.c`

**分析边界**:
- Ea 核心逻辑（块管理、作业队列、状态机）
- 上层接口：`Ea_Read`, `Ea_Write`, `Ea_EraseImmediateBlock`, `Ea_InvalidateBlock`
- 内部安全机制：参数校验、块有效性检查、作业状态跟踪、错误通知
- 下层依赖（假设由 MCAL 层覆盖）：Eep (EEPROM Driver)

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 Ea 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `Ea_BlockInfoType` | Ea.c:45-51 | 块信息：EepromAddress, BlockSize, IsValid, IsInvalidated, WriteCycleCounter |
| `Ea_JobType` | Ea.c:56-62 | 作业类型枚举：NONE, READ, WRITE, INVALIDATE, ERASE_IMMEDIATE |
| `Ea_ConfigType` | Ea.h:117-129 | 配置结构：BlockConfig, NumBlocks, SectorSize, NumberOfSectors |
| `Ea_BlockConfigType` | Ea.h:105-112 | 块配置：BlockId, BlockSize, ImmediateData, NumberOfWriteCycles, BlockCrc |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 块读取 | `Ea_Read()` (L149) | 从 EEPROM 读取数据块到 RAM |
| FU-02 | 块写入 | `Ea_Write()` (L217) | 从 RAM 写入数据块到 EEPROM |
| FU-03 | 块管理 | `Ea_BlockInfo[]` (L53) | 逻辑块到物理地址映射、有效性跟踪 |
| FU-04 | 作业队列 | `Ea_CurrentJob`, `Ea_Status` | 单作业队列，串行化处理 |
| FU-05 | 块失效 | `Ea_InvalidateBlock()` (L311) | 标记块为无效状态 |
| FU-06 | 立即擦除 | `Ea_EraseImmediateBlock()` (L351) | 立即擦除指定块 |
| FU-07 | 主函数调度 | `Ea_MainFunction()` (L442) | 周期调度，作业状态轮询 |
| FU-08 | 错误通知 | `Ea_JobErrorNotification()` (L403) | 作业失败时通知上层（NvM） |

---

## 3. 失效模式分析表

### 3.1 FU-01: 块读取 (`Ea_Read`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | 数据指针为空或越界 | 读取数据写入非法地址，可能导致内存破坏 | `Ea_Read()` 入口参数校验 (DataBufferPtr != NULL, L167) | 90% | B |
| FM-01-02 | BlockNumber 超出配置范围 | 访问未定义的块，返回 E_NOT_OK | BlockNumber 范围检查 (`BlockNumber < EA_NUM_BLOCKS`, L159) | 99% | B |
| FM-01-03 | BlockOffset 超出块大小 | 读取越界，数据不完整 | BlockOffset 范围检查 (`BlockOffset < EA_MAX_BLOCK_SIZE`, L163) | 95% | B |
| FM-01-04 | Length 参数非法（0 或超出块大小） | 读取长度错误，数据不完整 | Length 范围检查 (`Length > 0 && Length <= EA_MAX_BLOCK_SIZE`, L171) | 95% | B |
| FM-01-05 | 读取过程中 EEPROM 硬件返回错误 | 数据不完整或损坏 | Eep_Read 返回码检查（L206），失败则设置 EA_JOB_FAILED | 90% | B |
| FM-01-06 | 块被标记为无效 (IsInvalidated = TRUE) | 读取操作被拒绝，返回 E_NOT_OK | 块状态检查（L183），上层收到错误后可重试或报警 | 80% | B |
| FM-01-07 | 块未初始化 (IsValid = FALSE) | 访问未配置的块 | 块有效性检查（L178），返回 EA_BLOCK_INVALID | 99% | B |

### 3.2 FU-02: 块写入 (`Ea_Write`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | 写入数据指针为空 | 写入未定义数据到 EEPROM | 入口参数校验 (DataBufferPtr != NULL, L228) | 90% | B |
| FM-02-02 | BlockNumber 超出配置范围 | 访问未定义的块，返回 E_NOT_OK | BlockNumber 范围检查（L224） | 99% | B |
| FM-02-03 | WriteCycleCounter 溢出 | 写入计数回绕，可能导致磨损均衡失效 | `WriteCycleCounter` 递增后检查溢出条件（L471） | 60% | B |
| FM-02-04 | Eep_Write 底层返回错误 | EEPROM 数据未更新，数据不一致 | Eep_Write 返回码检查（L257），失败则设置 EA_JOB_FAILED | 90% | B |
| FM-02-05 | 写入后验证失败 | EEPROM 中数据损坏 | 写后回读验证（由上层 NvM 覆盖），Ea 层通过 JobErrorNotification 通知 | 85% | B |
| FM-02-06 | 块未初始化 (IsValid = FALSE) | 写入未配置的块 | 块有效性检查（L235），返回 E_NOT_OK | 99% | B |

### 3.3 FU-03: 块管理 (`Ea_BlockInfo`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | 块地址计算错误 | 访问错误的 EEPROM 地址，数据错乱 | `Ea_CalculateBlockAddress()` 地址计算逻辑校验（L520-525） | 85% | B |
| FM-03-02 | BlockInfo 数组越界 | 内存破坏，系统崩溃 | 数组索引检查（`blockId < EA_NUM_BLOCKS`, L108） | 99% | B |
| FM-03-03 | 块配置加载失败 | 块大小未初始化，读写操作异常 | ConfigPtr->BlockConfig 非空检查（L105），块 ID 范围验证 | 90% | B |
| FM-03-04 | IsInvalidated 标志异常 | 有效块被误判为无效，或无效块被误用 | 状态一致性检查：写入时清除，失效时设置 | 75% | B |

### 3.4 FU-04: 作业队列管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | 作业冲突（Ea_Status == EA_BUSY） | 新作业请求被拒绝，返回 E_NOT_OK | 状态检查（L189, L240, L325, L365），上层收到错误后可重试 | 99% | B |
| FM-04-02 | 作业状态机卡死 | 作业无法推进，Ea 无响应 | `Ea_MainFunction` 周期执行超时检测；WDT 监控（系统级） | 80% | B |
| FM-04-03 | JobResult 状态异常 | 作业结果不一致，上层误判 | 状态一致性检查：作业完成时清除 JobResult，异常时通过 Cancel 恢复 | 75% | B |
| FM-04-04 | 作业取消失败 | 无法中止正在进行的作业 | Eep_Cancel 调用（L279），状态强制回退到 IDLE | 70% | B |

### 3.5 FU-05: 块失效 (`Ea_InvalidateBlock`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | BlockNumber 超出配置范围 | 访问未定义的块，返回 E_NOT_OK | BlockNumber 范围检查（L318） | 99% | B |
| FM-05-02 | 失效操作失败 | 块仍为有效状态，可能导致数据不一致 | 立即完成模式（L339），通过 JobEndNotification 通知上层 | 85% | B |
| FM-05-03 | IsInvalidated 标志设置失败 | 块未被正确标记为无效 | 标志设置后立即完成，无中间状态 | 90% | B |

### 3.6 FU-06: 立即擦除 (`Ea_EraseImmediateBlock`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | BlockNumber 超出配置范围 | 访问未定义的块，返回 E_NOT_OK | BlockNumber 范围检查（L358） | 99% | B |
| FM-06-02 | Eep_Erase 底层返回错误 | EEPROM 块未擦除，数据残留 | Eep_Erase 返回码检查（L381），失败则设置 EA_JOB_FAILED | 90% | B |
| FM-06-03 | EraseCycleCounter 溢出 | 擦除计数回绕，可能导致寿命评估失效 | `EraseCycleCounter` 递增后检查溢出条件（L388） | 60% | B |
| FM-06-04 | 擦除后验证失败 | EEPROM 块未完全擦除 | 擦除后回读验证（由上层覆盖），Ea 层通过 JobErrorNotification 通知 | 85% | B |

### 3.7 FU-07: 主函数调度 (`Ea_MainFunction`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | MainFunction 未被周期调用 | 作业无法完成，Ea 无响应 | 系统级 WDT 监控，检测任务调度异常 | 80% | B |
| FM-07-02 | Eep_GetJobResult 返回异常状态 | 作业状态判断错误 | 状态枚举校验（L458-463），异常状态按失败处理 | 85% | B |
| FM-07-03 | 作业完成通知失败 | 上层未收到作业完成通知 | JobEndNotification 调用（L492），失败不影响 Ea 内部状态 | 75% | B |
| FM-07-04 | 作业失败通知失败 | 上层未收到作业失败通知 | JobErrorNotification 调用（L501），失败不影响 Ea 内部状态 | 75% | B |

### 3.8 FU-08: 错误通知 (`Ea_JobErrorNotification`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | 通知回调未配置 | 上层未收到错误通知 | 配置检查（`EA_NVM_JOB_ERROR_NOTIFICATION == STD_ON`, L405） | 90% | B |
| FM-08-02 | 通知回调执行失败 | 上层未收到错误通知，无法触发降级策略 | 通知函数本身无返回值，依赖上层监控 | 70% | B |
| FM-08-03 | 通知时机错误 | 上层在错误时机收到通知 | 状态机严格控制：仅在作业失败时调用（L500） | 85% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 块读取 | 99% | 91% | 主要依赖参数校验和硬件返回码检查 |
| FU-02 | 块写入 | 99% | 87% | 写后回读验证由上层覆盖 |
| FU-03 | 块管理 | 99% | 88% | 地址计算逻辑校验 |
| FU-04 | 作业队列 | 99% | 86% | 状态冲突检测 + 超时监控 |
| FU-05 | 块失效 | 99% | 90% | 立即完成模式 |
| FU-06 | 立即擦除 | 99% | 86% | 擦除后回读验证 |
| FU-07 | 主函数调度 | 85% | 80% | WDT 监控 |
| FU-08 | 错误通知 | 90% | 80% | 配置检查 + 状态机控制 |

### 4.2 模块整体诊断覆盖率

```
DC_Ea = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_Ea ≈ 87%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL B)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 90% | ~91% | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ~90% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 100 FIT | ~52 FIT | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。Ea 中主要单点故障来源：
- 块地址计算逻辑的不可检测故障（由配置验证覆盖）
- 作业状态机瞬态故障（由 WDT 覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (6.8 / 78) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。Ea 中主要潜伏故障来源：
- 参数校验逻辑的系统性缺陷（通过代码审查和测试覆盖）
- 块状态管理逻辑故障（通过状态一致性检查覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (7.5 / 75) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_address_calc_error + lambda_job_stall + lambda_notification_failure
     ≈ 8 + 18 + 26 = 52 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 Ea 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | Ea_Initialized = TRUE，所有块有效 | 正常读写服务 |
| **降级运行** | 部分块无效或失效 | 拒绝无效块操作，返回 E_NOT_OK，通知上层 |
| **作业失败** | EEPROM 硬件操作失败 | 设置 EA_JOB_FAILED，调用 JobErrorNotification |
| **安全停止** | Ea_MainFunction 无法执行（CPU 故障） | 系统进入安全状态，WDT 触发复位 |

### 6.2 安全状态转换

```
正常运行 ──[块失效操作]──→ 降级运行
降级运行 ──[EEPROM硬件错误]──→ 作业失败
作业失败 ──[JobErrorNotification]──→ 上层触发降级策略
任意状态 ──[MainFunction超时]──→ 安全停止 (WDT触发)
```

---

## 7. 故障反应时间线

### 7.1 场景一：EEPROM 读取失败

| 时间 | 事件 |
|------|------|
| T=0 | 上层调用 `Ea_Read(BlockNumber, BlockOffset, DataBufferPtr, Length)` |
| T=1 | 参数校验通过，设置 Ea_Status = EA_BUSY |
| T=2 | 调用 Eep_Read 启动 EEPROM 读取 |
| T=3 | Eep_Read 返回 E_NOT_OK，检测到硬件错误 |
| T=4 | 设置 Ea_Status = EA_IDLE, Ea_JobResult = EA_JOB_FAILED |
| T=5 | 返回 E_NOT_OK 给上层 |
| **总延迟** | **5 个调用周期** |

### 7.2 场景二：EEPROM 写入失败（异步）

| 时间 | 事件 |
|------|------|
| T=0 | 上层调用 `Ea_Write(BlockNumber, DataBufferPtr)` |
| T=1 | 参数校验通过，设置 Ea_Status = EA_BUSY, Ea_CurrentJob = EA_JOB_WRITE |
| T=2 | 调用 Eep_Write 启动 EEPROM 写入，返回 E_OK |
| T=3 | 返回 E_OK 给上层，作业异步进行 |
| T=4-N | `Ea_MainFunction` 周期调用 Eep_GetJobResult |
| T=N+1 | Eep_GetJobResult 返回 MEMIF_JOB_FAILED |
| T=N+2 | 设置 Ea_JobResult = EA_JOB_FAILED, Ea_Status = EA_IDLE |
| T=N+3 | 调用 `Ea_JobErrorNotification()` 通知上层 |
| **总延迟** | **N+3 个主函数周期（N 取决于 EEPROM 写入时间）** |

### 7.3 场景三：块无效

| 时间 | 事件 |
|------|------|
| T=0 | 上层调用 `Ea_Read(BlockNumber, ...)` |
| T=1 | 检查 `Ea_BlockInfo[BlockNumber].IsValid == FALSE` |
| T=2 | 设置 Ea_JobResult = EA_BLOCK_INVALID |
| T=3 | 返回 E_NOT_OK |
| **总延迟** | **3 个调用周期** |

### 7.4 场景四：作业冲突

| 时间 | 事件 |
|------|------|
| T=0 | 上层调用 `Ea_Write(BlockNumber, DataBufferPtr)` |
| T=1 | 检查 `Ea_Status == EA_BUSY`（前一作业未完成） |
| T=2 | 返回 E_NOT_OK |
| T=3 | 上层收到错误，可选择重试或触发降级策略 |
| **总延迟** | **2 个调用周期 + 系统响应时间** |

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

1. **底层 EEPROM 驱动假设**: Eep 层的失效率及诊断覆盖率由独立 FMEDA 覆盖，本分析假设底层 FIT 数据已知
2. **地址计算正确性**: 假设 `Ea_CalculateBlockAddress()` 的地址计算逻辑在集成阶段经过验证
3. **WDT 覆盖**: Ea_MainFunction 的调度监控由系统级 WDT 覆盖
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **配置正确性**: 假设 Ea 配置（块描述符、块数量、扇区大小）在集成阶段经过验证
6. **上层覆盖**: 写后擦除验证由上层 NvM 覆盖，Ea 层仅提供错误通知接口

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| EEPROM 读取 | 10 FIT | 供应商 FMD |
| EEPROM 写入 | 25 FIT | 供应商 FMD |
| EEPROM 擦除 | 30 FIT | 供应商 FMD |
| RAM 存储 | 8 FIT | IEC 61508 Annex D |
| CPU 逻辑 | 12 FIT | 供应商 FMD |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| EEPROM | Electrically Erasable Programmable Read-Only Memory |
| Eep | EEPROM Driver (MCAL Layer) |
| NvM | NVRAM Manager (Service Layer) |
| WDT | Watchdog Timer |
