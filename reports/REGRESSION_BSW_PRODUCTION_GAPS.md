# 全量回归报告：BSW 量产差距修复（P0 批次）

> **变更 ID**: fix-bsw-production-gaps
> **日期**: 2026-09-26
> **环境**: macOS (darwin, arm64) 本机原生构建 `build-native`
> **门禁口径**: `ctest --test-dir build-native` 零失败

## 1. 结论

- 全量 `ctest`：**104/104 通过，0 失败**（总耗时 6.35 s，2026-09-26）
- 全目标构建成功（`cmake --build build-native` exit 0）
- 验收标准 S1–S7 全部满足（逐项证据见 `openspec/changes/fix-bsw-production-gaps/proposal.md` 验收标准）
- **建议进入变更归档评审**（见 §5）

## 2. 关键套件结果

| 套件（ctest 注册项） | Unity 用例数 | 结果 | 关联 P0 |
|:---------------------|:------------:|:-----|:--------|
| `bswm_test`          | 16 | 通过 | P0-1 |
| `bswm_svc_test`      | 27 | 通过 | P0-1 |
| `linif_test`         | 26 | 通过 | P0-2 |
| `canif_test`         | 24 | 通过 | P0-4 |
| `cansm_test`         | 26 | 通过 | P0-4 |
| `cantp_test`         | 20 | 通过 | P0-4 |
| `mcal_can_test`      | 33 | 通过 | P0-4 |
| 其余 97 个注册项（含 `s0_smoke_test`、`com_test` 及既有回归面） | — | 通过 | P0-3/P0-5 |

说明：ctest 以二进制为单位注册（104 项）；"Unity 用例数"为各套件内部断言用例数，数字取自各阶段回归记录（`tasks.md` 备注）。

## 3. 变更覆盖摘要

- **P0-1 BswM**：Rule/ActionList 引擎（表达式、规则迁移、动作列表）+ EcuM 状态/唤醒回调实现；初始化调用核实保留 `NULL_PTR`（预编译约定 = 默认配置）
- **P0-2 LinIf**：Schedule 引擎（条目时序）、`Lin_SendFrame` 发送路径、唤醒/睡眠/调度请求 API 补齐
- **P0-3 双 Com 收敛**：移除 classic include 路径并删除 `src/bsw/classic/com/`（13 文件）
- **P0-4 CAN 栈**：CanIf 收发器/校验面、CanSM PN、CanTp STmin + FD 长度校验、MCAL Can FD 配置字段
- **P0-5 质量门禁**：`legacy/` 7 目录（75 文件）+ `tests/unit` 证死子集 + 断链资产清理；TODO/Stub 台账 `reports/TODO_STUB_LEDGER.md`

## 4. 明确不覆盖 / 遗留

- S32K312 真实硬件 HIL 验证、物理层 FD 时序标定（Track C）
- FreeRTOS 安全认证版替换评估（产品级决策）
- P1/P2 挂单项（`reports/TODO_STUB_LEDGER.md`：Reg_Macros/Dma/HSM stubs、`CanIf_CancelTransmit` 占位、时间戳类 TODO、跨层重复模块收敛等）
- `tests/unit` 存量历史测试树（被 tools/docs 引用）需另行立项清理（台账 §6）

## 5. 变更归档评审请求

请求对 `openspec/changes/fix-bsw-production-gaps/` 组织归档评审。评审要点：

1. **验收证据**：proposal.md S1–S7 勾选与证据链（tasks.md 逐项备注）
2. **决策修正记录**：design.md AD1（EcuM 初始化与未匹配组合语义的漂移修正）、AD7（P0-5 范围修正——"tests/unit 整树废弃"前提不成立）
3. **回归基线**：本报告 §1/§2（104/104，零失败）

归档前置条件均已满足：tasks.md 25/25 完成、全量 `ctest` 零失败、无未决 P0 项。评审通过后将变更移入 `openspec/changes/archive/`。
