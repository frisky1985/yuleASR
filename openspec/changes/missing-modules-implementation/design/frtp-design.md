# FrTp (FlexRay Transport Protocol) 设计规格

## 1. 模块概述

### 1.1 功能说明
FrTp 模块提供基于 FlexRay 的数据分片传输功能:
- 大数据帧分片传输 (类似 CAN TP)
- 流量控制
- 可靠传输 (带确认机制)
- 多连接支持

### 1.2 AUTOSAR 版本
基于 AUTOSAR Classic Platform 4.4.0, FrTp 规范

### 1.3 依赖模块
| 模块 | 作用 |
|------|------|
| FrIf | FlexRay 接口 |
| Det  | 错误检测 |
| PduR | PDU 路由 |

## 2. 架构设计

### 2.1 文件结构
```
src/bsw/ecual/frtp/
├── include/
│   ├── FrTp.h          # 外部API头文件
│   ├── FrTp_Cfg.h      # 配置头文件
│   ├── FrTp_Lcfg.h     # 链接配置头文件
│   └── FrTp_Private.h  # 私有头文件
└── src/
    ├── FrTp.c          # 主实现
    ├── FrTp_Rx.c       # 接收逻辑
    ├── FrTp_Tx.c       # 发送逻辑
    └── FrTp_TxSm.c     # 发送状态机
```

### 2.2 核心数据结构
```c
/* FrTp连接配置 */
typedef struct {
    FrTp_ConnectionIdxType connIdx;     /* 连接索引 */
    PduIdType txPduId;                   /* 发送PDU ID */
    PduIdType rxPduId;                   /* 接收PDU ID */
    uint16 maxPayload;                   /* 最大负载 */
    uint8 maxRetries;                    /* 最大重试次数 */
    uint16 timeoutAs;                    /* 确认超时 */
    uint16 timeoutBs;                    /* 等待超时 */
    uint16 timeoutCs;                    /* 发送超时 */
    boolean flowControlEnabled;          /* 流控使能 */
} FrTp_ConnectionConfigType;

/* 连接状态 */
typedef struct {
    FrTp_ConnectionStateType state;      /* 状态机状态 */
    PduLengthType dataLength;            /* 总数据长度 */
    PduLengthType bytesTransferred;      /* 已传输字节 */
    uint8 sequenceNumber;                /* 序列号 */
    uint8 blockSize;                     /* 块大小 */
    uint8 stMin;                         /* 最小间隔 */
    uint8 retryCount;                    /* 当前重试次数 */
    PduInfoType* pduInfo;                /* 当前PDU信息 */
} FrTp_ConnectionRuntimeType;

/* PDU类型定义 */
typedef enum {
    FRTP_PDU_SF = 0,    /* 单帧 */
    FRTP_PDU_FF,        /* 首帧 */
    FRTP_PDU_CF,        /* 连续帧 */
    FRTP_PDU_FC         /* 流量控制 */
} FrTp_PduType;
```

### 2.3 分片协议

```
发送方:  [FF][FF][CF][CF][CF]...
          ↓  ↓  ↓  ↓  ↓
接收方:  [FC]    [FC]    [FC]

FF (First Frame):  首帧，包含总长度
CF (Consecutive):  连续帧，带序列号
FC (Flow Control): 流量控制，调节速率
SF (Single Frame): 单帧，数据较小时使用
```

## 3. API 设计

| 函数名 | 功能 | 说明 |
|--------|------|------|
| FrTp_Init | 初始化 | 配置所有连接 |
| FrTp_Transmit | 请求发送 | PduR调用 |
| FrTp_CancelTransmit | 取消发送 | |
| FrTp_CancelReceive | 取消接收 | |
| FrTp_ChangeParameter | 修改参数 | STmin/BS |
| FrTp_RxIndication | 接收指示 | FrIf回调 |
| FrTp_TxConfirmation | 发送确认 | FrIf回调 |
| FrTp_MainFunction | 主函数 | 定期调用 |

## 4. 发送状态机

```
                     +-----------+
                     |   IDLE    |
                     +-----+-----+
                           | FrTp_Transmit()
                           v
              +---------------------------+
              |         TX_STARTED        |
              +-------------+-------------+
                            | SF/FF sent
                            v
              +---------------------------+
     +------->|     TX_WAIT_FC / TX_CF    |<-------+
     |        +-------------+-------------+        |
     |                      |                      |
     | FC received          | Last CF sent         | More CF
     | or SF complete       v                      |
     |        +-------------+-------------+        |
     +--------|     TX_WAIT_TX_CONFIRM      |        |
              +---------------------------+        |
                            |                      |
                            | TxConfirmation       |
                            v                      |
                     +-----------+                 |
                     |   IDLE    |-----------------+
                     +-----------+
```

## 5. 实现计划

| 阶段 | 任务 | 估计工时 |
|-----|------|---------|
| 1 | 头文件创建 | 4h |
| 2 | 数据结构定义 | 4h |
| 3 | 初始化逻辑 | 4h |
| 4 | 发送状态机 | 8h |
| 5 | 接收逻辑 | 8h |
| 6 | 流量控制 | 4h |
| 7 | 错误处理 | 4h |
| 8 | 单元测试 | 8h |
| **合计** | | **44h (~1周)** |

---
设计版本: 1.0
