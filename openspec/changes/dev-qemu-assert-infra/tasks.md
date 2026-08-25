# Tasks: QEMU 断言基础设施

> **变更 ID**: dev-qemu-assert-infra  
> **状态**: Proposed  
> **创建日期**: 2026-08-22

## 任务总览

```
dev-qemu-assert-infra
├── Specification
│   ├── proposal.md
│   ├── specs/QemuAssert_spec.md
│   └── design.md
├── Source Code
│   ├── common/qemu_assert.h
│   ├── common/qemu_assert.c
│   ├── common/unity_uart_output.c
│   └── p0_assert_infra/main_assert_test.c
├── CI Scripts
│   ├── ci/run_qemu_test.sh
│   └── ci/run_all_qemu_tests.sh
└── Verification
    └── QEMU semihosting exit code 验证
```

---

## Phase 1: 规范定义 (4h)

- [x] 创建 OpenSpec change 目录
- [x] 编写 `proposal.md`
- [x] 编写 `specs/QemuAssert_spec.md`（8 章节标准格式）
- [x] 编写 `design.md`

---

## Phase 2: 核心代码实现 (8h)

### common/ 基础设施
- [x] 实现 `common/qemu_assert.h` — API 声明
- [x] 实现 `common/qemu_assert.c` — semihosting SYS_EXIT 实现
- [x] 实现 `common/unity_uart_output.c` — Unity UART 输出移植
- [x] 实现 `common/flash_persist.h` — flash.bin 持久化 API 声明

### CI 脚本
- [x] 实现 `ci/run_qemu_test.sh` — 单测驱动脚本
- [x] 实现 `ci/run_all_qemu_tests.sh` — 批量运行脚本

### 验证镜像
- [x] 实现 `p0_assert_infra/main_assert_test.c` — 4 个 Scenario 测试入口
- [x] 实现 `p0_assert_infra/build.sh` — 构建脚本

---

## Phase 3: 验证 (12h)

- [ ] 验证 S1.1: SemihostingExit_Pass — QEMU exit(0) + UART 标记
- [ ] 验证 S1.2: SemihostingExit_Fail — QEMU exit(1) + UART 标记
- [ ] 验证 S1.3: UnityUartOutput — 3 个 Unity 测试 + 摘要输出
- [ ] 验证 S1.4: TimeoutGuard — 超时返回非零 exit code
- [ ] 验证 `run_qemu_test.sh` 在 CI runner 上的行为

---

## 里程碑

| 里程碑 | 日期 | 交付物 |
|:-------|:-----|:-------|
| 规范完成 | 2026-08-22 | proposal + spec + design |
| 代码完成 | 2026-08-25 | common/ + ci/ + p0_assert_infra/ |
| 验证通过 | 2026-08-26 | 4/4 Scenario PASS |

## 进度跟踪

| 任务 | 状态 | 负责人 | 开始日期 | 完成日期 |
|:-----|:-----|:-------|:---------|:---------|
| 规范定义 | ✅ | Track-A | 2026-08-22 | 2026-08-22 |
| common/ 实现 | ✅ | Track-A | 2026-08-24 | 2026-08-24 |
| p0 验证镜像 | ✅ | Track-A | 2026-08-25 | 2026-08-23 |
| Scenario 验证 | ⏳ | Track-A | 2026-08-26 | — |
