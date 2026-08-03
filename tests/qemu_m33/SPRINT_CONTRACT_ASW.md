# Sprint Contract: Os 层 + ASW 组件 @ QEMU M33

## Scope
- What: 在 QEMU mps2-an521 上验证 yuleASR Os 层驱动完整 ASW 组件栈
- In Scope:
  - 生产 Os.c + Os_TaskEntries.c + 测试版 Os_Cfg（8 任务匹配生产表）
  - Rte.c + Rte_Scheduler.c + Rte_AswScheduler.c + 8 个 ASW 组件
  - 5 个 BSW MainFunction 桩（BswM/Com/Dcm/NvM/Dem）
- Out of Scope:
  - 真实 BSW 栈（CanIf/Com/Dcm 等模块本体）
  - S32K312 硬件

## Architecture Decision
- 复用上轮验证的 QEMU 基建（NTZ port, UART, 编译脚本）
- Os_Cfg 用测试版（alarm 回调指向桩），任务表匹配生产 Os_TaskEntries
- ASW 组件通过 Rte_AswScheduler_Start 初始化 + Rte_Scheduler_MainFunction 周期调度

## Acceptance Criteria
| ID | Criterion | Pass Condition |
|----|-----------|----------------|
| A1 | 编译链接 | qemu_m33_asw.elf 生成无错误 |
| A2 | Os 层启动 | StartOS 后 Init 任务跑 Rte_Init/Rte_Start/Rte_AswScheduler_Start |
| A3 | ASW 组件初始化 | 8 组件 Init 全部执行（输出 SWC 状态） |
| A4 | 周期调度 | 10ms 任务驱动 Rte_Scheduler_MainFunction，ASW MainFunction 按周期执行 |
| A5 | 完整 PASS | 输出 QEMU_M33_ASW_PASS 标记 |
| A6 | 提交推送 | git 提交 + push v1.3.0 |

## Negotiation Log
| Round | Party | Action | Notes |
|-------|-------|--------|-------|
| 1 | Generator | 提出方案 B | 生产 Os_TaskEntries + 测试 Os_Cfg + BSW 桩 |
