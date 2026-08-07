# MISRA Compliance Report

**Generated**: 2026-08-07T17:25:17.509036
**Tool**: Cppcheck 2.17.1 from cppcheck-wheel 1.5.1
**Ruleset**: 2023

## Summary

- **Total Violations**: 2530
- **Unique Rules**: 23
- **Affected Files**: 207
- **Density**: 19.94 violations/KLOC

## By Severity

| Severity | Count |
|---------|------:|
| error | 27 |
| warning | 27 |
| style | 2375 |
| portability | 3 |

## By Rule Type

| Type | Count |
|------|------:|
| required | 30 |
| advisory | 29 |

## By Category

| Category | Count |
|----------|------:|
| 指针 (Pointer) | 1305 |
| unknown | 1135 |
| 标准库 (Standard Library) | 33 |
| 基本类型 (Essential Types) | 12 |
| 函数 (Functions) | 11 |
| 字符集 (Character Sets) | 8 |
| 指针类型转换 (Pointer Type Conversions) | 5 |
| 资源 (Resources) | 4 |
| 控制流 (Control Flow) | 3 |
| 副作用 (Side Effects) | 3 |
| 标识符 (Identifiers) | 3 |
| 注释 (Comments) | 2 |
| 预处理器 (Preprocessing) | 2 |
| 初始化 (Initialization) | 2 |
| 指针与数组 (Pointers & Arrays) | 1 |
| 字面量 (Literals) | 1 |

## Violations by Rule

- **misra-c2023-11.9** (1305 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:77` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:248` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:367` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:392` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:397` — The macro NULL shall be the only permitted form of integer null pointer constant
- **unknown** (1135 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:458` — Variable 'state->counter' is reassigned a value before the old one has been used
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:444` — state->counter is assigned
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:458` — state->counter is overwritten
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:459` — Variable '*status' is reassigned a value before the old one has been used. [redu
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:453` — *status is assigned
- **misra-c2023-10.5** (12 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Hsm_1.0.0.c:173` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Lockstep_1.0.0.c:307` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Lockstep_1.0.0.c:308` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Lockstep_1.0.0.c:309` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c:800` — misra violation 1005 with no text in the supplied rule-texts-file [misra-c2012-1
- **misra-c2023-21.16** (12 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/classic/com/Com_TxMode.c:371` — misra violation 2116 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c:800` — misra violation 2116 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c:802` — misra violation 2116 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c:804` — misra violation 2116 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c:806` — misra violation 2116 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-17.1** (11 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.h:260` — misra violation 1701 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:650` — misra violation 1701 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:729` — misra violation 1701 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:730` — misra violation 1701 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:732` — misra violation 1701 with no text in the supplied rule-texts-file [misra-c2012-1
- **misra-c2023-21.2** (9 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/tests/qemu_m33/include/string.h:9` — misra violation 2102 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/tests/qemu_m33/include/string.h:10` — misra violation 2102 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/tests/qemu_m33/include/string.h:11` — misra violation 2102 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/tests/qemu_m33/include/string.h:12` — misra violation 2102 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/tests/qemu_m33/include/string.h:13` — misra violation 2102 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-4.1** (8 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:27` — misra violation 401 with no text in the supplied rule-texts-file [misra-c2012-4.
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:28` — misra violation 401 with no text in the supplied rule-texts-file [misra-c2012-4.
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:29` — misra violation 401 with no text in the supplied rule-texts-file [misra-c2012-4.
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:30` — misra violation 401 with no text in the supplied rule-texts-file [misra-c2012-4.
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:31` — misra violation 401 with no text in the supplied rule-texts-file [misra-c2012-4.
- **misra-c2023-21.14** (7 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c:800` — misra violation 2114 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c:802` — misra violation 2114 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c:804` — misra violation 2114 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c:806` — misra violation 2114 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/mqtt/src/Mqtt_CertMgr.c:808` — misra violation 2114 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-11.1** (5 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:746` — misra violation 1101 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/exec/process_manager.c:382` — misra violation 1101 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/exec/process_manager.c:411` — misra violation 1101 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:787` — misra violation 1101 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:1020` — misra violation 1101 with no text in the supplied rule-texts-file [misra-c2012-1
- **misra-c2023-21.7** (4 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c:263` — misra violation 2107 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c:384` — misra violation 2107 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c:289` — misra violation 2107 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/pubsub/content_filtered_topic.c:297` — misra violation 2107 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-13.4** (3 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c:995` — misra violation 1304 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/csm/csm_core.c:547` — misra violation 1304 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/crypto_stack/csm/csm_core.c:578` — misra violation 1304 with no text in the supplied rule-texts-file [misra-c2012-1
- **misra-c2023-5.5** (3 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/eth/dds_tcpip_compat.c:33` — misra violation 505 with no text in the supplied rule-texts-file [misra-c2012-5.
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/eth/dds_tcpip_compat.c:67` — misra violation 505 with no text in the supplied rule-texts-file [misra-c2012-5.
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/transport/eth/dds_tcpip_compat.c:73` — misra violation 505 with no text in the supplied rule-texts-file [misra-c2012-5.
- **misra-c2023-16.4** (2 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c:327` — misra violation 1604 with no text in the supplied rule-texts-file [misra-c2012-1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c:385` — misra violation 1604 with no text in the supplied rule-texts-file [misra-c2012-1
- **misra-c2023-3.1** (2 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/cross/hello.c:1` — misra violation 301 with no text in the supplied rule-texts-file [misra-c2012-3.
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/fee/src/Fee_Lcfg.c:212` — misra violation 301 with no text in the supplied rule-texts-file [misra-c2012-3.
- **misra-c2023-9.4** (2 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/psa/crypto_struct.h:479` — misra violation 904 with no text in the supplied rule-texts-file [misra-c2012-9.
  - `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/psa/crypto_struct.h:517` — misra violation 904 with no text in the supplied rule-texts-file [misra-c2012-9.
- **misra-c2023-22.8** (2 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c:184` — misra violation 2208 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c:346` — misra violation 2208 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-22.9** (2 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c:184` — misra violation 2209 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/security/dds_access.c:346` — misra violation 2209 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-20.4** (1 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/mbedtls/build_info.h:101` — misra violation 2004 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-20.13** (1 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/src/Det.c:35` — misra violation 2013 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-18.7** (1 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/third_party/mbedtls/include/psa/crypto_struct.h:254` — misra violation 1807 with no text in the supplied rule-texts-file [misra-c2012-1

... and 3 more rules