# 变更提案：QEMU WDG 超时复位验证

> **变更 ID**: dev-qemu-wdg-timeout  
> **状态**: Proposed  
> **优先级**: P2  
> **负责人**: Track-B  
> **创建日期**: 2026-08-22  
> **依赖变更**: dev-qemu-assert-infra, dev-qemu-os-schedule  
> **目标版本**: v1.6.0-beta.1  
> **估计工时**: 36h (4.5d)

## 背景

yuleASR 已实现 Wdg 驱动（`src/bsw/mcal/wdg/`）和 WdgM 管理层（`src/bsw/services/wdgm/`），并在 `GAC_T68_SBM_MCU` 项目中经过实际运用。但在 yuleASR QEMU 验证环境中，WDG 从未被真实触发过超时复位。需要验证两条关键路径：(1) WdgM 活性监控正常时 WDG 被持续喂狗不超时；(2) 任务停止喂狗时 WDG 超时触发系统复位，QEMU 可观测到复位行为。

## 目标

1. 验证正常路径：`WdgM_MainFunction` 周期调用 `Wdg_SetTriggerCondition` 喂狗，系统持续运行 1 秒不复位
2. 验证超时路径：刻意停止喂狗，WDG 超时触发 `POWER_RESET`，QEMU 输出复位标记
3. 验证 WdgM 活性监控：被监控任务停止上报 `WdgM_CheckpointReached` 后，WdgM 停止喂狗，触发间接复位
4. 验证 WdgM 模式切换：`WdgM_SetMode(WDGM_GLOBAL_MODE_STOPPED)` 后 WDG 进入 OFF 模式

## 范围

### 包含内容
- `tests/qemu_full_stack/p3d_wdg_timeout/main_wdg_timeout.c` — 验证入口
- `tests/qemu_full_stack/p3d_wdg_timeout/wdg_qemu_stub.c` — QEMU WDG 硬件桩（CMSDK Watchdog）
- `tests/qemu_full_stack/p3d_wdg_timeout/build.sh` — 构建脚本

### 不包含内容
- S32K312 SWT（Software Watchdog Timer）真实寄存器驱动
- WdgM 外部复位检测（`WdgM_GetGlobalStatus` 跨复位持久化）

## 验收标准

- [ ] S10.1: 正常喂狗路径 — 系统运行 1 秒（100 个 10ms 周期），WDG 未超时，`wdg_timeout_count == 0`
- [ ] S10.2: 超时复位路径 — 停止喂狗后，WDG 计数超时，UART 输出 `WDG_TIMEOUT_RESET` 标记
- [ ] S10.3: WdgM 活性监控 — 被监控任务停止 `WdgM_CheckpointReached` 后，WdgM 在 deadline 内停止喂狗，触发超时
- [ ] S10.4: WdgM 模式切换 — `SetMode(STOPPED)` 后 WDG 进入 OFF 模式，`Wdg_SetMode(WDGIF_OFF_MODE)` 被调用
- [ ] CI `run_qemu_test.sh` exit code == 0，UART 含 `WDG_TIMEOUT_PASS`

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| QEMU mps2-an521 CMSDK Watchdog 实现与 S32K312 SWT 接口差异大 | 高 | 中 | WdgM 通过 WdgIf 抽象层调用，提供 QEMU 专用 Wdg stub 适配 CMSDK |
| WDG 超时导致 QEMU 进程退出，无法输出 PASS 标记 | 中 | 高 | 在超时 ISR 中先输出标记再执行复位；或使用 semihosting 在复位前同步输出 |
| WdgM deadline 计算依赖 OS tick，QEMU tick 精度影响测试稳定性 | 低 | 中 | deadline 放宽至 1 秒，避免因 QEMU 调度抖动误判 |
