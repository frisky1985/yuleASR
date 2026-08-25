# CONTEXT — yuleASR BSW

> 统一语言术语表（domain-modeling，纯术语，不含实现细节）
> 生成: 2026-08-25 | 维护: YuleTech 基础软件平台团队

## 领域概念

| 术语 | 定义 |
|:-----|:-----|
| **BSW** | 基础软件层（Basic Software），AUTOSAR 分层架构中位于 RTE 之下的软件集合 |
| **MCAL** | 微控制器抽象层，直接访问 MCU 外设寄存器的驱动集合 |
| **ECUAL** | ECU 抽象层，对 MCAL 驱动做统一接口抽象 |
| **Service Layer** | 服务层，提供通信/存储/诊断等服务（Com、PduR、NvM、Dcm、Dem） |
| **RTE** | 运行时环境，连接 ASW 组件与应用无关的 BSW 服务 |
| **ASW** | 应用软件组件（EngineControl、VehicleDynamics 等 8 个组件） |
| **OS** | 操作系统层，基于 FreeRTOS 的任务/事件/资源管理 |
| **Integration Layer** | 集成层（BswM 模式管理器、EcuM ECU 状态管理器） |
| **S32K312** | 目标 MCU（ARM Cortex-M33），yuleASR 的默认硬件平台 |
| **SHALL** | 需求语句中的强制性要求动词（AUTOSAR 规格惯例） |
| **SIL** | 软件在环（Software-in-the-Loop），QEMU 仿真运行 |
| **HIL** | 硬件在环（Hardware-in-the-Loop），真实目标板运行 |

## 模块命名约定

| 模式 | 含义 |
|:-----|:-----|
| `Mcu` / `Port` / `Dio` | MCAL 驱动模块（AUTOSAR 命名） |
| `CanIf` / `EthIf` / `MemIf` | ECUAL 接口模块（If 后缀 = Interface） |
| `Com` / `PduR` / `NvM` | 服务层模块（PduR = PDU Router） |
| `Rte` | RTE 运行时（Rte_ 前缀 API） |
| `BswM` / `EcuM` | 集成层管理器（M 后缀 = Manager） |

## 关键概念关系

- 需求 → 模块：每条需求（REQ/SWR 编号）追溯到一个或多个 BSW 模块
- 模块 → 测试：每个模块有对应单元测试（tests/unit/ 镜像目录结构）
- 分层依赖：ASW → RTE → Service → ECUAL → MCAL（单向，禁止循环）
