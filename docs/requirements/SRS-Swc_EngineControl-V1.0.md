# 🚗 AUTOSAR 软件需求规范 (SRS) — Swc_EngineControl

> 模板来源: `skills/prd-writing/templates/autosar_req_template.md` | 试跑验证 2026-08-14
> 实例化: 依据 yuleASR `src/application/engine_control/` 真实代码填充

## 1. 文档元数据

| 字段 | 内容 |
| :--- | :--- |
| **文档ID** | SRS-ECU-ENGCTRL-V1.0 |
| **项目名称** | YuleTech AutoSAR BSW（yuleASR） |
| **功能域** | 动力域（Engine Control） |
| **安全等级** | ASIL-B（依据 ISO 26262，扭矩/点火相关需功能安全） |
| **最后更新** | 2026-08-14 |
| **作者/审核** | 小明（模板实例化）/ [审核人] |

## 2. 系统级非功能需求 (NFR)

> 定义 ECU 整体的资源、性能与环境约束，作为所有 SWC 开发的顶层边界。

### 2.1 资源约束

- **CPU 负载率**：峰值 `< 70%`（最坏执行时间 WCET 统计）
- **RAM 占用**：静态 `< 15KB`，动态堆栈 `< 5KB`
- **Flash 占用**：代码段 `< 256KB`，数据段 `< 32KB`

### 2.2 实时性要求

- **主控制循环周期**：`10ms`（硬实时，Runnable `Swc_EngineControl_10ms`）
- **慢速控制循环周期**：`100ms`（Runnable `Swc_EngineControl_100ms`，由 MainFunction 每 10 tick 调度）
- **最大响应延迟**：从信号接收到动作输出 `< 2ms`

### 2.3 环境与可靠性

- **工作温度**：`-40℃ ~ +120℃`（代码常量 `ENG_MIN_OPERATING_TEMP=-40` / `ENG_MAX_OPERATING_TEMP=120`）
- **过温保护阈值**：`110℃`（`ENG_OVERHEAT_THRESHOLD`），燃油切断 `125℃`（`ENG_FUEL_CUTOFF_TEMP`）
- **看门狗策略**：外部看门狗刷新周期 `100ms`，超时触发 MCU 复位

## 3. 软件组件 (SWC) 详细需求

> 与 ARXML 中 PortInterface 和 Runnable 严格对应（实现见 `Swc_EngineControl.h/.c`，RTE 接口 `Rte_Swc.h`）。

### 3.1 组件概览

- **组件名称**：`Swc_EngineControl`
- **描述**：发动机控制与管理工作（状态机、喷油/点火计算、模式切换）
- **执行模式**：`TimingEvent`（10ms 快速环 + 100ms 慢速环 + 状态机）
- **安全机制**：输入信号范围检查、过温保护（燃油切断）、降级模式（LIMP_HOME）

### 3.2 接口定义 (Ports & Interfaces)

#### 输入端口 (RPort)

| 端口名 | 数据类型 | 单位 | 范围 | 更新周期 | 来源 SWC/BSW |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `ThrottlePosition` | `uint16` | % | 0-100 | 10ms | Rte_Read_ThrottlePosition |
| `CoolantTemperature` | `sint16` | ℃ | -40~125 | 10ms | Rte_Read_CoolantTemperature |
| `VehicleSpeed` | `uint16` | km/h | 0-300 | 10ms | Rte_Read_VehicleSpeed |

#### 输出端口 (PPort)

| 端口名 | 数据类型 | 单位 | 范围 | 更新周期 | 去向 SWC/BSW |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `EngineState` | `enum Swc_EngineStateType` | - | OFF/CRANKING/RUNNING/STOPPING/FAULT | 100ms | Rte_Write_EngineState |
| `EngineParameters` | `struct Swc_EngineParametersType` | - | speed/load/temp/throttle/inj/ign | 100ms | Rte_Write_EngineParameters |
| `EngineControlOutput` | `struct Swc_EngineControlOutputType` | - | fuelPulse/ignTiming/idleSpeed/cutoff | 10ms | Rte_Write_EngineControlOutput |
| `EngineMode` | `enum Swc_EngineControlModeType` | - | NORMAL/ECO/SPORT/LIMP_HOME | 事件 | Rte_Switch_EngineMode |

### 3.3 内部逻辑与算法

1. **初始化阶段 (`Swc_EngineControl_Init`)**：
   - 状态置 `ENGINE_STATE_OFF`，输出默认 `0`，PIM 参数复位（`pimFuelTrim=100`，`pimIgnitionOffset=0`）。
2. **状态机 (`Swc_EngineControl_StateMachine`)**：
   - 维护 `OFF → CRANKING → RUNNING → STOPPING → OFF` 迁移；故障时进入 `FAULT`，模式切换 `Rte_Switch_EngineMode`。
3. **运行阶段 (`Swc_EngineControl_10ms` 快速环)**：
   - **步骤 1 (信号采集)**：读取 ThrottlePosition / CoolantTemperature / VehicleSpeed。
   - **步骤 2 (喷油计算)**：`fuelTime = 2000 + load×100`，转速修正 `(speed-900)×5`，温度修正（<20℃ 冷启动加浓 `(20-temp)×50`；>90℃ 高温减油 `(temp-90)×10`），PIM trim 百分比，限幅 `500~15000µs`（`FUEL_INJ_MIN/MAX_TIME_US`）。
   - **步骤 3 (点火计算)**：基值 `10°`，转速修正 `/100`，负载修正 `/10`，温度修正（<0℃ 或 >100℃ 推迟 5°/3°），PIM offset，限幅 `0~45°`（`IGNITION_ADVANCE_MIN/MAX_DEG`）。
   - **步骤 4 (输出更新)**：写 `EngineControlOutput`（含 fuelCutoff/ignitionCutoff 标志）。
4. **慢速环 (`Swc_EngineControl_100ms`)**：发布 EngineState / EngineParameters。
5. **异常处理**：
   - 水温 ≥110℃：进入过热保护；≥125℃：触发燃油切断。
   - 怠速范围：800~1200 rpm（`ENG_MIN/MAX_IDLE_SPEED`），目标怠速由输出 idleSpeedTarget 控制。
   - 降级模式：模式切换至 `LIMP_HOME` 时受限输出。

## 4. 基础软件 (BSW) 配置需求

> 指导 BSW 模块配置，确保底层通信、诊断、网络管理与应用层匹配。

### 4.1 CAN 通信栈 (CanIf / PduR / Com)

- **波特率**：`500kbps`（CAN FD 可选 2Mbps 数据段）
- **通信矩阵**：`config/input/arxml/example.arxml`
- **PDU 路由规则**：
  - 接收：CAN 报文 → `ThrottlePosition` / `CoolantTemperature` / `VehicleSpeed` 信号
  - 发送：`EngineControlOutput` / `EngineState` / `EngineParameters` → CAN 报文
- **E2E 保护**：对关键信号（EngineControlOutput）启用 E2E Profile 1 (CRC + Counter)

### 4.2 网络管理 (Nm)

- **模式**：`AutoSAR NM`
- **节点类型**：`Full Node`
- **超时参数**：`NmTimeoutTime: 2000ms` / `NmWaitBusSleepTime: 5000ms`
- **行为**：连续 2s 未收到 NM 报文则判定网络休眠，关闭非必要外设电源。

### 4.3 诊断服务 (DCM / DEM)

- **支持 UDS 服务**：
  - `0x10` (Session Control): Default, Extended, Programming
  - `0x22` (ReadDataByIdentifier): DID `0xF190` (VIN), `0xF191` (SW Version), 发动机参数 DID
  - `0x27` (Security Access): Level 1 (Seed-Key)
  - `0x3E` (Tester Present)
- **DTC 策略**：
  - 过温 DTC：防抖时间 `100ms`，老化计数器 `40` 个驾驶循环后清除。
  - 输入信号超时/范围失效 DTC：进入降级模式并置位故障状态。

## 5. 测试与验收标准 (Verification Criteria)

> 将需求转化为可执行的测试用例，确保需求可测。

### 5.1 单元测试 (MIL/SIL)

- [ ] **正常工况**：标准转速/负载/温度输入，喷油时间与点火提前角符合计算公式预期（含 PIM 修正），误差 `< 1%`。
- [ ] **边界测试**：`FUEL_INJ` 限幅（500/15000µs）、`IGNITION_ADVANCE` 限幅（0/45°）、怠速范围（800/1200 rpm）不越界。
- [ ] **温度修正**：<20℃ 冷启动加浓、>90℃ 高温减油、>110℃ 过热保护、≥125℃ 燃油切断。
- [ ] **异常注入**：输入信号超时，验证进入降级模式并输出安全默认值。

### 5.2 集成测试 (HIL)

- [ ] **通信测试**：EngineControlOutput 报文发送周期误差 `< ±1ms`，无丢帧。
- [ ] **网络管理**：断网验证 ECU 在 2s 后进入 BusSleep 模式。
- [ ] **诊断测试**：非法 SID / 错误密钥验证 NRC 返回；DID 读取参数正确。

### 5.3 性能测试

- [ ] **WCET 分析**：`Swc_EngineControl_10ms` 最坏执行时间 `< 2ms`。
- [ ] **压力测试**：连续运行 72 小时，内存无泄漏，CPU 负载稳定。

## 💡 填充说明

- 本 SRS 由 `prd-writing` 技能 AUTOSAR 模板实例化，全部硬指标/端口/Runnable 取自真实代码：
  - 头文件: `src/application/engine_control/include/Swc_EngineControl.h`
  - 实现: `src/application/engine_control/src/Swc_EngineControl.c`
  - 常量: `FUEL_INJ_*` / `IGNITION_ADVANCE_*` / `ENG_*` 宏
- 待确认项 `[ ]`：安全等级 ASIL-B 为建议值，需功能安全团队确认；DTC 编号（U0100 等）需诊断规范确认。
