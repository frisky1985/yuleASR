---
title: E2E (End-to-End) Protection 模块
sidebar_label: e2e
description: "- [支持的Profile](#支持的profile)"
sidebar_position: 12
---

# E2E (End-to-End) Protection 模块

## 目录

- [概述](#概述)
- [功能特性](#功能特性)
- [架构设计](#架构设计)
- [支持的Profile](#支持的profile)
- [API参考](#api参考)
- [配置参数](#配置参数)
- [使用示例](#使用示例)
- [状态机](#状态机)
- [错误处理](#错误处理)
- [测试覆盖](#测试覆盖)

---

## 概述

E2E (End-to-End) Protection 模块是AutoSAR服务层的重要组成部分，为车载通信提供端到端的数据保护。该模块确保数据在传输过程中的完整性、真实性和时效性，满足ASIL-D级别的功能安全要求。

**版本信息**:
- AUTOSAR版本: R22-11
- 供应商ID: 0x0001
- 模块ID: 0xF0
- 软件版本: 1.0.0

---

## 功能特性

### 核心功能

| 功能 | 描述 | ASIL级别 |
|------|------|----------|
| CRC校验 | 检测数据传输错误 | D |
| 序列计数器 | 检测消息丢失、重复和乱序 | D |
| DataID | 防止消息错误路由 | D |
| 数据长度验证 | 检测长度篡改 (Profile 6/7) | D |
| 状态机管理 | E2E通信状态跟踪 | D |

### 支持的保护级别

| Profile | CRC类型 | 计数器 | 动态长度 | 典型应用 |
|---------|---------|--------|----------|----------|
| Profile 01 | CRC8 | 4-bit | 否 | 小数据量控制信号 |
| Profile 02 | CRC8 | 8-bit | 否 | 双冗余路径通信 |
| Profile 04 | CRC32 | 16-bit | 否 | 中等数据量通信 |
| Profile 05 | CRC64 | 32-bit | 否 | 高完整性要求 |
| Profile 06 | CRC64 | 16-bit | 是 | 变长数据保护 |
| Profile 07 | CRC32 | 8-bit | 是 | 变长数据保护 |

---

## 架构设计

### 模块架构

```
┌─────────────────────────────────────────────────────────────┐
│                      E2E Library (E2E.c)                     │
│                    ┌──────────────────┐                     │
│                    │  E2E_Init()       │                     │
│                    │  E2E_DeInit()     │                     │
│                    └──────────────────┘                     │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
┌──────────────┐   ┌──────────────┐   ┌──────────────┐
│  Profile 01  │   │  Profile 02  │   │  Profile 04  │
│  (CRC8)      │   │  (CRC8 Dual) │   │  (CRC32)     │
│              │   │              │   │              │
│ E2E_P01.c    │   │ E2E_P02.c    │   │ E2E_P04.c    │
│ E2E_P01.h    │   │ E2E_P02.h    │   │ E2E_P04.h    │
└──────────────┘   └──────────────┘   └──────────────┘
                                              │
┌──────────────┐   ┌──────────────┐          │
│  Profile 05  │   │  Profile 06  │          │
│  (CRC64)     │   │  (CRC64 Var) │          │
│              │   │              │          │
│ E2E_P05.c    │   │ E2E_P06.c    │          │
│ E2E_P05.h    │   │ E2E_P06.h    │          │
└──────────────┘   └──────────────┘          │
                                             ▼
                                    ┌──────────────┐
                                    │  Profile 07  │
                                    │  (CRC32 Var) │
                                    │              │
                                    │ E2E_P07.c    │
                                    │ E2E_P07.h    │
                                    └──────────────┘
```

### 文件结构

```
src/bsw/services/e2e/
├── include/
│   ├── E2E.h              # 主头文件
│   ├── E2E_Cfg.h          # 配置文件
│   ├── E2E_P01.h          # Profile 1
│   ├── E2E_P02.h          # Profile 2
│   ├── E2E_P04.h          # Profile 4
│   ├── E2E_P05.h          # Profile 5
│   ├── E2E_P06.h          # Profile 6
│   └── E2E_P07.h          # Profile 7
└── src/
    ├── E2E.c              # 主实现
    ├── E2E_P01.c          # Profile 1实现
    ├── E2E_P02.c          # Profile 2实现
    ├── E2E_P04.c          # Profile 4实现
    ├── E2E_P05.c          # Profile 5实现
    ├── E2E_P06.c          # Profile 6实现
    ├── E2E_P07.c          # Profile 7实现
    └── E2E_Lcfg.c         # 链接时配置
```

---

## 支持的Profile

### Profile 01: CRC8 + 4-bit Counter

适用于小数据量、低延迟的控制信号传输。

**特性**:
- CRC8 (多项式 0x1D)
- 4-bit 序列计数器
- 支持多种DataID模式
- 最大数据长度: 30字节

**DataID模式**:
| 模式 | 值 | 描述 |
|------|-----|------|
| E2E_P01_DATAID_BOTH | 0x00 | 双字节DataID包含在CRC计算中 |
| E2E_P01_DATAID_ALT | 0x01 | DataID高低字节交替 |
| E2E_P01_DATAID_LOW | 0x02 | 仅低字节DataID |
| E2E_P01_DATAID_NIBBLE | 0x03 | DataID半字节模式 |

### Profile 02: CRC8 + 8-bit Counter + Dual Path

支持双路径冗余传输，提高通信可靠性。

**特性**:
- CRC8 (多项式 0x2F)
- 8-bit 序列计数器
- 双路径支持
- 路径交替机制

### Profile 04: CRC32 + 16-bit Counter

适用于中等数据量的高性能通信。

**特性**:
- CRC32 (Ethernet多项式 0x04C11DB7)
- 16-bit 序列计数器
- 最大数据长度: 4096字节
- 可选DataID包含

### Profile 05: CRC64 + 32-bit Counter

提供最高级别的数据完整性保护。

**特性**:
- CRC64 (ECMA多项式 0x42F0E1EBA9EA3693)
- 32-bit 序列计数器
- 最大数据长度: 4096字节
- 可选DataID包含

### Profile 06: CRC64 + 16-bit Counter + 动态长度

支持变长数据的完整性保护。

**特性**:
- CRC64 + 长度字段验证
- 16-bit 序列计数器
- 支持32-128字节动态长度
- 长度包含在CRC计算中

### Profile 07: CRC32 + 8-bit Counter + 动态长度

Profile 4的变长版本，适用于中等变长数据。

**特性**:
- CRC32 + 长度字段验证
- 8-bit 序列计数器
- 支持16-64字节动态长度

---

## API参考

### E2E库API

#### E2E_Init
```c
Std_ReturnType E2E_Init(const void* ConfigPtr);
```
初始化E2E模块。

**参数**:
- `ConfigPtr`: 配置指针 (可为NULL)

**返回**:
- `E_OK`: 初始化成功

#### E2E_DeInit
```c
Std_ReturnType E2E_DeInit(void);
```
反初始化E2E模块。

**返回**:
- `E_OK`: 反初始化成功

### Profile 01 API

#### E2E_P01Protect
```c
Std_ReturnType E2E_P01Protect(
    const E2E_P01ConfigType* Config,
    E2E_P01ProtectStateType* State,
    uint8* Data
);
```
为数据添加E2E保护。

**参数**:
- `Config`: Profile 01配置
- `State`: 保护状态 (计数器)
- `Data`: 待保护数据缓冲区

**返回**:
- `E_OK`: 保护成功
- `E_NOT_OK`: 参数错误

#### E2E_P01Check
```c
Std_ReturnType E2E_P01Check(
    const E2E_P01ConfigType* Config,
    E2E_P01CheckStateType* State,
    const uint8* Data
);
```
验证受保护数据的完整性。

**参数**:
- `Config`: Profile 01配置
- `State`: 检查状态
- `Data`: 待检查数据

**返回**:
- `E_OK`: 检查完成

### Profile 02 API

#### E2E_P02Protect
```c
Std_ReturnType E2E_P02Protect(
    const E2E_P02ConfigType* Config,
    E2E_P02ProtectStateType* State,
    uint8* Data
);
```

#### E2E_P02Check
```c
Std_ReturnType E2E_P02Check(
    const E2E_P02ConfigType* Config,
    E2E_P02CheckStateType* State,
    const uint8* Data,
    uint8 PathId
);
```

### Profile 04 API

#### E2E_P04Protect
```c
Std_ReturnType E2E_P04Protect(
    const E2E_P04ConfigType* Config,
    E2E_P04ProtectStateType* State,
    uint8* Data
);
```

#### E2E_P04Check
```c
Std_ReturnType E2E_P04Check(
    const E2E_P04ConfigType* Config,
    E2E_P04CheckStateType* State,
    const uint8* Data
);
```

### Profile 05 API

#### E2E_P05Protect
```c
Std_ReturnType E2E_P05Protect(
    const E2E_P05ConfigType* Config,
    E2E_P05ProtectStateType* State,
    uint8* Data
);
```

#### E2E_P05Check
```c
Std_ReturnType E2E_P05Check(
    const E2E_P05ConfigType* Config,
    E2E_P05CheckStateType* State,
    const uint8* Data
);
```

### Profile 06 API

#### E2E_P06Protect
```c
Std_ReturnType E2E_P06Protect(
    const E2E_P06ConfigType* Config,
    E2E_P06ProtectStateType* State,
    uint8* Data,
    uint32 Length
);
```

#### E2E_P06Check
```c
Std_ReturnType E2E_P06Check(
    const E2E_P06ConfigType* Config,
    E2E_P06CheckStateType* State,
    const uint8* Data,
    uint32 Length
);
```

### Profile 07 API

#### E2E_P07Protect
```c
Std_ReturnType E2E_P07Protect(
    const E2E_P07ConfigType* Config,
    E2E_P07ProtectStateType* State,
    uint8* Data,
    uint32 Length
);
```

#### E2E_P07Check
```c
Std_ReturnType E2E_P07Check(
    const E2E_P07ConfigType* Config,
    E2E_P07CheckStateType* State,
    const uint8* Data,
    uint32 Length
);
```

### 状态映射API

每个Profile提供状态映射函数，将检查结果转换为状态机状态:

```c
void E2E_P01MapStatusToSM(
    E2E_PCheckStatusType CheckStatus,
    E2E_SMStateType* SMState,
    boolean* Error
);
```

---

## 配置参数

### 配置文件 (E2E_Cfg.h)

```c
/* Profile使能 */
#define E2E_PROFILE_01_ENABLED    STD_ON
#define E2E_PROFILE_02_ENABLED    STD_ON
#define E2E_PROFILE_04_ENABLED    STD_ON
#define E2E_PROFILE_05_ENABLED    STD_ON
#define E2E_PROFILE_06_ENABLED    STD_ON
#define E2E_PROFILE_07_ENABLED    STD_ON

/* CRC配置 */
#define E2E_USE_CRC_HARDWARE      STD_OFF
#define E2E_USE_CRC_SOFTWARE      STD_ON
#define E2E_CRC_TABLE_OPTIMIZED   STD_ON

/* 开发错误检测 */
#define E2E_DEV_ERROR_DETECT      STD_ON

/* 版本信息API */
#define E2E_VERSION_INFO_API      STD_ON

/* 最大数据长度 */
#define E2E_MAX_DATA_LENGTH_P01   30U
#define E2E_MAX_DATA_LENGTH_P02   256U
#define E2E_MAX_DATA_LENGTH_P04   4096U
#define E2E_MAX_DATA_LENGTH_P05   4096U
#define E2E_MAX_DATA_LENGTH_P06   4096U
#define E2E_MAX_DATA_LENGTH_P07   4096U

/* 状态机配置 */
#define E2E_SM_MAX_ERROR_WINDOW   15U
#define E2E_SM_MAX_SYNC_STEPS     2U
#define E2E_SM_MIN_OK_COUNT       2U
```

### Profile 01配置结构

```c
typedef struct {
    uint16 DataID;              /* 数据标识符 */
    uint16 DataLength;          /* 数据长度 */
    uint8 DataIDMode;           /* DataID模式 */
    uint8 CounterOffset;        /* 计数器字节偏移 */
    uint8 CRCOffset;            /* CRC字节偏移 */
    uint8 DataIDNibbleOffset;   /* DataID半字节偏移 */
} E2E_P01ConfigType;
```

---

## 使用示例

### Profile 01 基本用法

```c
#include "E2E.h"
#include "E2E_P01.h"

/* 发送端 */
void E2E_TxExample(void)
{
    E2E_P01ConfigType config = {
        .DataID = 0x1234,
        .DataLength = 16,
        .DataIDMode = E2E_P01_DATAID_BOTH,
        .CounterOffset = 1,
        .CRCOffset = 0
    };
    
    E2E_P01ProtectStateType txState = {0};
    uint8 data[16] = {0};
    
    /* 准备数据 */
    data[2] = 0x55;  /* 应用数据 */
    
    /* 添加E2E保护 */
    Std_ReturnType result = E2E_P01Protect(&config, &txState, data);
    if (result == E_OK) {
        /* 发送数据 */
        SendData(data, sizeof(data));
    }
}

/* 接收端 */
void E2E_RxExample(void)
{
    E2E_P01ConfigType config = {
        .DataID = 0x1234,
        .DataLength = 16,
        .DataIDMode = E2E_P01_DATAID_BOTH,
        .CounterOffset = 1,
        .CRCOffset = 0
    };
    
    E2E_P01CheckStateType rxState = {0};
    rxState.MaxDeltaCounterInit = 3;
    
    uint8 data[16];
    
    /* 接收数据 */
    ReceiveData(data, sizeof(data));
    
    /* 验证E2E保护 */
    Std_ReturnType result = E2E_P01Check(&config, &rxState, data);
    if (result == E_OK) {
        switch (rxState.Status) {
            case E2E_P_OK:
                /* 数据有效，处理 */
                ProcessData(data);
                break;
            case E2E_P_WRONGCRC:
                /* CRC错误，丢弃 */
                HandleCRCError();
                break;
            case E2E_P_SYNC:
                /* 需要同步 */
                HandleSyncRequired();
                break;
            default:
                break;
        }
    }
}
```

### Profile 06 变长数据用法

```c
#include "E2E_P06.h"

void E2E_VarLengthExample(uint8* data, uint32 length)
{
    E2E_P06ConfigType config = {
        .DataID = 0xDEADBEEF,
        .CounterOffset = 0,
        .CRCOffset = 16,
        .LengthOffset = 80,
        .MaxDeltaCounterInit = 3,
        .MinDataLength = 32,
        .MaxDataLength = 128,
        .IncludeDataID = TRUE
    };
    
    E2E_P06ProtectStateType txState = {0};
    
    /* 添加E2E保护 (包含长度验证) */
    Std_ReturnType result = E2E_P06Protect(&config, &txState, data, length);
    if (result == E_OK) {
        SendData(data, length);
    }
}
```

---

## 状态机

### 检查状态 (E2E_PCheckStatusType)

| 状态 | 描述 | 含义 |
|------|------|------|
| E2E_P_OK | 正常 | 数据有效且连续 |
| E2E_P_NONEWDATA | 无新数据 | 未接收到新数据 |
| E2E_P_WRONGCRC | CRC错误 | 数据完整性校验失败 |
| E2E_P_SYNC | 需要同步 | 检测到过多消息丢失 |
| E2E_P_INITIAL | 初始 | 首次接收到数据 |
| E2E_P_REPEATED | 重复 | 收到重复消息 |
| E2E_P_OKSOMELOST | 正常(有丢失) | 数据有效但丢失部分消息 |
| E2E_P_WRONGSEQUENCE | 序列错误 | 消息顺序错乱 |

### 状态机状态 (E2E_SMStateType)

```
         ┌─────────────┐
         │ E2E_SM_DEINIT│
         └──────┬──────┘
                │ E2E_Init()
                ▼
         ┌─────────────┐
         │ E2E_SM_NODATA│◄──────────┐
         └──────┬──────┘            │
                │ 首次数据           │
                ▼                   │
         ┌─────────────┐     E2E_P_SYNC
         │ E2E_SM_INIT │◄──────────┤
         └──────┬──────┘            │
                │ 连续有效           │
                ▼                   │
         ┌─────────────┐            │
         │ E2E_SM_VALID│            │
         └──────┬──────┘            │
                │ 错误              │
                ▼                   │
         ┌─────────────┐            │
         │E2E_SM_INVALID│───────────┘
         └─────────────┘  恢复
```

### 状态映射表

| 检查状态 | 状态机状态 | Error标志 |
|----------|-----------|-----------|
| E2E_P_OK | VALID | FALSE |
| E2E_P_OKSOMELOST | VALID | FALSE |
| E2E_P_WRONGCRC | INVALID | TRUE |
| E2E_P_WRONGSEQUENCE | INVALID | TRUE |
| E2E_P_REPEATED | INVALID | TRUE |
| E2E_P_SYNC | INIT | FALSE |
| E2E_P_INITIAL | NODATA | FALSE |
| E2E_P_NONEWDATA | INVALID | FALSE |

---

## 错误处理

### 错误代码

| 错误码 | 值 | 描述 |
|--------|-----|------|
| E2E_E_OK | 0x00 | 成功 |
| E2E_E_NOT_OK | 0x01 | 通用错误 |
| E2E_E_INPUTERR_NULL | 0x13 | 空指针参数 |
| E2E_E_INPUTERR_WRONG | 0x15 | 错误参数 |
| E2E_E_INTERR | 0x19 | 内部错误 |
| E2E_E_OK_SOMELOST | 0x26 | 成功但有丢失 |

### 错误处理策略

1. **空指针检查**: 所有API检查配置、状态和数据指针是否为NULL
2. **参数验证**: 验证数据长度、偏移量等参数有效性
3. **CRC错误**: 检测到CRC错误时返回E2E_P_WRONGCRC状态
4. **序列错误**: 检测到计数器异常时返回相应状态

---

## 测试覆盖

### 测试文件

- 测试路径: `tests/unit/autosar/services/test_e2e.c`
- 测试框架: cmocka
- 覆盖率目标: &gt;80%

### 测试用例分布

| 测试类别 | 用例数 | 覆盖内容 |
|----------|--------|----------|
| E2E Library | 3 | 初始化、反初始化、版本信息 |
| Profile 01 | 11 | 保护、检查、各种状态 |
| Profile 02 | 6 | 双路径、CRC验证 |
| Profile 04 | 6 | CRC32、16位计数器 |
| Profile 05 | 6 | CRC64、32位计数器 |
| Profile 06 | 7 | 动态长度、CRC64 |
| Profile 07 | 8 | 动态长度、CRC32 |
| 错误处理 | 1 | 错误代码验证 |
| **总计** | **48+** | |

### 运行测试

```bash
# 编译测试
cd tests/unit/autosar/services
gcc -o test_e2e test_e2e.c -I../../../../src/bsw/services/e2e/include -I../../../../src/bsw/services/crc/include -I../../../../src/common -lcmocka

# 运行测试
./test_e2e
```

---

## 性能指标

| Profile | CRC计算 | 内存占用(代码) | 内存占用(数据) | 延迟 |
|---------|---------|----------------|----------------|------|
| P01 | 表查找 | ~1KB | ~10B | &lt;1us |
| P02 | 表查找 | ~1KB | ~15B | &lt;1us |
| P04 | 表查找 | ~4KB | ~20B | &lt;5us |
| P05 | Crc库 | ~2KB | ~30B | &lt;10us |
| P06 | Crc库 | ~2KB | ~35B | &lt;15us |
| P07 | Crc库 | ~1KB | ~25B | &lt;10us |

---

## 参考文档

- AUTOSAR SWS E2E Library, R22-11
- AUTOSAR Specification of CRC Routines
- ISO 26262:2018 Road vehicles - Functional safety

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0.0 | 2025-05 | 初始版本，支持Profile 1/2/4/5/6/7 |