# Acceptance Matrix

> Generated: 2026-08-07
> Version: 0.2.0

## Summary

- Covered by tests: 38 (97%)
- Threshold: 60%

## Requirement → Acceptance test → Status

| Req ID | Requirement | 验证方法 | 测试文件 | 状态 |
|:------:|:------------|:---------|:--------|:----:|
| SWR-001.1-01 | SHALL support AUTOSAR Classic Platform 4.4.0 standard | Unit Test | tests/unit/autosar/services/Dcm/test_Dcm.c | ✅ |
| SWR-001.1-02 | SHALL implement MCAL abstraction layer covering 21 microcontroller driver modules | Unit Test | tests/unit/mcal/test_dio.c, tests/unit/mcal/test_can.c, tests/unit/mcal/test_mcu.c | ✅ |
| SWR-001.1-03 | SHALL implement ECUAL abstraction layer covering 29 ECU hardware driver modules | Unit Test | tests/unit/ecual/test_canif.c, tests/unit/ecual/test_ethswt.c | ✅ |
| SWR-001.1-04 | SHALL implement BSW Services layer covering 44 service modules | Unit Test | tests/unit/autosar/services/Dcm/test_Dcm.c, tests/unit/autosar/services/Nvm/test_Nvm.c | ✅ |
| SWR-001.1-05 | SHALL target NXP S32K312 microcontroller platform | Unit Test | tests/unit/mcal/test_mcu.c | ✅ |
| SWR-001.1-06 | SHALL support RTE generation for SWC-to-BSW communication | Unit Test | tests/unit/rte/test_rte_cs_operations.c | ✅ |
| SWR-002.1-01 | SHALL implement E2E communication protection for safety-critical signals | Unit Test | tests/unit/autosar/services/E2E/test_E2E.c, coverage_run/asil/test_e2e_coverage.c | ✅ |
| SWR-002.1-02 | SHALL support HSM-based cryptographic operations via Crypto module | Unit Test | tests/unit/mcal/test_Crypto.c | ✅ |
| SWR-002.1-03 | SHALL provide RAM safety and lockstep monitoring for ASIL-D decomposition | Unit Test | tests/unit/autosar/services/RamSafety_test.c | ✅ |
| SWR-002.1-04 | SHALL implement secure boot mechanism | — | — | ❌ |
| SWR-002.1-05 | SHOULD support SHE-compliant key management | Unit Test | tests/unit/autosar/services/CryIf_Test.c, tests/unit/keym/test_keym.c | ✅ |
| SWR-003.1-01 | SHALL implement CAN communication (Can, CanIf, CanTp, CanNm, CanSm) | Unit Test | tests/unit/autosar/mcal/test_CAN.c, tests/unit/autosar/services/test_cansm.c, tests/unit/autosar/services/test_canm.c, tests/unit/autosar/ecual/test_canNm.c | ✅ |
| SWR-003.1-02 | SHALL implement LIN communication (Lin, LinIf, LinTp, LinNm, LinSM) | Unit Test | tests/unit/autosar/mcal/test_LIN.c | ✅ |
| SWR-003.1-03 | SHALL implement Ethernet communication (Eth, EthIf, EthSm, SoAd, SomeIp, SomeIpSd) | Unit Test | tests/unit/mcal/test_eth.c, tests/unit/autosar/services/test_someip.c, tests/unit/autosar/services/test_someiptp.c, tests/unit/autosar/services/test_someipxf.c | ✅ |
| SWR-003.1-04 | SHALL implement DCM diagnostic communication manager | Unit Test | tests/unit/autosar/services/Dcm/test_Dcm.c, tests/unit/dcm/test_dcm.c, tests/unit/dcm/test_dcm_transfer.c | ✅ |
| SWR-003.1-05 | SHALL implement DoIP diagnostic over IP | Unit Test | tests/unit/doip/test_doip.c | ✅ |
| SWR-003.1-06 | SHOULD support J1939 transport and network management | Unit Test | tests/unit/j1939nm/test_j1939nm.c | ✅ |
| SWR-003.1-07 | SHOULD support SOME/IP Service Discovery | Unit Test | tests/unit/autosar/services/test_someipxf.c, tests/unit/autosar/services/test_someip.c | ✅ |
| SWR-004.1-01 | SHALL implement NVRAM manager (NvM) for persistent storage | Unit Test | tests/unit/autosar/services/Nvm/test_Nvm.c, coverage_run/asil/test_nvm_coverage.c | ✅ |
| SWR-004.1-02 | SHALL implement Flash EEPROM emulation (Fee) | Unit Test | tests/unit/fee/test_fee_init.c, tests/unit/fee/test_fee_read.c, tests/unit/fee/test_fee_write.c | ✅ |
| SWR-004.1-03 | SHALL implement internal/external EEPROM driver | Unit Test | tests/unit/autosar/mcal/test_eep.c | ✅ |
| SWR-004.1-04 | SHALL implement memory abstraction interface (MemIf) | Unit Test | tests/unit/autosar/services/test_memif.c | ✅ |
| SWR-004.1-05 | SHALL support flash driver for S32K312 on-chip flash | Unit Test | tests/unit/flash/test_flash_init.c, tests/unit/flash/test_flash_read.c, tests/unit/flash/test_flash_write.c | ✅ |
| SWR-005.1-01 | SHALL implement ECU state manager (EcuM) | Unit Test | tests/unit/ecum/test_ecum.c | ✅ |
| SWR-005.1-02 | SHALL implement BSW scheduler (BswM) with mode management | Unit Test | tests/unit/bswm/test_bswm.c | ✅ |
| SWR-005.1-03 | SHALL implement Watchdog manager (WdgM) | Unit Test | tests/unit/autosar/services/WdgM_Test.c, coverage_run/asil/test_wdgm_coverage.c | ✅ |
| SWR-005.1-04 | SHALL implement Default Error Tracer (Det) | Unit Test | tests/unit/det/Det_Test.c | ✅ |
| SWR-005.1-05 | SHALL implement Diagnostic Event Manager (Dem) | Unit Test | tests/unit/dem/test_dem.c | ✅ |
| SWR-005.1-06 | SHALL implement Function Inhibition Manager (FiM) | Unit Test | tests/unit/fim/test_fim.c | ✅ |
| SWR-005.1-07 | SHALL implement CRC calculator | Unit Test | tests/unit/crc/Crc_test.c, coverage_run/test_crc_coverage.c | ✅ |
| SWR-005.1-08 | SHALL implement OS (AUTOSAR SC4 compliant) | Unit Test | coverage_run/asil/test_os_timing_coverage.c, tests/unit/test_os_timing.c | ✅ |
| SWR-005.1-09 | SHALL support DLT (Diagnostic Log and Trace) | Unit Test | tests/unit/dlt/test_dlt.c | ✅ |
| SWR-005.1-10 | SHOULD support XCP calibration protocol | Unit Test | tests/unit/xcp/test_xcp.c | ✅ |
| SWR-006.1-01 | SHALL implement 21 MCAL modules (ADC, CAN, Crypto, DIO, EEP, ETH, FEE, Flash, FLS, GPT, I2C, ICU, LIN, MCU, OCU, PORT, PWM, RAMTST, SPI, UART, WDG) | Unit Test | tests/unit/mcal/test_adc.c, tests/unit/mcal/test_can.c, tests/unit/mcal/test_dio.c, tests/unit/mcal/test_gpt.c, tests/unit/mcal/test_mcu.c, tests/unit/mcal/test_eth.c | ✅ |
| SWR-006.1-02 | SHALL provide standardized AUTOSAR interface macros (SchM, Det, MemMap) | Unit Test | tests/unit/autosar/mcal/test_ADC.c, tests/unit/autosar/mcal/test_CAN.c | ✅ |
| SWR-007.1-01 | SHALL support ASW components: CommunicationManager, DiagnosticManager, EngineControl, IOControl, ModeManager, StorageManager, VehicleDynamics, WatchdogManager | Unit Test | tests/unit/autosar/services/Dcm/test_Dcm.c, tests/unit/autosar/services/WdgM_Test.c | ✅ |
| SWR-007.1-02 | SHALL implement RTE for component communication | Unit Test | tests/unit/rte/test_rte_cs_operations.c | ✅ |
| SWR-008.1-01 | SHALL integrate micro DDS middleware for inter-ECU communication | Unit Test | tests/e2e/test_e2e_dds_communication.c | ✅ |
| SWR-008.1-02 | SHOULD support DDS QoS policies | Unit Test | tests/unit/middleware/test_qos.c | ✅ |
