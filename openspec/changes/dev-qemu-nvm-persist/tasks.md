# Tasks: QEMU NvM 掉电恢复验证

> **变更 ID**: dev-qemu-nvm-persist  
> **状态**: Proposed  
> **创建日期**: 2026-08-22

## 任务总览

```
dev-qemu-nvm-persist
├── Specification
│   ├── proposal.md
│   └── specs/QemuNvmPersist_spec.md
├── Source Code
│   ├── Fls_Hw.c 修改（semihosting 宏）
│   ├── p2b_nvm_persist/main_nvm_write.c
│   ├── p2b_nvm_persist/main_nvm_read.c
│   └── p2b_nvm_persist/build.sh
└── Verification
    └── 4 个 Scenario 验证（两阶段）
```

---

## Phase 1: 规范定义 (3h)

- [x] 创建 OpenSpec change 目录
- [x] 编写 `proposal.md`
- [x] 编写 `specs/QemuNvmPersist_spec.md`

---

## Phase 2: 核心代码修改 (12h)

### Fls_Hw.c semihosting 宏
- [ ] 在 `src/bsw/mcal/fls/src/Fls_Hw.c` 新增：
  - [ ] `Fls_Hw_ExportToHost(const char *path)` — semihosting SYS_OPEN/WRITE/CLOSE
  - [ ] `Fls_Hw_ImportFromHost(const char *path)` — semihosting SYS_OPEN/READ/CLOSE
  - [ ] 宏 `#ifdef QEMU_NVM_PERSIST` 严格隔离

### 验证镜像
- [x] 实现 `p2b_nvm_persist/main_nvm_write.c` — 写入阶段
- [x] 实现 `p2b_nvm_persist/main_nvm_read.c` — 读回阶段
- [x] 实现 `p2b_nvm_persist/build.sh` — 支持 `build.sh write` / `build.sh read`

---

## Phase 3: 验证 (9h)

- [ ] 验证 S5.1: NvMWriteAndFlush — NVM_REQ_OK
- [ ] 验证 S5.2: FlashBinDump — flash.bin 存在 + magic 正确
- [ ] 验证 S5.3: PowerCycleRestore — 读回值 == 0xDEADBEEF
- [ ] 验证 S5.4: CorruptedBlockHandling — NVM_REQ_INTEGRITY_FAILED
- [ ] CI 两阶段 `run_qemu_test.sh` exit code == 0

---

## 里程碑

| 里程碑 | 日期 | 交付物 |
|:-------|:-----|:-------|
| 规范完成 | 2026-08-22 | proposal + spec |
| 代码完成 | 2026-09-06 | Fls_Hw.c 修改 + main_write + main_read |
| 验证通过 | 2026-09-08 | 4/4 Scenario PASS |

## 进度跟踪

| 任务 | 状态 | 负责人 | 开始日期 | 完成日期 |
|:-----|:-----|:-------|:---------|:---------|
| 规范定义 | ✅ | Track-A | 2026-08-22 | 2026-08-22 |
| Fls_Hw.c 修改 | ⏳ | Track-A | 2026-09-04 | — |
| 验证镜像 | ✅ | Track-A | 2026-09-05 | 2026-08-23 |
| Scenario 验证 | ⏳ | Track-A | 2026-09-08 | — |
