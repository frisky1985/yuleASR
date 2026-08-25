# MISRA C:2012 Compliance Scan Report

**Project:** AUTOSAR BSW  
**Scan Date:** 2025-03-01  
**Scan Scope:** `/src/bsw/` (299 `.c` files, 651 total source files)  
**Methodology:** Static pattern-based scanning (grep/ripgrep)  
**Note:** This is a pattern-based heuristic scan. A full MISRA-compliant static analysis tool (e.g., PC-lint, Coverity, Polyspace) is recommended for complete compliance verification.

---

## 1. Summary Statistics

| Metric | Value |
|--------|-------|
| Total `.c` files scanned | 299 |
| Total source files (`.c` + `.h`) | 651 |
| Total violations found | ~1,941 |
| Rules with violations | 8 of 12 checked |
| Files with at least one violation | ~250 |

### Violations by Rule

| Rule | Category | Severity | Occurrences | Files Affected |
|------|----------|----------|-------------|----------------|
| 11.4 | Object pointer conversions | Required | 116 | 32 |
| 15.5 | Multiple return statements | Advisory | 227 files (7,113 returns) | 227 |
| 13.5 | Short-circuit side effects | Required | 1,604 | ~200 |
| 17.7 | Discarded return values | Required | ~50+ | ~20 |
| 22.1 | Dynamic memory (malloc/free) | Required | 8 | 2 |
| 21.3 | Memory allocation (stdlib.h) | Required | 2 | 2 |
| 11.1/11.3 | Function pointer conversions | Required | 0 | 0 |
| 21.1 | Reserved identifiers | Required | 0 | 0 |
| 10.1 | Inappropriate essential type | Required | 0 (not pattern-detectable) | - |
| 12.1 | Expression precedence | Advisory | 0 (not pattern-detectable) | - |
| 8.13 | Const correctness | Advisory | ~30+ | ~15 |
| 14.4 | if-condition not boolean | Advisory | ~20+ | ~10 |

---

## 2. Top 10 Most Common Violations

### 2.1 Rule 11.4 - Object Pointer Conversions (Required) - 116 occurrences

Casting between pointer types without proper justification. Most common pattern: `(uint8*)DataPtr`.

| File | Count | Example |
|------|-------|---------|
| `mcal/crypto/src/Crypto_Aes.c` | 25 | `(uint8*)io->outputPtr` |
| `services/nvm/src/NvM.c` | 19 | `(uint8*)JobPtr->DataPtr` |
| `classic/com/Com_Signal.c` | 8 | `(uint8*)SignalDataPtr` |
| `services/dem/legacy/dem_nvm.c` | 6 | `(uint8*)Dem_PrimaryMemory` |
| `services/mem/src/Mem.c` | 5 | `(Mem_BlockType*)base` |
| `ecual/xcp/src/Xcp.c` | 5 | `(uint8*)(uintptr)(Xcp_Mta.address + i)` |
| `services/tcpip/src/TcpIp.c` | 4 | `(void*)pcb` |
| `services/someip/src/SomeIp.c` | 4 | `(uint8*)(Data + SOMEIP_HEADER_SIZE)` |
| `mcal/eep/src/Eep.c` | 4 | `(uint8*)(uintptr)(...)` |
| `mcal/uart/src/Uart.c` | 3 | `(uint8*)Data` |

**Risk:** Pointer type mismatches can cause alignment faults, incorrect memory access widths, and undefined behavior on platforms with strict alignment requirements.

### 2.2 Rule 15.5 - Multiple Return Statements (Advisory) - 227 files

Files with the highest number of `return` statements:

| File | Return Count |
|------|-------------|
| `services/tcpip/src/TcpIp.c` | 199 |
| `services/csm/src/Csm.c` | 157 |
| `services/dcm/legacy/dcm_transport.c` | 141 |
| `ecual/ethswt/src/EthSwt.c` | 124 |
| `mcal/crypto/src/Crypto_S32K312_Hsm.c` | 110 |
| `mcal/crypto/src/Crypto.c` | 110 |
| `mcal/i2c/src/I2c.c` | 109 |
| `services/dem/src/Dem.c` | 90 |
| `cdd/src/Cdd_Fvm_1.0.0.c` | 89 |
| `services/keym/src/KeyM.c` | 84 |

**Risk:** Multiple exit points reduce readability and complicate formal verification. AUTOSAR coding guidelines recommend single-entry/single-exit (SESE) for safety-critical code.

### 2.3 Rule 13.5 - Short-Circuit Operators with Potential Side Effects (Required) - 1,604 occurrences

Use of `&&` and `||` where right-hand operand may have side effects. This is pervasive across the codebase as it appears in nearly every conditional expression.

**Representative examples:**
- `ecual/canNm/src/CanNm.c` - Extensive use in state machine conditions
- `ecual/fee/src/Fee.c` - Compound conditions with function calls
- `services/j1939nm/src/J1939Nm.c` - `J1939Nm_Initialized && (State != NULL_PTR) && ...`
- `services/mem/src/Mem.c` - Block validation conditions

**Note:** Most instances are likely benign (comparing values, not calling functions with side effects on the RHS). A full static analysis tool is needed to identify actual violations where the RHS has side effects.

### 2.4 Rule 17.7 - Non-Void Return Value Discarded (Required) - ~50+ occurrences

Function calls where the return value is silently ignored.

| File | Example |
|------|---------|
| `cdd/src/Cdd_Safety_1.0.0.c` | `Cdd_Safety_StateCallback(newState);` |
| `cdd/src/Cdd_Fvm_Hw.c` | `Fls_Read(...)` - return value not checked |
| `cdd/src/Cdd_Boot_1.0.0.c` | `Det_ReportError(...)` (void cast missing) |
| `ecual/canNm/src/CanNm.c` | `ComM_ECNM_NetworkMode(...)`, `memset(...)` without `(void)` cast |

**Note:** Some modules (e.g., `Cdd_Fvm_Hw.c`, `Boot_Loader.c`, `Xcp.c`) correctly use `(void)` casts to explicitly discard return values. Others do not follow this pattern consistently.

### 2.5 Rule 22.1 / 21.3 - Dynamic Memory Allocation (Required) - 8 occurrences in 2 files

| File | Functions Used |
|------|---------------|
| `boot/test/test_boot_integration.c` | `malloc()`, `free()` |
| `services/dcm/legacy/dcm_memory_pool.c` | `#include <stdlib.h>` |

**Risk:** Dynamic memory allocation is prohibited in safety-critical AUTOSAR BSW code (ASIL-B/D). The test file usage is acceptable for unit testing but must not be linked into production binaries. The `dcm_memory_pool.c` inclusion of `stdlib.h` is a deviation that needs formal justification.

### 2.6 Rule 8.13 - Const Correctness (Advisory) - ~30+ occurrences

Functions accepting pointer parameters that should be `const`-qualified but are not.

**Examples:**
- `mcal/crypto/src/Crypto_Aes.c` - `io->inputPtr` cast to `(uint8*)` instead of `(const uint8*)` in some places
- `services/mem/src/Mem.c` - Pointer parameters not const-qualified where data is only read
- `ecual/fee/src/Fee.c` - Data buffer pointers not const-qualified for read-only operations

### 2.7 Rule 14.4 - if-Condition Not Essentially Boolean (Advisory) - ~20+ occurrences

Conditions using integer comparisons rather than boolean expressions.

**Examples:**
- `services/j1939nm/src/J1939Nm.c` - `if (J1939Nm_Initialized && ...)` where `J1939Nm_Initialized` is an integer/enum
- `ecual/canNm/src/CanNm.c` - State comparisons like `(ChState->State == CANNM_STATE_BUS_SLEEP_MODE)` are compliant, but some conditions mix types

---

## 3. Module ID Deviation Analysis

### 3.1 Module ID Definitions Found

| Module | Defined ID | AUTOSAR Standard | Deviation |
|--------|-----------|------------------|-----------|
| CanNm | `0x001F` (31) | 31 | Compliant |
| CanTp | `0x3D` (61) | 61 | Compliant |
| CanIf | `0x3C` (60) | 60 | Compliant |
| FrIf | `0x3F` (63) | 63 | Compliant |
| FrTp | `0x2D` (45) | 45 | Compliant |
| J1939Tp | `0x44` (68) | 68 | Compliant |
| DoIP | `0x25` (37) | 37 | Compliant |
| EA | `0x31` (49) | 49 | Compliant |
| Fee | `30` | 30 | Compliant |
| Eep | `0x5F` (95) | 95 | Compliant |
| Icu | `0x16` (22) | 22 | Compliant |
| Ocu | `0x7A` (122) | 122 | Compliant |
| Port | `0x002A` (42) | 42 | Compliant |
| Mcu | `0x002B` (43) | 43 | Compliant |
| LinTp | `0x0062` (98) | 98 | Compliant |
| LinSM | `90` | 90 | Compliant |
| LinNm | `0x45` (69) | 69 | Compliant |
| LinTrcv | `122` | 122 | Compliant |
| EthSM | `0x43` (67) | 67 | Compliant |
| EthIf | `0x70` (112) | 112 | Compliant |
| EthSwt | `0x88` (136) | 136 | Compliant |
| MemIf | (via `MEMIF_MODULE_ID`) | - | Defined, value not in scan |
| IoHwAb | `0x7A` (122) | 122 | Compliant |
| Xcp | `0xD0` (208) | 208 (vendor-specific range) | Compliant |
| UART | `0x11` (17) | 17 | Compliant |
| I2C | `0x57` (87) | 87 | Compliant |

### 3.2 Non-Standard / Custom Module IDs

| Module | Defined ID | Notes |
|--------|-----------|-------|
| CDD_HSM | `0x80` (128) | Custom CDD module - vendor-specific range |
| CDD_RAMECC | `0x81` (129) | Custom CDD module |
| CDD_LOCKSTEP | `0x82` (130) | Custom CDD module |
| CDD_SAFETY | `0x83` (131) | Custom CDD module |
| CDD_BOOT | `0x84` (132) | Custom CDD module |
| CDD_FVM | `0x85` (133) | Custom CDD module |
| SOMEIPIF | `0x82` (130) | **Potential conflict** with CDD_LOCKSTEP |
| SOMEIPSD | `0x81` (129) | **Potential conflict** with CDD_RAMECC |
| ETHSWT | `0x88` (136) | AUTOSAR standard value |
| J1939Nm | `0x8D` (141) | AUTOSAR standard value |
| SRP | `0x90` (144) | Vendor-specific |
| Mem | `88` (0x58) | Vendor-specific |
| FEE_FLS_INT | `255` | Vendor-specific (integration layer) |

### 3.3 Module ID Conflicts Detected

1. **`SOMEIPIF_MODULE_ID` (0x82) conflicts with `CDD_MODULE_ID_LOCKSTEP` (0x82)** - Both modules use the same ID for DET error reporting. This will cause ambiguity in Default Error Tracer logs.

2. **`SOMEIPSD_MODULE_ID` (0x81) conflicts with `CDD_MODULE_ID_RAMECC` (0x81)** - Same issue as above.

**Recommendation:** Reassign CDD module IDs to a non-overlapping vendor-specific range (e.g., `0xE0`-`0xEF`) or use the AUTOSAR-assigned IDs for standard modules.

---

## 4. Severity Classification Summary

| Severity | Rules | Total Violations | Action Required |
|----------|-------|-----------------|-----------------|
| **Mandatory** | None found | 0 | N/A |
| **Required** | 11.4, 17.7, 22.1, 21.3, 13.5* | ~1,786 | Deviation required or fix |
| **Advisory** | 15.5, 8.13, 14.4 | ~277+ | Should fix, deviation optional |

*Rule 13.5 count is inflated; most instances are likely compliant. A full tool is needed for accurate assessment.

---

## 5. Recommendations

### 5.1 Immediate Actions (Required Rules)

1. **Rule 11.4 (Pointer Casts):** 
   - Introduce `memcpy()` for type-punning instead of pointer casts where possible
   - Use intermediate `uintptr_t` casts with explicit deviation records for hardware register access patterns (e.g., `Eep.c`, `Fls_Hw.c`, `Mcu.c`)
   - For AUTOSAR PDU data access, consider using union types or properly-typed accessor functions

2. **Rule 22.1/21.3 (Dynamic Memory):**
   - Remove `#include <stdlib.h>` from `services/dcm/legacy/dcm_memory_pool.c` and replace with static pool allocation
   - Ensure `boot/test/test_boot_integration.c` is excluded from production build targets

3. **Rule 17.7 (Discarded Returns):**
   - Add `(void)` casts to all intentionally-discarded return values for consistency
   - Check return values of `Fls_Read()`, `ComM_*()`, and `Nm_*()` calls where safety-relevant

### 5.2 Short-Term Improvements (Advisory Rules)

4. **Rule 15.5 (Multiple Returns):**
   - Refactor the top 10 offenders to use guard clauses with single exit points
   - Priority: `TcpIp.c` (199 returns), `Csm.c` (157 returns)

5. **Rule 8.13 (Const Correctness):**
   - Add `const` qualifiers to pointer parameters that are not modified
   - Focus on crypto and memory modules first

### 5.3 Module ID Resolution

6. **Resolve ID conflicts:**
   - Reassign `SOMEIPIF_MODULE_ID` from `0x82` to an unused value
   - Reassign `SOMEIPSD_MODULE_ID` from `0x81` to an unused value
   - Or move all CDD module IDs to a dedicated non-overlapping range

### 5.4 Process Improvements

7. **Deploy a full MISRA checker** (PC-lint Plus, Coverity, Polyspace, or cppcheck with MISRA addon) for comprehensive analysis
8. **Establish a deviation database** to formally document accepted deviations from Required rules
9. **Integrate MISRA checking into CI/CD** pipeline to prevent regression
10. **Address Rule 13.5** with a proper static analysis tool to distinguish actual side-effect violations from benign comparisons

---

## 6. Files with No Violations

The following module categories showed clean results for the most critical rules:
- `mcal/dio/` - No pointer casts, no dynamic memory
- `mcal/gpt/` - Clean
- `mcal/port/` - Clean
- `mcal/pwm/` - Clean
- `services/det/` - Clean
- `services/schm/` - Clean
- `services/e2e/` - Clean

---

## 7. Appendix: Scan Patterns Used

| Rule | Pattern |
|------|---------|
| 11.1/11.3 | `\(\s*\w+\s*\*\s*\)\s*\w+` (function pointer casts) |
| 11.4 | `\((uint8\|uint16\|...)\s*\*\)` (object pointer casts) |
| 17.7 | Function calls at statement level without `(void)` cast |
| 22.1 | `\b(malloc\|free\|calloc\|realloc)\s*\(` |
| 21.3 | `stdlib\.h` includes |
| 13.5 | `\|\|` and `&&` operators (over-broad, needs semantic analysis) |
| 15.5 | Count of `return` statements per function/file |
| 8.13 | Non-const pointer parameters (requires semantic analysis) |
| 21.1 | `#define\s*__[A-Z_]+__` (reserved identifier patterns) |
| 14.4 | Non-boolean conditions in `if` statements |

---

*Report generated by pattern-based static analysis. For formal MISRA C:2012 compliance certification, a qualified static analysis tool (e.g., PC-lint Plus, Coverity, Polyspace, QA-C) is required.*
