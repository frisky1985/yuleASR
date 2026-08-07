# Branch Coverage Report — ASIL Modules (2026-08-07, batch E)

> **Scope**: E2E / NvM / WdgM / Com / Crc / Os (timing protection) / Det
> **Method**: native host test binaries linking **production sources from `src/`**
> (unchanged) compiled with `--coverage -fprofile-arcs -ftest-coverage`,
> measured with `lcov --branch-coverage` (BRDA records).
> **Reproduce**: `bash tools/run_branch_coverage.sh`
> **Commits**: 1aa9a994 (NvM), 5e48ff26 (Com), 4750aa6a (E2E), 63fa527a (Wdgm/Det/Os)

## Totals (production src/ only)

| Metric | Found | Hit | Rate | Batch D | Δ |
|:-------|------:|----:|-----:|--------:|--:|
| Lines | 2881 | 2638 | **91.57%** | 43.25% | +48.32pp |
| Branches | 1247 | 995 | **79.79%** | 31.19% | +48.60pp |
| Functions | 199 | 192 | 96.48% | 66.33% | +30.15pp |

**SWE.4.BP1 gate (statement ≥80%, branch ≥70%): ✅ MET on both axes.**

## Per-module data (batch E 当日实测)

| Module | File | Line % | Branch % |
|:-------|:-----|-------:|---------:|
| Os | `src/bsw/os/src/Os_TimingProtection.c` | 100.00% | 95.35% |
| Det | `src/bsw/services/det/src/Det.c` | 100.00% | 88.46% |
| E2E | `src/bsw/services/e2e/src/E2E_P02.c` | 100.00% | 83.78% |
| E2E | `src/bsw/services/e2e/src/E2E_P04.c` | 100.00% | 88.57% |
| Com | `src/bsw/services/com/src/Com.c` | 96.52% | 80.09% |
| NvM | `src/bsw/services/nvm/src/NvM_EccHandler.c` | 96.15% | 89.26% |
| E2E | `src/bsw/services/e2e/src/E2E_P06.c` | 94.24% | 83.64% |
| E2E | `src/bsw/services/e2e/src/E2E_P07.c` | 94.40% | 83.64% |
| Crc | `src/bsw/services/crc/src/Crc.c` | 92.50% | 100.00% |
| NvM | `src/bsw/services/nvm/src/NvM.c` | 88.69% | 74.34% |
| WdgM | `src/bsw/services/wdgm/src/Wdgm.c` | 85.20% | 77.36% |
| E2E | `src/bsw/services/e2e/src/E2E_P05.c` | 81.48% | 70.18% |
| E2E | `src/bsw/services/e2e/src/E2E_P01.c` | 98.15% | 87.23% |
| WdgM | `src/bsw/services/wdgm/src/WdgM_Cfg.c` | 66.67% | 30.00% |
| Det | `src/bsw/services/det/src/Det_Lcfg.c` | 0.00% | 0.00% |

Module-level notes:
- **Det_Lcfg.c 0%**: the Lcfg hook tables (`Det_ErrorHooks[]` etc.) are
  link-time configuration **not consumed by Det.c** — Det.c maintains its own
  internal hook arrays populated at runtime. Lcfg tables are dead configuration
  data; lines are function definitions never referenced (genuine gap, P2).
- **WdgM_Cfg.c 66.67%/30%**: `WdgM_WatchdogSetMode(OFF)` case and the
  safety-event callback cases 0x01/0x02/0x03/0x05 (supervision expired,
  lockstep, ramsafety, reset) are reachable only through the emergency-reset
  path (`WdgM_PerformReset` → `for(;;)`) which by design cannot run in a test
  binary; mode-change event (0x04) is covered.

## Genuine remaining gaps (real, not masked)

1. **Com.c `uint8` loop counter vs `COM_NUM_OF_SIGNALS=256`** —
   `Com_Init` uses `uint8 i` in `for (i=0U; i<COM_NUM_OF_SIGNALS; i++)`;
   with the production config (256 signals) the counter wraps at 255 and the
   loop never terminates on a host build. Test binary uses a host config
   override (`COM_NUM_OF_SIGNALS=8`, `coverage_run/asil/com_cfg_override.h`).
   Production `Com_Cfg.h`/`Com.c` untouched. **P1 finding — needs src fix.**
2. **E2E_Init / E2E_DeInit declared in E2E.h but not implemented** in
   `src/bsw/services/e2e/src/E2E.c` (file is an empty stub). Host stubs used;
   production gap logged.
3. **NvM.h declares APIs not implemented in NvM.c**: `NvM_CancelJobs`,
   `NvM_ReadPRAMBlock`, `NvM_WritePRAMBlock`, `NvM_RepairDamagedBlocks` —
   header/implementation mismatch (link would fail in production).
4. **WdgM safety response paths not executable in tests**: `WdgM_PerformReset`
   is an intentional `for(;;)` emergency loop; `WdgM_HandleLockstepError`,
   `WdgM_HandleRamSafetyError`, `WdgM_MainFunction` consecutive-error branch
   all funnel into it and would hang a test binary. **Gap: these ASIL-D
   response paths have no executable coverage; they are reviewed by
   inspection only (P2).**
5. **WdgM alive supervision expiry unreachable**: `expectedAliveIndications`
   is initialised to 0 and never populated from the entity config
   (`aliveSupRefCycle`), so `WdgM_CheckEntityAlive` / `WdgM_HandleExpiredSupervision`
   can never trigger via the public API. **P1 finding — production logic gap.**
6. **WdgM DeInit success path unreachable**: `WdgM_DisableAllowed` is a static
   FALSE with no setter, so `WdgM_DeInit` always returns
   `E_DISABLE_NOT_ALLOWED`. **P2 finding.**
7. **Det white-box build** exposes `Det.c` internals via `-Dstatic=` for the
   unit test only — production source untouched.

## Gate note

ISO 26262-6 §9.2 branch coverage and SWE.4.BP1 statement coverage now have
real measured data at **91.57% line / 79.79% branch** — both above the
SWE.4.BP1 thresholds (statement ≥80%, branch ≥70%). `yuleosh ev check`
SWE.4.BP1 / SWE.6.BP3 evidence reflects this measured data.
