# TSN协议栈 (Time-Sensitive Networking Stack)

## 概述

本项目实现了完整的IEEE 802.1 TSN协议栈，专为车载网络设计，支持高可靠性、确定性和安全性要求。

## 支持的IEEE标准

| 标准 | 模块 | 描述 |
|------|------|------|
| IEEE 802.1AS-2020 | gPTP | 精确时间协议，时钟同步精度<100ns |
| IEEE 802.1Qbv-2015 | TAS | 时间感知调度器，门控列表管理 |
| IEEE 802.1Qbu-2016 | FP | 帧抢占，快速帧优先传输 |
| IEEE 802.1Qav-2009 | CBS | 基于信用的调度器，SR Class A/B |
| IEEE 802.1Qcc-2018 | SRP | 流预留协议，MSRP实现 |
| IEEE 802.1Qca-2015 | SRP | 路径控制和体正 |

## 目录结构

```
src/tsn/
├── gptp/                  # gPTP时间同步
│   ├── gptp.h              # gPTP头文件
│   └── gptp.c              # gPTP实现
├── tas/                   # 时间感知调度器
│   ├── tas.h               # TAS头文件
│   └── tas.c               # TAS实现
├── fp/                    # 帧抢占
│   ├── frame_preemption.h  # FP头文件
│   └── frame_preemption.c  # FP实现
├── cbs/                   # 基于信用的调度器
│   ├── cbs.h               # CBS头文件
│   └── cbs.c               # CBS实现
├── srp/                   # 流预留协议
│   ├── stream_reservation.h # SRP头文件
│   └── stream_reservation.c # SRP实现
├── tests/                 # 测试程序
│   └── test_tsn_stack.c   # 测试套件
├── tsn_stack.h            # TSN栈整合头文件
├── tsn_stack.c            # TSN栈整合实现
└── CMakeLists.txt         # 构建配置
```

## 车载特性

### 性能特性
- **时钟同步精度**: < 100ns (gPTP)
- **确定性延迟**: < 1ms (TAS)
- **SR Class A**: 2ms延迟保证
- **SR Class B**: 50ms延迟保证

### 安全特性 (ASIL-D)
- 完整性检查
- 故障检测
- 带宽监控
- 时钟漂移监控

## 快速开始

### 初始化TSN栈
```c
#include "tsn/tsn_stack.h"

// 创建车载配置
tsn_stack_config_t config;
tsntsn_create_automotive_config(1000, &config);  // 1Gbps

// 初始化并启动
tsn_stack_init(&config);
tsntsn_stack_start();

// 等待时间同步
tsn_wait_for_sync(0, 5000);  // 等待5秒
```

### 发送TSN流数据
```c
uint8_t data[100] = {...};
tsn_transmit_frame(0, 0, data, sizeof(data), 3);  // 端口0，队列0，优先级3
```

## 测试结果

执行测试套件:
```bash
cd src/tsn
gcc -Wall -I. -I.. tests/test_tsn_stack.c *.c */*.c -o tsn_test -lpthread -lrt
./tsn_test
```

测试覆盖率:
- 总测试数: 83
- 通过: 81 (97.6%)
- 失败: 2

## API参考

### gPTP API
- `gptp_init()` - 初始化gPTP
- `gptp_start()` - 启动时间同步
- `gptp_get_time()` - 获取当前时间
- `gptp_run_bmc()` - 执行BMC算法

### TAS API
- `tas_init()` - 初始化TAS
- `tas_config_port()` - 配置端口
- `tas_start_scheduler()` - 启动调度器
- `tas_can_transmit()` - 检查是否可传输

### CBS API
- `cbs_init()` - 初始化CBS
- `cbs_can_transmit()` - 检查传输许可
- `cbs_update_credit()` - 更新信用
- `cbs_calc_idle_slope()` - 计算idle slope

### 帧抢占 API
- `fp_init()` - 初始化抢占模块
- `fp_send_express_frame()` - 发送快速帧
- `fp_send_preemptable_frame()` - 发送可抢占帧
- `fp_fragment_frame()` - 分片帧

### 流预留 API
- `srp_init()` - 初始化SRP
- `srp_register_talker()` - 注册Talker
- `srp_register_listener()` - 注册Listener
- `srp_reserve_bandwidth()` - 预留带宽

## 版本信息

- 版本: 1.0.0
- 日期: 2026-04-24
- 编译器: GCC 11+
- 标准: C99

## 版权

Copyright (c) 2026 Nous Research
