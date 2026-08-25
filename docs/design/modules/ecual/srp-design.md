# Srp Design Document

> **Module ID**: 0xA3  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Srp  
> **Source Path**: `src/bsw/ecual/srp/`  
> **Reference Document**: `docs/modules/srp.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Srp（Stream Reservation Protocol）模块实现了 IEEE 802.1Qat 标准的流预留协议，用于在以太网上为时间敏感流（Time-Sensitive Networking, TSN）预留带宽资源。该模块位于 ECUAL 层，为上层 SOME/IP 传输和其他时间敏感应用提供带宽保障。

主要职责：
- 流注册管理（Talker/Listener）
- SRP 帧的接收与解析
- 流预留状态管理（IDLE → REGISTERED → READY）
- 流注销与资源释放
- 流状态查询

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| IEEE 802.1Qat | 2009 | 流预留协议（SRP） |
| IEEE 802.1Q | 2018 | VLAN 桥接标准（含 SRP） |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SomeIp / SomeIpTp | SOME/IP 传输层 |
| 下层 | EthIf / EthSwT | 以太网接口/交换机 |
| 公共 | Det | 开发错误追踪 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│       SomeIp / SomeIpTp             │
├─────────────────────────────────────┤
│       Srp (ECUAL Layer)             │
├─────────────────────────────────────┤
│       EthIf / EthSwT (ECUAL)        │
├─────────────────────────────────────┤
│       MCAL (Eth)                    │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **流注册表（Stream Registry）**：维护所有已注册流的条目
- **Talker 管理器（Talker Manager）**：管理 Talker 角色的流注册
- **Listener 管理器（Listener Manager）**：管理 Listener 角色的流注册
- **SRP 帧处理器（SRP Frame Handler）**：解析接收到的 SRP 帧

### 3.3 文件结构

```
src/bsw/ecual/srp/
├── include/
│   ├── Srp.h
│   └── Srp_Cfg.h
└── src/
    ├── Srp.c
    └── Srp_Lcfg.c
```

---

## 4. 状态机

流预留状态机：

```
[SRP_STATE_IDLE]
    │ RegisterTalker / RegisterListener
    ▼
[SRP_STATE_REGISTERED]
    │ MainFunction (state transition)
    ▼
[SRP_STATE_READY]
    │ DeregisterStream / error
    ▼
[SRP_STATE_FAILED]
```

---

## 5. 核心数据结构

```c
/* 流 ID 类型（8 字节） */
typedef uint8 Srp_StreamIdType[8];

/* Talker 通告信息 */
typedef struct {
    Srp_StreamIdType StreamId;
    uint8 DataFrameParameters[20];
    uint8 TSpec[12];
    uint8 PriorityAndRank;
    uint16 AccumulatedLatency;
} Srp_TalkerAdvertiseType;

/* 流配置 */
typedef struct {
    Srp_StreamIdType StreamId;
    uint16 StreamVlanId;
    uint8 Priority;
    uint16 FrameSize;
    uint16 IntervalFrames;
    Srp_ReservationTypeType Role;
} Srp_StreamConfigType;

/* 内部流条目 */
typedef struct {
    Srp_StreamIdType          StreamId;
    Srp_ReservationStateType  State;
    Srp_ReservationTypeType   Role;
    uint16                    VlanId;
    uint8                     Priority;
    uint16                    FrameSize;
    uint16                    IntervalFrames;
    uint32                    AccumulatedLatency;
    uint32                    TTL;
} Srp_StreamEntryType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| Srp_Init | `void Srp_Init(const void* ConfigPtr)` | 初始化 | | SWS_Srp_00001 |
| Srp_DeInit | `void Srp_DeInit(void)` | 反初始化 | | SWS_Srp_00002 |
| Srp_RegisterTalker | `Std_ReturnType Srp_RegisterTalker(const Srp_TalkerAdvertiseType*)` | 注册 Talker | | SWS_Srp_00005 |
| Srp_RegisterListener | `Std_ReturnType Srp_RegisterListener(const Srp_StreamIdType)` | 注册 Listener | | SWS_Srp_00006 |
| Srp_DeregisterStream | `Std_ReturnType Srp_DeregisterStream(const Srp_StreamIdType)` | 注销流 | | SWS_Srp_00007 |
| Srp_GetStreamStatus | `Std_ReturnType Srp_GetStreamStatus(const Srp_StreamIdType, Srp_ReservationStateType*)` | 查询状态 | | SWS_Srp_00008 |
| Srp_RxIndication | `void Srp_RxIndication(const uint8* DataPtr, uint16 Length)` | 接收指示 | | SWS_Srp_00009 |
| Srp_MainFunction | `void Srp_MainFunction(void)` | 周期处理 | 状态转换 | SWS_Srp_00004 |
| Srp_GetVersionInfo | `void Srp_GetVersionInfo(Std_VersionInfoType*)` | 版本信息 | | SWS_Srp_00003 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| Srp_RxIndication | 下层以太网接收到 SRP 帧后调用 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | Init | — |
| 0x01 | DeInit | — |
| 0x02 | RegisterTalker | SRP_E_PARAM_POINTER, SRP_E_UNINIT, SRP_E_NO_RESOURCES |
| 0x03 | RegisterListener | SRP_E_UNINIT, SRP_E_NO_RESOURCES |
| 0x04 | DeregisterStream | SRP_E_UNINIT, SRP_E_PARAM_STREAM |
| 0x05 | RxIndication | — |
| 0x06 | MainFunction | — |
| 0x07 | GetStreamStatus | SRP_E_PARAM_POINTER, SRP_E_PARAM_STREAM |

---

## 7. 处理流程

### 7.1 Talker 注册流程

1. 调用 `Srp_RegisterTalker`，检查初始化状态和参数
2. 通过 `Srp_FindStream` 查找是否已存在
3. 若已存在则更新状态为 REGISTERED
4. 否则分配新条目（若未超过 SRP_MAX_STREAMS）
5. 填充 StreamId、Role=TALKER、Priority、AccumulatedLatency

### 7.2 状态转换流程（MainFunction）

1. 遍历所有已注册流
2. 对 REGISTERED 状态的流转为 READY
3. 表示带宽预留已完成

### 7.3 SRP 帧接收流程

1. `Srp_RxIndication` 接收原始 SRP 帧
2. 验证帧长度 >= 8 字节（SRP 头部长度）
3. 解析子类型（Talker Advertise / Listener Ready）
4. 更新对应流条目状态

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| SRP_DEV_ERROR_DETECT | STD_ON | 开发错误检测 |
| SRP_MAX_STREAMS | 16 | 最大流数 |
| SRP_STREAM_ID_SIZE | 8 | 流 ID 大小（字节） |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| Srp_Cfg.h | 预编译配置参数 |
| Srp_Lcfg.c | 链接时配置数据 |

### 8.3 构建后配置

不适用。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x10 | SRP_E_PARAM_POINTER | 空指针入参 |
| 0x20 | SRP_E_UNINIT | 模块未初始化 |
| 0x30 | SRP_E_PARAM_STREAM | 无效流 ID |
| 0x40 | SRP_E_NO_RESOURCES | 无空闲流条目 |

### 9.2 DEM 错误

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| — | — | 当前未定义 DEM 事件 |

### 9.3 安全机制

- 初始化状态检查
- 流数量上限保护（SRP_MAX_STREAMS = 16）
- 帧长度验证

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| 默认代码段 | Srp.c 全部函数 |
| 默认数据段 | 内部状态结构体 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~640 bytes | 16 流 × 40 bytes/流 |
| ROM | ~2 KB | 代码段 |
| 堆栈 | ~128 bytes | 函数调用栈 |

---

## 11. 集成指南

- 与上层集成：SomeIpTp 通过 `Srp_RegisterTalker`/`Srp_RegisterListener` 预留带宽
- 与下层集成：依赖 EthIf/EthSwT 发送/接收 SRP 帧
- 初始化顺序：EthIf → Det → Srp_Init
- MainFunction 周期建议：100ms

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| test_srp.c | 初始化/反初始化、Talker/Listener 注册、注销、状态转换、帧接收、流数量上限 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 带宽预留 | Talker 注册 → READY → 数据传输 |
| 流注销 | 验证资源正确释放 |
| SRP 帧交互 | 两个节点间的 SRP 帧交换 |

---

## 13. 实现说明 / TODO

- 基本流注册/注销功能已实现
- `Srp_MainFunction` 中 REGISTERED → READY 状态转换已实现
- `Srp_RxIndication` 仅解析帧子类型，需要完善完整的 SRP 消息处理
- 需要实现 SRP 帧的发送功能（Talker Advertise / Listener Ready）
- 需要实现 TTL 过期管理
- 需要实现与 EthSwT 的 VLAN 配置集成
- 编译时版本检查已实现

---

## 14. 参考资料

1. IEEE 802.1Qat-2009 Standard
2. AUTOSAR_SWS_Srp.pdf
3. `docs/modules/srp.md`
4. `src/bsw/ecual/srp/`
