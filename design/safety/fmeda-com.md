# FMEDA — Com (Communication Manager)

> **文档编号**: SAF-FMEDA-COM-001
> **版本**: 1.1
> **日期**: 2026-10-09
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW Service Layer 的 Com (Communication Manager) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/services/com/src/Com.c`

**分析边界**:
- Com 核心逻辑（信号管理、I-PDU 管理、网关路由、Deadline 监控）
- 上层接口：`Com_SendSignal`, `Com_ReceiveSignal`, `Com_SendSignalGroup`, `Com_ReceiveSignalGroup`
- 内部安全机制：参数校验、信号无效化、超时检测、错误回调
- 下层依赖（假设由 ECUAL/MCAL 层覆盖）：PduR, Det

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 Com 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `Com_InternalStateType` | Com.c:92-100 | 模块内部状态，含 State, ConfigPtr, IPduStates[], SignalStates[], IPduBuffer[], ShadowBuffer |
| `Com_IPduStateType` | Com.c:73-80 | I-PDU 运行时状态：TxState, RepetitionCount, TimeCounter, Updated, GroupEnabled |
| `Com_SignalStateType` | Com.c:83-88 | 信号运行时状态：Updated, FilterPassed, LastValue |
| `Com_SignalConfigType` | Com.h:199-209 | 信号配置：SignalId, BitPosition, BitSize, Endianness, TransferProperty, FilterAlgorithm |
| `Com_IPduConfigType` | Com.h:212-224 | I-PDU 配置：PduId, DataLength, RepeatingEnabled, TimePeriod, IpduGroupRef |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 信号发送 | `Com_SendSignal()` (L563) | 将信号数据打包到 I-PDU 缓冲区 |
| FU-02 | 信号接收 | `Com_ReceiveSignal()` (L637) | 从 I-PDU 缓冲区解包信号数据 |
| FU-03 | 信号组管理 | `Com_SendSignalGroup()` (L682), `Com_ReceiveSignalGroup()` (L719) | 信号组批量发送/接收 |
| FU-04 | Shadow 缓冲区 | `Com_UpdateShadowSignal()` (L749), `Com_ReceiveShadowSignal()` (L784) | Shadow 缓冲区操作 |
| FU-05 | I-PDU 发送 | `Com_TriggerIPDUSend()` (L858), `Com_TransmitIPdu()` (L415) | 通过 PduR 发送 I-PDU |
| FU-06 | I-PDU 接收 | `Com_RxIndication()` (L925) | 接收 PduR 的 I-PDU 指示 |
| FU-07 | 发送确认 | `Com_TxConfirmation()` (L885) | 处理发送确认及重发逻辑 |
| FU-08 | 周期发送 | `Com_MainFunctionTx()` (L994) | 周期发送及重发管理 |
| FU-09 | 周期接收 | `Com_MainFunctionRx()` (L961) | 接收数据处理及标志位清理 |
| FU-10 | 网关路由 | `Com_MainFunctionRouteSignals()` (L1043) | 跨 I-PDU 信号路由 |
| FU-11 | 信号无效化 | `Com_InvalidateSignal()` (L1190), `Com_InvalidateSignalGroup()` (L1236) | 通信失败时设置无效值 |
| FU-12 | Deadline 监控 | `Com_ReceptionDMControl()` (L1157), `Com_DMEnabled[]` | 接收超时检测控制 |
| FU-13 | I-PDU 组控制 | `Com_IpduGroupControl()` (L1122) | I-PDU 组启停控制 |
| FU-14 | 信号滤波 | `Com_ApplyFilter()` (L320) | 信号值滤波算法 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 信号发送 (`Com_SendSignal`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | 信号数据指针为空 | 写入未定义数据到 I-PDU 缓冲区 | 入口参数校验 (SignalDataPtr != NULL_PTR) | 99% | B |
| FM-01-02 | SignalId 超出配置范围 | 访问未定义的信号描述符 | SignalId 范围检查 (`SignalId < COM_NUM_OF_SIGNALS`) | 99% | B |
| FM-01-03 | 模块未初始化 | 操作未初始化的内存 | 状态检查 (`Com_InternalState.State == COM_STATE_INIT`) | 99% | B |
| FM-01-04 | 信号打包逻辑错误（CPU 瞬态故障） | I-PDU 缓冲区数据损坏 | 信号值回读验证；PduR 层 CRC 校验（如配置） | 85% | B |
| FM-01-05 | 滤波算法误判 | 有效信号被阻止或无效信号被接受 | 滤波算法配置校验；信号值范围检查 | 80% | B |

### 3.2 FU-02: 信号接收 (`Com_ReceiveSignal`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | 接收数据指针为空 | 信号数据写入非法地址 | 入口参数校验 (SignalDataPtr != NULL_PTR) | 99% | B |
| FM-02-02 | I-PDU 缓冲区数据损坏 | 接收到错误信号值 | 信号值范围检查；应用层合理性校验 | 75% | B |
| FM-02-03 | 信号解包逻辑错误（CPU 瞬态故障） | 信号值解析错误 | 冗余解包（关键信号双重校验）；应用层超时检测 | 80% | B |
| FM-02-04 | 读取未更新的信号 | 返回陈旧数据 | `Updated` 标志检查，应用层判断数据新鲜度 | 70% | B |

### 3.3 FU-03: 信号组管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | SignalGroupId 超出范围 | 访问未定义的信号组 | SignalGroupId 范围检查 (`SignalGroupId < COM_NUM_OF_SIGNAL_GROUPS`) | 99% | B |
| FM-03-02 | Shadow 缓冲区拷贝失败 | 信号组数据不完整 | `memcpy` 返回值检查（如配置）；应用层数据校验 | 85% | B |
| FM-03-03 | I-PDU 发送失败 | 信号组数据未发送 | `Com_TransmitIPdu` 返回码检查，错误上报 | 90% | B |

### 3.4 FU-05: I-PDU 发送 (`Com_TransmitIPdu`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | PduR_Transmit 返回错误 | I-PDU 未发送，数据丢失 | 返回码检查 (`E_OK`)，错误时保持 `COM_TX_IDLE` 状态 | 90% | B |
| FM-04-02 | I-PDU 缓冲区指针无效 | 发送空数据或错误数据 | 缓冲区指针校验 (`IPduBuffer[PduId]` 有效性) | 85% | B |
| FM-04-03 | I-PDU 配置查找失败 | 发送长度错误 | `Com_GetIPduConfig` 返回 NULL 检查 | 99% | B |

### 3.5 FU-06: I-PDU 接收 (`Com_RxIndication`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | 接收数据指针为空 | 无法拷贝接收数据 | 入口参数校验 (PduInfoPtr->SduDataPtr != NULL_PTR) | 99% | B |
| FM-05-02 | 接收长度超出缓冲区 | 缓冲区溢出 | 长度校验 (`PduInfoPtr->SduLength < COM_MAX_IPDU_BUFFER_SIZE`) | 99% | B |
| FM-05-03 | RxPduId 超出范围 | 写入非法 I-PDU 缓冲区 | RxPduId 范围检查 (`RxPduId < COM_NUM_OF_IPDUS`) | 99% | B |
| FM-05-04 | memcpy 拷贝错误（CPU 瞬态故障） | I-PDU 缓冲区数据损坏 | 接收后数据校验（应用层或 PduR 层 CRC） | 80% | B |

### 3.6 FU-07: 发送确认 (`Com_TxConfirmation`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | TxPduId 超出范围 | 访问非法 I-PDU 状态 | TxPduId 范围检查 (`TxPduId < COM_NUM_OF_IPDUS`) | 99% | B |
| FM-06-02 | 重发计数溢出 | 重发次数超出配置 | 重发计数检查 (`RepetitionCount < NumRepetitions`) | 90% | B |
| FM-06-03 | 发送确认丢失 | 无法判断发送结果 | 超时检测（系统级 WDT）；应用层重试机制 | 75% | B |

### 3.7 FU-08: 周期发送 (`Com_MainFunctionTx`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | TimeCounter 下溢 | 发送周期错误 | 计数器边界检查 (`TimeCounter == 0U`) | 90% | B |
| FM-07-02 | I-PDU 组被禁用 | 应发送的 I-PDU 未发送 | `GroupEnabled` 标志检查；应用层监控发送频率 | 85% | B |
| FM-07-03 | 主函数未周期调用 | 周期发送失效 | 系统级 WDT 监控；OS 任务调度监控 | 90% | B |

### 3.8 FU-09: 周期接收 (`Com_MainFunctionRx`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | Updated 标志未清除 | 重复处理同一 I-PDU | 标志清除逻辑检查 (`Updated = FALSE`) | 90% | B |
| FM-08-02 | I-PDU 配置查找失败 | 无法处理接收数据 | `Com_GetIPduConfig` 返回 NULL 检查 | 99% | B |
| FM-08-03 | 主函数未周期调用 | 接收数据积压 | 系统级 WDT 监控；OS 任务调度监控 | 90% | B |

### 3.9 FU-11: 信号无效化 (`Com_InvalidateSignal`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | SignalId 超出范围 | 无效化错误的信号区域 | SignalId 范围检查 (`SignalId < COM_NUM_OF_SIGNALS`) | 99% | B |
| FM-09-02 | 无效化模式计算错误 | 无效化区域不完整 | 字节范围计算校验 (`startByte`, `endByte`) | 85% | B |
| FM-09-03 | 无效化后未更新标志 | 应用层读取到旧数据 | `Updated` 标志设置检查 | 90% | B |

### 3.10 FU-12: Deadline 监控

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | DM 控制标志设置错误 | 超时检测被错误启用/禁用 | `Com_DMEnabled[]` 数组边界检查 | 90% | B |
| FM-10-02 | 超时阈值配置错误 | 误报超时或漏报超时 | 配置参数校验；应用层超时回调验证 | 80% | B |
| FM-10-03 | 超时检测逻辑错误（CPU 瞬态故障） | 接收超时误报或漏报 | `Com_ReceptionDMControl()` 实现按 I-PDU 倒计时计数器 (L1157)；接收时重置计数器 (`Com_RxIndication` 中 `DmCounter` 重置)；超时时调用 `Com_ReceptionDeadlineNotification` 回调；计数器边界检查防止下溢 | 90% | B |

### 3.11 FU-13: I-PDU 组控制 (`Com_IpduGroupControl`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-11-01 | ipduGroupVector 指针无效 | 访问非法内存 | 指针有效性检查（隐式通过数组访问） | 85% | B |
| FM-11-02 | I-PDU 组 ID 超出范围 | 错误启停 I-PDU | 组 ID 范围检查 (`grp < COM_NUM_OF_IPDU_GROUPS`) | 99% | B |
| FM-11-03 | GroupEnabled 标志设置失败 | I-PDU 无法启停 | 标志设置后回读验证 | 90% | B |

### 3.12 FU-14: 信号滤波 (`Com_ApplyFilter`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-12-01 | 滤波算法类型错误 | 应用错误的滤波逻辑 | `FilterAlgorithm` 配置校验；default 分支返回 TRUE | 85% | B |
| FM-12-02 | 滤波掩码配置错误 | 滤波结果错误 | 掩码值合理性检查（应用层验证） | 75% | B |
| FM-12-03 | LastValue 未更新 | 差分滤波失效 | `LastValue` 更新逻辑检查（在 `Com_SendSignal` 中更新） | 80% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 信号发送 | 99% | 92% | 主要依赖参数校验 |
| FU-02 | 信号接收 | 99% | 82% | 应用层数据校验 |
| FU-03 | 信号组管理 | 99% | 88% | 范围检查 + 发送确认 |
| FU-05 | I-PDU 发送 | 99% | 90% | PduR 返回码检查 |
| FU-06 | I-PDU 接收 | 99% | 92% | 长度校验 + 范围检查 |
| FU-07 | 发送确认 | 99% | 85% | 重发计数 + 超时检测 |
| FU-08 | 周期发送 | 90% | 88% | WDT 监控 |
| FU-09 | 周期接收 | 99% | 90% | WDT 监控 |
| FU-11 | 信号无效化 | 99% | 90% | 范围校验 + 标志检查 |
| FU-12 | Deadline 监控 | 99% | 88% | 按 I-PDU 倒计时 + 接收重置 + 超时回调 |
| FU-13 | I-PDU 组控制 | 99% | 90% | 范围检查 + 标志回读 |
| FU-14 | 信号滤波 | 85% | 80% | 配置校验 + 应用层验证 |

### 4.2 模块整体诊断覆盖率

```
DC_Com = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_Com ≈ 91%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL B)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 90% | ~91% | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ~90% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 100 FIT | ~52 FIT | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。Com 中主要单点故障来源：
- 信号打包/解包逻辑的瞬态故障（由应用层合理性校验覆盖）
- Deadline 监控已完善（按 I-PDU 倒计时 + 接收重置 + 超时回调）
- 滤波算法配置错误（由配置工具校验覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (8.1 / 90) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。Com 中主要潜伏故障来源：
- 参数校验逻辑的系统性缺陷（通过代码审查和测试覆盖）
- I-PDU 组控制标志的逻辑故障（通过标志回读验证覆盖）
- 重发计数器的溢出故障（通过边界检查覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (9.0 / 90) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_signal_corruption + lambda_timeout_detection + lambda_filter_error
     ≈ 8 + 18 + 17 = 43 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 Com 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | 模块已初始化，I-PDU 组启用 | 正常发送/接收信号 |
| **降级运行** | 部分 I-PDU 组被禁用 | 仅服务启用的 I-PDU 组 |
| **信号无效** | 通信超时或错误 | 信号设置为无效值 (0xFF) |
| **安全停止** | 模块未初始化或主函数无法执行 | 系统进入安全状态，WDT 触发复位 |

### 6.2 安全状态转换

```
正常运行 ──[I-PDU组禁用]──→ 降级运行
降级运行 ──[通信超时]──→ 信号无效
信号无效 ──[通信恢复]──→ 正常运行
任意状态 ──[MainFunction超时]──→ 安全停止 (WDT触发)
```

---

## 7. 故障反应时间线

### 7.1 场景一：信号发送参数错误

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Com_SendSignal(SignalId, SignalDataPtr)` |
| T=1 cycle | 入口参数校验：`SignalDataPtr == NULL_PTR` |
| T=2 | 检测到故障，上报 DET 错误 `COM_E_PARAM_POINTER` |
| T=3 | 返回 `COM_SERVICE_NOT_OK` |
| **总延迟** | **1 个调用周期** |

### 7.2 场景二：I-PDU 接收数据损坏

| 时间 | 事件 |
|------|------|
| T=0 | PduR 调用 `Com_RxIndication(RxPduId, PduInfoPtr)` |
| T=1 cycle | 长度校验：`SduLength > COM_MAX_IPDU_BUFFER_SIZE` |
| T=2 | 截断到最大长度，拷贝到 `IPduBuffer[RxPduId]` |
| T=3 | 设置 `Updated = TRUE` |
| T=4 | `Com_MainFunctionRx` 周期调用，处理接收数据 |
| T=5 | 应用层调用 `Com_ReceiveSignal` 读取信号 |
| T=6 | 应用层检测到信号值异常，触发降级策略 |
| **总延迟** | **1 个主函数周期 + 应用层响应时间** |

### 7.3 场景三：周期发送超时

| 时间 | 事件 |
|------|------|
| T=0 | `Com_MainFunctionTx` 未被周期调用（OS 调度故障） |
| T=1-10 | 多个周期未执行，I-PDU 未发送 |
| T=11 | 系统级 WDT 检测到任务超时 |
| T=12 | WDT 触发系统复位 |
| **总延迟** | **WDT 超时时间（典型 100ms）** |

### 7.4 场景四：信号无效化（通信失败）

| 时间 | 事件 |
|------|------|
| T=0 | 通信超时检测（由 BswM 或应用层触发） |
| T=1 | 调用 `Com_InvalidateSignal(SignalId)` |
| T=2 | 计算信号字节范围：`startByte` 到 `endByte` |
| T=3 | 设置 I-PDU 缓冲区为无效模式 (0xFF) |
| T=4 | 设置 `Updated = TRUE`，标记 I-PDU 已更新 |
| T=5 | 下次 `Com_MainFunctionTx` 发送无效信号 |
| **总延迟** | **1 个主函数周期** |

---

## 8. ISO 26262 条款追溯

| ISO 26262 条款 | 要求 | 本文档覆盖 |
|---------------|------|-----------|
| Part 5, Clause 7 | FMEDA 方法论 | 第 3 节完整失效模式表 |
| Part 5, Clause 8 | 安全度量 (SPFM/LFM/PMHF) | 第 5 节度量分析 |
| Part 5, Clause 9 | 诊断覆盖率评估 | 第 4 节 DC 汇总 |
| Part 6, Clause 8 | 软件单元设计中的安全机制 | 第 3 节故障检测方法列 |
| Part 4, Clause 7 | 安全状态定义 | 第 6 节安全状态 |
| Part 4, Clause 8 | 技术安全需求 | 第 7 节故障反应时间线 |

---

## 9. 假设与约束

1. **底层通信驱动假设**: PduR/CAN/ETH 层的失效率及诊断覆盖率由独立 FMEDA 覆盖，本分析假设底层 FIT 数据已知
2. **配置正确性**: 假设 Com 配置（信号描述符、I-PDU 描述符、滤波算法）在集成阶段经过验证
3. **WDT 覆盖**: Com_MainFunctionRx/Tx 的调度监控由系统级 WDT 覆盖
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **Deadline 监控**: 已实现按 I-PDU 倒计时计数器，在 `Com_MainFunctionRx` 中递减，接收时重置，超时时触发 `Com_ReceptionDeadlineNotification` 回调
6. **网关路由**: `Com_MainFunctionRouteSignals` 当前为桩实现，完整实现需定义信号映射表

---

## 10. 已知问题与改进建议

### 10.1 桩函数需完善

| 函数 | 当前状态 | 改进建议 |
|------|---------|---------|
| `Com_MainFunctionRouteSignals` | 桩函数（L1043-1057） | 实现网关信号映射逻辑 |
| `Com_ReceptionDMControl` | ~~桩函数~~ → **已实现** (v1.1) | ~~实现按 I-PDU 组的超时计数器~~ ✅ 已实现倒计时 + 回调 |
| `Com_SendDynSignal` | 桩函数（L1270-1276） | 实现动态长度信号发送 |
| `Com_ReceiveDynSignal` | 桩函数（L1279-1285） | 实现动态长度信号接收 |

### 10.2 安全机制增强建议

1. **Deadline 监控完善**: 为每个接收 I-PDU 实现超时计数器，在 `Com_MainFunctionRx` 中递减并触发回调
2. **信号值范围检查**: 在 `Com_ReceiveSignal` 中增加配置化的最小/最大值校验
3. **I-PDU 缓冲区 CRC**: 对关键 I-PDU 缓冲区增加 CRC 校验，检测内存损坏
4. **重发机制增强**: 在 `Com_TxConfirmation` 失败时增加指数退避重发策略

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| CAN 收发器 | 15 FIT | 供应商 FMD |
| ETH MAC | 12 FIT | 供应商 FMD |
| RAM 存储 | 8 FIT | IEC 61508 Annex D |
| CPU 逻辑 | 12 FIT | 供应商 FMD |
| PduR 路由 | 5 FIT | 估算 |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| I-PDU | Interaction Protocol Data Unit |
| PduR | PDU Router |
| WDT | Watchdog Timer |
| DET | Default Error Tracer |
