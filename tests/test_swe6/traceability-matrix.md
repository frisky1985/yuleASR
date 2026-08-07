# SWE.6 Traceability Matrix

> Generated: 2026-08-07T15:33:37Z
> Spec: docs/swe6-confirmation-spec.md

| SWE6 Requirement | TC-CONF | Evidence artifact | Status |
|:-----------------|:--------|:------------------|:-------|
| SWE6-REQ-001 (scope) | TC-CONF-001..007 | tests/test_swe6/run_qualification.sh | ✅ Covered |
| SWE6-REQ-002 (environment) | TC-CONF-001..007 | .github/workflows/ci.yml (native CMake/CTest + pytest) | ✅ Covered |
| SWE6-REQ-003 (test cases) | TC-CONF-001 | pytest test_can_communication (MCAL/CAN driver) | ✅ Covered |
| SWE6-REQ-003 (test cases) | TC-CONF-002 | build-coverage-asil/bin/srv_{crc,det,e2e,nvm,wdgm} | ✅ Covered |
| SWE6-REQ-003 (test cases) | TC-CONF-003 | pytest test_can_communication (e2e CAN) | ✅ Covered |
| SWE6-REQ-003 (test cases) | TC-CONF-004 | srv_crc + srv_e2e + pytest test_crc_real | ✅ Covered |
| SWE6-REQ-003 (test cases) | TC-CONF-005 | pytest test_diagnostic_stack | ✅ Covered |
| SWE6-REQ-003 (test cases) | TC-CONF-006 | srv_nvm + pytest test_nvm_stack | ✅ Covered |
| SWE6-REQ-003 (test cases) | TC-CONF-007 | srv_wdgm + pytest test_watchdog | ✅ Covered |
| SWE6-REQ-004 (execution plan) | TC-CONF-001..007 | tests/test_swe6/run_qualification.sh + CI L1/L2/L3 | ✅ Covered |
| SWE6-REQ-005 (reporting) | TC-CONF-001..007 | tests/test_swe6/qualification-report.md + .yuleosh/reports/swe6-report.json | ✅ Covered |
