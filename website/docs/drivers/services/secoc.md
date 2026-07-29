---
title: SecOC - Secure Onboard Communication
sidebar_label: secoc
description: "SecOC (Secure Onboard Communication) 是AUTOSAR服务层安全通信模块，为车载网络通信提供认证保护。它防止未经授权的消息注入和重放攻击，确保ECU间通信的完整性和真实性。"
sidebar_position: 26
---

# SecOC - Secure Onboard Communication

## Overview

SecOC (Secure Onboard Communication) 是AUTOSAR服务层安全通信模块，为车载网络通信提供认证保护。它防止未经授权的消息注入和重放攻击，确保ECU间通信的完整性和真实性。

## Standards

- AUTOSAR SWS Secure Onboard Communication (R22-11)
- AUTOSAR SWS CSM (Crypto Services Manager)
- ISO 26262 - 功能安全标准
- ISO/SAE 21434 - 网络安全工程标准

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│                      (SW Components)                         │
└───────────────────────┬─────────────────────────────────────┘
                        │ RTE
┌───────────────────────▼─────────────────────────────────────┐
│                    Service Layer                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                 SecOC Module                          │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────────┐   │  │
│  │  │  TX PDU  │  │  RX PDU  │  │  Freshness Mgr   │   │  │
│  │  │ Handler  │  │ Handler  │  │                  │   │  │
│  │  └────┬─────┘  └────┬─────┘  └────────┬─────────┘   │  │
│  │       │             │                 │             │  │
│  │  ┌────▼─────────────▼─────────────────▼─────────┐   │  │
│  │  │          Authentication Engine               │   │  │
│  │  │  (MAC Generation / Verification)             │   │  │
│  │  └──────────────────┬───────────────────────────┘   │  │
│  └─────────────────────┼─────────────────────────────────┘  │
└────────────────────────┼────────────────────────────────────┘
                         │
              ┌──────────┴──────────┐
              │                     │
        ┌─────▼─────┐        ┌──────▼──────┐
        │    CSM    │        │    PduR     │
        │ (Crypto)  │        │  (Routing)  │
        └───────────┘        └─────────────┘
```

## Security Features

### 1. 认证保护 (Authentication)

| 算法 | 描述 | 配置选项 |
|------|------|----------|
| AES-CMAC | AES-based MAC | `SECOC_AES_MAC` |
| HMAC-SHA256 | HMAC with SHA256 | `SECOC_HMAC_SHA256` |
| HMAC-SHA512 | HMAC with SHA512 | `SECOC_HMAC_SHA512` |

### 2. 新鲜度值 (Freshness Value)

| 类型 | 描述 | 应用场景 |
|------|------|----------|
| Counter | 单调递增计数器 | 常规周期性消息 |
| Timestamp | 基于时间戳 | 非周期性事件消息 |

### 3. 安全PDU格式

```
Secured PDU Structure:
┌─────────────────────────────────────────────────────────────────┐
│  Data (N bytes)  │  Freshness (M bytes)  │  Auth Code (L bytes)│
│                  │    (truncated)        │                     │
└─────────────────────────────────────────────────────────────────┘
        ↑                                          ↑
   原始应用数据                            认证信息 (MAC)
```

## API Reference

### 初始化与反初始化

#### SecOC_Init
```c
void SecOC_Init(const SecOC_ConfigType* configPtr);
```
- **功能**: 初始化SecOC模块
- **参数**: `configPtr` - 配置结构体指针
- **错误码**: 
  - `SECOC_E_ALREADY_INITIALIZED` - 已初始化
  - `SECOC_E_PARAM_POINTER` - 空指针
- **需求**: SWS_SecOC_00001

#### SecOC_DeInit
```c
void SecOC_DeInit(void);
```
- **功能**: 反初始化SecOC模块
- **错误码**: 
  - `SECOC_E_UNINIT` - 未初始化
- **需求**: SWS_SecOC_00002

### 传输接口

#### SecOC_IfTransmit
```c
Std_ReturnType SecOC_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
```
- **功能**: 发送安全PDU
- **参数**:
  - `TxPduId` - 发送PDU ID
  - `PduInfoPtr` - PDU信息指针
- **返回**: `E_OK` 或 `E_NOT_OK`
- **错误码**:
  - `SECOC_E_UNINIT` - 未初始化
  - `SECOC_E_PARAM_POINTER` - 空指针
  - `SECOC_E_INVALID_PDU_SDU_ID` - 无效PDU ID
- **需求**: SWS_SecOC_00041

### 接收接口

#### SecOC_IfRxIndication
```c
void SecOC_IfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
```
- **功能**: 接收安全PDU指示
- **参数**:
  - `RxPduId` - 接收PDU ID
  - `PduInfoPtr` - PDU信息指针
- **错误码**:
  - `SECOC_E_UNINIT` - 未初始化
  - `SECOC_E_PARAM_POINTER` - 空指针
- **需求**: SWS_SecOC_00043

### 验证状态管理

#### SecOC_VerifyStatusOverride
```c
Std_ReturnType SecOC_VerifyStatusOverride(PduIdType PduId, SecOC_VerificationStatusType status);
```
- **功能**: 覆盖PDU验证状态
- **参数**:
  - `PduId` - PDU ID
  - `status` - 新验证状态
- **返回**: `E_OK` 或 `E_NOT_OK`
- **需求**: SWS_SecOC_00081

#### SecOC_GetVerificationStatus
```c
SecOC_VerificationStatusType SecOC_GetVerificationStatus(PduIdType PduId);
```
- **功能**: 获取PDU验证状态
- **参数**: `PduId` - PDU ID
- **返回**: 验证状态
- **需求**: SWS_SecOC_00084

#### SecOC_GetVerificationResult
```c
Std_ReturnType SecOC_GetVerificationResult(PduIdType PduId, SecOC_VerificationResultType* resultPtr);
```
- **功能**: 获取PDU验证结果
- **参数**:
  - `PduId` - PDU ID
  - `resultPtr` - 结果存储指针
- **返回**: `E_OK` 或 `E_NOT_OK`
- **需求**: SWS_SecOC_00085

### 周期处理函数

#### SecOC_MainFunctionRx
```c
void SecOC_MainFunctionRx(void);
```
- **功能**: RX主处理函数
- **调用周期**: 由配置决定 (默认10ms)
- **需求**: SWS_SecOC_00091

#### SecOC_MainFunctionTx
```c
void SecOC_MainFunctionTx(void);
```
- **功能**: TX主处理函数
- **调用周期**: 由配置决定 (默认10ms)
- **需求**: SWS_SecOC_00092

## Configuration

### 基础配置 (SecOC_Cfg.h)

```c
/* 开发错误检测 */
#define SECOC_DEV_ERROR_DETECT          STD_ON

/* 版本信息API */
#define SECOC_VERSION_INFO_API          STD_ON

/* PDU配置 */
#define SECOC_NUM_TX_PDUS               4
#define SECOC_NUM_RX_PDUS               4
#define SECOC_MAX_PDU_LENGTH            64

/* 认证配置 */
#define SECOC_AUTH_ALGORITHM            SECOC_HMAC_SHA256
#define SECOC_AUTH_INFO_LENGTH          16

/* 新鲜度值配置 */
#define SECOC_FRESHNESS_VALUE_LENGTH    32      /* bits */
#define SECOC_FRESHNESS_VALUE_TX_LENGTH 16      /* bits transmitted */
#define SECOC_FRESHNESS_RESET_THRESHOLD 0xF0000000

/* 验证配置 */
#define SECOC_VERIFICATION_RETRY_COUNT  3
#define SECOC_VERIFICATION_TIMEOUT_MS   100
```

### PDU配置示例

```c
const SecOC_PduConfigType TxPduConfigs[SECOC_NUM_TX_PDUS] = {
    {
        .pduId = 0,
        .lowerLayerPduId = 0,
        .pduType = SECOC_IFPDU,
        .authConfig = {
            .algorithm = SECOC_HMAC_SHA256,
            .authInfoLength = 16,
            .dataId = 0
        },
        .freshnessConfig = {
            .type = SECOC_COUNTER,
            .freshnessValueId = 0,
            .freshnessValueLength = 32,
            .freshnessValueTxLength = 16
        },
        .useCryptographicPdu = FALSE,
        .authPduLength = 64
    },
    /* ... more PDUs ... */
};
```

## 状态机

### 验证状态

```
                    ┌─────────────┐
         ┌─────────►│ UNVERIFIED  │◄────────┐
         │          │  (初始状态)  │         │
         │          └──────┬──────┘         │
         │                 │                │
         │                 ▼                │
         │    ┌────────────────────────┐    │
         │    │   VERIFICATIONSUCCESS  │    │
         │    │      (验证成功)         │    │
         │    └────────────────────────┘    │
         │                 │                │
         │                 │ Override       │
         │                 ▼                │
         │    ┌────────────────────────┐    │
         └────┤   VERIFICATIONFAILURE  ├────┘
              │      (验证失败)         │
              └────────────────────────┘
                         │
                         │ Override
                         ▼
              ┌────────────────────────┐
              │   VERIFICATIONOVERRIDE │
              │      (状态覆盖)         │
              └────────────────────────┘
```

## 错误处理

### 开发错误码

| 错误码 | 值 | 描述 |
|--------|-----|------|
| `SECOC_E_PARAM_POINTER` | 0x01 | API调用时空指针 |
| `SECOC_E_INVALID_PDU_SDU_ID` | 0x02 | 无效PDU/SDU ID |
| `SECOC_E_INVALID_PARAMETER` | 0x03 | 无效参数 |
| `SECOC_E_UNINIT` | 0x04 | 未初始化调用API |
| `SECOC_E_ALREADY_INITIALIZED` | 0x05 | 重复初始化 |
| `SECOC_E_CRYPTO_FAILURE` | 0x06 | 加密操作失败 |

### 运行时错误码

| 错误码 | 值 | 描述 |
|--------|-----|------|
| `SECOC_E_CRYPTO_AUTH_FAILED` | 0x01 | 认证验证失败 |
| `SECOC_E_FRESHNESS_FAILURE` | 0x02 | 新鲜度值验证失败 |
| `SECOC_E_SEC_PAYLOAD_ERROR` | 0x03 | 安全载荷错误 |
| `SECOC_E_BUSY` | 0x04 | 模块忙 |

## 单元测试

### 测试覆盖

| 模块 | 测试文件 | 用例数 | 覆盖率 |
|------|----------|--------|--------|
| SecOC | `tests/unit/autosar/services/test_secoc.c` | 35+ | 80%+ |

### 测试类别

1. **初始化测试** (5个用例)
   - 正常初始化/反初始化
   - 重复初始化检测
   - 空指针处理

2. **传输测试** (5个用例)
   - 正常传输
   - 参数验证
   - CSM故障处理

3. **接收测试** (5个用例)
   - 正常接收
   - 验证失败检测
   - PDU长度验证

4. **验证状态测试** (6个用例)
   - 状态覆盖
   - 结果查询
   - 错误处理

5. **主函数测试** (4个用例)
   - 周期处理
   - 未初始化错误

6. **边界测试** (10+个用例)
   - 最大/最小PDU ID
   - 零长度PDU
   - 多PDU配置

### 运行测试

```bash
# 编译测试
cd /home/admin/yuleASR
gcc -I./src/bsw/services/secoc/include \
    -I./src/bsw/platform/include \
    -I./third_party/cmocka/include \
    -DUNIT_TEST \
    tests/unit/autosar/services/test_secoc.c \
    -lcmocka -o test_secoc

# 运行测试
./test_secoc
```

## 依赖模块

| 模块 | 用途 |
|------|------|
| CSM | 加密服务 (MAC生成/验证) |
| PduR | PDU路由 |
| Det | 错误报告 |
| SchM | 调度保护 |

## 使用示例

### 发送安全消息

```c
/* 初始化 */
SecOC_Init(&SecOC_Config);

/* 准备数据 */
uint8 appData[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
PduInfoType pduInfo;
pduInfo.SduDataPtr = appData;
pduInfo.SduLength = sizeof(appData);
pduInfo.MetaDataPtr = NULL;

/* 发送安全PDU */
Std_ReturnType result = SecOC_IfTransmit(SECOC_TX_PDU_ID_0, &pduInfo);
if (result == E_OK) {
    /* 发送成功 */
}
```

### 接收安全消息

```c
/* RxIndication回调 - 由PduR调用 */
void SecOC_IfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    /* SecOC自动验证认证码 */
    /* 验证通过后转发到上层 */
}

/* 检查验证状态 */
SecOC_VerificationStatusType status = SecOC_GetVerificationStatus(RxPduId);
if (status == SECOC_VERIFICATIONSUCCESS_STATUS) {
    /* 消息验证成功 */
}
```

## 性能指标

| 指标 | 典型值 | 最大值 |
|------|--------|--------|
| 初始化时间 | &lt; 1ms | 5ms |
| 认证生成时间 | &lt; 2ms | 5ms |
| 认证验证时间 | &lt; 2ms | 5ms |
| 内存占用 (RAM) | 2KB | 4KB |
| 内存占用 (ROM) | 8KB | 16KB |

## 故障排查

### 常见问题

| 问题 | 可能原因 | 解决方案 |
|------|----------|----------|
| 验证持续失败 | 新鲜度值不同步 | 检查Freshness Manager同步 |
| CSM返回错误 | 密钥未配置 | 检查CSM密钥配置 |
| PDU丢失 | 缓冲区不足 | 增加PDU缓冲区大小 |
| 延迟过高 | 加密运算慢 | 优化CSM配置或降低PDU频率 |

## 参考资料

- [AUTOSAR SWS Secure Onboard Communication](https://www.autosar.org/standards/)
- [AUTOSAR SWS CSM](https://www.autosar.org/standards/)
- [ISO 26262 - Road vehicles - Functional safety](https://www.iso.org/standard/68383.html)
- [ISO/SAE 21434 - Road vehicles - Cybersecurity engineering](https://www.iso.org/standard/70918.html)
