# MISRA Compliance Report

**Generated**: 2026-08-07T14:51:04.548908
**Tool**: Cppcheck 2.17.1 from cppcheck-wheel 1.5.1
**Ruleset**: 2023

## Summary

- **Total Violations**: 31433
- **Unique Rules**: 86
- **Affected Files**: 729
- **Density**: 107.44 violations/KLOC

## By Severity

| Severity | Count |
|---------|------:|
| error | 21 |
| warning | 28 |
| style | 31281 |
| portability | 3 |

## By Rule Type

| Type | Count |
|------|------:|
| required | 11005 |
| advisory | 19285 |

## By Category

| Category | Count |
|----------|------:|
| 未分类 (Uncategorized) | 10822 |
| 控制流 (Control Flow) | 6658 |
| 声明 (Declarations) | 3172 |
| 函数行为 | 2329 |
| 基本类型 (Essential Types) | 2060 |
| 表达式 (Expressions) | 1434 |
| 预处理器 (Preprocessing) | 1224 |
| 指针 (Pointer) | 1144 |
| unknown | 1143 |
| 标识符 (Identifiers) | 356 |
| 指针类型转换 (Pointer Type Conversions) | 307 |
| 标准库 (Standard Library) | 292 |
| 函数 (Functions) | 140 |
| 指针与数组 (Pointers & Arrays) | 113 |
| 字面量 (Literals) | 86 |
| 覆盖存储 (Overlapping Storage) | 78 |
| 初始化 (Initialization) | 51 |
| 副作用 (Side Effects) | 10 |
| 字符集 (Character Sets) | 8 |
| 资源 (Resources) | 4 |
| 注释 (Comments) | 2 |

## Violations by Rule

- **misra-c2023-2.5** (10207 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:24` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:18` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:19` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:24` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:25` — A project should not contain unused macro declarations [misra-c2012-2.5]
- **misra-c2023-15.5** (5684 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:61` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:93` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:95` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:97` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:99` — A function should have a single point of exit [misra-c2012-15.5]
- **misra-c2023-17.7** (2329 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:192` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:259` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:285` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:440` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:446` — Return value of non-void function shall be used [misra-c2012-17.7]
- **misra-c2023-8.7** (2250 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:171` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:202` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:244` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:297` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:363` — Functions/objects should not have external linkage if referenced in one TU [misr
- **misra-c2023-10.4** (1795 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:252` — Both operands shall have same essential type category [misra-c2012-10.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:284` — Both operands shall have same essential type category [misra-c2012-10.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:682` — Both operands shall have same essential type category [misra-c2012-10.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:226` — Both operands shall have same essential type category [misra-c2012-10.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:303` — Both operands shall have same essential type category [misra-c2012-10.4]
- **misra-c2023-12.1** (1360 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:156` — Operator precedence within expressions should be made explicit [misra-c2012-12.1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:248` — Operator precedence within expressions should be made explicit [misra-c2012-12.1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:284` — Operator precedence within expressions should be made explicit [misra-c2012-12.1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:392` — Operator precedence within expressions should be made explicit [misra-c2012-12.1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:472` — Operator precedence within expressions should be made explicit [misra-c2012-12.1
- **misra-c2023-11.9** (1144 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:77` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:248` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:367` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:392` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:397` — The macro NULL shall be the only permitted form of integer null pointer constant
- **unknown** (1143 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:458` — Variable 'state->counter' is reassigned a value before the old one has been used
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:444` — state->counter is assigned
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:458` — state->counter is overwritten
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:459` — Variable '*status' is reassigned a value before the old one has been used. [redu
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:453` — *status is assigned
- **misra-c2023-20.1** (674 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:109` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:133` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:156` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:779` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/include/Swc_DiagnosticManager.h:152` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-14.4** (545 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:453` — Controlling expression of if shall have essentially Boolean type [misra-c2012-14
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:455` — Controlling expression of if shall have essentially Boolean type [misra-c2012-14
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/watchdog_manager/src/Swc_WatchdogManager.c:187` — Controlling expression of if shall have essentially Boolean type [misra-c2012-14
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/watchdog_manager/src/Swc_WatchdogManager.c:316` — Controlling expression of if shall have essentially Boolean type [misra-c2012-14
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/watchdog_manager/src/Swc_WatchdogManager.c:467` — Controlling expression of if shall have essentially Boolean type [misra-c2012-14
- **misra-c2023-8.4** (471 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/platform/s32k312/src/Platform_RamSafety.c:37` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/cross/hello.c:21` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/cross/hello.c:19` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Safety_1.0.0.c:111` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Safety_1.0.0.c:130` — A compatible declaration shall be visible when an object or function is defined 
- **misra-c2023-2.3** (328 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:41` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:68` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:122` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:119` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.h:123` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
- **misra-c2023-20.5** (295 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:26` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:27` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:31` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:32` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:36` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-15.6** (288 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:406` — Body of iteration/selection statement shall be compound [misra-c2012-15.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:486` — Body of iteration/selection statement shall be compound [misra-c2012-15.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:584` — Body of iteration/selection statement shall be compound [misra-c2012-15.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:671` — Body of iteration/selection statement shall be compound [misra-c2012-15.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:672` — Body of iteration/selection statement shall be compound [misra-c2012-15.6]
- **misra-c2023-8.6** (251 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_verify.c:103` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/cross/hello.c:26` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c:408` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/src/Boot_Verify.c:102` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c:122` — An identifier with external linkage shall have exactly one external definition [
- **misra-c2023-2.4** (179 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:41` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:68` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/rtps/rtps_message.h:105` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/rtps/rtps_message.h:116` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/rtps/rtps_message.h:126` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
- **misra-c2023-21.3** (158 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:241` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:254` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:288` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:295` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:386` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
- **misra-c2023-5.9** (137 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:24` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:23` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:23` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:22` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:40` — Identifiers that define objects or functions shall be unique before linker stage
- **misra-c2023-5.6** (120 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/include/Crypto_Types.h:176` — A typedef name shall be a unique identifier [misra-c2012-5.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/third_party/crypto/aes_modes/include/CryptoStack_Types.h:20` — A typedef name shall be a unique identifier [misra-c2012-5.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/include/Crypto_Types.h:173` — A typedef name shall be a unique identifier [misra-c2012-5.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/third_party/crypto/aes_modes/include/CryptoStack_Types.h:21` — A typedef name shall be a unique identifier [misra-c2012-5.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/spi/include/Spi_Cfg.h:38` — A typedef name shall be a unique identifier [misra-c2012-5.6]
- **misra-c2023-20.9** (113 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Safety_1.0.0.c:35` — Preprocessor identifiers shall be defined before use [misra-c2012-20.9]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Boot_1.0.0.c:30` — Preprocessor identifiers shall be defined before use [misra-c2012-20.9]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Hsm_1.0.0.c:31` — Preprocessor identifiers shall be defined before use [misra-c2012-20.9]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_RamEcc_1.0.0.c:36` — Preprocessor identifiers shall be defined before use [misra-c2012-20.9]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd.c:40` — Preprocessor identifiers shall be defined before use [misra-c2012-20.9]

... and 66 more rules