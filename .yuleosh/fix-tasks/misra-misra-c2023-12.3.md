# MISRA Fix Task: misra-c2023-12.3

> Generated: 2026-08-07T12:39:04.024314
> Severity: advisory
> Spec Ref: SWE-MISRA-S1

## Rule: sizeof 操作数约束

sizeof 操作符不应应用于具有副作用的表达式

## Violations

| # | File | Line | Col | Message |
|--:|:-----|:----|:----|:--------|
| 1 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/tests/test_e2e_full.c` | 94 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 2 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/tests/test_e2e_full.c` | 121 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 3 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/tests/test_e2e_full.c` | 221 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 4 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/tests/test_e2e_full.c` | 302 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 5 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/tests/test_e2e_full.c` | 436 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 6 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/tests/test_e2e_full.c` | 661 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 7 | `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/vehicle_dynamics/src/Swc_VehicleDynamics.c` | 275 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 8 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/tests/test_bootloader.c` | 113 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 9 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/tests/test_bootloader.c` | 195 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 10 | `/Users/stefan/.openclaw/workspace/yuleASR/src/platform/s32k312/src/Platform_RamSafety.c` | 443 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 11 | `/Users/stefan/.openclaw/workspace/yuleASR/src/platform/s32k312/src/Platform_Lockstep.c` | 555 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 12 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Safety_1.0.0.c` | 133 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 13 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 216 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 14 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 298 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 15 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 373 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 16 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinSlave_Pid.c` | 29 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 17 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinMaster.c` | 719 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 18 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 176 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 19 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 197 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 20 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 229 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 21 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 311 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 22 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/soad/src/SoAd_Test.c` | 395 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 23 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt.c` | 146 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 24 | `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/mbedtls/x509.h` | 247 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 25 | `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/mbedtls/x509.h` | 248 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 26 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c` | 416 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 27 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/utils/eth_utils.c` | 40 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 28 | `/Users/stefan/.openclaw/workspace/yuleASR/src/eth_sm/tests/test_eth_sm.c` | 139 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 29 | `/Users/stefan/.openclaw/workspace/yuleASR/src/eth_sm/tests/test_eth_sm.c` | 639 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 30 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_secoc_freshness.c` | 125 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 31 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_secoc_freshness.c` | 126 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 32 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_csm.c` | 78 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 33 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_secoc_core.c` | 276 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 34 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_secoc_core.c` | 335 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 35 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_core.c` | 145 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 36 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_core.c` | 146 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 37 | `/Users/stefan/.openclaw/workspace/yuleASR/src/telemetry/telemetry.c` | 323 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 38 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/tests/test_dds_eth_transport.c` | 94 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 39 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 84 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 40 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 85 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 41 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 218 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 42 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 84 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 43 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 86 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 44 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 448 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 45 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 502 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 46 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 234 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 47 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/tests/test_dds_auth.c` | 83 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 48 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/tests/test_dds_auth.c` | 88 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 49 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/tests/test_dds_auth.c` | 89 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 50 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/tests/test_dds_access.c` | 171 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 51 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/tests/test_dds_security_manager.c` | 204 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 52 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/tests/test_dds_crypto.c` | 264 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 53 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/tests/test_dds_core.c` | 194 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 54 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/tests/test_dds_core.c` | 195 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 55 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/tests/test_tsn_stack.c` | 417 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 56 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_qos.c` | 43 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 57 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_qos.c` | 53 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 58 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_qos.c` | 62 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 59 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_topic.c` | 60 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 60 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_domain.c` | 53 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 61 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_domain.c` | 207 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 62 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_subscriber.c` | 60 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 63 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_subscriber.c` | 145 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 64 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_subscriber.c` | 146 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 65 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_subscriber.c` | 186 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 66 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_subscriber.c` | 187 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 67 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_publisher.c` | 60 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 68 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_publisher.c` | 145 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 69 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/tests/unity/test_publisher.c` | 146 | 0 | The comma operator should not be used [misra-c2012-12.3] |

## Fix Checklist

- [ ] Understand the violation context
- [ ] Apply fix to source code
- [ ] Re-run MISRA check to verify fix
- [ ] Update traceability matrix
- [ ] Document deviation if fix is not feasible

---
*Generated by yuleOSH MISRA fix-task generator*