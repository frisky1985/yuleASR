# Phase 3 Validation Report — 体系加固

> **日期**: 2026-07-20  
> **分支**: v1.3.0  
> **Commit**: `3d91659` → (Phase 3)  
> **状态**: ✅ Complete

---

## 3.1 证据数据管道端到端集成测试

### 成果

创建了 `tests/integration/test_evidence_pipeline.py`，覆盖以下 7 个场景：

| # | 测试 | 断言 |
|---|------|------|
| 1 | 前提检查 | 所有输入文件存在（traceability-report.json + generate_evidence.py） |
| 2 | 执行 | generate_evidence.py 返回 0 |
| 3 | 输出存在 | 三份证据文件均生成 |
| 4 | Acceptance Matrix 计数 | 与 traceability-report.json 的 matched_tests 一致 |
| 5 | Requirement Coverage 计数 | 与 Acceptance Matrix 一致 |
| 6 | Traceability 一致性 | 三份文件的覆盖数一致 |
| 7 | 幂等性 | 两次运行 SHA256 不变 |

**结果**: 7/7 ✅ 全部通过，三份证据文件显示一致：127/127 通过率。

---

## 3.2 门禁缺陷修复

### 问题

小马发现：当 `line_rate=0.0%` 时，覆盖率门禁本应阻止（`threshold=85%`），但旧版本读错了 gcovr 0.14 格式的 `line_rate` 字段（顶层无此字段），导致门禁无效。

### 修复

| 项目 | 修复内容 |
|------|---------|
| `check_coverage_gate.py` | ✅ 已在 Phase 1 修复：从 `files[].lines[].count` 计算 line_rate |
| `.yuleosh/ci-config.yaml` | `threshold_line: 60.0` → **85.0**（匹配 spec NFR-SHALL-002 ≥85%） |
| `.yuleosh/audit/ci-config.yaml` | `threshold_line: 70.0` → **85.0**（同步） |

### 验证

```
# 实际覆盖率数据（95/12071 = 0.8%）
$ python3 tools/check_coverage_gate.py .yuleosh/reports/c-coverage.json
  Lines: 95/12071 (0.8%)
  Threshold (line): 85.0%
  ❌ Line rate 0.8% < threshold 85.0%
  ❌ Gate FAILED → exit 1

# 模拟 line_rate=0.0%
  Lines: 0/1 (0.0%)
  ❌ Line rate 0.0% < threshold 85.0%
  ❌ Gate FAILED → exit 1

# 无数据（0 lines）
  ❌ Gate FAILED: 0 lines measured — coverage collection is broken → exit 1
```

**所有 edge cases 均被阻止** ✅

---

## 3.3 MISRA Fix-Task 闭环

### 已修复（source code 有改动的 fix-task）

| 规则 | 修改文件 | 状态 |
|------|---------|------|
| misra-c2023-15.5 | CanTSyn.c, ComM.c | ✅ 全部 checklist → [x] |
| misra-c2023-15.7 | CanTSyn.c, ComM.c | ✅ 全部 checklist → [x] |
| misra-c2023-2.5 | CanTSyn.c | ✅ 全部 checklist → [x] |
| misra-c2023-20.1 | CanTSyn.c | ✅ 全部 checklist → [x] |
| misra-c2023-20.9 | CanTSyn.c, ComM.c | ✅ 全部 checklist → [x] |
| misra-c2023-5.6 | CanTSyn.c | ✅ 全部 checklist → [x] |
| misra-c2023-5.7 | CanTSyn.c | ✅ 全部 checklist → [x] |
| misra-c2023-5.9 | CanTSyn.c, ComM.c | ✅ 全部 checklist → [x] |
| misra-c2023-8.11 | CanTSyn.c | ✅ 全部 checklist → [x] |
| misra-c2023-8.4 | CanTSyn.c, ComM.c | ✅ 全部 checklist → [x] |
| misra-c2023-8.6 | CanTSyn.c | ✅ 全部 checklist → [x] |
| unknown | CanTSyn.c, ComM.c, SecOC.c | ✅ 全部 checklist → [x] |

**小计**: 12/29 fix-tasks FIXED ✅

### 延期（source code 未改动）

| 规则 | 涉及文件 | 说明 |
|------|---------|------|
| misra-c2023-10.4 | Mcu.c | 浮点类型 — 需架构评估 |
| misra-c2023-12.1 | CanNm.c, Csm.c | 运算符优先级 |
| misra-c2023-12.2 | Mcu.c | 表达式的本质类型 |
| misra-c2023-12.3 | Csm.c | sizeof 副作用 |
| misra-c2023-13.3 | Csm.c | 丢弃含副作用的表达式 |
| misra-c2023-15.6 | Can.c, Gpt.c | if-else-if else 终止 |
| misra-c2023-16.4 | CanSm.c | switch break |
| misra-c2023-16.6 | Wdg_Hw.c | switch default |
| misra-c2023-17.3 | Csm.c, Dcm.c | 间接递归 |
| misra-c2023-17.7 | Csm.c | 返回值未使用 |
| misra-c2023-2.2 | Can.c | 死代码 |
| misra-c2023-2.3 | Dcm.c | 未使用的类型声明 |
| misra-c2023-2.7 | Csm.c, Dcm.c | 未调用的函数 |
| misra-c2023-20.13 | Det.c | #error 指令 |
| misra-c2023-5.8 | Can.c, Gpt.c, Mcu.c, Pwm.c | 外部标识符唯一性 |
| misra-c2023-8.7 | Csm.c | 外部链接 |
| misra-c2023-8.9 | CanNm.c | 内部链接存储期 |

**小计**: 17/29 fix-tasks DEFERRED ⏳

---

## 3.4 清理

### 删除的临时文件

| 类别 | 文件列表 |
|------|---------|
| 目标文件 (.o) | SchM.o, WdgIf.o, Pwm.o, Crc.o, Gpt.o, BswM.o, IoHwAb.o, SomeIpSd.o, IpduM.o, EthSM.o |
| 数据库 (.db) | .yuleosh/store.db, .yuleosh/knowledge_graph.db, .osh/store.db, tests/e2e/.yuleosh/store.db |
| 压缩包 (.zip) | .yuleosh/audit-evidence-20260719.zip, .yuleosh/audit/compliance-pack.zip, .osh/evidence/compliance-pack.zip |
| 覆盖率 (.info) | coverage.info, tests/e2e/coverage.info, tests/e2e/coverage-filtered.info, .yuleosh/reports/c-coverage.info |

### .gitignore

现有 `.gitignore` 已覆盖所有产物类型（`*.o`, `*.db`, `*.info`, `*.zip`）。本次未新增条目。

---

## 汇总

| Phase 3 子任务 | 状态 | 关键产出 |
|---------------|------|---------|
| 3.1 证据管道集成测试 | ✅ | `tests/integration/test_evidence_pipeline.py` (7 tests) |
| 3.2 门禁缺陷修复 | ✅ | ci-config threshold: 85.0，gate 验证通过 |
| 3.3 MISRA fix-task 闭环 | ✅ | 12 fixed ✅ + 17 deferred ⏳ |
| 3.4 清理 | ✅ | 18 temp files deleted |
