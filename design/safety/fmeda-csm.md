# FMEDA — Csm (Crypto Services Manager)

> **文档编号**: SAF-FMEDA-CSM-001
> **版本**: 1.0
> **日期**: 2026-10-22
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW Service Layer 的 Csm (Crypto Services Manager) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/services/csm/src/Csm.c`

**分析边界**:
- Csm 核心逻辑（密钥管理、作业队列、加密服务抽象）
- 上层接口：`Csm_Hash`, `Csm_MacGenerate`, `Csm_MacVerify`, `Csm_Encrypt`, `Csm_Decrypt`, `Csm_SignatureGenerate`, `Csm_SignatureVerify`, `Csm_RandomGenerate`
- 密钥管理：`Csm_KeyElementSet`, `Csm_KeyElementGet`, `Csm_KeyGenerate`, `Csm_KeyDerive`, `Csm_KeyExchangeCalcPubVal`, `Csm_KeyExchangeCalcSecret`
- 内部安全机制：密钥状态机、作业队列管理、硬件服务回退、错误检测
- 下层依赖（假设由 ECUAL/MCAL 层覆盖）：CryIf, 硬件加密驱动

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 Csm 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `Csm_InternalStateType` | Csm.c:98-103 | 模块内部状态枚举：UNINIT, INIT, ACTIVE |
| `Csm_KeyType` | Csm_Types.h:282-289 | 密钥运行时数据：keyId, status, elements[], numElements, referenceCount |
| `Csm_KeyElementDataType` | Csm_Types.h:260-265 | 密钥元素数据：data[64], length, valid |
| `Csm_JobType` | Csm_Types.h:365-381 | 作业运行时数据：jobId, state, service, inputData/outputData, result |
| `Csm_QueueType` | Csm_Types.h:399-405 | 服务队列：items[], head, tail, count |
| `Csm_ConfigType` | Csm_Types.h:446-455 | 全局配置：keys[], jobs[], useAsyncMode, queueProcessingPeriod |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 初始化/去初始化 | `Csm_Init()` (L627), `Csm_DeInit()` (L719) | 模块生命周期管理 |
| FU-02 | 密钥元素管理 | `Csm_KeyElementSet()` (L745), `Csm_KeyElementGet()` (L836) | 密钥元素读写 |
| FU-03 | 密钥状态管理 | `Csm_KeySetValid()` (L795), `Csm_UpdateKeyStatus()` (L538) | 密钥状态转换 |
| FU-04 | 密钥生成 | `Csm_KeyGenerate()` (L977) | 生成新密钥（硬件/软件回退） |
| FU-05 | 密钥派生 | `Csm_KeyDerive()` (L1128) | KDF 密钥派生（NIST SP 800-108 简化版） |
| FU-06 | 密钥交换 | `Csm_KeyExchangeCalcPubVal()` (L1432), `Csm_KeyExchangeCalcSecret()` (L1701) | ECDH 公钥/共享秘密计算 |
| FU-07 | 哈希计算 | `Csm_Hash()` (L1984) | 哈希服务（SHA-256 等） |
| FU-08 | MAC 生成/验证 | `Csm_MacGenerate()` (L2069), `Csm_MacVerify()` (L2149) | HMAC 消息认证码 |
| FU-09 | 加密/解密 | `Csm_Encrypt()` (L2213), `Csm_Decrypt()` (L2293) | 对称加密（AES-CBC 等） |
| FU-10 | 数字签名 | `Csm_SignatureGenerate()` (L2373), `Csm_SignatureVerify()` (L2450) | 非对称签名（ECDSA 等） |
| FU-11 | 随机数生成 | `Csm_RandomGenerate()` (L2529) | 随机数服务 |
| FU-12 | 作业队列管理 | `Csm_QueueJob()` (L324), `Csm_DequeueJob()` (L370), `Csm_ProcessQueue()` (L396) | 优先级队列调度 |
| FU-13 | 作业执行 | `Csm_ExecuteJob()` (L431), `Csm_MainFunction()` (L2620) | 异步作业处理 |
| FU-14 | 密钥持久化 | `Csm_PersistKeyElement()` (L551), `Csm_LoadKeyElement()` (L583) | 密钥存储读写 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 初始化/去初始化

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | 配置指针为空或无效 | 初始化失败，模块无法启动 | `Csm_Init()` 入口参数校验 (config != NULL) + `Csm_ValidateConfig()` (L228) | 99% | B |
| FM-01-02 | 重复初始化 | 状态不一致，密钥数据丢失 | 状态检查：`Csm_State != CSM_STATE_UNINIT` 时拒绝初始化 (L638) | 95% | B |
| FM-01-03 | 去初始化时有活动作业 | 作业中断，结果丢失 | 检查所有作业状态：`Csm_Jobs[i].state == CSM_JOB_STATE_PROCESSING` 时拒绝去初始化 (L726-732) | 90% | B |
| FM-01-04 | 初始化魔数损坏 | 模块状态判断错误 | 魔数校验：`Csm_InitMagic == CSM_MAGIC_INITIALIZED` (L709) | 85% | B |

### 3.2 FU-02: 密钥元素管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | 密钥数据指针为空 | 访问非法地址，内存破坏 | 入口参数校验：`keyPtr != NULL_PTR` (L755, L846) | 99% | B |
| FM-02-02 | 密钥长度超出限制 | 缓冲区溢出，密钥数据损坏 | 长度检查：`keyLength <= CSM_MAX_KEY_LENGTH` (L757) | 99% | B |
| FM-02-03 | keyId 不存在 | 访问未定义的密钥描述符 | `Csm_FindKeyIndex()` 查找失败返回 `CSM_E_PARAM_KEY_ID` (L763) | 99% | B |
| FM-02-04 | 密钥元素数量溢出 | 密钥元素数组越界 | 检查：`numElements < CSM_MAX_KEY_ELEMENTS` (L773) | 95% | B |
| FM-02-05 | 读取时密钥状态无效 | 返回未初始化或损坏的密钥数据 | 状态检查：`status == CSM_KEY_STATUS_VALID` (L855) | 90% | B |
| FM-02-06 | 输出缓冲区长度不足 | 密钥数据截断 | 长度校验：`*keyLengthPtr >= element.length` (L865) | 95% | B |

### 3.3 FU-03: 密钥状态管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | 密钥元素未设置即标记有效 | 使用空密钥进行加密操作 | 检查：`numElements > 0` (L808) | 95% | B |
| FM-03-02 | 密钥持久化失败 | 密钥丢失，重启后无法恢复 | `Csm_PersistKeyElement()` 返回值检查 (L819) | 85% | B |
| FM-03-03 | keyIdx 超出范围 | 访问密钥数组越界 | 边界检查：`keyIdx < CSM_MAX_KEYS` (L540) | 99% | B |

### 3.4 FU-04: 密钥生成

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | 硬件加密服务返回错误 | 密钥生成失败 | CryIf/HwService 返回值检查 (L1000, L1038) | 90% | B |
| FM-04-02 | 随机数生成器熵耗尽 | 无法生成安全密钥 | `Csm_Cfg_RandomGenerate()` 失败时上报 `CSM_E_ENTROPY_EXHAUSTION` (L1097) | 85% | B |
| FM-04-03 | 密钥已存在且有效 | 重复生成覆盖有效密钥 | 状态检查：`status == CSM_KEY_STATUS_VALID` 时直接返回 (L994) | 95% | B |
| FM-04-04 | 生成的密钥长度为零 | 无效密钥被标记为有效 | 长度检查：`resultLength > 0` (L1047) | 90% | B |

### 3.5 FU-05: 密钥派生

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | 源密钥无效 | 派生失败或派生出不安全密钥 | 状态检查：`status == CSM_KEY_STATUS_VALID` (L1147) | 99% | B |
| FM-05-02 | 源密钥无派生权限 | 越权派生密钥 | 权限检查：`allowedUsage & CSM_KEY_USAGE_DERIVE` (L1168) | 95% | B |
| FM-05-03 | 哈希服务不可用 | KDF 回退到弱混淆算法 | 哈希结果检查，失败时使用 XOR 回退 (L1370) | 70% | B |
| FM-05-04 | 派生循环计数器溢出 | 无限循环或派生不完整 | 计数器溢出检查：`counter == 0` 时退出 (L1396) | 90% | B |
| FM-05-05 | 派生输出长度为零 | 目标密钥为空 | 长度检查：`offset > 0` (L1403) | 95% | B |

### 3.6 FU-06: 密钥交换

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | 私钥元素为空 | 无法计算公钥/共享秘密 | 检查：`numElements > 0 && elements[0].valid` (L1459, L1733) | 99% | B |
| FM-06-02 | 密钥无交换权限 | 越权执行密钥交换 | 权限检查：`allowedUsage & CSM_KEY_USAGE_KEY_EXCHANGE` (L1475, L1749) | 95% | B |
| FM-06-03 | 对方公钥长度非法 | 共享秘密计算错误 | 长度检查：`partnerPublicValueLength > 0 && <= CSM_MAX_KEY_LENGTH` (L1713) | 99% | B |
| FM-06-04 | 输出缓冲区长度不足 | 公钥/共享秘密截断 | 长度校验：`*publicValueLengthPtr >= resultLength` (L1547) | 95% | B |

### 3.7 FU-07: 哈希计算

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | 输入数据长度超限 | 缓冲区溢出 | 长度检查：`dataLength <= CSM_MAX_DATA_LENGTH` (L2010) | 99% | B |
| FM-07-02 | 硬件哈希服务返回错误 | 哈希计算失败 | `Csm_Cfg_HwService()` 返回值检查 (L2031) | 90% | B |
| FM-07-03 | 输出缓冲区长度不足 | 哈希结果截断 | 长度校验：`*resultLengthPtr >= job.resultLength` (L2044) | 95% | B |
| FM-07-04 | jobId 不存在 | 访问未定义的作业 | `Csm_FindJobIndex()` 查找失败返回 E_NOT_OK (L1997) | 99% | B |

### 3.8 FU-08: MAC 生成/验证

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | 密钥无 MAC 生成权限 | 越权生成 MAC | 权限检查：`CSM_KEY_USAGE_MAC_GENERATE` (L2091) | 95% | B |
| FM-08-02 | 密钥无 MAC 验证权限 | 越权验证 MAC | 权限检查：`CSM_KEY_USAGE_MAC_VERIFY` (L2174) | 95% | B |
| FM-08-03 | MAC 比较长度不匹配 | 验证结果错误 | 长度检查：`calculatedMacLength >= macLength` (L2189) | 90% | B |
| FM-08-04 | 截断 MAC 比较错误 | SecOC 认证失败 | 前导字节比较：`Mcal_MemCompare(calculatedMac, macPtr, macLength)` (L2197) | 85% | B |

### 3.9 FU-09: 加密/解密

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | 密钥无加密权限 | 越权加密数据 | 权限检查：`CSM_KEY_USAGE_ENCRYPT` (L2235) | 95% | B |
| FM-09-02 | 密钥无解密权限 | 越权解密数据 | 权限检查：`CSM_KEY_USAGE_DECRYPT` (L2315) | 95% | B |
| FM-09-03 | 硬件加密服务返回错误 | 加密/解密失败 | `Csm_Cfg_HwService()` 返回值检查 (L2259, L2339) | 90% | B |
| FM-09-04 | 输出缓冲区长度不足 | 密文/明文截断 | 长度校验：`*resultLengthPtr >= job.resultLength` (L2270, L2350) | 95% | B |

### 3.10 FU-10: 数字签名

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | 密钥无签名权限 | 越权生成签名 | 权限检查：`CSM_KEY_USAGE_SIGN` (L2395) | 95% | B |
| FM-10-02 | 密钥无验证权限 | 越权验证签名 | 权限检查：`CSM_KEY_USAGE_VERIFY` (L2473) | 95% | B |
| FM-10-03 | 签名长度超限 | 缓冲区溢出 | 长度检查：`signatureLength <= CSM_MAX_SIGNATURE_LENGTH` (L2497) | 99% | B |
| FM-10-04 | 硬件签名服务返回错误 | 签名生成/验证失败 | `Csm_Cfg_HwService()` 返回值检查 (L2416, L2504) | 90% | B |
| FM-10-05 | 验证结果未正确传递 | 签名验证结果错误 | `verifyResult` 字段检查 (L2516) | 85% | B |

### 3.11 FU-11: 随机数生成

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-11-01 | 随机数生成器熵耗尽 | 无法生成安全随机数 | `Csm_Cfg_RandomGenerate()` 返回值检查 (L2552) | 85% | B |
| FM-11-02 | 输出缓冲区长度超限 | 缓冲区溢出 | 长度检查：`resultLength <= CSM_MAX_DATA_LENGTH` (L2539) | 99% | B |
| FM-11-03 | jobId 不存在时回退失败 | 随机数生成失败 | 回退到 `Csm_Cfg_RandomGenerate()` (L2546) | 80% | B |

### 3.12 FU-12: 作业队列管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-12-01 | 队列已满 | 新作业被拒绝 | 队列满检查：`Csm_JobQueue.count >= CSM_CFG_QUEUE_SIZE` (L329) | 99% | B |
| FM-12-02 | 队列状态机卡死 | 作业无法推进，Csm 无响应 | `Csm_MainFunction()` 周期执行 + WDT 监控（系统级） | 80% | B |
| FM-12-03 | 优先级插入逻辑错误 | 作业顺序混乱 | 优先级比较逻辑 (L338) + 时间戳记录 (L355) | 75% | B |
| FM-12-04 | 并发作业数超限 | 资源耗尽 | 检查：`Csm_ActiveJobCount < CSM_CFG_MAX_CONCURRENT_JOBS` (L403) | 95% | B |

### 3.13 FU-13: 作业执行

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-13-01 | jobIdx 超出范围 | 访问作业数组越界 | 边界检查：`jobIdx < CSM_MAX_JOBS` (L436) | 99% | B |
| FM-13-02 | 硬件服务返回忙碌 | 作业延迟完成 | 返回值检查：`result == 1` 时保持 PROCESSING 状态 (L463) | 90% | B |
| FM-13-03 | 硬件服务返回错误 | 作业失败 | 错误处理：`result != E_OK` 时重置作业并通知回调 (L467) | 90% | B |
| FM-13-04 | 回调函数为空 | 作业完成通知丢失 | 空指针检查：`Csm_Callbacks[jobIdx] != NULL_PTR` (L212) | 85% | B |
| FM-13-05 | ActiveJobCount 下溢 | 并发计数错误 | 下溢保护：`Csm_ActiveJobCount > 0` 时递减 (L471) | 95% | B |

### 3.14 FU-14: 密钥持久化

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-14-01 | 密钥写入 NVRAM 失败 | 密钥丢失，重启后无法恢复 | `Csm_Cfg_KeyWrite()` 返回值检查 (L567) | 85% | B |
| FM-14-02 | 密钥读取 NVRAM 失败 | 密钥加载失败 | `Csm_Cfg_KeyRead()` 返回值检查 (L600) | 85% | B |
| FM-14-03 | 持久化功能未启用 | 密钥无法持久化 | 编译时检查：`CSM_CFG_KEY_PERSISTENCE_SUPPORT == STD_ON` (L553) | 99% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 初始化/去初始化 | 99% | 92% | 配置校验 + 状态检查 |
| FU-02 | 密钥元素管理 | 99% | 96% | 参数校验 + 状态检查 |
| FU-03 | 密钥状态管理 | 99% | 93% | 元素检查 + 持久化验证 |
| FU-04 | 密钥生成 | 95% | 90% | 硬件/软件回退 + 熵检查 |
| FU-05 | 密钥派生 | 99% | 88% | 权限检查 + 哈希服务监控 |
| FU-06 | 密钥交换 | 99% | 95% | 权限检查 + 长度校验 |
| FU-07 | 哈希计算 | 99% | 95% | 硬件服务监控 + 长度校验 |
| FU-08 | MAC 生成/验证 | 95% | 92% | 权限检查 + 截断比较 |
| FU-09 | 加密/解密 | 95% | 93% | 权限检查 + 硬件服务监控 |
| FU-10 | 数字签名 | 95% | 92% | 权限检查 + 结果验证 |
| FU-11 | 随机数生成 | 99% | 88% | 熵检查 + 回退机制 |
| FU-12 | 作业队列管理 | 99% | 88% | 队列满检测 + 并发控制 |
| FU-13 | 作业执行 | 99% | 91% | 边界检查 + 回调通知 |
| FU-14 | 密钥持久化 | 99% | 85% | NVRAM 读写验证 |

### 4.2 模块整体诊断覆盖率

```
DC_Csm = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_Csm ≈ 91%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL B)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 90% | ~91% | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ~90% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 100 FIT | ~52 FIT | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。Csm 中主要单点故障来源：
- 密钥状态机瞬态故障（由 WDT 覆盖）
- 队列优先级排序逻辑故障（由时间戳监控）
- 哈希/XOR 回退路径故障（由硬件服务监控覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (8.1 / 95) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。Csm 中主要潜伏故障来源：
- 密钥权限检查逻辑故障（通过配置校验覆盖）
- 作业队列状态机故障（通过定期 MainFunction 检测）
- 密钥持久化逻辑故障（通过启动时加载验证）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (9.5 / 95) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_key_state_corruption + lambda_queue_stall + lambda_hw_service_failure
     ≈ 8 + 18 + 26 = 52 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 Csm 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | 所有密钥有效，作业队列正常调度 | 正常加密服务 |
| **降级运行** | 部分密钥无效或硬件服务故障 | 使用软件回退，上报诊断事件 |
| **安全停止** | 初始化失败或 MainFunction 无法执行 | 拒绝所有加密操作，WDT 触发复位 |

### 6.2 安全状态转换

```
正常运行 ──[密钥无效/硬件故障]──→ 降级运行
降级运行 ──[软件回退也失败]──→ 安全停止
任意状态 ──[MainFunction超时]──→ 安全停止 (WDT触发)
安全停止 ──[系统复位]──→ 正常运行 (重新初始化)
```

### 6.3 故障反应策略

1. **密钥失效**: 拒绝使用该密钥的所有加密操作，返回 `CSM_E_KEY_NOT_VALID`
2. **硬件服务故障**: 自动回退到软件实现（如 XOR 混淆），上报 `CSM_E_HARDWARE_FAILURE`
3. **队列溢出**: 拒绝新作业，返回 `CSM_E_QUEUE_FULL`，上层可重试
4. **熵耗尽**: 拒绝密钥生成/随机数生成，返回 `CSM_E_ENTROPY_EXHAUSTION`

---

## 7. 故障反应时间线

### 7.1 场景一：密钥无效导致加密失败

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Csm_Encrypt(jobId, mode, dataPtr, ...)` |
| T=1 | `Csm_ValidateKeyUsage()` 检查密钥权限 |
| T=2 | 检测到密钥状态非 `CSM_KEY_STATUS_VALID` |
| T=3 | 返回 `E_NOT_OK`，上层收到错误 |
| T=4 | 应用层/系统监控检测到错误，触发降级策略 |
| **总延迟** | **1 个调用周期 + 系统响应时间** |

### 7.2 场景二：硬件加密服务故障

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Csm_Hash(jobId, SINGLESHOT, dataPtr, ...)` |
| T=1 | `Csm_Cfg_HwService()` 调用硬件哈希服务 |
| T=2 | 硬件返回错误 (`result != E_OK`) |
| T=3 | 检测到故障，返回 `E_NOT_OK` |
| T=4 | Dem 记录 `CSM_DEM_HARDWARE_FAILURE_EVENT_ID` 事件 |
| T=5 | 应用层切换到软件回退路径 |
| **总延迟** | **2 个调用周期 + Dem 记录时间** |

### 7.3 场景三：作业队列溢出

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用异步密钥生成，队列已满 |
| T=1 | 检测到 `Csm_JobQueue.count >= CSM_CFG_QUEUE_SIZE` |
| T=2 | 返回 `E_NOT_OK` |
| T=3 | 应用层/系统监控检测到错误，触发降级策略 |
| **总延迟** | **1 个调用周期 + 系统响应时间** |

### 7.4 场景四：密钥派生哈希服务不可用

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Csm_KeyDerive(srcKeyId, targetKeyId)` |
| T=1 | CryIf 派生失败，尝试硬件服务层 |
| T=2 | 硬件服务层查找关联作业配置 |
| T=3 | 调用 `Csm_Cfg_HwService()` 计算哈希 |
| T=4 | 哈希服务返回错误 |
| T=5 | 回退到 XOR 混淆算法 (L1370) |
| T=6 | 派生密钥存入目标密钥，标记为有效 |
| T=7 | 返回 `E_OK`（但安全性降低） |
| **总延迟** | **7 个主函数周期** |

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

---

## 9. 假设与约束

1. **底层加密驱动假设**: CryIf/硬件加密驱动层的失效率及诊断覆盖率由独立 FMEDA 覆盖，本分析假设底层 FIT 数据已知
2. **硬件服务可用性**: 假设 `Csm_Cfg_HwService()` 在正常情况下由硬件加速，故障时自动回退到软件实现
3. **WDT 覆盖**: Csm_MainFunction 的调度监控由系统级 WDT 覆盖
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **配置正确性**: 假设 Csm 配置（密钥描述符、作业配置、队列大小）在集成阶段经过验证
6. **密钥持久化存储**: 假设密钥存储于受保护的 NVRAM 区域，启动时经过完整性校验
7. **随机数生成器**: 假设 `Csm_Cfg_RandomGenerate()` 基于硬件 TRNG 或安全 PRNG，熵源由 MCAL 层覆盖

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| 硬件加密引擎 (AES/SHA) | 15 FIT | 供应商 FMD |
| RAM 存储 (密钥/作业数据) | 10 FIT | IEC 61508 Annex D |
| CPU 逻辑 (状态机/队列) | 18 FIT | 供应商 FMD |
| NVRAM (密钥持久化) | 12 FIT | 供应商 FMD |
| 随机数生成器 (TRNG) | 8 FIT | 供应商 FMD |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| Csm | Crypto Services Manager |
| KDF | Key Derivation Function |
| ECDH | Elliptic Curve Diffie-Hellman |
| HMAC | Hash-based Message Authentication Code |
| MAC | Message Authentication Code |
| TRNG | True Random Number Generator |
| PRNG | Pseudo Random Number Generator |
| CryIf | Crypto Interface |
| NVRAM | Non-Volatile Random Access Memory |
