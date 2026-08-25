# Tasks: QEMU CAN 回环验证

> **变更 ID**: dev-qemu-can-loopback  
> **状态**: Proposed  
> **创建日期**: 2026-08-22

## 任务总览

```
dev-qemu-can-loopback
├── Specification
│   ├── proposal.md
│   ├── specs/QemuCanLoopback_spec.md
│   └── design.md
├── Source Code
│   ├── Can.c 修改（loopback 宏）
│   ├── p2a_can_loopback/main_can_loopback.c
│   ├── p2a_can_loopback/Can_Qemu_Lcfg.c
│   └── p2a_can_loopback/build.sh
└── Verification
    └── 4 个 Scenario 验证
```

---

## Phase 1: 规范定义 (4h)

- [x] 创建 OpenSpec change 目录
- [x] 编写 `proposal.md`
- [x] 编写 `specs/QemuCanLoopback_spec.md`
- [x] 编写 `design.md`

---

## Phase 2: 核心代码修改 (16h)

### Can.c loopback 宏
- [ ] 在 `src/bsw/mcal/can/src/Can.c` 的 `Can_Write` 函数末尾添加：
  ```c
  #ifdef QEMU_CAN_LOOPBACK
      CanIf_RxIndication(&mailbox, &can_pdu_info);
  #endif
  ```

### 验证镜像
- [ ] 实现 `p2a_can_loopback/Can_Qemu_Lcfg.c` — 最小化配置（1 controller / 4 PDU）
- [x] 实现 `p2a_can_loopback/main_can_loopback.c` — 4 个 Scenario 测试入口
- [x] 实现 `p2a_can_loopback/build.sh`

---

## Phase 3: 验证 (20h)

- [ ] 验证 S4.1: CanWriteLoopback — 回调计数 == 1
- [ ] 验证 S4.2: ComSignalReceive — 信号值 == 0x1234
- [ ] 验证 S4.3: RtePortRead — RTE 读出值 == 0x1234
- [ ] 验证 S4.4: MultiFrameSequence — 接收计数 == 5
- [ ] CI `run_qemu_test.sh` exit code == 0

---

## 里程碑

| 里程碑 | 日期 | 交付物 |
|:-------|:-----|:-------|
| 规范完成 | 2026-08-22 | proposal + spec + design |
| 代码完成 | 2026-09-01 | Can.c 修改 + main + lcfg + build.sh |
| 验证通过 | 2026-09-03 | 4/4 Scenario PASS |

## 进度跟踪

| 任务 | 状态 | 负责人 | 开始日期 | 完成日期 |
|:-----|:-----|:-------|:---------|:---------|
| 规范定义 | ✅ | Track-B | 2026-08-22 | 2026-08-22 |
| Can.c 修改 | ⏳ | Track-B | 2026-08-28 | — |
| 验证镜像 | ✅ | Track-B | 2026-08-29 | 2026-08-23 |
| Scenario 验证 | ⏳ | Track-B | 2026-09-03 | — |
