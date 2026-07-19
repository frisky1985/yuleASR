# yuleASR ASPICE 追溯链修复报告

> 生成时间: 2026-07-19T20:50+08:00
> 评估人: 小马 (Hermes) — 质量架构师
> 修复类型: Spec SHALL 补全 + Traceability 工具 Bug 修复 + 验收矩阵修复

---

## 1. 修复前状态

| 指标 | 修复前 | 修复后 |
|:-----|:------|:------|
| parse_spec 识别的 SHALL 语句 | 0 | 127 |
| traceability CLI 识别的 SHALL 语句 | 1 | 127 |
| 验收矩阵覆盖率 | 0% ❌ FAIL | 0%（需补齐测试）✅（矩阵已正确生成） |
| 可追溯的高层需求 | 0 | 12（docs/spec.md）|
| 可追溯的 BSW 服务需求 | 0 | 28（bsw-services-spec.md）|
| 可追溯的 ECUAL 模块需求 | 0 | 28（ecual-modules-spec.md）|
| 可追溯的 MCAL 驱动需求 | 0 | 39（mcal-drivers-spec.md）|
| 可追溯的模块级需求 | 0（未识别） | 19（module-requirements.md）|
| 可追溯的 MISRA/NFR 需求 | 0（由 #7 识别） | 1（7. MISRA 合规策略）|

## 2. 根因分析

### 2.1 工具 Bug #1: `_is_shall_table_header` 列名过窄

**文件**: `yuleosh/spec/validate.py`、`yuleosh/alm/traceability.py`

`_is_shall_table_header()` 要求第 2 列为 {"描述", "SHALL", "Description", "Statement"} 之一。
但 `docs/spec.md` 的列名为 "需求"、"优先级"、"指标"，`module-requirements.md` 的列名为 "Requirement"。

**修复**: 增加 "需求" 和 "Requirement" 的识别。

### 2.2 工具 Bug #2: `_is_table_separator` 不支持纯破折号分隔线

**文件**: `yuleosh/spec/validate.py`、`yuleosh/alm/traceability.py`

`_is_table_separator()` 要求分隔行包含 ":---" 或 " --- "。但所有 yuleASR spec 表格使用纯 `----` 分隔线（无冒号）。导致表格模式从未被激活。

**修复**: 重写 `_is_table_separator()`，支持 `|---|---|`（纯破折号）和 `|:---|---:|`（含冒号对齐）两种格式。

### 2.3 工具 Bug #3: 表格行仅识别 `-SHALL` 后缀 ID

**文件**: `yuleosh/spec/validate.py`、`yuleosh/alm/traceability.py`

表格行解析器要求 `row_id` 包含 `-SHALL`。但 `module-requirements.md` 使用 `DCM-REQ-01`、`COM-REQ-01` 等 ID，其中文本包含 "SHALL" 但 ID 不含。

**修复**: 增加回退匹配：当 `row_desc` 以 "SHALL" 开头时也视为 SHALL 行（即使 ID 为 `-REQ-` 格式）。

### 2.4 工具 Bug #4: 仅扫描单个 spec 文件

**文件**: `yuleosh/alm/traceability.py`（`generate_lrm`）、`yuleosh/evidence/collection.py`（`collect_requirements`）

- `generate_lrm()` 只扫描 `docs/spec.md` 或 `specs/spec.md`，忽略 `specs/*.md` 全部文件
- `collect_requirements()` 只解析单个文件，忽略 `module-requirements.md`、`bsw-services-spec.md` 等

**修复**: 两个函数均改为扫描 `docs/spec.md` + 所有 `specs/*.md` 文件，合并去重。

### 2.5 工具 Bug #5: `parse_spec` 不解析站外要点式 SHALL

**文件**: `yuleosh/spec/validate.py`

`parse_spec()` 只在 `current_req` 激活时处理 SHALL 要点。对于无 `req_pattern` 匹配的纯描述性 spec，要点式 SHALL 被忽略。

**修复**: 增加独立要点式 SHALL 的 fallback 捕获逻辑，为每个 SHALL 要点自动创建 SpecRequirement。

## 3. 修复详情

### 3.1 yuleOSH 工具层修改

| 文件 | 修改内容 |
|:-----|:---------|
| `yuleosh/spec/validate.py` | ① `_is_shall_table_header`: 增加 "需求"/"Requirement" 识别; ② `_is_table_separator`: 支持纯破折号; ③ 表格行 ID 支持 `-REQ-` + SHALL 描述; ④ 新增独立要点式 SHALL fallback |
| `yuleosh/alm/traceability.py` | ① `_is_shall_table_header`: 增加 "需求"/"Requirement" 识别; ② `_is_table_separator`: 支持纯破折号; ③ 表格行 ID 支持 `-REQ-` + SHALL 描述; ④ `generate_lrm`: 扫描全部 `specs/*.md` |
| `yuleosh/evidence/collection.py` | `collect_requirements`: 扫描全部 `specs/*.md` + `docs/spec.md`，合并去重 |

### 3.2 yuleASR Spec 层修改

| 文件 | 操作 | SHALL 数 |
|:-----|:-----|:---------|
| `specs/bsw-services-spec.md` | 新增 SHALL 要点（DCM/DEM/COM/PduR/NvM/EcuM/OS） | 28 |
| `specs/mcal-drivers-spec.md` | 新增 SHALL 要点（ADC/CAN/Crypto/DIO/PORT/GPT/ICU/MCU/WDG） | 39 |
| `specs/ecual-modules-spec.md` | 新增 SHALL 要点（CanIf/CanTp/CanNm/SoAd/SomeIpSd/DLT/XCP） | 28 |

### 3.3 未修改文件

| 文件 | 原因 |
|:-----|:-----|
| `docs/spec.md` | 保持原有格式（现已可解析） |
| `specs/module-requirements.md` | 保持原有格式（DCM-REQ-01 等，现已可解析） |
| `specs/misra-acceptance-matrix.md` | MISRA 偏差矩阵，非 requirement spec |

## 4. 验收矩阵当前状态

生成路径: `.osh/evidence/acceptance-matrix.md`

```
Summary:
  Total SHALL statements: 127
  Covered by tests: 0 (0%)
  Uncovered: 127
  Threshold: 100% → ❌ FAIL
```

状态为 **❌ FAIL** 是由于测试文件未关联需求（这是预期的——须运行 pipeline 补齐测试），**不是因为 0 条需求**。修复前矩阵显示 0 SHALL → FAIL，现已正确定位到"缺少测试覆盖"。

## 5. 后续建议

### 5.1 Spec 结构改进建议

1. **统一 SHALL 格式**: 建议将所有 bullet-point SHALL 迁移到统一的 table 格式，示例：
   ```markdown
   ### Requirement DCM-SHALL-001: UDS Diagnostic Services
   
   | ID | 描述 | ASIL |
   |:--|:-----|:----:|
   | DCM-SHALL-001 | The system SHALL support ISO 14229-1 UDS diagnostic services | B |
   
   #### Reason
   Required for OEM diagnostic compliance.
   
   #### Status
   APPROVED
   ```
2. **当前 auto-ID 问题**: bullet-point fallback 生成的 ID（如 `DCMDIAGN-SHALL-1`）存在截断不规范问题。建议人工审阅并赋予正式 ID。

### 5.2 yuleOSH 工具改进建议

1. **spec_path 配置化**: 建议在 `ci-config.yaml` 中增加 `spec_paths: []` 字段，允许项目指定需要扫描的 spec 文件列表。
2. **`-REQ-` vs `-SHALL-` 统一**: 当前工具对 ID 格式要求严格（`-SHALL-` 或表头含 SHALL 的描述），建议在 `parse_spec` 中增加对 `-REQ-` ID 且含 SHALL 内容的映射为 `-SHALL-` 的自动转换。
3. **GitHub issue 建议**: 建议将 `_is_table_separator` 的修复（Bug #2）和 `collect_requirements` 的多文件扫描（Bug #4）提 PR 到 yuleOSH 主仓。

### 5.3 测试覆盖补充建议

当前 127 SHALL 的测试覆盖率为 0%。建议：
1. 在每个测试文件中添加 `# Covers: REQ-SHALL-XXX` 注释
2. 或使用 `# Scenario-Ref: DCM-SHALL-01` 格式进行场景引用
3. 运行 pipeline（需 LLM agent）以自动匹配测试与需求

## 6. 总结

本次修复解决了 ASPICE 追溯链的核心断裂：通过修复 5 个工具 Bug 和补充 3 个 spec 文件的 SHALL 语句，使可追溯的需求从 **0** 提升到 **127** 条。验收矩阵从"空矩阵 FAIL"变为"有需求但无测试 FAIL"，定位精确至测试覆盖缺口。

下一步优先级: 补齐测试覆盖 → 验收矩阵 → 需运行 pipeline 生成。
