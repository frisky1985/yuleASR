# yuleASR — Requirement Traceability Matrix

> **Version**: 1.0 | **Date**: 2026-07-15
> **Standard**: AUTOSAR CP 4.4.0

## Requirement-to-Test Mapping

| SHALL ID | Spec Source | Test File | Test Function | Status |
|:---------|:------------|:----------|:--------------|:-------|
| SWR-001.1-01 | SRS-ARCH | tests/unit/autosar/test_dcm.c | test_dcm_basic | ✅ |
| SWR-001.1-02 | SRS-MCAL | tests/unit/mcal/test_adc.c | test_adc_init | ✅ |
| SWR-001.1-03 | SRS-ECUAL | tests/unit/ecual/test_canif.c | test_canif_init | ✅ |
| SWR-001.1-04 | SRS-SRV | tests/unit/services/test_ecum.c | test_ecum_startup | ✅ |
| SWR-001.1-05 | SRS-PLATFORM | tests/unit/platform/test_mcu.c | test_mcu_init | ✅ |
| SWR-002.1-01 | SRS-SAFETY | tests/unit/e2e/test_e2e.c | test_e2e_protect | ✅ |
| SWR-002.1-02 | SRS-SECURITY | tests/unit/crypto/test_crypto.c | test_crypto_aes | ✅ |
| SWR-003.1-01 | SRS-COM | tests/unit/can/test_can.c | test_can_transmit | ✅ |
| SWR-003.1-02 | SRS-COM | tests/unit/lin/test_lin.c | test_lin_transmit | ✅ |
| SWR-003.1-04 | SRS-DIAG | tests/unit/dcm/test_dcm_uds.c | test_dcm_session | ✅ |
| SWR-004.1-01 | SRS-MEM | tests/unit/nvm/test_nvm.c | test_nvm_read | ✅ |
| SWR-005.1-01 | SRS-SYS | tests/unit/services/test_ecum.c | test_ecum_shutdown | ✅ |
| SWR-005.1-03 | SRS-SYS | tests/unit/wdgm/test_wdgm.c | test_wdgm_supervision | ✅ |
| SWR-005.1-08 | SRS-SYS | tests/unit/os/test_os.c | test_os_task | ✅ |
| SWR-006.1-01 | SRS-MCAL | tests/unit/mcal/test_gpt.c | test_gpt_timer | ✅ |
| SWR-007.1-01 | SRS-ASW | tests/unit/asw/test_commgr.c | test_commgr_route | ✅ |
| SWR-008.1-01 | SRS-DDS | tests/unit/dds/test_dds.c | test_dds_publish | ✅ |
