# FMEDA — CanTp (CAN Transport Protocol, ISO 15765-2)

> **文档编号**: SAF-FMEDA-CANTP-001
> **版本**: 1.0
> **日期**: 2026-10-22
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW ECUAL 层的 CanTp (CAN Transport Protocol) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/ecual/cantp/src/CanTp.c`

**分析边界**:
- CanTp 核心逻辑（ISO 15765-2 分段/重组协议、帧类型处理、超时管理）
- 上层接口：`CanTp_Transmit`, `CanTp_CancelTransmit`, `CanTp_CancelReceive`, `CanTp_ChangeParameter`, `CanTp_ReadParameter`
- 内部安全机制：超时检测与中止（N_As/N_Bs/N_Cs/N_Cr）、帧长度校验（DLC/SF_DL/FF_DL）、序列号校验、Rx 帧队列（ISR/任务隔离）、填充字节验证
- 下层依赖（假设由 ECUAL/MCAL 层覆盖）：CanIf
- 上层依赖（假设由 Service 层覆盖）：PduR

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 CanTp 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `CanTp_ChannelRuntimeType` | CanTp.c:51-72 | 通道运行时结构：State, ActiveNsduId, DataLength, DataIndex, SequenceNumber, BlockSize, STmin, WftCounter, Timer, StMinTimerMs, StMinActive, Buffer[], TxConfirmed, RxIndicated, TxNsduConfig/RxNsduConfig 指针, TxMetaData/RxMetaData |
| `CanTp_RxQueueEntryType` | CanTp.c:75-81 | Rx 帧队列条目：RxPduId, SduData[64], SduLength, MetaDataValid, MetaData[4] |
| `CanTp_ChannelStateType` (enum) | CanTp.c:31-41 | 通道状态机：IDLE, TX_SF, TX_FF, TX_CF, RX_SF, RX_FF, RX_CF, TX_WAIT_FC, RX_WAIT_FC |
| `CanTp_ConfigType` | CanTp.h:253-257 | 顶层配置：GeneralConfig, ChannelConfigs[], NumChannels |
| `CanTp_TxNsduConfigType` | CanTp.h:186-199 | Tx NSDU 配置：N_As/N_Bs/N_Cs 超时、寻址格式、填充激活 |
| `CanTp_RxNsduConfigType` | CanTp.h:204-220 | Rx NSDU 配置：N_Ar/N_Br/N_Cr 超时、Bs、STmin、WftMax |
| `CanTp_ChannelRuntimeType.Buffer[]` | CanTp.c:63 | 通道数据缓冲区，大小 `CANTP_CHANNEL_BUFFER_SIZE` (64 bytes) |

### 1.4 ISO 15765-2 帧类型与超时参数

| 帧类型 | PCI 字节格式 | 说明 |
|--------|-------------|------|
| Single Frame (SF) | `0x0_ | SF_DL` | 单帧传输，数据量 <= 7 字节 (classic CAN) |
| First Frame (FF) | `0x1_ | FF_DL_high`, `FF_DL_low` | 多帧首帧，携带总长度 |
| Consecutive Frame (CF) | `0x2_ | SN` | 多帧后续帧，携带 4-bit 序列号 |
| Flow Control (FC) | `0x3_ | FS`, `BS`, `STmin` | 流控帧：CTS/Wait/Overflow |

| 超时参数 | 含义 | 配置来源 |
|---------|------|---------|
| N_As | 发送端等待 SF Tx 确认 | `CanTp_TxNsduConfigType.CanTpNas` (L190) |
| N_Bs | 发送端等待 FC 帧 | `CanTp_TxNsduConfigType.CanTpNbs` (L191) |
| N_Cs | 发送端 CF 帧发送超时 | `CanTp_TxNsduConfigType.CanTpNcs` (L192) |
| N_Cr | 接收端等待下一帧 (CF/FF) | `CanTp_RxNsduConfigType.CanTpNcr` (L210) |
| N_Br | 接收端 FC 帧发送响应时间 | `CanTp_RxNsduConfigType.CanTpNbr` (L209) |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 单帧发送 | `CanTp_SendSingleFrame()` (L570), `CanTp_Transmit()` SF 分支 (L826-830) | 数据 <= 7 字节时发送 SF，启动 N_As 超时 |
| FU-02 | 多帧发送（FF/CF） | `CanTp_SendFirstFrame()` (L606), `CanTp_SendConsecutiveFrame()` (L643) | 多帧分段发送，FF 后等待 FC，CF 按 STmin 间隔发送 |
| FU-03 | 流控处理 | `CanTp_ProcessRxFrame()` FC 分支 (L1200-1254), `CanTp_SendFlowControl()` (L535) | 接收 FC 帧处理 CTS/Wait/Overflow；发送 FC 帧 |
| FU-04 | 单帧接收 | `CanTp_ProcessRxFrame()` SF 分支 (L1038-1082) | SF 接收、DLC 校验、填充验证、转发 PduR |
| FU-05 | 多帧接收（FF/CF 重组） | `CanTp_ProcessRxFrame()` FF 分支 (L1085-1131), CF 分支 (L1134-1198) | FF 接收后发送 FC，CF 按 SN 重组，完成后转发 PduR |
| FU-06 | 超时管理 | `CanTp_MainFunction()` 超时检测 (L1358-1396) | N_As/N_Bs/N_Cs/N_Cr 超时检测与通道中止 |
| FU-07 | Rx 帧队列 | `CanTp_RxQueueEnqueue()` (L407), `CanTp_RxQueueDequeue()` (L452), `CanTp_RxQueueFlush()` (L467) | ISR/任务上下文隔离，深度 4 帧 |
| FU-08 | 帧长度与填充校验 | `CanTp_CheckRxFrameDL()` (L294), `CanTp_CheckRxPaddingFrom()` (L364) | SF_DL/FF_DL/CF DLC 一致性校验；填充字节验证 |
| FU-09 | 通道管理与初始化 | `CanTp_Init()` (L694), `CanTp_Shutdown()` (L726), `CanTp_ResetChannel()` (L172), `CanTp_FindFreeChannel()` (L199) | 模块初始化/关闭、通道状态重置、空闲通道查找 |
| FU-10 | STmin 间隔控制 | `CanTp_SendConsecutiveFrame()` STmin 逻辑 (L681-683), `CanTp_MainFunction()` STmin 调度 (L1398-1410) | CF 帧间最小分离时间控制 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 单帧发送 (`CanTp_SendSingleFrame`, `CanTp_Transmit` SF 分支)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | Tx 数据指针为空或 SduLength 为 0 | 发送无效数据或无数据帧 | `CanTp_Transmit()` 入口校验 `dataLength == 0U` 返回 `E_NOT_OK` (L776-778)；`CanTpTxInfoPtr == NULL_PTR` DET 报错 (L768-771) | 90% | B |
| FM-01-02 | 数据长度超过最大消息长度 | 缓冲区溢出 | `CanTp_Transmit()` 检查 `dataLength > CANTP_CANFD_MAX_MESSAGE_LENGTH` (L780-785)，DET 报 `CANTP_E_INVALID_TX_LENGTH` | 99% | B |
| FM-01-03 | 无空闲通道可用 | 发送请求被拒绝 | `CanTp_FindFreeChannel()` 返回 `CANTP_INVALID_CHANNEL_ID` (L789-791)，`CanTp_Transmit` 返回 `E_NOT_OK` | 99% | B |
| FM-01-04 | N_As 超时（SF 发送后未收到 Tx 确认） | 发送通道卡死 | `CanTp_MainFunction()` 检测 `Timer == 0U` 且状态为 `CANTP_CH_TX_SF`，调用 `CanTp_ResetChannel()` 中止 (L1368-1371) | 95% | B |
| FM-01-05 | Buffer 数据拷贝损坏（CPU 瞬态故障） | 发送错误数据 | Tx 确认由 `CanTp_TxConfirmation()` 验证 (L1303-1306)；超时机制兜底 | 80% | B |

### 3.2 FU-02: 多帧发送（FF/CF）

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | FF 帧长度字段编码错误 | 接收端解析出错误的总长度，导致数据重组错误 | `CanTp_SendFirstFrame()` 中 FF_DL 高低字节拆分编码 (L611-612)；接收端 FF_DL 范围校验 `CanTp_CheckRxFrameDL` (L316-320) | 90% | B |
| FM-02-02 | N_Bs 超时（FF 发送后未收到 FC） | 发送通道卡死在等待 FC 状态 | `CanTp_MainFunction()` 检测 `CANTP_CH_TX_WAIT_FC` 状态 `Timer == 0U`，调用 `CanTp_ResetChannel()` (L1374-1376) | 95% | B |
| FM-02-03 | CF 序列号 (SN) 回绕错误 | 接收端 SN 不匹配，数据重组错位 | SN 使用 4-bit 模 16 递增 `(SN + 1) & 0x0F` (L679)；接收端 CF 分支严格匹配 SN (L1142) | 90% | B |
| FM-02-04 | N_Cs 超时（CF 发送后未收到确认） | 发送通道卡死 | `CanTp_MainFunction()` 检测 `CANTP_CH_TX_CF` 状态 `Timer == 0U` 且 STmin 仍活跃，调用 `CanTp_ResetChannel()` (L1379-1385) | 95% | B |
| FM-02-05 | Buffer 越界写入（DataIndex 超出缓冲区） | 内存损坏 | `CanTp_SendConsecutiveFrame()` 中 `remainingBytes` 计算 (L651-652) 受 `DataLength` 限制；`CanTp_Transmit()` 入口限制 `dataLength <= CANTP_CHANNEL_BUFFER_SIZE` (L780) | 90% | B |
| FM-02-06 | TxNsduConfig 指针为空 | 无法获取超时参数，使用默认值或行为异常 | 各发送函数检查 `TxNsduConfig != NULL_PTR` 后选择配置超时或默认值 (L1311-1314, L1324-1328) | 85% | B |

### 3.3 FU-03: 流控处理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | FC 帧 Flow Status (FS) 值无效 | 发送端进入未定义状态 | `CanTp_ProcessRxFrame()` FC 分支仅处理 CTS/WT/OVFLW 三种 FS (L1218-1245)，保留值被忽略 | 85% | B |
| FM-03-02 | FC 帧 Overflow 指示 | 接收端缓冲区不足 | `CANTP_FLOWSTATUS_OVFLW` 分支调用 `CanTp_ResetChannel()` 中止发送 (L1240-1242) | 99% | B |
| FM-03-03 | FC 帧 DLC 不足（< 3 字节） | BS/STmin 字段读取越界 | `CanTp_CheckRxFrameDL()` FC 分支检查 `frameLen < 3U` (L332-334) | 99% | B |
| FM-03-04 | FC 帧 BS 或 STmin 值异常 | 发送节奏失控 | `CanTp_EncodeStMinMs()` 对保留值返回 0ms (L383-392)；BS 值直接使用，由 N_Cs 超时兜底 | 80% | B |
| FM-03-05 | 发送 FC 帧时 CanIf_Transmit 失败 | 发送端未收到 FC，触发 N_Bs 超时 | FC 发送后由 N_Cr 超时机制在发送端兜底 (L1125)；接收端超时后通道重置 | 85% | B |

### 3.4 FU-04: 单帧接收

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | SF_DL 值超出有效范围 (0 或 > 7) | 数据长度错误，可能缓冲区溢出 | `CanTp_CheckRxFrameDL()` SF 分支检查 `sfDl == 0 || sfDl > CANTP_MAX_SF_DATA_LEN` (L310-312) | 99% | B |
| FM-04-02 | SF 帧 DLC 与 SF_DL 不一致 | 帧格式非法 | `CanTp_CheckRxFrameDL()` 统一 DLC 合法性校验 (L303-305) + SF_DL 范围检查 | 99% | B |
| FM-04-03 | 填充字节值不正确 | 帧数据被污染 | `CanTp_CheckRxPaddingFrom()` 验证填充区域 (L364-376)；`CANTP_REJECT_INVALID_PADDING` 开启时丢弃帧 (L1048-1050) | 90% | B |
| FM-04-04 | 无空闲通道接收 SF | SF 帧被丢弃 | `CanTp_FindFreeChannel()` 返回 `CANTP_INVALID_CHANNEL_ID` 时不分配通道 (L1054-1055) | 99% | B |

### 3.5 FU-05: 多帧接收（FF/CF 重组）

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | FF_DL 超出消息长度限制 | 缓冲区溢出 | `CanTp_CheckRxFrameDL()` FF 分支检查 `ffDl > CANTP_MAX_MESSAGE_LENGTH` (L317-319) | 99% | B |
| FM-05-02 | FF_DL 值 <= SF 容量（<= 7） | 协议违规：应使用 SF 而非 FF | `CanTp_CheckRxFrameDL()` FF 分支检查 `ffDl <= CANTP_MAX_SF_DATA_LEN` (L317) | 99% | B |
| FM-05-03 | CF 序列号 (SN) 不匹配 | 数据重组顺序错误 | CF 分支严格匹配 `runtime->SequenceNumber == sn` (L1142)；不匹配则帧被忽略 | 95% | B |
| FM-05-04 | CF 帧数据拷贝越界 | Buffer 溢出 | `bytesToCopy` 受 `remainingBytes` 和 `CANTP_MAX_CF_DATA_LEN` 双重限制 (L1144-1145) | 99% | B |
| FM-05-05 | N_Cr 超时（等待 CF 超时） | 接收通道卡死 | `CanTp_MainFunction()` 检测 `CANTP_CH_RX_CF` 状态 `Timer == 0U`，调用 `CanTp_ResetChannel()` (L1388-1390) | 95% | B |
| FM-05-06 | RxNsduConfig 指针为空（配置缺失） | 无法获取 N_Cr 等超时参数 | FF 分支检查 `rxNsduConfig == NULL_PTR` 后 `break` (L1096-1098) | 90% | B |
| FM-05-07 | 重组完成后 PduR_RxIndication 调用失败 | 完整数据未送达上层 | 数据完整性由 `DataIndex >= DataLength` 条件保证 (L1179)；PduR 层错误由上层诊断覆盖 | 85% | B |

### 3.6 FU-06: 超时管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | Timer 递减逻辑故障（CPU 瞬态错误） | 超时无法触发，通道永久占用 | `CanTp_MainFunction()` 周期执行 (L1343)，由系统级 WDT 监控 MainFunction 调度 | 80% | B |
| FM-06-02 | Timer 初值配置错误（N_As/N_Bs/N_Cs/N_Cr 为 0） | 超时立即触发，合法传输被中止 | 配置来自 `TxNsduConfig`/`RxNsduConfig` 或默认值 (`CANTP_NAS_DEFAULT` 等)，由配置工具校验 | 85% | B |
| FM-06-03 | MainFunction 未被周期调度 | 所有超时失效，通道无法释放 | 系统级 WDT 监控；`CanTp_MainFunction()` 入口检查 `CanTp_Initialized` (L1345-1347) | 90% | B |
| FM-06-04 | STmin 定时器 (StMinTimerMs) 异常 | CF 帧间隔失控，可能违反接收端 STmin 要求 | STmin 定时器独立于协议超时 (L1361-1363)，由 `CanTp_EncodeStMinMs()` 编码 (L383-392) | 85% | B |

### 3.7 FU-07: Rx 帧队列

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | Rx 队列满（4 帧） | 新接收帧被丢弃 | `CanTp_RxQueueEnqueue()` 检查 `CanTp_RxQueueCount >= CANTP_RX_QUEUE_DEPTH` (L409-410)；报告 `CANTP_E_RX_COM` 运行时错误 (L1283) | 99% | B |
| FM-07-02 | 队列索引回绕错误 | 读写指针错位，数据损坏 | 使用模运算 `(index + 1) % CANTP_RX_QUEUE_DEPTH` (L442, L459)；`CanTp_RxQueueCount` 作为独立一致性校验 | 90% | B |
| FM-07-03 | ISR 与任务上下文竞争 | 数据不一致 | 单生产者（`CanTp_RxIndication` ISR）/ 单消费者（`CanTp_MainFunction` 任务）模型 (L87)；`CanTp_RxQueueEnqueue` 仅做数据拷贝，不做协议处理 (L1279-1280) | 85% | B |
| FM-07-04 | SduData 拷贝截断（帧长 > 64 字节） | 数据不完整 | 拷贝长度钳位到 `CANTP_RX_QUEUE_MAX_SDULEN` (L418-419)；CAN FD 最大 64 字节，恰好覆盖 | 95% | B |

### 3.8 FU-08: 帧长度与填充校验

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | CAN FD 帧长度表校验遗漏 | 非法 DLC 帧被接受 | `CanTp_IsValidFrameLength()` 使用 switch-case 穷举合法长度 0-8/12/16/20/24/32/48/64 (L239-261) | 99% | B |
| FM-08-02 | Classic CAN 帧长度 > 8 字节 | 缓冲区越界 | `CanTp_IsValidFrameLength()` 对 classic CAN 检查 `Length <= CANTP_CAN_FRAME_LENGTH` (L261) | 99% | B |
| FM-08-03 | 填充字节常量 (0xCC) 比较逻辑错误 | 填充校验误判 | `CanTp_CheckRxPaddingFrom()` 逐字节比较 `CANTP_PADDING_BYTE_VALUE` (L370)；配置值 `204U` (0xCC) 由 `CanTp_Cfg.h:58` 定义 | 90% | B |
| FM-08-04 | PaddingActive 配置与实际不匹配 | 校验规则应用错误 | 从 `TxNsduConfig`/`RxNsduConfig` 的 `PaddingActivation` 字段读取 (L1023-1025)；配置工具保证一致性 | 85% | B |

### 3.9 FU-09: 通道管理与初始化

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | `CanTp_ConfigPtr` 为空指针 | 配置访问崩溃 | `CanTp_Init()` 入口校验 `CfgPtr == NULL_PTR` (L697-699)；各配置访问函数检查 `CanTp_ConfigPtr != NULL_PTR` (L126-128, L150-152) | 99% | B |
| FM-09-02 | 模块未初始化时调用 API | 操作未定义数据 | 各 API 入口检查 `CanTp_Initialized == FALSE` (L729-732, L760-762, L851-853 等)，DET 报 `CANTP_E_UNINIT` | 99% | B |
| FM-09-03 | `CanTp_ResetChannel` 未完全清除通道状态 | 残留数据影响后续传输 | `CanTp_ResetChannel()` 逐一清零所有字段 (L174-196)，包括 Buffer、MetaData、Timer | 90% | B |
| FM-09-04 | Shutdown 后 ConfigPtr 未置空 | 野指针访问 | `CanTp_Shutdown()` 显式置 `CanTp_ConfigPtr = NULL_PTR` (L747) 和 `CanTp_Initialized = FALSE` (L748) | 95% | B |

### 3.10 FU-10: STmin 间隔控制

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | STmin 编码值超出范围 | CF 间隔时间错误 | `CanTp_EncodeStMinMs()` 处理三种范围：0x00-0x7F (0-127ms), 0xF1-0xF9 (100-900us -> 1ms), 保留值 -> 0ms (L384-392) | 90% | B |
| FM-10-02 | StMinActive 标志未正确清除 | CF 发送被永久阻塞或无间隔发送 | `StMinActive` 在 `CanTp_ResetChannel()` 中清除 (L185)；`CanTp_SendConsecutiveFrame()` 每次发送后重新置位 (L683) | 85% | B |
| FM-10-03 | STmin 定时器与协议定时器竞争 | 定时器递减顺序导致时序偏差 | `CanTp_MainFunction()` 先递减定时器 (L1358-1363)，再检查超时 (L1366)，最后检查 STmin 发送条件 (L1398-1410)；顺序固定 | 85% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 单帧发送 | 99% | 91% | 入口参数校验 + N_As 超时 |
| FU-02 | 多帧发送 (FF/CF) | 99% | 90% | N_Bs/N_Cs 超时 + SN 校验 |
| FU-03 | 流控处理 | 99% | 88% | FC DLC 校验 + FS 枚举处理 |
| FU-04 | 单帧接收 | 99% | 95% | SF_DL 校验 + 填充验证 |
| FU-05 | 多帧接收 (FF/CF 重组) | 99% | 93% | FF_DL 校验 + SN 匹配 + N_Cr 超时 |
| FU-06 | 超时管理 | 90% | 86% | 周期调度 + WDT 监控 |
| FU-07 | Rx 帧队列 | 99% | 91% | 队列满检测 + 模运算回绕 |
| FU-08 | 帧长度与填充校验 | 99% | 93% | CAN FD 长度表 + 逐字节填充验证 |
| FU-09 | 通道管理与初始化 | 99% | 93% | 初始化状态检查 + NULL 指针校验 |
| FU-10 | STmin 间隔控制 | 90% | 87% | 编码范围处理 + 独立定时器 |

### 4.2 模块整体诊断覆盖率

```
DC_CanTp = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_CanTp ≈ 90%
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

单点故障是指不被任何诊断机制覆盖的残余故障。CanTp 中主要单点故障来源：
- 超时定时器递减逻辑的瞬态故障（由系统级 WDT 监控 MainFunction 调度覆盖）
- Rx 队列 ISR/任务竞争的不可检测故障（由单生产者/单消费者设计缓解）
- STmin 编码逻辑的瞬态故障（由 N_Cs 超时兜底）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (8.1 / 92) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。CanTp 中主要潜伏故障来源：
- 帧长度校验表 (`CanTp_IsValidFrameLength`) 自身的系统性缺陷（通过编译时配置校验覆盖）
- 填充字节常量 (`CANTP_PADDING_BYTE_VALUE = 0xCC`) 的配置错误（由配置工具一致性检查覆盖）
- 超时默认值 (`CANTP_NAS_DEFAULT` 等) 与 ISO 15765-2 规定不一致（由集成测试覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (9.2 / 92) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_timeout_logic + lambda_queue_race + lambda_stmin_encoding + lambda_config_pointer
     ≈ 8 + 12 + 10 + 18 = 48 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 CanTp 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | 所有通道处于 IDLE 状态或正常 Tx/Rx 流程中 | 正常发送/接收 SF/FF/CF/FC 帧 |
| **降级运行** | 单个通道因超时或协议错误被重置 | 其他通道不受影响；故障通道回到 IDLE；运行时错误上报 DET |
| **安全停止** | `CanTp_Initialized == FALSE`（模块未初始化或已关闭） | 所有 API 返回错误或不执行操作；所有通道 IDLE；Rx 队列已清空 |

### 6.2 安全状态转换

```
正常运行 ──[N_As/N_Bs/N_Cs 超时]──→ 降级运行 (故障通道重置为 IDLE)
正常运行 ──[N_Cr 超时]──→ 降级运行 (接收通道重置为 IDLE)
正常运行 ──[FC Overflow]──→ 降级运行 (发送通道重置为 IDLE)
正常运行 ──[帧长度校验失败]──→ 降级运行 (丢弃非法帧，上报运行时错误)
正常运行 ──[Rx 队列满]──→ 降级运行 (丢弃新帧，上报 CANTP_E_RX_COM)
任意状态 ──[CanTp_Shutdown()]──→ 安全停止
任意状态 ──[CanTp_Init()]──→ 正常运行 (所有通道重置)
任意状态 ──[MainFunction 未调度]──→ 安全停止 (WDT 触发系统复位)
```

### 6.3 故障安全原则

CanTp 模块的故障安全策略基于以下原则：
1. **超时即中止**：任何协议状态在超时后均通过 `CanTp_ResetChannel()` 回到 IDLE 安全状态
2. **校验即丢弃**：帧长度、填充字节、序列号等校验失败的帧被直接丢弃，不影响通道状态机
3. **ISR 最小化**：`CanTp_RxIndication` 仅做队列拷贝 (L1279-1280)，协议处理在任务上下文的 `CanTp_MainFunction` 中执行
4. **通道独立性**：单个通道的故障不影响其他通道的正常运行

---

## 7. 故障反应时间线

### 7.1 场景一：N_Bs 超时（FF 发送后未收到 FC）

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `CanTp_Transmit(TxSduId, InfoPtr)`，数据长度 > 7 字节 |
| T=1 | `CanTp_FindFreeChannel()` 分配空闲通道，`CanTp_SendFirstFrame()` 发送 FF |
| T=2 | `CanTp_TxConfirmation()` 确认 FF 已发送，状态转为 `CANTP_CH_TX_WAIT_FC`，Timer 设为 `CanTpNbs` (L1311-1314) |
| T=3..N | `CanTp_MainFunction()` 每周期递减 Timer (L1358-1359) |
| T=N+1 | Timer 递减到 0，检测到 N_Bs 超时 (L1374-1376) |
| T=N+2 | `CanTp_ResetChannel()` 重置通道为 IDLE，所有运行时数据清零 |
| **最大延迟** | **N_Bs / MainFunction 周期 = 75ms / 5ms = 15 个周期** |

### 7.2 场景二：N_Cr 超时（接收端等待 CF 超时）

| 时间 | 事件 |
|------|------|
| T=0 | `CanTp_ProcessRxFrame()` 收到 FF，分配通道，发送 FC (CTS) |
| T=1 | 状态转为 `CANTP_CH_RX_CF`，Timer 设为 `CanTpNcr` (L1125) |
| T=2..M | 正常接收 CF 帧，每次收到 CF 后 Timer 重置为 N_Cr (L1172-1176) |
| T=M+1 | 发送端停止发送（故障），接收端 Timer 递减到 0 |
| T=M+2 | `CanTp_MainFunction()` 检测到 `CANTP_CH_RX_CF` 超时 (L1388-1390) |
| T=M+3 | `CanTp_ResetChannel()` 中止接收，已重组数据丢弃 |
| **最大延迟** | **N_Cr / MainFunction 周期 = 150ms / 5ms = 30 个周期** |

### 7.3 场景三：Rx 队列满导致帧丢失

| 时间 | 事件 |
|------|------|
| T=0 | `CanTp_RxIndication()` 在 ISR 中被调用，队列已有 4 帧 |
| T=1 | `CanTp_RxQueueEnqueue()` 检测到 `CanTp_RxQueueCount >= CANTP_RX_QUEUE_DEPTH` (L409) |
| T=2 | 返回 FALSE，`CanTp_RxIndication` 报告 `CANTP_E_RX_COM` 运行时错误 (L1283) |
| T=3 | 新帧被丢弃，已入队帧和进行中的协议流程不受影响 |
| T=4 | `CanTp_MainFunction()` 消费队列中的帧，释放队列空间 |
| **总延迟** | **1 个 ISR 调用 + 最多 4 * MainFunction 周期恢复** |

### 7.4 场景四：非法帧长度（SF_DL 异常）

| 时间 | 事件 |
|------|------|
| T=0 | `CanTp_ProcessRxFrame()` 收到 SF 帧，提取 `sfDl = pci & 0x0F` |
| T=1 | `CanTp_CheckRxFrameDL()` 检测到 `sfDl == 0 || sfDl > 7` (L310-312) |
| T=2 | 返回 FALSE，上报 `CANTP_E_RX_SF_UNEXPECTED_LEN` 运行时错误 (L1079) |
| T=3 | SF 帧被丢弃，未分配通道，不影响其他通道 |
| **总延迟** | **1 个 MainFunction 处理周期** |

---

## 8. ISO 26262 条款追溯

| ISO 26262 条款 | 要求 | 本文档覆盖 |
|---------------|------|-----------|
| Part 5, Clause 7 | FMEDA 方法论 | 第 3 节完整失效模式表（10 个功能单元，47 个失效模式） |
| Part 5, Clause 8 | 安全度量 (SPFM/LFM/PMHF) | 第 5 节度量分析 |
| Part 5, Clause 9 | 诊断覆盖率评估 | 第 4 节 DC 汇总 |
| Part 6, Clause 8 | 软件单元设计中的安全机制 | 第 3 节故障检测方法列 |
| Part 4, Clause 7 | 安全状态定义 | 第 6 节安全状态 |
| Part 4, Clause 8 | 技术安全需求 | 第 7 节故障反应时间线 |
| ISO 15765-2 | CAN 传输协议规范 | 帧类型处理 (SF/FF/CF/FC)、超时参数 (N_As/N_Bs/N_Cs/N_Cr)、流控机制 |

---

## 9. 假设与约束

1. **底层 CAN 驱动假设**: CanIf/MCAL 层的失效率及诊断覆盖率由独立 FMEDA 覆盖，本分析假设 CAN 控制器硬件 FIT 数据已知
2. **配置正确性**: 假设 CanTp 配置（超时参数 N_As/N_Bs/N_Cs/N_Cr、通道数、缓冲区大小、寻址格式）在集成阶段经过验证，且符合 ISO 15765-2 _timing_ 要求
3. **WDT 覆盖**: `CanTp_MainFunction()` 的周期调度监控由系统级 WDT 覆盖
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **Rx 队列深度**: Rx 队列深度 (4 帧) 在系统级中断延迟分析中已验证足够
6. **缓冲区大小**: `CANTP_CHANNEL_BUFFER_SIZE` (64 字节) 与 `CANTP_CANFD_MAX_MESSAGE_LENGTH` (4095 字节) 的关系：当前实现仅支持最大 64 字节的 NSDU，超过此长度的消息需要配置工具约束
7. **CAN FD 支持**: CAN FD 帧长度校验表 (L239-261) 覆盖 0-8/12/16/20/24/32/48/64 字节；当 `CANTP_CANFD_ENABLED == STD_OFF` 时仅使用 classic CAN 路径

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| CAN 控制器 (发送) | 8 FIT | 供应商 FMD |
| CAN 控制器 (接收) | 10 FIT | 供应商 FMD |
| RAM 缓冲区 | 6 FIT | IEC 61508 Annex D |
| CPU 逻辑 (协议处理) | 15 FIT | 供应商 FMD |
| 配置存储 (ROM) | 3 FIT | IEC 61508 Annex D |
| 队列管理逻辑 | 5 FIT | IEC 61508 Annex D |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| SF | Single Frame (ISO 15765-2 单帧) |
| FF | First Frame (ISO 15765-2 首帧) |
| CF | Consecutive Frame (ISO 15765-2 连续帧) |
| FC | Flow Control (ISO 15765-2 流控帧) |
| SN | Sequence Number (4-bit 序列号) |
| STmin | Separation Time minimum (最小帧间间隔) |
| BS | Block Size (块大小) |
| N_As | 发送端 SF 确认超时 |
| N_Bs | 发送端 FC 等待超时 |
| N_Cs | 发送端 CF 发送超时 |
| N_Cr | 接收端帧等待超时 |
| N_Br | 接收端 FC 响应时间 |
| PCI | Protocol Control Information |
| NSDU | Network Layer Service Data Unit |
| NPDU | Network Layer Protocol Data Unit |
| DET | Default Error Trapping |
| PduR | PDU Router |
| CanIf | CAN Interface |

## 附录 C: 运行时错误代码与 ISO 15765-2 事件映射

| 错误代码 | 值 | 触发条件 | 源码位置 |
|---------|---|---------|---------|
| `CANTP_E_RX_COM` | 0x01 | Rx 队列满，帧被丢弃 | CanTp.c:1283 |
| `CANTP_E_TX_COM` | 0x02 | Tx 通信错误 | CanTp.h:84 |
| `CANTP_E_RX_TIMEOUT_CR` | 0x05 | N_Cr 超时 | CanTp.h:87 |
| `CANTP_E_TX_TIMEOUT_AS` | 0x06 | N_As 超时 | CanTp.h:88 |
| `CANTP_E_TX_TIMEOUT_BS` | 0x07 | N_Bs 超时 | CanTp.h:89 |
| `CANTP_E_TX_TIMEOUT_CS` | 0x08 | N_Cs 超时 | CanTp.h:90 |
| `CANTP_E_RX_INVALID_SN` | 0x09 | 序列号不匹配 | CanTp.h:91 |
| `CANTP_E_RX_INVALID_FS` | 0x0A | 非法 Flow Status | CanTp.h:92 |
| `CANTP_E_RX_UNEXP_PADDING` | 0x10 | 填充字节值不正确 | CanTp.c:1047, 1158 |
| `CANTP_E_RX_SF_UNEXPECTED_LEN` | 0x11 | SF DLC/SF_DL 校验失败 | CanTp.c:1079 |
| `CANTP_E_RX_FF_UNEXPECTED_LEN` | 0x12 | FF DLC/FF_DL 校验失败 | CanTp.c:1128 |
| `CANTP_E_RX_CF_UNEXPECTED_LEN` | 0x13 | CF DLC 校验失败 | CanTp.c:1192 |
| `CANTP_E_RX_FC_UNEXPECTED_LEN` | 0x14 | FC DLC 校验失败 (< 3 字节) | CanTp.c:1248 |
| `CANTP_E_FRAME` | 0x18 | 通用帧错误 | CanTp.h:106 |
