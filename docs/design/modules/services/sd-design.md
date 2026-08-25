# Sd Design Document

> **Module ID**: 0x93  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_ServiceDiscovery  
> **Source Path**: `src/bsw/services/sd/`  
> **Reference Document**: `docs/modules/sd.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Sd（Service Discovery）模块是 AUTOSAR 服务层的核心组件，基于 SOME/IP 协议提供服务的发现、提供与订阅管理功能。该模块维护本地服务注册表，处理 SD 消息的序列化与反序列化，支持 Find/Offer/Subscribe 工作流。

主要职责：
- 服务提供管理（Offer/StopOffer）
- 服务发现管理（FindService）
- 事件组订阅管理（Subscribe/Unsubscribe EventGroup）
- SD 消息序列化/反序列化
- TTL 生命周期管理
- 周期性 Offer/Find 消息发送

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS ServiceDiscovery | 4.4.0 | 服务发现模块规范 |
| AUTOSAR PRS SOMEIPServiceDiscoveryProtocol | 4.4.0 | SOME/IP SD 协议规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SomeIp | SOME/IP 通信模块 |
| 上层 | ASWC | 应用层软件组件 |
| 下层 | SoAd | Socket 适配器 |
| 公共 | Det | 开发错误追踪 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        Application Layer            │
├─────────────────────────────────────┤
│           SomeIp                    │
├─────────────────────────────────────┤
│        Sd (Services Layer)          │
├─────────────────────────────────────┤
│            SoAd                     │
├─────────────────────────────────────┤
│          TcpIp / MCAL               │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **服务注册表（Offered Services Registry）**：管理本地提供的服务实例
- **发现注册表（Found Services Registry）**：管理远程发现的服务实例
- **订阅管理器（Subscription Manager）**：管理事件组订阅状态
- **SD 消息处理器（SD Message Handler）**：处理 SD 消息的序列化/反序列化
- **生命周期管理器（Lifetime Manager）**：管理 TTL 过期与续约

### 3.3 文件结构

```
src/bsw/services/sd/
├── include/
│   ├── Sd.h
│   └── Sd_Cfg.h
└── src/
    └── Sd.c
```

---

## 4. 状态机

模块级状态机：

```
[SD_STATE_UNINIT (0x00)]
    │ Sd_Init
    ▼
[SD_STATE_INIT (0x01)]
    │ Sd_DeInit
    ▼
[SD_STATE_UNINIT (0x00)]
```

服务实例状态：

```
SD_SERVICE_STATUS_NOT_OFFERED (0x00)
    │ OfferService
    ▼
SD_SERVICE_STATUS_OFFERED (0x01)
    │ discovered by client
    ▼
SD_SERVICE_STATUS_AVAILABLE (0x02)
```

订阅状态：

```
SD_SUBSCRIBER_NOT_SUBSCRIBED (0x00)
    │ SubscribeEventGroup
    ▼
SD_SUBSCRIBER_SUBSCRIBE_PENDING (0x02)
    │ SetEventStatus(READY)
    ▼
SD_SUBSCRIBER_SUBSCRIBED (0x01)
```

---

## 5. 核心数据结构

```c
/* IPv4 端点 */
typedef struct {
    uint32              Addr;
    uint16              Port;
    Sd_ProtocolType     Protocol;
} Sd_Ipv4EndpointType;

/* 服务实例 */
typedef struct {
    Sd_ServiceIdType    ServiceId;
    Sd_InstanceIdType   InstanceId;
} Sd_ServiceInstanceType;

/* 服务条目 */
typedef struct {
    Sd_ServiceInstanceType  Service;
    Sd_MajorVersionType     MajorVersion;
    Sd_MinorVersionType     MinorVersion;
    Sd_TtlType              Ttl;
    Sd_Ipv4EndpointType     Endpoint;
    Sd_ServiceStatusType    Status;
    uint32                  RemainingLifetimeMs;
} Sd_ServiceEntryType;

/* 事件组条目 */
typedef struct {
    Sd_ServiceInstanceType  Service;
    Sd_EventGroupIdType     EventGroupId;
    Sd_SubscriberStatusType SubscriberStatus;
    Sd_EventGroupStatusType EventStatus;
} Sd_EventGroupEntryType;

/* 全局配置 */
typedef struct {
    uint8   MaxServices;
    uint8   MaxSubscriptions;
    uint32  OfferCycleTimeMs;
    uint32  FindCycleTimeMs;
    uint32  TtlDefault;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} Sd_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| Sd_Init | `void Sd_Init(const Sd_ConfigType* ConfigPtr)` | 初始化 | | SWS_Sd_00001 |
| Sd_DeInit | `void Sd_DeInit(void)` | 反初始化 | | SWS_Sd_00002 |
| Sd_GetVersionInfo | `void Sd_GetVersionInfo(Std_VersionInfoType*)` | 版本信息 | 条件编译 | SWS_Sd_00003 |
| Sd_FindService | `Std_ReturnType Sd_FindService(Sd_ServiceIdType, Sd_InstanceIdType, Sd_Ipv4EndpointType*)` | 查找服务 | | SWS_Sd_00005 |
| Sd_OfferService | `Std_ReturnType Sd_OfferService(Sd_ServiceIdType, Sd_InstanceIdType, Sd_MajorVersionType, Sd_MinorVersionType, const Sd_Ipv4EndpointType*)` | 提供服务 | | SWS_Sd_00006 |
| Sd_StopService | `Std_ReturnType Sd_StopService(Sd_ServiceIdType, Sd_InstanceIdType)` | 停止提供 | | SWS_Sd_00007 |
| Sd_SubscribeEventGroup | `Std_ReturnType Sd_SubscribeEventGroup(Sd_ServiceIdType, Sd_InstanceIdType, Sd_EventGroupIdType)` | 订阅事件组 | | SWS_Sd_00008 |
| Sd_UnsubscribeEventGroup | `Std_ReturnType Sd_UnsubscribeEventGroup(Sd_ServiceIdType, Sd_InstanceIdType, Sd_EventGroupIdType)` | 取消订阅 | | SWS_Sd_00009 |
| Sd_SetEventStatus | `Std_ReturnType Sd_SetEventStatus(Sd_ServiceIdType, Sd_InstanceIdType, Sd_EventGroupIdType, Sd_EventGroupStatusType)` | 设置事件状态 | | SWS_Sd_00010 |
| Sd_MainFunction | `void Sd_MainFunction(void)` | 周期处理 | TTL 管理、周期消息 | SWS_Sd_00004 |
| Sd_HandleMessage | `Std_ReturnType Sd_HandleMessage(const uint8* Data, uint16 Length)` | 处理 SD 消息 | | SWS_Sd_00011 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| Sd_HandleMessage | 下层 SOME/IP 接收到 SD 消息后调用 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x01 | Init | SD_E_PARAM_POINTER, SD_E_ALREADY_INITIALIZED |
| 0x02 | DeInit | SD_E_UNINIT |
| 0x03 | GetVersionInfo | SD_E_PARAM_POINTER |
| 0x10 | FindService | SD_E_UNINIT, SD_E_PARAM_POINTER |
| 0x11 | OfferService | SD_E_UNINIT, SD_E_PARAM_POINTER |
| 0x12 | StopService | SD_E_UNINIT |
| 0x13 | SubscribeEventGroup | SD_E_UNINIT |
| 0x14 | UnsubscribeEventGroup | SD_E_UNINIT |
| 0x15 | SetEventStatus | SD_E_UNINIT |
| 0x17 | MainFunction | — |
| 0x18 | HandleMessage | SD_E_UNINIT |

---

## 7. 处理流程

### 7.1 Offer 流程

1. 调用 `Sd_OfferService`，检查是否已存在
2. 若已存在则更新版本和 TTL；否则分配新条目
3. 设置状态为 `SD_SERVICE_STATUS_OFFERED`
4. `MainFunction` 周期性发送 OfferService SD 消息

### 7.2 Find 流程

1. 调用 `Sd_FindService` 在已发现服务注册表中查找
2. 若找到则返回端点信息（E_OK）
3. 若未找到返回 E_NOT_OK
4. `MainFunction` 周期性发送 FindService SD 消息

### 7.3 TTL 生命周期管理

1. `MainFunction` 每个周期递减 `RemainingLifetimeMs`
2. 当 TTL 过期时，从已发现服务数组中移除条目（数组压缩）
3. 周期 Offer 消息实现 TTL 续约

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| SD_DEV_ERROR_DETECT | STD_ON | 开发错误检测 |
| SD_VERSION_INFO_API | STD_ON | 版本信息 API |
| SD_MAX_OFFERED_SERVICES | 配置 | 最大提供服务数 |
| SD_MAX_FOUND_SERVICES | 配置 | 最大发现服务数 |
| SD_MAX_SUBSCRIPTIONS | 配置 | 最大订阅数 |
| SD_MAIN_FUNCTION_PERIOD_MS | 配置 | 主函数周期 |
| SD_INITIAL_DELAY_MS | 配置 | 初始延迟 |
| SD_OFFER_CYCLE_TIME_MS | 配置 | Offer 周期 |
| SD_FIND_CYCLE_TIME_MS | 配置 | Find 周期 |
| SD_TTL_DEFAULT_SEC | 配置 | 默认 TTL |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| Sd_Cfg.h | 预编译配置参数 |

### 8.3 构建后配置

不适用。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x01 | SD_E_PARAM_POINTER | 空指针入参 |
| 0x02 | SD_E_PARAM_CONFIG | 无效配置 |
| 0x03 | SD_E_UNINIT | 模块未初始化 |
| 0x04 | SD_E_ALREADY_INITIALIZED | 重复初始化 |
| 0x05 | SD_E_INVALID_SERVICEID | 无效服务 ID |
| 0x06 | SD_E_INVALID_INSTANCEID | 无效实例 ID |
| 0x07 | SD_E_INVALID_EVENTGROUPID | 无效事件组 ID |
| 0x08 | SD_E_SERVICE_NOT_FOUND | 服务未找到 |
| 0x09 | SD_E_SERVICE_ALREADY_OFFERED | 服务已提供 |
| 0x0A | SD_E_NO_FREE_ENTRY | 无空闲条目 |
| 0x0B | SD_E_NOT_SUPPORTED | 不支持的操作 |

### 9.2 DEM 错误

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| — | — | 当前未定义 DEM 事件 |

### 9.3 安全机制

- 初始化状态检查
- 参数有效性验证
- 数组边界保护（MaxServices/MaxSubscriptions）

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| 默认代码段 | Sd.c 全部函数 |
| 默认数据段 | 内部状态结构体 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~1-2 KB | 服务注册表 + 订阅表 |
| ROM | ~4 KB | 代码段 |
| 堆栈 | ~256 bytes | 函数调用栈 |

---

## 11. 集成指南

- 与上层集成：SomeIp 模块调用 `Sd_OfferService`/`Sd_FindService` 进行服务管理
- 与下层集成：通过 SoAd 发送/接收 SD 消息
- 初始化顺序：Det → TcpIp → SoAd → SomeIp → Sd_Init
- MainFunction 周期建议：100ms

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| test_sd.c | 初始化/反初始化、Offer/Find/Stop 流程、订阅管理、TTL 过期、数组压缩 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 服务发现 | 两个节点间的 Find/Offer 交互 |
| 订阅管理 | SubscribeEventGroup/UnsubscribeEventGroup 完整流程 |
| TTL 过期 | 验证服务在 TTL 过期后自动移除 |

---

## 13. 实现说明 / TODO

- `Sd_LocalSendOfferMessages` 和 `Sd_LocalSendFindMessages` 为桩实现，需要实现实际的 SD 消息序列化
- `Sd_HandleMessage` 为桩实现，需要实现 SD 消息反序列化与注册表更新
- 初始延迟机制已实现（`SD_INITIAL_DELAY_MS`）
- 数组压缩逻辑已实现（移除过期条目时通过移位压缩）
- 需要添加 SD 消息的 Options 处理（IPv4 端点选项）

---

## 14. 参考资料

1. AUTOSAR_SWS_ServiceDiscovery.pdf
2. AUTOSAR_PRS_SOMEIPServiceDiscoveryProtocol.pdf
3. `docs/modules/sd.md`
4. `src/bsw/services/sd/`
