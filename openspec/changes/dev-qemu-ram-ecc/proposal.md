# 变更提案：QEMU RAM ECC 故障注入验证

> **变更 ID**: dev-qemu-ram-ecc  
> **状态**: Proposed  
> **优先级**: P2  
> **负责人**: Track-B  
> **创建日期**: 2026-08-22  
> **依赖变更**: dev-qemu-assert-infra, dev-qemu-os-schedule  
> **目标版本**: v1.6.0-beta.1  
> **估计工时**: 40h (5d)

## 背景

yuleASR 已实现 RAM ECC 检测模块（`src/safety/ram/`、`src/safety/saferam/`、`src/bsw/cdd/Cdd_RamEcc_1.0.0.c`），以及 S32K312 硬件 ECC 支持（MSCM/FCCU 寄存器操作）。但该模块从未在 QEMU 上运行过端到端验证：ECC 故障注入 → SafeRAM 检测 → Dem 故障上报 → 安全状态迁移。需要在 QEMU mps2-an521 上通过软件位翻转模拟 ECC 故障，验证完整的故障检测与处理链路。

## 目标

1. 通过软件位翻转模拟 1-bit 纠正性 ECC 错误（SEC），验证 RamSafety 检测并纠正后继续运行
2. 通过软件位翻转模拟 2-bit 不可纠正 ECC 错误（DED），验证 RamSafety 触发安全状态迁移
3. 验证 Dem 故障事件（DTC_RAM_ECC_CORRECTED / DTC_RAM_ECC_UNCORRECTED）正确上报
4. 验证 SafeRAM 写保护区域在故障后不被修改（SafeRAM 分区完整性）

## 范围

### 包含内容
- `tests/qemu_full_stack/p3a_ram_ecc/main_ram_ecc.c` — 验证入口
- `tests/qemu_full_stack/p3a_ram_ecc/ram_ecc_fault_inject.c` — 故障注入工具
- `tests/qemu_full_stack/p3a_ram_ecc/build.sh` — 构建脚本

### 不包含内容
- S32K312 MSCM 硬件寄存器 ECC（使用软件位翻转模拟，不依赖 QEMU 硬件 ECC 支持）
- Flash ECC（留待后续变更）

## 验收标准

- [ ] S9.1: 注入 1-bit 翻转（SEC 错误）— `RamSafety_MainFunction` 检测并纠正，模块状态保持 `RAMSAFETY_OK`，Dem 上报 `DTC_RAM_ECC_CORRECTED`
- [ ] S9.2: 注入 2-bit 翻转（DED 错误）— `RamSafety_EnterSafeState` 被调用，模块状态迁移至 `RAMSAFETY_FAILED`
- [ ] S9.3: SafeRAM 写保护分区在故障注入后数据完整（March C- 校验通过）
- [ ] S9.4: 连续 5 次 SEC 错误后触发降级策略（错误计数阈值 = 5）
- [ ] CI `run_qemu_test.sh` exit code == 0，UART 含 `RAM_ECC_PASS`

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| QEMU 无硬件 ECC 支持，无法真实触发 MSCM 中断 | 高 | 中 | 使用软件回调钩子（fault injection hook）模拟 ECC 中断，不依赖 MSCM 寄存器 |
| RamSafety 安全状态迁移导致验证进程无法退出 | 中 | 高 | `EnterSafeState` 重写为 QEMU 测试桩，调用 `Qemu_ReportPass` 后正常退出 |
| March C- 算法执行时间超过 QEMU watchdog 超时 | 低 | 中 | 限制检测区域大小（256 字节），确保执行时间 < 10ms |
