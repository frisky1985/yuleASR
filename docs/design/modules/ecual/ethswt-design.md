# EthSwt Design Document

> **Module ID**: 0x9E  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_EthernetSwitchDriver  
> **Source Path**: `src/bsw/ecual/ethswt/`  
> **Reference Document**: `docs/modules/ethswt.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

EthSwt（Ethernet Switch Driver）是 AUTOSAR ECU 抽象层中负责管理多端口以太网交换机的核心模块。该模块提供交换机端口配置、帧转发、VLAN 过滤、流控、MAC 地址过滤、端口镜像和统计等完整功能。

主要功能：
- 多端口交换机管理（最多 ETHSWT_MAX_PORTS = 8 端口）
- 端口使能/禁用、速率/双工配置
- 完整的 VLAN 管理（成员表、PVID、VID-PCP 映射、802.1p 优先级）
- 帧转发（含入站/出站 VLAN 成员过滤）
- 流控（Pause 帧、高/低水位线）
- MAC 地址过滤
- 端口统计（TX/RX 帧、字节、VLAN 帧、丢弃帧等）
- 端口镜像
- 硬件抽象 + 软件仿真回退

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS EthernetSwitchDriver | 4.4.0 | 以太网交换机驱动规范 |
| IEEE 802.1Q | - | VLAN 标记标准 |
| IEEE 802.1p | - | 优先级标记标准 |
| IEEE 802.3x | - | 流控标准 |

### 2.2 模块依赖

| 模块 | 依赖方向 | 说明 |
|------|----------|------|
| Det | 调用 | 默认错误追踪 |
| Eth (MCAL) | 调用 | 以太网底层驱动 |
| Std_Types | 包含 | AUTOSAR 标准类型 |
| string.h | 包含 | memset 等内存操作 |

## 3. 架构设计

### 3.1 分层位置

```
+-----------------------------------+
|       BSW Services / ECUAL        |
+-----------------------------------+
|    ECUAL Layer (EthSwt)           |  <-- 本模块
+-----------------------------------+
|    Eth (MCAL)                     |
+-----------------------------------+
|    Hardware (Switch ASIC/SoC)     |
+-----------------------------------+
```

### 3.2 内部组件

- **端口管理器**：管理 8 个端口的运行时状态（使能、链路、速率、双工、统计）
- **VLAN 表管理器**：维护最多 16 个 VLAN 条目，支持成员过滤
- **帧转发引擎**：实现入站/出站 VLAN 成员过滤、Untagged 帧处理
- **流控引擎**：基于 TX 队列深度的 Pause 帧生成和接收处理
- **镜像引擎**：将指定端口的帧复制到镜像端口
- **统计计数器**：每端口 15 个 64 位统计计数器

### 3.3 文件结构

```
src/bsw/ecual/ethswt/
├── include/
│   ├── EthSwt.h          # 公共 API 头文件
│   └── EthSwt_Cfg.h      # 预编译配置头文件
└── src/
    └── EthSwt.c           # 模块实现
```

## 4. 状态机

EthSwt 采用简化的两状态模型：

```
    STATE_UNINIT (0x00) ──── Init() ────> STATE_INIT (0x01)
         ^                                    │
         └──────── DeInit() ─────────────────┘
```

端口级别状态：
- **ETHSWT_PORT_DISABLED (0x00)**：端口禁用
- **ETHSWT_PORT_ENABLED (0x01)**：端口使能

链路状态：
- **ETHSWT_LINK_DOWN (0x00)**：链路断开
- **ETHSWT_LINK_UP (0x01)**：链路建立

## 5. 核心数据结构

### 5.1 端口运行时状态

```c
typedef struct {
    EthSwt_PortEnableType        Enable;        /* 端口使能状态 */
    EthSwt_LinkStateType         LinkState;     /* 链路状态 */
    EthSwt_SpeedType             Speed;         /* 端口速率 */
    EthSwt_DuplexType            Duplex;        /* 双工模式 */
    EthSwt_PortStatsType         Stats;         /* 端口统计 */
    EthSwt_MacAddrType           MacAddr;       /* MAC 地址 */
    EthSwt_MacAddrType           FilterMac;     /* 过滤 MAC 地址 */
    boolean                      FilterEnabled; /* 过滤使能 */
    uint16                       Pvid;          /* 端口 VLAN ID */
    EthSwt_FlowControlConfigType FlowControl;   /* 流控配置 */
    uint16                       TxQueueDepth;  /* TX 队列深度 */
    boolean                      PauseActive;   /* TX Pause 激活 */
    boolean                      PauseReceived; /* RX Pause 激活 */
} EthSwt_PortStateType;
```

### 5.2 VLAN 配置

```c
typedef struct {
    uint16  VlanId;          /* VLAN ID (0-4095) */
    uint8   PortMask;        /* 成员端口位掩码 */
    boolean Tagged;          /* 标记/非标记 */
    uint8   VlanPriority;    /* 802.1p PCP (0-7) */
    boolean DropUntagged;    /* 丢弃非标记入站帧 */
} EthSwt_VlanConfigType;
```

### 5.3 端口统计

```c
typedef struct {
    uint64 TxFrames;          /* 发送帧数 */
    uint64 RxFrames;          /* 接收帧数 */
    uint64 TxBytes;           /* 发送字节数 */
    uint64 RxBytes;           /* 接收字节数 */
    uint64 TxErrors;          /* 发送错误 */
    uint64 RxErrors;          /* 接收错误 */
    uint64 Collisions;        /* 冲突数 */
    uint64 DroppedFrames;     /* 丢弃帧数 */
    uint64 RxPauseFrames;     /* 接收 Pause 帧 */
    uint64 TxPauseFrames;     /* 发送 Pause 帧 */
    uint64 RxVlanFrames;      /* 接收 VLAN 帧 */
    uint64 TxVlanFrames;      /* 发送 VLAN 帧 */
    uint64 RxFilteredFrames;  /* 入站过滤丢弃 */
    uint64 TxFilteredFrames;  /* 出站过滤丢弃 */
    uint64 MirroredFrames;    /* 镜像帧数 */
} EthSwt_PortStatsType;
```

### 5.4 内部模块状态

```c
typedef struct {
    uint8                        State;       /* 模块状态 */
    const EthSwt_ConfigType*     ConfigPtr;   /* 配置指针 */
    EthSwt_PortStateType         Ports[8];    /* 端口状态数组 */
    EthSwt_VlanConfigType        Vlans[16];   /* VLAN 表 */
    uint8                        NumVlans;    /* VLAN 数量 */
    EthSwt_MirrorConfigType      Mirror;      /* 镜像配置 */
    uint32                       TickCounter; /* 时钟计数器 */
} EthSwt_InternalStateType;
```

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `EthSwt_Init(ConfigPtr)` | 0x01 | 初始化交换机驱动 | SWS_EthSwt_00001 |
| `EthSwt_DeInit()` | 0x02 | 去初始化 | SWS_EthSwt_00002 |
| `EthSwt_GetVersionInfo(*versioninfo)` | 0x03 | 获取版本信息 | SWS_EthSwt_00003 |
| `EthSwt_SetPortEnable(PortId, Enable)` | 0x10 | 设置端口使能 | SWS_EthSwt_00004 |
| `EthSwt_GetPortEnable(PortId, *Enable)` | 0x19 | 获取端口使能 | SWS_EthSwt_00005 |
| `EthSwt_SetSpeed(PortId, Speed, Duplex)` | 0x11 | 设置端口速率 | SWS_EthSwt_00006 |
| `EthSwt_GetSpeed(PortId, *Speed, *Duplex)` | 0x1A | 获取端口速率 | SWS_EthSwt_00007 |
| `EthSwt_GetLinkState(PortId, *LinkState)` | 0x12 | 获取链路状态 | SWS_EthSwt_00008 |
| `EthSwt_ConfigVlan(*VlanConfig)` | 0x13 | 配置 VLAN（追加） | SWS_EthSwt_00009 |
| `EthSwt_SetVlanConfig(*VlanConfig)` | 0x1C | 设置 VLAN（upsert） | SWS_EthSwt_00010 |
| `EthSwt_GetVlanConfig(VlanId, *VlanConfig)` | 0x1D | 获取 VLAN 配置 | SWS_EthSwt_00011 |
| `EthSwt_AddVlanMember(VlanId, PortId, Tagged)` | 0x1E | 添加 VLAN 成员 | SWS_EthSwt_00012 |
| `EthSwt_RemoveVlanMember(VlanId, PortId)` | 0x1F | 移除 VLAN 成员 | SWS_EthSwt_00013 |
| `EthSwt_SetPvid(PortId, VlanId)` | 0x20 | 设置端口 PVID | SWS_EthSwt_00014 |
| `EthSwt_GetPvid(PortId, *VlanId)` | 0x21 | 获取端口 PVID | SWS_EthSwt_00015 |
| `EthSwt_SetVidPcpMap(VlanId, Pcp)` | 0x22 | 设置 VID-PCP 映射 | SWS_EthSwt_00016 |
| `EthSwt_GetVidPcpMap(VlanId, *Pcp)` | 0x23 | 获取 VID-PCP 映射 | SWS_EthSwt_00017 |
| `EthSwt_ForwardFrame(SrcPort, DstMask, *Data, Len)` | 0x14 | 转发帧 | SWS_EthSwt_00018 |
| `EthSwt_ForwardFrameVlan(SrcPort, VlanId, DstMask, *Data, Len)` | 0x24 | VLAN 帧转发 | SWS_EthSwt_00019 |
| `EthSwt_GetPortStats(PortId, *Stats)` | 0x15 | 获取端口统计 | SWS_EthSwt_00020 |
| `EthSwt_GetStatistics(PortId, *Stats)` | 0x2A | 获取统计（SWS 名称） | SWS_EthSwt_00021 |
| `EthSwt_ResetStatistics(PortId)` | 0x2B | 重置统计 | SWS_EthSwt_00022 |
| `EthSwt_SetMacFilter(PortId, *Mac, Enable)` | 0x16 | 设置 MAC 过滤 | SWS_EthSwt_00023 |
| `EthSwt_GetMacFilter(PortId, *Mac, *Enable)` | 0x1B | 获取 MAC 过滤 | SWS_EthSwt_00024 |
| `EthSwt_SetFlowControl(PortId, *Config)` | 0x25 | 设置流控 | SWS_EthSwt_00025 |
| `EthSwt_GetFlowControl(PortId, *Config)` | 0x26 | 获取流控 | SWS_EthSwt_00026 |
| `EthSwt_SetPauseTime(PortId, PauseTime)` | 0x27 | 设置 Pause 时间 | SWS_EthSwt_00027 |
| `EthSwt_GetPauseTime(PortId, *PauseTime)` | 0x28 | 获取 Pause 时间 | SWS_EthSwt_00028 |
| `EthSwt_IndicatePause(PortId, Pause)` | 0x29 | Pause 指示 | SWS_EthSwt_00029 |
| `EthSwt_SetPortMirroring(*MirrorConfig)` | 0x2C | 设置端口镜像 | SWS_EthSwt_00030 |
| `EthSwt_GetPortMirroring(*MirrorConfig)` | 0x2D | 获取端口镜像 | SWS_EthSwt_00031 |
| `EthSwt_MainFunction()` | 0x17 | 周期处理函数 | SWS_EthSwt_00032 |
| `EthSwt_Reset()` | 0x18 | 重置交换机 | SWS_EthSwt_00033 |

### 6.2 回调函数

本模块无回调函数定义。

### 6.3 服务 ID 与错误码

**DET 错误码：**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| ETHSWT_E_PARAM_POINTER | 0x01 | 空指针 |
| ETHSWT_E_PARAM_CONFIG | 0x02 | 无效配置 |
| ETHSWT_E_UNINIT | 0x03 | 未初始化 |
| ETHSWT_E_ALREADY_INITIALIZED | 0x04 | 重复初始化 |
| ETHSWT_E_INVALID_PORT | 0x05 | 无效端口 |
| ETHSWT_E_INVALID_SPEED | 0x06 | 无效速率 |
| ETHSWT_E_INVALID_VLAN | 0x07 | 无效 VLAN |
| ETHSWT_E_INVALID_MAC | 0x08 | 无效 MAC |
| ETHSWT_E_PORT_DISABLED | 0x09 | 端口已禁用 |
| ETHSWT_E_BUFFER_FULL | 0x0A | 缓冲区满 |
| ETHSWT_E_INIT_FAILED | 0x0B | 初始化失败 |
| ETHSWT_E_NOT_SUPPORTED | 0x0C | 不支持 |
| ETHSWT_E_INVALID_PCP | 0x0D | 无效 PCP |
| ETHSWT_E_VLAN_NOT_FOUND | 0x0E | VLAN 未找到 |
| ETHSWT_E_VLAN_FULL | 0x0F | VLAN 表满 |
| ETHSWT_E_INVALID_WATERMARK | 0x10 | 无效水位线 |
| ETHSWT_E_MIRROR_INVALID | 0x11 | 无效镜像配置 |
| ETHSWT_E_INVALID_PAUSE | 0x12 | 无效 Pause 参数 |

## 7. 处理流程

### 7.1 帧转发流程（含 VLAN 过滤）

```
ForwardFrame(SrcPort, DstPortMask, FrameData, Length)
  │
  ├── VlanId = ETHSWT_VLAN_NONE (非标记帧)
  │     └── effectiveVlan = SrcPort.Pvid
  │
  ├── 入站过滤: SrcPort 必须是 VLAN 成员
  │     └── 非成员 → RxFilteredFrames++ → 丢弃
  │
  ├── DropUntagged 检查: 非标记帧 + DropUntagged=TRUE → 丢弃
  │
  ├── TX 统计 + 流控检查
  │     └── TxQueueDepth >= HighWatermark → PauseActive=TRUE
  │
  ├── 镜像: 复制到 MirrorDestinationPort
  │
  └── 出站过滤: 遍历 DstPortMask
        └── 每端口检查 VLAN 成员资格
              └── 非成员 → TxFilteredFrames++ → 跳过
```

### 7.2 MainFunction 周期处理

每 10ms 调用一次：
1. 链路状态轮询（每 100ms 一次，通过 TickCounter 分频）
2. TX 队列排空（TxQueueDepth 递减）
3. Pause 释放检查（TxQueueDepth <= LowWatermark 时释放 Pause）

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| ETHSWT_DEV_ERROR_DETECT | STD_ON | DET 使能 |
| ETHSWT_VERSION_INFO_API | STD_ON | 版本信息 API |
| ETHSWT_MAX_PORTS | 8 | 最大端口数 |
| ETHSWT_MAX_VLANS | 16 | 最大 VLAN 数 |
| ETHSWT_MAX_MAC_FILTERS | 32 | 最大 MAC 过滤数 |
| ETHSWT_ENABLE_VLAN | STD_ON | VLAN 功能使能 |
| ETHSWT_ENABLE_FLOW_CONTROL | STD_ON | 流控使能 |
| ETHSWT_ENABLE_MIRRORING | STD_ON | 镜像使能 |
| ETHSWT_LINK_POLL_INTERVAL_MS | 100 | 链路轮询间隔 |
| ETHSWT_MAIN_FUNCTION_PERIOD_MS | 10 | 主函数周期 |
| ETHSWT_DEFAULT_HIGH_WATERMARK | 32 | 默认高水位线 |
| ETHSWT_DEFAULT_LOW_WATERMARK | 8 | 默认低水位线 |
| ETHSWT_DEFAULT_PAUSE_TIME | 512 | 默认 Pause 时间 |

### 8.2 链接时配置

通过 `EthSwt_ConfigType` 在初始化时传入，包含端口配置、VLAN 配置、流控配置和镜像配置。

### 8.3 构建后配置

本实现不支持构建后配置。

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 均进行：
- 初始化状态检查（ETHSWT_E_UNINIT）
- 端口有效性检查（ETHSWT_E_INVALID_PORT）
- 指针有效性检查（ETHSWT_E_PARAM_POINTER）
- VLAN PCP 范围检查（0-7）
- 水位线有效性检查（High > Low）

### 9.2 DEM 错误

本实现未集成 DEM 事件上报。

### 9.3 安全机制

- **端口禁用时自动清除链路状态**：SetPortEnable(DISABLED) 同时清除 LinkState
- **VLAN 配置净化**：初始化时自动修正无效的 PCP 值和布尔字段
- **ETHSWT_ALL_PORTS (255)**：支持 ResetStatistics 一次重置所有端口

## 10. 内存与性能

### 10.1 MemMap 分区

本实现未使用 MemMap 分区宏。

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| Ports[8] | ~800 bytes | 每端口含 15×8=120 bytes 统计 |
| Vlans[16] | ~96 bytes | 每 VLAN 6 bytes |
| Mirror | ~3 bytes | 镜像配置 |
| TickCounter | 4 bytes | 时钟计数器 |
| **总计 RAM** | **~900 bytes** | 运行时状态 |

## 11. 集成指南

### 集成步骤

1. 在 `EthSwt_Cfg.h` 中配置端口数、VLAN 数等参数
2. 准备 `EthSwt_ConfigType` 配置结构体（端口、VLAN、流控、镜像）
3. 调用 `EthSwt_Init(&config)` 初始化
4. 将 `EthSwt_MainFunction()` 加入 10ms 周期任务
5. 使用 `EthSwt_ForwardFrame()` / `EthSwt_ForwardFrameVlan()` 进行帧转发

### 注意事项

- 帧转发时，非标记帧使用源端口 PVID 进行 VLAN 过滤
- 未知 VLAN（不在成员表中）的帧不进行过滤（兼容行为）
- 端口禁用或 Pause 激活时，帧将被丢弃并计入 DroppedFrames

## 12. 测试策略

### 12.1 单元测试

| 测试场景 | 预期结果 |
|----------|----------|
| Init 正常初始化 | 所有端口禁用，VLAN 表加载 |
| SetPortEnable + GetPortEnable | 端口状态正确切换 |
| VLAN 配置和查询 | ConfigVlan / SetVlanConfig / GetVlanConfig 一致 |
| AddVlanMember / RemoveVlanMember | PortMask 正确更新 |
| ForwardFrame 入站过滤 | 非成员端口帧被过滤 |
| ForwardFrame 出站过滤 | 非成员目的端口被跳过 |
| DropUntagged 功能 | 非标记帧被丢弃 |
| 流控 Pause 触发 | TxQueueDepth >= HighWatermark 时 PauseActive |
| 流控 Pause 释放 | TxQueueDepth <= LowWatermark 时释放 |
| 端口镜像 | 源端口帧复制到镜像端口 |
| ResetStatistics(ALL_PORTS) | 所有端口统计清零 |

### 12.2 集成测试

| 测试场景 | 验证点 |
|----------|--------|
| EthSwt + Eth 联合测试 | 帧收发正确 |
| 多 VLAN 隔离 | 不同 VLAN 间帧不互通 |
| 流控压力测试 | 高负载下 Pause 帧正确生成 |
| 镜像调试测试 | 镜像端口捕获所有指定流量 |

## 13. 实现说明 / TODO

### 当前实现特点

- 完整的软件仿真交换机实现
- 8 端口 + 16 VLAN 的完整管理
- 入站/出站 VLAN 成员过滤
- 基于水位线的流控机制
- 端口镜像功能
- 15 项端口统计计数器

### 待实现项

- [ ] 硬件交换机 ASIC 寄存器操作
- [ ] 实际 PHY 链路状态读取
- [ ] DEM 事件上报
- [ ] MemMap 内存分区
- [ ] 多播/广播风暴保护
- [ ] 端口速率自协商实际实现

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| AUTOSAR_SWS_EthernetSwitchDriver.pdf | AUTOSAR EthSwt 规范 |
| IEEE 802.1Q-2018 | VLAN 标记标准 |
| IEEE 802.3x-1997 | 流控标准 |
| EthSwt.h | 模块公共接口定义 |
| EthSwt_Cfg.h | 预编译配置定义 |
| EthSwt.c | 模块实现源码 |
