# SomeIpIf Design Document

> **Module ID**: 0xA4  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_SOMEIPTransformer  
> **Source Path**: `src/bsw/ecual/someipif/`  
> **Reference Document**: `docs/modules/someipif.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

SomeIpIf（SOME/IP Interface）模块是 AUTOSAR ECUAL 层的组件，提供 SOME/IP 协议的头部构建与解析功能，作为上层 SOME/IP 服务与下层 SoAd（Socket Adapter）之间的适配层。该模块负责将 PDU 数据封装为 SOME/IP 报文格式，并处理接收到的 SOME/IP 报文的解封装。

主要职责：
- SOME/IP 报文头部构建（16 字节标准头）
- PDU 数据封装与发送
- SOME/IP 报文接收与解析
- 通道状态管理（INIT/ONLINE/OFFLINE）
- 服务到端点的地址映射
- TX 缓冲区管理

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS SOMEIPTransformer | 4.4.0 | SOME/IP 接口规范 |
| SOME/IP Protocol Specification | 1.x | SOME/IP 协议规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SomeIp / SomeIpSd | SOME/IP 服务模块 |
| 下层 | SoAd | Socket 适配器 |
| 公共 | Det | 开发错误追踪 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│       SomeIp / SomeIpSd             │
├─────────────────────────────────────┤
│     SomeIpIf (ECUAL Layer)          │
├─────────────────────────────────────┤
│          SoAd (ECUAL)               │
├─────────────────────────────────────┤
│       TcpIp / MCAL (Eth)            │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **头部构建器（Header Builder）**：构建 SOME/IP 16 字节标准头
- **TX 缓冲区管理器（TX Buffer Manager）**：管理待发送报文缓冲
- **通道状态管理器（Channel State Manager）**：管理通道在线/离线状态
- **服务映射器（Service Mapper）**：将服务 ID 映射到端点地址

### 3.3 文件结构

```
src/bsw/ecual/someipif/
├── include/
│   ├── SomeIpIf.h
│   └── SomeIpIf_Cfg.h
└── src/
    ├── SomeIpIf.c
    └── SomeIpIf_Lcfg.c
```

---

## 4. 状态机

模块状态机：

```
[SOMEIPIF_UNINIT]
    │ SomeIpIf_Init
    ▼
[SOMEIPIF_INIT]
    │ SetState(Online)
    ▼
[SOMEIPIF_ONLINE] ◄──► [SOMEIPIF_OFFLINE]
```

---

## 5. 核心数据结构

```c
/* 端点类型 */
typedef struct {
    uint32 IpAddress;
    uint16 Port;
    uint8  ConnectionType;  /* TCP=0x00, UDP=0x01 */
} SomeIpIf_EndpointType;

/* 服务配置 */
typedef struct {
    uint16 ServiceId;
    uint16 InstanceId;
    SomeIpIf_EndpointType Endpoint;
    boolean IsReliable;
} SomeIpIf_ServiceConfigType;

/* 通道配置 */
typedef struct {
    uint8  ChannelId;
    uint32 LocalIp;
    uint32 SubnetMask;
    uint16 UdpPort;
    uint16 TcpPort;
    uint8  NumRxFilters;
    const uint16* RxServiceIds;
} SomeIpIf_ChannelConfigType;

/* TX 缓冲区 */
typedef struct {
    uint8  data[1400];
    uint16 length;
    uint16 pduId;
    uint32 targetIp;
    uint16 targetPort;
} SomeIpIf_TxBufferType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| SomeIpIf_Init | `void SomeIpIf_Init(const SomeIpIf_ConfigType* ConfigPtr)` | 初始化 | | SWS_SomeIpIf_00001 |
| SomeIpIf_DeInit | `void SomeIpIf_DeInit(void)` | 反初始化 | | SWS_SomeIpIf_00002 |
| SomeIpIf_Transmit | `Std_ReturnType SomeIpIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)` | 发送 PDU | 封装 SOME/IP 头 | SWS_SomeIpIf_00005 |
| SomeIpIf_RxIndication | `void SomeIpIf_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | 接收指示 | 解析 SOME/IP 头 | SWS_SomeIpIf_00006 |
| SomeIpIf_MainFunction | `void SomeIpIf_MainFunction(void)` | 周期处理 | 发送缓冲数据 | SWS_SomeIpIf_00004 |
| SomeIpIf_SetState | `Std_ReturnType SomeIpIf_SetState(uint8 ChannelId, boolean Online)` | 设置状态 | | SWS_SomeIpIf_00007 |
| SomeIpIf_GetVersionInfo | `void SomeIpIf_GetVersionInfo(Std_VersionInfoType*)` | 版本信息 | | SWS_SomeIpIf_00003 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| SomeIpIf_RxIndication | 下层 SoAd 接收到 SOME/IP 报文后调用 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | Init | SOMEIPIF_E_PARAM_POINTER |
| 0x01 | DeInit | — |
| 0x02 | Transmit | SOMEIPIF_E_UNINIT, SOMEIPIF_E_PARAM_POINTER, SOMEIPIF_E_TRANSMIT_FAILED |
| 0x03 | RxIndication | — |
| 0x04 | MainFunction | — |
| 0x05 | SetState | — |

---

## 7. 处理流程

### 7.1 发送流程

1. 调用 `SomeIpIf_Transmit`，检查初始化和 OFFLINE 状态
2. 检查 TX 缓冲区是否已满（SOMEIPIF_MAX_SDUS = 8）
3. 根据 TxPduId 查找通道配置，获取 ServiceId 和目标端点
4. 调用 `SomeIpIf_BuildHeader` 构建 16 字节 SOME/IP 头
5. 将 PDU 数据拷贝到头部之后
6. 存入 TX 缓冲区

### 7.2 头部构建流程

1. 填充 Service ID（2 字节）和 Method ID（2 字节）
2. 填充 Length（4 字节，含头部长度）
3. 填充 Client ID（2 字节）和 Session ID（2 字节）
4. 填充 Protocol Version（0x01）、Interface Version（0x01）
5. 填充 Message Type 和 Return Code

### 7.3 MainFunction 处理

1. 遍历所有 TX 缓冲区
2. 通过 SoAd 发送每个缓冲的报文
3. 清空缓冲区计数

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| SOMEIPIF_DEV_ERROR_DETECT | STD_ON | 开发错误检测 |
| SOMEIPIF_MAX_SDUS | 8 | 最大 TX 缓冲数 |
| SOMEIP_PROTOCOL_VERSION | 0x01 | SOME/IP 协议版本 |
| SOMEIP_INTERFACE_VERSION | 0x01 | 接口版本 |
| SOMEIP_HEADER_LEN | 16 | 头部长度 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| SomeIpIf_Cfg.h | 预编译配置参数 |
| SomeIpIf_Lcfg.c | 通道/服务/端点配置数据 |

### 8.3 构建后配置

不适用。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x10 | SOMEIPIF_E_PARAM_POINTER | 空指针入参 |
| 0x20 | SOMEIPIF_E_UNINIT | 模块未初始化 |
| 0x30 | SOMEIPIF_E_PARAM_PDU | 无效 PDU ID |
| 0x40 | SOMEIPIF_E_TRANSMIT_FAILED | 发送失败 |

### 9.2 DEM 错误

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| — | — | 当前未定义 DEM 事件 |

### 9.3 安全机制

- 初始化状态检查
- OFFLINE 状态发送保护
- TX 缓冲区溢出保护
- PDU 长度截断保护（不超过 1400 字节）

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| 默认代码段 | SomeIpIf.c 全部函数 |
| 默认数据段 | 内部状态 + TX 缓冲区 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~11.2 KB | 8 × 1400 bytes TX 缓冲区 |
| ROM | ~2 KB | 代码段 |
| 堆栈 | ~256 bytes | 函数调用栈 |

---

## 11. 集成指南

- 与上层集成：SomeIp 模块通过 `SomeIpIf_Transmit` 发送 SOME/IP 报文
- 与下层集成：通过 SoAd 发送/接收原始 Socket 数据
- 初始化顺序：TcpIp → SoAd → Det → SomeIpIf_Init → SomeIpIf_SetState(Online)
- MainFunction 周期建议：1-10ms

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| test_someipif.c | 初始化/反初始化、头部构建、发送流程、状态管理、缓冲区溢出 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 端到端传输 | PDU → SOME/IP 封装 → SoAd 发送 |
| 接收解析 | 接收 SOME/IP 报文 → 解析头部 → 转发上层 |
| 状态切换 | ONLINE/OFFLINE 状态对发送的影响 |

---

## 13. 实现说明 / TODO

- SOME/IP 头部构建已完整实现（`SomeIpIf_BuildHeader`）
- TX 缓冲区管理和发送流程已实现
- `SomeIpIf_RxIndication` 为桩实现，需要实现 SOME/IP 头部解析
- MainFunction 中的 SoAd 发送调用为简化实现
- 需要添加 SOME/IP TP（Transport Protocol）分段支持
- 需要添加服务发现集成（与 SomeIpSd 协作）
- 编译时版本检查已实现

---

## 14. 参考资料

1. AUTOSAR_SWS_SOMEIPTransformer.pdf
2. SOME/IP Protocol Specification
3. `docs/modules/someipif.md`
4. `src/bsw/ecual/someipif/`
