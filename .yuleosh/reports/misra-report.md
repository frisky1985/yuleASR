# MISRA Compliance Report

**Generated**: 2026-07-22T13:44:20.714904
**Tool**: Cppcheck 2.17.1 from cppcheck-wheel 1.5.1
**Ruleset**: 2023

## Summary

- **Total Violations**: 1127
- **Unique Rules**: 2
- **Affected Files**: 181
- **Density**: 17.76 violations/KLOC

## By Severity

| Severity | Count |
|---------|------:|
| error | 17 |
| warning | 2 |
| style | 673 |
| portability | 2 |

## By Rule Type

| Type | Count |
|------|------:|
| required | 372 |

## By Category

| Category | Count |
|----------|------:|
| unknown | 755 |
| 指针 (Pointer) | 372 |

## Violations by Rule

- **unknown** (755 violations)
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