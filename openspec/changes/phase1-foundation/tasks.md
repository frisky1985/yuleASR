# Phase 1: Foundation - Implementation Tasks

## Milestone M1.1: Det Implementation (3 days)

### Day 1: Setup and API Definition
- [x] T001: Create Det module directory structure
  - `src/bsw/general/det/include/`
  - `src/bsw/general/det/src/`
  - `tests/unit/det/`
- [ ] T002: Create Det.h with public API declarations
- [ ] T003: Define error codes and types
- [ ] T004: Create Det_Cfg.h template

### Day 2: Core Implementation
- [ ] T005: Implement Det_Init()
- [ ] T006: Implement Det_ReportError()
- [ ] T007: Implement Det_Start()
- [ ] T008: Implement Det_ReportRuntimeError()
- [ ] T009: Implement Det_ReportTransientFault()

### Day 3: Testing and Validation
- [ ] T010: Create unit tests for all APIs
- [ ] T011: Achieve >90% test coverage
- [ ] T012: Run static analysis (MISRA)
- [ ] T013: **Gate Review M1.1**

**Exit Criteria:**
- All Det APIs implemented
- Unit test coverage > 90%
- No MISRA violations
- OpenSpec scenario passed

---

## Milestone M1.2: Fls Implementation (1 week)

### Day 1-2: Setup and API
- [ ] T014: Create Fls module directory structure
- [ ] T015: Create Fls.h with public API
- [ ] T016: Define Fls types and constants
- [ ] T017: Create Fls_Internal.h
- [ ] T018: Create Fls_Cfg.h template

### Day 3-4: Core Implementation
- [ ] T019: Implement Fls_Init()
- [ ] T020: Implement state machine
- [ ] T021: Implement Fls_Erase() (async)
- [ ] T022: Implement Fls_Write() (async)

### Day 5: Read and Utility Functions
- [ ] T023: Implement Fls_Read()
- [ ] T024: Implement Fls_GetStatus()
- [ ] T025: Implement Fls_GetJobResult()
- [ ] T026: Implement Fls_Cancel()
- [ ] T027: Implement Fls_MainFunction()

### Day 6-7: Testing and Integration
- [ ] T028: Create unit tests (mock hardware)
- [ ] T029: Create integration test with Fee
- [ ] T030: Test async operation flow
- [ ] T031: **Gate Review M1.2**

**Exit Criteria:**
- All Fls APIs implemented
- Async operation working
- Fee integration verified
- Test coverage > 85%

---

## Milestone M1.3: WdgIf + Wdgm Implementation (1 week)

### Day 1-2: WdgIf Implementation
- [ ] T032: Create WdgIf module structure
- [ ] T033: Implement WdgIf_SetTriggerCondition()
- [ ] T034: Implement WdgIf_SetMode()
- [ ] T035: Create WdgIf_Cfg.h
- [ ] T036: Unit test WdgIf

### Day 3-5: Wdgm Core Implementation
- [ ] T037: Create Wdgm module structure
- [ ] T038: Implement Wdgm_Init()
- [ ] T039: Implement Wdgm_SetMode()
- [ ] T040: Implement Wdgm_CheckpointReached()
- [ ] T041: Implement alive supervision
- [ ] T042: Implement deadline supervision

### Day 6: Wdgm Advanced Features
- [ ] T043: Implement logical supervision
- [ ] T044: Implement Wdgm_GetLocalStatus()
- [ ] T045: Implement Wdgm_GetGlobalStatus()
- [ ] T046: Implement Wdgm_PerformReset()

### Day 7: Integration and Testing
- [ ] T047: Unit test Wdgm
- [ ] T048: Integration test with EcuM
- [ ] T049: Test supervision scenarios
- [ ] T050: **Gate Review M1.3**

**Exit Criteria:**
- WdgIf and Wdgm implemented
- All supervision types working
- EcuM integration verified
- Deadline monitoring tested

---

## Phase 1 Completion Gates

### Gate 1: Code Quality
- [ ] All modules compile without warnings
- [ ] MISRA C:2012 compliance check passed
- [ ] Cyclomatic complexity < 10 per function
- [ ] Code coverage > 85%

### Gate 2: Integration
- [ ] Det integrates with Dcm/Dem
- [ ] Fls integrates with Fee
- [ ] Wdgm integrates with EcuM
- [ ] No linker errors

### Gate 3: Documentation
- [ ] API documentation complete (Doxygen)
- [ ] SWS requirements traced
- [ ] Configuration guide updated
- [ ] OpenSpec scenarios pass

---

## Task Tracking

| Task | Status | Owner | Started | Completed |
|:-----|:-------|:------|:--------|:----------|
| T001 | ✅ | System | 2026-04-28 | 2026-04-28 |
| T002 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T003 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T004 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T005 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T006 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T007 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T008 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T009 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T010 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T011 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T012 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T013 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T014 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T015 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T016 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T017 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T018 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T019 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T020 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T021 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T022 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T023 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T024 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T025 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T026 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T027 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T028 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T029 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T030 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T031 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T032 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T033 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T034 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T035 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T036 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T037 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T038 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T039 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T040 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T041 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T042 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T043 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T044 | ✅ | AI | 2026-04-28 | 2026-04-28 |
| T045 | ✅ | AI | 2026-04-28 | 2026-04-28 |

**Legend:**
- ✅ Complete
- 🔄 In Progress
- ⏳ Pending
- ❌ Blocked

| T061 | ✅ | AI | 2026-04-28 | 2026-04-28 |  /* Phase 3 网络扩展 */
| T062 | ✅ | AI | 2026-04-28 | 2026-04-28 |  /* SoAd */
| T063 | ✅ | AI | 2026-04-28 | 2026-04-28 |  /* SomeIpXf */
| T064 | ✅ | AI | 2026-04-28 | 2026-04-28 |  /* SomeIpTp */
| T065 | ✅ | AI | 2026-04-28 | 2026-04-28 |  /* StbM */
| T066 | ✅ | AI | 2026-04-28 | 2026-04-28 |  /* 硬件适配层 Fls_Hw */
| T067 | ✅ | AI | 2026-04-28 | 2026-04-28 |  /* 硬件适配层 Wdg_Hw */
| T068 | ✅ | AI | 2026-04-28 | 2026-04-28 |  /* 系统集成测试 */
| T069 | ✅ | AI | 2026-04-28 | 2026-04-28 |  /* 最终报告生成 */
| T070 | ⏳ | - | - | - |  /* 剩余可选模块 */