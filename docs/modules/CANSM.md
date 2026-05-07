# CAN State Manager (CANSM)

## 模块概述

CAN 状态管理

## AUTOSAR 版本

- AUTOSAR Classic Platform 4.4.0

## 主要功能

- 初始化和反初始化
- 核心功能处理
- 事件管理

## 主要 API

| API 名称 | 功能 |
|---------|------|
| Cansm_Init() | 模块初始化 |
| Cansm_DeInit() | 模块反初始化 |
| Cansm_MainFunction() | 主函数 |

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
#include "Cansm.h"

void example(void)
{
    /* 初始化 */
    Cansm_Init(NULL);
    
    /* 主函数 */
    Cansm_MainFunction();
}
```

## 源代码路径

- `src/bsw/services/cansm/`

## 测试

- 单元测试: `tests/unit/cansm/`
