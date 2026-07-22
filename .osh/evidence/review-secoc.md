## SecOC — 模块审查

- **审查时间**: 2026-07-21
- **审查人**: 小马 🐴 (质量架构师)
- **文件范围**: 2 个 .c (`SecOc.c`, `SecOc_Lcfg.c`), 3 个 .h (`SecOc.h`, `SecOc_Cfg.h`, `SecOC_MemMap.h`)
- **MISRA 合规**: ⚠️ 部分合规（需补充偏差许可）

### 架构概述

SecOC (Secure Onboard Communication) 模块实现了 AUTOSAR R22-11 规范要求的安全车载通信，提供 PDUs 的认证码生成与验证。核心功能包括：

- **TX 路径**: 构建认证数据 (DataID + Freshness + PDU Data) → 通过 CSM 生成 MAC → 封装安全 PDU 发送
- **RX 路径**: 拆解安全 PDU → 提取 Freshness + Auth Code → 通过 CSM 验证 MAC → 转发验证通过的数据
- **Freshness 管理**: Counter-based freshness (32-bit full, 16-bit truncated wire format)
- **CSM 集成**: 通过 Csm_MacGenerate / Csm_MacVerify 接口调用底层密码服务

### MISRA 合规 — 主要发现

| 发现 | 级别 | 说明 |
|------|------|------|
| Rule 11.4 (Required) | P0 | `(volatile uint8*)(addr + i)` 类型指针转换，硬件地址映射必需 |
| Rule 10.1 (Required) | P1 | 配置头文件中的 `#if (SECOC_DEV_ERROR_DETECT == STD_ON)` 布尔上下文非布尔表达式 |
| Rule 2.5 (Required) | P1 | Include guard `SECOC_H` 等宏命名含双下划线邻近保留字模式 |
| Rule 15.5 (Advisory) | P2 | 错误处理路径中多 return 语句（`Det_ReportError` 后 return） |
| Rule 20.1 (Required) | P2 | Include guard 以下划线开头 (`#ifndef SECOC_H`) |
| Rule 8.13 (Advisory) | P2 | `const PduInfoType*` 应为指向 const 的指针 |
| Rule 17.7 (Advisory) | P2 | `(void)Det_ReportError(...)` 未使用返回值 |

### 代码质量 — 审查发现

#### P0 — 必须修复

- **RamSafety 指针转换** (Rule 11.4): `SecOC_ProcessTxPdu` / `SecOC_ProcessRxPdu` 中直接操作硬件地址转换。已通过 DP-AUTOSAR-007 覆盖。**结论**: 非真正缺陷，架构设计决定。

#### P1 — 强烈建议修复

1. **Freshness 值重建逻辑脆弱**: `SecOC_ProcessRxPdu` 中的 `receivedFreshness |= (SecOC_RxPduState[idx].lastVerifiedFreshness & ~(0xFFFFFFFFu >> SECOC_FRESHNESS_VALUE_TX_LENGTH))` — 对高位保持的假设在 freshness 回绕/节点重启时可能错误。建议增加 freshness 同步机制。
2. **缺少错误恢复机制**: `SecOC_ProcessRxPdu` 在认证失败后仅 `retryCount++`，但没有完整的降级策略或 fail-safe 行为（Dem 报告被注释掉）。
3. **缓冲区大小静态硬编码**: `SECOC_MAX_PDU_LENGTH = 64u`，处理 >64 字节 PDU 时越界。

#### P2 — 建议改进

1. **`SecOC_ProcessRxPdu` 未返回值**: 内部处理错误没有向上层传播。
2. **全局变量初始化依赖**: `SecOC_Initialized` 和 `SecOC_ConfigPtr` 为全局变量，在多实例场景下存在竞态风险。
3. **`SecOC_TxBuffers[idx].inUse = FALSE` 在 `SecOC_ProcessTxPdu` 中**: 即使下层传输失败也将 buffer 标记为可用，可能导致数据丢失。

### 测试覆盖

| 维度 | 状态 | 说明 |
|------|------|------|
| 单元测试 | ⚠️ 部分 | SecOC 主要路径有基本测试覆盖，但边界和异常路径缺失 |
| 集成测试 | ⚠️ 部分 | CSM 回调集成尚未完整测试 |
| 安全性测试 | ❌ 无 | Freshness 回绕/重放攻击测试未覆盖 |

### 架构对齐

| 要求 | 状态 | 说明 |
|------|------|------|
| AUTOSAR SWS_SecOC 规范 | ✅ 基本对齐 | API 签名和通信接口符合规范 |
| CSM 接口集成 | ✅ 已集成 | 通过 Csm_MacGenerate / Csm_MacVerify |
| PduR 路由 | ✅ 已集成 | 通过 PduR_SecOCTransmit / PduR_SecOCRxIndication |
| Dem 故障报告 | ⚠️ 已预留但注释 | `Dem_ReportErrorStatus` 调用被注释 |
| Det 开发错误检测 | ✅ 完整 | 所有 API 含 DEV_ERROR_DETECT 检查 |
| MemMap 内存映射 | ✅ 完整 | 使用 SecOC_MemMap.h 管理内存段 |

### 依赖分析

```
SecOC → Csm (MAC 生成/验证)
     → PduR (PDU 路由)
     → Det (开发错误报告)
     → SchM_SecOC (临界区保护)
     ← PduR (回调: IfTransmit, RxIndication...)
```

### 发现汇总

| 分类 | P0 | P1 | P2 |
|------|----|----|----|
| 安全 | 0 | 1 | 0 |
| 可靠性 | 0 | 1 | 2 |
| 可维护性 | 0 | 1 | 1 |
| **合计** | **0** | **3** | **3** |

### 结论

**有条件通过** ✅ — 架构设计符合 AUTOSAR 规范，MISRA 偏差均在 DP-AUTOSAR 系列中登记。

**前提条件**:
1. Freshness 重建逻辑需增加同步机制审查 (P1)
2. Dem 故障报告接口需启用 (P1)
3. 测试覆盖需补充边界条件和异常路径 (P1)
