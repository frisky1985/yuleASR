# MISRA Fix Task: misra-c2023-17.8

> Generated: 2026-08-07T12:39:04.034524
> Severity: required
> Spec Ref: SWE-MISRA-S1

## Rule: Boolean arguments for bool parameter

布尔型参数应使用 bool 类型

## Violations

| # | File | Line | Col | Message |
|--:|:-----|:----|:----|:--------|
| 1 | `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/storage_manager/src/Swc_StorageManager.c` | 278 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 2 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_partition.c` | 73 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 3 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_partition.c` | 76 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 4 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_partition.c` | 78 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 5 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinSlave_Pid.c` | 33 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 6 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinMaster.c` | 722 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 7 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/src/Boot_Image.c` | 82 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 8 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Private.h` | 187 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 9 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Private.h` | 192 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 10 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/someipxf/src/SomeIpXf.c` | 144 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 11 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/dcm/src/dcm_transfer.c` | 214 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 12 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/dcm/src/dcm_transfer.c` | 221 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 13 | `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/mbedtls/x509.h` | 357 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 14 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/utils/eth_utils.c` | 56 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 15 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/utils/eth_utils.c` | 57 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 16 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_core.c` | 238 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 17 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_core.c` | 239 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 18 | `/Users/stefan/.openclaw/workspace/yuleASR/src/telemetry/telemetry.c` | 143 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 19 | `/Users/stefan/.openclaw/workspace/yuleASR/src/telemetry/telemetry.c` | 144 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 20 | `/Users/stefan/.openclaw/workspace/yuleASR/src/telemetry/telemetry.c` | 195 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 21 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 146 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 22 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 147 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 23 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 563 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 24 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 570 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 25 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 575 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 26 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 576 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 27 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 578 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 28 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 579 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 29 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 586 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 30 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/ownership.c` | 253 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 31 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/ownership.c` | 256 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 32 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 70 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 33 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 71 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 34 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 82 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 35 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 86 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 36 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 91 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 37 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 92 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 38 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 96 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 39 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 97 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 40 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/cbs/cbs.c` | 427 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 41 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/fp/frame_preemption.c` | 86 | 0 | A function parameter should not be modified [misra-c2012-17.8] |
| 42 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/transport/udp.c` | 339 | 0 | A function parameter should not be modified [misra-c2012-17.8] |

## Fix Checklist

- [ ] Understand the violation context
- [ ] Apply fix to source code
- [ ] Re-run MISRA check to verify fix
- [ ] Update traceability matrix
- [ ] Document deviation if fix is not feasible

---
*Generated by yuleOSH MISRA fix-task generator*