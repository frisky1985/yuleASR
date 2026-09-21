# Design: Fix EcuM Unit Tests, ModuleId Registry, and MCAL Test Build Path

## Objective

Fix the defects found during review of the current working tree so that the
substantiated EcuM/DIO/WDG unit tests actually compile, run, and pass without
breaking the existing AUTOSAR framework.

## Verified defects

1. **EcuM test API mismatch**  
   `tests/bsw/services/ecum/test_ecum.c` calls `EcuM_RequestRun()` and
   `EcuM_ReleaseRun()`.  The header declares `EcuM_RequestRUN(EcuM_UserType)`
   and `EcuM_ReleaseRUN(EcuM_UserType)` (uppercase `RUN` + user argument).
   This is a compile error.

2. **EcuM state-machine expectations are wrong**  
   `EcuM_Init()` in `src/bsw/services/ecum/src/EcuM.c` calls
   `EcuM_StartupOne()`, which automatically calls `EcuM_StartupTwo()` and
   ends in `ECUM_STATE_RUN`.  The tests expect `ECUM_STATE_STARTUP` after
   `EcuM_Init()` and call `EcuM_StartupOne()` after `EcuM_Init()`, which
   triggers `ECUM_E_WRONG_API_ORDER`.

3. **`EcuM_Shutdown()` hangs the test runner**  
   `EcuM_PerformShutdown()` calls `EcuM_AL_SwitchOff()` and then enters
   `while(1)`.  The default weak implementation of `EcuM_AL_SwitchOff()`
   returns, so the test process never returns from `EcuM_Shutdown()`.

4. **`ModuleId.h` registry is incomplete and collides**  
   - Missing entry for `MODULE_ID_ECUM` (0x0A, used by `EcuM.c`).
   - Missing entry for `MODULE_ID_MEMIF_SERVICES` (0x4C, used by
     `src/bsw/services/memif`).
   - `MODULE_ID_ETHTSYN` currently occupies 0x0A in the registry, but
     `src/bsw/services/ethtsyn/include/EthTSyn.h:17` hardcodes
     `ETHTSYN_MODULE_ID 0x0AUL`, which collides with EcuM.  EcuM is core
     startup/shutdown logic and keeps the standard 0x0A value; EthTSyn
     must be moved.

5. **`ModuleId.h` is not consumed by module headers**  
   No BSW header currently `#include "ModuleId.h"`; values are still
   hardcoded with comments pointing to the registry.  This allows the
   registry and the headers to drift.

6. **Substantiated MCAL tests are not built**  
   `tests/mock/CMakeLists_MCAL_Tests.txt` expects test sources in
   `tests/mock/test_mcal_*.c`, but the new DIO/WDG tests are in
   `tests/bsw/mcal/dio/test_dio.c` and `tests/bsw/mcal/wdg/test_wdg.c`.
   The substantiated tests are currently dead code.

## Proposed changes

### 1. EcuM tests (`tests/bsw/services/ecum/test_ecum.c`)

- Rename `EcuM_RequestRun()` → `EcuM_RequestRUN(0U)`.
- Rename `EcuM_ReleaseRun()` → `EcuM_ReleaseRUN(0U)`.
- After `EcuM_Init()`, expect `ECUM_STATE_RUN` instead of
  `ECUM_STATE_STARTUP`.
- Remove the sequence `EcuM_Init(); EcuM_StartupOne(); EcuM_StartupTwo();`
  and replace it with `EcuM_Init();` only, because `Init` already performs
  the full startup sequence.
- For shutdown tests, provide a test-local override of `EcuM_AL_SwitchOff()`
  that records it was called and then returns.  This prevents the infinite
  loop while still allowing the test to verify that `EcuM_Shutdown()`
  transitions the state to `ECUM_STATE_SHUTDOWN` before the callout.
  The test will assert state **before** the call to `EcuM_Shutdown()` or
  verify state inside the override by reading the global state variable.
  To keep the test deterministic, add a helper that calls the internal
  shutdown steps up to (but not including) `EcuM_PerformShutdown()`.
  If no such helper exists, the test will instead:
  1. Initialise EcuM.
  2. Call `EcuM_SelectShutdownTarget(ECUM_SHUTDOWN_TARGET_OFF, 0U)`.
  3. Override `EcuM_AL_SwitchOff` to set a flag.
  4. Call `EcuM_Shutdown()` — because the override returns, the loop is
     reached.  The test cannot assert after the loop, so the override will
     itself assert that the current state is `ECUM_STATE_SHUTDOWN` before
     returning.

### 2. `ModuleId.h` (`src/common/include/ModuleId.h`)

- Add `MODULE_ID_ECUM 0x0AU` in the Core section.
- Add `MODULE_ID_MEMIF_SERVICES 0x4CU` in the Services section.
- Move `MODULE_ID_ETHTSYN` from 0x0A to 0x0DU to avoid collision with EcuM.

### 2a. EthTSyn header (`src/bsw/services/ethtsyn/include/EthTSyn.h`)

- Add `#include "ModuleId.h"`.
- Change `ETHTSYN_MODULE_ID 0x0AUL` to `ETHTSYN_MODULE_ID MODULE_ID_ETHTSYN`.

### 3. Consume the registry in module headers and SWC sources

- Add `#include "ModuleId.h"` to every BSW header and application SWC file
  that already references the registry in comments and defines a
  `XXX_MODULE_ID` macro.  Keep the existing macro names; map each one to the
  canonical `MODULE_ID_xxx` value:
  - ECUAL: `EthIf.h`, `MemIf.h` (ECUAL), `EthSM.h` (ECUAL), `FiM.h` (ECUAL),
    `IoHwAb.h`, `SomeIpIf.h`, `SomeIpSd.h` (ECUAL)
  - MCAL/Services: `Crypto_S32K312_Hsm.h`, `Ocu.h`, `SomeIp.h`,
    `Csm_Types.h`, `Dcm.h`, `BswM.h`, `CryIf.h`, `DoCan.h`, `DoIP.h`,
    `EthSM.h` (Services), `FiM.h` (Services), `LinTp.h` (Services),
    `MemIf.h` (Services)
  - Application SWC: `Swc_EngineControl.c`, `Swc_VehicleDynamics.c`,
    `Swc_DiagnosticManager.h`, `Swc_CommunicationManager.c`,
    `Swc_StorageManager.c`, `Swc_IOControl.c`, `Swc_ModeManager.c`,
    `Swc_WatchdogManager.c`

### 4. MCAL test build path (`tests/mock/CMakeLists_MCAL_Tests.txt`)

- Change `MCAL_TEST_SRC_DIR` from `${CMAKE_CURRENT_SOURCE_DIR}/mock` to
  `${CMAKE_CURRENT_SOURCE_DIR}/bsw/mcal` so that the glob finds
  `tests/bsw/mcal/dio/test_dio.c` and `tests/bsw/mcal/wdg/test_wdg.c`.
- The existing `if(EXISTS ...)` blocks for DIO and WDG will then pick up
  the new files automatically because they use the same target names.
- This preserves the existing CMake structure and does not delete any
  source files.

## Scope and non-scope

**In scope:**
- Correcting the EcuM test to match the real implementation.
- Completing `ModuleId.h` with the two missing entries and resolving the
  EthTSyn/EcuM collision.
- Making module headers consume `ModuleId.h`.
- Wiring the new DIO/WDG tests into the existing CMake targets.

**Not in scope:**
- Rewriting the EcuM production state machine.
- Deleting or relocating the existing `tests/mock/` directory.
- Refactoring the broader module-ID migration across files not already
  touched in the current working tree.

## Verification

- `cmake -DBUILD_TESTING=ON -S . -B build-test && cmake --build build-test`
  must succeed.
- `ctest --test-dir build-test -R ecum` must pass.
- `ctest --test-dir build-test -R mcal_dio_test` must pass.
- `ctest --test-dir build-test -R mcal_wdg_test` must pass.

## References

- `src/bsw/services/ecum/src/EcuM.c:152-295` — startup sequence
- `src/bsw/services/ecum/src/EcuM.c:820-835` — shutdown infinite loop
- `src/bsw/services/ecum/include/EcuM.h:275-277` — request/run prototypes
- `tests/mock/CMakeLists_MCAL_Tests.txt:184-186` — MCAL test source path
