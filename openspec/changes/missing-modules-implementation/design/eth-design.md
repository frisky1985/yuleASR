# Eth (Ethernet Driver) 设计规格

## 1. 模块概述

### 1.1 功能说明
Eth 模块提供对以太网 MAC 控制器的统一访问接口，支持:
- MAC 初始化与配置
- 数据帧发送/接收
- 物理层事件通知
- PHY 管理接口

### 1.2 AUTOSAR 版本
基于 AUTOSAR Classic Platform 4.4.0, Eth 驱动规范

### 1.3 依赖模块
| 模块 | 作用 |
|------|------|
| Det  | 错误检测与报告 |
| EcuM | 初始化协调 |
| Port (驱动) | Pin配置 |

## 2. 架构设计

### 2.1 文件结构
```
src/bsw/mcal/eth/
├── include/
│   ├── Eth.h          # 外部API头文件 (已存在)
│   ├── Eth_Cfg.h      # 配置头文件 (已存在)
│   ├── Eth_Lcfg.h     # 链接配置头文件
│   └── Eth_Private.h  # 私有头文件
└── src/
    ├── Eth.c          # 主实现文件 (待实现)
    └── Eth_Irq.c      # 中断处理
```

### 2.2 核心数据结构
```c
/* Eth控制器状态 */
typedef struct {
    Eth_StateType state;           /* 模块状态 */
    Eth_ModeType mode;             /* 当前模式 */
    uint8 ctrlIdx;                 /* 控制器索引 */
    boolean initDone;              /* 初始化完成标志 */
} Eth_CtrlStateType;

/* 发送描述符 */
typedef struct {
    uint8* dataPtr;                /* 数据指针 */
    uint16 len;                    /* 数据长度 */
    Eth_BufIdxType bufIdx;         /* 缓冲区索引 */
} Eth_TxDescType;

/* 接收描述符 */
typedef struct {
    uint8* dataPtr;                /* 数据指针 */
    uint16 len;                    /* 数据长度 */
    Eth_TimeStampType timestamp;   /* 收到时间戳 */
} Eth_RxDescType;
```

## 3. API 设计

### 3.1 核心函数

| 函数名 | 功能 | ASIL 等级 |
|--------|------|-----------|
| Eth_Init | 初始化Eth模块 | QM |
| Eth_ControllerInit | 初始化MAC控制器 | QM |
| Eth_SetControllerMode | 设置控制器模式 | QM |
| Eth_GetControllerMode | 获取当前模式 | QM |
| Eth_WriteMII | 写入PHY寄存器 | QM |
| Eth_ReadMII | 读取PHY寄存器 | QM |
| Eth_GetPhyAddress | 获取PHY地址 | QM |
| Eth_ProvideTxBuffer | 获取发送缓冲区 | QM |
| Eth_Transmit | 发送数据帧 | QM |
| Eth_Receive | 接收数据帧 | QM |
| Eth_TxConfirmation | 发送确认回调 | QM |
| Eth_RxIndication | 接收指示回调 | QM |

### 3.2 安全机制

```c
/* 运行时错误检测 */
#if (ETH_DEV_ERROR_DETECT == STD_ON)
    #define ETH_REPORT_ERROR(ApiId, ErrorId)         Det_ReportError(ETH_MODULE_ID, ETH_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define ETH_REPORT_ERROR(ApiId, ErrorId)
#endif

/* 状态检查宏 */
#define ETH_CHECK_STATE_INIT()     do { if (!Eth_InternalState.initDone) {         ETH_REPORT_ERROR(ETH_SID_INIT, ETH_E_UNINIT);         return E_NOT_OK;     }} while(0)
```

## 4. 状态机

```
                    +-----------+
                    |  ETH_UNINIT |
                    +-----+-----+
                          | Eth_Init()
                          v
                    +-----------+
              +---->|  ETH_INIT   |
              |     +-----+-----+
              |           | Eth_ControllerInit()
              |           v
              |     +-----------+
       active | +-->| ETH_ACTIVE  |<--+
              | |   +-----+-----+    |
              | |         |          | Eth_SetControllerMode(ACTIVE)
              | |         | Eth_SetControllerMode(DOWN)
              | |         v          |
              | |   +-----------+    |
              +-----| ETH_DOWN  |----+
                    +-----------+
```

## 5. 实现计划

### 5.1 阶段任务

| 阶段 | 任务 | 估计工时 |
|-----|------|---------|
| 1 | Eth.h 完善 (API补全) | 4h |
| 2 | Eth.c 基础框架 | 6h |
| 3 | 初始化逻辑 | 4h |
| 4 | 发送功能 | 6h |
| 5 | 接收功能 | 6h |
| 6 | PHY管理 | 4h |
| 7 | 中断处理 | 4h |
| 8 | 单元测试 | 8h |
| **合计** | | **42h (~1周)** |

### 5.2 安全考量
- 所有中断服务程序使用最小化代码
- 关键数据结构使用 volatile
- 完整的错误检测和报告

---
设计版本: 1.0
设计状态: 待审核
