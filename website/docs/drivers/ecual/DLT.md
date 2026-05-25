---
title: DLT
sidebar_label: DLT
description: "- AUTOSAR Classic Platform 4.4.0"
sidebar_position: 7
---

# Diagnostic Log and Trace (DLT)

## 模块概述

诊断日志和追踪

## AUTOSAR 版本

- AUTOSAR Classic Platform 4.4.0

## 主要功能

- 初始化和反初始化
- 核心功能处理
- 事件管理

## 主要 API

| API 名称 | 功能 |
|---------|------|
| Dlt_Init() | 模块初始化 |
| Dlt_DeInit() | 模块反初始化 |
| Dlt_MainFunction() | 主函数 |

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
#include "Dlt.h"

void example(void)
{
    /* 初始化 */
    Dlt_Init(NULL);
    
    /* 主函数 */
    Dlt_MainFunction();
}
```

## 源代码路径

- `src/bsw/services/dlt/`

## 测试

- 单元测试: `tests/unit/dlt/`
