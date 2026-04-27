# AutoSAR BSW 开发 Skill

## 概述
本 Skill 定义了基于 OpenSpec + Superpowers + Harness Engineering 的 AutoSAR BSW 开发方法论。

## 开发原则

### 1. 规范驱动开发 (OpenSpec)
- 所有功能必须有对应的 OpenSpec 规范
- 规范位于 `openspec/specs/` 目录
- 使用 Gherkin 风格描述场景

### 2. 分层架构
```
RTE (Run Time Environment)
    ↑
Services (Com, PduR, NvM, Dcm, Dem)
    ↑
ECUAL (CanIf, IoHwAb, CanTp, EthIf, MemIf, Fee, Ea, FrIf, LinIf)
    ↑
MCAL (Mcu, Port, Dio, Can, Spi, Gpt, Pwm, Adc, Wdg)
    ↑
Hardware (i.MX8M Mini)
```

### 3. 命名规范
| 类型 | 规范 | 示例 |
|:-----|:-----|:-----|
| 文件名 | PascalCase | `CanIf.c`, `CanIf.h` |
| 函数名 | ModuleName_FunctionName | `CanIf_Init`, `CanIf_Transmit` |
| 宏定义 | MODULENAME_MACRO_NAME | `CANIF_E_UNINIT` |
| 类型名 | ModuleName_TypeName | `CanIf_ControllerModeType` |
| 变量名 | camelCase | `canIfStatus`, `txPduId` |

### 4. 代码结构
```c
/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : [Peripheral Name]
* Dependencies         : [Dependency List]
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : YYYY-MM-DD
* Author               : [Author Name]
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
```

## 开发流程

### 1. 创建变更
```bash
/triple-new "添加 [模块名] 模块"
```

### 2. 编写规范
在 `openspec/changes/[change]/specs/` 创建规范文件：
- 使用 Gherkin 语法描述场景
- 定义 Given-When-Then 步骤
- 明确输入输出

### 3. 设计架构
在 `openspec/changes/[change]/design.md` 创建设计文档：
- 模块架构图
- 接口定义
- 数据结构
- 状态机

### 4. 创建任务
在 `openspec/changes/[change]/tasks.md` 创建任务列表：
- 头文件创建
- 源文件实现
- 功能验证
- 集成测试

### 5. 开发实现
- 创建头文件 (Module.h, Module_Cfg.h)
- 创建源文件 (Module.c)
- 实现所有 API
- 添加 DET 错误检测

### 6. 验证审查
```bash
/triple-verify
```
- 代码规范检查
- 功能验证
- 接口兼容性检查

### 7. 归档合并
```bash
/triple-archive
```
- 合并规范到 `openspec/specs/`
- 移动变更到 `openspec/archived/`
- Git 合并到 main

## 代码规范

### 1. 错误检测
```c
#if (MODULE_DEV_ERROR_DETECT == STD_ON)
    #define MODULE_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(MODULE_MODULE_ID, MODULE_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define MODULE_DET_REPORT_ERROR(ApiId, ErrorId)
#endif
```

### 2. 内存分区
```c
#define MODULE_START_SEC_CODE
#include "MemMap.h"

/* Code here */

#define MODULE_STOP_SEC_CODE
#include "MemMap.h"
```

### 3. 函数实现模板
```c
/**
 * @brief   [Function description]
 * @param   [Parameter name] - [Parameter description]
 * @return  [Return description]
 */
ReturnType Module_FunctionName(ParamType ParamName)
{
    ReturnType result = E_NOT_OK;

#if (MODULE_DEV_ERROR_DETECT == STD_ON)
    /* Parameter validation */
    if (ParamName == NULL_PTR)
    {
        MODULE_DET_REPORT_ERROR(API_ID, ERROR_CODE);
        return E_NOT_OK;
    }
#endif

    /* Function implementation */

    return result;
}
```

## 模块开发清单

### 头文件 (Module.h)
- [ ] 版本定义
- [ ] 服务 ID 定义
- [ ] 错误码定义
- [ ] 类型定义
- [ ] 函数原型

### 配置文件 (Module_Cfg.h)
- [ ] 预编译配置
- [ ] 常量定义
- [ ] 句柄定义

### 源文件 (Module.c)
- [ ] 包含头文件
- [ ] 局部常量
- [ ] 局部宏
- [ ] 局部类型
- [ ] 局部变量
- [ ] 局部函数原型
- [ ] 局部函数实现
- [ ] 全局函数实现

## 验证清单

### 功能验证
- [ ] 初始化功能
- [ ] 反初始化功能
- [ ] 核心功能
- [ ] 错误处理
- [ ] 边界条件

### 代码质量
- [ ] 命名规范
- [ ] 代码结构
- [ ] 注释完整
- [ ] 错误检测
- [ ] 内存分区

### 接口兼容性
- [ ] 头文件兼容
- [ ] 下层模块兼容
- [ ] 上层模块兼容

## 最佳实践

1. **先写规范，后写代码**
2. **使用 TDD 方法**
3. **保持函数单一职责**
4. **添加充分的错误检测**
5. **使用静态分析工具**
6. **编写清晰的注释**
7. **遵循 AutoSAR 标准**
