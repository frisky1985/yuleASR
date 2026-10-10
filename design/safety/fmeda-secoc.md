# FMEDA — SecOC (Secure Onboard Communication)

> **文档编号**: SAF-FMEDA-SECOC-001
> **版本**: 1.0
> **日期**: 2026-10-24
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW Service Layer 的 SecOC (Secure Onboard Communication) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/services/secoc/src/SecOC.c`

**分析边界**:
- SecOC 核心逻辑（PDU 认证/验证、Freshness 管理、安全状态机）
- 上层接口：`SecOC_IfTransmit`, `SecOC_IfRxIndication`, `SecOC_MainFunctionTx`, `SecOC_MainFunctionRx`
- 内部安全机制：HMAC-SHA256/AES-CMAC 认证码生成与验证、计数器型 Freshness 值管理、Freshness NvM 持久化、验证重试计数
- 下层依赖（假设由 ECUAL/MCAL 层覆盖）：Csm (Crypto Service Manager), PduR, NvM, Det

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 SecOC 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `SecOC_BufferType` | SecOC.c:54-59 | PDU 缓冲区：data[256], length, inUse, pduId |
| `SecOC_RxPduStateType` | SecOC.c:61-69 | RX PDU 状态：freshnessValue, lastVerifiedFreshness, status, lastResult, retryCount, authInProgress, timeoutCounter |
| `SecOC_TxPduStateType` | SecOC.c:71-74 | TX PDU 状态：freshnessValue, txInProgress |
| `SecOC_FreshnessNvMType` | SecOC.c:92-97 | Freshness 持久化结构：txFreshness[], rxLastVerifiedFreshness[], syncMasterFreshness, magic (0x534F4346) |
| `SecOC_ConfigType` | SecOC.h:199-209 | 模块配置：txPduConfigs, rxPduConfigs, numTxPdus, numRxPdus, overrideStatusAllowed |
| `SecOC_PduConfigType` | SecOC.h:184-194 | PDU 配置：pduId, authConfig, freshnessConfig, dataToAuthOffset/Length |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | TX PDU 认证发送 | `SecOC_ProcessTxPdu()` (L262), `SecOC_IfTransmit()` (L569) | 对发送 PDU 附加 Freshness 值并生成 MAC 认证码 |
| FU-02 | RX PDU 认证验证 | `SecOC_ProcessRxPdu()` (L347), `SecOC_IfRxIndication()` (L618) | 对接收 PDU 提取并验证 MAC 认证码，丢弃未验证消息 |
| FU-03 | Freshness 值管理 | `SecOC_IncrementFreshness()` (L154), `SecOC_SyncMasterFreshness` (L89) | 计数器型 Freshness 值递增、溢出保护（阈值 0xF0000000） |
| FU-04 | 认证数据构建 | `SecOC_BuildAuthData()` (L210) | 拼接 DataID + Freshness + PDU Data 构建认证输入数据 |
| FU-05 | MAC 生成/验证 | `SecOC_GenerateAuthCode()` (L237), `SecOC_VerifyAuthCode()` (L250) | 调用 Csm 接口进行 MAC 生成和验证（SINGLESHOT 模式） |
| FU-06 | Freshness NvM 持久化 | `SecOC_RestoreFreshnessFromNvM()` (L165), `SecOC_ScheduleFreshnessWrite()` (L186) | 重启时恢复 Freshness 值，脏数据调度写入 NvM |
| FU-07 | 验证状态管理 | `SecOC_GetVerificationStatus()` (L753), `SecOC_VerifyStatusOverride()` (L730) | 查询/覆盖 PDU 验证状态 |
| FU-08 | 主函数调度 | `SecOC_MainFunctionTx()` (L686), `SecOC_MainFunctionRx()` (L711) | 周期调度 TX/RX 处理，驱动 Freshness 持久化写入 |
| FU-09 | 初始化/反初始化 | `SecOC_Init()` (L450), `SecOC_DeInit()` (L508) | 模块初始化状态清零、Freshness NvM 恢复、反初始化资源释放 |

---

## 3. 失效模式分析表

### 3.1 FU-01: TX PDU 认证发送 (`SecOC_ProcessTxPdu`, `SecOC_IfTransmit`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | TX 缓冲区指针为空或 PduId 越界 | 发送未认证数据或内存访问异常 | `SecOC_IfTransmit()` 入口校验：`idx < 0` 返回 E_NOT_OK (L589)；`SecOC_GetTxPduIndex()` 范围检查 `pduId < SECOC_NUM_TX_PDUS` (L132) | 99% | B |
| FM-01-02 | TX 缓冲区已被占用 (`inUse = TRUE`) | 新 PDU 被丢弃，发送延迟 | `SecOC_IfTransmit()` 检查 `SecOC_TxBuffers[idx].inUse` (L597)，返回 E_NOT_OK；上层可重试 | 90% | B |
| FM-01-03 | MAC 生成失败（Csm 返回错误） | PDU 未附加认证码即被丢弃，不发送 | `SecOC_GenerateAuthCode()` 返回 E_NOT_OK 时，`SecOC_ProcessTxPdu()` 直接返回 E_NOT_OK (L303-304)，不执行 `PduR_SecOCTransmit` | 99% | B |
| FM-01-04 | Freshness 值溢出超过重置阈值 | Freshness 回绕至 0，可能导致重放保护失效 | `SecOC_IncrementFreshness()` 检查 `freshness >= SECOC_FRESHNESS_RESET_THRESHOLD` (0xF0000000) 时重置为 0 (L158-159)；配合 NvM 持久化防止重启回绕 | 85% | B |
| FM-01-05 | `memcpy` 数据拷贝长度错误 | 发送数据截断或缓冲区溢出 | `SecOC_IfTransmit()` 校验 `SduLength <= SECOC_MAX_PDU_LENGTH` (256) 后才执行拷贝 (L652)；`SecOC_BuildAuthData` 使用固定长度拼接 | 90% | B |
| FM-01-06 | SecOC 未初始化时调用发送 | 使用未初始化的 Freshness 值和缓冲区 | `SecOC_IfTransmit()` 检查 `SecOC_Initialized_Local == FALSE` 时报 Det 错误并返回 E_NOT_OK (L575-578) | 99% | B |

### 3.2 FU-02: RX PDU 认证验证 (`SecOC_ProcessRxPdu`, `SecOC_IfRxIndication`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | 接收 PDU 长度不足（小于 Freshness + MAC 长度） | 无法提取有效认证数据 | `SecOC_ProcessRxPdu()` 检查 `securedPduLen < (SECOC_FRESHNESS_TX_BYTE_LEN + SECOC_AUTH_INFO_LENGTH)` (L376)，失败则标记 `SECOC_VERIFICATIONFAILURE_STATUS` 并丢弃 | 99% | B |
| FM-02-02 | MAC 验证失败（篡改或传输错误） | 接收伪造/损坏数据 | `SecOC_VerifyAuthCode()` 返回非 `CSM_E_VER_OK` 时，标记 `SECOC_VERIFICATIONFAILURE` (L428-429)，**不转发数据到上层**（安全状态） | 99% | B |
| FM-02-03 | RX 缓冲区已被占用（前一个 PDU 尚未验证完成） | 新接收 PDU 被丢弃 | `SecOC_IfRxIndication()` 检查 `SecOC_RxBuffers[idx].inUse` (L645)，busy 时直接 return | 90% | B |
| FM-02-04 | 验证重试次数超限 | 持续验证失败，该 PDU 通道不可用 | `retryCount >= SECOC_VERIFICATION_RETRY_COUNT` (3) 时上报诊断事件 (L432-434)；上层可通过 `SecOC_GetVerificationStatus()` 查询状态 | 85% | B |
| FM-02-05 | Freshness 值重建错误（截断恢复不完整） | 使用错误的 Freshness 值验证，导致合法消息被拒绝 | RX 路径通过高位拼接重建完整 Freshness (L394-395)：`receivedFreshness |= (lastVerifiedFreshness & ~mask)`；验证失败时不更新 `lastVerifiedFreshness` (L413 仅在成功时更新) | 80% | B |
| FM-02-06 | SecOC 未初始化时调用接收 | 使用未初始化状态进行验证 | `SecOC_IfRxIndication()` 检查 `SecOC_Initialized_Local == FALSE` 时报 Det 错误 (L623-626) | 99% | B |

### 3.3 FU-03: Freshness 值管理 (`SecOC_IncrementFreshness`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | Freshness 计数器内存位翻转 | Freshness 值跳变，导致后续验证失败或重放窗口异常 | NvM 持久化恢复时通过 magic 校验 (0x534F4346) 检测数据完整性 (L174)；验证失败不更新 `lastVerifiedFreshness` | 90% | B |
| FM-03-02 | Freshness 溢出阈值配置错误 | Freshness 过早重置或无法重置 | 编译时配置校验：`SECOC_FRESHNESS_RESET_THRESHOLD` (0xF0000000) 与 `SECOC_MAX_FRESHNESS_VALUE` (0xFFFFFFFF) 交叉验证 | 85% | B |
| FM-03-03 | `SecOC_SyncMasterFreshness` 与 TX Freshness 不同步 | 多 PDU 间 Freshness 不一致 | 同步主计数器在 Init 时统一初始化 (L491)；NvM 持久化时同步保存/恢复 (L183, L199) | 80% | B |

### 3.4 FU-04: 认证数据构建 (`SecOC_BuildAuthData`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | `authData` 缓冲区溢出 | 栈/内存破坏，认证数据损坏 | 缓冲区大小 `SECOC_MAX_PDU_LENGTH + SECOC_FRESHNESS_BYTE_LEN + 1` (L265) 覆盖最大 PDU 长度 + Freshness + DataID；编译时约束 | 85% | B |
| FM-04-02 | Freshness 字节序编码错误 | TX/RX 端 Freshness 不一致，验证永远失败 | TX 和 RX 使用相同的字节序拼接逻辑 (L221-223 vs L388-390)；单元测试覆盖字节序一致性 | 80% | B |
| FM-04-03 | DataID 与 PDU 配置不匹配 | 认证数据构建使用错误 DataID，MAC 验证失败 | DataID 来源于 PDU 配置中的 `pduId` (L297)，与 Lcfg 配置交叉验证 | 75% | B |

### 3.5 FU-05: MAC 生成/验证 (`SecOC_GenerateAuthCode`, `SecOC_VerifyAuthCode`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | Csm MAC 生成返回非 E_OK | TX PDU 无法发送 | `SecOC_ProcessTxPdu()` 检查返回值 (L303)，失败则丢弃 PDU 并返回 E_NOT_OK | 99% | B |
| FM-05-02 | Csm MAC 验证返回非 `CSM_E_VER_OK` | 接收 PDU 被丢弃（安全行为） | `SecOC_ProcessRxPdu()` 检查 `verifyResult == CSM_E_VER_OK` (L410)，非 OK 则标记失败、不转发数据 | 99% | B |
| FM-05-03 | Csm 作业 ID 配置错误 | MAC 操作使用错误密钥或算法 | 编译时配置校验：`SECOC_CSM_JOB_ID_AUTH` / `SECOC_CSM_JOB_ID_VERIFY` 与 Csm 配置一致性检查 | 85% | B |
| FM-05-04 | SINGLESHOT 模式配置被误改为 START/FINISH | MAC 生成/验证流程不完整 | 代码硬编码 `CSM_OPERATIONMODE_SINGLESHOT` (L242, L254)；注释说明此为 P1 Phase 8 修复项 | 90% | B |
| FM-05-05 | `authCodeLen` 传入截断长度导致 Csm 失败 | MAC 生成失败，TX 路径中断 | 传入 `SECOC_MAX_AUTH_INFO_LEN` (32) 而非截断长度 (16)，确保 Csm 缓冲区足够 (L273)；注释说明此为 P1 Phase 8 修复项 | 95% | B |

### 3.6 FU-06: Freshness NvM 持久化 (`SecOC_RestoreFreshnessFromNvM`, `SecOC_ScheduleFreshnessWrite`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | NvM 读取失败 | Freshness 值从 0 开始，可能允许重放攻击 | `NvM_ReadBlock` 返回非 E_OK 时函数直接 return (L171-172)，Freshness 保持 Init 初始化的 0 | 75% | B |
| FM-06-02 | NvM 数据 magic 校验失败 | 恢复无效的 Freshness 数据 | `restored.magic != SECOC_FRESHNESS_NVM_MAGIC` (0x534F4346) 检查 (L174-175)，失败则不恢复 | 95% | B |
| FM-06-03 | NvM 写入调度失败 | Freshness 值未持久化，重启后回绕 | `SecOC_ScheduleFreshnessWrite()` 仅在 `SecOC_FreshnessDirty == TRUE` 时写入 (L190)；写入后清除脏标志 (L202) | 85% | B |
| FM-06-04 | 频繁 NvM 写入导致 Flash 磨损 | NvM 块寿命降低 | 脏标志机制避免不必要的写入 (L190-191)；仅在 TX 发送或 RX 验证成功时设置脏标志 (L292, L415) | 80% | B |

### 3.7 FU-07: 验证状态管理 (`SecOC_GetVerificationStatus`, `SecOC_VerifyStatusOverride`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | 验证状态覆盖被滥用 | 未验证消息被标记为已验证，绕过安全机制 | `SecOC_VerifyStatusOverride()` 检查 `overrideStatusAllowed` 配置 (L741)；当前配置 `SecOc_Config.overrideStatusAllowed = FALSE` (Lcfg.c:156) 禁止覆盖 | 95% | B |
| FM-07-02 | PduId 无效导致状态查询返回默认值 | 上层无法获取正确验证状态 | `SecOC_GetVerificationStatus()` 对 `idx < 0` 返回 `SECOC_UNVERIFIED` (L758)；上层应将未验证消息视为不可信 | 99% | B |
| FM-07-03 | `resultPtr` 为空指针 | 验证结果写入非法地址 | `SecOC_GetVerificationResult()` 入口检查 `resultPtr == NULL_PTR` (L772)，返回 E_NOT_OK | 99% | B |

### 3.8 FU-08: 主函数调度 (`SecOC_MainFunctionTx`, `SecOC_MainFunctionRx`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | MainFunction 未被周期调用 | TX PDU 积压不发送，RX PDU 积压不验证 | 系统级 WDT 监控（BswM/SchM 调度监控）；`SecOC_Initialized_Local` 检查 (L690, L715) | 80% | B |
| FM-08-02 | TX 处理中 `txInProgress` 标志异常 | PDU 重复发送或永远不发送 | `SecOC_MainFunctionTx()` 检查 `txInProgress == FALSE` 才处理 (L696)；`SecOC_TxConfirmation()` 清除标志 (L677) | 85% | B |
| FM-08-03 | RX 处理中 `authInProgress` 标志未清除 | PDU 验证状态不一致，后续 PDU 被阻塞 | `SecOC_ProcessRxPdu()` 在所有退出路径均清除 `authInProgress = FALSE` (L379, L438) 和 `inUse = FALSE` (L380, L439) | 90% | B |
| FM-08-04 | SecOC 未初始化时调用主函数 | 使用未初始化状态处理 PDU | `SecOC_MainFunctionTx/Rx()` 检查 `SecOC_Initialized_Local == FALSE` 时直接 return (L690, L715) | 99% | B |

### 3.9 FU-09: 初始化/反初始化 (`SecOC_Init`, `SecOC_DeInit`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | 重复初始化 | 状态被重置，正在进行的认证/验证中断 | `SecOC_Init()` 检查 `SecOC_Initialized_Local == TRUE` 时报 `SECOC_E_ALREADY_INITIALIZED` (L455-458) | 99% | B |
| FM-09-02 | `configPtr` 为空指针 | 模块使用空配置运行 | `SecOC_Init()` 检查 `configPtr == NULL_PTR` 时报 `SECOC_E_PARAM_POINTER` (L461-463) | 99% | B |
| FM-09-03 | 反初始化后调用 API | 使用已释放的状态数据 | 各 API 入口检查 `SecOC_Initialized_Local == FALSE` 时报 `SECOC_E_UNINIT` (L513, L575, L623) | 99% | B |
| FM-09-04 | 排他区保护失效（中断嵌套） | 初始化过程中状态被并发访问破坏 | `SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0()` / `SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0()` 保护关键区 (L468, L501) | 90% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | TX PDU 认证发送 | 99% | 94% | MAC 生成失败检测 + 参数校验 |
| FU-02 | RX PDU 认证验证 | 99% | 94% | MAC 验证失败丢弃 + 长度检查 |
| FU-03 | Freshness 值管理 | 90% | 85% | NvM magic 校验 + 溢出保护 |
| FU-04 | 认证数据构建 | 85% | 80% | 缓冲区大小约束 + 字节序一致性 |
| FU-05 | MAC 生成/验证 | 99% | 92% | Csm 返回值检查 + SINGLESHOT 模式 |
| FU-06 | Freshness NvM 持久化 | 95% | 85% | magic 校验 + 脏标志调度 |
| FU-07 | 验证状态管理 | 99% | 98% | override 禁止 + 空指针检查 |
| FU-08 | 主函数调度 | 99% | 89% | WDT 监控 + 状态标志管理 |
| FU-09 | 初始化/反初始化 | 99% | 97% | 重复初始化检测 + 排他区保护 |

### 4.2 模块整体诊断覆盖率

```
DC_SecOC = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_SecOC ≈ 91%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL B)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 90% | ~91% | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ~90% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 100 FIT | ~50 FIT | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。SecOC 中主要单点故障来源：
- 认证数据构建中字节序编码的不可检测故障（极低概率，由单元测试和 TX/RX 对称逻辑覆盖）
- Freshness 同步主计数器与 TX 计数器不同步（由 NvM 持久化覆盖）
- MainFunction 调度瞬态故障（由 WDT 覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (8.1 / 95) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。SecOC 中主要潜伏故障来源：
- Csm MAC 算法实现的系统性缺陷（通过 Csm 模块独立 FMEDA 覆盖）
- Freshness NvM 持久化逻辑故障（通过 magic 校验和脏标志机制检测）
- 验证状态覆盖逻辑故障（通过 `overrideStatusAllowed` 配置禁止）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (9.5 / 95) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_freshness_desync + lambda_auth_data_build + lambda_nv_restore_failure
     ≈ 8 + 17 + 25 = 50 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 SecOC 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | 模块已初始化，所有 PDU 认证/验证正常 | TX 路径正常附加 MAC 发送；RX 路径验证通过后转发数据到上层 |
| **降级运行** | 部分 PDU 验证失败但在重试阈值内 | 验证失败的 PDU 被丢弃，其他 PDU 正常处理；`retryCount` 递增 |
| **安全停止（TX）** | MAC 生成持续失败或 Freshness NvM 恢复失败 | TX PDU 全部无法发送；上报诊断事件 `SECOC_E_CRYPTO_AUTH_FAILED` |
| **安全停止（RX）** | 所有 RX PDU 验证持续失败 | 所有接收消息被丢弃，信号置为无效值；上报诊断事件 |

### 6.2 安全状态转换

```
正常运行 ──[RX MAC验证失败]──→ 降级运行 (retryCount++)
降级运行 ──[retryCount >= 3]──→ 安全停止(RX) (上报 Dem)
正常运行 ──[TX MAC生成失败]──→ 安全停止(TX) (PDU丢弃)
任意状态 ──[MainFunction超时]──→ 安全停止 (WDT触发)
安全停止(RX) ──[SecOC_DeInit + SecOC_Init]──→ 正常运行 (恢复)
```

### 6.3 安全行为关键约束

1. **未验证消息丢弃**: `SecOC_ProcessRxPdu()` 中 MAC 验证失败时，**不调用** `PduR_SecOCRxIndication()` (L427-429 分支无转发)，确保伪造/篡改数据不会到达应用层
2. **信号无效化**: 验证失败后 `status` 置为 `SECOC_VERIFICATIONFAILURE_STATUS`，上层通过 `SecOC_GetVerificationStatus()` 可检测到异常
3. **Freshness 不回绕**: 验证失败时不更新 `lastVerifiedFreshness` (L413 仅在成功分支执行)，防止攻击者通过注入伪造消息推进 Freshness 计数器

---

## 7. 故障反应时间线

### 7.1 场景一：RX PDU MAC 验证失败（单次）

| 时间 | 事件 |
|------|------|
| T=0 | 下层调用 `SecOC_IfRxIndication(RxPduId, PduInfoPtr)` |
| T=1 cycle | 数据拷贝到 `SecOC_RxBuffers[idx]`，`inUse = TRUE` (L654-655) |
| T=2 cycles | `SecOC_MainFunctionRx()` 调度，调用 `SecOC_ProcessRxPdu()` (L721) |
| T=3 cycles | 提取 Freshness 值、重组认证数据、调用 `SecOC_VerifyAuthCode()` (L407) |
| T=4 cycles | Csm 返回 `verifyResult != CSM_E_VER_OK`，检测到故障 |
| T=5 cycles | 标记 `SECOC_VERIFICATIONFAILURE`，`retryCount++` (L429-430)；**不转发数据** |
| T=6 cycles | `authInProgress = FALSE`, `inUse = FALSE` (L438-439)，缓冲区释放 |
| **总延迟** | **约 6 个主函数周期（60ms @ 10ms 周期）** |

### 7.2 场景二：RX PDU 验证重试超限

| 时间 | 事件 |
|------|------|
| T=0 ~ T=5 | 第 1 次验证失败（同场景一） |
| T=6 ~ T=11 | 第 2 次接收 + 验证失败，`retryCount = 2` |
| T=12 ~ T=17 | 第 3 次接收 + 验证失败，`retryCount = 3 >= SECOC_VERIFICATION_RETRY_COUNT` |
| T=18 | 触发诊断事件上报（L433-434） |
| T=19 | 上层通过 `SecOC_GetVerificationStatus()` 查询到 `SECOC_VERIFICATIONFAILURE_STATUS` |
| **总延迟** | **约 18 个主函数周期（180ms @ 10ms 周期）** |

### 7.3 场景三：TX MAC 生成失败

| 时间 | 事件 |
|------|------|
| T=0 | 上层调用 `SecOC_IfTransmit(TxPduId, PduInfoPtr)` |
| T=1 cycle | 数据拷贝到 `SecOC_TxBuffers[idx]`，`inUse = TRUE` (L604-606) |
| T=2 cycles | `SecOC_MainFunctionTx()` 调度，调用 `SecOC_ProcessTxPdu()` (L698) |
| T=3 cycles | Freshness 递增、`SecOC_BuildAuthData()` 构建认证数据 (L296) |
| T=4 cycles | `SecOC_GenerateAuthCode()` 调用 Csm，返回 E_NOT_OK (L303) |
| T=5 cycles | `SecOC_ProcessTxPdu()` 返回 E_NOT_OK，**PDU 不发送** |
| T=6 cycles | `inUse = FALSE`, `txInProgress = FALSE` (L337-338)，缓冲区释放 |
| **总延迟** | **约 5 个主函数周期（50ms @ 10ms 周期）** |

### 7.4 场景四：Freshness NvM 恢复失败（重启后）

| 时间 | 事件 |
|------|------|
| T=0 | 系统重启，调用 `SecOC_Init()` |
| T=1 cycle | 状态清零，`SecOC_Initialized_Local = TRUE` (L493) |
| T=2 cycles | 调用 `SecOC_RestoreFreshnessFromNvM()` (L498) |
| T=3 cycles | `NvM_ReadBlock()` 返回失败或 magic 不匹配 (L171-175) |
| T=4 cycles | 函数 return，Freshness 值保持 0 |
| T=5 cycles | 后续 TX 发送从 Freshness=0 开始，RX 验证使用 lastVerifiedFreshness=0 |
| **影响** | **重启后首次通信窗口内，重放保护能力降低；NvM 恢复后首次正常通信即恢复正常保护** |

---

## 8. ISO 26262 条款追溯

| ISO 26262 条款 | 要求 | 本文档覆盖 |
|---------------|------|-----------|
| Part 5, Clause 7 | FMEDA 方法论 | 第 3 节完整失效模式表 |
| Part 5, Clause 8 | 安全度量 (SPFM/LFM/PMHF) | 第 5 节度量分析 |
| Part 5, Clause 9 | 诊断覆盖率评估 | 第 4 节 DC 汇总 |
| Part 6, Clause 8 | 软件单元设计中的安全机制 | 第 3 节故障检测方法列 |
| Part 4, Clause 7 | 安全状态定义 | 第 6 节安全状态 |
| Part 4, Clause 8 | 技术安全需求 | 第 7 节故障反应时间线 |
| Part 6, Clause 7 | 安全机制（密码学完整性） | FU-02 RX 验证丢弃 + FU-05 MAC 验证 |
| Part 6, Clause 9 | 防重放攻击（Freshness 管理） | FU-03 Freshness 递增 + FU-06 NvM 持久化 |

---

## 9. 假设与约束

1. **Csm 层假设**: Crypto Service Manager (Csm) 的失效率及诊断覆盖率由独立 FMEDA 覆盖，本分析假设 Csm 的 MAC 生成/验证功能满足 ASIL B 要求
2. **密钥安全**: 假设 MAC 密钥存储于 HSM (Hardware Security Module) 保护区域，密钥本身不受 SecOC 模块失效影响
3. **NvM 层假设**: Freshness 持久化依赖 NvM 模块，NvM 的失效率及诊断覆盖率由 `SAF-FMEDA-NVM-001` 覆盖
4. **WDT 覆盖**: SecOC_MainFunctionTx/Rx 的调度监控由系统级 WDT 覆盖
5. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
6. **配置正确性**: 假设 SecOC 配置（PDU 数量、Freshness 长度、认证算法、Csm Job ID）在集成阶段经过验证
7. **Freshness 同步**: 假设通信对端使用相同的 Freshness 重建算法（高位拼接），且 Freshness 同步机制（`SECOC_ENABLE_FRESHNESS_SYNC = STD_ON`）正常工作

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| RAM 缓冲区 (TX/RX) | 8 FIT | IEC 61508 Annex D |
| CPU 逻辑（认证数据构建） | 10 FIT | 供应商 FMD |
| Csm MAC 生成/验证 | 15 FIT | Csm FMEDA 估算 |
| NvM Freshness 读写 | 12 FIT | NvM FMEDA (SAF-FMEDA-NVM-001) |
| PduR 接口调用 | 5 FIT | PduR FMEDA 估算 |
| Det 错误上报 | 3 FIT | Det FMEDA 估算 |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| SecOC | Secure Onboard Communication |
| MAC | Message Authentication Code |
| HMAC | Hash-based Message Authentication Code |
| Csm | Crypto Service Manager |
| Freshness | 新鲜度值（防重放攻击计数器） |
| PduR | PDU Router |
| NvM | NVRAM Manager |
| Det | Default Error Tracer |
| HSM | Hardware Security Module |
| ASIL | Automotive Safety Integrity Level |
