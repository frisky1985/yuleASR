# MISRA Compliance Report

**Generated**: 2026-08-25T20:56:48.070951
**Tool**: Cppcheck 2.21.0
**Ruleset**: 2023

## Summary

- **Total Violations**: 74
- **Unique Rules**: 1
- **Affected Files**: 5
- **Density**: 17.24 violations/KLOC

## By Severity

| Severity | Count |
|---------|------:|
| error | 51 |
| warning | 7 |
| style | 13 |

## By Rule Type

| Type | Count |
|------|------:|

## By Category

| Category | Count |
|----------|------:|
| unknown | 74 |

## Violations by Rule

- **unknown** (74 violations)
  - `/Users/ingeek/workspace/AUTOSAR/src/bsw/services/comM/src/ComM.c:765` — The statement 'if (ComM_ChannelStates[Channel].CurrentMode!=newMode) ComM_Channe
  - `/Users/ingeek/workspace/AUTOSAR/src/bsw/services/comM/src/ComM.c:766` — Assignment 'ComM_ChannelStates[Channel].CurrentMode=newMode'
  - `/Users/ingeek/workspace/AUTOSAR/src/bsw/services/comM/src/ComM.c:765` — Condition 'ComM_ChannelStates[Channel].CurrentMode!=newMode' is redundant
  - `/Users/ingeek/workspace/AUTOSAR/src/bsw/ecual/canNm/src/CanNm.c:353` — Unused variable: ChCfg [unusedVariable]
  - `/Users/ingeek/workspace/AUTOSAR/src/bsw/ecual/canNm/src/CanNm.c:502` — Variable 'ChCfg' is assigned a value that is never used. [unreadVariable]