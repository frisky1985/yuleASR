# UdpNm Design Document

> **Module ID**: 0x31 (49)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_UDPNetworkManagement  
> **Source Path**: `src/bsw/services/udpNm/`  
> **Reference Document**: `docs/modules/udpnm.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

UdpNm（UDP Network Management）是 AUTOSAR BSW 服务层的 UDP 网络管理模块。该模块在以太网之上实现基于 OSEK NM 协议的网络管理功能，通过 UDP 套接字传输 NM PDU，管理以太网节点的通信状态。

UdpNm 与 CanNm 功能类似，但传输层由 CAN 替换为 UDP/IP。模块支持最多 4 个通道，每个通道独立运行 OSEK NM 状态机，实现 Bus Sleep、Repeat Message、Normal Operation、Ready Sleep、Prepare Bus Sleep 等状态转换。UdpNm 通过 SoAd 进行 UDP 消息的发送和接收。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS UDPNetworkManagement | 4.4.0 | UDP 网络管理模块规范 |
| OSEK NM Specification | V2.5.3 | OSEK 网络管理协议基础 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Nm | 通用 NM 接口 |
| 上层 | ComM | 通信管理器 |
| 下层 | SoAd | 套接字适配器（UDP 传输） |
| 下层 | PduR | PDU 路由 |
| 下层 | Det | 开发错误检测 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│     ComM / Nm (上层调用者)           │
├─────────────────────────────────────┤
│     UdpNm (UDP 网络管理)             │
├─────────────────────────────────────┤
│     SoAd (套接字适配器)               │
├─────────────────────────────────────┤
│     TcpIp / EthIf (以太网栈)         │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **OSEK NM 状态机**: 五状态模型（与 CanNm 相同）
- **定时器管理**: NM Timer、Timeout Timer、Wait Bus Sleep Timer、Repeat Message Timer、Immediate Timer
- **PDU 构建/解析**: Node ID + CBV + User Data 的 8 字节 PDU
- **应用回调**: 通过弱符号定义的应用层回调（状态变更通知、休眠模式进入等）
- **通道管理**: 最多 4 个独立通道

### 3.3 文件结构

```
src/bsw/services/udpNm/
├── include/
│   ├── UdpNm.h         # 公共 API 与类型声明
│   └── UdpNm_Cfg.h     # 预编译配置（自动生成）
└── src/
    ├── UdpNm.c          # 核心实现
    └── UdpNm_Lcfg.c     # 链接时配置
```

---

## 4. 状态机

UdpNm 实现了与 CanNm 相同的 OSEK NM 五状态模型：

```
                    NetworkRequest / RxInd
          ┌──────────────────────────────────────┐
          │                                      │
          ▼                                      │
    ┌──────────┐    RxInd/Request    ┌─────────────────┐
    │ Bus Sleep ├───────────────────►│ Repeat Message   │
    │ (0x01)   │                     │ (0x05)           │
    └──────────┘                     └────────┬────────┘
          ▲                                   │
          │ TWbs 超时                         │ RepeatMsg Timer 超时
          │                                   │
    ┌─────┴───────────┐                      ▼
    │Prepare Bus Sleep│         ┌────────────────────┐
    │ (0x02)          │         │ Normal Operation    │
    └─────┬───────────┘         │ (0x04)              │
          ▲                     └────────┬───────────┘
          │ TError 超时                   │ NetworkReleased
          │                              ▼
          │                     ┌─────────────────┐
          └─────────────────────│  Ready Sleep     │
                                │  (0x03)          │
                                └─────────────────┘
```

**状态枚举**:

| 状态 | 值 | 说明 |
|------|----|------|
| `UDPNM_STATE_UNINIT` | 0x00 | 未初始化 |
| `UDPNM_STATE_BUS_SLEEP` | 0x01 | 总线休眠 |
| `UDPNM_STATE_PREPARE_BUS_SLEEP` | 0x02 | 准备总线休眠 |
| `UDPNM_STATE_READY_SLEEP` | 0x03 | 就绪休眠 |
| `UDPNM_STATE_NORMAL_OPERATION` | 0x04 | 正常运行 |
| `UDPNM_STATE_REPEAT_MESSAGE` | 0x05 | 重复消息 |
| `UDPNM_STATE_SYNCHRONIZE` | 0x06 | 同步 |

**模式映射**:

| 模式 | 值 | 说明 |
|------|----|------|
| `UDPNM_MODE_BUS_SLEEP` | 0x00 | 总线休眠模式 |
| `UDPNM_MODE_PREPARE_BUS_SLEEP` | 0x01 | 准备总线休眠模式 |
| `UDPNM_MODE_SYNCHRONIZE` | 0x02 | 同步模式 |
| `UDPNM_MODE_NETWORK` | 0x03 | 网络模式 |

---

## 5. 核心数据结构

### 5.1 通道运行时状态 `UdpNm_InternalChannelType`

```c
typedef struct {
    UdpNm_StateType State;                  // 当前状态
    UdpNm_ModeType Mode;                    // 当前模式
    UdpNm_TimerType TimerNM;                // NM 消息定时器
    UdpNm_TimerType TimerTimeout;           // 超时定时器
    UdpNm_TimerType TimerWaitBusSleep;      // 等待总线休眠定时器
    UdpNm_TimerType TimerRepeatMessage;     // 重复消息定时器
    UdpNm_TimerType TimerImmediate;         // 立即发送定时器
    uint8 ImmediateTxCounter;               // 立即发送计数器
    boolean NetworkRequested;               // 网络请求标志
    boolean CommunicationEnabled;           // 通信使能标志
    boolean RemoteSleepInd;                 // 远程休眠指示
    boolean LocalSleepInd;                  // 本地休眠指示
    boolean SleepReadyBit;                  // 休眠就绪位
    uint8 TxPduData[UDPNM_PDU_LENGTH];     // 发送 PDU 缓冲区 (8 bytes)
    uint8 RxPduData[UDPNM_PDU_LENGTH];     // 接收 PDU 缓冲区 (8 bytes)
    boolean RxIndPending;                  // 接收指示挂起
    boolean TxConfPending;                 // 发送确认挂起
    boolean Initialized;                   // 通道初始化标志
} UdpNm_InternalChannelType;
```

### 5.2 NM PDU 格式

与 CanNm 相同的 8 字节 PDU 格式:

| 字节 | 内容 | 说明 |
|------|------|------|
| Byte 0 | Source Node ID | 源节点标识符 |
| Byte 1 | CBV | 控制位向量 |
| Byte 2-7 | User Data | 用户数据（6 字节） |

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `UdpNm_Init(ConfigPtr)` | 0x00 | 初始化模块 | SWS_UdpNm_00001 |
| `UdpNm_DeInit()` | 0x01 | 反初始化 | SWS_UdpNm_00002 |
| `UdpNm_PassiveStartUp(nmChannelHandle)` | 0x02 | 被动启动 | SWS_UdpNm_00003 |
| `UdpNm_NetworkRequest(nmChannelHandle)` | 0x03 | 请求网络 | SWS_UdpNm_00004 |
| `UdpNm_NetworkRelease(nmChannelHandle)` | 0x04 | 释放网络 | SWS_UdpNm_00005 |
| `UdpNm_DisableCommunication(nmChannelHandle)` | 0x05 | 禁止通信 | SWS_UdpNm_00006 |
| `UdpNm_EnableCommunication(nmChannelHandle)` | 0x06 | 使能通信 | SWS_UdpNm_00007 |
| `UdpNm_GetUserData(nmChannelHandle, nmUserDataPtr)` | 0x07 | 获取用户数据 | SWS_UdpNm_00008 |
| `UdpNm_SetUserData(nmChannelHandle, nmUserDataPtr)` | 0x08 | 设置用户数据 | SWS_UdpNm_00009 |
| `UdpNm_GetPduData(nmChannelHandle, nmPduDataPtr)` | 0x09 | 获取 PDU 数据 | SWS_UdpNm_00010 |
| `UdpNm_GetState(nmChannelHandle, nmStatePtr, nmModePtr)` | 0x0A | 获取状态 | SWS_UdpNm_00011 |
| `UdpNm_GetVersionInfo(VersionInfoPtr)` | 0x0B | 获取版本信息 | SWS_UdpNm_00012 |
| `UdpNm_RequestBusSynchronization(nmChannelHandle)` | 0x0C | 请求总线同步 | SWS_UdpNm_00013 |
| `UdpNm_CheckRemoteSleepIndication(...)` | 0x0D | 检查远程休眠 | SWS_UdpNm_00014 |
| `UdpNm_SetSleepReadyBit(...)` | 0x0E | 设置休眠就绪位 | SWS_UdpNm_00015 |
| `UdpNm_Transmit(nmChannelHandle, PduInfoPtr)` | 0x11 | 发送 NM 消息 | SWS_UdpNm_00016 |
| `UdpNm_MainFunction()` | 0x60 | 主函数 | SWS_UdpNm_00017 |

### 6.2 回调函数

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `UdpNm_TxConfirmation(UdpNmTxPduId)` | 0x61 | 发送确认 | SWS_UdpNm_00100 |
| `UdpNm_RxIndication(UdpNmRxPduId, PduInfoPtr)` | 0x62 | 接收指示 | SWS_UdpNm_00101 |
| `UdpNm_RemoteSleepIndication(nmChannelHandle)` | 0x63 | 远程休眠指示 | SWS_UdpNm_00102 |
| `UdpNm_RemoteSleepCancellation(nmChannelHandle)` | 0x64 | 远程休眠取消 | SWS_UdpNm_00103 |

### 6.3 应用回调（弱符号）

| 回调函数 | 说明 |
|----------|------|
| `Appl_UdpNm_StateChangeNotification(channel, prev, curr)` | 状态变更通知 |
| `Appl_UdpNm_RemoteSleepIndication(channel)` | 远程休眠指示 |
| `Appl_UdpNm_RemoteSleepCancellation(channel)` | 远程休眠取消 |
| `Appl_UdpNm_NetworkStartIndication(channel)` | 网络启动指示 |
| `Appl_UdpNm_NetworkModeEntry(channel)` | 网络模式进入 |
| `Appl_UdpNm_BusSleepModeEntry(channel)` | 总线休眠模式进入 |
| `Appl_UdpNm_PrepareBusSleepModeEntry(channel)` | 准备总线休眠进入 |

### 6.4 错误码

| 错误码 | 宏名 | 说明 |
|--------|------|------|
| 0x00 | `UDPNM_E_NO_ERROR` | 无错误 |
| 0x01 | `UDPNM_E_UNINIT` | 未初始化 |
| 0x02 | `UDPNM_E_INVALID_CHANNEL` | 无效通道 |
| 0x03 | `UDPNM_E_INVALID_POINTER` | 无效指针 |
| 0x04 | `UDPNM_E_INIT_FAILED` | 初始化失败 |
| 0x05 | `UDPNM_E_NOT_OK` | 操作失败 |

---

## 7. 处理流程

### 7.1 主函数处理流程

`UdpNm_MainFunction()` 周期调用（默认 10ms）:

1. 检查模块已初始化
2. 遍历所有通道:
   - 递减所有定时器（TimerNM、TimerTimeout、TimerWaitBusSleep、TimerRepeatMessage、TimerImmediate）
   - 执行状态机处理:
     - **Bus Sleep**: 等待 NetworkRequest 或 RxInd
     - **Repeat Message**: 快速发送 NM PDU，超时后转 Normal/ReadySleep
     - **Normal Operation**: 正常周期发送，NetworkReleased 后转 ReadySleep
     - **Ready Sleep**: 继续发送，Timeout 后转 PrepareBusSleep
     - **Prepare Bus Sleep**: 等待 TWbs，超时后转 BusSleep

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `UDPNM_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `UDPNM_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `UDPNM_NUMBER_OF_CHANNELS` | 4 | 通道数量 |
| `UDPNM_PDU_LENGTH` | 8 | PDU 长度 |
| `UDPNM_MSG_CYCLE_TIME` | 256 | NM 消息周期 (ms) |
| `UDPNM_MSG_TIMEOUT_TIME` | 512 | NM 消息超时 (ms) |
| `UDPNM_TIMEOUT_TIME` | 1024 | NM 超时 (ms) |
| `UDPNM_REPEAT_MESSAGE_TIME` | 128 | 重复消息时间 (ms) |
| `UDPNM_WAIT_BUS_SLEEP_TIME` | 1920 | 等待总线休眠 (ms) |
| `UDPNM_IMMEDIATE_NM_CYCLE_TIME` | 20 | 立即发送周期 (ms) |
| `UDPNM_IMMEDIATE_NM_TRANSMISIONS` | 5 | 立即发送次数 |
| `UDPNM_MAIN_FUNCTION_PERIOD` | 10 | 主函数周期 (ms) |
| `UDPNM_USER_DATA_OFFSET` | 2 | 用户数据偏移 |
| `UDPNM_USER_DATA_LENGTH` | 6 | 用户数据长度 |

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 入口通过 `UdpNm_ValidateChannel()` 统一进行初始化状态、通道有效性、指针非空检查。

### 9.2 DEM 错误

模块不直接报告 DEM 事件。

### 9.3 安全机制

- **状态变更通知**: 通过 `UDPNM_STATE_CHANGE_NOTIFICATION` 宏通知应用层
- **模式进入通知**: Bus Sleep / Prepare Bus Sleep / Network Mode 进入时回调
- **通信使能控制**: `CommunicationEnabled` 标志控制状态机执行
- **MemMap 内存分段**: 使用 MemMap 实现内存分区保护

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 段名 | 类型 | 内容 |
|------|------|------|
| `UDPNM_START_SEC_VAR_INIT_UNSPECIFIED` | 初始化变量 | 模块初始化标志、配置指针、通道状态数组 |
| `UDPNM_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据 | `UdpNm_Config` |
| `UDPNM_START_SEC_CODE` | 代码段 | 所有函数实现 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| 每通道 RAM | ~48 bytes | InternalChannelType |
| 4 通道总 RAM | ~192 bytes | 通道状态数组 |
| PDU 缓冲区 | 16 bytes/ch | TxPduData[8] + RxPduData[8] |
| ROM（代码） | ~5 KB | 状态机 + API + 回调 |

---

## 11. 集成指南

1. **Nm 集成**: 实现 Nm 回调接口
2. **SoAd 集成**: 配置 TxPduId/RxPduId，通过 SoAd 发送/接收 NM PDU
3. **ComM 集成**: ComM 通过 `UdpNm_NetworkRequest()` / `UdpNm_NetworkRelease()` 管理网络
4. **应用回调**: 实现 `Appl_UdpNm_*` 系列回调函数（或使用弱符号默认实现）
5. **SchM 集成**: 配置 `UdpNm_MainFunction()` 调用周期（10ms）

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| 初始化测试 | NULL 指针检测、通道初始化验证 |
| 状态转换测试 | 所有 5 个状态之间的合法转换路径 |
| 定时器测试 | 各定时器到期触发正确的状态转换 |
| PDU 构建测试 | Node ID、CBV、User Data 正确设置 |
| 通道验证测试 | 无效通道号返回 E_NOT_OK |
| 通信控制测试 | DisableCommunication/EnableCommunication |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| UdpNm-SoAd 集成 | NM PDU 通过 SoAd/TcpIp 端到端传输 |
| 多通道测试 | 4 个通道独立状态管理 |
| 休眠序列 | Bus Sleep → Wakeup → Bus Sleep 完整循环 |

---

## 13. 实现说明 / TODO

- `UdpNm_TransmitMessage()` 中实际 SoAd 发送调用为桩实现（注释 "In real implementation, this would call SoAd APIs"）
- `UdpNm_Transmit()` API 已实现但发送路径依赖 SoAd 集成
- 部分 NM 回调（如 `Nm_RxIndication`）在 RxIndication 处理中未调用
- 远程休眠指示的超时检测逻辑尚未完整实现

---

## 14. 参考资料

- AUTOSAR SWS UDPNetworkManagement (AUTOSAR_SWS_UDPNetworkManagement.pdf)
- OSEK/VDX Network Management Specification
- 源码: `src/bsw/services/udpNm/`

## 需求追溯表

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_UdpNm | — | UDPNM 模块级需求归属 |
