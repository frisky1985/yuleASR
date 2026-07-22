# MISRA Compliance Report

**Generated**: 2026-07-23T01:43:49.299263
**Tool**: Cppcheck 2.17.1 from cppcheck-wheel 1.5.1
**Ruleset**: 2023

## Summary

- **Total Violations**: 1141
- **Unique Rules**: 3
- **Affected Files**: 186
- **Density**: 17.5 violations/KLOC

## By Severity

| Severity | Count |
|---------|------:|
| error | 17 |
| warning | 2 |
| style | 685 |
| portability | 2 |

## By Rule Type

| Type | Count |
|------|------:|
| required | 372 |
| advisory | 4 |

## By Category

| Category | Count |
|----------|------:|
| unknown | 765 |
| 指针 (Pointer) | 372 |
| 基本类型 (Essential Types) | 4 |

## Violations by Rule

- **unknown** (765 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/rte/include/Rte.h:21` — Include file: "Compiler.h" not found. [missingInclude]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/rte/include/Rte.h:79` — Include file: "MemMap.h" not found. [missingInclude]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/rte/include/Rte.h:353` — Include file: "MemMap.h" not found. [missingInclude]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/rte/include/Rte_Swc.h:178` — Include file: "MemMap.h" not found. [missingInclude]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/rte/include/Rte_Swc.h:264` — Include file: "MemMap.h" not found. [missingInclude]
- **misra-c2023-11.9** (372 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinMaster_Diagnostic.c:55` — misra violation 1109 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinMaster_Diagnostic.c:83` — misra violation 1109 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinMaster_Diagnostic.c:113` — misra violation 1109 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinMaster_Diagnostic.c:142` — misra violation 1109 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinMaster_Diagnostic.c:193` — misra violation 1109 with no text in the supplied rule-texts-file [misra-c2012-1
- **misra-c2023-10.5** (4 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Hsm_1.0.0.c:173` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Lockstep_1.0.0.c:307` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Lockstep_1.0.0.c:308` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Lockstep_1.0.0.c:309` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1