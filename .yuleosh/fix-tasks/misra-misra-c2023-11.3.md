# MISRA Fix Task: misra-c2023-11.3

> Generated: 2026-08-07T14:30:06.005840
> Severity: required
> Spec Ref: SWE-MISRA-S1

## Rule: Cast of pointer to integer type

将指针转换为整数类型应谨慎

## Violations

| # | File | Line | Col | Message |
|--:|:-----|:----|:----|:--------|
| 1 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_Cfg.c` | 428 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 2 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mem/src/Mem.c` | 160 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 3 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mem/src/Mem.c` | 220 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 4 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/ethSm/src/EthSM_Lcfg.c` | 292 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 5 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_optimized.c` | 153 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 6 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/eth/dds_eth_transport.c` | 838 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 7 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/eth/dds_eth_transport.c` | 885 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 8 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/eth/dds_eth_transport.c` | 928 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 9 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/eth/dds_eth_transport.c` | 970 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 10 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/eth/dds_eth_transport.c` | 1106 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 11 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 854 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 12 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 906 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 13 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 375 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 14 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 378 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 15 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 381 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 16 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 384 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 17 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 387 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 18 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 390 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 19 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 393 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 20 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 396 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 21 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 399 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 22 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/transport/udp.c` | 644 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 23 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/transport/udp.c` | 661 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 24 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/transport/udp.c` | 662 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 25 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/transport/udp.c` | 705 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 26 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/transport/udp.c` | 706 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 27 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/transport/udp.c` | 1155 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 28 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/subscriber.c` | 34 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 29 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/subscriber.c` | 74 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 30 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/subscriber.c` | 94 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 31 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/writer.c` | 36 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 32 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/writer.c` | 92 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 33 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/writer.c` | 114 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 34 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/writer.c` | 136 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 35 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/topic.c` | 36 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 36 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/topic.c` | 87 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 37 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/topic.c` | 119 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 38 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/topic.c` | 135 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 39 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/topic.c` | 149 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 40 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/reader.c` | 36 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 41 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/reader.c` | 90 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 42 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/reader.c` | 112 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 43 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/reader.c` | 133 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 44 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/reader.c` | 170 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 45 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/publisher.c` | 34 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 46 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/publisher.c` | 74 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 47 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/publisher.c` | 94 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 48 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/domain.c` | 36 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 49 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/domain.c` | 104 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 50 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/domain.c` | 125 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 51 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/domain.c` | 148 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 52 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/core/domain.c` | 167 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 53 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/qos/qos_policy.c` | 73 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 54 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/qos/qos_policy.c` | 88 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 55 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/qos/qos_policy.c` | 128 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 56 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_icmp.c` | 377 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 57 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_udp.c` | 326 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |
| 58 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_arp.c` | 334 | 0 | Cast between pointer to different object types shall not be performed [misra-c20 |

## Fix Checklist

- [ ] Understand the violation context
- [ ] Apply fix to source code
- [ ] Re-run MISRA check to verify fix
- [ ] Update traceability matrix
- [ ] Document deviation if fix is not feasible

---
*Generated by yuleOSH MISRA fix-task generator*