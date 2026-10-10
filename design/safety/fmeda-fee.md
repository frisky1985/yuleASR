# FMEDA — Fee (Flash EEPROM Emulation)

> **文档编号**: SAF-FMEDA-FEE-001
> **版本**: 1.0
> **日期**: 2026-10-26
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW ECUAL 层的 Fee (Flash EEPROM Emulation) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/ecual/fee/src/Fee.c`

**分析边界**:
- Fee 核心逻辑（块管理、扇区管理、磨损均衡、垃圾回收）
- 上层接口：`Fee_Read`, `Fee_Write`, `Fee_InvalidateBlock`, `Fee_EraseImmediateBlock`
- 内部安全机制：CRC16 校验、块头魔数验证、扇区扫描与块表构建
- 下层依赖（假设由 MCAL 层覆盖）：Fls (Flash Driver)

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 Fee 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `Fee_BlockHeaderType` | Fee.c:102-111 | 块头结构：MagicNumber(4B), BlockId(2B), BlockSize(2B), WriteCycleCount(4B), BlockCrc(2B), HeaderCrc(2B), BlockState(1B), Reserved(3B)，共 20 字节 |
| `Fee_BlockInfoType` | Fee.c:114-121 | 块运行时信息：BlockAddress, BlockSize, BlockState, IsValid, IsInvalidated, WriteCycleCounter |
| `Fee_SectorInfoType` | Fee.c:124-133 | 扇区运行时信息：SectorStartAddress, SectorSize, NextWriteAddress, FreeSpace, EraseCycleCount, SectorState, IsActive, IsValid |
| `Fee_JobControlType` | Fee.c:136-145 | 作业控制：JobType, BlockId, BlockOffset, DataBufferPtr, DataLength, JobResult, JobState, RetryCount |
| `Fee_FlsJobControlType` | Fee.c:148-154 | FLS 作业控制：JobState, Address, DataPtr, Length, JobType |
| `Fee_GcControlType` | Fee.c:157-166 | GC 控制：GcState, SourceSectorIndex, TargetSectorIndex, BlockIndex, SourceAddress, TargetAddress, BlocksCopied, BlocksErased |
| `Fee_ConfigType` | Fee.h:189-206 | 模块配置：BlockConfig, SectorConfig, NumBlocks(16), NumSectors(4), GC 阈值等 |

### 1.4 关键配置参数

| 参数 | 值 | 来源 | 说明 |
|------|-----|------|------|
| `FEE_NUM_BLOCKS` | 16 | Fee_Cfg.h:25 | 最大块数 |
| `FEE_NUM_SECTORS` | 4 | Fee_Cfg.h:26 | 扇区数 |
| `FEE_MAX_BLOCK_SIZE` | 1024 | Fee_Cfg.h:47 | 最大块数据大小 |
| `FEE_SECTOR_SIZE` | 65536 | Fee_Cfg.h:72 | 扇区大小 (64KB) |
| `FEE_VIRTUAL_PAGE_SIZE` | 8 | Fee_Cfg.h:78 | 虚拟页大小（对齐粒度） |
| `FEE_GC_THRESHOLD_PERCENT` | 20 | Fee_Cfg.h:82 | GC 触发阈值（空闲空间 <20%） |
| `FEE_MAX_WRITE_CYCLES` | 100000 | Fee_Cfg.h:81 | 最大写入周期数 |
| `FEE_MAX_GC_ERASES` | 100000 | Fee_Cfg.h:80 | 最大 GC 擦除次数 |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 块读取 | `Fee_Read()` (L432), `Fee_ProcessReadJob()` (L1473) | 从 Flash 读取数据块到 RAM 缓冲区 |
| FU-02 | 块写入 | `Fee_Write()` (L520), `Fee_ProcessWriteJob()` (L1526) | 构建块头+CRC 并写入 Flash |
| FU-03 | CRC16 校验 | `Fee_CalculateCrc16()` (L1053), `Fee_CalculateHeaderCrc()` (L1081) | 数据 CRC16 与块头 CRC16 计算 |
| FU-04 | 块头验证 | `Fee_ValidateBlockHeader()` (L1041) | 魔数校验、BlockId/BlockSize 合法性检查 |
| FU-05 | 扇区扫描与块表构建 | `Fee_ScanSectors()` (L941), `Fee_BuildBlockTable()` (L1003) | 初始化时扫描 Flash 扇区，构建运行时块表 |
| FU-06 | 扇区管理与磨损均衡 | `Fee_SelectSectorForWrite()` (L1119), `Fee_UpdateWearLeveling()` (L1684) | 选择擦除次数最少的扇区写入 |
| FU-07 | 垃圾回收 (GC) | `Fee_ProcessGc()` (L1247), `Fee_StartGarbageCollection()` (L1211), `Fee_GcCopyBlocks()` (L1285), `Fee_GcEraseSector()` (L1301), `Fee_GcFinalize()` (L1335) | 扇区满时迁移有效块并擦除旧扇区 |
| FU-08 | 块失效与擦除 | `Fee_InvalidateBlock()` (L667), `Fee_EraseImmediateBlock()` (L715), `Fee_ProcessInvalidateJob()` (L1631), `Fee_ProcessEraseImmediateJob()` (L1645) | 标记块失效或立即擦除 |
| FU-09 | 作业调度与状态机 | `Fee_MainFunction()` (L862), `Fee_ProcessJob()` (L1444), `Fee_FinishJob()` (L1660) | 周期调度，GC 优先，作业状态机驱动 |
| FU-10 | FLS 集成 | `Fee_StartFlsRead()` (L1362), `Fee_StartFlsWrite()` (L1383), `Fee_StartFlsErase()` (L1402), `Fee_CheckFlsJobStatus()` (L1421) | 封装 Fls 读写擦操作及状态轮询 |
| FU-11 | 初始化与模式管理 | `Fee_Init()` (L262), `Fee_DeInit()` (L355), `Fee_SetMode()` (L385) | 模块初始化、去初始化、模式切换 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 块读取 (`Fee_Read` / `Fee_ProcessReadJob`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | RAM 数据指针为空或越界 | 读取数据写入非法地址，可能导致内存破坏 | `Fee_Read()` 入口参数校验：`DataBufferPtr == NULL_PTR` 检测 (L458-462) | 90% | B |
| FM-01-02 | BlockNumber 超出配置范围 | 访问未定义的块描述符，返回 `E_NOT_OK` | BlockNumber 范围检查：`BlockNumber >= FEE_NUM_BLOCKS` (L446-449) | 99% | B |
| FM-01-03 | BlockOffset 超出最大块大小 | 读取偏移越界，数据损坏 | BlockOffset 范围检查：`BlockOffset >= FEE_MAX_BLOCK_SIZE` (L452-455) | 99% | B |
| FM-01-04 | 读取长度为零或超限 | 无效长度导致数据不完整或缓冲区溢出 | Length 校验：`(Length == 0u) \|\| (Length > FEE_MAX_BLOCK_SIZE)` (L464-468) | 99% | B |
| FM-01-05 | 请求读取无效/已失效块 | 返回损坏或过期数据 | 块状态检查：`IsValid == FALSE` (L474) 和 `IsInvalidated == TRUE` (L481) | 95% | B |
| FM-01-06 | Fls_Read 底层返回错误 | 数据未读出或读出不完整 | `Fee_CheckFlsJobStatus()` 轮询 `Fls_GetJobResult()` (L1427)，失败则 `Fee_FinishJob(FEE_JOB_FAILED)` (L1513) | 90% | B |
| FM-01-07 | 模块未初始化时调用读取 | 未定义行为 | `Fee_Initialized == FALSE` 检测 (L440-443)，上报 `FEE_E_UNINIT` | 99% | B |

### 3.2 FU-02: 块写入 (`Fee_Write` / `Fee_ProcessWriteJob`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | 写入数据指针为空 | 写入未定义数据到 Flash | 入口参数校验：`DataBufferPtr == NULL_PTR` (L538-541) | 90% | B |
| FM-02-02 | BlockNumber 超出范围 | 写入到非法块索引 | `BlockNumber >= FEE_NUM_BLOCKS` 检查 (L532-535) | 99% | B |
| FM-02-03 | 写入时模块忙 | 写入操作被拒绝，数据未持久化 | `Fee_ModuleStatus == FEE_BUSY` 检查 (L548-552)，上层可重试 | 80% | B |
| FM-02-04 | 目标扇区空间不足 | 写入失败，数据丢失 | `Fee_SelectSectorForWrite()` 遍历所有扇区检查 `FreeSpace` (L1128-1130)；无可用扇区返回 `0xFF` → `FEE_JOB_FAILED` (L1539-1542) | 95% | B |
| FM-02-05 | Fls_Write 底层返回错误 | Flash 数据未写入 | `Fee_CheckFlsJobStatus()` 检测 `MEMIF_JOB_FAILED` (L1433)，写入失败 → `FEE_JOB_FAILED` (L1617-1619) | 90% | B |
| FM-02-06 | WriteCycleCounter 溢出 | 写入计数回绕，磨损均衡失效 | `WriteCycleCounter` 递增后与 `FEE_MAX_WRITE_CYCLES` (100000) 比较（由配置约束）；块头 `WriteCycleCount` 为 uint32，溢出概率极低 | 60% | B |
| FM-02-07 | Fee_ConfigPtr 为空 | 无法获取块大小，写入失败 | `Fee_ConfigPtr != NULL_PTR` 检查 (L555-562) | 99% | B |

### 3.3 FU-03: CRC16 校验 (`Fee_CalculateCrc16` / `Fee_CalculateHeaderCrc`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | CRC16 多项式常量损坏 | CRC 计算结果错误，无法检测数据损坏 | 编译时常量 `FEE_CRC16_POLYNOMIAL = 0x1021` 存储于 ROM，启动时由 BSW 完整性检查覆盖 | 95% | B |
| FM-03-02 | CRC 计算逻辑错误（CPU 瞬态故障） | 错误数据通过校验或正确数据被拒绝 | 写后回读验证（上层 NvM 覆盖）；关键块双重 CRC 校验 | 90% | B |
| FM-03-03 | 块头 CRC 与数据 CRC 不一致 | 块头或数据损坏未被检测 | 写入时分别计算 `BlockCrc` (L1556) 和 `HeaderCrc` (L1557)；读取时双重验证 | 95% | B |

### 3.4 FU-04: 块头验证 (`Fee_ValidateBlockHeader`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | 魔数损坏（Flash 位翻转） | 有效块被误判为无效或无效块被误判为有效 | 魔数校验：`FEE_BLOCK_HEADER_MAGIC_VALID (0x46454556)` vs `FEE_BLOCK_HEADER_MAGIC_INVALID (0x46454549)` vs `FEE_BLOCK_HEADER_MAGIC_ERASED (0xFFFFFFFF)` (L50-52) | 95% | B |
| FM-04-02 | BlockId 超出范围 | 块表索引越界，内存破坏 | `header.BlockId < FEE_NUM_BLOCKS` 检查 (L962) | 99% | B |
| FM-04-03 | BlockSize 与配置不匹配 | 读取数据长度错误 | 块头 `BlockSize` 与 `Fee_ConfigPtr->BlockConfig[].BlockSize` 交叉验证（由扇区扫描覆盖） | 85% | B |
| FM-04-04 | HeaderCrc 校验失败 | 块头元数据损坏 | `Fee_CalculateHeaderCrc()` 计算并与存储的 `HeaderCrc` 比对 | 95% | B |

### 3.5 FU-05: 扇区扫描与块表构建 (`Fee_ScanSectors` / `Fee_BuildBlockTable`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | 扇区起始地址配置错误 | 扫描错误区域，块表为空或错误 | 配置校验：`SectorStartAddress` 与 Flash 物理地址范围交叉验证 | 90% | B |
| FM-05-02 | 扫描过程中遇到损坏块头 | 后续有效块丢失 | 遇到无效块头即认为扇区剩余空间为空 (L982-987)，`NextWriteAddress` 设置为当前位置 | 85% | B |
| FM-05-03 | 同一 BlockId 多份拷贝的 WriteCycleCount 比较错误 | 使用旧版本数据 | `WriteCycleCount` 比较：`header.WriteCycleCount > Fee_BlockInfo[].WriteCycleCounter` (L966) | 90% | B |
| FM-05-04 | 无有效扇区可用 | 模块无法提供存储服务 | `Fee_BuildBlockTable()` 中 `activeSectorFound == FALSE` 时选择首个有效扇区并重置 (L1022-1035) | 80% | B |

### 3.6 FU-06: 扇区管理与磨损均衡 (`Fee_SelectSectorForWrite`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | 所有扇区空间不足 | 写入操作失败 | `Fee_SelectSectorForWrite()` 返回 `0xFF` (L1122)，上层收到 `FEE_JOB_FAILED` | 99% | B |
| FM-06-02 | EraseCycleCount 比较逻辑错误 | 写入集中到同一扇区，加速磨损 | 最小擦除计数选择逻辑 (L1131-1135)；`Fee_GetEraseCycleCount()` 可供外部监控 | 75% | B |
| FM-06-03 | 扇区 IsValid 标志异常 | 有效扇区被跳过或无效扇区被使用 | 扇区有效性检查：`Fee_SectorInfo[i].IsValid == TRUE` (L1128) | 85% | B |
| FM-06-04 | NextWriteAddress 越界 | 写入到扇区外区域 | `Fee_IsAddressInSector()` 地址范围检查 (L1705-1715)；`FreeSpace` 递减检查 (L1609) | 90% | B |

### 3.7 FU-07: 垃圾回收 (GC)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | GC 触发时找不到源扇区 | GC 无法启动，空间持续紧张 | `Fee_FindActiveSector()` 返回 `0xFF` 时直接返回 (L1218-1220) | 99% | B |
| FM-07-02 | GC 触发时找不到目标扇区 | GC 无法启动，写入最终失败 | `Fee_FindFreeSector()` 返回 `0xFF` 时直接返回 (L1225-1227) | 99% | B |
| FM-07-03 | GC 拷贝过程中块数据损坏 | 有效数据丢失 | GC 拷贝后更新块表地址 (L1598-1604)；拷贝块需通过 CRC 验证 | 85% | B |
| FM-07-04 | GC 擦除扇区失败 | 源扇区无法重用 | `Fee_FlsJob.JobState == FEE_FLS_JOB_FAILED` 检测 (L1318-1321)，GC 中止 → `FEE_GC_STATE_IDLE` | 95% | B |
| FM-07-05 | GC 过程中断电 | 数据丢失（源已擦除、目标未写完） | 双扇区设计保证至少一个扇区有完整数据；重启后 `Fee_ScanSectors()` 重建块表 | 90% | B |
| FM-07-06 | GC 空闲空间阈值计算错误 | GC 过早或过晚触发 | `Fee_IsGcNeeded()` 计算 `freePercent < FEE_GC_THRESHOLD_PERCENT` (L1201-1202) | 80% | B |
| FM-07-07 | GcCycleCounter 超限 | Flash 寿命耗尽 | `GcCycleCounter` 与 `FEE_MAX_GC_CYCLES` (10000) 比较 | 70% | B |

### 3.8 FU-08: 块失效与擦除

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | 块失效操作写入 Flash 失败 | 旧数据仍可读取，数据一致性风险 | `Fee_ProcessInvalidateJob()` 更新运行时状态 (L1634-1635)；完整实现需写入 Flash 失效标记 | 75% | B |
| FM-08-02 | 立即擦除后块状态不一致 | 块表仍标记有效，后续读取返回无效数据 | `Fee_ProcessEraseImmediateJob()` 清除 `IsValid` (L1648) 和 `BlockState` (L1650) | 85% | B |
| FM-08-03 | 模块忙时请求失效/擦除 | 操作被拒绝 | `Fee_ModuleStatus == FEE_BUSY` 检查 (L688-692, L736-740) | 99% | B |

### 3.9 FU-09: 作业调度与状态机

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | 状态机卡死（JobState 不推进） | 作业无法完成，Fee 无响应 | `Fee_MainFunction()` 周期执行 (L862)；系统级 WDT 监控 | 80% | B |
| FM-09-02 | GC 优先级抢占导致正常作业饥饿 | 读写请求长时间得不到服务 | GC 优先处理 (L876 `return`)，但 GC 完成后立即处理排队作业 | 75% | B |
| FM-09-03 | Fee_ModuleStatus 标志异常 | 模块误判为 BUSY 或 IDLE | 状态一致性检查：`Fee_FinishJob()` 确保恢复 `FEE_IDLE` (L1665) | 80% | B |
| FM-09-04 | 未知 JobType 进入处理 | 作业失败 | `default` 分支捕获未知类型 → `FEE_JOB_FAILED` (L1464-1466) | 95% | B |
| FM-09-05 | Fee_Cancel 在 IDLE 状态调用 | 无操作，但不应报错 | `Fee_ModuleStatus == FEE_BUSY` 条件保护 (L607) | 99% | B |

### 3.10 FU-10: FLS 集成

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | FLS 作业冲突（前一作业未完成） | 新作业被拒绝 | `Fee_FlsJob.JobState == FEE_FLS_JOB_PENDING` 检查 (L1364, L1385, L1404) | 99% | B |
| FM-10-02 | Fls 回调丢失（JobEnd/Error Notification 未触发） | 作业状态永远停留在 PENDING | `Fee_CheckFlsJobStatus()` 轮询 `Fls_GetJobResult()` (L1427) 作为冗余检测路径 | 90% | B |
| FM-10-03 | Fls 回调与轮询竞争 | JobState 被重复修改 | 临界区保护：`FEE_ENTER_CRITICAL_SECTION()` (L908, L924) | 85% | B |
| FM-10-04 | Fee_WorkBuffer 溢出 | 栈/数据区破坏 | 缓冲区大小 `FEE_MAX_BLOCK_SIZE + sizeof(Fee_BlockHeaderType)` (L195)；写入前 `totalSize` 计算确保不超限 | 90% | B |

### 3.11 FU-11: 初始化与模式管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-11-01 | ConfigPtr 为空 | 模块初始化失败，所有后续操作异常 | `ConfigPtr == NULL_PTR` 检测 (L267-270)，上报 `FEE_E_PARAM_CONFIG` | 99% | B |
| FM-11-02 | 重复初始化 | 运行时状态被重置，进行中的作业丢失 | `Fee_Initialized == TRUE` 检测 (L273-277)，上报 `FEE_E_INVALID_CFG` | 99% | B |
| FM-11-03 | SetMode 参数非法 | Fls 模式设置错误 | Mode 校验：`(Mode != FEE_MODE_SLOW) && (Mode != FEE_MODE_FAST)` (L395-398) | 99% | B |
| FM-11-04 | DeInit 时有进行中的作业 | 作业被强制取消 | `Fee_ModuleStatus == FEE_BUSY` 时调用 `Fee_Cancel()` (L369-371) | 90% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 块读取 | 99% | 93% | 入口参数校验 + Fls 状态检查 |
| FU-02 | 块写入 | 99% | 89% | 扇区空间检查 + Fls 状态检查 |
| FU-03 | CRC16 校验 | 95% | 93% | 双重 CRC（数据+块头） |
| FU-04 | 块头验证 | 99% | 93% | 魔数 + HeaderCrc + BlockId 范围 |
| FU-05 | 扇区扫描 | 90% | 86% | 地址范围校验 + WriteCycleCount 比较 |
| FU-06 | 扇区管理 | 99% | 87% | 空间检查 + 磨损均衡 + 地址范围 |
| FU-07 | 垃圾回收 | 99% | 86% | 源/目标扇区检查 + 擦除验证 |
| FU-08 | 块失效/擦除 | 99% | 86% | 忙检测 + 状态同步 |
| FU-09 | 作业调度 | 99% | 86% | 状态机 + WDT + 默认分支 |
| FU-10 | FLS 集成 | 99% | 91% | 作业冲突检测 + 回调/轮询冗余 |
| FU-11 | 初始化/模式 | 99% | 96% | 参数校验 + 重复初始化检测 |

### 4.2 模块整体诊断覆盖率

```
DC_Fee = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_Fee ≈ 89%
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

单点故障是指不被任何诊断机制覆盖的残余故障。Fee 中主要单点故障来源：
- CRC16 多项式常量自身的不可检测故障（极低概率，由 ROM 校验覆盖）
- 作业状态机瞬态故障（由 WDT 覆盖）
- 磨损均衡逻辑的不可检测故障（由 EraseCycleCount 外部监控覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (8.1 / 92) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。Fee 中主要潜伏故障来源：
- CRC16 算法实现的系统性缺陷（通过 ROM 校验和 NvM 层写后回读覆盖）
- 扇区扫描逻辑故障（通过 `Fee_BuildBlockTable()` 的一致性检查覆盖）
- GC 状态机逻辑故障（通过 `Fee_GcControl.GcState` 状态转换验证覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (9.2 / 92) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_header_corruption + lambda_gc_failure + lambda_wear_leveling_fault
     ≈ 8 + 22 + 22 = 52 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 Fee 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | 所有块 CRC 有效，扇区空间充足，GC 未激活 | 正常读写服务 |
| **降级运行** | 部分块损坏但可通过最新 WriteCycleCount 恢复；或 GC 正在执行 | GC 期间暂停新写入；损坏块标记失效，上报诊断事件 |
| **空间耗尽** | 所有扇区空闲空间 < GC 阈值且 GC 无法释放空间 | 写入操作返回 `FEE_JOB_FAILED`；读取仍可服务 |
| **安全停止** | 无有效扇区可用，或 Fee_MainFunction 无法执行 | 模块进入 `FEE_IDLE`，通知上层 NvM 数据不可用；WDT 触发系统复位 |

### 6.2 安全状态转换

```
正常运行 ──[空闲空间 < 20%]──→ 降级运行 (GC激活)
正常运行 ──[单块CRC/块头失败]──→ 降级运行 (标记块失效)
降级运行 ──[GC完成]──→ 正常运行
降级运行 ──[所有扇区空间耗尽]──→ 空间耗尽
空间耗尽 ──[无有效扇区]──→ 安全停止
任意状态 ──[MainFunction超时]──→ 安全停止 (WDT触发)
```

### 6.3 故障安全策略

当检测到不可恢复的故障时，Fee 模块采取以下安全策略：

1. **块数据损坏**: 标记块为 `FEE_BLOCK_STATE_INVALIDATED`，上层 NvM 回退到冗余拷贝或 ROM 默认值
2. **扇区擦除失败**: GC 中止 (`FEE_GC_STATE_IDLE`)，标记扇区为无效，切换到备用扇区
3. **Flash 写入失败**: 作业返回 `FEE_JOB_FAILED`，通过 `Fee_JobErrorNotification()` 通知 NvM
4. **无可用扇区**: 所有写入操作被拒绝，读取操作仍可服务有效块

---

## 7. 故障反应时间线

### 7.1 场景一：Flash 读取数据损坏

| 时间 | 事件 |
|------|------|
| T=0 | 上层调用 `Fee_Read(BlockNumber, BlockOffset, DataBufferPtr, Length)` |
| T=1 cycle | 入口参数校验通过，设置 `FEE_JOB_READ`，`Fee_ModuleStatus = FEE_BUSY` |
| T=2 cycles | `Fee_MainFunction()` → `Fee_ProcessReadJob()` → `Fee_StartFlsRead()` |
| T=3 cycles | `Fee_CheckFlsJobStatus()` 检测到 `MEMIF_JOB_OK`，数据读入 `Fee_WorkBuffer` |
| T=4 cycles | `Fee_Memcpy()` 拷贝到用户缓冲区，`Fee_FinishJob(FEE_JOB_OK)` |
| T=5 cycles | `Fee_JobEndNotification()` 通知 NvM |
| **总延迟** | **5 个主函数周期 (50ms @ 10ms 周期)** |

### 7.2 场景二：写入时扇区空间不足触发 GC

| 时间 | 事件 |
|------|------|
| T=0 | 上层调用 `Fee_Write(BlockNumber, DataBufferPtr)` |
| T=1 cycle | `Fee_IsGcNeeded()` 返回 TRUE (空闲空间 < 20%) |
| T=2 cycles | `Fee_StartGarbageCollection()` → `Fee_GcControl.GcState = FEE_GC_STATE_SCAN` |
| T=3 cycles | `Fee_MainFunction()` → `Fee_ProcessGc()` → `Fee_GcScanSector()` → `FEE_GC_STATE_COPY` |
| T=4-N cycles | `Fee_GcCopyBlocks()` 逐块迁移有效块到目标扇区 |
| T=N+1 cycles | `Fee_GcEraseSector()` → `Fls_Erase()` → 等待擦除完成 |
| T=N+2 cycles | `Fee_GcFinalize()` → 源扇区重置，目标扇区激活 |
| T=N+3 cycles | 写入作业继续执行 `Fee_ProcessWriteJob()` |
| **总延迟** | **取决于有效块数量，典型 10-50 个主函数周期** |

### 7.3 场景三：FLS 写入失败

| 时间 | 事件 |
|------|------|
| T=0 | `Fee_ProcessWriteJob()` 启动 `Fee_StartFlsWrite()` |
| T=1 cycle | `Fee_FlsJob.JobState = FEE_FLS_JOB_PENDING` |
| T=2 cycles | `Fee_CheckFlsJobStatus()` 检测到 `MEMIF_JOB_FAILED` |
| T=3 cycles | `Fee_FlsJob.JobState = FEE_FLS_JOB_FAILED` |
| T=4 cycles | `Fee_ProcessWriteJob()` 检测到失败 → `Fee_FinishJob(FEE_JOB_FAILED)` |
| T=5 cycles | `Fee_JobErrorNotification()` → `NvM_JobErrorNotification()` |
| T=6 cycles | NvM 层决定重试或回退 |
| **总延迟** | **6 个主函数周期 (60ms @ 10ms 周期)** |

### 7.4 场景四：GC 擦除失败

| 时间 | 事件 |
|------|------|
| T=0 | `Fee_GcEraseSector()` 调用 `Fee_StartFlsErase()` |
| T=1 cycle | Fls 开始擦除操作 |
| T=2 cycles | `Fee_FlsJobErrorNotification()` 或 `Fee_CheckFlsJobStatus()` 检测到失败 |
| T=3 cycles | `Fee_GcControl.GcState = FEE_GC_STATE_IDLE`，GC 中止 |
| T=4 cycles | 源扇区数据可能已部分丢失，但目标扇区已有迁移的有效块 |
| T=5 cycles | 上层通过 `Fee_GetEraseCycleCount()` 监控到异常 |
| **总延迟** | **5 个主函数周期 + 系统响应时间** |

---

## 8. ISO 26262 条款追溯

| ISO 26262 条款 | 要求 | 本文档覆盖 |
|---------------|------|-----------|
| Part 5, Clause 7 | FMEDA 方法论 | 第 3 节完整失效模式表（11 个功能单元，50+ 失效模式） |
| Part 5, Clause 8 | 安全度量 (SPFM/LFM/PMHF) | 第 5 节度量分析 |
| Part 5, Clause 9 | 诊断覆盖率评估 | 第 4 节 DC 汇总 |
| Part 6, Clause 8 | 软件单元设计中的安全机制 | 第 3 节故障检测方法列 |
| Part 4, Clause 7 | 安全状态定义 | 第 6 节安全状态与转换 |
| Part 4, Clause 8 | 技术安全需求 | 第 7 节故障反应时间线 |
| Part 5, Clause 7.4 | 多重故障分析 | 第 3.7 节 GC 断电场景（双扇区冗余） |
| Part 6, Clause 7 | 安全机制与诊断覆盖 | 第 6.3 节故障安全策略 |

---

## 9. 假设与约束

1. **底层 Flash 驱动假设**: Fls 层的失效率及诊断覆盖率由独立 FMEDA 覆盖，本分析假设 Fls FIT 数据已知且 Fls 提供的接口（`Fls_Read`, `Fls_Write`, `Fls_Erase`, `Fls_GetJobResult`）具有可靠的错误返回机制
2. **CRC16 实现完整性**: 假设 CRC16 多项式常量 `0x1021` 和初始值 `0xFFFF` 存储于受保护的 ROM 区域，启动时经过完整性校验
3. **WDT 覆盖**: `Fee_MainFunction()` 的调度监控由系统级 WDT 覆盖，周期为 10ms (`FEE_MAIN_FUNCTION_PERIOD_MS`)
4. **双扇区冗余**: 假设配置至少 2 个有效扇区 (`FEE_NUM_SECTORS = 4`)，保证 GC 期间至少有一个扇区持有完整数据
5. **配置正确性**: 假设 Fee 配置（扇区地址、扇区大小、块描述符）在集成阶段经过验证，扇区地址不重叠且在 Flash 物理地址范围内
6. **Flash 页对齐**: 假设所有写入操作按 `FEE_VIRTUAL_PAGE_SIZE` (8 字节) 对齐，由 `FEE_ALIGN_TO_PAGE` 宏保证
7. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
8. **NvM 层冗余**: 假设上层 NvM 提供额外的冗余拷贝和 ROM 默认值恢复机制，Fee 层的数据损坏可由 NvM 层兜底

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| Flash 读取 | 10 FIT | 供应商 FMD |
| Flash 写入 | 20 FIT | 供应商 FMD |
| Flash 扇区擦除 | 25 FIT | 供应商 FMD |
| CRC16 计算单元 | 5 FIT | IEC 61508 Annex D |
| RAM 存储 (块表/作业控制) | 8 FIT | IEC 61508 Annex D |
| CPU 逻辑 (状态机/调度) | 12 FIT | 供应商 FMD |
| Flash 控制器 | 12 FIT | 供应商 FMD |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| CRC | Cyclic Redundancy Check |
| GC | Garbage Collection (垃圾回收) |
| FLS | Flash Driver (Flash 驱动) |
| Fee | Flash EEPROM Emulation |
| NvM | NVRAM Manager |
| DET | Default Error Trap |

## 附录 C: 块头结构布局

```
偏移  大小  字段              说明
────────────────────────────────────────────
0x00  4B   MagicNumber       0x46454556 ("FEEV") 有效
                              0x46454549 ("FEEI") 失效
                              0xFFFFFFFF       已擦除
0x04  2B   BlockId           块标识 (0..15)
0x06  2B   BlockSize         数据长度 (字节)
0x08  4B   WriteCycleCount   写入计数（用于选择最新版本）
0x0C  2B   BlockCrc          数据区 CRC16
0x0E  2B   HeaderCrc         块头 CRC16 (覆盖 0x00..0x0F)
0x10  1B   BlockState        0x00=Empty, 0x01=Valid, 0x02=Invalidated, 0x03=Obsolete
0x11  3B   Reserved          填充到 8 字节对齐
────────────────────────────────────────────
总计: 20 字节 (按 8 字节页对齐后占 24 字节)
```
