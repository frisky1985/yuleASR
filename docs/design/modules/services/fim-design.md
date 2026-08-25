# FiM Design Document

> **Module ID**: 0x65  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_FunctionInhibitionManager  
> **Source Path**: `src/bsw/services/fim/`  
> **Reference Document**: `docs/modules/fim.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

FiM (Function Inhibition Manager) 是 AUTOSAR BSW 服务层的功能抑制管理模块，负责基于诊断事件（DTC）状态管理功能权限。当 Dem 检测到特定诊断事件状态变化时，FiM 根据配置的抑制规则自动禁止或恢复相关功能的执行权限。FiM 是 ASIL 安全链路中的关键组件，确保在故障条件下相关功能被正确抑制，防止不安全行为。

FiM 模块支持以下核心能力：
- 基于 Dem 事件状态（TestFailed、Pending、Confirmed 等）的功能抑制
- 支持 Summary Event 聚合多个事件的抑制判断
- 功能可用性（Availability）管理
- 运行时权限查询与抑制状态查询

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS FunctionInhibitionManager | 4.4.0 | FiM 模块规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SWC / Com | 调用 FiM_GetFunctionPermission 查询功能权限 |
| 下层 | Dem | 提供事件状态（Dem_GetEventFailed），FiM 监听事件变化 |
| 下层 | Det | 开发错误报告 |
| 下层 | EcuM | 初始化阶段调用 FiM_Init |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│    SWC / Com (Function callers)     │
├─────────────────────────────────────┤
│        FiM (Services Layer)         │
├─────────────────────────────────────┤
│     Dem (Event Status Provider)     │
│     Det (Error Tracing)             │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Function State Manager**: 管理每个 Function ID 的权限状态、可用性、抑制状态
- **Inhibition Calculator**: 根据关联的 Dem 事件状态计算抑制掩码
- **Summary Event Manager**: 管理聚合事件状态，将多个事件合并为一个抑制判断源
- **Event Inhibition Checker**: 检查单个事件的 UDS 状态字节是否匹配配置的抑制条件

### 3.3 文件结构

```
src/bsw/services/fim/
├── include/
│   ├── FiM.h           # 公共 API 声明、类型定义
│   └── FiM_Cfg.h       # 预编译配置（函数数量、事件数、周期等）
└── src/
    ├── FiM.c            # 核心实现
    └── FiM_Lcfg.c       # 链接时配置
```

---

## 4. 状态机

```
          FiM_Init()
UNINIT ──────────────► INIT
  ▲                      │
  │    FiM_DeInit()      │
  └──────────────────────┘
                         │
          ┌──────────────┴──────────────┐
          │                             │
   DemTriggerOnEventStatus()    MainFunction()
   (更新抑制掩码)               (周期性重新计算权限)
```

FiM 模块有两个状态：
- **UNINIT (0x00)**: 模块未初始化，所有 API 调用将被 DET 拒绝
- **INIT (0x01)**: 模块已初始化，正常处理功能权限管理

---

## 5. 核心数据结构

### 5.1 功能状态类型

```c
typedef struct {
    FiM_PermissionStateType Permission;           /* 当前权限 (ALLOWED/DENIED) */
    boolean Available;                             /* 功能可用性 */
    FiM_InhibitionStatusType InhibitionStatus;     /* 抑制状态 */
    uint8 LastCalculatedInhibitionMask;            /* 最近计算的抑制掩码 */
} FiM_FunctionStateType;
```

### 5.2 事件抑制配置类型

```c
typedef struct {
    Dem_EventIdType EventId;                       /* 关联的 Dem 事件 ID */
    uint8 InhibitionMask;                          /* 抑制条件掩码 */
    boolean UseSummaryEvent;                       /* 是否使用聚合事件 */
    FiM_SummaryEventIdType SummaryEventId;         /* 聚合事件 ID */
} FiM_EventInhibitionType;
```

### 5.3 功能配置类型

```c
typedef struct {
    FiM_FunctionIdType FunctionId;                 /* 功能 ID */
    const FiM_EventInhibitionType* EventInhibitions; /* 事件抑制数组 */
    uint8 NumEventInhibitions;                     /* 事件抑制数量 */
    boolean FunctionAvailable;                     /* 默认可用性 */
} FiM_FunctionConfigType;
```

### 5.4 模块内部状态

```c
typedef struct {
    uint8 State;                                   /* 模块状态 (UNINIT/INIT) */
    const FiM_ConfigType* ConfigPtr;               /* 配置指针 */
    FiM_FunctionStateType FunctionStates[FIM_NUM_FUNCTIONS];        /* 功能状态数组 */
    FiM_SummaryEventStateType SummaryEventStates[FIM_NUM_SUMMARY_EVENTS]; /* 聚合事件状态 */
} FiM_InternalStateType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | Service ID | 说明 | SWS 需求 |
|-----|-----------|------|----------|
| `void FiM_Init(const FiM_ConfigType* ConfigPtr)` | 0x01 | 初始化 FiM 模块，配置功能状态 | SWS_FiM_00001 |
| `void FiM_DeInit(void)` | 0x02 | 反初始化，清除配置指针 | SWS_FiM_00002 |
| `void FiM_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 0x03 | 获取版本信息 | SWS_FiM_00003 |
| `Std_ReturnType FiM_SetFunctionAvailable(FiM_FunctionIdType FID, boolean Availability)` | 0x04 | 设置功能可用性 | SWS_FiM_00004 |
| `Std_ReturnType FiM_GetFunctionPermission(FiM_FunctionIdType FID, FiM_PermissionStateType* Permission)` | 0x05 | 获取功能权限 | SWS_FiM_00005 |
| `Std_ReturnType FiM_SetFunctionPermission(FiM_FunctionIdType FID, FiM_PermissionStateType Permission)` | 0x06 | 设置功能权限（测试用） | SWS_FiM_00006 |
| `Std_ReturnType FiM_GetInhibitionStatus(FiM_FunctionIdType FID, FiM_InhibitionStatusType* InhibitionStatus)` | 0x07 | 获取抑制状态 | SWS_FiM_00007 |
| `void FiM_DemTriggerOnMonitorStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)` | 0x08 | Dem 监控状态变化触发 | SWS_FiM_00100 |
| `void FiM_DemTriggerOnEventStatus(Dem_EventIdType EventId, Dem_UdsStatusByteType EventStatusOld, Dem_UdsStatusByteType EventStatusNew)` | 0x09 | Dem 事件状态变化触发 | SWS_FiM_00101 |
| `void FiM_MainFunction(void)` | 0x0A | 周期性主函数 | SWS_FiM_00102 |

### 6.2 回调函数

FiM 不定义回调接口。Dem 通过 `FiM_DemTriggerOnEventStatus()` 和 `FiM_DemTriggerOnMonitorStatus()` 主动通知事件状态变化。

### 6.3 服务 ID 与错误码

**Service IDs:**

| SID | 名称 | 值 |
|-----|------|-----|
| FIM_SID_INIT | Init | 0x01 |
| FIM_SID_DEINIT | DeInit | 0x02 |
| FIM_SID_GETVERSIONINFO | GetVersionInfo | 0x03 |
| FIM_SID_SETFUNCTIONAVAILABLE | SetFunctionAvailable | 0x04 |
| FIM_SID_GETFUNCTIONPERMISSION | GetFunctionPermission | 0x05 |
| FIM_SID_SETFUNCTIONPERMISSION | SetFunctionPermission | 0x06 |
| FIM_SID_GETINHIBITIONSTATUS | GetInhibitionStatus | 0x07 |
| FIM_SID_DEMTRIGGERONMONITORSTATUS | DemTriggerOnMonitorStatus | 0x08 |
| FIM_SID_DEMTRIGGERONEVENTSTATUS | DemTriggerOnEventStatus | 0x09 |
| FIM_SID_MAINFUNCTION | MainFunction | 0x0A |

**DET Error Codes:**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| FIM_E_PARAM_CONFIG | 0x01 | 配置参数无效 |
| FIM_E_PARAM_POINTER | 0x02 | NULL 指针 |
| FIM_E_PARAM_FID | 0x03 | 无效 Function ID |
| FIM_E_PARAM_EVENTID | 0x04 | 无效 Event ID |
| FIM_E_UNINIT | 0x05 | 模块未初始化 |
| FIM_E_INIT_FAILED | 0x06 | 初始化失败 |

**Inhibition Mask 类型:**

| 掩码 | 值 | 说明 |
|------|-----|------|
| FIM_INHIBITION_MASK_NONE | 0x00 | 无抑制 |
| FIM_INHIBITION_MASK_TEST_FAILED | 0x01 | TestFailed 时抑制 |
| FIM_INHIBITION_MASK_TEST_FAILED_TOC | 0x02 | TestFailedThisOperationCycle 时抑制 |
| FIM_INHIBITION_MASK_PENDING | 0x04 | Pending DTC 时抑制 |
| FIM_INHIBITION_MASK_CONFIRMED | 0x08 | Confirmed DTC 时抑制 |
| FIM_INHIBITION_MASK_TEST_NOT_COMPLETED | 0x10 | 测试未完成时抑制 |
| FIM_INHIBITION_MASK_WARNING_INDICATOR | 0x20 | 警告指示器时抑制 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 ConfigPtr 是否为 NULL，若为 NULL 则报告 DET 错误并返回
2. 检查 NumFunctions 是否超过 FIM_NUM_FUNCTIONS 上限
3. 存储配置指针到内部状态
4. 初始化所有 FunctionStates 为默认权限（FIM_PERMISSION_ALLOWED）和默认可用性
5. 初始化所有 SummaryEventStates 为未失败
6. 遍历所有已配置功能，调用 `FiM_UpdateFunctionPermission()` 计算初始权限
7. 设置模块状态为 FIM_STATE_INIT

### 7.2 抑制计算流程

1. 根据 FunctionId 查找功能配置
2. 遍历该功能的所有事件抑制配置
3. 对每个事件抑制：
   - 若使用 Summary Event，检查聚合事件状态
   - 否则调用 `Dem_GetEventFailed()` 获取事件状态
   - 构建 UDS 状态字节，检查是否匹配抑制条件
4. 合并所有匹配的抑制掩码
5. 若最终掩码非零，设置 Permission = DENIED，InhibitionStatus = YES
6. 若掩码为零，设置 Permission = ALLOWED，InhibitionStatus = NO

### 7.3 Dem 事件触发流程

1. Dem 调用 `FiM_DemTriggerOnEventStatus(EventId, Old, New)`
2. FiM 遍历所有功能配置，查找关联此 EventId 的功能
3. 对每个受影响的功能重新计算抑制掩码
4. 更新功能权限状态

---

## 8. 配置设计

### 8.1 预编译配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `FIM_DEV_ERROR_DETECT` | STD_ON | 启用开发错误检测 |
| `FIM_VERSION_INFO_API` | STD_ON | 启用版本信息 API |
| `FIM_NUM_FUNCTIONS` | 32U | 最大功能数量 |
| `FIM_NUM_EVENTS_PER_FUNCTION` | 8U | 每个功能最大关联事件数 |
| `FIM_NUM_SUMMARY_EVENTS` | 16U | 聚合事件数量 |
| `FIM_MAIN_FUNCTION_PERIOD_MS` | 10U | 主函数周期 (ms) |
| `FIM_FID_MIN` | 1U | 最小有效 Function ID |
| `FIM_FID_MAX` | 31U | 最大有效 Function ID |
| `FIM_DEFAULT_PERMISSION` | FIM_PERMISSION_ALLOWED | 默认权限 |
| `FIM_DEFAULT_AVAILABILITY` | STD_ON | 默认可用性 |

### 8.2 链接时配置

通过 `FiM_Lcfg.c` 提供 `FiM_Config` 全局常量，包含功能配置数组和聚合事件数组。

### 8.3 构建后配置

不支持构建后配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 场景 | API | 错误码 |
|------|-----|--------|
| ConfigPtr 为 NULL | FiM_Init | FIM_E_PARAM_POINTER |
| NumFunctions 超限 | FiM_Init | FIM_E_PARAM_CONFIG |
| 模块未初始化时调用 | 所有 API | FIM_E_UNINIT |
| FID 超出范围 | Set/Get API | FIM_E_PARAM_FID |
| 输出指针为 NULL | Get API | FIM_E_PARAM_POINTER |

### 9.2 DEM 错误

FiM 不直接报告 DEM 事件。FiM 的抑制状态可作为 Dem 事件的消费者，而非生产者。

### 9.3 安全机制

- 所有公共 API 在 DEV_ERROR_DETECT 启用时进行参数校验
- FunctionId 范围检查（FIM_FID_MIN ~ FIM_FID_MAX）
- 抑制掩码计算使用位运算，确保确定性行为
- 默认权限为 ALLOWED，符合故障安全原则（无配置时功能可用）

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 段 | 变量 | 说明 |
|----|------|------|
| FIM_START_SEC_VAR_CLEARED_UNSPECIFIED | FiM_InternalState | 模块内部状态 |
| FIM_START_SEC_CONFIG_DATA_UNSPECIFIED | FiM_Config | 全局配置 |
| FIM_START_SEC_CODE | 所有函数 | 代码段 |

### 10.2 资源估算

- **RAM**: FiM_InternalState = sizeof(FiM_FunctionStateType) × 32 + sizeof(FiM_SummaryEventStateType) × 16 + 8 ≈ 32×4 + 16×2 + 8 = 192 字节
- **ROM**: ~3 KB（代码段 + 配置常量）
- **性能**: GetFunctionPermission 为 O(1) 查询；CalculateInhibitionMask 为 O(N) N=事件数

---

## 11. 集成指南

- SWC 在功能执行前调用 `FiM_GetFunctionPermission(FID, &permission)` 检查权限
- Dem 在事件状态变化时调用 `FiM_DemTriggerOnEventStatus()` 通知 FiM
- EcuM 在启动序列中调用 `FiM_Init()` 初始化
- SCHM 以 10ms 周期调用 `FiM_MainFunction()` 进行周期性权限重新计算
- Function ID 范围 1~31，需在配置中与 Dem EventId 建立映射关系

---

## 12. 测试策略

### 12.1 单元测试

- 初始化/反初始化测试（NULL 指针、重复初始化）
- 单个功能权限查询测试
- 抑制掩码计算测试（TestFailed、Pending、Confirmed 各条件组合）
- Summary Event 聚合逻辑测试
- Function Availability 对权限的影响测试
- FID 边界值测试（0, 1, 31, 32）

### 12.2 集成测试

- Dem 事件状态变化 → FiM 权限更新 → SWC 权限查询完整链路
- 多事件同时触发抑制的优先级测试
- MainFunction 周期性重新计算的正确性

---

## 13. 实现说明 / TODO

- 当前实现中 `FiM_DemTriggerOnMonitorStatus` 和 `FiM_DemTriggerOnEventStatus` 函数体在源码中未完整展示（FiM.c 截断于 533 行），需补充完整实现
- Summary Event 的状态更新通过 `Dem_GetEventFailed()` 查询，可优化为事件驱动
- 抑制掩码与 UDS 状态字节的映射关系需与 Dem 模块对齐

---

## 14. 参考资料

- AUTOSAR_SWS_FunctionInhibitionManager.pdf (R4.4.0)
- AUTOSAR_SWS_EventStatusManagement.pdf
- yuleASR FiM 模块源码: `src/bsw/services/fim/`
