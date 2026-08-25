# 变更提案：QEMU NvM 掉电恢复验证

> **变更 ID**: dev-qemu-nvm-persist  
> **状态**: Proposed  
> **优先级**: P2  
> **负责人**: Track-A  
> **创建日期**: 2026-08-22  
> **依赖变更**: dev-qemu-assert-infra, dev-qemu-ecum-startup  
> **目标版本**: v1.6.0-beta.1  
> **估计工时**: 24h (3d)

## 背景

NvM 的掉电恢复是嵌入式系统的关键安全场景。当前在 native 测试中验证了 CRC 和写计数器，但从未在 QEMU 上验证"写入 → QEMU reset → 读回"的完整持久化路径。

## 目标

1. 通过 semihosting `SYS_WRITE` 将 `Fls_Hw_MockFlash` 数组内容导出到 host 文件 `flash.bin`
2. 第二次 QEMU 启动时加载 `flash.bin`，验证 `NvM_ReadAll` 恢复数据
3. 验证 CRC 损坏场景下 `NVM_REQ_INTEGRITY_FAILED` 错误码

## 范围

### 包含内容
- `src/bsw/mcal/fls/src/Fls_Hw.c` — 添加 semihosting export 宏
- `tests/qemu_full_stack/p2b_nvm_persist/main_nvm_write.c` — 写入阶段入口
- `tests/qemu_full_stack/p2b_nvm_persist/main_nvm_read.c` — 读回阶段入口
- `tests/qemu_full_stack/p2b_nvm_persist/build.sh` — 两阶段构建脚本

### 不包含内容
- 真实 Flash 硬件模拟（使用内存数组 + semihosting 导出）

## 验收标准

- [ ] S5.1: `NvM_GetErrorStatus()` == `NVM_REQ_OK`
- [ ] S5.2: `flash.bin` 大小 > 0，magic 字节 == 0xAA55
- [ ] S5.3: 重启后 `Rte_Read_OdometerValue()` == 0xDEADBEEF
- [ ] S5.4: CRC 损坏后 `NVM_REQ_INTEGRITY_FAILED`
- [ ] CI 两阶段均 exit code == 0

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| semihosting SYS_WRITE 文件操作在 QEMU 上行为不一致 | 中 | 高 | 先验证 SYS_OPEN/WRITE/CLOSE 基本操作 |
| Fls_Hw_MockFlash 数组过大导致 DTCM 溢出 | 低 | 中 | 使用 4KB 最小化 Flash 模拟区 |
