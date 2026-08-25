# SomeIp Design Document

> **Module ID**: 0x70 (112)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS SOME/IP Protocol  
> **Source Path**: `src/bsw/services/someip/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

SomeIp (Scalable service-Oriented MiddlewarE over IP) 是面向服务的车载通信协议。SomeIp 模块实现 SOME/IP 消息的序列化/反序列化、服务发现（SD）、方法调用/事件通知/字段读写的协议处理。SomeIp 是 AUTOSAR Adaptive 和 Classic 平台间通信的核心中间件。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS SOME/IP | 4.4.0 | SOME/IP 协议规范 |
| SOME/IP Protocol Specification | 1.x | GENIVI SOME/IP |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SoAd, RTE | 面向服务适配 / 运行时环境 |
| 下层 | TcpIp, UdpIp | 传输层 |
| 同层 | SomeIpSd | 服务发现 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│       SoAd / RTE / SWC              │
├─────────────────────────────────────┤
│    SomeIp + SomeIpSd (Services)     │
├─────────────────────────────────────┤
│      TcpIp / UdpIp / SoAd           │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Message Handler**: SOME/IP 消息头解析/构造
- **Method Invocation**: 方法调用/响应处理
- **Event Notification**: 事件发布/订阅处理
- **Field Access**: 字段 Get/Set/Notifier 处理
- **Serializer**: 数据序列化/反序列化

### 3.3 文件结构

```
src/bsw/services/someip/
├── include/
│   ├── SomeIp.h        # 公共 API
│   ├── SomeIp_Cfg.h    # 服务配置
│   ├── SomeIpSd.h      # 服务发现 API
│   └── SomeIpSd_Cfg.h  # SD 配置
└── src/
    ├── SomeIp.c         # 核心实现
    └── SomeIpSd.c       # 服务发现实现
```

---

## 4. 状态机

### 4.1 服务状态

```
          SomeIp_Init()
  DOWN ──────────────────► INITIAL
                             │
              OfferService() │ FindService()
                             ▼
                         NOT_READY
                             │
              Service Discovery (SD)
                             ▼
                          READY
                     (服务可用)
```

---

## 5. 数据结构

```c
typedef struct {
    uint16 ServiceId;
    uint16 MethodId;
    uint16 ClientId;
    uint16 SessionId;
    uint8  ProtocolVersion;
    uint8  InterfaceVersion;
    uint8  MessageType;     /* REQUEST / RESPONSE / NOTIFICATION */
    uint8  ReturnCode;
    uint8* PayloadPtr;
    uint32 PayloadLength;
} SomeIp_MessageType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void SomeIp_Init(const SomeIp_ConfigType* Config)` | 初始化 | SWS_SomeIp_00001 |
| `Std_ReturnType SomeIp_SendRequest(SomeIp_MessageType* Msg)` | 发送方法请求 | SWS_SomeIp_00004 |
| `Std_ReturnType SomeIp_SendResponse(SomeIp_MessageType* Msg)` | 发送响应 | SWS_SomeIp_00005 |
| `Std_ReturnType SomeIp_SendEvent(uint16 ServiceId, uint16 EventId, const uint8* Data, uint32 Len)` | 发送事件 |  |
| `void SomeIp_RxIndication(const SomeIp_MessageType* Msg)` | 接收回调 | SWS_SomeIp_00007 |
| `void SomeIp_MainFunction(void)` | 周期主函数 |  |

---

## 7. 处理流程

### 7.1 方法调用流程

1. Client 端调用 `SomeIp_SendRequest(Msg)`
2. SomeIp 序列化消息头 + Payload
3. 通过 SoAd 发送到 Server IP:Port
4. Server 端 SomeIp_RxIndication 接收
5. Server 处理后调用 `SomeIp_SendResponse`
6. Client 端 RxIndication 收到响应

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `SOMEIP_MAX_SERVICES` | 16U | 最大服务数 |
| `SOMEIP_MAX_MESSAGE_SIZE` | 4096U | 最大消息长度 |
| `SOMEIP_SD_ENABLED` | STD_ON | 服务发现支持 |
| `SOMEIP_MAIN_FUNCTION_PERIOD` | 10U | 主函数周期 (ms) |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `SOMEIP_E_UNINIT` | 初始化前调用 |
| `SOMEIP_E_INV_SERVICE` | 无效服务 ID |
| `SOMEIP_E_NO_CONNECTION` | 网络不可达 |
| `SOMEIP_E_TIMEOUT` | 请求超时 |

---

## 10. 内存与性能

- **RAM**: 服务注册表 ~16 × 32B = 512 字节
- **ROM**: ~8 KB 代码
- **性能**: 消息序列化 ~5 µs/KB

---

## 11. 集成指南

- SoAd 提供 Socket 抽象层
- SomeIpSd 管理服务可用性发现
- 服务接口通过 ARXML 配置

---

## 12. 测试策略

- 方法调用/响应往返测试
- 事件发布/订阅测试
- 服务发现 OFFER/FIND 测试
- 消息序列化正确性测试
- 超时/重连测试

---

## 13. 实现说明

- 消息头 16 字节，大端序
- 支持 TP (Transport Protocol) 分段传输大数据
- SD 使用 UDP 组播 (224.0.0.1:30490)

---

## 14. 参考文献

- AUTOSAR_SWS_SOMEIP.pdf (R4.4.0)
- GENIVI SOME/IP Protocol Specification
- yuleASR SomeIp 源码: `src/bsw/services/someip/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_SomeIp_00006 | `SomeIp_UnregisterEvent` | 测试 test_SomeIp_UnregisterEvent_ValidCall_ShouldSucceed 覆盖: SomeIp_UnregisterEvent_ValidCall_ShouldSucceed 场景 |
| SWS_SomeIp_00010 | `SomeIp_GetServiceState` | 测试 test_SomeIp_GetServiceState_ValidCall_ShouldReturnState 覆盖: SomeIp_GetServiceState_ValidCall_ShouldReturnState 场景 |
