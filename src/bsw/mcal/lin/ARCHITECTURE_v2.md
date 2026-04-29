# LinSlave v2.0 架构设计文档

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2024-xx-xx | 初始版本 |
| 2.0.0 | 2024-xx-xx | 配置表支持 + TP + UDS |

## 1. 系统概述

### 1.1 设计目标

LinSlave v2.0 是一个通用的 LIN 从机协议实现，支持：

- 配置表驱动的PID管理 (替代硬编码)
- LIN TP (Transport Protocol) 多帧传输
- UDS (Unified Diagnostic Services) 诊断服务

### 1.2 架构层次

```
┌────────────────────────────────────────────────────────────────┐
│                        应用层                             │
│  [诊断服务]    [信号处理]    [网络管理]                  │
└────────────────────────────────────────────────────────────────┘
                              │
┌────────────────────────────────────────────────────────────────┐
│                       UDS层 (ISO 14229-1)                     │
│  LinSlave_Uds.h/c - 诊断服务调度器                            │
└────────────────────────────────────────────────────────────────┘
                              │
┌────────────────────────────────────────────────────────────────┐
│                    TP层 (ISO 17987)                          │
│  LinSlave_Tp.h/c - 多帧传输协议                          │
│  SF(单帧) / FF(首帧) / CF(连续帧) / FC(流控)         │
└────────────────────────────────────────────────────────────────┘
                              │
┌────────────────────────────────────────────────────────────────┐
│                核心层 (Lin Slave v2.0)                     │
│  LinSlave.h/c - 状态机、配置表管理、报文处理               │
└────────────────────────────────────────────────────────────────┘
                              │
┌────────────────────────────────────────────────────────────────┐
│                     HAL层                                  │
│  LinSlave_Hal.h/c - UART驱动、中断管理                     │
└────────────────────────────────────────────────────────────────┘
```

## 2. 配置表设计

### 2.1 核心结构

```c
/* PID配置条目 */
typedef struct {
    uint8 Pid;                      /* Protected ID */
    LinSlave_DirectionType Direction;     /* RX/TX/RX_TX */
    LinSlave_ChecksumType ChecksumType;   /* CLASSIC/ENHANCED */
    LinSlave_MsgTypeType MsgType;         /* SIGNAL/DIAGNOSTIC */
    LinSlave_TPType TPType;               /* NONE/LIN_TP */
    uint8 DataLength;               /* 数据长度 (1-8) */
    union {
        LinSlave_DataCallbackFuncType DataCallback;
        LinSlave_DiagCallbackFuncType DiagCallback;
    } Callback;
    LinSlave_FrameErrorCallbackFuncType ErrorCallback;
} LinSlave_PidConfigEntryType;

/* 配置表 */
typedef struct {
    uint8 VersionMajor;
    uint8 VersionMinor;
    uint8 VersionPatch;
    uint8 NodeId;
    uint32 BaudRate;
    uint8 EntryCount;
    const LinSlave_PidConfigType* const* Entries;
} LinSlave_ConfigTableType;
```

### 2.2 配置流程

1. 创建 `LinSlave_PidConfigEntryType` 数组
2. 填充回调函数
3. 创建 `LinSlave_ConfigTableType`
4. 调用 `LinSlave_InitWithConfigTable()`

### 2.3 示例配置

```c
static const LinSlave_PidConfigEntryType PidEntry_0x10 = {
    0x10,                           /* PID */
    LINSLAVE_DIR_RX,                /* 接收 */
    LINSLAVE_CSUM_CLASSIC,          /* 经典校验和 */
    LINSLAVE_MSG_TYPE_SIGNAL,       /* 信号报文 */
    LINSLAVE_TP_NONE,               /* 无TP */
    8,                              /* 8字节 */
    {.DataCallback = SignalRxCallback},
    FrameErrorCallback
};
```

## 3. TP协议设计

### 3.1 协议标识 (PCI)

| 类型 | PCI范围 | 说明 |
|------|---------|------|
| SF (Single Frame) | 0x00-0x07 | 单帧, 数据长度在1-7字节 |
| FF (First Frame) | 0x10-0x1F | 首帧, 数据长度>6字节 |
| CF (Consecutive Frame) | 0x20-0x2F | 连续帧, SN序列号 |
| FC (Flow Control) | 0x30-0x3F | 流控帧, BS+STmin |

### 3.2 状态机

```
IDLE -> RX_FF -> RX_CF -> IDLE
          ↓
      TX_FC (FlowControl)

IDLE -> TX_FF -> WAIT_FC -> TX_CF -> IDLE
```

### 3.3 API接口

```c
LinSlave_Tp_StatusType LinSlave_Tp_Init(void);
LinSlave_Tp_StatusType LinSlave_Tp_Transmit(uint8 ChannelId, const uint8* DataPtr, uint16 Length);
LinSlave_Tp_StatusType LinSlave_Tp_ProcessFrame(uint8 Pid, uint8 Pci, const uint8* DataPtr, uint8 Length);
void LinSlave_Tp_MainFunction(void);
```

## 4. UDS框架设计

### 4.1 服务注册

```c
typedef LinSlave_Uds_StatusType (*LinSlave_Uds_ServiceHandlerFuncType)(
    const LinSlave_Uds_RequestType* Request,
    LinSlave_Uds_ResponseType* Response
);

typedef struct {
    uint8 Sid;                              /* 服务ID */
    LinSlave_Uds_ServiceHandlerFuncType Handler;
    boolean NeedsSecurity;                  /* 需要安全验证 */
    boolean NeedsSession;                   /* 需要特定会话 */
    LinSlave_Uds_SessionType MinSession;    /* 最小会话 */
    LinSlave_Uds_SecurityLevelType MinSecurityLevel;
} LinSlave_Uds_ServiceConfigType;
```

### 4.2 内置服务

| SID | 服务 | 说明 |
|-----|------|------|
| 0x10 | DiagnosticSessionControl | 诊断会话控制 |
| 0x11 | ECUReset | ECU复位 |
| 0x3E | TesterPresent | 保活检测 |

### 4.3 负响响应码 (NRC)

```c
#define LINSLAVE_UDS_NRC_SERVICE_NOT_SUPPORTED      0x11
#define LINSLAVE_UDS_NRC_SUBFUNCTION_NOT_SUPPORTED  0x12
#define LINSLAVE_UDS_NRC_INCORRECT_MESSAGE_LENGTH   0x13
#define LINSLAVE_UDS_NRC_SECURITY_ACCESS_DENIED     0x33
```

## 5. 状态机设计

### 5.1 状态定义

```c
typedef enum {
    LINSLAVE_STATE_UNINIT = 0,      /* 未初始化 */
    LINSLAVE_STATE_IDLE,            /* 空闲 */
    LINSLAVE_STATE_RX_BREAK,        /* 接收Break */
    LINSLAVE_STATE_RX_SYNC,         /* 接收Sync */
    LINSLAVE_STATE_RX_DATA,         /* 接收数据 */
    LINSLAVE_STATE_RX_CSUM,         /* 接收校验和 */
    LINSLAVE_STATE_TX_RESPONSE      /* 发送响应 */
} LinSlave_StateType;
```

### 5.2 状态转换

```
【正常接收流程】
IDLE --(Break)--> RX_BREAK --(0x55)--> RX_SYNC --(PID)--> RX_DATA --(Data)--> RX_CSUM --(Checksum)--> IDLE
                                                                          ↓
                                                                    发送响应
```

## 6. 文件组织

```
include/
├── LinSlave.h           # 主头文件
├── LinSlave_Cfg.h       # 配置头文件
├── LinSlave_Types.h     # 类型定义
├── LinSlave_CfgTable.h  # 配置表头文件 (v2.0)
├── LinSlave_Tp.h        # TP头文件 (v2.0)
└── LinSlave_Uds.h       # UDS头文件 (v2.0)

src/
├── LinSlave.c           # 主实现
├── LinSlave_Pid.c       # PID处理
├── LinSlave_Checksum.c  # 校验和计算
├── LinSlave_CfgTable.c  # 配置表实现 (v2.0)
├── LinSlave_Tp.c        # TP实现 (v2.0)
├── LinSlave_Uds.c       # UDS实现 (v2.0)
└── LinSlave_Hal.c       # HAL层

example/
└── LinSlave_ExampleCfg.c # 配置示例

tests/
└── test_linslave_v2.c   # 测试套件
```

## 7. 使用示例

### 7.1 基本初始化

```c
#include "LinSlave.h"

int main(void)
{
    /* 方式1: 原始方式 */
    LinSlave_Init(&LinSlave_DefaultConfig);
    
    /* 方式2: 配置表方式 (v2.0) */
    LinSlave_InitWithConfigTable(&LinSlave_ConfigTable);
    
    while (1) {
        /* 周期调用主函数 (10ms) */
        LinSlave_MainFunction();
    }
}
```

### 7.2 中断处理

```c
void UART_IRQHandler(void)
{
    uint8 rxByte = UART->DR;
    
    if (UART->SR & UART_SR_BREAK) {
        LinSlave_BreakDetected();
    } else {
        LinSlave_RxInterruptHandler(rxByte);
    }
}
```

## 8. 编译参考

| 参数 | 值 | 说明 |
|------|------|------|
| LINSLAVE_MAX_PID_ENTRIES | 16 | 最大PID条目数 |
| LINSLAVE_TP_MAX_FRAME_LEN | 4095 | TP最大帧长度 |
| LINSLAVE_TP_MAX_PDUs | 2 | 最大TP通道数 |
| LINSLAVE_UDS_MAX_SID_SERVICES | 32 | 最大服务数 |
| LINSLAVE_UDS_MAX_DID | 256 | 最大DID数 |

## 9. 版本说明

### v2.0.0 新增功能

1. 配置表驱动的PID管理
2. LIN TP多帧传输支持
3. UDS诊断服务框架
4. 动态服务注册
5. 会话管理
6. 安全级别支持

### 兼容性

- 保持与v1.0的API兼容
- 新增 `LinSlave_InitWithConfigTable()` 接口
- 新增 `LinSlave_MainFunction()` 周期函数
