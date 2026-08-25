# 变更提案：QEMU 断言基础设施

> **变更 ID**: dev-qemu-assert-infra  
> **状态**: Proposed  
> **优先级**: P0  
> **负责人**: Track-A  
> **创建日期**: 2026-08-22  
> **依赖变更**: 无  
> **目标版本**: v1.6.0-alpha.1  
> **估计工时**: 24h (3d)

## 背景

当前 yuleASR 的 QEMU 验证（`tests/qemu_m33/`）使用 `bkpt #0` 作为测试结束标记，CI 无法自动判定 PASS/FAIL。所有后续 QEMU 全栈验证变更（C2-C10）都依赖一个可靠的断言基础设施：能够通过 semihosting `SYS_EXIT` 报告退出码，并通过 UART 输出可 grep 的标记字符串。

## 目标

1. 实现 `Qemu_ReportPass()` / `Qemu_ReportFail()` / `Qemu_Assert()` API，基于 ARM semihosting `SYS_EXIT(0x18)` 指令
2. 移植 Unity 测试框架输出到 QEMU CMSDK UART
3. 实现 `ci/run_qemu_test.sh` 驱动脚本，支持 `timeout` + `grep MARKER` 判定
4. 验证 semihosting exit code 能被 QEMU `--semihosting-config enable=on,target=native` 正确捕获

## 范围

### 包含内容
- `tests/qemu_full_stack/common/qemu_assert.h` — 断言 API 声明
- `tests/qemu_full_stack/common/qemu_assert.c` — semihosting SYS_EXIT 实现
- `tests/qemu_full_stack/common/unity_uart_output.c` — Unity 输出重定向
- `tests/qemu_full_stack/ci/run_qemu_test.sh` — 单测驱动脚本
- `tests/qemu_full_stack/p0_assert_infra/` — 验证镜像

### 不包含内容
- QEMU 板级移植（复用 `tests/qemu_m33/` 已有资产）
- FreeRTOS 集成（C2 负责）
- 具体 BSW 模块验证（C3-C10 负责）

## 验收标准

- [ ] `qemu_assert.h` 导出 `Qemu_ReportPass/Fail/Assert` 三个函数
- [ ] `Qemu_ReportPass()` 调用后 QEMU 以 exit code 0 退出
- [ ] `Qemu_ReportFail()` 调用后 QEMU 以 exit code 1 退出
- [ ] UART 输出含 `QEMU_FULL_STACK_PASS` 或 `QEMU_FULL_STACK_FAIL` 标记
- [ ] `run_qemu_test.sh` 在 30s 超时时返回非零 exit code
- [ ] Unity 测试摘要通过 UART 正确输出
- [ ] CI job skeleton 可执行

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| QEMU semihosting SYS_EXIT 行为不符合预期 | 低 | 高 | 先验证 `bkpt #0xAB` + `SYS_EXIT` 在 mps2-an521 上的实际行为 |
| Unity 输出缓冲与 UART 写入不兼容 | 低 | 中 | UnityOutputChar 逐字符转发，无缓冲问题 |
| CI runner QEMU 版本不支持 semihosting | 低 | 高 | CI step 显式 `apt install qemu-system-arm` 并验证版本 |
