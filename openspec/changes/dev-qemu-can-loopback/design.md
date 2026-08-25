# QEMU CAN 回环验证 — 设计文档

## Architecture Overview

```
┌─────────────────────────────────────────────────┐
│  Test Image (main_can_loopback.c)                │
│  ├── Can_Write(Hth0, &pdu)                       │
│  └── Qemu_Assert(rx_count == 5, ...)             │
├─────────────────────────────────────────────────┤
│  Can.c (生产代码, #ifdef QEMU_CAN_LOOPBACK)      │
│  └── Can_Write 末尾 → CanIf_RxIndication()      │
├─────────────────────────────────────────────────┤
│  CanIf.c (生产代码)                              │
│  └── CanIf_RxIndication → PduR_CanIfRxIndication│
├─────────────────────────────────────────────────┤
│  PduR.c (生产代码)                               │
│  └── PduR_CanIfRxIndication → Com_RxIndication  │
├─────────────────────────────────────────────────┤
│  Com.c (生产代码)                                │
│  └── Com_RxIndication → 信号解析 → SignalData  │
├─────────────────────────────────────────────────┤
│  Rte.c (生产代码)                                │
│  └── Rte_Read_EngineSpeed_u16() → SignalData    │
└─────────────────────────────────────────────────┘
```

## Component Design

### Can.c Loopback Hook

在 `Can_Write` 函数末尾（TX 成功后）插入回环调用：

```c
Std_ReturnType Can_Write(uint8 Hth, const Can_PduType *PduInfo)
{
    /* ... 原有 TX 逻辑 ... */

#ifdef QEMU_CAN_LOOPBACK
    /* 软件回环：直接注入 RX 路径 */
    Can_HwHandleType hwHandle = CanConfiguredHTHs[Hth].HwHandle;
    Can_IdType canId = PduInfo->id;
    uint8 dlc = PduInfo->length;
    const uint8 *sdu = PduInfo->sdu;

    CanIf_RxIndication(
        &(Can_HwHandleType){.ControllerId = 0U, .HwHandle = hwHandle},
        &(Can_PduType){.id = canId, .length = dlc, .sdu = (uint8*)sdu}
    );
#endif

    return E_OK;
}
```

### Can_Qemu_Lcfg.c 最小化配置

```c
/* 1 CAN controller, 1 HTH, 4 PDU */
static const Can_ConfigType Can_QemuConfig = {
    .ControllerCount = 1,
    .HthCount = 1,
    .HthConfig = &Can_QemuHthConfig[0],
    .ControllerConfig = &Can_QemuControllerConfig[0],
};
```

## Testing Strategy

### CI 判定

```bash
cd tests/qemu_full_stack/p2a_can_loopback && ./build.sh
cd ../ci && ./run_qemu_test.sh ../p2a_can_loopback/qemu_p2a.elf \
  CAN_LOOPBACK_PASS p2a_can.log
```

## Configuration Strategy

| 宏 | 编译命令 | 用途 |
|----|---------|------|
| `QEMU_CAN_LOOPBACK` | `-DQEMU_CAN_LOOPBACK` | Can.c 回环分支 |
| `CAN_QEMU_MIN_CONFIG` | `-DCAN_QEMU_MIN_CONFIG` | 最小化配置表 |
