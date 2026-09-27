# 设计文档：fix-bsw-production-gaps

## 决策记录

### AD1: BswM 引擎模型（AUTOSAR 4.4 对齐子集）

现状 292 行实现缺少 AUTOSAR SWS 的核心概念：模式请求端口（ModeRequestPort）、规则条件表达式（Rule/LogicalExpression）、动作列表（ActionList）与规则状态迁移。重写采用如下模型：

- **模式请求端口**：每个端口 = `(PortId, SwCompositionId, ModeRequestPortType)`，运行时保存当前请求值与有效标志。`BswM_RequestMode(SwCompositionId, Mode)` 按组合 ID 定位端口并写入请求（多端口命中时逐一更新）；未匹配任何端口的组合不报 DET，请求仍锁存供 `BswM_GetRequestedMode` 读取；DET 仅覆盖未初始化（`BSWM_E_UNINIT`）与 `Mode > BSWM_MODE_VALUE_MAX`（`BSWM_E_PARAM_MODE`）。
- **规则**：`BswM_RuleType` = { 条件表达式索引, 真分支 ActionListId, 假分支 ActionListId, 初始状态 }。条件为表达式树：`BswM_ExpressionType` 支持 `BSWM_EQ/BSWM_NEQ`（端口值比较）、`BSWM_AND/BSWM_OR/BSWM_NOT`（逻辑组合），叶子为 `(PortId, Operator, Value)`。求值结果缓冲在规则运行时表中。
- **规则状态迁移**：仅当规则求值结果发生变化（FALSE→TRUE 执行真分支，TRUE→FALSE 执行假分支）时执行动作列表，避免每周期重复执行动作。初始状态可配置（`BswM_RULE_INITIAL_STATE`）。
- **动作列表**：`BswM_ActionListType` = 动作索引数组；动作执行器按类型分发：`BSWM_ACTION_USER_CALLOUT`（配置函数指针 + 参数）、`BSWM_ACTION_MODE_SWITCH`（调用注册的模式回调）。函数指针保持 `void(*)(BswM_ModeType)` 既有形状，避免破坏现有测试。
- **执行时机**：`BswM_RequestMode` 标记待处理；`BswM_MainFunction`（10ms 闹钟已接线，`Os_Cfg.c:326`）统一求值并执行迁移。不在 ISR 上下文做规则求值，保证可重入安全。
- **EcuM 联动**：新增 `BswM_EcuM_CurrentState(EcuM_StateType)` / `BswM_EcuM_CurrentWakeup(...)` 实现（`EcuM.c` 已 extern 声明并调用），将 EcuM 状态映射为 `BSWM_ECUM_STATE_*` 模式请求端口值，走同一规则引擎；`EcuM_ProcessStartupTwo` 的 `BswM_Init(NULL_PTR)` 予以保留——本代码库预编译约定下 NULL 选用 `BswM_Lcfg.c` 的默认配置对象，非 DET 错误（`test_BswM_Init_NullPtr_ShouldSelectDefaultConfig` 固化），EcuM 调用点无需修改。

既有 API（`BswM_GetCurrentMode`/`BswM_GetRequestedMode`/`BswM_GetVersionInfo`）保留语义：CurrentMode = 最近一次规则迁移请求的目标模式；RequestedMode = 最近一次 `BswM_RequestMode` 的入参。

### AD2: LinIf Schedule 引擎

- **通道状态机**：`LINIF_CHANNEL_UNINIT → LINIF_CHANNEL_INIT → LINIF_CHANNEL_ONLINE → LINIF_CHANNEL_SLEEP`，逐通道保存 `currentSchedule`、`currentEntry`、`entryDelayCounter`、`ticksInSchedule`。
- **调度执行**（`LinIf_MainFunction`，每毫秒 tick）：每通道若当前调度表非 NULL，按当前条目推进：条目 `DelayMs` 到期 → 执行条目动作（对主节点发送帧调用 `Lin_SendFrame(channel, pid, dlc, data)`；对发布帧从 `LinIf_Transmit` 写入的 PDU 缓冲取数），`currentEntry = (currentEntry + 1) % EntryCount`。到达最后一个条目后循环回第一个条目（AUTOSAR 连续调度语义）。
- **调度请求**：`LinIf_ScheduleRequest(Channel, Schedule)` 校验调度表存在后将请求置入通道待处理位；`LinIf_MainFunction` 在条目边界切换（AUTOSAR 允许立即或在当前条目结束时切换，取"立即切换 + 从条目 0 开始"实现并由用例固化）。切换完成调用上层回调 `LinIf_ScheduleRequestConfirmation(Channel, Schedule)`（弱符号 hook，LinNm 已按此名定义回调）。
- **发送路径**：`LinIf_Transmit(TxPduId, PduInfoPtr)` 按 PDU→帧映射写入帧缓冲（带 DLC 校验与 DET `LINIF_E_PARAM_PDU`）；未在调度中的帧标记为待发，下一周期由调度器发送。
- **唤醒/睡眠**：`LinIf_WakeUp(Channel)` → `Lin_WakeUp`；`LinIf_GotoSleep(Channel)` → `Lin_GoToSleep`；完成回调 `LinIf_WakeUpConfirmation`/`LinIf_GotoSleepConfirmation`（弱符号）。全部 API 带 `state < INIT` 的 DET 守卫。
- **Rx 派发**：`LinIf_RxIndication` 按 PID 找帧配置，复制到发布缓冲并调用 `LinIf_RxCallback` 弱符号钩子（上层 LIN TP/SW 挂接点）。

### AD3: 双 Com 收敛

- 事实：`classic/com` 进全局 include 路径（根 `CMakeLists.txt:125`）但无任何 `add_subdirectory`/目标编译；实际参与链接的是 `services/com`。头文件解析优先级不确定 = 错配风险。
- 决策：**删除 `classic/com` 目录并移除 include 路径**。理由：`services/com` 有测试资产、被 GLOB 编译、被 Rte/Os_Cfg/IpduM 链接；classic 版无任何编译接线，是纯遗留物。
- 验证顺序：先删 include 路径 → 全量构建（暴露所有 `Com.h` 解析失败点）→ 通过后删目录 → 全量 ctest。
- `tests/unit/com/CMakeLists.txt` 引用的 `src/autosar/classic/com` 路径本就不存在（废弃树），随 P0-5 一并清理。

### AD4: CanIf/CanSM 网络管理面

- `CanIf_SetTrcvMode` 从空壳改为调用 `CanTrcv_SetOpMode`（收发器驱动 API 存在时）并保存模式；`CanIf_CheckWakeup` 已实现，补充 `CanIf_CheckValidation`（聚合各控制器唤醒校验）与 `CanIf_GetTxConfirmationState`（按 PDU 查询确认状态）。
- CanSM 补 PN 面：`CanSM_ConfirmPnAvailability(NetworkHandleType)`（透传 `CanIf_ConfirmPnAvailability` 语义）与 `CanSM_ClearTrcvWufFlagIndication(NetworkHandleType)`；两者纳入既有状态机（仅 FULLCOM/校验阶段可调用，否则 DET）。

### AD5: CanTp 机制强化

- **STmin 计时器**：MainFunction 现有注释明确"真实实现应有独立 STmin 计时器"。为每通道增加 `StMinTimerMs` 与 `StMinActive`，发送 CF 前检查计时到期，消除连续 CF 无间隔发送的协议违规。
- **CAN FD 长度校验**：引入 `CanTp_FrameLength` 辅助（CAN 8 字节 / CAN FD 合法 DLC 集合 {0..8,12,16,20,24,32,48,64}），FF 的 DL 与长度一致性校验支持 FD 语义；保持 8 字节经典路径行为不变。
- **接收队列**：保持现有逐通道缓冲结构（单帧接收已满足窗口控制器诊断路径），不为假设需求引入队列抽象——记录到 P1 观察项。

### AD6: CAN FD 配置字段（MCAL Can）

- `Can_BaudrateConfigType` 增加 `boolean FdEnabled` 与 FD 数据段波特率字段（`FdDataBaudRate` 等，默认关闭）；`Can_PduType` 增加 `boolean FdFrame` 标志；`Can_Write` 按标志选择经典/FD 帧装配路径。保持既有字段顺序与初始化（追加字段置尾，兼容现有初始化器）。
- 目标芯片 S32K312 FlexCAN 支持 FD；本变更只落**配置面与数据路径开关**，物理层时序参数由板级配置在 HIL 阶段标定（记录到不包含内容）。

### AD7: 质量门禁清理边界

- `legacy/` 7 个目录（crypto/csm/dcm/dem/doip/nvm/xcp）已不被 GLOB 编译（`services/CMakeLists.txt:58` 过滤 `*_impl.c`）；删除目录前逐目录确认无 include 引用。
- `tests/unit` 废弃树：从 `tests/CMakeLists.txt:30` 移除 `add_subdirectory(unit)`，确认无其他目标依赖后删除目录树。
- TODO/stub 123 处不做一次性清零（范围失控）；生成《TODO/Stub 台账》挂单到 reports/，各模块归属明确，作为 P1/P2 输入。

## 测试修正策略

- 所有行为变更遵循"先红后绿"：扩展用例先行，断言语义化黄金值（如调度条目时序、动作执行计数），删除占位性断言。
- 回归口径：全量 `ctest --test-dir build-native`，零失败为门禁。
