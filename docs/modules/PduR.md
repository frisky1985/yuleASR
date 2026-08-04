# PDU Router (PDUR)

## 模块概述

PDU 路由器

## AUTOSAR 版本

- AUTOSAR Classic Platform 4.4.0

## 主要功能

- 初始化和反初始化
- 核心功能处理
- 事件管理

## 主要 API

| API 名称 | 功能 |
|---------|------|
| Pdur_Init() | 模块初始化 |
| Pdur_DeInit() | 模块反初始化 |
| Pdur_MainFunction() | 主函数 |

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
#include "Pdur.h"

void example(void)
{
    /* 初始化 */
    Pdur_Init(NULL);
    
    /* 主函数 */
    Pdur_MainFunction();
}
```

## 源代码路径

- `src/bsw/services/pdur/`

## 测试

- 单元测试: `tests/unit/pdur/`
