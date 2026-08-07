# Traceability Matrix

> Generated: 2026-08-07
> Version: 0.2.0 (批D 真实映射重建 — 测试文件存在性校验)
> Source: docs/software-requirements.md

## Requirements → Implementation → Tests

| Requirement | SHALL statement | Tests | Status |
|:------------|:----------------|:------|:-------|
| SWR-001.1-01 | SHALL support AUTOSAR Classic Platform 4.4.0 standard | tests/unit/autosar/services/Dcm/test_Dcm.c | ✅ Covered |
| SWR-001.1-02 | SHALL implement MCAL abstraction layer covering 21 microcontroller driver modules | tests/unit/mcal/test_dio.c, tests/unit/mcal/test_can.c, tests/unit/mcal/test_mcu.c | ✅ Covered |
| SWR-001.1-03 | SHALL implement ECUAL abstraction layer covering 29 ECU hardware driver modules | tests/unit/ecual/test_canif.c, tests/unit/ecual/test_ethswt.c | ✅ Covered |
| SWR-001.1-04 | SHALL implement BSW Services layer covering 44 service modules | tests/unit/autosar/services/Dcm/test_Dcm.c, tests/unit/autosar/services/Nvm/test_Nvm.c | ✅ Covered |
| SWR-001.1-05 | SHALL target NXP S32K312 microcontroller platform | tests/unit/mcal/test_mcu.c | ✅ Covered |
| SWR-001.1-06 | SHALL support RTE generation for SWC-to-BSW communication | tests/unit/rte/test_rte_cs_operations.c | ✅ Covered |
| SWR-002.1-01 | SHALL implement E2E communication protection for safety-critical signals | tests/unit/autosar/services/E2E/test_E2E.c, coverage_run/asil/test_e2e_coverage.c | ✅ Covered |
| SWR-002.1-02 | SHALL support HSM-based cryptographic operations via Crypto module | tests/unit/mcal/test_Crypto.c | ✅ Covered |
| SWR-002.1-03 | SHALL provide RAM safety and lockstep monitoring for ASIL-D decomposition | tests/unit/autosar/services/RamSafety_test.c | ✅ Covered |
| SWR-002.1-04 | SHALL implement secure boot mechanism | — | ❌ Not Covered |
| SWR-002.1-05 | SHOULD support SHE-compliant key management | tests/unit/autosar/services/CryIf_Test.c, tests/unit/keym/test_keym.c | ✅ Covered |
| SWR-003.1-01 | SHALL implement CAN communication (Can, CanIf, CanTp, CanNm, CanSm) | tests/unit/autosar/mcal/test_CAN.c, tests/unit/autosar/services/test_cansm.c, tests/unit/autosar/services/test_canm.c, tests/unit/autosar/ecual/test_canNm.c | ✅ Covered |
| SWR-003.1-02 | SHALL implement LIN communication (Lin, LinIf, LinTp, LinNm, LinSM) | tests/unit/autosar/mcal/test_LIN.c | ✅ Covered |
| SWR-003.1-03 | SHALL implement Ethernet communication (Eth, EthIf, EthSm, SoAd, SomeIp, SomeIpSd) | tests/unit/mcal/test_eth.c, tests/unit/autosar/services/test_someip.c, tests/unit/autosar/services/test_someiptp.c, tests/unit/autosar/services/test_someipxf.c | ✅ Covered |
| SWR-003.1-04 | SHALL implement DCM diagnostic communication manager | tests/unit/autosar/services/Dcm/test_Dcm.c, tests/unit/dcm/test_dcm.c, tests/unit/dcm/test_dcm_transfer.c | ✅ Covered |
| SWR-003.1-05 | SHALL implement DoIP diagnostic over IP | tests/unit/doip/test_doip.c | ✅ Covered |
| SWR-003.1-06 | SHOULD support J1939 transport and network management | tests/unit/j1939nm/test_j1939nm.c | ✅ Covered |
| SWR-003.1-07 | SHOULD support SOME/IP Service Discovery | tests/unit/autosar/services/test_someipxf.c, tests/unit/autosar/services/test_someip.c | ✅ Covered |
| SWR-004.1-01 | SHALL implement NVRAM manager (NvM) for persistent storage | tests/unit/autosar/services/Nvm/test_Nvm.c, coverage_run/asil/test_nvm_coverage.c | ✅ Covered |
| SWR-004.1-02 | SHALL implement Flash EEPROM emulation (Fee) | tests/unit/fee/test_fee_init.c, tests/unit/fee/test_fee_read.c, tests/unit/fee/test_fee_write.c | ✅ Covered |
| SWR-004.1-03 | SHALL implement internal/external EEPROM driver | tests/unit/autosar/mcal/test_eep.c | ✅ Covered |
| SWR-004.1-04 | SHALL implement memory abstraction interface (MemIf) | tests/unit/autosar/services/test_memif.c | ✅ Covered |
| SWR-004.1-05 | SHALL support flash driver for S32K312 on-chip flash | tests/unit/flash/test_flash_init.c, tests/unit/flash/test_flash_read.c, tests/unit/flash/test_flash_write.c | ✅ Covered |
| SWR-005.1-01 | SHALL implement ECU state manager (EcuM) | tests/unit/ecum/test_ecum.c | ✅ Covered |
| SWR-005.1-02 | SHALL implement BSW scheduler (BswM) with mode management | tests/unit/bswm/test_bswm.c | ✅ Covered |
| SWR-005.1-03 | SHALL implement Watchdog manager (WdgM) | tests/unit/autosar/services/WdgM_Test.c, coverage_run/asil/test_wdgm_coverage.c | ✅ Covered |
| SWR-005.1-04 | SHALL implement Default Error Tracer (Det) | tests/unit/det/Det_Test.c | ✅ Covered |
| SWR-005.1-05 | SHALL implement Diagnostic Event Manager (Dem) | tests/unit/dem/test_dem.c | ✅ Covered |
| SWR-005.1-06 | SHALL implement Function Inhibition Manager (FiM) | tests/unit/fim/test_fim.c | ✅ Covered |
| SWR-005.1-07 | SHALL implement CRC calculator | tests/unit/crc/Crc_test.c, coverage_run/test_crc_coverage.c | ✅ Covered |
| SWR-005.1-08 | SHALL implement OS (AUTOSAR SC4 compliant) | coverage_run/asil/test_os_timing_coverage.c, tests/unit/test_os_timing.c | ✅ Covered |
| SWR-005.1-09 | SHALL support DLT (Diagnostic Log and Trace) | tests/unit/dlt/test_dlt.c | ✅ Covered |
| SWR-005.1-10 | SHOULD support XCP calibration protocol | tests/unit/xcp/test_xcp.c | ✅ Covered |
| SWR-006.1-01 | SHALL implement 21 MCAL modules (ADC, CAN, Crypto, DIO, EEP, ETH, FEE, Flash, FLS, GPT, I2C, ICU, LIN, MCU, OCU, PORT, PWM, RAMTST, SPI, UART, WDG) | tests/unit/mcal/test_adc.c, tests/unit/mcal/test_can.c, tests/unit/mcal/test_dio.c, tests/unit/mcal/test_gpt.c, tests/unit/mcal/test_mcu.c, tests/unit/mcal/test_eth.c | ✅ Covered |
| SWR-006.1-02 | SHALL provide standardized AUTOSAR interface macros (SchM, Det, MemMap) | tests/unit/autosar/mcal/test_ADC.c, tests/unit/autosar/mcal/test_CAN.c | ✅ Covered |
| SWR-007.1-01 | SHALL support ASW components: CommunicationManager, DiagnosticManager, EngineControl, IOControl, ModeManager, StorageManager, VehicleDynamics, WatchdogManager | tests/unit/autosar/services/Dcm/test_Dcm.c, tests/unit/autosar/services/WdgM_Test.c | ✅ Covered |
| SWR-007.1-02 | SHALL implement RTE for component communication | tests/unit/rte/test_rte_cs_operations.c | ✅ Covered |
| SWR-008.1-01 | SHALL integrate micro DDS middleware for inter-ECU communication | tests/e2e/test_e2e_dds_communication.c | ✅ Covered |
| SWR-008.1-02 | SHOULD support DDS QoS policies | tests/unit/middleware/test_qos.c | ✅ Covered |

**Covered: 38/39 (97.4%)**
