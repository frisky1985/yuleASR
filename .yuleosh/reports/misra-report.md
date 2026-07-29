# MISRA Compliance Report

**Generated**: 2026-07-26T13:15:37.175183
**Tool**: Cppcheck 2.17.1 from cppcheck-wheel 1.5.1
**Ruleset**: 2023

## Summary

- **Total Violations**: 768
- **Unique Rules**: 2
- **Affected Files**: 174
- **Density**: 12.11 violations/KLOC

## By Severity

| Severity | Count |
|---------|------:|
| error | 16 |
| warning | 2 |
| style | 313 |
| portability | 2 |

## By Rule Type

| Type | Count |
|------|------:|
| advisory | 4 |

## By Category

| Category | Count |
|----------|------:|
| unknown | 764 |
| 基本类型 (Essential Types) | 4 |

## Violations by Rule

- **unknown** (764 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/rte/include/Rte.h:21` — Include file: "Compiler.h" not found. [missingInclude]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/rte/include/Rte.h:79` — Include file: "MemMap.h" not found. [missingInclude]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/rte/include/Rte.h:353` — Include file: "MemMap.h" not found. [missingInclude]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/rte/include/Rte_Swc.h:178` — Include file: "MemMap.h" not found. [missingInclude]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/rte/include/Rte_Swc.h:264` — Include file: "MemMap.h" not found. [missingInclude]
- **misra-c2023-10.5** (4 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Hsm_1.0.0.c:173` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Lockstep_1.0.0.c:307` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Lockstep_1.0.0.c:308` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Lockstep_1.0.0.c:309` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1