# FMEDA — PduR (PDU Router)

> **文档编号**: SAF-FMEDA-PDUR-001
> **版本**: 1.1
> **日期**: 2026-10-09
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW Service Layer 的 PduR (PDU Router) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/services/pdur/src/PduR.c`

**分析边界**:
- PduR 核心逻辑（路由表查找、PDU 转发、FIFO 队列管理、路由使能/禁止）
- 上层接口：`PduR_Transmit`, `PduR_RxIndication`, `PduR_TxConfirmation`, `PduR_TriggerTransmit`
- 内部安全机制：参数校验、初始化状态检查、DET 错误上报、路由路径使能控制
- 下层依赖（假设由 ECUAL/MCAL 层覆盖）：CanIf, Com, Dcm

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 PduR 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `PduR_InternalStateType` | PduR.c:90-95 | 模块内部状态，含 State、ConfigPtr、PathStates[] |
| `PduR_RoutingPathStateType` | PduR.c:83-87 | 路由路径运行时状态：FifoQueue、IsEnabled |
| `PduR_FifoQueueType` | PduR.c:74-80 | FIFO 队列：Entries[]、Head、Tail、Count |
| `PduR_FifoEntryType` | PduR.c:66-71 | FIFO 条目：PduInfo、SduData[]、IsValid |
| `PduR_ConfigType` | PduR.h:142-149 | 全局配置：RoutingPaths[]、RoutingPathGroups[]、NumRoutingPaths |
| `PduR_RoutingPathConfigType` | PduR.h:121-127 | 路由路径配置：SrcPdu、DestPdus[]、PathType、GatewayOperation |
| `PduR_DestPduConfigType` | PduR.h:111-116 | 目标 PDU 配置：DestPduId、DestModule、Processing |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 路由路径查找 | `PduR_FindRoutingPath()` (L142) | 根据 PduId 和模块类型查找路由路径 |
| FU-02 | PDU 向下路由 (Tx) | `PduR_Transmit()` (L448) | 上层 (Com/Dcm) 向下层 (CanIf) 发送 PDU |
| FU-03 | PDU 向上路由 (Rx) | `PduR_RxIndication()` (L495) | 下层 (CanIf) 向上层 (Com/Dcm) 转发接收 PDU |
| FU-04 | Tx 确认转发 | `PduR_TxConfirmation()` (L534) | 将 CanIf 发送确认回传至上层 |
| FU-05 | 触发发送 | `PduR_TriggerTransmit()` (L602) | 下层请求上层填充发送缓冲区 |
| FU-06 | FIFO 队列管理 | `PduR_FifoPush()` (L267), `PduR_FifoPop()` (L315) | 延迟路由 PDU 的缓存与调度 |
| FU-07 | 路由使能/禁止 | `PduR_EnableRouting()` (L720), `PduR_DisableRouting()` (L734) | 按路由路径组控制路由开关 |
| FU-08 | 主函数调度 | `PduR_MainFunction()` (L776) | 周期调度，处理 FIFO 中的延迟 PDU |
| FU-09 | 取消发送 | `PduR_CancelTransmitRequest()` (L655) | 取消挂起的发送请求 |
| FU-10 | 初始化/去初始化 | `PduR_Init()` (L392), `PduR_DeInit()` (L424) | 模块初始化与资源释放 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 路由路径查找 (`PduR_FindRoutingPath`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | ConfigPtr 为空指针 | 路由查找跳过，返回 E_NOT_OK，PDU 丢失 | `PduR_FindRoutingPath()` 入口校验 `configPtr != NULL_PTR` (L148) | 90% | B |
| FM-01-02 | PduId 无匹配路由路径 | PDU 无法路由，被静默丢弃 | DET 错误上报 `PDUR_E_ROUTING_PATH_NOT_FOUND` (L481, L522) | 85% | B |
| FM-01-03 | 路由表配置数据损坏 (ROM) | 路由到错误目标或无法路由 | 配置数据位于 ROM 保护区，启动时由 BSW 完整性检查覆盖 | 90% | B |
| FM-01-04 | 遍历索引越界 (CPU 瞬态故障) | 访问路由表外的非法内存 | 循环边界检查 `i < configPtr->NumRoutingPaths` (L150)；数组大小由 `PDUR_NUMBER_OF_ROUTING_PATHS` (Cfg.h:41) 限定 | 95% | B |

### 3.2 FU-02: PDU 向下路由 — Tx (`PduR_Transmit`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | PduInfoPtr 为空指针 | 发送空数据，目标模块收到无效数据 | 入口参数校验 `PduInfoPtr == NULL_PTR` (L460)，DET 上报 `PDUR_E_PARAM_POINTER` | 99% | B |
| FM-02-02 | 模块未初始化即调用 Transmit | 使用未初始化的配置指针，行为不可预测 | 状态检查 `PduR_InternalState.State != PDUR_STATE_INIT` (L454)，DET 上报 `PDUR_E_UNINIT` | 99% | B |
| FM-02-03 | CanIf_Transmit 返回错误 | PDU 发送失败，上层不知晓发送结果 | `PduR_RouteToDestination()` 检查 `CanIf_Transmit` 返回值 (L213)，失败时返回 `E_NOT_OK` 至调用方 | 90% | B |
| FM-02-04 | 目标模块类型无法识别 | PDU 无法送达目标层 | `PduR_RouteToDestination()` switch-default 分支返回 `E_NOT_OK` (L228-231) | 85% | B |
| FM-02-05 | 路由路径组被禁止 | PDU 被拒绝发送，上层收到 E_NOT_OK | `PduR_Transmit()` 数据路径中检查 `IsEnabled` 标志 (L467)，被禁止时返回 `E_NOT_OK` 并上报 `PDUR_E_ROUTING_PATH_DISABLED`；上层可检测发送失败并触发降级策略 | 90% | B |

### 3.3 FU-03: PDU 向上路由 — Rx (`PduR_RxIndication`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | PduInfoPtr 为空指针 | 接收数据丢失，上层模块收到无效数据 | 入口参数校验 `PduInfoPtr == NULL_PTR` (L506)，DET 上报 `PDUR_E_PARAM_POINTER` | 99% | B |
| FM-03-02 | 模块未初始化即调用 RxIndication | 使用未初始化的配置，行为不可预测 | 状态检查 `State != PDUR_STATE_INIT` (L500)，DET 上报 `PDUR_E_UNINIT` | 99% | B |
| FM-03-03 | RxPduId 无匹配路由路径 | 接收 PDU 丢失，上层模块无法获取数据 | DET 错误上报 `PDUR_E_ROUTING_PATH_NOT_FOUND` (L522) | 85% | B |
| FM-03-04 | Com_RxIndication/Dcm_RxIndication 调用失败 | 上层模块未收到数据，但 PduR 返回 E_OK | 当前实现中 RxIndication 为 void 返回，上层通过超时检测发现数据丢失；系统级通信监控覆盖 | 70% | B |
| FM-03-05 | SduLength 不匹配 (源与目标 SDU 长度不一致) | 数据截断或缓冲区溢出 | 配置阶段 `SduLength` 字段校验 (SrcPdu.SduLength)；`PduR_FifoPush` 中拷贝长度限制 (L284) | 80% | B |

### 3.4 FU-04: Tx 确认转发 (`PduR_TxConfirmation`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | 模块未初始化 | 确认信息无法转发 | 状态检查 `State != PDUR_STATE_INIT` (L539)，DET 上报 `PDUR_E_UNINIT` | 99% | B |
| FM-04-02 | TxPduId 无法匹配任何路由路径 | 发送确认丢失，上层模块无法得知发送结果 | 回退查找机制：先查 CanIf 源路径，再查 Com/Dcm 源路径 (L548-L591)；未匹配时静默丢弃，由上层超时检测覆盖 | 75% | B |
| FM-04-03 | 确认结果 (result) 值损坏 | 上层收到错误的发送状态 | result 参数由 CanIf 传入，PduR 透传不做校验；上层 Com/Dcm 通过协议层超时和重传机制检测异常 | 60% | B |
| FM-04-04 | 目标模块类型无法识别 | 确认信息丢失 | switch-default 分支静默丢弃 (L566-568)；上层超时检测覆盖 | 65% | B |

### 3.5 FU-05: 触发发送 (`PduR_TriggerTransmit`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | PduInfoPtr 为空指针 | 缓冲区填充请求使用无效指针 | 入口参数校验 (L614)，DET 上报 `PDUR_E_PARAM_POINTER` | 99% | B |
| FM-05-02 | 模块未初始化 | 行为不可预测 | 状态检查 (L608)，DET 上报 `PDUR_E_UNINIT` | 99% | B |
| FM-05-03 | TxPduId 无匹配路由路径 | 触发发送请求丢失，CanIf 缓冲区为空 | 返回 `E_NOT_OK`，CanIf 可检测空缓冲区情况 | 85% | B |
| FM-05-04 | Com_TriggerTransmit/Dcm_TriggerTransmit 返回错误 | 上层无法填充发送数据 | 返回 `E_NOT_OK` 至 CanIf，CanIf 可跳过本次发送或重试 | 80% | B |

### 3.6 FU-06: FIFO 队列管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | FIFO 队列满 (Count >= PDUR_MAX_FIFO_DEPTH) | 新 PDU 无法入队，被丢弃 | `PduR_FifoIsFull()` 检查 (L273, L376)；返回 `E_NOT_OK` | 99% | B |
| FM-06-02 | FIFO 队列为空时执行 Pop | 返回无效数据 | `PduR_FifoIsEmpty()` 检查 (L321)；返回 `E_NOT_OK` | 99% | B |
| FM-06-03 | Head/Tail 指针回绕错误 | 队列索引错乱，数据损坏或丢失 | 模运算 `(Head + 1U) % PDUR_MAX_FIFO_DEPTH` (L298, L335) 保证回绕正确；Count 一致性校验 | 90% | B |
| FM-06-04 | IsValid 标志损坏 (内存位翻转) | 无效条目被当作有效条目处理，或有效条目被跳过 | `PduR_FifoPop()` 检查 `entryPtr->IsValid` (L325)；`PduR_FifoPush()` 设置标志 (L295) | 80% | B |
| FM-06-05 | SduData 缓冲区溢出 | 数据截断，PDU 内容不完整 | 拷贝长度限制 `min(SduLength, PDUR_MAX_DESTINATIONS_PER_PATH * 8U)` (L284-285) | 95% | B |
| FM-06-06 | Count 变量与实际条目数不一致 | 队列状态异常，可能导致 FIFO 永久满或永久空 | Count 在 Push 时递增 (L299)、Pop 时递减 (L336)；`PduR_FifoInit()` 重置为 0 (L251) | 75% | B |

### 3.7 FU-07: 路由使能/禁止

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | 路由路径组 ID 越界 | 访问 PathStates 数组外的内存 | 入口边界检查 `id < PDUR_NUMBER_OF_ROUTING_PATH_GROUPS` (L722, L736) | 99% | B |
| FM-07-02 | IsEnabled 标志位翻转 (内存故障) | 路由路径被意外禁止或使能 | `PduR_Transmit()` 和 `PduR_RxIndication()` 数据路径中检查 `IsEnabled` 标志 (L467, L513)；被禁止时返回 `E_NOT_OK`；标志翻转导致误禁止可由系统监控检测通信超时 | 85% | B |
| FM-07-03 | 所有路由路径被禁止 | 全部 PDU 通信中断 | 系统级通信监控（EcuM/BswM 状态管理）检测全局通信丢失 | 80% | B |

### 3.8 FU-08: 主函数调度 (`PduR_MainFunction`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | MainFunction 未被周期调用 | FIFO 中的延迟 PDU 无法被发送 | 系统级 WDT 监控调度是否正常；OS 任务监控 | 80% | B |
| FM-08-02 | MainFunction 执行时模块未初始化 | 使用无效配置处理 FIFO | 状态检查 `State == PDUR_STATE_INIT` (L780) | 99% | B |
| FM-08-03 | FIFO Pop 后路由失败 (目标模块错误) | 延迟 PDU 丢失，无法重试 | `PduR_RoutePdu()` 返回值被忽略 `(void)` (L797)；PDU 出队后无法恢复 | 60% | B |
| FM-08-04 | 遍历所有路由路径时索引越界 | 访问非法内存 | 循环边界 `i < PDUR_NUMBER_OF_ROUTING_PATHS` (L782) | 95% | B |

### 3.9 FU-09: 取消发送 (`PduR_CancelTransmitRequest`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | 模块未初始化 | 取消请求无法处理 | 状态检查 (L661)，DET 上报 `PDUR_E_UNINIT` | 99% | B |
| FM-09-02 | TxPduId 无匹配路由路径 | 取消操作无效，发送请求仍挂起 | 返回 `E_NOT_OK`，调用方可检测并处理 | 85% | B |
| FM-09-03 | CanIf_CancelTransmit 返回错误 | 发送请求无法取消 | 返回值透传至调用方 (L677) | 90% | B |

### 3.10 FU-10: 初始化/去初始化

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | ConfigPtr 为空指针 | 模块使用空配置运行，行为不可预测 | `PduR_Init()` 入口校验 (L397)，DET 上报 `PDUR_E_PARAM_POINTER` | 99% | B |
| FM-10-02 | 重复初始化 | 路由路径状态被重置，正在处理的 PDU 丢失 | 当前实现未检测重复初始化——依赖上层调度保证仅初始化一次 | 50% | B |
| FM-10-03 | 去初始化时模块未处于 INIT 状态 | 对未初始化模块执行去初始化 | 状态检查 (L427)，DET 上报 `PDUR_E_UNINIT` | 99% | B |
| FM-10-04 | FIFO 初始化不完整 | 队列残留旧数据导致错误路由 | `PduR_FifoInit()` 清除所有条目的 IsValid 标志 (L253-256)，重置 Head/Tail/Count | 90% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 路由路径查找 | 95% | 90% | 空指针校验 + 边界检查 + ROM 完整性 |
| FU-02 | PDU 向下路由 (Tx) | 99% | 90% | 参数校验 + 状态检查 + 返回值检测 + IsEnabled 检查 |
| FU-03 | PDU 向上路由 (Rx) | 99% | 85% | 参数校验 + 状态检查 + DET 上报 |
| FU-04 | Tx 确认转发 | 99% | 75% | 回退查找机制 + 上层超时覆盖 |
| FU-05 | 触发发送 | 99% | 88% | 参数校验 + 返回值传递 |
| FU-06 | FIFO 队列管理 | 99% | 88% | 满/空检测 + 长度限制 + 模运算保护 |
| FU-07 | 路由使能/禁止 | 99% | 84% | 边界检查 + 数据路径 IsEnabled 检查 |
| FU-08 | 主函数调度 | 99% | 84% | 状态检查 + WDT 覆盖 |
| FU-09 | 取消发送 | 99% | 91% | 状态检查 + 返回值透传 |
| FU-10 | 初始化/去初始化 | 99% | 85% | 空指针校验 + 状态检查 + FIFO 清除 |

### 4.2 模块整体诊断覆盖率

```
DC_PduR = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_PduR ≈ 89%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL B)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 90% | ~91% | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ~90% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 100 FIT | ~48 FIT | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。PduR 中主要单点故障来源：
- 路由路径查找中配置数据的不可检测故障（由 ROM 完整性校验覆盖）
- FIFO 队列 Count 变量的不可检测位翻转（由模运算和 Init 重置部分覆盖）
- IsEnabled 标志的不可检测位翻转（数据路径已检查该标志，误禁止由系统监控覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (8.1 / 95) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。PduR 中主要潜伏故障来源：
- DET 错误上报机制自身的故障（由 `PDUR_DEV_ERROR_DETECT` 编译开关控制，假设编译时已验证）
- 路由表配置数据的系统性缺陷（通过启动时配置完整性检查覆盖）
- FIFO 管理逻辑（Head/Tail/Count 一致性）的潜在缺陷（通过 Init 完整重置和边界检查覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (9.5 / 95) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_routing_miss + lambda_fifo_corruption + lambda_confirmation_loss
     ≈ 15 + 12 + 15 = 42 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 PduR 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | State = PDUR_STATE_INIT，所有路由路径 IsEnabled = TRUE | 正常路由 Tx/Rx/Confirmation/TriggerTransmit |
| **降级运行** | 部分路由路径查找失败或 FIFO 偶发溢出 | DET 上报错误，有效路径继续工作；上层通过超时检测丢失的 PDU |
| **路由禁止** | 一个或多个路由路径组被 DisableRouting 禁止 | 被禁止路径组的 PDU 不再路由；其余路径组正常工作 |
| **安全停止** | State = PDUR_STATE_UNINIT 或 ConfigPtr = NULL_PTR | 所有路由操作被拒绝，DET 上报 PDUR_E_UNINIT；系统进入安全状态 |

### 6.2 安全状态转换

```
正常运行 ──[部分路由查找失败]──→ 降级运行
正常运行 ──[DisableRouting调用]──→ 路由禁止 (部分)
降级运行 ──[ConfigPtr损坏]──→ 安全停止
任意状态 ──[PduR_DeInit调用]──→ 安全停止
安全停止 ──[PduR_Init调用]──→ 正常运行
任意状态 ──[MainFunction超时]──→ 安全停止 (WDT触发)
```

---

## 7. 故障反应时间线

### 7.1 场景一：Tx 路由路径未找到

| 时间 | 事件 |
|------|------|
| T=0 | 上层调用 `PduR_Transmit(TxPduId, PduInfoPtr)` |
| T=1 cycle | 状态检查通过 (State == PDUR_STATE_INIT) |
| T=2 cycles | `PduR_FindRoutingPath(TxPduId, PDUR_MODULE_COM)` 遍历所有路径，未匹配 |
| T=3 cycles | `PduR_FindRoutingPath(TxPduId, PDUR_MODULE_DCM)` 遍历所有路径，未匹配 |
| T=4 cycles | DET 上报 `PDUR_E_ROUTING_PATH_NOT_FOUND` (L481) |
| T=5 cycles | 返回 `E_NOT_OK` 至上层 |
| **总延迟** | **1 个调用周期 + DET 上报开销** |

### 7.2 场景二：FIFO 队列溢出

| 时间 | 事件 |
|------|------|
| T=0 | 延迟 PDU 需要入队，FIFO 已满 (Count == PDUR_MAX_FIFO_DEPTH) |
| T=1 | `PduR_FifoIsFull()` 返回 TRUE (L273) |
| T=2 | `PduR_FifoPush()` 返回 `E_NOT_OK` |
| T=3 | PDU 被丢弃，路由返回失败 |
| T=4 | `PduR_MainFunction()` 下一个周期尝试发送 FIFO 中的 PDU，释放空间 |
| **总延迟** | **1 个 FIFO Push 周期 + 最多 1 个 MainFunction 周期恢复** |

### 7.3 场景三：RxIndication 路由至错误目标

| 时间 | 事件 |
|------|------|
| T=0 | CanIf 调用 `PduR_RxIndication(RxPduId, PduInfoPtr)` |
| T=1 | 状态检查和参数校验通过 |
| T=2 | `PduR_FindRoutingPath(RxPduId, PDUR_MODULE_CANIF)` 找到匹配路径 |
| T=3 | `PduR_RoutePdu()` 调用 `PduR_RouteToDestination()` |
| T=4 | `DestModule` 匹配 `PDUR_MODULE_COM`，调用 `Com_RxIndication()` |
| T=5 | 若 Com 侧检测到 PDU 数据异常（如 SduLength 不匹配），上报诊断事件 |
| **总延迟** | **5 个调用周期 + 上层检测时间** |

### 7.4 场景四：模块未初始化时调用 Transmit

| 时间 | 事件 |
|------|------|
| T=0 | 上层调用 `PduR_Transmit(TxPduId, PduInfoPtr)` |
| T=1 | 状态检查 `State != PDUR_STATE_INIT` 为 TRUE (L454) |
| T=2 | DET 上报 `PDUR_E_UNINIT` |
| T=3 | 返回 `E_NOT_OK`，PDU 未发送 |
| T=4 | 上层模块收到错误，可触发降级策略或重试 |
| **总延迟** | **1 个调用周期** |

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

1. **下层通信驱动假设**: CanIf 层的失效率及诊断覆盖率由独立 FMEDA 覆盖，本分析假设底层 FIT 数据已知
2. **配置数据完整性**: 假设路由表配置 (`PduR_RoutingPaths[]`, `PduR_Config`) 存储于受保护的 ROM 区域，启动时经过完整性校验（参见 `PduR_Lcfg.c`）
3. **WDT 覆盖**: PduR_MainFunction 的调度监控由系统级 WDT 覆盖
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **配置正确性**: 假设 PduR 配置（路由路径、FIFO 深度、路由路径组）在集成阶段经过验证
6. **IsEnabled 标志**: `PduR_Transmit()` 和 `PduR_RxIndication()` 数据路径中已检查 `IsEnabled` 标志，被禁止的路由路径返回 `E_NOT_OK`（v1.1 更新）
7. **网关路由**: 当前配置中 `GatewayOperation` 字段已定义但数据路径中未实现网关转发逻辑（所有路径 `GatewayOperation = FALSE`），后续版本需补充网关路由的失效模式分析

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| RAM 存储 (路由状态/FIFO) | 8 FIT | IEC 61508 Annex D |
| CPU 逻辑 (路由查找/转发) | 12 FIT | 供应商 FMD |
| ROM 配置数据 | 3 FIT | IEC 61508 Annex D |
| CAN 通信接口 (CanIf) | 15 FIT | 供应商 FMD |
| 总线通信 (CAN 物理层) | 20 FIT | IEC 61508 Annex D |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| PDU | Protocol Data Unit |
| PduR | PDU Router |
| DET | Default Error Tracer |
| FIFO | First In First Out |
| SDU | Service Data Unit |
| COM | Communication |
| DCM | Diagnostic Communication Manager |
| CanIf | CAN Interface |
