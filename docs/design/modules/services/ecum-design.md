# EcuM (ECU State Manager) Design Document

> **Module ID**: 0x0A  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.0.3  
> **SWS Reference**: AUTOSAR_SWS_ECUStateManager  
> **Source Path**: `src/bsw/services/ecum/`  
> **Reference Document**: `docs/modules/ECUM.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

EcuM 负责管理 ECU 的整体状态，包括多阶段启动（Startup）、正常运行（RUN）、睡眠（Sleep）、唤醒（Wakeup）和关闭（Shutdown）。它通过 RUN 请求机制协调各模块/用户对 ECU 运行状态的需求，并通过 BswM 通知当前 ECU 状态和唤醒事件。

主要功能：
- 多阶段启动序列（StartupOne / StartupTwo / StartupThree）
- RUN / POST_RUN 状态管理与请求计数
- 睡眠、Halt、Poll 模式进入与唤醒恢复
- 关闭目标选择（OFF / RESET / SLEEP）与关闭原因记录
- 唤醒源注册、使能、验证与过期处理
- 启动目标（OEM Bootloader / Sys Bootloader / Application）选择

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS ECU State Manager | 4.0.3 | 模块软件规范 |
| AUTOSAR Classic Platform | 4.0.x | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | RTE, SW-C | 通过 RTE 调用 RUN 请求与状态查询 |
| 同层 | BswM | 状态变更与唤醒事件通知 |
| 同层 | NvM | 启动时 ReadAll，关闭时 WriteAll |
| 同层 | ComM | 通信模式请求与释放 |
| 同层 | WdgM | 启动/关闭/睡眠阶段的看门狗管理 |
| 同层 | SchM | BSW 调度器初始化 |
| 下层 | Mcu, Port, Dio, Gpt | 启动阶段底层驱动初始化（通过 callout） |
| 公共 | Det | 开发错误检测（可选） |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│           RTE / SW-C                │
├─────────────────────────────────────┤
│  BswM        ComM       NvM  WdgM   │
├─────────────────────────────────────┤
│            EcuM (Services)          │
├─────────────────────────────────────┤
│  SchM      Mcu/Port/Dio/Gpt         │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **状态机核心**：维护 ECU 主状态、子状态、RUN 请求、关闭目标。
- **启动管理器**：按 StartupOne/Two/Three 分阶段初始化 BSW 与应用。
- **关闭管理器**：执行 GoOffOne（写 NV、反初始化）和 GoOffTwo（关 OS、复位/断电）。
- **睡眠管理器**：处理 GoSleep、GoHalt、GoPoll 及 WakeupRestart。
- **唤醒源管理器**：维护 32 个唤醒源的状态（Pending / Validated / Expired / Disabled）。
- **Callout 层**：通过弱符号默认实现，供集成商覆盖。

### 3.3 文件结构

```
src/bsw/services/ecum/
├── include/
│   ├── EcuM.h
│   └── EcuM_Cfg.h
└── src/
    └── EcuM.c
```

---

## 4. 状态机

### 4.1 主状态

```
OFF -- EcuM_Init() --> STARTUP -- StartupTwo完成 --> RUN
RUN  -- 所有RUN请求释放 --> POST_RUN
POST_RUN -- 关闭目标=SLEEP --> SLEEP(GO_SLEEP/SLEEP/WAKE_SLEEP)
POST_RUN -- 关闭目标=OFF/RESET --> SHUTDOWN(GO_OFF_ONE/GO_OFF_TWO/RESET)
SLEEP -- 唤醒事件 --> WAKE_SLEEP -- WakeupRestart --> RUN
```

### 4.2 子状态

| 阶段 | 子状态 | 说明 |
|------|--------|------|
| 启动 | `ECUM_SUBSTATE_STARTUP_ONE` | 前 OS 初始化 |
| 启动 | `ECUM_SUBSTATE_STARTUP_TWO` | 后 OS 初始化 |
| 启动 | `ECUM_SUBSTATE_STARTUP_THREE` | SW-C 初始化 |
| 运行 | `ECUM_SUBSTATE_RUN` | 正常运行 |
| 运行 | `ECUM_SUBSTATE_POST_RUN` | 后运行过渡 |
| 睡眠 | `ECUM_SUBSTATE_GO_SLEEP` | 准备进入睡眠 |
| 睡眠 | `ECUM_SUBSTATE_SLEEP` | 睡眠中 |
| 睡眠 | `ECUM_SUBSTATE_WAKEUP_ONE` | 唤醒后第一阶段 |
| 睡眠 | `ECUM_SUBSTATE_WAKEUP_TWO` | 唤醒后第二阶段 |
| 关闭 | `ECUM_SUBSTATE_GO_OFF_ONE` | 写 NV、反初始化 |
| 关闭 | `ECUM_SUBSTATE_GO_OFF_TWO` | 关 OS、复位/断电 |
| 关闭 | `ECUM_SUBSTATE_RESET` | 复位状态 |
| 特殊 | `ECUM_SUBSTATE_HALT` | CPU 停机 |
| 特殊 | `ECUM_SUBSTATE_POLL` | 主动轮询等待唤醒 |

---

## 5. 核心数据结构

### 5.1 状态与目标类型

```c
typedef uint8 EcuM_StateType;      /* OFF / STARTUP / RUN / POST_RUN / SLEEP / SHUTDOWN */
typedef uint8 EcuM_SubStateType;   /* 启动/运行/睡眠/关闭子状态 */
typedef uint8 EcuM_ShutdownTargetType; /* OFF / RESET / SLEEP */
typedef uint8 EcuM_ShutdownCauseType;  /* ECU_STATE / WATCHDOG / HARDWARE / SOFTWARE / ... */
typedef uint8 EcuM_BootTargetType;     /* OEM_BOOTLOADER / SYS_BOOTLOADER / APPLICATION */
typedef uint32 EcuM_WakeupSourceType;  /* 位掩码，最多 32 个唤醒源 */
typedef uint8 EcuM_WakeupStatusType;   /* NONE / PENDING / VALIDATED / EXPIRED / DISABLED */
```

### 5.2 配置结构

```c
typedef struct {
    EcuM_WakeupSourceType WakeupSource;
    uint32 ValidationTimeout;
    boolean CheckWakeupTimeEnabled;
    uint32 CheckWakeupTime;
} EcuM_WakeupSourceConfigType;

typedef struct {
    const EcuM_WakeupSourceConfigType* WakeupSources;
    uint8 NumWakeupSources;
    boolean ComMConfigEnabled;
    boolean NvmConfigEnabled;
    boolean WdgMConfigEnabled;
} EcuM_ConfigType;
```

### 5.3 关键运行时变量

| 变量 | 类型 | 说明 |
|------|------|------|
| `EcuM_CurrentState` | `EcuM_StateType` | 当前主状态 |
| `EcuM_CurrentSubState` | `EcuM_SubStateType` | 当前子状态 |
| `EcuM_RunRequests` | `uint32` | 32 位 RUN 请求掩码 |
| `EcuM_KilledRunRequests` | `uint32` | 被 KillAllRUNRequests 保存的请求 |
| `EcuM_PendingWakeupEvents` | `EcuM_WakeupSourceType` | 待验证唤醒源 |
| `EcuM_ValidatedWakeupEvents` | `EcuM_WakeupSourceType` | 已验证唤醒源 |
| `EcuM_ExpiredWakeupEvents` | `EcuM_WakeupSourceType` | 已过期唤醒源 |
| `EcuM_WakeupStatus[]` | `EcuM_WakeupStatusType[32]` | 每个唤醒源的状态 |
| `EcuM_WakeupValidationTimer[]` | `uint32[32]` | 每个唤醒源的验证倒计时 |
| `EcuM_ShutdownTarget` | `EcuM_ShutdownTargetType` | 当前关闭目标 |
| `EcuM_SleepMode` / `EcuM_ShutdownMode` | `uint8` | 模式参数 |

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | SWS 需求 | 备注 |
|-----|------|------|----------|------|
| `EcuM_Init` | `void EcuM_Init(void)` | 初始化并进入 StartupOne | SWS_EcuM_00001 | 无参数，使用链接配置 |
| `EcuM_StartupOne` | `void EcuM_StartupOne(void)` | 执行前 OS 启动阶段 | SWS_EcuM_00010 | |
| `EcuM_StartupTwo` | `void EcuM_StartupTwo(void)` | 执行后 OS 启动阶段 | SWS_EcuM_00011 | |
| `EcuM_MainFunction` | `void EcuM_MainFunction(void)` | 周期处理 RUN/POST_RUN/SLEEP 状态 | SWS_EcuM_00060 | 10ms 周期 |
| `EcuM_RequestRUN` | `Std_ReturnType EcuM_RequestRUN(EcuM_UserType user)` | 用户请求 RUN | SWS_EcuM_00090 | user < ECUM_MAX_USERS |
| `EcuM_ReleaseRUN` | `Std_ReturnType EcuM_ReleaseRUN(EcuM_UserType user)` | 用户释放 RUN | SWS_EcuM_00091 | |
| `EcuM_KillAllRUNRequests` | `Std_ReturnType EcuM_KillAllRUNRequests(void)` | 强制清除所有 RUN 请求 | SWS_EcuM_00092 | 保存到 KilledRunRequests |
| `EcuM_GetState` | `Std_ReturnType EcuM_GetState(EcuM_StateType* state)` | 获取主状态 | SWS_EcuM_00021 | |
| `EcuM_GetSubState` | `Std_ReturnType EcuM_GetSubState(EcuM_SubStateType* subState)` | 获取子状态 | SWS_EcuM_00022 | |
| `EcuM_Shutdown` | `void EcuM_Shutdown(void)` | 启动关闭序列 | SWS_EcuM_00035 | |
| `EcuM_SelectShutdownTarget` | `Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType target, uint8 mode)` | 选择关闭目标 | SWS_EcuM_00030 | |
| `EcuM_GetShutdownTarget` | `Std_ReturnType EcuM_GetShutdownTarget(...)` | 查询关闭目标 | SWS_EcuM_00031 | |
| `EcuM_GetLastShutdownTarget` | `Std_ReturnType EcuM_GetLastShutdownTarget(...)` | 查询上次关闭目标 | SWS_EcuM_00032 | 当前从内存返回 |
| `EcuM_SelectShutdownCause` | `Std_ReturnType EcuM_SelectShutdownCause(EcuM_ShutdownCauseType cause)` | 选择关闭原因 | SWS_EcuM_00033 | |
| `EcuM_GetShutdownCause` | `Std_ReturnType EcuM_GetShutdownCause(EcuM_ShutdownCauseType* cause)` | 查询关闭原因 | SWS_EcuM_00034 | |
| `EcuM_GoSleep` | `void EcuM_GoSleep(void)` | 进入睡眠序列 | SWS_EcuM_00080 | |
| `EcuM_GoHalt` | `void EcuM_GoHalt(void)` | 进入 Halt 模式 | SWS_EcuM_00081 | |
| `EcuM_GoPoll` | `void EcuM_GoPoll(void)` | 进入 Poll 模式 | SWS_EcuM_00082 | |
| `EcuM_WakeupRestart` | `void EcuM_WakeupRestart(void)` | 唤醒后重启序列 | SWS_EcuM_00083 | |
| `EcuM_SetWakeupEvent` | `void EcuM_SetWakeupEvent(EcuM_WakeupSourceType sources)` | 设置唤醒事件 | SWS_EcuM_00040 | |
| `EcuM_ClearWakeupEvent` | `void EcuM_ClearWakeupEvent(EcuM_WakeupSourceType sources)` | 清除唤醒事件 | SWS_EcuM_00041 | |
| `EcuM_EnableWakeupSources` | `Std_ReturnType EcuM_EnableWakeupSources(...)` | 使能唤醒源 | SWS_EcuM_00043 | |
| `EcuM_DisableWakeupSources` | `Std_ReturnType EcuM_DisableWakeupSources(...)` | 禁用唤醒源 | SWS_EcuM_00044 | |
| `EcuM_GetStatusOfWakeupSource` | `EcuM_WakeupStatusType EcuM_GetStatusOfWakeupSource(...)` | 查询单个唤醒源状态 | SWS_EcuM_00045 | |
| `EcuM_GetWakeupSources` | `Std_ReturnType EcuM_GetWakeupSources(...)` | 获取所有待验证/已验证源 | SWS_EcuM_00046 | |
| `EcuM_CheckValidation` | `Std_ReturnType EcuM_CheckValidation(...)` | 检查唤醒源是否已验证 | SWS_EcuM_00047 | |
| `EcuM_CheckWakeup` | `void EcuM_CheckWakeup(EcuM_WakeupSourceType sources)` | 检查唤醒源 | SWS_EcuM_00042 | |
| `EcuM_SelectBootTarget` | `Std_ReturnType EcuM_SelectBootTarget(EcuM_BootTargetType target)` | 选择启动目标 | SWS_EcuM_00050 | |
| `EcuM_GetBootTarget` | `Std_ReturnType EcuM_GetBootTarget(...)` | 获取启动目标 | SWS_EcuM_00051 | |
| `EcuM_SelectApplicationMode` | `Std_ReturnType EcuM_SelectApplicationMode(...)` | 选择应用模式 | SWS_EcuM_00100 | 初始化前有效 |
| `EcuM_GetApplicationMode` | `Std_ReturnType EcuM_GetApplicationMode(...)` | 获取应用模式 | SWS_EcuM_00101 | |
| `EcuM_ComM_RequestComMode` | `Std_ReturnType EcuM_ComM_RequestComMode(uint8 channel, EcuM_ModeType mode)` | 转发 ComM 请求 | SWS_EcuM_00110 | |
| `EcuM_ComM_ReleaseComMode` | `Std_ReturnType EcuM_ComM_ReleaseComMode(uint8 channel)` | 转发 ComM 释放 | SWS_EcuM_00111 | |
| `EcuM_StartBswMode` | `void EcuM_StartBswMode(EcuM_BswModeType mode)` | 启动 BSW 模式 | SWS_EcuM_00120 | |
| `EcuM_StopBswMode` | `void EcuM_StopBswMode(EcuM_BswModeType mode)` | 停止 BSW 模式 | SWS_EcuM_00121 | |
| `EcuM_GetVersionInfo` | `void EcuM_GetVersionInfo(Std_VersionInfoType* versionInfo)` | 版本信息 | SWS_EcuM_00070 | |

### 6.2 回调 / Callout

| Callout | 说明 |
|---------|------|
| `EcuM_DriverInitOne/Two/Three` | 集成商分阶段初始化驱动 |
| `EcuM_DriverRestart` | 唤醒后重新初始化驱动 |
| `EcuM_AL_*` | 抽象层 callout，包括 SwitchOff、Reset、EnterSleep、WakeupCheck 等 |
| `BswM_EcuM_CurrentState` | 通知 BswM 当前 ECU 状态 |
| `BswM_EcuM_CurrentWakeup` | 通知 BswM 唤醒源状态变化 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | Init | ECUM_E_NOT_INITIALIZED |
| 0x01 | Shutdown | ECUM_E_NOT_INITIALIZED |
| 0x02 | RequestRUN | ECUM_E_NOT_INITIALIZED, ECUM_E_INVALID_PAR |
| 0x03 | ReleaseRUN | ECUM_E_NOT_INITIALIZED, ECUM_E_INVALID_PAR |
| 0x04 | SelectShutdownTarget | ECUM_E_NOT_INITIALIZED, ECUM_E_INVALID_PAR |
| 0x05 | GetState | ECUM_E_NOT_INITIALIZED, ECUM_E_NULL_POINTER |
| 0x06 | ComM_RequestComMode | ECUM_E_NOT_INITIALIZED |
| 0x07 | SetWakeupEvent | ECUM_E_NOT_INITIALIZED |
| 0x0F | StartupOne | ECUM_E_NOT_INITIALIZED, ECUM_E_WRONG_API_ORDER |
| 0x10 | StartupTwo | ECUM_E_NOT_INITIALIZED, ECUM_E_WRONG_API_ORDER |
| 0x11 | Sleep | ECUM_E_NOT_INITIALIZED, ECUM_E_STATE_CHANGE_FAILED |
| 0x1B | MainFunction | ECUM_E_NOT_INITIALIZED |

---

## 7. 处理流程

### 7.1 启动流程

1. `EcuM_Init()` 设置状态为 `STARTUP / STARTUP_ONE`，调用 `EcuM_StartupOne()`。
2. `EcuM_ProcessStartupOne()` 调用 `EcuM_DriverInitOne()`，使能唤醒源，启动 OS，进入 `STARTUP_TWO`。
3. `EcuM_StartupTwo()` 验证当前子状态后调用 `EcuM_ProcessStartupTwo()`。
4. `EcuM_ProcessStartupTwo()` 初始化 SchM、BswM、ComM、NvM，调用 `NvM_ReadAll()`，启动 RTE，调用 `EcuM_DriverInitThree()`，最终进入 `RUN`。

### 7.2 RUN → POST_RUN → 关闭/睡眠

1. `EcuM_MainFunction()` 在 RUN 状态下调用 `EcuM_CheckRunRequests()`。
2. 当 `EcuM_RunRequests == 0` 时，状态切换到 `POST_RUN / POST_RUN`，通知 BswM。
3. 在 `EcuM_ProcessPostRun()` 中根据 `EcuM_ShutdownTarget` 决定：
   - `SLEEP`：进入 `SLEEP / GO_SLEEP`，调用 `EcuM_GoSleep()`。
   - `OFF` / `RESET`：进入 `SHUTDOWN / GO_OFF_ONE`，调用 `EcuM_Shutdown()`。

### 7.3 睡眠与唤醒

1. `EcuM_ProcessGoSleep()` 通知 BswM，停止 RTE，释放 ComM，取消/等待 NvM，反初始化 WdgM。
2. `EcuM_PerformSleep()` 使能唤醒源，调用 `EcuM_AL_EnterSleep()`。
3. 唤醒后执行 `EcuM_WakeupRestart()` → `WAKE_SLEEP / WAKEUP_ONE`。
4. `EcuM_ProcessWakeupOne()` 调用 `EcuM_DriverRestart()` 与 `EcuM_AL_WakeupValidation()`。
5. `EcuM_ProcessWakeupTwo()` 重新初始化 SchM、BswM、ComM、RTE，回到 `RUN`。

### 7.4 唤醒源验证

1. `EcuM_SetWakeupEvent()` 将源加入 `Pending`，状态设为 `PENDING`，启动验证定时器。
2. `EcuM_ValidateWakeupSources()` 每个 MainFunction 周期递减定时器，归零后标记为 `VALIDATED`。
3. `EcuM_ExpireWakeupSources()` 处理未验证且定时器归零的源，标记为 `EXPIRED`。
4. 状态变化通过 `BswM_EcuM_CurrentWakeup()` 通知 BswM。

---

## 8. 配置设计

### 8.1 预编译配置（`EcuM_Cfg.h`）

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `ECUM_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `ECUM_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `ECUM_MAX_USERS` | 32U | 最大 RUN 请求用户数 |
| `ECUM_MAX_WAKEUP_SOURCES` | 32U | 最大唤醒源数量 |
| `ECUM_CONFIGURED_WAKEUP_SOURCES` | 位掩码 | 已配置唤醒源 |
| `ECUM_MAIN_FUNCTION_PERIOD` | 10U | MainFunction 周期（ms） |
| `ECUM_WAKEUP_VALIDATION_TIMEOUT` | 100U | 唤醒验证超时（ms） |
| `ECUM_NVM_READALL_TIMEOUT` | 10000U | ReadAll 超时（ms） |
| `ECUM_NVM_WRITEALL_TIMEOUT` | 10000U | WriteAll 超时（ms） |
| `ECUM_DEFAULT_SLEEP_MODE` | 0U | 默认睡眠模式 |
| `ECUM_DEFAULT_RESET_TYPE` | 0U | 默认复位类型 |
| `ECUM_NVM_ENABLED` | STD_ON | NvM 集成开关 |
| `ECUM_WDGM_ENABLED` | STD_ON | WdgM 集成开关 |
| `ECUM_COMM_ENABLED` | STD_ON | ComM 集成开关 |
| `ECUM_BSWM_ENABLED` | STD_ON | BswM 集成开关 |
| `ECUM_SCHM_ENABLED` | STD_ON | SchM 集成开关 |
| `ECUM_RTE_ENABLED` | STD_ON | RTE 集成开关 |
| `ECUM_POLL_MODE_SUPPORTED` | STD_ON | Poll 模式支持 |
| `ECUM_HALT_MODE_SUPPORTED` | STD_ON | Halt 模式支持 |

### 8.2 链接时配置

`EcuM_Config` 在 `EcuM_Lcfg.c` 中定义（当前代码中通过外部符号引用，未在提供文件内展开）。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x10 | `ECUM_E_NOT_INITIALIZED` | API 在初始化前调用 |
| 0x11 | `ECUM_E_INVALID_PAR` | 参数越界或非法 |
| 0x12 | `ECUM_E_NULL_POINTER` | 空指针入参 |
| 0x13 | `ECUM_E_STATE_CHANGE_FAILED` | 非法状态转换 |
| 0x16 | `ECUM_E_WRONG_API_ORDER` | StartupOne/Two 调用顺序错误 |

### 9.2 DEM 错误

当前实现未直接调用 DEM，依赖 BswM 与集成商处理。

### 9.3 安全机制

- 多阶段启动确保 OS 启动前只初始化必要驱动。
- 关闭序列先写 NV 再关 OS，降低数据丢失风险。
- 唤醒源验证超时机制防止误唤醒。
- 所有状态转换通过 DET 进行参数与顺序校验。

---

## 10. 内存与性能

### 10.1 MemMap 分区

当前实现未显式使用 MemMap 分区（依赖标准 C 静态变量）。后续应补充：

| 分区 | 用途 |
|------|------|
| `ECUM_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化全局变量 |
| `ECUM_START_SEC_VAR_INIT_UNSPECIFIED` | 初始化全局变量 |
| `ECUM_START_SEC_CODE` | 代码段 |
| `ECUM_START_SEC_CONFIG_DATA_UNSPECIFIED` | 链接配置数据 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~500 B | 状态变量、32 唤醒源数组、RUN 请求掩码 |
| ROM | ~20 KB | 代码与默认 callout |
| 堆栈 | 中等 | 启动/关闭序列嵌套较深 |

---

## 11. 集成指南

- **启动入口**：复位后首先调用 `EcuM_Init()`，由启动代码或 C 运行时负责。
- **OS 集成**：`EcuM_ProcessStartupOne()` 中应调用 `StartOs()`，当前代码以注释形式保留。
- **BswM 集成**：确保 `BswM_EcuM_CurrentState` 与 `BswM_EcuM_CurrentWakeup` 已实现。
- **NvM 集成**：启动调用 `NvM_ReadAll()`，关闭调用 `NvM_WriteAll()` 并等待完成。
- **Callout 覆盖**：使用 `__attribute__((weak))` 默认 callout，集成商在链接时提供强符号。
- **多核**：`ECUM_MULTI_CORE_SUPPORT` 当前为 STD_OFF。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `tests/unit/autosar/services/EcuM_Test.c` | 初始化、RUN 请求、状态转换、关闭目标、唤醒源管理 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 正常启动 | StartupOne → StartupTwo → RUN |
| RUN 请求释放 | RUN → POST_RUN → SHUTDOWN |
| 睡眠唤醒 | RUN → SLEEP → WakeupRestart → RUN |
| 复位关闭 | RUN → SHUTDOWN → RESET |
| 唤醒源验证 | Pending → Validated / Expired |

---

## 13. 实现说明 / TODO

- `EcuM_GetLastShutdownTarget()` 当前从内存返回，未真正从 NV 读取。
- 多个底层驱动初始化调用（Mcu_Init、Port_Init 等）当前为注释，需集成商在 callout 中实现。
- `EcuM_PerformShutdown()` 与 `EcuM_PerformReset()` 的硬件操作由 `EcuM_AL_*` callout 覆盖。
- `EcuM_DisableInterrupts()` / `EcuM_EnableInterrupts()` 当前为空实现，需对接 OS/Mcu。
- 多核支持未实现。

---

## 14. 参考资料

1. AUTOSAR_SWS_ECUStateManager.pdf
2. `docs/modules/ECUM.md`
3. `src/bsw/services/ecum/EcuM.h`
4. `src/bsw/services/ecum/EcuM.c`
5. `src/bsw/services/ecum/EcuM_Cfg.h`

## 需求追溯表

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_EcuM | — | ECUM 模块级需求归属 |
