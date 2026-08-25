# <Module> Design Document

> **Module ID**: 0xNN  
> **AUTOSAR Layer**: MCAL | ECUAL | Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_<Module>  
> **Source Path**: `src/bsw/<layer>/<module>/`  
> **Reference Document**: `docs/modules/<module>.md`  
> **Doc Version**: 1.0  
> **Status**: Draft | Review | Approved

---

## 1. 模块概述

简要描述模块职责、在 AUTOSAR 分层中的位置、主要上下游模块。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS <Module> | 4.4.0 | 模块软件规范 |
| AUTOSAR Classic Platform | 4.x | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | ... | ... |
| 下层 | ... | ... |
| 同层 | ... | ... |
| 公共 | Det, Dem | 错误追踪与诊断事件（可选） |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│           Upper Layers              │
├─────────────────────────────────────┤
│     <Module> (MCAL/ECUAL/Services)  │
├─────────────────────────────────────┤
│           Lower Layers              │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- 子组件 A：...
- 子组件 B：...

### 3.3 文件结构

```
src/bsw/<layer>/<module>/
├── include/
│   ├── <Module>.h
│   ├── <Module>_Cfg.h
│   └── <Module>_MemMap.h
└── src/
    ├── <Module>.c
    ├── <Module>_Irq.c
    └── <Module>_Lcfg.c
```

---

## 4. 状态机

描述模块关键状态机（如控制器状态、通道状态、会话状态）。

```
[State A] -- event --> [State B]
```

---

## 5. 核心数据结构

列出关键类型、配置结构和运行时状态结构。

```c
typedef struct {
    uint32 field;
} Module_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 |
|-----|------|------|------|
| Module_Init | `void Module_Init(const Module_ConfigType* ConfigPtr)` | 初始化 | ... |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| ... | ... |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x01 | Init | MODULE_E_PARAM_POINTER |

---

## 7. 处理流程

### 7.1 关键流程 A

1. 步骤一
2. 步骤二
3. 步骤三

### 7.2 关键流程 B

...

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| MODULE_DEV_ERROR_DETECT | STD_OFF | 开发错误检测开关 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| Module_Lcfg.c | 链接时配置数据 |

### 8.3 构建后配置

| 配置 | 说明 |
|------|------|
| ... | ... |

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x01 | MODULE_E_PARAM_POINTER | 空指针入参 |

### 9.2 DEM 错误

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| ... | ... | ... |

### 9.3 安全机制

- ASIL 等级：...
- 安全机制：...

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| MODULE_START_SEC_CODE | 代码段 |
| MODULE_START_SEC_VAR_CLEARED_UNSPECIFIED | 零初始化变量 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ... | ... |
| ROM | ... | ... |
| 堆栈 | ... | ... |

---

## 11. 集成指南

- 与上层模块集成：...
- 与下层模块集成：...
- 初始化顺序：...

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| test_module.c | 初始化、API、错误处理 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| ... | ... |

---

## 13. 实现说明 / TODO

- 与 AUTOSAR SWS 的已知偏差
- 待完善项
- 平台相关说明

---

## 14. 参考资料

1. AUTOSAR_SWS_<Module>.pdf
2. `docs/modules/<module>.md`
3. `src/bsw/<layer>/<module>/`
