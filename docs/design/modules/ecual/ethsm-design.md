# EthSM Design Document

> **Module ID**: 0x43  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_EthernetStateManager  
> **Source Path**: `src/bsw/ecual/ethSm/`  
> **Reference Document**: `docs/modules/ethsm.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

EthSM（Ethernet State Manager）是 AUTOSAR ECU 抽象层（ECUAL）中负责管理以太网通信状态的核心模块。该模块实现了一个完整的状态机，用于控制以太网网络在 NO_COM、WAIT_TRCVLINK、WAIT_ONLINE、ONHOLD 和 COM_READY 等状态之间的转换。

EthSM 的主要职责包括：
- 管理以太网网络的通信状态机
- 协调收发器链路建立与 TcpIp 协议栈上线的时序
- 与 ComM（Communication Manager）交互，上报网络模式变化
- 提供超时监控和重试机制
- 支持多网络（最多 ETHSM_MAX_NETWORKS = 2）的独立管理
- 支持唤醒功能（Wake-up Support）

本实现基于 YuleTech AutoSAR BSW 平台，目标硬件为 NXP i.MX8M Mini。

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS EthernetStateManager | 4.4.0 | 以太网状态管理器规范 |
| AUTOSAR CP R21-11 | - | 基础软件规范 |
| IEEE 802.3 | - | 以太网物理层标准 |

### 2.2 模块依赖

| 模块 | 依赖方向 | 说明 |
|------|----------|------|
| EthIf | 调用 | 以太网接口层，用于控制器模式控制和链路状态查询 |
| ComM | 双向 | 通信管理器，接收通信模式请求并上报模式变化 |
| Det | 调用 | 默认错误追踪（条件编译 ETHSM_DEV_ERROR_DETECT） |
| Std_Types | 包含 | AUTOSAR 标准类型定义 |
| ComStack_Types | 包含 | 通信栈类型定义 |

## 3. 架构设计

### 3.1 分层位置

```
+-----------------------------------+
|       BSW Services (ComM)         |
+-----------------------------------+
|       ECUAL Layer (EthSM)         |  <-- 本模块
+-----------------------------------+
|       EthIf / EthTrcv             |
+-----------------------------------+
|       MCAL (Eth, Port, Mcu)       |
+-----------------------------------+
|       Hardware (NXP i.MX8M Mini)  |
+-----------------------------------+
```

EthSM 位于 ECU 抽象层，向上为 ComM 提供以太网通信状态管理，向下通过 EthIf 控制以太网控制器和收发器。

### 3.2 内部组件

- **状态机引擎**：每个网络独立维护状态机，处理 UNINIT → NO_COM → WAIT_TRCVLINK → WAIT_ONLINE → COM_READY → ONHOLD 的完整状态转换
- **超时管理器**：管理 WAIT_TRCVLINK（100ms）和 WAIT_ONLINE（5000ms）超时
- **重试机制**：最大重试次数 ETHSM_MAX_RETRIES = 3，重试延迟 ETHSM_RETRY_DELAY_MS = 100ms
- **ComM 通知器**：将内部状态映射为 ComM 模式（NO_COM → COMM_NO_COMMUNICATION, WAIT_* → COMM_SILENT_COMMUNICATION, COM_READY → COMM_FULL_COMMUNICATION）

### 3.3 文件结构

```
src/bsw/ecual/ethSm/
├── include/
│   ├── EthSM.h           # 公共 API 头文件
│   └── EthSM_Cfg.h       # 预编译配置头文件
└── src/
    ├── EthSM.c            # 模块实现
    └── EthSM_Lcfg.c       # 链接时配置表
```

## 4. 状态机

EthSM 实现了一个 6 状态的网络状态机：

```
                    EthSM_Init()
    UNINIT ──────────────────────> NO_COM
                                     │
                    FullComm 请求     │  NoComm 请求
                                     │  / 超时
    NO_COM <─────────────────────────┤
        │                            │
        │ FullComm 请求              │
        v                            │
    WAIT_TRCVLINK ──── Link Up ────> WAIT_ONLINE
        │                               │
        │ 超时                          │ TcpIp Online
        │                               v
        └───────────────────────> COM_READY
                                    │
                    TcpIp OnHold    │  NoComm 请求
                                    v
                                 ONHOLD
                                    │
                    TcpIp Online    │
                    ──────────────> COM_READY
```

**状态说明：**

| 状态 | 枚举值 | ComM 映射 | 说明 |
|------|--------|-----------|------|
| ETHSM_STATE_UNINIT | 0 | - | 模块未初始化 |
| ETHSM_STATE_NO_COM | 1 | COMM_NO_COMMUNICATION | 无通信请求 |
| ETHSM_STATE_WAIT_TRCVLINK | 2 | COMM_SILENT_COMMUNICATION | 等待收发器链路建立 |
| ETHSM_STATE_WAIT_ONLINE | 3 | COMM_SILENT_COMMUNICATION | 等待 TcpIp 上线 |
| ETHSM_STATE_ONHOLD | 4 | COMM_SILENT_COMMUNICATION | 通信暂停 |
| ETHSM_STATE_COM_READY | 5 | COMM_FULL_COMMUNICATION | 通信就绪 |

## 5. 核心数据结构

### 5.1 运行时状态结构

```c
typedef struct {
    EthSM_StateType currentState;           /* 当前状态机状态 */
    ComM_ModeType requestedComMode;         /* 请求的通信模式 */
    ComM_ModeType currentComMode;           /* 当前通信模式 */
    TcpIp_StateType tcpIpState;             /* TcpIp 状态 */
    uint16 timeoutCounter;                  /* 超时计数器（ms） */
    uint8 retryCounter;                     /* 重试计数器 */
    boolean linkStateUp;                    /* 收发器链路状态 */
    boolean initDone;                       /* 网络初始化标志 */
} EthSM_NetworkStateType;
```

### 5.2 配置结构

```c
/* TcpIp 控制器映射 */
typedef struct {
    EthSM_NetworkHandleType networkHandle;
    uint8  tcpIpCtrlIdx;
    boolean dhcpEnabled;
    uint32 staticIpAddress;
    uint32 subnetMask;
    uint32 gatewayAddress;
} EthSM_TcpIpMappingType;

/* 网络配置 */
typedef struct {
    EthSM_NetworkHandleType networkHandle;
    uint8 ctrlIdx;
    uint8 trcvIdx;
    uint8 tcpIpCtrlIdx;
    ComM_ChannelHandleType comMChannel;
    uint16 timeoutWaitTrcvLink;
    uint16 timeoutWaitOnline;
    boolean wakeUpSupport;
    uint8 wakeUpSource;
    boolean wakeUpByBus;
} EthSM_NetworkConfigType;
```

### 5.3 链接时配置表

`EthSM_Lcfg.c` 定义了以下配置表：
- `EthSM_NetworkConfig[ETHSM_MAX_NETWORKS]` — 网络配置表
- `EthSM_CtrlConfig[ETHSM_MAX_NETWORKS]` — 控制器配置表（含 MAC 地址、MTU）
- `EthSM_TrcvConfig[ETHSM_MAX_NETWORKS]` — 收发器配置表（自协商、速率、双工）
- `EthSM_TcpIpMapping[ETHSM_MAX_NETWORKS]` — TcpIp 映射表（静态 IP 配置）

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `EthSM_Init(ConfigPtr)` | 0x01 | 初始化模块，所有网络进入 NO_COM 状态 | SWS_EthSM_00001 |
| `EthSM_DeInit()` | 0x02 | 去初始化，关闭所有以太网控制器 | SWS_EthSM_00002 |
| `EthSM_Start()` | 0x03 | 启动以太网状态管理 | SWS_EthSM_00003 |
| `EthSM_Stop()` | 0x04 | 停止以太网状态管理 | SWS_EthSM_00004 |
| `EthSM_GetState()` | 0x05 | 获取当前状态 | SWS_EthSM_00005 |
| `EthSM_SetState(State)` | 0x06 | 设置目标状态 | SWS_EthSM_00006 |
| `EthSM_MainFunction()` | 0x07 | 周期处理函数（10ms 周期） | SWS_EthSM_00007 |
| `EthSM_GetVersionInfo(*VersionInfo)` | 0x08 | 获取版本信息（条件编译） | SWS_EthSM_00008 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| `ComM_BusSM_ModeIndication(channelHandle, mode)` | 通知 ComM 模式变化（条件编译 ETHSM_STATE_CHANGE_CALLBACK） |

### 6.3 服务 ID 与错误码

**DET 错误码：**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| ETHSM_E_NOT_INITIALIZED | 0x01 | 模块未初始化 |
| ETHSM_E_INVALID_NETWORK_HANDLE | 0x02 | 无效网络句柄 |
| ETHSM_E_INVALID_POINTER | 0x03 | 空指针 |
| ETHSM_E_INVALID_PARAMETER | 0x04 | 无效参数 |
| ETHSM_E_ALREADY_INITIALIZED | 0x05 | 重复初始化 |
| ETHSM_E_NOT_SUPPORTED | 0x06 | 不支持的操作 |
| ETHSM_E_TCPIP_MODE_FAILED | 0x07 | TcpIp 模式切换失败 |
| ETHSM_E_TRANSCEIVER_ERROR | 0x08 | 收发器错误 |

## 7. 处理流程

### 7.1 初始化流程

1. 检查是否已初始化（ETHSM_INITIALIZED = 0xA5）
2. 保存配置指针
3. 遍历所有网络，初始化状态为 NO_COM
4. 设置标记为已初始化

### 7.2 MainFunction 周期处理

每 10ms 调用一次，遍历所有网络：
1. 根据当前状态调用对应的状态处理函数
2. 状态处理函数检查请求模式和外部事件
3. 执行超时计数和状态转换
4. 转换时通知 ComM

### 7.3 通信建立流程

```
RequestComMode(FULL_COMM) → NO_COM 处理
  → EthIf_SetControllerMode(ACTIVE) → WAIT_TRCVLINK
    → 轮询链路状态 → Link Up → WAIT_ONLINE
      → 等待 TcpIpModeIndication(ONLINE) → COM_READY
        → ComM 通知 FULL_COMMUNICATION
```

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| ETHSM_VERSION_INFO_API | STD_ON | 版本信息 API 使能 |
| ETHSM_DEV_ERROR_DETECT | STD_ON | DET 错误检测使能 |
| ETHSM_MAX_NETWORKS | 2 | 最大网络数 |
| ETHSM_WAKEUP_SUPPORT | STD_ON | 唤醒支持 |
| ETHSM_MAIN_FUNCTION_CYCLE_MS | 10 | 主函数周期（ms） |
| ETHSM_TIMEOUT_WAIT_TRCVLINK | 100 | 链路等待超时（ms） |
| ETHSM_TIMEOUT_WAIT_ONLINE | 5000 | TcpIp 上线超时（ms） |
| ETHSM_MAX_RETRIES | 3 | 最大重试次数 |
| ETHSM_LINK_DEBOUNCE_TIME | 20 | 链路去抖时间（ms） |

### 8.2 链接时配置

链接时配置在 `EthSM_Lcfg.c` 中定义，包括：
- 网络到控制器/收发器/TcpIp 的映射关系
- 静态 IP 地址配置（Network 0: 192.168.1.100, Network 1: 192.168.2.100）
- MAC 地址配置
- 超时参数

### 8.3 构建后配置

本实现不支持构建后配置变体（ETHSM_CONFIG_VARIANT = PRECOMPILE）。

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 均进行参数验证：
- 初始化状态检查（ETHSM_E_NOT_INITIALIZED / ETHSM_E_ALREADY_INITIALIZED）
- 网络句柄有效性检查（ETHSM_E_INVALID_NETWORK_HANDLE）
- 指针有效性检查（ETHSM_E_INVALID_POINTER）
- 参数范围检查（ETHSM_E_INVALID_PARAMETER）

### 9.2 DEM 错误

本实现未集成 DEM 事件上报。

### 9.3 安全机制

- **初始化魔数**：使用 0xA5 标记初始化状态，防止未初始化访问
- **超时保护**：WAIT_TRCVLINK 和 WAIT_ONLINE 状态均有超时保护
- **重试限制**：控制器启动失败最多重试 3 次
- **TcpIp 回调静默失败**：回调函数在无效参数时静默返回，不触发 DET

## 10. 内存与性能

### 10.1 MemMap 分区

本实现未使用 MemMap 分区宏。所有运行时数据位于默认 BSS 段。

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| EthSM_NetworkState[2] | ~32 bytes | 每网络 16 bytes × 2 |
| EthSM_NetworkConfig[2] | ~24 bytes | 只读配置 |
| EthSM_CtrlConfig[2] | ~20 bytes | 只读配置 |
| EthSM_TrcvConfig[2] | ~12 bytes | 只读配置 |
| EthSM_TcpIpMapping[2] | ~28 bytes | 只读配置 |
| **总计 RAM** | **~32 bytes** | 运行时变量 |
| **总计 ROM** | **~84 bytes** | 配置常量 |

## 11. 集成指南

### 集成步骤

1. 在 `EthSM_Cfg.h` 中配置网络数量和超时参数
2. 在 `EthSM_Lcfg.c` 中配置网络映射表（控制器、收发器、TcpIp、ComM 通道）
3. 将 `EthSM_Init(&EthSM_Config)` 加入初始化序列
4. 将 `EthSM_MainFunction()` 加入 10ms 周期任务
5. 配置 ComM 通道与 EthSM 网络的对应关系
6. 确保 EthIf 模块已正确初始化

### 注意事项

- EthSM 依赖 EthIf 的 `EthIf_SetControllerMode` 和 `EthIf_GetTransceiverLinkState` 接口
- TcpIp 模块需注册 `EthSM_TcpIpModeIndication` 回调
- ComM 需通过 `ComM_BusSM_ModeIndication` 接收状态通知

## 12. 测试策略

### 12.1 单元测试

| 测试场景 | 预期结果 |
|----------|----------|
| Init 后状态检查 | 所有网络处于 NO_COM 状态 |
| 重复 Init | 触发 DET 错误 ETHSM_E_ALREADY_INITIALIZED |
| DeInit 后调用 API | 触发 DET 错误 ETHSM_E_NOT_INITIALIZED |
| RequestComMode(FULL) → 链路建立 → TcpIp Online | 状态转换至 COM_READY |
| WAIT_TRCVLINK 超时 | 返回 NO_COM 状态 |
| WAIT_ONLINE 超时 | 返回 NO_COM 状态 |
| COM_READY 状态下链路断开 | 返回 WAIT_TRCVLINK 状态 |
| COM_READY 状态下 TcpIp OnHold | 转换至 ONHOLD 状态 |
| ONHOLD 状态下 TcpIp Online | 恢复至 COM_READY 状态 |
| 无效网络句柄 | 返回 E_NOT_OK，触发 DET |
| 空指针参数 | 返回 E_NOT_OK，触发 DET |

### 12.2 集成测试

| 测试场景 | 验证点 |
|----------|--------|
| EthSM + ComM 联合测试 | 模式指示正确传递 |
| EthSM + EthIf + EthTrcv 联合测试 | 完整通信建立流程 |
| EthSM + TcpIp 联合测试 | TcpIp 状态回调正确触发状态转换 |
| 多网络并发测试 | 两个网络独立运行状态机 |
| 唤醒功能测试 | 从睡眠状态恢复通信 |

## 13. 实现说明 / TODO

### 当前实现特点

- 支持 2 个独立以太网网络的完整状态机管理
- 实现了重试机制和超时保护
- 通过条件编译支持功能裁剪（版本信息、DET、唤醒、回调）
- 使用 `EthSM_GetInternalState` 提供调试接口

### 待实现项

- [ ] DEM 事件上报集成
- [ ] MemMap 内存分区支持
- [ ] 构建后配置（Post-Build）变体支持
- [ ] 链路去抖机制的实际实现（当前配置了 ETHSM_LINK_DEBOUNCE_TIME 但未使用）
- [ ] 多网络独立超时参数的运行时可配置性

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| AUTOSAR_SWS_EthernetStateManager.pdf | AUTOSAR EthSM 规范 |
| AUTOSAR_SWS_CommunicationManager.pdf | ComM 接口规范 |
| AUTOSAR_SWS_EthernetInterface.pdf | EthIf 接口规范 |
| EthSM.h | 模块公共接口定义 |
| EthSM_Cfg.h | 预编译配置定义 |
| EthSM.c | 模块实现源码 |
| EthSM_Lcfg.c | 链接时配置表 |
