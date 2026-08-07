# MISRA Fix Task: misra-c2023-11.5

> Generated: 2026-08-07T12:39:04.021586
> Severity: required
> Spec Ref: SWE-MISRA-S1

## Rule: Pointer to void shall not be converted to pointer to object

void 指针不应转换为对象指针（除赋值）

## Violations

| # | File | Line | Col | Message |
|--:|:-----|:----|:----|:--------|
| 1 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 297 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 2 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 330 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 3 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 379 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 4 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 416 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 5 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 480 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 6 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 512 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 7 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 559 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 8 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 587 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 9 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 631 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 10 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 665 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 11 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 727 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 12 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 763 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 13 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 827 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 14 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 863 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 15 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 934 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 16 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c` | 987 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 17 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c` | 111 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 18 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c` | 126 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 19 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c` | 571 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 20 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c` | 581 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 21 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c` | 234 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 22 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c` | 267 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 23 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c` | 321 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 24 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c` | 377 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 25 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c` | 507 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 26 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_rollback.c` | 164 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 27 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_rollback.c` | 422 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 28 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_rollback.c` | 596 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 29 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 168 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 30 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 381 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 31 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Signal.c` | 61 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 32 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Signal.c` | 64 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 33 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Signal.c` | 67 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 34 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Signal.c` | 70 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 35 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Signal.c` | 73 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 36 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Signal.c` | 76 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 37 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Signal.c` | 79 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 38 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Signal.c` | 82 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 39 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Signal.c` | 85 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 40 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Transmit.c` | 341 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 41 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Transmit.c` | 344 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 42 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Transmit.c` | 347 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 43 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Transmit.c` | 350 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 44 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Transmit.c` | 353 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 45 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Transmit.c` | 356 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 46 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Transmit.c` | 359 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 47 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Transmit.c` | 362 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 48 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Transmit.c` | 365 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 49 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_Transmit.c` | 368 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 50 | `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/mbedtls/pk.h` | 1041 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 51 | `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/mbedtls/pk.h` | 1066 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 52 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_Tls.c` | 708 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 53 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_Tls.c` | 728 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 54 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_Tls.c` | 748 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 55 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/com/src/Com.c` | 288 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 56 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/com/src/Com.c` | 298 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 57 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/com/src/Com.c` | 302 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 58 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/com/src/Com.c` | 314 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 59 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/com/src/Com.c` | 324 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 60 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/com/src/Com.c` | 328 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 61 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mem/src/Mem.c` | 562 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 62 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mem/src/Mem.c` | 563 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 63 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/someipsd/src/SomeIpSd.c` | 108 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 64 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/ecual/srp/src/Srp.c` | 75 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 65 | `/Users/stefan/.openclaw/workspace/yuleASR/src/ethernet/driver/eth_dma.c` | 146 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 66 | `/Users/stefan/.openclaw/workspace/yuleASR/src/ethernet/driver/eth_dma.c` | 203 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 67 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 215 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 68 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 783 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 69 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 837 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 70 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 996 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 71 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 1016 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 72 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/keym/keym_core.c` | 67 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 73 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/keym/keym_core.c` | 68 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 74 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/keym/keym_core.c` | 270 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 75 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/keym/keym_core.c` | 345 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 76 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/keym/keym_core.c` | 513 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 77 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/keym/keym_core.c` | 679 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 78 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/keym/keym_core.c` | 721 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 79 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_cryif.c` | 53 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 80 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_cryif.c` | 60 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 81 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_cryif.c` | 80 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 82 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_cryif.c` | 103 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 83 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_cryif.c` | 124 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 84 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_cryif.c` | 142 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 85 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_keym.c` | 258 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 86 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/tests/test_csm.c` | 348 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 87 | `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/secoc/secoc_optimized.c` | 199 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 88 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/core/dds_domain.c` | 282 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 89 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/core/dds_domain.c` | 410 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 90 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/core/dds_domain.c` | 417 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 91 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/core/dds_domain.c` | 424 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 92 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/core/dds_domain.c` | 440 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 93 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/core/dds_domain.c` | 447 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 94 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/core/dds_domain.c` | 454 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 95 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/ownership.c` | 64 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 96 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/ownership.c` | 65 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 97 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 365 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 98 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 369 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 99 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 372 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 100 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 375 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 101 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 378 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 102 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 381 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 103 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 384 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 104 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 387 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 105 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 390 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 106 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 393 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 107 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 396 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 108 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 399 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 109 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 402 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 110 | `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c` | 403 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 111 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_socket.c` | 561 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 112 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_socket.c` | 569 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 113 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_socket.c` | 578 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 114 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_socket.c` | 587 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 115 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_socket.c` | 596 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 116 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_socket.c` | 605 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 117 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_socket.c` | 615 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 118 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_socket.c` | 624 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 119 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_socket.c` | 657 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |
| 120 | `/Users/stefan/.openclaw/workspace/yuleASR/src/tcpip/tcpip_socket.c` | 666 | 0 | Conversion from pointer to void to pointer to object shall not be performed [mis |

## Fix Checklist

- [ ] Understand the violation context
- [ ] Apply fix to source code
- [ ] Re-run MISRA check to verify fix
- [ ] Update traceability matrix
- [ ] Document deviation if fix is not feasible

---
*Generated by yuleOSH MISRA fix-task generator*