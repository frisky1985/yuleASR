# ETH-DDS 架构设计文档

**版本**: 2.0.0  
**最后更新**: 2026-04-24

---

## 目录

1. [架构总览](#1-架构总览)
2. [层次设计](#2-层次设计)
3. [模块交互](#3-模块交互)
4. [数据流设计](#4-数据流设计)
5. [安全架构](#5-安全架构)
6. [平台适配层](#6-平台适配层)

---

## 1. 架构总览

### 1.1 系统架构图

```
╔══════════════════════════════════════════════════════════════════════════════════╗
║                        ETH-DDS Integration Framework                        ║
╚══════════════════════════════════════════════════════════════════════════════════╝

╔══════════════════════════════════════════════════════════════════════════════════╗
║                           应用层 (Application Layer)                          ║
║  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐   ║
║  │  AUTOSAR      │  │   AUTOSAR     │  │  Application │  │   Config    │   ║
║  │  Classic RTE  │  │   Adaptive    │  │   Services   │  │   Manager   │   ║
║  │              │  │   ara::com    │  │              │  │             │   ║
║  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘   ║
╚══════════════════════════════════════════════════════════════════════════════════╝
                                    │
                                    ▼
╔══════════════════════════════════════════════════════════════════════════════════╗
║                         DDS中间件层 (DDS Middleware)                         ║
║  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐   ║
║  │   Security   │  │   Advanced  │  │    Core     │  │   Runtime   │   ║
║  │   Plugin     │  │   Features  │  │   Services  │  │   Monitor   │   ║
║  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘   ║
║      │                │                │                │               ║
║      └───────────────┼───────────────┼───────────────┼───────────────┘               ║
║                        ┌───────────────┐                                   ║
║                        │   RTPS Protocol  │                                   ║
║                        └───────────────┘                                   ║
╚══════════════════════════════════════════════════════════════════════════════════╝
                                    │
                                    ▼
╔══════════════════════════════════════════════════════════════════════════════════╗
║                      传输层 (Transport Layer)                               ║
║  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐   ║
║  │   UDP/TCP    │  │    SHM       │  │    TSN      │  │  Transport │   ║
║  │   Transport  │  │ Transport   │  │ Transport  │  │   Manager  │   ║
║  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘   ║
╚══════════════════════════════════════════════════════════════════════════════════╝
                                    │
                                    ▼
╔══════════════════════════════════════════════════════════════════════════════════╗
║                     以太网驱动层 (Ethernet Driver)                        ║
║  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐   ║
║  │  MAC Driver  │  │   DMA Mgr   │  │  PHY Ctrl  │  │  Eth Mgr  │   ║
║  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘   ║
╚══════════════════════════════════════════════════════════════════════════════════╝
                                    │
                                    ▼
╔══════════════════════════════════════════════════════════════════════════════════╗
║                      平台适配层 (Platform Abstraction)                    ║
║  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐   ║
║  │    Linux    │  │  FreeRTOS   │  │ Bare-metal │  │   HAL     │   ║
║  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘   ║
╚══════════════════════════════════════════════════════════════════════════════════╝
                                    │
                                    ▼
╔══════════════════════════════════════════════════════════════════════════════════╗
║                        硬件层 (Hardware Layer)                              ║
║  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐   ║
║  │  Ethernet  │  │    PHY    │  │   Switch   │  │  Hardware │   ║
║  │   MAC     │  │  Layer   │  │   TSN    │  │  Timers   │   ║
║  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘   ║
╚══════════════════════════════════════════════════════════════════════════════════╝
```

### 1.2 设计原则

1. **分层解耦**: 每个层次通过明确定义的API交互，上层不依赖下层实现细节
2. **模块化设计**: 功能模块可独立编译、启用/禁用
3. **平台无关**: 核心代码与平台无关，通过适配层隐藏平台差异
4. **性能优先**: 关键路径零拷贝、缓存友好
5. **可测试性**: 每个模块可独立测试，支持模拟和模拟

---

## 2. 层次设计

### 2.1 应用层 (Application Layer)

应用层提供与AUTOSAR和用户应用的集成接口:

```c
// AUTOSAR Classic RTE接口
Std_ReturnType Rte_Write_SensorData(uint8_t data);
Std_ReturnType Rte_Read_ActuatorData(uint8_t* data);

// AUTOSAR Adaptive ara::com接口
ara::com::FindServiceHandle FindService(ara::com::ServiceIdentifier id);
ara::com::Future<Result> CallMethod(MethodId id, const Request& req);
```

**责任**:
- AUTOSAR标准接口实现
- 应用数据类型映射
- E2E保护集成

### 2.2 DDS中间件层 (DDS Middleware)

DDS层实现OMG DDS规范的核心功能:

```c
// DDS核心API
typedef struct {
    DDS_DomainId_t domain_id;
    DDS_QosPolicy_t qos;
} DDS_DomainParticipantConfig_t;

DDS_ReturnCode_t DDS_DomainParticipant_create(
    DDS_DomainParticipant_t** participant,
    const DDS_DomainParticipantConfig_t* config
);
```

**责任**:
- DomainParticipant管理
- Topic和数据类型支持
- QoS策略执行
- RTPS协议处理
- 发现和匹配

### 2.3 传输层 (Transport Layer)

传输层提供多种传输机制:

```c
// 传输接口抽象
typedef struct {
    const char* name;
    eth_dds_ret_t (*init)(void* config);
    eth_dds_ret_t (*send)(const void* data, size_t len, const EthAddr_t* dest);
    eth_dds_ret_t (*receive)(void* data, size_t* len, EthAddr_t* src);
    eth_dds_ret_t (*close)(void);
} transport_interface_t;
```

**责任**:
- 数据库缓冲管理
- 网络I/O操作
- 多传输适配
- 流量控制

### 2.4 以太网驱动层 (Ethernet Driver)

底层硬件控制:

```c
// MAC驱动接口
typedef struct {
    uint8_t mac_addr[6];
    uint32_t speed;      // Mbps
    uint8_t duplex;      // 0=half, 1=full
} eth_mac_config_t;

eth_dds_ret_t eth_mac_init(const eth_mac_config_t* config);
eth_dds_ret_t eth_mac_send(const eth_frame_t* frame);
eth_dds_ret_t eth_mac_receive(eth_frame_t* frame);
```

**责任**:
- MAC层控制
- DMA管理
- 中断处理
- PHY管理

---

## 3. 模块交互

### 3.1 模块间通信

```
┌───────────────────────────────────────────────────────────────────┐
│                    消息通信 (Message Passing)                           │
├───────────────────────────────────────────────────────────────────┤
│                                                                      │
│   ┌───────────┐    ┌───────────┐    ┌───────────┐    ┌───────────┐    │
│   │  AUTOSAR   │────»│    DDS     │────»│ Transport│────»│ Ethernet│    │
│   │   Layer    │    │  Middleware│    │   Layer  │    │ Driver │    │
│   └───────────┘    └───────────┘    └───────────┘    └───────────┘    │
│         «───────────────────────────────────────────────µ              │
│                        事件/回调 (Events/Callbacks)                     │
│                                                                      │
└───────────────────────────────────────────────────────────────────┘
```

### 3.2 事件通知机制

```c
// 事件回调定义
typedef enum {
    ETH_EVENT_LINK_UP,
    ETH_EVENT_LINK_DOWN,
    ETH_EVENT_RX_FRAME,
    ETH_EVENT_TX_COMPLETE,
    DDS_EVENT_PARTICIPANT_DISCOVERED,
    DDS_EVENT_DATA_AVAILABLE,
    DDS_EVENT_LIVELINESS_CHANGED,
    TSN_EVENT_SYNC_STATUS,
    TSN_EVENT_SCHEDULE_CHANGED
} eth_dds_event_type_t;

typedef struct {
    eth_dds_event_type_t type;
    void* data;
    size_t data_len;
    void* user_context;
} eth_dds_event_t;

typedef void (*eth_dds_event_callback_t)(const eth_dds_event_t* event);

eth_dds_ret_t eth_dds_register_callback(
    eth_dds_event_type_t event,
    eth_dds_event_callback_t callback,
    void* user_context
);
```

### 3.3 数据流管道

```
┌──────────────────────────────────────────────────────────────────────────────────────┐
│                              数据流管道 (Data Flow Pipeline)                          │
├──────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                       │
│   发送方:                                                                              │
│   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐  │
│   │ App Data  │─»│  Serialize │─»│   QoS     │─»│  Security │─»│  Transmit │  │
│   │  (User)    │   │   (CDR)    │   │  Policy   │   │ (Encrypt) │   │(Transport)│  │
│   └─────────────┘   └─────────────┘   └─────────────┘   └─────────────┘   └─────────────┘  │
│                                                                                       │
│   ──────────────────────────────────────────────────────────────────────────────────────────────────────────────────  │
│                                                                                       │
│   接收方:                                                                              │
│   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐  │
│   │  Receive  │─»│  Security │─»│   QoS     │─»│ Deserialize│─»│ App Data  │  │
│   │(Transport)│   │(Decrypt/  │   │  Check   │   │   (CDR)    │   │  (User)   │  │
│   └─────────────┘   │  Verify)  │   │          │   └─────────────┘   └─────────────┘  │
│                       └─────────────┘   └─────────────┘                      │
│                                                                                       │
└──────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. 数据流设计

### 4.1 数据库管理

```c
// 数据库配置
typedef struct {
    uint32_t max_samples;        // 最大样本数
    uint32_t max_instances;      // 最大实例数
    size_t max_sample_size;      // 单个样本最大大小
    eth_buffer_policy_t policy;  // 缓冲策略
} eth_buffer_pool_config_t;

// 环形缓冲区设计
typedef struct {
    volatile uint32_t head;
    volatile uint32_t tail;
    uint32_t mask;
    uint8_t* buffer;
    eth_buffer_pool_config_t config;
} eth_ring_buffer_t;
```

### 4.2 零拷贝设计

```
┌─────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                              零拷贝数据传输 (Zero-Copy)                             │
├──────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                       │
│   Writer                                    Reader                                    │
│   ┌─────────────┐                                ┌─────────────┐                      │
│   │  User     │                                │  User     │                      │
│   │  Buffer   │───────────────────────────────────────»│  Buffer   │                      │
│   └─────────────┘                                └─────────────┘                      │
│          │                                               ▲                           │
│          │                                               │                           │
│          ▼                                               │                           │
│   ┌─────────────┐                                ┌─────────────┐                      │
│   │  Loan    │───────────────────────────────────────»│ Return   │                      │
│   │  Buffer  │                                │  Buffer  │                      │
│   └─────────────┘                                └─────────────┘                      │
│          │                                               ▲                           │
│          │                                               │                           │
│          ▼                                               │                           │
│   ┌─────────────┐                                ┌─────────────┐                      │
│   │  SHM     │───────────────────────────────────────»│  SHM     │                      │
│   │  Pool    │                                │  Pool    │                      │
│   └─────────────┘                                └─────────────┘                      │
│                                                                                       │
└──────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 5. 安全架构

### 5.1 安全层次

```
╔═══════════════════════════════════════════════════════════════════════════════════╗
║                      DDS-Security 架构                                         ║
╚══════════════════════════════════════════════════════════════════════════════════╝

╔══════════════════════════════════════════════════════════════════════════════════╗
║                        应用层 (Application)                                 ║
║   DDS API调用 ──────────────────────────────────────────────────║
╚════════════════════════════════════─═════════════════════════════════════════════════════════════════════╝
                                    │
╔══════════════════════════════════════════════════════════════════════════════════╗
║                      安全插件层 (Security Plugins)                          ║
║   ┌───────────────┐   ┌───────────────┐   ┌───────────────┐                 ║
║   │Authentication│   │ Cryptography │   │Access Control│                 ║
║   │  (DDS-Auth)  │   │ (DDS-Crypto) │   │  (DDS-Access)│                 ║
║   └───────────────┘   └───────────────┘   └───────────────┘                 ║
╚══════════════════════════════════════════════════════════════════════════════════╝
                                    │
╔══════════════════════════════════════════════════════════════════════════════════╗
║                       DDS核心层 (DDS Core)                                    ║
║   RTPS协议处理 ──────────────────────────────────────────────────║
╚════════════════════════════════════─═════════════════════════════════════════════════════════════════════╝
                                    │
╔═════════════════════════════════════════════════════════════════════════════════╗
║                      安全传输层 (Secure Transport)                           ║
║   DTLS/TLS加密 ──────────────────────────────────────────────────║
╚════════════════════════════════════─════════════─────────────────────────────────────────────────═══════════════╝
```

### 5.2 E2E保护

```c
// E2E配置
typedef struct {
    e2e_profile_t profile;        // E2E Profile 1/2/...
    uint16_t data_id;             // 数据ID
    uint32_t max_delta_counter;   // 最大序列号差值
    bool crc_included;            // CRC是否包含在数据中
} e2e_config_t;

// E2E保护流程
e2e_ret_t e2e_protect(e2e_config_t* config, uint8_t* data, size_t len);
e2e_ret_t e2e_check(e2e_config_t* config, const uint8_t* data, size_t len);
```

---

## 6. 平台适配层

### 6.1 抽象层设计

```c
// 线程抽象
typedef void* eth_thread_t;
typedef struct {
    void* (*start_routine)(void*);
    void* arg;
    size_t stack_size;
    int priority;
    const char* name;
} eth_thread_attr_t;

eth_thread_t eth_thread_create(const eth_thread_attr_t* attr);
void eth_thread_join(eth_thread_t thread);
void eth_thread_sleep(uint32_t ms);

// 信号量抽象
typedef void* eth_sem_t;
eth_sem_t eth_sem_create(int initial_count);
void eth_sem_wait(eth_sem_t sem);
bool eth_sem_trywait(eth_sem_t sem, uint32_t timeout_ms);
void eth_sem_post(eth_sem_t sem);
void eth_sem_destroy(eth_sem_t sem);

// 时间抽象
uint64_t eth_get_time_ns(void);
void eth_delay_ns(uint64_t ns);
```

### 6.2 平台特定实现

| 平台 | 线程 | 同步 | 时间 | 网络 |
|------|------|------|------|------|
| Linux | pthread | pthread mutex/cond | clock_gettime | BSD socket |
| FreeRTOS | xTaskCreate | xSemaphore | xTaskGetTickCount | lwIP |
| Bare-metal | 轮询 | 中断禁用 | SysTick | 驱动直接 |

---

## 附录

### A. 编码规范

- 源文件编码: UTF-8
- 缩进: 4空格
- 命名规范: snake_case (函数/变量), UPPER_SNAKE_CASE (宏), PascalCase (类型)
- 文件命名: 小写，下划线分隔

### B. 版本管理

版本号格式: MAJOR.MINOR.PATCH
- MAJOR: 不兼容的API变更
- MINOR: 向后兼容的功能添加
- PATCH: 问题修复

---

*本文档由ETH-DDS架构团队维护*
*版本: 2.0.0*
