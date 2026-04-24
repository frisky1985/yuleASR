# 温度传感器示例项目

## 简介

本示例展示了如何使用ETH-DDS Integration库构建一个完整的温度监控应用。该应用包含一个模拟温度传感器，通过DDS协议发布温度和湿度数据，以及一个订阅者接收并处理这些数据。

## 功能特性

- **温度发布者**
  - 模拟温度传感器数据采集
  - 可配置的发布间隔
  - 支持自定义传感器ID和位置信息
  - 温度和湿度数据同步发布

- **温度订阅者**
  - 实时接收温度数据
  - 高/低温度报警
  - 移动平均温度统计
  - 详细的数据统计信息

## 编译和运行

### 使用CMake构建

```bash
# 进入项目根目录
cd /path/to/eth-dds-integration

# 创建构建目录
mkdir build && cd build

# 配置项目（启用示例）
cmake -DBUILD_EXAMPLES=ON ..

# 构建
make -j$(nproc)
```

### 运行发布者

```bash
# 运行发布者
./examples/temperature_sensor/temp_sensor_publisher

# 查看帮助
./examples/temperature_sensor/temp_sensor_publisher --help

# 使用自定义参数
./examples/temperature_sensor/temp_sensor_publisher \
    --interval 2000 \
    --sensor-id 0x0002 \
    --location "Server_Room"
```

### 运行订阅者

```bash
# 运行订阅者
./examples/temperature_sensor/temp_sensor_subscriber

# 查看帮助
./examples/temperature_sensor/temp_sensor_subscriber --help

# 显示统计信息
./examples/temperature_sensor/temp_sensor_subscriber --stats
```

## 数据格式

### 温度数据结构

```c
typedef struct {
    uint32_t sensor_id;           /* 传感器ID */
    float temperature;            /* 温度值(摄氏度) */
    float humidity;               /* 湿度百分比 */
    uint64_t timestamp;           /* 时间戳(ms) */
    uint32_t sequence_num;        /* 序列号 */
    char location[32];            /* 位置信息 */
} temperature_data_t;
```

### DDS主题配置

- **Topic Name**: `TemperatureData`
- **Domain ID**: 0
- **QoS**: Reliable, Transient Local

## 命令行参数

### 发布者参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-i, --interval <ms>` | 发布间隔(毫秒) | 1000 |
| `-s, --sensor-id <id>` | 传感器ID(16进制) | 0x0001 |
| `-l, --location <loc>` | 传感器位置 | Room_101 |
| `-h, --help` | 显示帮助信息 | - |

### 订阅者参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `--stats` | 退出时显示统计 | false |
| `--high-temp <temp>` | 高温报警阈值 | 28.0°C |
| `--low-temp <temp>` | 低温报警阈值 | 15.0°C |
| `-h, --help` | 显示帮助信息 | - |

## 输出示例

### 发布者输出

```
========================================
  Temperature Sensor Publisher
========================================
Sensor ID:  0x0001
Location:   Room_101
Interval:   1000 ms
DDS Topic:  TemperatureData
Domain ID:  0
----------------------------------------
Press Ctrl+C to stop

[Seq:0000] Sensor 0001 | Location: Room_101    | Temp:  22.53°C | Humidity:  54.2% | Time: 1713987600000
[Seq:0001] Sensor 0001 | Location: Room_101    | Temp:  23.12°C | Humidity:  53.8% | Time: 1713987601000
[Seq:0002] Sensor 0001 | Location: Room_101    | Temp:  24.05°C | Humidity:  53.1% | Time: 1713987602000
...

Publisher stopped.
Total samples published: 100
```

### 订阅者输出

```
========================================
  Temperature Sensor Subscriber
========================================
DDS Topic:       TemperatureData
Domain ID:       0
High threshold:  28.0°C
Low threshold:   15.0°C
----------------------------------------
Press Ctrl+C to stop

[Seq:0000] Sensor 0001 | Location: Room_101    | Temp:  22.53°C | Humidity:  54.2% | Time: 1713987600000
[Seq:0001] Sensor 0001 | Location: Room_101    | Temp:  23.12°C | Humidity:  53.8% | Time: 1713987601000
[Seq:0002] Sensor 0001 | Location: Room_101    | Temp:  27.50°C | Humidity:  52.5% | Time: 1713987602000
  [ALARM] High temperature! 27.50°C exceeds threshold 28.0°C
...

----------------------------------------
  Statistics Report
----------------------------------------
  Total samples:    100
  Min temperature:  18.23°C
  Max temperature:  28.50°C
  Avg temperature:  23.45°C
  Moving average:   23.48°C (100 samples)
  Alarms triggered: 2
----------------------------------------

Subscriber stopped.
```

## 扩展开发

### 添加真实传感器支持

要将模拟传感器替换为真实传感器，需要修改以下函数：

```c
// 在temp_sensor_publisher.c中

static float read_temperature(void)
{
    // 替换为真实的传感器读取
    // 示例: 读取I2C温度传感器
    float temp_celsius;
    i2c_sensor_read(&temp_celsius);
    return temp_celsius;
}

static float read_humidity(void)
{
    // 替换为真实的湿度传感器读取
    float humidity_percent;
    i2c_sensor_read_humidity(&humidity_percent);
    return humidity_percent;
}
```

### 添加更多传感器类型

可以扩展数据结构以包含更多传感器数据：

```c
typedef struct {
    uint32_t sensor_id;
    float temperature;
    float humidity;
    float pressure;           // 添加: 气压
    float light_level;        // 添加: 光照强度
    uint64_t timestamp;
    uint32_t sequence_num;
    char location[32];
} sensor_data_t;
```

## 相关文档

- [API文档](../../docs/api.md)
- [开发者指南](../../docs/developer_guide.md)
- [README](../../README.md)

## 版本历史

- **v1.0.0** (2026-04-24) - 初始版本
  - 基本的温度发布和订阅功能
  - 温度报警机制
  - 统计功能
