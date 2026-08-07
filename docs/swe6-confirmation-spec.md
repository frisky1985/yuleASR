# yuleASR — SWE.6 Software Qualification Test Confirmation Specification

> ASPICE SWE.6 (Software Qualification Test) — 确认测试规范
> 格式: OpenSpec 合规（RFC 2119 SHALL + GIVEN/WHEN/THEN 场景）
> 用途: `yuleosh swe6 check` / CI L3 合格性测试门禁的真实数据源
> 首次落盘: 2026-08-07（修复 P2-2 SWE.6 假绿 — 此前工具报"15 项全就绪"但本文件缺失）

## 1. 范围 (Scope)

- The SWE.6 qualification test scope SHALL cover the end-to-end BSW software stack: MCAL drivers, ECUAL services, and BSW services (Det, NvM, E2E, WdgM, Com, Dcm, Dem).
- The qualification tests SHALL verify that the integrated software satisfies the software requirements defined in `docs/software-requirements.md`.
- The qualification test suite SHALL be executable in a deterministic CI environment (native x86_64 build, no hardware required).

## 2. 测试环境 (Test Environment)

- The test environment SHALL provide a native (host) build of the AUTOSAR BSW stack using CMake + GCC/Clang.
- The test environment SHALL provide C unit test execution via CTest and Python test execution via pytest.
- The test environment SHALL record coverage data (line/branch) for the software under test.
- The test environment SHALL be reproducible from the repository without proprietary tools.

## 3. 测试用例规范 (Test Case Specification)

### TC-CONF-001: MCAL driver qualification

- GIVEN a native build of the MCAL layer with mock hardware
- WHEN the CTest suite for MCAL drivers is executed
- THEN all MCAL driver tests SHALL pass
- AND the test report SHALL record pass/fail per driver module

### TC-CONF-002: BSW service qualification (Det/NvM/E2E/WdgM)

- GIVEN a native build of the BSW services layer
- WHEN the CTest suite for BSW services is executed
- THEN all BSW service tests SHALL pass
- AND the Det error reporting path SHALL be verified end-to-end

### TC-CONF-003: End-to-end CAN communication qualification

- GIVEN a native build with the CAN stack enabled
- WHEN the e2e CAN communication test is executed
- THEN frames SHALL be transmitted and received successfully
- AND the test SHALL report throughput and error counts

### TC-CONF-004: End-to-end CRC/E2E protection qualification

- GIVEN a native build with E2E protection enabled
- WHEN the e2e CRC real-data test is executed
- THEN CRC computation SHALL match the reference implementation
- AND corrupted-frame detection SHALL be demonstrated

### TC-CONF-005: Diagnostic stack qualification

- GIVEN a native build with Dcm/Dem enabled
- WHEN the e2e diagnostic stack test is executed
- THEN diagnostic request/response cycles SHALL complete
- AND Dem event storage SHALL be verified

### TC-CONF-006: NvM stack qualification

- GIVEN a native build with NvM enabled
- WHEN the e2e NVM stack test is executed
- THEN block write/read cycles SHALL persist data correctly
- AND error handling for invalid blocks SHALL be verified

### TC-CONF-007: Watchdog qualification

- GIVEN a native build with WdgM enabled
- WHEN the e2e watchdog test is executed
- THEN supervision cycles SHALL operate within the configured deadlines
- AND expired-supervision detection SHALL be demonstrated

## 4. 执行计划 (Execution Plan)

- The regression suite SHALL run on every CI run (L1 unit / L2 integration / L3 e2e).
- A smoke subset SHALL complete within the CI layer timeout budget.
- Qualification results SHALL be recorded in `.osh/evidence/` and summarized in the SWE.6 report.
- A failed qualification test SHALL block the release pipeline.

## 5. 报告规范 (Reporting)

- The test report SHALL include: pass rate, coverage summary, and deviation list.
- Each deviation SHALL reference a documented justification.
- The report SHALL be archived under `.yuleosh/reports/` with a timestamp.

## 6. 追溯 (Traceability)

- SWE6-REQ-001 (scope) → TC-CONF-001, TC-CONF-002, TC-CONF-003
- SWE6-REQ-002 (environment) → CI L1/L2/L3 pipeline stages
- SWE6-REQ-003 (test cases) → TC-CONF-001 … TC-CONF-007
- SWE6-REQ-004 (execution plan) → CI regression schedule
- SWE6-REQ-005 (reporting) → `.yuleosh/reports/swe6-report.json`
