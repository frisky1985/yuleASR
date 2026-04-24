# ETH-DDS 集成指南

**版本**: 2.0.0  
**最后更新**: 2026-04-24

---

## 目录

1. [环境准备](#1-环境准备)
2. [快速启动](#2-快速启动)
3. [配置详解](#3-配置详解)
4. [平台特定指南](#4-平台特定指南)
5. [常见问题](#5-常见问题)
6. [性能优化](#6-性能优化)

---

## 1. 环境准备

### 1.1 系统要求

| 组件 | 最低版本 | 推荐版本 |
|------|----------|----------|
| CMake | 3.14 | 3.20+ |
| GCC | 7.0 | 11.0+ |
| Clang | 6.0 | 14.0+ |
| Python | 3.6 | 3.10+ (用于测试) |

### 1.2 依赖安装

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libpthread-stubs0-dev \
    libssl-dev \
    python3 \
    python3-pip
```

#### CentOS/RHEL/Fedora
```bash
sudo yum install -y \
    gcc \
    gcc-c++ \
    cmake3 \
    git \
    openssl-devel \
    python3 \
    python3-pip
```

### 1.3 获取代码

```bash
# 从GitHub克隆
git clone https://github.com/eth-dds/eth-dds-integration.git
cd eth-dds-integration

# 检出特定版本
git checkout v2.0.0
```

---

## 2. 快速启动

### 2.1 基本构建

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make -j$(nproc)

# 运行测试
make test
```

### 2.2 安装到系统

```bash
# 系统级安装
sudo make install

# 更新库缓存
sudo ldconfig
```

### 2.3 简单示例

创建 `hello_eth_dds.c`:

```c
#include "eth_dds.h"
#include <stdio.h>

int main(void) {
    // 初始化
    eth_dds_config_t config = ETH_DDS_CONFIG_DEFAULT;
    config.flags = ETH_DDS_INIT_ALL;
    
    if (ETH_DDS_IS_ERROR(eth_dds_init(&config))) {
        fprintf(stderr, "Init failed: %s\n", eth_dds_get_last_error());
        return 1;
    }
    
    printf("ETH-DDS Version: %s\n", eth_dds_get_version(NULL, NULL, NULL));
    
    // 检查模块
    printf("Modules:\n");
    printf("  Ethernet: %s\n", 
           eth_dds_module_available('E') ? "Yes" : "No");
    printf("  DDS: %s\n", 
           eth_dds_module_available('D') ? "Yes" : "No");
    printf("  TSN: %s\n", 
           eth_dds_module_available('T') ? "Yes" : "No");
    
    // 清理
    eth_dds_deinit();
    return 0;
}
```

编译运行:

```bash
gcc -o hello_eth_dds hello_eth_dds.c -leth_dds -lpthread
./hello_eth_dds
```

---

## 3. 配置详解

### 3.1 CMake选项

| 选项 | 默认 | 说明 | 示例 |
|------|------|------|------|
| `CMAKE_BUILD_TYPE` | Release | 构建类型 | `Debug`, `Release`, `RelWithDebInfo` |
| `BUILD_EXAMPLES` | ON | 构建示例程序 | `-DBUILD_EXAMPLES=OFF` |
| `BUILD_TESTS` | ON | 构建测试套件 | `-DBUILD_TESTS=OFF` |
| `BUILD_SHARED_LIBS` | OFF | 构建共享库 | `-DBUILD_SHARED_LIBS=ON` |
| `ENABLE_ETHERNET` | ON | 启用Ethernet | `-DENABLE_ETHERNET=OFF` |
| `ENABLE_DDS` | ON | 启用DDS | `-DENABLE_DDS=ON` |
| `ENABLE_TSN` | ON | 启用TSN | `-DENABLE_TSN=ON` |
| `ENABLE_AUTOSAR_CLASSIC` | ON | AUTOSAR Classic | `-DENABLE_AUTOSAR_CLASSIC=ON` |
| `ENABLE_AUTOSAR_ADAPTIVE` | ON | AUTOSAR Adaptive | `-DENABLE_AUTOSAR_ADAPTIVE=ON` |
| `ENABLE_DDS_SECURITY` | ON | DDS安全 | `-DENABLE_DDS_SECURITY=ON` |
| `ENABLE_DDS_ADVANCED` | ON | DDS高级功能 | `-DENABLE_DDS_ADVANCED=ON` |

### 3.2 最小化构建

只构建DDS核心：

```bash
cmake .. \
    -DENABLE_ETHERNET=OFF \
    -DENABLE_TSN=OFF \
    -DENABLE_AUTOSAR_CLASSIC=OFF \
    -DENABLE_AUTOSAR_ADAPTIVE=OFF \
    -DENABLE_DDS_SECURITY=OFF \
    -DENABLE_DDS_ADVANCED=OFF \
    -DENABLE_DDS_RUNTIME=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TESTS=OFF
```

### 3.3 完整功能构建

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_ALL_FEATURES=ON \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_TESTS=ON
```

---

## 4. 平台特定指南

### 4.1 Linux平台

#### 实时Linux (PREEMPT_RT)

配置实时Linux支持：

```bash
cmake .. \
    -DCMAKE_C_FLAGS="-DREALTIME_LINUX" \
    -DENABLE_TSN=ON
```

#### TSN配置

启用Linux TSN子系统支持：

```bash
# 检查TSN支持
ethtool -T eth0

# 配置时间戳
sudo ptp4l -i eth0 -m
```

### 4.2 FreeRTOS平台

#### 交叉编译配置

```bash
# 设置交叉编译器
export CROSS_COMPILE=arm-none-eabi-

# 配置
cmake .. \
    -DCMAKE_SYSTEM_NAME=FreeRTOS \
    -DCMAKE_C_COMPILER=${CROSS_COMPILE}gcc \
    -DENABLE_PLATFORM_FREERTOS=ON \
    -DENABLE_PLATFORM_LINUX=OFF
```

#### FreeRTOS配置示例

```c
// FreeRTOSConfig.h 中添加
#define configUSE_ETH_DDS 1
#define configETH_DDS_STACK_SIZE 4096
```

### 4.3 裸机平台

#### 构建配置

```bash
cmake .. \
    -DCMAKE_SYSTEM_NAME=Generic \
    -DCMAKE_C_COMPILER=arm-none-eabi-gcc \
    -DENABLE_PLATFORM_BAREMETAL=ON \
    -DENABLE_PLATFORM_LINUX=OFF \
    -DBUILD_SHARED_LIBS=OFF
```

#### 启动代码

```c
#include "eth_dds.h"

void system_init(void) {
    // 硬件初始化
    hw_init();
    
    // ETH-DDS初始化
    eth_dds_config_t config = ETH_DDS_CONFIG_DEFAULT;
    config.flags = ETH_DDS_INIT_ETHERNET | ETH_DDS_INIT_DDS;
    eth_dds_init(&config);
}

void main_loop(void) {
    while (1) {
        // 处理以太网中断
        eth_mac_process();
        
        // 处理DDS事件
        dds_process_events();
    }
}
```

---

## 5. 常见问题

### 5.1 构建问题

#### 找不到头文件

**问题**: `eth_dds.h: No such file or directory`

**解决**:
```bash
# 确保安装成功
sudo make install
sudo ldconfig

# 编译时添加包含路径
gcc -I/usr/local/include ...
```

#### 链接错误

**问题**: `undefined reference to 'eth_dds_init'`

**解决**:
```bash
# 链接时指定库
gcc -o myapp myapp.c -leth_dds -lpthread

# 如果安装在非标准路径
gcc -o myapp myapp.c -L/usr/local/lib -leth_dds -lpthread -Wl,-rpath,/usr/local/lib
```

### 5.2 运行时问题

#### 权限错误

**问题**: 无法打开网络接口

**解决**:
```bash
# 使用sudo运行
sudo ./myapp

# 或设置能力
cap_net_raw,cap_net_admin=ep ./myapp
```

#### 内存不足

**问题**: `无法分配内存`

**解决**:
```bash
# 检查系统限制
ulimit -a

# 增加内存限制
ulimit -v 1048576  # 1GB
```

### 5.3 性能问题

#### 高延迟

**调试步骤**:
1. 检查CPU使用率: `top` / `htop`
2. 检查网络延迟: `ping -c 100 <target>`
3. 启用性能分析: 编译时添加 `-DENABLE_PROFILING=ON`

#### 数据丢失

**调试步骤**:
1. 检查缓冲区大小配置
2. 启用DDS可靠性QoS
3. 检查网络质量和丢包率

---

## 6. 性能优化

### 6.1 编译优化

```bash
# 高性能构建
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-O3 -march=native -flto" \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -flto" \
    -DBUILD_SHARED_LIBS=OFF
```

### 6.2 运行时优化

#### CPU亲和性

```bash
# 绑定到特定CPU
taskset -c 0 ./myapp

# 或在代码中设置
#include <sched.h>
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(0, &cpuset);
sched_setaffinity(0, sizeof(cpuset), &cpuset);
```

#### 实时优先级

```bash
# 使用chrt设置实时优先级
sudo chrt -f 99 ./myapp
```

#### 内存锁定

```c
// 锁定关键内存页
#include <sys/mman.h>
mlockall(MCL_CURRENT | MCL_FUTURE);
```

### 6.3 网络优化

#### TSN配置示例

```bash
# 配置TAS门控调度
sudo tc qdisc add dev eth0 root taprio \
    num_tc 3 \
    map 2 2 1 0 2 2 2 2 2 2 2 2 2 2 2 2 \
    queues 1@0 1@0 1@0 \
    base-time 0 \
    sched-interval 1000000 \
    flags 0x1
```

#### 网络参数调整

```bash
# 增大socket缓冲区
sudo sysctl -w net.core.rmem_max=134217728
sudo sysctl -w net.core.wmem_max=134217728

# 禁用拓展确认
sudo ethtool -K eth0 tso off gso off gro off
```

---

## 附录

### A. 完整配置文件示例

```json
{
  "eth_dds": {
    "version": "2.0.0",
    "domain": {
      "id": 0,
      "name": "default"
    },
    "dds": {
      "discovery": {
        "lease_duration_sec": 10,
        "announcement_period_sec": 5
      },
      "qos": {
        "default_reliability": "RELIABLE",
        "default_durability": "VOLATILE"
      }
    },
    "transport": {
      "udp": {
        "enabled": true,
        "port": 7400,
        "interface": "eth0"
      },
      "shm": {
        "enabled": true,
        "segment_size": 16777216
      }
    },
    "tsn": {
      "enabled": true,
      "gptp": {
        "priority1": 246,
        "priority2": 248
      }
    }
  }
}
```

### B. 调试工具

```bash
# 启用详细日志
export ETH_DDS_LOG_LEVEL=4

# 使用gdb调试
gdb ./myapp
(gdb) run
(gdb) backtrace

# 使用Valgrind检查内存
valgrind --leak-check=full ./myapp

# 网络抓包
sudo tcpdump -i eth0 -w eth_dds.pcap
```

### C. 常用命令

| 命令 | 说明 |
|------|------|
| `make clean` | 清理构建目录 |
| `make install` | 安装到系统 |
| `make uninstall` | 卸载 |
| `make package` | 生成打包文件 |
| `ctest -V` | 详细测试输出 |
| `ctest -R <pattern>` | 运行匹配的测试 |

---

*本文档由ETH-DDS项目团队维护*
*版本: 2.0.0*
