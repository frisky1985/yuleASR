# DoIP Design Document

> **Module ID**: 0x4C (76)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS Diagnostics over IP  
> **Source Path**: `src/bsw/services/doip/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

DoIP (Diagnostics over IP) 实现基于 TCP/IP 的 UDS 诊断传输，是 ISO 13400 标准的 AUTOSAR 实现。DoIP 允许外部诊断仪通过以太网连接到 ECU，执行 UDS 服务（0x10/0x22/0x27/0x31/0x34 等）。DoIP 管理 TCP 连接的建立/维护、诊断消息的路由、车辆标识发现（Vehicle Identification）和路由激活（Routing Activation）。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Diagnostics over IP | 4.4.0 | DoIP 规范 |
| ISO 13400-2 | 2019 | DoIP 国际标准 |
| ISO 14229 (UDS) | — | 统一诊断服务 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Dcm | 诊断通信管理器 |
| 下层 | TcpIp, SoAd | TCP 传输 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│            Dcm                      │
├─────────────────────────────────────┤
│          DoIP (Services)            │
├─────────────────────────────────────┤
│      TcpIp / SoAd (TCP)            │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Connection Manager**: TCP 连接生命周期管理
- **Routing Activation**: 诊断连接激活/去激活
- **Vehicle Info Handler**: 车辆标识广播响应
- **Message Router**: DoIP 头解析 + UDS 消息路由到 Dcm
- **Timing Monitor**: 连接超时/诊断超时管理

### 3.3 文件结构

```
src/bsw/services/doip/
├── include/
│   ├── DoIP.h       # 公共 API
│   └── DoIP_Cfg.h   # 配置
└── src/
    └── DoIP.c        # 核心实现
```

---

## 4. 状态机

```
           DoIP_Init()
  CLOSED ──────────────► TCP_LISTEN
                           │
              TCP Connect (Tester)
                           │
                           ▼
                     TCP_CONNECTED
                           │
              Routing Activation Request
                           │
                           ▼
                    ACTIVATED
               (诊断会话可用)
```

---

## 5. 数据结构

```c
typedef enum {
    DOIP_STATE_CLOSED = 0,
    DOIP_STATE_TCP_LISTEN,
    DOIP_STATE_TCP_CONNECTED,
    DOIP_STATE_ACTIVATED
} DoIP_StateType;

typedef struct {
    uint8  ProtocolVersion;
    uint8  InverseVersion;
    uint16 PayloadType;
    uint32 PayloadLength;
    uint8* DataPtr;
} DoIP_HeaderType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void DoIP_Init(const DoIP_ConfigType* Config)` | 初始化 | SWS_DoIP_00001 |
| `void DoIP_DeInit(void)` | 反初始化 | SWS_DoIP_00002 |
| `Std_ReturnType DoIP_Transmit(const uint8* Data, uint16 Length)` | 发送 UDS 响应 |  |
| `void DoIP_RxIndication(const uint8* Data, uint16 Length)` | TCP 接收回调 |  |
| `void DoIP_MainFunction(void)` | 周期主函数 | SWS_DoIP_00004 |
| `DoIP_StateType DoIP_GetState(void)` | 获取连接状态 |  |

---

## 7. 处理流程

### 7.1 诊断会话建立流程

1. 外部 Tester 发起 TCP 连接到 DoIP 端口 (13400)
2. Tester 发送 Vehicle Identification Request → DoIP 响应车辆信息
3. Tester 发送 Routing Activation Request → DoIP 验证并激活
4. 激活后 Tester 发送 UDS 消息 → DoIP 剥离 DoIP 头 → 转发给 Dcm
5. Dcm 处理 UDS → 响应通过 DoIP 添加 DoIP 头 → TCP 发回 Tester

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `DOIP_PORT` | 13400U | TCP 监听端口 |
| `DOIP_MAX_CONNECTIONS` | 1U | 最大并发诊断连接 |
| `DOIP_ROUTING_ACTIVATION_TIMEOUT` | 5000U | 激活超时 (ms) |
| `DOIP_GENERAL_INACTIVITY_TIMEOUT` | 300000U | 非活动超时 (ms) |
| `DOIP_VIN` | "WBA..." | 17 字节车辆识别号 |
| `DOIP_LOGICAL_ADDRESS` | 0x0001U | ECU 逻辑地址 |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `DOIP_E_UNINIT` | 初始化前调用 |
| `DOIP_E_NO_CONNECTION` | 无活跃诊断连接 |
| `DOIP_E_ROUTING_REJECTED` | 路由激活被拒绝 |
| `DOIP_E_TIMEOUT` | 连接超时 |

---

## 10. 内存与性能

- **RAM**: 连接状态 ~128 字节 + 接收缓冲区 ~4096 字节
- **ROM**: ~5 KB 代码
- **性能**: 消息路由 ~10 µs

---

## 11. 集成指南

- Dcm 通过 DoIP 接收外部诊断请求
- TCP 传输通过 SoAd 的 Socket 抽象
- Vehicle Identification 使用 UDP 广播 (224.0.0.1:13400)

---

## 12. 测试策略

- 连接建立/断开测试
- 路由激活/去激活测试
- UDS 消息透传测试
- 超时断开测试
- 并发连接拒绝测试

---

## 13. 实现说明

- DoIP 头 8 字节（版本 + 反版本 + 类型 + 长度）
- 支持 DoIP 2019 版本（含 TLS 可选）
- Vehicle Identification 响应包含 VIN + Logical Address + ECU Group

---

## 14. 参考文献

- AUTOSAR_SWS_DiagnosticsOverIP.pdf (R4.4.0)
- ISO 13400-2:2019
- yuleASR DoIP 源码: `src/bsw/services/doip/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_DoIP_00003 | `DoIP_GetVersionInfo` | 测试 test_DoIP_GetVersionInfo 覆盖: DoIP_GetVersionInfo 场景 |
| SWS_DoIP_00005 | `DoIP_RxIndication` | 测试 test_DoIP_RxIndication_ValidCall_ShouldSucceed 覆盖: DoIP_RxIndication_ValidCall_ShouldSucceed 场景 |
| SWS_DoIP_00006 | `DoIP_SoAdIfRxIndication` | 测试 test_DoIP_SoAdIfRxIndication 覆盖: DoIP_SoAdIfRxIndication 场景 |
| SWS_DoIP_00007 | `DoIP_GetConnectionState` | 测试 test_DoIP_GetConnectionState_ValidCall_ShouldReturnState 覆盖: DoIP_GetConnectionState_ValidCall_ShouldReturnState 场景 |
| SWS_DoIP_00008 | `DoIP_ActivateRouting` | 测试 test_DoIP_ActivateRouting_ValidCall_ShouldSucceed 覆盖: DoIP_ActivateRouting_ValidCall_ShouldSucceed 场景 |
| SWS_DoIP_00009 | `DoIP_DeactivateRouting` | 测试 test_DoIP_DeactivateRouting_ValidCall_ShouldSucceed 覆盖: DoIP_DeactivateRouting_ValidCall_ShouldSucceed 场景 |
| SWS_DoIP_00017 | `DoIP_SoAdTpTxConfirmation` | 测试 test_DoIP_SoAdTpTxConfirmation 覆盖: DoIP_SoAdTpTxConfirmation 场景 |
