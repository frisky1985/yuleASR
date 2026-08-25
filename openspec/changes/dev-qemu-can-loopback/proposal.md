# 变更提案：QEMU CAN 回环验证

> **变更 ID**: dev-qemu-can-loopback  
> **状态**: Proposed  
> **优先级**: P2  
> **负责人**: Track-B  
> **创建日期**: 2026-08-22  
> **依赖变更**: dev-qemu-assert-infra, dev-qemu-os-schedule, dev-qemu-ecum-startup  
> **目标版本**: v1.6.0-alpha.1  
> **估计工时**: 40h (5d)

## 背景

当前 QEMU 验证中 CAN 通路完全使用 BSW 桩，`CanIf_RxIndication` 从未被真实 `Can.c` 调用。需要在 QEMU 上验证 `Can_Write → CanIf_RxIndication → Com_RxIndication → RTE 信号 → ASW 读取` 的完整软件回环路径。

## 目标

1. 在 `Can.c` 中通过 `#ifdef QEMU_CAN_LOOPBACK` 宏实现软件回环（`Can_Write` 后直接调用 `CanIf_RxIndication`）
2. 验证 Com 信号从 CAN 帧到 `Com_ReceiveSignal()` 的完整路径
3. 验证 RTE 端口读取与 Com 信号一致
4. 验证连续多帧无丢失

## 范围

### 包含内容
- `src/bsw/mcal/can/src/Can.c` — 添加 loopback 宏分支
- `tests/qemu_full_stack/p2a_can_loopback/main_can_loopback.c` — 验证入口
- `tests/qemu_full_stack/p2a_can_loopback/Can_Qemu_Lcfg.c` — CAN 配置
- `tests/qemu_full_stack/p2a_can_loopback/build.sh` — 构建脚本

### 不包含内容
- QEMU socketcan/vcan（不依赖 Linux 内核模块）
- FlexCAN 寄存器级模拟（软件回环无需硬件模拟）
- UDS 诊断注入（C6 负责）

## 验收标准

- [ ] S4.1: `Can_Write` 后 `CanIf_RxIndication` 回调计数 == 1
- [ ] S4.2: `Com_ReceiveSignal()` 读出值 == 0x1234U
- [ ] S4.3: `Rte_Read_EngineSpeed_u16()` == 0x1234U
- [ ] S4.4: 连续 5 帧，接收计数 == 5
- [ ] CI `run_qemu_test.sh` exit code == 0，UART 含 `CAN_LOOPBACK_PASS`

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| Can.c 链接依赖过多 | 中 | 高 | 只链接 Can.c + CanIf + Com + PduR，其余用桩 |
| Com 信号路由配置复杂 | 中 | 中 | 使用最小化配置（1 controller / 4 PDU） |
| RTE 端口未连接 | 低 | 中 | 测试入口中显式调用 `Rte_Init` 并注册端口 |
