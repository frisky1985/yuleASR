# MISRA Compliance Report

**Generated**: 2026-07-20T10:43:00
**Tool**: Cppcheck 2.17.1 from cppcheck-wheel 1.5.1
**Ruleset**: 2023

## Summary

- **Total Violations**: 258 (full scan across all src/)
- **Unique Rules**: 258
- **Affected Files**: 26+ (previously only 4 — include path fix now enables broader scanning)
- **Previously blocked files**: CanTSyn.c, CanIf.c, Com.c, CanSM.c (CanTSyn.h not found)
- **Affected Files**: 4
- **Density**: 129.0 violations/KLOC

## By Severity

| Severity | Count |
|---------|------:|
| error | 71 |
| style | 173 |

## By Rule Type

| Type | Count |
|------|------:|
| required | 96 |
| advisory | 36 |

## By Category

| Category | Count |
|----------|------:|
| unknown | 126 |
| 预处理器 (Preprocessing) | 44 |
| 控制流 (Control Flow) | 42 |
| 声明 (Declarations) | 37 |
| 未分类 (Uncategorized) | 9 |
