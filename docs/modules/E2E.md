# E2E (End-to-End Protection) 使用手册

## 概述

E2E保护库提供端到端的数据保护机制，确保通信数据的完整性和时序性。支持多种保护策略(Profile)，满足不同的安全需求。

## 功能特点

| Profile | CRC长度 | 计数器 | 数据ID | 适用场景 |
|---------|---------|---------|---------|---------|
| Profile 1 | CRC8 | 4位 | 2字节 | 标准CAN数据 |
| Profile 2 | CRC8 | 4位 | 1字节 | 多数据ID |
| Profile 4 | CRC16 | 16位 | 4字节 | 大数据量 |
| Profile 5 | CRC32 | 8位 | 4字节 | 最高安全级别 |
| Profile 7 | CRC64 | 32位 | 4字节 | AUTOSAR Adaptive |

## 配置方法

### Profile 1 配置

```c
#include "E2E.h"

/* Profile 1 配置 */
E2E_P01ConfigType p01Config = {
    .CRCOffset = 0,                    /* CRC位置 */
    .CounterOffset = 8,                /* 计数器位置 */
    .DataID = 0x1234,                  /* 数据ID */
    .DataIDMode = E2E_P01_DATAID_BOTH, /* ID模式 */
    .DataLength = 16,                  /* 数据长度(位) */
    .MaxDeltaCounterInit = 1           /* 最大计数器差值 */
};
```

### Profile 4 配置（推荐用于大数据）

```c
E2E_P04ConfigType p04Config = {
    .CRCOffset = 0,                    /* CRC16位置 */
    .CounterOffset = 16,               /* 计数器位置 */
    .DataID = 0x12345678,              /* 数据ID */
    .MinDataLength = 32,               /* 最小数据长度 */
    .MaxDataLength = 256,              /* 最大数据长度 */
    .MaxDeltaCounter = 1               /* 最大计数器差值 */
};
```

## API参考

### Profile 1 API

```c
/* 保护数据 */
Std_ReturnType E2E_P01Protect(
    const E2E_P01ConfigType* Config,
    E2E_P01ProtectStateType* State,
    uint8* Data
);

/* 检查数据 */
Std_ReturnType E2E_P01Check(
    const E2E_P01ConfigType* Config,
    E2E_P01CheckStateType* State,
    const uint8* Data,
    E2E_CheckResultType* CheckResult
);
```

### Profile 4 API

```c
/* 保护数据 */
Std_ReturnType E2E_P04Protect(
    const E2E_P04ConfigType* Config,
    E2E_P04ProtectStateType* State,
    uint8* Data,
    uint16 Length
);

/* 检查数据 */
Std_ReturnType E2E_P04Check(
    const E2E_P04ConfigType* Config,
    E2E_P04CheckStateType* State,
    const uint8* Data,
    uint16 Length,
    E2E_CheckResultType* CheckResult
);
```

### 检查结果

| 结果 | 值 | 说明 |
|------|-----|------|
| E2E_P0XSTATUS_OK | 0x00 | 检查通过 |
| E2E_P0XSTATUS_NONEWDATA | 0x01 | 没有新数据 |
| E2E_P0XSTATUS_WRONGCRC | 0x02 | CRC错误 |
| E2E_P0XSTATUS_WRONGSEQUENCE | 0x03 | 序列错误 |
| E2E_P0XSTATUS_ERROR | 0x07 | 一般错误 |

## 使用示例

### 基本使用（Profile 1）

```c
#include "E2E.h"

/* 配置和状态 */
E2E_P01ConfigType config;
E2E_P01ProtectStateType protectState;
E2E_P01CheckStateType checkState;

void InitE2E(void) {
    /* 配置 */
    config.CRCOffset = 0;
    config.CounterOffset = 8;
    config.DataID = 0x1234;
    config.DataIDMode = E2E_P01_DATAID_BOTH;
    config.DataLength = 16;
    config.MaxDeltaCounterInit = 1;
    
    /* 初始化状态 */
    memset(&protectState, 0, sizeof(protectState));
    memset(&checkState, 0, sizeof(checkState));
}

void SendProtectedData(void) {
    uint8 data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    /* 保护数据 */
    E2E_P01Protect(&config, &protectState, data);
    
    /* 发送数据 */
    SendData(data, 16);
}

void ReceiveAndCheckData(void) {
    uint8 data[16];
    E2E_CheckResultType result;
    
    /* 接收数据 */
    ReceiveData(data, 16);
    
    /* 检查数据 */
    E2E_P01Check(&config, &checkState, data, &result);
    
    if (result == E2E_P01STATUS_OK) {
        /* 数据有效，处理 */
        ProcessData(data);
    } else {
        /* 数据无效，处理错误 */
        HandleE2EError(result);
    }
}
```

### 高级使用（Profile 4 + 动态长度）

```c
void SendVariableLengthData(uint8* data, uint16 length) {
    E2E_P04ConfigType config;
    E2E_P04ProtectStateType state;
    
    /* 配置变长支持 */
    config.CRCOffset = 0;
    config.CounterOffset = 16;
    config.DataID = 0x12345678;
    config.MinDataLength = 32;
    config.MaxDataLength = 256;
    config.MaxDeltaCounter = 1;
    
    memset(&state, 0, sizeof(state));
    
    /* 保护变长数据 */
    E2E_P04Protect(&config, &state, data, length);
    
    SendData(data, length);
}
```

## 问题排查

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 检查失败 (WRONGCRC) | 数据损坏或配置不匹配 | 检查DataID和CRC偏移设置 |
| 序列错误 | 丢包或重复 | 检查通信链路 |
| 数据长度错误 | 数据长度不在范围内 | 检查Min/MaxDataLength设置 |
| 无新数据 | 未接收到数据 | 检查调用频率 |

## ASIL安全等级

E2E库设计用于ASIL-D级应用，具有：
- CRC错误检测
- 序列错误检测
- 重复消息检测

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-04-30 | 支持Profile 1/2/4/5 |
