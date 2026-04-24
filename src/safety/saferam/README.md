# SafeRAM - 安全RAM保护系统

## 概述

SafeRAM是一个符合AUTOSAR R22-11和ISO 26262 ASIL-D标准的安全RAM保护系统，专为车载场景设计。

## 组件

### 1. RAM分区保护 (ram_partition.c/h)
- RAM分区管理（Stack/Heap/Static/BSS）
- 分区间隔离（Guard Zone）
- 红色/黄色/绿色分区安全级别
- MPU/MMU整合支持
- 分区权限控制

### 2. 堆栈保护 (stack_protection.c/h)
- 栈溢出检测（Stack Canary）
- 栈深度监控
- 任务栈检查
- 中断栈监视
- 水印监控

### 3. 堆保护 (heap_protection.c/h)
- 堆内存验证
- 内存块重叠检测
- 释放后清零（Zero-on-Free）
- 双重释放检测
- 缓冲区溢出/下溢检测
- 内存泄漏检测

### 4. 安全数据结构 (safe_data.c/h)
- 一致性校验（CRC-8/16/32）
- 反相数据存储（1-out-of-2）
- 多份冗余（2-out-of-3投票）
- 时间戳监视
- 序列号验证
- 数据新鲜度检查

### 5. 内存故障注入 (fault_injection.c/h)
- ECC错误注入
- 位翻转注入
- 越界访问测试
- 漂移故障检测
- 安全机制测试

### 6. SafeRAM管理器 (saferam_manager.c/h)
- 初始化验证
- 定期自检
- 错误回调
- 安全计数器
- 诊断信息记录

## 编译

```bash
mkdir build
cd build
cmake ..
make
```

## 使用示例

### 基本初始化

```c
#include "saferam.h"

void main(void) {
    // 配置SafeRAM
    SafeRamConfigType config;
    memset(&config, 0, sizeof(config));
    
    config.flags = SAFERAM_FLAG_ENABLE_PARTITIONS |
                   SAFERAM_FLAG_ENABLE_STACK |
                   SAFERAM_FLAG_ENABLE_HEAP |
                   SAFERAM_FLAG_ENABLE_PERIODIC |
                   SAFERAM_FLAG_ENABLE_COUNTERS |
                   SAFERAM_FLAG_ASIL_D;
    
    config.asil_level = ASIL_D;
    config.periodic_interval_ms = 1000;
    config.response_warning = SAFERAM_RESPONSE_LOG;
    config.response_error = SAFERAM_RESPONSE_NOTIFY;
    config.response_critical = SAFERAM_RESPONSE_SAFE_STATE;
    
    // 初始化
    SafeRam_Init(&config);
    SafeRam_Start();
    
    // 主循环
    while (1) {
        SafeRam_PeriodicTask();
        // 应用代码...
    }
}
```

### 使用快速初始化宏

```c
#include "saferam.h"

void main(void) {
    SAFERAM_INIT_DEFAULT();
    SafeRam_Start();
    
    while (1) {
        SAFERAM_CHECK();
        
        if (!SAFERAM_IS_HEALTHY()) {
            // 处理错误
        }
    }
}
```

### 注册错误回调

```c
void MyErrorHandler(uint8_t severity, uint8_t source, 
                    uint32_t error_code, const SafeRamErrorRecordType *record) {
    // 处理错误
    printf("SafeRAM Error: severity=%d, code=0x%X\n", severity, error_code);
}

void main(void) {
    SafeRam_Init(&config);
    SafeRam_RegisterErrorCallback(MyErrorHandler);
    // ...
}
```

### RAM分区注册

```c
RamPartitionConfigType part_config;
memset(&part_config, 0, sizeof(part_config));

strncpy(part_config.name, "TaskStack", 31);
part_config.type = RAM_PARTITION_TYPE_STACK;
part_config.safety_level = RAM_PARTITION_LEVEL_RED;
part_config.start_addr = 0x20000000;
part_config.size = 0x00010000;
part_config.permissions = RAM_PERM_READ | RAM_PERM_WRITE;
part_config.enable_guard_zones = TRUE;

uint8_t partition_id;
SafeRam_RegisterPartition(&part_config, &partition_id);
```

### 堆栈保护注册

```c
StackConfigType stack_config;
memset(&stack_config, 0, sizeof(stack_config));

strncpy(stack_config.name, "MainTask", 15);
stack_config.type = STACK_TYPE_TASK;
stack_config.base_addr = 0x20010000;
stack_config.size = 0x00010000;
stack_config.enable_canary = TRUE;
stack_config.enable_watermark = TRUE;
stack_config.warning_threshold = 80;
stack_config.critical_threshold = 90;

uint8_t stack_id;
SafeRam_RegisterStack(&stack_config, &stack_id);
```

### 安全数据使用

```c
SafeDataElementType safe_value;
SafeDataConfigType data_config;

memset(&data_config, 0, sizeof(data_config));
data_config.data_type = SAFE_DATA_TYPE_UINT32;
data_config.redundancy_mode = SAFE_DATA_REDUNDANCY_INV;
data_config.crc_type = SAFE_DATA_CRC_32;
data_config.max_age_ms = 5000;

SafeData_InitElement(&safe_value, &data_config);

// 写入数据
uint32_t value = 42;
SafeData_WriteElement(&safe_value, &value);

// 读取并验证数据
SafeDataValidationType result;
SafeData_ReadElement(&safe_value, &value, &result);

if (result.is_valid) {
    // 数据有效
} else {
    // 数据损坏或过期
}
```

### 故障注入测试

```c
// 配置故障注入
FaultInjectionConfigType fault_config;
memset(&fault_config, 0, sizeof(fault_config));

fault_config.mode = FAULT_INJ_MODE_SINGLE_SHOT;
fault_config.fault_type = FAULT_TYPE_BIT_FLIP;
fault_config.target_addr = 0x20000000;
fault_config.bit_position = 5;

FaultInjection_Configure(&fault_config);

// 触发故障注入
FaultInjectionResultType result;
FaultInjection_Trigger(&result);

if (result.success) {
    printf("Fault injected at 0x%X, original=0x%X, corrupted=0x%X\n",
           result.fault_addr, result.original_value, result.corrupted_value);
}
```

## 文件列表

- `saferam.h` - 主头文件，包含所有组件
- `saferam_manager.h/c` - SafeRAM管理器
- `ram_partition.h/c` - RAM分区保护
- `stack_protection.h/c` - 堆栈保护
- `heap_protection.h/c` - 堆保护
- `safe_data.h/c` - 安全数据结构
- `fault_injection.h/c` - 故障注入测试
- `CMakeLists.txt` - 构建配置
- `README.md` - 本文档

## 安全特性

### ASIL-D合规性
- 端到端保护
- 安全监控
- 故障检测和响应
- 诊断信息记录
- 安全计数器

### 内存保护
- MPU/MMU集成支持
- 分区隔离
- 权限控制
- 边界检查

### 故障处理
- 分级错误响应（Warning/Error/Critical/Fatal）
- 可配置故障响应策略
- 安全状态进入
- 系统复位控制

## 许可证

Copyright (c) 2024 - AUTOSAR R22-11 compliant, ISO 26262 ASIL-D Safety Level
