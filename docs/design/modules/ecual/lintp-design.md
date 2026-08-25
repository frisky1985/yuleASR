# LinTp (LIN Transport Protocol) Design Document

> **Module ID**: 0x39  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_LINTransportProtocol  
> **Source Path**: `src/bsw/ecual/linTp/`  
> **Reference Document**: `docs/modules/lintp.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

LinTp 模块实现 ISO 17987-2 LIN 传输协议，为 LIN 总线网络提供分段与重组（Segmentation and Reassembly）能力。当 N-SDU 数据超过单帧容量（6 字节）时，LinTp 使用首帧（FF）、连续帧（CF）和流控帧（FC）机制实现多帧传输。

主要功能：
- **单帧（SF）传输**：<= 6 字节数据的单帧直接传输
- **多帧传输**：FF + CF + FC 机制，支持最大 4095 字节数据
- **流控管理**：BlockSize 和 STmin 参数控制传输速率
- **多通道支持**：最多 2 个 LIN 通道并发传输
- **PduR 集成**：通过 PduR 的 CopyTxData/CopyRxData/StartOfReception 接口传输数据
- **超时管理**：N_As / N_Cs / N_Cr 三个超时定时器

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| ISO 17987-2 | Latest | LIN 传输协议规范 |
| AUTOSAR SWS LIN Transport Protocol | 4.4.0 | 模块软件规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | PduR | 路由层，数据拷贝与接收通知 |
| 同层 | LinIf | LIN 接口层，帧收发与触发传输 |
| 公共 | Det | 开发错误检测与报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│         PduR / Application          │
├─────────────────────────────────────┤
│         LinTp (ECUAL)               │
├─────────────────────────────────────┤
│         LinIf (LIN Interface)       │
├─────────────────────────────────────┤
│         Lin Driver                  │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **TX 状态机**：管理发送流程（SF → FF → CF → WAIT_FC → COMPLETED）
- **RX 状态机**：管理接收流程（WAIT_FF → WAIT_CF → SEND_FC → COMPLETED）
- **PCI 处理器**：构建和解析协议控制信息（SF/FF/CF/FC）
- **通道运行时管理**：每个通道维护序列号、块计数、超时等状态
- **缓冲区管理**：静态分配 TX/RX 缓冲区（每通道 4095 字节）

### 3.3 文件结构

```
src/bsw/ecual/linTp/
├── include/
│   ├── LinTp.h          -- 公共 API 与类型定义
│   └── LinTp_Cfg.h      -- 预编译配置参数
└── src/
    ├── LinTp.c           -- 核心实现
    └── LinTp_Lcfg.c      -- 链接时配置数据
```

---

## 4. 状态机

### 4.1 通道状态

```
LINTP_CH_IDLE -- Transmit() --> LINTP_CH_TX_ACTIVE
LINTP_CH_IDLE -- RxIndication(SF/FF) --> LINTP_CH_RX_ACTIVE
LINTP_CH_TX_ACTIVE -- 完成 --> LINTP_CH_IDLE
LINTP_CH_TX_ACTIVE -- 超时/错误 --> LINTP_CH_IDLE
LINTP_CH_RX_ACTIVE -- 完成 --> LINTP_CH_IDLE
LINTP_CH_RX_ACTIVE -- 超时/错误 --> LINTP_CH_IDLE
```

### 4.2 TX 子状态

```
TX_IDLE -- Transmit() --> TX_SF (数据 <= 6 字节)
TX_IDLE -- Transmit() --> TX_FF (数据 > 6 字节)
TX_SF -- TxConfirm --> TX_COMPLETED
TX_FF -- TxConfirm --> TX_WAIT_FC
TX_WAIT_FC -- FC(CTS) --> TX_CF
TX_WAIT_FC -- FC(WAIT) --> TX_WAIT_FC (WftCount++)
TX_WAIT_FC -- FC(OVFLW) --> TX_ERROR
TX_CF -- TxConfirm(最后一包) --> TX_COMPLETED
TX_CF -- TxConfirm(还有数据) --> TX_CF (SN++)
TX_CF -- 块完成 --> TX_WAIT_FC (等待下一个 FC)
```

### 4.3 RX 子状态

```
RX_IDLE -- SF 接收 --> RX_COMPLETED (直接完成)
RX_IDLE -- FF 接收 --> RX_SEND_FC
RX_SEND_FC -- FC 发送 --> RX_WAIT_CF
RX_WAIT_CF -- CF 接收(正确 SN) --> RX_WAIT_CF (SN++)
RX_WAIT_CF -- CF 接收(块完成) --> RX_SEND_FC (发新 FC)
RX_WAIT_CF -- 所有数据收完 --> RX_COMPLETED
RX_WAIT_CF -- SN 错误 --> RX_ERROR
```

### 4.4 PCI 帧格式

**单帧 (SF)：**
```
Byte 0: [0x00 | DL(4bit)]  -- PCI 类型 + 数据长度
Byte 1~N: 数据 (最多 6 字节)
```

**首帧 (FF)：**
```
Byte 0: [0x10 | DL_high(4bit)]  -- PCI 类型 + 数据长度高 4 位
Byte 1: DL_low(8bit)             -- 数据长度低 8 位
Byte 2~6: 数据 (5 字节)
```

**连续帧 (CF)：**
```
Byte 0: [0x20 | SN(4bit)]  -- PCI 类型 + 序列号
Byte 1~6: 数据 (6 字节)
```

**流控帧 (FC)：**
```
Byte 0: [0x30 | FS(4bit)]  -- PCI 类型 + 流控状态
Byte 1: BlockSize           -- 块大小
Byte 2: STmin               -- 最小分隔时间
```

---

## 5. 核心数据结构

### 5.1 通道配置

```c
typedef struct {
    uint16 ChannelId;              /* 通道 ID */
    uint16 LinIfChannelId;         /* 关联 LinIf 通道 */
    uint32 N_As;                   /* 发送响应超时 (ms) */
    uint32 N_Cs;                   /* 发送确认超时 (ms) */
    uint32 N_Cr;                   /* 接收确认超时 (ms) */
    uint8 DefaultBs;               /* 默认块大小 */
    uint8 DefaultStMin;            /* 默认最小分隔时间 (ms) */
    boolean TransmitCancellation;  /* 支持发送取消 */
} LinTp_ChannelConfigType;
```

### 5.2 NSDU 配置

```c
typedef struct {
    uint16 NsduId;                 /* N-SDU ID */
    uint16 ChannelId;              /* 关联通道 */
    uint16 PduRNSduId;             /* PduR N-SDU ID */
    uint8 Address;                 /* 网络源地址 */
    boolean IsTx;                  /* TRUE=TX, FALSE=RX */
} LinTp_NsduConfigType;
```

### 5.3 通道运行时数据

```c
typedef struct {
    LinTp_ChannelStateType State;     /* 通道状态 */
    LinTp_TxStateType TxState;        /* TX 子状态 */
    LinTp_RxStateType RxState;        /* RX 子状态 */
    uint16 CurrentNsduId;             /* 当前 N-SDU */
    PduLengthType DataLength;         /* 总数据长度 */
    PduLengthType DataIndex;          /* 当前数据索引 */
    uint8 SequenceNumber;             /* CF 序列号 (0~15) */
    uint8 BlockSize;                  /* 当前块大小 */
    uint8 BlockCount;                 /* 当前块已发/收帧数 */
    uint8 StMin;                      /* 最小分隔时间 */
    uint8 WftCount;                   /* Wait 帧计数 */
    uint32 TimeoutCounter;            /* 超时计数器 */
    boolean BufferProvided;           /* PduR 已提供缓冲 */
    uint8 *TxBuffer;                  /* TX 缓冲区指针 */
    uint8 *RxBuffer;                  /* RX 缓冲区指针 */
} LinTp_ChannelRuntimeType;
```

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `LinTp_Init(ConfigPtr)` | 0x01 | 初始化模块 | SWS_LinTp_00001 |
| `LinTp_DeInit()` | 0x02 | 反初始化模块 | SWS_LinTp_00002 |
| `LinTp_Transmit(TxPduId, PduInfoPtr)` | 0x03 | 请求发送 | SWS_LinTp_00003 |
| `LinTp_CancelTransmit(TxPduId)` | 0x04 | 取消发送 | SWS_LinTp_00004 |
| `LinTp_CancelReceive(RxPduId)` | 0x05 | 取消接收 | SWS_LinTp_00005 |
| `LinTp_ChangeParameter(PduId, Parameter, Value)` | 0x06 | 修改参数 (BS/STmin) | SWS_LinTp_00006 |
| `LinTp_GetVersionInfo(VersionInfo)` | 0x07 | 获取版本信息 | SWS_LinTp_00007 |
| `LinTp_MainFunction()` | 0x08 | 周期处理函数 | SWS_LinTp_00008 |

### 6.2 回调函数

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `LinTp_RxIndication(RxPduId, PduInfoPtr)` | 0x42 (66) | LinIf 接收指示 | SWS_LinTp_00100 |
| `LinTp_TxConfirmation(TxPduId, result)` | 0x43 (67) | LinIf 发送确认 | SWS_LinTp_00101 |
| `LinTp_TriggerTransmit(TxPduId, PduInfoPtr)` | 0x44 (68) | LinIf 触发发送（提供数据） | SWS_LinTp_00102 |

### 6.3 错误码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `LINTP_E_UNINIT` | 0x00 | 模块未初始化 |
| `LINTP_E_INVALID_PDU_SDU_ID` | 0x01 | 无效 PDU/SDU ID |
| `LINTP_E_PARAM_POINTER` | 0x02 | 空指针 |
| `LINTP_E_INVALID_PARAMETER` | 0x03 | 无效参数 |
| `LINTP_E_INIT_FAILED` | 0x04 | 初始化失败 |
| `LINTP_E_INVALID_TX_ID` | 0x05 | 无效 TX ID |
| `LINTP_E_INVALID_RX_ID` | 0x06 | 无效 RX ID |
| `LINTP_E_INVALD_NSDU_ID` | 0x20 (32) | 无效 N-SDU ID |
| `LINTP_E_INVALD_NSA` | 0x30 (48) | 无效网络地址 |
| `LINTP_E_TIMEOUT` | 0x70 (112) | 超时 |
| `LINTP_E_INVALID_FRAME` | 0x71 (113) | 无效帧 |
| `LINTP_E_BUFFER_OVERFLOW` | 0x72 (114) | 缓冲区溢出 |
| `LINTP_E_SEQUENCE_ERROR` | 0x73 (115) | 序列号错误 |
| `LINTP_E_INVALID_FC` | 0x74 (116) | 无效流控帧 |

---

## 7. 处理流程

### 7.1 发送流程

1. `LinTp_Transmit()` 被调用
2. 根据数据长度判断帧类型：
   - <= `LINTP_SF_MAX_DATA_LENGTH` (6 字节) → SF 模式
   - > 6 字节 → FF 模式
3. SF 模式：
   - 设置 `TxState = LINTP_TX_SF`
   - 通过 `TriggerTransmit` 提供 SF 数据
   - TxConfirmation 后完成
4. FF 模式：
   - 设置 `TxState = LINTP_TX_FF`
   - 通过 `TriggerTransmit` 提供 FF 数据（5 字节载荷）
   - TxConfirmation 后进入 `TX_WAIT_FC`
5. 收到 FC(CTS)：
   - 更新 BlockSize 和 STmin
   - 切换到 `TX_CF`，开始发送连续帧
6. CF 发送：
   - 每帧 6 字节载荷，序列号递增 (0~15 循环)
   - 块计数达到 BlockSize 后等待下一个 FC
   - 所有数据发完后通知 PduR

### 7.2 接收流程

1. `LinTp_RxIndication()` 被 LinIf 调用
2. 解析 PCI 类型（首字节高 4 位）：
   - `0x00` → SF 处理
   - `0x10` → FF 处理
   - `0x20` → CF 处理
   - `0x30` → FC 处理
3. SF 接收：
   - 调用 `PduR_LinTpStartOfReception()` 请求缓冲
   - 调用 `PduR_LinTpCopyRxData()` 拷贝数据
   - 通知 `PduR_LinTpRxIndication()` 完成
4. FF 接收：
   - 解析 12 位数据长度
   - 请求 PduR 缓冲
   - 发送 FC(CTS) 开始接收 CF
5. CF 接收：
   - 验证序列号
   - 拷贝 6 字节载荷到 PduR
   - 块完成后发送新 FC
   - 全部完成后通知 PduR

### 7.3 TriggerTransmit 流程

`LinTp_TriggerTransmit()` 由 LinIf 在调度表触发时调用，根据当前 TX 状态提供帧数据：
- `TX_SF`：构建 SF PCI + 数据
- `TX_FF`：构建 FF PCI + 首 5 字节数据
- `TX_CF`：构建 CF PCI + 6 字节数据
- `RX_SEND_FC`：构建 FC PCI + BS + STmin

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `LINTP_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `LINTP_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `LINTP_MAX_CHANNEL_COUNT` | 2 | 最大通道数 |
| `LINTP_NSDU_COUNT` | 4 | N-SDU 数量 |
| `LINTP_CANCEL_TRANSMIT_API` | STD_ON | 发送取消 API |
| `LINTP_CANCEL_RECEIVE_API` | STD_ON | 接收取消 API |
| `LINTP_CHANGE_PARAMETER_API` | STD_ON | 参数修改 API |
| `LINTP_SUPPORT_WAIT_FRAMES` | STD_ON | Wait 帧支持 |
| `LINTP_BUFFER_SIZE` | 4095 | 缓冲区大小 |
| `LINTP_SF_MAX_DATA_LENGTH` | 6 | SF 最大数据长度 |
| `LINTP_FF_MAX_DATA_LENGTH` | 5 | FF 最大数据长度 |
| `LINTP_CF_MAX_DATA_LENGTH` | 6 | CF 最大数据长度 |
| `LINTP_BS_DEFAULT` | 8 | 默认块大小 |
| `LINTP_STMIN_DEFAULT` | 20 | 默认 STmin (ms) |
| `LINTP_MAX_WFT` | 10 | 最大 Wait 帧数 |
| `LINTP_NAS_DEFAULT` | 1000 | N_As 默认值 (ms) |
| `LINTP_NCS_DEFAULT` | 1000 | N_Cs 默认值 (ms) |
| `LINTP_NCR_DEFAULT` | 1000 | N_Cr 默认值 (ms) |

### 8.2 链接时配置

链接时配置定义在 `LinTp_Lcfg.c` 中：

**通道配置：**
- Channel 0 (Master): N_As=1000ms, BS=8, STmin=20ms
- Channel 1 (Slave): N_As=1000ms, BS=8, STmin=20ms

**N-SDU 配置：**
- NSDU 0: TX, Channel 0, NSA=0x01 (诊断主节点)
- NSDU 1: RX, Channel 0, NSA=0x10 (诊断从节点)
- NSDU 2: TX, Channel 1, NSA=0x7E (功能广播地址)
- NSDU 3: RX, Channel 1, NSA=0x11 (诊断从节点 2)

### 8.3 构建后配置

支持构建后配置变体（`LINTP_POSTBUILD_VARIANTS`）：
- Variant 1: 标准配置（N_As=1000ms, BS=8, STmin=20ms）
- Variant 2: 扩展时序配置（N_As=2000ms, BS=16, STmin=10ms）

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有 API 包含完整的参数验证：
- 未初始化 → `LINTP_E_UNINIT`
- 空指针 → `LINTP_E_PARAM_POINTER`
- 无效 N-SDU ID → `LINTP_E_INVALID_TX_ID` / `LINTP_E_INVALID_RX_ID`
- 无效参数 → `LINTP_E_INVALID_PARAMETER`

运行时错误通过 `Det_ReportRuntimeError` 报告：
- 无效帧 → `LINTP_E_INVALID_FRAME`
- 序列号错误 → `LINTP_E_SEQUENCE_ERROR`
- 无效 FC → `LINTP_E_INVALID_FC`

### 9.2 DEM 错误

当前实现未报告 DEM 事件。

### 9.3 安全机制

- **MemMap 保护**：所有变量和代码段使用标准 MemMap 分区
- **序列号验证**：CF 接收时严格验证 SN 连续性
- **Wait 帧限制**：最多 `LINTP_MAX_WFT` (10) 个 Wait 帧，防止无限等待
- **超时保护**：N_As / N_Cs / N_Cr 三个超时定时器
- **缓冲区溢出保护**：数据长度超过 `LINTP_BUFFER_SIZE` 时发送 FC(OVFLW)

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 类型 | 说明 |
|------|------|------|
| `LINTP_START_SEC_VAR_CLEARED_UNSPECIFIED` | 已清零变量 | 内部状态、缓冲区 |
| `LINTP_START_SEC_CONST_UNSPECIFIED` | 常量数据 | 通道/NSDU 配置 |
| `LINTP_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据 | 模块配置结构体 |
| `LINTP_START_SEC_CODE` | 代码段 | 所有 API 函数 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| `LinTp_Internal` | ~80 bytes | 模块内部状态 |
| `LinTp_TxBuffer[2][4095]` | 8190 bytes | TX 缓冲区 |
| `LinTp_RxBuffer[2][4095]` | 8190 bytes | RX 缓冲区 |
| 代码段 | ~5 KB (估算) | 状态机 + PCI 处理 |

---

## 11. 集成指南

1. **LinIf 集成**：
   - `LinTp_RxIndication` → LinIf 接收回调
   - `LinTp_TxConfirmation` → LinIf 发送确认回调
   - `LinTp_TriggerTransmit` → LinIf 调度表触发回调
2. **PduR 集成**：
   - `PduR_LinTpStartOfReception` → 接收缓冲请求
   - `PduR_LinTpCopyRxData` → 接收数据拷贝
   - `PduR_LinTpCopyTxData` → 发送数据请求
   - `PduR_LinTpRxIndication` → 接收完成通知
   - `PduR_LinTpTxConfirmation` → 发送完成通知
3. **调度配置**：`LinTp_MainFunction()` 建议 1ms 周期调用
4. **NSDU 映射**：确保 LinTp N-SDU ID 与 PduR N-SDU ID 正确映射

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| Init/DeInit | 通道重置、缓冲区初始化 |
| SF 发送/接收 | 单帧完整流程 |
| FF/CF/FC 发送 | 多帧发送流程 |
| FF/CF/FC 接收 | 多帧接收与重组 |
| 序列号验证 | 错误 SN 检测 |
| 流控处理 | CTS/WAIT/OVFLW 处理 |
| 超时处理 | N_As/N_Cs/N_Cr 超时 |
| 参数修改 | BS/STmin 动态修改 |
| 发送/接收取消 | CancelTransmit/CancelReceive |
| Wait 帧限制 | WftCount 超限中止 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| PduR 集成 | 数据拷贝与通知 |
| LinIf 集成 | 帧收发与 TriggerTransmit |
| 大数据传输 | 4095 字节极限传输 |
| 多通道并发 | 2 通道同时传输 |

---

## 13. 实现说明 / TODO

- **ProvideRxBuffer**：当前返回 `E_OK` 但无实际操作，缓冲由静态数组提供
- **SF 接收中的 PduR 交互**：`CurrentNsduId` 在 SF 接收前未从 NsduConfig 中正确设置
- **FF 接收中的 PduR 交互**：同上，`CurrentNsduId` 需在 FF 处理前设置
- **STmin 延迟**：CF 发送间的 STmin 延迟依赖 LIN 调度表，未显式实现
- **扩展寻址**：`LINTP_EXTENDED_ADDRESSING_SUPPORT = STD_OFF`，未实现

---

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| ISO 17987-2 | LIN 传输协议规范 |
| AUTOSAR_SWS_LINTransportProtocol | AUTOSAR LIN TP 模块规范 |
| AUTOSAR_SWS_PduRouter | PDU 路由器规范 |
| `src/bsw/ecual/linTp/` | 源代码目录 |
