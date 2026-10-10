# yuleASR — Safety Manual (ISO 26262:2018)

> **文档**: Safety Manual (安全手册)
> **版本**: 1.1 | **日期**: 2026-10-09
> **状态**: 正式发布
> **ASIL 目标**: ASIL B (混合 ASIL B + QM)
> **平台**: NXP S32K312 (ARM Cortex-M4F, 80MHz)
> **参考标准**: ISO 26262:2018 (Parts 1-12)

---

## 0 文档控制

### 0.1 修订历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0 | 2026-07-30 | 初始版本 |
| 1.1 | 2026-10-09 | 修正 ASIL 等级为 ASIL-B (与 HARA/Safety Case 对齐); 补充安全偏差、DFA、FMEDA 引用 |

### 0.2 参考文档

| 编号 | 文档 | 路径 |
|:----:|------|:-----|
| [SM-01] | HARA 分析 | docs/safety/HARA_ANALYSIS.md |
| [SM-02] | 安全架构 | docs/safety/safety-architecture.md |
| [SM-03] | DFA 分析 | docs/safety/dfa-analysis.md |
| [SM-04] | FMEA/FTA 量化分析 | docs/safety/fmea-fta-quantitative-analysis.md |
| [SM-05] | Safety Case (GSN) | docs/safety/safety-case.md |
| [SM-06] | 验证报告 | docs/safety/VERIFICATION_REPORT.md |
| [SM-07] | ASIL 分解报告 | docs/safety/asil-decomposition-report.md |
| [SM-08] | MPU 分区方案 | docs/safety/mpu-partition.md |

---

## 1 范围与目标

### 1.1 项目概述

yuleASR 是基于 AUTOSAR Classic Platform R21-11 的基础软件平台，部署于 NXP S32K312 (Cortex-M4F, 80MHz)。目标应用为车身/舒适系统，安全等级为 **ASIL B**。

### 1.2 安全目标

| 安全目标 | 描述 | ASIL | 分解策略 |
|:---------|:-----|:----:|:---------|
| SG-001 | 通信数据完整性 | B | B(B) + QM(B) |
| SG-002 | 任务定时保证 | B | B(B) + QM(B) |
| SG-003 | 数据持久化可靠性 | A | 不分解，直接 ASIL A |

### 1.3 安全度量目标 (ISO 26262-5 §9.4)

| 度量 | ASIL B 目标 | 实测值 | 状态 |
|:-----|:-----------:|:------:|:----:|
| SPFM (单点故障度量) | ≥ 90% | 93.0% | ✅ |
| LFM (潜在故障度量) | ≥ 60% | 88.3% | ✅ |
| PMHF (概率度量) | < 100 FIT | 20.2 FIT | ✅ |

---

## 2 安全关键模块

### 2.1 ASIL-B 模块清单

| 模块 | 安全机制 | FMEDA DC | 安全需求 |
|:-----|:---------|:--------:|:---------|
| E2E | CRC + Counter + DataID 端到端保护 | 90% | SG-001 |
| WdgM | 看门狗监控 + 时间窗口检查 | 85% | SG-002 |
| NvM | 冗余存储 + CRC 校验 + 写计数器 | 91% | SG-003 |
| SecOC | MAC 认证 + Freshness 计数器 | 88% | SG-001 |
| CryIf | HSM 抽象 + 密钥管理 | 85% | SG-001 |
| RamSafety | RAM 安全测试 (March C-) | 90% | SG-002 |
| OS (Timing) | 执行/锁定/间隔时间监控 | 95% | SG-002 |

### 2.2 QM 模块 (通过 E2E 保护)

Com, PduR, Can, CanIf, Lin, LinIf, SPI, Eth 等非安全模块通过 E2E Profile 1/4 端到端审计保护，无需独立安全目标。

---

## 3 安全机制

### 3.1 数据完整性 (E2E + SecOC)

- **E2E Profile 1**: 32-bit CRC + 4-bit Counter + DataID，保护 PDU 数据
- **E2E Profile 4**: 32-bit CRC + 32-bit Counter，保护大帧数据
- **SecOC**: AES-128-CMAC 认证 + 32-bit Freshness 计数器，防止重放攻击
- **故障响应**: E2E 校验失败 → 静默抑制发送 → DTC 设置 → 驱动降级

### 3.2 时间监控 (OS Timing + WdgM)

- **执行时间监控**: 任务超出预算时间时触发报警
- **锁定时间监控**: 资源持有超时检测
- **间隔时间监控**: 任务激活过快检测
- **WdgM**: 窗口看门狗，检查 AliveSupervision 循环计数

### 3.3 数据存储安全 (NvM + Cdd_Fvm)

- **冗余存储**: 双 Bank 存储 (Primary + Failover)
- **CRC32 校验**: 每个 Block 独立 CRC
- **写计数器**: 磨损均衡 + 写入次数限制
- **Freshness 持久化**: SecOC 计数器通过 NvM Block 8 持久化 (48 bytes)

### 3.4 硬件安全机制

| 机制 | 覆盖 | DC | ASIL |
|:-----|:-----|:--:|:----:|
| ECC (Flash/RAM) | 存储完整性 | 95% | B |
| WDG (看门狗) | 程序流监控 | 90% | B |
| HSM (硬件安全模块) | 密码运算加速 | 85% | B |
| MPU (内存保护) | 区域隔离 | 80% | B |
| Lockstep (双核锁步) | 硬件故障检测 | 95% | B |

---

## 4 安全偏差与缓解

### 4.1 已接受偏差

| 偏差 ID | 描述 | 缓解措施 | 接受理由 |
|:--------|:-----|:---------|:---------|
| D-FFI-001 | 无 MPU/MMU 硬件隔离 | E2E 端到端保护 + WdgM 时间监控 | 软件级 FFI 缓解已验证 (DFA 93% 通过率) |
| D-FFI-002 | 无独立安全内核 | CryIf HSM 隔离 + OS 任务优先级隔离 | HSM 提供硬件级密钥保护 |
| D-MC/DC-001 | MC/DC 覆盖率边界 | 分支覆盖率 79.79% 替代 MC/DC | ASIL B 允许分支覆盖作为 MC/DC 近似 |

### 4.2 偏差评审记录

所有偏差已通过独立评审，记录于:
- DFA 报告 [SM-03 §6]
- Safety Case [SM-05 §5.3]
- ASIL 分解报告 [SM-07 §4]

---

## 5 开发流程

### 5.1 V-Model 工作产品

| 阶段 | ISO 26262 条款 | 工作产品 | 状态 |
|:-----|:--------------|:---------|:----:|
| 概念阶段 | Part 3 | HARA [SM-01], Safety Goal [SM-05] | ✅ |
| 产品开发 (硬件) | Part 5 | FMEDA [SM-02 §4], DFA [SM-03] | ✅ |
| 产品开发 (软件) | Part 6 | 代码实现, 单元测试 [SM-06] | ✅ |
| 验证 | Part 4, 6 | 覆盖率 [SM-06], 集成测试 | ✅ |
| 确认 | Part 7 | Safety Case [SM-05] | ✅ |
| 安全分析 | Part 9 | FMEA/FTA [SM-04], ASIL 分解 [SM-07] | ✅ |

### 5.2 安全生命周期活动

1. **危害分析与风险评估 (HARA)** — ISO 26262-3 §7
2. **安全目标定义** — ISO 26262-3 §8
3. **ASIL 分解** — ISO 26262-9 §5
4. **安全需求导出** — ISO 26262-4 §7
5. **安全设计** — ISO 26262-5 §8-10
6. **安全验证** — ISO 26262-6 §9
7. **安全确认** — ISO 26262-7 §8
8. **安全分析 (FMEDA/DFA)** — ISO 26262-9 §4-8

---

## 6 安全治理

### 6.1 安全计划

- 安全经理: 质量架构师
- 独立评审: 13 个 ASIL-B 模块审查记录 [R12]
- CI 自动化: L1/L2/L3 三级质量门禁 [R13]

### 6.2 配置管理

- 版本控制: Git (全量追溯)
- 基线管理: Release Tag + Change Log
- 变更控制: 所有安全相关变更需评审

### 6.3 工具认证

| 工具 | 用途 | 认证状态 |
|:-----|:-----|:---------|
| GCC 13 | 编译器 | 合格 (MISRA 验证) |
| lcov | 覆盖率测量 | 合格 (分支覆盖率验证) |
| CMake | 构建系统 | 合格 (确定性构建) |
| yuleosh MISRA | 静态分析 | 合格 (C:2023 规则集) |

---

## 7 结论

本安全手册定义了 yuleASR 平台的安全目标、安全机制、开发流程和安全治理框架。所有安全目标已通过 FMEDA 定量分析、DFA 相依失效分析和验证测试得到证明。三个已接受偏差均有充分的缓解措施和独立评审记录。

Safety Case [SM-05] 提供了完整的 GSN 论证结构，证明 yuleASR 对指定安全目标是充分安全的。
