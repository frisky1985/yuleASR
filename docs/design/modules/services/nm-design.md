# Nm Design Document

> **Module ID**: 0x30 (48)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_NetworkManagement  
> **Source Path**: `src/bsw/services/nm/`  
> **Reference Document**: `docs/modules/nm.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Nm（Network Management Interface）是 AUTOSAR BSW 服务层的通用网络管理接口模块。该模块为上层（如 ComM）提供统一的网络管理抽象，屏蔽底层总线特定的 NM 实现（CanNm、UdpNm、FrNm、LinNm）的差异。

Nm 模块本身不实现具体的 NM 协议逻辑，而是作为接口层将上层请求路由到对应的 BusNm 模块（CanNm/UdpNm 等），同时提供跨总线的状态查询、用户数据管理和协调器功能。模块支持最多 8 个通道，涵盖 CAN、LIN、FlexRay 和以太网四种总线类型。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS NetworkManagement | 4.0.3 | 通用网络管理接口规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | ComM | 通信管理器 |
| 下层 | CanNm | CAN 网络管理实现 |
| 下层 | UdpNm | UDP 网络管理实现 |
| 下层 | FrNm | FlexRay 网络管理（可选） |
| 下层 | Det | 开发错误检测 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│     ComM (通信管理器)                │
├─────────────────────────────────────┤
│     Nm (通用 NM 接口)                │
├─────────────────────────────────────┤
│  CanNm │ UdpNm │ FrNm │ LinNm      │
├─────────────────────────────────────┤
│  CanIf │ SoAd  │ FrIf  │ LinIf      │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **通道状态管理**: 维护每个通道的状态、模式、网络请求标志、通信使能标志
- **用户数据管理**: 每通道 8 字节用户数据缓冲区
- **状态变更通知**: 向应用层通知 NM 状态变化
- **远程休眠管理**: 跟踪和通知远程休眠指示
- **BusNm 路由**: 将请求路由到对应的总线 NM 实现

### 3.3 文件结构

```
src/bsw/services/nm/
├── include/
│   ├── Nm.h          # 公共 API 与类型声明
│   └── Nm_Cfg.h      # 预编译配置（自动生成）
└── src/
    ├── Nm.c           # 核心实现
    └── Nm_Lcfg.c      # 链接时配置
```

---

## 4. 状态机

Nm 定义了与 BusNm 一致的状态和模式枚举：

**状态定义**:

| 状态 | 值 | 说明 |
|------|----|------|
| `NM_STATE_UNINIT` | 0x00 | 未初始化 |
| `NM_STATE_BUS_SLEEP` | 0x01 | 总线休眠 |
| `NM_STATE_PREPARE_BUS_SLEEP` | 0x02 | 准备总线休眠 |
| `NM_STATE_READY_SLEEP` | 0x03 | 就绪休眠 |
| `NM_STATE_NORMAL_OPERATION` | 0x04 | 正常运行 |
| `NM_STATE_REPEAT_MESSAGE` | 0x05 | 重复消息 |
| `NM_STATE_SYNCHRONIZE` | 0x06 | 同步 |

**模式定义**:

| 模式 | 值 | 说明 |
|------|----|------|
| `NM_MODE_BUS_SLEEP` | 0x00 | 总线休眠模式 |
| `NM_MODE_PREPARE_BUS_SLEEP` | 0x01 | 准备总线休眠 |
| `NM_MODE_SYNCHRONIZE` | 0x02 | 同步模式 |
| `NM_MODE_NETWORK` | 0x03 | 网络模式 |

**总线类型**:

| 类型 | 值 | 说明 |
|------|----|------|
| `NM_BUSNM_CANNM` | 0x00 | CAN 网络管理 |
| `NM_BUSNM_FRNM` | 0x01 | FlexRay 网络管理 |
| `NM_BUSNM_UDPNM` | 0x02 | UDP 网络管理 |
| `NM_BUSNM_LINNM` | 0x03 | LIN 网络管理 |

---

## 5. 核心数据结构

### 5.1 内部状态

```c
// 每通道状态数组
static Nm_StateType Nm_ChannelState[NM_MAX_CHANNELS];          // 通道状态
static Nm_ModeType Nm_ChannelMode[NM_MAX_CHANNELS];            // 通道模式
static boolean Nm_ChannelInitialized[NM_MAX_CHANNELS];         // 初始化标志
static boolean Nm_NetworkRequested[NM_MAX_CHANNELS];           // 网络请求标志
static boolean Nm_CommunicationEnabled[NM_MAX_CHANNELS];       // 通信使能标志
static boolean Nm_RemoteSleepInd[NM_MAX_CHANNELS];             // 远程休眠指示
static uint8 Nm_UserData[NM_MAX_CHANNELS][8];                  // 用户数据
static boolean Nm_ModuleInitialized = FALSE;                    // 模块初始化标志
```

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `Nm_Init(ConfigPtr)` | 0x01 | 初始化模块 | SWS_Nm_00001 |
| `Nm_DeInit()` | 0x02 | 反初始化 | SWS_Nm_00002 |
| `Nm_GetVersionInfo(VersionInfo)` | 0x03 | 获取版本信息 | SWS_Nm_00003 |
| `Nm_PassiveStartUp(nmChannelHandle)` | 0x04 | 被动启动 | SWS_Nm_00004 |
| `Nm_NetworkRequest(nmChannelHandle)` | 0x05 | 请求网络 | SWS_Nm_00005 |
| `Nm_NetworkRelease(nmChannelHandle)` | 0x06 | 释放网络 | SWS_Nm_00006 |
| `Nm_DisableCommunication(nmChannelHandle)` | 0x07 | 禁止通信 | SWS_Nm_00007 |
| `Nm_EnableCommunication(nmChannelHandle)` | 0x08 | 使能通信 | SWS_Nm_00008 |
| `Nm_GetState(nmChannelHandle, nmStatePtr)` | 0x09 | 获取状态 | SWS_Nm_00009 |
| `Nm_GetLocalNodeIdentifier(nmChannelHandle, nmNodeIdPtr)` | 0x0A | 获取节点 ID | SWS_Nm_00011 |
| `Nm_GetPduData(nmChannelHandle, nmPduDataPtr)` | 0x0B | 获取 PDU 数据 | SWS_Nm_00010 |
| `Nm_GetUserData(nmChannelHandle, nmUserDataPtr)` | 0x0C | 获取用户数据 | SWS_Nm_00012 |
| `Nm_RepeatMessageRequest(nmChannelHandle)` | 0x0D | 重复消息请求 | SWS_Nm_00013 |
| `Nm_MainFunction()` | 0x60 | 主函数 | SWS_Nm_00015 |

### 6.2 回调函数（由 BusNm 调用）

| 函数 | 说明 |
|------|------|
| `Nm_BusSleepModeEntry(nmNetworkHandle)` | 总线休眠模式进入 |
| `Nm_PrepareBusSleepModeEntry(nmNetworkHandle)` | 准备总线休眠进入 |
| `Nm_NetworkModeEntry(nmNetworkHandle)` | 网络模式进入 |
| `Nm_NetworkStartIndication(nmNetworkHandle)` | 网络启动指示 |
| `Nm_RxIndication(nmNetworkHandle, nmPduDataPtr)` | 接收指示 |
| `Nm_StateChangeNotification(nmNetworkHandle, prev, curr)` | 状态变更通知 |
| `Nm_RemoteSleepIndication(nmNetworkHandle)` | 远程休眠指示 |
| `Nm_RemoteSleepCancellation(nmNetworkHandle)` | 远程休眠取消 |

### 6.3 应用回调（弱符号）

| 回调函数 | 说明 |
|----------|------|
| `Appl_Nm_StateChangeNotification(ch, prev, curr)` | 状态变更通知 |
| `Appl_Nm_RemoteSleepIndication(ch)` | 远程休眠指示 |
| `Appl_Nm_RemoteSleepCancellation(ch)` | 远程休眠取消 |

### 6.4 错误码

| 错误码 | 宏名 | 说明 |
|--------|------|------|
| 0x01 | `NM_E_UNINIT` | 未初始化 |
| 0x02 | `NM_E_INVALID_CHANNEL` | 无效通道 |
| 0x03 | `NM_E_INVALID_POINTER` | 无效指针 |
| 0x04 | `NM_E_NOT_OK` | 操作失败 |

---

## 7. 处理流程

### 7.1 主函数处理流程

`Nm_MainFunction()` 周期调用（默认 10ms）:

1. 检查模块已初始化
2. 遍历所有通道（跳过通信禁止的通道）:
   - **Repeat Message 状态**: 如果 NetworkRequested 为 TRUE，转换到 Normal Operation
   - **Ready Sleep 状态**: 如果 NetworkReleased 且 RemoteSleepInd 为 TRUE，转换到 Prepare Bus Sleep

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `NM_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `NM_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `NM_MAX_CHANNELS` | 8 | 最大通道数 |
| `NM_NODE_COUNT` | 4 | 节点数量 |
| `NM_CLUSTER_COUNT` | 2 | 簇数量 |
| `NM_NODE_ID` | 1 | 本节点 ID |
| `NM_TIMEOUT_TIME` | 1000 | 超时时间 (ms) |
| `NM_MAIN_FUNCTION_PERIOD` | 10 | 主函数周期 (ms) |
| `NM_STATE_CHANGE_IND_ENABLED` | STD_ON | 状态变更指示使能 |
| `NM_REMOTE_SLEEP_IND_ENABLED` | STD_ON | 远程休眠指示使能 |
| `NM_COORDINATOR_SUPPORT_ENABLED` | STD_ON | 协调器支持 |

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 入口进行模块初始化状态、通道有效性、指针非空检查。

### 9.2 DEM 错误

模块不直接报告 DEM 事件。

### 9.3 安全机制

- **编译时版本检查**: `#error` 宏确保 AR 版本一致性
- **状态变更通知**: 通过 `NM_STATE_CHANGE_NOTIFICATION` 宏通知应用层
- **远程休眠管理**: 跟踪远程节点休眠状态

---

## 10. 内存与性能

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| 每通道 RAM | ~18 bytes | 状态+模式+标志+用户数据 |
| 8 通道总 RAM | ~144 bytes | 通道状态数组 |
| ROM（代码） | ~3 KB | API 实现 + 回调 |

---

## 11. 集成指南

1. **ComM 集成**: ComM 通过 Nm 接口请求/释放网络
2. **BusNm 集成**: CanNm/UdpNm 等调用 Nm 回调接口通知状态变化
3. **应用回调**: 实现 `Appl_Nm_*` 系列回调函数
4. **EcuM 集成**: 在启动阶段调用 `Nm_Init()`
5. **SchM 集成**: 配置 `Nm_MainFunction()` 调用周期

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| 初始化测试 | NULL 指针检测、通道初始化验证 |
| 网络请求/释放 | NetworkRequest → Repeat Message → Normal Operation |
| 状态查询 | GetState/GetMode 返回值正确性 |
| 用户数据 | SetUserData/GetUserData 数据一致性 |
| 远程休眠 | RemoteSleepIndication/Cancellation |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| Nm-CanNm 集成 | CAN 网络的端到端 NM 流程 |
| Nm-UdpNm 集成 | UDP 网络的端到端 NM 流程 |
| 多总线集成 | CAN + Ethernet 混合网络管理 |

---

## 13. 实现说明 / TODO

- `Nm_GetMode()` 的 DET 报告 SID 使用硬编码 `0x90` 而非宏定义
- `Nm_SetUserData()` 的 DET 报告 SID 使用硬编码 `0x91`
- `Nm_GetPduData()`、`Nm_RepeatMessageRequest()` 函数原型已声明但未在 Nm.c 中实现
- `Nm_GetCoordinatorSleepReady()` 函数原型已声明但未实现
- 主函数中的状态机处理较为简化，缺少定时器管理
- BusNm 路由逻辑（根据通道类型分发到 CanNm/UdpNm）尚未实现

---

## 14. 参考资料

- AUTOSAR SWS NetworkManagement (AUTOSAR_SWS_NetworkManagement.pdf)
- OSEK/VDX Network Management Specification
- 源码: `src/bsw/services/nm/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Nm | — | NM 模块级需求归属 |
| SWS_Nm_00014 | `Nm_CheckRemoteSleepIndication` | 测试 test_Nm_CheckRemoteSleepIndication 覆盖: Nm_CheckRemoteSleepIndication 场景 |
| SWS_Nm_00100 | `Nm_Callback_NetworkModeEntry` | 测试 test_Nm_Callback_NetworkModeEntry 覆盖: Nm_Callback_NetworkModeEntry 场景 |
