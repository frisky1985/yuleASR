# Tasks: 核心 BSW 模块开发任务

> **变更ID**: init-core-bsw  
> **状态**: 活跃  
> **创建日期**: 2026-04-13

## 任务总览

```
Phase 1: 基础架构 (Week 1-2)
├── Task 1.1: 项目目录结构初始化
├── Task 1.2: 通用类型定义
├── Task 1.3: 构建系统搭建
└── Task 1.4: 测试框架集成

Phase 2: MCAL 核心模块 (Week 3-6)
├── Task 2.1: MCU Driver 实现
├── Task 2.2: GPIO (Port) Driver 实现
├── Task 2.3: CAN Driver 实现
└── Task 2.4: HAL 抽象层实现

Phase 3: 代码生成器 (Week 7-9)
├── Task 3.1: ARXML 解析器
├── Task 3.2: 模板引擎集成
├── Task 3.3: 代码生成核心
└── Task 3.4: 生成验证

Phase 4: 配置工具 (Week 10-12)
├── Task 4.1: 前端框架搭建
├── Task 4.2: 配置界面实现
├── Task 4.3: 后端 API 开发
└── Task 4.4: 前后端集成

Phase 5: 集成测试 (Week 13-14)
├── Task 5.1: 模块集成测试
├── Task 5.2: 端到端测试
├── Task 5.3: 性能测试
└── Task 5.4: 文档完善
```

---

## Phase 1: 基础架构

### Task 1.1: 项目目录结构初始化

**描述**: 建立完整的 BSW 项目目录结构

**验收标准**:
- [ ] 创建 src/bsw/ 目录结构
- [ ] 创建 tests/ 目录结构
- [ ] 创建 docs/ 目录结构
- [ ] 创建 tools/ 目录结构

**估计工时**: 4h

**依赖**: 无

---

### Task 1.2: 通用类型定义

**描述**: 定义 BSW 通用类型和头文件

**验收标准**:
- [ ] 创建 Std_Types.h (标准类型)
- [ ] 创建 Std_ReturnType.h (返回类型)
- [ ] 创建 Compiler.h (编译器抽象)
- [ ] 创建 Platform_Types.h (平台类型)

**代码规范**:
```c
/* Std_Types.h */
#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t sint8;
typedef int16_t sint16;
typedef int32_t sint32;
typedef int64_t sint64;

typedef uint8 Std_ReturnType;
#define E_OK        0x00
#define E_NOT_OK    0x01

#endif
```

**估计工时**: 8h

**依赖**: Task 1.1

---

### Task 1.3: 构建系统搭建

**描述**: 搭建 Makefile 构建系统

**验收标准**:
- [ ] 创建根目录 Makefile
- [ ] 支持多模块编译
- [ ] 支持交叉编译 (ARM GCC)
- [ ] 支持编译选项配置

**估计工时**: 8h

**依赖**: Task 1.1

---

### Task 1.4: 测试框架集成

**描述**: 集成 Unity 单元测试框架

**验收标准**:
- [ ] 引入 Unity 测试框架
- [ ] 创建测试 Makefile
- [ ] 编写示例测试用例
- [ ] 集成到 CI 流程

**估计工时**: 8h

**依赖**: Task 1.1

---

## Phase 2: MCAL 核心模块

### Task 2.1: MCU Driver 实现

**描述**: 实现 MCU 驱动模块

**验收标准**:
- [ ] 创建 Mcu.h 头文件 (API 声明)
- [ ] 创建 Mcu.c 实现文件
- [ ] 实现 Mcu_Init() 函数
- [ ] 实现 Mcu_InitClock() 函数
- [ ] 实现 Mcu_SetMode() 函数
- [ ] 实现 Mcu_GetResetReason() 函数
- [ ] 创建 Mcu_Cfg.h 配置头文件
- [ ] 单元测试覆盖率 > 80%

**TDD 循环**:
```
RED:  编写测试 test_Mcu_Init_shouldInitializeClock
GREEN: 实现 Mcu_Init() 最小代码
REFACTOR: 优化代码结构

RED:  编写测试 test_Mcu_GetResetReason_shouldReturnValidReason
GREEN: 实现 Mcu_GetResetReason()
REFACTOR: 提取公共函数
```

**估计工时**: 40h

**依赖**: Task 1.2, Task 1.4

---

### Task 2.2: GPIO (Port) Driver 实现

**描述**: 实现 GPIO 驱动模块

**验收标准**:
- [ ] 创建 Port.h 头文件
- [ ] 创建 Port.c 实现文件
- [ ] 实现 Port_Init() 函数
- [ ] 实现 Port_SetPinDirection() 函数
- [ ] 实现 Port_RefreshPortDirection() 函数
- [ ] 创建 Dio.h (Digital I/O)
- [ ] 实现 Dio_WriteChannel() 函数
- [ ] 实现 Dio_ReadChannel() 函数
- [ ] 单元测试覆盖率 > 85%

**估计工时**: 32h

**依赖**: Task 1.2, Task 1.4

---

### Task 2.3: CAN Driver 实现

**描述**: 实现 CAN 驱动模块

**验收标准**:
- [ ] 创建 Can.h 头文件
- [ ] 创建 Can.c 实现文件
- [ ] 实现 Can_Init() 函数
- [ ] 实现 Can_Write() 函数
- [ ] 实现 Can_MainFunction_Write() 函数
- [ ] 实现 Can_MainFunction_Read() 函数
- [ ] 实现中断处理函数
- [ ] 支持标准帧和扩展帧
- [ ] 单元测试覆盖率 > 75%

**估计工时**: 48h

**依赖**: Task 1.2, Task 1.4

---

### Task 2.4: HAL 抽象层实现

**描述**: 实现硬件抽象层 (S32K3 平台)

**验收标准**:
- [ ] 创建 Mcu_Hw.h/c (MCU 硬件访问)
- [ ] 创建 Port_Hw.h/c (GPIO 硬件访问)
- [ ] 创建 Can_Hw.h/c (CAN 硬件访问)
- [ ] 实现寄存器访问宏
- [ ] 实现中断向量表

**估计工时**: 24h

**依赖**: Task 2.1, Task 2.2, Task 2.3

---

## Phase 3: 代码生成器

### Task 3.1: ARXML 解析器

**描述**: 实现 ARXML 配置文件解析器

**验收标准**:
- [ ] 创建 arxml_parser.py
- [ ] 支持 ECUC 配置解析
- [ ] 支持模块配置提取
- [ ] 支持参数验证
- [ ] 解析错误报告

**估计工时**: 32h

**依赖**: 无

---

### Task 3.2: 模板引擎集成

**描述**: 集成 Jinja2 模板引擎

**验收标准**:
- [ ] 安装 Jinja2 依赖
- [ ] 创建模板目录结构
- [ ] 创建基础模板 (header.j2, source.j2)
- [ ] 实现模板加载器

**估计工时**: 16h

**依赖**: 无

---

### Task 3.3: 代码生成核心

**描述**: 实现代码生成器核心逻辑

**验收标准**:
- [ ] 创建 generator.py
- [ ] 实现 generate_from_arxml() 函数
- [ ] 实现模块代码生成
- [ ] 实现配置代码生成
- [ ] 支持增量生成

**估计工时**: 32h

**依赖**: Task 3.1, Task 3.2

---

### Task 3.4: 生成验证

**描述**: 验证生成的代码可编译

**验收标准**:
- [ ] 生成 Mcu 配置代码
- [ ] 生成 Port 配置代码
- [ ] 生成 CAN 配置代码
- [ ] 验证代码可编译通过
- [ ] 验证代码可正常运行

**估计工时**: 16h

**依赖**: Task 3.3, Task 2.4

---

## Phase 4: 配置工具

### Task 4.1: 前端框架搭建

**描述**: 搭建 Vue 3 + TypeScript 前端框架

**验收标准**:
- [ ] 创建 Vue 3 项目
- [ ] 配置 TypeScript
- [ ] 集成 Element Plus
- [ ] 配置 Pinia 状态管理
- [ ] 配置 Vue Router

**估计工时**: 16h

**依赖**: 无

---

### Task 4.2: 配置界面实现

**描述**: 实现基础软件配置界面

**验收标准**:
- [ ] 实现 ModuleTree 组件
- [ ] 实现 ConfigForm 组件
- [ ] 实现 ParameterEditor 组件
- [ ] 实现配置导入/导出
- [ ] 实现配置校验显示

**估计工时**: 40h

**依赖**: Task 4.1

---

### Task 4.3: 后端 API 开发

**描述**: 开发配置工具后端 API

**验收标准**:
- [ ] 创建 Spring Boot 项目
- [ ] 实现 ConfigAPI
- [ ] 实现 ModuleAPI
- [ ] 实现 GeneratorAPI
- [ ] 集成数据库 (MySQL)

**估计工时**: 40h

**依赖**: 无

---

### Task 4.4: 前后端集成

**描述**: 集成前后端功能

**验收标准**:
- [ ] 配置 Axios HTTP 客户端
- [ ] 实现 API 调用
- [ ] 实现错误处理
- [ ] 实现加载状态
- [ ] 端到端测试通过

**估计工时**: 24h

**依赖**: Task 4.2, Task 4.3

---

## Phase 5: 集成测试

### Task 5.1: 模块集成测试

**描述**: 进行模块间集成测试

**验收标准**:
- [ ] MCU + GPIO 集成测试
- [ ] GPIO + CAN 集成测试
- [ ] 完整 BSW 集成测试
- [ ] 集成测试报告

**估计工时**: 24h

**依赖**: Task 2.4, Task 3.4

---

### Task 5.2: 端到端测试

**描述**: 进行端到端功能测试

**验收标准**:
- [ ] 配置工具 → 代码生成 → 编译 流程测试
- [ ] 配置导入/导出测试
- [ ] 代码生成验证测试
- [ ] 端到端测试报告

**估计工时**: 16h

**依赖**: Task 4.4, Task 3.4

---

### Task 5.3: 性能测试

**描述**: 进行性能测试

**验收标准**:
- [ ] ARXML 解析性能 (< 2s)
- [ ] 代码生成性能 (< 5s)
- [ ] 配置界面加载性能 (< 1s)
- [ ] 性能测试报告

**估计工时**: 16h

**依赖**: Task 4.4, Task 3.4

---

### Task 5.4: 文档完善

**描述**: 完善项目文档

**验收标准**:
- [ ] 更新 README.md
- [ ] 编写开发指南
- [ ] 编写 API 文档
- [ ] 编写用户手册

**估计工时**: 24h

**依赖**: 所有前置任务

---

## 任务依赖图

```
Task 1.1 ──┬── Task 1.2 ──┬── Task 2.1 ──┐
           │              │              │
           ├── Task 1.3   ├── Task 2.2 ──┼── Task 2.4 ──┬── Task 5.1
           │              │              │              │
           └── Task 1.4   └── Task 2.3 ──┘              ├── Task 5.2
                                                        │
Task 3.1 ──┬── Task 3.2 ──┬── Task 3.3 ──┬── Task 3.4 ──┤
                                                        │
Task 4.1 ──┬── Task 4.2 ──┐                             │
           │              │                             │
           └── Task 4.3 ──┴── Task 4.4 ─────────────────┤
                                                        │
                                           Task 5.3 ────┤
                                                        │
                                           Task 5.4 ────┘
```

---

## 里程碑

| 里程碑 | 日期 | 交付物 |
|:-------|:-----|:-------|
| M1: 基础架构完成 | Week 2 | 项目结构 + 构建系统 |
| M2: MCAL 核心完成 | Week 6 | MCU/GPIO/CAN 驱动 |
| M3: 代码生成器完成 | Week 9 | 可生成配置代码 |
| M4: 配置工具完成 | Week 12 | 可配置 + 生成 + 编译 |
| M5: MVP 发布 | Week 14 | 完整功能 + 文档 |

---

## 进度跟踪

| 任务 | 状态 | 负责人 | 开始日期 | 完成日期 |
|:-----|:-----|:-------|:---------|:---------|
| Task 1.1 | ⬜ 未开始 | TBD | - | - |
| Task 1.2 | ⬜ 未开始 | TBD | - | - |
| Task 1.3 | ⬜ 未开始 | TBD | - | - |
| Task 1.4 | ⬜ 未开始 | TBD | - | - |
| Task 2.1 | ⬜ 未开始 | TBD | - | - |
| Task 2.2 | ⬜ 未开始 | TBD | - | - |
| Task 2.3 | ⬜ 未开始 | TBD | - | - |
| Task 2.4 | ⬜ 未开始 | TBD | - | - |
| Task 3.1 | ⬜ 未开始 | TBD | - | - |
| Task 3.2 | ⬜ 未开始 | TBD | - | - |
| Task 3.3 | ⬜ 未开始 | TBD | - | - |
| Task 3.4 | ⬜ 未开始 | TBD | - | - |
| Task 4.1 | ⬜ 未开始 | TBD | - | - |
| Task 4.2 | ⬜ 未开始 | TBD | - | - |
| Task 4.3 | ⬜ 未开始 | TBD | - | - |
| Task 4.4 | ⬜ 未开始 | TBD | - | - |
| Task 5.1 | ⬜ 未开始 | TBD | - | - |
| Task 5.2 | ⬜ 未开始 | TBD | - | - |
| Task 5.3 | ⬜ 未开始 | TBD | - | - |
| Task 5.4 | ⬜ 未开始 | TBD | - | - |

---

**状态说明**:
- ⬜ 未开始
- 🟡 进行中
- 🟢 已完成
- 🔴 阻塞
