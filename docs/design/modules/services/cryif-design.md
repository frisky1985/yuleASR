# CryIf Design Document

> **Module ID**: 0x6F  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_CryptoInterface  
> **Source Path**: `src/bsw/services/cryif/`  
> **Reference Document**: `docs/modules/cryif.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

CryIf (Crypto Interface) 是 AUTOSAR BSW 服务层的密码接口抽象模块，为上层模块（CSM）提供统一的密码运算访问接口。CryIf 将密码操作请求路由到底层密码驱动（Crypto Driver），实现密码硬件和算法的抽象。CryIf 管理密码通道（Channel）、密钥（Key）和作业（Job），支持同步和异步密码操作。

CryIf 模块支持以下核心能力：
- 密码作业管理（ProcessJob / CancelJob）
- 密钥管理（KeyElementSet/Get/Copy、KeyGenerate、KeyDerive）
- 密钥交换协议支持（公钥计算、共享密钥计算）
- 证书管理（解析、验证）
- 通道到密码驱动的映射
- 同步/异步处理模式
- 安全等级管理

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS CryptoInterface | 4.4.0 | CryIf 模块规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | CSM | 调用 CryIf_ProcessJob 提交密码运算 |
| 下层 | Crypto Driver | 底层密码硬件驱动 |
| 下层 | Det | 开发错误报告 |
| 下层 | EcuM | 初始化阶段调用 CryIf_Init |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│     CSM (Crypto Stack Manager)      │
├─────────────────────────────────────┤
│     CryIf (Services Layer)          │
├─────────────────────────────────────┤
│     Crypto Driver (Hardware)        │
│     Det (Error Tracing)             │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Channel Manager**: 管理密码通道，每个通道映射到一个密码驱动对象
- **Key Store**: 管理密码密钥及其安全等级
- **Job Scheduler**: 管理密码作业（同步/异步调度）
- **Buffer Pool**: 密码运算缓冲区管理
- **Driver Mapper**: 将通道/密钥映射到底层密码驱动

### 3.3 文件结构

```
src/bsw/services/cryif/
├── include/
│   ├── CryIf.h           # 公共 API 声明
│   ├── CryIf_Cfg.h       # 预编译配置
│   ├── CryIf_Types.h     # 类型定义（算法族、模式、安全等级等）
│   └── CryIf_MemMap.h    # 内存段映射
└── src/
    └── CryIf.c            # 核心实现
```

---

## 4. 状态机

```
          CryIf_Init()
UNINIT ──────────────► INIT
  ▲                      │
  │    CryIf_DeInit()    │
  └──────────────────────┘
                         │
          ┌──────────────┴──────────────┐
          │                             │
   ProcessJob()                  MainFunction()
   (提交密码作业)                (处理异步作业)
```

CryIf 模块有两个状态：
- **CRYIF_STATE_UNINIT (0x00)**: 模块未初始化
- **CRYIF_STATE_INIT (0x01)**: 模块已初始化

---

## 5. 核心数据结构

### 5.1 通道类型

```c
typedef struct {
    CryIf_ChannelIdType channelId;    /* 通道 ID */
    uint8 driverIndex;                /* 密码驱动索引 */
    uint8 channelIndex;               /* 驱动对象索引 */
    uint32 maxKeySize;                /* 最大密钥大小 */
    uint32 maxJobSize;                /* 最大作业大小 */
    boolean isActive;                 /* 通道激活标志 */
} CryIf_ChannelType;
```

### 5.2 密钥类型

```c
typedef struct {
    CryIf_KeyIdType keyId;            /* CryIf 密钥 ID */
    CryIf_KeyIdType cryptoKeyId;      /* 密码驱动密钥 ID */
    uint32 keyElementCount;           /* 密钥元素数量 */
    CryIf_SecurityLevelType securityLevel; /* 安全等级 */
    boolean isValid;                  /* 密钥有效标志 */
} CryIf_KeyType;
```

### 5.3 作业类型

```c
typedef struct {
    CryIf_JobIdType jobId;                              /* 作业 ID */
    CryIf_JobPrimitiveInfoType jobPrimitiveInfo;         /* 算法信息 */
    CryIf_JobPrimitiveInputOutputType jobPrimitiveInputOutput; /* I/O */
    CryIf_ProcessingType processingType;                 /* 同步/异步 */
    CryIf_NotificationCallbackType callback;             /* 完成回调 */
    uint32 priority;                                     /* 优先级 */
    boolean isBusy;                                      /* 忙碌标志 */
} CryIf_JobType;
```

### 5.4 内部数据类型

```c
typedef struct {
    CryIf_StateType state;                    /* 模块状态 */
    const CryIf_ConfigType* configPtr;        /* 配置指针 */
    CryIf_ChannelType channels[CRYIF_CFG_MAX_CHANNEL_COUNT]; /* 通道数组 */
    CryIf_KeyType keys[CRYIF_CFG_MAX_KEY_COUNT];             /* 密钥数组 */
    CryIf_JobType jobs[CRYIF_CFG_MAX_JOB_COUNT];             /* 作业数组 */
} CryIf_InternalDataType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | Service ID | 说明 | SWS 需求 |
|-----|-----------|------|----------|
| `void CryIf_Init(const CryIf_ConfigType* configPtr)` | 0x01 | 初始化 | SWS_CryIf_00001 |
| `void CryIf_DeInit(void)` | 0x02 | 反初始化 | SWS_CryIf_00002 |
| `void CryIf_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 0x03 | 版本信息 | SWS_CryIf_00003 |
| `Std_ReturnType CryIf_ProcessJob(CryIf_ChannelIdType, CryIf_JobType*)` | 0x04 | 处理密码作业 | SWS_CryIf_00005 |
| `Std_ReturnType CryIf_CancelJob(CryIf_ChannelIdType, CryIf_JobType*)` | 0x05 | 取消作业 | SWS_CryIf_00006 |
| `Std_ReturnType CryIf_KeyElementSet(CryIf_KeyIdType, CryIf_KeyElementIdType, const uint8*, uint32)` | 0x06 | 设置密钥元素 | SWS_CryIf_00007 |
| `Std_ReturnType CryIf_KeySetValid(CryIf_KeyIdType)` | 0x07 | 设置密钥有效 | SWS_CryIf_00008 |
| `Std_ReturnType CryIf_KeyElementGet(CryIf_KeyIdType, CryIf_KeyElementIdType, uint8*, uint32*)` | 0x08 | 获取密钥元素 | SWS_CryIf_00009 |
| `Std_ReturnType CryIf_KeyElementCopy(...)` | 0x09 | 复制密钥元素 | SWS_CryIf_00010 |
| `Std_ReturnType CryIf_KeyCopy(CryIf_KeyIdType, CryIf_KeyIdType)` | 0x0A | 复制密钥 | SWS_CryIf_00012 |
| `Std_ReturnType CryIf_KeyElementIdsGet(CryIf_KeyIdType, CryIf_KeyElementIdType*, uint32*)` | 0x0B | 获取密钥元素 ID 列表 | SWS_CryIf_00013 |
| `Std_ReturnType CryIf_RandomSeed(CryIf_KeyIdType, const uint8*, uint32)` | 0x0C | 随机数种子 | SWS_CryIf_00015 |
| `Std_ReturnType CryIf_KeyGenerate(CryIf_KeyIdType)` | 0x0D | 生成密钥 | SWS_CryIf_00016 |
| `Std_ReturnType CryIf_KeyDerive(CryIf_KeyIdType, CryIf_KeyIdType)` | 0x0E | 密钥派生 | SWS_CryIf_00017 |
| `Std_ReturnType CryIf_KeyExchangeCalcPubValue(CryIf_KeyIdType, uint8*, uint32*)` | 0x0F | 密钥交换公钥计算 | SWS_CryIf_00018 |
| `Std_ReturnType CryIf_KeyExchangeCalcSecret(CryIf_KeyIdType, const uint8*, uint32)` | 0x10 | 密钥交换共享密钥 | SWS_CryIf_00019 |
| `Std_ReturnType CryIf_CertificateParse(CryIf_KeyIdType)` | 0x11 | 证书解析 | SWS_CryIf_00020 |
| `Std_ReturnType CryIf_CertificateVerify(CryIf_KeyIdType, CryIf_KeyIdType)` | 0x12 | 证书验证 | SWS_CryIf_00021 |
| `void CryIf_CallbackNotification(CryIf_ChannelIdType, CryIf_JobType*, CryIf_ResultType)` | 0x13 | 回调通知 | SWS_CryIf_00022 |
| `void CryIf_MainFunction(void)` | 0x14 | 主函数 | SWS_CryIf_00004 |

### 6.2 回调函数

```c
typedef void (*CryIf_NotificationCallbackType)(CryIf_JobIdType jobId, CryIf_ResultType result);
```

### 6.3 服务 ID 与错误码

**DET Error Codes:**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| CRYIF_E_PARAM_POINTER | 0x01 | NULL 指针 |
| CRYIF_E_UNINIT | 0x02 | 模块未初始化 |
| CRYIF_E_INIT_FAILED | 0x03 | 初始化失败 |
| CRYIF_E_PARAM_HANDLE | 0x04 | 无效句柄（通道/密钥 ID） |
| CRYIF_E_PARAM_VALUE | 0x05 | 无效参数值 |
| CRYIF_E_BUSY | 0x06 | 通道忙碌 |

**算法族 (CryIf_AlgorithmFamilyEnumType):**

| 值 | 名称 | 说明 |
|----|------|------|
| 0x01 | AES | 对称加密 |
| 0x02 | DES | DES 加密 |
| 0x03 | RSA | 非对称加密 |
| 0x04 | ECC | 椭圆曲线 |
| 0x05~0x0D | SHA1/SHA2/SHA3 | 哈希算法 |
| 0x0E | HMAC | 消息认证码 |
| 0x11 | DRBG | 随机数生成 |
| 0x15 | ECDSA | 椭圆曲线签名 |
| 0x16 | ED25519 | EdDSA 签名 |

**安全等级 (CryIf_SecurityLevelEnumType):**

| 等级 | 值 | 说明 |
|------|-----|------|
| CRYIF_SEC_LEVEL_NONE | 0x00 | 无安全保护 |
| CRYIF_SEC_LEVEL_1~7 | 0x01~0x07 | 安全等级递增 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 configPtr 和 generalConfig 指针非 NULL
2. 存储配置指针
3. 初始化所有通道为未激活状态
4. 根据配置激活通道，设置驱动映射
5. 初始化所有密钥为无效状态
6. 根据配置更新密钥映射
7. 初始化所有作业为非忙碌状态
8. 设置模块状态为 CRYIF_STATE_INIT

### 7.2 密码作业处理流程

1. 检查模块已初始化、通道有效、作业指针非 NULL
2. 获取通道配置
3. 存储作业引用到内部作业数组
4. 根据处理类型：
   - 同步：立即完成，标记非忙碌，触发回调
   - 异步：保持忙碌状态，等待 MainFunction 或回调处理

### 7.3 MainFunction 流程

1. 检查模块已初始化
2. 遍历所有作业
3. 对忙碌的异步作业，轮询底层驱动状态
4. 作业完成时标记非忙碌并触发回调

---

## 8. 配置设计

### 8.1 预编译配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `CRYIF_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `CRYIF_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `CRYIF_CFG_MAX_CHANNEL_COUNT` | 4U | 最大通道数 |
| `CRYIF_CFG_MAX_KEY_COUNT` | 8U | 最大密钥数 |
| `CRYIF_CFG_MAX_JOB_COUNT` | 16U | 最大作业数 |
| `CRYIF_CFG_NUM_CHANNELS` | 2U | 配置通道数 |
| `CRYIF_CFG_NUM_KEYS` | 4U | 配置密钥数 |
| `CRYIF_CFG_MAIN_FUNCTION_PERIOD_MS` | 10U | 主函数周期 |
| `CRYIF_CFG_MAX_BUFFER_SIZE` | 1024U | 缓冲区大小 |
| `CRYIF_CFG_MAX_KEY_ELEMENT_SIZE` | 256U | 最大密钥元素大小 |
| `CRYIF_KEY_ELEMENT_COPY_API` | STD_ON | 密钥元素复制 API |
| `CRYIF_KEY_VALID_CHECK_API` | STD_ON | 密钥有效性检查 API |

### 8.2 链接时配置

通过配置工具定义通道配置（驱动索引、对象索引、最大密钥/作业大小）和密钥配置（密码密钥 ID、安全等级）。

### 8.3 构建后配置

不支持构建后配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 场景 | API | 错误码 |
|------|-----|--------|
| configPtr 为 NULL | CryIf_Init | CRYIF_E_PARAM_POINTER |
| 模块未初始化 | 所有 API | CRYIF_E_UNINIT |
| 通道 ID 无效 | ProcessJob | CRYIF_E_PARAM_HANDLE |
| 作业指针为 NULL | ProcessJob | CRYIF_E_PARAM_POINTER |
| 密钥 ID 无效 | KeyElementSet | CRYIF_E_PARAM_HANDLE |
| 密钥元素长度超限 | KeyElementSet | CRYIF_E_PARAM_VALUE |

### 9.2 DEM 错误

CryIf 不直接报告 DEM 事件。密码操作失败通过 CSM 层上报。

### 9.3 安全机制

- 密钥安全等级管理（NONE ~ LEVEL_7）
- 通道激活检查确保未配置通道不可用
- 密钥有效性验证（KeyValidCheck）
- 缓冲区池管理防止内存溢出
- 回调通知机制确保异步操作结果及时传递

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 段 | 变量 | 说明 |
|----|------|------|
| 静态 | CryIf_InternalData | 模块内部数据 |
| 静态 | CryIf_BufferPool[1024] | 密码运算缓冲区 |
| CODE | 所有函数 | 代码段 |

### 10.2 资源估算

- **RAM**: CryIf_InternalData ≈ 4×48 + 8×20 + 16×64 ≈ 1376 字节 + BufferPool 1024 字节 ≈ 2.4 KB
- **ROM**: ~8 KB（代码段，含所有密钥管理和作业管理逻辑）
- **性能**: ProcessJob 同步模式为 O(1)；异步模式取决于底层驱动延迟

---

## 11. 集成指南

- CSM 通过 `CryIf_ProcessJob(channelId, &job)` 提交密码运算
- 通道 ID 映射到具体密码驱动对象（Channel 0 → Driver 0 Object 0）
- 密钥通过 `CryIf_KeyElementSet()` 设置密钥材料后使用
- 密钥交换流程：KeyGenerate → KeyExchangeCalcPubValue → KeyExchangeCalcSecret
- 证书流程：CertificateParse → CertificateVerify
- SCHM 以 10ms 周期调用 `CryIf_MainFunction()` 处理异步作业

---

## 12. 测试策略

### 12.1 单元测试

- 初始化/反初始化测试
- 通道有效性验证测试
- 同步作业处理测试（加密、解密、哈希、MAC）
- 异步作业处理和回调通知测试
- 密钥元素设置/获取/复制测试
- 密钥生成和派生测试
- 密钥交换流程测试
- 证书解析和验证测试

### 12.2 集成测试

- CSM → CryIf → Crypto Driver 完整密码运算链路
- 多通道并发操作测试
- 异步作业完成和回调正确性

---

## 13. 实现说明 / TODO

- 底层密码驱动调用当前为注释占位（`/* Crypto_Driver_... */`），需对接实际 HSM 驱动
- 密钥元素复制（KeyElementCopy/KeyElementCopyPartial）为简化实现
- BufferPool 当前为单一布尔管理，可优化为多缓冲区池
- 证书解析/验证功能需集成 X.509 解析库

---

## 14. 参考资料

- AUTOSAR_SWS_CryptoInterface.pdf (R4.4.0)
- AUTOSAR_SWS_CryptoStackManager.pdf
- AUTOSAR_SWS_CryptoDriver.pdf
- yuleASR CryIf 模块源码: `src/bsw/services/cryif/`
