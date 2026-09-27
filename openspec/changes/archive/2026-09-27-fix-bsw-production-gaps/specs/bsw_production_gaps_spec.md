# BSW Production Gaps (BSW 量产差距修复) Module Specification

> **Module:** BswProductionGaps (BswM / LinIf / Com / CanIf / CanSM / CanTp / Can)
> **Layer:** Services / ECUAL / MCAL
> **Standard:** AUTOSAR Classic Platform R22-11
> **Platform:** Host (MockHAL) / S32K312
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

本变更按《BSW 深度对标与量产差距分析》P0 清单修复五项量产阻断差距：BswM 决策引擎缺失、LinIf 调度引擎占位、双 Com 错配、CAN 栈瘦身与 CAN FD 宣称不符、质量门禁残留。

### Key Responsibilities
- BswM：模式请求 → 规则表达式求值 → 规则状态迁移 → 动作列表执行；EcuM 状态/唤醒联动
- LinIf：多通道调度表推进、帧发送路径、唤醒/睡眠/状态、调度请求确认
- Com：唯一主线 `services/com`，杜绝头文件解析歧义
- CanIf/CanSM/CanTp/Can：唤醒校验、PN、STmin 计时、FD 长度与配置面

---

## 2. API List

### 2.1 BswM

| API | Change |
|-----|--------|
| `void BswM_Init(const BswM_ConfigType*)` | 保持；NULL 按预编译约定选用默认配置（`BswM_Lcfg.c`）；配置包含端口/表达式/规则/动作表 |
| `Std_ReturnType BswM_RequestMode(uint8 SwCompositionId, BswM_ModeType Mode)` | 保持签名；按组合 ID 定位端口（多端口命中逐一更新）；未匹配组合不报 DET、请求仍锁存；仅未初始化与 `Mode > BSWM_MODE_VALUE_MAX` 上报 DET |
| `void BswM_MainFunction(void)` | 求值全部规则，执行状态迁移对应动作列表 |
| `void BswM_EcuM_CurrentState(EcuM_StateType)` | **新增实现**（EcuM 已调用） |
| `void BswM_EcuM_CurrentWakeup(EcuM_WakeupSourceType, EcuM_WakeupStatusType)` | **新增实现**（EcuM 已调用） |
| `BswM_ModeType BswM_GetCurrentMode/GetRequestedMode(void)` | 保持语义 |

### 2.2 LinIf

| API | Change |
|-----|--------|
| `void LinIf_Init(const LinIf_ConfigType*)` | 逐通道状态初始化 |
| `Std_ReturnType LinIf_Transmit(PduIdType, const PduInfoType*)` | 由空壳改为帧缓冲写入 + 下一调度点发送 |
| `Std_ReturnType LinIf_ScheduleRequest(uint8 Channel, LinIf_ScheduleTableType)` | 由仅声明改为实现（校验 + 待切换） |
| `Std_ReturnType LinIf_WakeUp/GotoSleep(uint8 Channel)` | 由仅声明改为实现（调 MCAL Lin） |
| `void LinIf_MainFunction(void)` | 条目时序推进 + 条目动作执行 |
| `void LinIf_RxIndication(uint8, const LinIf_PduType*)` | 按 PID 派发到上层钩子 |

### 2.3 CAN 栈

| API | Change |
|-----|--------|
| `Std_ReturnType CanIf_SetTrcvMode(...)` | 空壳 → 实化（含模式保存与 DET） |
| `Std_ReturnType CanIf_CheckValidation(...)` | 新增 |
| `CanIf_TxConfirmationStateType CanIf_GetTxConfirmationState(PduIdType)` | 新增 |
| `Std_ReturnType CanSM_ConfirmPnAvailability(NetworkHandleType)` | 新增 |
| `Std_ReturnType CanSM_ClearTrcvWufFlagIndication(NetworkHandleType)` | 新增 |
| CanTp `MainFunction` | STmin 计时器生效 |
| `Can_PduType` / `Can_BaudrateConfigType` | 追加 `FdFrame` / `FdEnabled`+FD 波特率字段 |

---

## 3. Scenarios（黄金用例口径）

### 3.1 BswM
1. 请求 ECU 状态 STARTUP → 规则 1 条件满足 → 执行动作列表（用户回调计数 +1），`GetRequestedMode` 反映最后一次请求
2. 规则初始为 TRUE 的分支：首轮 MainFunction 不重复执行动作（迁移仅发生一次）
3. 表达式 AND：两个端口同时满足才迁移；任一不满足则执行假分支
4. `BswM_RequestMode` 未匹配组合 ID → 不报 DET，请求仍锁存（`GetRequestedMode` 可见）；非法 Mode（> `BSWM_MODE_VALUE_MAX`）→ 返回 `E_NOT_OK` 并 DET `BSWM_E_PARAM_MODE`
5. EcuM 回调：`BswM_EcuM_CurrentState(ECUM_STATE_RUN)` 触发对应端口请求并参与求值

### 3.2 LinIf
1. 调度表条目 DelayMs 累计到期才发送（tick 推进黄金断言调用次数）
2. `ScheduleRequest` 切换后从条目 0 开始并回调 `LinIf_ScheduleRequestConfirmation`
3. `LinIf_Transmit` 写入的 PDU 在下一调度点由 `Lin_SendFrame` 以配置 PID/DLC 发送
4. `LinIf_WakeUp/GotoSleep` 调用 MCAL 对应 API 并推进通道状态
5. `LinIf_Transmit(PduId 越界)` → `LINIF_E_PARAM_PDU`

### 3.3 CAN
1. `CanIf_SetTrcvMode(TRCV_MODE_STANDBY)` 保存模式并可查询
2. `CanIf_CheckValidation` 有唤醒标志时返回 E_OK 并清除
3. `CanSM_ConfirmPnAvailability` 在 FULLCOM 下成功、NOCOM 下 DET
4. CanTp STmin 未到期不发送连续 CF；到期后发送
5. CAN FD：DL=12 的 FF 在 FD 通道通过校验，经典通道拒绝

---

## 4. Dependencies

- `Os_Cfg.c` 10ms 报警已调用 `BswM_MainFunction`（`OsAlarm_BswM_MainFunction`）
- `EcuM.c` 已 extern 声明 `BswM_EcuM_CurrentState/CurrentWakeup` 并在状态迁移点调用
- MCAL `Lin.h` 提供 `Lin_SendFrame/WakeUp/GoToSleep/GetStatus` 完整 API
- MCAL `Can.h` 为 FD 配置字段承载方

---

## 5. Constraints

- 不破坏既有 API 签名与语义（BswM 既有 12+ 用例、LinIf 13 用例、CanIf 17 用例、CanTp 13 用例为回归基线）
- 规则求值仅发生在 `BswM_MainFunction`（任务上下文），不在请求路径做级联执行
- 所有删除动作以"先移除构建引用 → 构建验证 → 再删文件"顺序执行

---

## 6. Known Limitations

- 物理层 FD 时序参数标定、HIL 验证不在本变更（Track C）
- CanTp 接收队列（多帧并发）不在本变更，记录 P1
- MISRA 存量违规与跨层重复模块收敛（P2）另行处理
