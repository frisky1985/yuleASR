## TcpIp 模块 — 模块审查
- 审查时间: 2026-07-22
- 审查人: 小马 (质量架构师)
- 结论: 通过
- 发现: P2 分类

### 审查范围
- 源文件: `src/bsw/services/tcpip/`
- 测试文件: `tests/unit/services/test_tcpip.c`
- 规范引用: AUTOSAR_SWS_TcpIp

### 审查项

#### ✅ 1. TCP/IP 协议栈实现
- `TcpIp.c`: 基于 lwIP 或内部轻量栈的 TCP/IP 协议栈实现
- 核心 API:
  - `TcpIp_Init()` / `TcpIp_DeInit()`: 初始化/反初始化
  - `TcpIp_Create()`: 创建 socket (AF_INET, SOCK_STREAM/SOCK_DGRAM)
  - `TcpIp_Close()`: 关闭 socket
  - `TcpIp_Bind()`: 绑定地址端口
  - `TcpIp_Listen()`: TCP 监听
  - `TcpIp_Connect()`: TCP 连接
  - `TcpIp_Send()` / `TcpIp_Recv()`: 数据收发
  - `TcpIp_MainFunction()`: 周期性处理

#### ✅ 2. 配置项支持
- `TcpIp_Cfg.h`: NumSockets (8), NumTcpPbufs (16), TcpRcvBufSize (4096), TcpSndBufSize (4096), UdpRcvBufSize (2048), EthLinkCheckIntervalMs (100)
- `TcpIp.h`: Socket 地址结构、错误码定义

#### ✅ 3. Socket 管理
- TCP socket (SOCK_STREAM): 面向连接可靠传输
- UDP socket (SOCK_DGRAM): 无连接不可靠传输
- SocketID 类型: `TcpIp_SocketIdType`，`TCPIP_SOCKETID_INVALID` 表示无效
- 地址族: `TCPIP_AF_INET` (IPv4)

#### ✅ 4. 测试覆盖
- `test_tcpip.c` (自定义测试框架): 15+ 测试用例，覆盖：
  - Init/DeInit: 有效配置、NULL 配置、双初始化、未初始化 DeInit
  - TCP socket 创建/绑定/监听/连接/关闭
  - UDP socket 创建/发送/接收/关闭
  - 边界条件: 超过最大 socket 数、无效 socket ID
  - 错误路径: 地址使用中、连接超时

#### ⚠️ 5. 发现项

| ID | 严重度 | 描述 | 位置 |
|----|--------|------|------|
| TCPIP-P2-001 | P2 | 多线程环境下的 socket 操作缺少互斥保护测试 | test_tcpip.c |
| TCPIP-P2-002 | P2 | TCP 连接超时机制的可配置性不完整，CONNECT_TIMEOUT 硬编码 | TcpIp.c: connect timeout |
| TCPIP-P2-003 | P2 | Socket 接收缓冲区满时的行为（丢弃/阻塞/通知）未文档化 | TcpIp.h |

### 总体评价
TcpIp 模块是项目中新增的 AUTOSAR TCP/IP 栈实现，核心 API 完整，单元测试覆盖 Initialization、Socket 生命周期、数据收发等主要路径。测试框架为自定义框架（带 ASSERT_EQ/ASSERT_NE 宏），测试结构良好。仅发现 P2 级多线程保护和可配置性改进项。
