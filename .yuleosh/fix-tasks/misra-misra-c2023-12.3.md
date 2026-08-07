# MISRA Fix Task: misra-c2023-12.3

> Generated: 2026-08-07T14:51:08.929022
> Severity: advisory
> Spec Ref: SWE-MISRA-S1

## Rule: sizeof 操作数约束

sizeof 操作符不应应用于具有副作用的表达式

## Violations

| # | File | Line | Col | Message |
|--:|:-----|:----|:----|:--------|
| 1 | `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/vehicle_dynamics/src/Swc_VehicleDynamics.c` | 275 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 2 | `/Users/stefan/.openclaw/workspace/yuleASR/src/platform/s32k312/src/Platform_RamSafety.c` | 443 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 3 | `/Users/stefan/.openclaw/workspace/yuleASR/src/platform/s32k312/src/Platform_Lockstep.c` | 555 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 4 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Safety_1.0.0.c` | 133 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 5 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 216 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 6 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 298 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 7 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 373 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 8 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinSlave_Pid.c` | 29 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 9 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinMaster.c` | 719 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 10 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 176 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 11 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 197 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 12 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 229 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 13 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 311 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 14 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/soad/src/SoAd_Test.c` | 396 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 15 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt.c` | 146 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 16 | `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/mbedtls/x509.h` | 247 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 17 | `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/mbedtls/x509.h` | 248 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 18 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c` | 416 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 19 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/utils/eth_utils.c` | 40 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 20 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_core.c` | 145 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 21 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_core.c` | 146 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 22 | `/Users/stefan/.openclaw/workspace/yuleASR/src/telemetry/telemetry.c` | 323 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 23 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 84 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 24 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 85 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 25 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 218 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 26 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 84 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 27 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 86 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 28 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 449 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 29 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 503 | 0 | The comma operator should not be used [misra-c2012-12.3] |
| 30 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 234 | 0 | The comma operator should not be used [misra-c2012-12.3] |

## Fix Checklist

- [ ] Understand the violation context
- [ ] Apply fix to source code
- [ ] Re-run MISRA check to verify fix
- [ ] Update traceability matrix
- [ ] Document deviation if fix is not feasible

---
*Generated by yuleOSH MISRA fix-task generator*