# CanNm Design Document

> **Module ID**: 0x1F (31)  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS CAN Network Management  
> **Source Path**: `src/bsw/ecual/canNm/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

CanNm (CAN Network Management) 负责 CAN 总线上的网络状态协调，确保所有节点在网络通信（Network Mode）和总线休眠（Bus-Sleep Mode）之间同步切换。通过 NM PDU 的周期广播和超时检测，实现分布式网络状态一致性。CanNm 与 CanSM 协同工作：CanSM 发出网络模式请求，CanNm 负责 NM 协议层面的实现。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS CAN Network Management | 4.4.0 | CAN NM 协议规范 |
| OSEK/VDX NM | 3.0 | 直接 NM 协议基础 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | CanSM | 请求网络模式切换 |
| 上层 | ComM | 通信管理器通知 |
| 下层 | CanIf | NM PDU 收发 |
| 下层 | Det | 错误报告 |
| 同层 | NM | NM 通用接口 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│      CanSM / ComM / NM              │
├─────────────────────────────────────┤
│         CanNm (ECUAL)               │
├─────────────────────────────────────┤
│         CanIf / Can                 │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **NM State Machine**: 控制 Network/Ready-Sleep/Bus-Sleep/Repeat Message 状态
- **NM PDU Handler**: 构造/解析 NM PDU (含 Source Node ID、状态向量)
- **Timeout Monitor**: 监控 NM PDU 超时（Timeout 检测节点掉线）
- **Repeat Message Timer**: 控制 Repeat Message State 持续时间

### 3.3 文件结构

```
src/bsw/ecual/canNm/
├── include/
│   ├── CanNm.h       # 公共 API
│   └── CanNm_Cfg.h   # 通道配置
└── src/
    ├── CanNm.c        # 核心实现
    └── CanNm_Lcfg.c   # 链接时配置
```

---

## 4. 状态机

```
                    NetworkRequest()
  BUS-SLEEP ──────────────────────────► NM-CONFIG
                                           │
                            ┌──────────────┤
                            ▼              ▼
                      NM-READY-SLEEP   NM-NORMAL
                            │              │
                   Timeout     │    Rx NM PDU
                   (all sleep) │    (network active)
                            ▼  │
                       BUS-SLEEP
```

### 状态说明

| 状态 | 说明 |
|------|------|
| Bus-Sleep | 不发送 NM PDU，CAN 控制器可进入 Sleep |
| NM-Config | 网络拓扑正在变化，等待稳定 |
| Normal | 周期广播 NM PDU，网络活跃 |
| Ready-Sleep | 本节点请求睡眠，等待其他节点确认 |

---

## 5. 数据结构

```c
typedef enum {
    CANNM_STATE_BUS_SLEEP = 0,
    CANNM_STATE_READY_SLEEP,
    CANNM_STATE_NORMAL,
    CANNM_STATE_REPEAT_MESSAGE,
    CANNM_STATE_NM_OFF
} CanNm_StateType;

typedef struct {
    uint8  NodeId;           /* 本节点 NM 地址 */
    uint32 MsgCycleTime;     /* NM PDU 发送周期 (ms) */
    uint32 TimeoutTime;      /* NM 超时时间 (ms) */
    uint32 RepeatMsgTime;    /* Repeat Message State 时间 */
    uint32 WaitBusSleepTime; /* 进入 Bus-Sleep 等待时间 */
    PduIdType TxPduId;       /* NM PDU TX PDU ID */
    PduIdType RxPduId;       /* NM PDU RX PDU ID */
} CanNm_ChannelConfigType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void CanNm_Init(const CanNm_ConfigType* Config)` | 初始化 CanNm | SWS_CanNm_00001 |
| `void CanNm_DeInit(void)` | 反初始化 | SWS_CanNm_00002 |
| `Std_ReturnType CanNm_NetworkRequest(uint8 Channel)` | 请求进入网络模式 | SWS_CanNm_00004 |
| `Std_ReturnType CanNm_NetworkRelease(uint8 Channel)` | 释放网络请求（允许睡眠） | SWS_CanNm_00005 |
| `Std_ReturnType CanNm_PassiveStartUp(uint8 Channel)` | 被动启动 | SWS_CanNm_00003 |
| `void CanNm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfo)` | FrIf 接收回调 | SWS_CanNm_00008 |
| `void CanNm_TxConfirmation(PduIdType TxPduId, Std_ReturnType Result)` | 发送确认回调 | SWS_CanNm_00009 |
| `void CanNm_MainFunction(void)` | 周期主函数 | SWS_CanNm_00006 |
| `CanNm_StateType CanNm_GetState(uint8 Channel)` | 获取当前 NM 状态 | SWS_CanNm_00011 |

---

## 7. 处理流程

### 7.1 MainFunction 流程

1. 遍历每个通道
2. 根据当前状态执行对应逻辑：
   - **Normal**: 检查 NM PDU 发送定时器 → 到期则发送 NM PDU
   - **Normal**: 检查远程节点超时 → 超时则进入 Repeat Message
   - **Repeat Message**: 检查 Repeat 定时器 → 到期且无网络请求则进入 Ready-Sleep
   - **Ready-Sleep**: 检查 Wait Bus-Sleep 定时器 → 到期则进入 Bus-Sleep

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `CANNM_MAIN_FUNCTION_PERIOD` | 5U | MainFunction 周期 (ms) |
| `CANNM_MSG_CYCLE_TIME` | 100U | NM PDU 发送周期 (ms) |
| `CANNM_TIMEOUT_TIME` | 500U | 远程节点超时 (ms) |
| `CANNM_REPEAT_MSG_TIME` | 2000U | Repeat Message 持续时间 |
| `CANNM_WAIT_BUS_SLEEP_TIME` | 5000U | Bus-Sleep 等待时间 |
| `CANNM_NODE_ID` | 0x01U | 本节点 NM 地址 |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `CANNM_E_UNINIT` | API 在初始化前调用 |
| `CANNM_E_INV_CHANNEL` | 通道号越界 |
| `CANNM_E_NULL_POINTER` | NULL 指针参数 |

---

## 10. 内存与性能

- **RAM**: 每通道 ~32 字节（状态 + 定时器 + 标志位）
- **ROM**: ~4 KB 代码
- **性能**: MainFunction 每通道 ~5 µs

---

## 11. 集成指南

- CanSM 通过 `CanNm_NetworkRequest/Release` 控制网络状态
- ComM 通过 NM 回调感知网络状态变化
- NM PDU 通过 CanIf 在固定 CAN ID 上收发
- 需配置 NM PDU 的 PDU 长度（通常 8 字节）

---

## 12. 测试策略

- 网络请求/释放状态转换测试
- NM PDU 超时检测测试
- 多节点同步行为测试
- Repeat Message State 时序测试
- Bus-Sleep 进入/退出测试

---

## 13. 实现说明

- NM PDU 格式遵循 OSEK NM: Byte0=Source, Byte1=Status, Byte2-3=Timeout
- 使用 CanIf 的 TX/RX 回调接口
- 定时器基于 MainFunction 周期计数实现

---

## 14. 参考文献

- AUTOSAR_SWS_CANNetworkManagement.pdf (R4.4.0)
- OSEK/VDX Network Management Specification v3.0
- yuleASR CanNm 源码: `src/bsw/ecual/canNm/`
