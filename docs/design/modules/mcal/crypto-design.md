# Crypto Design Document

> **Module ID**: 0x1E
> **AUTOSAR Layer**: MCAL
> **AUTOSAR Version**: Classic Platform 4.4.0
> **SWS Reference**: AUTOSAR_SWS_Crypto
> **Source Path**: `src/bsw/mcal/crypto/`
> **Reference Document**: `docs/modules/Crypto.md`
> **Doc Version**: 1.0
> **Status**: Draft

---

## 1. 模块概述

Crypto（Crypto Driver）位于 MCAL 层，为上层（CSM / CryIf / 应用）提供密码学服务。实现支持软件算法库 Mbed TLS 与硬件安全模块 HSM 双后端，覆盖 CCC Digital Key 所需的 ECDSA、ECDH、AES-GCM、SHA-256、HKDF、HMAC、随机数生成等算法，并提供密钥管理接口与 BLAKE2 哈希扩展。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Crypto | 4.4.0 | Crypto 软件规范 |
| AUTOSAR Classic Platform | 4.x | 经典平台 |
| CCC Digital Key | - | 数字车钥匙合规算法 |

### 2.2 模块依赖

| 依赖方向 | 模块/库 | 说明 |
|----------|---------|------|
| 上层 | Csm / CryIf / Application | 密码学服务调用 |
| 下层 | Mbed TLS | 软件算法实现 |
| 下层 | HSM Driver（可选） | 硬件加速与安全密钥存储 |
| 公共 | Det | 开发错误检测 |
| 公共 | blake2 | BLAKE2 哈希支持 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│    Csm / CryIf / Application / CCC  │
├─────────────────────────────────────┤
│         Crypto (MCAL)               │
├─────────────────────────────────────┤
│   Mbed TLS / HSM / TRNG Hardware    │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Job Manager**：`Crypto_ProcessJob`、`Crypto_MainFunction` 管理同步/异步作业队列。
- **Key Manager**：`Crypto_KeyElementSet/Get`、`Crypto_KeyValidSet`、`Crypto_KeyGenerate` 等。
- **Software Backend**：通过 `Crypto_MbedTLS_xxx` 调用 Mbed TLS。
- **HSM Backend**：通过 `Crypto_Hsm_xxx` 调用硬件安全模块（可选）。
- **CCC Extension**：数字车钥匙专用的加解密、签名、证书验证、会话密钥派生。
- **BLAKE2 Extension**：增量式 BLAKE2b/BLAKE2s 哈希接口。

### 3.3 文件结构

```
src/bsw/mcal/crypto/
├── include/
│   ├── Crypto.h
│   ├── Crypto_Cfg.h
│   ├── Crypto_Types.h
│   ├── Crypto_MemMap.h
│   ├── Crypto_HwTrng.h
│   ├── Crypto_MbedTLS_Mem.h
│   ├── Crypto_S32K312_Hsm.h
│   └── SchM_Crypto.h
├── src/
│   ├── Crypto.c
│   ├── Crypto_Cfg.c
│   ├── Crypto_Aes.c
│   ├── Crypto_Hsm.c
│   ├── Crypto_HwTrng.c
│   ├── Crypto_MbedTLS.c
│   ├── Crypto_MbedTLS_Mem.c
│   └── Crypto_S32K312_Hsm.c
└── legacy/
    └── _crypto_hsm_*.c
```

---

## 4. 状态机

### 4.1 驱动状态

```
UNINIT -- Crypto_Init() --> INIT
INIT -- Crypto_DeInit() --> UNINIT
INIT -- Crypto_ProcessJob() --> BUSY/IDLE
```

### 4.2 作业状态

```
IDLE -- ProcessJob(sync)  --> PROCESSING --> IDLE
IDLE -- ProcessJob(async) --> QUEUED --> PROCESSING --> IDLE
QUEUED/PROCESSING -- CancelJob --> CANCELED
```

---

## 5. 核心数据结构

```c
typedef uint32 Crypto_JobIdType;
typedef uint32 Crypto_JobStateType;
typedef uint32 Crypto_KeyIdType;
typedef uint32 Crypto_KeyElementIdType;

typedef struct {
    Crypto_AlgorithmFamilyType family;
    Crypto_AlgorithmModeType   mode;
    uint32                     keyLength;
    Crypto_EccCurveType        curve;
} Crypto_AlgorithmInfoType;

typedef struct {
    Crypto_KeyElementIdType id;
    uint32                  size;
    boolean                 allowPartialAccess;
    boolean                 writeAccess;
    uint8*                  data;
} Crypto_KeyElementType;

typedef struct {
    Crypto_KeyIdType         keyId;
    uint32                   numElements;
    Crypto_KeyElementType*   keyElements;
    Crypto_KeyTypeEnum       keyType;
    uint8                    keyState;
} Crypto_KeyType;

typedef struct {
    uint32                      callbackId;
    Crypto_AlgorithmInfoType*   algorithm;
    Crypto_ServiceInfoType      service;
    Crypto_ProcessingType       processingType;
    boolean                     primitiveCallbackUpdateNotification;
} Crypto_JobPrimitiveInfoType;

typedef struct {
    uint32    jobId;
    uint32    jobPriority;
} Crypto_JobInfoType;

typedef struct {
    uint32*   inputPtr;
    uint32    inputLength;
    Crypto_OperationModeType mode;
    uint32*   secondaryInputPtr;
    uint32    secondaryInputLength;
    uint32*   tertiaryInputPtr;
    uint32    tertiaryInputLength;
    uint32*   outputPtr;
    uint32*   outputLengthPtr;
    uint32*   secondaryOutputPtr;
    uint32*   secondaryOutputLengthPtr;
    uint64    input64;
    const uint8*  input8Ptr;
    uint8*        output8Ptr;
    uint32*       outputLength8Ptr;
    Crypto_VerifyResultType* verifyPtr;
} Crypto_JobPrimitiveInputOutputType;

typedef struct {
    uint32                              jobId;
    Crypto_JobStateType                 jobState;
    Crypto_JobPrimitiveInputOutputType* jobPrimitiveInputOutput;
    Crypto_JobPrimitiveInfoType*        jobPrimitiveInfo;
    Crypto_JobInfoType*                 jobInfo;
    uint32                              cryptoKeyId;
    uint32                              targetCryptoKeyId;
    uint32                              jobRedirectionInfoRef;
    uint32                              targetKeyId;
} Crypto_JobType;

typedef struct {
    boolean hsmEnabled;
    uint32  hsmInstanceId;
    uint32  hsmChannelId;
    uint32  hsmCommandTimeout;
    uint32  hsmResponseTimeout;
} Crypto_HsmConfigType;

typedef struct {
    const Crypto_DriverObjectConfigType* driverObjects;
    uint32                               numDriverObjects;
    const Crypto_ChannelConfigType*      channels;
    uint32                               numChannels;
    Crypto_KeyType*                      keys;
    uint32                               numKeys;
    Crypto_HsmConfigType                 hsmConfig;
    boolean                              hwAccelerationEnabled;
    uint32                               clockFrequency;
} Crypto_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

#### 标准 AUTOSAR API

| API | 签名 | 功能 | SWS 需求 | SWS ID |
|-----|------|------|----------|--------|
| Crypto_Init | `void Crypto_Init(const Crypto_ConfigType* configPtr)` | 初始化 Crypto 驱动 | SWS_Crypto_00001 | SWS_Crypto_00001 |
| Crypto_DeInit | `void Crypto_DeInit(void)` | 反初始化 | SWS_Crypto_00002 | SWS_Crypto_00002 |
| Crypto_GetVersionInfo | `void Crypto_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 版本信息 | SWS_Crypto_00003 | SWS_Crypto_00003 |
| Crypto_ProcessJob | `Std_ReturnType Crypto_ProcessJob(uint32 objectId, Crypto_JobType* job)` | 处理密码作业 | SWS_Crypto_00004 | SWS_Crypto_00004 |
| Crypto_CancelJob | `Std_ReturnType Crypto_CancelJob(uint32 objectId, Crypto_JobType* job)` | 取消作业 | SWS_Crypto_00005 | SWS_Crypto_00005 |

#### 密钥管理 API

| API | 签名 | 功能 | SWS 需求 | SWS ID |
|-----|------|------|----------|--------|
| Crypto_KeyElementSet | `Std_ReturnType Crypto_KeyElementSet(Crypto_KeyIdType id, Crypto_KeyElementIdType elemId, const uint8* keyPtr, uint32 keyLength)` | 设置密钥元素 | SWS_Crypto_00006 | SWS_Crypto_00006 |
| Crypto_KeyElementGet | `Std_ReturnType Crypto_KeyElementGet(...)` | 读取密钥元素 | SWS_Crypto_00007 | SWS_Crypto_00007 |
| Crypto_KeyValidSet | `Std_ReturnType Crypto_KeyValidSet(Crypto_KeyIdType id, boolean valid)` | 设置密钥有效性 | SWS_Crypto_00008 | SWS_Crypto_00008 |
| Crypto_KeyElementIdsGet | `Std_ReturnType Crypto_KeyElementIdsGet(...)` | 获取元素 ID 列表 | SWS_Crypto_00009 | SWS_Crypto_00009 |
| Crypto_KeyElementCopy | `Std_ReturnType Crypto_KeyElementCopy(...)` | 复制密钥元素 | SWS_Crypto_00010 | SWS_Crypto_00010 |
| Crypto_KeyElementMove | `Std_ReturnType Crypto_KeyElementMove(...)` | 移动密钥元素 | - | SWS_Crypto_00011 |
| Crypto_KeyElementClear | `Std_ReturnType Crypto_KeyElementClear(...)` | 清除密钥元素 | - | SWS_Crypto_00012 |
| Crypto_KeyCopy | `Std_ReturnType Crypto_KeyCopy(...)` | 复制密钥 | - | SWS_Crypto_00013 |
| Crypto_KeyGenerate | `Std_ReturnType Crypto_KeyGenerate(Crypto_KeyIdType id)` | 生成密钥 | SWS_Crypto_00011 | SWS_Crypto_00014 |

#### 密码运算 API

| API | 签名 | 功能 | SWS 需求 | SWS ID |
|-----|------|------|----------|--------|
| Crypto_KeyDerive | `Std_ReturnType Crypto_KeyDerive(Crypto_KeyIdType src, Crypto_KeyIdType dst)` | 密钥派生 | SWS_Crypto_00012 | SWS_Crypto_00015 |
| Crypto_KeyExchangeCalcSecret | `Std_ReturnType Crypto_KeyExchangeCalcSecret(Crypto_KeyIdType id, const uint8* pubKeyPtr, uint32 len)` | ECDH 共享密钥 | SWS_Crypto_00013 | SWS_Crypto_00016 |
| Crypto_RandomGenerate | `Std_ReturnType Crypto_RandomGenerate(Crypto_KeyIdType id, uint8* resultPtr, uint32 len)` | 随机数生成 | SWS_Crypto_00014 | SWS_Crypto_00017 |
| Crypto_RandomSeed | `Std_ReturnType Crypto_RandomSeed(Crypto_KeyIdType id, const uint8* entropyPtr, uint32 len)` | 随机数种子 | SWS_Crypto_00015 | SWS_Crypto_00018 |

#### HSM 专用 API（`CRYPTO_CFG_HSM_ENABLED == STD_ON`）

| API | 功能 | SWS 需求 | SWS ID |
|-----|------|----------|--------|
| `Crypto_HsmIsAvailable` | 检查 HSM 是否可用 | SWS_Crypto_00016 |
| `Crypto_HsmGetStatus` | 获取 HSM 状态 | SWS_Crypto_00017 |
| `Crypto_HsmLoadKey` | 加载密钥到 HSM | SWS_Crypto_00018 |
| `Crypto_HsmUnloadKey` | 卸载密钥 | - |
| `Crypto_HsmSelfTest` | HSM 自测 | SWS_Crypto_00019 |
| `Crypto_HsmGetId` | 获取 HSM 唯一 ID | SWS_Crypto_00020 |

#### CCC Digital Key 专用 API

| API | 功能 | SWS 需求 | SWS ID |
|-----|------|----------|--------|
| `Crypto_CccGenerateAttestation` | 设备证明签名 | SWS_Crypto_00026 |
| `Crypto_CccVerifyOwnerCertificate` | 车主证书验证（当前 stub） | SWS_Crypto_00027 |
| `Crypto_CccDeriveSessionKey` | ECDH 派生会话密钥 | SWS_Crypto_00028 |
| `Crypto_CccEncrypt` | AES-GCM 加密 | SWS_Crypto_00029 |
| `Crypto_CccDecrypt` | AES-GCM 解密 | SWS_Crypto_00030 |

#### BLAKE2 扩展 API

| API | 功能 | SWS 需求 | SWS ID |
|-----|------|----------|--------|
| `Crypto_Blake2b` / `Crypto_Blake2s` | 一次性 BLAKE2 哈希 | SWS_Crypto_00021 / SWS_Crypto_00022 |
| `Crypto_Blake2b_Start` | 启动增量哈希 | SWS_Crypto_00023 |
| `Crypto_Blake2b_Update` | 更新增量哈希 | SWS_Crypto_00024 |
| `Crypto_Blake2b_Finish` | 完成增量哈希 | SWS_Crypto_00025 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| `Crypto_JobNotification` | 作业完成通知（weak，可被应用覆盖） |
| `Crypto_ErrorNotification` | 错误通知（weak） |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 | SWS ID |
|-----|-----|------------|--------|
| 0x00 | Crypto_Init | CRYPTO_E_PARAM_POINTER / CRYPTO_E_ALREADY_INITIALIZED | SWS_Crypto_00019 |
| 0x01 | Crypto_DeInit | - | SWS_Crypto_00020 |
| 0x03 | Crypto_ProcessJob | CRYPTO_E_UNINIT / CRYPTO_E_PARAM_POINTER / CRYPTO_E_PARAM_HANDLE / CRYPTO_E_QUEUE_FULL | SWS_Crypto_00021 |
| 0x0E | Crypto_CancelJob | CRYPTO_E_UNINIT / CRYPTO_E_PARAM_POINTER | SWS_Crypto_00022 |
| 0x11 | Crypto_KeyElementSet | CRYPTO_E_UNINIT / CRYPTO_E_PARAM_POINTER / CRYPTO_E_PARAM_VALUE / CRYPTO_E_SMALL_BUFFER | SWS_Crypto_00023 |
| 0x13 | Crypto_KeyValidSet | CRYPTO_E_UNINIT | SWS_Crypto_00024 |
| 0x14 | Crypto_KeyElementGet | CRYPTO_E_UNINIT / CRYPTO_E_PARAM_POINTER | SWS_Crypto_00025 |
| 0x15 | Crypto_KeyExchangeCalcSecret | CRYPTO_E_UNINIT / CRYPTO_E_PARAM_POINTER | SWS_Crypto_00026 |
| 0x16 | Crypto_KeyDerive | CRYPTO_E_UNINIT | SWS_Crypto_00027 |
| 0x18 | Crypto_RandomGenerate | CRYPTO_E_UNINIT / CRYPTO_E_PARAM_POINTER / CRYPTO_E_PARAM_VALUE | SWS_Crypto_00028 |
| 0x19 | Crypto_KeyGenerate | CRYPTO_E_UNINIT / CRYPTO_E_PARAM_HANDLE | SWS_Crypto_00029 |
| 0x80~0x82 | HSM/BLAKE2 内部 SID | CRYPTO_E_UNINIT / CRYPTO_E_NOT_SUPPORTED |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 `configPtr` 非空，否则报 `CRYPTO_E_PARAM_POINTER`。
2. 检查驱动状态为 `UNINIT`，否则报 `CRYPTO_E_ALREADY_INITIALIZED`。
3. 保存配置指针，清空作业队列。
4. 调用 `Crypto_MbedTLS_Init()` 初始化软件后端；失败则报错并返回。
5. 若 `CRYPTO_CFG_HSM_ENABLED == STD_ON`：
   - 调用 `Crypto_Hsm_Init()`；
   - 若 HSM 可用，执行 `Crypto_Hsm_SelfTest()`。
6. 置 `Crypto_DriverState = CRYPTO_DRIVER_INIT`。

### 7.2 作业处理流程

1. `Crypto_ProcessJob` 校验驱动状态、作业指针、`objectId`。
2. `Crypto_ValidateJob` 校验 `jobPrimitiveInfo` 与 `jobPrimitiveInputOutput`。
3. 若 `processingType == CRYPTO_PROCESSING_SYNC`，直接调用 `Crypto_ProcessJobInternal`。
4. 若为 ASYNC：
   - 队列满则报 `CRYPTO_E_QUEUE_FULL`；
   - 否则入队，状态置 `QUEUED`。
5. `Crypto_ProcessJobInternal`：
   - 若 HSM 可用且为 SYNC，先尝试 HSM；成功则回调并返回；
   - HSM 失败且允许 fallback 时，使用 Mbed TLS；
   - 最终状态置 `IDLE`，触发 `Crypto_JobNotification`。
6. `Crypto_MainFunction` 周期性地从队列弹出并处理。

### 7.3 密钥管理流程

- **KeyElementSet**：查找 Key -> 查找 Element -> 校验写权限与长度 -> 拷贝数据。
- **KeyElementGet**：查找 Key/Element -> 校验缓冲区大小 -> 拷贝数据并返回长度。
- **KeyValidSet**：设置 `keyState` 为 `CRYPTO_KEY_VALID` 或 `CRYPTO_KEY_INVALID`。
- **KeyGenerate**：优先尝试 HSM `Crypto_Hsm_LoadKey`，否则回退 Mbed TLS。

### 7.4 反初始化流程

1. 清空作业队列。
2. 若 HSM 使能，调用 `Crypto_Hsm_DeInit()`。
3. 调用 `Crypto_MbedTLS_DeInit()`。
4. 状态置 `UNINIT`，清除 magic 与配置指针。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `CRYPTO_CFG_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `CRYPTO_CFG_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `CRYPTO_CFG_RUNTIME_ERROR_DETECT` | STD_OFF | 运行时错误检测 |
| `CRYPTO_CFG_HSM_ENABLED` | STD_ON | HSM 后端使能 |
| `CRYPTO_CFG_HSM_FALLBACK_TO_SW` | STD_ON | HSM 失败回退软件 |
| `CRYPTO_NUM_KEYS` | 16U | 密钥数量 |
| `CRYPTO_NUM_CHANNELS` | 8U | 通道数量 |
| `CRYPTO_NUM_DRIVER_OBJECTS` | 4U | Driver Object 数量 |
| `CRYPTO_CFG_QUEUE_SIZE` | 8U | 作业队列大小 |
| `CRYPTO_CFG_MAX_KEY_SIZE` | 128U | 最大密钥长度 |
| `CRYPTO_CFG_MAX_SIGNATURE_SIZE` | 72U | 最大签名长度 |
| `CRYPTO_CFG_CCC_KEY/IV/TAG_SIZE` | 16/12/16 | CCC 算法长度 |
| `CRYPTO_HSM_SUPPORT_*` | STD_ON | HSM 支持的算法开关 |
| `CRYPTO_KEY_ID_*` / `CRYPTO_KEY_ELEMENT_ID_*` | - | 密钥与元素 ID |
| `CRYPTO_DRIVER_OBJECT_*_ID` | - | Driver Object ID |
| `CRYPTO_CHANNEL_*` | - | 通道 ID |

### 8.2 链接时配置

`Crypto_Cfg.c` 提供运行时密钥表、密钥元素数据缓冲区、Driver Object 表、Channel 表以及全局 `Crypto_Config`。

### 8.3 构建后配置

当前实现主要依赖预编译与链接时配置，未启用 Post-Build 配置变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误（Crypto.h 中定义）

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x01 | CRYPTO_E_UNINIT | 模块未初始化 |
| 0x02 | CRYPTO_E_ALREADY_INITIALIZED | 重复初始化 |
| 0x03 | CRYPTO_E_PARAM_POINTER | 空指针 |
| 0x04 | CRYPTO_E_PARAM_HANDLE | Key/Channel/Object 句柄无效 |
| 0x05 | CRYPTO_E_PARAM_VALUE | 参数值非法（如长度为 0） |
| 0x06 | CRYPTO_E_PARAM_STATE | 状态非法（如 BLAKE2 未 Start） |
| 0x07 | CRYPTO_E_SMALL_BUFFER | 缓冲区不足 |
| 0x08 | CRYPTO_E_NOT_SUPPORTED | 功能不支持 |
| 0x09 | CRYPTO_E_QUEUE_FULL | 作业队列满 |
| 0x0A | CRYPTO_E_JOB_CANCELED | 作业被取消 |

### 9.2 扩展错误码（Crypto_Types.h）

包含 `CRYPTO_E_KEY_NOT_VALID`、`CRYPTO_E_KEY_SIZE_MISMATCH`、`CRYPTO_E_HSM_GENERAL_ERROR`、`CRYPTO_E_HSM_NOT_RESPONDING` 等，用于 Mbed TLS/HSM 内部返回。

### 9.3 安全机制

- 密钥元素支持 `readAccess/writeAccess/allowPartialAccess` 控制。
- HSM 后端提供安全密钥存储与算法卸载。
- CCC 相关接口使用专用 Key ID（如 `CRYPTO_KEY_ID_CCC_DEVICE_KEY`）。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| `CRYPTO_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据段（`Crypto_Config`） |
| `CRYPTO_STOP_SEC_CONFIG_DATA_UNSPECIFIED` | |
| `CRYPTO_START_SEC_VAR_INIT_UNSPECIFIED` | 已初始化变量（驱动状态、配置指针、magic） |
| `CRYPTO_STOP_SEC_VAR_INIT_UNSPECIFIED` | |
| `CRYPTO_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化变量（队列头尾、计数器） |
| `CRYPTO_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | |
| `CRYPTO_START_SEC_VAR_CLEARED_BOOLEAN` | 布尔变量（`Crypto_HsmAvailable`） |
| `CRYPTO_STOP_SEC_VAR_CLEARED_BOOLEAN` | |
| `CRYPTO_START_SEC_CODE` | 代码段 |
| `CRYPTO_STOP_SEC_CODE` | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | 较大 | 运行时密钥表、密钥元素缓冲区、队列状态 |
| ROM | 较大 | Mbed TLS 库、配置表、HSM 驱动 |
| 堆栈 | 中等 | 密码运算上下文较大，需评估 |

---

## 11. 集成指南

- 在 `EcuM` 阶段调用 `Crypto_Init(&Crypto_Config)`，必须在 CSM 使用之前完成。
- 确保 Mbed TLS 已正确链接并配置堆/锁（多任务环境）。
- 若使能 HSM，需先完成 HSM 驱动与电源初始化。
- 上层 CSM/CryIf 通过 `Crypto_ProcessJob` 调用服务，异步作业需在 `Crypto_MainFunction` 中周期性处理。
- 覆盖 `Crypto_JobNotification` 以实现作业完成通知。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `test_crypto.c` | 初始化、KeyElementSet/Get、ProcessJob、CancelJob、随机数、BLAKE2 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 与 CSM/CryIf 集成 | 验证标准 AUTOSAR 密码服务调用链 |
| HSM 回退 | 模拟 HSM 失败时正确回退 Mbed TLS |
| CCC Digital Key | 验证 AES-GCM、ECDSA、ECDH 流程 |

---

## 13. 实现说明 / TODO

- 源码中 `CRYPTO_MODULE_ID` 定义为 `110U`（0x6E），与 AUTOSAR 标准 `0x1E` 不一致；建议后续统一。
- `Crypto_QueuePush/QueuePop` 为简化实现，未使用预分配池，生产环境需完善。
- `Crypto_CccVerifyOwnerCertificate` 当前返回失败，需要补充证书链验证逻辑。
- `Crypto_HsmGetId` 尚未实现。
- `Crypto_RandomSeed` 为空实现，依赖 Mbed TLS 内部熵源。
- `Crypto_Types.h` 与 `Crypto_Cfg.c` 中已修复 Config-time 与 Runtime key 结构体字段偏移不一致的问题。

---

## 14. 参考资料

1. AUTOSAR_SWS_Crypto.pdf
2. `docs/modules/Crypto.md`
3. `src/bsw/mcal/crypto/`
