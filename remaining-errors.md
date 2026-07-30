# Remaining Architecture-Level Compiler Errors

> Generated: 2026-07-30
> The batch of ~123 pre-staged errors is now **~90% resolved**.
> These remaining issues require dedicated architectural refactoring.

---

## 1. DoIp.c — DoIP API Mismatch (~20 errors)

**Files affected:** `src/bsw/services/doip/src/DoIp.c`
**Root cause:** The `DoIP → DoCan` refactoring changed the DoIP header types but the `.c` implementation was not updated.

**Specific issues:**

| Symbol in .c | Actual in header | Category |
|---|---|---|
| `DoIP_ConnectionType` | `DoIP_ConnectionStateType` (enum) | Type name changed |
| `DOIP_CON_STATE_CLOSED` | `DOIP_CONN_STATE_CLOSED` | Macro name (missing 'N') |
| `DOIP_CON_STATE_ROUTING_ACTIVE` | `DOIP_CONN_STATE_ACTIVE` | Macro name renamed |
| `DOIP_MAX_PAYLOAD_LENGTH` | Not defined anywhere | Missing config const |
| `DOIP_CFG_ANNOUNCE_WAIT` | Not defined anywhere | Missing config const |
| `DoIP_Connections[i].TcpSocket` | `DoIP_ConnectionStateType` is an enum, not struct | Structural mismatch |
| `DoIP_Connections[i].TcpPort` | Same as above | Structural mismatch |
| `DoIP_Init(const void* ConfigPtr)` | `DoIP_Init(const DoIP_ConfigType*)` | Signature mismatch |

**Recommendation:** Dedicated architectural refactoring sprint to align DoIp.c with the refactored DoIP API. The implementation uses connection state as a struct with TCP/UDP fields, but the header defines it as a pure state enum. A `DoIP_ConnectionRuntimeType` struct is needed.

---

## 2. LdCom — Missing Com_Types.h include

**Files affected:** `src/bsw/services/ldcom/include/LdCom.h`
**Error:** `fatal error: 'Com_Types.h' file not found`
**Root cause:** `LdCom.h` includes `Com_Types.h` which is expected from the Com module but may not exist or is not in the include path.

**Recommendation:** Create a stub `Com_Types.h` with the needed type definitions, or verify the Com module generation is complete.

---

## 3. LinSM — Expected identifier syntax error

**Files affected:** `src/bsw/services/linsm/include/LinSM.h:90`
**Error:** `expected identifier`
**Root cause:** Likely same Vim line-number corruption pattern as DoIp.c and FiM.c, or a missing/truncated enum/struct definition.

**Recommendation:** Inspect around line 90 for corrupted content.

---

## 4. LinTp — Multiple unknown type errors (~15 errors)

**Files affected:** `src/bsw/services/lntm/src/LinTp.c`
**Root cause:** The LinTp.c source file uses types (e.g., `LinTp_StateType`, `LinTp_NADType`, `LinTp_ChannelType`) that are not defined in the module headers, or are not properly included.

**Recommendation:** Check if the LinTp module was partially refactored. The header types may have been renamed or the include chain is incomplete.

---

## 5. KeyM — AR version mismatch

**Files affected:** `src/bsw/services/keym/src/KeyM.c:43`
**Error:** `#error "KeyM.c: AR minor version mismatch"`
**Root cause:** Compile-time `#error` triggered by version check `KEYM_AR_RELEASE_MINOR_VERSION != 4u`.

**Recommendation:** Update the version check or the `KEYM_AR_RELEASE_MINOR_VERSION` definition to match. Consider changing to `#warning` during development.

---

## 6. J1939Tp — Function signature conflict

**Files affected:** `src/bsw/services/j1939tp/src/J1939Tp.c:631`
**Error:** `conflicting types for 'PduR_J1939TpRxIndication'`
**Root cause:** The Stub implementation of `PduR_J1939TpRxIndication` in J1939Tp.c has a different signature than declared in the PduR module header.

**Recommendation:** Align the stub function signature with PduR's declaration.

---

## Summary

| Module | Errors | Type | Fix |
|---|---|---|---|
| DoIp.c | ~20 | Architecture refactoring | ⭐ High priority |
| LdCom | 1 | Missing include | Low |
| LinSM | 1 | Syntax error | Low |
| LinTp | ~15 | Undefined types | Medium |
| KeyM | 1 | Version check | Low |
| J1939Tp | 1 | Signature mismatch | Low |
| **Total remaining** | **~39** | | |

**Progress: ~120 errors → ~39 errors (68% reduction, 84 fix commits pushed)**
