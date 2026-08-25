# 变更提案：QEMU IRQ 驱动验证

> **变更 ID**: dev-qemu-irq-driven  
> **状态**: Proposed  
> **优先级**: P2  
> **负责人**: Track-B  
> **创建日期**: 2026-08-22  
> **依赖变更**: dev-qemu-assert-infra, dev-qemu-os-schedule  
> **目标版本**: v1.6.0-beta.1  
> **估计工时**: 36h (4.5d)

## 背景

yuleASR 的 MCAL 中断层（Gpt_Irq、Wdg_Irq、Can_Irq 等）在 QEMU 上从未被中断实际触发过。当前 QEMU 验证仅在 FreeRTOS tick（SysTick）驱动的软件轮询模式下运行，IRQ 向量未经验证。需要在 QEMU mps2-an521 上触发 NVIC 中断，验证 ISR 注册、中断上下文切换、以及 RTOS 任务唤醒路径。

## 目标

1. 通过 QEMU 虚拟定时器（CMSDK APB Timer0）触发周期 IRQ，验证 `Gpt_Isr` 执行路径
2. 验证 IRQ 优先级配置（NVIC_SetPriority / NVIC_EnableIRQ）生效
3. 验证从 ISR 中调用 FreeRTOS `xTaskNotifyFromISR` 正确唤醒等待任务
4. 验证嵌套中断（Nested IRQ）场景下堆栈完整性

## 范围

### 包含内容
- `tests/qemu_full_stack/p3d_irq_driven/main_irq_driven.c` — 验证入口
- `tests/qemu_full_stack/p3d_irq_driven/irq_timer_stub.c` — APB Timer0 配置与 ISR stub
- `tests/qemu_full_stack/p3d_irq_driven/build.sh` — 构建脚本

### 不包含内容
- 真实 S32K312 GPT 寄存器驱动（留待硬件在环阶段）
- DMA 驱动中断（留待后续变更）

## 验收标准

- [ ] S7.1: APB Timer0 周期 IRQ 触发 ≥ 10 次，计数器累计正确（`IRQ_COUNT == 10`）
- [ ] S7.2: ISR 中调用 `xTaskNotifyFromISR` 后，等待任务在 1 个 tick 内被唤醒
- [ ] S7.3: 嵌套中断场景下（Timer0 中嵌套 UART0 IRQ）任务堆栈无溢出（FreeRTOS stack watermark > 0）
- [ ] S7.4: NVIC 优先级配置验证：低优先级 ISR 被高优先级 ISR 抢占，执行顺序正确
- [ ] CI `run_qemu_test.sh` exit code == 0，UART 含 `IRQ_DRIVEN_PASS`

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| QEMU mps2-an521 APB Timer 中断号与 S32K312 不同 | 高 | 中 | 使用 CMSDK 通用 APB Timer，不依赖 S32K312 特定 IRQ 号 |
| FreeRTOS 临界区导致 ISR 被屏蔽 | 中 | 高 | 验证前确认 configMAX_SYSCALL_INTERRUPT_PRIORITY 配置 |
| 嵌套中断时序在 QEMU 与真实硬件行为差异 | 中 | 低 | 仅验证 NVIC 逻辑，不验证时序精度 |
