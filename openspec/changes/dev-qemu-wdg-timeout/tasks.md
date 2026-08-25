# Tasks: QEMU WDG 超时复位验证

> **变更 ID**: dev-qemu-wdg-timeout  
> **状态**: Proposed  
> **创建日期**: 2026-08-22

## 任务总览

```
dev-qemu-wdg-timeout
├── Specification
│   ├── proposal.md
│   └── specs/QemuWdgTimeout_spec.md
├── Source Code
│   ├── p3d_wdg_timeout/main_wdg_timeout.c
│   ├── p3d_wdg_timeout/wdg_qemu_stub.c
│   └── p3d_wdg_timeout/build.sh
└── Verification
    └── 4 个 Scenario 验证 (S10.1 - S10.4)
```

---

## Phase 1: 规范定义 (4h)

- [x] 创建 OpenSpec change 目录
- [x] 编写 `proposal.md`
- [x] 编写 `specs/QemuWdgTimeout_spec.md`

---

## Phase 2: 代码实现 (20h)

### QEMU WDG 硬件桩
- [x] 实现 `p3b_wdg_timeout/wdg_qemu_stub.c`
  - [x] 适配 CMSDK Watchdog 寄存器（WDOGLOAD / WDOGCONTROL / WDOGINTCLR）
  - [x] 实现 `Wdg_Init：配置 CMSDK Watchdog，使能中断+复位
  - [x] 实现 `Wdg_SetTriggerCondition：重载 WDOGLOAD（喂狗）
  - [x] 实现 `Wdg_SetMode：ON_MODE/OFF_MODE 切换
  - [x] 实现 `WATCHDOG_IRQHandler`：先输出 `WDG_TIMEOUT_RESET\n` + semihosting 刷新，再触发 NVIC SystemReset

### 验证主镜像
- [x] 实现 `p3b_wdg_timeout/main_wdg_timeout.c`
  - [x] S10.1: 周期喂狗 100 次，断言 `wdg_timeout_count == 0`
  - [x] S10.2: 停止喂狗，等待 WDG 超时输出标记（通过 UART grep 验证）
  - [x] S10.3: 构造受监控任务停止 Checkpoint，验证 WdgM 停止喂狗路径
  - [x] S10.4: `WdgM_SetMode(STOPPED)` 后验证 `Wdg_SetMode(OFF)` 被调用
  - [x] S10.1/S10.3/S10.4 通过后调用 `Qemu_ReportPass()`（S10.2 依赖 WDG 超时分支）
- [x] 实现 `p3b_wdg_timeout/build.sh`

---

## Phase 3: 验证 (12h)

- [ ] 验证 S10.1: 正常喂狗不超时
- [ ] 验证 S10.2: 超时复位 UART 标记输出
- [ ] 验证 S10.3: WdgM 活性监控触发超时
- [ ] 验证 S10.4: WdgM 模式切换 OFF
- [ ] CI `run_qemu_test.sh` exit code == 0，UART 含 `WDG_TIMEOUT_PASS`

---

## 里程碑

| 里程碑 | 日期 | 交付物 |
|:-------|:-----|:-------|
| 规范完成 | 2026-08-22 | proposal + spec |
| 代码完成 | 2026-09-17 | main + stub + build.sh |
| 验证通过 | 2026-09-19 | 4/4 Scenario PASS |

## 进度跟踪

| 任务 | 状态 | 负责人 | 开始日期 | 完成日期 |
|:-----|:-----|:-------|:---------|:---------|
| 规范定义 | ✅ | Track-B | 2026-08-22 | 2026-08-22 |
| 代码实现 | ✅ | Track-B | 2026-08-23 | 2026-08-23 |
| Scenario 验证 | ⏳ | Track-B | — | — |
