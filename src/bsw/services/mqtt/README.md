# MQTT 模块 (AUTOSAR 非标准扩展)

## 概述

MQTT模块是为AUTOSAR软件架构设计的非标准扩展模块，提供MQTT协议客户端功能，使车载系统能够与云服务进行通信。

## 架构位置

```
┌───────────────────────────────────────────────────┐
│          应用软件组件 (ASW)            │
├───────────────────────────────────────────────────┤
│              │                    │         │
│              ▼                    ▼         │
│      ┌───────────┐        ┌───────────┐       │
│      │   MQTT    │        │    COM      │       │
│      │  (Service) │        │  (Service)  │       │
│      └───────────┘        └───────────┘       │
│              │                    │         │
│              ▼                    ▼         │
├───────────────────────────────────────────────────┤
│              │                    │         │
│              ▼                    ▼         │
│      ┌───────────┐        ┌───────────┐       │
│      │   SoAd    │        │   PDU-R    │       │
│      │  (ECUAL)  │        │  (Service) │       │
│      └───────────┘        └───────────┘       │
│              │                    │         │
│              ▼                    ▼         │
├───────────────────────────────────────────────────┤
│      ┌───────────────────────────────────────┐     │
│      │        TCP/IP Stack (MCAL/IoHwAb)         │     │
│      └───────────────────────────────────────┘     │
│                        │                         │
│                        ▼                         │
├───────────────────────────────────────────────────┤
│              Ethernet Driver (MCAL)               │
└───────────────────────────────────────────────────┘
```

## 功能特性

### 协议支持
- MQTT 3.1.1 (标准版本)
- MQTT 5.0 (通过配置开启)

### 核心功能
- 连接管理：Connect/Disconnect/Ping
- 消息发布：Publish (QoS 0/1/2)
- 订阅管理：Subscribe/Unsubscribe
- 自动重连：可配置的重连机制
- 状态监测：连接状态回调

### 安全特性
- 用户名/密码认证
- TLS/SSL加密 (可选配置)
- 清洁会话支持

### 性能特性
- 非阻塞式API设计
- 可配置缓冲区大小
- 状态机驱动的连接管理
- 支持最多4个并发连接
- 每连接最多8个订阅

## API使用示例

### 初始化和连接

```c
#include "Mqtt.h"

void Mqtt_InitExample(void)
{
    Mqtt_ReturnType result;
    Mqtt_ConnectionConfigType connConfig;
    
    /* 初始化模块 */
    result = Mqtt_Init(&Mqtt_Config);
    if (result != MQTT_OK) {
        /* 错误处理 */
        return;
    }
    
    /* 配置连接参数 */
    connConfig.brokerHost = "mqtt.example.com";
    connConfig.brokerPort = 1883;
    connConfig.clientId = "Vehicle_001";
    connConfig.keepAliveSeconds = 60;
    connConfig.cleanSession = MQTT_CLEAN_SESSION_TRUE;
    connConfig.version = MQTT_VERSION_311;
    connConfig.username = "vehicle_user";
    connConfig.password = "secure_pass";
    connConfig.connectTimeoutMs = 5000;
    connConfig.autoReconnect = TRUE;
    connConfig.reconnectIntervalMs = 5000;
    
    /* 建立连接 */
    result = Mqtt_Connect(0, &connConfig);
    if (result != MQTT_OK) {
        /* 错误处理 */
        return;
    }
}
```

### 发布消息

```c
void Mqtt_PublishExample(void)
{
    Mqtt_ReturnType result;
    Mqtt_PublishMessageType message;
    uint8 payload[] = {0x01, 0x02, 0x03, 0x04}; /* CAN报文数据 */
    
    message.topic = "vehicle/canbus/ecu1";
    message.payload = payload;
    message.payloadLength = sizeof(payload);
    message.qos = MQTT_QOS_1;
    message.retain = MQTT_RETAIN_FALSE;
    
    result = Mqtt_Publish(0, &message, NULL);
    if (result != MQTT_OK) {
        /* 错误处理 */
    }
}
```

### 订阅主题

```c
void Mqtt_OnMessageReceived(Mqtt_ConnectionIdType connId,
                             const Mqtt_ReceivedMessageType* msg)
{
    /* 处理接收到的消息 */
    printf("Topic: %s\n", msg->topic);
    printf("Payload length: %u\n", msg->payloadLength);
    /* 解析负载数据... */
}

void Mqtt_SubscribeExample(void)
{
    Mqtt_ReturnType result;
    Mqtt_SubscriptionType subscription;
    
    subscription.topicFilter = "cloud/commands/+";
    subscription.maxQoS = MQTT_QOS_1;
    subscription.subscriptionId = 1;
    
    result = Mqtt_Subscribe(0, &subscription, Mqtt_OnMessageReceived);
    if (result != MQTT_OK) {
        /* 错误处理 */
    }
}
```

### 主循环函数

```c
void Mqtt_MainFunctionExample(void)
{
    /* 在OS任务中定期调用，建议10ms周期 */
    Mqtt_MainFunction();
}
```

## 配置选项

### Mqtt_Cfg.h 主要配置

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| MQTT_DEV_ERROR_DETECT | STD_ON | 开发错误检测使能 |
| MQTT_SUPPORT_V50 | STD_ON | MQTT 5.0协议支持 |
| MQTT_SUPPORT_TLS | STD_OFF | TLS加密支持 |
| MQTT_MAX_CONNECTIONS | 4 | 最大连接数 |
| MQTT_MAX_SUBSCRIPTIONS_PER_CONN | 8 | 每连接最大订阅数 |
| MQTT_SEND_BUFFER_SIZE | 2048 | 发送缓冲区大小(字节) |
| MQTT_RECV_BUFFER_SIZE | 2048 | 接收缓冲区大小(字节) |

## 文件结构

```
src/bsw/services/mqtt/
├── include/
│   ├── Mqtt.h         # 标准API头文件
│   └── Mqtt_Cfg.h     # 配置头文件
├── src/
│   ├── Mqtt.c         # 核心实现
│   └── Mqtt_Lcfg.c    # 链接时配置表
├── CMakeLists.txt
└── README.md
```

## 依赖关系

### 必需依赖
- **Std_Types.h** - AUTOSAR标准类型
- **ComStack_Types.h** - 通信栈类型
- **TcpIp** - TCP/IP协议栈

### 可选依赖
- **Det.h** - 开发错误追踪 (当MQTT_DEV_ERROR_DETECT=STD_ON时)

## 开发状态

- [x] 核心MQTT协议实现 (CONNECT, PUBLISH, SUBSCRIBE, PING, DISCONNECT)
- [x] 状态机驱动的连接管理
- [x] 自动重连机制
- [x] 基本的QoS 0支持
- [ ] QoS 1和QoS 2完整实现 (需要持久化存储)
- [ ] TLS/SSL加密支持
- [ ] MQTT 5.0特性 (用户属性、原因码等)
- [ ] WebSocket运输支持

## 使用场景

### 典型应用场景
1. **车联网 (V2X)** - 与云平台的双向通信
2. **OTA升级** - 软件更新包下发和状态上报
3. **远程诊断** - 车辆数据实时上传至云端
4. **车队管理** - 多车辆数据汇总与分析

### 与标准AUTOSAR模块的区别

| 特性 | 标准AUTOSAR模块 | MQTT模块 |
|------|------------------|-----------|
| 协议 | 车载专用 (CAN/DoIP/SOME/IP) | 互联网协议 |
| 安全性 | ASIL-D | QM (可配置) |
| 拥有者 | AUTOSAR联盟 | YuleTech (非标准) |
| 认证 | 需要第三方认证 | 内部验证 |

## 许可

MIT License - 详见 LICENSE 文件

## 作者

YuleTech Embedded Team

---

**Note**: 本模块为AUTOSAR非标准扩展，不适合要求严格AUTOSAR认证的项目。用于需要与云服务通信的车联网应用场景。
