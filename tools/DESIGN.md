# DDS监控诊断工具设计文档

## 概述

本文档描述车载DDS监控诊断工具的软件架构和设计决策。

## 设计目标

1. **低开销**: CPU使用<1%，内存占用<10MB
2. **实时性**: 延迟监控精度<1us
3. **可靠性**: 支持ASIL D安全等级
4. **扩展性**: 支持OTA升级和功能扩展
5. **安全性**: 完整的审计日志和故障预警

## 架构设计

### 总体架构

```
+------------------+     +------------------+     +------------------+
|   dds_monitor_cli |     |  dds_analyzer    |     |  dds_qos_monitor |
|   (统一入口)      |<--->|  (流量分析)      |     |  (QoS监控)       |
+------------------+     +------------------+     +------------------+
         |                       |                       |
         |              +------------------+             |
         +------------->|   health_check   |<------------+
                        |   (健康检查)      |
                        +------------------+
                                 |
                        +------------------+
                        |    dds_log       |
                        |   (日志系统)      |
                        +------------------+
```

### 模块设计

#### 1. 日志系统 (dds_log)

**核心特性**:
- 异步日志队列
- 分级日志过滤
- 模块级别配置
- 日志轮转管理

**数据流程**:
```
日志记录 -> 级别过滤 -> 异步队列 -> 刷新线程 -> 输出目标
                                            -> 文件
                                            -> 控制台
                                            -> 回调
```

#### 2. QoS监控 (qos_monitor)

**统计模型**:
```c
结构: 环形缓冲区 + 滑动窗口
采样: O(1) 时间复杂度
百分位: 计算 O(n log n)
刷新: 定时器触发
```

**监控指标**:
- 延迟: min/max/avg/p50/p90/p95/p99/p999/p9999
- 吞吐量: msg/s, bytes/s, Mbps
- 丢包率: ppm(parts per million)
- 抖动: 标准差、方差

#### 3. 健康检查 (health_check)

**检查项**:
```
节点状态: 心跳超时检测
主题匹配: QoS兼容性检查
连接质量: 延迟、丢包、抖动
资源使用: CPU/内存/网络
```

**状态机**:
```
HEALTHY -> DEGRADED -> UNHEALTHY -> CRITICAL -> OFFLINE
   ^____________________________________________|
```

#### 4. 流量分析 (dds_analyzer)

**报文解析**:
```
以太网帧 -> IP头 -> UDP头 -> RTPS头 -> RTPS子消息
```

**子消息类型**:
- DATA: 数据发布
- HEARTBEAT: 心跳检测
- ACKNACK: 确认机制
- GAP: 丢包通知

## 数据结构

### 日志条目
```c
typedef struct dds_log_entry {
    uint64_t            timestamp_ns;      // 纳秒级时间戳
    dds_log_level_t     level;             // 日志级别
    dds_log_type_t      type;              // 日志类型
    char                module[32];        // 模块名
    char                tag[64];           // 标签
    char                message[2048];     // 消息
    char                ecu_id[8];         // ECU ID
    char                uds_session[16];   // UDS会话
    uint32_t            sequence_num;      // 序列号
} dds_log_entry_t;
```

### QoS状态
```c
typedef struct dds_qos_status {
    dds_latency_stats_t         latency;       // 延迟统计
    dds_throughput_stats_t      throughput;    // 吞吐量
    dds_loss_stats_t            loss;          // 丢包率
    dds_jitter_stats_t          jitter;        // 抖动
    dds_availability_stats_t    availability;  // 可用性
    uint32_t                    overall_score; // 综合评分
    bool                        has_warnings;  // 警告状态
    bool                        has_errors;    // 错误状态
    bool                        has_critical;  // 严重状态
} dds_qos_status_t;
```

### 健康报告
```c
typedef struct dds_health_report {
    dds_health_status_t     overall_status;       // 总体状态
    uint32_t                overall_score;        // 总体评分
    uint64_t                uptime_seconds;       // 运行时间
    uint32_t                healthy_nodes;        // 健康节点数
    uint32_t                degraded_nodes;       // 退化节点数
    uint32_t                unhealthy_nodes;      // 不健康节点数
    uint32_t                offline_nodes;        // 离线节点数
    uint32_t                matched_topics;       // 匹配主题数
    uint32_t                unmatched_topics;     // 未匹配主题数
    uint32_t                active_alarms;        // 活动告警数
} dds_health_report_t;
```

## 线程模型

### 日志系统
```
主线程:    日志记录 -> 队列推入
刷新线程:  队列弹出 -> 输出处理
```

### QoS监控
```
主线程:    数据采集 -> 环形缓冲区
工作线程:  统计计算 -> 状态更新 -> 回调触发
```

### 健康检查
```
工作线程:  定时检查 -> 状态更新 -> 告警检测 -> 回调触发
```

### 流量分析
```
捕获线程:  pcap读取 -> 报文解析 -> 回调触发
```

## 内存管理

### 内存分配策略

| 模块 | 分配策略 | 预估内存 |
|------|----------|---------|
| dds_log | 固定大小环形缓冲区 | 2-10MB |
| qos_monitor | 动态缓冲区(可配置) | 1-5MB |
| health_check | 固定大小数组 | 1-2MB |
| dds_analyzer | 动态分配(临时) | <5MB |

### 内存对齐
- 所有结构体自然对齐
- 缓冲区按页对齐(4KB)
- 避免内存碎片

## 性能优化

### 时间复杂度

| 操作 | 时间复杂度 | 说明 |
|------|----------|-----|
| 日志记录 | O(1) | 无锁队列推入 |
| QoS采集 | O(1) | 环形缓冲区写入 |
| 百分位计算 | O(n log n) | 排序算法 |
| 健康检查 | O(n) | 遍历节点列表 |
| 报文解析 | O(n) | 线性解析 |

### 优化策略

1. **环形缓冲区**: 避免动态内存分配
2. **无锁数据结构**: 使用原子操作
3. **事件驱动**: 避免轮询
4. **批量处理**: 减少系统调用

## 安全设计

### ASIL等级支持

| ASIL等级 | 心跳超时 | 延迟阈值 | 检查频率 |
|---------|---------|----------|---------|
| QM | 500ms | 50ms | 1s |
| ASIL B | 100ms | 10ms | 100ms |
| ASIL D | 50ms | 5ms | 50ms |

### 安全机制
1. **心跳监测**: 自动检测节点失效
2. **资源限制**: 内存和CPU使用上限
3. **审计日志**: 所有关键操作记录
4. **故障隔离**: 模块独立运行

## 拓展设计

### 支持的扩展点
1. 新的监控指标
2. 自定义告警规则
3. 外部存储适配
4. 云端上报接口
5. 可视化界面支持

### 插件架构
```
核心层 <- 抽象层 <- 插件层
```

## 测试策略

### 单元测试
- 日志系统: 并发写入测试
- QoS监控: 统计准确性测试
- 健康检查: 状态转换测试
- 流量分析: 报文解析测试

### 集成测试
- 多模块协同
- 高压场景
- 安全性测试

### 车载测试
- 模拟车载环境
- 温度/震动测试
- 电磁干扰测试

## 版本历史

### v1.0.0 (当前版本)
- 初始版本发布
- 基本监控功能
- UDS支持
- ASIL B/D支持

## 参考文档

- [DDS-RTPS Specification](https://www.omg.org/spec/DDSI-RTPS/)
- [AUTOSAR Adaptive Platform](https://www.autosar.org/standards/adaptive-platform/)
- [ISO 26262](https://www.iso.org/standard/68383.html)
