# SecOC (Secure Onboard Communication) Design Document

> **Module ID**: 0x96 (150u)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.7.0 (R22-11)  
> **SWS Reference**: AUTOSAR_SWS_SecureOnboardCommunication  
> **Source Path**: `src/bsw/services/secoc/`  
> **Reference Document**: `docs/modules/SECOC.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

SecOC 为车载通信提供认证保护，通过在 PDU 中附加新鲜度值（Freshness Value）和认证信息（Authentication Code / Message Authenticator），防止重放攻击、伪造消息等安全威胁。它位于 PDU Router 与上层 COM 模块之间，对 IF 型和 TP 型 PDU 均提供支持。

主要功能：
- 发送方向：为 PDU 生成新鲜度值与认证码，构建安全 PDU 后通过 PduR 发送
- 接收方向：解析安全 PDU，验证新鲜度值与认证码，仅将验证通过的数据递交给上层
- 支持计数器型和时间戳型新鲜度值
- 提供验证状态覆盖接口，便于调试与诊断
- 通过 Csm 调用 MAC 生成/验证服务

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Secure Onboard Communication | 4.7.0 / R22-11 | 模块软件规范 |
| AUTOSAR Classic Platform | 4.7.x | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Com / Dcm / 其他使用 PDU 的模块 | 通过 PduR 路由 |
| 同层 | PduR | 发送/接收 PDU 路由 |
| 同层 | Csm | MAC 生成与验证 |
| 同层 | SchM | 独占区保护 |
| 公共 | Det | 开发错误检测（可选） |
| 公共 | Dem | 认证失败等运行时错误（当前未直接调用） |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│           Com / Dcm / App           │
├─────────────────────────────────────┤
│            PduR (Services)          │
├─────────────────────────────────────┤
│            SecOC (Services)         │
├─────────────────────────────────────┤
│            PduR / Csm               │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **TX 处理单元**：缓存待发送 PDU，生成新鲜度值与认证码，构建安全 PDU 并调用 `PduR_SecOCTransmit()`。
- **RX 处理单元**：接收安全 PDU，解析原始数据、截断新鲜度值与认证码，重建完整新鲜度值并验证。
- **新鲜度管理器**：维护每个 PDU 的计数器，支持递增、溢出回绕与同步（简化实现）。
- **认证数据构建器**：按 `DataID || Freshness || PDUData` 格式构建待认证数据。
- **状态管理器**：记录每个 RX PDU 的验证状态与结果。

### 3.3 文件结构

```
src/bsw/services/secoc/
├── include/
│   ├── SecOC.h
│   ├── SecOC_Cfg.h
│   ├── SecOC_MemMap.h
│   └── SchM_SecOC.h
└── src/
    └── SecOC.c
```

---

## 4. 状态机

### 4.1 模块生命周期

```
UNINIT -- SecOC_Init() --> INITIALIZED -- SecOC_DeInit() --> UNINIT
```

### 4.2 RX PDU 验证状态

| 状态 | 说明 |
|------|------|
| `SECOC_UNVERIFIED` | 尚未验证 |
| `SECOC_VERIFICATIONSUCCESS_STATUS` | 验证成功 |
| `SECOC_VERIFICATIONFAILURE_STATUS` | 验证失败 |
| `SECOC_VERIFICATIONOVERRIDE` | 验证被外部覆盖 |

### 4.3 验证结果

| 结果 | 说明 |
|------|------|
| `SECOC_VERIFICATIONSUCCESS` | 认证成功 |
| `SECOC_VERIFICATIONFAILURE` | 认证失败 |
| `SECOC_FRESHNESSFAILURE` | 新鲜度验证失败 |
| `SECOC_AUTHENTICATIONBUILDFAILURE` | 认证数据构建失败 |
| `SECOC_NO_VERIFICATION` | 未执行验证 |

---

## 5. 核心数据结构

### 5.1 枚举类型

```c
typedef enum {
    SECOC_VERIFICATIONSUCCESS = 0,
    SECOC_VERIFICATIONFAILURE,
    SECOC_FRESHNESSFAILURE,
    SECOC_AUTHENTICATIONBUILDFAILURE,
    SECOC_NO_VERIFICATION
} SecOC_VerificationResultType;

typedef enum {
    SECOC_UNVERIFIED = 0,
    SECOC_VERIFICATIONSUCCESS_STATUS,
    SECOC_VERIFICATIONFAILURE_STATUS,
    SECOC_VERIFICATIONOVERRIDE
} SecOC_VerificationStatusType;

typedef enum {
    SECOC_AES_MAC = 0,
    SECOC_HMAC_SHA256,
    SECOC_HMAC_SHA512
} SecOC_AuthAlgorithmType;

typedef enum {
    SECOC_COUNTER = 0,
    SECOC_TIMESTAMP
} SecOC_FreshnessValueType;

typedef enum {
    SECOC_IFPDU = 0,
    SECOC_TPPDU
} SecOC_PduType;
```

### 5.2 PDU 配置

```c
typedef struct {
    PduIdType pduId;
    PduIdType lowerLayerPduId;
    SecOC_PduType pduType;
    SecOC_AuthBuildConfigType authConfig;
    SecOC_FreshnessValueConfigType freshnessConfig;
    boolean useCryptographicPdu;
    uint8 dataToAuthOffset;
    uint8 dataToAuthLength;
    uint16 authPduLength;
} SecOC_PduConfigType;
```

### 5.3 模块配置

```c
typedef struct {
    const SecOC_PduConfigType* txPduConfigs;
    uint16 numTxPdus;
    const SecOC_PduConfigType* rxPduConfigs;
    uint16 numRxPdus;
    uint16 mainFunctionPeriodRx;
    uint16 mainFunctionPeriodTx;
    boolean devErrorDetect;
    boolean versionInfoApi;
    boolean overrideStatusAllowed;
} SecOC_ConfigType;
```

### 5.4 运行时状态

```c
typedef struct {
    uint8 data[SECOC_MAX_PDU_LENGTH];
    PduLengthType length;
    boolean inUse;
    PduIdType pduId;
} SecOC_BufferType;

typedef struct {
    uint32 freshnessValue;
    uint32 lastVerifiedFreshness;
    SecOC_VerificationStatusType status;
    SecOC_VerificationResultType lastResult;
    uint8 retryCount;
    boolean authInProgress;
    uint16 timeoutCounter;
} SecOC_RxPduStateType;

typedef struct {
    uint32 freshnessValue;
    boolean txInProgress;
} SecOC_TxPduStateType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | SWS 需求 | 备注 |
|-----|------|------|----------|------|
| `SecOC_Init` | `void SecOC_Init(const SecOC_ConfigType* configPtr)` | 初始化模块 | SWS_SecOC_00001 | |
| `SecOC_DeInit` | `void SecOC_DeInit(void)` | 反初始化模块 | SWS_SecOC_00002 | |
| `SecOC_GetVersionInfo` | `void SecOC_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 版本信息 | SWS_SecOC_00040 | 受 `SECOC_VERSION_INFO_API` 控制 |
| `SecOC_IfTransmit` | `Std_ReturnType SecOC_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)` | IF 型安全发送 | SWS_SecOC_00010 | 缓存数据，由 MainFunctionTx 处理 |
| `SecOC_IfRxIndication` | `void SecOC_IfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | IF 型接收指示 | SWS_SecOC_00011 | 缓存安全 PDU |
| `SecOC_VerifyStatusOverride` | `Std_ReturnType SecOC_VerifyStatusOverride(PduIdType PduId, SecOC_VerificationStatusType status)` | 覆盖验证状态 | SWS_SecOC_00012 | 受 `overrideStatusAllowed` 控制 |
| `SecOC_GetVerificationStatus` | `SecOC_VerificationStatusType SecOC_GetVerificationStatus(PduIdType PduId)` | 获取验证状态 | SWS_SecOC_00013 | |
| `SecOC_GetVerificationResult` | `Std_ReturnType SecOC_GetVerificationResult(PduIdType PduId, SecOC_VerificationResultType* resultPtr)` | 获取验证结果 | SWS_SecOC_00014 | |
| `SecOC_CopyTxData` | `BufReq_ReturnType SecOC_CopyTxData(...)` | TP 型发送数据拷贝 | SWS_SecOC_00030 | 当前声明未实现 |
| `SecOC_CopyRxData` | `BufReq_ReturnType SecOC_CopyRxData(...)` | TP 型接收数据拷贝 | SWS_SecOC_00031 | 当前声明未实现 |
| `SecOC_StartOfReception` | `BufReq_ReturnType SecOC_StartOfReception(...)` | TP 型开始接收 | SWS_SecOC_00032 | 当前声明未实现 |
| `SecOC_TxConfirmation` | `void SecOC_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)` | 发送确认 | SWS_SecOC_00033 | 当前声明未实现 |
| `SecOC_MainFunctionRx` | `void SecOC_MainFunctionRx(void)` | RX 周期处理 | SWS_SecOC_00021 | 处理待验证 PDU |
| `SecOC_MainFunctionTx` | `void SecOC_MainFunctionTx(void)` | TX 周期处理 | SWS_SecOC_00020 | 生成安全 PDU 并发送 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| `PduR_SecOCTransmit` | 向 PduR 发送安全 PDU |
| `PduR_SecOCRxIndication` | 将验证通过的 PDU 递交给上层 |
| `Csm_MacGenerate` | 生成 MAC（通过 Csm） |
| `Csm_MacVerify` | 验证 MAC（通过 Csm） |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x01 | Init | `SECOC_E_ALREADY_INITIALIZED`, `SECOC_E_PARAM_POINTER` |
| 0x02 | DeInit | `SECOC_E_UNINIT` |
| 0x03 | GetVersionInfo | `SECOC_E_PARAM_POINTER` |
| 0x41 | IfTransmit | `SECOC_E_UNINIT`, `SECOC_E_PARAM_POINTER`, `SECOC_E_INVALID_PDU_SDU_ID` |
| 0x43 | IfRxIndication | `SECOC_E_UNINIT` |
| 0x81 | VerifyStatusOverride | `SECOC_E_INVALID_PDU_SDU_ID` |
| 0x91 | MainFunctionRx | - |
| 0x92 | MainFunctionTx | - |

---

## 7. 处理流程

### 7.1 发送流程（SecOC_IfTransmit + MainFunctionTx）

1. `SecOC_IfTransmit()` 检查初始化状态与 PDU ID，将原始 PDU 拷贝到 `SecOC_TxBuffers[idx]`。
2. `SecOC_MainFunctionTx()` 遍历待发送缓冲区。
3. `SecOC_ProcessTxPdu()`：
   - 递增 `freshnessValue`（溢出后回绕到 0）。
   - 构建认证数据：`DataID || Freshness || PDUData`。
   - 调用 `Csm_MacGenerate()` 生成认证码。
   - 构建安全 PDU：`[原始数据][截断新鲜度值][认证码]`。
   - 调用 `PduR_SecOCTransmit()` 发送。

### 7.2 接收流程（SecOC_IfRxIndication + MainFunctionRx）

1. `SecOC_IfRxIndication()` 缓存安全 PDU 到 `SecOC_RxBuffers[idx]`（当前代码中该函数声明存在，但实现未在提供的源文件中完整展示）。
2. `SecOC_MainFunctionRx()` 遍历待验证缓冲区。
3. `SecOC_ProcessRxPdu()`：
   - 检查 PDU 长度是否包含新鲜度和认证码。
   - 从安全 PDU 中提取截断新鲜度值。
   - 利用 `lastVerifiedFreshness` 高比特重建完整新鲜度值。
   - 提取接收到的认证码。
   - 构建认证数据并调用 `Csm_MacVerify()`。
   - 验证通过：更新状态、记录新鲜度、调用 `PduR_SecOCRxIndication()` 递交原始数据。
   - 验证失败：增加重试计数，超过阈值后报告 DEM（当前为注释）。

### 7.3 新鲜度值管理

- 使用 32 位计数器，每次发送或成功验证后递增。
- 当计数器达到 `SECOC_FRESHNESS_RESET_THRESHOLD`（默认 0xF0000000）时回绕到 0。
- 接收端通过保留上一次验证值的高位来重建截断计数器。

---

## 8. 配置设计

### 8.1 预编译配置（`SecOC_Cfg.h`）

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `SECOC_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `SECOC_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `SECOC_NUM_TX_PDUS` | 4U | TX PDU 数量 |
| `SECOC_NUM_RX_PDUS` | 4U | RX PDU 数量 |
| `SECOC_MAX_PDU_LENGTH` | 256U | 最大 PDU 长度 |
| `SECOC_AUTH_ALGORITHM` | `SECOC_HMAC_SHA256` | 认证算法 |
| `SECOC_AUTH_INFO_LENGTH` | 16U | 认证信息长度（字节） |
| `SECOC_FRESHNESS_VALUE_TYPE` | `SECOC_COUNTER` | 新鲜度值类型 |
| `SECOC_FRESHNESS_VALUE_LENGTH` | 32U | 完整新鲜度值长度（比特） |
| `SECOC_FRESHNESS_VALUE_TX_LENGTH` | 16U | 传输的新鲜度值长度（比特） |
| `SECOC_FRESHNESS_RESET_THRESHOLD` | 4026531840U | 计数器回绕阈值 |
| `SECOC_VERIFICATION_RETRY_COUNT` | 3U | 验证失败重试次数 |
| `SECOC_VERIFICATION_TIMEOUT_MS` | 100U | 验证超时（ms） |
| `SECOC_ENABLE_FRESHNESS_SYNC` | STD_ON | 新鲜度同步使能 |
| `SECOC_OVERRIDE_STATUS_ALLOWED` | STD_ON | 允许覆盖验证状态 |
| `SECOC_CSM_JOB_ID_AUTH` | `CSM_JOB_ID_MAC_GENERATE_1` | Csm MAC 生成作业 ID |
| `SECOC_CSM_JOB_ID_VERIFY` | `CSM_JOB_ID_MAC_VERIFY_1` | Csm MAC 验证作业 ID |
| `SECOC_MAX_CRYPTO_OPERATIONS` | 4U | 最大并发加密操作数 |

### 8.2 链接时配置

`SecOC_Config` 中定义 TX/RX PDU 配置数组：

- `txPduConfigs[]`：发送 PDU 配置
- `rxPduConfigs[]`：接收 PDU 配置
- 每个 PDU 配置包含算法、新鲜度、DataID、长度等参数

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x01 | `SECOC_E_PARAM_POINTER` | 空指针入参 |
| 0x02 | `SECOC_E_INVALID_PDU_SDU_ID` | 非法 PDU ID |
| 0x03 | `SECOC_E_INVALID_PARAMETER` | 非法参数 |
| 0x04 | `SECOC_E_UNINIT` | 未初始化调用 API |
| 0x05 | `SECOC_E_ALREADY_INITIALIZED` | 重复初始化 |
| 0x06 | `SECOC_E_CRYPTO_FAILURE` | 加密操作失败 |

### 9.2 运行时错误

| 错误码 | 名称 | 说明 |
|--------|------|------|
| 0x01 | `SECOC_E_CRYPTO_AUTH_FAILED` | 认证失败 |
| 0x02 | `SECOC_E_FRESHNESS_FAILURE` | 新鲜度失败 |
| 0x03 | `SECOC_E_SEC_PAYLOAD_ERROR` | 安全载荷错误 |
| 0x04 | `SECOC_E_BUSY` | 模块忙碌 |

### 9.3 安全机制

- 每个 PDU 拥有独立的新鲜度计数器，防止重放攻击。
- 认证数据包含 DataID，防止跨 PDU 认证码复用。
- 验证失败不递交数据，避免上层处理被篡改消息。
- SchM 独占区保护关键数据结构。

---

## 10. 内存与性能

### 10.1 MemMap 分区

当前实现使用 `SecOC_MemMap.h` 分区：

| 分区 | 用途 |
|------|------|
| `SECOC_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化全局变量 |
| `SECOC_START_SEC_CODE` | 代码段 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~(NUM_TX+NUM_RX) * (MAX_PDU_LENGTH + 32) | TX/RX 缓冲区与状态 |
| ROM | ~10 KB | 代码与配置表 |
| 周期 | 中等 | 每个 PDU 需调用 Csm MAC 操作 |

---

## 11. 集成指南

- **PduR 集成**：确保 PduR 正确路由 SecOC 的 TX/RX PDU，并实现 `PduR_SecOCTransmit()` 与 `PduR_SecOCRxIndication()`。
- **Csm 集成**：配置对应的 MAC 生成/验证作业 ID，确保密钥已设置。
- **SchM 集成**：SecOC 使用 `SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0()` 保护初始化与反初始化。
- **TP 支持**：当前 TP 相关回调（`CopyTxData`、`CopyRxData`、`StartOfReception`）仅声明未实现，需补充完整。
- **新鲜度管理器**：当前为简化计数器实现，生产环境建议对接独立 Freshness Manager。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `tests/unit/autosar/services/SecOC_Test.c` | 初始化、发送/接收、认证验证、新鲜度管理、状态覆盖 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 正常发送 | 原始 PDU → 安全 PDU → PduR |
| 正常接收 | 安全 PDU → 验证通过 → 上层接收 |
| 认证失败 | 篡改认证码 → 验证失败 → 不递交上层 |
| 重放攻击 | 使用旧新鲜度值 → 验证失败 |
| 状态覆盖 | 调用 `VerifyStatusOverride` 后强制通过 |

---

## 13. 实现说明 / TODO

- `SecOC_IfRxIndication()` 的完整实现未在提供源文件中展示，需补充。
- TP 相关回调（`CopyTxData`、`CopyRxData`、`StartOfReception`、`TxConfirmation`）当前仅声明未实现。
- `SecOC_MainFunctionTx()` 与 `SecOC_MainFunctionRx()` 在提供的源文件中未完整展示实现，需确保遍历所有 `inUse` 缓冲区。
- DEM 报告当前为注释，需根据项目需求启用。
- 新鲜度同步主计数器 `SecOC_SyncMasterFreshness` 当前仅定义未使用。
- 时间戳型新鲜度当前未实现，仅支持计数器型。

---

## 14. 参考资料

1. AUTOSAR_SWS_SecureOnboardCommunication.pdf
2. `docs/modules/SECOC.md`
3. `src/bsw/services/secoc/SecOC.h`
4. `src/bsw/services/secoc/SecOC.c`
5. `src/bsw/services/secoc/SecOC_Cfg.h`

## 需求追溯表

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_SecOC | — | SECOC 模块级需求归属 |
