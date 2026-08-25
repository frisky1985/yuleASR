# Integration Strategy — yuleASR BSW

> 生成: 2026-08-25 (yuleOSH 审计闭环 — SWE.5.BP1)
> 适用项目: /Users/ingeek/workspace/AUTOSAR
> 状态: DRAFT — 按 ASPICE SWE.5 要求定义集成序列与桩/驱动策略

## 1. 集成目标

将已完成的 BSW 单元（MCAL 10 / ECUAL 9 / Service 5 / RTE 1 / ASW 8 / OS 1 / Integration 2 = 35 模块）
按依赖序集成为可运行的软件栈，验证层间接口与数据流。

## 2. 集成序列（自底向上）

| 阶段 | 集成范围 | 前置条件 | 验证方法 |
|:-----|:---------|:---------|:---------|
| I-1 | MCAL 驱动层 (Mcu/Port/Dio/Can/Spi/Gpt/Pwm/Adc/Wdg/Icu/Lin) | 各驱动单测通过 | 单元测试 + 寄存器级 stub |
| I-2 | ECUAL 层 (CanIf/IoHwAb/CanTp/EthIf/MemIf/Fee/Ea/FrIf/LinIf) | MCAL 接口可用 | 接口桩测试 |
| I-3 | Service 层 (Com/PduR/NvM/Dcm/Dem) | ECUAL 可用 | 服务层集成测试 |
| I-4 | RTE + ASW 组件 (8/8) | Service 层可用 | RTE 生成器输出验证 |
| I-5 | OS + Integration (Os/BswM/EcuM) | 以上全部 | 系统启动序列验证 |
| I-6 | 全栈 (S32K312 target) | 交叉编译通过 | QEMU/SIL 运行 |

## 3. 桩/驱动 (Stubs/Drivers) 策略

- **MCAL 硬件抽象**: 测试期使用寄存器 stub（tests/mock/），替换真实外设寄存器映射
- **SIL 环境**: `tests/fixtures/prebuilt/` 预编译 .elf，QEMU M33 运行
- **RTE 通信**: Rte_ComInterface stub 模拟 COM 信号收发
- **驱动**: 各层测试驱动（test drivers）存放于 `tests/unit/` 对应目录

## 4. 集成测试准则

- 每阶段集成后运行对应集成测试（tests/integration/）
- 集成失败即回退至最近可用基线（git tag 或 step-cache）
- 集成测试结果归档至 `.osh/evidence/`（yuleOSH audit evidence）

## 5. 验收标准

- [ ] I-1 ~ I-6 全部阶段有执行记录（passing-run evidence）
- [ ] 集成测试 100% 通过
- [ ] 交叉编译 (arm-none-eabi) 通过
- [ ] QEMU/SIL 运行无致命错误
