# J1939Nm (J1939 Network Management) Design Document

> **Module ID**: 0x8D  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_J1939NetworkManagement  
> **Source Path**: `src/bsw/services/j1939nm/`  
> **Reference Document**: `docs/modules/j1939nm.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

J1939Nm 模块实现 SAE J1939-81 网络管理协议，负责商用车 CAN 网络中的节点地址声明（Address Claiming）管理。该模块确保网络中每个 ECU 拥有唯一的源地址（SA），通过 NAME（64 位全局唯一标识符）优先级机制解决地址冲突。

主要功能：
- **地址声明（Address Claiming）**：通过 Address Claimed 消息（PGN 0xEE00）声明节点地址
- **地址冲突检测与解决**：基于 NAME 优先级比较，低 NAME 值（高优先级）保留地址
- **Bus-Off 恢复**：检测 CAN Bus-Off 状态并执行恢复流程
- **节点检测与监控**：发现网络中的其他节点并监控其在线状态
- **多通道支持**：支持多个 CAN 通道独立管理

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| SAE J1939-81 | Latest | J1939 网络管理层 |
| AUTOSAR SWS J1939 Network Management | 4.4.0 | 模块软件规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | RTE / Application | 地址查询与状态通知 |
| 同层 | CanIf | CAN 消息收发、Bus-Off 通知 |
| 公共 | Det | 开发错误检测与报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│         RTE / Application           │
├─────────────────────────────────────┤
│         J1939Nm (Services)          │
├─────────────────────────────────────┤
│         CanIf (CAN Interface)       │
├─────────────────────────────────────┤
│         Can Driver / CanTrcv        │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **NM 状态机**：管理每个通道的网络管理主状态（UNINIT → WAIT_FOR_AC → NORMAL_OPERATION）
- **地址声明状态机**：管理地址声明子状态（IDLE → WAITING → CLAIMED → CONFLICT → CANNOT_CLAIM）
- **NAME 比较器**：64 位 NAME 值比较，确定地址冲突中的优先级
- **PDU 构建器**：构建 Address Claimed / Cannot Claim / Request for AC 消息
- **Bus-Off 管理器**：处理 Bus-Off 事件和恢复定时器
- **通道状态管理**：每个通道独立维护地址、NAME、定时器状态

### 3.3 文件结构

```
src/bsw/services/j1939nm/
├── include/
│   ├── J1939Nm.h          -- 公共 API 与类型定义
│   └── J1939Nm_Cfg.h      -- 预编译配置参数
└── src/
    └── J1939Nm.c           -- 核心实现
```

---

## 4. 状态机

### 4.1 NM 主状态机

```
UNINIT -- Init() --> WAIT_FOR_AC
WAIT_FOR_AC -- 地址声明成功 --> NORMAL_OPERATION
WAIT_FOR_AC -- 无法声明地址 --> TX_CANNOT_CLAIM
NORMAL_OPERATION -- Bus-Off --> BUS_OFF
BUS_OFF -- 恢复定时器到期 --> WAIT_FOR_AC
```

状态说明：
- `J1939NM_STATE_UNINIT`：模块未初始化
- `J1939NM_STATE_BUS_OFF`：CAN Bus-Off 状态，等待恢复
- `J1939NM_STATE_WAIT_FOR_AC`：等待地址声明完成
- `J1939NM_STATE_AC_DELAY`：地址声明延迟中（随机退避）
- `J1939NM_STATE_NORMAL_OPERATION`：正常运行，地址已声明
- `J1939NM_STATE_TX_AC`：正在发送 Address Claimed 消息
- `J1939NM_STATE_TX_CANNOT_CLAIM`：无法声明地址，使用 NULL 地址

### 4.2 地址声明状态机

```
IDLE -- 启动声明 --> WAITING
WAITING -- 延迟到期，发送 AC --> CLAIMED
WAITING -- 发送失败 --> CANNOT_CLAIM
CLAIMED -- 检测到冲突 --> CONFLICT
CONFLICT -- NAME 较低(保持) --> CLAIMED (重发 AC)
CONFLICT -- NAME 较高(让出) + 支持任意地址 --> WAITING (新地址)
CONFLICT -- NAME 较高(让出) + 不支持任意地址 --> CANNOT_CLAIM
CONFLICT -- 重试次数超限 --> CANNOT_CLAIM
CANNOT_CLAIM -- 使用 NULL 地址(254) --> (终态)
```

### 4.3 通道运行时状态

```c
typedef struct {
    J1939Nm_StateType State;           /* NM 主状态 */
    J1939Nm_AcStateType AcState;       /* 地址声明子状态 */
    J1939Nm_NameType Name;             /* 当前 NAME */
    J1939Nm_AddressType Address;       /* 当前地址 */
    J1939Nm_AddressType PreferredAddress; /* 首选地址 */
    boolean BusOffState;               /* Bus-Off 状态 */
    boolean AddressClaimed;            /* 地址已声明标志 */
    uint16 AcDelayTimer;               /* AC 延迟定时器 */
    uint16 AcTimeoutTimer;             /* AC 超时定时器 */
    uint16 BusOffRecoveryTimer;        /* Bus-Off 恢复定时器 */
    uint16 AcRepeatTimer;              /* AC 重复发送定时器 */
    uint8 AcRetryCount;                /* AC 重试计数 */
} J1939Nm_ChannelStateType;
```

---

## 5. 核心数据结构

### 5.1 J1939 NAME 结构

NAME 为 64 位值（`uint64`），按 J1939-81 定义包含以下字段：

| 字段 | 位宽 | 说明 |
|------|------|------|
| Identity Number | 21 bits | 车辆识别码 |
| Manufacturer Code | 11 bits | 制造商代码 |
| ECU Instance | 3 bits | ECU 实例号 |
| Function Instance | 5 bits | 功能实例号 |
| Function | 8 bits | 功能定义 |
| Reserved | 1 bit | 保留位 |
| Vehicle System | 7 bits | 车辆系统 |
| Vehicle System Instance | 4 bits | 车辆系统实例 |
| Industry Group | 3 bits | 行业组 |
| Arbitrary Address Capable | 1 bit | 支持任意地址 |

### 5.2 配置类型

```c
typedef struct {
    J1939Nm_ChannelType ChannelId;
    J1939Nm_NodeType NodeId;
    J1939Nm_NameType Name;              /* 64 位 NAME */
    J1939Nm_AddressType Address;        /* 默认地址 */
    J1939Nm_AddressType PreferredAddress; /* 首选地址 */
    boolean ArbitraryAddressCapable;    /* 任意地址能力 */
    uint8 AcDelayMin;                   /* 最小 AC 延迟 (ms) */
    uint8 AcDelayMax;                   /* 最大 AC 延迟 (ms) */
    uint8 AcTimeout;                    /* AC 超时 (ms) */
    uint8 BusOffRecoveryTime;           /* Bus-Off 恢复时间 (ms) */
} J1939Nm_ChannelConfigType;
```

### 5.3 PDU 标识

| PDU | PGN | CAN ID | 说明 |
|-----|-----|--------|------|
| Address Claimed | 0xEE00 | 0x00EE00 | 地址声明消息 |
| Request for AC | 0xEA00 | 0x00EA00 | 请求地址声明 |
| Cannot Claim | 0xEEFF | 0x00EEFF | 无法声明（NULL 地址） |

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `J1939Nm_Init(ConfigPtr)` | 0x01 | 初始化模块 |
| `J1939Nm_DeInit()` | 0x02 | 反初始化模块 |
| `J1939Nm_GetVersionInfo(VersionInfo)` | 0x03 | 获取版本信息 |
| `J1939Nm_GetState(Channel, State)` | 0x04 | 获取 NM 状态 |
| `J1939Nm_GetBusOffState(Channel, BusOffState)` | 0x05 | 获取 Bus-Off 状态 |
| `J1939Nm_SetBusOffState(Channel, BusOffState)` | 0x06 | 设置 Bus-Off 状态 |
| `J1939Nm_GetAddress(Channel, Address)` | 0x07 | 获取当前地址 |
| `J1939Nm_SetAddress(Channel, Address)` | 0x08 | 设置地址 |
| `J1939Nm_GetName(Channel, Name)` | 0x09 | 获取 NAME |
| `J1939Nm_SetName(Channel, Name)` | 0x0A | 设置 NAME |
| `J1939Nm_MainFunction()` | 0x0B | 周期处理函数 |
| `J1939Nm_RequestAddressClaimed(Channel)` | - | 请求发送 AC 消息 |
| `J1939Nm_RequestCannotClaimAddress(Channel)` | - | 请求发送 Cannot Claim |
| `J1939Nm_HandleAddressConflict(Channel, Name, Address)` | - | 处理地址冲突 |

### 6.2 回调函数

| 函数 | SID | 说明 |
|------|-----|------|
| `J1939Nm_BusOffCbk(Channel)` | 0x0C | Bus-Off 回调 |
| `J1939Nm_RxIndication(Channel, CanId, Data, DataLength)` | 0x0D | 接收指示 |
| `J1939Nm_TxConfirmation(Channel, TxPduId, result)` | 0x0E | 发送确认 |

### 6.3 服务 ID 与错误码

**DET 错误码：**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `J1939NM_E_NOT_INITIALIZED` | 0x01 | 模块未初始化 |
| `J1939NM_E_INVALID_PARAMETER` | 0x02 | 无效参数 |
| `J1939NM_E_INVALID_POINTER` | 0x03 | 空指针 |
| `J1939NM_E_INVALID_STATE` | 0x04 | 无效状态 |
| `J1939NM_E_INIT_FAILED` | 0x05 | 初始化失败 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 `ConfigPtr` 空指针
2. 保存配置指针
3. 遍历所有通道（`J1939NM_NUMBER_OF_CHANNELS`）：
   - 设置状态为 `WAIT_FOR_AC`
   - 设置 AC 子状态为 `IDLE`
   - 加载 NAME 和首选地址
   - 地址初始化为 NULL (254)
   - 所有定时器清零
4. 设置 `J1939Nm_Initialized = TRUE`

### 7.2 地址声明流程

1. **延迟阶段**：从 `IDLE` 进入 `WAITING`，计算随机延迟：
   ```
   AcDelayTimer = AcDelayMin + (NAME % 100) % (AcDelayMax - AcDelayMin + 1)
   ```
2. **发送阶段**：延迟到期后，构建 AC PDU（8 字节 NAME，小端序），通过 CanIf 发送
3. **确认阶段**：发送成功后进入 `CLAIMED`，启动重复发送定时器
4. **重复发送**：每 `AC_REPEAT_TIME_MS / MAIN_FUNCTION_PERIOD_MS` 个周期重复发送 AC

### 7.3 地址冲突处理

1. `J1939Nm_RxIndication()` 收到 Address Claimed 消息（PGN 0xEE00）
2. 提取源地址（CAN ID 低 8 位）和 NAME（8 字节小端序）
3. 比较源地址与本地地址：
   - 相同 → 调用 `J1939Nm_HandleAddressConflict()`
4. NAME 比较结果：
   - 本地 NAME < 远程 NAME → 本地优先级高，重发 AC 保持地址
   - 本地 NAME > 远程 NAME → 本地优先级低，进入 CONFLICT 状态
   - 相同 NAME → 视为冲突
5. CONFLICT 处理：
   - 支持任意地址：尝试新地址（PreferredAddress + RetryCount），最多 10 次
   - 不支持或重试超限：发送 Cannot Claim，使用 NULL 地址

### 7.4 Bus-Off 恢复流程

1. `J1939Nm_BusOffCbk()` 被调用
2. 设置 `BusOffState = TRUE`，状态切换到 `BUS_OFF`
3. 启动恢复定时器：`BusOffRecoveryTimer = BUS_OFF_RECOVERY_TIME_MS / MAIN_FUNCTION_PERIOD_MS`
4. `MainFunction()` 递减定时器
5. 定时器到期后：清除 Bus-Off 状态，回到 `WAIT_FOR_AC` 重新声明地址

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `J1939NM_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `J1939NM_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `J1939NM_NODE_DETECTION_ENABLED` | STD_ON | 节点检测 |
| `J1939NM_NODE_MONITORING_ENABLED` | STD_ON | 节点监控 |
| `J1939NM_BUS_OFF_RECOVERY_ENABLED` | STD_ON | Bus-Off 恢复 |
| `J1939NM_NUMBER_OF_CHANNELS` | 1 | 通道数 |
| `J1939NM_NUMBER_OF_NODES` | 1 | 节点数 |
| `J1939NM_AC_DELAY_MIN_MS` | 50 | 最小 AC 延迟 |
| `J1939NM_AC_DELAY_MAX_MS` | 150 | 最大 AC 延迟 |
| `J1939NM_AC_TIMEOUT_MS` | 250 | AC 超时 |
| `J1939NM_BUS_OFF_RECOVERY_TIME_MS` | 1000 | Bus-Off 恢复时间 |
| `J1939NM_AC_REPEAT_TIME_MS` | 1000 | AC 重复间隔 |
| `J1939NM_MAIN_FUNCTION_PERIOD_MS` | 10 | 主函数周期 |
| `J1939NM_DEFAULT_ADDRESS` | 128 | 默认地址 |
| `J1939NM_NULL_ADDRESS` | 254 | NULL 地址 |
| `J1939NM_GLOBAL_ADDRESS` | 255 | 全局地址 |

### 8.2 链接时配置

当前实现不包含独立的链接时配置文件，配置通过 `J1939Nm_Config` 全局结构体引用。

### 8.3 构建后配置

不支持构建后配置变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 均包含以下检查：
- 初始化状态检查 → `J1939NM_E_NOT_INITIALIZED`
- 空指针检查 → `J1939NM_E_INVALID_POINTER`
- 通道 ID 范围检查 → `J1939NM_E_INVALID_PARAMETER`

### 9.2 DEM 错误

当前实现未报告 DEM 事件。可扩展添加地址冲突事件记录。

### 9.3 安全机制

- **MemMap 保护**：变量和配置数据使用标准 MemMap 分区宏
- **NAME 优先级**：基于 J1939-81 的确定性地址冲突解决
- **随机退避**：AC 延迟使用 NAME 值作为随机种子，避免多节点同时声明
- **重试限制**：最多 10 次地址重试，防止无限循环

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 类型 | 说明 |
|------|------|------|
| `J1939NM_START_SEC_VAR_INIT_UNSPECIFIED` | 已初始化变量 | 模块初始化标志、配置指针 |
| `J1939NM_START_SEC_VAR_CLEARED_UNSPECIFIED` | 已清零变量 | 通道状态数组 |
| `J1939NM_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据 | 全局配置结构体 |
| `J1939NM_START_SEC_CODE` | 代码段 | 所有 API 函数 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| `J1939Nm_ChannelStates[]` | N x ~32 bytes | 通道运行时状态 |
| TX 数据缓冲 | 8 bytes | AC PDU 构建 |
| 代码段 | ~3 KB (估算) | 状态机 + 消息处理 |

---

## 11. 集成指南

1. **CanIf 路由配置**：
   - RX: PGN 0xEE00 (Address Claimed) → `J1939Nm_RxIndication`
   - RX: PGN 0xEA00 (Request for AC) → `J1939Nm_RxIndication`
   - TX: Address Claimed → CanIf_Transmit
   - Bus-Off → `J1939Nm_BusOffCbk`
2. **调度配置**：`J1939Nm_MainFunction()` 建议 10ms 周期调用
3. **NAME 配置**：每个节点需配置唯一的 64 位 NAME 值
4. **地址规划**：首选地址应在 J1939 有效范围 (0-253) 内

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| Init/DeInit | 状态转换与参数初始化 |
| NAME 比较 | 验证优先级比较逻辑 |
| AC PDU 构建 | 验证 64 位 NAME 小端序编码 |
| 地址冲突 | 高/低 NAME 冲突处理 |
| 任意地址 | 地址重试逻辑（最多 10 次） |
| Bus-Off 恢复 | 定时器到期与状态恢复 |
| 空指针/无效参数 | DET 错误报告 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| 多节点地址声明 | 3+ 节点同时启动的地址分配 |
| 冲突解决 | NAME 优先级验证 |
| Bus-Off 恢复 | 物理层故障恢复后的地址重声明 |
| Request for AC | 新节点加入时的地址查询 |

---

## 13. 实现说明 / TODO

- **CanIf_Transmit 调用**：`J1939Nm_TransmitAddressClaimed()` 等函数中的 CanIf_Transmit 调用为占位实现，需集成实际 CanIf 驱动
- **Request for AC 处理**：收到请求后应重发 AC，当前已实现但依赖 CanIf
- **TxConfirmation**：当前为空实现，可扩展用于状态机优化
- **节点检测**：`NodeDetectionEnabled` 配置已定义但未完整实现
- **节点监控**：`NodeMonitoringEnabled` 配置已定义但未完整实现

---

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| SAE J1939-81 | J1939 网络管理层规范 |
| AUTOSAR_SWS_J1939NetworkManagement | AUTOSAR J1939 NM 模块规范 |
| AUTOSAR_SWS_CanInterface | CAN 接口规范 |
| `src/bsw/services/j1939nm/` | 源代码目录 |
