# 变更提案：QEMU UDS 诊断注入验证

> **变更 ID**: dev-qemu-uds-inject  
> **状态**: Proposed  
> **优先级**: P2  
> **负责人**: Track-B  
> **创建日期**: 2026-08-22  
> **依赖变更**: dev-qemu-assert-infra, dev-qemu-can-loopback  
> **目标版本**: v1.6.0-beta.1  
> **估计工时**: 40h (5d)

## 背景

DCM 的 UDS 状态机（IDLE → RX_IN_PROGRESS → PROCESSING → TX_IN_PROGRESS）在 QEMU 上从未运行。需要通过 CAN loopback 注入 UDS 请求帧，验证 Dcm 生成正确响应。

## 目标

1. 通过 `CanIf_RxIndication` 注入 UDS 请求帧（0x22/0x11/0x2E 等 SID）
2. 验证 Dcm `Dcm_MainFunction` 轮询处理后的响应帧内容
3. 验证 P2 超时行为
4. 验证 CanTp 多帧重组

## 范围

### 包含内容
- `tests/qemu_full_stack/p2c_uds_inject/main_uds_inject.c` — 验证入口
- `tests/qemu_full_stack/p2c_uds_inject/uds_response_capture.c` — 响应捕获
- `tests/qemu_full_stack/p2c_uds_inject/build.sh` — 构建脚本

### 不包含内容
- QEMU socketcan（使用 CAN loopback 注入）
- 真实 ECU 物理寻址

## 验收标准

- [ ] S6.1: 注入 `22 F1 90`，响应 `62 F1 90 <VIN[17]>`
- [ ] S6.2: 注入 `11 03`，触发 `Mcu_PerformReset`
- [ ] S6.3: 注入 SID `0xFF`，响应 `7F FF 11`
- [ ] S6.4: CanTp 多帧重组 + 多帧响应
- [ ] CI `run_qemu_test.sh` exit code == 0，UART 含 `UDS_INJECT_PASS`

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| Dcm 需要完整 BSW 配置才能初始化 | 高 | 高 | 提供最小化 Dcm_Cfg.h |
| CanTp 多帧时序复杂 | 中 | 中 | 先验证单帧，再扩展多帧 |
