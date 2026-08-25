# TcpIp Design Document

> **Module ID**: 0x98 (152)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_TcpIpStackIntegration  
> **Source Path**: `src/bsw/services/tcpip/`  
> **Reference Document**: `docs/modules/tcpip.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

TcpIp（TCP/IP Stack）是 AUTOSAR BSW 服务层的 TCP/IP 协议栈适配模块。该模块将 lwIP 轻量级 TCP/IP 栈封装为 AUTOSAR 标准接口，为上层 SoAd（Socket Adapter）提供套接字级别的 TCP/UDP 通信能力。

TcpIp 管理套接字表（最多 8 个套接字），实现完整的 TCP 状态机（RFC 793 的 11 状态模型），支持零拷贝 RX/TX 缓冲区模型、VLAN 配置、统计计数器等高级特性。模块同时支持原生模拟模式（无 lwIP）和 lwIP 硬件集成模式。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS TcpIpStackIntegration | 4.4.0 | TCP/IP 栈集成规范 |
| RFC 793 | - | TCP 协议规范 |
| RFC 768 | - | UDP 协议规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SoAd | 套接字适配器，调用 TcpIp API |
| 上层 | MQTT | MQTT 客户端 |
| 下层 | lwIP | 轻量级 TCP/IP 栈（可选） |
| 下层 | EthIf | 以太网接口 |
| 下层 | Det | 开发错误检测 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   SoAd / MQTT (上层调用者)           │
├─────────────────────────────────────┤
│   TcpIp (TCP/IP 适配层)              │
├─────────────────────────────────────┤
│   lwIP (TCP/IP 协议栈)               │
├─────────────────────────────────────┤
│   EthIf / Eth Driver (以太网)        │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **套接字管理**: 管理最多 8 个套接字槽位，每个槽位包含 PCB 指针、地址信息、状态机、缓冲区
- **TCP 状态机**: 实现 RFC 793 的 11 个 TCP 状态（CLOSED → LISTEN → SYN_SENT → SYN_RECEIVED → ESTABLISHED → ... → CLOSED）
- **连接状态派生**: 从 TCP 协议状态自动派生连接状态（ConnectionState）
- **RX 缓冲区**: 双模型——用户附加缓冲区（零拷贝）+ 内部池缓冲区（环形队列，深度 2）
- **TX 缓冲区**: 内部池缓冲区（1518 字节），零拷贝 TX 模型
- **VLAN 管理**: 接口级 VLAN 配置（VID、PCP、Drop Untagged）
- **统计计数器**: 13 个单调递增计数器（TxPackets、RxBytes、TcpActiveOpens 等）

### 3.3 文件结构

```
src/bsw/services/tcpip/
├── include/
│   ├── TcpIp.h         # 公共 API 与类型声明
│   └── TcpIp_Cfg.h     # 预编译配置（自动生成）
└── src/
    └── TcpIp.c          # 核心实现（lwIP 集成 + 原生模拟）
```

---

## 4. 状态机

### 4.1 TCP 协议状态机 (RFC 793)

```
                        TcpIp_Listen()
              CLOSED ──────────────────► LISTEN
                │                          │
  TcpIp_Connect()│                          │ Incoming SYN
                ▼                          ▼
            SYN_SENT ──────────────► SYN_RECEIVED
                │     SYN-ACK           │
                │                       │ ACK
                ▼                       ▼
            ESTABLISHED ◄──────────── ESTABLISHED
                │
    ┌───────────┴───────────┐
    │ Active Close          │ Passive Close
    ▼                       ▼
FIN_WAIT_1             CLOSE_WAIT
    │                       │
    ▼                       ▼
FIN_WAIT_2             LAST_ACK
    │                       │
    ▼                       │
TIME_WAIT ─────────────────┘
    │ 2MSL timeout
    ▼
  CLOSED
```

### 4.2 TCP 状态定义

| 状态 | 值 | 说明 |
|------|----|------|
| `TCPIP_TCPSTATE_CLOSED` | 0x00 | 关闭 |
| `TCPIP_TCPSTATE_LISTEN` | 0x01 | 监听 |
| `TCPIP_TCPSTATE_SYN_SENT` | 0x02 | SYN 已发送 |
| `TCPIP_TCPSTATE_SYN_RECEIVED` | 0x03 | SYN 已接收 |
| `TCPIP_TCPSTATE_ESTABLISHED` | 0x04 | 已建立 |
| `TCPIP_TCPSTATE_FIN_WAIT_1` | 0x05 | FIN 等待 1 |
| `TCPIP_TCPSTATE_FIN_WAIT_2` | 0x06 | FIN 等待 2 |
| `TCPIP_TCPSTATE_CLOSE_WAIT` | 0x07 | 关闭等待 |
| `TCPIP_TCPSTATE_CLOSING` | 0x08 | 正在关闭 |
| `TCPIP_TCPSTATE_LAST_ACK` | 0x09 | 最后 ACK |
| `TCPIP_TCPSTATE_TIME_WAIT` | 0x0A | 时间等待 |

---

## 5. 核心数据结构

### 5.1 套接字条目 `TcpIp_SocketEntryType`

```c
typedef struct {
    boolean             InUse;        // 槽位使用中
    TcpIp_SockTypeType  SockType;    // SOCK_STREAM / SOCK_DGRAM
    TcpIp_DomainType    Domain;      // AF_INET / AF_INET6
    uint16              LocalPort;   // 本地端口
    uint8               LocalAddr[16];  // 本地地址
    uint16              RemotePort;  // 远端端口
    uint8               RemoteAddr[16]; // 远端地址
    boolean             IsConnected; // 已连接标志
    void*               Pcb;         // lwIP PCB 指针
    TcpIp_TcpStateType  TcpState;    // TCP 协议状态
    TcpIp_ConnectionStateType ConnState; // 连接状态
    uint8               Backlog;     // 监听 backlog
    boolean             CloseInProgress; // 关闭进行中
    // RX 缓冲区
    uint8*              RxUserBuf;   // 用户附加缓冲区
    uint16              RxUserCapacity;
    uint8               RxPool[2][1518]; // 内部池缓冲区
    uint8               RxHead, RxTail, RxCount;
    // TX 缓冲区
    uint8               TxBuf[1518]; // 内部 TX 缓冲区
    uint16              TxLen;
    // 选项
    boolean TcpReuseAddr, TcpKeepAlive, TcpNoDelay;
    uint16  TcpMaxSeg;
    uint8   UdpTtl, UdpTos;
} TcpIp_SocketEntryType;
```

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `TcpIp_Init(ConfigPtr)` | 0x01 | 初始化协议栈 | SWS_TcpIp_00001 |
| `TcpIp_DeInit()` | 0x02 | 反初始化 | SWS_TcpIp_00002 |
| `TcpIp_GetVersionInfo(versioninfo)` | 0x03 | 获取版本信息 | SWS_TcpIp_00003 |
| `TcpIp_Create(domain, type, SocketId)` | 0x12 | 创建套接字 | SWS_TcpIp_00004 |
| `TcpIp_Close(SocketId, Force)` | 0x13 | 关闭连接 | SWS_TcpIp_00005 |
| `TcpIp_Bind(SocketId, Addr)` | 0x26 | 绑定本地地址 | SWS_TcpIp_00006 |
| `TcpIp_Send(SocketId, Data, Length)` | 0x10 | 发送数据 | SWS_TcpIp_00007 |
| `TcpIp_Receive(SocketId, Buffer, MaxLen, ReceivedLen)` | 0x11 | 接收数据 | SWS_TcpIp_00008 |
| `TcpIp_Listen(SocketId, Backlog)` | 0x20 | 监听（TCP 服务器） | SWS_TcpIp_00009 |
| `TcpIp_Connect(SocketId, RemoteAddr)` | 0x21 | 连接远端 | SWS_TcpIp_00010 |
| `TcpIp_Accept(SocketId, NewSocketId)` | 0x22 | 接受连接 | SWS_TcpIp_00011 |
| `TcpIp_Abort(SocketId)` | 0x23 | 中止连接 | SWS_TcpIp_00012 |
| `TcpIp_SetRemoteAddr(SocketId, RemoteAddr)` | 0x24 | 设置远端地址 | SWS_TcpIp_00013 |
| `TcpIp_GetConnectionState(SocketId, ConnState)` | 0x29 | 获取连接状态 | SWS_TcpIp_00014 |
| `TcpIp_GetTcpState(SocketId, TcpState)` | 0x2A | 获取 TCP 状态 | SWS_TcpIp_00015 |
| `TcpIp_ChangeTcpState(SocketId, NewState)` | 0x2E | 驱动 TCP 状态机 | SWS_TcpIp_00016 |
| `TcpIp_SetRxBuffer(SocketId, Buffer, Capacity)` | 0x2F | 附加 RX 缓冲区 | SWS_TcpIp_00017 |
| `TcpIp_GetRxBuffer(SocketId, DataPtr, Length)` | 0x30 | 获取 RX 数据 | SWS_TcpIp_00018 |
| `TcpIp_ReleaseRxBuffer(SocketId)` | 0x31 | 释放 RX 缓冲区 | SWS_TcpIp_00019 |
| `TcpIp_GetTxBuffer(SocketId, DataPtr, Length)` | 0x32 | 获取 TX 缓冲区 | SWS_TcpIp_00020 |
| `TcpIp_ReleaseTxBuffer(SocketId, Length)` | 0x33 | 提交 TX 数据 | SWS_TcpIp_00021 |
| `TcpIp_SetTcpOption(SocketId, Option, Value)` | 0x34 | 设置 TCP 选项 | SWS_TcpIp_00022 |
| `TcpIp_SetUdpOption(SocketId, Option, Value)` | 0x36 | 设置 UDP 选项 | SWS_TcpIp_00023 |
| `TcpIp_SetVlanConfig(VlanConfigPtr)` | 0x3A | 设置 VLAN 配置 | SWS_TcpIp_00024 |
| `TcpIp_GetStatistics(StatisticsPtr)` | 0x3C | 获取统计信息 | SWS_TcpIp_00025 |
| `TcpIp_MainFunction()` | - | 主函数 | SWS_TcpIp_00026 |

### 6.2 回调函数

| 函数 | SID | 说明 | SWS 需求 |
|------|-----|------|----------|
| `TcpIp_RxIndication(SocketId, Data, Length)` | 0x38 | 数据接收入口 | SWS_TcpIp_00027 |
| `TcpIp_TxConfirmation(SocketId, Success)` | 0x39 | 发送确认 | SWS_TcpIp_00028 |

### 6.3 错误码

| 错误码 | 宏名 | 说明 |
|--------|------|------|
| 0x01 | `TCPIP_E_PARAM_POINTER` | 无效指针 |
| 0x03 | `TCPIP_E_UNINIT` | 未初始化 |
| 0x05 | `TCPIP_E_INVALID_SOCKET` | 无效套接字 |
| 0x06 | `TCPIP_E_INVALID_PROTOCOL` | 无效协议 |
| 0x0B | `TCPIP_E_BUFFER_OVERFLOW` | 缓冲区溢出 |
| 0x0F | `TCPIP_E_INVALID_STATE` | 无效状态 |
| 0x10 | `TCPIP_E_NOBUFS` | 无可用缓冲区 |
| 0x13 | `TCPIP_E_ISCONN` | 已连接 |
| 0x16 | `TCPIP_E_NOTCONN` | 未连接 |
| 0x18 | `TCPIP_E_CONNRESET` | 连接重置 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查重复初始化（DET 报错）
2. 校验 ConfigPtr 非空
3. 保存配置指针
4. 设置链路状态为 UP、接口状态为 UP、IP 地址已分配
5. 设置默认 IPv4 地址/掩码/网关
6. 清零套接字表
7. 如启用 lwIP，调用 `lwip_init()`

### 7.2 主函数处理流程

`TcpIp_MainFunction()` 周期调用（默认 10ms）:

1. 轮询链路状态
2. 推进优雅关闭序列: FIN_WAIT_1 → FIN_WAIT_2 → TIME_WAIT → CLOSED
3. 如启用 lwIP，调用 `sys_check_timeouts()` 处理 ARP/TCP 定时器

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `TCPIP_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `TCPIP_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `TCPIP_MAX_SOCKETS` | 8 | 最大套接字数 |
| `TCPIP_MAX_RX_BUFFERS` | 2 | 每套接字 RX 缓冲深度 |
| `TCPIP_TCP_RCV_BUF_SIZE` | 4096 | TCP 接收缓冲区 |
| `TCPIP_TCP_SND_BUF_SIZE` | 4096 | TCP 发送缓冲区 |
| `TCPIP_UDP_RCV_BUF_SIZE` | 2048 | UDP 接收缓冲区 |
| `TCPIP_PBUF_POOL_BUF_SIZE` | 1518 | 池缓冲区大小（含以太网帧） |
| `TCPIP_ENABLE_IPV4` | STD_ON | IPv4 使能 |
| `TCPIP_ENABLE_IPV6` | STD_OFF | IPv6 使能 |
| `TCPIP_ENABLE_TCP` | STD_ON | TCP 使能 |
| `TCPIP_ENABLE_UDP` | STD_ON | UDP 使能 |
| `TCPIP_VLAN_SUPPORT` | STD_ON | VLAN 支持 |
| `TCPIP_ENABLE_STATISTICS` | STD_ON | 统计计数器 |

### 8.2 链接时配置

通过 `TcpIp_ConfigType` 结构体在链接时配置。

### 8.3 构建后配置

不支持构建后配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 入口进行初始化状态检查和指针非空检查。套接字操作额外检查套接字 ID 有效性。

### 9.2 DEM 错误

模块不直接报告 DEM 事件。

### 9.3 安全机制

- **TCP 状态转换验证**: `TcpIp_LocalIsValidTransition()` 验证每次状态变更的合法性
- **缓冲区溢出保护**: RX 数据长度不超过池缓冲区大小
- **Backlog 限制**: 监听连接的挂起连接数不超过 `TCPIP_MAX_PENDING_CONNECTIONS`
- **编译时版本检查**: `#error` 宏确保 AR 版本一致性

---

## 10. 内存与性能

### 10.1 MemMap 分区

静态分配，通过 `static` 变量管理。

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| 每套接字 RAM | ~3.5 KB | SocketEntryType（含缓冲区） |
| 8 套接字总 RAM | ~28 KB | 套接字表 |
| 模块状态 RAM | ~200 bytes | InternalStateType |
| ROM（代码） | ~12 KB | 完整 API 实现 |

---

## 11. 集成指南

1. **lwIP 集成**: 定义 `TCPIP_ENABLE_LWIP` 宏启用 lwIP 后端
2. **SoAd 集成**: SoAd 通过 `TcpIp_Create/Send/Receive` 进行数据传输
3. **EthIf 集成**: lwIP 通过 EthIf 发送/接收以太网帧
4. **EcuM 集成**: 在启动阶段调用 `TcpIp_Init()`
5. **SchM 集成**: 配置 `TcpIp_MainFunction()` 调用周期（10ms）

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| 套接字生命周期 | Create → Bind → Listen → Connect → Close |
| TCP 状态机 | 所有合法状态转换路径验证 |
| 非法状态转换 | 验证无效转换返回 `TCPIP_E_INVALID_STATE` |
| 零拷贝 RX | SetRxBuffer → RxIndication → GetRxBuffer → ReleaseRxBuffer |
| 零拷贝 TX | GetTxBuffer → 填充数据 → ReleaseTxBuffer |
| VLAN 配置 | SetVlanConfig/GetVlanConfig 验证 |
| 统计计数器 | 操作后验证计数器递增正确 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| TcpIp-SoAd 集成 | TCP/UDP 端到端数据传输 |
| lwIP 集成 | 真实网络环境下的协议栈功能 |
| 多连接并发 | 8 个套接字同时工作的稳定性 |

---

## 13. 实现说明 / TODO

- 单接口平台偏差: `TcpIp_GetIPv4Addr/GetIPv6Addr/GetLinkState` 不接受 IfIdx 参数
- `TcpIp_Close` 关闭连接同时释放套接字槽位（yuleASR 特有行为）
- 原生模拟模式下 TCP 连接同步完成（无真实三次握手）
- IPv6 支持当前为桩实现（返回 `TCPIP_E_NOT_SUPPORTED`）
- DHCP/DNS 功能已禁用（`TCPIP_ENABLE_DHCP = STD_OFF`）

---

## 14. 参考资料

- AUTOSAR SWS TcpIpStackIntegration (AUTOSAR_SWS_TcpIpStackIntegration.pdf)
- RFC 793 (Transmission Control Protocol)
- RFC 768 (User Datagram Protocol)
- lwIP Documentation
- 源码: `src/bsw/services/tcpip/`
