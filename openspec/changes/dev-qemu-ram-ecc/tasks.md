# Tasks: QEMU RAM ECC 故障注入验证

> **变更 ID**: dev-qemu-ram-ecc  
> **状态**: Proposed  
> **创建日期**: 2026-08-22

## 任务总览

```
dev-qemu-ram-ecc
├── Specification
│   ├── proposal.md
│   └── specs/QemuRamEcc_spec.md
├── Source Code
│   ├── p3a_ram_ecc/main_ram_ecc.c
│   ├── p3a_ram_ecc/ram_ecc_fault_inject.c
│   └── p3a_ram_ecc/build.sh
└── Verification
    └── 4 个 Scenario 验证 (S9.1 - S9.4)
```

---

## Phase 1: 规范定义 (4h)

- [x] 创建 OpenSpec change 目录
- [x] 编写 `proposal.md`
- [x] 编写 `specs/QemuRamEcc_spec.md`

---

## Phase 2: 代码实现 (24h)

### 故障注入工具
- [x] 实现 `p3a_ram_ecc/ram_ecc_fault_inject.c`
  - [x] `FaultInject_FlipBit1(addr, bitPos)` — 翻转 1 bit（SEC 模拟）
  - [x] `FaultInject_FlipBit2(addr, bitPos1, bitPos2)` — 翻转 2 bit（DED 模拟）
  - [x] `FaultInject_RegisterEccCallback(cb)` — 注册 ECC 中断模拟回调（替代 MSCM 中断）
  - [x] `FaultInject_TriggerEccIrq(type)` — 直接调用 RamSafety ECC 处理函数

### 验证主镜像
- [x] 实现 `p3a_ram_ecc/main_ram_ecc.c`
  - [x] 初始化 RamSafety（4 个 RAM 分区配置）
  - [x] S9.1: 注入 SEC，验证状态保持 RAMSAFETY_OK + Dem DTC 上报
  - [x] S9.2: 注入 DED，验证 EnterSafeState 被调用（测试桩记录调用）
  - [x] S9.3: March C- 校验写保护分区完整性
  - [x] S9.4: 连续 5 次 SEC，验证错误计数阈值触发降级
  - [x] 全部通过后调用 `Qemu_ReportPass()`
- [x] 实现 `p3a_ram_ecc/build.sh`

---

## Phase 3: 验证 (12h)

- [ ] 验证 S9.1: SEC 错误检测与纠正
- [ ] 验证 S9.2: DED 错误触发安全状态
- [ ] 验证 S9.3: SafeRAM 分区写保护完整性
- [ ] 验证 S9.4: 错误计数阈值降级策略
- [ ] CI `run_qemu_test.sh` exit code == 0，UART 含 `RAM_ECC_PASS`

---

## 里程碑

| 里程碑 | 日期 | 交付物 |
|:-------|:-----|:-------|
| 规范完成 | 2026-08-22 | proposal + spec |
| 代码完成 | 2026-09-15 | main + fault_inject + build.sh |
| 验证通过 | 2026-09-17 | 4/4 Scenario PASS |

## 进度跟踪

| 任务 | 状态 | 负责人 | 开始日期 | 完成日期 |
|:-----|:-----|:-------|:---------|:---------|
| 规范定义 | ✅ | Track-B | 2026-08-22 | 2026-08-22 |
| 代码实现 | ✅ | Track-B | 2026-08-23 | 2026-08-23 |
| Scenario 验证 | ⏳ | Track-B | — | — |
