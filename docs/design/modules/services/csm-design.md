# Csm (Crypto Services Manager) Design Document

> **Module ID**: 0x70  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.7.0  
> **SWS Reference**: AUTOSAR_SWS_CryptoServicesManager  
> **Source Path**: `src/bsw/services/csm/`  
> **Reference Document**: `docs/modules/CSM.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Csm 位于 AUTOSAR Services 层，为上层模块（SecOC、KeyM、Crypto Stack 等）提供统一的密码服务接口。它抽象了底层硬件加密引擎（通过 CryIf/Crypto）与软件加密实现，管理密钥、加密作业、服务队列和异步处理。

主要功能：
- 密钥管理：设置/获取密钥元素、复制密钥、生成/派生密钥、密钥交换
- 对称/非对称加密服务：Hash、MAC、Encrypt、Decrypt、Signature、Verify
- 随机数生成
- 异步服务队列：按优先级排队，支持并发执行
- 硬件服务抽象：通过配置回调 `Csm_Cfg_HwService()` 对接具体硬件
- 密钥持久化：通过 `Csm_Cfg_KeyWrite/KeyRead` 对接 NV 存储

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Crypto Services Manager | 4.7.0 | 模块软件规范 |
| AUTOSAR Classic Platform | 4.7.x | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SecOC, KeyM, 应用层 | 调用加密服务 |
| 同层 | CryIf | 当前代码包含头文件，主要硬件服务通过配置回调实现 |
| 公共 | Det | 开发错误检测（可选） |
| 公共 | Dem | 硬件故障、密钥无效等诊断事件（可选） |
| 集成 | 硬件加密驱动 | 通过 `Csm_Cfg_HwService()` 调用 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        SecOC / KeyM / App           │
├─────────────────────────────────────┤
│            Csm (Services)           │
├─────────────────────────────────────┤
│         CryIf / Csm_Cfg_HwService   │
├─────────────────────────────────────┤
│      Crypto HW / SW Implementations │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **密钥管理器**：维护 `Csm_Keys[]` 数组，支持密钥元素设置/获取/复制/持久化。
- **作业管理器**：维护 `Csm_Jobs[]` 数组，每个作业绑定服务类型、密钥、算法。
- **队列管理器**：在 `CSM_CFG_QUEUE_SUPPORT == STD_ON` 时按优先级管理待处理作业。
- **执行引擎**：`Csm_MainFunction()` 中处理队列，调用 `Csm_Cfg_HwService()` 执行作业。
- **回调通知器**：作业完成后调用注册的回调函数。
- **配置适配层**：`Csm_Cfg.h` 中定义的硬件/持久化/时间戳回调。

### 3.3 文件结构

```
src/bsw/services/csm/
├── include/
│   ├── Csm.h
│   ├── Csm_Types.h
│   ├── Csm_Cfg.h
│   └── Csm_MemMap.h
└── src/
    └── Csm.c
```

---

## 4. 状态机

### 4.1 模块状态

```
UNINIT -- Csm_Init() --> ACTIVE -- Csm_DeInit() --> UNINIT
```

中间经过 `INIT` 子状态，初始化完成后进入 `ACTIVE`。

### 4.2 密钥状态

| 状态 | 说明 |
|------|------|
| `CSM_KEY_STATUS_EMPTY` | 空槽位 |
| `CSM_KEY_STATUS_INVALID` | 已分配 ID，但元素未设置完整 |
| `CSM_KEY_STATUS_VALID` | 密钥有效，可用于服务 |

### 4.3 作业状态

| 状态 | 说明 |
|------|------|
| `CSM_JOB_STATE_IDLE` | 空闲 |
| `CSM_JOB_STATE_PROCESSING` | 正在处理 |
| `CSM_JOB_STATE_RESULT_READY` | 结果就绪 |

---

## 5. 核心数据结构

### 5.1 密钥与元素

```c
typedef struct {
    uint32 keyId;
    Csm_KeyStatusType status;
    uint8 numElements;
    uint32 referenceCount;
    Csm_KeyElementType elements[CSM_MAX_KEY_ELEMENTS];
} Csm_KeyType;
```

### 5.2 作业

```c
typedef struct {
    uint32 jobId;
    Csm_JobStateType state;
    Csm_ServiceType service;
    uint32 keyId;
    Csm_AlgorithmType algorithm;
    const uint8* inputData;
    uint32 inputLength;
    uint8* outputData;
    uint32 outputLength;
    uint32 resultLength;
    Std_ReturnType result;
    boolean verifyResult;
} Csm_JobType;
```

### 5.3 队列

```c
typedef struct {
    uint32 jobId;
    Csm_JobPriorityType priority;
    uint32 timestamp;
} Csm_QueueItemType;

typedef struct {
    Csm_QueueItemType items[CSM_CFG_QUEUE_SIZE];
    uint8 head;
    uint8 tail;
    uint8 count;
} Csm_QueueType;
```

### 5.4 配置相关类型

关键类型定义在 `Csm_Types.h` 中，包括 `Csm_ServiceType`、`Csm_AlgorithmType`、`Csm_JobPriorityType`、`Csm_ConfigType` 等。

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | SWS 需求 | 备注 |
|-----|------|------|----------|------|
| `Csm_Init` | `Std_ReturnType Csm_Init(const Csm_ConfigType* config)` | 初始化 Csm | SWS_Csm_00001 | 配置密钥、作业、队列 |
| `Csm_DeInit` | `Std_ReturnType Csm_DeInit(void)` | 反初始化 | SWS_Csm_00002 | 检查无活动作业 |
| `Csm_KeyElementSet` | `Std_ReturnType Csm_KeyElementSet(...)` | 设置密钥元素 | SWS_Csm_00010 | |
| `Csm_KeySetValid` | `Std_ReturnType Csm_KeySetValid(uint32 keyId)` | 设置密钥有效 | SWS_Csm_00011 | |
| `Csm_KeyElementGet` | `Std_ReturnType Csm_KeyElementGet(...)` | 获取密钥元素 | SWS_Csm_00012 | |
| `Csm_KeyElementCopy` | `Std_ReturnType Csm_KeyElementCopy(...)` | 复制密钥元素 | SWS_Csm_00013 | |
| `Csm_KeyCopy` | `Std_ReturnType Csm_KeyCopy(...)` | 复制完整密钥 | SWS_Csm_00014 | |
| `Csm_KeyElementIdsGet` | `Std_ReturnType Csm_KeyElementIdsGet(...)` | 获取密钥元素 ID 列表 | SWS_Csm_00015 | |
| `Csm_KeyGenerate` | `Std_ReturnType Csm_KeyGenerate(uint32 keyId)` | 生成密钥 | SWS_Csm_00016 | |
| `Csm_KeyDerive` | `Std_ReturnType Csm_KeyDerive(...)` | 派生密钥 | SWS_Csm_00017 | |
| `Csm_KeyExchangeCalcPubVal` | `Std_ReturnType Csm_KeyExchangeCalcPubVal(...)` | 计算密钥交换公钥 | SWS_Csm_00018 | |
| `Csm_KeyExchangeCalcSecret` | `Std_ReturnType Csm_KeyExchangeCalcSecret(...)` | 计算共享秘密 | SWS_Csm_00019 | |
| `Csm_Hash` | `Std_ReturnType Csm_Hash(...)` | 哈希计算 | SWS_Csm_00030 | |
| `Csm_MacGenerate` | `Std_ReturnType Csm_MacGenerate(...)` | 生成 MAC | SWS_Csm_00040 | |
| `Csm_MacVerify` | `Std_ReturnType Csm_MacVerify(...)` | 验证 MAC | SWS_Csm_00041 | |
| `Csm_Encrypt` | `Std_ReturnType Csm_Encrypt(...)` | 加密 | SWS_Csm_00050 | |
| `Csm_Decrypt` | `Std_ReturnType Csm_Decrypt(...)` | 解密 | SWS_Csm_00051 | |
| `Csm_SignatureGenerate` | `Std_ReturnType Csm_SignatureGenerate(...)` | 生成签名 | SWS_Csm_00060 | |
| `Csm_SignatureVerify` | `Std_ReturnType Csm_SignatureVerify(...)` | 验证签名 | SWS_Csm_00061 | |
| `Csm_RandomGenerate` | `Std_ReturnType Csm_RandomGenerate(...)` | 生成随机数 | SWS_Csm_00070 | |
| `Csm_JobKeySetUp` | `Std_ReturnType Csm_JobKeySetUp(...)` | 设置作业密钥 | SWS_Csm_00080 | |
| `Csm_JobKeySetUpAsync` | `Std_ReturnType Csm_JobKeySetUpAsync(...)` | 异步设置作业密钥 | SWS_Csm_00081 | |
| `Csm_CancelJob` | `Std_ReturnType Csm_CancelJob(uint32 jobId)` | 取消作业 | SWS_Csm_00090 | |
| `Csm_MainFunction` | `void Csm_MainFunction(void)` | 处理队列与异步作业 | SWS_Csm_00100 | |
| `Csm_RegisterCallback` | `Std_ReturnType Csm_RegisterCallback(...)` | 注册作业完成回调 | SWS_Csm_00101 | |
| `Csm_GetKeyStatus` | `Std_ReturnType Csm_GetKeyStatus(...)` | 获取密钥状态 | SWS_Csm_00102 | |
| `Csm_GetJobState` | `Std_ReturnType Csm_GetJobState(...)` | 获取作业状态 | SWS_Csm_00103 | |
| `Csm_GetVersionInfo` | `void Csm_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 版本信息 | SWS_Csm_00104 | |

### 6.2 配置回调

| 回调 | 说明 |
|------|------|
| `Csm_Cfg_KeyWrite` | 将密钥元素写入持久化存储 |
| `Csm_Cfg_KeyRead` | 从持久化存储读取密钥元素 |
| `Csm_Cfg_HwService` | 调用硬件加密服务 |
| `Csm_Cfg_RandomGenerate` | 硬件随机数生成 |
| `Csm_Cfg_GetTimestamp` | 获取当前时间戳（ms） |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | Init | `CSM_E_PARAM_POINTER`, `CSM_E_ALREADY_INITIALIZED` |
| 0x01 | DeInit | `CSM_E_NOT_INITIALIZED` |
| 0x10 | KeyElementSet | `CSM_E_PARAM_POINTER`, `CSM_E_PARAM_KEY` |
| 0x30 | Hash | `CSM_E_NOT_INITIALIZED` |
| 0x40 | MacGenerate | `CSM_E_NOT_INITIALIZED` |
| 0x50 | Encrypt | `CSM_E_NOT_INITIALIZED` |
| 0x70 | RandomGenerate | `CSM_E_NOT_INITIALIZED` |
| 0xA0 | MainFunction | - |

---

## 7. 处理流程

### 7.1 初始化

1. `Csm_Init()` 验证配置非空且未重复初始化。
2. 校验 `numKeys <= CSM_MAX_KEYS`、`numJobs <= CSM_MAX_JOBS`。
3. 清空 `Csm_Keys[]`、`Csm_Jobs[]`、回调数组。
4. 根据配置分配密钥 ID 与作业 ID，设置服务类型、密钥、算法。
5. 初始化队列头尾指针与计数。
6. 设置魔数与状态为 `ACTIVE`。

### 7.2 同步服务调用

1. 各服务 API（如 `Csm_Hash`、`Csm_MacGenerate`）检查初始化状态。
2. 查找作业索引，验证密钥使用权限。
3. 直接调用 `Csm_Cfg_HwService()` 执行。
4. 返回结果，必要时通知回调。

### 7.3 异步队列处理

1. 服务请求将作业加入队列 `Csm_QueueJob()`，按优先级插入（高优先级在前）。
2. `Csm_MainFunction()` 调用 `Csm_ProcessQueue()`。
3. 当 `Csm_ActiveJobCount < CSM_CFG_MAX_CONCURRENT_JOBS` 时出队并执行。
4. `Csm_ExecuteJob()` 调用硬件服务：
   - `E_OK`：状态变为 `RESULT_READY`，通知回调。
   - `E_BUSY`（值为 1）：保持 `PROCESSING`，下次继续。
   - 其他错误：状态回到 `IDLE`，通知回调，减少活动计数。

### 7.4 密钥持久化

1. `Csm_PersistKeyElement()` 查找密钥与元素索引。
2. 调用 `Csm_Cfg_KeyWrite()` 将元素数据写入存储。
3. `Csm_LoadKeyElement()` 通过 `Csm_Cfg_KeyRead()` 读取并标记元素有效。

---

## 8. 配置设计

### 8.1 预编译配置（`Csm_Cfg.h`）

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `CSM_CFG_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `CSM_CFG_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `CSM_CFG_ASYNC_SUPPORT` | STD_ON | 异步服务支持 |
| `CSM_CFG_QUEUE_SUPPORT` | STD_ON | 队列支持 |
| `CSM_CFG_KEY_PERSISTENCE_SUPPORT` | STD_ON | 密钥持久化支持 |
| `CSM_CFG_RANDOM_GENERATOR_SUPPORT` | STD_ON | 随机数生成支持 |
| `CSM_CFG_DEM_INTEGRATION` | STD_ON | DEM 集成 |
| `CSM_CFG_SERVICE_TIMEOUT_MS` | 1000U | 服务超时 |
| `CSM_CFG_MAIN_FUNCTION_PERIOD_MS` | 10U | 主函数周期 |
| `CSM_CFG_QUEUE_SIZE` | `CSM_MAX_QUEUE_DEPTH` | 队列深度 |
| `CSM_CFG_MAX_CONCURRENT_JOBS` | 4U | 最大并发作业数 |
| `CSM_KEY_ID_NONE` | 0xFFFFFFFFU | 无效密钥 ID |
| `CSM_JOB_ID_NONE` | 0xFFFFFFFFU | 无效作业 ID |
| 默认算法/密钥长度 | - | 哈希/加密/MAC/签名默认值 |

### 8.2 链接时配置

`Csm_Config` 在 `Csm_Cfg.h` 中声明为外部符号，具体定义在链接时配置源文件中。包含：

- 密钥配置数组
- 作业配置数组
- 默认算法参数

---

## 9. 错误处理与安全

### 9.1 DET 错误

Csm.h 中定义了 API ID，错误码在 `Csm_Types.h` 中（如 `CSM_E_NOT_INITIALIZED`、`CSM_E_PARAM_POINTER` 等）。具体错误码值需参考 `Csm_Types.h`。

常见错误：

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| `CSM_E_NOT_INITIALIZED` | 未初始化 | API 在初始化前调用 |
| `CSM_E_PARAM_POINTER` | 空指针 | 配置或数据指针为空 |
| `CSM_E_ALREADY_INITIALIZED` | 重复初始化 | Init 被调用两次 |
| `CSM_E_PARAM_KEY` | 密钥错误 | 密钥 ID 无效或状态不允许 |

### 9.2 DEM 事件

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| 1 | `CSM_DEM_HARDWARE_FAILURE_EVENT_ID` | 硬件加密失败 |
| 2 | `CSM_DEM_KEY_INVALID_EVENT_ID` | 密钥无效 |
| 3 | `CSM_DEM_SERVICE_TIMEOUT_EVENT_ID` | 服务超时 |
| 4 | `CSM_DEM_ENTROPY_EXHAUSTION_EVENT_ID` | 熵耗尽 |

### 9.3 安全机制

- 密钥状态机确保只有 `VALID` 密钥才能用于服务。
- 队列优先级与并发限制防止资源耗尽。
- 硬件服务抽象允许替换为安全认证的加密实现。
- 持久化接口隔离 NV 存储细节。

---

## 10. 内存与性能

### 10.1 MemMap 分区

当前实现使用 `Csm_MemMap.h` 分区：

| 分区 | 用途 |
|------|------|
| `CSM_START_SEC_VAR_INIT_UNSPECIFIED` | 初始化状态变量 |
| `CSM_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化运行时数组 |
| `CSM_START_SEC_CODE` | 代码段 |
| `CSM_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~数 KB | 密钥数组、作业数组、队列、回调数组 |
| ROM | ~20 KB | 代码与配置表 |
| 周期 | 取决于硬件 | 异步作业通过 MainFunction 轮询 |

---

## 11. 集成指南

- **硬件集成**：实现 `Csm_Cfg_HwService()`，根据 `jobId`、`serviceType`、`input`/`output` 分发到具体硬件或软件加密库。
- **密钥存储**：实现 `Csm_Cfg_KeyWrite()` / `Csm_Cfg_KeyRead()`，对接 NvM 或专用密钥存储区。
- **时间戳**：`Csm_Cfg_GetTimestamp()` 用于队列超时与排序，需对接 GPT/OS 时间服务。
- **随机数**：`Csm_Cfg_RandomGenerate()` 可对接硬件 TRNG。
- **CryIf**：当前代码包含 `CryIf.h`，但实际硬件服务通过配置回调实现，可根据项目需要改为 CryIf 路由。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `tests/unit/csm/Csm_Test.c` | 初始化、密钥管理、队列、同步/异步服务、回调 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 正常哈希 | 数据 → Csm_Hash → 硬件服务 → 结果 |
| 异步 MAC | 请求入队 → MainFunction 出队执行 → 回调 |
| 队列满 | 高优先级作业覆盖或返回 E_NOT_OK |
| 密钥持久化 | 设置密钥 → 持久化 → 重启加载 |
| 并发限制 | 超过 `CSM_CFG_MAX_CONCURRENT_JOBS` 时排队 |

---

## 13. 实现说明 / TODO

- `Csm_FindKeyElementIndex()` 当前简化处理，未按 `elementId` 精确匹配，需改为遍历比较。
- `Csm_ExecuteJob()` 使用 `(Std_ReturnType)1` 表示 `E_BUSY`，建议显式使用宏。
- `Csm_Cfg_HwService()` 由集成商实现，当前为外部声明。
- DEM 调用当前为注释，需根据需求启用。
- 部分高级功能（如密钥派生、密钥交换）在 `Csm.h` 中声明，但具体实现依赖硬件服务。
- 软件 KDF 回退逻辑当前未在源码中体现，需根据项目补充。

---

## 14. 参考资料

1. AUTOSAR_SWS_CryptoServicesManager.pdf
2. `docs/modules/CSM.md`
3. `src/bsw/services/csm/Csm.h`
4. `src/bsw/services/csm/Csm.c`
5. `src/bsw/services/csm/Csm_Cfg.h`
6. `src/bsw/services/csm/Csm_Types.h`

## 需求追溯表

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Csm | — | CSM 模块级需求归属 |
