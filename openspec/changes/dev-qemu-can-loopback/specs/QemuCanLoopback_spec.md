# QemuCanLoopback (QEMU CAN 回环验证) Module Specification

> **Module:** QemuCanLoopback (QEMU CAN 回环验证)  
> **Layer:** Test Infrastructure  
> **Standard:** AUTOSAR Classic Platform 4.4.0  
> **Platform:** QEMU mps2-an521 (Cortex-M33)  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

在 QEMU 仿真器上验证 CAN 通信栈的软件回环路径：`Can_Write → CanIf_RxIndication → PduR → Com_RxIndication → RTE 信号 → ASW 读取`。通过宏控制实现软件回环，不依赖 QEMU socketcan/vcan 内核模块。

### Key Responsibilities
- 验证 Can MCAL `Can_Write` 软件回环注入路径
- 验证 CanIf → PduR → Com 信号路由
- 验证 RTE 端口读取与 Com 信号一致
- 验证连续多帧无丢失

---

## 2. API List

### 2.1 Can MCAL APIs Under Test

| API | Description |
|-----|-------------|
| `Can_Init(Controller, Config)` | CAN 控制器初始化 |
| `Can_Write(Hth, PduInfo)` | 发送 CAN 帧（回环注入 RxIndication） |

### 2.2 CanIf APIs Under Test

| API | Called By | Description |
|-----|-----------|-------------|
| `CanIf_Init(Config)` | 测试入口 | CanIf 初始化 |
| `CanIf_RxIndication(Mailbox, PduInfo)` | Can_Write 回环 | 接收回调 |

### 2.3 Com APIs Under Test

| API | Description |
|-----|-------------|
| `Com_Init(Config)` | Com 初始化 |
| `Com_RxIndication(PduId)` | PDU 接收指示 |
| `Com_ReceiveSignal(SignalId, Data)` | 信号读取 |

---

## 3. Data Types

### 3.1 Can QEMU Loopback Config

```c
/* Can_Qemu_Lcfg.c — QEMU 最小化配置 */
#define CAN_QEMU_CONTROLLER_COUNT  1
#define CAN_QEMU_HTH_COUNT         1
#define CAN_QEMU_PDU_COUNT         4
#define CAN_QEMU_TX_CAN_ID         0x100U
#define CAN_QEMU_RX_CAN_ID         0x101U
#define CAN_QEMU_SIGNAL_TEST       0x00U  /* Signal_EngineSpeed */
```

---

## 4. Error Handling

| Error Code | Value | Description |
|------------|-------|-------------|
| `QEMU_FAIL_CAN_LOOPBACK` | 20 | CanIf_RxIndication 回调计数 != 1 |
| `QEMU_FAIL_COM_SIGNAL` | 21 | Com_ReceiveSignal 读出值不匹配 |
| `QEMU_FAIL_RTE_PORT` | 22 | RTE 端口读取值不匹配 |
| `QEMU_FAIL_MULTI_FRAME` | 23 | 多帧接收计数 != 5 |

---

## 5. Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `QEMU_CAN_LOOPBACK` | STD_ON | 启用 Can.c 软件回环宏 |
| `CAN_QEMU_TX_CAN_ID` | 0x100 | 发送 CAN ID |
| `CAN_QEMU_RX_CAN_ID` | 0x101 | 接收 CAN ID |
| `CAN_QEMU_TEST_SIGNAL_VALUE` | 0x1234 | 测试信号值 |

---

## 6. Scenarios

### Scenario 1: CanWriteLoopback
**Description:** Can_Write 后 CanIf_RxIndication 被调用
**Flow:**
1. 初始化 Can + CanIf
2. 调用 `Can_Write(Hth0, &pdu)` 发送一帧
3. 回环路径触发 `CanIf_RxIndication`
4. 断言回调计数 == 1
**Expected Result:** `Qemu_Assert(rx_callback_count == 1, "CAN loopback")`

### Scenario 2: ComSignalReceive
**Description:** Com 信号经 RxIndication 更新后正确读出
**Flow:**
1. 初始化 Can + CanIf + Com
2. `Can_Write` 发送含 Signal_EngineSpeed = 0x1234 的帧
3. 回环 → CanIf → PduR → `Com_RxIndication`
4. 调用 `Com_ReceiveSignal(Signal_EngineSpeed, &data)`
5. 断言 data == 0x1234
**Expected Result:** `Qemu_Assert(signal_value == 0x1234U, "Com signal")`

### Scenario 3: RtePortRead
**Description:** ASW 通过 RTE 读出与 Com 一致的值
**Flow:**
1. 完成 Scenario 2 的回环
2. 调用 `Rte_Read_EngineSpeed_u16(&value)`
3. 断言 value == 0x1234
**Expected Result:** `Qemu_Assert(rte_value == 0x1234U, "RTE port")`

### Scenario 4: MultiFrameSequence
**Description:** 连续 5 帧不同 ID，无丢失
**Flow:**
1. 循环调用 `Can_Write` 发送 5 帧（ID 递增）
2. 每帧回环触发 RxIndication
3. 断言接收计数 == 5
**Expected Result:** `Qemu_Assert(rx_count == 5, "Multi-frame")`

---

## 7. Dependencies

### Upper Layer Modules
- 无

### Lower Layer Modules
- **Can.c** (`src/bsw/mcal/can/src/Can.c`): 生产代码，修改点（loopback 宏）
- **CanIf.c** (`src/bsw/ecual/canif/src/CanIf.c`): 生产代码
- **Com.c** (`src/bsw/services/com/src/Com.c`): 生产代码
- **PduR.c** (`src/bsw/services/pdur/src/PduR.c`): 生产代码
- **Rte.c** (`src/middleware/rte/src/Rte.c`): 生产代码
- **qemu_assert**: 断言报告

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-08-22 | YuleTech | Initial CAN loopback verification specification |
