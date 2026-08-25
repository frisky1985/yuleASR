# CanM (CanNm) Design Document

> **Module ID**: 0x35 (53)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_CANNetworkManagement  
> **Source Path**: `src/bsw/services/canm/`  
> **Reference Document**: `docs/modules/canm.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

CanM（CAN Network Management，源码中命名为 CanNm）是 AUTOSAR BSW 服务层的 CAN 网络管理模块。该模块实现了基于 OSEK NM 协议的 CAN 网络管理功能，负责管理 CAN 总线上的节点通信状态，包括网络请求/释放、节点检测、总线休眠/唤醒等核心功能。

CanNm 通过周期性发送 NM PDU（8 字节 CAN 帧）来维护网络拓扑感知，支持直接网络（Direct Network）和间接网络（Indirect Network）两种模式。模块管理多个 CAN 通道，每个通道独立运行 OSEK NM 状态机，实现 Bus Sleep、Repeat Message、Normal Operation、Ready Sleep、Prepare Bus Sleep 等状态转换。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS CANNetworkManagement | 4.4.0 | CAN 网络管理模块规范 |
| OSEK NM Specification | V2.5.3 | OSEK 网络管理协议基础 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Nm | 通用 NM 接口，状态变更通知 |
| 上层 | ComM | 通信管理器，请求网络通信 |
| 下层 | CanIf | CAN 接口，PDU 发送/接收 |
| 下层 | Det | 开发错误检测与报告 |
| 下层 | PduR | PDU 路由器 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│     ComM / Nm (上层调用者)           │
├─────────────────────────────────────┤
│     CanNm (CAN 网络管理)             │
├─────────────────────────────────────┤
│     CanIf / PduR (下层服务)          │
├─────────────────────────────────────┤
│     Can Driver (CAN 驱动)            │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **OSEK NM 状态机**: 实现 Bus Sleep / Repeat Message / Normal Operation / Ready Sleep / Prepare Bus Sleep 五个状态
- **定时器管理**: 管理 NM Timer (TTyp)、Timeout Timer (TMax/TError)、Wait Bus Sleep Timer (TWbs)、Repeat Message Timer、Immediate Transmission Timer
- **PDU 处理**: 构建和解析 OSEK NM PDU（Source Node ID + CBV + User Data）
- **CBV 处理**: 控制位向量（Control Bit Vector）的置位/清除/检测
- **通道管理**: 多通道独立管理，每个通道维护独立的状态和定时器

### 3.3 文件结构

```
src/bsw/services/canm/
├── include/
│   ├── CanNm.h         # 公共 API 与类型声明
│   └── CanNm_Cfg.h     # 预编译配置（自动生成）
└── src/
    └── CanNm.c          # 核心实现（状态机、定时器、PDU处理）
```

---

## 4. 状态机

CanNm 实现了 OSEK NM 五状态模型：

```
                    NetworkRequest / RxInd
          ┌──────────────────────────────────────┐
          │                                      │
          ▼                                      │
    ┌──────────┐    RxInd/Request    ┌─────────────────┐
    │ Bus Sleep ├───────────────────►│ Repeat Message   │
    └──────────┘                     └────────┬────────┘
          ▲                                   │
          │ TWbs 超时                         │ RepeatMsg Timer 超时
          │                                   │ (NetworkRequested=TRUE)
    ┌─────┴───────────┐                      ▼
    │Prepare Bus Sleep│         ┌────────────────────┐
    └─────┬───────────┘         │ Normal Operation    │
          ▲                     └────────┬───────────┘
          │ TError 超时                   │ NetworkReleased
          │                              ▼
          │                     ┌─────────────────┐
          └─────────────────────│  Ready Sleep     │
                                └─────────────────┘
```

**状态说明**:

| 状态 | 枚举值 | 说明 |
|------|--------|------|
| `CANNM_STATE_BUS_SLEEP` | 1 | 总线休眠，停止所有 NM 通信 |
| `CANNM_STATE_REPEAT_MESSAGE` | 5 | 重复消息状态，快速发送 NM PDU |
| `CANNM_STATE_NORMAL_OPERATION` | 4 | 正常运行，周期性发送 NM PDU |
| `CANNM_STATE_READY_SLEEP` | 3 | 就绪休眠，等待超时进入 Prepare Bus Sleep |
| `CANNM_STATE_PREPARE_BUS_SLEEP` | 2 | 准备总线休眠，启动 TWbs 定时器 |

**模式映射**:

| 模式 | 枚举值 | 对应状态 |
|------|--------|----------|
| `CANNM_MODE_BUS_SLEEP` | 1 | Bus Sleep |
| `CANNM_MODE_PREPARE_BUS_SLEEP` | 2 | Prepare Bus Sleep |
| `CANNM_MODE_NETWORK` | 4 | Repeat Message / Normal Operation / Ready Sleep |

---

## 5. 核心数据结构

### 5.1 通道配置 `CanNm_ChannelConfigType`

```c
typedef struct {
    uint8 NodeId;                       // 节点标识符（源地址）
    uint8 ClusterId;                    // 簇标识符
    boolean PassiveModeEnabled;         // 被动模式使能
    boolean RepeatMessageIndEnabled;    // 重复消息指示使能
    boolean NodeDetectionEnabled;       // 节点检测使能
    boolean NodeIdEnabled;             // PDU 中 Node ID 使能
    boolean BusSynchronizationEnabled;  // 总线同步使能
    boolean RemoteSleepIndEnabled;      // 远程休眠指示使能
    boolean UserDataEnabled;            // NM PDU 中用户数据使能
    uint8 UserDataOffset;               // 用户数据偏移
    uint8 UserDataLength;               // 用户数据长度
    const CanNm_TimingType *Timing;     // 定时配置指针
    const CanNm_PduType *Pdu;           // PDU 配置指针
} CanNm_ChannelConfigType;
```

### 5.2 通道运行时状态 `CanNm_ChannelType`

```c
typedef struct {
    CanNm_StateType State;              // 当前状态
    CanNm_ModeType Mode;                // 当前模式
    uint16 TimerNM;                     // NM 消息定时器 (TTyp)
    uint16 TimerTimeout;                // 超时定时器 (TMax/TError)
    uint16 TimerWaitBusSleep;           // 等待总线休眠定时器 (TWbs)
    uint16 TimerRepeatMessage;          // 重复消息定时器
    uint16 TimerImmediate;              // 立即发送定时器
    uint8 ImmediateTxCounter;           // 立即发送计数器
    boolean NetworkRequested;           // 网络请求标志
    boolean BusOff;                     // Bus Off 标志
    boolean RemoteSleepInd;             // 远程休眠指示
    boolean LocalSleepInd;              // 本地休眠指示
    uint8 TxPduData[CANNM_PDU_LENGTH]; // 发送 PDU 缓冲区
    uint8 RxPduData[CANNM_PDU_LENGTH]; // 接收 PDU 缓冲区
    boolean RxIndPending;              // 接收指示挂起
    boolean TxConfPending;             // 发送确认挂起
} CanNm_ChannelType;
```

### 5.3 OSEK NM PDU 格式

| 字节 | 内容 | 说明 |
|------|------|------|
| Byte 0 | Source Node ID | 源节点标识符 |
| Byte 1 | CBV (Control Bit Vector) | 控制位向量 |
| Byte 2-7 | User Data | 用户数据（可配置） |

**CBV 位定义**:

| 位 | 名称 | 说明 |
|----|------|------|
| 0x01 | Repeat Message | 重复消息请求 |
| 0x04 | Active Wakeup | 主动唤醒指示 |
| 0x08 | NM Coordinator Sleep | NM 协调器休眠位 |
| 0x10 | Partial Network | 部分网络信息 |

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `CanNm_Init(ConfigPtr)` | 0x00 | 初始化模块 |
| `CanNm_DeInit()` | 0x01 | 反初始化模块 |
| `CanNm_PassiveStartUp(nmChannelHandle)` | 0x02 | 被动启动 |
| `CanNm_NetworkRequest(nmChannelHandle)` | 0x03 | 请求网络 |
| `CanNm_NetworkRelease(nmChannelHandle)` | 0x04 | 释放网络 |
| `CanNm_DisableCommunication(nmChannelHandle)` | 0x05 | 禁止通信 |
| `CanNm_EnableCommunication(nmChannelHandle)` | 0x06 | 使能通信 |
| `CanNm_GetUserData(nmChannelHandle, nmUserDataPtr)` | 0x07 | 获取用户数据 |
| `CanNm_SetUserData(nmChannelHandle, nmUserDataPtr)` | 0x08 | 设置用户数据 |
| `CanNm_GetPduData(nmChannelHandle, nmPduDataPtr)` | 0x09 | 获取 PDU 数据 |
| `CanNm_GetState(nmChannelHandle, nmStatePtr, nmModePtr)` | 0x0A | 获取状态/模式 |
| `CanNm_GetVersionInfo(VersionInfoPtr)` | 0x0B | 获取版本信息 |
| `CanNm_RequestBusSynchronization(nmChannelHandle)` | 0x0C | 请求总线同步 |
| `CanNm_CheckRemoteSleepIndication(...)` | 0x0D | 检查远程休眠指示 |
| `CanNm_SetSleepReadyBit(...)` | 0x0E | 设置休眠就绪位 |
| `CanNm_MainFunction()` | 0x60 | 主函数（周期调用） |

### 6.2 回调函数

| 函数 | SID | 说明 |
|------|-----|------|
| `CanNm_TxConfirmation(CanNmTxPduId)` | 0x61 | 发送确认回调 |
| `CanNm_RxIndication(CanNmRxPduId, PduInfoPtr)` | 0x62 | 接收指示回调 |
| `CanNm_ControllerBusOff(Controller)` | 0x63 | 控制器 Bus Off 回调 |

### 6.3 服务 ID 与错误码

**DET 错误码**:

| 错误码 | 宏名 | 说明 |
|--------|------|------|
| 0x00 | `CANNM_E_NO_ERROR` | 无错误 |
| 0x01 | `CANNM_E_UNINIT` | 模块未初始化 |
| 0x02 | `CANNM_E_INVALID_CHANNEL` | 无效通道 |
| 0x03 | `CANNM_E_INVALID_POINTER` | 无效指针 |
| 0x04 | `CANNM_E_INIT_FAILED` | 初始化失败 |
| 0x05 | `CANNM_E_NOT_OK` | 操作失败 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 校验 `ConfigPtr` 非空
2. 保存全局配置指针 `CanNm_ConfigPtr`
3. 遍历所有通道，初始化 `CanNm_ChannelType`:
   - 状态设为 `CANNM_STATE_BUS_SLEEP`
   - 模式设为 `CANNM_MODE_BUS_SLEEP`
   - 所有定时器清零
   - 所有标志位清 FALSE
   - PDU 缓冲区清零

### 7.2 主函数处理流程

`CanNm_MainFunction()` 周期调用（默认 10ms），执行以下步骤:

1. 遍历所有通道
2. 递减各定时器（TimerNM、TimerTimeout、TimerWaitBusSleep、TimerRepeatMessage、TimerImmediate）
3. 定时器到期时触发状态转换
4. 在 Repeat Message / Normal Operation 状态下，TimerNM 到期时发送 NM PDU
5. 立即发送计数器 > 0 时，以快速周期发送 NM PDU

### 7.3 NM 消息发送流程

1. 设置 Source Node ID 到 PDU Byte 0
2. 构建 PduInfo 结构
3. 标记 TxConfPending = TRUE
4. 调用 `Nm_NetworkStartIndication()` 通知上层
5. 重置 NM Timer

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `CANNM_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `CANNM_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `CANNM_NUMBER_OF_CHANNELS` | 1 | 通道数量 |
| `CANNM_PDU_LENGTH` | 8 | PDU 长度（字节） |
| `CANNM_MSG_CYCLE_TIME` | 100 | NM 消息周期 (ms) |
| `CANNM_MSG_TIMEOUT_TIME` | 600 | NM 消息超时 (ms) |
| `CANNM_REPEAT_MESSAGE_TIME` | 1500 | 重复消息时间 (ms) |
| `CANNM_WAIT_BUS_SLEEP_TIME` | 2000 | 等待总线休眠时间 (ms) |
| `CANNM_MAIN_FUNCTION_PERIOD` | 10 | 主函数周期 (ms) |
| `CANNM_NODE_ID` | 1 | 节点 ID |
| `CANNM_MAX_NODES` | 8 | 最大节点数 |

### 8.2 链接时配置

通过 `CanNm_Config` 全局常量提供链接时配置，包含通道配置数组和通道数量。

### 8.3 构建后配置

不支持构建后配置。所有配置在编译时或链接时确定。

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 入口均进行参数校验:
- 模块初始化状态检查（`CANNM_E_UNINIT`）
- 通道号有效性检查（`CANNM_E_INVALID_CHANNEL`）
- 指针参数非空检查（`CANNM_E_INVALID_POINTER`）

### 9.2 DEM 错误

模块未直接报告 DEM 事件。Bus Off 等硬件错误通过 `CanNm_ControllerBusOff()` 回调传递至 CanSM 处理。

### 9.3 安全机制

- **状态变更通知**: 通过 `Nm_StateChangeNotification()` 通知上层状态变化
- **模式进入通知**: Bus Sleep / Prepare Bus Sleep / Network Mode 进入时分别回调
- **远程休眠取消**: 收到 NM 消息时自动取消远程休眠指示
- **MemMap 内存分段**: 使用 `#include "MemMap.h"` 实现内存分区保护

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 段名 | 类型 | 内容 |
|------|------|------|
| `CANNM_START_SEC_VAR_INIT_BOOLEAN` | 初始化变量 | `CanNm_Initialized` |
| `CANNM_START_SEC_VAR_NOINIT_UNSPECIFIED` | 未初始化变量 | `CanNm_Channels[]`, `CanNm_ConfigPtr`, `CanNm_TxPduInfo[]` |
| `CANNM_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据 | `CanNm_Config` |
| `CANNM_START_SEC_CODE` | 代码段 | 所有函数实现 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| 每通道 RAM | ~64 bytes | CanNm_ChannelType 结构体 |
| PDU 缓冲区 | 16 bytes/ch | TxPduData[8] + RxPduData[8] |
| 定时器 | 10 bytes/ch | 5 个 uint16 定时器 |
| ROM（代码） | ~4 KB | 状态机 + API 实现 |

---

## 11. 集成指南

1. **EcuM 集成**: 在 `EcuM_StartUpTwo()` 中调用 `CanNm_Init()`
2. **ComM 集成**: ComM 通过 `CanNm_NetworkRequest()` / `CanNm_NetworkRelease()` 管理网络状态
3. **Nm 集成**: 实现 Nm 回调接口（`Nm_StateChangeNotification`、`Nm_BusSleepModeEntry` 等）
4. **CanIf 集成**: 配置 NM PDU 的 Tx/Rx PDU ID，注册回调
5. **SchM 集成**: 在 SchM 调度中配置 `CanNm_MainFunction()` 的调用周期（10ms）
6. **Det 集成**: 配置 `CANNM_DEV_ERROR_DETECT = STD_ON` 启用开发时错误检测

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| 初始化测试 | 验证 NULL 指针检测、通道初始化状态 |
| 状态转换测试 | 验证所有 5 个状态之间的合法转换路径 |
| 定时器测试 | 验证各定时器到期触发正确的状态转换 |
| PDU 构建测试 | 验证 Source Node ID、CBV 位的正确设置 |
| CBV 解析测试 | 验证接收 PDU 中 Repeat Message 位的检测 |
| 错误注入测试 | 验证 DET 错误报告的正确性 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| ComM-CanNm 集成 | 验证 NetworkRequest/Release 的完整流程 |
| NM 休眠序列 | 验证 Bus Sleep → Wakeup → Bus Sleep 完整循环 |
| 多节点 NM | 验证 Repeat Message 请求的跨节点传播 |
| Bus Off 恢复 | 验证 Bus Off 后的状态恢复流程 |

---

## 13. 实现说明 / TODO

- 当前实现中 `CanNm_Init()` 的通道初始化循环内 `cfgPtr` 赋值语句为空操作（`cfgPtr ;`），需要补充实际配置处理
- 部分 NM 回调（如 `Nm_BusSleepModeEntry`）通过条件编译宏控制，需要确保配置一致性
- `CanNm_ControllerBusOff()` 函数原型已声明但未在 CanNm.c 中实现
- 立即发送功能（Immediate Transmission）通过 `CANNM_IMMEDIATE_TRANSMISSION_ENABLED` 宏控制

---

## 14. 参考资料

- AUTOSAR SWS CANNetworkManagement (AUTOSAR_SWS_CANNetworkManagement.pdf)
- OSEK/VDX Network Management Specification (OSEK NM V2.5.3)
- AUTOSAR Template Specification - ECU Configuration
- 源码: `src/bsw/services/canm/`
