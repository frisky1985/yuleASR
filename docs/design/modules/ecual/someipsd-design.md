# SomeIpSd Design Document

> **Module ID**: 0xA5  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_PRS_SOMEIPServiceDiscoveryProtocol  
> **Source Path**: `src/bsw/ecual/someipsd/`  
> **Reference Document**: `docs/modules/someipsd.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

SomeIpSd（SOME/IP Service Discovery）模块是 AUTOSAR ECUAL 层的组件，实现了 SOME/IP 服务发现协议。该模块在 SOME/IP 报文基础上提供服务的发现（Find）、提供（Offer）、停止提供（StopOffer）和事件组订阅（Subscribe）功能，是 SOME/IP 通信中服务生命周期管理的核心。

主要职责：
- 服务发现（FindService）
- 服务提供（OfferService / StopOffer）
- 事件组订阅（SubscribeEventGroup）
- SD 消息构建与解析
- 服务状态管理（DOWN / AVAILABLE / NOT_AVAILABLE）
- 订阅状态管理（NOT_REQUESTED / PENDING / ACKNOWLEDGED / REJECTED）
- TTL 生命周期管理
- 周期性 Offer 消息发送

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR PRS SOMEIPServiceDiscoveryProtocol | 4.4.0 | SOME/IP SD 协议规范 |
| SOME/IP Service Discovery Specification | 1.x | SOME/IP SD 协议 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SomeIp / SomeIpIf | SOME/IP 通信接口 |
| 下层 | SomeIpIf / SoAd | SOME/IP 接口 / Socket 适配器 |
| 公共 | Det | 开发错误追踪 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│       SomeIp (Services)             │
├─────────────────────────────────────┤
│    SomeIpSd (ECUAL Layer)           │
├─────────────────────────────────────┤
│    SomeIpIf / SoAd (ECUAL)          │
├─────────────────────────────────────┤
│       TcpIp / MCAL (Eth)            │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **服务注册表（Service Registry）**：维护所有已知服务的状态
- **SD 消息构建器（SD Message Builder）**：构建 Find/Offer/Subscribe 等 SD 消息
- **SD 消息解析器（SD Message Parser）**：解析接收到的 SD 消息
- **TTL 管理器（TTL Manager）**：管理服务条目的生命周期
- **会话管理器（Session Manager）**：管理 SD 会话 ID

### 3.3 文件结构

```
src/bsw/ecual/someipsd/
├── include/
│   ├── SomeIpSd.h
│   └── SomeIpSd_Cfg.h
└── src/
    ├── SomeIpSd.c
    └── SomeIpSd_Lcfg.c
```

---

## 4. 状态机

服务状态机：

```
[SD_STATE_DOWN]
    │ OfferService / 收到 Offer
    ▼
[SD_STATE_AVAILABLE]
    │ StopOffer / TTL 过期
    ▼
[SD_STATE_NOT_AVAILABLE]
```

订阅状态机：

```
[SD_SUBSCRIPTION_NOT_REQUESTED]
    │ SubscribeEventGroup
    ▼
[SD_SUBSCRIPTION_PENDING]
    │ 收到 SubscribeAck
    ▼
[SD_SUBSCRIPTION_ACKNOWLEDGED]
    │ 收到 SubscribeNack / TTL 过期
    ▼
[SD_SUBSCRIPTION_REJECTED]
```

---

## 5. 核心数据结构

```c
/* SD 条目类型 */
typedef enum {
    SD_ENTRY_FIND_SERVICE = 0x00,
    SD_ENTRY_OFFER_SERVICE = 0x01,
    SD_ENTRY_SUBSCRIBE_EVENTGROUP = 0x06,
    SD_ENTRY_SUBSCRIBE_ACK = 0x07
} SomeIpSd_EntryTypeType;

/* SD 条目 */
typedef struct {
    SomeIpSd_EntryTypeType Type;
    uint16 ServiceId;
    uint16 InstanceId;
    uint8 MajorVersion;
    uint32 MinorVersion;
    uint32 TTL;
} SomeIpSd_EntryType;

/* 服务配置 */
typedef struct {
    uint16 ServiceId;
    uint16 InstanceId;
    uint32 TTL;
    boolean IsServer;
    uint16 EndpointTcp;
    uint16 EndpointUdp;
    uint8  MajorVersion;
    uint32 MinorVersion;
} SomeIpSd_ServiceConfigType;

/* 内部服务条目 */
typedef struct {
    uint16         ServiceId;
    uint16         InstanceId;
    uint8          MajorVersion;
    uint32         MinorVersion;
    uint32         TTL;
    uint32         RemainingTTL;
    SomeIpSd_ServiceStateType State;
    SomeIpSd_SubscriptionStateType SubState;
    boolean        IsServer;
} Sd_ServiceEntryType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| SomeIpSd_Init | `void SomeIpSd_Init(const void* ConfigPtr)` | 初始化 | | SWS_SomeIpSd_00001 |
| SomeIpSd_DeInit | `void SomeIpSd_DeInit(void)` | 反初始化 | | SWS_SomeIpSd_00002 |
| SomeIpSd_FindService | `Std_ReturnType SomeIpSd_FindService(uint16 ServiceId, uint16 InstanceId)` | 发现服务 | 发送 Find 消息 | SWS_SomeIpSd_00005 |
| SomeIpSd_OfferService | `Std_ReturnType SomeIpSd_OfferService(uint16 ServiceId, uint16 InstanceId)` | 提供服务 | 发送 Offer 消息 | SWS_SomeIpSd_00006 |
| SomeIpSd_StopOffer | `Std_ReturnType SomeIpSd_StopOffer(uint16 ServiceId, uint16 InstanceId)` | 停止提供 | | SWS_SomeIpSd_00007 |
| SomeIpSd_SubscribeEventGroup | `Std_ReturnType SomeIpSd_SubscribeEventGroup(uint16 ServiceId, uint16 EventGroupId)` | 订阅事件组 | | SWS_SomeIpSd_00008 |
| SomeIpSd_GetServiceState | `SomeIpSd_ServiceStateType SomeIpSd_GetServiceState(uint16 ServiceId, uint16 InstanceId)` | 查询状态 | |  |
| SomeIpSd_RxIndication | `void SomeIpSd_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | 接收指示 | | SWS_SomeIpSd_00009 |
| SomeIpSd_MainFunction | `void SomeIpSd_MainFunction(void)` | 周期处理 | TTL 管理、周期 Offer | SWS_SomeIpSd_00004 |
| SomeIpSd_GetVersionInfo | `void SomeIpSd_GetVersionInfo(Std_VersionInfoType*)` | 版本信息 | | SWS_SomeIpSd_00003 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| SomeIpSd_RxIndication | 下层接收到 SD 消息后调用 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | Init | SD_E_PARAM_POINTER |
| 0x01 | DeInit | — |
| 0x02 | FindService | SD_E_UNINIT, SD_E_NOT_FOUND |
| 0x03 | OfferService | SD_E_UNINIT |
| 0x04 | StopOffer | SD_E_UNINIT, SD_E_NOT_FOUND |
| 0x05 | SubscribeEventGroup | SD_E_UNINIT, SD_E_NOT_FOUND |
| 0x06 | RxIndication | — |
| 0x07 | MainFunction | — |
| 0x08 | GetServiceState | — |

---

## 7. 处理流程

### 7.1 FindService 流程

1. 调用 `SomeIpSd_FindService`，检查初始化状态
2. 查找服务是否已知（已存在于注册表）
3. 若未知则分配新条目，状态设为 SD_STATE_DOWN
4. 构建 SD Find 消息（`Sd_BuildHeader` + Find Entry）
5. 通过 SoAd 发送 Find 消息

### 7.2 OfferService 流程

1. 调用 `SomeIpSd_OfferService`
2. 查找或创建服务条目
3. 设置 IsServer=TRUE、State=AVAILABLE、TTL=3000
4. 构建 SD Offer 消息并发送

### 7.3 接收处理流程

1. `SomeIpSd_RxIndication` 接收 SD 消息
2. 验证消息长度 >= 头部 + 条目长度（16 + 16 = 32 字节）
3. 解析 Message Type、Entry Type、ServiceId、InstanceId
4. 若为 OfferService 消息，更新对应服务状态为 AVAILABLE

### 7.4 MainFunction 周期处理

1. 递增 tickCounter
2. 遍历所有服务条目：
   - AVAILABLE 状态的服务递减 RemainingTTL
   - TTL 归零时转为 NOT_AVAILABLE
   - Server 服务每 100 tick 周期重新发送 Offer

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| SD_MAX_SERVICES | 16 | 最大服务数 |
| SD_DEFAULT_TTL | 3000 | 默认 TTL（tick 单位） |
| SD_PROTOCOL_VERSION | 0x01 | SD 协议版本 |
| SD_INTERFACE_VERSION | 0x01 | SD 接口版本 |
| SD_SOMEIP_HEADER_LEN | 16 | SOME/IP 头部长度 |
| SD_ENTRY_LEN | 16 | SD 条目长度 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| SomeIpSd_Cfg.h | 预编译配置参数 |
| SomeIpSd_Lcfg.c | 服务配置数据 |

### 8.3 构建后配置

不适用。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x10 | SD_E_PARAM_POINTER | 空指针入参 |
| 0x20 | SD_E_UNINIT | 模块未初始化 |
| 0x30 | SD_E_PARAM_SERVICE | 无效服务参数 |
| 0x40 | SD_E_NOT_FOUND | 服务未找到 |

### 9.2 DEM 错误

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| — | — | 当前未定义 DEM 事件 |

### 9.3 安全机制

- 初始化状态检查
- 服务数量上限保护（SD_MAX_SERVICES = 16）
- 消息长度验证
- TTL 过期自动失效

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| 默认代码段 | SomeIpSd.c 全部函数 |
| 默认数据段 | 内部状态结构体 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~640 bytes | 16 服务 × 40 bytes/服务 |
| ROM | ~3 KB | 代码段 |
| 堆栈 | ~256 bytes | 函数调用栈 + 消息构建 |

---

## 11. 集成指南

- 与上层集成：SomeIp 模块通过 `SomeIpSd_FindService`/`SomeIpSd_OfferService` 管理服务
- 与下层集成：通过 SomeIpIf/SoAd 发送/接收 SD 消息
- 初始化顺序：TcpIp → SoAd → SomeIpIf → Det → SomeIpSd_Init
- MainFunction 周期建议：10-100ms
- SD 消息使用 SOME/IP Service ID = 0xFFFF, Method ID = 0x8100

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| test_someipsd.c | 初始化/反初始化、Find/Offer/StopOffer、订阅管理、TTL 过期、消息构建、接收解析 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 服务发现 | 两个节点间的 Find/Offer 交互 |
| 订阅流程 | Subscribe → Ack → 事件接收 |
| TTL 过期 | 验证服务在 TTL 过期后自动失效 |
| 周期 Offer | 验证 Server 服务周期性发送 Offer |

---

## 13. 实现说明 / TODO

- Find/Offer/StopOffer 基本功能已实现
- SD 消息头部构建已实现（`Sd_BuildHeader`）
- 接收处理已实现基本的 Offer 消息解析
- TTL 管理和周期 Offer 已在 MainFunction 中实现
- SubscribeEventGroup 仅设置状态为 PENDING，需要实现完整的订阅消息发送
- 需要添加 SubscribeAck/SubscribeNack 的接收处理
- 需要添加 Options 处理（IPv4 端点选项）
- 需要实现与 SomeIpIf 的更深层集成

---

## 14. 参考资料

1. AUTOSAR_PRS_SOMEIPServiceDiscoveryProtocol.pdf
2. SOME/IP Service Discovery Specification
3. `docs/modules/someipsd.md`
4. `src/bsw/ecual/someipsd/`
