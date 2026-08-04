# CONTEXT.md — yuleASR 领域术语表

> 本文档是项目的**统一语言**（Ubiquitous Language）。所有 agent 在命名代码、测试、接口、评审时必须使用本表术语。
> 本文档**只含术语定义，不含实现细节**。实现决策见 `docs/adr/`。
> 更新规则：术语在会话中结晶后立即更新（domain-modeling 纪律），不批量攒。

## 核心概念

- **yuleASR** — 轻量级 AUTOSAR Classic Platform BSW 参考实现，覆盖 MCAL/ECUAL/服务层的嵌入式汽车电子基础软件栈。
- **BSW** — 基础软件层（AUTOSAR Classic），位于 MCAL 之上、RTE 之下，提供 ECU 级服务（通信/存储/诊断）。
- **MCAL** — 微控制器抽象层，直接驱动芯片外设（Can/Spi/Gpt/Adc/Wdg 等），BSW 最底层。
- **ECUAL** — ECU 抽象层，封装 MCAL 之上的接口（CanIf/IoHwAb/CanTp/MemIf），提供 IO 与外设抽象。
- **RTE** — 运行时环境，连接 ASW 组件与 BSW 服务的通信中间件（Rte_ComInterface/Rte_NvMInterface/Rte_Scheduler）。
- **ASW** — 应用软件层（EngineControl/VehicleDynamics/DiagnosticManager 等 8 组件），承载整车功能逻辑。
- **OS** — 操作系统层，基于 FreeRTOS（V10.6.x/V11.x），提供任务/事件/资源/告警管理。
- **Module（模块）** — BSW/ASW 中一个可独立编译、有明确接口的功能单元（如 CanIf、NvM、Dem）。
- **E2E** — End-to-End Protection，通信数据完整性保护（CRC/DataID/Counter），保障安全相关信号。
- **SecOC** — Secure Onboard Communication，车载安全通信（MAC 认证），与 E2E 互补。
- **BswM** — 基础软件模式管理器，协调 ECU 运行模式（STARTUP/RUN/SHUTDOWN）。
- **EcuM** — ECU 状态管理器，负责 ECU 上下电状态机。
- **MISRA C:2023** — C 代码合规标准，yuleASR 内置规则集与偏差管理。

## Agent 角色（三人小队）

- **小明** — 项目经理/编排器。需求入口、流程编排、最终评审（业务价值维度）、争议仲裁。
- **小克** — 架构师/开发者/测试者。架构设计、代码开发、自测、技术债跟踪、根因分析。
- **小马** — 质量架构师/评审者。Spec 契约层、验收矩阵、前置架构评审、正式评审、变更影响分析、质量评分。

## 常用缩写

- **HITL** — Human In The Loop，需要人参与的流程。
- **P0/P1/P2** — 缺陷/评审发现的分级：P0 阻断、P1 重要、P2 建议。
- **ADR** — Architecture Decision Record，架构决策记录（`docs/adr/`）。
- **S2S** — Server-to-Server，服务端间通信（相对 S2C 车云/端云）。
- **SIL/HIL** — Software-in-the-Loop / Hardware-in-the-Loop，测试层级。
- **QEMU** — 开源硬件模拟器，用于无板验证（yuleASR 用 mps2-an521 Cortex-M33 模型）。
