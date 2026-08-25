# Requirements Index — yuleASR BSW

> 生成: 2026-08-25 (yuleOSH 审计闭环 — SWE.1.BP2 需求结构化)
> 说明: 本文件为需求文档的**功能区域索引**，指向既有需求文档，不替代原文档。
> 全部需求标识符与 SHALL 语句以原文档为准。

## 功能区域分组

| 功能区域 | 需求文档 | 覆盖模块 | 需求 ID 前缀 |
|:---------|:---------|:---------|:-------------|
| **BSW 平台基础** | docs/requirements.md | 平台/OS/启动 | SWR-001.x, SYS-REQ-BSW |
| **MCAL 驱动** | docs/specs/mcal-drivers-spec.md | Mcu/Port/Dio/Can/Spi/Gpt/Pwm/Adc/Wdg/Icu/Lin | SWR-002.x |
| **ECUAL 抽象** | docs/specs/ecual-modules-spec.md | CanIf/CanTp/EthIf/MemIf/Fee/Ea/FrIf/LinIf | SWR-003.x |
| **服务层** | docs/specs/bsw-services-spec.md | Com/PduR/NvM/Dcm/Dem | SWR-004.x |
| **RTE/ASW** | docs/specs/ | Rte + 8 ASW 组件 | SWR-005.x |
| **安全/HSM** | docs/specs/COMPLETE_HSM_INTEGRATION_REPORT.md | SecOC/Crypto/HSM | SWR-006.x |
| **专项规格** | docs/specs/SRS-Swc_EngineControl-V1.0.md | EngineControl | SRS-* |

## 需求属性约定

- **优先级**: P0(量产门槛) / P1(高) / P2(中) — 见各需求文档状态字段
- **状态**: ✅ Covered / ⚠️ Partial / ❌ Missing — 由 yuleOSH traceability 生成
- **追溯**: 每条需求 → 系统需求 (SYS-REQ) + 测试文件 (tests/...)

## 结构化验证

- 需求总数: 56 唯一 ID / 287 SHALL 语句（yuleOSH ev check 实测）
- 测试追溯: 150 reqs 关联 7 test files（yuleOSH CI layer1 requirements-trace）
- 双向追溯矩阵: `yuleosh traceability` → .osh/evidence/traceability-matrix.md
