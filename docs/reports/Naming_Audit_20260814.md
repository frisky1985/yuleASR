# AUTOSAR 命名规范审计报告

> 审计日期: 2026-08-14 | 依据: .ai-rules.md 5.1 第 5 条（AUTOSAR 命名标准）
> 范围: src/ 全部 C/H 文件（排除 common/platform/libs/microdds/dds/third_party/generated/bootloader）

## 结论摘要

| 类别 | 数量 | 处置 |
|------|------|------|
| 标准 AUTOSAR 命名（NvM_WriteBlock 等） | 776 | ✅ 合规 |
| 小写模块内实现（dcm.c/csm_core.c 等） | ~100 | ⚠️ 记 tech-debt，不批量改名（破坏 include/CMake 引用，风险>收益） |
| legacy 目录（dcm/dem 旧实现） | 45 | ⏸️ 豁免（legacy 冻结，不重构） |
| bootloader bl_* | 12 | ⏸️ 豁免（bootloader 自成体系） |
| ara_*（Adaptive）/ asw_* | 3 | ✅ 合规（AUTOSAR Adaptive 标准前缀） |
| microdds 第三方库 | - | ✅ 豁免（第三方依赖） |

## 主要违规模式（新代码禁止）

1. **模块内实现文件小写**：`dcm.c` → 应为 `Dcm.c`；`csm_core.c` → 应为 `Csm_Core.c`
2. **通用名无模块前缀**：`cbs.c`（tsn）→ 应为 `Tsn_Cbs.c` 或按模块前缀
3. **违反**: 文件名不以模块缩写大写开头

## 处置原则（2026-08-14 定）

- **存量违规不批量改名**：162 处改名会破坏 include/CMake 引用面，属风格非功能缺陷
- **新代码强制合规**：AI 生成/新增文件必须遵循 AUTOSAR 命名（.ai-rules.md 已生效）
- **模块重构时顺带改名**：legacy 重写/dcm 重构时按新规范命名
- 跟踪: yuleASR tech-debt（命名合规专项，P2）

## 审计方法

```bash
# 违规扫描（排除豁免目录）
find src -name "*.c" -o -name "*.h" | grep -vE "/common/|/platform/|/libs/|/microdds/|/dds/|/third_party/|/generated/|/bootloader/" \
  | awk -F/ '{print $NF}' | grep -vE "^[A-Z]|^Rte_|^Std_|^Platform_|^test_|^unity|_test\.|_mock\.|_stub\." \
  | grep -E "^[a-z]" | sort -u
```
