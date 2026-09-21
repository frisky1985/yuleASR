# Fix: Det Module ID Conflict Resolution

## Status
- **Phase**: Design
- **Priority**: P0 (Release Blocker)
- **Created**: 2026-09-03

## Problem Statement

Det (Default Error Tracer) uses Module ID to identify the error source. When multiple modules share the same ID, Det cannot distinguish them, making diagnostic error tracing unreliable.

### Verified Conflicts (20+ cross-module collisions)

| ID Value | Conflicting Modules | Severity |
|----------|-------------------|----------|
| 0x10 | WDG vs ICU_LCFG vs ICU_PRIVATE | Medium (ICU internal IDs wrong) |
| 0x12 | BswM vs ComM | High (both active in startup) |
| 0x16 | Icu vs MemIf (ECUAL+Services) | High |
| 0x29 | DCM (new) vs DIO | High (DCM should be 0x35) |
| 0x31 | EA vs PDUR (IsoTP) | Medium |
| 0x43 | EthSM (ECUAL) vs SoAd (Services) | High |
| 0x4D | IpduM (ECUAL) vs DoCan | Medium |
| 0x50 | CAN vs SOAD (legacy) vs SWC | High |
| 0x55 | TcpIp vs FiM (Services) | Medium |
| 0x70 | RTE vs EthIf vs SomeIp vs Csm | Critical (4 modules!) |
| 0x71 | FiM (ECUAL) vs SD vs SomeIpSd (Services) | Critical (3 modules) |
| 0x7A | SPI vs OCU vs IoHwAb | High (3 modules) |
| 0x7B | PWM vs SomeIpXf | Medium |
| 0x7C | CryIf vs SomeIpTp | Medium |
| 0x81 | CDD_RamEcc vs S32K312_HSM vs SomeIpSd (ECUAL) | Critical (3 modules) |
| 0x82 | CDD_Lockstep vs SomeIpIf vs SWC_DiagMgr | Critical (3 modules) |
| 0x83 | CDD_Safety vs SWC_CommMgr | High |
| 0x84 | CDD_Boot vs SWC_StorageMgr | High |
| 0x85 | CDD_FVM vs SWC_IOControl | High |
| 0x8A | EthSM (Services) vs RamTst (Services) | Medium |
| 0x90 | Srp vs LinTp (Services) | Medium |

### Root Cause
No centralized Module ID registry exists. Each module independently chose its ID value without global coordination.

## Design

### Principle
1. **Single source of truth**: One header file defines all Module IDs
2. **AUTOSAR-aligned**: Use standard AUTOSAR Module ID values where defined
3. **Non-overlapping ranges**: Each layer gets its own ID range
4. **Non-breaking**: Existing code keeps working; only the `#define` source changes

### ID Range Allocation

| Range | Layer | Example |
|-------|-------|---------|
| 0x00-0x0F | OS & Core | OS=0x01, Det=0x02 |
| 0x10-0x2F | MCAL Drivers | Wdg=0x10, Icu=0x16, Dio=0x29, Mcu=0x2B |
| 0x30-0x4F | ECUAL | CanIf=0x3C, CanTp=0x3D, EthIf=0x40 |
| 0x50-0x6F | Services | Can=0x50→MCAL!, Com=0x1E, Dcm=0x35 |
| 0x70-0x7F | Middleware | RTE=0x70, DDS=0x7E |
| 0x80-0x8F | CDD | Cdd_Hsm=0x80, Cdd_RamEcc=0x81 |
| 0x90-0x9F | Safety/Custom | Srp=0x90, RamSafety=0x91 |
| 0xA0-0xAF | Network Mgmt | CanSm=0xA0, EthSM=0xA1 |
| 0xB0-0xBF | Application/SWC | SwC_Engine=0xB0, SwC_Vehicle=0xB1 |
| 0xC0-0xCF | Diagnostic | DoIP=0xC0, DoCan=0xC1 |
| 0xD0-0xDF | Measurement | Xcp=0xD0 |
| 0xE0-0xEF | Security | SecOC=0xE0, Csm=0xE1, CryIf=0xE2, KeyM=0xE3 |
| 0xF0-0xFF | Protection | E2E=0xF0, Crc=0xF1 |

### Implementation Strategy

1. Create `src/common/include/ModuleId.h` with all unique IDs
2. Each conflicting module header: replace local `#define XXX_MODULE_ID` with `#include "ModuleId.h"` and use the centralized value
3. Non-conflicting modules: leave unchanged (no risk)
4. Add compile-time duplicate detection macro

### Changes Required

**New file:**
- `src/common/include/ModuleId.h` — centralized registry

**Modified files (only conflicting modules, ~30 headers):**
- Each conflicting module's header gets its MODULE_ID from ModuleId.h
- Source files using the MODULE_ID macro need no changes (same macro name)

## Risk Assessment
- **Low risk**: Macro names unchanged, only values change
- **Det history**: Old Det entries will have different module associations (acceptable for development phase)
- **Test impact**: Tests checking specific MODULE_ID values need updating

## Verification
- Compile-time assertion: no two MODULE_IDs share the same value
- Grep validation: `grep -rh 'MODULE_ID.*0x' src/ | sort` shows no duplicates
- All existing tests pass after ID value updates
