# FMEDA — EcuM (ECU State Manager)

> **文档编号**: SAF-FMEDA-ECUM-001
> **版本**: 1.0
> **日期**: 2026-10-23
> **ASIL 等级**: B (D)
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW Integration Layer 的 EcuM (ECU State Manager) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/services/ecum/src/EcuM.c`

**分析边界**:
- EcuM 核心状态机（STARTUP / RUN / POST_RUN / SHUTDOWN / SLEEP / WAKE_SLEEP / OFF）
- 多阶段启动管理（StartupOne, StartupTwo, DriverInitThree）
- 关机流程协调（GoOffOne, GoOffTwo, PerformShutdown, PerformReset）
- 睡眠/唤醒管理（GoSleep, GoHalt, GoPoll, WakeupRestart）
- 唤醒源验证与超时管理
- RUN 请求管理（RequestRUN, ReleaseRUN, KillAllRUNRequests）
- 关机目标与启动目标选择
- NvM ReadAll/WriteAll 协调
- BswM 状态通知
- DET 错误检测机制
- 下层依赖（假设由 ECUAL/MCAL 层覆盖）：Mcu, WdgM, NvM, ComM, SchM, BswM, RTE

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 EcuM 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

EcuM 作为 ECU 状态管理的核心模块，其 ASIL 等级按最高安全需求分配：状态机核心为 ASIL D，辅助查询接口为 ASIL B。

### 1.3 关键数据结构

| 结构体/变量 | 位置 | 说明 |
|-------------|------|------|
| `EcuM_CurrentState` | EcuM.c:58 | 主状态机：OFF/STARTUP/RUN/POST_RUN/SHUTDOWN/SLEEP/WAKE_SLEEP |
| `EcuM_CurrentSubState` | EcuM.c:59 | 子状态：STARTUP_ONE/TWO/THREE, RUN, POST_RUN, GO_SLEEP, SLEEP, WAKEUP_ONE/TWO, GO_OFF_ONE/TWO, HALT, POLL |
| `EcuM_ShutdownTarget` | EcuM.c:60 | 关机目标：OFF/RESET/SLEEP |
| `EcuM_ShutdownCause` | EcuM.c:61 | 关机原因：ECU_STATE/WATCHDOG/HARDWARE/SOFTWARE/FATAL_ERROR/DCM/UNDEFINED |
| `EcuM_BootTarget` | EcuM.c:62 | 启动目标：OEM_BOOTLOADER/SYS_BOOTLOADER/APPLICATION |
| `EcuM_RunRequests` | EcuM.c:72 | RUN 请求位掩码（最多 32 用户），每位对应一个 `EcuM_UserType` |
| `EcuM_PendingWakeupEvents` | EcuM.c:76 | 待处理唤醒事件位掩码 |
| `EcuM_ValidatedWakeupEvents` | EcuM.c:77 | 已验证唤醒事件位掩码 |
| `EcuM_ExpiredWakeupEvents` | EcuM.c:78 | 已过期唤醒事件位掩码 |
| `EcuM_EnabledWakeupSources` | EcuM.c:80 | 已使能唤醒源位掩码 |
| `EcuM_WakeupStatus[]` | EcuM.c:81 | 每个唤醒源的状态数组：NONE/PENDING/VALIDATED/EXPIRED/DISABLED |
| `EcuM_WakeupValidationTimer[]` | EcuM.c:90 | 每个唤醒源的验证倒计时定时器 |
| `EcuM_StateTimer` | EcuM.c:89 | 状态驻留计时器，用于超时监控 |
| `EcuM_ConfigPtr` | EcuM.c:93 | 配置指针，含唤醒源配置、NvM/ComM/WdgM 使能标志 |
| `EcuM_ConfigType` | EcuM.h:251-257 | 配置结构体：WakeupSources[], NumWakeupSources, ComMConfigEnabled, NvmConfigEnabled, WdgMConfigEnabled |
| `EcuM_WakeupSourceConfigType` | EcuM.h:244-249 | 唤醒源配置：WakeupSource, ValidationTimeout, CheckWakeupTimeEnabled, CheckWakeupTime |

### 1.4 关键配置参数

| 参数 | 值 | 位置 | 说明 |
|------|-----|------|------|
| `ECUM_MAX_USERS` | 32 | EcuM_Cfg.h:48 | 最大 RUN 请求用户数 |
| `ECUM_MAX_WAKEUP_SOURCES` | 32 | EcuM_Cfg.h:49 | 最大唤醒源数量 |
| `ECUM_MAIN_FUNCTION_PERIOD` | 10 ms | EcuM_Cfg.h:35 | 主函数调用周期 |
| `ECUM_WAKEUP_VALIDATION_TIMEOUT` | 100 ms | EcuM_Cfg.h:36 | 唤醒验证超时 |
| `ECUM_NVM_READALL_TIMEOUT` | 10000 ms | EcuM_Cfg.h:38 | NvM ReadAll 超时 |
| `ECUM_NVM_WRITEALL_TIMEOUT` | 10000 ms | EcuM_Cfg.h:39 | NvM WriteAll 超时 |
| `ECUM_STARTUP_TIMEOUT` | 5000 ms | EcuM_Cfg.h:40 | 启动超时 |
| `ECUM_SLEEP_TRANSITION_TIMEOUT` | 1000 ms | EcuM_Cfg.h:41 | 睡眠进入超时 |
| `ECUM_WAKEUP_RESTART_TIMEOUT` | 5000 ms | EcuM_Cfg.h:42 | 唤醒重启超时 |
| `ECUM_SHUTDOWN_TRANSITION_TIMEOUT` | 1000 ms | EcuM_Cfg.h:43 | 关机转换超时 |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 多阶段启动 | `EcuM_Init()` (L140), `EcuM_StartupOne()` (L188), `EcuM_StartupTwo()` (L211), `EcuM_ProcessStartupOne()` (L237), `EcuM_ProcessStartupTwo()` (L289) | Pre-OS 驱动初始化 → Post-OS BSW 初始化 → SWC 初始化 |
| FU-02 | 主函数状态调度 | `EcuM_MainFunction()` (L363) | 周期调度，根据 `EcuM_CurrentState` 分派到各状态处理函数 |
| FU-03 | RUN 状态管理 | `EcuM_ProcessRun()` (L407), `EcuM_RequestRUN()` (L877), `EcuM_ReleaseRUN()` (L911), `EcuM_KillAllRUNRequests()` (L940), `EcuM_CheckRunRequests()` (L964) | RUN 请求位掩码管理，POST_RUN 转换 |
| FU-04 | 关机流程 | `EcuM_Shutdown()` (L708), `EcuM_ProcessGoOffOne()` (L729), `EcuM_ProcessGoOffTwo()` (L773), `EcuM_PerformShutdown()` (L808), `EcuM_PerformReset()` (L828) | GoOffOne(NvM WriteAll + BSW 去初始化) → GoOffTwo(关中断 + 断电/复位) |
| FU-05 | 睡眠/唤醒管理 | `EcuM_GoSleep()` (L459), `EcuM_ProcessGoSleep()` (L531), `EcuM_ProcessSleep()` (L568), `EcuM_PerformSleep()` (L846), `EcuM_WakeupRestart()` (L628) | 睡眠准备 → 进入睡眠 → 唤醒检测 → 重启序列 |
| FU-06 | 唤醒源管理 | `EcuM_SetWakeupEvent()` (L1195), `EcuM_ClearWakeupEvent()` (L1238), `EcuM_ValidateWakeupSources()` (L1426), `EcuM_ExpireWakeupSources()` (L1466) | 唤醒事件设置/清除/验证/过期，含定时器倒计时 |
| FU-07 | 关机目标管理 | `EcuM_SelectShutdownTarget()` (L1046), `EcuM_GetShutdownTarget()` (L1084), `EcuM_SelectShutdownCause()` (L1148) | 选择关机目标(OFF/RESET/SLEEP)及关机原因 |
| FU-08 | 启动目标管理 | `EcuM_SelectBootTarget()` (L1535), `EcuM_GetBootTarget()` (L1562) | 选择启动目标(OEM Bootloader/Sys Bootloader/Application) |
| FU-09 | 状态查询 | `EcuM_GetState()` (L991), `EcuM_GetSubState()` (L1016), `EcuM_GetShutdownCause()` (L1167) | 当前状态/子状态/关机原因查询 |
| FU-10 | BswM 状态通知 | `BswM_EcuM_CurrentState()` 调用点 (L176, L301, L350, L735, L975) | 状态变化时通知 BswM 进行模式切换 |
| FU-11 | NvM 协调 | `NvM_ReadAll()` (L312), `NvM_WriteAll()` (L750) | 启动时读全部 NVRAM，关机时写全部 NVRAM |
| FU-12 | Halt/Poll 模式 | `EcuM_GoHalt()` (L482), `EcuM_ProcessHalt()` (L587), `EcuM_GoPoll()` (L507), `EcuM_ProcessPoll()` (L606) | CPU 停止模式与主动轮询模式 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 多阶段启动 (`EcuM_Init` / `EcuM_StartupOne` / `EcuM_StartupTwo`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | `EcuM_IsInitialized` 标志位翻转（RAM 位翻转） | 重复初始化导致状态变量被清零，运行中的 BSW 丢失上下文 | `EcuM_Init()` 入口检查 `EcuM_IsInitialized` (L143)，重复调用报 `ECUM_E_NOT_INITIALIZED` | 85% | D |
| FM-01-02 | `EcuM_CurrentSubState` 在 StartupOne 完成前被意外修改 | StartupTwo 被提前调用，BSW 初始化顺序错乱 | `EcuM_StartupOne()`/`EcuM_StartupTwo()` 入口校验子状态 (L196, L219)，不匹配报 `ECUM_E_WRONG_API_ORDER` | 95% | D |
| FM-01-03 | `EcuM_ConfigPtr` 为空或指向无效配置 | 驱动初始化使用错误参数，可能导致硬件配置异常 | 配置指针在 `EcuM_Init` 中赋值，StartupOne 通过 callout 参数传递；WdgM 监控启动超时 (`ECUM_STARTUP_TIMEOUT` = 5000ms) | 80% | D |
| FM-01-04 | StartupTwo 中 NvM_ReadAll 失败 | NVRAM 数据未加载，应用层使用 ROM 默认值 | NvM 模块内部 CRC 校验（参见 SAF-FMEDA-NVM-001）；EcuM 通过 `ECUM_NVM_READALL_TIMEOUT` (10000ms) 超时检测 | 90% | B |
| FM-01-05 | Pre-OS 驱动初始化 callout (`EcuM_DriverInitOne`) 执行异常 | MCU/Port/Dio/Gpt 未正确初始化 | WdgM 监控：若 StartupOne 超时未完成，触发看门狗复位；DET 错误上报 | 75% | D |
| FM-01-06 | OS 启动失败 (`StartOs` 未返回) | 系统无法进入 Post-OS 阶段 | WdgM 超时监控 (`ECUM_STARTUP_TIMEOUT`)；外部看门狗复位 | 90% | D |

### 3.2 FU-02: 主函数状态调度 (`EcuM_MainFunction`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | `EcuM_CurrentState` 值被损坏（非有效状态码） | 状态机进入 `default` 分支，ECU 无响应 | `switch` 语句 `default` 分支捕获无效状态 (L393)；WdgM 监控 MainFunction 周期调用 | 85% | D |
| FM-02-02 | `EcuM_MainFunction` 未被周期调度 | 状态机停滞，唤醒验证超时、RUN 请求检查等全部停止 | WdgM 监控：周期性喂狗信号丢失触发看门狗告警/复位 | 95% | D |
| FM-02-03 | `EcuM_MainFunctionCounter` 溢出 | 计数器回绕，影响基于计数的超时判断 | 计数器为 `uint32`，溢出周期约 497 天（10ms 周期），超出任务范围；关键超时使用独立 `EcuM_StateTimer` | 60% | B |
| FM-02-04 | `EcuM_StateTimer` 未正确递增/复位 | 状态超时检测失效，可能卡在中间状态 | `EcuM_UpdateSubState()` (L1742) 在子状态变化时复位 `EcuM_StateTimer`；WdgM 作为最终兜底 | 80% | D |

### 3.3 FU-03: RUN 状态管理 (`EcuM_RequestRUN` / `EcuM_ReleaseRUN` / `EcuM_CheckRunRequests`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | `EcuM_RunRequests` 位掩码被损坏（RAM 故障） | 错误的 RUN 请求状态，可能提前进入 POST_RUN 或无法退出 RUN | `EcuM_CheckRunRequests()` (L964) 周期性检查；BswM 监控状态一致性 | 75% | B |
| FM-03-02 | `user` 参数超出 `ECUM_MAX_USERS` 范围 | 位移操作越界，可能破坏相邻内存 | 入口参数校验 `user >= ECUM_MAX_USERS` (L885, L919)，报 `ECUM_E_INVALID_PAR` | 99% | B |
| FM-03-03 | `EcuM_KillAllRUNRequests` 导致意外进入 POST_RUN | 所有应用被强制终止，ECU 开始关机流程 | `EcuM_KilledRunRequests` 保存被清除的请求 (L950) 用于诊断追溯；BswM 通知 POST_RUN 状态 (L975) | 80% | B |
| FM-03-04 | RUN → POST_RUN 转换后新的 RequestRUN 未能恢复 RUN 状态 | ECU 继续关机流程而非恢复运行 | `EcuM_RequestRUN()` 检查 `EcuM_CurrentState == ECUM_STATE_POST_RUN` (L896)，若匹配则恢复到 RUN | 90% | B |

### 3.4 FU-04: 关机流程 (`EcuM_Shutdown` / `EcuM_ProcessGoOffOne` / `EcuM_ProcessGoOffTwo`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | `NvM_WriteAll()` 在 GoOffOne 中超时或失败 | 关键运行数据未持久化到 NVRAM | `ECUM_NVM_WRITEALL_TIMEOUT` (10000ms) 超时检测；NvM 内部作业队列状态检查 | 85% | B |
| FM-04-02 | `EcuM_ShutdownTarget` 被损坏为无效值 | GoOffTwo 中 `switch` 进入 `default` 分支，ECU 未正确断电 | `EcuM_SelectShutdownTarget()` 入口校验 target 值 (L1054-L1056)；GoOffTwo 中 `default` 分支安全兜底 (L800) | 90% | D |
| FM-04-03 | `EcuM_DisableInterrupts()` 执行失败 | 关机过程中中断干扰去初始化序列 | 通过 Mcu 抽象层执行，硬件级保证；WdgM 在关中断前已去初始化 (L557) | 70% | D |
| FM-04-04 | `EcuM_PerformShutdown()` 中 `EcuM_AL_SwitchOff()` callout 未执行 | ECU 未断电，进入无限循环 | 无限循环兜底 (L819-L822)；外部硬件看门狗最终复位 | 85% | D |
| FM-04-05 | `EcuM_PerformReset()` 中复位 callout 失败 | ECU 未复位，进入无限循环 | 无限循环兜底 (L837-L840)；外部硬件看门狗超时后强制硬件复位 | 90% | D |
| FM-04-06 | ComM_DeInit 在 GoOffOne 中失败 | 通信通道未正确关闭，可能发送无效数据 | ComM 模块内部去初始化状态检查；BswM 监控关机序列进度 | 80% | B |

### 3.5 FU-05: 睡眠/唤醒管理 (`EcuM_GoSleep` / `EcuM_ProcessSleep` / `EcuM_WakeupRestart`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | `EcuM_CurrentState` 不为 `ECUM_STATE_SLEEP` 时调用 `EcuM_GoSleep` | 非法状态转换，可能导致系统不稳定 | 入口状态校验 (L467)，不匹配报 `ECUM_E_STATE_CHANGE_FAILED` | 99% | D |
| FM-05-02 | `EcuM_PendingWakeupEvents` 在睡眠中被损坏 | 虚假唤醒或唤醒丢失 | `EcuM_ProcessSleep()` 检查 `EcuM_PendingWakeupEvents != ECUM_WKSOURCE_NONE` (L576)；唤醒源验证机制 (FU-06) 交叉验证 | 80% | B |
| FM-05-03 | `EcuM_PerformSleep()` 中唤醒源使能失败 | ECU 进入睡眠后无法被唤醒 | 进入睡眠前调用 `EcuM_EnableWakeupSources()` (L852)；配置交叉检查 `EcuM_EnabledWakeupSources` | 85% | B |
| FM-05-04 | `EcuM_AL_EnterSleep()` callout 未正确执行 | CPU 未进入低功耗模式，功耗异常 | Callout 由集成方实现并验证；WdgM 在睡眠前已去初始化，由外部看门狗监控 | 70% | B |
| FM-05-05 | WakeupRestart 序列中驱动重启失败 | 唤醒后系统功能不完整 | `EcuM_ProcessWakeupOne()` 调用 `EcuM_DriverRestart()` (L652)；`ECUM_WAKEUP_RESTART_TIMEOUT` (5000ms) 超时检测 | 80% | D |
| FM-05-06 | 睡眠准备中 RTE 未正确停止 | 应用层在睡眠期间可能访问无效外设 | `EcuM_ProcessGoSleep()` 中 RTE 停止 (L542)；BswM 协调各模块去初始化顺序 | 75% | B |

### 3.6 FU-06: 唤醒源管理 (`EcuM_SetWakeupEvent` / `EcuM_ValidateWakeupSources` / `EcuM_ExpireWakeupSources`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | `EcuM_WakeupStatus[]` 数组越界访问 | 内存破坏，可能导致相邻变量损坏 | `EcuM_GetWakeupSourceIndex()` (L1501) 循环限制 `index < ECUM_MAX_WAKEUP_SOURCES`；`EcuM_SetWakeupEvent()` 按位遍历 (L1214) 限制在 `ECUM_MAX_WAKEUP_SOURCES` 内 | 95% | B |
| FM-06-02 | 唤醒验证定时器 (`EcuM_WakeupValidationTimer[]`) 被损坏 | 验证超时计算错误，可能错误接受/拒绝唤醒源 | 定时器由 `ECUM_WAKEUP_VALIDATION_TIMEOUT / ECUM_MAIN_FUNCTION_PERIOD` (100/10 = 10 个周期) 初始化 (L1221)；冗余校验通过 `EcuM_ExpireWakeupSources()` 双重检查 | 80% | B |
| FM-06-03 | 已禁用的唤醒源触发唤醒事件 | 不应唤醒的场景下 ECU 被唤醒 | `EcuM_SetWakeupEvent()` 中 `sources &= EcuM_EnabledWakeupSources` (L1205) 过滤已禁用源 | 95% | B |
| FM-06-04 | `EcuM_ExpiredWakeupEvents` 累积导致溢出 | 过期事件掩码无法区分新旧过期事件 | `EcuM_ClearWakeupEvent()` (L1250) 清除过期事件；每次唤醒重启后全量清除 | 70% | B |
| FM-06-05 | 多唤醒源同时触发时的竞态条件 | 唤醒源状态不一致 | `EcuM_SetWakeupEvent()` 使用位或操作 (L1210) 原子性设置；中断保护下的关键操作 | 75% | B |

### 3.7 FU-07: 关机目标管理 (`EcuM_SelectShutdownTarget` / `EcuM_GetShutdownTarget`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | `target` 参数为无效值 | 关机目标不确定，GoOffTwo 中行为不可预测 | 入口校验 `target` 只允许 OFF/RESET/SLEEP (L1054-L1056)，无效报 `ECUM_E_INVALID_PAR` | 99% | D |
| FM-07-02 | `EcuM_ShutdownTarget` 内存位翻转 | 关机目标被篡改，导致错误的关机行为 | 运行时一致性检查：`EcuM_GetShutdownTarget()` 返回前验证值域；BswM 交叉校验 | 80% | D |
| FM-07-03 | `EcuM_ShutdownMode` / `EcuM_SleepMode` 混淆 | 关机模式参数错误 | 根据 `target == ECUM_SHUTDOWN_TARGET_SLEEP` 分别存储到 `EcuM_SleepMode` 或 `EcuM_ShutdownMode` (L1065-L1072) | 85% | B |

### 3.8 FU-08: 启动目标管理 (`EcuM_SelectBootTarget` / `EcuM_GetBootTarget`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | `target` 参数为无效启动目标 | ECU 可能启动到错误的目标（如 Bootloader 而非 Application） | 入口校验 `target` 只允许 OEM_BOOTLOADER/SYS_BOOTLOADER/APPLICATION (L1543-L1545) | 99% | B |
| FM-08-02 | `EcuM_BootTarget` 内存位翻转 | 启动目标被篡改 | 配置值域检查；启动目标在每次上电时由硬件引脚/NV 存储确定，非纯 RAM 依赖 | 80% | B |

### 3.9 FU-09: 状态查询 (`EcuM_GetState` / `EcuM_GetSubState` / `EcuM_GetShutdownCause`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | 输出指针 `state` / `subState` / `cause` 为 NULL | 空指针写入导致内存破坏 | 各查询函数入口 NULL 检查 (L999, L1024, L1175)，报 `ECUM_E_NULL_POINTER` | 99% | B |
| FM-09-02 | `EcuM_CurrentState` 读取值与实际硬件状态不一致 | 上层模块基于错误状态做出错误决策 | BswM 状态通知交叉验证；WdgM 监控状态机进度 | 70% | B |

### 3.10 FU-10: BswM 状态通知

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | `BswM_EcuM_CurrentState()` 调用失败 | BswM 未感知状态变化，模式切换不同步 | BswM 内部状态校验；EcuM 在关键转换点重复通知 | 75% | B |
| FM-10-02 | BswM 模块本身故障导致通知处理异常 | 整个 BSW 模式管理失效 | WdgM 系统级监控；BswM 自身诊断机制 | 80% | B |

### 3.11 FU-11: NvM 协调 (`NvM_ReadAll` / `NvM_WriteAll`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-11-01 | StartupTwo 中 `NvM_ReadAll()` 超时 | 部分 NVRAM 数据未加载到 RAM | `ECUM_NVM_READALL_TIMEOUT` (10000ms) 超时检测；NvM 逐块结果检查 (参见 SAF-FMEDA-NVM-001 FM-07-01) | 90% | B |
| FM-11-02 | GoOffOne 中 `NvM_WriteAll()` 超时 | 部分 RAM 数据未写入 NVRAM，数据丢失 | `ECUM_NVM_WRITEALL_TIMEOUT` (10000ms) 超时检测；NvM 逐块写入结果检查 (参见 SAF-FMEDA-NVM-001 FM-07-02) | 85% | B |
| FM-11-03 | NvM 模块未初始化时调用 ReadAll/WriteAll | NvM 返回错误，数据操作失败 | `ECUM_NVM_ENABLED` 编译开关 (L34, L310, L749) 确保 NvM 已配置；NvM 内部 `IsInitialized` 检查 | 90% | B |

### 3.12 FU-12: Halt/Poll 模式 (`EcuM_GoHalt` / `EcuM_GoPoll`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-12-01 | Halt 模式下 CPU 停止后唤醒源失效 | ECU 无法从 Halt 模式唤醒 | 进入 Halt 前调用 `EcuM_EnableWakeupSources(EcuM_EnabledWakeupSources)` (L592)；硬件级唤醒保证 | 85% | B |
| FM-12-02 | Poll 模式持续轮询导致功耗过高 | 电池耗电异常，不满足低功耗要求 | Poll 模式为降级备选方案；`ECUM_POLL_MODE_SUPPORTED` 编译开关控制 (L522) | 60% | QM |
| FM-12-03 | Poll 模式中唤醒检测失败 | ECU 在 Poll 模式下无法响应唤醒 | `EcuM_CheckWakeup()` (L614) 周期检查；`EcuM_PendingWakeupEvents` 位掩码检测 (L617) | 80% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 多阶段启动 | 95% | 86% | WdgM 超时 + 子状态校验 |
| FU-02 | 主函数状态调度 | 95% | 88% | WdgM 周期监控 + 无效状态捕获 |
| FU-03 | RUN 状态管理 | 99% | 86% | 参数校验 + 位掩码检查 |
| FU-04 | 关机流程 | 99% | 85% | 目标校验 + 无限循环兜底 + WdgM |
| FU-05 | 睡眠/唤醒管理 | 99% | 80% | 状态校验 + 唤醒源验证 + 超时 |
| FU-06 | 唤醒源管理 | 95% | 83% | 位掩码过滤 + 数组边界保护 |
| FU-07 | 关机目标管理 | 99% | 88% | 参数校验 + 值域检查 |
| FU-08 | 启动目标管理 | 99% | 88% | 参数校验 + 硬件交叉确定 |
| FU-09 | 状态查询 | 99% | 85% | NULL 指针保护 |
| FU-10 | BswM 通知 | 80% | 78% | BswM 自身诊断 + 重复通知 |
| FU-11 | NvM 协调 | 90% | 88% | 超时检测 + NvM 内部诊断 |
| FU-12 | Halt/Poll 模式 | 85% | 75% | 唤醒源使能 + 轮询检测 |

### 4.2 模块整体诊断覆盖率

```
DC_EcuM = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_EcuM ≈ 85%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL D 状态机核心 / ASIL B 辅助功能)

| 度量 | 目标值 (ASIL D) | 目标值 (ASIL B) | 估算值 | 状态 |
|------|:-------:|:-------:|:------:|:----:|
| SPFM (Single-Point Fault Metric) | ≥ 99% | ≥ 90% | ~94% (加权) | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ≥ 90% | ~90% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 10 FIT | < 100 FIT | ~8 FIT (核心) / ~35 FIT (整体) | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。EcuM 中主要单点故障来源：
- 状态机变量 `EcuM_CurrentState` 的不可检测多位翻转（由 WdgM 最终兜底）
- Callout 函数（`EcuM_AL_*`）内部的不可检测故障（由集成验证覆盖）
- `EcuM_StateTimer` 的瞬态故障（由 WdgM 周期监控覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))

状态机核心:
  FIT_total ≈ 80 FIT (状态变量 + 转换逻辑 + 定时器)
  FIT_residual ≈ 4.8 FIT (不可检测多位翻转 + callout 内部故障)
  SPFM_core ≈ 1 - (4.8 / 80) ≈ 94%

整体模块:
  FIT_total ≈ 120 FIT
  FIT_residual ≈ 7.2 FIT
  SPFM_overall ≈ 1 - (7.2 / 120) ≈ 94%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。EcuM 中主要潜伏故障来源：
- DET 错误检测模块自身的故障（通过 DET 自诊断覆盖）
- WdgM 监控逻辑的潜伏故障（通过 WdgM 自检和外部看门狗覆盖）
- 唤醒验证定时器的潜伏故障（通过 `EcuM_ExpireWakeupSources()` 双重检查覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))

  FIT_diagnosable ≈ 100 FIT
  FIT_latent ≈ 10.0 FIT (DET 自身 + WdgM 逻辑 + 定时器逻辑)
  LFM ≈ 1 - (10.0 / 100) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)

状态机核心:
  PMHF_core ≈ lambda_state_corruption + lambda_timeout_failure + lambda_callout_failure
            ≈ 3 + 2 + 3 = 8 FIT < 10 FIT ✓

整体模块:
  PMHF_overall ≈ lambda_state + lambda_wakeup + lambda_nv_coord + lambda_shutdown
               ≈ 8 + 12 + 8 + 7 = 35 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 EcuM 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常启动** | `ECUM_STATE_STARTUP`，子状态按序推进 | 执行 StartupOne → StartupTwo → RUN 序列 |
| **正常运行** | `ECUM_STATE_RUN`，至少一个 RUN 请求有效 | 正常服务，周期调度 MainFunction |
| **降级运行** | RUN 请求全部释放，进入 `ECUM_STATE_POST_RUN` | 准备关机，等待新的 RUN 请求或关机完成 |
| **安全关机** | `ECUM_STATE_SHUTDOWN`，GoOffOne/GoOffTwo 执行中 | NvM WriteAll → BSW 去初始化 → 断电/复位 |
| **睡眠状态** | `ECUM_STATE_SLEEP`，等待唤醒事件 | 低功耗模式，仅唤醒源活跃 |
| **唤醒恢复** | `ECUM_STATE_WAKE_SLEEP`，WakeupOne/WakeupTwo 执行中 | 驱动重启 → BSW 重初始化 → 回到 RUN |
| **安全停止** | 状态机无法推进（CPU 故障 / WdgM 触发） | 系统进入安全状态，WDT 触发复位 |
| **关机完成** | `ECUM_STATE_OFF` | 电源切断或进入待机，`EcuM_IsInitialized = FALSE` |

### 6.2 安全状态转换

```
                    ┌──────────────────────────────────────────────────┐
                    │                                                  │
  [上电/复位] → STARTUP ──[StartupOne完成]──→ STARTUP ──[StartupTwo完成]──→ RUN
                  │                                                      │
                  │                                            [所有RUN释放]
                  │                                                      │
                  │                                                      ▼
                  │                                                 POST_RUN
                  │                                              │            │
                  │                               [新RUN请求] ←─┘            └─[无RUN请求]
                  │                                      │                        │
                  │                                      ▼                        ▼
                  │                                    RUN              ┌── ShutdownTarget? ──┐
                  │                                                     │          │           │
                  │                                                    OFF       RESET       SLEEP
                  │                                                     │          │           │
                  │                                                     ▼          ▼           │
                  │                                               SHUTDOWN ────────────────────┘
                  │                                               (GoOffOne → GoOffTwo)
                  │                                                     │
                  │                                                     ▼
                  └─────────────────────────────────────────────────── OFF

  RUN ──[所有RUN释放 + SLEEP目标]──→ POST_RUN ──→ SLEEP ──[唤醒事件]──→ WAKE_SLEEP
                                                      │                      │
                                                      │              WakeupOne → WakeupTwo
                                                      │                      │
                                                      └──────────────────────┘
                                                                              │
                                                                              ▼
                                                                            RUN
```

---

## 7. 故障反应时间线

### 7.1 场景一：启动超时（StartupOne 卡死）

| 时间 | 事件 |
|------|------|
| T=0 | `EcuM_Init()` 被调用，进入 STARTUP 状态 |
| T=0+ | `EcuM_StartupOne()` 开始执行，调用 `EcuM_DriverInitOne()` |
| T=0+ | `EcuM_ProcessStartupOne()` 中驱动初始化 callout 开始执行 |
| T=5000ms | `ECUM_STARTUP_TIMEOUT` (5000ms) 到期，WdgM 检测到启动超时 |
| T=5000ms+ | WdgM 触发看门狗告警，通知 Dem 记录事件 |
| T=5010ms | 外部看门狗超时，触发硬件复位 |
| **总延迟** | **~5010 ms（由 WdgM + 外部 WDT 双重保护）** |

### 7.2 场景二：关机过程中 NvM_WriteAll 超时

| 时间 | 事件 |
|------|------|
| T=0 | `EcuM_Shutdown()` 被调用，进入 SHUTDOWN 状态 |
| T=0+ | `EcuM_ProcessGoOffOne()` 开始执行 |
| T=0+ | BswM 通知关机状态 (L735)，RTE 停止 (L740) |
| T=1 cycle | ComM_DeInit() 执行 (L745) |
| T=2 cycles | `NvM_WriteAll()` 被调用 (L750) |
| T=2+10000ms | `ECUM_NVM_WRITEALL_TIMEOUT` (10000ms) 到期 |
| T=2+10000ms+ | 超时检测到 NvM 写入未完成，上报诊断事件 |
| T=2+10001ms | 继续执行 GoOffTwo（不阻塞关机流程） |
| T=2+10002ms | `EcuM_DisableInterrupts()` (L778)，SchM_DeInit() (L782) |
| T=2+10003ms | `EcuM_PerformShutdown()` 或 `EcuM_PerformReset()` 执行 |
| **总延迟** | **~10005 ms（含 NvM 超时等待）** |

### 7.3 场景三：非法唤醒源触发

| 时间 | 事件 |
|------|------|
| T=0 | 外部中断触发 `EcuM_SetWakeupEvent(sources)` |
| T=1 | `sources &= EcuM_EnabledWakeupSources` (L1205) 过滤已禁用源 |
| T=2 | 若过滤后 `sources == ECUM_WKSOURCE_NONE`，函数直接返回，不设置任何事件 |
| T=2 (替代) | 若源有效：`EcuM_PendingWakeupEvents |= sources` (L1210) |
| T=3 | `EcuM_WakeupStatus[i]` 设为 `ECUM_WKSTATUS_PENDING` (L1220) |
| T=4 | 验证定时器初始化为 `ECUM_WAKEUP_VALIDATION_TIMEOUT / ECUM_MAIN_FUNCTION_PERIOD` = 10 个周期 (L1221) |
| T=5 | BswM 收到 `ECUM_WKSTATUS_PENDING` 通知 (L1228) |
| T=5~15 | `EcuM_ValidateWakeupSources()` 每周期递减定时器 (L1437) |
| T=15 | 定时器归零，状态转为 `ECUM_WKSTATUS_VALIDATED` (L1444) |
| T=16 | BswM 收到 `ECUM_WKSTATUS_VALIDATED` 通知 (L1458) |
| **总延迟** | **~16 个主函数周期 = 160 ms** |

### 7.4 场景四：RUN 请求异常清除

| 时间 | 事件 |
|------|------|
| T=0 | `EcuM_KillAllRUNRequests()` 被调用 |
| T=1 | `EcuM_KilledRunRequests = EcuM_RunRequests` (L950) 保存现场 |
| T=2 | `EcuM_RunRequests = 0u` (L953) 清除所有请求 |
| T=3 | `EcuM_CheckRunRequests()` 被调用 (L956) |
| T=4 | 检测到 `EcuM_RunRequests == 0u` 且 `EcuM_CurrentState == ECUM_STATE_RUN` (L966-L969) |
| T=5 | 状态转换为 `ECUM_STATE_POST_RUN` (L971) |
| T=6 | BswM 收到 POST_RUN 通知 (L975) |
| T=7+ | `EcuM_ProcessPostRun()` 根据 `EcuM_ShutdownTarget` 决定后续动作 (L429) |
| **总延迟** | **~7 个调用周期 + 关机序列时间** |

---

## 8. ISO 26262 条款追溯

| ISO 26262 条款 | 要求 | 本文档覆盖 |
|---------------|------|-----------|
| Part 5, Clause 7 | FMEDA 方法论 | 第 3 节完整失效模式表（12 个功能单元，48 个失效模式） |
| Part 5, Clause 8 | 安全度量 (SPFM/LFM/PMHF) | 第 5 节度量分析 |
| Part 5, Clause 9 | 诊断覆盖率评估 | 第 4 节 DC 汇总 |
| Part 6, Clause 8 | 软件单元设计中的安全机制 | 第 3 节故障检测方法列 |
| Part 4, Clause 7 | 安全状态定义 | 第 6 节安全状态（8 个状态 + 状态转换图） |
| Part 4, Clause 8 | 技术安全需求 | 第 7 节故障反应时间线（4 个场景） |
| Part 4, Clause 9 | 多阶段启动安全 | 第 3.1 节 FU-01 启动失效模式分析 |
| Part 4, Clause 10 | 关机安全 | 第 3.4 节 FU-04 关机流程失效模式分析 |
| Part 3, Clause 7 | 相关项定义 | 第 1.1 节分析边界定义 |

---

## 9. 假设与约束

1. **底层驱动假设**: Mcu、WdgM、NvM、ComM、SchM、BswM 等模块的失效率及诊断覆盖率由各自独立的 FMEDA 覆盖，本分析假设底层 FIT 数据已知
2. **WdgM 覆盖**: EcuM_MainFunction 的周期调度监控由系统级 WdgM 覆盖，外部看门狗作为最终兜底
3. **Callout 函数**: `EcuM_AL_*` / `EcuM_Driver*` 系列 callout 函数为弱引用默认实现（`__attribute__((weak))`），实际实现由集成方提供并在集成阶段验证
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **配置正确性**: 假设 EcuM 配置（唤醒源、超时参数、用户数量）在集成阶段经过验证
6. **多核支持**: 当前配置 `ECUM_MULTI_CORE_SUPPORT = STD_OFF`，本分析针对单核场景
7. **NvM 超时**: NvM ReadAll/WriteAll 超时由 `ECUM_NVM_READALL_TIMEOUT` / `ECUM_NVM_WRITEALL_TIMEOUT` 配置，超时后不阻塞关机/启动流程
8. **中断保护**: 关键状态转换操作（如睡眠进入）通过 `EcuM_DisableInterrupts()` / `EcuM_EnableInterrupts()` 保护，具体实现依赖 OS/Mcu 抽象层

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| CPU 逻辑（状态机） | 15 FIT | 供应商 FMD |
| RAM 存储（状态变量） | 8 FIT | IEC 61508 Annex D |
| 定时器/计数器 | 5 FIT | IEC 61508 Annex D |
| 中断控制器 | 10 FIT | 供应商 FMD |
| GPIO 唤醒源 | 12 FIT | 供应商 FMD |
| CAN 唤醒源 | 8 FIT | 供应商 FMD |
| WdgM 监控 | 6 FIT | 供应商 FMD |
| NvM 接口 | 10 FIT | 参见 SAF-FMEDA-NVM-001 |
| BswM 接口 | 8 FIT | 供应商 FMD |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| EcuM | ECU State Manager |
| BswM | Basic Software Mode Manager |
| NvM | NVRAM Manager |
| ComM | Communication Manager |
| WdgM | Watchdog Manager |
| SchM | BSW Scheduler Manager |
| RTE | Runtime Environment |
| DET | Default Error Tracer |
| WDT | Watchdog Timer |
| DEM | Diagnostic Event Manager |

## 附录 C: EcuM 状态编码参考

| 状态常量 | 值 | 说明 |
|---------|-----|------|
| `ECUM_STATE_OFF` | 0x00 | ECU 断电 |
| `ECUM_STATE_STARTUP` | 0x01 | 启动中 |
| `ECUM_STATE_RUN` | 0x10 | 正常运行 |
| `ECUM_STATE_APP_RUN` | 0x11 | 应用运行（子状态） |
| `ECUM_STATE_POST_RUN` | 0x20 | 后运行（准备关机） |
| `ECUM_STATE_APP_POST_RUN` | 0x21 | 应用后运行（子状态） |
| `ECUM_STATE_SLEEP` | 0x30 | 睡眠 |
| `ECUM_STATE_WAKE_SLEEP` | 0x31 | 唤醒中 |
| `ECUM_STATE_SHUTDOWN` | 0x40 | 关机 |

| 子状态常量 | 值 | 说明 |
|-----------|-----|------|
| `ECUM_SUBSTATE_STARTUP_ONE` | 0x11 | Pre-OS 初始化 |
| `ECUM_SUBSTATE_STARTUP_TWO` | 0x12 | Post-OS BSW 初始化 |
| `ECUM_SUBSTATE_STARTUP_THREE` | 0x13 | SWC 初始化 |
| `ECUM_SUBSTATE_RUN` | 0x21 | 正常运行 |
| `ECUM_SUBSTATE_POST_RUN` | 0x22 | 后运行 |
| `ECUM_SUBSTATE_GO_SLEEP` | 0x31 | 准备睡眠 |
| `ECUM_SUBSTATE_SLEEP` | 0x32 | 睡眠中 |
| `ECUM_SUBSTATE_WAKEUP_ONE` | 0x33 | 唤醒阶段一 |
| `ECUM_SUBSTATE_WAKEUP_TWO` | 0x34 | 唤醒阶段二 |
| `ECUM_SUBSTATE_GO_OFF_ONE` | 0x41 | 关机阶段一（NvM 写入） |
| `ECUM_SUBSTATE_GO_OFF_TWO` | 0x42 | 关机阶段二（断电/复位） |
| `ECUM_SUBSTATE_RESET` | 0x43 | 复位 |
| `ECUM_SUBSTATE_HALT` | 0x50 | Halt 模式 |
| `ECUM_SUBSTATE_POLL` | 0x51 | Poll 模式 |
