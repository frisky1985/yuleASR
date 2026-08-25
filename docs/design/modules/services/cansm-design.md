# CanSM Design Document

> **Module ID**: 0x36 (54)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_CANStateManagement  
> **Source Path**: `src/bsw/services/cansm/`  
> **Reference Document**: `docs/modules/cansm.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

CanSM（CAN State Management）是 AUTOSAR BSW 服务层的 CAN 状态管理模块。该模块负责管理 CAN 网络的通信状态，根据 ComM（Communication Manager）的请求在 No Communication、Silent Communication、Full Communication 三种模式之间切换。

CanSM 通过 Bus State Machine（BSM）管理每个 CAN 网络的生命周期，处理控制器模式转换（Started/Stopped/Sleep）、Bus Off 检测与恢复、波特率切换等核心功能。模块支持多网络配置，每个网络独立运行 BSM 状态机。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS CANStateManagement | 4.4.0 | CAN 状态管理模块规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | ComM | 通信管理器，请求通信模式切换 |
| 下层 | CanIf | CAN 接口，控制器模式控制 |
| 下层 | Det | 开发错误检测与报告 |
| 下层 | EcuM | 初始化管理 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│     ComM (通信管理器)                │
├─────────────────────────────────────┤
│     CanSM (CAN 状态管理)             │
├─────────────────────────────────────┤
│     CanIf (CAN 接口)                 │
├─────────────────────────────────────┤
│     Can Driver (CAN 驱动)            │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Bus State Machine (BSM)**: 核心状态机，管理 NOCOM / SILENTCOM / FULLCOM / SILENTCOM_BOR 等主状态
- **子状态机**: 每个主状态内含子状态（如 NOCOM 下的 CC_SLEEP_WAIT、FC_CC_START_WAIT 等）
- **Bus Off 管理**: Bus Off 计数器、阈值检测、两级恢复策略（L1/L2）
- **波特率管理**: 支持运行时波特率切换
- **定时器管理**: 模式切换超时定时器、Bus Off 恢复定时器

### 3.3 文件结构

```
src/bsw/services/cansm/
├── include/
│   ├── CanSm.h         # 公共 API 与类型声明
│   └── CanSm_Cfg.h     # 预编译配置（自动生成）
└── src/
    ├── CanSm.c          # 核心实现（BSM 状态机）
    └── CanSm_Lcfg.c     # 链接时配置
```

---

## 4. 状态机

### 4.1 BSM 主状态机

```
                    RequestComMode(FULL)
          ┌───────────────────────────────────────┐
          │                                       │
          ▼                                       │
    ┌──────────┐  RequestComMode(SILENT)   ┌────────────┐
    │   NOCOM   ├──────────────────────────►│ SILENTCOM  │
    └─────┬────┘                           └──────┬─────┘
          │                                       │
          │ RequestComMode(FULL)                  │ RequestComMode(FULL)
          │                                       │
          ▼                                       ▼
    ┌──────────┐                           ┌─────────────────┐
    │ FULLCOM   │◄─────────────────────────│SILENTCOM_BOR    │
    └─────┬────┘  BusOff Recovery 完成     └─────────────────┘
          │
          │ BusOff 事件 (超过阈值)
          ▼
    ┌─────────────────┐
    │SILENTCOM_BOR    │
    └─────────────────┘
```

### 4.2 BSM 状态定义

| 状态 | 枚举值 | 说明 |
|------|--------|------|
| `CANSM_BSM_S_NOTINITIALIZED` | 0 | 未初始化 |
| `CANSM_BSM_S_NOCOM` | 1 | 无通信（控制器 Sleep/Stopped） |
| `CANSM_BSM_S_SILENTCOM` | 2 | 静默通信（Listen Only） |
| `CANSM_BSM_S_FULLCOM` | 3 | 全通信（正常收发） |
| `CANSM_BSM_S_SILENTCOM_BOR` | 4 | Bus Off 恢复中 |
| `CANSM_BSM_S_WAIT_MODE_CHANGE` | 5 | 等待模式切换确认 |
| `CANSM_BSM_S_CHECKWAKEUP` | 6 | 检查唤醒 |
| `CANSM_BSM_S_CHANGEBAUDRATE` | 7 | 切换波特率 |

### 4.3 NOCOM 子状态

| 子状态 | 说明 |
|--------|------|
| `CANSM_S_NOCOM_NOP` | 空闲，等待请求 |
| `CANSM_S_RESTART_CC` | 重启控制器 |
| `CANSM_S_RESTART_CC_WAIT` | 等待重启确认 |
| `CANSM_S_CC_STOPPED` | 控制器已停止 |
| `CANSM_S_CC_SLEEP` | 控制器休眠 |
| `CANSM_S_CC_SLEEP_WAIT` | 等待休眠确认 |
| `CANSM_S_CC_OFFLINE` | 控制器离线 |

### 4.4 FULLCOM 子状态

| 子状态 | 说明 |
|--------|------|
| `CANSM_S_FULLCOM_NOP` | 空闲，等待请求 |
| `CANSM_S_FC_CC_START` | 启动控制器 |
| `CANSM_S_FC_CC_START_WAIT` | 等待启动确认 |
| `CANSM_S_FC_CC_ONLINE` | 控制器在线 |

### 4.5 Bus Off 恢复子状态

| 子状态 | 说明 |
|--------|------|
| `CANSM_S_BUSOFF_CHECK` | 检查 Bus Off 状态 |
| `CANSM_S_BUSOFF_RECOVERY_L1` | L1 恢复（短暂等待） |
| `CANSM_S_BUSOFF_RECOVERY_L2` | L2 恢复（延长等待） |
| `CANSM_S_BOR_RESTART_CC` | 重启控制器 |
| `CANSM_S_BOR_RESTART_CC_WAIT` | 等待重启确认 |

---

## 5. 核心数据结构

### 5.1 网络配置 `CanSm_NetworkConfigType`

```c
typedef struct {
    uint8 NetworkHandle;            // ComM 通道句柄
    uint8 ControllerId;             // CAN 控制器 ID
    uint8 NumBaudrates;             // 支持的波特率数量
    const CanSm_BaudrateConfigType* BaudrateConfigs; // 波特率配置
    uint16 MainFunctionPeriodMs;    // 主函数周期 (ms)
    uint16 BusOffRecoveryTimeMs;    // Bus Off 恢复超时 (ms)
    uint8  BusOffThreshold;         // Bus Off 阈值
    boolean WakeupSupport;          // 唤醒支持
    boolean BusOffRecoveryEnabled;  // 自动 Bus Off 恢复
    boolean TransceiverSupport;     // 收发器管理支持
    uint8  TransceiverId;           // 收发器 ID
} CanSm_NetworkConfigType;
```

### 5.2 网络运行时状态 `CanSm_NetworkStateType`

```c
typedef struct {
    CanSm_BsmStateType BsmState;           // 当前 BSM 状态
    uint8 SubState;                         // 当前子状态
    ComM_ModeType RequestedComMMode;        // 请求的 ComM 模式
    ComM_ModeType CurrentComMMode;          // 当前 ComM 模式
    uint16 ModeRequestTimer;                // 模式请求超时定时器
    uint16 BusOffRecoveryTimer;             // Bus Off 恢复定时器
    uint8 BusOffCounter;                    // Bus Off 事件计数器
    boolean BusOffEventPending;             // Bus Off 事件挂起
    uint16 CurrentBaudrate;                 // 当前波特率
    uint8 RequestedBaudrateIndex;           // 请求的波特率索引
    boolean BaudrateChangePending;          // 波特率变更挂起
    CanIf_ControllerModeType RequestedCtrlMode; // 请求的控制器模式
    boolean ModeChangePending;              // 模式切换挂起
    boolean Initialized;                    // 网络初始化标志
} CanSm_NetworkStateType;
```

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `CanSM_Init(ConfigPtr)` | 0x00 | 初始化模块 | SWS_CanSM_00001 |
| `CanSM_DeInit()` | 0x01 | 反初始化 | SWS_CanSM_00002 |
| `CanSM_RequestComMode(Network, ComM_Mode)` | 0x02 | 请求通信模式 | SWS_CanSM_00003 |
| `CanSM_GetCurrentComMode(Network, ComM_ModePtr)` | 0x03 | 获取当前通信模式 | SWS_CanSM_00004 |
| `CanSM_ControllerBusOff(ControllerId)` | 0x04 | Bus Off 回调 | SWS_CanSM_00005 |
| `CanSM_MainFunction()` | 0x05 | 主函数 | SWS_CanSM_00006 |
| `CanSM_ControllerModeIndication(ControllerId, Mode)` | 0x07 | 控制器模式指示 | SWS_CanSM_00007 |
| `CanSM_GetVersionInfo(VersionInfo)` | 0x09 | 获取版本信息 | SWS_CanSM_00009 |
| `CanSM_SetBaudrate(Network, BaudRate)` | 0x14 | 设置波特率 | SWS_CanSM_00010 |
| `CanSM_GetBaudrate(Network, BaudRatePtr)` | 0x15 | 获取波特率 | SWS_CanSM_00011 |
| `CanSM_GetCurrentInternalState(Network, StatePtr)` | 0x12 | 获取内部状态 | SWS_CanSM_00012 |

### 6.2 回调函数

| 函数 | 说明 |
|------|------|
| `CanSM_ControllerBusOff(ControllerId)` | CanIf Bus Off 事件回调 |
| `CanSM_ControllerModeIndication(ControllerId, Mode)` | CanIf 控制器模式变更确认 |

### 6.3 服务 ID 与错误码

**DET 错误码**:

| 错误码 | 宏名 | 说明 |
|--------|------|------|
| 0x01 | `CANSM_E_PARAM_POINTER` | 无效指针参数 |
| 0x02 | `CANSM_E_PARAM_CONTROLLER` | 无效控制器参数 |
| 0x03 | `CANSM_E_PARAM_INVALID_NETWORK_MODE` | 无效网络模式 |
| 0x04 | `CANSM_E_INVALID_COMM_REQUEST` | 无效通信请求 |
| 0x05 | `CANSM_E_MODE_REQUEST_TIMEOUT` | 模式请求超时 |
| 0x06 | `CANSM_E_UNEXPECTED_EXECUTION` | 意外执行 |
| 0x07 | `CANSM_E_NOT_INITIALIZED` | 未初始化 |
| 0x08 | `CANSM_E_INVALID_BAUDRATE` | 无效波特率 |
| 0x09 | `CANSM_E_BUSOFF_RECOVERY_ACTIVE` | Bus Off 恢复中 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 设置 `InitStatus = CANSM_INIT`
2. 遍历所有网络，初始化 BSM 状态为 `CANSM_BSM_S_NOCOM`
3. 初始化子状态、定时器、计数器
4. 保存配置指针

### 7.2 主函数处理流程

`CanSM_MainFunction()` 周期调用（默认 10ms），执行:

1. 遍历所有网络
2. 根据当前 BSM 状态分发处理:
   - `NOCOM`: 处理模式切换请求（SILENT/FULL）
   - `SILENTCOM`: 处理模式切换请求（NOCOM/FULL）
   - `FULLCOM`: 处理模式切换请求（NOCOM/SILENT），检测 Bus Off
   - `SILENTCOM_BOR`: 执行 Bus Off 恢复序列
3. 递减超时定时器，检测超时错误

### 7.3 Bus Off 恢复流程

1. `CanSM_ControllerBusOff()` 被 CanIf 调用
2. BusOffCounter 递增
3. 若 BusOffCounter < 阈值: 尝试立即重启（STOPPED → STARTED）
4. 若 BusOffCounter >= 阈值: 进入 SILENTCOM_BOR 状态
   - L1 恢复: 等待 `CANSM_BUSOFF_RECOVERY_L1_MS` (100ms)
   - L2 恢复: 等待 `CANSM_BUSOFF_RECOVERY_L2_MS` (1000ms)
   - 执行 STOPPED → STARTED 重启序列

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `CANSM_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `CANSM_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `CANSM_NUM_NETWORKS` | 2 | 网络数量 |
| `CANSM_SET_BAUDRATE_API` | STD_ON | 波特率设置 API |
| `CANSM_DEFAULT_BAUDRATE` | 500 | 默认波特率 (kbps) |
| `CANSM_MODE_CHANGE_REQUEST_TIMEOUT_MS` | 100 | 模式切换超时 (ms) |
| `CANSM_BUSOFF_RECOVERY_L1_MS` | 100 | L1 恢复时间 (ms) |
| `CANSM_BUSOFF_RECOVERY_L2_MS` | 1000 | L2 恢复时间 (ms) |
| `CANSM_BUSOFF_THRESHOLD` | 10 | Bus Off 阈值 |
| `CANSM_MAX_NETWORKS` | 4 | 最大网络数 |

### 8.2 链接时配置

通过 `CanSm_Config` 和 `CanSm_ConfigPtr` 提供链接时配置。

### 8.3 构建后配置

不支持构建后配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 入口进行参数校验（指针非空、网络句柄有效、模块已初始化）。

### 9.2 DEM 错误

Bus Off 事件通过 `CANSM_E_BUSOFF_RECOVERY_ACTIVE` 报告，可由 Dem 记录为 NVM 事件。

### 9.3 安全机制

- **模式切换超时检测**: 防止控制器模式切换无限等待
- **Bus Off 阈值保护**: 超过阈值后进入安全恢复模式
- **版本检查**: 编译时 AR 版本一致性检查（`#error` 宏）

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 段名 | 类型 | 内容 |
|------|------|------|
| `CANSM_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据 | `CanSm_Config` |
| `CANSM_START_SEC_CODE` | 代码段 | 所有函数实现 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| 每网络 RAM | ~40 bytes | CanSm_NetworkStateType |
| 波特率配置 | 24 bytes/网络 | 4 个波特率配置项 |
| ROM（代码） | ~5 KB | BSM 状态机 + API |

---

## 11. 集成指南

1. **ComM 集成**: ComM 通过 `CanSM_RequestComMode()` 请求通信模式
2. **CanIf 集成**: 配置控制器 ID，注册 Bus Off 和模式指示回调
3. **EcuM 集成**: 在启动阶段调用 `CanSM_Init()`
4. **SchM 集成**: 配置 `CanSM_MainFunction()` 调用周期（10ms）
5. **波特率配置**: 每个网络支持 125K/250K/500K/1000K 四种波特率

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| 初始化测试 | NULL 指针检测、状态初始化验证 |
| 模式切换测试 | NOCOM → FULLCOM → NOCOM 完整序列 |
| Bus Off 恢复测试 | 低于阈值/超过阈值的恢复路径 |
| 超时测试 | 模式切换超时 DET 错误报告 |
| 波特率测试 | SetBaudrate/GetBaudrate 功能验证 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| ComM-CanSM 集成 | 完整的通信模式请求/确认流程 |
| CanIf-CanSM 集成 | 控制器模式切换的端到端验证 |
| 多网络测试 | 两个 CAN 网络独立状态管理 |

---

## 13. 实现说明 / TODO

- `CanSm_Lcfg.c` 中 `CanSm_Config` 初始化为 `{ 0U }`，需要由配置工具填充实际值
- 部分子状态（如 `CANSM_S_CC_STOPPED_WAIT`、`CANSM_S_BOR_CC_STOPPED_WAIT`）的处理逻辑尚未完整实现
- `CanSM_GetCurrentComMode()`、`CanSM_DeInit()` 函数原型已声明但实现待补充
- `CanSM_ControllerErrorSstatusIndication()` SID=0x3C 已声明但未实现

---

## 14. 参考资料

- AUTOSAR SWS CANStateManagement (AUTOSAR_SWS_CANStateManagement.pdf)
- AUTOSAR SWS CommunicationManager
- 源码: `src/bsw/services/cansm/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_CanSM | — | CANSM 模块级需求归属 |
| SWS_CanSM_00008 | `CanSm_ControllerBusOff_Recovery` | 测试 test_CanSm_ControllerBusOff_Recovery_ShouldNotCrash 覆盖: CanSm_ControllerBusOff_Recovery_ShouldNotCrash 场景 |
