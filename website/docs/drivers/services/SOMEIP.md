---
title: SOMEIP
sidebar_label: SOMEIP
description: "- AUTOSAR Classic Platform 4.4.0"
sidebar_position: 28
---

# Some/IP Protocol (SOMEIP)

## 模块概述

Some/IP 协议

## AUTOSAR 版本

- AUTOSAR Classic Platform 4.4.0

## 主要功能

- 初始化和反初始化
- 核心功能处理
- 事件管理

## 主要 API

| API 名称 | 功能 |
|---------|------|
| Someip_Init() | 模块初始化 |
| Someip_DeInit() | 模块反初始化 |
| Someip_MainFunction() | 主函数 |

## 配置参数

### 编译时配置

- 模块使能/禁用
- 功能开关

### 链接时配置

- 配置表参数
- 回调函数指针

## 依赖关系

- DET (可选)
- DEM (可选)

## 使用示例

```c
#include "Someip.h"

void example(void)
{
    /* 初始化 */
    Someip_Init(NULL);
    
    /* 主函数 */
    Someip_MainFunction();
}
```

## 源代码路径

- `src/bsw/services/someip/`

## 测试

- 单元测试: `tests/unit/someip/`
