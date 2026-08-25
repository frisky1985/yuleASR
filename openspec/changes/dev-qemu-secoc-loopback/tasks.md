# Tasks: QEMU SecOC 回环验证

> **变更 ID**: dev-qemu-secoc-loopback  
> **状态**: Proposed  
> **创建日期**: 2026-08-22

## 任务总览

```
dev-qemu-secoc-loopback
├── Specification
│   ├── proposal.md
│   └── specs/QemuSecocLoopback_spec.md
├── Source Code
│   ├── p3c_secoc_loopback/main_secoc_loopback.c
│   ├── p3c_secoc_loopback/secoc_crypto_stub.c
│   └── p3c_secoc_loopback/build.sh
└── Verification
    └── 4 个 Scenario 验证 (S8.1 - S8.4)
```

---

## Phase 1: 规范定义 (4h)

- [x] 创建 OpenSpec change 目录
- [x] 编写 `proposal.md`
- [x] 编写 `specs/QemuSecocLoopback_spec.md`

---

## Phase 2: 代码实现 (24h)

### CSM/Crypto 桩
- [x] 实现 `p3c_secoc_loopback/secoc_crypto_stub.c`
  - [x] 实现 `Csm_MacGenerate：调用 `libs_aes` AES-128-CMAC 生成 4 字节 CMAC
  - [x] 实现 `Csm_MacVerify：比较实际 CMAC 与 PDU 内嵌 CMAC
  - [x] 实现 `FvM_GetRxFreshnessValue` / `FvM_UpdateCounter`：RAM 单调计数器

### 验证主镜像
- [x] 实现 `p3c_secoc_loopback/main_secoc_loopback.c`
  - [x] 初始化 SecOC 发送方（密钥、FreshnessValue 起始值）
  - [x] S8.1: 发送合法 PDU，断言 `SECOC_AUTHPDU_ACCEPTED`
  - [x] S8.2: 篡改 CMAC 最后字节，断言 `SECOC_AUTHPDU_REJECTED`
  - [x] S8.3: 重放旧 FreshnessValue 帧，断言验证失败
  - [x] S8.4: 10 帧连续回环，统计通过/拒绝计数
  - [x] 全部通过后调用 `Qemu_ReportPass()`
- [x] 实现 `p3c_secoc_loopback/build.sh`

---

## Phase 3: 验证 (16h)

- [ ] 验证 S8.1: 合法 PDU 被接受
- [ ] 验证 S8.2: 篡改 PDU 被拒绝
- [ ] 验证 S8.3: 重放攻击被拒绝
- [ ] 验证 S8.4: 10 帧连续回环无误判
- [ ] CI `run_qemu_test.sh` exit code == 0，UART 含 `SECOC_LOOPBACK_PASS`

---

## 里程碑

| 里程碑 | 日期 | 交付物 |
|:-------|:-----|:-------|
| 规范完成 | 2026-08-22 | proposal + spec |
| 代码完成 | 2026-09-12 | main + stub + build.sh |
| 验证通过 | 2026-09-15 | 4/4 Scenario PASS |

## 进度跟踪

| 任务 | 状态 | 负责人 | 开始日期 | 完成日期 |
|:-----|:-----|:-------|:---------|:---------|
| 规范定义 | ✅ | Track-B | 2026-08-22 | 2026-08-22 |
| 代码实现 | ✅ | Track-B | 2026-08-23 | 2026-08-23 |
| Scenario 验证 | ⏳ | Track-B | — | — |
