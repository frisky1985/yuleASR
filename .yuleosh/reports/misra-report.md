# MISRA Compliance Report

**Generated**: 2026-08-07T12:39:00.980777
**Tool**: Cppcheck 2.17.1 from cppcheck-wheel 1.5.1
**Ruleset**: 2023

## Summary

- **Total Violations**: 36219
- **Unique Rules**: 87
- **Affected Files**: 758
- **Density**: 119.41 violations/KLOC

## By Severity

| Severity | Count |
|---------|------:|
| error | 21 |
| warning | 28 |
| style | 36064 |
| portability | 4 |

## By Rule Type

| Type | Count |
|------|------:|
| required | 14498 |
| advisory | 20530 |

## By Category

| Category | Count |
|----------|------:|
| 未分类 (Uncategorized) | 10840 |
| 控制流 (Control Flow) | 7800 |
| 函数行为 | 4339 |
| 声明 (Declarations) | 3477 |
| 基本类型 (Essential Types) | 2268 |
| 表达式 (Expressions) | 1502 |
| 预处理器 (Preprocessing) | 1233 |
| unknown | 1191 |
| 指针 (Pointer) | 1179 |
| 函数 (Functions) | 646 |
| 标识符 (Identifiers) | 421 |
| 指针类型转换 (Pointer Type Conversions) | 315 |
| 标准库 (Standard Library) | 312 |
| 副作用 (Side Effects) | 284 |
| 字面量 (Literals) | 142 |
| 指针与数组 (Pointers & Arrays) | 114 |
| 覆盖存储 (Overlapping Storage) | 78 |
| 初始化 (Initialization) | 64 |
| 字符集 (Character Sets) | 8 |
| 资源 (Resources) | 4 |
| 注释 (Comments) | 2 |

## Violations by Rule

- **misra-c2023-2.5** (10225 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:24` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:18` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:19` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:24` — A project should not contain unused macro declarations [misra-c2012-2.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/cppcheck-config.h:25` — A project should not contain unused macro declarations [misra-c2012-2.5]
- **misra-c2023-15.5** (6815 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:61` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:93` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:95` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:97` — A function should have a single point of exit [misra-c2012-15.5]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:99` — A function should have a single point of exit [misra-c2012-15.5]
- **misra-c2023-17.7** (4339 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:192` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:258` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:284` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:439` — Return value of non-void function shall be used [misra-c2012-17.7]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:445` — Return value of non-void function shall be used [misra-c2012-17.7]
- **misra-c2023-8.7** (2278 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:171` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:202` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:244` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:296` — Functions/objects should not have external linkage if referenced in one TU [misr
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:362` — Functions/objects should not have external linkage if referenced in one TU [misr
- **misra-c2023-10.4** (1988 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:252` — Both operands shall have same essential type category [misra-c2012-10.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:283` — Both operands shall have same essential type category [misra-c2012-10.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:681` — Both operands shall have same essential type category [misra-c2012-10.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:226` — Both operands shall have same essential type category [misra-c2012-10.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:303` — Both operands shall have same essential type category [misra-c2012-10.4]
- **misra-c2023-12.1** (1389 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:156` — Operator precedence within expressions should be made explicit [misra-c2012-12.1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:248` — Operator precedence within expressions should be made explicit [misra-c2012-12.1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:283` — Operator precedence within expressions should be made explicit [misra-c2012-12.1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:391` — Operator precedence within expressions should be made explicit [misra-c2012-12.1
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:471` — Operator precedence within expressions should be made explicit [misra-c2012-12.1
- **unknown** (1191 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:458` — Variable 'state->counter' is reassigned a value before the old one has been used
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:444` — state->counter is assigned
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:458` — state->counter is overwritten
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:459` — Variable '*status' is reassigned a value before the old one has been used. [redu
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:453` — *status is assigned
- **misra-c2023-11.9** (1179 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:77` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:248` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:366` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:391` — The macro NULL shall be the only permitted form of integer null pointer constant
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:396` — The macro NULL shall be the only permitted form of integer null pointer constant
- **misra-c2023-8.4** (684 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/platform/s32k312/src/Platform_RamSafety.c:37` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/cross/hello.c:21` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/cross/hello.c:19` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Safety_1.0.0.c:111` — A compatible declaration shall be visible when an object or function is defined 
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/cdd/src/Cdd_Safety_1.0.0.c:130` — A compatible declaration shall be visible when an object or function is defined 
- **misra-c2023-20.1** (674 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:109` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:133` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:156` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c:775` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/diagnostic_manager/include/Swc_DiagnosticManager.h:152` — misra violation 2001 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-17.3** (576 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:178` — A function shall not be declared implicitly [misra-c2012-17.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:158` — A function shall not be declared implicitly [misra-c2012-17.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:479` — A function shall not be declared implicitly [misra-c2012-17.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:562` — A function shall not be declared implicitly [misra-c2012-17.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/tests/test_bootloader.c:138` — A function shall not be declared implicitly [misra-c2012-17.3]
- **misra-c2023-14.4** (547 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:452` — Controlling expression of if shall have essentially Boolean type [misra-c2012-14
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:454` — Controlling expression of if shall have essentially Boolean type [misra-c2012-14
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/watchdog_manager/src/Swc_WatchdogManager.c:187` — Controlling expression of if shall have essentially Boolean type [misra-c2012-14
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/watchdog_manager/src/Swc_WatchdogManager.c:316` — Controlling expression of if shall have essentially Boolean type [misra-c2012-14
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/asw/watchdog_manager/src/Swc_WatchdogManager.c:467` — Controlling expression of if shall have essentially Boolean type [misra-c2012-14
- **misra-c2023-2.3** (328 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:41` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:68` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:122` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:119` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.h:123` — The character sequence /* shall not be used within a comment [misra-c2012-2.3]
- **misra-c2023-8.6** (308 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bootloader/tests/test_bootloader.c:561` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/tests/test_e2e_full.c:780` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/cross/hello.c:26` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_verify.c:103` — An identifier with external linkage shall have exactly one external definition [
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c:408` — An identifier with external linkage shall have exactly one external definition [
- **misra-c2023-15.6** (297 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:405` — Body of iteration/selection statement shall be compound [misra-c2012-15.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:485` — Body of iteration/selection statement shall be compound [misra-c2012-15.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:583` — Body of iteration/selection statement shall be compound [misra-c2012-15.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:670` — Body of iteration/selection statement shall be compound [misra-c2012-15.6]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:671` — Body of iteration/selection statement shall be compound [misra-c2012-15.6]
- **misra-c2023-20.5** (295 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:26` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:27` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:31` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:32` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/det/include/Det_MemMap.h:36` — misra violation 2005 with no text in the supplied rule-texts-file [misra-c2012-2
- **misra-c2023-13.3** (281 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c:257` — Increment/decrement expression should have no other side effects [misra-c2012-13
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:255` — Increment/decrement expression should have no other side effects [misra-c2012-13
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:261` — Increment/decrement expression should have no other side effects [misra-c2012-13
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:291` — Increment/decrement expression should have no other side effects [misra-c2012-13
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:829` — Increment/decrement expression should have no other side effects [misra-c2012-13
- **misra-c2023-5.9** (200 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:23` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:22` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_dds_integration.c:24` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_state_machine.c:23` — Identifiers that define objects or functions shall be unique before linker stage
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/e2e/e2e_protection.c:40` — Identifiers that define objects or functions shall be unique before linker stage
- **misra-c2023-2.4** (179 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:41` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/common/types/eth_types.h:68` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/rtps/rtps_message.h:105` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/rtps/rtps_message.h:116` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/dds/rtps/rtps_message.h:126` — A section of code shall not be commented out using slash-star [misra-c2012-2.4]
- **misra-c2023-21.3** (162 violations)
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:241` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:254` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:287` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:294` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]
  - `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c:385` — Memory allocation functions of stdlib.h shall not be used [misra-c2012-21.3]

... and 67 more rules