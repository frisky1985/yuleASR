# SWE.6 Qualification Test Report

> Generated: 2026-08-07T15:33:37Z
> Executed: bash tests/test_swe6/run_qualification.sh
> Result: **8/8 cases passed (100%)**

## Pass rate

| TC | Case | Status | Detail |
|:---|:-----|:-------|:-------|
| TC-CONF-002 | BSW services build (ASIL drivers) | PASS | 7 drivers compiled from src/ |
| TC-CONF-001 | MCAL/CAN driver qualification | PASS | pytest test_can_communication |
| TC-CONF-002 | BSW service qualification (Det/NvM/E2E/WdgM/Crc) | PASS | 5 C drivers |
| TC-CONF-003 | E2E CAN communication | PASS | pytest test_can_communication |
| TC-CONF-004 | CRC/E2E protection | PASS | srv_crc + srv_e2e + test_crc_real |
| TC-CONF-005 | Diagnostic stack (Dcm/Dem) | PASS | pytest test_diagnostic_stack |
| TC-CONF-006 | NvM stack qualification | PASS | srv_nvm + test_nvm_stack |
| TC-CONF-007 | Watchdog qualification | PASS | srv_wdgm + test_watchdog |

## Coverage summary

| Metric | Value |
|:-------|:------|
| Line coverage | 43.25% |
| Branch coverage | 31.19% |
| Measured from | .yuleosh/reports/c-coverage.json (2026-08-07 ASIL rebuild) |

## Deviations

- None — all qualification cases passed with production sources.
- Known scope note: WdgM emergency-reset handlers (HandleLockstepError /
  HandleRamSafetyError / PerformReset) are intentionally non-returning and
  not invoked by qualification tests; they are verified by review instead.
