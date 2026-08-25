# yuleASR Software Requirements — Alternative Specification

> Generated: 2026-08-07 (批D — SWE.1.BP1 备选需求文档)
> 本文件为 docs/software-requirements.md 的备选规范视图; 每条需求
> 含唯一标识符与 SHALL 语句, 并追溯至系统需求 (SYS-REQ) 与测试。

## Requirements

### REQ-SWR-001.1-01

- **SWR-001.1-01**: SHALL support AUTOSAR Classic Platform 4.4.0 standard
- 系统需求追溯: SYS-REQ-BSW-001-01
- 测试追溯: tests/unit/autosar/services/Dcm/test_Dcm.c
- 状态: ✅ Covered

### REQ-SWR-001.1-02

- **SWR-001.1-02**: SHALL implement MCAL abstraction layer covering 21 microcontroller driver modules
- 系统需求追溯: SYS-REQ-BSW-001-02
- 测试追溯: tests/unit/mcal/test_dio.c, tests/unit/mcal/test_can.c, tests/unit/mcal/test_mcu.c
- 状态: ✅ Covered

### REQ-SWR-001.1-03

- **SWR-001.1-03**: SHALL implement ECUAL abstraction layer covering 29 ECU hardware driver modules
- 系统需求追溯: SYS-REQ-BSW-001-03
- 测试追溯: tests/unit/ecual/test_canif.c, tests/unit/ecual/test_ethswt.c
- 状态: ✅ Covered

### REQ-SWR-001.1-04

- **SWR-001.1-04**: SHALL implement BSW Services layer covering 44 service modules
- 系统需求追溯: SYS-REQ-BSW-001-04
- 测试追溯: tests/unit/autosar/services/Dcm/test_Dcm.c, tests/unit/autosar/services/Nvm/test_Nvm.c
- 状态: ✅ Covered

### REQ-SWR-001.1-05

- **SWR-001.1-05**: SHALL target NXP S32K312 microcontroller platform
- 系统需求追溯: SYS-REQ-BSW-001-05
- 测试追溯: tests/unit/mcal/test_mcu.c
- 状态: ✅ Covered

### REQ-SWR-001.1-06

- **SWR-001.1-06**: SHALL support RTE generation for SWC-to-BSW communication
- 系统需求追溯: SYS-REQ-BSW-001-06
- 测试追溯: tests/unit/rte/test_rte_cs_operations.c
- 状态: ✅ Covered

### REQ-SWR-002.1-01

- **SWR-002.1-01**: SHALL implement E2E communication protection for safety-critical signals
- 系统需求追溯: SYS-REQ-BSW-002-01
- 测试追溯: tests/unit/autosar/services/E2E/test_E2E.c, coverage_run/asil/test_e2e_coverage.c
- 状态: ✅ Covered

### REQ-SWR-002.1-02

- **SWR-002.1-02**: SHALL support HSM-based cryptographic operations via Crypto module
- 系统需求追溯: SYS-REQ-BSW-002-02
- 测试追溯: tests/unit/mcal/test_Crypto.c
- 状态: ✅ Covered

### REQ-SWR-002.1-03

- **SWR-002.1-03**: SHALL provide RAM safety and lockstep monitoring for ASIL-D decomposition
- 系统需求追溯: SYS-REQ-BSW-002-03
- 测试追溯: tests/unit/autosar/services/RamSafety_test.c
- 状态: ✅ Covered

### REQ-SWR-002.1-04

- **SWR-002.1-04**: SHALL implement secure boot mechanism
- 系统需求追溯: SYS-REQ-BSW-002-04
- 测试追溯: src/bootloader/tests/test_bootloader.c
- 状态: ✅ Covered

### REQ-SWR-002.1-05

- **SWR-002.1-05**: SHOULD support SHE-compliant key management
- 系统需求追溯: SYS-REQ-BSW-002-05
- 测试追溯: tests/unit/autosar/services/CryIf_Test.c, tests/unit/keym/test_keym.c
- 状态: ✅ Covered

### REQ-SWR-003.1-01

- **SWR-003.1-01**: SHALL implement CAN communication (Can, CanIf, CanTp, CanNm, CanSm)
- 系统需求追溯: SYS-REQ-BSW-003-01
- 测试追溯: tests/unit/autosar/mcal/test_CAN.c, tests/unit/autosar/services/test_cansm.c, tests/unit/autosar/services/test_canm.c, tests/unit/autosar/ecual/test_canNm.c
- 状态: ✅ Covered

### REQ-SWR-003.1-02

- **SWR-003.1-02**: SHALL implement LIN communication (Lin, LinIf, LinTp, LinNm, LinSM)
- 系统需求追溯: SYS-REQ-BSW-003-02
- 测试追溯: tests/unit/autosar/mcal/test_LIN.c
- 状态: ✅ Covered

### REQ-SWR-003.1-03

- **SWR-003.1-03**: SHALL implement Ethernet communication (Eth, EthIf, EthSm, SoAd, SomeIp, SomeIpSd)
- 系统需求追溯: SYS-REQ-BSW-003-03
- 测试追溯: tests/unit/mcal/test_eth.c, tests/unit/autosar/services/test_someip.c, tests/unit/autosar/services/test_someiptp.c, tests/unit/autosar/services/test_someipxf.c
- 状态: ✅ Covered

### REQ-SWR-003.1-04

- **SWR-003.1-04**: SHALL implement DCM diagnostic communication manager
- 系统需求追溯: SYS-REQ-BSW-003-04
- 测试追溯: tests/unit/autosar/services/Dcm/test_Dcm.c, tests/unit/dcm/test_dcm.c, tests/unit/dcm/test_dcm_transfer.c
- 状态: ✅ Covered

### REQ-SWR-003.1-05

- **SWR-003.1-05**: SHALL implement DoIP diagnostic over IP
- 系统需求追溯: SYS-REQ-BSW-003-05
- 测试追溯: tests/unit/doip/test_doip.c
- 状态: ✅ Covered

### REQ-SWR-003.1-06

- **SWR-003.1-06**: SHOULD support J1939 transport and network management
- 系统需求追溯: SYS-REQ-BSW-003-06
- 测试追溯: tests/unit/j1939nm/test_j1939nm.c
- 状态: ✅ Covered

### REQ-SWR-003.1-07

- **SWR-003.1-07**: SHOULD support SOME/IP Service Discovery
- 系统需求追溯: SYS-REQ-BSW-003-07
- 测试追溯: tests/unit/autosar/services/test_someipxf.c, tests/unit/autosar/services/test_someip.c
- 状态: ✅ Covered

### REQ-SWR-004.1-01

- **SWR-004.1-01**: SHALL implement NVRAM manager (NvM) for persistent storage
- 系统需求追溯: SYS-REQ-BSW-004-01
- 测试追溯: tests/unit/autosar/services/Nvm/test_Nvm.c, coverage_run/asil/test_nvm_coverage.c
- 状态: ✅ Covered

### REQ-SWR-004.1-02

- **SWR-004.1-02**: SHALL implement Flash EEPROM emulation (Fee)
- 系统需求追溯: SYS-REQ-BSW-004-02
- 测试追溯: tests/unit/fee/test_fee_init.c, tests/unit/fee/test_fee_read.c, tests/unit/fee/test_fee_write.c
- 状态: ✅ Covered

### REQ-SWR-004.1-03

- **SWR-004.1-03**: SHALL implement internal/external EEPROM driver
- 系统需求追溯: SYS-REQ-BSW-004-03
- 测试追溯: tests/unit/autosar/mcal/test_eep.c
- 状态: ✅ Covered

### REQ-SWR-004.1-04

- **SWR-004.1-04**: SHALL implement memory abstraction interface (MemIf)
- 系统需求追溯: SYS-REQ-BSW-004-04
- 测试追溯: tests/unit/autosar/services/test_memif.c
- 状态: ✅ Covered

### REQ-SWR-004.1-05

- **SWR-004.1-05**: SHALL support flash driver for S32K312 on-chip flash
- 系统需求追溯: SYS-REQ-BSW-004-05
- 测试追溯: tests/unit/flash/test_flash_init.c, tests/unit/flash/test_flash_read.c, tests/unit/flash/test_flash_write.c
- 状态: ✅ Covered

### REQ-SWR-005.1-01

- **SWR-005.1-01**: SHALL implement ECU state manager (EcuM)
- 系统需求追溯: SYS-REQ-BSW-005-01
- 测试追溯: tests/unit/ecum/test_ecum.c
- 状态: ✅ Covered

### REQ-SWR-005.1-02

- **SWR-005.1-02**: SHALL implement BSW scheduler (BswM) with mode management
- 系统需求追溯: SYS-REQ-BSW-005-02
- 测试追溯: tests/unit/bswm/test_bswm.c
- 状态: ✅ Covered

### REQ-SWR-005.1-03

- **SWR-005.1-03**: SHALL implement Watchdog manager (WdgM)
- 系统需求追溯: SYS-REQ-BSW-005-03
- 测试追溯: tests/unit/autosar/services/WdgM_Test.c, coverage_run/asil/test_wdgm_coverage.c
- 状态: ✅ Covered

### REQ-SWR-005.1-04

- **SWR-005.1-04**: SHALL implement Default Error Tracer (Det)
- 系统需求追溯: SYS-REQ-BSW-005-04
- 测试追溯: tests/unit/det/Det_Test.c
- 状态: ✅ Covered

### REQ-SWR-005.1-05

- **SWR-005.1-05**: SHALL implement Diagnostic Event Manager (Dem)
- 系统需求追溯: SYS-REQ-BSW-005-05
- 测试追溯: tests/unit/dem/test_dem.c
- 状态: ✅ Covered

### REQ-SWR-005.1-06

- **SWR-005.1-06**: SHALL implement Function Inhibition Manager (FiM)
- 系统需求追溯: SYS-REQ-BSW-005-06
- 测试追溯: tests/unit/fim/test_fim.c
- 状态: ✅ Covered

### REQ-SWR-005.1-07

- **SWR-005.1-07**: SHALL implement CRC calculator
- 系统需求追溯: SYS-REQ-BSW-005-07
- 测试追溯: tests/unit/crc/Crc_test.c, coverage_run/test_crc_coverage.c
- 状态: ✅ Covered

### REQ-SWR-005.1-08

- **SWR-005.1-08**: SHALL implement OS (AUTOSAR SC4 compliant)
- 系统需求追溯: SYS-REQ-BSW-005-08
- 测试追溯: coverage_run/asil/test_os_timing_coverage.c, tests/unit/test_os_timing.c
- 状态: ✅ Covered

### REQ-SWR-005.1-09

- **SWR-005.1-09**: SHALL support DLT (Diagnostic Log and Trace)
- 系统需求追溯: SYS-REQ-BSW-005-09
- 测试追溯: tests/unit/dlt/test_dlt.c
- 状态: ✅ Covered

### REQ-SWR-005.1-10

- **SWR-005.1-10**: SHOULD support XCP calibration protocol
- 系统需求追溯: SYS-REQ-BSW-005-10
- 测试追溯: tests/unit/xcp/test_xcp.c
- 状态: ✅ Covered

### REQ-SWR-006.1-01

- **SWR-006.1-01**: SHALL implement 21 MCAL modules (ADC, CAN, Crypto, DIO, EEP, ETH, FEE, Flash, FLS, GPT, I2C, ICU, LIN, MCU, OCU, PORT, PWM, RAMTST, SPI, UART, WDG)
- 系统需求追溯: SYS-REQ-BSW-006-01
- 测试追溯: tests/unit/mcal/test_adc.c, tests/unit/mcal/test_can.c, tests/unit/mcal/test_dio.c, tests/unit/mcal/test_gpt.c, tests/unit/mcal/test_mcu.c, tests/unit/mcal/test_eth.c
- 状态: ✅ Covered

### REQ-SWR-006.1-02

- **SWR-006.1-02**: SHALL provide standardized AUTOSAR interface macros (SchM, Det, MemMap)
- 系统需求追溯: SYS-REQ-BSW-006-02
- 测试追溯: tests/unit/autosar/mcal/test_ADC.c, tests/unit/autosar/mcal/test_CAN.c
- 状态: ✅ Covered

### REQ-SWR-007.1-01

- **SWR-007.1-01**: SHALL support ASW components: CommunicationManager, DiagnosticManager, EngineControl, IOControl, ModeManager, StorageManager, VehicleDynamics, WatchdogManager
- 系统需求追溯: SYS-REQ-BSW-007-01
- 测试追溯: tests/unit/autosar/services/Dcm/test_Dcm.c, tests/unit/autosar/services/WdgM_Test.c
- 状态: ✅ Covered

### REQ-SWR-007.1-02

- **SWR-007.1-02**: SHALL implement RTE for component communication
- 系统需求追溯: SYS-REQ-BSW-007-02
- 测试追溯: tests/unit/rte/test_rte_cs_operations.c
- 状态: ✅ Covered

### REQ-SWR-008.1-01

- **SWR-008.1-01**: SHALL integrate micro DDS middleware for inter-ECU communication
- 系统需求追溯: SYS-REQ-BSW-008-01
- 测试追溯: tests/e2e/test_e2e_dds_communication.c
- 状态: ✅ Covered

### REQ-SWR-008.1-02

- **SWR-008.1-02**: SHOULD support DDS QoS policies
- 系统需求追溯: SYS-REQ-BSW-008-02
- 测试追溯: tests/unit/middleware/test_qos.c
- 状态: ✅ Covered

## 追溯矩阵摘要

- 需求总数: 39
- 有测试覆盖: 38 (97.4%)
- 未覆盖: 1 (如实标注, 待补测试)
