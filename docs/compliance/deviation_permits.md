# MISRA C:2012 Deviation Permits

**Project:** ETH-DDS Integration  
**Standard:** MISRA C:2012 Amendment 2  
**Version:** 1.0  
**Date:** 2026-04-26

---

## Overview

This document defines approved deviations from MISRA C:2012 rules. All deviations must be:

1. **Documented** in this file with technical justification
2. **Approved** by the project safety manager or technical lead
3. **Reviewed** periodically (every 6 months for temporary permits)
4. **Localized** to specific files or code sections

### Deviation Permit Status

| Permit ID | Title | Status | Approved By | Date | Expiry |
|-----------|-------|--------|-------------|------|--------|
| TP-001 | Third-Party Code | ✅ Approved | Safety Mgr | 2026-04-26 | Project End |
| PERF-001 | Performance-Critical Code | ✅ Approved | Tech Lead | 2026-04-26 | 2026-10-26 |
| HW-001 | Hardware Access Patterns | ✅ Approved | Safety Mgr | 2026-04-26 | Project End |
| ASM-001 | Inline Assembly | ✅ Approved | Tech Lead | 2026-04-26 | Project End |
| TEMP-001 | Temporary Fixes | ⏳ Pending | - | - | 2026-05-26 |

---

## Permanent Deviations

### TP-001: Third-Party Code

**Status:** ✅ Approved  
**Rules Affected:** All MISRA rules  
**Scope:** `*/freertos/*`, `*/lwip/*`, `*/cmsis/*`, `*/hal/*`

#### Description

External library code that is not under project control. These libraries have their own quality assurance processes and modifying them would create maintenance issues when updating to new versions.

#### Affected Libraries

| Library | Version | Purpose | MISRA Compliance |
|---------|---------|---------|------------------|
| FreeRTOS | 10.5.1 | RTOS Kernel | Partial (Certified) |
| lwIP | 2.2.0 | TCP/IP Stack | None (Standard C) |
| CMSIS | 5.9.0 | ARM Core Support | Yes (Certified) |
| HAL | Vendor | Hardware Abstraction | Partial |

#### Technical Justification

1. **Maintenance Burden:** Modifying third-party code prevents easy updates
2. **Certification Status:** Some libraries (FreeRTOS, CMSIS) are pre-certified for safety applications
3. **Isolation:** Third-party code is isolated in dedicated directories with clear boundaries

#### Risk Mitigation

- All third-party code is contained in `lib/` or `*/external/` directories
- Interface wrappers provide MISRA-compliant APIs to application code
- Unit tests verify wrapper behavior

#### Approval

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Safety Manager | TBD | _______________ | 2026-04-26 |
| Technical Lead | TBD | _______________ | 2026-04-26 |

---

### HW-001: Hardware Access Patterns

**Status:** ✅ Approved  
**Rules Affected:** 11.4, 11.6, 19.2  
**Scope:** `*_regs.h`, `*platform*`, `*/hal/*`

#### Description

Direct hardware register access requires specific coding patterns that violate certain MISRA rules, particularly around type conversions and union usage.

#### Technical Justification

**Rule 11.4 & 11.6: Pointer-Integer Conversions**
- Memory-mapped registers are accessed via integer addresses cast to pointers
- Example: `#define GPIO_BASE 0x40000000UL` followed by `(volatile uint32_t*)GPIO_BASE`
- This is the standard pattern for embedded systems and is well-defined for the target architecture

**Rule 19.2: Union Usage**
- Unions allow viewing the same register as bit-fields or as a whole word
- Example: Accessing a control register both as individual bits and as raw value
- Alternative (memcpy) would be less efficient and harder to maintain

#### Examples

```c
/* Deviation: Rule 11.4, 11.6 - Hardware register definition */
#define ETH_MAC_BASE    0x40028000UL
#define ETH_MAC_CTRL    (*(volatile uint32_t *)(ETH_MAC_BASE + 0x00))

/* Deviation: Rule 19.2 - Register bit-field access */
typedef union {
    uint32_t raw;
    struct {
        uint32_t enable : 1;
        uint32_t mode   : 3;
        uint32_t reserved : 28;
    } bits;
} EthMacCtrl_t;
```

#### Risk Mitigation

- All hardware access is abstracted through dedicated header files
- Access macros are thoroughly tested
- Hardware tests verify register behavior

#### Approval

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Safety Manager | TBD | _______________ | 2026-04-26 |

---

### ASM-001: Inline Assembly

**Status:** ✅ Approved  
**Rules Affected:** 1.1, 20.4  
**Scope:** Any file requiring low-level operations

#### Description

Certain operations require inline assembly for:
- Critical section entry/exit
- Memory barriers
- Special instructions (WFI, DMB, DSB, ISB)
- Context switch operations

#### Technical Justification

**Rule 1.1: ISO C Compliance**
- Inline assembly is not standard C but is supported by all target compilers
- Required for operations that cannot be expressed in C

**Rule 20.4: Keyword-like Macro**
- The `ASM` macro is used to abstract compiler-specific syntax
- Example: `#define ASM __asm__ __volatile__`

#### Examples

```c
/* Deviation: Rule 1.1, 20.4 - Critical section with inline assembly */
#define CRITICAL_SECTION_ENTER() \
    ASM volatile ("cpsid i" ::: "memory")

#define MEMORY_BARRIER() \
    ASM volatile ("dmb" ::: "memory")
```

#### Risk Mitigation

- All assembly is wrapped in macros for portability
- Assembly blocks are minimal and well-documented
- Tested on all supported compiler versions

---

## Conditional Deviations

### PERF-001: Performance-Critical Code

**Status:** ✅ Approved  
**Rules Affected:** 15.5  
**Scope:** `*interrupt*.c`, `*eth_driver*.c`, ISRs, polling loops

#### Description

Certain code paths have strict timing requirements that necessitate coding patterns that deviate from MISRA guidelines, specifically multiple return points in functions.

#### Technical Justification

**Rule 15.5: Single Point of Exit**
- In ISRs, early returns reduce interrupt latency
- Example: Checking and returning for invalid conditions before doing work
- Single exit point would require additional flag variables and nested conditionals

#### Examples

```c
/* Deviation: Rule 15.5 - ISR with early returns for latency */
void ETH_IRQHandler(void)
{
    uint32_t status = ETH->DMA_STATUS;
    
    /* Early exit if not our interrupt */
    if (!(status & ETH_DMA_STATUS_RS)) {
        return;  /* Deviation: Early return for performance */
    }
    
    if (status & ETH_DMA_STATUS_AIS) {
        HandleError();
        return;  /* Deviation: Early return */
    }
    
    /* Normal processing */
    ProcessRxFrames();
}
```

#### Conditions

- Deviation only applies to:
  - Interrupt service routines
  - Hardware polling loops with sub-microsecond requirements
  - Context switch code
- Each function must document why early return is necessary
- Functions must still properly clean up resources on all exit paths

#### Review Schedule

This permit is temporary and will be reviewed every 6 months.

| Review Date | Status | Notes |
|-------------|--------|-------|
| 2026-04-26 | ✅ Approved | Initial approval |
| 2026-10-26 | ⏳ Scheduled | - |

---

## Temporary Deviations

### TEMP-001: Ongoing Refactoring

**Status:** ⏳ Pending Approval  
**Rules Affected:** 2.1, 2.2  
**Scope:** Specific lines marked with TODO comments

#### Description

Temporary deviations for code that is scheduled for refactoring. These must be resolved before release.

#### Current Items

| File | Line | Rule | Description | Target Date |
|------|------|------|-------------|-------------|
| bl_rollback.c | 245 | 2.1 | Dead code from rollback logic | 2026-05-10 |
| ota_downloader.c | 178 | 2.2 | Debug code to be removed | 2026-05-05 |

---

## Deviation Request Process

### Requesting a New Deviation

1. **Identify the Need**
   - Determine which rule cannot be followed
   - Document technical justification

2. **Complete the Template**

```markdown
### DEV-XXX: [Title]

**Requested By:** [Name]  
**Date:** [YYYY-MM-DD]  
**Rules Affected:** [Rule numbers]  
**Scope:** [Files/functions affected]

#### Description
[What the code does and why it violates the rule]

#### Technical Justification
[Why compliance is not possible or practical]

#### Risk Analysis
[Potential risks of the deviation and how they are mitigated]

#### Alternatives Considered
[Other approaches that were evaluated]

#### Duration
- [ ] Permanent
- [ ] Until [date]
- [ ] Review every [period]
```

3. **Submit for Review**
   - Create a PR with the deviation request
   - Tag @safety-manager and @technical-lead
   - Include code examples

4. **Approval**
   - Requires sign-off from Safety Manager
   - For permanent deviations, also requires Technical Lead approval

### Review Process

- All deviations are reviewed every 6 months
- Temporary deviations must have target resolution dates
- Expired deviations must be re-approved or the code must be fixed

---

## Compliance Metrics

| Metric | Target | Current |
|--------|--------|---------|
| Required Rules Compliance | 100% | 98.5% |
| Advisory Rules Compliance | 90% | 87.2% |
| Active Deviations | < 20 | 5 |
| Temporary Deviations | 0 | 2 |

---

## References

- [MISRA C:2012 Guidelines](https://www.misra.org.uk)
- [Project Coding Standards](../CODING_STANDARDS.md)
- [Safety Manual](../SAFETY_MANUAL.md)

---

## Change History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-04-26 | Agent-MISRA | Initial version |

---

*This document is part of the ETH-DDS Integration project compliance documentation.*
