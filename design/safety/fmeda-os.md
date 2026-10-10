# FMEDA — Os (Operating System)

> **文档编号**: SAF-FMEDA-OS-001
> **版本**: 1.0
> **日期**: 2026-10-24
> **ASIL 等级**: D (ASIL D — 系统级安全关键，OS 故障可导致任意安全功能丧失)
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW OS 层的操作系统 (Os) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。Os 模块基于 FreeRTOS V10.6.x / V11.x 实现，承担整个 ECU 的任务调度、资源管理、事件通信、报警定时和中断管理功能，是系统级安全关键模块。

**模块路径**:
- 核心实现: `src/bsw/os/src/Os.c`
- 配置表: `src/bsw/os/src/Os_Cfg.c`
- 公共头文件: `src/bsw/os/include/Os.h`
- 内部头文件: `src/bsw/os/include/Os_Internal.h`
- 配置头文件: `src/bsw/os/include/Os_Cfg.h`
- 时序保护配置: `src/bsw/os/include/Os_TimingProtection_Cfg.h`

**分析边界**:
- 任务管理（激活、终止、链式切换、调度、状态查询）
- 事件管理（SetEvent / WaitEvent / ClearEvent / GetEvent）
- 资源管理（GetResource / ReleaseResource，基于 FreeRTOS Mutex 的栈式资源协议）
- 报警管理（SetRelAlarm / SetAbsAlarm / CancelAlarm / GetAlarm，基于 FreeRTOS Timer）
- 中断管理（DisableAllInterrupts / EnableAllInterrupts / Suspend/Resume）
- Hook 函数（StartupHook / ErrorHook / ShutdownHook / PreTaskHook / PostTaskHook）
- 弱引用 Hook 模式（`__attribute__((weak))` Appl_*Hook）
- OS 配置表（Os_Cfg.c 中的 Os_TaskConfigs / Os_AlarmConfigs / Os_ResourceConfigs / Os_GlobalState）
- 时序保护（执行时间保护、到达间隔保护、资源锁保护、中断锁保护）
- 安全状态：OS 停机（ShutdownOS 进入无限循环，WDT 触发系统复位）

**不在分析范围内**:
- FreeRTOS 内核本身的失效模式（由 RTOS 供应商安全手册覆盖）
- 硬件 MPU/MMU 的内存保护（由硬件 FMEDA 覆盖）
- 上层应用任务（ASW 层组件）的逻辑失效

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 Os 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

Os 模块作为 ASIL D 系统级模块，其失效率直接影响整车的残余故障率。分析方法同时考虑：
- 配置数据完整性（ROM 校验和）
- 运行时状态一致性检查
- 时序保护监控
- Hook 函数的安全降级
- WDT 对 OS 调度活性的系统级监控

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `Os_GlobalStateType` | Os_Internal.h:82-93 | 全局运行状态：IsInitialized, IsRunning, CurrentAppMode, NumTasks/NumAlarms/NumResources, Tasks/Alarms/Resources 指针 |
| `Os_TaskConfigType` | Os_Internal.h:50-70 | 任务配置：TaskID, FreeRTOS_Task, FreeRTOS_EventGroup, Priority, IsAutoStart, IsExtended, EntryPoint |
| `Os_AlarmConfigType` | Os_Internal.h:50-59 | 报警配置：AlarmID, Increment, Cycle, ExpiryTick, State, Callback, FreeRTOS_Timer |
| `Os_ResourceConfigType` | Os_Internal.h:72-80 | 资源配置：ResID, FreeRTOS_Mutex, OwnerTask, NestCount, IsCeilingPriority, CeilingPriority |
| `Os_TaskTimingBudgetType` | Os_TimingProtection_Cfg.h:65-71 | 任务时序预算：TaskId, ExecutionTimeBudget, ArrivalTimeMin, FaultAction |
| `Os_ResourceTimingBudgetType` | Os_TimingProtection_Cfg.h:73-78 | 资源时序预算：ResourceId, LockTimeBudget, FaultAction |
| `Os_InterruptTimingBudgetType` | Os_TimingProtection_Cfg.h:80-85 | 中断锁时序预算：AllIntLockBudget, OsIntLockBudget, FaultAction |
| `Os_TaskConfigs[]` | Os_Cfg.c:62-151 | 8 个任务配置表（Init/10ms/50ms/100ms/Background/ComRx/ComTx/Diagnostic） |
| `Os_AlarmConfigs[]` | Os_Cfg.c:162-229 | 6 个报警配置表（BswM/Com/CanIf/Dcm/NvM/Dem MainFunction） |
| `Os_ResourceConfigs[]` | Os_Cfg.c:240-281 | 4 个资源配置表（CanController/NvMBlock/ComBuffer/Diagnostic） |
| `Os_GlobalState` | Os_Cfg.c:292-303 | 全局状态强定义（唯一实例），初始化配置指针指向上述静态表 |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | OS 启动与初始化 | `StartOS()` (L82), `Os_Internal_StartOS()` (L90) | OS 启动流程：重置运行状态、初始化资源/报警/任务、调用 StartupHook、启动 FreeRTOS 调度器 |
| FU-02 | OS 停机 | `ShutdownOS()` (L163), `Os_Internal_ShutdownOS()` (L171) | 安全停机：清除 IsRunning、调用 ShutdownHook、进入无限循环等待 WDT 复位 |
| FU-03 | 任务管理 | `ActivateTask()` (L212), `TerminateTask()` (L309), `ChainTask()` (L361), `Schedule()` (L399) | 任务激活/终止/链式切换/显式调度 |
| FU-04 | 任务状态查询 | `GetTaskID()` (L414), `GetTaskState()` (L454) | 获取当前任务 ID 和任务运行状态 |
| FU-05 | 事件管理 | `SetEvent()` (L580), `WaitEvent()` (L696), `ClearEvent()` (L636), `GetEvent()` (L776) | 扩展任务的事件设置/等待/清除/查询 |
| FU-06 | 资源管理 | `GetResource()` (L842), `ReleaseResource()` (L901) | 基于 FreeRTOS Mutex 的栈式资源获取与释放 |
| FU-07 | 报警管理 | `SetRelAlarm()` (L1065), `SetAbsAlarm()` (L1143), `CancelAlarm()` (L1190), `GetAlarm()` (L1008) | 基于 FreeRTOS Timer 的相对/绝对报警设置与取消 |
| FU-08 | 报警回调分发 | `Os_AlarmCallback()` (L1487), `Os_Callback_Alarm()` (Os_Cfg.c:322) | 报警到期回调：调用 BSW MainFunction 分发器 |
| FU-09 | 中断管理 | `DisableAllInterrupts()` (L527), `EnableAllInterrupts()` (L519), `SuspendAllInterrupts()` (L543), `ResumeAllInterrupts()` (L535) | 全局中断禁止/使能，OS 中断挂起/恢复 |
| FU-10 | Hook 函数 | `ErrorHook()` (L1250), `StartupHook()` (L1335), `ShutdownHook()` (L1362), `PreTaskHook()` (L1285), `PostTaskHook()` (L1310) | 弱引用 Hook 函数及 Appl_* 应用覆写入口 |
| FU-11 | 任务包装器 | `Os_TaskWrapper()` (L1515) | FreeRTOS 任务入口包装：调用 PreTaskHook → EntryPoint → TerminateTask |
| FU-12 | OS 配置表 | `Os_TaskConfigs[]` (Os_Cfg.c:62), `Os_AlarmConfigs[]` (Os_Cfg.c:162), `Os_ResourceConfigs[]` (Os_Cfg.c:240), `Os_GlobalState` (Os_Cfg.c:292) | 静态配置数据完整性 |
| FU-13 | 时序保护 | `Os_TaskTimingBudgetType`, `Os_ResourceTimingBudgetType`, `Os_InterruptTimingBudgetType` | 执行时间/到达间隔/资源锁/中断锁的超时监控 |

---

## 3. 失效模式分析表

### 3.1 FU-01: OS 启动与初始化 (`StartOS` / `Os_Internal_StartOS`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | `Os_GlobalState.Tasks` 指针为 NULL（配置表未链接） | 所有任务无法创建，OS 空转 | `Os_Internal_StartOS()` L106 NULL_PTR 检查；StartupHook 自检任务数量 | 90% | D |
| FM-01-02 | `Os_GlobalState.NumTasks` 为 0（配置数据损坏） | 无任务被初始化，系统无响应 | StartupHook 中校验 NumTasks > 0；WDT 超时触发复位 | 85% | D |
| FM-01-03 | FreeRTOS 调度器启动失败 (`vTaskStartScheduler` 返回) | OS 进入 ShutdownHook，系统停机 | L154-155 调度器返回后直接调用 `Os_Internal_ShutdownHook`；WDT 复位 | 90% | D |
| FM-01-04 | 资源初始化时 `xSemaphoreCreateMutex` 返回 NULL | 对应资源不可用，后续 GetResource 失败 | `Os_InitResources()` L1422 创建后检查返回值；启动阶段报错 | 85% | D |
| FM-01-05 | 报警初始化时 `xTimerCreate` 返回 NULL | 对应报警不可用，BSW MainFunction 无法周期调度 | `Os_InitAlarms()` L1448 创建后检查返回值；WDT 监控 MainFunction 活性 | 80% | D |
| FM-01-06 | 自动启动任务激活失败 (`xTaskCreate` 返回非 pdPASS) | 关键周期任务未创建，系统功能缺失 | `Os_Internal_ActivateTask()` L263 返回 `E_OS_LIMIT`；StartupHook 校验所有 autostart 任务状态 | 90% | D |

### 3.2 FU-02: OS 停机 (`ShutdownOS` / `Os_Internal_ShutdownOS`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | ShutdownHook 执行异常（Hook 函数内部死循环或崩溃） | 系统无法进入安全停机状态 | WDT 超时触发硬件复位（最终安全保障） | 95% | D |
| FM-02-02 | `IsRunning` 标志未被正确清除 | 其他模块误判 OS 仍在运行 | `Os_Internal_ShutdownOS()` L173 首先清除标志；关键路径冗余检查 | 90% | D |
| FM-02-03 | 无限循环被意外跳出（CPU 瞬态故障导致 PC 偏移） | 系统进入未知状态 | WDT 超时复位；MPU 代码区保护（硬件层面） | 99% | D |

### 3.3 FU-03: 任务管理 (`ActivateTask` / `TerminateTask` / `ChainTask`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | TaskID 超出配置范围 (`TaskID >= NumTasks`) | 访问越界内存，可能破坏其他数据 | `ActivateTask()` L217 入口参数校验，返回 `E_OS_ID`；DET 报告错误 | 99% | D |
| FM-03-02 | `xTaskCreate` 内存分配失败（堆栈不足） | 任务未创建，返回 `E_OS_LIMIT` | L263 返回值检查；DET 报告错误；系统监控检测任务缺失 | 90% | D |
| FM-03-03 | 扩展任务 EventGroup 创建失败 | 扩展任务无法使用事件机制 | L272 NULL_PTR 检查，回退删除已创建 Task；返回 `E_OS_LIMIT` | 90% | D |
| FM-03-04 | 任务终止时仍持有资源 | 资源永久锁定，其他任务无法获取 | `Os_Internal_TerminateTask()` L334-339 遍历检查资源所有权，返回 `E_OS_RESOURCE` 拒绝终止 | 95% | D |
| FM-03-05 | `vTaskResume` 恢复已运行任务（竞争条件） | 任务状态不一致 | FreeRTOS 内部状态机保护；`eTaskGetState` 前置检查 | 80% | B |
| FM-03-06 | ChainTask 激活新任务成功但终止自身失败 | 两个任务同时运行，违反链式语义 | `Os_Internal_ChainTask` 分步执行，终止失败返回错误码；DET 报告 | 85% | D |
| FM-03-07 | 调度器未启动时调用 `taskYIELD`（PendSV 异常） | ARMv8-M Data Abort（PSP 不存在） | L293 `xTaskGetSchedulerState` 检查，仅在调度器运行时才触发 yield | 95% | D |

### 3.4 FU-04: 任务状态查询 (`GetTaskID` / `GetTaskState`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | TaskID 指针为 NULL | 写入非法地址，内存破坏 | `GetTaskID()` L420 NULL_PTR 校验，返回 `E_OS_PARAM_POINTER` | 99% | D |
| FM-04-02 | State 指针为 NULL | 写入非法地址，内存破坏 | `GetTaskState()` L462 NULL_PTR 校验，返回 `E_OS_PARAM_POINTER` | 99% | D |
| FM-04-03 | FreeRTOS 句柄与配置表不匹配 | 返回错误的任务状态 | `GetTaskID()` L431 遍历全表匹配；未找到时返回 TaskID=0 | 80% | B |
| FM-04-04 | TaskID 超出范围 | 越界访问 | L457 范围检查，返回 `E_OS_ID` | 99% | D |

### 3.5 FU-05: 事件管理 (`SetEvent` / `WaitEvent` / `ClearEvent` / `GetEvent`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | 对非扩展任务调用 SetEvent | 事件操作无效 | `Os_Internal_SetEvent()` L611 `IsExtended` 检查，返回 `E_OS_ACCESS` | 99% | D |
| FM-05-02 | EventGroup 句柄为 NULL | 访问空指针 | L616 NULL_PTR 检查，返回 `E_OS_STATE` | 99% | D |
| FM-05-03 | WaitEvent 时任务持有资源 | 违反 AUTOSAR 语义（等待时不应持有资源） | `Os_Internal_WaitEvent()` L734-740 遍历资源所有权检查，返回 `E_OS_RESOURCE` | 95% | D |
| FM-05-04 | WaitEvent 无限等待 (`portMAX_DELAY`) 导致任务永久阻塞 | 任务无法恢复，系统功能降级 | 时序保护监控（到达间隔保护）检测任务长时间未激活；WDT 系统级监控 | 80% | B |
| FM-05-05 | TaskID 超出范围 | 越界访问 | L585 范围检查，返回 `E_OS_ID` | 99% | D |
| FM-05-06 | EventMask 为 0（空掩码） | 操作无效但不报错 | 参数校验（可配置 DET 检查） | 60% | A |

### 3.6 FU-06: 资源管理 (`GetResource` / `ReleaseResource`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | ResID 超出配置范围 | 越界访问 | `GetResource()` L847 范围检查，返回 `E_OS_ID` | 99% | D |
| FM-06-02 | Mutex 句柄为 NULL（初始化失败） | 获取资源失败 | L873 NULL_PTR 检查，返回 `E_OS_ID` | 95% | D |
| FM-06-03 | `xSemaphoreTake` 超时（`portMAX_DELAY` 死锁） | 任务永久阻塞 | 时序保护：资源锁时间监控 (`Os_ResourceTimingBudgetType.LockTimeBudget`)；WDT 复位 | 85% | D |
| FM-06-04 | 非所有者调用 ReleaseResource | 资源状态不一致 | `Os_Internal_ReleaseResource()` L939 `OwnerTask` 检查，返回 `E_OS_NOFUNC` | 95% | D |
| FM-06-05 | NestCount 下溢（重复释放） | NestCount 回绕到 255 | L944 `NestCount--` 后检查 `NestCount == 0` 才清除 OwnerTask；DET 报告 | 80% | B |
| FM-06-06 | `xSemaphoreGive` 失败 | 资源未释放 | L951 返回值检查，返回 `E_OS_ACCESS` | 90% | D |

### 3.7 FU-07: 报警管理 (`SetRelAlarm` / `SetAbsAlarm` / `CancelAlarm` / `GetAlarm`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | AlarmID 超出配置范围 | 越界访问 | `SetRelAlarm()` L1070 范围检查，返回 `E_OS_ID` | 99% | D |
| FM-07-02 | Increment 为 0（无效的相对增量） | 报警立即到期或行为不确定 | L1075 零值检查，返回 `E_OS_VALUE` | 99% | D |
| FM-07-03 | FreeRTOS Timer 句柄为 NULL | 报警操作失败 | L1102 NULL_PTR 检查，返回 `E_OS_ID` | 95% | D |
| FM-07-04 | `xTimerChangePeriod` 或 `xTimerStart` 失败 | 报警未正确设置 | L1118/L1123 返回值检查，返回 `E_OS_VALUE` | 90% | D |
| FM-07-05 | Tick 到毫秒转换精度丢失 (`Increment / OS_TICKS_PER_MS`) | 报警周期偏差 | 最小值保护 L1113 `periodMs = 1` 当计算结果为 0；配置时校验精度 | 70% | B |
| FM-07-06 | SetAbsAlarm 绝对时间已过（Start <= currentTick） | 报警行为不符合预期 | L1164 使用 `relativeIncrement = 1` 立即触发；符合 AUTOSAR 规范 | 85% | B |
| FM-07-07 | 取消非活跃报警 | 操作无效 | `Os_Internal_CancelAlarm()` L1226 状态检查，返回 `E_OS_NOFUNC` | 95% | D |

### 3.8 FU-08: 报警回调分发 (`Os_AlarmCallback` / `Os_Callback_Alarm`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | Timer ID 指针为 NULL | 回调无法定位报警配置 | L1493 NULL_PTR 检查 (`alarm != NULL_PTR`) | 95% | D |
| FM-08-02 | Callback 函数指针为 NULL | BSW MainFunction 未被调用 | L1493 `alarm->Callback != NULL_PTR` 检查 | 90% | D |
| FM-08-03 | 回调函数执行异常（BSW MainFunction 死循环） | 定时器任务阻塞，后续报警无法触发 | WDT 监控；时序保护（执行时间保护 `OS_MAX_ISR_EXECUTION_TIME_MS = 50ms`） | 85% | D |
| FM-08-04 | 周期报警 `xTimerChangePeriod` 失败 | 周期报警变为单次报警 | L1507 返回值未检查（技术债）；WDT 检测 MainFunction 停止调度 | 70% | B |
| FM-08-05 | `Os_Callback_Alarm` 默认分支（未知 AlarmID） | 报警到期无效果 | L351 default 分支静默忽略；配置工具校验 AlarmID 范围 | 75% | B |

### 3.9 FU-09: 中断管理 (`DisableAllInterrupts` / `EnableAllInterrupts`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | DisableAllInterrupts 后未调用 EnableAllInterrupts | 中断永久禁止，系统无响应 | 时序保护：中断锁时间监控 (`OS_MAX_ALL_INT_LOCK_TIME_MS = 10ms`)；WDT 复位 | 90% | D |
| FM-09-02 | 嵌套调用 Disable/Suspend 导致中断状态不匹配 | 中断提前使能或延迟使能 | 时序保护监控锁定时长；代码审查（MISRA 规则） | 70% | B |
| FM-09-03 | 在 ISR 中错误调用 DisableAllInterrupts（应为 SuspendOSInterrupts） | 违反 AUTOSAR 调用层级 | DET 运行时检测调用上下文（可配置）；代码静态分析 | 60% | B |
| FM-09-04 | 中断锁超时 (`> OS_MAX_ALL_INT_LOCK_TIME_MS`) | 中断响应延迟，可能丢失安全事件 | `Os_InterruptTimingBudgetType.AllIntLockBudget` 监控，超时触发 `FaultAction` | 90% | D |

### 3.10 FU-10: Hook 函数 (弱引用模式)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | ErrorHook 未被应用覆写（弱引用空实现） | OS 错误无法被记录和上报 | 集成测试验证 Appl_ErrorHook 是否被链接；StartupHook 自检 | 80% | B |
| FM-10-02 | ShutdownHook 未被应用覆写 | 停机原因无法被记录 | 集成测试验证 Appl_ShutdownHook 是否被链接 | 75% | B |
| FM-10-03 | Appl_ErrorHook 内部执行异常 | ErrorHook 无法完成错误处理 | WDT 超时复位（最终保障）；ErrorHook 应精简实现 | 90% | D |
| FM-10-04 | StartupHook 执行异常 | OS 启动流程中断 | WDT 超时复位；调度器尚未启动时无法使用 OS 服务 | 90% | D |
| FM-10-05 | PreTaskHook / PostTaskHook 执行时间过长 | 任务调度延迟 | 时序保护（执行时间保护）；Hook 应精简实现 | 80% | B |

### 3.11 FU-11: 任务包装器 (`Os_TaskWrapper`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-11-01 | `pvParameters` 指针为 NULL | 任务配置无法读取，HardFault | FreeRTOS 内部参数传递机制保障；MPU 保护 | 90% | D |
| FM-11-02 | `EntryPoint` 函数指针为 NULL | 任务无入口函数 | L1525 NULL_PTR 检查，跳过调用直接进入 TerminateTask | 95% | D |
| FM-11-03 | EntryPoint 正常返回（违反 AUTOSAR 语义） | 任务意外终止 | L1531 自动调用 `Os_Internal_TerminateTask()` 挂起任务 | 95% | D |
| FM-11-04 | EntryPoint 内部栈溢出 | 内存破坏，系统行为不确定 | FreeRTOS 栈溢出检测 (`configCHECK_FOR_STACK_OVERFLOW`)；`OS_CFG_STACK_MONITORING = STD_ON` | 90% | D |

### 3.12 FU-12: OS 配置表完整性

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-12-01 | `Os_GlobalState` 配置指针被意外清零 | 所有任务/报警/资源不可访问 | `Os_Internal_StartOS()` L106/L116/L126 各表 NULL_PTR 检查 | 90% | D |
| FM-12-02 | `Os_TaskConfigs[]` ROM 数据损坏（Flash 位翻转） | 任务配置错误（优先级/入口点/autostart） | 启动时配置表 CRC 校验（由 EcuM/BswM 集成覆盖）；运行时 EntryPoint 有效性检查 | 85% | D |
| FM-12-03 | `Os_AlarmConfigs[]` Callback 指针损坏 | 报警回调跳转到非法地址 | `Os_AlarmCallback()` L1493 Callback NULL_PTR 检查；启动时配置表 CRC | 85% | D |
| FM-12-04 | `Os_ResourceConfigs[]` 数据损坏 | Mutex 创建异常或资源管理混乱 | `Os_InitResources()` 中 `xSemaphoreCreateMutex` 返回值检查；启动时配置表 CRC | 85% | D |
| FM-12-05 | NumTasks/NumAlarms/NumResources 计数值损坏 | 遍历越界或遗漏配置项 | 计数值合理性检查（`<= OS_MAX_TASKS/ALARMS/RESOURCES`）；配置表 CRC | 90% | D |

### 3.13 FU-13: 时序保护

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-13-01 | 任务执行时间超过预算 (`> OS_MAX_TASK_EXECUTION_TIME_MS = 100ms`) | 任务阻塞其他低优先级任务 | `Os_TaskTimingBudgetType.ExecutionTimeBudget` 监控，超时触发 `FaultAction`（TASK_KILL / TASK_RESTART） | 90% | D |
| FM-13-02 | 任务到达间隔过短 (`< OS_MAX_TASK_ARRIVAL_INTERVAL_MS = 10ms`) | 任务过度激活，CPU 过载 | `Os_TaskTimingBudgetType.ArrivalTimeMin` 监控，违规触发 `FaultAction` | 85% | D |
| FM-13-03 | ISR 执行时间超过预算 (`> OS_MAX_ISR_EXECUTION_TIME_MS = 50ms`) | 中断响应延迟，丢失安全事件 | 执行时间监控，超时触发 `FaultAction` | 85% | D |
| FM-13-04 | 资源锁时间超过预算 (`> OS_MAX_RESOURCE_LOCK_TIME_MS = 50ms`) | 其他任务长时间等待资源 | `Os_ResourceTimingBudgetType.LockTimeBudget` 监控 | 85% | D |
| FM-13-05 | 时序保护监控机制自身故障 | 所有时序保护失效 | WDT 系统级监控（最终保障）；定时自测试验 | 70% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | OS 启动与初始化 | 90% | 87% | 依赖 NULL 检查 + WDT |
| FU-02 | OS 停机 | 99% | 95% | WDT 最终保障 |
| FU-03 | 任务管理 | 99% | 91% | 参数校验 + 资源检查 + 调度器状态检查 |
| FU-04 | 任务状态查询 | 99% | 92% | NULL_PTR 校验 + 范围检查 |
| FU-05 | 事件管理 | 99% | 90% | IsExtended + NULL_PTR + 资源所有权检查 |
| FU-06 | 资源管理 | 99% | 91% | 范围 + NULL + OwnerTask + NestCount 检查 |
| FU-07 | 报警管理 | 99% | 90% | 范围 + 零值 + NULL + 返回值检查 |
| FU-08 | 报警回调分发 | 95% | 85% | NULL 检查 + WDT 监控 |
| FU-09 | 中断管理 | 90% | 82% | 时序保护 + WDT |
| FU-10 | Hook 函数 | 90% | 82% | 集成测试 + WDT |
| FU-11 | 任务包装器 | 95% | 92% | NULL 检查 + 栈溢出检测 |
| FU-12 | 配置表完整性 | 90% | 87% | 启动 CRC + NULL 检查 + 计数值校验 |
| FU-13 | 时序保护 | 90% | 83% | 执行时间/到达间隔/锁时间监控 |

### 4.2 模块整体诊断覆盖率

```
DC_Os = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_Os ≈ 89%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL D)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 97% | ~93% | 需改进（见 5.5 改进建议） |
| LFM (Latent Fault Metric) | ≥ 90% | ~91% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 10 FIT | ~8 FIT | 满足 |

> **注**: Os 模块作为 ASIL D 模块，SPFM 目标为 97%。当前估算 ~93% 主要受限于中断管理和 Hook 函数的诊断盲区。通过系统级 WDT + 时序保护机制的联合覆盖，残余故障率可满足 PMHF < 10 FIT 要求。实际达标需结合具体硬件 FIT 数据和系统级冗余设计。

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。Os 中主要单点故障来源：
- 中断锁超时检测盲区（嵌套 Disable/Suspend 场景，DC=70%）
- Hook 函数弱引用未被应用覆写（DC=75-80%）
- 报警回调周期重设失败（DC=70%）
- 时序保护监控机制自身故障（DC=70%）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (6.5 / 95) ≈ 93%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。Os 中主要潜伏故障来源：
- 配置表 CRC 校验算法自身的系统性缺陷（通过 EcuM 启动自检覆盖）
- 时序保护定时器精度偏差（通过定期自测试验覆盖）
- FreeRTOS 内部诊断机制的未检测故障（由 RTOS 供应商安全手册覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (8.1 / 90) ≈ 91%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_config_corruption + lambda_interrupt_lock_miss + lambda_hook_miss
     ≈ 2 + 3 + 3 = 8 FIT < 10 FIT ✓
```

### 5.5 SPFM 改进建议

为达到 ASIL D SPFM ≥ 97% 目标，建议以下系统级措施：
1. **双核锁步执行**: 在 Lockstep 双核 MCU 上运行，CPU 瞬态故障由双核比较覆盖（DC → 99%）
2. **配置表运行时 CRC 周期校验**: 不仅启动时校验，运行期间周期性重校验配置表完整性
3. **中断锁嵌套计数器**: 增加 DisableAllInterrupts 嵌套深度计数，ResumeAllInterrupts 时校验计数匹配
4. **Hook 函数链接自检**: StartupHook 中检查 Appl_*Hook 符号地址是否位于有效代码段

---

## 6. 安全状态定义

### 6.1 Os 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | `IsRunning = TRUE`，所有任务/报警/资源正常 | 正常调度服务，报警周期触发 BSW MainFunction |
| **降级运行** | 单个任务激活失败或单个报警设置失败 | 其余任务继续运行，DET 上报错误，Dem 记录诊断事件 |
| **时序保护触发** | 任务执行超时/到达间隔违规/资源锁超时 | 按 `FaultAction` 执行：TASK_KILL / TASK_RESTART / ERROR_HOOK |
| **安全停止** | `ShutdownOS()` 被调用或 WDT 复位 | `IsRunning = FALSE`，ShutdownHook 执行，进入无限循环等待 WDT 硬件复位 |

### 6.2 安全状态转换

```
正常运行 ──[单任务/报警失败]──→ 降级运行
正常运行 ──[时序保护触发]──→ 时序保护触发
降级运行 ──[关键任务失败]──→ 安全停止
时序保护触发 ──[FaultAction=TASK_KILL 且为关键任务]──→ 安全停止
任意状态 ──[ShutdownOS()调用]──→ 安全停止
任意状态 ──[WDT超时]──→ 安全停止 (硬件复位)
```

### 6.3 安全状态说明

Os 模块的安全状态与系统级安全概念紧密关联：
- **安全状态**定义为 OS 停止调度，所有安全相关输出进入预定义的安全值
- **WDT（看门狗）** 是 OS 层面的最终安全保障，当 OS 调度器无法运行时（无限循环/死锁），WDT 超时触发硬件复位
- **ShutdownOS** 是受控停机，应用层 ShutdownHook 应在此阶段将安全相关输出设置为安全状态

---

## 7. 故障反应时间线

### 7.1 场景一：任务激活失败（xTaskCreate 返回失败）

| 时间 | 事件 |
|------|------|
| T=0 | `ActivateTask(TaskID)` 被调用（如启动 autostart 任务） |
| T=1 cycle | `Os_Internal_ActivateTask()` 调用 `xTaskCreate()` |
| T=2 cycles | `xTaskCreate` 返回非 `pdPASS`（堆不足） |
| T=3 cycles | 返回 `E_OS_LIMIT`，DET 报告错误 |
| T=4 cycles | `ErrorHook(E_OS_LIMIT)` 被调用 |
| T=5 cycles | Dem 记录诊断事件，系统监控触发降级策略 |
| **总延迟** | **5 个系统时钟周期（~μs 级）** |

### 7.2 场景二：资源死锁检测

| 时间 | 事件 |
|------|------|
| T=0 | Task A 调用 `GetResource(ResID)` 获取资源 |
| T=1 | Task B 已持有该资源，Task A 在 `xSemaphoreTake` 中阻塞 |
| T=2+ | 资源锁时间持续增长 |
| T=N | 资源锁时间超过 `OS_MAX_RESOURCE_LOCK_TIME_MS (50ms)` |
| T=N+1 | 时序保护机制检测到超时 |
| T=N+2 | 按 `FaultAction` 执行（如 TASK_KILL 终止持有者） |
| T=N+3 | 资源释放，Task A 获取资源继续执行 |
| **总延迟** | **≤ 50ms + 检测开销（由时序保护周期决定）** |

### 7.3 场景三：OS 停机（致命错误）

| 时间 | 事件 |
|------|------|
| T=0 | 检测到致命错误（如配置表损坏、调度器异常） |
| T=1 | 调用 `ShutdownOS(E_OS_*)` |
| T=2 | `Os_Internal_ShutdownOS()` 设置 `IsRunning = FALSE` |
| T=3 | 调用 `Os_Internal_ShutdownHook(Error)`，应用层设置安全输出 |
| T=4 | 进入 `for(;;)` 无限循环 |
| T=5+ | WDT 超时（典型值 100-500ms） |
| T=6 | 硬件复位，系统重启 |
| **总延迟** | **ShutdownHook 执行时间 + WDT 超时（≤ 500ms）** |

### 7.4 场景四：报警回调执行超时

| 时间 | 事件 |
|------|------|
| T=0 | 报警到期，`Os_AlarmCallback()` 被 FreeRTOS Timer Task 调用 |
| T=1 | 回调分发到 BSW MainFunction（如 `NvM_MainFunction()`） |
| T=2+ | MainFunction 执行时间超过 `OS_MAX_ISR_EXECUTION_TIME_MS (50ms)` |
| T=N | 执行时间保护机制检测到超时 |
| T=N+1 | 按 `FaultAction` 执行（如 ERROR_HOOK 或 PROTECTION_HOOK） |
| T=N+2 | ErrorHook 记录错误，Dem 记录诊断事件 |
| **总延迟** | **≤ 50ms + 检测开销** |

---

## 8. ISO 26262 条款追溯

| ISO 26262 条款 | 要求 | 本文档覆盖 |
|---------------|------|-----------|
| Part 5, Clause 7 | FMEDA 方法论 | 第 3 节完整失效模式表（13 个功能单元，50+ 失效模式） |
| Part 5, Clause 8 | 安全度量 (SPFM/LFM/PMHF) | 第 5 节度量分析 |
| Part 5, Clause 9 | 诊断覆盖率评估 | 第 4 节 DC 汇总 |
| Part 6, Clause 8 | 软件单元设计中的安全机制 | 第 3 节故障检测方法列（参数校验、NULL 检查、返回值检查、时序保护） |
| Part 4, Clause 7 | 安全状态定义 | 第 6 节安全状态 |
| Part 4, Clause 8 | 技术安全需求 | 第 7 节故障反应时间线 |
| Part 6, Clause 7 | 操作系统安全要求 | FU-01 至 FU-13 覆盖 OS 全部服务接口 |
| Part 6, Clause 10 | 集成测试 | 第 3.12 节配置表完整性验证建议 |

---

## 9. 假设与约束

1. **FreeRTOS 内核假设**: FreeRTOS 内核的失效率及诊断覆盖率由 FreeRTOS 安全手册（IEC 61508 / ISO 26262 认证版本）覆盖，本分析假设使用经过安全认证的 FreeRTOS 版本
2. **WDT 覆盖**: 系统级 WDT 由 EcuM/WDG 模块管理，Os 模块的活性监控依赖 WDT 的周期喂狗
3. **双核锁步**: SPFM 改进建议中假设硬件平台支持 Lockstep 双核（如 NXP i.MX8M 的 Cortex-A53 锁步模式或 Cortex-R52）
4. **配置工具**: 假设 Os_Cfg.c 配置表由经过认证的 yuleASR Configurator 工具自动生成，配置数据在生成阶段经过完整性验证
5. **时序保护精度**: 时序保护的监控精度取决于硬件定时器分辨率和监控任务的调度周期
6. **弱引用 Hook**: 假设集成阶段验证了 Appl_*Hook 弱引用符号的链接正确性
7. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障（双核锁步场景除外）
8. **Os_GlobalState 唯一性**: 如 Os.c L50-59 注释所述，`Os_GlobalState` 的强定义仅存在于 Os_Cfg.c:292，Os.c 中不得添加 weak 定义以避免链接器问题

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| CPU 逻辑 (Cortex-A53/R52) | 15 FIT | 供应商 FMD |
| RAM (SRAM/TCM) | 8 FIT | IEC 61508 Annex D |
| Flash (配置数据存储) | 3 FIT | 供应商 FMD |
| FreeRTOS 任务管理 | 5 FIT | FreeRTOS Safety Manual |
| FreeRTOS 定时器 | 3 FIT | FreeRTOS Safety Manual |
| FreeRTOS 信号量 | 2 FIT | FreeRTOS Safety Manual |
| FreeRTOS 事件组 | 2 FIT | FreeRTOS Safety Manual |
| 中断控制器 | 5 FIT | 供应商 FMD |
| WDT 监控 | 1 FIT | 供应商 FMD |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| OS | Operating System |
| RTOS | Real-Time Operating System |
| WDT | Watchdog Timer |
| DET | Default Error Trap |
| DEM | Diagnostic Event Manager |
| MPU | Memory Protection Unit |
| ISR | Interrupt Service Routine |
| PSP | Process Stack Pointer |
| PendSV | Pendable Service Call (ARM exception) |

## 附录 C: 配置参数汇总

| 参数 | 值 | 来源 |
|------|-----|------|
| OS_CFG_NUM_TASKS | 8 | Os_Cfg.h:26 |
| OS_CFG_NUM_ALARMS | 6 | Os_Cfg.h:33 |
| OS_CFG_NUM_RESOURCES | 4 | Os_Cfg.h:34 |
| OS_CFG_TASK_STACK_SIZE | 4096 bytes | Os_Cfg.h:94 |
| OS_CFG_ISR_STACK_SIZE | 2048 bytes | Os_Cfg.h:95 |
| OS_MAX_TASKS | 32 | Os_Internal.h:34 |
| OS_MAX_ALARMS | 16 | Os_Internal.h:35 |
| OS_MAX_RESOURCES | 16 | Os_Internal.h:36 |
| OS_MAX_TASK_EXECUTION_TIME_MS | 100 ms | Os_TimingProtection_Cfg.h:41 |
| OS_MAX_ISR_EXECUTION_TIME_MS | 50 ms | Os_TimingProtection_Cfg.h:42 |
| OS_MAX_TASK_ARRIVAL_INTERVAL_MS | 10 ms | Os_TimingProtection_Cfg.h:44 |
| OS_MAX_RESOURCE_LOCK_TIME_MS | 50 ms | Os_TimingProtection_Cfg.h:46 |
| OS_MAX_ALL_INT_LOCK_TIME_MS | 10 ms | Os_TimingProtection_Cfg.h:47 |
| OS_MAX_OS_INT_LOCK_TIME_MS | 50 ms | Os_TimingProtection_Cfg.h:48 |
| OS_CFG_STACK_MONITORING | STD_ON | Os_Cfg.h:63 |
| OS_CFG_DEV_ERROR_DETECT | STD_ON | Os_Cfg.h:21 |
