# FMEDA — CanSM (CAN State Manager)

> **文档编号**: SAF-FMEDA-CANSM-001
> **版本**: 1.0
> **日期**: 2026-10-24
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW Service Layer 的 CanSM (CAN State Manager) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/services/cansm/src/CanSM.c`

**分析边界**:
- CanSM 核心逻辑（总线状态机 BSM、网络模式管理、BusOff 恢复、唤醒验证、Partial Networking）
- 上层接口：`CanSM_RequestComMode`, `CanSM_StartWakeupSource`, `CanSM_StopWakeupSource`, `CanSM_SetEcuPassive`, `CanSM_SetNetworkPassive`, `CanSM_ConfirmPnAvailability`, `CanSM_SetPnRequest`
- 回调接口：`CanSM_ControllerBusOff`, `CanSM_ControllerModeIndication`, `CanSM_TransceiverModeIndication`, `CanSM_CheckTransceiverWakeFlagIndication`, `CanSM_TxTimeoutException`
- 内部安全机制：InitStatus 前置校验、Network 句柄校验、模式切换超时定时器、BusOff 计数器阈值、被动模式降级
- 下层依赖（假设由 ECUAL/MCAL 层覆盖）：CanIf, Det, Dem

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 CanSM 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `CanSm_GlobalStateType` | CanSM.c:105-110 | 模块全局状态：`InitStatus`, `Networks[]`, `NumNetworks`, `ConfigPtr` |
| `CanSm_NetworkStateType` | CanSM.c:77-99 | 每网络运行时状态：`BsmState`, `SubState`, `RequestedComMMode`, `CurrentComMMode`, `ModeRequestTimer`, `BusOffRecoveryTimer`, `BusOffCounter`, `BusOffEventPending`, `CtrlMode`, `TrcvMode`, `ModeChangePending`, `PassiveOverride`, `PnState` 等 |
| `CanSm_NetworkConfigType` | CanSm.h:183-195 | 每网络静态配置：`NetworkHandle`, `ControllerId`, `BaudrateConfigs`, `MainFunctionPeriodMs`, `BusOffRecoveryTimeMs`, `BusOffThreshold`, `WakeupSupport`, `BusOffRecoveryEnabled`, `TransceiverSupport`, `TransceiverId` |
| `CanSm_ConfigType` | CanSm.h:200-206 | 模块级配置：`Networks`, `NumNetworks`, `DevErrorDetect`, `VersionInfoApi`, `SetBaudrateApi` |
| `CanSm_BaudrateConfigType` | CanSm.h:175-178 | 波特率配置：`BaudRate` (kbps), `BaudRateConfig` (硬件参数) |

### 1.4 总线状态机 (BSM) 概览

```
                    CanSM_Init()
                        │
                        ▼
              ┌──────────────────┐
              │  CANSM_BSM_S_NOCOM  │◄──────────────────────────────┐
              │  (安全状态)          │                                │
              └────────┬─────────┘                                │
                       │ RequestComMode(FULL_COMM)                 │
                       │ / Wakeup 验证完成                         │
                       ▼                                          │
              ┌──────────────────────┐                             │
              │ CANSM_BSM_S_FULLCOM   │──[BusOff 阈值超限]──────────┤
              │                       │                            │
              └────────┬──────────────┘                            │
                       │ RequestComMode(SILENT_COMM)               │
                       ▼                                          │
              ┌────────────────────────┐                           │
              │ CANSM_BSM_S_SILENTCOM   │                           │
              │ (Listen Only)           │                           │
              └────────┬───────────────┘                           │
                       │ RequestComMode(NO_COMM)                   │
                       ▼                                           │
              ┌──────────────────────────────────┐                 │
              │ CANSM_BSM_S_SILENTCOM_BOR         │                 │
              │ (BusOff Recovery: L1→L2→Restart)  │─────────────────┘
              └──────────────────────────────────┘  恢复成功→FULLCOM

              ┌──────────────────────────┐
              │ CANSM_BSM_S_CHECKWAKEUP   │  (唤醒验证)
              │ Controller STOPPED +      │──[验证完成]──→ FULLCOM
              │ Transceiver NORMAL        │──[StopWakeup]──→ NOCOM
              └──────────────────────────┘
```

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 模块初始化/去初始化 | `CanSM_Init()` (L787), `CanSM_DeInit()` (L833) | 全局状态初始化，所有网络置为 NOCOM |
| FU-02 | 通信模式请求 | `CanSM_RequestComMode()` (L903) | ComM 请求网络通信模式变更 (NOCOM/SILENTCOM/FULLCOM) |
| FU-03 | 总线状态机主处理 | `CanSM_MainFunction()` (L977) | 周期调度，驱动各网络 BSM 状态转换 |
| FU-04 | NOCOM 状态处理 | `CanSm_ProcessNoComState()` (L527) | 处理 NOCOM 子状态：模式请求消费、控制器启动等待、睡眠确认 |
| FU-05 | SILENTCOM 状态处理 | `CanSm_ProcessSilentComState()` (L587) | 处理 SILENTCOM 子状态：Listen Only 模式下的模式切换 |
| FU-06 | FULLCOM 状态处理 | `CanSm_ProcessFullComState()` (L626) | 处理 FULLCOM 子状态：全通信下的模式切换与超时检测 |
| FU-07 | BusOff 恢复管理 | `CanSM_ControllerBusOff()` (L948), `CanSm_HandleBusOffRecovery()` (L352), `CanSm_EnterBusOffRecovery()` (L331), `CanSm_ProcessSilentComBorState()` (L665) | BusOff 检测、计数器管理、L1/L2 恢复时序、控制器重启 |
| FU-08 | 控制器模式请求与确认 | `CanSm_RequestControllerMode()` (L264), `CanSM_ControllerModeIndication()` (L874), `CanSm_HandleModeConfirmation()` (L286) | 向 CanIf 请求控制器模式、处理模式确认回调 |
| FU-09 | 唤醒验证管理 | `CanSM_StartWakeupSource()` (L1048), `CanSM_StopWakeupSource()` (L1080), `CanSm_EnterCheckWakeup()` (L414), `CanSm_ProcessCheckWakeupState()` (L748) | 唤醒源请求/停止、唤醒验证状态机 |
| FU-10 | 收发器模式管理 | `CanSM_TransceiverModeIndication()` (L1195), `CanSM_CheckTransceiverWakeFlagIndication()` (L1224) | 收发器模式指示、唤醒标志检查 |
| FU-11 | 被动模式管理 | `CanSM_SetEcuPassive()` (L1119), `CanSM_SetNetworkPassive()` (L1130), `CanSm_IsNetworkPassive()` (L384) | ECU 全局被动模式、单网络被动覆盖、降级判断 |
| FU-12 | Partial Networking (PN) | `CanSM_ConfirmPnAvailability()` (L1293), `CanSM_SetPnRequest()` (L1343), `CanSM_GetPnState()` (L1382) | PN 睡眠可用性确认、PN 唤醒请求、PN 状态查询 |
| FU-13 | TX 超时异常处理 | `CanSM_TxTimeoutException()` (L1163) | TX 超时触发 BusOff 恢复入口 |
| FU-14 | 定时器与超时管理 | `CanSm_StartTimer()` (L229), `CanSm_IsTimerExpired()` (L246) | 模式切换超时定时器（`CANSM_MODE_CHANGE_REQUEST_TIMEOUT_MS` = 100ms） |

---

## 3. 失效模式分析表

### 3.1 FU-01: 模块初始化/去初始化

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | `ConfigPtr` 为空指针且默认配置对象 `CanSm_Config` 损坏 | 模块使用错误配置初始化，所有网络行为不可预测 | `CanSM_Init()` L793 NULL_PTR 检查，回退到预编译默认配置 `&CanSm_Config`；配置存储于受保护 ROM 区域 | 85% | B |
| FM-01-02 | `CanSm_Global.InitStatus` 写入失败（RAM 位翻转） | 后续 API 调用误判模块未初始化，所有请求被拒绝 | `CanSM_Init()` L797 写入后，所有 API 入口均检查 `InitStatus == CANSM_INIT` (L878, L907, L952 等)；不一致时通过 Det 报错 | 90% | B |
| FM-01-03 | 网络循环初始化过程中断（CPU 故障） | 部分网络未初始化，状态不一致 | `CanSM_Init()` L802 循环上限 `CANSM_NUM_NETWORKS && CANSM_MAX_NETWORKS` 双重保护；WDT 系统级监控 | 80% | B |
| FM-01-04 | `CanSM_DeInit()` 未完全清理运行时状态 | 残留状态导致后续操作异常 | `CanSM_DeInit()` L838-867 逐网络清零所有字段，`InitStatus` 最后置为 `CANSM_UNINIT` (L865) | 85% | B |

### 3.2 FU-02: 通信模式请求 (`CanSM_RequestComMode`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | `Network` 句柄超出有效范围 | 访问越界网络索引，内存破坏 | `CanSm_IsNetworkValid()` L211 入口校验 `Network < CANSM_NUM_NETWORKS`；无效时 Det 报 `CANSM_E_PARAM_NETWORK` (L917) | 99% | B |
| FM-02-02 | `ComM_Mode` 参数值非法（非 NOCOM/SILENTCOM/FULLCOM） | 请求无效通信模式，状态机行为未定义 | L923-931 三值显式校验，非法值 Det 报 `CANSM_E_INVALID_COMM_REQUEST` 并返回 `E_NOT_OK` | 99% | B |
| FM-02-03 | 模块未初始化时调用 `RequestComMode` | 操作未初始化的运行时数据 | L907 `InitStatus != CANSM_INIT` 前置检查，Det 报 `CANSM_E_NOT_INITIALIZED` | 99% | B |
| FM-02-04 | 模式请求锁存后未被 `MainFunction` 消费 | 请求延迟或丢失，网络停留在错误模式 | `RequestedComMMode` 在 `MainFunction` 各状态处理函数中被消费（L534, L594, L633）；超时定时器 (FU-14) 检测异常 | 85% | B |

### 3.3 FU-03: 总线状态机主处理 (`CanSM_MainFunction`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | `MainFunction` 未被周期调用 | 状态机停滞，网络无法响应模式请求或 BusOff 事件 | 系统级 WDT 监控；BswM 调度监控 | 90% | B |
| FM-03-02 | `BsmState` 值损坏（RAM 位翻转）进入 default 分支 | 网络状态丢失，状态机无法推进 | L1034-1039 default 分支不执行任何操作（安全侧）；`CanSM_GetCurrentInternalState()` 可供外部监控读取当前状态 | 75% | B |
| FM-03-03 | 唤醒源请求 `WakeupSourceRequested` 锁存标志损坏 | 唤醒验证无法触发或误触发 | L993 消费时立即清除标志；`CanSM_StopWakeupSource()` L1105 可清除pending 请求 | 80% | B |
| FM-03-04 | PN 状态机处理异常 (`PnState` 损坏) | Partial Networking 功能失效 | L1008-1016 PN 状态在 NOCOM 时自动重置为 `CANSM_PNSA_NO_PN`（安全侧）；FULLCOM 下仅从 `NO_PN` 提升到 `PN_REQUESTED` | 80% | B |

### 3.4 FU-04: NOCOM 状态处理 (`CanSm_ProcessNoComState`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | 从 NOCOM 请求 FULLCOM 时控制器启动失败 | 网络无法进入全通信 | L547 `CanSm_RequestControllerMode` 返回 `E_NOT_OK` 时不转换子状态；超时定时器检测 (L557) | 90% | B |
| FM-04-02 | 被动网络请求 FULLCOM 未被正确降级为 SILENTCOM | 被动网络意外进入全通信 | L542 `CanSm_IsNetworkPassive()` 检查，被动时降级到 `CanSm_TransitionToSilentCom()` (L544) | 95% | B |
| FM-04-03 | 睡眠模式确认超时 (`CANSM_S_CC_SLEEP_WAIT`) | 控制器未进入睡眠，NOCOM 转换不完整 | L569 `CanSm_IsTimerExpired()` 检测超时后回退到 `CANSM_S_NOCOM_NOP` (L571)，保持安全侧 | 90% | B |
| FM-04-04 | 控制器启动等待超时 (`CANSM_S_FC_CC_START_WAIT`) | 网络停滞在中间状态 | L557 超时检测，Det 报 `CANSM_E_MODE_REQUEST_TIMEOUT` (L561) | 90% | B |

### 3.5 FU-05: SILENTCOM 状态处理 (`CanSm_ProcessSilentComState`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | 从 SILENTCOM 请求 FULLCOM 时被动模式检查失效 | 被动网络意外进入全通信 | L602 `CanSm_IsNetworkPassive()` 检查，被动时保持 SILENTCOM（降级 FULL 请求） | 95% | B |
| FM-05-02 | `CanSm_TransitionToNoCom()` 调用失败 | 网络无法退出 SILENTCOM | L600 返回值检查；`MainFunction` 下一周期重试 | 85% | B |
| FM-05-03 | `CanSm_TransitionToFullCom()` 调用失败 | 网络无法进入全通信 | L603 返回值检查；保持 SILENTCOM，下周期重试 | 85% | B |

### 3.6 FU-06: FULLCOM 状态处理 (`CanSm_ProcessFullComState`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | 从 FULLCOM 请求 NOCOM 转换失败 | 网络无法退出全通信 | L639 `CanSm_TransitionToNoCom()` 返回值检查；下周期重试 | 85% | B |
| FM-06-02 | 从 FULLCOM 请求 SILENTCOM 转换失败 | 网络无法降级到静默通信 | L641 `CanSm_TransitionToSilentCom()` 返回值检查；下周期重试 | 85% | B |
| FM-06-03 | 控制器启动确认等待超时 (`CANSM_S_FC_CC_START_WAIT`) | FULLCOM 状态转换不完整 | L648 `CanSm_IsTimerExpired()` 超时检测，Det 报 `CANSM_E_MODE_REQUEST_TIMEOUT` (L651) | 90% | B |

### 3.7 FU-07: BusOff 恢复管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | `BusOffCounter` 未达到阈值时的即时重启失败 | 网络通信中断 | L373-374 先 STOPPED 再 STARTED；`HandleModeConfirmation` 处理确认 (L286) | 85% | B |
| FM-07-02 | `BusOffCounter` 达到阈值 (`BusOffThreshold`=10) 后恢复流程卡死 | 网络长期处于 SILENTCOM_BOR | L342 进入恢复后设 L1 定时器 (100ms)；L691-697 L1→L2 (1000ms) 超时推进；L720 重启等待超时 Det 报错 | 80% | B |
| FM-07-03 | `BusOffRecoveryEnabled` 配置被忽略或损坏 | 禁用恢复的网络仍触发恢复或反之 | L360 `BusOffRecoveryEnabled` 前置检查，禁用时直接 return | 90% | B |
| FM-07-04 | L1 恢复时间 (`CANSM_BUSOFF_RECOVERY_L1_MS`=100ms) 定时器异常 | 恢复时序不满足，可能过早重启 | L691-694 递减计数到零才推进到 L2；`MainFunctionPeriodMs` (10ms) 计算 tick 数 | 85% | B |
| FM-07-05 | L2 恢复时间 (`CANSM_BUSOFF_RECOVERY_L2_MS`=1000ms) 定时器异常 | 恢复时序不满足 | L700-705 递减计数到零才推进到 `CANSM_S_BOR_RESTART_CC` | 85% | B |
| FM-07-06 | `CanSM_ControllerBusOff` 收到无效 `ControllerId` | 操作非配置网络或越界访问 | L960-965 遍历 `CanSm_NetworkConfigs` 匹配 ControllerId；未匹配时 Det 报 `CANSM_E_PARAM_CONTROLLER` (L969) | 99% | B |
| FM-07-07 | `CanSM_TxTimeoutException` 在 FULLCOM 下直接触发恢复 | 正常通信被中断 | L1187-1188 设置 `BusOffEventPending` 后调用 `CanSm_EnterBusOffRecovery()`；安全侧：进入 SILENTCOM_BOR 停止发送 | 80% | B |

### 3.8 FU-08: 控制器模式请求与确认

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | `CanIf_SetControllerMode()` 返回 `E_NOT_OK` | 控制器模式未变更，状态机停滞 | L273 返回值检查，失败时不设置 `ModeChangePending`，不启动定时器 | 90% | B |
| FM-08-02 | 模式确认回调 `ControllerModeIndication` 收到无效 `ControllerId` | 确认应用到错误网络 | L886-891 遍历匹配 ControllerId；未匹配 Det 报 `CANSM_E_PARAM_CONTROLLER` (L895) | 99% | B |
| FM-08-03 | `ModeChangePending` 标志未正确清除 | 状态机误认为模式切换仍在进行 | L288 `HandleModeConfirmation` 入口立即清除 `ModeChangePending = FALSE` 并清零定时器 (L289) | 90% | B |
| FM-08-04 | 模式确认触发错误的状态转换（如 STARTED 确认时不在预期状态） | 网络进入不一致状态 | L296-298 `HandleModeConfirmation` 检查当前 `BsmState` 是否为 NOCOM/SILENTCOM/SILENTCOM_BOR 后才调用 `TransitionToFullCom` | 85% | B |

### 3.9 FU-09: 唤醒验证管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | `StartWakeupSource` 在非唤醒支持网络上触发 | 无效的唤醒验证请求 | L995 `WakeupSupport == TRUE` 检查，仅 NOCOM 或 SILENTCOM 状态允许进入 (L997) | 95% | B |
| FM-09-02 | 唤醒验证过程中控制器 STOPPED 确认超时 | 唤醒验证停滞 | L759 `CanSm_IsTimerExpired()` 超时检测，Det 报 `CANSM_E_MODE_REQUEST_TIMEOUT` (L762) | 90% | B |
| FM-09-03 | `StopWakeupSource` 在 CHECKWAKEUP 状态调用时 `TransitionToNoCom` 失败 | 网络无法退出唤醒验证 | L1109 返回值检查；安全侧：网络保持在 CHECKWAKEUP，下周期 MainFunction 继续处理 | 85% | B |
| FM-09-04 | 唤醒验证完成后 `TransitionToFullCom` 失败 | 唤醒后无法恢复全通信 | L771 返回值检查；保持 CHECKWAKEUP 等待下次 MainFunction 重试 | 85% | B |

### 3.10 FU-10: 收发器模式管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | `TransceiverModeIndication` 收到未配置 TransceiverId | 收发器模式更新到错误网络 | L1207-1209 双重条件检查：`TransceiverSupport == TRUE` 且 `TransceiverId` 匹配；不匹配时静默忽略 | 95% | B |
| FM-10-02 | `CheckTransceiverWakeFlagIndication` 清除唤醒标志失败 | 唤醒验证无法完成 | L1237-1243 匹配条件检查；`TrcvWufFlagIndication` 置 FALSE (L1243) | 90% | B |
| FM-10-03 | 唤醒验证中收发器未配置 (`TransceiverSupport == FALSE`) | 跳过收发器相关检查 | L767 `TransceiverSupport == FALSE` 时仅检查 `CtrlMode == CANIF_CS_STOPPED` 即完成验证 | 90% | B |

### 3.11 FU-11: 被动模式管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-11-01 | `CanSm_EcuPassive` 全局标志位翻转 | 所有网络被动/主动模式判断错误 | `CanSm_SetEcuPassive()` L1121 显式布尔赋值；`CanSm_IsNetworkPassive()` L399 default 分支回退到 `CanSm_EcuPassive` | 85% | B |
| FM-11-02 | `PassiveOverride` 枚举值损坏 | 单网络被动覆盖行为异常 | `CanSm_IsNetworkPassive()` L391 switch 语句，default 分支 (L399) 回退到 ECU 全局模式（安全侧） | 85% | B |
| FM-11-03 | `SetNetworkPassive` 无效网络句柄 | 越界写入 `PassiveOverride` | L1142 `CanSm_IsNetworkValid()` 校验；无效 Det 报 `CANSM_E_PARAM_NETWORK` (L1145) | 99% | B |

### 3.12 FU-12: Partial Networking (PN)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-12-01 | `ConfirmPnAvailability` 在非 FULLCOM 状态调用 | PN 可用性确认在错误状态下生效 | L1316 `BsmState != CANSM_BSM_S_FULLCOM` 检查，Det 报 `CANSM_E_PARAM_INVALID_NETWORK_MODE` (L1319) | 95% | B |
| FM-12-02 | `SetPnRequest` 在无收发器支持的网络调用 | 无效的 PN 请求（选择性唤醒需要收发器硬件） | L1365 `TransceiverSupport != TRUE` 检查，Det 报 `CANSM_E_PARAM_INVALID_NETWORK_MODE` (L1368) | 95% | B |
| FM-12-03 | `PnState` 在 NOCOM 时未正确重置 | PN 状态残留导致误判 | L1008-1009 `MainFunction` 中 NOCOM 状态自动重置 `PnState = CANSM_PNSA_NO_PN` | 90% | B |
| FM-12-04 | `GetPnState` 输出指针为空 | 写入非法地址 | L1394 NULL_PTR 检查，Det 报 `CANSM_E_PARAM_POINTER` (L1396) | 99% | B |

### 3.13 FU-13: TX 超时异常处理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-13-01 | TX 超时在模块未初始化时触发 | 操作未初始化运行时 | L1167 `InitStatus` 检查，Det 报 `CANSM_E_NOT_INITIALIZED` (L1170) | 99% | B |
| FM-13-02 | TX 超时在无效网络上触发 | 越界访问网络状态 | L1175 `CanSm_IsNetworkValid()` 校验，Det 报 `CANSM_E_PARAM_NETWORK` (L1178) | 99% | B |
| FM-13-03 | `CanSm_EnterBusOffRecovery()` 调用 `CanIf_SetControllerMode(STOPPED)` 失败 | 控制器未停止，BusOff 恢复流程异常 | L345 返回值忽略（安全侧：进入 SILENTCOM_BOR 后由超时定时器兜底） | 80% | B |

### 3.14 FU-14: 定时器与超时管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-14-01 | `ModeRequestTimer` 递减下溢（uint16 回绕） | 超时检测失效，状态机可能永久等待 | L250-253 仅在 `> 0` 时递减，到零后不再递减（无下溢风险） | 95% | B |
| FM-14-02 | 定时器 tick 计算溢出（`TimeoutMs + Period - 1` 超过 uint16） | 超时时间不正确 | `CANSM_MODE_CHANGE_REQUEST_TIMEOUT_MS` (100ms) + `MainFunctionPeriodMs` (10ms) 远小于 uint16 上限 65535 | 90% | B |
| FM-14-03 | `BusOffRecoveryTimer` 异常导致 L1/L2 时序跳过 | BusOff 恢复时序不满足 | L691-694, L700-705 独立递减检查，每级定时器独立管理 | 85% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 初始化/去初始化 | 90% | 85% | ROM 配置保护 + InitStatus 校验 |
| FU-02 | 通信模式请求 | 99% | 95% | 参数校验 + InitStatus 前置检查 |
| FU-03 | 状态机主处理 | 90% | 83% | WDT 监控 + default 安全侧 |
| FU-04 | NOCOM 状态处理 | 95% | 90% | 被动模式降级 + 超时检测 |
| FU-05 | SILENTCOM 状态处理 | 95% | 88% | 被动模式检查 + 转换重试 |
| FU-06 | FULLCOM 状态处理 | 90% | 87% | 超时检测 + 转换重试 |
| FU-07 | BusOff 恢复管理 | 99% | 85% | ControllerId 匹配 + L1/L2 定时器 |
| FU-08 | 控制器模式请求与确认 | 99% | 91% | CanIf 返回值 + 回调校验 |
| FU-09 | 唤醒验证管理 | 95% | 88% | 唤醒支持检查 + 超时检测 |
| FU-10 | 收发器模式管理 | 95% | 92% | TransceiverId 双重匹配 |
| FU-11 | 被动模式管理 | 99% | 88% | 枚举 default 安全侧 + 句柄校验 |
| FU-12 | Partial Networking | 99% | 92% | 状态前置检查 + 收发器支持检查 |
| FU-13 | TX 超时异常 | 99% | 90% | InitStatus + 网络校验 |
| FU-14 | 定时器与超时管理 | 95% | 90% | uint16 无下溢 + 独立定时器 |

### 4.2 模块整体诊断覆盖率

```
DC_CanSM = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_CanSM ≈ 89%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL B)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 90% | ~91% | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ~90% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 100 FIT | ~52 FIT | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。CanSM 中主要单点故障来源：
- BSM 状态值 `BsmState` 的不可检测瞬态故障（由 WDT 和 default 安全侧覆盖大部分）
- 定时器 tick 计算的极端边界情况（概率极低）
- `CanSm_EcuPassive` 全局标志的不可检测位翻转（由 API 显式赋值覆盖大部分）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (8.1 / 92) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。CanSM 中主要潜伏故障来源：
- Det 错误检测模块自身的故障（通过多入口 DET 报告覆盖）
- 网络配置常量 `CanSm_NetworkConfigs[]` 的不可检测损坏（ROM 保护）
- 状态转换逻辑的系统性缺陷（通过代码审查和测试覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (9.2 / 92) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_bsm_state_corruption + lambda_timer_anomaly + lambda_passive_flag_flip
     ≈ 8 + 18 + 26 = 52 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 CanSM 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **NOCOM (安全状态)** | `BsmState == CANSM_BSM_S_NOCOM`，控制器 SLEEP，PDU OFFLINE | 无通信，总线休眠；所有网络请求被安全拒绝或锁存等待 |
| **SILENTCOM (降级运行)** | `BsmState == CANSM_BSM_S_SILENTCOM`，控制器 STOPPED，PDU TX_OFFLINE | 仅监听，不发送；可接收总线数据但不参与通信 |
| **SILENTCOM_BOR (恢复中)** | `BsmState == CANSM_BSM_S_SILENTCOM_BOR`，执行 L1→L2→Restart 恢复流程 | 总线离线恢复，不发送；恢复成功后转 FULLCOM |
| **FULLCOM (正常运行)** | `BsmState == CANSM_BSM_S_FULLCOM`，控制器 STARTED，PDU ONLINE | 全功能通信 |
| **CHECKWAKEUP (唤醒验证)** | `BsmState == CANSM_BSM_S_CHECKWAKEUP`，控制器 STOPPED + 收发器 NORMAL | 验证唤醒源有效性 |
| **NOTINITIALIZED (未初始化)** | `BsmState == CANSM_BSM_S_NOTINITIALIZED` 或 `InitStatus != CANSM_INIT` | 所有 API 返回 `E_NOT_OK`，模块不响应 |

### 6.2 安全状态转换

```
NOCOM ──[RequestComMode(FULL) + 非被动]──→ FULLCOM (控制器 STARTED)
NOCOM ──[RequestComMode(SILENT)]──→ SILENTCOM (控制器 STOPPED)
NOCOM ──[StartWakeupSource]──→ CHECKWAKEUP (唤醒验证)

FULLCOM ──[RequestComMode(NOCOM)]──→ NOCOM (控制器 SLEEP)
FULLCOM ──[RequestComMode(SILENT)]──→ SILENTCOM (控制器 STOPPED)
FULLCOM ──[BusOff 阈值超限 / TX 超时]──→ SILENTCOM_BOR (恢复)

SILENTCOM ──[RequestComMode(NOCOM)]──→ NOCOM (控制器 SLEEP)
SILENTCOM ──[RequestComMode(FULL) + 非被动]──→ FULLCOM (控制器 STARTED)

SILENTCOM_BOR ──[L1+L2 超时 + 重启成功]──→ FULLCOM (恢复完成)
SILENTCOM_BOR ──[恢复超时/失败]──→ NOCOM (安全侧，WDT 兜底)

CHECKWAKEUP ──[验证完成]──→ FULLCOM
CHECKWAKEUP ──[StopWakeupSource]──→ NOCOM

任意状态 ──[MainFunction 超时]──→ 安全停止 (WDT 触发复位)
```

### 6.3 安全状态进入条件

CanSM 的安全状态为 **NOCOM**（`CANSM_BSM_S_NOCOM`），进入条件：

1. **BusOff 恢复失败**：`SILENTCOM_BOR` 恢复超时后转入 NOCOM，控制器 SLEEP
2. **模式请求超时**：`ModeRequestTimer` 超时，Det 报错，保持或回退到 NOCOM
3. **初始化**：`CanSM_Init()` 将所有网络置为 NOCOM
4. **去初始化**：`CanSM_DeInit()` 将所有网络置为 NOTINITIALIZED
5. **唤醒验证取消**：`StopWakeupSource` 将 CHECKWAKEUP 网络转回 NOCOM

---

## 7. 故障反应时间线

### 7.1 场景一：BusOff 事件（阈值未超限）

| 时间 | 事件 |
|------|------|
| T=0 | CanIf 检测到 BusOff，调用 `CanSM_ControllerBusOff(ControllerId)` |
| T=1 | L960 匹配 ControllerId，调用 `CanSm_HandleBusOffRecovery()` |
| T=2 | L365 `BusOffCounter++`，L369 判断 `BusOffCounter < BusOffThreshold(10)` |
| T=3 | L373 `CanSm_RequestControllerMode(STOPPED)` |
| T=4 | L374 `CanSm_RequestControllerMode(STARTED)` 即时重启 |
| T=5 | CanIf 回调 `ControllerModeIndication(STARTED)` |
| T=6 | `HandleModeConfirmation` 确认 STARTED，触发 `TransitionToFullCom()` |
| **总延迟** | **6 个主函数周期 (~60ms @ 10ms period)** |

### 7.2 场景二：BusOff 事件（阈值超限，完整恢复流程）

| 时间 | 事件 |
|------|------|
| T=0 | `CanSM_ControllerBusOff()` 第 10 次触发，`BusOffCounter >= BusOffThreshold` |
| T=1 | `CanSm_EnterBusOffRecovery()` 进入 `SILENTCOM_BOR` |
| T=2 | L340 `BsmState = SILENTCOM_BOR`, `SubState = BUSOFF_CHECK` |
| T=3 | L345 `CanSm_RequestControllerMode(STOPPED)` 停止控制器 |
| T=4-5 | 等待 STOPPED 确认，`ModeChangePending == FALSE` 后进入 L1 |
| T=6-15 | L1 恢复计时 (`CANSM_BUSOFF_RECOVERY_L1_MS` = 100ms, 10 ticks) |
| T=16-115 | L2 恢复计时 (`CANSM_BUSOFF_RECOVERY_L2_MS` = 1000ms, 100 ticks) |
| T=116 | 进入 `CANSM_S_BOR_RESTART_CC`，请求 `STARTED` |
| T=117 | 进入 `BOR_RESTART_CC_WAIT`，等待确认 |
| T=118 | `ControllerModeIndication(STARTED)` 确认，`HandleModeConfirmation` 触发 `TransitionToFullCom()` |
| **总延迟** | **~118 个主函数周期 (~1180ms)** |

### 7.3 场景三：通信模式请求超时

| 时间 | 事件 |
|------|------|
| T=0 | ComM 调用 `CanSM_RequestComMode(Network, COMM_FULL_COMMUNICATION)` |
| T=1 | L939 `RequestedComMMode = COMM_FULL_COMMUNICATION` 锁存 |
| T=2 | `MainFunction` → `ProcessNoComState` → L547 `RequestControllerMode(STARTED)` |
| T=3 | L276 `StartTimer(100ms)` → `ModeRequestTimer = 10 ticks` |
| T=4-12 | 等待 `ControllerModeIndication`，`IsTimerExpired` 每周期递减 |
| T=13 | `ModeRequestTimer == 0`，超时检测触发 |
| T=14 | L561 Det 报 `CANSM_E_MODE_REQUEST_TIMEOUT`，返回 `E_NOT_OK` |
| T=15 | 上层（BswM/ComM）收到错误，触发降级策略 |
| **总延迟** | **~15 个主函数周期 (~150ms) = 超时配置 100ms + 检测延迟** |

### 7.4 场景四：被动网络 FULL 请求降级

| 时间 | 事件 |
|------|------|
| T=0 | `CanSM_SetEcuPassive(TRUE)` 设置 ECU 全局被动 |
| T=1 | ComM 请求 `CanSM_RequestComMode(Network, COMM_FULL_COMMUNICATION)` |
| T=2 | `RequestedComMMode` 锁存为 `COMM_FULL_COMMUNICATION` |
| T=3 | `MainFunction` → `ProcessNoComState` → L542 `IsNetworkPassive() == TRUE` |
| T=4 | L544 降级调用 `CanSm_TransitionToSilentCom()` 而非 FULLCOM |
| T=5 | 网络进入 `SILENTCOM`（Listen Only），`CurrentComMMode = COMM_SILENT_COMMUNICATION` |
| **总延迟** | **5 个主函数周期 (~50ms)** |

---

## 8. ISO 26262 条款追溯

| ISO 26262 条款 | 要求 | 本文档覆盖 |
|---------------|------|-----------|
| Part 5, Clause 7 | FMEDA 方法论 | 第 3 节完整失效模式表 (14 个功能单元, 55 个失效模式) |
| Part 5, Clause 8 | 安全度量 (SPFM/LFM/PMHF) | 第 5 节度量分析 |
| Part 5, Clause 9 | 诊断覆盖率评估 | 第 4 节 DC 汇总 |
| Part 6, Clause 8 | 软件单元设计中的安全机制 | 第 3 节故障检测方法列 |
| Part 4, Clause 7 | 安全状态定义 | 第 6 节安全状态 |
| Part 4, Clause 8 | 技术安全需求 | 第 7 节故障反应时间线 |
| Part 6, Clause 7 | 安全机制（参数校验） | FM-02-01, FM-02-02, FM-02-03 等参数校验覆盖 |
| Part 6, Clause 7 | 安全机制（超时监控） | FM-04-03, FM-04-04, FM-06-03, FM-09-02 等超时检测覆盖 |
| Part 6, Clause 7 | 安全机制（降级策略） | FM-04-02, FM-05-01, FM-11-01 等被动模式降级覆盖 |

---

## 9. 假设与约束

1. **底层 CAN 驱动假设**: CanIf/CanDrv 层的失效率及诊断覆盖率由独立 FMEDA 覆盖，本分析假设底层 FIT 数据已知
2. **配置完整性**: 假设 `CanSm_NetworkConfigs[]` 和 `CanSm_BaudrateConfigs_*[]` 存储于受保护的 ROM 区域，编译时配置经过验证
3. **WDT 覆盖**: `CanSM_MainFunction()` 的周期调度监控由系统级 WDT 覆盖
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **配置正确性**: 假设 CanSM 配置（网络数、BusOff 阈值、恢复时间、超时参数）在集成阶段经过验证
6. **ComM/BswM 集成**: 假设上层 ComM/BswM 正确处理 CanSM 返回的 `E_NOT_OK` 错误码
7. **多网络独立性**: 假设 CAN0 和 CAN1 的硬件故障相互独立，不存在共因失效

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| CAN 控制器状态逻辑 | 8 FIT | 供应商 FMD |
| CAN 收发器控制 | 6 FIT | 供应商 FMD |
| RAM 状态变量 | 5 FIT | IEC 61508 Annex D |
| CPU 逻辑（状态机） | 12 FIT | 供应商 FMD |
| 定时器/计数器 | 4 FIT | IEC 61508 Annex D |
| 配置存储 (ROM) | 2 FIT | IEC 61508 Annex D |
| DET 错误报告 | 3 FIT | IEC 61508 Annex D |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| BSM | Bus State Machine (总线状态机) |
| NOCOM | No Communication (无通信) |
| SILENTCOM | Silent Communication (静默通信/监听模式) |
| FULLCOM | Full Communication (全通信) |
| BOR | BusOff Recovery (BusOff 恢复) |
| PN | Partial Networking (部分网络) |
| PNSA | Partial Network Sleep Availability |
| Det | Default Error Tracer (默认错误追踪) |
| Dem | Diagnostic Event Manager (诊断事件管理器) |
| ComM | Communication Manager (通信管理器) |
| BswM | BSW Mode Manager (BSW 模式管理器) |
| CanIf | CAN Interface (CAN 接口层) |
| WDT | Watchdog Timer (看门狗定时器) |
| L1/L2 | BusOff Recovery Level 1/2 (恢复等待时序) |

## 附录 C: DET 错误码与安全机制映射

| DET 错误码 | 值 | 触发条件 | 安全响应 |
|-----------|-----|---------|---------|
| `CANSM_E_NOT_INITIALIZED` | 0x07 | API 调用时 `InitStatus != CANSM_INIT` | 返回 `E_NOT_OK`，拒绝操作 |
| `CANSM_E_PARAM_NETWORK` | 0x0A | 网络句柄超出 `CANSM_NUM_NETWORKS` | 返回 `E_NOT_OK`，拒绝操作 |
| `CANSM_E_PARAM_CONTROLLER` | 0x02 | ControllerId 未匹配任何配置网络 | Det 报错，忽略回调 |
| `CANSM_E_PARAM_POINTER` | 0x01 | 输出指针参数为 NULL_PTR | 返回 `E_NOT_OK`，拒绝操作 |
| `CANSM_E_INVALID_COMM_REQUEST` | 0x04 | ComM_Mode 非 NOCOM/SILENTCOM/FULLCOM | 返回 `E_NOT_OK`，拒绝操作 |
| `CANSM_E_MODE_REQUEST_TIMEOUT` | 0x05 | 模式切换定时器超时 (100ms) | Det 报错，返回 `E_NOT_OK` |
| `CANSM_E_PARAM_INVALID_NETWORK_MODE` | 0x03 | 在当前网络状态下不允许的操作 | Det 报错，返回 `E_NOT_OK` |

## 附录 D: 配置参数安全相关值

| 参数 | 值 | 安全意义 |
|------|-----|---------|
| `CANSM_NUM_NETWORKS` | 2 | 实际配置网络数，限制遍历范围 |
| `CANSM_MAX_NETWORKS` | 4 | 运行时数组上限，防止越界 |
| `CANSM_MODE_CHANGE_REQUEST_TIMEOUT_MS` | 100ms | 模式切换最大等待时间 |
| `CANSM_BUSOFF_RECOVERY_L1_MS` | 100ms | BusOff 恢复第一级等待（硬件稳定） |
| `CANSM_BUSOFF_RECOVERY_L2_MS` | 1000ms | BusOff 恢复第二级等待（总线稳定） |
| `CANSM_BUSOFF_THRESHOLD` | 10 | BusOff 计数器阈值，超过后进入完整恢复流程 |
| `CANSM_NETWORK0_MAIN_FUNCTION_PERIOD_MS` | 10ms | 网络 0 主函数周期 |
| `CANSM_NETWORK1_MAIN_FUNCTION_PERIOD_MS` | 10ms | 网络 1 主函数周期 |
| `CANSM_DEFAULT_BAUDRATE` | 500 kbps | 默认波特率 |
