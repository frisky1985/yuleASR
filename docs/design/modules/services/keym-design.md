# KeyM Design Document

> **Module ID**: 0x71  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_KeyManager  
> **Source Path**: `src/bsw/services/keym/`  
> **Reference Document**: `docs/modules/keym.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

KeyM (Key Manager) 是 AUTOSAR BSW 服务层的密钥管理模块，负责密码学密钥的全生命周期管理。KeyM 提供密钥的创建、存储、更新、验证、分发和销毁功能，支持多种密钥类型（AES、RSA、ECC、HMAC 等）和多种密钥格式（Raw、DER、PEM、COSE、JWK）。KeyM 是车载安全通信（SecOC、TLS、诊断安全访问）的基础设施。

KeyM 模块支持以下核心能力：
- 密钥生命周期管理（NEW → UPDATE → VALID → INVALID）
- 密钥元素管理（Key Material、IV、Salt、Tag 等）
- 密钥格式解析与转换
- 异步密钥操作支持
- 密钥有效期管理和过期检测
- NvM 持久化存储支持
- 密钥保护（XOR 混淆）

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS KeyManager | 4.7.0 | KeyM 模块规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | CSM | 调用 KeyM_GetKey 获取密钥材料 |
| 下层 | CryIf | 密钥映射到密码接口 |
| 下层 | NvM | 密钥持久化存储 |
| 下层 | Det | 开发错误报告 |
| 下层 | SchM | 排他区保护（多核安全） |
| 下层 | EcuM | 初始化阶段调用 KeyM_Init |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│    CSM / SecOC / DiagSecurity       │
├─────────────────────────────────────┤
│       KeyM (Services Layer)         │
├─────────────────────────────────────┤
│  CryIf | NvM | Det | SchM          │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Key Store**: 管理所有密钥的运行时数据（keyData、状态、版本、有效期）
- **Key Element Store**: 管理密钥元素（IV、Salt、Tag 等子组件）
- **Operation Queue**: 异步操作队列，支持非阻塞密钥操作
- **Lifecycle Manager**: 密钥生命周期状态机管理
- **Protection Engine**: 密钥数据保护（XOR 混淆）

### 3.3 文件结构

```
src/bsw/services/keym/
├── include/
│   ├── KeyM.h           # 公共 API 声明、类型定义
│   ├── KeyM_Cfg.h       # 预编译配置
│   ├── KeyM_MemMap.h    # 内存段映射
│   └── SchM_KeyM.h      # 排他区定义
└── src/
    └── KeyM.c            # 核心实现
```

---

## 4. 状态机

### 4.1 模块状态

```
          KeyM_Init()
UNINIT ──────────────► INIT
  ▲                      │
  │    KeyM_DeInit()     │
  └──────────────────────┘
```

### 4.2 密钥生命周期状态

```
    SetKey() / ParseKey()          FinalizeKey()
         ┌────────► NEW ──────────────► VALID
         │            │                   │
  UpdateKey()    UpdateKey()         (过期/撤销)
         │            │                   │
         └────────► UPDATE ──────────────► INVALID
```

密钥状态：
- **KEYM_KEY_STATUS_NEW (0)**: 新创建，尚未生效
- **KEYM_KEY_STATUS_UPDATE (1)**: 正在更新中
- **KEYM_KEY_STATUS_VALID (2)**: 有效，可被密码操作使用
- **KEYM_KEY_STATUS_INVALID (3)**: 无效或已撤销

---

## 5. 核心数据结构

### 5.1 密钥运行时类型

```c
typedef struct {
    uint8 keyData[KEYM_MAX_KEY_LENGTH];     /* 密钥材料 (256字节) */
    uint32 keyLength;                        /* 实际密钥长度 */
    KeyM_KeyStatusType keyStatus;            /* 当前状态 */
    uint32 keyVersion;                       /* 版本号 */
    uint32 validFrom;                        /* 生效时间戳 */
    uint32 validTo;                          /* 过期时间戳 (0=永不过期) */
    boolean isValid;                         /* 有效性标志 */
    boolean isLocked;                        /* 操作锁定标志 */
    uint32 operationCounter;                 /* 操作计数器 */
} KeyM_KeyRuntimeType;
```

### 5.2 密钥元素运行时类型

```c
typedef struct {
    uint8 elementData[KEYM_MAX_KEY_LENGTH]; /* 元素数据 */
    uint32 elementLength;                    /* 元素长度 */
    boolean inUse;                           /* 使用中标志 */
} KeyM_KeyElementRuntimeType;
```

### 5.3 操作队列类型

```c
typedef struct {
    KeyM_KeyIdType keyId;                    /* 密钥 ID */
    uint8 operationType;                     /* 操作类型 */
    boolean inUse;                           /* 队列槽使用中 */
    KeyM_OperationResultType result;         /* 操作结果 */
} KeyM_OperationQueueType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | Service ID | 说明 | SWS 需求 |
|-----|-----------|------|----------|
| `void KeyM_Init(const KeyM_ConfigType* ConfigPtr)` | 0x00 | 初始化 | SWS_KeyM_00001 |
| `void KeyM_DeInit(void)` | 0x01 | 反初始化（清除所有密钥数据） | SWS_KeyM_00002 |
| `void KeyM_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 0x02 | 版本信息 | SWS_KeyM_00003 |
| `Std_ReturnType KeyM_SetKey(KeyM_KeyIdType, const uint8*, uint32, KeyM_KeyFormatType)` | 0x10 | 设置密钥 | SWS_KeyM_00005 |
| `Std_ReturnType KeyM_GetKey(KeyM_KeyIdType, uint8*, uint32*, KeyM_KeyFormatType*)` | 0x11 | 获取密钥 | SWS_KeyM_00006 |
| `Std_ReturnType KeyM_UpdateKey(KeyM_KeyIdType, const uint8*, uint32, KeyM_KeyFormatType)` | 0x12 | 更新密钥 | SWS_KeyM_00007 |
| `Std_ReturnType KeyM_FinalizeKey(KeyM_KeyIdType)` | 0x13 | 最终化密钥（使其有效） | SWS_KeyM_00008 |
| `Std_ReturnType KeyM_ParseKey(KeyM_KeyIdType, const uint8*, uint32, KeyM_KeyFormatType)` | 0x20 | 解析密钥格式 | SWS_KeyM_00009 |
| `Std_ReturnType KeyM_ConvertKey(KeyM_KeyIdType, uint8*, uint32*, KeyM_KeyFormatType)` | 0x21 | 转换密钥格式 | SWS_KeyM_00010 |
| `Std_ReturnType KeyM_CopyKey(KeyM_KeyIdType, KeyM_KeyIdType)` | 0x22 | 复制密钥 | SWS_KeyM_00011 |
| `Std_ReturnType KeyM_KeyElementSet(KeyM_KeyIdType, KeyM_KeyElementIdType, const uint8*, uint32)` | 0x30 | 设置密钥元素 | SWS_KeyM_00012 |
| `Std_ReturnType KeyM_KeyElementGet(KeyM_KeyIdType, KeyM_KeyElementIdType, uint8*, uint32*)` | 0x31 | 获取密钥元素 | SWS_KeyM_00013 |
| `Std_ReturnType KeyM_KeyStatusGet(KeyM_KeyIdType, KeyM_KeyStatusType*)` | 0x40 | 获取密钥状态 | SWS_KeyM_00014 |
| `Std_ReturnType KeyM_KeyVersionGet(KeyM_KeyIdType, uint32*)` | 0x41 | 获取密钥版本 | SWS_KeyM_00015 |
| `Std_ReturnType KeyM_KeyValidityGet(KeyM_KeyIdType, uint32*, uint32*)` | 0x42 | 获取有效期 | SWS_KeyM_00016 |
| `Std_ReturnType KeyM_KeyInfoGet(KeyM_KeyIdType, KeyM_KeyInfoType*)` | 0x43 | 获取密钥完整信息 | SWS_KeyM_00017 |
| `Std_ReturnType KeyM_SetNotificationCallback(KeyM_NotificationCallbackType)` | 0x50 | 设置通知回调 | SWS_KeyM_00018 |
| `void KeyM_MainFunction(void)` | 0x60 | 主函数 | SWS_KeyM_00004 |

### 6.2 回调函数

```c
typedef void (*KeyM_NotificationCallbackType)(
    KeyM_KeyIdType keyId,
    KeyM_OperationResultType result,
    const uint8* dataPtr,
    uint32 dataLength
);
```

### 6.3 服务 ID 与错误码

**DET Error Codes:**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| KEYM_E_PARAM_POINTER | 0x01 | NULL 指针 |
| KEYM_E_PARAM_HANDLE | 0x02 | 无效密钥句柄 |
| KEYM_E_PARAM_LENGTH | 0x03 | 无效长度 |
| KEYM_E_UNINIT | 0x04 | 模块未初始化 |
| KEYM_E_ALREADY_INITIALIZED | 0x05 | 重复初始化 |
| KEYM_E_INVALID_KEY | 0x06 | 无效密钥 ID |
| KEYM_E_INVALID_KEY_FORMAT | 0x07 | 无效密钥格式 |
| KEYM_E_INVALID_KEY_STATUS | 0x08 | 无效密钥状态 |
| KEYM_E_KEY_NOT_AVAILABLE | 0x09 | 密钥不可用 |

**预定义密钥 ID:**

| 密钥 ID | 名称 | 说明 |
|---------|------|------|
| 0 | KEYM_KEY_ID_MASTER | 主密钥 |
| 1 | KEYM_KEY_ID_AES_128 | AES-128 密钥 |
| 2 | KEYM_KEY_ID_AES_256 | AES-256 密钥 |
| 3 | KEYM_KEY_ID_HMAC_SHA256 | HMAC-SHA256 密钥 |
| 4 | KEYM_KEY_ID_RSA_2048 | RSA-2048 密钥 |
| 5 | KEYM_KEY_ID_ECC_P256 | ECC P-256 密钥 |
| 6 | KEYM_KEY_ID_SESSION | 会话密钥 |
| 7 | KEYM_KEY_ID_RESERVED | 保留 |

---

## 7. 处理流程

### 7.1 密钥设置流程

1. 参数校验（初始化状态、指针、密钥 ID、长度）
2. 进入排他区
3. 检查密钥是否被锁定
4. 拷贝密钥数据到 KeyM_Keys[keyId].keyData
5. 设置状态为 NEW，isValid = FALSE
6. 递增版本号
7. 设置有效期（validFrom = 当前时间）
8. 退出排他区

### 7.2 密钥最终化流程

1. 参数校验
2. 进入排他区
3. 检查密钥数据长度 > 0
4. 设置状态为 VALID，isValid = TRUE
5. 退出排他区
6. 触发通知回调

### 7.3 MainFunction 流程

1. 检查模块已初始化
2. 进入排他区
3. 递增系统时间计数器
4. 处理异步操作队列（完成 pending 操作并触发回调）
5. 检查所有有效密钥的过期状态
6. 退出排他区

---

## 8. 配置设计

### 8.1 预编译配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `KEYM_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `KEYM_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `KEYM_NUM_KEYS` | 8U | 密钥数量 |
| `KEYM_NUM_CERTIFICATES` | 4U | 证书数量 |
| `KEYM_MAX_KEY_ELEMENTS` | 8U | 每密钥最大元素数 |
| `KEYM_MAX_KEY_LENGTH` | 256U | 最大密钥长度 (字节) |
| `KEYM_MAX_CERT_SIZE` | 2048U | 最大证书大小 |
| `KEYM_OPERATION_QUEUE_SIZE` | 4U | 异步操作队列大小 |
| `KEYM_ASYNC_OPERATIONS` | STD_ON | 异步操作使能 |
| `KEYM_NVM_STORAGE` | STD_ON | NvM 持久化使能 |
| `KEYM_CERTIFICATE_MANAGEMENT` | STD_ON | 证书管理使能 |
| `KEYM_MAX_KEY_LIFETIME` | 31536000U | 最大密钥生命周期 (秒, =1年) |

### 8.2 链接时配置

通过配置工具生成密钥配置数组，定义每个密钥的类型、长度、用途。

### 8.3 构建后配置

不支持构建后配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 场景 | API | 错误码 |
|------|-----|--------|
| 重复初始化 | KeyM_Init | KEYM_E_ALREADY_INITIALIZED |
| NULL 指针 | 所有 API | KEYM_E_PARAM_POINTER |
| 模块未初始化 | 所有 API | KEYM_E_UNINIT |
| 无效密钥 ID | Set/Get/Update | KEYM_E_INVALID_KEY |
| 密钥长度超限 | Set/Update | KEYM_E_PARAM_LENGTH |
| 密钥被锁定时写入 | Set/Update | 返回 E_NOT_OK |

### 9.2 DEM 错误

KeyM 不直接报告 DEM 事件。密钥操作失败可通过 CSM 上报。

### 9.3 安全机制

- 密钥数据使用 XOR 混淆保护（KeyM_ProtectKeyData）
- 密钥操作使用 SchM 排他区保护（多核安全）
- 密钥版本递增机制防止版本回退
- 密钥有效期管理，过期自动标记为 INVALID
- DeInit 时清除所有密钥数据（安全擦除）
- 密钥锁定机制防止并发修改

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 段 | 变量 | 说明 |
|----|------|------|
| KEYM_START_SEC_VAR_CLEARED_UNSPECIFIED | KeyM_Keys[], KeyM_KeyElements[][], KeyM_OpQueue[] | 密钥运行时数据 |
| KEYM_START_SEC_CODE | 所有函数 | 代码段 |

### 10.2 资源估算

- **RAM**: KeyM_Keys[8] = 8 × (256+4+4+4+4+4+1+1+4) ≈ 8 × 282 = 2256 字节
  KeyM_KeyElements[8][8] = 64 × (256+4+1) ≈ 16640 字节
  总计 ≈ 19 KB
- **ROM**: ~6 KB（代码段）
- **性能**: SetKey/GetKey 为 O(N) N=密钥长度；MainFunction 为 O(K+Q) K=密钥数 Q=队列大小

---

## 11. 集成指南

- CSM 通过 `KeyM_GetKey()` 获取密钥材料用于密码运算
- SecOC 通过 `KeyM_GetKey()` 获取 MAC 密钥
- 诊断安全访问通过 `KeyM_KeyElementSet()` 设置安全访问密钥
- 密钥更新流程：SetKey → FinalizeKey（或 UpdateKey → FinalizeKey）
- SCHM 以适当周期调用 `KeyM_MainFunction()` 处理异步操作和过期检查

---

## 12. 测试策略

### 12.1 单元测试

- 初始化/反初始化测试
- 密钥生命周期转换测试（NEW → VALID, NEW → UPDATE → VALID）
- 密钥过期检测测试
- 密钥锁定/解锁测试
- 密钥元素设置/获取测试
- 密钥复制测试
- 版本号递增和回绕测试
- 异步操作队列测试

### 12.2 集成测试

- CSM → KeyM → CryIf 完整密钥获取链路
- NvM 密钥持久化和恢复测试
- 密钥过期对 SecOC 通信的影响

---

## 13. 实现说明 / TODO

- 密钥格式解析（DER/PEM/COSE/JWK）当前为占位实现，需补充完整的 ASN.1 解析
- XOR 混淆使用硬编码密钥，生产版本应使用硬件安全模块（HSM）保护
- KeyM_ProtectKeyData 中的 protectionKey 为编译时常量，安全性有限
- 异步操作队列当前为模拟完成，需对接实际 HSM 驱动

---

## 14. 参考资料

- AUTOSAR_SWS_KeyManager.pdf (R4.7.0)
- AUTOSAR_SWS_CryptoStackManager.pdf
- yuleASR KeyM 模块源码: `src/bsw/services/keym/`
