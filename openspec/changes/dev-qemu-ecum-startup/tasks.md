# Tasks: QEMU EcuM 启动序列验证

> **变更 ID**: dev-qemu-ecum-startup  
> **状态**: Proposed  
> **创建日期**: 2026-08-22

## 任务总览

```
dev-qemu-ecum-startup
├── Specification
│   ├── proposal.md
│   └── specs/QemuEcuMStartup_spec.md
├── Source Code
│   ├── p1b_ecum_startup/main_ecum_startup.c
│   ├── p1b_ecum_startup/ecum_test_stubs.c
│   └── p1b_ecum_startup/build.sh
└── Verification
    └── 4 个 Scenario 验证
```

---

## Phase 1: 规范定义 (3h)

- [x] 创建 OpenSpec change 目录
- [x] 编写 `proposal.md`
- [x] 编写 `specs/QemuEcuMStartup_spec.md`

---

## Phase 2: 代码实现 (16h)

### 桩实现
- [x] 实现 `p1b_ecum_startup/ecum_test_stubs.c`
  - [x] Mcu_Init/DeInit stub（输出 `MCAL_INIT_DONE` / `MCAL_DEINIT`）
  - [x] Port_Init/DeInit stub
  - [x] Dio_Init/DeInit stub
  - [x] CanIf_Init/DeInit stub（输出 `CANIF_INIT_DONE` / `ECUAL_DEINIT`）
  - [x] Service 层桩（Com/Dcm/NvM/Dem Init/DeInit）

### 验证镜像
- [x] 实现 `p1b_ecum_startup/main_ecum_startup.c`
  - [x] 调用 `EcuM_Init()` 阶段一
  - [x] 调用 `StartOS()`
  - [x] 调用 `EcuM_StartupTwo()` 阶段二
  - [x] 调用 `EcuM_StartupThree()` 阶段三
  - [x] 验证 BswM 状态 == RUN
  - [x] 调用 `EcuM_RequestShutdown()` 验证逆序 deinit
  - [x] 全部通过后调用 `Qemu_ReportPass()`
- [x] 实现 `p1b_ecum_startup/build.sh`

---

## Phase 3: 验证 (13h)

- [ ] 验证 S3.1: StartupPhaseOrder — 三阶段顺序正确
- [ ] 验证 S3.2: BswMRUNRequest — BswM 状态 == RUN
- [ ] 验证 S3.3: McalInitFirst — MCAL 先于 ECUAL
- [ ] 验证 S3.4: OrderlyShutdown — 逆序 deinit 正确
- [ ] CI `run_qemu_test.sh` exit code == 0

---

## 里程碑

| 里程碑 | 日期 | 交付物 |
|:-------|:-----|:-------|
| 规范完成 | 2026-08-22 | proposal + spec |
| 代码完成 | 2026-08-30 | main + stubs + build.sh |
| 验证通过 | 2026-09-01 | 4/4 Scenario PASS |

## 进度跟踪

| 任务 | 状态 | 负责人 | 开始日期 | 完成日期 |
|:-----|:-----|:-------|:---------|:---------|
| 规范定义 | ✅ | Track-A | 2026-08-22 | 2026-08-22 |
| 代码实现 | ✅ | Track-A | 2026-08-27 | 2026-08-23 |
| Scenario 验证 | ⏳ | Track-A | 2026-09-01 | — |
