# SoAd Design Document

> **Module ID**: 0x97 (151)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_SocketAdaptor  
> **Source Path**: `src/bsw/services/soad/`  
> **Reference Document**: `docs/modules/soad.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

SoAd（Socket Adapter）是 AUTOSAR BSW 服务层的套接字适配器模块。该模块为上层协议（如 SOME/IP、DoIP）提供统一的 TCP/UDP 通信接口，将 AUTOSAR PDU 路由机制与底层 TcpIp 栈的套接字 API 进行桥接。

SoAd 管理套接字连接的生命周期（创建、绑定、连接、发送、接收、关闭），支持 TCP 客户端/服务器模式和 UDP 单播/多播模式。模块还负责 PDU 头的构建与解析（可选的 8 字节头部），以及连接超时管理和 IP 地址分配管理。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS SocketAdaptor | R22-11 (4.7.0) | 套接字适配器模块规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SOME/IP | SOME/IP 消息传输 |
| 上层 | DoIP | 诊断 over IP |
| 上层 | PduR | PDU 路由 |
| 下层 | TcpIp | TCP/IP 协议栈 |
| 下层 | Det | 开发错误检测 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   SOME/IP / DoIP / SD (上层协议)     │
├─────────────────────────────────────┤
│   SoAd (套接字适配器)                 │
├─────────────────────────────────────┤
│   TcpIp (TCP/IP 协议栈)              │
├─────────────────────────────────────┤
│   EthIf / Eth Driver (以太网驱动)    │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **连接管理**: 管理最多 16 个套接字连接的状态（Closed/Connecting/Connected/Disconnecting/Listening）
- **PDU 路由**: 根据 PDU ID 查找路由配置，将 PDU 分发到对应的套接字连接
- **PDU 头处理**: 构建/解析 8 字节 PDU 头（Message Type、Length、Request ID、Protocol/Interface Version）
- **缓冲区管理**: 每个连接独立的 Rx/Tx 缓冲区（最大 1508 字节 = 1500 PDU + 8 Header）
- **超时管理**: 连接超时（5000ms）、断开超时（2000ms）

### 3.3 文件结构

```
src/bsw/services/soad/
├── include/
│   ├── SoAd.h          # 公共 API 与类型声明
│   ├── SoAd_Cfg.h      # 预编译配置（自动生成）
│   └── SoAd_MemMap.h   # 内存段映射
└── src/
    ├── SoAd.c           # 核心实现
    └── SoAd_Test.c      # 测试辅助
```

---

## 4. 状态机

### 4.1 模块状态

```
              SoAd_Init()
   UNINIT ──────────────► INIT
     ▲                      │
     │   SoAd_DeInit()      │
     └──────────────────────┘
```

### 4.2 连接状态

```
                  OpenTcpConnection / OpenUdpConnection
   ┌────────┐ ─────────────────────────────────────► ┌────────────┐
   │ CLOSED │                                        │ CONNECTING │
   └────────┘ ◄───────────────────────────────────── └──────┬─────┘
        ▲            Timeout / Close                          │
        │                                                     │ Connected
        │   CloseTcpConnection                                ▼
        │ ◄────────────────────────────────────────── ┌───────────┐
        │                                              │ CONNECTED │
        │                                              └───────────┘
        ▲
        │
   ┌──────────────┐
   │  LISTENING   │  (TCP Server)
   └──────────────┘
```

| 状态 | 枚举值 | 说明 |
|------|--------|------|
| `SOAD_CONN_STATE_CLOSED` | 0 | 连接关闭 |
| `SOAD_CONN_STATE_CONNECTING` | 1 | 连接建立中 |
| `SOAD_CONN_STATE_CONNECTED` | 2 | 连接已建立 |
| `SOAD_CONN_STATE_DISCONNECTING` | 3 | 断开中 |
| `SOAD_CONN_STATE_LISTENING` | 4 | 监听中（TCP 服务器） |

---

## 5. 核心数据结构

### 5.1 连接运行时状态 `SoAd_ConnectionStateType`

```c
typedef struct {
    SoAd_ConnStateType State;          // 连接状态
    TcpIp_SocketIdType SocketId;       // TcpIp 套接字 ID
    uint16 ConnGrpId;                  // 连接组 ID
    TcpIp_SockAddrType RemoteAddr;     // 远端地址
    uint32 ConnectTimeout;             // 连接超时计数器
    uint32 DisconnectTimeout;          // 断开超时计数器
    boolean CloseRequested;            // 关闭请求标志
    boolean AbortRequested;            // 中止请求标志
} SoAd_ConnectionStateType;
```

### 5.2 PDU 头格式

| 偏移 | 长度 | 字段 | 说明 |
|------|------|------|------|
| 0 | 1 | Message Type | 消息类型（0x01=REQUEST, 0x80=RETURN） |
| 1 | 3 | Message Length | 消息长度（大端） |
| 3 | 4 | Request ID | 请求标识（含 PDU ID） |
| 7 | 1 | Protocol Version | 协议版本 |
| 8 | 1 | Interface Version | 接口版本 |

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `SoAd_Init(ConfigPtr)` | 0x01 | 初始化模块 | SWS_SoAd_00001 |
| `SoAd_DeInit()` | 0x02 | 反初始化 | SWS_SoAd_00002 |
| `SoAd_GetVersionInfo(versioninfo)` | 0x03 | 获取版本信息 | SWS_SoAd_00003 |
| `SoAd_OpenTcpConnection(SoConId)` | 0x04 | 打开 TCP 连接 | SWS_SoAd_00004 |
| `SoAd_OpenUdpConnection(SoConId)` | 0x05 | 打开 UDP 连接 | SWS_SoAd_00005 |
| `SoAd_CloseTcpConnection(SoConId, Abort)` | 0x06 | 关闭 TCP 连接 | SWS_SoAd_00006 |
| `SoAd_CloseUdpConnection(SoConId)` | 0x07 | 关闭 UDP 连接 | SWS_SoAd_00007 |
| `SoAd_Send(SoConId, PduInfoPtr)` | 0x08 | 发送数据 | SWS_SoAd_00008 |
| `SoAd_Receive(SoConId, PduInfoPtr, Length)` | 0x09 | 接收数据 | SWS_SoAd_00009 |
| `SoAd_GetRemoteAddr(SoConId, IpAddrPtr, PortPtr)` | 0x0A | 获取远端地址 | SWS_SoAd_00010 |
| `SoAd_SetRemoteAddr(SoConId, IpAddrPtr)` | 0x0B | 设置远端地址 | SWS_SoAd_00011 |
| `SoAd_ReleaseIpAddrAssignment(LocalAddrId)` | 0x0C | 释放 IP 地址 | SWS_SoAd_00012 |
| `SoAd_RequestIpAddrAssignment(LocalAddrId, Type)` | 0x0D | 请求 IP 地址 | SWS_SoAd_00013 |
| `SoAd_MainFunction()` | 0x0E | 主函数 | SWS_SoAd_00014 |
| `SoAd_RequestConnMode(SoConId, Mode)` | 0x0F | 请求连接模式 | SWS_SoAd_00015 |

### 6.2 回调函数

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `SoAd_RxIndication(SocketId, RemoteAddrPtr, BufPtr, Length)` | 0x10 | 接收指示 | SWS_SoAd_00016 |
| `SoAd_TxConfirmation(SocketId, Length)` | 0x11 | 发送确认 | SWS_SoAd_00017 |
| `SoAd_TcpIpEvent(SocketId, Event, EventStatus)` | 0x12 | TcpIp 事件 | SWS_SoAd_00018 |
| `SoAd_LocalIpAddrAssignmentChg(LocalAddrId, State)` | 0x14 | IP 地址变更 | SWS_SoAd_00019 |

### 6.3 服务 ID 与错误码

**DET 错误码**:

| 错误码 | 宏名 | 说明 |
|--------|------|------|
| 0x01 | `SOAD_E_PARAM_POINTER` | 无效指针 |
| 0x02 | `SOAD_E_PARAM_CONFIG` | 无效配置 |
| 0x03 | `SOAD_E_UNINIT` | 模块未初始化 |
| 0x04 | `SOAD_E_ALREADY_INITIALIZED` | 重复初始化 |
| 0x05 | `SOAD_E_INVALID_CONNID` | 无效连接 ID |
| 0x06 | `SOAD_E_INVALID_PDUID` | 无效 PDU ID |
| 0x07 | `SOAD_E_INVALID_SOCKETID` | 无效套接字 ID |
| 0x08 | `SOAD_E_INVALID_ADDRESS` | 无效地址 |
| 0x09 | `SOAD_E_CONNECTION_STATE` | 连接状态错误 |
| 0x0A | `SOAD_E_BUFFER_OVERFLOW` | 缓冲区溢出 |
| 0x0B | `SOAD_E_INIT_FAILED` | 初始化失败 |
| 0x0C | `SOAD_E_NOT_SUPPORTED` | 不支持的操作 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查是否已初始化（DET 报错 `SOAD_E_ALREADY_INITIALIZED`）
2. 校验 `ConfigPtr` 非空
3. 保存配置指针
4. 初始化所有连接状态为 CLOSED
5. 初始化所有 Rx/Tx 缓冲区为无效
6. 设置模块状态为 INIT

### 7.2 TCP 连接建立流程

1. 校验模块状态和连接 ID
2. 检查连接当前为 CLOSED 状态
3. 查找连接配置
4. 调用 `TcpIp_Create(AF_INET, SOCK_STREAM, &socketId)` 创建 TCP 套接字
5. 设置连接状态为 CONNECTING
6. 设置连接超时计数器（5000ms / 10ms = 500 ticks）
7. 如有远端端口配置，调用 `TcpIp_Bind()` 绑定

### 7.3 主函数处理流程

`SoAd_MainFunction()` 周期调用（默认 10ms）:

1. 检查模块已初始化
2. 遍历所有连接更新超时:
   - CONNECTING 状态: 递减连接超时，到期则关闭连接
   - DISCONNECTING 状态: 递减断开超时，到期则强制关闭

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `SOAD_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `SOAD_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `SOAD_NUMBER_OF_SOCKETS` | 8 | 套接字数量 |
| `SOAD_NUMBER_OF_CONNECTIONS` | 16 | 连接数量 |
| `SOAD_NUMBER_OF_CONNECTION_GROUPS` | 4 | 连接组数量 |
| `SOAD_NUMBER_OF_PDU_ROUTES` | 32 | PDU 路由数量 |
| `SOAD_MAX_PDU_LENGTH` | 1500 | 最大 PDU 长度 |
| `SOAD_MAX_HEADER_LENGTH` | 8 | 最大头部长度 |
| `SOAD_CONNECT_TIMEOUT_MS` | 5000 | 连接超时 (ms) |
| `SOAD_DISCONNECT_TIMEOUT_MS` | 2000 | 断开超时 (ms) |
| `SOAD_PDU_HEADER_ENABLE` | STD_ON | PDU 头使能 |
| `SOAD_PDU_HEADER_LENGTH` | 8 | PDU 头长度 |

### 8.2 链接时配置

通过 `SoAd_Config` 全局常量提供链接时配置。

### 8.3 构建后配置

不支持构建后配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 入口均进行完整的参数校验（初始化状态、指针非空、连接 ID 有效性、连接状态检查）。

### 9.2 DEM 错误

模块不直接报告 DEM 事件。连接失败等运行时错误通过回调通知上层。

### 9.3 安全机制

- **重复初始化保护**: 检测并拒绝重复的 Init 调用
- **连接超时保护**: 防止连接建立无限等待
- **缓冲区溢出保护**: PDU 长度不超过 MAX_PDU_LENGTH + MAX_HEADER_LENGTH
- **PDU 头校验**: 解析时验证头部字段有效性

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 段名 | 类型 | 内容 |
|------|------|------|
| `SOAD_START_SEC_VAR_CLEARED_UNSPECIFIED` | 清零变量 | `SoAd_InternalState` |
| `SOAD_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据 | `SoAd_Config` |
| `SOAD_START_SEC_CODE` | 代码段 | 所有函数实现 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| 每连接 RAM | ~60 bytes | ConnectionStateType |
| Rx 缓冲区 | 1508 bytes/ch | Buffer + Length + IsValid |
| Tx 缓冲区 | 1510 bytes/ch | Buffer + Length + IsValid + IsPending |
| 总 RAM (16连接) | ~49 KB | 连接状态 + 缓冲区 |
| ROM（代码） | ~6 KB | 连接管理 + PDU 处理 |

---

## 11. 集成指南

1. **TcpIp 集成**: 配置套接字参数，注册 Rx/Tx 回调
2. **SOME/IP 集成**: 配置 PDU 路由，SOME/IP 消息通过 SoAd_Send/Receive 传输
3. **DoIP 集成**: DoIP 诊断消息通过 SoAd 的 TCP 连接传输
4. **PduR 集成**: PDU 路由表配置 TxPduId/RxPduId 到 SoConId 的映射
5. **SD 集成**: SOME/IP-SD 通过 UDP 连接组进行服务发现

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| 初始化测试 | NULL 指针、重复初始化检测 |
| TCP 连接测试 | Open/Close/Send/Receive 完整流程 |
| UDP 连接测试 | Open/Close/Send 流程 |
| PDU 头测试 | BuildPduHeader/ParsePduHeader 正确性 |
| 超时测试 | 连接/断开超时触发正确行为 |
| 错误注入测试 | 无效连接 ID、未初始化调用 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| SoAd-TcpIp 集成 | 端到端 TCP/UDP 数据传输 |
| SOME/IP-SoAd 集成 | SOME/IP 消息通过 SoAd 传输 |
| DoIP-SoAd 集成 | DoIP 诊断会话的 TCP 连接管理 |

---

## 13. 实现说明 / TODO

- `SoAd_RequestConnMode()` 当前返回 `E_NOT_OK`，需要实现连接模式切换逻辑
- `SoAd_ReleaseIpAddrAssignment()` 和 `SoAd_RequestIpAddrAssignment()` 为桩实现
- `SoAd_RxIndication()` 回调内处理逻辑为空，需要实现 PDU 头解析和上层通知
- `SoAd_TxConfirmation()`、`SoAd_TcpIpEvent()`、`SoAd_LocalIpAddrAssignmentChg()` 为空实现
- `SoAd_IfTransmit()` 函数原型已声明但未实现
- 连接组（Connection Group）管理逻辑尚未完整实现

---

## 14. 参考资料

- AUTOSAR SWS SocketAdaptor (AUTOSAR_SWS_SocketAdaptor.pdf)
- AUTOSAR SWS TcpIp
- RFC 793 (TCP), RFC 768 (UDP)
- 源码: `src/bsw/services/soad/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_SoAd_00020 | `soad_trigger_transmit` | 测试 soad_trigger_transmit 覆盖: soad_trigger_transmit 场景 |
| SWS_SoAd_00101 | `SoAd_ModuleConstants_Exist` | 测试 test_SoAd_ModuleConstants_Exist 覆盖: SoAd_ModuleConstants_Exist 场景 |
| SWS_SoAd_00102 | `SoAd_ServiceIDs_Exist` | 测试 test_SoAd_ServiceIDs_Exist 覆盖: SoAd_ServiceIDs_Exist 场景 |
| SWS_SoAd_00103 | `SoAd_ErrorCodes_Exist` | 测试 test_SoAd_ErrorCodes_Exist 覆盖: SoAd_ErrorCodes_Exist 场景 |
| SWS_SoAd_00104 | `SoAd_ConnStateTypes_Exist` | 测试 test_SoAd_ConnStateTypes_Exist 覆盖: SoAd_ConnStateTypes_Exist 场景 |
| SWS_SoAd_00105 | `SoAd_ProtocolTypes_Exist` | 测试 test_SoAd_ProtocolTypes_Exist 覆盖: SoAd_ProtocolTypes_Exist 场景 |
| SWS_SoAd_00106 | `SoAd_ConnModeRequestTypes_Exist` | 测试 test_SoAd_ConnModeRequestTypes_Exist 覆盖: SoAd_ConnModeRequestTypes_Exist 场景 |
