# MISRA Compliance Report

**Generated**: 2026-08-07T16:48:16.344440
**Tool**: Cppcheck 2.17.1 from cppcheck-wheel 1.5.1
**Ruleset**: 2023

## Summary

- **Total Violations**: 26798
- **Unique Rules**: 85
- **Affected Files**: 721
- **Density**: 94.29 violations/KLOC

## By Severity

| Severity | Count |
|---------|------:|
| error | 27 |
| warning | 27 |
| style | 26643 |
| portability | 3 |

## By Rule Type

| Type | Count |
|------|------:|
| required | 7051 |
| advisory | 18612 |

## By Category

| Category | Count |
|----------|------:|
| 未分类 (Uncategorized) | 10253 |
| 控制流 (Control Flow) | 5717 |
| 声明 (Declarations) | 3127 |
| 函数行为 | 2294 |
| 指针 (Pointer) | 1305 |
| 预处理器 (Preprocessing) | 1207 |
| unknown | 1135 |
| 标识符 (Identifiers) | 326 |
| 指针类型转换 (Pointer Type Conversions) | 302 |
| 标准库 (Standard Library) | 289 |
| 基本类型 (Essential Types) | 242 |
| 表达式 (Expressions) | 133 |
| 函数 (Functions) | 130 |
| 指针与数组 (Pointers & Arrays) | 112 |
| 字面量 (Literals) | 85 |
| 覆盖存储 (Overlapping Storage) | 74 |
| 初始化 (Initialization) | 50 |
| 字符集 (Character Sets) | 8 |
| 资源 (Resources) | 4 |
| 副作用 (Side Effects) | 3 |
| 注释 (Comments) | 2 |

## Violations by Rule

- **misra-c2023-2.5** (9648 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:24` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:18` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:19` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:24` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:25` — A project should not contain unused macro declarations [misra-c2012-2.5]
- **misra-c2023-15.5** (5582 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:61` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:93` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:95` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:97` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:99` — A function should have a single point of exit [misra-c2012-15.5]
- **misra-c2023-17.7** (2294 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:192` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:259` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:285` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:440` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:446` — Return value of non-void function shall be used [misra-c2012-17.7]
- **misra-c2023-8.7** (2235 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:171` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:202` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:244` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:297` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:363` — Functions/objects should not have external linkage if referenced in one TU [misr
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
- **misra-c2023-20.1** (670 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:109` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:133` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:156` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:779` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/include/Swc_DiagnosticManager.h:152` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-8.4** (449 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/platform/s32k312/src/Platform_RamSafety.c:37` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/cross/hello.c:21` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/cross/hello.c:19` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Safety_1.0.0.c:111` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Safety_1.0.0.c:130` — A compatible declaration shall be visible when an object or function is defined 
- **misra-c2023-2.3** (325 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:41` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:68` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:122` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:119` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.h:123` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
- **misra-c2023-20.5** (283 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:26` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:27` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:31` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:32` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:36` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-8.6** (247 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_verify.c:103` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/cross/hello.c:26` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c:408` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/src/Boot_Verify.c:102` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c:122` — An identifier with external linkage shall have exactly one external definition [
- **misra-c2023-2.4** (177 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:41` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:68` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/rtps/rtps_message.h:105` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/rtps/rtps_message.h:116` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/rtps/rtps_message.h:126` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
- **misra-c2023-21.3** (158 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:241` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:254` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:288` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:296` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:387` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
- **misra-c2023-5.6** (120 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/include/Crypto_Types.h:176` — A typedef name shall be a unique identifier [misra-c2012-5.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/third_party/crypto/aes_modes/include/CryptoStack_Types.h:20` — A typedef name shall be a unique identifier [misra-c2012-5.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/crypto/include/Crypto_Types.h:173` — A typedef name shall be a unique identifier [misra-c2012-5.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/third_party/crypto/aes_modes/include/CryptoStack_Types.h:21` — A typedef name shall be a unique identifier [misra-c2012-5.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/spi/include/Spi_Cfg.h:38` — A typedef name shall be a unique identifier [misra-c2012-5.6]
- **misra-c2023-20.9** (112 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Safety_1.0.0.c:35` — Preprocessor identifiers shall be defined before use [misra-c2012-20.9]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Boot_1.0.0.c:30` — Preprocessor identifiers shall be defined before use [misra-c2012-20.9]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Hsm_1.0.0.c:31` — Preprocessor identifiers shall be defined before use [misra-c2012-20.9]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_RamEcc_1.0.0.c:36` — Preprocessor identifiers shall be defined before use [misra-c2012-20.9]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd.c:40` — Preprocessor identifiers shall be defined before use [misra-c2012-20.9]
- **misra-c2023-18.4** (111 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:573` — Pointer arithmetic operators should not be applied to pointer type [misra-c2012-
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:574` — Pointer arithmetic operators should not be applied to pointer type [misra-c2012-
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:583` — Pointer arithmetic operators should not be applied to pointer type [misra-c2012-
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c:593` — Pointer arithmetic operators should not be applied to pointer type [misra-c2012-
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_secure_boot.c:608` — Pointer arithmetic operators should not be applied to pointer type [misra-c2012-
- **misra-c2023-11.5** (109 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:297` — Conversion from pointer to void to pointer to object shall not be performed [mis
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:330` — Conversion from pointer to void to pointer to object shall not be performed [mis
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:379` — Conversion from pointer to void to pointer to object shall not be performed [mis
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:416` — Conversion from pointer to void to pointer to object shall not be performed [mis
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:480` — Conversion from pointer to void to pointer to object shall not be performed [mis
- **misra-c2023-5.9** (107 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:23` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:22` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:24` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:23` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:40` — Identifiers that define objects or functions shall be unique before linker stage
- **misra-c2023-2.7** (95 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_rollback.c:585` — There should be no unused parameters in functions [misra-c2012-2.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_rollback.c:610` — There should be no unused parameters in functions [misra-c2012-2.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/bl_partition.c:355` — There should be no unused parameters in functions [misra-c2012-2.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinSlave_Tp.c:240` — There should be no unused parameters in functions [misra-c2012-2.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/lin/src/LinSlave_Tp.c:224` — There should be no unused parameters in functions [misra-c2012-2.7]
- **misra-c2023-10.3** (91 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:23` — Complex expression shall only be assigned to same essential type category [misra
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:125` — Complex expression shall only be assigned to same essential type category [misra
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:135` — Complex expression shall only be assigned to same essential type category [misra
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:24` — Complex expression shall only be assigned to same essential type category [misra
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:192` — Complex expression shall only be assigned to same essential type category [misra

... and 65 more rules