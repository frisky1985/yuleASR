# 第1章: 快速入门

**目标**: 帮助开发者在15分钟内搭建环境并运行第一个DDS应用。

---

## 1.1 环境要求

### 支持的平台

| 平台 | 版本 | 状态 |
|------|------|------|
| Linux (x86_64) | Ubuntu 20.04+ | 全功能支持 |
| Linux (ARM64) | Yocto/Linux | 全功能支持 |
| FreeRTOS | v10.4+ | 核心功能支持 |
| Bare-metal | - | 核心功能支持 |

### 缺少依赖

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libpcre2-dev \
    uuid-dev \
    doxygen \
    graphviz \
    python3 \
    python3-pip

# 安装Python依赖
pip3 install -r requirements.txt
```

---

## 1.2 获取源代码

```bash
# 克隆仓库
git clone https://github.com/your-org/eth-dds-integration.git
cd eth-dds-integration

# 检出最新发布版本
git checkout v2.0.0
```

---

## 1.3 编译和安装

### 基础编译

```bash
# 创建构建目录
mkdir build && cd build

# 配置
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DENABLE_SECURITY=ON \
         -DENABLE_TSN=ON

# 编译
make -j$(nproc)

# 安装
sudo make install
```

### 编译选项

| 选项 | 说明 | 默认 |
|-----|------|------|
| `CMAKE_BUILD_TYPE` | 构建类型 (Debug/Release) | Release |
| `ENABLE_SECURITY` | 启用DDS-Security | ON |
| `ENABLE_TSN` | 启用TSN功能 | ON |
| `ENABLE_DIAGNOSTICS` | 启用诊断栈 | ON |
| `ENABLE_OTA` | 启用OTA功能 | ON |
| `ENABLE_TESTS` | 构建测试 | OFF |
| `PLATFORM` | 目标平台 (linux/freertos/baremetal) | linux |

### 交叉编译

```bash
# 针对ARM平台的交叉编译
mkdir build_arm && cd build_arm
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-arm.cmake \
         -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## 1.4 第一个DDS应用

### 步骤1: 定义数据类型

创建 `HelloWorld.idl`:

```idl
module HelloWorld {
    struct Msg {
        string message;
        long count;
    };
};
```

生成C代码:

```bash
# 使用IDL编译器
dds_idlc HelloWorld.idl
```

### 步骤2: 写发布者代码

创建 `publisher.c`:

```c
#include "dds/dds.h"
#include "HelloWorld.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    dds_entity_t participant;
    dds_entity_t topic;
    dds_entity_t writer;
    dds_return_t rc;
    HelloWorld_Msg msg;
    int count = 0;
    
    /* 创建参与者 */
    participant = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);
    if (participant < 0) {
        fprintf(stderr, "Failed to create participant: %s\n", 
                dds_strerror(participant));
        return -1;
    }
    
    /* 创建主题 */
    topic = dds_create_topic(participant, &HelloWorld_Msg_desc,
                              "HelloWorld", NULL, NULL);
    if (topic < 0) {
        fprintf(stderr, "Failed to create topic: %s\n", 
                dds_strerror(topic));
        return -1;
    }
    
    /* 创建发布者 */
    writer = dds_create_writer(participant, topic, NULL, NULL);
    if (writer < 0) {
        fprintf(stderr, "Failed to create writer: %s\n", 
                dds_strerror(writer));
        return -1;
    }
    
    printf("Publisher started. Sending messages...\n");
    
    /* 发布消息 */
    while (count < 10) {
        msg.message = "Hello, World!";
        msg.count = count;
        
        rc = dds_write(writer, &msg);
        if (rc != DDS_RETCODE_OK) {
            fprintf(stderr, "Write failed: %s\n", dds_strerror(rc));
        } else {
            printf("Sent: %s (count=%d)\n", msg.message, count);
        }
        
        dds_sleepfor(DDS_SECS(1));
        count++;
    }
    
    /* 清理 */
    rc = dds_delete(participant);
    if (rc != DDS_RETCODE_OK) {
        fprintf(stderr, "Delete failed: %s\n", dds_strerror(rc));
        return -1;
    }
    
    return 0;
}
```

### 步骤3: 写订阅者代码

创建 `subscriber.c`:

```c
#include "dds/dds.h"
#include "HelloWorld.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    dds_entity_t participant;
    dds_entity_t topic;
    dds_entity_t reader;
    dds_return_t rc;
    void *samples[MAX_SAMPLES];
    dds_sample_info_t info[MAX_SAMPLES];
    
    /* 创建参与者 */
    participant = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);
    if (participant < 0) {
        fprintf(stderr, "Failed to create participant: %s\n", 
                dds_strerror(participant));
        return -1;
    }
    
    /* 创建主题 */
    topic = dds_create_topic(participant, &HelloWorld_Msg_desc,
                              "HelloWorld", NULL, NULL);
    if (topic < 0) {
        fprintf(stderr, "Failed to create topic: %s\n", 
                dds_strerror(topic));
        return -1;
    }
    
    /* 创建订阅者 */
    reader = dds_create_reader(participant, topic, NULL, NULL);
    if (reader < 0) {
        fprintf(stderr, "Failed to create reader: %s\n", 
                dds_strerror(reader));
        return -1;
    }
    
    printf("Subscriber started. Waiting for messages...\n");
    
    /* 接收消息 */
    while (true) {
        rc = dds_take(reader, samples, info, MAX_SAMPLES, MAX_SAMPLES);
        if (rc < 0) {
            fprintf(stderr, "Take failed: %s\n", dds_strerror(rc));
            break;
        }
        
        for (int i = 0; i < rc; i++) {
            if (info[i].valid_data) {
                HelloWorld_Msg *msg = (HelloWorld_Msg *)samples[i];
                printf("Received: %s (count=%d)\n", 
                       msg->message, msg->count);
            }
        }
        
        dds_sleepfor(DDS_MSECS(100));
    }
    
    /* 清理 */
    rc = dds_delete(participant);
    if (rc != DDS_RETCODE_OK) {
        fprintf(stderr, "Delete failed: %s\n", dds_strerror(rc));
        return -1;
    }
    
    return 0;
}
```

### 步骤4: 编译和运行

```bash
# 编译
gcc -o publisher publisher.c HelloWorld.c \
    -I/usr/local/include -L/usr/local/lib -ldds -lpthread

gcc -o subscriber subscriber.c HelloWorld.c \
    -I/usr/local/include -L/usr/local/lib -ldds -lpthread

# 运行（两个终端）
./subscriber
./publisher
```

### 预期输出

```
# 订阅者终端
Subscriber started. Waiting for messages...
Received: Hello, World! (count=0)
Received: Hello, World! (count=1)
Received: Hello, World! (count=2)
...

# 发布者终端
Publisher started. Sending messages...
Sent: Hello, World! (count=0)
Sent: Hello, World! (count=1)
Sent: Hello, World! (count=2)
...
```

---

## 1.5 运行示例程序

ETH-DDS包含丰富的示例程序:

```bash
# 列出所有示例
ls examples/

# 运行示例
cd examples/hello_world
./run.sh

# 运行安全示例
cd examples/security/basic_auth
./run.sh

# 运行TSN示例
cd examples/tsn/time_triggered
./run.sh
```

### 示例列表

| 示例 | 路径 | 描述 |
|-----|-------|------|
| hello_world | examples/hello_world | 基础发布/订阅 |
| chat_room | examples/chat_room | 多对话应用 |
| throughput | examples/throughput | 吞吐量测试 |
| latency | examples/latency | 延迟测试 |
| security/basic_auth | examples/security/basic_auth | 认证和加密 |
| tsn/time_triggered | examples/tsn/time_triggered | 时间触发发送 |
| autosar/rte_integration | examples/autosar/rte_integration | AUTOSAR集成 |

---

## 1.6 常见问题

### Q1: 编译失败，提示找不到头文件

**解决方案**:
```bash
# 确认安装路径
export CPLUS_INCLUDE_PATH=/usr/local/include:$CPLUS_INCLUDE_PATH
export LIBRARY_PATH=/usr/local/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# 更新动态链接库缓存
sudo ldconfig
```

### Q2: 运行时提示"连接被拒绝"

**解决方案**:
```bash
# 检查防火墙
checkFirewall

# 确保端口未被占用
netstat -tuln | grep 7410

# 检查网络配置
ifconfig
```

### Q3: 发现失败，订阅者接不到消息

**解决方案**:
- 确认多播地址配置正确
- 检查网络连通性 (`ping`)
- 检查日志输出 (`DDS_DEBUG=1`)

---

## 1.7 下一步

现在您已经运行了第一个DDS应用，建议学习:

1. @ref concepts "概念指南" - 深入了解DDS概念
2. @ref configuration "配置指南" - 学习QoS配置
3. @ref security_guide "安全指南" - 添加安全功能

---

**版本**: 2.0.0  
**上一章**: 文档主页  
**下一章**: @subpage concepts