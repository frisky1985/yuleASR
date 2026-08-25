# SomeIpTp (SOME/IP Transport Protocol) Design Document

> **Module ID**: 0x9B  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_SOMEIPTransportProtocol  
> **Source Path**: `src/bsw/services/someiptp/`  
> **Reference Document**: `docs/modules/someiptp.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

SomeIpTp 模块实现 SOME/IP 传输协议，负责大尺寸 SOME/IP 消息的分段（Fragmentation）与重组（Reassembly）。当 SOME/IP 消息超过单个以太网帧的有效载荷大小时，SomeIpTp 将其分割为多个段（Segment）进行传输，并在接收端重组为完整消息。

主要功能：
- **分段发送**：将大 PDU 分割为最大 1392 字节的段，添加 TP 头（4 字节偏移+标志位）
- **重组接收**：按偏移量重组接收到的段为完整 PDU
- **TP 头管理**：4 字节 TP 头包含 30 位偏移量和 1 位 More Segments 标志
- **超时管理**：TX/RX 超时检测与通道重置
- **多通道支持**：最多 4 个独立 TP 通道并发传输
- **SoAd 集成**：通过 SoAd 接口发送分段数据

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS SOME/IP Transport Protocol | R22-11 | 模块软件规范 |
| SOME/IP Protocol Specification | 1.x | SOME/IP 协议规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SomeIpXf | 转换器层，接收重组后的完整消息 |
| 同层 | SoAd | Socket Adaptor，发送分段数据 |
| 公共 | Det | 开发错误检测与报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│         SomeIpXf / SD / Application │
├─────────────────────────────────────┤
│         SomeIpTp (Services)         │
├─────────────────────────────────────┤
│         SoAd (Socket Adaptor)       │
├─────────────────────────────────────┤
│         TcpIp / EthIf               │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **分段引擎**：将大 PDU 按 MaxSegmentSize 分割，添加 TP 头
- **重组引擎**：按偏移量将段数据拷贝到重组缓冲区
- **TP 头处理器**：构建和解析 4 字节 TP 头（偏移量 + More Segments 标志）
- **通道管理器**：管理 4 个独立通道的 TX/RX 状态
- **超时管理器**：TX/RX 超时计数与通道重置

### 3.3 文件结构

```
src/bsw/services/someiptp/
├── include/
│   ├── SomeIpTp.h          -- 公共 API 与类型定义
│   ├── SomeIpTp_Cfg.h      -- 预编译配置参数
│   └── SomeIpTp_MemMap.h   -- MemMap 宏定义
└── src/
    ├── SomeIpTp.c           -- 核心实现
    └── SomeIpTp_Test.c      -- 测试代码
```

---

## 4. 状态机

### 4.1 模块状态

```
SOMEIPTP_STATE_UNINIT -- Init() --> SOMEIPTP_STATE_INIT
SOMEIPTP_STATE_INIT -- DeInit() --> SOMEIPTP_STATE_UNINIT
```

### 4.2 通道状态

```
CHANNEL_IDLE -- Transmit() --> CHANNEL_TX_ACTIVE
CHANNEL_TX_ACTIVE -- 段发送 --> CHANNEL_TX_WAIT_CONFIRM
CHANNEL_TX_WAIT_CONFIRM -- TxConfirm(E_OK) --> CHANNEL_TX_ACTIVE (下一段)
CHANNEL_TX_WAIT_CONFIRM -- TxConfirm(E_OK) + 最后段 --> CHANNEL_IDLE
CHANNEL_TX_WAIT_CONFIRM -- 超时 --> CHANNEL_IDLE (reset)

CHANNEL_IDLE -- RxIndication(offset=0) --> CHANNEL_RX_ACTIVE
CHANNEL_RX_ACTIVE -- RxIndication(moreSeg=TRUE) --> CHANNEL_RX_ACTIVE
CHANNEL_RX_ACTIVE -- RxIndication(moreSeg=FALSE) --> CHANNEL_RX_COMPLETED
CHANNEL_RX_COMPLETED -- 通知上层 --> CHANNEL_IDLE
CHANNEL_RX_ACTIVE -- 超时 --> CHANNEL_IDLE (reset)
```

### 4.3 TP 头格式（4 字节）

```
Bit 31:    RES (保留位)
Bit 30:    More Segments (1=还有后续段, 0=最后段)
Bit 29-0:  Offset (字节偏移量, 最大 ~1GB)
```

编码为大端序 4 字节：
```c
value = (moreSeg ? 0x40000000 : 0x00) | (offset & 0x3FFFFFFF)
```

---

## 5. 核心数据结构

### 5.1 通道配置

```c
typedef struct {
    PduIdType TxPduId;          /* 发送 PDU ID */
    PduIdType RxPduId;          /* 接收 PDU ID */
    uint32 MaxPduLength;        /* 最大 PDU 长度 */
    uint16 MaxSegmentSize;      /* 最大段大小 (1392) */
    uint32 TxTimeout;           /* 发送超时 (ms) */
    uint32 RxTimeout;           /* 接收超时 (ms) */
    uint8 MaxRetries;           /* 最大重试次数 */
} SomeIpTp_ChannelConfigType;
```

### 5.2 重组缓冲区

```c
typedef struct {
    uint8* Data;                /* 数据指针 */
    uint32 Length;              /* 当前数据长度 */
    uint32 MaxLength;           /* 最大长度 (65536) */
    uint32 NextOffset;          /* 期望的下一偏移量 */
    boolean MoreSegmentsExpected; /* 预期还有后续段 */
    boolean IsComplete;         /* 重组完成标志 */
} SomeIpTp_RxBufferType;
```

### 5.3 分段缓冲区

```c
typedef struct {
    const uint8* Data;          /* 原始数据指针 */
    uint32 Length;              /* 总长度 */
    uint32 CurrentOffset;       /* 当前偏移量 */
    uint32 RemainingLength;     /* 剩余长度 */
    uint16 CurrentSegmentSize;  /* 当前段大小 */
} SomeIpTp_TxBufferType;
```

### 5.4 通道运行时状态

```c
typedef struct {
    SomeIpTp_ChannelStateType State;     /* 通道状态 */
    uint32 TimeoutCounter;               /* 超时计数器 */
    uint8 RetryCount;                    /* 重试计数 */
    SomeIpTp_RxBufferType RxBuffer;      /* 重组缓冲区 */
    SomeIpTp_TxBufferType TxBuffer;      /* 分段缓冲区 */
    PduInfoType CurrentPduInfo;          /* 当前 PDU 信息 */
} SomeIpTp_ChannelType;
```

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `SomeIpTp_Init(ConfigPtr)` | 0x01 | 初始化模块 |
| `SomeIpTp_DeInit()` | 0x02 | 反初始化模块 |
| `SomeIpTp_GetVersionInfo(versioninfo)` | 0x03 | 获取版本信息 |
| `SomeIpTp_Transmit(TxPduId, PduInfoPtr, RetryInfoPtr, TxDataCntPtr)` | 0x04 | 分段发送 |
| `SomeIpTp_CancelTransmit(TxPduId)` | 0x08 | 取消发送 |
| `SomeIpTp_MainFunction()` | 0x07 | 周期处理函数 |
| `SomeIpTp_BuildTpHeader(Offset, MoreSegments, Buffer)` | - | 构建 TP 头 |
| `SomeIpTp_ParseTpHeader(Buffer, Offset, MoreSegments)` | - | 解析 TP 头 |
| `SomeIpTp_GetRxBufferStatus(RxPduId, BufferSizePtr)` | - | 获取接收缓冲状态 |

### 6.2 回调函数

| 函数 | SID | 说明 |
|------|-----|------|
| `SomeIpTp_RxIndication(RxPduId, PduInfoPtr)` | 0x05 | 接收段指示 |
| `SomeIpTp_TxConfirmation(TxPduId, result)` | 0x06 | 发送段确认 |
| `SomeIpXf_RxIndication(RxPduId, PduInfoPtr)` | - | 重组完成后通知上层 |

### 6.3 错误码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `SOMEIPTP_E_PARAM_POINTER` | 0x01 | 空指针 |
| `SOMEIPTP_E_PARAM_CONFIG` | 0x02 | 配置错误 |
| `SOMEIPTP_E_UNINIT` | 0x03 | 未初始化 |
| `SOMEIPTP_E_ALREADY_INITIALIZED` | 0x04 | 重复初始化 |
| `SOMEIPTP_E_INVALID_PDU_ID` | 0x05 | 无效 PDU ID |
| `SOMEIPTP_E_INVALID_BUFFER_SIZE` | 0x06 | 无效缓冲区大小 |
| `SOMEIPTP_E_INVALID_PARAMETER` | 0x07 | 无效参数 |
| `SOMEIPTP_E_FRAGMENTATION_ERROR` | 0x08 | 分段错误 |
| `SOMEIPTP_E_REASSEMBLY_ERROR` | 0x09 | 重组错误 |
| `SOMEIPTP_E_TIMEOUT` | 0x0A | 超时 |
| `SOMEIPTP_E_BUFFER_OVERFLOW` | 0x0B | 缓冲区溢出 |

---

## 7. 处理流程

### 7.1 分段发送流程

1. `SomeIpTp_Transmit()` 被上层调用
2. 根据 TxPduId 查找对应通道
3. 初始化 TX 缓冲区：
   - `Data = PduInfoPtr->SduDataPtr`
   - `Length = PduInfoPtr->SduLength`
   - `CurrentOffset = 0`
   - `RemainingLength = SduLength`
4. 设置状态为 `CHANNEL_TX_ACTIVE`
5. 调用 `SomeIpTp_SendNextSegment()`：
   - 计算段大小 = min(RemainingLength, MaxSegmentSize)
   - 构建 TP 头（4 字节）：偏移量 + MoreSegments 标志
   - 拷贝段数据到临时缓冲区
   - 通过 `SoAd_Transmit()` 发送
6. 发送确认后：
   - 更新 `CurrentOffset += segLen`
   - 更新 `RemainingLength -= segLen`
   - 若 RemainingLength > 0 → 继续发送下一段
   - 若 RemainingLength == 0 → 传输完成

### 7.2 重组接收流程

1. `SomeIpTp_RxIndication()` 被 SoAd 调用
2. 根据 RxPduId 查找对应通道
3. 解析 TP 头（4 字节）获取偏移量和 MoreSegments 标志
4. 偏移量 == 0 → 新会话开始，重置重组缓冲区
5. 验证偏移量 == `NextOffset`（顺序接收）
6. 拷贝段数据到重组缓冲区：
   - `memcpy(RxBuffer.Data[RxBuffer.Length], PduData[4:], payloadLen)`
   - 更新 `Length += payloadLen`
   - 更新 `NextOffset += payloadLen`
7. MoreSegments == FALSE → 重组完成：
   - 设置 `IsComplete = TRUE`
   - 状态切换到 `CHANNEL_RX_COMPLETED`
   - 调用 `SomeIpXf_RxIndication()` 通知上层

### 7.3 超时处理

`SomeIpTp_MainFunction()` 周期调用 `SomeIpTp_UpdateTimeouts()`：
- 遍历所有通道
- 对 `TX_WAIT_CONFIRM` 和 `RX_ACTIVE` 状态的通道递减超时计数器
- 计数器归零 → 报告 DET 错误 `SOMEIPTP_E_TIMEOUT` → 重置通道

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `SOMEIPTP_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `SOMEIPTP_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `SOMEIPTP_NUMBER_OF_CHANNELS` | 4 | 通道数 |
| `SOMEIPTP_RX_BUFFER_SIZE` | 65536 | RX 缓冲区大小 (64KB) |
| `SOMEIPTP_TX_BUFFER_SIZE` | 65536 | TX 缓冲区大小 (64KB) |
| `SOMEIPTP_MAX_PDU_LENGTH` | 65536 | 最大 PDU 长度 |
| `SOMEIPTP_MAX_SEGMENT_SIZE` | 1392 | 最大段大小 |
| `SOMEIPTP_MAX_RETRIES` | 3 | 最大重试次数 |
| `SOMEIPTP_TX_TIMEOUT_MS` | 1000 | TX 超时 (ms) |
| `SOMEIPTP_RX_TIMEOUT_MS` | 1000 | RX 超时 (ms) |
| `SOMEIPTP_RETRANSMIT_TIMEOUT_MS` | 500 | 重传超时 (ms) |
| `SOMEIPTP_MAIN_FUNCTION_PERIOD_MS` | 10 | 主函数周期 (ms) |

### 8.2 链接时配置

通道配置通过 `SomeIpTp_Config` 全局结构体引用，包含 4 个通道的 TX/RX PDU ID 映射。

### 8.3 构建后配置

不支持构建后配置变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误

- `Init`: 重复初始化 → `SOMEIPTP_E_ALREADY_INITIALIZED`；空指针 → `SOMEIPTP_E_PARAM_POINTER`
- `DeInit`: 未初始化 → `SOMEIPTP_E_UNINIT`
- `Transmit`: 未初始化 → `SOMEIPTP_E_UNINIT`；空指针 → `SOMEIPTP_E_PARAM_POINTER`
- `RxIndication`: 偏移量不连续 → `SOMEIPTP_E_REASSEMBLY_ERROR`
- `MainFunction`: 超时 → `SOMEIPTP_E_TIMEOUT`

### 9.2 DEM 错误

当前实现未报告 DEM 事件。

### 9.3 安全机制

- **MemMap 保护**：RX 缓冲区池使用 `SOMEIPTP_START_SEC_VAR_CLEARED_UNSPECIFIED` 分区
- **偏移量验证**：接收时严格验证段的偏移量连续性
- **缓冲区溢出保护**：重组数据长度不超过 `MaxLength` (65536)
- **超时保护**：TX/RX 超时防止通道死锁
- **DET 宏封装**：通过 `SOMEIPTP_DET_REPORT_ERROR` 宏统一控制错误报告

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 类型 | 说明 |
|------|------|------|
| `SOMEIPTP_START_SEC_VAR_CLEARED_UNSPECIFIED` | 已清零变量 | 内部状态、RX 缓冲区池 |
| `SOMEIPTP_START_SEC_CODE` | 代码段 | 所有 API 函数 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| `SomeIpTp_InternalState` | ~100 bytes | 模块内部状态 |
| `RxBufferPool[4][65536]` | 262144 bytes | RX 缓冲区池 (256KB) |
| `Channels[4]` | ~200 bytes | 通道运行时状态 |
| 段发送临时缓冲 | ~1400 bytes | 栈上分配 |
| 代码段 | ~3 KB (估算) | 分段/重组逻辑 |

---

## 11. 集成指南

1. **SoAd 集成**：
   - 发送段 → `SoAd_Transmit(TxPduId, PduInfoPtr)`
   - 接收段 → `SomeIpTp_RxIndication(RxPduId, PduInfoPtr)`
   - 发送确认 → `SomeIpTp_TxConfirmation(TxPduId, result)`
2. **SomeIpXf 集成**：
   - 重组完成 → `SomeIpXf_RxIndication(RxPduId, PduInfoPtr)`
3. **调度配置**：`SomeIpTp_MainFunction()` 建议 10ms 周期调用
4. **PDU 路由**：配置 TX/RX PDU ID 与 SoAd Socket 的映射

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| Init/DeInit | 通道重置与状态管理 |
| TP 头构建 | 偏移量 + MoreSegments 编码 |
| TP 头解析 | 偏移量 + MoreSegments 解码 |
| 分段发送 | 大 PDU 分割为多段 |
| 重组接收 | 多段重组为完整 PDU |
| 偏移量验证 | 乱序段检测 |
| 超时处理 | TX/RX 超时后的通道重置 |
| 缓冲区溢出 | 超过 MaxLength 的保护 |
| 多通道并发 | 4 通道同时传输 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| SoAd 集成 | 段收发与确认 |
| SomeIpXf 集成 | 重组完成通知 |
| 大数据传输 | 64KB 极限传输 |
| 网络超时 | 通信中断后的超时恢复 |

---

## 13. 实现说明 / TODO

- **CancelTransmit**：头文件声明但 `.c` 文件中未实现
- **GetRxBufferStatus**：头文件声明但 `.c` 文件中未实现
- **重试机制**：`RetryCount` 字段已定义但重试逻辑未实现
- **TxConfirmation 处理**：头文件声明但 `.c` 文件中未实现，发送确认后的状态更新逻辑待完善
- **SoAd_Transmit 外部声明**：在 `SomeIpTp_SendNextSegment()` 中使用 `extern` 声明，应通过头文件引入
- **SomeIpXf_RxIndication 外部声明**：在 `SomeIpTp_ProcessReassembly()` 中使用 `extern` 声明，应通过头文件引入

---

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| AUTOSAR_SWS_SOMEIPTransportProtocol | SOME/IP TP 模块规范 |
| SOME/IP Protocol Specification | SOME/IP 协议规范 |
| AUTOSAR_SWS_SocketAdaptor | SoAd 规范 |
| `src/bsw/services/someiptp/` | 源代码目录 |
