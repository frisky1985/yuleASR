# OpenSpec Change Proposal: Missing Classic AUTOSAR Modules Implementation

## 概述
本变更旨在实现和完善 yuleASR 项目中未完成的 Classic AUTOSAR BSW 模块。

## 目标
1. 完成 Eth (Ethernet) 驱动的核心实现
2. 新增 Icu (输入捕获) 驱动模块
3. 新增 FrTp (FlexRay Transport Protocol) 模块
4. 新增 Ocu (输出比较) 驱动模块 (可选)

## 范围
覆盖以下模块:
- MCAL 层: Eth, Icu, Ocu
- ECUAL 层: FrTp

## 接受标准
- 所有模块通过 AUTOSAR 规范验证
- 每个模块配套单元测试覆盖率 >= 80%
- 通过 ASIL-D 安全门禁检查
- 与现有模块无缺失接口冲突

## 时间线
- 里程碑 M1: Eth 实现 (1周)
- 里程碑 M2: Icu 实现 (5天)
- 里程碑 M3: FrTp 实现 (1周)
- 里程碑 M4: Ocu 实现 (3天, 可选)

## 影响分析
- 新增源文件: ~15 个
- 修改文件: 无 (新增模块)
- 风险: 低 (独立模块，无破坏性变更)

---
提案人: OSH Orchestrator
日期: 2026-04-29
状态: 待审批
