# yuleASR 嵌入式埋点设计方案

## 设计目标

在资源极度受限的嵌入式环境中实现高效的埋点系统：
- **RAM占用**: < 4KB (可配置)
- **CPU开销**: < 1% (典型场景)
- **零动态内存分配**: 静态分配，无堆使用
- **实时性友好**: 非阻塞写入，无锁设计
- **持久化**: 支持NvM存储关键事件

## 核心设计原则

### 1. 双缓冲环形缓冲区 (Double-Buffered Ring Buffer)

```
┌─────────────────────────────────────────────────────────────┐
│                    Telemetry RAM Layout                     │
├─────────────────────────────────────────────────────────────┤
│  Active Buffer (2KB)    │  Flush Buffer (2KB)   │  Meta    │
│  ┌─────────────────┐    │  ┌─────────────────┐  │  (256B)  │
│  │ Event 1         │    │  │ (Empty/Copied)  │  │          │
│  │ Event 2         │    │  │                 │  │          │
│  │ Event 3         │───→│  │                 │  │          │
│  │ ...             │    │  │                 │  │          │
│  └─────────────────┘    │  └─────────────────┘  │          │
│         ↑                        ↓               │          │
│    Writer (ISR/    │      Reader (Low Priority) │          │
│    Task)           │                              │          │
└────────────────────┴──────────────────────────────┴──────────┘
```

**RAM配置**: 4KB + 256B = **4.25KB**

### 2. 事件编码优化

#### 2.1 变长事件格式

```c
// 基础事件头 (2 bytes)
typedef struct {
    uint8_t  event_id;      // 事件ID (0-255)
    uint8_t  timestamp_lo;  // 时间戳低8位 (相对)
} TelEventHeader_t;

// 扩展头 (可选，2 bytes)
typedef struct {
    uint8_t  timestamp_hi;  // 时间戳高8位
    uint8_t  data_len : 4;  // 数据长度 (0-15 bytes)
    uint8_t  flags    : 4;  // 标志位
} TelEventExtHeader_t;
```

#### 2.2 事件类型与大小

| 事件类型 | ID范围 | 大小 | 说明 |
|---------|--------|------|------|
| **Instant** | 0-63 | 2B | 瞬时事件，无payload |
| **Counter** | 64-127 | 4B | 计数器事件 (8-bit value) |
| **State** | 128-191 | 6B | 状态变更 (16-bit old + new) |
| **Metric** | 192-223 | 8B | 度量值 (32-bit value + context) |
| **Trace** | 224-255 | 16B | 详细跟踪 (完整payload) |

**平均事件大小**: ~4 bytes

**缓冲区容量**: 2048B / 4B = **512 events**

### 3. 时间戳压缩

```c
// 相对时间戳 + delta编码
typedef struct {
    uint32_t base_timestamp;    // 基准时间戳 (秒级)
    uint16_t last_relative;     // 上次相对时间戳 (ms)
} TelTimeContext_t;

// 存储: base + 8-bit delta (最大255ms)
// 溢出时创建新基准点事件
```

### 4. 编译期配置 (零开销原则)

```c
// telemetry_cfg.h
#define TEL_ENABLE_MODULE_DDS       1
#define TEL_ENABLE_MODULE_ETH       1
#define TEL_ENABLE_MODULE_DIAG      0  // 禁用节省RAM
#define TEL_ENABLE_MODULE_SECOC     0

#define TEL_LEVEL_DDS               TEL_LEVEL_BASIC   // 仅关键事件
#define TEL_LEVEL_ETH               TEL_LEVEL_DETAILED // 详细事件

// 每个模块的缓冲区配置
#define TEL_DDS_BUFFER_SIZE         512   // 512B
#define TEL_ETH_BUFFER_SIZE         1024  // 1KB
#define TEL_SHARED_BUFFER_SIZE      2048  // 2KB 共享
```

## 架构设计

### 1. 分层架构

```
┌─────────────────────────────────────────────────────────────┐
│  Application Layer (ASW)                                     │
│  • Business logic events                                     │
│  • User-defined metrics                                      │
├─────────────────────────────────────────────────────────────┤
│  RTE Interface                                               │
│  • Tel_SendEvent()                                           │
│  • Tel_LogMetric()                                           │
├─────────────────────────────────────────────────────────────┤
│  Telemetry Core (Service Layer)                              │
│  • Event filtering                                           │
│  • Buffer management                                         │
│  • Compression                                               │
├─────────────────────────────────────────────────────────────┤
│  Transport Adapters                                          │
│  • DDS Publisher (zero-copy)                                 │
│  • DoIP/Diagnostic                                           │
│  • NvM Persistence                                           │
└─────────────────────────────────────────────────────────────┘
```

### 2. 核心组件

#### 2.1 事件过滤器 (编译期+运行期)

```c
// 编译期过滤 (零开销)
#if TEL_ENABLE_MODULE_DDS && TEL_LEVEL_DDS >= TEL_LEVEL_BASIC
    #define TEL_DDS_EVENT(id, ...) Tel_LogEvent(TEL_MOD_DDS, id, ##__VA_ARGS__)
#else
    #define TEL_DDS_EVENT(id, ...) ((void)0)
#endif

// 运行期过滤 (位掩码)
typedef struct {
    uint32_t module_mask;      // 模块使能位图
    uint8_t  min_level;        // 最小级别
    uint32_t event_blacklist;  // 特定事件黑名单
} TelFilterConfig_t;
```

#### 2.2 环形缓冲区管理

```c
typedef struct {
    volatile uint16_t write_idx;    // 写入索引 (volatile)
    volatile uint16_t read_idx;     // 读取索引
    uint16_t          capacity;     // 容量
    uint8_t          *buffer;       // 缓冲区指针
    uint16_t          overflow_cnt; // 溢出计数
} TelRingBuffer_t;

// 无锁写入 (单生产者)
static inline bool Tel_RB_Write(TelRingBuffer_t *rb, const uint8_t *data, uint8_t len) {
    uint16_t next_idx = (rb->write_idx + len) % rb->capacity;
    
    // 检查溢出
    if (next_idx == rb->read_idx) {
        rb->overflow_cnt++;
        return false;
    }
    
    // 写入数据
    for (uint8_t i = 0; i < len; i++) {
        rb->buffer[(rb->write_idx + i) % rb->capacity] = data[i];
    }
    
    // 内存屏障后更新索引
    __DSB();
    rb->write_idx = next_idx;
    return true;
}
```

#### 2.3 传输管理器

```c
typedef enum {
    TEL_TRANSPORT_DDS,      // 实时传输到云端
    TEL_TRANSPORT_DIAG,     // 诊断会话下载
    TEL_TRANSPORT_NVM,      // 持久化存储
    TEL_TRANSPORT_COUNT
} TelTransportType_t;

typedef struct {
    TelTransportType_t type;
    uint16_t           priority;
    uint16_t           batch_size;    // 批量传输大小
    uint32_t           interval_ms;   // 传输间隔
    bool               (*init)(void);
    bool               (*send)(const uint8_t *data, uint16_t len);
    void               (*deinit)(void);
} TelTransport_t;
```

## 模块集成设计

### 1. DDS集成 (零拷贝)

```c
// DDS Topic for telemetry
typedef struct {
    uint32_t seq_num;           // 序列号
    uint32_t timestamp;         // 时间戳
    uint16_t event_count;       // 事件数量
    uint8_t  compression;       // 压缩算法
    uint8_t  payload[1400];     // MTU优化
} TelDdsSample_t;

// 共享缓冲区避免拷贝
typedef struct {
    TelRingBuffer_t *rb;        // 指向Telemetry缓冲区
    uint16_t         read_pos;  // DDS读取位置
    TelDdsSample_t   sample;    // 预分配样本
} TelDdsPublisher_t;
```

### 2. 诊断集成 (UDS 0x22 ReadDataByIdentifier)

```c
// DID定义
#define DID_TELEMETRY_STATUS    0xF400  // 埋点状态
#define DID_TELEMETRY_CONFIG    0xF401  // 配置参数
#define DID_TELEMETRY_BUFFER    0xF402  // 缓冲区内容
#define DID_TELEMETRY_OVERFLOW  0xF403  // 溢出计数

// 诊断回调
Std_ReturnType Tel_Diag_ReadData(uint16_t did, uint8_t *data, uint16_t max_len) {
    switch (did) {
        case DID_TELEMETRY_STATUS:
            data[0] = gTelState.is_enabled;
            data[1] = gTelState.active_transports;
            return E_OK;
            
        case DID_TELEMETRY_OVERFLOW:
            data[0] = (gTelRingBuffer.overflow_cnt >> 8) & 0xFF;
            data[1] = gTelRingBuffer.overflow_cnt & 0xFF;
            return E_OK;
            
        default:
            return E_NOT_OK;
    }
}
```

### 3. NvM集成 (关键事件持久化)

```c
// 关键事件类型 (掉电保存)
typedef enum {
    TEL_EVT_FAULT_CRITICAL,     // 关键故障
    TEL_EVT_FAULT_DIAG,         // 诊断故障
    TEL_EVT_STATE_CHANGE,       // 状态变更
    TEL_EVT_COUNT               // 关键事件计数
} TelCriticalEvent_t;

// NvM存储块配置
#define TEL_NVM_BLOCK_SIZE      256   // 256B
#define TEL_NVM_MAX_EVENTS      32    // 最多32个关键事件

typedef struct {
    uint32_t magic;                     // 魔数 (0x54454C01)
    uint16_t write_idx;
    uint16_t crc;
    TelEvent_t events[TEL_NVM_MAX_EVENTS];
} TelNvmBlock_t;

// 掉电保护写入
void Tel_Nvm_WriteCritical(const TelEvent_t *event) {
    // 写入NvM (异步，写入保护)
    NvM_WriteBlock(NVM_BLOCK_ID_TELEMETRY, event);
}
```

## 事件定义规范

### 1. 事件ID分配

```c
// telemetry_events.h
// 格式: TEL_EVT_<MODULE>_<DESCRIPTION>

// System (0x00-0x0F)
#define TEL_EVT_SYS_BOOT            0x00  // 系统启动
#define TEL_EVT_SYS_SHUTDOWN        0x01  // 系统关闭
#define TEL_EVT_SYS_FAULT           0x02  // 系统故障
#define TEL_EVT_SYS_WATCHDOG        0x03  // 看门狗复位

// EcuM (0x10-0x1F)
#define TEL_EVT_ECUM_STATE_CHANGE   0x10  // 状态变更
#define TEL_EVT_ECUM_WAKEUP         0x11  // 唤醒事件

// BswM (0x20-0x2F)
#define TEL_EVT_BSWM_RULE_EVAL      0x20  // 规则评估
#define TEL_EVT_BSWM_ACTION_EXEC    0x21  // 动作执行

// DDS (0x30-0x4F)
#define TEL_EVT_DDS_DISCOVERY       0x30  // 发现完成
#define TEL_EVT_DDS_PUB_MATCH       0x31  // 发布者匹配
#define TEL_EVT_DDS_SUB_MATCH       0x32  // 订阅者匹配
#define TEL_EVT_DDS_SAMPLE_SENT     0x33  // 样本发送
#define TEL_EVT_DDS_SAMPLE_RECV     0x34  // 样本接收
#define TEL_EVT_DDS_HEARTBEAT       0x35  // 心跳事件

// Ethernet (0x50-0x6F)
#define TEL_EVT_ETH_LINK_UP         0x50  // 链路上升
#define TEL_EVT_ETH_LINK_DOWN       0x51  // 链路下降
#define TEL_EVT_ETH_TX_COMPLETE     0x52  // 发送完成
#define TEL_EVT_ETH_RX_COMPLETE     0x53  // 接收完成
#define TEL_EVT_ETH_ERROR           0x54  // 错误事件

// Security (0x70-0x7F)
#define TEL_EVT_SEC_AUTH_FAIL       0x70  // 认证失败
#define TEL_EVT_SEC_FRESHNESS_FAIL  0x71  // 新鲜值失效

// Diagnostics (0x80-0x8F)
#define TEL_EVT_DIAG_SESSION_START  0x80  // 会话开始
#define TEL_EVT_DIAG_SESSION_STOP   0x81  // 会话结束
#define TEL_EVT_DIAG_DTC_SET        0x82  // DTC设置

// OTA (0x90-0x9F)
#define TEL_EVT_OTA_START           0x90  // OTA开始
#define TEL_EVT_OTA_COMPLETE        0x91  // OTA完成
#define TEL_EVT_OTA_FAIL            0x92  // OTA失败

// User-defined (0xA0-0xFF)
#define TEL_EVT_USER_BASE           0xA0
```

### 2. 事件宏定义

```c
// 便捷宏
#define TEL_LOG_INSTANT(id) \
    Tel_LogEvent(&(TelEvent_t){.header = {id, Tel_GetTimestampLo()}})

#define TEL_LOG_COUNTER(id, value) \
    Tel_LogEvent(&(TelEvent_t){.header = {id, Tel_GetTimestampLo()}, \
                               .data = {value}})

#define TEL_LOG_STATE(id, old_state, new_state) \
    Tel_LogEvent(&(TelEvent_t){.header = {id, Tel_GetTimestampLo()}, \
                               .data = {old_state, new_state}})

#define TEL_LOG_METRIC(id, value32) \
    Tel_LogEvent(&(TelEvent_t){.header = {id, Tel_GetTimestampLo()}, \
                               .data = {(value32>>24)&0xFF, (value32>>16)&0xFF, \
                                        (value32>>8)&0xFF, value32&0xFF}})
```

## 配置示例

### 最小配置 (1KB RAM)

```c
// telemetry_minimal_cfg.h
#define TEL_ENABLE_GLOBAL           1
#define TEL_BUFFER_SIZE             768   // 768B
#define TEL_ENABLE_TIMESTAMP        1
#define TEL_ENABLE_MODULE_SYS       1
#define TEL_ENABLE_MODULE_DDS       1
#define TEL_ENABLE_MODULE_ETH       0
#define TEL_ENABLE_MODULE_DIAG      0
#define TEL_ENABLE_NVM              0
#define TEL_ENABLE_COMPRESSION      0
```

### 标准配置 (4KB RAM)

```c
// telemetry_standard_cfg.h
#define TEL_ENABLE_GLOBAL           1
#define TEL_BUFFER_SIZE             3584  // 3.5KB
#define TEL_ENABLE_TIMESTAMP        1
#define TEL_ENABLE_TIME_SYNC        1
#define TEL_ENABLE_MODULE_SYS       1
#define TEL_ENABLE_MODULE_DDS       1
#define TEL_ENABLE_MODULE_ETH       1
#define TEL_ENABLE_MODULE_DIAG      1
#define TEL_ENABLE_MODULE_SECOC     1
#define TEL_ENABLE_MODULE_OTA       1
#define TEL_ENABLE_NVM              1
#define TEL_ENABLE_COMPRESSION      1
#define TEL_COMPRESSION_ALGO        TEL_COMPRESS_RLE
```

### 完整配置 (8KB RAM)

```c
// telemetry_full_cfg.h
#define TEL_ENABLE_GLOBAL           1
#define TEL_BUFFER_SIZE             7168  // 7KB
#define TEL_ENABLE_TIMESTAMP        1
#define TEL_ENABLE_TIME_SYNC        1
#define TEL_ENABLE_HIGH_RES_TIME    1
#define TEL_ENABLE_ALL_MODULES      1
#define TEL_ENABLE_NVM              1
#define TEL_ENABLE_COMPRESSION      1
#define TEL_COMPRESSION_ALGO        TEL_COMPRESS_LZ4
#define TEL_ENABLE_REALTIME_STREAM  1
#define TEL_ENABLE_OFFLINE_ANALYSIS 1
```

## 实现计划

### Phase 1: 核心框架 (1周)
- [ ] 环形缓冲区实现
- [ ] 事件编码/解码
- [ ] 基础API设计
- [ ] 编译期配置系统

### Phase 2: 模块集成 (1周)
- [ ] DDS传输适配器
- [ ] 诊断接口集成
- [ ] NvM持久化
- [ ] 时间戳服务

### Phase 3: 优化与测试 (1周)
- [ ] 压缩算法实现
- [ ] 性能测试
- [ ] RAM使用优化
- [ ] 故障注入测试

## 预期效果

| 指标 | 目标值 | 验证方法 |
|------|--------|----------|
| RAM占用 | < 4KB | 静态分析 |
| CPU开销 | < 1% | 示波器测量 |
| 事件丢失率 | < 0.1% | 压力测试 |
| 传输延迟 | < 10ms | 网络分析 |

## 总结

本方案通过以下策略实现嵌入式友好的埋点系统：

1. **双缓冲环形缓冲区**: 避免动态分配，实现零拷贝
2. **变长事件编码**: 平均4B/事件，最大化存储效率
3. **编译期过滤**: 禁用模块零开销
4. **多模式配置**: 1KB/4KB/8KB 适应不同资源场景
5. **模块化集成**: 与DDS/诊断/NvM无缝集成

这是一个为yuleASR量身定制的埋点方案，既满足功能需求，又严格控制资源使用。
