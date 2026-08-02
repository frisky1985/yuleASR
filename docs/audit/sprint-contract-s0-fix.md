# Sprint Contract: S0 致命问题修复（能链接、能启动）

> 基于 2026-08-01 全量检查结果。目标：把"可编译的演示级代码"推进到"能链接、能启动"。

## Scope

### In Scope
1. **FreeRTOS 内核入库**（S0-2）：从官方 FreeRTOS-Kernel V11.1.0 引入 tasks.c/queue.c/timers.c/event_groups.c/stream_buffer.c/list.c + 官方头文件，替换现有 stub 头
2. **OS 任务入口补齐**（S0-3）：实现 8 个 OsTask_*_Entry（Init/10ms/50ms/100ms/Background/ComMainFunctionRx/Tx/Diagnostic）
3. **调度链修复**（S0-4）：
   - Os_InitAlarms 不再用 NULL_PTR 覆盖配置表 Callback
   - SetRelAlarm 在启动流程中真正被调用（报警可触发）
   - Rte_AswScheduler_Start 被调用（ASW 周期任务进入调度）
4. **RTE 数据通路恢复**（S0-5）：
   - Rte_Start() 在 EcuM 中恢复（取消注释）
   - Rte_ConnectPort 建立端口连接
   - Rte_Read_SWC_* / Rte_Write_SWC_* 34 个符号实现（ASW 链接）
5. **ASW 链接修复**（S0-1）：8 个 Rte_Read_SWC_* 未定义符号有提供者

### Out of Scope（后续 sprint）
- MCAL 桩模块（ocu/eth/fee/eep/lin）
- ECUAL 浅实现（ethif/frif/iohwab 不调 MCAL）
- Services 桩（cansm/canm/secoc 等 7 个）
- 功能安全文档造假问题
- 测试体系重建

## Architecture Decision
- **architect-lead**: Hermes（遵循 AUTOSAR 分层，OS 层基于 FreeRTOS 内核 + 自有 Os.c 封装）
- **FreeRTOS 版本**: V11.1.0（官方 LTS 分支，与 AGENTS.md 声称的 V11.x 一致）
- **放置位置**: `third_party/freertos/`（内核源码 + 官方 include），`src/bsw/os/include/` 保留自有 Os.h/Os_Cfg.h，删除同名 stub（task.h/queue.h/timers.h/FreeRTOS.h 等）
- **移植层**: native 用 Posix port（可运行验证）；ARM 用 ARM_CM33（S32K312）
- **FreeRTOSConfig.h**: 重写为完整配置（当前 74 行 stub 不够）
- **Rte_Read_SWC_* 实现**: 在 Rte.c 中实现，经 Rte_Read/Rte_Write 端口转发（连接 Rte_ConnectPort 建立的端口表）

## Testable Behaviors
### 构建
- [ ] B1.1: `cmake -B build-s0 -DCMAKE_BUILD_TYPE=Debug` 配置成功 | Owner: Generator
- [ ] B1.2: `cmake --build build-s0` 全绿（native） | Owner: Generator

### 链接
- [ ] B2.1: `nm build-s0/lib/bsw/os/libOs.a | grep " T xTaskCreate"` 非空 | Owner: Evaluator
- [ ] B2.2: 最终链接无 undefined reference（Rte_Read_SWC_* 全部有定义） | Owner: Evaluator

### 运行
- [ ] B3.1: native 可执行文件运行 ≥3s 不崩溃 | Owner: Evaluator
- [ ] B3.2: 报警回调触发（Os_AlarmCallback → BswM_MainFunction 路径可达） | Owner: Evaluator
- [ ] B3.3: ASW 组件 Init/MainFunction 被 Rte_AswScheduler 调用 | Owner: Evaluator

### 交叉编译
- [ ] B4.1: ARM 交叉编译（arm-none-eabi-gcc）链接通过 | Owner: Evaluator

## Acceptance Criteria
| ID | Criterion | Pass Condition | Fail Condition | Priority | Owner |
|----|-----------|----------------|----------------|----------|-------|
| C1 | FreeRTOS 内核入库 | tasks.c 等在内且被编译 | 内核符号仍缺失 | P0 | Generator |
| C2 | Os_InitAlarms 修复 | Callback 从配置表复制 | 仍 NULL_PTR | P0 | Generator |
| C3 | 任务入口 8 个实现 | 8 个 OsTask_*_Entry 有定义 | 仍 extern 无定义 | P0 | Generator |
| C4 | Rte_Read_SWC_* 实现 | 34 个符号有定义 | 链接 undefined | P0 | Generator |
| C5 | Rte_Start 恢复 | EcuM 调用 Rte_Start() | 仍注释 | P0 | Generator |
| C6 | 调度链完整 | SetRelAlarm 被调用 + ASW 调度启动 | 报警不触发 | P0 | Generator |
| C7 | native 构建+运行 | B1/B2/B3 全过 | 编译或运行失败 | P0 | Evaluator |
| C8 | ARM 链接通过 | B4.1 过 | 链接失败 | P1 | Evaluator |

## Responsibility Matrix
| Criterion | Responsible | Fallback |
|-----------|-------------|----------|
| C1-C6 | Generator (Hermes) | — |
| C7-C8 | Evaluator (Hermes) | Generator 修复迭代 |

## Negotiation Log
| Round | Party | Action | Notes |
|-------|-------|--------|-------|
| 1 | Generator | 提案 | 基于全量检查 5 项 S0 制定 |
| 1 | architect-lead | APPROVE | FreeRTOS V11.1.0 + third_party 放置 + Posix/CM33 双移植合理 |
| 1 | Evaluator | APPROVE | 验收标准全部可客观验证（nm/grep/运行） |
