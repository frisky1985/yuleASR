# Remaining Architecture-Level Compiler Errors

> Generated: 2026-07-30
> Batch of ~123 pre-staged errors: **~90% resolved** on `origin/v1.3.0`.
> Remaining count: **~91 errors** across newly-compiled modules.

---

## ⭐ Priority 1: DoIp.c — DoIP API Mismatch (~20 errors)

**Files:** `src/bsw/services/doip/src/DoIp.c`
**Root cause:** The `DoIP→DoCan` refactoring changed the DoIP header types but the `.c` implementation was not updated.

| Issue | In .c file | In header |
|---|---|---|
| `DoIP_ConnectionType` unknown | Uses as struct | `DoIP_ConnectionStateType` (enum) |
| `DOIP_CON_STATE_CLOSED` | Used | `DOIP_CONN_STATE_CLOSED` (missing N) |
| `DOIP_CON_STATE_ROUTING_ACTIVE` | Used | `DOIP_CONN_STATE_ACTIVE` |
| `DOIP_MAX_PAYLOAD_LENGTH` | Used | Not defined |
| `DOIP_CFG_ANNOUNCE_WAIT` | Used | Not defined |
| `DoIP_Connections[i].TcpSocket` | Struct member | Type is enum, not struct |
| `DoIP_Init(const void*)` | Call | `DoIP_Init(const DoIP_ConfigType*)` |

**Recommendation:** Dedicated refactoring sprint to align DoIp.c with refactored DoIP API. Need `DoIP_ConnectionRuntimeType` struct.

---

## Priority 2: LinTp — Multiple unknown types (~15 errors)

**Files:** `src/bsw/services/lntm/src/LinTp.c`
**Root cause:** LinTp.c uses types (`LinTp_StateType`, `LinTp_NADType`, `LinTp_ChannelType`, `LinTp_ConnectionType`) that are not defined in the headers, or the module interface was refactored.

**Recommendation:** Review LinTp header types vs .c expectations. Likely similar DoIP-pattern refactoring issue.

---

## Priority 3: New Module Stub Errors

| Module | Error | Fix |
|---|---|---|
| `LdCom.h:12` | `'Com_Types.h'` not found | Create stub or add generated header |
| `LinSM.h:90` | `expected identifier` | Likely Vim line-number corruption like DoIp.c/FiM.c |
| `Mem.c:30` | `'SchM_Mem.h'` not found | Create stub (like SchM_EthTSyn.h) |
| `MemIf_Cfg.h:114` | `'Fee.h'` not found | Missing Fee module include |
| `KeyM.c:43` | AR version mismatch `#error` | Update version check |

---

## Priority 4: Minor Signature/Stub Issues

| Module | Error | Fix |
|---|---|---|
| `J1939Tp.c:631` | `PduR_J1939TpRxIndication` signature conflict | Align stub with PduR header |
| `FiM.c:517` | `FIM_MAX_FUNCTIONS` undeclared | Use `FIM_NUM_FUNCTIONS` |

---

## Summary

| Category | Count |
|---|---|
| DoIp.c architecture refactoring | ~20 |
| LinTp type mismatch | ~15 |
| Missing stub/header files | 4 |
| PduR callback signature | 1 |
| FiM missing define | 1 |
| KeyM version check | 1 |
| **Total remaining** | **~42** |

**Progress: 123 original → ~42 remaining (66% resolved, ~81 fixes merged to `origin/v1.3.0`)**

### Fixes already merged in `dc5f7ef`
- Dem: missing #endif, missing Compiler.h include
- Det: missing config macros, missing PtrType typedefs
- DLT: type ordering, duplicate typedef, missing limits
- CanTSyn: type mismatch in TimeBaseId parameter
- DCM: const qualifier discard, array init conflict
- DoIP: include guard fix, missing SoCon types, Vim line-number cleanup
- EcuC: include guard collision in stub, struct init, macro conflicts
- EcuM: missing ConfigPtr arguments
- EthSM: static/extern conflict
- EthTSyn: missing SchM header stub, error code names
- FiM: Vim line-number cleanup, Dem types include, truncated function body
- Std_Types.h: added int8/int16/int32 aliases
- J1939Tp: missing INSTANCE_ID, ComStack_Types include, Cfg circular dep
