# J1939Tp (J1939 Transport Protocol) Design Document

> **Module ID**: 0x4D  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_J1939TransportProtocol  
> **Source Path**: `src/bsw/services/j1939tp/`  
> **Reference Document**: `docs/modules/j1939tp.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

J1939Tp 模块实现 SAE J1939-21 传输协议，为商用车 CAN 网络提供多帧消息传输能力。当参数组（PG）数据超过单帧 8 字节限制时，J1939Tp 负责将大数据分割为多个 TP.DT 数据包进行传输，并通过 TP.CM（连接管理）协议协调收发双方。

主要功能：
- **BAM（Broadcast Announce Message）**：无连接广播传输，适用于广播类 PG
- **CMDT（Connection Mode Data Transfer）**：基于 RTS/CTS 握手的可靠传输
- **分段与重组**：将最大 1785 字节数据分割为 7 字节/包的 TP.DT 帧
- **超时管理**：T1/T2/T3/T4 四个超时定时器
- **多通道支持**：最多 4 个 TX 通道和 4 个 RX 通道并发传输

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| SAE J1939-21 | Latest | J1939 数据链路层传输协议 |
| AUTOSAR SWS J1939 Transport Protocol | 4.4.0 | 模块软件规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | PduR | 路由层，接收重组后的完整 PDU |
| 同层 | CanIf | CAN 接口层，发送/接收 TP.CM 和 TP.DT 消息 |
| 公共 | Det | 开发错误检测与报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│         PduR / Application          │
├─────────────────────────────────────┤
│         J1939Tp (Services)          │
├─────────────────────────────────────┤
│         CanIf (CAN Interface)       │
├─────────────────────────────────────┤
│         Can Driver                  │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **TX 通道管理器**：管理最多 4 个发送通道的状态机
- **RX 通道管理器**：管理最多 4 个接收通道的状态机
- **TP.CM 处理器**：构建和解析连接管理消息（RTS/CTS/EOM/Abort/BAM）
- **TP.DT 处理器**：构建和解析数据传输消息（7 字节有效载荷）
- **超时管理器**：管理 T1~T4 四个协议定时器
- **缓冲区管理**：TX/RX 各 1785 字节内部缓冲区

### 3.3 文件结构

```
src/bsw/services/j1939tp/
├── include/
│   ├── J1939Tp.h          -- 公共 API 与类型定义
│   └── J1939Tp_Cfg.h      -- 预编译配置参数
└── src/
    ├── J1939Tp.c           -- 核心实现
    └── J1939Tp_Lcfg.c      -- 链接时配置数据
```

---

## 4. 状态机

### 4.1 TX 通道状态

```
IDLE -- Transmit() --> RTS_TX
RTS_TX -- SendTpCm(RTS) --> CTS_RX
CTS_RX -- 收到 CTS --> DT_TX
CTS_RX -- T1 超时 --> IDLE (abort)
DT_TX -- 所有包发完 --> EOM_ACK
DT_TX -- 收到 EOM ACK --> IDLE
IDLE -- BAM 模式 --> BAM_TX
BAM_TX -- 发送 BAM + DT 包 --> IDLE
```

### 4.2 RX 通道状态

```
IDLE -- 收到 RTS --> DT_RX
IDLE -- 收到 BAM --> BAM_RX
DT_RX -- 所有包收完 --> IDLE (forward to PduR)
DT_RX -- T2 超时 --> IDLE (abort)
BAM_RX -- 所有包收完 --> IDLE (forward to PduR)
```

### 4.3 TP.CM 控制字节

| 控制字节 | 值 | 说明 |
|----------|-----|------|
| `J1939TP_CM_RTS` | 16 (0x10) | Request To Send |
| `J1939TP_CM_CTS` | 17 (0x11) | Clear To Send |
| `J1939TP_CM_ACK` | 19 (0x13) | End of Message Acknowledgment |
| `J1939TP_CM_BAM` | 32 (0x20) | Broadcast Announce Message |
| `J1939TP_CM_ABORT` | 255 (0xFF) | Connection Abort |

---

## 5. 核心数据结构

### 5.1 连接配置

```c
typedef struct {
    J1939Tp_SduType SduId;          /* SDU 标识 */
    J1939Tp_CommunicationType ComType; /* 通信类型 (BAM/CTS/DIRECT) */
    uint8 BlockSize;                 /* 块大小 */
    uint16 T1Timeout;                /* T1 超时 (ms) */
    uint16 T2Timeout;                /* T2 超时 (ms) */
    uint16 T3Timeout;                /* T3 超时 (ms) */
    uint16 T4Timeout;                /* T4 超时 (ms) */
    PduIdType TxPduId;               /* TP.CM TX PDU */
    PduIdType TxDtPduId;             /* TP.DT TX PDU */
    PduIdType RxPduId;               /* 接收 PDU */
} J1939Tp_ConnectionConfigType;
```

### 5.2 PG 配置

```c
typedef struct {
    J1939Tp_PgType PgId;            /* PG 标识 */
    PduIdType PduId;                 /* PDU ID */
    boolean DirectNPdu;              /* 直接 N-PDU */
    boolean PgIsVariable;            /* 可变长度 */
    uint16 PgLength;                 /* PG 长度 */
    uint8 DirectSdu;                 /* 直接 SDU */
    uint8 MetaDataLength;            /* 元数据长度 */
} J1939Tp_PgConfigType;
```

### 5.3 TX 通道运行时状态

```c
typedef struct {
    J1939Tp_StateType State;         /* 通道状态 */
    uint8 SeqNumber;                 /* 序列号 */
    uint16 TotalBytes;               /* 总字节数 */
    uint16 SentBytes;                /* 已发送字节数 */
    uint16 PacketsToSend;            /* 待发送包数 */
    uint16 PacketsSent;              /* 已发送包数 */
    uint8 DestAddr;                  /* 目标地址 */
    uint8 SrcAddr;                   /* 源地址 */
    uint32 Pgn;                      /* 参数组编号 */
    uint16 T1Timer;                  /* T1 定时器 */
    uint16 T3Timer;                  /* T3 定时器 */
    uint16 T4Timer;                  /* T4 定时器 */
    boolean BcTimerActive;           /* 广播定时器激活 */
} J1939Tp_TxChannelType;
```

### 5.4 RX 通道运行时状态

```c
typedef struct {
    J1939Tp_StateType State;         /* 通道状态 */
    uint8 SeqNumber;                 /* 序列号 */
    uint16 TotalBytes;               /* 总字节数 */
    uint16 ReceivedBytes;            /* 已接收字节数 */
    uint16 PacketsToReceive;         /* 待接收包数 */
    uint16 PacketsReceived;          /* 已接收包数 */
    uint8 SrcAddr;                   /* 源地址 */
    uint8 DestAddr;                  /* 目标地址 */
    uint32 Pgn;                      /* 参数组编号 */
    uint16 T1Timer;                  /* T1 定时器 */
    uint16 T2Timer;                  /* T2 定时器 */
    uint16 NBrTimer;                 /* NBr 定时器 */
} J1939Tp_RxChannelType;
```

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `J1939Tp_Init(ConfigPtr)` | 0x01 | 初始化模块 |
| `J1939Tp_DeInit()` | 0x02 | 反初始化模块 |
| `J1939Tp_GetVersionInfo(VersionInfo)` | 0x03 | 获取版本信息 |
| `J1939Tp_Transmit(TxSduId, TxInfoPtr)` | 0x05 | 请求发送 PDU |
| `J1939Tp_CancelTransmit(TxSduId)` | 0x06 | 取消发送 |
| `J1939Tp_CancelReceive(RxSduId)` | 0x07 | 取消接收 |
| `J1939Tp_ChangeParameter(SduId, Parameter, Value)` | 0x08 | 修改 TP 参数 |
| `J1939Tp_MainFunction()` | 0x04 | 周期处理函数 |

### 6.2 回调函数

| 函数 | SID | 说明 |
|------|-----|------|
| `J1939Tp_RxIndication(RxPduId, PduInfoPtr)` | 0x42 | CAN 接收指示 |
| `J1939Tp_TxConfirmation(TxPduId, result)` | 0x40 | CAN 发送确认 |
| `PduR_J1939TpRxIndication(RxPduId, PduInfoPtr)` | - | 向 PduR 指示完整 PDU 接收 |

### 6.3 服务 ID 与错误码

**DET 错误码：**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `J1939TP_E_NO_ERROR` | 0x00 | 无错误 |
| `J1939TP_E_PARAM_POINTER` | 0x01 | 空指针 |
| `J1939TP_E_UNINIT` | 0x02 | 模块未初始化 |
| `J1939TP_E_INVALID_PDU_SDU_ID` | 0x03 | 无效 PDU/SDU ID |
| `J1939TP_E_INVALID_TSA` | 0x04 | 无效目标地址 |
| `J1939TP_E_INVALID_PARAMETER` | 0x05 | 无效参数 |
| `J1939TP_E_CANCEL_NOT_SUPPORTED` | 0x06 | 取消不支持 |
| `J1939TP_E_INVALID_VALUE` | 0x07 | 无效值 |
| `J1939TP_E_INIT_FAILED` | 0x08 | 初始化失败 |

**TP 参数类型：**

| 参数 | 说明 |
|------|------|
| `J1939TP_PARAM_BROADCAST_TIME` | 广播间隔时间 |
| `J1939TP_PARAM_BLOCK_SIZE` | 块大小 |
| `J1939TP_PARAM_T1` ~ `J1939TP_PARAM_T4` | 超时参数 |
| `J1939TP_PARAM_N_Bs` / `N_Cs` / `N_Br` / `N_Ar` | J1939 时序参数 |

---

## 7. 处理流程

### 7.1 发送流程（CMDT 模式）

1. `J1939Tp_Transmit()` 被上层调用
2. 数据长度 <= 8 字节：直接通过 `CanIf_Transmit()` 单帧发送
3. 数据长度 > 8 字节：
   - 查找空闲 TX 通道
   - 查找关联的连接配置
   - 初始化通道状态为 `RTS_TX`
   - 计算包数：`PacketsToSend = (SduLength + 6) / 7`
   - 复制数据到内部 TX 缓冲区
4. `MainFunction()` 处理 `RTS_TX` 状态：
   - 构建 TP.CM RTS 消息（8 字节）
   - 通过 `CanIf_Transmit()` 发送
   - 状态切换到 `CTS_RX`，启动 T1 定时器
5. 收到 CTS 后切换到 `DT_TX`：
   - 构建 TP.DT 消息：Byte 0 = 序列号，Byte 1-7 = 7 字节数据
   - 逐包发送，递增序列号
   - 所有包发完后切换到 `EOM_ACK`
6. 收到 EOM ACK 后重置通道

### 7.2 接收流程

1. `J1939Tp_RxIndication()` 被 CanIf 调用
2. 根据 PGN 判断消息类型：
   - PGN 60416 (0xEC00) → TP.CM 消息
   - PGN 60160 (0xEB00) → TP.DT 消息
3. TP.CM 处理：
   - RTS (0x10) → 解析总长度/包数/PGN，发送 CTS，进入 `DT_RX`
   - BAM (0x20) → 解析总长度/包数/PGN，进入 `BAM_RX`
   - CTS (0x11) → TX 通道切换到 `DT_TX`
   - ACK (0x13) → TX 通道重置（传输完成）
   - Abort (0xFF) → 重置 TX/RX 通道
4. TP.DT 处理：
   - 验证序列号 = 期望值 + 1
   - 复制 7 字节有效载荷到 RX 缓冲区
   - 所有包收完后通过 `PduR_J1939TpRxIndication()` 通知上层

### 7.3 TP.CM 消息格式（8 字节）

```
Byte 0: 控制字节 (RTS=0x10, CTS=0x11, ACK=0x13, BAM=0x20, Abort=0xFF)
Byte 1: 总字节数 (低字节)
Byte 2: 总字节数 (高字节)
Byte 3: 包数
Byte 4: 最大包数 (CTS) / 保留 (RTS)
Byte 5: PGN 低字节
Byte 6: PGN 中字节
Byte 7: PGN 高字节
```

### 7.4 TP.DT 消息格式（8 字节）

```
Byte 0: 序列号 (1~255)
Byte 1-7: 数据 (7 字节)
```

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `J1939TP_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `J1939TP_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `J1939TP_MAX_TX_CHANNELS` | 4 | 最大 TX 通道数 |
| `J1939TP_MAX_RX_CHANNELS` | 4 | 最大 RX 通道数 |
| `J1939TP_MAIN_FUNCTION_PERIOD` | 10 | 主函数周期 (ms) |
| `J1939TP_T1_TIMEOUT_DEFAULT` | 750 | T1 超时 (ms) |
| `J1939TP_T2_TIMEOUT_DEFAULT` | 1250 | T2 超时 (ms) |
| `J1939TP_T3_TIMEOUT_DEFAULT` | 1250 | T3 超时 (ms) |
| `J1939TP_T4_TIMEOUT_DEFAULT` | 1050 | T4 超时 (ms) |
| `J1939TP_N_BROADCAST_TIME` | 50 | 广播间隔 (ms) |
| `J1939TP_MAX_PG` | 32 | 最大 PG 数量 |
| `J1939TP_MAX_CONNECTIONS` | 8 | 最大连接数 |
| `J1939TP_MAX_TP_SIZE` | 1785 | 最大 TP 数据大小 |
| `J1939TP_PGN_TP_CM` | 60416 | TP.CM PGN |
| `J1939TP_PGN_TP_DT` | 60160 | TP.DT PGN |

### 8.2 链接时配置

链接时配置定义在 `J1939Tp_Lcfg.c` 中：
- **连接配置表**：8 个连接，包含 CMDT、BAM、DIRECT 三种类型
- **PG 配置表**：32 个参数组，包含固定/可变长度 PG
- 连接 0: CMDT, T1=750ms, T2=1250ms
- 连接 1: BAM 广播
- 连接 2: 直接传输

### 8.3 构建后配置

不支持构建后配置变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误

- `Init`: 空指针检查 → `J1939TP_E_PARAM_POINTER`
- `Transmit`: 未初始化检查 → `J1939TP_E_UNINIT`；空指针 → `J1939TP_E_PARAM_POINTER`
- `CancelTransmit/CancelReceive`: 未初始化检查 → `J1939TP_E_UNINIT`
- `RxIndication`: 空指针 → `J1939TP_E_PARAM_POINTER`
- `GetVersionInfo`: 空指针 → `J1939TP_E_PARAM_POINTER`

### 9.2 DEM 错误

当前实现未报告 DEM 事件。

### 9.3 安全机制

- **MemMap 保护**：TX/RX 缓冲区使用 `J1939TP_START_SEC_VAR_NO_INIT_UNSPECIFIED` 分区
- **序列号验证**：接收时验证 TP.DT 序列号连续性
- **超时保护**：T1~T4 定时器防止通道死锁
- **Abort 处理**：收到 Abort 消息立即重置通道

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 类型 | 说明 |
|------|------|------|
| `J1939TP_START_SEC_VAR_INIT_UNSPECIFIED` | 已初始化变量 | 模块状态、配置指针 |
| `J1939TP_START_SEC_VAR_NO_INIT_UNSPECIFIED` | 未初始化变量 | TX/RX 通道、缓冲区 |
| `J1939TP_START_SEC_CODE` | 代码段 | 所有 API 函数 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| `J1939Tp_TxChannels[]` | 4 x ~40 bytes | TX 通道运行时状态 |
| `J1939Tp_RxChannels[]` | 4 x ~36 bytes | RX 通道运行时状态 |
| `J1939Tp_TxBuffer` | 1785 bytes | TX 数据缓冲区 |
| `J1939Tp_RxBuffer` | 1785 bytes | RX 数据缓冲区 |
| 代码段 | ~4 KB (估算) | 状态机 + 消息处理 |

---

## 11. 集成指南

1. **CanIf 路由**：
   - TP.CM PGN 60416 → `J1939Tp_RxIndication`
   - TP.DT PGN 60160 → `J1939Tp_RxIndication`
   - TX 确认 → `J1939Tp_TxConfirmation`
2. **PduR 集成**：完整 PDU 通过 `PduR_J1939TpRxIndication()` 上报
3. **调度配置**：`J1939Tp_MainFunction()` 建议 10ms 周期调用
4. **CAN ID 编码**：使用宏 `J1939TP_CAN_ID(prio, pgn, sa)` 构建 29 位扩展 CAN ID

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| Init/DeInit | 通道重置验证 |
| 单帧发送 | <= 8 字节直接透传 |
| CMDT 发送 | RTS → CTS → DT → EOM 完整流程 |
| BAM 发送 | BAM → DT 广播流程 |
| 接收重组 | DT 包序列号验证与数据重组 |
| 超时处理 | T1~T4 超时后的通道重置 |
| Abort 处理 | 连接中止与通道重置 |
| 多通道并发 | 同时使用多个 TX/RX 通道 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| PduR 集成 | 完整 PDU 上报验证 |
| CanIf 集成 | TP.CM/DT 消息收发 |
| 大数据传输 | 1785 字节极限传输 |
| 超时恢复 | 通信中断后的超时恢复 |

---

## 13. 实现说明 / TODO

- **ChangeParameter**：当前为占位实现，返回 `E_NOT_OK`
- **BAM TX**：`J1939TP_STATE_BAM_TX` 状态处理逻辑待完善
- **CTS 发送**：RX 端收到 RTS 后的 CTS 发送逻辑待完善
- **TxConfirmation**：当前为空实现
- **PGN 提取**：`J1939Tp_RxIndication` 中 PGN 提取为硬编码 0，需从 CAN ID 元数据中提取
- **版本检查**：编译时 AR 版本检查已实现

---

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| SAE J1939-21 | J1939 数据链路层传输协议 |
| AUTOSAR_SWS_J1939TransportProtocol | AUTOSAR J1939 TP 模块规范 |
| AUTOSAR_SWS_PduRouter | PDU 路由器规范 |
| `src/bsw/services/j1939tp/` | 源代码目录 |

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_J1939Tp_00001 | `j1939tp` | 测试 test_j1939tp_Init_should_initialize 覆盖: j1939tp_Init_should_initialize 场景 |
| SWS_J1939Tp_00002 | `J1939Tp_DeInit` | 测试 test_J1939Tp_DeInit_ValidCall_ShouldSucceed 覆盖: J1939Tp_DeInit_ValidCall_ShouldSucceed 场景 |
| SWS_J1939Tp_00003 | `J1939Tp_GetVersionInfo` | 测试 test_J1939Tp_GetVersionInfo_NullPtr 覆盖: J1939Tp_GetVersionInfo_NullPtr 场景 |
| SWS_J1939Tp_00004 | `J1939Tp_MainFunction` | 测试 test_J1939Tp_MainFunction_ValidCall_ShouldSucceed 覆盖: J1939Tp_MainFunction_ValidCall_ShouldSucceed 场景 |
| SWS_J1939Tp_00005 | `J1939Tp_CancelTransmit` | 测试 test_J1939Tp_CancelTransmit_NoActiveSession 覆盖: J1939Tp_CancelTransmit_NoActiveSession 场景 |
| SWS_J1939Tp_00006 | `J1939Tp_CancelTransmit` | 测试 test_J1939Tp_CancelTransmit_ValidCall_ShouldSucceed 覆盖: J1939Tp_CancelTransmit_ValidCall_ShouldSucceed 场景 |
| SWS_J1939Tp_00007 | `J1939Tp_GetState` | 测试 test_J1939Tp_GetState_ValidCall_ShouldSucceed 覆盖: J1939Tp_GetState_ValidCall_ShouldSucceed 场景 |
| SWS_J1939Tp_00008 | `J1939Tp_ChangeParameter` | 测试 test_J1939Tp_ChangeParameter 覆盖: J1939Tp_ChangeParameter 场景 |
| SWS_J1939Tp_00009 | `J1939Tp_RxIndication` | 测试 test_J1939Tp_RxIndication_ValidPdu 覆盖: J1939Tp_RxIndication_ValidPdu 场景 |
| SWS_J1939Tp_00010 | `J1939Tp_TxConfirmation` | 测试 test_J1939Tp_TxConfirmation 覆盖: J1939Tp_TxConfirmation 场景 |
