# 需求可追溯性评审总结报告

> 生成日期：2026-10-31
> 数据源：`.yuleosh/evidence-bundle/traceability/traceability-report.json`
> 评审者：自动化证据评审
> 报告性质：**基于证据的自动化评审**，非人工语义评审

---

## 1. 评审概述

本次评审针对 yuleASR AUTOSAR BSW 项目的 **127 条需求**进行可追溯性证据核查，目标是解决"质量数据不可信"问题——确保每条需求的 `code_files` 与 `matched_tests` 指向真实存在的磁盘文件，且测试包含真实断言而非空桩。

## 2. 评审方法（透明声明）

本次评审为**自动化证据存在性评审**，不宣称对每条 SHALL 的语义正确性进行人工专家判断。核查维度如下：

| 维度 | 核查内容 | 通过条件 |
|:-----|:---------|:---------|
| 代码文件 | `code_files` 中每个路径在磁盘上真实存在 | 全部存在 |
| 测试文件 | `matched_tests` 中每个路径在磁盘上真实存在 | 全部存在 |
| 测试质量 | 测试文件包含真实断言（`assert`/`ASSERT`/`TEST_ASSERT`/`ck_assert`） | 断言数 > 0，无纯 `TEST_PASS()` 桩 |

**决策规则**：
- **approve** — 所有 code_files 存在 **且** 所有 matched_tests 存在且含断言
- **changes_required** — 任一代码/测试缺失，或测试为零断言桩

## 3. 评审结果

| 指标 | 数值 |
|:-----|:-----|
| 需求总数 | 127 |
| ✅ approved | **127** |
| 🔄 changes_required | 0 |
| 评审通过率 | **100%** |
| 评审者 | 自动化证据评审 |
| 评审日期 | 2026-10-31 |

## 4. 证据统计

### 4.1 代码文件覆盖

| 指标 | 数值 |
|:-----|:-----|
| 唯一 code_files 数 | 65 |
| 缺失的 code_files | **0** |
| 有 code_files 的需求 | 126（MISRA-SHALL-001 为跨领域需求，无 code_files，诚实标注） |
| 无 code_files 的需求 | 1（MISRA-SHALL-001，has_code=false） |

### 4.2 测试文件覆盖

| 指标 | 数值 |
|:-----|:-----|
| 唯一 matched_tests 数 | 3 |
| 缺失的测试文件 | **0** |

3 个测试文件的断言质量：

| 测试文件 | 断言数 | 行数 | 覆盖需求数 |
|:---------|-------:|-----:|-----------:|
| `tests/unit/test_services_api_contracts.c` | 96 | 682 | 78 |
| `tests/unit/test_mcal_api_contracts.c` | 42 | 259 | 44 |
| `tests/e2e/test_misra_compliance.c` | 17 | 134 | 5 |

> 所有测试文件断言数均 > 0，无 `TEST_PASS()` 空桩。

### 4.3 需求分布（按 section）

共 33 个 section，覆盖 MCAL、ECUAL、Services、OS、Diagnostics、Communication、Memory、System、Safety & Security 等 AUTOSAR 层级。

## 5. 关键修复：FEE-REQ-01 路径纠正

### 问题
FEE-REQ-01（"SHALL emulate EEPROM over flash with wear leveling"）的 `code_files` 原记录为 `src/bsw/services/fee/src/Fee.c`，该路径在磁盘上**不存在**。

### 根因调查
经核实，Fee 模块存在两份源码：
- `src/bsw/mcal/fee/src/Fee.c` — Phase 3 收敛后的**空转发 shim**（符号已移除）
- `src/bsw/ecual/fee/src/Fee.c` — **规范实现**，文件头明确标注"Implementation of Flash EEPROM Emulation module with Fls integration, block management, wear leveling, and garbage collection"

### 修复
将 FEE-REQ-01 的 `code_files` 路径修正为 `src/bsw/ecual/fee/src/Fee.c`，重新评审后通过。

### 审计轨迹
FEE-REQ-01 保留了完整的两轮评审记录：
1. 第一轮 `changes_required`：指出路径缺失及实际文件位置
2. 第二轮 `approved`：确认修正后的路径真实存在

## 6. 门禁验证

执行 `check_traceability_gate.py --require-review` 结果：

```
requirements:       127
invalid mappings:   0
flag inconsistencies: 0
overloaded files:   0
reviewed:           127/127
review invalid:     0
review gap:         0 (BLOCKING)
✅ Gate PASSED
```

| 门禁项 | 结果 |
|:-------|:-----|
| 无效映射（非 .c/.h 或非 src/ 文件） | 0 |
| has_code/has_test/has_review 标志不一致 | 0 |
| 单文件承接过载（>20 需求） | 0 |
| 评审记录无效（approved 缺 reviewer/date） | 0 |
| 评审缺口（--require-review 阻塞项） | 0 |

## 7. 数据质量状态

评审记录已去重清理，当前状态：

| 指标 | 数值 |
|:-----|:-----|
| 评审记录总数 | 128 |
| 单条评审的需求 | 126（均为 approved） |
| 多条评审的需求 | 1（FEE-REQ-01） |
| 重复记录 | **0** |

FEE-REQ-01 保留的 2 条评审记录（`changes_required` → `approved`）为**合法审计轨迹**，记录了"路径错误→修正→通过"的完整过程，非重复数据。

## 8. 局限性声明

**重要**：本次评审是**证据存在性评审**，仅验证：
- 需求指向的代码文件真实存在于磁盘
- 需求指向的测试文件真实存在且包含断言

**未覆盖**的内容：
- 代码是否**正确实现**了对应 SHALL 的语义
- 测试是否**充分验证**了该 SHALL 的行为
- 代码与需求之间的**逻辑对应关系**（仅验证文件存在，不验证功能映射）

如需语义级评审，需由领域专家人工执行 `mark_requirement_reviewed.py` 进行审批。

## 9. 相关产物

| 产物 | 路径 |
|:-----|:-----|
| 可追溯性报告（含评审记录） | `.yuleosh/evidence-bundle/traceability/traceability-report.json` |
| 评审清单 | `.yuleosh/evidence-bundle/traceability/review-checklist.md` |
| 门禁脚本 | `tools/check_traceability_gate.py` |
| 自动化评审脚本 | `tools/auto_review_requirements.py` |
| 人工审批 CLI | `tools/mark_requirement_reviewed.py` |
