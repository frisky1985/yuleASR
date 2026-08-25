# EthTSyn Design Document

> **Module ID**: 0x9D  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_EthernetTimeSynchronization  
> **Source Path**: `src/bsw/services/ethtsyn/`  
> **Reference Document**: `docs/modules/ethtsyn.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

EthTSyn（Ethernet Time Synchronization）是 AUTOSAR ECU 抽象层模块，提供基于 IEEE 802.1AS（gPTP）的以太网时间同步服务。该模块负责管理 PTP（Precision Time Protocol）端口状态、提供全局同步时钟的读写接口，以及支持时钟速率调整伺服环路。

本实现为轻量级桩实现（stub），提供完整的 API 结构和基本功能框架，用于满足 AUTOSAR 可追溯性要求。完整的 gPTP 实现需要硬件特定的 PTP 时间戳支持。

主要功能：
- 提供全局同步时钟的读取和设置
- 管理 gPTP 端口状态机
- 支持时钟速率调整（伺服环路）
- 提供时钟身份标识查询
- 支持多域（domain）配置

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS EthernetTimeSynchronization | 4.4.0 | 以太网时间同步规范 |
| IEEE 802.1AS (gPTP) | - | 音视频桥接系统时间同步标准 |
| IEEE 1588 (PTP) | v2 | 精密时钟协议 |

### 2.2 模块依赖

| 模块 | 依赖方向 | 说明 |
|------|----------|------|
| Eth | 调用 | 以太网 MCAL 驱动，用于 PTP 帧收发 |
| Det | 调用 | 默认错误追踪 |
| SchM_EthTSyn | 包含 | 调度管理器，提供独占区域宏 |
| Std_Types | 包含 | AUTOSAR 标准类型定义 |

## 3. 架构设计

### 3.1 分层位置

```
+-----------------------------------+
|       BSW Services                |
+-----------------------------------+
|    ECUAL Layer (EthTSyn)          |  <-- 本模块
+-----------------------------------+
|    Eth (MCAL)                     |
+-----------------------------------+
|    Hardware (PTP Timestamping)    |
+-----------------------------------+
```

**注意**：虽然源代码位于 `src/bsw/services/ethtsyn/` 目录下，但 EthTSyn 在 AUTOSAR 架构中属于 ECUAL 层模块。

### 3.2 内部组件

- **本地时钟管理**：维护秒（seconds）和纳秒（nanoseconds）精度的本地时钟
- **端口状态管理**：管理 gPTP 端口状态机（INIT → LISTENING → MASTER/SLAVE 等）
- **DET 错误检测**：所有 API 均进行初始化和参数验证

### 3.3 文件结构

```
src/bsw/services/ethtsyn/
├── include/
│   ├── EthTSyn.h          # 公共 API 头文件
│   └── SchM_EthTSyn.h     # 调度管理器头文件（独占区域宏）
└── src/
    └── EthTSyn.c           # 模块实现（桩实现）
```

## 4. 状态机

EthTSyn 管理 gPTP 端口状态机，端口状态定义如下：

```
    ETHTSYN_PORT_INIT
         │
         v
    ETHTSYN_PORT_LISTENING
         │
    ┌────┴────┐
    v         v
ETHTSYN_    ETHTSYN_
PORT_MASTER PORT_SLAVE
    │         │
    v         v
ETHTSYN_    ETHTSYN_
PORT_PASSIVE PORT_UNCALIBRATED
    │
    v
ETHTSYN_PORT_FAULTY / ETHTSYN_PORT_DISABLED
```

| 状态 | 说明 |
|------|------|
| ETHTSYN_PORT_INIT | 端口初始化 |
| ETHTSYN_PORT_FAULTY | 端口故障 |
| ETHTSYN_PORT_DISABLED | 端口禁用 |
| ETHTSYN_PORT_LISTENING | 端口监听中 |
| ETHTSYN_PORT_PRE_MASTER | 预主状态 |
| ETHTSYN_PORT_MASTER | 主时钟 |
| ETHTSYN_PORT_PASSIVE | 被动状态 |
| ETHTSYN_PORT_UNCALIBRATED | 未校准 |
| ETHTSYN_PORT_SLAVE | 从时钟 |

**注意**：当前桩实现中，`EthTSyn_GetPortState` 固定返回 `ETHTSYN_PORT_LISTENING`。

## 5. 核心数据结构

### 5.1 配置结构

```c
typedef struct {
    uint8 domainNumber;              /* gPTP 域编号 */
    boolean masterOnly;              /* 仅主时钟模式 */
    uint16 logSyncInterval;          /* Sync 报文间隔（对数） */
    uint16 logAnnounceInterval;      /* Announce 报文间隔 */
    uint16 logPdelayReqInterval;     /* Pdelay_Req 报文间隔 */
    uint16 priority1;                /* 优先级 1 */
    uint16 priority2;                /* 优先级 2 */
    uint8 clockClass;                /* 时钟等级 */
    uint8 clockAccuracy;             /* 时钟精度 */
    uint16 offsetScaledLogVariance;  /* 偏移缩放对数方差 */
} EthTSyn_ConfigType;
```

### 5.2 时间戳结构

```c
typedef struct {
    uint64 seconds;       /* 秒 */
    uint32 nanoseconds;   /* 纳秒 */
} EthTSyn_TimestampType;
```

### 5.3 时钟身份标识

```c
typedef struct {
    uint8 id[8];          /* 8 字节时钟身份标识 */
} EthTSyn_ClockIdentityType;
```

### 5.4 运行时变量

```c
static uint64 EthTSyn_LocalSeconds = 0U;      /* 本地时钟秒 */
static uint32 EthTSyn_LocalNanoSeconds = 0U;   /* 本地时钟纳秒 */
static boolean EthTSyn_Initialized = FALSE;     /* 初始化标志 */
```

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `EthTSyn_Init(config)` | 0x00 | 初始化模块 |
| `EthTSyn_DeInit()` | - | 去初始化模块 |
| `EthTSyn_MainFunction()` | - | 周期处理函数 |
| `EthTSyn_GetTime(*timestamp)` | 0x01 | 获取当前同步时间 |
| `EthTSyn_SetTime(*timestamp)` | 0x02 | 设置本地时间 |
| `EthTSyn_AdjustRate(num, denom)` | 0x03 | 调整时钟速率（伺服环路） |
| `EthTSyn_GetPortState(portIndex, *state)` | 0x04 | 获取端口状态 |
| `EthTSyn_GetClockIdentity(*identity)` | 0x05 | 获取时钟身份标识 |
| `EthTSyn_GetVersionInfo(*versioninfo)` | - | 获取版本信息（条件编译） |

### 6.2 回调函数

本模块无回调函数定义。

### 6.3 服务 ID 与错误码

**DET 错误码（使用通用 DET 错误码）：**

| 错误码 | 说明 |
|--------|------|
| DET_E_PARAM_POINTER | 空指针参数 |
| DET_E_ALREADY_INITIALIZED | 重复初始化 |
| DET_E_UNINIT | 模块未初始化 |

## 7. 处理流程

### 7.1 初始化流程

1. 检查配置指针是否为 NULL（触发 DET）
2. 检查是否已初始化（触发 DET）
3. 保存配置指针
4. 清零本地时钟（seconds = 0, nanoseconds = 0）
5. 设置初始化标志为 TRUE

### 7.2 时间获取流程

1. 检查初始化状态
2. 检查输出指针有效性
3. 将本地秒和纳秒复制到输出结构

### 7.3 时间设置流程

1. 检查初始化状态
2. 检查输入指针有效性
3. 将输入时间值写入本地时钟变量

### 7.4 MainFunction 周期处理

当前桩实现中 MainFunction 仅检查初始化状态，无实际处理逻辑。完整实现应包含：
- PTP 帧收发处理
- Best Master Clock 算法
- 时钟伺服环路维护
- 路径延迟测量

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| ETHTSYN_MODULE_ID | 0x0A | 模块 ID |
| ETHTSYN_DEV_ERROR_DETECT | STD_ON | DET 错误检测使能 |
| ETHTSYN_VERSION_INFO_API | STD_ON | 版本信息 API 使能 |

### 8.2 链接时配置

配置通过 `EthTSyn_ConfigType` 结构体在初始化时传入，支持以下参数：
- gPTP 域编号
- 主时钟优先模式
- Sync/Announce/Pdelay 报文间隔
- 时钟优先级和等级

### 8.3 构建后配置

本实现不支持构建后配置。

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 均进行以下检查：
- 初始化状态检查（DET_E_UNINIT）
- 指针参数有效性检查（DET_E_PARAM_POINTER）
- 重复初始化检查（DET_E_ALREADY_INITIALIZED）
- 除零检查（AdjustRate 的 rateDenominator）

### 9.2 DEM 错误

本实现未集成 DEM 事件上报。

### 9.3 安全机制

- **独占区域**：通过 SchM_EthTSyn 提供 `SchM_Enter_EthTSyn_EXCLUSIVE_AREA_0()` 和 `SchM_Exit_EthTSyn_EXCLUSIVE_AREA_0()` 宏（当前为空实现）
- **静默失败**：DeInit 不进行初始化检查，直接清零状态

## 10. 内存与性能

### 10.1 MemMap 分区

本实现未使用 MemMap 分区宏。

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| EthTSyn_LocalSeconds | 8 bytes | 本地时钟秒 |
| EthTSyn_LocalNanoSeconds | 4 bytes | 本地时钟纳秒 |
| EthTSyn_Initialized | 1 byte | 初始化标志 |
| EthTSyn_ConfigPtr | 4 bytes | 配置指针 |
| **总计 RAM** | **~17 bytes** | 运行时变量 |

## 11. 集成指南

### 集成步骤

1. 准备 `EthTSyn_ConfigType` 配置结构体
2. 在初始化序列中调用 `EthTSyn_Init(&config)`
3. 将 `EthTSyn_MainFunction()` 加入周期任务
4. 通过 `EthTSyn_GetTime()` 获取全局同步时间
5. 确保 Eth MCAL 驱动支持 PTP 时间戳功能

### 注意事项

- 本实现为桩实现，`EthTSyn_AdjustRate` 和 `EthTSyn_GetClockIdentity` 不进行实际操作
- `EthTSyn_GetPortState` 固定返回 LISTENING 状态
- 完整实现需要硬件 PTP 时间戳支持

## 12. 测试策略

### 12.1 单元测试

| 测试场景 | 预期结果 |
|----------|----------|
| Init 正常初始化 | 返回 E_OK，时钟清零 |
| Init NULL 指针 | 触发 DET，返回 E_NOT_OK |
| 重复 Init | 触发 DET，返回 E_NOT_OK |
| DeInit 后 GetTime | 触发 DET，返回 E_NOT_OK |
| GetTime 正常读取 | 返回 E_OK，时间值正确 |
| SetTime 后 GetTime | 返回设置的时间值 |
| AdjustRate 除零 | 触发 DET，返回 E_NOT_OK |
| GetPortState 查询 | 返回 LISTENING 状态 |

### 12.2 集成测试

| 测试场景 | 验证点 |
|----------|--------|
| EthTSyn + Eth 联合测试 | PTP 帧收发功能 |
| 多域时间同步 | 不同域的时间独立性 |
| 时钟伺服环路 | 时间同步精度 |

## 13. 实现说明 / TODO

### 当前实现特点

- 轻量级桩实现，提供完整 API 框架
- 基本的时间读写功能
- 完整的 DET 错误检测

### 待实现项

- [ ] 完整的 gPTP 状态机实现（IEEE 802.1AS）
- [ ] PTP 帧收发处理
- [ ] Best Master Clock 算法
- [ ] 时钟伺服环路（Clock Servo）
- [ ] 路径延迟测量（Pdelay 机制）
- [ ] 硬件 PTP 时间戳集成
- [ ] 独占区域的实际实现（禁用中断）
- [ ] DEM 事件上报

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| AUTOSAR_SWS_EthernetTimeSynchronization.pdf | AUTOSAR EthTSyn 规范 |
| IEEE 802.1AS-2020 | gPTP 标准 |
| IEEE 1588-2019 | PTP v2 标准 |
| EthTSyn.h | 模块公共接口定义 |
| EthTSyn.c | 模块实现源码 |
| SchM_EthTSyn.h | 调度管理器定义 |
