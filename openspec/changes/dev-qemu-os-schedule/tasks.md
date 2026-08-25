# Tasks: QEMU OS 调度验证

> **变更 ID**: dev-qemu-os-schedule  
> **状态**: Proposed  
> **创建日期**: 2026-08-22

## 任务总览

```
dev-qemu-os-schedule
├── Specification
│   ├── proposal.md
│   └── specs/QemuOsSchedule_spec.md
├── Source Code
│   ├── p1a_os_schedule/main_os_schedule.c
│   └── p1a_os_schedule/build.sh
└── Verification
    └── 4 个 Scenario 验证
```

---

## Phase 1: 规范定义 (3h)

- [x] 创建 OpenSpec change 目录
- [x] 编写 `proposal.md`
- [x] 编写 `specs/QemuOsSchedule_spec.md`

---

## Phase 2: 代码实现 (16h)

### 验证镜像
- [x] 实现 `p1a_os_schedule/main_os_schedule.c`
  - [x] 任务配置表（Task-A 高优先级 + Task-B 低优先级）
  - [x] Alarm 500ms 周期配置
  - [x] SysTick 计数验证逻辑
  - [x] 任务优先级抢占验证逻辑（UART 输出 + 计数器）
  - [x] TerminateTask 验证逻辑
  - [x] Alarm 到期验证逻辑
  - [x] 全部通过后调用 `Qemu_ReportPass()`
- [x] 实现 `p1a_os_schedule/build.sh`
  - [x] 复用 `tests/qemu_m33/build.sh` 的 CFLAGS/INCLUDES 模板
  - [x] 添加 `common/qemu_assert.c` 和 `common/unity_uart_output.c`
  - [x] 链接生产 `Os.c`
  - [x] 添加 `run` 目标调用 `ci/run_qemu_test.sh`

---

## Phase 3: 验证 (13h)

- [ ] 验证 S2.1: SysTickAdvance — tick count 差值 ≥ 90
- [ ] 验证 S2.2: TaskPriorityPreempt — `A:1 A:2 A:3` 先于 `B:1`
- [ ] 验证 S2.3: TerminateTask — b_run_count == 1
- [ ] 验证 S2.4: AlarmExpiry — alarm_cb_count >= 3
- [ ] CI `run_qemu_test.sh` exit code == 0

---

## 里程碑

| 里程碑 | 日期 | 交付物 |
|:-------|:-----|:-------|
| 规范完成 | 2026-08-22 | proposal + spec |
| 代码完成 | 2026-08-26 | main_os_schedule.c + build.sh |
| 验证通过 | 2026-08-27 | 4/4 Scenario PASS |

## 进度跟踪

| 任务 | 状态 | 负责人 | 开始日期 | 完成日期 |
|:-----|:-----|:-------|:---------|:---------|
| 规范定义 | ✅ | Track-B | 2026-08-22 | 2026-08-22 |
| 代码实现 | ✅ | Track-B | 2026-08-24 | 2026-08-23 |
| Scenario 验证 | ⏳ | Track-B | 2026-08-27 | — |
