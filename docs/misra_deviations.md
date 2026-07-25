# MISRA C:2012 Deviation Permits — yuleASR

## Document Information

| Property | Value |
|:---------|:------|
| Project | yuleASR — AUTOSAR BSW Platform |
| Standard | MISRA C:2012 Amendment 2 |
| Version | 2.0 |
| Date | 2026-07-26 |
| Config Source | `.yuleosh/ci-config.yaml` |

---

## 1. Overview

This document records all formal deviation permits for MISRA C:2012 rule violations
across the yuleASR BSW platform. Deviations are justified per AUTOSAR R21-11 conventions
and the MISRA deviation permit guidelines.

### Deviation Categories

| Category | Description |
|:---------|:------------|
| **Required (deviations)** | Required rule violations with formal deviation and technical justification |
| **Advisory (deviations)** | Advisory rule violations with documented pattern acceptance |
| **Exclude Paths** | Paths excluded from MISRA scanning entirely |

### Current Baseline

| Metric | Value |
|:-------|:------|
| Advisory violations | 66 (Phase 2 baseline) |
| Fail threshold | 13000 (Phase 1) |
| Violations per KLOC | 150.0 max |
| Active profile | `safety` |

---

## 2. Global Deviations (ci-config)

The following deviations apply project-wide (`src/**`) via `ci-config.yaml`:

| # | Rule | Scope | Reason |
|:-:|:-----|:------|:-------|
| G1 | misra-c2012-20.9 | `src/**` | AUTOSAR R21-11 §8.4 — `#if defined()` required for config switches |
| G2 | misra-c2012-7.2 | `src/**` | AUTOSAR embedded unsigned/signed comparison (Std_ReturnType + uint32) |
| G3 | misra-c2012-5.8 | `src/**` | AUTOSAR BSW config struct naming convention overlaps across modules |
| G4 | misra-c2012-15.7 | `src/**` | AUTOSAR switch-case complete coverage; no else needed |
| G5 | misra-c2012-2.7 | `src/**` | AUTOSAR BSW API signature uniformity requires unused params |
| G6 | misra-c2012-15.6 | `src/**` | AUTOSAR error-handling nested if/else required for DET traceability |
| G7 | misra-c2012-11.5 | `src/bsw/mcal/**` | Hardware register and memory-mapped I/O void* conversion |
| G8 | misra-c2012-8.8 | `src/**` | AUTOSAR BSW inter-module API external linkage |
| G9 | misra-c2012-5.7 | `src/**` | AUTOSAR config type tag naming per module |
| G10 | misra-c2012-10.8 | `src/**` | AUTOSAR integer casting for Std_ReturnType and module IDs |

---

## 3. Safety Profile Deviations (detailed permits)

The safety profile defines 19 detailed deviation permits, each with scope, justification,
and expiry date.

### DP-AUTOSAR-001: Rule 15.5 — Multiple Return Points

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-15.5 (Advisory) |
| Scope | `src/**` |
| Reason | AUTOSAR BSW error handling uses multiple-return pattern for early exits on failure conditions |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-002: Rule 17.7 — Unused Return Value

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-17.7 (Required) |
| Scope | `src/**/det*` |
| Reason | DET/TRACE diagnostic call return value intentionally unused |
| Expiry | 2027-01-21 |

### DP-AUTOSAR-003: Rule 2.5 — Include Guard

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-2.5 (Required) |
| Scope | `src/**` |
| Reason | AUTOSAR R21-11 §7.2 include guard naming (`_MODULE_H_` convention) |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-004: Rule 10.1 — Config Macro Boolean

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-10.1 (Required) |
| Scope | `src/**/cfg/*` |
| Reason | AUTOSAR config macro boolean flag pattern (STD_ON/STD_OFF) |
| Expiry | 2027-01-21 |

### DP-AUTOSAR-005: Rule 14.4 — Config Enum Coverage

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-14.4 (Required) |
| Scope | `src/**` |
| Reason | AUTOSAR config enum switch-case complete coverage, no default needed |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-006: Rule 8.13 — Pointer Const Qualification

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-8.13 (Advisory) |
| Scope | `src/bsw/services/pdur/**` |
| Reason | PduR API signature compatibility (non-const pointer interface) |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-007: Rule 11.4 — Convert to/from Pointer

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-11.4 (Required) |
| Scope | `src/bsw/services/ramsafety/**;src/bsw/mcal/**` |
| Reason | Hardware register access and RamSafety address mapping |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-008: Rule 15.1 — goto Statement

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-15.1 (Required) |
| Scope | `src/bsw/mcal/crypto/src/Crypto_MbedTLS.c;src/bsw/boot/src/Boot_Loader.c;src/bsw/services/mqtt/src/Mqtt_Tls.c` |
| Reason | `goto cleanup/fail` for error handling in crypto/bootloader/TLS contexts |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-009: Rule 19.2 — Union Use

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-19.2 (Required) |
| Scope | `src/bsw/mcal/wdg/include/Wdg_Hw.h;src/bsw/services/dem/legacy/**;src/bsw/services/dcm/legacy/**;src/bsw/services/wdgm/include/WdgM.h;src/micro-dds/src/serialization/cdr.c` |
| Reason | Union for hardware register mapping, legacy type polymorphism, and serialization |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-010: Rule 20.9 — Preprocessor Directive

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-20.9 (Required) |
| Scope | `src/**` |
| Reason | AUTOSAR R21-11 §8.4 requires `#if defined()` for config switches; intentional deviation |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-011: Rule 7.2 — Unsigned/Signed Mixing

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-7.2 (Required) |
| Scope | `src/**` |
| Reason | AUTOSAR embedded code mixes unsigned counters with signed Std_ReturnType; per AUTOSAR_SWS_PlatformTypes.pdf |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-012: Rule 5.8 — Tag Name Overlap

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-5.8 (Required) |
| Scope | `src/**` |
| Reason | AUTOSAR BSW config struct naming convention overlaps across modules; unique per module scope |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-013: Rule 15.7 — Empty Else

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-15.7 (Required) |
| Scope | `src/**` |
| Reason | AUTOSAR switch-case complete coverage pattern; no else needed as all cases handled |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-014: Rule 2.7 — Unused Parameter

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-2.7 (Required) |
| Scope | `src/**` |
| Reason | AUTOSAR BSW function signature compatibility requires unused params for API uniformity |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-015: Rule 15.6 — If/Else Depth

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-15.6 (Required) |
| Scope | `src/**` |
| Reason | AUTOSAR error-handling nested if/else pattern; any depth required for DET traceability |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-016: Rule 11.5 — Void Pointer Conversion

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-11.5 (Required) |
| Scope | `src/bsw/mcal/**;src/bsw/services/ramsafety/**;src/platform/**` |
| Reason | AUTOSAR hardware register and memory-mapped I/O requires void pointer conversion |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-017: Rule 8.8 — External Linkage

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-8.8 (Required) |
| Scope | `src/**` |
| Reason | AUTOSAR BSW inter-module API requires external linkage for service functions |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-018: Rule 5.7 — Tag Uniqueness

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-5.7 (Required) |
| Scope | `src/**` |
| Reason | AUTOSAR config type tag naming per module; tags unique within translation unit |
| Expiry | 2027-07-21 |

### DP-AUTOSAR-019: Rule 10.8 — Integer Casting

| Field | Value |
|:------|:-------|
| Rule | misra-c2012-10.8 (Required) |
| Scope | `src/**` |
| Reason | AUTOSAR integer type safe casting for Std_ReturnType and module IDs |
| Expiry | 2027-07-21 |

---

## 4. Advisory Accepted Patterns

The following advisory rules are accepted as project patterns (refer to corresponding
DP for justification):

| Advisory Rule | Linked DP |
|:--------------|:----------|
| misra-c2012-15.5 | DP-AUTOSAR-001 |
| misra-c2012-17.7 | DP-AUTOSAR-002 |
| misra-c2012-8.13 | DP-AUTOSAR-006 |
| misra-c2012-15.1 | DP-AUTOSAR-008 |
| misra-c2012-19.2 | DP-AUTOSAR-009 |
| misra-c2012-20.9 | DP-AUTOSAR-010 |
| misra-c2012-5.8 | DP-AUTOSAR-012 |
| misra-c2012-2.7 | DP-AUTOSAR-014 |
| misra-c2012-15.6 | DP-AUTOSAR-015 |
| misra-c2012-11.5 | DP-AUTOSAR-016 |
| misra-c2012-8.8 | DP-AUTOSAR-017 |
| misra-c2012-5.7 | DP-AUTOSAR-018 |
| misra-c2012-10.8 | DP-AUTOSAR-019 |

---

## 5. Excluded Paths

The following paths are excluded from MISRA scanning entirely:

| Path | Reason |
|:-----|:-------|
| `tests/**` | Test code — not production |
| `src/**/*_test.c` | Unit test files |
| `src/**/*_test.h` | Unit test headers |
| `src/**/legacy/**` | Legacy code scheduled for rework |
| `third_party/**` | Third-party dependencies |
| `build/**` | Build artifacts |
| `Drivers/**` | Vendor drivers |
| `Middlewares/**` | Vendor Middleware |
| `CMSIS/**` | CMSIS headers |

---

## 6. Code Categories

| Category | Paths | Action | Blocks CI |
|:---------|:------|:-------|:----------|
| Template | `src/yuleosh/templates/**` | Exclude | No |
| Third-party | `third_party/**`, `Drivers/**`, `Middlewares/**`, `CMSIS/**`, `vendor/**` | Alert | No |
| Business | `src/**` | Enforce | No |

---

## 7. Compliance Verification

### Verification Methods

| Method | Status | Evidence |
|:-------|:-------|:---------|
| Static Analysis (cppcheck) | ✅ Active | `.yuleosh/reports/misra-report.json` |
| Raw Output | ✅ Active | `.yuleosh/reports/misra-raw-output.txt` |
| CI L1 Gate | ✅ Active | `fail_threshold=13000` |
| Deviation Config | ✅ Active | `.yuleosh/ci-config.yaml` |

### Review Sign-off

| Role | Status |
|:-----|:-------|
| Safety Manager | Baseline accepted Phase 1 |
| Software Architect | Baseline accepted Phase 1 |
| Compliance Officer | Baseline accepted Phase 1 |

---

## 8. Change History

| Version | Date | Author | Changes |
|:--------|:-----|:-------|:--------|
| 2.0 | 2026-07-26 | CI Team | Full rewrite: reflect `.yuleosh/ci-config.yaml` deviations with 19 safety-profile DPs |
| 1.0 | 2026-04-29 | Compliance Team | Initial deviation permits for COM module |

---

## 9. References

1. MISRA C:2012 Guidelines for the Use of the C Language in Critical Systems
2. MISRA C:2012 Amendment 2
3. AUTOSAR R21-11 — Classic Platform
4. ISO 26262-6:2018 Road vehicles — Functional safety
5. `.yuleosh/ci-config.yaml` — Live deviation configuration
6. `tools/run_misra_check.sh` — MISRA check script
7. `tools/misra/fix_from_report.py` — Automated fix tool
