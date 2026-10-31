# yuleASR 量产准备计划 (Production Readiness Plan)

> 版本: v0.2 (PR-0.1 基线重建后)
> 生成日期: 2026-11-01
> 目标: 基于第一性原理的量产就绪评估 + 分层任务拆解 + 反幻觉纪律

---

## 0. 反幻觉纪律 (Anti-Hallucination Rules)

1. **所有数字必须来自脚本输出** — `reports/baseline/summary.json`、`module-matrix-summary.json`、`misra-gate.json`。禁止凭记忆或估计填写。
2. **未验证的事实标记为 `[PENDING]`** — 不允许用"大约""可能"等模糊词替代真实数据。
3. **决策必须有记录** — 每个 D-n 决策记录选项、选择、理由、遗留风险。
4. **风险必须显式登记** — 不可静默接受 (如 FreeRTOS 无 ISO 26262 认证)。
5. **门禁阈值来自 `.yuleosh/ci-config.yaml`** — 不重复定义，不硬编码。

---

## 1. 事实基线 (Facts Baseline)

> 数据来源: `tools/capture_baseline.sh` → `reports/baseline/summary.json` + `module-matrix-summary.json`
> 捕获日期: 2026-10-31, git_sha: 26ac7e6d (dirty)

| ID | 事实 | 值 | 来源 |
|:---|:-----|:---|:-----|
| F1 | 模块实现总数 (src/bsw 下) | 103 | module-matrix-summary.json total_rows |
| F2 | 唯一模块名数 (大小写不敏感) | 94 | module-matrix-summary.json unique_modules |
| F3 | 精确重复实现组数 | 6 (ramtst, fim, ipdum, j1939tp, memif, xcp) | duplicate_groups |
| F4 | 大小写冲突组数 | 3 (doIP/doip, ethSm/ethsm, linSM/linsm) | name_collision_groups |
| F5 | 配置模板总数 | 92 (mcal 20 / ecual 25 / services 46 / os 1) | templates 目录计数 |
| F6 | 无模板模块数 (NO_TEMPLATE) | 2 (Boot, Cdd) | status_counts |
| F7 | 模板层级不匹配数 (TEMPLATE_LAYER_MISMATCH) | 9 (重复模块, 延迟至 PR-1.3) | status_counts |
| F8 | 无测试模块数 (NO_TEST) | 18 | status_counts |
| F9 | 覆盖率为 0 的模块数 (COVERAGE_ZERO) | 12 | status_counts |
| F10 | 低覆盖率模块数 (COVERAGE_LOW, <60%) | 17 | status_counts |
| F11 | MISRA required 违规总数 | 58 | summary.json misra.required |
| F12 | MISRA total 违规总数 | 1546 (deviated 1488) | summary.json misra.total |
| F13 | 单元测试总数 / 失败数 | 192 / 2-3 (Safety_Stack_UnitTest, test_dds1_tbf, Safety_Manager_UnitTest; 后两者偶发) | summary.json ctest |
| F14 | 行覆盖率 / 分支覆盖率 | 63.54% / 50.99% (295 文件) | summary.json coverage |
| F15 | 无需求模块数 (NO_REQUIREMENT) | 87 | status_counts |

---

## 2. 决策寄存器 (Decision Register)

| ID | 决策 | 选项 | 选择 | 理由 | 遗留风险 |
|:---|:-----|:-----|:-----|:-----|:---------|
| D1 | 安全目标等级 | ASIL-A/B/C/D | **ASIL-D** | BSW 含制动/转向/动力相关服务 (Dem/Dcm/WdgM/E2E/SecOC)，按最高等级统一要求 | — |
| D2 | OS 路线 | FreeRTOS / 自研 / 商用 RTOS | **FreeRTOS** | 现有实现基于 FreeRTOS V10.6.x/V11.x，迁移成本最低 | **RISK-01**: FreeRTOS 无 ISO 26262 认证，需在安全案例中声明使用条件或改用认证版 |
| D3 | 模块集合 (裁剪 vs 全量) | 全量 97 模块 / 核心子集 | **[PENDING]** | 待 PR-1.3 定主后决定：9 个重复模块 (doIP/FiM/Xcp/EthSM/IpduM/J1939Tp/LinSM/MemIf/RamTst) 需选 canonical 实现 | RISK-02: 重复实现可能导致链接冲突 / 维护混乱 |

### 风险登记

- **RISK-01**: FreeRTOS 缺少 ISO 26262 功能安全认证。
  - 缓解: 安全案例中声明 FreeRTOS 仅用于非安全相关调度，或采购 SafeRTOS 认证版。
  - 状态: 开放，需安全经理批准。
- **RISK-02**: 9 个模块存在 ecual/services 双份实现。
  - 缓解: PR-1.3 选定 canonical，删除另一份。
  - 状态: 开放，PR-1.3 处理。

---

## 3. 分层任务拆解 (8 Layers)

### L0 — 基础设施 (Infrastructure)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L0-1 基线捕获脚本 | `bash tools/capture_baseline.sh --record-only` 退出码 0，产出 summary.json | P0 |
| L0-2 可复现性校验 | 两次运行 summary.json 除时间戳/git_sha 外完全一致 | P0 |
| L0-3 模块关联矩阵 | `python3 tools/generate_module_matrix.py` 产出 CSV + summary JSON | P0 |

### L1 — 构建与测试 (Build & Test)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L1-1 修复失败用例 | ctest fail=0 (当前 3 个失败: Safety_Stack_UnitTest, test_dds1_tbf, Safety_Manager_UnitTest) | P0 |
| L1-2 覆盖率门禁硬化 | `coverage.enabled=true`，line≥60% / branch≥50% 阻断 CI | P0 |
| L1-3 覆盖率提升 | line≥80% / branch≥60% (量产目标) | P1 |

### L2 — 静态分析 (MISRA)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L2-1 MISRA required 清零 | `misra.gate.required=0` (当前 58) | P0 |
| L2-2 通配偏差逐条复核 | `wildcard_deviations` 从 pending 转为逐条批准 | P1 |
| L2-3 MISRA total ratchet | total ≤ 1547 (只降不升) | P0 |

### L3 — 需求可追溯性 (Traceability)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L3-1 需求覆盖率 | 所有模块 requirement_count > 0 (当前 NO_REQUIREMENT 模块清零) | P0 |
| L3-2 人工审查 | human_reviewed_requirements == requirement_count | P1 |
| L3-3 导出 required MISRA 违规 CSV | `reports/misra-required-violations.csv` 含 rule/file/line/reason | P1 |

### L4 — 模块治理 (Module Governance)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L4-1 重复模块定主 | 9 个重复模块选定 canonical，删除另一份 | P0 |
| L4-2 模板层级对齐 | TEMPLATE_LAYER_MISMATCH=0 | P0 |
| L4-3 无模板补齐 | NO_TEMPLATE=0 | P1 |

### L5 — 安全机制 (Safety Mechanisms)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L5-1 E2E 保护覆盖 | Com 信号 E2E 保护率 100% (安全相关) | P1 |
| L5-2 SecOC 集成 | SecOC 与 PduR/Com 集成测试通过 | P1 |
| L5-3 WdgM 监控 | WdgM 监督实体覆盖率 ≥ 90% | P1 |

### L6 — 集成与发布 (Integration & Release)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L6-1 系统集成测试 | tests/integration/ 全部通过 | P1 |
| L6-2 发布包构建 | `release/` 产物完整，签名校验通过 | P2 |
| L6-3 文档完整性 | API 文档 + 安全案例 + 集成指南齐全 | P2 |

### L7 — CI/CD 门禁 (CI Gates)

| 任务 | 命令级验收门禁 | 优先级 |
|:-----|:---------------|:-------|
| L7-1 三门禁接入 CI | coverage + misra + traceability 全部 fail-closed | P0 |
| L7-2 可复现性 CI | 每次 PR 运行 capture_baseline 并 diff 摘要 | P1 |

---

## 4. 优先级总览

### P0 (量产阻断)
- L0-1/2/3: 基线基础设施
- L1-1/2: 测试修复 + 覆盖率门禁硬化
- L2-1/3: MISRA required 清零 + ratchet
- L3-1: 需求覆盖
- L4-1/2: 重复模块定主 + 模板对齐
- L7-1: CI 门禁接入

### P1 (量产成熟度)
- L1-3: 覆盖率提升至 80/60
- L2-2: 通配偏差复核
- L3-2/3: 人工审查 + MISRA 导出
- L4-3: 无模板补齐
- L5-1/2/3: 安全机制
- L6-1: 系统集成测试
- L7-2: 可复现性 CI

### P2 (发布完善)
- L6-2/3: 发布包 + 文档

---

## 5. PR 路线图

| PR | 名称 | 内容 | 依赖 |
|:---|:-----|:-----|:-----|
| PR-0.1 | 基线捕获 | capture_baseline.sh + summary.json | — |
| PR-0.2 | 模块矩阵 | generate_module_matrix.py + CSV | — |
| PR-0.3 | CI 三门禁 | coverage.enabled=true + misra/trace fail-closed | PR-0.1, PR-0.2 |
| PR-1.3 | 重复模块定主 | 9 个模块选 canonical | PR-0.2 |
| PR-3.1 | MISRA required 导出 | CSV 导出 58 条 required 违规 | PR-0.1 |
| PR-4.1 | 失败用例修复 | 3 个 ctest 失败修复 | PR-0.1 |
