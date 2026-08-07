# MISRA Fix Task: misra-c2023-21.6

> Generated: 2026-08-07T12:39:04.045857
> Severity: advisory
> Spec Ref: SWE-MISRA-S1

## Rule: <stdlib.h> functions should not be used

<stdlib.h> 的函数不应使用（atoi/atof/exit等）

## Violations

| # | File | Line | Col | Message |
|--:|:-----|:----|:----|:--------|
| 1 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c` | 17 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 2 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c` | 12 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 3 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c` | 12 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 4 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 12 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 5 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/tests/test_e2e_full.c` | 14 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 6 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c` | 16 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 7 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c` | 14 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 8 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/tests/test_bootloader.c` | 11 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 9 | `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/mbedtls/bignum.h` | 20 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 10 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinSlave_Hal.c` | 21 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 11 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_verify.c` | 6 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 12 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 13 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/cryif/include/CryIf_Cfg.h` | 185 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 14 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/someipxf/src/SomeIpXf_Test.c` | 22 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 15 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/someiptp/src/SomeIpTp_Test.c` | 22 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 16 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/stbm/src/StbM_Test.c` | 22 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 17 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/soad/src/SoAd_Test.c` | 22 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 18 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt.c` | 20 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 19 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_Tls.c` | 16 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 20 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 21 | `/Users/stefan/.openclaw/workspace/yuleASR/src/ethernet/eth_manager.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 22 | `/Users/stefan/.openclaw/workspace/yuleASR/src/ethernet/driver/eth_mac_driver.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 23 | `/Users/stefan/.openclaw/workspace/yuleASR/src/ethernet/tests/test_eth_manager.c` | 34 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 24 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/utils/eth_utils.c` | 8 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 25 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 7 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 26 | `/Users/stefan/.openclaw/workspace/yuleASR/src/eth_sm/tests/test_eth_sm.c` | 30 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 27 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/keym/keym_core.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 28 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_cryif.c` | 8 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 29 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_keym.c` | 8 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 30 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_csm.c` | 8 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 31 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_secoc_core.c` | 11 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 32 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_core.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 33 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_dds_integration.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 34 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_freshness.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 35 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/cryif/cryif_core.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 36 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/csm/csm_core.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 37 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/csm/csm_jobs.c` | 11 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 38 | `/Users/stefan/.openclaw/workspace/yuleASR/src/telemetry/tests/test_telemetry.c` | 20 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 39 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/tests/test_dds_eth_transport.c` | 8 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 40 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/eth/dds_eth_transport.c` | 9 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 41 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/eth/dds_eth_discovery.c` | 9 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 42 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/tsn/dds_tsn_transport.c` | 9 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 43 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 44 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 45 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 13 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 46 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_security_manager.c` | 14 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 47 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/tests/test_runner.c` | 6 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 48 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/runtime/dds_runtime.c` | 11 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 49 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/tests/test_dds_core.c` | 14 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 50 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/tsn_stack.c` | 14 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 51 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/cbs/cbs.c` | 15 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 52 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/fp/frame_preemption.c` | 15 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 53 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/tests/test_tsn_stack.c` | 8 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 54 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/gptp/gptp.c` | 17 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 55 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/srp/stream_reservation.c` | 15 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 56 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/tas/tas.c` | 15 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 57 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/unity.h` | 15 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 58 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/examples/hello_world/main.c` | 9 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |
| 59 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/transport/udp.c` | 20 | 0 | misra violation 2106 with no text in the supplied rule-texts-file [misra-c2012-2 |

## Fix Checklist

- [ ] Understand the violation context
- [ ] Apply fix to source code
- [ ] Re-run MISRA check to verify fix
- [ ] Update traceability matrix
- [ ] Document deviation if fix is not feasible

---
*Generated by yuleOSH MISRA fix-task generator*