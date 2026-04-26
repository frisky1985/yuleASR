# 第5章: 调试指南

**目标**: 提供日志、跟踪和常见问题排查方法。

---

## 5.1 日志系统

### 5.1.1 日志级别

ETH-DDS提供五级日志:

| 级别 | 值 | 描述 |
|------|-----|------|
| TRACE | 0 | 详细追踪信息 |
| DEBUG | 1 | 调试信息 |
| INFO | 2 | 一般信息 |
| WARN | 3 | 警告信息 |
| ERROR | 4 | 错误信息 |
| FATAL | 5 | 致命错误 |

### 5.1.2 配置日志

```c
#include "dds/dds.h"

// 设置日志级别
dds_set_log_mask(DDS_LC_ERROR | DDS_LC_WARNING | DDS_LC_INFO);

// 设置日志输出回调
dds_set_log_sink(my_log_sink, NULL);
```

### 5.1.3 环境变量配置

```bash
# 设置日志级别
export DDS_LOG_LEVEL=DEBUG

# 设置日志输出
export DDS_LOG_FILE=/var/log/dds.log
export DDS_LOG_FILE_MAX_SIZE=100MB
export DDS_LOG_FILE_MAX_FILES=10

# 启用特定模块日志
export DDS_LOG_MODULES=dds,security,transport
```

### 5.1.4 自定义日志处理器

```c
void my_log_sink(void *arg, const dds_log_data_t *data)
{
    const char *level_str[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
    };
    
    printf("[%s] %s:%d - %s\n",
           level_str[data->level],
           data->file,
           data->line,
           data->message);
}

dds_set_log_sink(my_log_sink, NULL);
```

## 5.2 跟踪系统

### 5.2.1 DDS跟踪

DDS内置跟踪功能记录关键事件:

```c
#include "dds/dds.h"

dds_entity_t participant;
dds_entity_t trace_writer;

// 创建跟踪写入器
trace_writer = dds_create_writer(participant, trace_topic, NULL, NULL);

// 启用内部跟踪
dds_enable_tracing(participant, DDS_TRACE_ALL);

// 设置跟踪回调
dds_set_trace_callback(trace_callback, NULL);
```

### 5.2.2 跟踪类型

```c
编码typedef enum {
    DDS_TRACE_ENTITY_CREATE,    // 实体创建
    DDS_TRACE_ENTITY_DELETE,    // 实体删除
    DDS_TRACE_WRITE,            // 写入数据
    DDS_TRACE_READ,             // 读取数据
    DDS_TRACE_DISCOVERY,        // 发现事件
    DDS_TRACE_QOS_CHANGE,       // QoS变更
    DDS_TRACE_SECURITY,         // 安全事件
    DDS_TRACE_TRANSPORT,        // 传输层事件
    DDS_TRACE_ALL               // 所有事件
} dds_trace_event_t;
```

### 5.2.3 运行时追踪

```bash
# 启用所有追踪
export DDS_TRACE=1
export DDS_TRACE_MASK=ALL

# 追踪特定模块
export DDS_TRACE_MASK=DISCOVERY,SECURITY

# 输出追踪到文件
export DDS_TRACE_FILE=/tmp/dds_trace.log

# 追踪格式
export DDS_TRACE_FORMAT=JSON
```

## 5.3 网络调试

### 5.3.1 网络连接检查

```bash
# 检查DDS使用的端口
netstat -tuln | grep 7410

# 检查多播组成员
ip maddr show

# 检查路由
ip route get 239.255.0.1

# 使用tcpdump抓包
sudo tcpdump -i eth0 port 7410 -w dds_traffic.pcap
```

### 5.3.2 RTPS协议分析

```bash
# 启用RTPS日志
export DDS_RTPS_TRACE=1

# 详细RTPS追踪
export DDS_RTPS_TRACE_LEVEL=DEBUG

# 追踪发现过程
export DDS_DISCOVERY_TRACE=1
```

### 5.3.3 网络诊断工具

```bash
# 使用DDS内置诊断工具
./tools/dds_network_diag --interface eth0

# 检查连接状态
./tools/dds_network_diag --check-connectivity

# 测试延迟
./tools/dds_network_diag --latency-test --target 192.168.1.100

# 测试带宽
./tools/dds_network_diag --bandwidth-test --duration 10s
```

## 5.4 常见问题排查

### 5.4.1 连接问题

**现象**: 发布者和订阅者无法发现彼此

**检查清单**:

1. 检查网络连接
```bash
ping <remote_ip>
telnet <remote_ip> 7410
```

2. 检查多播配置
```bash
# 检查多播地址
ip maddr show

# 检查IGMP加入
ip igmp show
```

3. 检查防火墙
```bash
# 检查iptables规则
sudo iptables -L -v -n

# 允许DDS端口
sudo iptables -A INPUT -p udp --dport 7410:7420 -j ACCEPT
```

4. 检查DDS配置
```bash
# 确认域ID一致
echo $DDS_DOMAIN_ID

# 确认主题名称匹配
```

### 5.4.2 性能问题

**现象**: 延迟高、吞吐量低

**检查清单**:

1. 检查资源使用
```bash
# CPU使用率
top -p $(pgrep -d',' dds)

# 内存使用
free -h
cat /proc/$(pgrep dds)/status | grep VmRSS

# 网络使用
iftop -i eth0
```

2. 检查DDS统计信息
```c
// 获取写入器统计
dds_writer_stats_t stats;
dds_get_writer_stats(writer, &stats);
printf("Sent: %lu, Dropped: %lu, Latency: %lu us\n",
       stats.messages_sent,
       stats.messages_dropped,
       stats.avg_latency_us);
```

3. 优化建议
- 增加发送缓冲区大小
- 调整QoS历史策略
- 使用共享内存传输

### 5.4.3 内存泄漏

**现象**: 内存使用持续增长

**检查方法**:

1. 使用valgrind
```bash
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes ./my_dds_app
```

2. 使用地址侦测
```bash
# 编译时启用ASan
export CFLAGS="-fsanitize=address -fno-omit-frame-pointer"
export LDFLAGS="-fsanitize=address"
```

3. DDS内存监控
```c
// 获取DDS内存统计
dds_memory_stats_t mem_stats;
dds_get_memory_stats(&mem_stats);
printf("Total: %zu, Used: %zu, Free: %zu\n",
       mem_stats.total,
       mem_stats.used,
       mem_stats.free);
```

### 5.4.4 安全认证失败

**现象**: 安全连接建立失败

**检查清单**:

1. 检查证书
```bash
# 验证证书
openssl x509 -in cert.pem -text -noout

# 检查证书有效期
openssl x509 -in cert.pem -dates -noout

# 验证证书链
openssl verify -CAfile ca.pem cert.pem
```

2. 检查权限配置
```bash
# 确认权限文件存在
ls -la acl/permissions.xml

# 验证权限文件
./tools/dds_security --validate-permissions acl/permissions.xml
```

3. 查看安全日志
```bash
# 启用详细安全日志
export DDS_SECURITY_LOG=DEBUG
```

## 5.5 调试工具

### 5.5.1 DDS监控器

```bash
# 启动图形化DDS监控器
./tools/dds_monitor

# 监控特定域
./tools/dds_monitor --domain 0

# 监控特定主题
./tools/dds_monitor --topic VehicleStatus
```

### 5.5.2 数据追踪器

```bash
# 抓取所有DDS数据
./tools/dds_sniffer --interface eth0 --output capture.dds

# 过滤特定主题
./tools/dds_sniffer --topic VehicleStatus

# 实时显示
./tools/dds_sniffer --live
```

### 5.5.3 性能分析器

```bash
# 运行性能测试
./tools/dds_perf --mode throughput --duration 60
./tools/dds_perf --mode latency --samples 10000

# 生成性能报告
./tools/dds_perf --report --output perf_report.html
```

## 5.6 调试技巧

### 5.6.1 核心转储分析

```bash
# 生成核心转储
ulimit -c unlimited
./my_dds_app
# 程序崩溃后生成core文件

# 分析核心转储
gdb ./my_dds_app core
(gdb) bt        # 查看调用栈
(gdb) info threads  # 查看所有线程
```

### 5.6.2 线程分析

```bash
# 查看DDS线程
ps -T -p $(pgrep dds)

# 追踪线程
strace -f -e trace=network ./my_dds_app

# 线程调试
gdb -p $(pgrep dds)
(gdb) info threads
(gdb) thread apply all bt
```

### 5.6.3 内存调试

```c
// 在代码中添加内存检查
#include <malloc.h>

void check_memory(void) {
    struct mallinfo mi = mallinfo();
    printf("Total allocated: %d bytes\n", mi.uordblks);
    printf("Total free: %d bytes\n", mi.fordblks);
}
```

---

**版本**: 2.0.0  
**上一章**: @ref configuration  
**下一章**: @ref deployment
