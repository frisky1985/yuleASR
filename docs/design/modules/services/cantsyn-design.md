# CanTSyn (CAN Time Synchronization) Design Document

> **Module ID**: 0xA4  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_CANTimeSynchronization  
> **Source Path**: `src/bsw/services/cantsyn/`  
> **Reference Document**: `docs/modules/cantsyn.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

CanTSyn 模块负责通过 CAN 总线实现分布式汽车系统中的全局时间同步。该模块支持 Time Master（时间主节点）和 Time Slave（时间从节点）两种工作模式，提供微秒级的同步精度。

主要功能：
- **Time Master**：周期性发送 SYNC 和 FUP 消息，广播全局时间
- **Time Slave**：接收 SYNC/FUP 消息，调整本地时间
- **OCS（Offset Correction Scale）**：支持可选的用户数据和时间偏移校正
- **StbM 集成**：与 Synchronized Time-base Manager 协同工作，管理多个时间域
- **CRC 校验**：可选的消息 CRC 安全机制
- **去抖动处理**：防止时间同步中的瞬态干扰

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS CAN Time Synchronization | R22-11 | 模块软件规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | StbM | 提供全局时间基准，调用 StbM_SetGlobalTime / StbM_GetCurrentTime |
| 同层 | CanIf | CAN 接口层，发送/接收 SYNC/FUP/OCS 消息 |
| 公共 | Det | 开发错误检测与报告 |
| 公共 | Os | 操作系统服务 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│         StbM / Application          │
├─────────────────────────────────────┤
│         CanTSyn (Services)          │
├─────────────────────────────────────┤
│         CanIf (CAN Driver)          │
├─────────────────────────────────────┤
│         CanTrcv / Can               │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **时间域管理器**：管理多个时间域（最多 CANTSYN_NUMBER_OF_TIME_DOMAINS = 2），每个域独立维护序列计数器、时间戳和用户数据
- **SYNC 消息处理器**：构建和解析 SYNC 消息（16 字节），包含纳秒/秒级时间戳
- **OFS 消息处理器**：构建和解析偏移校正消息（12 字节）
- **传输状态机**：管理 TX 总线状态（IDLE/BUSY），确保消息有序发送
- **StbM 接口层**：通过 StbM 获取和设置全局时间

### 3.3 文件结构

```
src/bsw/services/cantsyn/
├── include/
│   ├── CanTSyn.h          -- 公共 API 与类型定义
│   └── CanTSyn_Cfg.h      -- 预编译配置参数
└── src/
    ├── CanTSyn.c           -- 核心实现
    └── CanTSyn_Lcfg.c      -- 链接时配置数据
```

---

## 4. 状态机

### 4.1 模块状态

```
CANTSYN_STATE_UNINIT -- CanTSyn_Init() --> CANTSYN_STATE_INIT
CANTSYN_STATE_INIT -- CanTSyn_DeInit() --> CANTSYN_STATE_UNINIT
```

模块仅有两种状态：未初始化（UNINIT）和已初始化（INIT）。所有 API 调用在 UNINIT 状态下将触发 DET 错误 `CANTSYN_E_UNINIT`。

### 4.2 传输状态

```
CANTSYN_TX_IDLE -- CanIf_Transmit() 成功 --> CANTSYN_TX_BUSY
CANTSYN_TX_BUSY -- TxConfirmation(E_OK) --> CANTSYN_TX_IDLE
```

传输状态机确保同一时刻只有一个 SYNC/FUP/OCS 消息在总线上。

### 4.3 时间域状态

每个时间域维护以下运行时信息：
- `TimeDomainId`：域标识（0 ~ NUMBER_OF_TIME_DOMAINS-1）
- `SequenceCounter`：序列计数器（0~15 循环）
- `TxTimeStamp` / `RxTimeStamp`：发送/接收时间戳
- `UserData[3]`：3 字节用户数据缓存
- `TimeBaseStatus`：时间基准状态标志

---

## 5. 核心数据结构

### 5.1 配置类型

```c
/* 时间基准配置 */
typedef struct {
    uint8 timeBaseId;           /* 时间基准 ID */
    uint8 domainId;             /* 时间域 ID */
    uint8 masterConfig;         /* 主节点配置 (0:None, 1:Slave, 2:Master) */
    boolean IsTimeMaster;       /* 是否作为时间主节点 */
    PduIdType TxPduId;          /* SYNC 消息发送 PDU ID */
    uint32 syncPeriodMs;        /* SYNC 周期 (ms) */
    uint32 debounceTimeMs;      /* 去抖时间 (ms) */
    uint32 syncTimeoutMs;       /* 同步超时 (ms) */
    boolean crcSecured;         /* CRC 校验使能 */
    boolean useImmediateTransmission; /* 即时传输模式 */
    uint32 syncCanId;           /* SYNC CAN ID */
    uint32 fupCanId;            /* FUP CAN ID */
    uint32 ocsCanId;            /* OCS CAN ID */
    PduIdType syncTxPduId;      /* SYNC TX PDU */
    PduIdType fupTxPduId;       /* FUP TX PDU */
    PduIdType ocsTxPduId;       /* OCS TX PDU */
    PduIdType syncRxPduId;      /* SYNC RX PDU */
    PduIdType fupRxPduId;       /* FUP RX PDU */
    PduIdType ocsRxPduId;       /* OCS RX PDU */
} CanTSyn_TimeBaseConfigType;

/* 全局配置 */
typedef struct {
    const CanTSyn_TimeBaseConfigType* timeBaseConfigs;
    const CanTSyn_SlaveConfigType* slaveConfigs;
    const CanTSyn_MasterConfigType* masterConfigs;
    uint8 numTimeBases;
    uint8 numSlaves;
    uint8 numMasters;
    boolean devErrorDetect;
    boolean versionInfoApi;
} CanTSyn_ConfigType;
```

### 5.2 消息数据结构

```c
/* SYNC/FUP/OCS 消息 */
typedef struct {
    uint8 type;                  /* 消息类型 */
    uint8 sequenceCounter;       /* 序列计数器 */
    uint8 messageCounter;        /* 消息计数器 */
    uint8 sgw;                   /* 同步网关信息 */
    uint8 ofs;                   /* 偏移信息 */
    StbM_TimeStampType timeStamp; /* 时间戳 */
    StbM_UserDataType userData;  /* 用户数据 (OCS) */
    uint8 crc;                   /* CRC (可选) */
} CanTSyn_MessageType;
```

### 5.3 内部运行时结构

```c
typedef struct {
    CanTSyn_StateType       State;      /* 模块状态 */
    CanTSyn_TxStateType     TxState;    /* 传输状态 */
    uint32                  TxCounter;  /* 发送计数 */
    uint32                  RxCounter;  /* 接收计数 */
    StbM_SynchronizedTimeBaseType TimeBaseRef; /* 时间基准引用 */
} CanTSyn_InternalType;
```

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `CanTSyn_Init(ConfigPtr)` | 0x01 | 初始化模块，配置时间域 |
| `CanTSyn_DeInit()` | 0x02 | 反初始化模块 |
| `CanTSyn_GetVersionInfo(versioninfo)` | 0x03 | 获取版本信息 |
| `CanTSyn_SetTransmissionMode(timeBaseId, txMode)` | 0x04 | 设置传输模式 |
| `CanTSyn_GetSyncReceived(timeBaseId)` | 0x05 | 查询同步接收状态 |
| `CanTSyn_GetCurrentVirtualTime(timeBaseId, virtualTimePtr)` | 0x07 | 获取虚拟时间 |
| `CanTSyn_SetGlobalTime(timeBaseId, timeStampPtr, userDataPtr)` | 0x08 | 设置全局时间 (Master) |
| `CanTSyn_SetRateCorrection(timeBaseId, rateCorrection)` | 0x09 | 设置速率校正 (ppm) |
| `CanTSyn_SetUserData(timeBaseId, userDataPtr)` | 0x0D | 设置用户数据 |
| `CanTSyn_GetUserData(timeBaseId, userDataPtr)` | 0x0E | 获取用户数据 |
| `CanTSyn_MainFunction()` | 0x0C | 周期处理函数 |

### 6.2 回调函数

| 函数 | SID | 说明 |
|------|-----|------|
| `CanTSyn_RxIndication(RxPduId, PduInfoPtr)` | 0x0A | CAN 接收指示 |
| `CanTSyn_TxConfirmation(TxPduId, result)` | 0x0B | CAN 发送确认 |
| `CanTSyn_TimeTxConfirmationSYNC(TxPduId, timeStampPtr)` | 0x10 | SYNC 硬件时间戳确认 |
| `CanTSyn_TimeTxConfirmationFUP(TxPduId, timeStampPtr)` | 0x11 | FUP 硬件时间戳确认 |
| `CanTSyn_TimeTxConfirmationOCS(TxPduId, timeStampPtr)` | 0x12 | OCS 硬件时间戳确认 |

### 6.3 服务 ID 与错误码

**DET 错误码：**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `CANTSYN_E_NO_ERROR` | 0x00 | 无错误 |
| `CANTSYN_E_PARAM_POINTER` | 0x01 | 空指针参数 |
| `CANTSYN_E_PARAM_CONFIG` | 0x02 | 配置参数错误 |
| `CANTSYN_E_UNINIT` | 0x03 | 模块未初始化 |
| `CANTSYN_E_ALREADY_INITIALIZED` | 0x04 | 重复初始化 |
| `CANTSYN_E_INVALID_TIMEBASE_ID` | 0x05 | 无效时间基准 ID |
| `CANTSYN_E_INVALID_DOMAIN_ID` | 0x06 | 无效时间域 ID |
| `CANTSYN_E_INVALID_PDU_ID` | 0x07 | 无效 PDU ID |
| `CANTSYN_E_INVALID_CAN_ID` | 0x08 | 无效 CAN ID |
| `CANTSYN_E_INVALID_DLC` | 0x09 | 无效 DLC |
| `CANTSYN_E_SYNC_LOST` | 0x0A | 同步丢失 |
| `CANTSYN_E_TIME_NOT_AVAILABLE` | 0x0B | 时间不可用 |
| `CANTSYN_E_TRANSMISSION_FAILED` | 0x0C | 传输失败 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 `ConfigPtr` 是否为空指针
2. 检查是否已初始化（防止重复初始化）
3. 遍历所有时间域，初始化序列计数器、用户数据、状态标志
4. 设置模块状态为 `CANTSYN_STATE_INIT`
5. 重置传输状态为 `CANTSYN_TX_IDLE`

### 7.2 SYNC 消息发送流程（Master 模式）

1. `CanTSyn_MainFunction()` 周期调用
2. 遍历所有时间域，检查 `IsTimeMaster == TRUE`
3. 从 StbM 获取当前时间 (`StbM_GetCurrentTime`)
4. 调用 `CanTSyn_PrepareSyncMessage()` 构建 SYNC 消息：
   - Byte 0: 消息类型 (0x10) + 时间域 ID
   - Byte 1-4: 纳秒（32-bit 大端）
   - Byte 5-8: 秒（32-bit 大端）
   - Byte 9-11: 用户数据 + 序列计数器
   - Byte 12-15: 部分秒（保留）
5. 通过 `CanIf_Transmit()` 发送
6. 递增序列计数器（0~15 循环）

### 7.3 SYNC 消息接收流程（Slave 模式）

1. `CanTSyn_RxIndication()` 被 CanIf 调用
2. 从 PDU 数据中提取时间域 ID 和消息类型
3. 根据消息类型分发处理：
   - `CANTSYN_SYNC_MSG_TYPE (0x10)` → `CanTSyn_ProcessSyncMessage()`
   - `CANTSYN_OFS_MSG_TYPE (0x20)` → `CanTSyn_ProcessOfsMessage()`
4. SYNC 处理：解析时间戳，调用 `StbM_SetGlobalTime()` 更新本地时间
5. OFS 处理：解析偏移时间戳，调用 `StbM_UpdateGlobalTimeOffset()` 校正

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `CANTSYN_DEV_ERROR_DETECT` | STD_ON | 开发错误检测使能 |
| `CANTSYN_VERSION_INFO_API` | STD_ON | 版本信息 API 使能 |
| `CANTSYN_NUM_TIME_DOMAINS` | 2 | 时间域数量 |
| `CANTSYN_TIME_MASTER_SUPPORT` | STD_ON | 时间主节点支持 |
| `CANTSYN_TIME_SLAVE_SUPPORT` | STD_ON | 时间从节点支持 |
| `CANTSYN_SYNC_PERIOD_MS` | 10 | SYNC 周期 (ms) |
| `CANTSYN_SYNC_TIMEOUT_MS` | 100 | 同步超时 (ms) |
| `CANTSYN_MAIN_FUNCTION_PERIOD_MS` | 1 | 主函数周期 (ms) |
| `CANTSYN_CRC_SECURED` | STD_OFF | CRC 校验 |
| `CANTSYN_USER_DATA_SUPPORT` | STD_ON | 用户数据支持 |
| `CANTSYN_RATE_CORRECTION_SUPPORT` | STD_ON | 速率校正支持 |
| `CANTSYN_MAX_TIME_BASES` | 4 | 最大时间基准数 |
| `CANTSYN_SYNC_DLC` | 8 | SYNC 消息 DLC |
| `CANTSYN_FUP_DLC` | 8 | FUP 消息 DLC |
| `CANTSYN_OCS_DLC` | 8 | OCS 消息 DLC |

### 8.2 链接时配置

链接时配置定义在 `CanTSyn_Lcfg.c` 中，包含 `CanTSyn_TimeDomainConfig[]` 数组。当前配置包含 1 个时间域：
- TimeDomain 0: Master 模式，SYNC CAN ID = 0x180，FUP CAN ID = 0x280，OCS CAN ID = 0x380

### 8.3 构建后配置

不支持构建后配置变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 均包含参数验证：
- 空指针检查 → `CANTSYN_E_PARAM_POINTER`
- 模块状态检查 → `CANTSYN_E_UNINIT` / `CANTSYN_E_ALREADY_INITIALIZED`
- PDU ID 范围检查 → `CANTSYN_E_INVALID_PDU_SDU_ID`
- 时间域 ID 范围检查 → 静默丢弃无效消息

### 9.2 DEM 错误

当前实现未报告 DEM 事件。可扩展添加：
- 同步丢失事件 (`CANTSYN_E_SYNC_LOST`)
- 传输失败事件 (`CANTSYN_E_TRANSMISSION_FAILED`)

### 9.3 安全机制

- **MemMap 保护**：配置数据使用 `CANTSYN_START_SEC_CONFIG_DATA_UNSPECIFIED` / `STOP` 宏进行内存分区
- **代码段保护**：API 函数使用 `CANTSYN_START_SEC_CODE` / `STOP` 宏
- **CRC 校验**：可选的消息 CRC 安全模式（当前配置关闭）
- **序列计数器**：4-bit 序列计数器检测消息丢失

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 类型 | 说明 |
|------|------|------|
| `CANTSYN_START_SEC_VAR_CLEARED_*` | 已清零变量 | 内部运行时状态 |
| `CANTSYN_START_SEC_CONFIG_DATA_*` | 配置数据 | 链接时配置结构体 |
| `CANTSYN_START_SEC_CODE` | 代码段 | 所有 API 函数 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| `CanTSyn_Internal` | ~24 bytes | 模块内部状态 |
| `CanTSyn_TimeDomains[]` | 2 x ~32 bytes | 时间域运行时数据 |
| TX 缓冲区 | 16 bytes | 栈上分配，SYNC 消息长度 |
| 代码段 | ~2 KB (估算) | 含消息构建/解析逻辑 |

---

## 11. 集成指南

1. **StbM 配置**：需配置与 CanTSyn 对应的时间基准，确保 `TimeBaseId` 映射正确
2. **CanIf 路由**：
   - SYNC RX PDU → `CanTSyn_RxIndication`
   - SYNC/FUP/OCS TX PDU → `CanTSyn_TxConfirmation`
3. **调度配置**：`CanTSyn_MainFunction()` 建议 1ms 周期调用
4. **CAN ID 规划**：
   - SYNC: 0x180 (base)
   - FUP: 0x280 (base)
   - OCS: 0x380 (base)
5. **DLC 配置**：SYNC/FUP/OCS 消息均为 8 字节 DLC

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| Init/DeInit | 验证初始化/反初始化状态转换 |
| 空指针检测 | 所有 API 的 NULL_PTR 参数 |
| 重复初始化 | 检测 `E_ALREADY_INITIALIZED` |
| SYNC 消息构建 | 验证时间戳编码正确性（大端序） |
| SYNC 消息解析 | 验证时间戳解码正确性 |
| 序列计数器 | 验证 0~15 循环 |
| TX 状态机 | IDLE → BUSY → IDLE 转换 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| Master-Slave 同步 | 端到端时间同步精度验证 |
| 多时间域 | 同时运行多个时间域 |
| StbM 集成 | 验证 StbM_SetGlobalTime / StbM_GetCurrentTime |
| 超时处理 | SYNC 超时后的同步丢失检测 |

---

## 13. 实现说明 / TODO

- **FUP 消息发送**：当前 `CanTSyn_MainFunction()` 仅实现 SYNC 发送，FUP（延迟模式）待实现
- **CRC 校验**：`CANTSYN_CRC_SECURED = STD_OFF`，CRC 计算/验证逻辑待实现
- **DeInit 函数**：头文件声明但 `.c` 文件中未实现
- **SetTransmissionMode / GetTransmissionMode**：头文件声明但 `.c` 文件中未实现
- **SetGlobalTime / SetRateCorrection**：头文件声明但 `.c` 文件中未实现
- **SetUserData / GetUserData**：头文件声明但 `.c` 文件中未实现
- **TimeTxConfirmationSYNC/FUP/OCS**：头文件声明但 `.c` 文件中未实现
- **GetSyncReceived / GetCurrentVirtualTime**：头文件声明但 `.c` 文件中未实现

---

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| AUTOSAR_SWS_CANTimeSynchronization | CAN 时间同步模块规范 |
| AUTOSAR_SWS_StbM | 同步时间基准管理器规范 |
| AUTOSAR_SWS_CanInterface | CAN 接口规范 |
| IEEE 1588 | 精确时间协议参考 |
| `src/bsw/services/cantsyn/` | 源代码目录 |
