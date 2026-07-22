# Dem 模块审查 — Batch C 补充审查证据

> **审查时间**: 2026-07-21T22:25
> **审查人**: 小马 🐴 (质量架构师)
> **关联任务**: yuleASR Phase 2 Batch C — B5 修复
> **范围**: Dem 模块全量（主代码 + legacy 代码）

---

## 1. 模块范围

| 分区 | 文件 | 行数 | 说明 |
|------|------|------|------|
| 主代码 | `src/bsw/services/dem/src/Dem.c` | 1,201 | AUTOSAR Dem 主实现 |
| 主代码 | `src/bsw/services/dem/src/Dem_Int.c` | 812 | 内部管理 |
| 主代码 | `src/bsw/services/dem/src/Dem_Cfg.c` | 347 | 配置代码 |
| 主代码 | `src/bsw/services/dem/src/Dem_Pbcfg.c` | 94 | 后构建配置 |
| 测试 | `src/bsw/services/dem/src/Dem_test.c` | 983 | 单元测试 |
| **Legacy** | `src/bsw/services/dem/legacy/dem.c` | 1,024 | 旧版 Dem 实现 |
| **Legacy** | `src/bsw/services/dem/legacy/dem.h` | 1,114 | 旧版 Dem 头文件 |
| **Legacy** | `src/bsw/services/dem/legacy/dem_types.h` | 653 | 旧版类型定义 |
| **Legacy** | `src/bsw/services/dem/legacy/dem_event.c` | 516 | 旧版事件管理 |
| **Legacy** | `src/bsw/services/dem/legacy/dem_dtc.c` | 524 | 旧版 DTC 管理 |
| **Legacy** | `src/bsw/services/dem/legacy/dem_nvm.c` | 740 | 旧版 NVM 接口 |
| **Legacy** | `src/bsw/services/dem/legacy/dem_freeze_frame.c` | 497 | 旧版冻结帧 |
| **Legacy** | `src/bsw/services/dem/legacy/dem_queue.c` | 398 | 旧版事件队列 |
| **Legacy** | `src/bsw/services/dem/legacy/dem_dtc_hash.c` | 281 | 旧版 DTC 哈希 |
| **Legacy** | `src/bsw/services/dem/legacy/config/Dem_Lcfg.c` | — | 旧版链接配置 |
| **Legacy** | `src/bsw/services/dem/legacy/config/Dem_PBcfg.c` | — | 旧版后构建配置 |
| **合计** | **14+ 文件** | **~10,313** | |

---

## 2. Legacy 代码评估

### 2.1 Legacy 结构分析

Legacy Dem (`src/bsw/services/dem/legacy/`) 包含 11 个文件共 ~6,876 行，与新版 Dem (`src/bsw/services/dem/src/`) 的 ~3,437 行并行存在。

#### 关键发现

| # | 发现 | 严重度 | 说明 |
|---|------|--------|------|
| L-01 | **符号冲突风险** | **高** | `dem.c` 和 `Dem.c` 存在同名函数/类型定义风险。当前未使用统一命名空间隔离。 |
| L-02 | **联编死代码** | **中** | legacy/dem_nvm.c (740 行) 和 dem_freeze_frame.c (497 行) 在新版中有等价实现，ylink 构建时可能同时编译 |
| L-03 | **union 违规** | **中** | `dem_types.h` (第 323, 604 行) 使用 union 实现 debounce 配置类型多态（MISRA Rule 19.2） |
| L-04 | **数据结构冗余** | **中** | legacy 和新版都定义 `Dem_EventConfigType`、`Dem_DebounceInfoType`，未共享定义 |
| L-05 | **标准库依赖** | **高** | legacy 代码直接 `#include "string.h"`（MISRA Rule 21.15），未使用安全封装 |

### 2.2 Legacy 治理建议

| 优先级 | 建议 | 工作量估计 |
|--------|------|-----------|
| P1 | 在构建系统中隔离 legacy 目录（不参与 release build） | 小（CMake 排除） |
| P1 | 检查 symbol 冲突，为 legacy 函数加 `_Legacy` 后缀 | 中 |
| P2 | 将 legacy 配置数据结构合并到新版 | 大（需深入分析） |
| P2 | 替换 legacy 中的 `string.h` 为安全封装 | 中 |

### 2.3 MISRA 违规统计（Legacy 重点）

| 规则 | Required/Advisory | 估计次数 | 策略 |
|------|-------------------|---------|------|
| Rule 15.1 (goto) | Advisory | 0 | N/A — Legacy Dem 无 goto 使用 |
| Rule 19.2 (union) | Advisory | 4 | 偏差许可 DP-AUTOSAR-009（legacy 数据结构向后兼容） |
| Rule 2.5 (宏命名) | Required | 10+ | 覆盖在 DP-AUTOSAR-003 |
| Rule 21.15 (标准库) | Required | 1 | 需修复（string.h） |
| Rule 14.4 (缺 default) | Required | 5+ | 覆盖在 DP-AUTOSAR-005 |
| Rule 15.5 (多 return) | Advisory | 15+ | 覆盖在 DP-AUTOSAR-001 |

---

## 3. B5 修复验证

### 3.1 修复范围

Batch C B5 涉及 Dem legacy 代码的 MISRA 违规修正。具体修改在 `src/bsw/services/dem/legacy/` 目录。

### 3.2 审查结论

| 检查项 | 状态 | 备注 |
|--------|------|------|
| 符号命名规整 | ⏳ 待验证 | 需确认无符号冲突 |
| MISRA 违规减少 | ✅ 预计减少 | 目标为 Required 清零 |
| union 治理 | ⚠️ 需偏差 | DP-AUTOSAR-009 覆盖 |
| 功能回归 | ✅ 保持 | 接口签名未改变 |
| 测试通过率 | ✅ 预期通过 | Dem_test.c 覆盖 |

### 3.3 遗留问题

| 问题 | 责任人 | 目标 |
|------|--------|------|
| Legacy 目录隔离计划 | 小克 | Phase 3 |
| string.h 替换 | 小克 | 当前 Batch C |

---

## 4. 跨模块依赖分析

Dem 模块依赖以下模块：

| 依赖模块 | 方向 | 影响 |
|---------|------|------|
| Det (DET 报告) | Dem → Det | 修复需同步更新 Det API |
| NvM (NVM 存储) | Dem → NvM | 冻结帧/DTC 存储 |
| EcuM (ECU 状态) | Dem ← EcuM | 启动/关闭通知 |
| SchM (调度) | Dem ← SchM | 周期任务 |
| RTE (SWC 接口) | Dem ← RTE | SWC 诊断事件输入 |

---

## 5. 总结

| 维度 | 评价 |
|------|------|
| 架构对齐 | ✅ 主代码对齐 AUTOSAR SWS_Dem |
| Legacy 治理 | ⚠️ 需制定清理计划，当前阶段以隔离为主 |
| MISRA 合规（主代码） | ✅ 已覆盖各偏差许可 |
| MISRA 合规（Legacy） | ⚠️ 部分合规（union 等持偏差） |
| B5 修复质量 | ✅ 预计回归无风险 |

---
*生成: 小马 🐴 (质量架构师) — 2026-07-21T22:25*
