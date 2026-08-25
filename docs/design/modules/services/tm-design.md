# Tm Design Document

> **Module ID**: 0xA2  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Tm  
> **Source Path**: `src/bsw/services/tm/`  
> **Reference Document**: `docs/modules/tm.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Tm（Time Manager）模块是 AUTOSAR 服务层的一部分，提供集中化的时间管理服务，包括时间基管理、全局时间获取与设置、时间基同步以及持续时间计算。该模块为其他 BSW 模块提供统一的时间参考。

主要职责：
- 时间基（Time Base）管理
- 全局时间（Global Time）获取与设置
- 时间基同步
- 持续时间计算
- 周期性时间基更新

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Tm | R21-11 §12.15 | 时间管理模块软件规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | 各 BSW 模块 | 需要时间服务的模块 |
| 下层 | Os / Mcal | 系统定时器 / 硬件定时器 |
| 公共 | Det | 开发错误追踪 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        BSW Modules (Users)          │
├─────────────────────────────────────┤
│        Tm (Services Layer)          │
├─────────────────────────────────────┤
│        Os / Mcal (Timer)            │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **时间基管理器（Time Base Manager）**：管理多个时间基的值与状态
- **全局时间管理器（Global Time Manager）**：维护全局时间（秒 + 纳秒）
- **同步管理器（Sync Manager）**：处理时间基之间的同步
- **持续时间计算器（Duration Calculator）**：计算两个时间点之间的间隔

### 3.3 文件结构

```
src/bsw/services/tm/
├── include/
│   └── Tm.h
└── src/
    └── Tm.c
```

---

## 4. 状态机

时间基状态：

```
TM_STATUS_FREE_RUNNING
    │ synchronization event
    ▼
TM_STATUS_SYNCHRONIZED
    │ stop request
    ▼
TM_STATUS_STOPPED
    │ restart
    ▼
TM_STATUS_RUNNING
    │ error
    ▼
TM_STATUS_ERROR
```

---

## 5. 核心数据结构

```c
/* 时间基类型 */
typedef uint64 Tm_TimeBaseType;
typedef uint32 Tm_DurationType;

/* 时间基状态 */
typedef enum {
    TM_STATUS_RUNNING,
    TM_STATUS_STOPPED,
    TM_STATUS_SYNCHRONIZED,
    TM_STATUS_FREE_RUNNING,
    TM_STATUS_ERROR
} Tm_StatusType;

/* 时间基信息 */
typedef struct {
    Tm_TimeBaseType currentValue;
    Tm_DurationType resolution;
    boolean isSynchronized;
    Tm_StatusType status;
} Tm_TimeBaseInfoType;

/* 全局时间 */
typedef struct {
    uint32 secondsHigh;
    uint32 secondsLow;
    uint32 nanoseconds;
} Tm_GlobalTimeType;

/* 配置 */
typedef struct {
    uint8 numTimeBases;
    Tm_DurationType defaultResolution;
    boolean enableSync;
} Tm_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| Tm_Init | `Std_ReturnType Tm_Init(const Tm_ConfigType* config)` | 初始化 | | SWS_Tm_00001 |
| Tm_DeInit | `void Tm_DeInit(void)` | 反初始化 | | SWS_Tm_00002 |
| Tm_MainFunction | `void Tm_MainFunction(void)` | 周期处理 | 递增本地时间 | SWS_Tm_00003 |
| Tm_GetTimeBaseValue | `Std_ReturnType Tm_GetTimeBaseValue(uint8 timeBaseId, Tm_TimeBaseType* value)` | 获取时间基值 | | SWS_Tm_00004 |
| Tm_SetTimeBaseValue | `Std_ReturnType Tm_SetTimeBaseValue(uint8 timeBaseId, Tm_TimeBaseType value)` | 设置时间基值 | | SWS_Tm_00005 |
| Tm_GetTimeBaseInfo | `Std_ReturnType Tm_GetTimeBaseInfo(uint8 timeBaseId, Tm_TimeBaseInfoType* info)` | 获取时间基信息 | | SWS_Tm_00006 |
| Tm_GetGlobalTime | `Std_ReturnType Tm_GetGlobalTime(Tm_GlobalTimeType* time)` | 获取全局时间 | | SWS_Tm_00007 |
| Tm_SetGlobalTime | `Std_ReturnType Tm_SetGlobalTime(const Tm_GlobalTimeType* time)` | 设置全局时间 | | SWS_Tm_00008 |
| Tm_SyncTimeBase | `Std_ReturnType Tm_SyncTimeBase(uint8 sourceId, uint8 targetId)` | 同步时间基 | | SWS_Tm_00009 |
| Tm_GetElapsedDuration | `Tm_DurationType Tm_GetElapsedDuration(uint8 timeBaseId, Tm_TimeBaseType since)` | 计算经过时间 | |  |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| — | 当前无回调接口 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | Init | DET_E_PARAM_POINTER |
| 0x01 | GetTimeBaseValue | DET_E_UNINIT, DET_E_PARAM_POINTER |
| 0x02 | SetTimeBaseValue | DET_E_UNINIT |
| 0x03 | GetTimeBaseInfo | DET_E_PARAM_POINTER |
| 0x04 | GetGlobalTime | DET_E_PARAM_POINTER |
| 0x05 | SetGlobalTime | DET_E_UNINIT |
| 0x06 | SyncTimeBase | DET_E_UNINIT |
| 0x07 | GetElapsedDuration | DET_E_UNINIT |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 Config 指针有效性
2. 检查是否已初始化（防止重复初始化）
3. 保存配置指针，清零本地时间
4. 设置初始化标志为 TRUE

### 7.2 全局时间获取

1. 调用 `Tm_GetGlobalTime`，检查指针有效性
2. 将内部 `Tm_LocalTime` 转换为秒和纳秒
3. `secondsLow = Tm_LocalTime / 1000`
4. `nanoseconds = (Tm_LocalTime % 1000) * 1000000`

### 7.3 持续时间计算

1. 调用 `Tm_GetElapsedDuration`，传入起始时间
2. 返回 `Tm_LocalTime - since`（若当前时间大于起始时间）
3. 否则返回 0

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| TM_DEV_ERROR_DETECT | STD_ON | 开发错误检测 |
| TM_MODULE_ID | 0x0C | 模块标识符 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| Tm_ConfigType | 通过 `Tm_Init` 参数传入 |

### 8.3 构建后配置

不适用。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| DET_E_PARAM_POINTER | 空指针 | Config 或输出参数为 NULL_PTR |
| DET_E_UNINIT | 未初始化 | 模块未初始化时调用 API |

### 9.2 DEM 错误

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| — | — | 当前未定义 DEM 事件 |

### 9.3 安全机制

- 初始化状态检查
- 参数有效性验证
- 时间回绕保护

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| 默认代码段 | Tm.c 全部函数 |
| 默认数据段 | 配置指针、初始化标志、本地时间 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~24 bytes | 配置指针 + 初始化标志 + 本地时间(uint64) |
| ROM | ~1.5 KB | 代码段（桩实现） |
| 堆栈 | ~64 bytes | 函数调用栈 |

---

## 11. 集成指南

- 与上层集成：各 BSW 模块通过 `Tm_GetGlobalTime`/`Tm_GetTimeBaseValue` 获取时间
- 与下层集成：依赖 Os/Mcal 提供硬件定时器访问
- 初始化顺序：Os → Det → Tm_Init
- MainFunction 周期建议：1ms（高精度时间基）

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| test_tm.c | 初始化/反初始化、时间基读写、全局时间转换、持续时间计算、空指针检测 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 时间基同步 | 验证 SyncTimeBase 的正确性 |
| 全局时间精度 | 验证秒/纳秒转换精度 |
| 时间回绕 | 验证 64 位时间基回绕处理 |

---

## 13. 实现说明 / TODO

- 当前为桩实现，仅维护单一 `Tm_LocalTime` 变量
- 未实现多时间基管理（`numTimeBases` 配置未使用）
- `Tm_SetGlobalTime` 为空实现，需要与外部时间源（如 gPTP）集成
- `Tm_SyncTimeBase` 为空实现，需要实现时间基同步算法
- `Tm_MainFunction` 仅简单递增，需要集成真实硬件定时器
- 需要添加时间基状态管理

---

## 14. 参考资料

1. AUTOSAR_SWS_Tm.pdf (R21-11 §12.15)
2. `docs/modules/tm.md`
3. `src/bsw/services/tm/`
