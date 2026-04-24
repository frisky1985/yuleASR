# Cross-Layer Integration Test Framework - Tasks

## Status

| Task | Status |
|:-----|:-------|
| Create integration test directory structure | Done |
| Create integration_test_cfg.h (test configuration) | Done |
| Create integration_test.c (mocks + test cases) | Done |
| Create test_runner.c (main function) | Done |
| Create OpenSpec specification | Done |
| Git commit and push | Pending |

## Task List

### 1. Create integration test framework
- [x] Create `src/bsw/integration/tests/` directory
- [x] Implement `integration_test_cfg.h` with test-specific macros
- [x] Implement mock/stub layer:
  - [x] Can_Mock (virtual CAN message buffer)
  - [x] Fee_Mock (virtual NV memory array)
  - [x] Gpt_Mock (manual time advancement)
  - [x] Det_Mock (DET error capture)
  - [x] OS_Mock (alarm dispatch)
  - [x] MCAL stubs (Mcu, Port, Dio, Spi, Adc, Pwm, Wdg)
  - [x] ECUAL stubs (MemIf, Ea, EthIf, LinIf, IoHwAb)
- [x] Implement RTE mock interfaces

### 2. Create test configuration objects
- [x] CanIf test config (1 controller, 8 TX/RX PDUs)
- [x] Com test config (8 signals, 8 IPDUs)
- [x] CanTp test config (1 channel, diagnostic NSDU)
- [x] NvM test config (8 blocks, EngineConfig block)
- [x] Dcm test config (DID 0xF190 VIN read)
- [x] BswM test config (1 rule, RUN/SHUTDOWN actions)
- [x] Dem test config (empty, init-only)
- [x] EcuM test config

### 3. Implement test scenarios
- [x] Test 1: CAN signal end-to-end (MCAL->ECUAL->Service->RTE)
- [x] Test 2: NV data recovery (Fee->NvM->RTE)
- [x] Test 3: Diagnostic request end-to-end (Can->CanIf/CanTp->PduR->Dcm)
- [x] Test 4: Mode switch BswM->EcuM
- [x] Test 5: OS Alarm cyclic scheduling

### 4. Create test runner
- [x] Implement `test_runner.c` with main()
- [x] Integrate with existing `test_framework.h`

### 5. Create OpenSpec specification
- [x] Create `openspec/changes/dev-integration-tests/specs/IntegrationTests_spec.md`
- [x] Document architecture, mock layer, and test scenarios
- [x] Document compilation and execution instructions

### 6. Git commit and push
- [ ] Stage all new files
- [ ] Commit with conventional commit message
- [ ] Push to origin master

## Notes

- Test code follows the same style as `PduR_test.c`
- Mock functions are kept minimal, tracking only necessary state
- No modification to tested module source code
- All configurations use smaller queue/buffer sizes for fast test execution
- DET error detection is enabled to validate error paths
