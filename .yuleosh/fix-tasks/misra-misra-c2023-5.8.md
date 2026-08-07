# MISRA Fix Task: misra-c2023-5.8

> Generated: 2026-08-07T12:39:04.047216
> Severity: required
> Spec Ref: SWE-MISRA-S1

## Rule: Identifiers in external scope shall be distinct

外部作用域的标识符应唯一

## Violations

| # | File | Line | Col | Message |
|--:|:-----|:----|:----|:--------|
| 1 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/swc/src/Swc.c` | 68 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 2 | `/Users/stefan/.openclaw/workspace/yuleASR/src/rte/include/Rte_Swc.h` | 223 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 3 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/can/src/Can.c` | 96 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 4 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/can/src/Can_Lcfg.c` | 6 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 5 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/pwm/src/Pwm.c` | 74 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 6 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/pwm/src/Pwm_Lcfg.c` | 6 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 7 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/gpt/src/Gpt.c` | 84 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 8 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/gpt/src/Gpt_Lcfg.c` | 6 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 9 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/spi/src/Spi.c` | 98 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 10 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/spi/src/Spi_Lcfg.c` | 7 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 11 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_core.c` | 174 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 12 | `/Users/stefan/.openclaw/workspace/yuleASR/third_party/crypto/hash/include/hash_algos.h` | 247 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 13 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_core.c` | 178 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 14 | `/Users/stefan/.openclaw/workspace/yuleASR/third_party/crypto/hash/include/hash_algos.h` | 256 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 15 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_core.c` | 189 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 16 | `/Users/stefan/.openclaw/workspace/yuleASR/third_party/crypto/hash/include/hash_algos.h` | 264 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 17 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/xcp/src/Xcp_Lcfg.c` | 168 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 18 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/xcp/src/Xcp.c` | 2021 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 19 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/linif/include/LinIf.h` | 52 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 20 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/os/include/Os.h` | 163 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 21 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/linSM/src/LinSM_Lcfg.c` | 126 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 22 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/linsm/src/LinSM_Lcfg.c` | 4 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 23 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/comM/src/ComM.c` | 51 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 24 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/comM/src/ComM_Lcfg.c` | 343 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 25 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/doIP/src/DoIP.c` | 67 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 26 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/doip/src/DoIP.c` | 186 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 27 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/dlt/src/Dlt_Lcfg.c` | 26 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 28 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/dlt/src/Dlt.c` | 29 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 29 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/dlt/src/Dlt_Lcfg.c` | 321 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 30 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/dlt/src/Dlt.c` | 31 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 31 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/mcu/src/Mcu.c` | 84 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 32 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/mcu/src/Mcu_Lcfg.c` | 5 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 33 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/linTp/src/LinTp.c` | 84 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 34 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/lntm/src/LinTp.c` | 147 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 35 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/linTp/src/LinTp.c` | 85 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 36 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/lntm/src/LinTp.c` | 188 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 37 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/linTp/src/LinTp.c` | 86 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 38 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/lntm/src/LinTp.c` | 215 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 39 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/someipsd/src/SomeIpSd.c` | 76 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 40 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/sd/src/Sd.c` | 306 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 41 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/linSM/src/LinSM.c` | 114 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 42 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/linsm/src/LinSM_Lcfg.c` | 5 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 43 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Private.h` | 131 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 44 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/com/src/Com.c` | 336 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 45 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/xcp/src/Xcp.c` | 74 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 46 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/xcp/src/Xcp_Lcfg.c` | 55 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 47 | `/Users/stefan/.openclaw/workspace/yuleASR/src/telemetry/tests/test_telemetry.c` | 385 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |
| 48 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/tests/test_dds_eth_transport.c` | 53 | 0 | Identifiers that define objects or functions shall have unique names [misra-c201 |

## Fix Checklist

- [ ] Understand the violation context
- [ ] Apply fix to source code
- [ ] Re-run MISRA check to verify fix
- [ ] Update traceability matrix
- [ ] Document deviation if fix is not feasible

---
*Generated by yuleOSH MISRA fix-task generator*