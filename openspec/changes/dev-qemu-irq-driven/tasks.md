# Tasks: QEMU IRQ 驱动验证

> **变更 ID**: dev-qemu-irq-driven  
> **状态**: Proposed  
> **创建日期**: 2026-08-22

## 任务总览

```
dev-qemu-irq-driven
├── Specification
│   ├── proposal.md
│   └── specs/QemuIrqDriven_spec.md
├── Source Code
│   ├── p3d_irq_driven/main_irq_driven.c
│   ├── p3d_irq_driven/irq_timer_stub.c
│   └── p3d_irq_driven/build.sh
└── Verification
    └── 4 个 Scenario 验证 (S7.1 - S7.4)
```

---

## Phase 1: 规范定义 (4h)

- [x] 创建 OpenSpec change 目录
- [x] 编写 `proposal.md`
- [x] 编写 `specs/QemuIrqDriven_spec.md`

---

## Phase 2: 代码实现 (20h)

### APB Timer0 配置与 ISR
- [x] 实现 `p3d_irq_driven/irq_timer_stub.c`
  - [x] 配置 CMSDK APB Timer0 寄存器（LOAD / CTRL）
  - [x] 实现 `TIMER0_IRQHandler`，累加全局计数器并调用 `xTaskNotifyFromISR`
  - [x] 实现嵌套 IRQ 场景：在 TIMER0_IRQHandler 内使能 UART0_IRQ 并验证抢占

### 验证主镜像
- [x] 实现 `p3d_irq_driven/main_irq_driven.c`
  - [x] S7.1: 等待 IRQ_COUNT 累计到 10，打印并断言
  - [x] S7.2: 任务等待 `ulTaskNotifyTake`，验证 ISR 唤醒延迟 ≤ 1 tick
  - [x] S7.3: 检查 `uxTaskGetStackHighWaterMark` > 0
  - [x] S7.4: 验证嵌套 IRQ 执行顺序（高优先级先返回）
  - [x] 全部通过后调用 `Qemu_ReportPass()`
- [x] 实现 `p3d_irq_driven/build.sh`

---

## Phase 3: 验证 (12h)

- [ ] 验证 S7.1: APB Timer0 周期 IRQ 触发计数
- [ ] 验证 S7.2: ISR 到任务唤醒延迟
- [ ] 验证 S7.3: 嵌套中断堆栈完整性
- [ ] 验证 S7.4: NVIC 优先级抢占顺序
- [ ] CI `run_qemu_test.sh` exit code == 0，UART 含 `IRQ_DRIVEN_PASS`

---

## 里程碑

| 里程碑 | 日期 | 交付物 |
|:-------|:-----|:-------|
| 规范完成 | 2026-08-22 | proposal + spec |
| 代码完成 | 2026-09-10 | main + stub + build.sh |
| 验证通过 | 2026-09-12 | 4/4 Scenario PASS |

## 进度跟踪

| 任务 | 状态 | 负责人 | 开始日期 | 完成日期 |
|:-----|:-----|:-------|:---------|:---------|
| 规范定义 | ✅ | Track-B | 2026-08-22 | 2026-08-22 |
| 代码实现 | ✅ | Track-B | 2026-08-23 | 2026-08-23 |
| Scenario 验证 | ⏳ | Track-B | — | — |
