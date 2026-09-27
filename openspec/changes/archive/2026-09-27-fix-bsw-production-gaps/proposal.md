# 变更提案：BSW 量产差距修复（P0 批次）

> **变更 ID**: fix-bsw-production-gaps
> **状态**: Completed (2026-09-26)，待归档评审
> **优先级**: P0
> **负责人**: Track-B（BSW 量产收敛）
> **创建日期**: 2026-09-26
> **目标版本**: v1.8.0
> **估计工时**: 80h (10d)

## 背景

《BSW 深度对标与量产差距分析（2026-09-26）》（`.qoder/skills/autosar-benchmark-compare/references/bsw-depth-analysis-2026-09-26.md`）在对 EasyXMen 开源基线做模块级深度扫描后，识别出 5 项量产前必须处置的 P0 差距。本变更按该报告建议排期，逐项落地修复。

| # | P0 项 | 现状（2026-09-26） | 量产风险 |
|:--|:------|:-------------------|:---------|
| P0-1 | BswM 决策与定档 | 292 行最小实现，无 Rule/ActionList 引擎；EcuM 联动接口 `BswM_EcuM_CurrentState` 有调用无实现；`EcuM` 以 `BswM_Init(NULL_PTR)` 初始化（DET 开启时被拒） | 通信控制/唤醒睡眠仲裁无法实现，SOP 集成阻断 |
| P0-2 | LinIf 加厚 | 361 行；`LinIf_Transmit` 为空壳（void 参数）；schedule 引擎为 `tickCount % DelayMs` 占位；`LinIf_WakeUp/GotoSleep/ScheduleRequest` 只有声明无定义 | 窗口控制器 LIN 子节点不可用，链接失败风险 |
| P0-3 | 双 Com 收敛 | `classic/com`（13 文件）进全局 include 路径但从不编译；头文件解析可能命中 classic、实现来自 `services/com`，形成配置错配 | 编译期/运行期行为不一致，排查成本极高 |
| P0-4 | CAN 栈加厚与 CAN FD | `CanIf_SetTrcvMode` 为空壳；CanSM 无 PN API；CanTp 无 STmin 计时器/FD 长度校验；Can 驱动无 FD 配置字段，而 `cantp_config.json` 声明 `use_can_fd: true` | 网关/网络管理场景不可用；FD 宣称与实现不符 |
| P0-5 | 质量门禁 | 123 处 TODO/stub/placeholder（50 文件）；7 个 `legacy/` 目录残留；废弃 `tests/unit` 树仍在聚合构建 | 缺陷可见性差，遗留代码误解风险 |

## 目标

1. **P0-1**: 实现 AUTOSAR 4.4 对齐的 Rule/ActionList 引擎（模式请求端口、表达式求值、动作列表、规则状态迁移），补齐 EcuM/BswM 双向联动接口；EcuM 初始化调用经核实按预编译约定保留（`BswM_Init(NULL_PTR)` = 选用默认配置）
2. **P0-2**: 实现真实 LinIf：Schedule 引擎（按通道、按条目时序、随动计数）、`LinIf_Transmit` 到 MCAL `Lin_SendFrame` 的发送路径、唤醒/睡眠/状态 API、调度请求确认回调
3. **P0-3**: `services/com` 定为唯一主线；移除 `classic/com` 的 include 路径并删除目录，锁死头文件解析
4. **P0-4**: CanIf 收发器唤醒/校验面补齐；CanSM PN API；CanTp STmin 计时器 + CAN FD 长度校验；Can 驱动 FD 配置字段与 FD DLC 支持
5. **P0-5**: 废弃测试树与 `legacy/` 目录清理；TODO/stub 逐项挂单或清除；覆盖率门禁接入 CI 现状核实

## 范围

### 包含内容
- `src/bsw/services/bswm/` — Rule/ActionList 引擎重写 + Lcfg 接线
- `src/bsw/services/ecum/` — BswM 初始化与回调联动修正
- `src/bsw/ecual/linif/` — Schedule 引擎、发送路径、唤醒/睡眠
- `src/bsw/classic/com/` — 删除（收敛到 services/com）
- `CMakeLists.txt` — 移除 classic/com include 路径
- `src/bsw/ecual/canif/`、`src/bsw/services/cansm/`、`src/bsw/ecual/cantp/`、`src/bsw/mcal/can/` — P0-4 面
- `tests/bsw/services/bswm/`、`tests/bsw/ecual/linif/`、`tests/bsw/ecual/{canif,cantp}/`、`tests/bsw/services/cansm/` — 测试扩展
- `tests/unit/` 废弃树清理、`src/bsw/**/legacy/` 清理

### 不包含内容
- Dem/Crypto/Csm/ComM 中等差距（P1，另变更处理）
- 跨层重复模块收敛（fee/memif/fim/ipdum 等 6+ 对，P2）
- MISRA Rule 11.4/13.5 存量违规清零（P2 批次）
- FreeRTOS 安全认证版替换评估（需产品级决策）
- S32K312 真实硬件 HIL 验证（Track C）

## 验收标准

- [x] S1: BswM 规则引擎在模式请求 → 规则求值 → 动作列表执行链路上有黄金用例（含表达式 AND/OR 与规则状态迁移）
  - 证据：`bswm_test` 16 例 + `bswm_svc_test` 27 例全通过；覆盖 EQ/NEQ/AND/OR/NOT 求值、规则 FALSE→TRUE / TRUE→FALSE 迁移动作计数、`BswM_EcuM_CurrentState(ECUM_STATE_RUN)` 触发端口请求并参与求值
- [x] S2: `BswM_EcuM_CurrentState/CurrentWakeup` 有实现并被 EcuM 调用链验证；EcuM 初始化调用经核实按预编译约定保留（`BswM_Init(NULL_PTR)` = 选用默认配置，非 DET 错误），由 `test_BswM_Init_NullPtr_ShouldSelectDefaultConfig` 固化
  - 证据：`EcuM.c` STARTUP/RUN/SLEEP/SHUTDOWN 与 Wakeup 校验回调调用点（L176/301/350/537/677/735）+ `test_BswM_DefaultConfig_ValidatedWakeup_ShouldRequestWakeupMode`
- [x] S3: LinIf 调度表按条目时序推进有黄金用例；`LinIf_Transmit` 调用 `Lin_SendFrame`；`WakeUp/GotoSleep/ScheduleRequest` 可链接
  - 证据：`linif_test` 26 例全通过（时序推进/条目循环/调度切换/发送缓冲/唤醒/睡眠/Rx 派发）；三个此前仅声明的 API 已补齐定义（消除 LinSM/LinNm 链接隐患）
- [x] S4: 全仓 `#include "Com.h"` 在移除 classic include 路径后构建通过，`ctest` 无回归；`classic/com` 目录删除
  - 证据：移除根 `CMakeLists.txt` include 路径后全量构建 exit 0、无解析失败点；`src/bsw/classic/com/`（13 文件）已删除；`com_test` 与全量 `ctest` 无回归
- [x] S5: CanIf 收发器唤醒/校验用例通过；CanSM PN API 有实现与用例；CanTp STmin 计时器用例通过；CAN FD 长度校验用例通过
  - 证据：`canif_test` 24 / `cansm_test` 26 / `cantp_test` 20 / `mcal_can_test` 33 例全通过
- [x] S6: 废弃 `tests/unit` 与 `legacy/` 目录移除后全量回归通过
  - 证据：`legacy/` 7 目录 75 文件、`tests/unit` 证死子集（com/common_stubs/mqtt/ecuc/`__pycache__`）、断链资产（tests/dcm/tests/dem/tests/unit/diagnostics）删除后 104/104；范围修正见 tasks.md 备注 4 与台账 §6（"整树废弃"前提不成立）
- [x] S7: 全量 `ctest` 零失败
  - 证据：`ctest --test-dir build-native` 104/104 零失败（2026-09-26）

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| 移除 classic/com include 路径后某些 TU 编译失败 | 中 | 高 | 先移除路径→全量构建→再删目录；任何失败点即时定位 |
| BswM 重写破坏既有 12+ 用例与 S0 smoke | 中 | 高 | 保留既有 API 语义（GetCurrentMode/GetRequestedMode），扩展而非替换；逐用例回归 |
| LinIf 调度时序用例依赖 tick 语义 | 中 | 中 | 以显式 tick 推进（MainFunction 调用次数）驱动黄金断言 |
| 子代理并行改动构建文件冲突 | 低 | 中 | P0 项按序单线程推进，构建文件改动集中于收尾 |

## 交付物与交叉引用

- 设计与决策记录：`openspec/changes/fix-bsw-production-gaps/design.md`（AD1–AD7）
- 任务台账与逐项证据：`openspec/changes/fix-bsw-production-gaps/tasks.md`
- 规范与黄金用例口径：`openspec/changes/fix-bsw-production-gaps/specs/bsw_production_gaps_spec.md`
- TODO/Stub 台账：`reports/TODO_STUB_LEDGER.md`（BSW 123 处 = TODO 2 + Stub 121；含模块归属与 P1/P2 挂单）
- 回归口径：`ctest --test-dir build-native` — 104/104 零失败（2026-09-26）
- 基线分析：`.qoder/skills/autosar-benchmark-compare/references/bsw-depth-analysis-2026-09-26.md`
