# Micro-DDS

轻量级DDS（Data Distribution Service）实现，专为资源受限的嵌入式MCU设计。

## 特性

- **超低内存占用**: 目标 ROM < 50KB, RAM < 16KB
- **静态内存分配**: 无动态内存分配（无malloc/free）
- **DDS标准兼容**: 支持OMG DDS标准核心API
- **QoS支持**: Reliability, Durability, Deadline, LatencyBudget, Liveliness等
- **传输层**: UDPv4支持
- **跨平台**: 可移植到多种嵌入式平台

## 快速开始

### 构建

```bash
mkdir build && cd build
cmake ..
make
```

### Hello World示例

```c
#include "microdds/microdds.h"
#include <stdio.h>

int main(void) {
    /* 初始化Micro-DDS */
    MicroDDS_init();
    
    /* 创建域参与者 */
    DDS_DomainParticipant participant = DDS_DomainParticipant_create(0, NULL);
    if (participant == NULL) {
        printf("Failed to create participant\n");
        return 1;
    }
    
    /* 创建主题 */
    DDS_Topic topic = DDS_Topic_create(participant, "HelloWorld", "string", NULL);
    if (topic == NULL) {
        printf("Failed to create topic\n");
        return 1;
    }
    
    /* 清理 */
    DDS_Topic_delete(topic);
    DDS_DomainParticipant_delete(participant);
    MicroDDS_shutdown();
    
    return 0;
}
```

## 配置选项

通过编译时宏定义配置：

| 宏 | 默认值 | 说明 |
|-----|--------|------|
| `MICRODDS_MAX_PARTICIPANTS` | 4 | 最大域参与者数量 |
| `MICRODDS_MAX_TOPICS` | 8 | 最大主题数量 |
| `MICRODDS_MAX_PUBLISHERS` | 8 | 最大发布者数量 |
| `MICRODDS_MAX_SUBSCRIBERS` | 8 | 最大订阅者数量 |
| `MICRODDS_MAX_DATA_WRITERS` | 16 | 最大数据写入器数量 |
| `MICRODDS_MAX_DATA_READERS` | 16 | 最大数据读取器数量 |
| `MICRODDS_TOPIC_NAME_MAX` | 64 | 主题名称最大长度 |
| `MICRODDS_TYPE_NAME_MAX` | 64 | 类型名称最大长度 |
| `MICRODDS_BUFFER_POOL_SIZE` | 8 | 缓冲区池大小 |
| `MICRODDS_BUFFER_SIZE` | 512 | 每个缓冲区大小 |

## 支持的QoS策略

- **Reliability**: BEST_EFFORT / RELIABLE
- **Durability**: VOLATILE / TRANSIENT_LOCAL
- **Deadline**: 截止日期策略
- **LatencyBudget**: 延迟预算策略
- **Liveliness**: AUTOMATIC / MANUAL_BY_PARTICIPANT / MANUAL_BY_TOPIC
- **History**: KEEP_LAST / KEEP_ALL
- **ResourceLimits**: 资源限制策略

## 目录结构

```
micro-dds/
├── include/microdds/    # 头文件
│   ├── microdds.h      # 主API
│   ├── types.h         # 类型定义
│   └── qos.h           # QoS策略
├── src/               # 源代码
│   ├── core/           # 核心功能
│   ├── transport/      # 传输层
│   ├── qos/            # QoS实现
│   └── utils/          # 工具函数
├── examples/          # 示例程序
├── tests/             # 测试套件
└── CMakeLists.txt   # CMake配置
```

## 版本历史

### v0.1.0 (2024-05)
- 初始版本
- 支持域参与者、主题、发布者、订阅者基本功能
- 支持基本QoS策略

## 许可证

MIT License - 详见 LICENSE 文件

## 联系方式

- 公司: 上海予乐电子科技有限公司
- 项目: YuleTech Micro-DDS
