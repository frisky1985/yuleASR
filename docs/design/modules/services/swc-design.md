# Swc Design Document

> **Module ID**: 0xA1  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Swc  
> **Source Path**: `src/bsw/services/swc/`  
> **Reference Document**: `docs/modules/swc.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Swc（Software Component）模块是 AUTOSAR 服务层的扩展模块，提供软件组件的生命周期管理、Runnable 实体调度、端口接口管理和 RTE 事件处理功能。该模块为应用层软件组件提供运行时基础设施支持。

主要职责：
- 软件组件实例的创建与销毁
- 组件状态管理（UNINIT → INIT → ACTIVE → SUSPENDED）
- Runnable 实体的调度与执行
- 端口接口的连接与数据读写
- RTE 事件的注册、触发与处理
- 事件队列管理

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Swc | 4.4.0 | 软件组件模块规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | ASWC | 应用层软件组件 |
| 同层 | RTE | 运行时环境 |
| 公共 | Det | 开发错误追踪 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        Application Layer            │
├─────────────────────────────────────┤
│       Swc (Services Layer)          │
├─────────────────────────────────────┤
│            RTE                      │
├─────────────────────────────────────┤
│        BSW Services / ECUAL         │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **组件实例管理器（Component Instance Manager）**：管理组件实例的创建/销毁/状态
- **Runnable 调度器（Runnable Scheduler）**：调度 Runnable 实体的执行
- **端口接口管理器（Port Interface Manager）**：管理端口连接与数据读写
- **事件队列（Event Queue）**：环形缓冲区管理 RTE 事件

### 3.3 文件结构

```
src/bsw/services/swc/
├── include/
│   ├── Swc.h
│   └── Swc_Cfg.h
└── src/
    ├── Swc.c
    └── Swc_Lcfg.c
```

---

## 4. 状态机

组件状态机：

```
[SWC_STATE_UNINIT]
    │ CreateInstance
    ▼
[SWC_STATE_INIT]
    │ SetComponentState(ACTIVE)
    ▼
[SWC_STATE_ACTIVE] ◄──► [SWC_STATE_SUSPENDED]
    │ error               │
    ▼                     ▼
[SWC_STATE_ERROR]    [SWC_STATE_INIT]
```

Runnable 执行状态机：

```
[SWC_RUNNABLE_IDLE]
    │ ActivateRunnable
    ▼
[SWC_RUNNABLE_READY]
    │ ScheduleRunnables
    ▼
[SWC_RUNNABLE_RUNNING]
    │ execution complete
    ▼
[SWC_RUNNABLE_COMPLETED]
    │ TerminateRunnable
    ▼
[SWC_RUNNABLE_IDLE]
```

---

## 5. 核心数据结构

```c
/* 组件实例 */
typedef struct {
    Swc_ComponentHandleType handle;
    Swc_StateType state;
    const struct Swc_ComponentConfigType* config;
    void* instanceData;
    uint16 portCount;
    uint16 runnableCount;
} Swc_ComponentInstanceType;

/* Runnable 实体 */
typedef struct {
    Swc_RunnableHandleType handle;
    Swc_RunnableStateType state;
    const struct Swc_RunnableConfigType* config;
    Swc_ComponentHandleType ownerComponent;
    uint32 executionCounter;
    uint32 lastExecutionTime;
} Swc_RunnableEntityType;

/* 内部数据 */
typedef struct {
    Swc_ComponentInstanceType instances[SWC_MAX_COMPONENT_INSTANCES];
    Swc_RunnableEntityType runnables[SWC_MAX_COMPONENT_INSTANCES * SWC_MAX_RUNNABLES_PER_COMPONENT];
    Swc_EventQueueEntryType eventQueue[SWC_EVENT_QUEUE_SIZE];
    uint8 eventQueueHead;
    uint8 eventQueueTail;
    uint8 eventQueueCount;
    boolean initialized;
    uint32 cycleCounter;
} Swc_InternalDataType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| Swc_Init | `void Swc_Init(const Swc_ConfigType* ConfigPtr)` | 初始化 | | SWS_Swc_00001 |
| Swc_DeInit | `void Swc_DeInit(void)` | 反初始化 | | SWS_Swc_00002 |
| Swc_GetVersionInfo | `Std_ReturnType Swc_GetVersionInfo(Std_VersionInfoType*)` | 版本信息 | | SWS_Swc_00003 |
| Swc_CreateInstance | `Std_ReturnType Swc_CreateInstance(Swc_ComponentHandleType, void*, Swc_ComponentHandleType*)` | 创建实例 | | SWS_Swc_00005 |
| Swc_DestroyInstance | `Std_ReturnType Swc_DestroyInstance(Swc_ComponentHandleType)` | 销毁实例 | | SWS_Swc_00006 |
| Swc_SetComponentState | `Std_ReturnType Swc_SetComponentState(Swc_ComponentHandleType, Swc_StateType)` | 设置状态 | | SWS_Swc_00007 |
| Swc_GetComponentState | `Swc_StateType Swc_GetComponentState(Swc_ComponentHandleType)` | 获取状态 | |  |
| Swc_ActivateRunnable | `Std_ReturnType Swc_ActivateRunnable(Swc_RunnableHandleType)` | 激活 Runnable | | SWS_Swc_00008 |
| Swc_TerminateRunnable | `Std_ReturnType Swc_TerminateRunnable(Swc_RunnableHandleType)` | 终止 Runnable | | SWS_Swc_00009 |
| Swc_ScheduleRunnables | `void Swc_ScheduleRunnables(void)` | 调度执行 | | SWS_Swc_00010 |
| Swc_IsRunnableReady | `Std_ReturnType Swc_IsRunnableReady(Swc_RunnableHandleType, boolean*)` | 查询就绪 | | SWS_Swc_00011 |
| Swc_ConnectPort | `Std_ReturnType Swc_ConnectPort(Swc_PortHandleType, const void*)` | 连接端口 | 桩实现 | SWS_Swc_00012 |
| Swc_DisconnectPort | `Std_ReturnType Swc_DisconnectPort(Swc_PortHandleType)` | 断开端口 | 桩实现 | SWS_Swc_00013 |
| Swc_WritePortData | `Std_ReturnType Swc_WritePortData(Swc_PortHandleType, const void*, uint16)` | 写端口数据 | 桩实现 | SWS_Swc_00014 |
| Swc_ReadPortData | `Std_ReturnType Swc_ReadPortData(Swc_PortHandleType, void*, uint16*)` | 读端口数据 | 桩实现 | SWS_Swc_00015 |
| Swc_RegisterEvent | `Std_ReturnType Swc_RegisterEvent(Swc_ComponentHandleType, const Swc_RteEventType*)` | 注册事件 | 桩实现 | SWS_Swc_00016 |
| Swc_TriggerEvent | `Std_ReturnType Swc_TriggerEvent(Swc_EventHandleType)` | 触发事件 | | SWS_Swc_00017 |
| Swc_EnableEvent | `Std_ReturnType Swc_EnableEvent(Swc_EventHandleType)` | 启用事件 | 桩实现 | SWS_Swc_00018 |
| Swc_DisableEvent | `Std_ReturnType Swc_DisableEvent(Swc_EventHandleType)` | 禁用事件 | 桩实现 | SWS_Swc_00019 |
| Swc_ProcessEvents | `void Swc_ProcessEvents(void)` | 处理事件 | | SWS_Swc_00020 |
| Swc_MainFunction | `void Swc_MainFunction(void)` | 周期处理 | | SWS_Swc_00004 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| Swc_RunnableFuncType | Runnable 执行函数指针 `void (*)(void)` |
| Swc_RteCallbackType | RTE 事件回调 `Std_ReturnType (*)(Swc_ComponentHandleType, Swc_EventHandleType)` |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x01 | Init | SWC_E_PARAM_POINTER, SWC_E_ALREADY_INITIALIZED |
| 0x02 | DeInit | SWC_E_UNINIT |
| 0x03 | Runnable | SWC_E_UNINIT |
| 0x04 | GetVersionInfo | SWC_E_PARAM_POINTER |
| 0x05 | ActivateRunnable | SWC_E_UNINIT, SWC_E_INVALID_RUNNABLE |
| 0x06 | TriggerEvent | SWC_E_UNINIT, SWC_E_EVENT_QUEUE_FULL |

---

## 7. 处理流程

### 7.1 组件实例创建流程

1. 调用 `Swc_CreateInstance`，验证 componentId 和 outHandle
2. 查找空闲实例槽（state == SWC_STATE_UNINIT）
3. 填充配置、实例数据、端口数、Runnable 数
4. 设置状态为 SWC_STATE_INIT，返回句柄

### 7.2 Runnable 调度流程

1. `Swc_MainFunction` 递增 cycleCounter
2. 调用 `Swc_ProcessEvents` 处理事件队列
3. 调用 `Swc_ScheduleRunnables` 遍历所有 Runnable
4. 对 READY 状态的 Runnable 调用 `Swc_ExecuteRunnable`
5. 执行 runnableFunc，更新执行计数和时间

### 7.3 事件触发流程

1. 调用 `Swc_TriggerEvent`，检查事件队列是否已满
2. 将事件写入环形缓冲区尾部
3. `Swc_ProcessEvents` 从头部取出事件并处理

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| SWC_DEV_ERROR_DETECT | STD_ON | 开发错误检测 |
| SWC_MAX_COMPONENT_INSTANCES | 配置 | 最大组件实例数 |
| SWC_MAX_RUNNABLES_PER_COMPONENT | 配置 | 每组件最大 Runnable 数 |
| SWC_EVENT_QUEUE_SIZE | 配置 | 事件队列大小 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| Swc_Lcfg.c | 组件配置数据 |
| Swc_Cfg.h | 预编译配置参数 |

### 8.3 构建后配置

不适用。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x01 | SWC_E_PARAM_POINTER | 空指针入参 |
| 0x02 | SWC_E_INVALID_COMPONENT | 无效组件 ID |
| 0x03 | SWC_E_INVALID_RUNNABLE | 无效 Runnable 句柄 |
| 0x04 | SWC_E_INVALID_PORT | 无效端口句柄 |
| 0x05 | SWC_E_UNINIT | 模块未初始化 |
| 0x06 | SWC_E_ALREADY_INITIALIZED | 重复初始化 |
| 0x07 | SWC_E_EVENT_QUEUE_FULL | 事件队列已满 |

### 9.2 DEM 错误

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| — | — | 当前未定义 DEM 事件 |

### 9.3 安全机制

- 初始化状态检查
- 句柄有效性验证
- 事件队列溢出保护

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| 默认代码段 | Swc.c 全部函数 |
| 默认数据段 | 内部数据结构 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~2-4 KB | 实例数组 + Runnable 数组 + 事件队列 |
| ROM | ~4 KB | 代码段 |
| 堆栈 | ~256 bytes | 函数调用栈 |

---

## 11. 集成指南

- 与上层集成：ASWC 通过 `Swc_CreateInstance` 创建组件实例
- 与同层集成：通过 RTE 进行事件分发
- 初始化顺序：Det → Swc_Init → Swc_CreateInstance → Swc_ActivateRunnable
- MainFunction 周期建议：1-10ms

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| test_swc.c | 初始化/反初始化、实例创建/销毁、状态管理、Runnable 调度、事件队列 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 组件生命周期 | 创建 → 激活 → 运行 → 挂起 → 销毁 |
| Runnable 调度 | 多 Runnable 并发调度与执行 |
| 事件队列溢出 | 验证队列满时的错误处理 |

---

## 13. 实现说明 / TODO

- 端口接口函数（ConnectPort/DisconnectPort/WritePortData/ReadPortData）为桩实现
- 事件注册（RegisterEvent）、启用/禁用（EnableEvent/DisableEvent）为桩实现
- `Swc_ProcessQueuedEvents` 仅清空队列，未实现实际事件到 Runnable 的映射
- 需要实现基于周期的 Runnable 自动调度
- 需要实现 RTE 事件与 Runnable 的关联机制

---

## 14. 参考资料

1. AUTOSAR_SWS_Swc.pdf
2. `docs/modules/swc.md`
3. `src/bsw/services/swc/`
