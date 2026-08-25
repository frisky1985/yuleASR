# 变更提案：QEMU OS 调度验证

> **变更 ID**: dev-qemu-os-schedule  
> **状态**: Proposed  
> **优先级**: P1  
> **负责人**: Track-B  
> **创建日期**: 2026-08-22  
> **依赖变更**: dev-qemu-assert-infra  
> **目标版本**: v1.6.0-alpha.1  
> **估计工时**: 32h (4d)

## 背景

`tests/qemu_m33/build.sh` 已验证 FreeRTOS + yuleASR Os.c 在 QEMU mps2-an521 上能启动并输出 `QEMU_M33_OS_PASS`，但使用 `bkpt #0` 作为结束标记，无法被 CI 自动判定。需要用 C1 的 semihosting 断言替换，并增加任务切换、优先级抢占、Alarm 到期等硬断言。

## 目标

1. 基于 `tests/qemu_m33/os_test_main.c` 原型，将 `bkpt #0` 替换为 `Qemu_ReportPass()` / `Qemu_ReportFail()`
2. 验证 FreeRTOS SysTick 在 1000Hz 配置下正确推进 tick counter
3. 验证任务优先级抢占行为（高优先级 Task-A 先于低优先级 Task-B 执行）
4. 验证 `TerminateTask()` 后任务不再被调度
5. 验证 AUTOSAR Alarm 到期回调被正确调用

## 范围

### 包含内容
- `tests/qemu_full_stack/p1a_os_schedule/main_os_schedule.c` — 验证入口
- `tests/qemu_full_stack/p1a_os_schedule/build.sh` — 构建脚本
- 直接链接 `src/bsw/os/src/Os.c`（生产代码）

### 不包含内容
- EcuM 启动序列（C3 负责）
- BSW 服务层真实代码（C4-C10 负责）

## 验收标准

- [ ] `Qemu_ReportPass()` 在所有 Scenario 通过后调用
- [ ] S2.1: 100 次 SysTick 后 `xTaskGetTickCount() >= 100`
- [ ] S2.2: UART 序列 `A:1 A:2 A:3` 先于 `B:1`
- [ ] S2.3: `TerminateTask()` 后无第二个 `B:` 输出
- [ ] S2.4: 500ms alarm 回调被调用 ≥ 3 次
- [ ] CI `run_qemu_test.sh` exit code == 0

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| FreeRTOS SysTick 在 QEMU 上频率不准 | 低 | 中 | 验证 100ms 内 tick count ≥ 90 即可 |
| 任务切换 UART 输出顺序不确定 | 中 | 中 | 用计数器而非纯 UART 顺序判定 |
