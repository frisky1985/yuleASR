# Traceability Matrix

> Generated: 2026-08-25
> Version: 0.1.0

## Requirements → Implementation → Tests

### MCAL-SHALL-001
- Req ID: MCAL-SHALL-001
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ MCAL SHALL 提供标准 AUTOSAR API (如 Adc_Init, Can_Write)

### MCAL-SHALL-002
- Req ID: MCAL-SHALL-002
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 所有 MCAL 模块 SHALL 支持同步和中断两种操作模式

### MCAL-SHALL-003
- Req ID: MCAL-SHALL-003
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ MCAL SHALL 使用 MISRA C:2023 合规编码风格

### ECUAL-SHALL-001
- Req ID: ECUAL-SHALL-001
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ ECUAL SHALL 使用 MCAL API, 不直接操作硬件寄存器

### ECUAL-SHALL-002
- Req ID: ECUAL-SHALL-002
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 看门狗管理器 SHALL 在超时前刷新

### SVC-SHALL-001
- Req ID: SVC-SHALL-001
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ OS SHALL 提供符合 OSEK/AUTOSAR OS 的调度服务

### SVC-SHALL-002
- Req ID: SVC-SHALL-002
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 通信栈 (CAN/以太网) SHALL 实现 PDU 路由

### SVC-SHALL-003
- Req ID: SVC-SHALL-003
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 诊断事件管理器 (Dem) SHALL 记录并上报 DTC

### NFR-SHALL-001
- Req ID: NFR-SHALL-001
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 代码 MISRA C:2023 合规

### NFR-SHALL-002
- Req ID: NFR-SHALL-002
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 单元测试行覆盖率

### NFR-SHALL-003
- Req ID: NFR-SHALL-003
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 条件覆盖率

### NFR-SHALL-004
- Req ID: NFR-SHALL-004
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ 静态分析 (cppcheck)

### None
- Req ID: None
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ SHALL 使用 MISRA C:2023 `safety` 配置

### SWR-001.1-01
- Req ID: SWR-001.1-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/services/Dcm/test_Dcm.c
- SHALL details:
  ❌ **SWR-001.1-01**: SHALL support AUTOSAR Classic Platform 4.4.0 standard

### SWR-001.1-02
- Req ID: SWR-001.1-02
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/mcal/test_ADC.c
- SHALL details:
  ❌ **SWR-001.1-02**: SHALL implement MCAL abstraction layer covering 21 microcontroller driver modules

### SWR-001.1-03
- Req ID: SWR-001.1-03
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/ecual/test_CanIf.c
- SHALL details:
  ❌ **SWR-001.1-03**: SHALL implement ECUAL abstraction layer covering 29 ECU hardware driver modules

### SWR-001.1-04
- Req ID: SWR-001.1-04
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/services/Dcm/test_Dcm.c
- SHALL details:
  ❌ **SWR-001.1-04**: SHALL implement BSW Services layer covering 44 service modules

### SWR-001.1-05
- Req ID: SWR-001.1-05
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/mcal/test_mcu.c
- SHALL details:
  ❌ **SWR-001.1-05**: SHALL target NXP S32K312 microcontroller platform

### SWR-001.1-06
- Req ID: SWR-001.1-06
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-001.1-06**: SHALL support RTE generation for SWC-to-BSW communication

### SWR-002.1-01
- Req ID: SWR-002.1-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/services/E2E/test_E2E.c
- SHALL details:
  ❌ **SWR-002.1-01**: SHALL implement E2E communication protection for safety-critical signals

### SWR-002.1-02
- Req ID: SWR-002.1-02
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/mcal/test_Crypto.c
- SHALL details:
  ❌ **SWR-002.1-02**: SHALL support HSM-based cryptographic operations via Crypto module

### SWR-002.1-03
- Req ID: SWR-002.1-03
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-002.1-03**: SHALL provide RAM safety and lockstep monitoring for ASIL-D decomposition

### SWR-002.1-04
- Req ID: SWR-002.1-04
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-002.1-04**: SHALL implement secure boot mechanism

### SWR-003.1-01
- Req ID: SWR-003.1-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/mcal/test_CAN.c
- SHALL details:
  ❌ **SWR-003.1-01**: SHALL implement CAN communication (Can, CanIf, CanTp, CanNm, CanSm)

### SWR-003.1-02
- Req ID: SWR-003.1-02
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/mcal/test_LIN.c
- SHALL details:
  ❌ **SWR-003.1-02**: SHALL implement LIN communication (Lin, LinIf, LinTp, LinNm, LinSM)

### SWR-003.1-03
- Req ID: SWR-003.1-03
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-003.1-03**: SHALL implement Ethernet communication (Eth, EthIf, EthSm, SoAd, SomeIp, SomeIpSd)

### SWR-003.1-04
- Req ID: SWR-003.1-04
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/services/Dcm/test_Dcm.c
- SHALL details:
  ❌ **SWR-003.1-04**: SHALL implement DCM diagnostic communication manager

### SWR-003.1-05
- Req ID: SWR-003.1-05
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-003.1-05**: SHALL implement DoIP diagnostic over IP

### SWR-004.1-01
- Req ID: SWR-004.1-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/services/Nvm/test_Nvm.c
- SHALL details:
  ❌ **SWR-004.1-01**: SHALL implement NVRAM manager (NvM) for persistent storage

### SWR-004.1-02
- Req ID: SWR-004.1-02
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-004.1-02**: SHALL implement Flash EEPROM emulation (Fee)

### SWR-004.1-03
- Req ID: SWR-004.1-03
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-004.1-03**: SHALL implement internal/external EEPROM driver

### SWR-004.1-04
- Req ID: SWR-004.1-04
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-004.1-04**: SHALL implement memory abstraction interface (MemIf)

### SWR-004.1-05
- Req ID: SWR-004.1-05
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-004.1-05**: SHALL support flash driver for S32K312 on-chip flash

### SWR-005.1-01
- Req ID: SWR-005.1-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/services/test_ecum.c
- SHALL details:
  ❌ **SWR-005.1-01**: SHALL implement ECU state manager (EcuM)

### SWR-005.1-02
- Req ID: SWR-005.1-02
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-005.1-02**: SHALL implement BSW scheduler (BswM) with mode management

### SWR-005.1-03
- Req ID: SWR-005.1-03
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/mcal/test_wdg.c
- SHALL details:
  ❌ **SWR-005.1-03**: SHALL implement Watchdog manager (WdgM)

### SWR-005.1-04
- Req ID: SWR-005.1-04
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-005.1-04**: SHALL implement Default Error Tracer (Det)

### SWR-005.1-05
- Req ID: SWR-005.1-05
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-005.1-05**: SHALL implement Diagnostic Event Manager (Dem)

### SWR-005.1-06
- Req ID: SWR-005.1-06
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-005.1-06**: SHALL implement Function Inhibition Manager (FiM)

### SWR-005.1-07
- Req ID: SWR-005.1-07
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-005.1-07**: SHALL implement CRC calculator

### SWR-005.1-08
- Req ID: SWR-005.1-08
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_os_timing.c
- SHALL details:
  ❌ **SWR-005.1-08**: SHALL implement OS (AUTOSAR SC4 compliant)

### SWR-005.1-09
- Req ID: SWR-005.1-09
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-005.1-09**: SHALL support DLT (Diagnostic Log and Trace)

### SWR-006.1-01
- Req ID: SWR-006.1-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/autosar/mcal/test_gpt.c
- SHALL details:
  ❌ **SWR-006.1-01**: SHALL implement 21 MCAL modules (ADC, CAN, Crypto, DIO, EEP, ETH, FEE, Flash, FLS, GPT, I2C, ICU, LIN, MCU, OCU, PORT, PWM, RAMTST, SPI, UART, WDG)

### SWR-006.1-02
- Req ID: SWR-006.1-02
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-006.1-02**: SHALL provide standardized AUTOSAR interface macros (SchM, Det, MemMap)

### SWR-007.1-01
- Req ID: SWR-007.1-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/services/test_comm.c
- SHALL details:
  ❌ **SWR-007.1-01**: SHALL support ASW components: CommunicationManager, DiagnosticManager, EngineControl, IOControl, ModeManager, StorageManager, VehicleDynamics, WatchdogManager

### SWR-007.1-02
- Req ID: SWR-007.1-02
- SHALL statements: 1
- Status: ❌ Not Covered
- Scenarios: 0 ⚠️
- Test files: 0 ❌ Not covered by any test
- SHALL details:
  ❌ **SWR-007.1-02**: SHALL implement RTE for component communication

### SWR-008.1-01
- Req ID: SWR-008.1-01
- SHALL statements: 1
- Status: ✅ Covered
- Scenarios: 0 ⚠️
- Test files: 1 ✅ Covered: tests/unit/test_dds_qualification.c
- SHALL details:
  ❌ **SWR-008.1-01**: SHALL integrate micro DDS middleware for inter-ECU communication

## Summary
- Total Requirements: 47
- Requirements with implementation: 85 (180%)
- Requirements with test coverage: 17 (36%)
- Uncovered SHALLs: 30
- Scenarios: 0
- Reviews: 0
- CI Runs: 0
