# ECUAL Production Defects (LinIf / CanIf / CanTp 缺陷修复与测试实化) Module Specification

> **Module:** EcualProductionDefects (LinIf / CanIf 缺陷修复 + CanTp/LinIf/CanIf 测试实化)
> **Layer:** ECUAL
> **Standard:** AUTOSAR Classic Platform R22-11 (SWS LinIf / SWS CanIf / SWS CanTp)
> **Platform:** Host (桩驱动) / S32K312
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

本变更（P0-4）修复 4 处 ECUAL 源码缺陷，并将车窗控制器 ECU 关键路径上 3 个空心测试套件（CanTp / LinIf / CanIf）实化为编译真实源码、断言正确的接线测试。其中 D5（LinIf 初始化后状态未激活）为 SOP 阻断级缺陷——合法初始化后整个 LIN 接口对所有 API 调用永久拒绝。

### Key Responsibilities
- D5: `LinIf_Init` 将模块状态翻转到 `LINIF_INIT`，初始化后的 API 可用
- D6: `LinIf_GetVersionInfo` 空指针路径按 AUTOSAR 约定上报 DET
- D7: `LinIf_Config` 链接时配置对象外部可见（LCFG 契约）
- D8: `CanIf.h` 自洽包含并声明驱动侧入口回调原型
- 测试实化：cantp_test（14）、linif_test（13）、canif_test（17）接线 CMake 并全量通过

---

## 2. API List

### 2.1 Modified APIs

| API | Defect | Change |
|-----|--------|--------|
| `void LinIf_Init(const LinIf_ConfigType*)` | D5 | 末尾增加 `LinIf_State.state = LINIF_INIT`（原仅写 configPtr） |
| `void LinIf_GetVersionInfo(Std_VersionInfoType*)` | D6 | NULL 路径增加 `Det_ReportError(LINIF_MODULE_ID, 0U, 0x08U, LINIF_E_PARAM_POINTER)`（受 `LINIF_DEV_ERROR_DETECT` 守卫） |

### 2.2 Configuration Objects

| Object | Defect | Change |
|--------|--------|--------|
| `LinIf_Config` (LinIf_Lcfg.c) | D7 | 去除 `static`，`LinIf.h` 增加 `extern const LinIf_ConfigType LinIf_Config;` |

### 2.3 Header Contracts

| Header | Defect | Change |
|--------|--------|--------|
| `CanIf.h` | D8 | 增加 `#include "Can.h"`；声明 `CanIf_TxConfirmation`、`CanIf_ControllerBusOff`、`CanIf_RxIndication` 原型 |

---

## 3. Data Types

无新增数据类型。`LinIf_StateType`（内部静态状态机）、`Can_HwType`（引自 Can.h）等现有类型布局不变。

---

## 4. Error Handling

| API | Condition | Error Code | SID |
|-----|-----------|------------|-----|
| `LinIf_GetVersionInfo` | `versioninfo == NULL` | `LINIF_E_PARAM_POINTER` (0x10) | 0x08 |

LinIf 文件内 SID 约定（本次测试实化固化）：INIT=0x00, DEINIT=0x01, TRANSMIT=0x02, RX_INDICATION=0x03, MAINFUNCTION=0x04, SCHEDULE=0x05；错误码 PARAM_POINTER=0x10, UNINIT=0x20。

---

## 5. Configuration Parameters

无新增配置项。`LINIF_DEV_ERROR_DETECT` 现有语义不变（D6 修复受其守卫）。

---

## 6. Scenarios

### Scenario S5: LinIfActivatedAfterInit
**Description:** `LinIf_Init` 后模块进入 `LINIF_INIT`，受 E_UNINIT 守卫的 API 可用
**Flow:**
1. `LinIf_Init(&LinIf_Config)`
2. `LinIf_Transmit(0U, &pduInfo)`
**Expected Result:** 返回 E_OK；修复前返回 E_NOT_OK 并上报 E_UNINIT（验收 S5）

### Scenario S6: LinIfVersionInfoParamCheck
**Description:** `LinIf_GetVersionInfo` 空指针上报 DET
**Flow:**
1. `LinIf_GetVersionInfo(NULL_PTR)`
**Expected Result:** `Det_ReportError` 收到 `(LINIF_MODULE_ID, 0U, 0x08U, LINIF_E_PARAM_POINTER)`（验收 S6）

### Scenario S7: CanIfHeaderSelfContained
**Description:** CanIf.h 可被任意 TU 单独包含，驱动侧回调原型可见
**Flow:**
1. TU 仅 `#include "CanIf.h"`
2. 调用 `CanIf_RxIndication(&mailbox, &pduInfo)`、`CanIf_TxConfirmation(0U)`、`CanIf_ControllerBusOff(0U)`
**Expected Result:** 编译通过并链接（验收 S7）

### Scenario S8: CanIfRxIndicationDispatch
**Description:** 接收指示按 Mailbox 的 Hoh + CanId 匹配 RxPdu 配置并通知 PduR
**Flow:**
1. `CanIf_Init(&testConfig)`（1 控制器 + 4 RxPdu，Hrh 0 / CanId 0x150 等）
2. 构造 `Can_HwType mailbox = { .CanId = 0x150U, .Hoh = 0U, .ControllerId = 0U }`
3. `CanIf_RxIndication(&mailbox, &pduInfo)`
**Expected Result:** `PduR_RxIndication` 被调用 1 次且 PDU id 匹配 0x150 对应条目的 PduId

### Scenario S9: CanTpNbsTimeoutReset
**Description:** N_Bs 超时（75 个 MainFunction 周期未收 FC）复位发送通道
**Flow:**
1. `CanTp_Init(&CanTp_Config)`；`CanTp_Transmit(len=10)` → 首帧发出，通道进入 TX_WAIT_FC
2. 调用 75 次 `CanTp_MainFunction()`
3. `CanTp_CancelTransmit(...)`
**Expected Result:** CancelTransmit 返回 E_NOT_OK，证明通道已被超时复位（可重新发起传输）

---

## 7. Dependencies

- `Can.h`（`Can_HwType` / `Can_PduType` / `Can_ReturnType` 定义源，D8 后由 CanIf.h 传递包含）
- `PduR.h`（上层回调目标）
- `Det.h`（D6 DET 上报）
- 接线测试框架：Unity + 手写 runner（RUN_TEST）；桩 `CanIf_Transmit` / `Can_Write` / `Can_SetControllerMode`
- CI：`ctest` 全量回归（104/104）

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-09-25 | YuleTech | Initial spec: D5-D8 ECUAL 缺陷修复与 CanTp/LinIf/CanIf 测试实化 |
