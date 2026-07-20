# Phase 1 Validation — P0 问题全量修复

> 验证日期: 2026-07-20
> 分支: v1.3.0
> Commit: e5caf1d

## P0-1: evidence数据矛盾 ✅

### 问题
`traceability-matrix.md` 全部 127 条需求显示 `✅ Covered`，即使 0 个测试文件。

### 根因
生成代码使用了 `shall_count > 0` 判定覆盖状态（永远为真），而不是实际测试映射 `matched_tests`。

### 修复
- 新建 `tools/generate_evidence.py`，从 `traceability-report.json` 读取 `matched_tests` 数据
- 三个证据文件均改用 `matched_tests` 判定覆盖状态

### 验证
| 证据文件 | 覆盖数 | 总数 | 符合预期 |
|:---------|:------:|:----:|:--------:|
| traceability-matrix.md | 0 | 127 | ✅ |
| requirement-coverage.md | 0 | 127 | ✅ |
| acceptance-matrix.md | 0 | 127 | ✅ |

**三份证据文件 %通过率一致: 0% ✅**

## P0-2: MISRA CI 报告 ✅

### 问题
- `fail_threshold: 2000` 过于宽松（258 Layer1 违规远低于此值）
- `misra-report.json` 的 `by_severity` 和 `by_rule_type` 为空
- 无端到端测试验证

### 修复
- `fail_threshold`: 2000 → **100**
- `violations_per_kloc`: 200 → **50**
- 修复 MISRA 报告解析，从 raw output 提取结构化数据
- 新建 `tests/e2e/test_misra_ci.py`（6 个测试全通过）

### 验证
```
MISRA violations: 9758
  by_severity: {style: 9758}
  by_rule_type: {required: 4507, advisory: 5251}
fail_threshold: 100
Gate: ❌ FAIL (9758 ≥ 100)
L1 report: 258 MISRA violations (96 required, 36 advisory)
```

## P0-4: 覆盖率范围 ✅

### 问题
- 覆盖率仅采集 Crc.c + Det.c（2 文件，114 行）
- `line_rate=0.0` 可绕过 85% 门禁（小马发现）
- 无真实覆盖率基线

### 修复
- 覆盖范围扩大: 2 文件/114 行 → **63 文件/12071 行**
- 修正门禁逻辑: 0 行数据 → 正确失败
- 生成真实覆盖率基线: 0.8% line, 0.4% branch
- 新建 `tools/check_coverage_gate.py` 门禁检查脚本

### 验证
```
Files measured: 63
Total lines: 12071
Line rate: 0.8%
Branch rate: 0.4%
Threshold: 60.0%
Gate: ❌ FAIL (0.8% < 60.0%)
Bug fix: line_rate=0.0 → correct FAIL ✓
```

## 核心原则
**说真话，数字低可以接受。**
