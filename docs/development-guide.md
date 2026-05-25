# 开发指南

## 概述

本文档提供 YuleTech AutoSAR BSW Platform 的开发规范和最佳实践。

## 目录

- [开发环境](#开发环境)
- [代码规范](#代码规范)
- [开发流程](#开发流程)
- [调试技巧](#调试技巧)
- [测试指南](#测试指南)

---

## 开发环境

### 工具链要求

| 工具 | 版本 | 用途 |
|:-----|:-----|:-----|
| GCC ARM Embedded | 10.3.1+ | 编译器 |
| CMake | 3.20+ | 构建系统 |
| Python | 3.9+ | 代码生成 |
| Git | 2.30+ | 版本控制 |
| VS Code | 最新 | IDE |

### 环境配置

#### 1. 安装 ARM GCC 工具链

```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi

# macOS
brew install arm-none-eabi-gcc

# Windows
# 下载并安装 from https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
```

#### 2. 安装 CMake

```bash
# Ubuntu/Debian
sudo apt-get install cmake

# macOS
brew install cmake

# Windows
# 下载并安装 from https://cmake.org/download/
```

#### 3. 配置 VS Code

安装以下扩展：
- C/C++ (Microsoft)
- CMake Tools
- Python
- GitLens

### 项目设置

```bash
# 克隆仓库
git clone https://github.com/yuletech/autosar-bsw-platform.git
cd autosar-bsw-platform

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc-toolchain.cmake

# 构建
make -j$(nproc)
```

---

## 代码规范

### 命名规范

#### 文件名

| 类型 | 规范 | 示例 |
|:-----|:-----|:-----|
| 源文件 | PascalCase | `CanIf.c`, `PduR.c` |
| 头文件 | PascalCase | `CanIf.h`, `PduR_Cfg.h` |
| 配置文件 | PascalCase + _Cfg | `CanIf_Cfg.h` |

#### 函数名

```c
/* 模块名_函数名 */
void CanIf_Init(const CanIf_ConfigType* ConfigPtr);
Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
void NvM_MainFunction(void);
```

#### 宏定义

```c
/* 模块名_描述 */
#define CANIF_E_UNINIT          (0x01U)
#define PDUR_NUMBER_OF_ROUTING_PATHS    (16U)
#define NVM_NUM_OF_NVRAM_BLOCKS         (32U)
```

#### 类型名

```c
/* 模块名_类型名_Type */
typedef struct {
    PduIdType SourcePduId;
    uint8 SourceModule;
    PduLengthType SduLength;
} PduR_SrcPduConfigType;

typedef enum {
    NVM_BLOCK_NATIVE = 0,
    NVM_BLOCK_REDUNDANT,
    NVM_BLOCK_DATASET
} NvM_BlockManagementType;
```

#### 变量名

```c
/* camelCase */
uint8 canIfStatus;
PduIdType txPduId;
const PduR_ConfigType* configPtr;
```

### 代码格式

#### 文件头

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
```

#### 函数注释

```c
/**
 * @brief   Initializes the module
 * @param   ConfigPtr - Pointer to configuration structure
 * @return  None
 * @pre     None
 * @post    Module initialized
 */
void Module_Init(const Module_ConfigType* ConfigPtr);

/**
 * @brief   Transmits data
 * @param   TxPduId - PDU identifier
 * @param   PduInfoPtr - Pointer to PDU data
 * @return  E_OK if transmission successful, E_NOT_OK otherwise
 * @pre     Module initialized
 * @post    Data queued for transmission
 */
Std_ReturnType Module_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
```

#### 代码结构

```c
/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Module.h"
#include "Module_Cfg.h"
#include "Det.h"
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define MODULE_INSTANCE_ID      (0x00U)
#define MODULE_STATE_UNINIT     (0x00U)
#define MODULE_STATE_INIT       (0x01U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (MODULE_DEV_ERROR_DETECT == STD_ON)
    #define MODULE_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(MODULE_MODULE_ID, MODULE_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define MODULE_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
typedef struct {
    uint8 State;
    const Module_ConfigType* ConfigPtr;
} Module_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define MODULE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC Module_InternalStateType Module_InternalState;

#define MODULE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType Module_ValidateParameter(const void* param);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define MODULE_START_SEC_CODE
#include "MemMap.h"

STATIC Std_ReturnType Module_ValidateParameter(const void* param)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (param != NULL_PTR)
    {
        result = E_OK;
    }
    
    return result;
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

void Module_Init(const Module_ConfigType* ConfigPtr)
{
#if (MODULE_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        MODULE_DET_REPORT_ERROR(MODULE_API_ID_INIT, MODULE_E_PARAM_POINTER);
        return;
    }
#endif

    Module_InternalState.ConfigPtr = ConfigPtr;
    Module_InternalState.State = MODULE_STATE_INIT;
}

#define MODULE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
```

### 错误处理

#### DET 错误检测

```c
#if (MODULE_DEV_ERROR_DETECT == STD_ON)
    #define MODULE_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(MODULE_MODULE_ID, MODULE_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define MODULE_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

void Module_Function(const void* param)
{
#if (MODULE_DEV_ERROR_DETECT == STD_ON)
    if (param == NULL_PTR)
    {
        MODULE_DET_REPORT_ERROR(MODULE_API_ID_FUNCTION, MODULE_E_PARAM_POINTER);
        return;
    }
    
    if (Module_InternalState.State != MODULE_STATE_INIT)
    {
        MODULE_DET_REPORT_ERROR(MODULE_API_ID_FUNCTION, MODULE_E_UNINIT);
        return;
    }
#endif

    /* Function implementation */
}
```

### 内存分区

```c
#define MODULE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC Module_InternalStateType Module_InternalState;

#define MODULE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define MODULE_START_SEC_CODE
#include "MemMap.h"

void Module_Function(void)
{
    /* Function implementation */
}

#define MODULE_STOP_SEC_CODE
#include "MemMap.h"
```

---

## 开发流程

### 1. 创建新模块

#### 目录结构

```
src/bsw/services/<module>/
├── include/
│   ├── <Module>.h          # 公共头文件
│   └── <Module>_Cfg.h      # 配置文件
└── src/
    └── <Module>.c          # 实现文件
```

#### 文件创建清单

- [ ] 创建目录结构
- [ ] 编写 <Module>.h 头文件
- [ ] 编写 <Module>_Cfg.h 配置文件
- [ ] 编写 <Module>.c 实现文件
- [ ] 添加 MemMap 分区
- [ ] 实现 DET 错误检测
- [ ] 编写单元测试
- [ ] 更新 CMakeLists.txt

### 2. 使用 OpenSpec 开发

#### 创建变更

```bash
/triple-new "添加 <模块名> 模块"
```

#### 编写规范

```markdown
# <模块名> 规范

## 功能描述
...

## Scenarios

### Scenario 1: 初始化
- **Given** 模块未初始化
- **When** 调用 <Module>_Init()
- **Then** 模块状态变为已初始化

### Scenario 2: 数据传输
- **Given** 模块已初始化
- **When** 调用 <Module>_Transmit()
- **Then** 数据成功发送
```

#### 开发执行

```bash
/triple-dev
```

#### 验证审查

```bash
/triple-verify
```

### 3. TDD 开发流程

#### 红-绿-重构

1. **红**: 编写失败的测试
2. **绿**: 编写最简单的代码使测试通过
3. **重构**: 优化代码，保持测试通过

#### 示例

```c
/* 测试：初始化应该设置状态为已初始化 */
void test_Module_Init_should_set_state_to_initialized(void)
{
    /* Given */
    const Module_ConfigType config = { /* ... */ };
    
    /* When */
    Module_Init(&config);
    
    /* Then */
    TEST_ASSERT_EQUAL(MODULE_STATE_INIT, Module_GetState());
}
```

---

## 调试技巧

### 使用 DET

```c
/* 启用 DET 错误检测 */
#define MODULE_DEV_ERROR_DETECT     (STD_ON)

/* 在错误位置调用 */
MODULE_DET_REPORT_ERROR(MODULE_API_ID_FUNCTION, MODULE_E_PARAM_POINTER);
```

### 日志输出

```c
#ifdef DEBUG
    #define MODULE_DEBUG_PRINT(fmt, ...) \
        printf("[MODULE] " fmt "\n", ##__VA_ARGS__)
#else
    #define MODULE_DEBUG_PRINT(fmt, ...)
#endif

MODULE_DEBUG_PRINT("TxPduId=%d, Length=%d", TxPduId, PduInfoPtr->SduLength);
```

### 断言

```c
#include <assert.h>

void Module_Function(const void* param)
{
    assert(param != NULL_PTR);
    
    /* Function implementation */
}
```

### GDB 调试

```bash
# 启动 GDB
gdb ./build/test_module

# 设置断点
(gdb) break Module_Init
(gdb) break Module_Transmit

# 运行
(gdb) run

# 查看变量
(gdb) print Module_InternalState
(gdb) print configPtr

# 单步执行
(gdb) step
(gdb) next

# 查看调用栈
(gdb) backtrace
```

---

## 测试指南

### 单元测试

#### 测试框架

使用 Unity 测试框架：

```c
#include "unity.h"
#include "Module.h"

void setUp(void)
{
    Module_DeInit();
}

void tearDown(void)
{
    Module_DeInit();
}

void test_Module_Init_should_initialize_module(void)
{
    /* Given */
    const Module_ConfigType config = { /* ... */ };
    
    /* When */
    Module_Init(&config);
    
    /* Then */
    TEST_ASSERT_EQUAL(MODULE_STATE_INIT, Module_GetState());
}

void test_Module_Transmit_should_return_OK_when_initialized(void)
{
    /* Given */
    const Module_ConfigType config = { /* ... */ };
    const PduInfoType pduInfo = { /* ... */ };
    Module_Init(&config);
    
    /* When */
    Std_ReturnType result = Module_Transmit(0, &pduInfo);
    
    /* Then */
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_Module_Transmit_should_return_NOT_OK_when_not_initialized(void)
{
    /* Given */
    const PduInfoType pduInfo = { /* ... */ };
    /* Module not initialized */
    
    /* When */
    Std_ReturnType result = Module_Transmit(0, &pduInfo);
    
    /* Then */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_Module_Init_should_initialize_module);
    RUN_TEST(test_Module_Transmit_should_return_OK_when_initialized);
    RUN_TEST(test_Module_Transmit_should_return_NOT_OK_when_not_initialized);
    return UNITY_END();
}
```

### 集成测试

```c
/* 测试模块间交互 */
void test_Com_to_PduR_integration(void)
{
    /* Given */
    Com_Init(&comConfig);
    PduR_Init(&pdurConfig);
    
    uint8 data[] = {0x01, 0x02, 0x03};
    
    /* When */
    Com_SendSignal(COM_SIGNAL_ID, data);
    
    /* Then */
    /* Verify PduR received the data */
}
```

### 测试覆盖率

```bash
# 生成覆盖率报告
make coverage

# 查看报告
open build/coverage/index.html
```

目标覆盖率：
- 语句覆盖率 > 80%
- 分支覆盖率 > 70%
- 函数覆盖率 > 90%

---

## 最佳实践

### 1. 保持函数简洁

```c
/* 好的做法 */
void Module_Process(void)
{
    Module_ProcessInput();
    Module_ProcessState();
    Module_ProcessOutput();
}

/* 避免 */
void Module_Process(void)
{
    /* 100+ lines of code */
}
```

### 2. 使用有意义的命名

```c
/* 好的做法 */
PduIdType txPduId;
PduLengthType sduLength;

/* 避免 */
uint8 x;
uint16 len;
```

### 3. 避免魔法数字

```c
/* 好的做法 */
#define MAX_ROUTING_PATHS   (16U)
#define TIMEOUT_MS          (100U)

/* 避免 */
if (count > 16)  /* 魔法数字 */
```

### 4. 检查返回值

```c
/* 好的做法 */
Std_ReturnType result = Module_Function();
if (result != E_OK)
{
    /* Handle error */
}

/* 避免 */
Module_Function();  /* 忽略返回值 */
```

### 5. 使用 const 修饰符

```c
/* 好的做法 */
void Function(const uint8* data, uint16 length);

/* 避免 */
void Function(uint8* data, uint16 length);
```

---

## 版本历史

| 版本 | 日期 | 变更描述 |
|:-----|:-----|:---------|
| 1.0.0 | 2026-04-15 | 初始版本，完整开发指南 |

---

**文档版本**: 1.0.0
**最后更新**: 2026-04-15
**作者**: YuleTech AutoSAR Team
