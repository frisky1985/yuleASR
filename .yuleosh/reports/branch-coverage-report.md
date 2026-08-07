# Branch Coverage Report — ASIL Modules (2026-08-07)

> **Scope**: E2E / NvM / WdgM / Com / Crc / Os (timing protection) / Det
> **Method**: native host test binaries linking **production sources from `src/`**
> (unchanged) compiled with `--coverage -fprofile-arcs -ftest-coverage`,
> measured with `lcov --branch-coverage` (BRDA records).
> **Reproduce**: `bash tools/run_branch_coverage.sh`
> **Commit**: HEAD after batch D rebuild

## Why this report exists

量产检视发现旧 `c-coverage.json`（2026-07-26 产物）branch 覆盖率为 0.0%
（found 0 / hit 0），且仅覆盖 7 个文件（含 `coverage_run/` 测试桩与测试文件）。
本报告以当日真实运行数据重建 ASIL 相关模块的分支/行覆盖率。

## Totals (production src/ only)

| Metric | Found | Hit | Rate |
|:-------|------:|----:|-----:|
| Lines | 2881 | 1246 | 43.25% |
| Branches | 1247 | 389 | **31.19%** |
| Functions | 199 | 132 | 66.33% |

Branch coverage > 0% — ISO 26262-6 §9.2 branch coverage requirement now has
real measured data (previously 0.0% / no data).

## Per-module data (当日)

| Module | File | Line % | Branch data |
|:-------|:-----|-------:|:------------|
| Crc | `src/bsw/services/crc/src/Crc.c` | 92.50% | ✅ |
| Det | `src/bsw/services/det/src/Det.c` | 83.78% | ✅ |
| Os | `src/bsw/os/src/Os_TimingProtection.c` | 89.72% | ✅ |
| WdgM | `src/bsw/services/wdgm/src/Wdgm.c` | 72.70% | ✅ |
| E2E | `src/bsw/services/e2e/src/E2E_P01.c` | 68.52% | ✅ |
| E2E | `src/bsw/services/e2e/src/E2E_P02.c` | 47.30% | ✅ |
| E2E | `src/bsw/services/e2e/src/E2E_P04.c` | 67.07% | ✅ |
| E2E | `src/bsw/services/e2e/src/E2E_P05.c` | 59.88% | ✅ |
| E2E | `src/bsw/services/e2e/src/E2E_P06.c` | 64.03% | ✅ |
| E2E | `src/bsw/services/e2e/src/E2E_P07.c` | 60.00% | ✅ |
| Com | `src/bsw/services/com/src/Com.c` | 33.81% | ✅ |
| NvM | `src/bsw/services/nvm/src/NvM.c` | 19.65% | ✅ |
| NvM | `src/bsw/services/nvm/src/NvM_EccHandler.c` | 22.65% | ✅ |
| WdgM | `src/bsw/services/wdgm/src/WdgM_Cfg.c` | 38.10% | ✅ |

Branch coverage was measured per module (BRDA records present in
`coverage_asil_raw.info` / `coverage_asil_src.info`).

## Test drivers (coverage_run/asil/)

| Driver | Links (production) | Stubs used |
|:-------|:-------------------|:-----------|
| `test_e2e_coverage.c` | E2E.c, E2E_P01..P07, Crc.c, Det.c | CRC64, E2E_Init/DeInit (declared but not implemented in src — finding) |
| `test_nvm_coverage.c` | NvM.c, NvM_Redundant.c, NvM_EccHandler*.c, Det.c | MemIf device layer, NvM_Config, Mcal interrupts |
| `test_wdgm_coverage.c` | Wdgm.c, WdgM_Cfg.c, Det.c | Mcal interrupts |
| `test_com_coverage.c` | Com.c, Com_Lcfg.c, Det.c | PduR/Dcm (stubs_com.c), cfg override |
| `test_crc_coverage.c` | Crc.c, Crc_Lcfg.c, Det.c | — |
| `Det_Test.c` (white-box) | Det.c (linkage exposed via `-Dstatic=`), Det_Lcfg.c | — |
| `test_os_timing_coverage.c` | Os_TimingProtection.c, Det.c | FreeRTOS tick, OS hooks/budgets |

## Findings logged (real, not masked)

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
4. **WdgM_HandleLockstepError / WdgM_HandleRamSafetyError / WdgM_PerformReset**
   invoke the emergency-reset path (`for(;;)`), by design not callable from
   a test binary; excluded from the driver with a comment.
5. **Det white-box build** exposes `Det.c` internals via `-Dstatic=` for the
   unit test only — production source untouched.

## Gate note

Branch gate (ISO 26262-6 §9.2) now has real measured data. The overall line
rate (43.25%) reflects only the 7 ASIL modules currently instrumented —
SWE.4 statement-coverage ≥80% gate remains **not met** at project scope and
is honestly reported red by `yuleosh ev check` until the remaining modules
(Com/NvM depth, ECUAL/MCAL) gain coverage drivers.
