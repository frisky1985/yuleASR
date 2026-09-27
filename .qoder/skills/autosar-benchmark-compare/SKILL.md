---
name: autosar-benchmark-compare
description: yuleASR AUTOSAR Classic 项目（/Users/ingeek/workspace/AUTOSAR）与开源基线的完整性对标分析。当用户要求"对比/对标开源 AUTOSAR"、"项目完整性评估"、"模块覆盖差距分析"、"BSW 深度对比"、"量产差距分析"、"completeness vs open source"时使用。两档流程：目录级覆盖矩阵（快速）与 BSW 深度对比（模块 LOC/头文件符号面/量产差距，脚本化，报告产出到 references/）。
---

# AUTOSAR 开源基线对标分析

## 何时使用

用户要求以下任一任务时执行本流程：
- 与开源 AUTOSAR 对比 / 对标 / 差距分析
- 评估当前项目完整性 / 模块覆盖率
- 询问"我们缺什么模块"或"我们比开源多什么"
- 要求深度对比 / 量产差距分析（在执行步骤之后追加「深度对比」章节流程）

## 基线档案（2026-09-26 核实）

| 基线 | 定位 | 许可证 | 活跃度 | 数据源 |
|:-----|:-----|:-------|:-------|:-------|
| **开源小满 EasyXMen**（普华基础软件） | **主基线**：完整 CP 栈（BSW+RTOS+Test），中国 OEM 生态共建 | LGPL-2.1（量产静态链接需合规评估） | 高（V25.10 在更） | 主仓 AtomGit/Gitee `easyxmen/XMen`；GitHub 镜像 `easyxmen/EasyXMen` |
| Eclipse OpenBSW（Elektrobit） | 工程实践基线：现代 C++ BSW（诊断/SOME-IP/日志），**非严格 AUTOSAR 符合** | Apache-2.0 | 高（2026-09 仍在推） | github.com/eclipse-openbsw/openbsw |
| openAUTOSAR/classic-platform | 仅架构参考：Arctic Core 分叉，AUTOSAR 4.0 时代代码 | GPL-2.0（**禁止进入商业产品**） | 低（2024-08 后停滞） | github.com/openAUTOSAR/classic-platform |

## 执行步骤

1. **枚举本地模块树**：
   ```bash
   ls /Users/ingeek/workspace/AUTOSAR/src/bsw/{mcal,ecual,services,os,cdd}
   ```
2. **刷新基线模块清单**（EasyXMen 主基线，GitHub 镜像 API）：
   ```bash
   curl -sL "https://api.github.com/repos/easyxmen/EasyXMen/contents/BSWCode"   # BSW 模块
   curl -sL "https://api.github.com/repos/easyxmen/EasyXMen/contents/"          # RTOS/Drivers/Test
   ```
   注意：EasyXMen `Drivers/` 为空（MCAL 不开源）——**MCAL 层无开源对标物**。
3. **构建差距矩阵**，逐模块三分类：
   - ✔ 双方都有（写本地对应路径，注意层级差异）
   - ＋ 仅本地（超出基线）
   - − 仅基线（缺口）
4. **补充维度**（可选深挖）：测试成熟度、配置工具链、芯片支持（S32K312/i.MX8M vs TC4x/RISC-V）。
5. **输出**：差距矩阵表 + 统计（覆盖 X/Y）+ 缺口清单 + 本地超出项 + 许可风险提示。

## 深度对比（量产维度）

当用户要求"深入对比 / 量产差距 / BSW 深度分析"时，在目录矩阵之后追加：

1. **量化扫描**：`node scripts/scan-module-metrics.js <父目录>` 逐模块统计 .c/.h 数、LOC、头文件声明符号数（已剔除 #define 宏噪声）。
   - 基线全仓清单一次拉取：`curl -sL "https://api.github.com/repos/easyxmen/EasyXMen/git/trees/master?recursive=1" -o /tmp/emx_tree.json`；
   - 关键模块头文件从 raw.githubusercontent 下载（`--retry 3` 防 SSL 抖动，跳过 >250KB 文件）。
2. **符号面 diff**：`node scripts/api-surface-diff.js <我方模块目录> <基线头文件目录>` 输出双方独有符号名。
   - 双方层级不同需手工映射（如 Crypto_62 ↔ mcal/crypto，BswM ↔ services/bswm）；
   - 同名模块多层重复时（ecual/services/classic）逐一对比并记录归属疑问。
3. **判定分级**：显著差距 / 中等 / 相当 / 我方更深（结合代码量级 + 符号差 + 关键 API 名语义，勿只看数量）。
4. **产出**：`references/bsw-depth-analysis-<date>.md`（对照表 + 量产差距清单 P0/P1/P2 + 结构性风险），原始 diff/扫描文本同目录留档。
5. 符号差含内部符号与命名差异，解读时区分"公开 API 缺失"与"实现风格不同"。

## 结论口径（必须遵守）

- 声明"**目录级覆盖**"口径——目录存在 ≠ 功能等价/规范符合度，不得写成"功能完整实现"。
- 深度结论须标注数据口径（头文件符号面 / 代码量级估算）；基线 .c 未下载时不得断言实现质量。
- 主基线的 MCAL 缺失、许可差异（LGPL/GPL）必须在结论中提示。
- 历史执行结果：`references/gap-matrix-2026-09-26.md`（52/52 目录覆盖）、`references/bsw-depth-analysis-2026-09-26.md`（深度对标与量产差距）+ 原始数据两份（api-symbol-diff / easyxmen-header-scan）。

## 已知注意

- EasyXMen 主仓在 AtomGit/Gitee；GitHub 仅为镜像，刷新失败时改用 `curl https://gitee.com/api/v5/repos/easyxmen/XMen/contents/BSWCode`。
- 全仓清单用 `git/trees/<branch>?recursive=1` 一次拉取（勿逐目录打 contents API，省配额）；raw.githubusercontent 需 `--retry 3`。
- 对比耗时：目录矩阵约 5-10 分钟；深度模式含下载约 15-30 分钟（视网络）。
