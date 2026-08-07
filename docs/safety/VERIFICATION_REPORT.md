# 验证报告 — ISO 26262-6

> **修订**: 2026-08-07（批D）— 以当日真实测量数据重写。
> **前版问题**: 声称 E2E 98% / 95% / 90%（语句/分支/MCDC）且无日期、无方法、
> 无证据 — 与实测不符（旧 c-coverage.json branch 0.0%）。本版只陈述可复现
> 的真实数据；无数据处如实标注"待测/无证据"。

## 1. 测试覆盖率（实测，2026-08-07）

> 数据源: `.yuleosh/reports/c-coverage.json` + `.yuleosh/reports/branch-coverage-report.md`
> 方法: 原生 host 测试二进制链接 `src/` 生产代码，`--coverage` 编译，
> lcov `--branch-coverage` 测量（BRDA）。复现: `bash tools/run_branch_coverage.sh`

| 指标 | Found | Hit | 覆盖率 |
|:-----|------:|----:|-------:|
| 语句（行） | 2881 | 1246 | **43.25%** |
| 分支 | 1247 | 389 | **31.19%** |
| 函数 | 199 | 132 | 66.33% |

### ASIL 模块逐模块数据（2026-08-07）

| 模块 | 文件 | 行覆盖率 | 分支数据 |
|:-----|:-----|--------:|:--------|
| Crc | `src/bsw/services/crc/src/Crc.c` | 92.50% | ✅ |
| Det | `src/bsw/services/det/src/Det.c` | 83.78% | ✅ |
| Os | `src/bsw/os/src/Os_TimingProtection.c` | 89.72% | ✅ |
| WdgM | `src/bsw/services/wdgm/src/Wdgm.c` | 72.70% | ✅ |
| E2E | `src/bsw/services/e2e/src/E2E_P01..P07.c` | 47–69% | ✅ |
| Com | `src/bsw/services/com/src/Com.c` | 33.81% | ✅ |
| NvM | `src/bsw/services/nvm/src/NvM.c` + EccHandler | 19.65% / 22.65% | ✅ |

**MC/DC**: 未测量（无数据，如实标注）— 需要工具支持（如 VectorCAST 或
gcov 扩展）。ISO 26262-6 §9.2 分支覆盖率要求已获得真实测量数据
（此前为 0.0% 无数据）。

**项目级整体行覆盖率**: 无全仓测量（本次仅 ASIL 7 模块被插桩）；
SWE.4 语句覆盖率 ≥80% 门禁未达成 — 如实标红。

## 2. 安全分析

### FMEA / FTA
- 参见 `docs/safety/fmea-fta-quantitative-analysis.md`（2026-08-04 版本，
  609 行）。
- ⚠️ **FTA 顶层事件概率 < 10^-8/h 的定量结论本报告不背书** — 该数字的
  计算输入（失效率库、共因因子）需独立评审证据；当前无独立评审记录。

### HARA
- 参见 `docs/safety/HARA_ANALYSIS.md`（2026-08-04 版本，88 行）。

## 3. 验证活动（真实证据状态）

| 活动 | 方法 | 工具 | 状态（2026-08-07） |
|:-----|:-----|:-----|:-------------------|
| ASIL 模块单元测试 | 白盒（链接真实 src） | gcc + lcov（7 驱动全部 PASS） | ✅ 有当日证据 |
| SWE.6 合格性测试 | 端到端（pytest e2e + C 驱动） | pytest + 自制 | ✅ 8/8 PASS（`tests/test_swe6/`） |
| SWE.4 语句覆盖 ≥80% | 覆盖率测量 | lcov | ❌ 未达成（43.25%） |
| MISRA 静态分析 | 全仓扫描 | yuleosh misra（C:2023） | ✅ 有报告（基线 36212） |
| 故障注入/模拟测试 | — | — | ❌ 无当日证据（如实标注） |

## 4. 已知缺口（不掩盖）

1. Com `Com_Init` uint8 循环计数 vs 256 信号 — host 构建死循环（配置覆盖仅用于测试）。
2. E2E_Init/E2E_DeInit 声明于 E2E.h 但 src 未实现。
3. NvM.h 声明 `NvM_CancelJobs` 等 4 个 API 在 NvM.c 无实现。
4. WdgM 紧急复位路径（HandleLockstepError 等）设计上不返回，仅评审验证。
5. MC/DC 与全仓覆盖率无数据。
