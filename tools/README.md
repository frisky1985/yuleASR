# DDS监控诊断工具集

车载DDS监控诊断工具，支持流量分析、QoS监控、健康检查和日志系统。

## 功能特性

### 1. 网络流量分析器 (dds_analyzer)
- 实时RTPS报文捕获
- 报文解析和显示
- 过滤和搜索功能
- pcap导入/导出
- 车载诊断协议(UDS)支持
- CAN桥接分析

### 2. QoS性能监控 (qos_monitor)
- 延迟统计（min/max/avg/p99/p999）
- 吞吐量监控
- 丢包率统计
- 抖动分析
- 实时图表（文本模式）
- 低开销设计（<1% CPU）

### 3. DDS健康检查 (health_check)
- 节点健康状态监控
- 主题匹配状态检测
- 连接质量检测
- 故障预警
- ASIL安全等级支持
- 资源使用监控

### 4. 日志系统 (dds_log)
- 分级日志（DEBUG/INFO/WARN/ERROR/FATAL）
- 日志轮转
- 异步日志
- 车载安全审计日志
- UDS诊断日志
- OTA日志上报支持

### 5. 监控工具CLI (dds_monitor_cli)
- 统一监控工具入口
- 实时监控模式
- 报告生成
- 交互式命令行
- JSON输出支持

## 快速开始

### 编译

```bash
cd tools
make all
```

### 安装

```bash
sudo make install
```

### 使用示例

#### 实时监控模式
```bash
dds_monitor_cli -m realtime --qos --health
```

#### 报文捕获
```bash
dds_monitor_cli -m capture -i eth0 -d 60 -o capture.pcap
```

#### 单次健康检查
```bash
dds_monitor_cli -m oneshot --health -v
```

#### 生成报告
```bash
dds_monitor_cli -m report --qos --health --json -o report.json
```

#### 交互模式
```bash
dds_monitor_cli -m interactive
```

## 命令行选项

```
Options:
  -m, --mode MODE          运行模式: realtime, oneshot, capture, report, interactive
  -i, --interface IF       网络接口名称 (默认: any)
  -o, --output FILE        输出文件
  -f, --file FILE          输入文件 (pcap)
  -d, --duration SEC       捕获/监控持续时间 (秒)
  -r, --refresh MS         刷新间隔 (毫秒, 默认: 1000)
  -t, --topic TOPIC        过滤主题名
  --filter FILTER          BPF过滤器表达式
  --qos                    启用QoS监控
  --health                 启用健康检查
  --analyzer               启用流量分析
  --json                   输出JSON格式
  -v, --verbose            详细输出
  -h, --help               显示帮助
  --version                显示版本
```

## API使用

### 日志系统

```c
#include "dds_log.h"

// 初始化日志系统
dds_log_config_t config = {
    .level = DDS_LOG_LEVEL_INFO,
    .enable_console = true,
    .enable_file = true,
    .enable_rotation = true
};
dds_log_init(&config);

// 记录日志
DDS_LOG_INFO("MODULE", "TAG", "Message: %s", "hello");
DDS_LOG_ERROR("MODULE", "TAG", "Error occurred");

// 审计日志
DDS_LOG_AUDIT(DDS_AUDIT_EVENT_CONFIG_CHANGE, "Configuration updated");

// 设置UDS会话
dds_log_set_uds_session("session123");

// 清理
dds_log_deinit();
```

### QoS监控

```c
#include "qos_monitor.h"

// 初始化
dds_qos_monitor_init(NULL);

// 创建监控器
dds_qos_obj_info_t info = {
    .type = DDS_QOS_OBJ_TOPIC,
    .name = "Temperature",
    .topic_name = "vehicle/temp"
};
dds_qos_monitor_t* monitor = dds_qos_monitor_create(&info);

// 记录数据
dds_qos_record_latency(monitor, 150);  // 150us
dds_qos_record_send(monitor, timestamp, size);
dds_qos_record_receive(monitor, timestamp, size, sequence_num);

// 获取状态
dds_qos_status_t status;
dds_qos_get_status(monitor, &status);

// 生成报告
char buffer[4096];
dds_qos_generate_text_report(monitor, buffer, sizeof(buffer));

// 清理
dds_qos_monitor_destroy(monitor);
dds_qos_monitor_deinit();
```

### 健康检查

```c
#include "health_check.h"

// 初始化
dds_health_init(NULL);

// 注册节点
dds_node_health_t node = {
    .node_name = "SensorNode",
    .guid = "node123",
    .status = DDS_HEALTH_HEALTHY,
    .asil_level = 2  // ASIL B
};
dds_health_register_node(&node);

// 更新心跳
dds_health_update_heartbeat("node123");

// 获取报告
dds_health_report_t report;
dds_health_get_report(&report);

// 清理
dds_health_deinit();
```

### 流量分析

```c
#include "dds_analyzer.h"

// 初始化
dds_analyzer_init();

// 创建捕获会话
dds_capture_config_t config = {
    .interface_name = "eth0",
    .promiscuous_mode = true
};
dds_capture_session_t* session = dds_capture_create(&config);

// 设置回调
dds_capture_set_packet_callback(session, packet_callback, NULL);
dds_capture_set_stats_callback(session, stats_callback, 1000);

// 开始捕获
dds_capture_start(session);

// 等待
dds_capture_wait(session, 60000);

// 停止
dds_capture_stop(session);
dds_capture_destroy(session);

// 清理
dds_analyzer_deinit();
```

## 车载特性

### UDS诊断支持
- 支持UDS会话跟踪
- UDS诊断事件日志
- 诊断数据解码

### OTA日志上报
- 日志自动压缩
- OTA格式导出
- 安全审计日志

### 安全等级
- ASIL D支持
- 超时检测
- 故障预警

### 性能要求
- CPU使用<1%
- 内存占用<10MB
- 延迟监控精度<1us

## 目录结构

```
tools/
├── dds_analyzer/
│   ├── dds_analyzer.h
│   ├── dds_analyzer.c
│   └── README.md
├── dds_monitor/
│   ├── qos_monitor.h
│   ├── qos_monitor.c
│   ├── health_check.h
│   ├── health_check.c
│   ├── dds_monitor_cli.c
│   └── README.md
├── common/
│   └── log/
│       ├── dds_log.h
│       └── dds_log.c
├── Makefile
└── README.md
```

## 依赖

- libpcap (报文捕获)
- zlib (日志压缩)
- pthread (线程支持)
- C99标准库

## 安全合规

- ISO 26262 ASIL D支持
- AUTOSAR兼容
- 安全审计日志
- 加密日志传输

## 许可证

Copyright (c) 2024 Automotive DDS Integration Project
