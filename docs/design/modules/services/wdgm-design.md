# WdgM (Watchdog Manager) Design Document

> **Module ID**: 0x0D  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.7.0  
> **SWS Reference**: AUTOSAR_SWS_WatchdogManager  
> **Source Path**: `src/bsw/services/wdgm/`  
> **Reference Document**: `docs/modules/WDGM.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

WdgM 负责监督 ECU 中各软件实体的活性（Alive Supervision），并在此基础上控制看门狗硬件的触发。它支持窗口看门狗（WWD）与独立看门狗（IWD）两种硬件类型，并与 Lockstep、RamSafety 等安全模块集成，可在检测到故障时执行复位等安全动作。

主要功能：
- 初始化/反初始化看门狗管理模块
- 设置看门狗模式（OFF / SLOW / FAST）
- 接收各监督实体的检查点报告（Checkpoint）
- 周期性评估监督实体活性状态
- 在窗口内触发底层看门狗
- 处理 Lockstep / RamSafety 错误事件
- 在监督失败或安全事件时执行复位

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Watchdog Manager | 4.7.0 | 模块软件规范 |
| AUTOSAR Classic Platform | 4.7.x | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | RTE / SW-C | 调用 `WdgM_CheckpointReached()` 报告检查点 |
| 同层 | Lockstep | 接收 Lockstep 错误通知 |
| 同层 | RamSafety | 接收 RamSafety 错误通知 |
| 下层 | Wdg Driver | 通过 `WdgM_WatchdogTrigger()` / `WdgM_WatchdogSetMode()` 调用 |
| 公共 | Det | 开发错误检测（可选） |
| 公共 | Dem | 监督超时等诊断事件（可选） |
| 公共 | Mcal | 中断使能/禁用 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        RTE / SW-C / Safety Modules  │
├─────────────────────────────────────┤
│            WdgM (Services)          │
├─────────────────────────────────────┤
│        Wdg Driver (MCAL/ECUAL)      │
├─────────────────────────────────────┤
│        Watchdog Hardware (WWD/IWD)  │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **状态机核心**：维护 WdgM 模块状态与当前模式。
- **监督实体管理器**：维护最多 `WDGM_CFG_MAX_SUPERVISED_ENTITIES` 个实体的运行时状态。
- **活性监督器**：在 `WdgM_MainFunction()` 中检查每个实体的检查点计数是否满足期望。
- **看门狗触发器**：按 `supervisionCycleMs` 周期调用 `WdgM_WatchdogTrigger()`。
- **安全事件处理器**：处理 Lockstep/RamSafety 错误，执行安全响应。
- **事件通知器**：通过注册的 `WdgM_SafetyCallback` 通知应用层安全事件。

### 3.3 文件结构

```
src/bsw/services/wdgm/
├── include/
│   ├── WdgM.h
│   ├── WdgM_Cfg.h
│   └── WdgM_MemMap.h
└── src/
    ├── WdgM.c
    └── WdgM_Cfg.c
```

---

## 4. 状态机

### 4.1 模块状态

```
UNINIT -- WdgM_Init() --> INIT --> ACTIVE
ACTIVE -- WdgM_DeInit() --> UNINIT
ACTIVE -- 监督超时 --> SUPERVISION_EXPIRED -- 安全动作 --> 复位
```

### 4.2 监督实体状态

| 状态 | 说明 |
|------|------|
| `WDGM_SE_STATE_CORRECT` | 实体监督正常 |
| `WDGM_SE_STATE_INCORRECT` | 实体监督异常 |
| `WDGM_SE_STATE_EXPIRED` | 实体监督超时 |
| `WDGM_SE_STATE_DEACTIVATED` | 实体被去激活 |

### 4.3 看门狗模式

| 模式 | 值 | 说明 |
|------|-----|------|
| `WDGM_WATCHDOG_MODE_OFF` | 0x00 | 关闭 |
| `WDGM_WATCHDOG_MODE_SLOW` | 0x01 | 慢速模式 |
| `WDGM_WATCHDOG_MODE_FAST` | 0x02 | 快速模式 |

---

## 5. 核心数据结构

### 5.1 看门狗配置

```c
typedef enum {
    WDGM_WATCHDOG_WWD = 0,
    WDGM_WATCHDOG_IWD
} WdgM_WatchdogType;

typedef struct {
    WdgM_WatchdogType type;
    uint16 triggerPeriodMs;
    uint16 windowStartMs;
    uint16 windowEndMs;
    boolean enabled;
} WdgM_WatchdogConfigType;
```

### 5.2 监督实体配置

```c
typedef struct {
    uint16 seId;
    WdgM_SupervisionType supervisionType;
    boolean enabled;
    struct {
        WdgM_AliveSupervisionType alive;
        WdgM_DeadlineSupervisionType deadline;
    } config;
} WdgM_SupervisedEntityConfigType;
```

### 5.3 监督实体运行时

```c
typedef struct {
    uint16 seId;
    WdgM_SEStateType state;
    uint16 aliveCounter;
    uint16 expectedAliveIndications;
    uint32 timestampStart;
    uint32 timestampStop;
    uint8 consecutiveErrors;
    boolean deactivated;
} WdgM_SupervisedEntityType;
```

### 5.4 全局状态

```c
typedef struct {
    uint32 expiredSupervisionCycles;
    uint32 totalRefreshes;
    uint32 failedRefreshes;
    uint32 lockstepErrors;
    uint32 ramSafetyErrors;
    uint8 currentMode;
} WdgM_GlobalStatusType;
```

### 5.5 安全回调

```c
typedef void (*WdgM_SafetyCallbackType)(
    uint8 eventType,
    uint32 errorCode,
    const void* context
);
```

---

## 6. API 设计

### 6.1 公共接口

| API | SWS 需求 | 签名 | 功能 | 备注 |
|-----|----------|------|------|------|
| `WdgM_Init` | SWS_WdgM_00001 | `Std_ReturnType WdgM_Init(const WdgM_ConfigType* config)` | 初始化 WdgM | 配置监督实体、设置安全魔数 |
| `WdgM_DeInit` | SWS_WdgM_00002 | `Std_ReturnType WdgM_DeInit(void)` | 反初始化 | 需允许禁用 |
| `WdgM_GetState` | SWS_WdgM_00003 | `WdgM_StateType WdgM_GetState(void)` | 获取模块状态 | |
| `WdgM_SetMode` | SWS_WdgM_00004 | `Std_ReturnType WdgM_SetMode(uint8 mode)` | 设置看门狗模式 | 需允许关闭 |
| `WdgM_GetMode` | SWS_WdgM_00005 | `uint8 WdgM_GetMode(void)` | 获取当前模式 | |
| `WdgM_IsDisableAllowed` | SWS_WdgM_00006 | `boolean WdgM_IsDisableAllowed(void)` | 查询是否允许禁用 | |
| `WdgM_CheckpointReached` | SWS_WdgM_00007 | `Std_ReturnType WdgM_CheckpointReached(uint16 seId)` | 报告检查点 | 增加活计数 |
| `WdgM_UpdateAliveIndication` | SWS_WdgM_00008 | `Std_ReturnType WdgM_UpdateAliveIndication(uint16 seId)` | 更新活指示 | 实际调用 CheckpointReached |
| `WdgM_GetSEState` | SWS_WdgM_00009 | `Std_ReturnType WdgM_GetSEState(uint16 seId, WdgM_SEStateType* state)` | 获取实体状态 | |
| `WdgM_DeactivateSupervisionEntity` | SWS_WdgM_00010 | `Std_ReturnType WdgM_DeactivateSupervisionEntity(uint16 seId)` | 去激活实体 | |
| `WdgM_ActivateSupervisionEntity` | SWS_WdgM_00011 | `Std_ReturnType WdgM_ActivateSupervisionEntity(uint16 seId)` | 激活实体 | |
| `WdgM_GetGlobalStatus` | SWS_WdgM_00012 | `Std_ReturnType WdgM_GetGlobalStatus(WdgM_GlobalStatusType* status)` | 获取全局统计 | |
| `WdgM_MainFunction` | SWS_WdgM_00013 | `void WdgM_MainFunction(void)` | 周期监督与触发 | 建议 10ms |
| `WdgM_TriggerWatchdog` | SWS_WdgM_00014 | `void WdgM_TriggerWatchdog(void)` | 触发看门狗 | |
| `WdgM_PerformReset` | SWS_WdgM_00015 | `void WdgM_PerformReset(void)` | 执行立即复位 | |
| `WdgM_GetFirstExpiredSEID` | SWS_WdgM_00016 | `Std_ReturnType WdgM_GetFirstExpiredSEID(uint16* seId)` | 获取首个超时实体 ID | |
| `WdgM_HandleLockstepError` | SWS_WdgM_00017 | `void WdgM_HandleLockstepError(uint32 errorCode)` | 处理 Lockstep 错误 | |
| `WdgM_HandleRamSafetyError` | SWS_WdgM_00018 | `void WdgM_HandleRamSafetyError(uint32 errorCode)` | 处理 RamSafety 错误 | |
| `WdgM_RegisterSafetyCallback` | SWS_WdgM_00019 | `Std_ReturnType WdgM_RegisterSafetyCallback(...)` | 注册安全事件回调 | |
| `WdgM_GetVersionInfo` | SWS_WdgM_00020 | `void WdgM_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 版本信息 | |

### 6.2 配置回调

| 回调 | SWS 需求 | 说明 |
|------|----------|------|
| `WdgM_WatchdogTrigger` | SWS_WdgM_00111 | 底层看门狗触发函数 |
| `WdgM_WatchdogSetMode` | SWS_WdgM_00112 | 底层看门狗模式设置函数 |
| `WdgM_SafetyEventCallback` | SWS_WdgM_00113 | 安全事件回调（应用层实现） |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | Init | `WDGM_E_PARAM_POINTER`, `WDGM_E_ALREADY_INITIALIZED` |
| 0x01 | DeInit | `WDGM_E_NOT_INITIALIZED`, `WDGM_E_DISABLE_NOT_ALLOWED` |
| 0x02 | GetState | - |
| 0x03 | SetMode | `WDGM_E_NOT_INITIALIZED`, `WDGM_E_PARAM_MODE`, `WDGM_E_DISABLE_NOT_ALLOWED` |
| 0x0E | CheckpointReached | `WDGM_E_NOT_INITIALIZED`, `WDGM_E_CPID_NOT_CONFIGURED` |
| 0x11 | UpdateAliveIndication | `WDGM_E_NOT_INITIALIZED` |
| 0x08 | MainFunction | - |
| 0x09 | TriggerWatchdog | - |
| 0x20 | HandleLockstepError | `WDGM_E_NOT_INITIALIZED` |
| 0x21 | HandleRamSafetyError | `WDGM_E_NOT_INITIALIZED` |

---

## 7. 处理流程

### 7.1 初始化

1. `WdgM_Init()` 验证配置非空且未初始化。
2. 调用 `Mcal_DisableAllInterrupts()`。
3. 初始化所有监督实体为 `DEACTIVATED`。
4. 根据配置使能部分实体，状态设为 `CORRECT`。
5. 初始化全局状态、计时器、错误计数。
6. 设置安全魔数：`WDGM_SAFETY_MAGIC_INIT` → `WDGM_SAFETY_MAGIC_ACTIVE`。
7. 状态置为 `ACTIVE`，通知事件回调。

### 7.2 检查点报告

1. `WdgM_CheckpointReached(seId)` 查找实体索引。
2. 更新 `timestampStop`、递增 `aliveCounter`、清除连续错误。
3. 若实体未去激活，状态设为 `CORRECT`。

### 7.3 周期监督

1. `WdgM_MainFunction()` 检查安全魔数。
2. 递增 `WdgM_CycleTimer` 与 `WdgM_TriggerTimer`。
3. 调用 `WdgM_UpdateSupervision()`：
   - 对每个激活实体调用 `WdgM_CheckEntityAlive()`。
   - 若 `aliveCounter < expectedAliveIndications`，递增 `consecutiveErrors`。
   - 当 `consecutiveErrors >= WDGM_CFG_FAILURE_THRESHOLD` 时，调用 `WdgM_HandleExpiredSupervision()`。
4. 若 `TriggerTimer >= supervisionCycleMs`，重置计时器并在 `ACTIVE` 状态下触发看门狗。
5. 若 `WdgM_ConsecutiveErrors >= failureThreshold`，执行安全动作（复位）。

### 7.4 安全事件响应

1. `WdgM_HandleLockstepError()` / `WdgM_HandleRamSafetyError()` 更新错误计数。
2. 通知安全事件回调。
3. 调用 `WdgM_PerformSafetyAction()`，最终执行 `WdgM_PerformReset()`。

### 7.5 看门狗触发

1. `WdgM_TriggerWatchdog()` 检查模块状态与安全魔数。
2. 调用 `WdgM_PlatformTrigger()` → `WdgM_WatchdogTrigger()`。
3. 递增 `totalRefreshes`。

---

## 8. 配置设计

### 8.1 预编译配置（`WdgM_Cfg.h`）

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `WDGM_CFG_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `WDGM_CFG_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `WDGM_CFG_WWD_ENABLE` | STD_ON | 窗口看门狗使能 |
| `WDGM_CFG_IWD_ENABLE` | STD_ON | 独立看门狗使能 |
| `WDGM_CFG_SUPERVISION_CYCLE_MS` | 10U | 监督周期 |
| `WDGM_CFG_FAILURE_THRESHOLD` | 3U | 连续错误阈值 |
| `WDGM_CFG_MAX_SUPERVISED_ENTITIES` | 8U | 最大监督实体数 |
| `WDGM_CFG_LOCKSTEP_INTEGRATION` | STD_ON | Lockstep 集成 |
| `WDGM_CFG_RAMSAFETY_INTEGRATION` | STD_ON | RamSafety 集成 |
| `WDGM_CFG_DEM_INTEGRATION` | STD_ON | DEM 集成 |
| `WDGM_CFG_WWD_TRIGGER_PERIOD_MS` | 50U | WWD 触发周期 |
| `WDGM_CFG_WWD_TIMEOUT_MS` | 100U | WWD 超时 |
| `WDGM_CFG_IWD_TRIGGER_PERIOD_MS` | 100U | IWD 触发周期 |
| `WDGM_CFG_IWD_TIMEOUT_MS` | 200U | IWD 超时 |
| `WDGM_CFG_WWD_WINDOW_START_PERCENT` | 50U | WWD 窗口起始百分比 |
| `WDGM_CFG_WWD_WINDOW_END_PERCENT` | 100U | WWD 窗口结束百分比 |
| 预定义 SEID | 1U-7U | 主循环、通信、诊断、存储、安全监控、Lockstep、RamSafety |
| DEM 事件 ID | 1U-4U | 监督超时、设置模式失败、Lockstep 错误、RamSafety 错误 |

### 8.2 链接时配置

`WdgM_Config` 与 `WdgM_ConfigDebug` 在 `WdgM_Cfg.c` 中定义，包含：

- 看门狗配置数组
- 监督实体配置数组
- 失败阈值与监督周期

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x10 | `WDGM_E_NOT_INITIALIZED` | API 在初始化前调用 |
| 0x11 | `WDGM_E_ALREADY_INITIALIZED` | 重复初始化 |
| 0x12 | `WDGM_E_PARAM_POINTER` | 空指针入参 |
| 0x13 | `WDGM_E_PARAM_SEID` | 非法监督实体 ID |
| 0x14 | `WDGM_E_PARAM_MODE` | 非法模式 |
| 0x15 | `WDGM_E_DISABLE_NOT_ALLOWED` | 不允许禁用时尝试关闭 |
| 0x16 | `WDGM_E_SET_MODE_FAILED` | 设置模式失败 |
| 0x17 | `WDGM_E_DATA_CORRUPTION` | 安全魔数校验失败 |
| 0x18 | `WDGM_E_CPID_NOT_CONFIGURED` | 检查点 ID 未配置 |
| 0x19 | `WDGM_E_SUPERVISION_EXPIRED` | 监督超时 |

### 9.2 DEM 事件

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| 1 | `WDGM_DEM_SUPERVISION_EXPIRED_EVENT_ID` | 监督实体超时 |
| 2 | `WDGM_DEM_SET_MODE_FAILED_EVENT_ID` | 设置模式失败 |
| 3 | `WDGM_DEM_LOCKSTEP_ERROR_EVENT_ID` | Lockstep 错误 |
| 4 | `WDGM_DEM_RAMSAFETY_ERROR_EVENT_ID` | RamSafety 错误 |

### 9.3 安全机制

- ASIL-D 级别设计，使用安全魔数进行运行时完整性检查。
- 初始化与反初始化时关中断，防止竞争条件。
- 监督超时、Lockstep/RamSafety 错误均触发复位。
- 看门狗触发仅在 `ACTIVE` 状态下执行，避免未初始化时误触发。

---

## 10. 内存与性能

### 10.1 MemMap 分区

当前实现使用 `WdgM_MemMap.h` 分区：

| 分区 | 用途 |
|------|------|
| `WDGM_START_SEC_VAR_INIT_UNSPECIFIED` | 初始化状态变量 |
| `WDGM_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化运行时数组 |
| `WDGM_START_SEC_CODE` | 代码段 |
| `WDGM_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~300 B | 监督实体数组、全局状态、计时器 |
| ROM | ~10 KB | 代码与配置表 |
| 周期 | 低 | 主函数仅遍历实体并触发看门狗 |

---

## 11. 集成指南

- **看门狗驱动**：实现 `WdgM_WatchdogTrigger()` 与 `WdgM_WatchdogSetMode()`，对接具体 WWD/IWD 驱动。
- **监督实体分配**：为关键任务分配唯一 SEID（如主循环、通信、诊断、安全监控）。
- **检查点调用**：在任务关键位置调用 `WdgM_CheckpointReached(seId)`。
- **Lockstep/RamSafety**：将错误事件路由到 `WdgM_HandleLockstepError()` / `WdgM_HandleRamSafetyError()`。
- **安全回调**：注册 `WdgM_SafetyCallback` 以接收事件通知，便于记录诊断信息。
- **Mcal 集成**：确保 `Mcal_DisableAllInterrupts()` / `Mcal_EnableAllInterrupts()` 可用。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `tests/unit/autosar/services/WdgM_Test.c` | 初始化、模式设置、检查点、监督超时、Lockstep/RamSafety 错误 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 正常监督 | 周期报告检查点 → 看门狗正常触发 |
| 监督超时 | 停止报告检查点 → 连续错误达到阈值 → 复位 |
| 模式切换 | SLOW ↔ FAST ↔ OFF |
| 安全事件 | Lockstep/RamSafety 错误 → 复位 |
| 去激活/激活实体 | 去激活后不再监督 |

---

## 13. 实现说明 / TODO

- `WdgM_PlatformTrigger()` / `WdgM_PlatformSetMode()` 当前直接调用配置回调，未进行窗口时间检查。
- `WdgM_PerformReset()` 中硬件复位调用被注释，当前进入死循环作为 fallback。
- DEM 报告当前为注释，需根据项目需求启用。
- 当前活性监督逻辑较简单，未完整实现参考周期与最小/最大活计数比较。
- Deadline Supervision 与 Logical Supervision 类型在配置结构中存在，但主函数中未实际处理。
- `WdgM_DisableAllowed` 当前为 FALSE，需根据项目需求在适当时机设置。

---

## 14. 参考资料

1. AUTOSAR_SWS_WatchdogManager.pdf
2. `docs/modules/WDGM.md`
3. `src/bsw/services/wdgm/WdgM.h`
4. `src/bsw/services/wdgm/WdgM.c`
5. `src/bsw/services/wdgm/WdgM_Cfg.h`
