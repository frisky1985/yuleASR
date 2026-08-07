# yuleASR — Requirement Traceability Matrix

> **Version**: 1.0 | **Date**: 2026-07-15
> **Standard**: AUTOSAR CP 4.4.0

## Requirement-to-Test Mapping

| SHALL ID | Spec Source | Test File | Test Function | Status |
|:---------|:------------|:----------|:--------------|:-------|
| SWR-001.1-01 | SRS-ARCH | tests/unit/autosar/services/Dcm/test_Dcm.c | test_Dcm_Init_ValidConfig | ✅ |
| SWR-001.1-02 | SRS-ARCH | tests/unit/autosar/mcal/test_ADC.c | test_init_deinit | ✅ |
| SWR-001.1-03 | SRS-ARCH | tests/unit/autosar/ecual/test_CanIf.c | test_CanIf_Init_ValidConfig | ✅ |
| SWR-001.1-04 | SRS-ARCH | tests/unit/autosar/services/Dcm/test_Dcm.c | test_Dcm_MainFunction_Initialized | ✅ |
| SWR-001.1-05 | SRS-ARCH | tests/unit/autosar/mcal/test_mcu.c | mcu_init_valid_config | ✅ |
| SWR-002.1-01 | SRS-SAFETY | tests/unit/autosar/services/E2E/test_E2E.c | test_E2E_P01_Protect_Check_RoundTrip | ✅ |
| SWR-002.1-02 | SRS-SAFETY | tests/unit/autosar/mcal/test_Crypto.c | test_init_deinit | ✅ |
| SWR-003.1-01 | SRS-COM | tests/unit/autosar/mcal/test_CAN.c | test_init | ✅ |
| SWR-003.1-02 | SRS-COM | tests/unit/autosar/mcal/test_LIN.c | test_init_deinit | ✅ |
| SWR-003.1-04 | SRS-COM | tests/unit/autosar/services/Dcm/test_Dcm.c | test_Dcm_MainFunction_Uninit | ✅ |
| SWR-004.1-01 | SRS-MEM | tests/unit/autosar/services/Nvm/test_Nvm.c | test_NvM_Init_ValidConfig | ✅ |
| SWR-005.1-01 | SRS-SYS | tests/unit/services/test_ecum.c | ecum_init_startup_state | ✅ |
| SWR-005.1-03 | SRS-SYS | tests/unit/autosar/mcal/test_wdg.c | test_wdg_Init_should_initialize_successfully | ✅ |
| SWR-005.1-08 | SRS-SYS | tests/unit/test_os_timing.c | test_Os_Timing_Execution_Budget | ✅ |
| SWR-006.1-01 | SRS-MCAL | tests/unit/autosar/mcal/test_gpt.c | test_init_valid | ✅ |
| SWR-007.1-01 | SRS-ASW | tests/unit/services/test_comm.c | comm_init_valid_config | ✅ |
| SWR-008.1-01 | SRS-DDS | tests/unit/test_dds_qualification.c | test_dds_init_and_config | ✅ |
