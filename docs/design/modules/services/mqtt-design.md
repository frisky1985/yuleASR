# Mqtt Design Document

> **Module ID**: 0xA0  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Mqtt  
> **Source Path**: `src/bsw/services/mqtt/`  
> **Reference Document**: `docs/modules/mqtt.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Mqtt 模块是 AUTOSAR 非标准扩展模块，提供 MQTT 3.1.1 和 5.0 协议客户端支持，用于车载系统与云服务的通信。该模块位于服务层，通过 SoAd（Socket Adapter）或 TCP/IP 协议栈与网络交互，为上层应用软件组件（ASWC）提供发布/订阅模式的消息传输能力。

主要职责：
- MQTT 协议连接管理（连接/断开/自动重连）
- 消息发布（Publish）与订阅（Subscribe/Unsubscribe）
- TLS 加密通信支持（可选）
- 保活机制（PING/PONG）
- 非阻塞状态机驱动
- 连接统计信息维护

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| MQTT 3.1.1 协议 | OASIS Standard | 消息队列遥测传输 |
| MQTT 5.0 协议 | OASIS Standard | MQTT 增强版本 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | ASWC | 应用层软件组件 |
| 下层 | TcpIp / SoAd | TCP/IP 协议栈与 Socket 适配器 |
| 下层 | Mqtt_Tls | TLS 加密子系统（可选） |
| 公共 | Det | 开发错误追踪 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        Application Layer            │
├─────────────────────────────────────┤
│          Mqtt (Services)            │
├─────────────────────────────────────┤
│       Mqtt_Tls (可选 TLS 层)        │
├─────────────────────────────────────┤
│         TcpIp / SoAd                │
├─────────────────────────────────────┤
│          MCAL (Eth)                 │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **连接管理器（Connection Manager）**：管理多个 MQTT 连接的生命周期
- **报文编码器（Packet Encoder）**：编码 CONNECT/PUBLISH/SUBSCRIBE/PING 等 MQTT 报文
- **报文解码器（Packet Decoder）**：解码 CONNACK/PUBACK/SUBACK 等响应报文
- **状态机引擎（State Machine Engine）**：驱动连接状态转换
- **订阅管理器（Subscription Manager）**：管理主题订阅与回调分发
- **TLS 子系统（TLS Subsystem）**：通过 Mqtt_Tls 提供加密通信

### 3.3 文件结构

```
src/bsw/services/mqtt/
├── include/
│   ├── Mqtt.h
│   ├── Mqtt_Cfg.h
│   ├── Mqtt_CertMgr.h
│   ├── Mqtt_Tls.h
│   └── mbedtls/
├── src/
│   ├── Mqtt.c
│   ├── Mqtt_CertMgr.c
│   ├── Mqtt_Lcfg.c
│   └── Mqtt_Tls.c
```

---

## 4. 状态机

MQTT 连接状态机：

```
[MQTT_STATE_UNINIT]
    │ Init
    ▼
[MQTT_STATE_DISCONNECTED]
    │ Connect
    ▼
[MQTT_STATE_CONNECTING]
    │ TCP connected
    ▼
[MQTT_STATE_TCP_CONNECTING]
    │ (TLS enabled?) ──yes──► [MQTT_STATE_TLS_HANDSHAKING]
    │ no                          │ TLS done
    ▼                             ▼
[MQTT_STATE_MQTT_CONNECTING] ◄──┘
    │ CONNACK received
    ▼
[MQTT_STATE_CONNECTED]
    │ Disconnect / Error
    ▼
[MQTT_STATE_DISCONNECTING]
    │ done
    ▼
[MQTT_STATE_DISCONNECTED]
    │ auto-reconnect
    ▼
[MQTT_STATE_RECONNECTING]
```

---

## 5. 核心数据结构

```c
/* 连接状态 */
typedef enum {
    MQTT_STATE_UNINIT = 0,
    MQTT_STATE_DISCONNECTED,
    MQTT_STATE_CONNECTING,
    MQTT_STATE_TCP_CONNECTING,
    MQTT_STATE_TLS_HANDSHAKING,   /* 条件编译 */
    MQTT_STATE_MQTT_CONNECTING,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_DISCONNECTING,
    MQTT_STATE_RECONNECTING
} Mqtt_ConnectionStateType;

/* 连接配置 */
typedef struct {
    const char* brokerHost;
    uint16 brokerPort;
    const char* clientId;
    uint16 keepAliveSeconds;
    Mqtt_CleanSessionType cleanSession;
    Mqtt_ProtocolVersionType version;
    const char* username;
    const char* password;
    uint16 connectTimeoutMs;
    uint16 recvTimeoutMs;
    uint16 sendTimeoutMs;
    boolean autoReconnect;
    uint16 reconnectIntervalMs;
    /* TLS 配置 (条件编译) */
} Mqtt_ConnectionConfigType;

/* 发布消息 */
typedef struct {
    const char* topic;
    const uint8* payload;
    uint32 payloadLength;
    Mqtt_QoSType qos;
    Mqtt_RetainType retain;
} Mqtt_PublishMessageType;

/* 连接信息（只读统计） */
typedef struct {
    Mqtt_ConnectionStateType state;
    uint32 messagesSent;
    uint32 messagesReceived;
    uint32 bytesSent;
    uint32 bytesReceived;
    uint32 connectCount;
    uint32 disconnectCount;
    uint32 reconnectCount;
    uint32 lastErrorCode;
} Mqtt_ConnectionInfoType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| Mqtt_Init | `Mqtt_ReturnType Mqtt_Init(const Mqtt_ConfigType* config)` | 初始化模块 | | SWS_Mqtt_00001 |
| Mqtt_DeInit | `Mqtt_ReturnType Mqtt_DeInit(void)` | 反初始化 | 断开所有连接 | SWS_Mqtt_00002 |
| Mqtt_Connect | `Mqtt_ReturnType Mqtt_Connect(Mqtt_ConnectionIdType, const Mqtt_ConnectionConfigType*)` | 连接代理 | 异步 | SWS_Mqtt_00005 |
| Mqtt_Disconnect | `Mqtt_ReturnType Mqtt_Disconnect(Mqtt_ConnectionIdType)` | 断开连接 | | SWS_Mqtt_00005 |
| Mqtt_Publish | `Mqtt_ReturnType Mqtt_Publish(Mqtt_ConnectionIdType, const Mqtt_PublishMessageType*, Mqtt_PublishCallbackType)` | 发布消息 | | SWS_Mqtt_00005 |
| Mqtt_Subscribe | `Mqtt_ReturnType Mqtt_Subscribe(Mqtt_ConnectionIdType, const Mqtt_SubscriptionType*, Mqtt_MessageCallbackType)` | 订阅主题 | | SWS_Mqtt_00005 |
| Mqtt_Unsubscribe | `Mqtt_ReturnType Mqtt_Unsubscribe(Mqtt_ConnectionIdType, const char*)` | 取消订阅 | | SWS_Mqtt_00005 |
| Mqtt_Ping | `Mqtt_ReturnType Mqtt_Ping(Mqtt_ConnectionIdType)` | 保活请求 | | SWS_Mqtt_00005 |
| Mqtt_GetConnectionState | `Mqtt_ConnectionStateType Mqtt_GetConnectionState(Mqtt_ConnectionIdType)` | 获取状态 | | SWS_Mqtt_00005 |
| Mqtt_GetConnectionInfo | `Mqtt_ReturnType Mqtt_GetConnectionInfo(Mqtt_ConnectionIdType, Mqtt_ConnectionInfoType*)` | 获取统计 | | SWS_Mqtt_00005 |
| Mqtt_MainFunction | `void Mqtt_MainFunction(void)` | 周期处理 | 驱动状态机 | SWS_Mqtt_00004 |
| Mqtt_SetConnectionCallback | `void Mqtt_SetConnectionCallback(Mqtt_ConnectionIdType, Mqtt_ConnectionCallbackType)` | 设置回调 | | SWS_Mqtt_00005 |
| Mqtt_GetVersionInfo | `void Mqtt_GetVersionInfo(Std_VersionInfoType*)` | 版本信息 | 条件编译 | SWS_Mqtt_00003 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| Mqtt_ConnectionCallbackType | 连接状态变化通知 |
| Mqtt_MessageCallbackType | 收到订阅消息通知 |
| Mqtt_PublishCallbackType | 发布完成确认 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x01 | Init | MQTT_E_PARAM_CONFIG, MQTT_E_ALREADY_INITIALIZED |
| 0x02 | DeInit | MQTT_E_UNINIT |
| 0x03 | Connect | MQTT_E_UNINIT, MQTT_E_PARAM_CONNECTION, MQTT_E_PARAM_CONFIG, MQTT_E_PARAM_POINTER |
| 0x04 | Disconnect | MQTT_E_UNINIT, MQTT_E_PARAM_CONNECTION |
| 0x05 | Publish | MQTT_E_UNINIT, MQTT_E_PARAM_CONNECTION, MQTT_E_PARAM_POINTER, MQTT_E_PARAM_TOPIC |
| 0x06 | Subscribe | MQTT_E_UNINIT, MQTT_E_PARAM_CONNECTION, MQTT_E_PARAM_TOPIC |
| 0x07 | Unsubscribe | MQTT_E_UNINIT, MQTT_E_PARAM_CONNECTION, MQTT_E_PARAM_TOPIC |
| 0x08 | GetVersionInfo | — |
| 0x09 | MainFunction | — |
| 0x0A | Ping | MQTT_E_UNINIT, MQTT_E_NOCONN, MQTT_E_BUSY |

---

## 7. 处理流程

### 7.1 连接流程

1. 调用 `Mqtt_Connect`，保存连接配置
2. 状态转为 `MQTT_STATE_CONNECTING`，创建 TCP Socket
3. `MainFunction` 驱动状态机进入 `MQTT_STATE_TCP_CONNECTING`
4. TCP 连接成功后，可选进入 TLS 握手
5. 发送 MQTT CONNECT 报文（`Mqtt_EncodeConnect`）
6. 等待 CONNACK，成功后进入 `MQTT_STATE_CONNECTED`

### 7.2 发布流程

1. 调用 `Mqtt_Publish`，检查连接状态
2. 生成 Packet ID（`Mqtt_GetNextPacketId`）
3. 编码 PUBLISH 报文（`Mqtt_EncodePublish`）
4. 通过 TCP/TLS 发送（`Mqtt_SendPacket`）
5. 更新统计信息，触发用户回调

### 7.3 订阅流程

1. 调用 `Mqtt_Subscribe`，查找空闲订阅槽
2. 填充内部订阅信息，状态设为 `SUB_STATE_PENDING`
3. 编码 SUBSCRIBE 报文并发送
4. 收到 SUBACK 后将状态转为 `SUB_STATE_ACTIVE`

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| MQTT_DEV_ERROR_DETECT | STD_ON | 开发错误检测 |
| MQTT_SUPPORT_TLS | STD_ON/OFF | TLS 支持开关 |
| MQTT_VERSION_INFO_API | STD_ON | 版本信息 API 开关 |
| MQTT_MAX_CONNECTIONS | 配置 | 最大连接数 |
| MQTT_MAX_SUBSCRIPTIONS_PER_CONN | 配置 | 每连接最大订阅数 |
| MQTT_MAX_TOPIC_LENGTH | 配置 | 最大主题长度 |
| MQTT_SEND_BUFFER_SIZE | 配置 | 发送缓冲区大小 |
| MQTT_RECV_BUFFER_SIZE | 配置 | 接收缓冲区大小 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| Mqtt_Lcfg.c | 全局配置指针 `Mqtt_ConfigPtr` |

### 8.3 构建后配置

不适用。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x01 | MQTT_E_PARAM_POINTER | 空指针入参 |
| 0x02 | MQTT_E_PARAM_CONFIG | 无效配置 |
| 0x03 | MQTT_E_PARAM_CONNECTION | 无效连接 ID |
| 0x04 | MQTT_E_PARAM_TOPIC | 无效主题 |
| 0x05 | MQTT_E_PARAM_PAYLOAD | 无效负载 |
| 0x06 | MQTT_E_PARAM_QOS | 无效 QoS |
| 0x07 | MQTT_E_UNINIT | 模块未初始化 |
| 0x08 | MQTT_E_ALREADY_INITIALIZED | 重复初始化 |
| 0x09 | MQTT_E_CONNECTION_FAILED | 连接失败 |
| 0x0A | MQTT_E_PUBLISH_FAILED | 发布失败 |
| 0x0B | MQTT_E_SUBSCRIBE_FAILED | 订阅失败 |
| 0x0C | MQTT_E_BUFFER_OVERFLOW | 缓冲区溢出 |

### 9.2 DEM 错误

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| — | — | 当前未定义 DEM 事件 |

### 9.3 安全机制

- TLS 加密通信（mbedTLS 集成）
- 用户名/密码认证
- 资源保护：连接数限制、缓冲区大小限制

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| 默认代码段 | Mqtt.c 全部函数 |
| 默认数据段 | 连接数组、初始化标志 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | 每连接 ~2-4 KB | 发送/接收缓冲区 + 订阅管理 |
| ROM | ~8 KB | 报文编解码 + 状态机 |
| 堆栈 | ~512 bytes | 编码函数调用栈 |

---

## 11. 集成指南

- 与上层集成：ASWC 通过 `Mqtt_Publish`/`Mqtt_Subscribe` 与云服务通信
- 与下层集成：依赖 TcpIp 提供 Socket 接口（`TcpIp_SocketCreate`、`TcpIp_Send`、`TcpIp_Receive`）
- TLS 集成：通过 `Mqtt_Tls` 子系统与 mbedTLS 交互
- 初始化顺序：TcpIp → SoAd → Mqtt_Init → Mqtt_Connect
- MainFunction 周期建议：10-100ms

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| test_mqtt.c | 初始化/反初始化、CONNECT 报文编码、PUBLISH 编解码、SUBSCRIBE 流程、状态机转换 |
| test_mqtt_tls.c | TLS 握手、加密发送/接收 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 端到端发布 | 连接代理 → 发布消息 → 验证接收 |
| 自动重连 | 断开连接后验证自动重连 |
| TLS 通信 | 验证加密连接正确性 |
| 多连接 | 同时管理多个 MQTT 连接 |

---

## 13. 实现说明 / TODO

- CONNECT 报文编码已完整实现（`Mqtt_EncodeConnect`）
- PUBLISH/SUBSCRIBE/PING/DISCONNECT 报文编码已实现
- 接收报文解析尚未完整实现（`Mqtt_ReceivePacket` 仅接收原始数据）
- CONNACK/SUBACK 处理逻辑待完善
- 自动重连机制框架已搭建但尚未完整实现
- `Mqtt_CheckTimeout` 使用 `Mqtt_TickCount` 作为时间源，需替换为真实系统时间
- TLS 集成依赖 mbedTLS，需验证嵌入式平台兼容性

---

## 14. 参考资料

1. MQTT 3.1.1 Protocol Specification (OASIS Standard)
2. MQTT 5.0 Protocol Specification (OASIS Standard)
3. `docs/modules/mqtt.md`
4. `src/bsw/services/mqtt/`
