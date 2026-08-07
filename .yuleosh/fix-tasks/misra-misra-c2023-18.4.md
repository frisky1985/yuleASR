# MISRA Fix Task: misra-c2023-18.4

> Generated: 2026-08-07T12:39:04.034733
> Severity: required
> Spec Ref: SWE-MISRA-S1

## Rule: + - += -= shall not be applied to pointer to void

+ - += -= 不应用于 void 指针

## Violations

| # | File | Line | Col | Message |
|--:|:-----|:----|:----|:--------|
| 1 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c` | 571 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 2 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c` | 572 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 3 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c` | 581 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 4 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c` | 593 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 5 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c` | 608 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 6 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_partition.c` | 627 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 7 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 251 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 8 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 322 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 9 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 327 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 10 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 343 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 11 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 410 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 12 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 415 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 13 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/src/Crypto_MbedTLS.c` | 722 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 14 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 286 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 15 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 287 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 16 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 297 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 17 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 328 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 18 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 332 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 19 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 339 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 20 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 346 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 21 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 348 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 22 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 363 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 23 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 364 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 24 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 387 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 25 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 388 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 26 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 441 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 27 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 452 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 28 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 457 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 29 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 579 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 30 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 582 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 31 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 632 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 32 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 642 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 33 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 649 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 34 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 655 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 35 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 771 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 36 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 774 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 37 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 852 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 38 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 865 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 39 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 879 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 40 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 881 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 41 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 895 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 42 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 896 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 43 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 910 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 44 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 911 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 45 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 963 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 46 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 1000 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 47 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 1005 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 48 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 1028 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 49 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 1035 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 50 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 1048 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 51 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 1055 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 52 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 1063 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 53 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/uart/src/Uart.c` | 1072 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 54 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 179 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 55 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 197 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 56 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/src/Boot_Image.c` | 94 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 57 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/xcp/src/Xcp.c` | 1575 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 58 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/xcp/src/Xcp.c` | 1594 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 59 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/xcp/src/Xcp.c` | 1595 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 60 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/xcp/src/Xcp.c` | 1598 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 61 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/sd/src/Sd.c` | 414 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 62 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/sd/src/Sd.c` | 489 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 63 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/someip/src/SomeIp.c` | 178 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 64 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mem/src/Mem.c` | 220 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 65 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mem/src/Mem.c` | 534 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 66 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mem/src/Mem.c` | 606 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 67 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 391 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 68 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 403 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 69 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 408 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 70 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 413 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 71 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 416 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 72 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 425 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 73 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 430 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 74 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 435 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 75 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 439 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 76 | `/Users/stefan/.openclaw/workspace/yuleASR/src/telemetry/telemetry.c` | 281 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 77 | `/Users/stefan/.openclaw/workspace/yuleASR/src/telemetry/telemetry_dds.c` | 61 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 78 | `/Users/stefan/.openclaw/workspace/yuleASR/src/telemetry/tests/test_telemetry.c` | 681 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 79 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/eth/dds_eth_transport.c` | 1117 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 80 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 501 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 81 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 515 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 82 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 516 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 83 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 867 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 84 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 882 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 85 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 939 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 86 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_crypto.c` | 940 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 87 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 146 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 88 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 158 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 89 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 163 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 90 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 570 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 91 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 601 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 92 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_auth.c` | 765 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 93 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 140 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 94 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 159 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 95 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 284 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 96 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 305 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 97 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 309 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 98 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 313 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 99 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c` | 397 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 100 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 283 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 101 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 365 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 102 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/fp/frame_preemption.c` | 369 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 103 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/fp/frame_preemption.c` | 370 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 104 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/fp/frame_preemption.c` | 374 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 105 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/fp/frame_preemption.c` | 420 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 106 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/fp/frame_preemption.c` | 421 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 107 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/fp/frame_preemption.c` | 425 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 108 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/fp/frame_preemption.c` | 501 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 109 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/fp/frame_preemption.c` | 597 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 110 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tsn/fp/frame_preemption.c` | 751 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 111 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/transport/udp.c` | 662 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 112 | `/Users/stefan/.openclaw/workspace/yuleASR/src/micro-dds/src/transport/udp.c` | 706 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |
| 113 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_icmp.c` | 384 | 0 | Pointer arithmetic operators should not be applied to pointer type [misra-c2012- |

## Fix Checklist

- [ ] Understand the violation context
- [ ] Apply fix to source code
- [ ] Re-run MISRA check to verify fix
- [ ] Update traceability matrix
- [ ] Document deviation if fix is not feasible

---
*Generated by yuleOSH MISRA fix-task generator*