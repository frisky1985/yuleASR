# 变更提案：QEMU SecOC 回环验证

> **变更 ID**: dev-qemu-secoc-loopback  
> **状态**: Proposed  
> **优先级**: P2  
> **负责人**: Track-B  
> **创建日期**: 2026-08-22  
> **依赖变更**: dev-qemu-assert-infra, dev-qemu-can-loopback  
> **目标版本**: v1.6.0-beta.1  
> **估计工时**: 44h (5.5d)

## 背景

yuleASR 已实现 SecOC（`src/bsw/services/secoc/`），但其 CMAC 鉴权路径（`SecOC_VerifyParsedAuthenticatedPdu` → `Csm_MacVerify` → CSM/Crypto）从未在 QEMU 上端到端运行过。SecOC 验证需要：发送方生成 CMAC → 附加到 PDU → 接收方验证 CMAC → 通过/失败。当前仅有单元测试验证孤立函数，缺少全栈回环验证。

## 目标

1. 在 QEMU 上构建 SecOC 发送→CAN loopback→SecOC 接收的完整回环路径
2. 验证合法 PDU（正确 CMAC）被 SecOC 接收方接受（`SECOC_AUTHPDU_ACCEPTED`）
3. 验证篡改 PDU（错误 CMAC）被 SecOC 接收方拒绝（`SECOC_AUTHPDU_REJECTED`）
4. 验证 FreshnessValue（防重放）机制生效

## 范围

### 包含内容
- `tests/qemu_full_stack/p3c_secoc_loopback/main_secoc_loopback.c` — 验证入口
- `tests/qemu_full_stack/p3c_secoc_loopback/secoc_crypto_stub.c` — CSM/Crypto 桩（AES-128 CMAC）
- `tests/qemu_full_stack/p3c_secoc_loopback/build.sh` — 构建脚本

### 不包含内容
- 硬件 HSM 加速路径（使用软件 AES-128 桩）
- SecOC 密钥管理（使用硬编码测试密钥）

## 验收标准

- [ ] S8.1: 合法 PDU（CMAC 匹配）— SecOC 验证返回 `E_OK`，`SECOC_AUTHPDU_ACCEPTED` 计数 +1
- [ ] S8.2: 篡改 PDU（CMAC 最后 1 字节翻转）— SecOC 验证返回 `E_NOT_OK`，`SECOC_AUTHPDU_REJECTED` 计数 +1
- [ ] S8.3: 重放攻击（FreshnessValue 不变的重发帧）— SecOC 验证失败
- [ ] S8.4: 10 帧连续回环，合法帧全通过，篡改帧全拒绝，无误判
- [ ] CI `run_qemu_test.sh` exit code == 0，UART 含 `SECOC_LOOPBACK_PASS`

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| SecOC 依赖 Csm/Crypto 模块配置复杂 | 高 | 高 | 提供最小化 AES-128 CMAC 软件桩，绕过 Csm 配置层 |
| FreshnessValue 管理依赖 NvM 持久化 | 中 | 中 | 测试环境使用 RAM 模拟，单调递增即可 |
| SecOC PDU 截断长度与 CAN DLC 冲突 | 低 | 中 | 固定 8 字节 DLC，CMAC 截断至 4 字节 |
