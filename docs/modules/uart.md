# UART 模块

## 概述

UART串口通信驱动，支持异步发收

## 功能特性

- 初始化与配置
- 核心功能API
- 错误处理

## API接口

### 初始化函数

```c
void uart_Init(const uart_ConfigType* ConfigPtr);
```

### 版本信息

```c
void uart_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

## 配置参数

| 参数名 | 类型 | 描述 |
|:------|:-----|:------|
| ConfigPtr | const uart_ConfigType* | 配置指针 |

## 使用示例

```c
#include "uart.h"

void example(void) {
    // 初始化
    uart_Init(NULL);
}
```

## 测试覆盖

- [x] 初始化测试
- [x] 参数验证

## 版本历史

| 版本 | 日期 | 描述 |
|:-----|:-----|:-----|
| 1.0 | 2026-01-09 | 初始版本 |
