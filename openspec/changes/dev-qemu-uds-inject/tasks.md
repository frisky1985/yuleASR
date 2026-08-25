# Tasks: QEMU UDS 诊断注入验证

> **变更 ID**: dev-qemu-uds-inject  
> **状态**: Proposed  
> **创建日期**: 2026-08-22

## 任务总览

```
dev-qemu-uds-inject
├── Specification
│   ├── proposal.md
│   └── specs/QemuUdsInject_spec.md
├── Source Code
│   ├── p2c_uds_inject/main_uds_inject.c
│   ├── p2c_uds_inject/uds_response_capture.c
│   └── p2c_uds_inject/build.sh
└── Verification
    └── 4 个 Scenario 验证
```

---

## Phase 1: 规范定义 (4h)

- [x] 创建 OpenSpec change 目录
- [x] 编写 `proposal.md`
- [x] 编写 `specs/QemuUdsInject_spec.md`

---

## Phase 2: 代码实现 (20h)

### 响应捕获
- [x] 实现 `p2c_uds_inject/uds_response_capture.c`
  - [x] 重写 `Can_Write` stub，将响应帧写入全局 buffer
  - [x] 提供 `UdsCapture_GetResponse()` API

### 验证镜像
- [x] 实现 `p2c_uds_inject/main_uds_inject.c`
  - [x] S6.1: ReadDataByIdentifier 测试
  - [x] S6.2: EcuReset 测试
  - [x] S6.3: NegativeResponse 测试
  - [x] S6.4: MultiFrameIsoTp 测试
  - [x] 全部通过后调用 `Qemu_ReportPass()`
- [x] 实现 `p2c_uds_inject/build.sh`

---

## Phase 3: 验证 (16h)

- [ ] 验证 S6.1: ReadDataByIdentifier_F190
- [ ] 验证 S6.2: EcuReset_SoftReset
- [ ] 验证 S6.3: NegativeResponse
- [ ] 验证 S6.4: MultiFrameIsoTp
- [ ] CI `run_qemu_test.sh` exit code == 0

---

## 里程碑

| 里程碑 | 日期 | 交付物 |
|:-------|:-----|:-------|
| 规范完成 | 2026-08-22 | proposal + spec |
| 代码完成 | 2026-09-08 | main + capture + build.sh |
| 验证通过 | 2026-09-10 | 4/4 Scenario PASS |

## 进度跟踪

| 任务 | 状态 | 负责人 | 开始日期 | 完成日期 |
|:-----|:-----|:-------|:---------|:---------|
| 规范定义 | ✅ | Track-B | 2026-08-22 | 2026-08-22 |
| 代码实现 | ✅ | Track-B | 2026-09-04 | 2026-08-23 |
| Scenario 验证 | ⏳ | Track-B | 2026-09-10 | — |
