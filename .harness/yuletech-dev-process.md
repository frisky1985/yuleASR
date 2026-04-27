---
name: yuletech-dev-process
description: 予乐科技 AutoSAR BSW 开发流程 Skill。定义基于 OpenSpec + Superpowers + Harness Engineering 的完整开发方法论，包括规范驱动开发、TDD 实践、代码规范、验证流程和 Git 工作流。适用于 AutoSAR BSW 模块开发、代码审查和项目维护。
---

# 予乐科技 AutoSAR BSW 开发流程

## 概述

本 Skill 定义了予乐科技基于 OpenSpec + Superpowers + Harness Engineering 的 AutoSAR BSW 开发方法论。

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

## 开发工作流

### Stage 0: 初始化工程
```bash
/triple-init
```
- 初始化项目结构
- 配置开发环境
- 设置质量门禁

### Stage 1: 需求探索
```bash
/triple-explore
```
- 分析需求背景
- 识别技术约束
- 评估实现方案

### Stage 2: 创建变更
```bash
/triple-new "添加 [模块名] 模块"
```
创建变更目录结构：
```
openspec/changes/[change-name]/
├── proposal.md      # 变更提案
├── specs/           # 规范定义
│   └── spec.md      # Gherkin 场景
├── design.md        # 架构设计
└── tasks.md         # 任务拆解
```

### Stage 3: 开发执行
```bash
/triple-dev
```
遵循 TDD 红-绿-重构循环：
1. **红**: 编写失败的测试
2. **绿**: 编写最小实现使测试通过
3. **重构**: 优化代码结构

### Stage 4: 验证审查
```bash
/triple-verify
```
检查清单：
- [ ] 所有 scenarios 通过
- [ ] 代码规范符合要求
- [ ] 圈复杂度 < 10
- [ ] 测试覆盖率 > 80%

### Stage 5: 归档合并
```bash
/triple-archive
```
- 合并规范到 `openspec/specs/`
- 移动变更到 `openspec/archived/`
- 创建 Pull Request

## 代码规范

### 文件头模板
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

### 错误检测
```c
#if (MODULE_DEV_ERROR_DETECT == STD_ON)
    #define MODULE_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(MODULE_MODULE_ID, MODULE_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define MODULE_DET_REPORT_ERROR(ApiId, ErrorId)
#endif
```

### 内存分区
```c
#define MODULE_START_SEC_CODE
#include "MemMap.h"

/* Code here */

#define MODULE_STOP_SEC_CODE
#include "MemMap.h"
```

## Git 工作流

### 分支策略
- `main`: 主分支，稳定版本
- `feature/*`: 功能分支
- `fix/*`: 修复分支

### 提交规范
```
<type>: <subject>

<body>

🤖 Generated with [Qoder](https://qoder.com)
```

Type 类型：
- `feat`: 新功能
- `fix`: 修复
- `docs`: 文档
- `style`: 格式
- `refactor`: 重构
- `test`: 测试
- `chore`: 构建/工具

### Pull Request 流程
1. 从 main 创建功能分支
2. 开发并提交代码
3. 运行验证 `/triple-verify`
4. 创建 PR，填写描述
5. 代码审查
6. 合并到 main

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

## 最佳实践

1. **先写规范，后写代码**
2. **使用 TDD 方法**
3. **保持函数单一职责**
4. **添加充分的错误检测**
5. **使用静态分析工具**
6. **编写清晰的注释**
7. **遵循 AutoSAR 标准**
