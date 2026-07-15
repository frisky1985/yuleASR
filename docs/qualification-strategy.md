# yuleASR — Qualification Test Strategy

> **Version**: 1.0 | **Date**: 2026-07-15
> **Project**: yuleASR AUTOSAR BSW Platform (S32K312)

## 1. Scope

This document defines the qualification test strategy for yuleASR BSW platform. All 18 software requirements (SWR-001 through SWR-008) must have corresponding qualification tests.

## 2. Test Environments

| Environment | Purpose | Coverage |
|:------------|:--------|:---------|
| **SIL** (Software-in-the-Loop) | Unit and integration tests on host PC | Unit/Integration |
| **DIL** (Device-in-the-Loop) | Target execution on S32K312 dev board | Integration/System |
| **HIL** (Hardware-in-the-Loop) | Full system with CAN/LIN/Ethernet I/O | Qualification |

## 3. Acceptance Criteria per Requirement

| Requirement | Acceptance Criteria | Test Env | Priority |
|:------------|:-------------------|:---------|:---------|
| SWR-001: Platform Architecture | All 94 BSW modules compile and link for S32K312 | SIL | P0 |
| SWR-001.1-05: S32K312 Target | Application boots on S32K312 target board | DIL | P0 |
| SWR-002: Safety & Security | E2E error injection detected + HSM crypto verified | HIL | P0 |
| SWR-003: Communication | CAN/LIN/Ethernet frames transmitted/received correctly | HIL | P0 |
| SWR-004: Memory | NvM read/write cycles verified with power-loss recovery | HIL | P0 |
| SWR-005: System Services | EcuM startup/shutdown sequence verified | DIL | P0 |
| SWR-006: MCAL Drivers | All 21 MCAL modules functional on target | DIL | P1 |
| SWR-007: ASW Components | All 8 ASW components functional with RTE | SIL | P1 |
| SWR-008: Micro DDS | DDS pub/sub communication end-to-end | HIL | P2 |

## 4. Test Levels

### Level 1: SIL (Software-in-the-Loop)
- **Host**: x86_64 Linux/macOS
- **Test Framework**: Unity/CMock
- **Coverage**: Unit tests + integration tests
- **Pass Criteria**: 100% pass rate, branch coverage ≥ 80%

### Level 2: DIL (Device-in-the-Loop)
- **Target**: S32K312 Evaluation Board
- **Debugger**: SEGGER J-Link / PEmicro
- **Coverage**: Integration tests + system smoke tests
- **Pass Criteria**: All target tests pass

### Level 3: HIL (Hardware-in-the-Loop)
- **Hardware**: S32K312 + CAN/LIN/Ethernet I/O boards
- **Stimulus**: Vector CANoe / custom HIL scripts
- **Coverage**: Full qualification tests
- **Pass Criteria**: All HIL tests pass, performance within spec

## 5. Test Execution Plan

### Phase 1: SIL Qualification (Weekly)
- Run `tests/unit/` — all unit tests
- Run `tests/integration/` — all integration tests
- Generate coverage report

### Phase 2: DIL Qualification (Per Release)
- Flash target with qualification build
- Run embedded test suite
- Capture trace/log output

### Phase 3: HIL Qualification (Per Major Release)
- Full system test with CAN/LIN/Ethernet stimulators
- Stress test (72h continuous)
- Fault injection testing
