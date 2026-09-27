# 任务清单

## P0-1 BswM Rule/ActionList 引擎

- [x] 核实基线与调用点：BswM 现有用例（tests/bsw/services/bswm 两个套件）、EcuM 调用链、Os_Cfg 10ms 闹钟接线
- [x] 重写 `BswM.h` / `BswM_Cfg.h`：端口/表达式/规则/动作列表类型与配置宏（含 BSWM_MAX_EXPRESSIONS）
- [x] 重写 `BswM.c`：请求-求值-迁移-动作执行引擎 + EcuM 回调实现（CurrentState/CurrentWakeup）
- [x] 重写 `BswM_Lcfg.c`：窗口控制器场景配置（EcuM/ComM/NM 端口、三条仲裁规则、动作表）
- [x] 修正 `EcuM.c`：按预编译约定 `BswM_Init(NULL_PTR)` 直接选用默认配置，仅清理陈旧 stub 前向声明与 TODO（init 调用点无需修改）
- [x] 扩展 `tests/bsw/services/bswm/`：表达式求值（EQ/NEQ/AND/OR/NOT）、状态迁移、动作计数、EcuM 回调黄金用例（16 + 27 例）
- [x] 回归 `bswm_test` / `bswm_svc_test` 与全量 ctest（104/104 通过；并修复 Os_Cfg 与 CAN 相关测试目标缺失 MCAL include 路径导致的全量构建中断）

## P0-2 LinIf Schedule 引擎

- [x] 重写 `LinIf.c`：通道状态机、调度推进（条目倒计时）、发送路径（Lin_SendFrame）、唤醒/睡眠、调度确认回调（含弱符号上游钩子）
- [x] 扩展 `LinIf.h` / `LinIf_Cfg.h`：通道状态枚举、TX PDU 映射表、回调原型与 MAX_FRAME_LENGTH/MAX_TX_PDUS 宏
- [x] 更新 `LinIf_Lcfg.c`：3 条调度表（Normal 三条目 / DiagRequest）+ 事件触发帧 + TX PDU 映射
- [x] 扩展 `tests/bsw/ecual/linif/`：时序推进、条目循环、调度切换、发送缓冲、唤醒/睡眠、Rx 派发黄金用例（26 例）
- [x] 回归 `linif_test` 与全量 ctest（104/104 通过；并补齐原缺失的 `LinIf_WakeUp/GotoSleep/ScheduleRequest` 定义，消除 LinSM/LinNm 链接隐患）

## P0-3 双 Com 收敛

- [x] 移除根 `CMakeLists.txt:125` 的 `src/bsw/classic/com` include 路径，全量构建验证解析收敛（exit 0，无 `Com.h` 解析失败点）
- [x] 确认无 TU 依赖 classic/com 独有头文件（`Com_Private/Transmit/TxMode/ErrorHandling/DeadlineMon` 仅被该目录内文件引用，且该目录无 CMakeLists、从未参与编译）后删除 `src/bsw/classic/com/` 目录（13 个文件）
- [x] 全量 ctest 回归（104/104 通过）

## P0-4 CAN 栈加厚与 CAN FD

- [x] CanIf：`CanIf_SetTrcvMode` 实化（映射 `CanTrcv_SetOpMode` + 模式保存）、`CanIf_CheckValidation`、`CanIf_GetTxConfirmationState` + 用例（24 例全通过；TX 路径补 `FdFrame = FALSE` 初始化，修复新增字段未初始化读取）
- [x] CanSM：`CanSM_ConfirmPnAvailability` / `CanSM_ClearTrcvWufFlagIndication` 实现与状态机接入（FULLCOM/校验阶段守卫）+ 用例（26 例全通过；附带实化 Init/DeInit/ControllerModeIndication/GetCurrentInternalState）
- [x] CanTp：STmin 计时器（0x00-0x7F ms 及 0xF1-0xF9 编码）、CAN FD 长度校验（合法 DLC 集合）+ 用例（20 例全通过）
- [x] MCAL Can：`FdEnabled`/FD 波特率字段、`FdFrame` 标志与写入路径开关（FD 非法长度/非 FD 控制器 DET）+ 用例（33 例全通过）
- [x] 回归全量 ctest（104/104 通过；集成期修复 `s0_smoke_test` 链接缺 `ecual_cantrcv`/`mcal_dio`/`service_ecum`/`service_comM`/`service_schm` 导致的链接失败）

## P0-5 质量门禁

- [x] `tests/unit` 废弃范围核实与清理：核实"整树废弃"前提不成立（10 个 ctest 用例 #41-50 + 内置 Unity 框架仍在用，`add_subdirectory(unit)` 予以保留）；删除已证死目录 `com/`(13)/`common_stubs/`(3)/`mqtt/`(7)/`ecuc/`(1)/`__pycache__/`(1) 及 `shall-to-test-mapping.json`；清理断链引用（`batch10_coverage.sh` 指向 classic/com 的 COM 段）；其余"构建未引用但被 tools/docs 引用"的历史测试树登记 P2（台账 §6）
- [x] 逐目录核实 `legacy/` 无 include 引用后删除（crypto/csm/dcm/dem/doip/nvm/xcp，75 文件；审计确认无生产 include、GLOB 不覆盖）；连带删除断链测试资产 `tests/dcm/`(4)/`tests/dem/`(3)/`tests/unit/diagnostics/`(2)；更新 services/mcal CMake 中过期的 legacy 注释
- [x] 生成 TODO/Stub 台账 `reports/TODO_STUB_LEDGER.md`（BSW 123 处 = TODO 2 + Stub 121，全库 160 处；含模块归属、热点与 P1/P2 挂单）
- [x] 全量 ctest 回归（104/104 通过）

## 收尾

- [x] 更新 proposal.md 验收勾选与 references 报告交叉引用（S1–S7 全部勾选并附证据；新增"交付物与交叉引用"章节；状态置为 Completed/待归档评审；同步修正 S2 与目标 1 的 EcuM 漂移表述）
- [x] 全量回归报告 + 变更归档评审请求（`reports/REGRESSION_BSW_PRODUCTION_GAPS.md`；`ctest` 104/104 零失败，2026-09-26；归档评审请求见报告 §5）

## 进度统计

- 总任务数: 25
- 已完成: 25
- 完成率: 100%
- 备注 1: P0-1 期间额外修复 `src/bsw/os/CMakeLists.txt` 及 cansm/cantpsyn/cantp 测试目标缺失 `src/bsw/mcal/can/include` 的全量构建中断（此前 `all` 目标从未构建成功）
- 备注 2: P0-2 补齐 `LinIf_WakeUp/GotoSleep/ScheduleRequest` 三个此前"仅声明未实现"的 API（LinSM/LinNm 调用方存在链接隐患），并新增 `src/bsw/mcal/lin/include` 到 linif_test 目标
- 备注 3: P0-4 集成期修复两处跨模块缺口：`CanIf_Transmit` 补 `canPdu.FdFrame = FALSE`（新增字段未初始化读取）；`s0_smoke_test` 链接补 `ecual_cantrcv`/`mcal_dio`/`service_ecum`/`service_comM`/`service_schm`（CanIf_SetTrcvMode 实化后暴露的真实依赖链缺失）
- 备注 4: P0-5 范围修正——原设计"删除 tests/unit 整树"前提经核实不成立（存活 10 个用例与内置 Unity 框架），已按"仅删证死项"重执行；`tests/unit` 仍存量的历史测试树需连同 tools/docs 引用另行立项清理（见 `reports/TODO_STUB_LEDGER.md` §6）
- 备注 5: 收尾阶段对文档漂移做了与实现对齐的修正（依据 `BswM.c` 行为及 `tests/bsw/services/bswm` 断言）：design.md AD1（`BswM_Init(NULL_PTR)`=默认配置予以保留；未匹配组合不报 DET、请求仍锁存）、proposal.md S2/目标 1、spec §2.1/§3.1#4（DET 仅覆盖未初始化与 `Mode > BSWM_MODE_VALUE_MAX`）
