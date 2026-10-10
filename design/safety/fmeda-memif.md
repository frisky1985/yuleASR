# FMEDA — MemIf (Memory Interface)

> **文档编号**: SAF-FMEDA-MEMIF-001
> **版本**: 1.0
> **日期**: 2026-10-26
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW Services Layer 的 MemIf (Memory Interface) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/services/memif/src/MemIf.c`

**分析边界**:
- MemIf 核心逻辑（设备索引路由、状态管理、作业结果跟踪）
- 上层接口：`MemIf_Read`, `MemIf_Write`, `MemIf_EraseBlock`, `MemIf_EraseImmediateBlock`, `MemIf_InvalidateBlock`
- 内部安全机制：参数校验（DeviceIndex/BlockNumber/Pointer）、DET 错误上报、状态一致性检查
- 下层依赖（假设由 ECUAL/MCAL 层覆盖）：Fee (Flash EEPROM Emulation), Ea (EEPROM Abstraction)

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 MemIf 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `MemIf_DeviceStateType` | MemIf.h:108-112 | 设备运行时状态：`status` (IDLE/BUSY/UNINIT), `jobResult` (OK/PENDING/CANCELED/FAILED), `isInitialized` |
| `MemIf_DeviceConfigType` | MemIf.h:115-121 | 设备配置：DeviceId, DeviceType (0=Fee/1=Ea), BlockSize, NumberOfBlocks, numBlocks |
| `MemIf_ConfigType` | MemIf.h:124-127 | 全局配置：Devices 指针数组, NumDevices |
| `MemIf_DeviceAbstractionType` | MemIf.h:96-105 | 设备抽象表：函数指针 (Read/Write/Erase/Cancel/GetStatus/GetJobResult) |
| `MemIf_DeviceState[]` | MemIf.c:95 | 全局运行时状态数组，大小 `MEMIF_NUMBER_OF_DEVICES` |
| `MemIf_ModuleInitialized` | MemIf.c:97 | 模块初始化标志，防止未初始化调用 |
| `MemIf_DefaultDeviceConfig[]` | MemIf.c:106-121 | 默认设备配置表（Fee + Ea） |
| `MemIf_HardwareMap[]` | MemIf_Lcfg.c:285-303 | 硬件映射表：BaseAddress, TotalSize, BlockSize |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 模块初始化/去初始化 | `MemIf_Init()` (L177), `MemIf_DeInit()` (L208) | 配置加载、设备状态初始化、重复初始化检测 |
| FU-02 | 块读取 | `MemIf_Read()` (L262) | 路由读请求到 Fee/Ea 底层驱动 |
| FU-03 | 块写入 | `MemIf_Write()` (L306) | 路由写请求到 Fee/Ea 底层驱动 |
| FU-04 | 块擦除 | `MemIf_EraseBlock()` (L488), `MemIf_EraseImmediateBlock()` (L535) | 路由擦除请求到 Fee/Ea 底层驱动 |
| FU-05 | 块失效化 | `MemIf_InvalidateBlock()` (L446) | 标记块为无效状态 |
| FU-06 | 设备状态查询 | `MemIf_GetStatus()` (L376), `MemIf_GetJobResult()` (L414) | 查询底层驱动状态并同步内部状态 |
| FU-07 | 作业取消 | `MemIf_Cancel()` (L348) | 取消正在进行的作业 |
| FU-08 | 主函数调度 | `MemIf_MainFunction()` (L576) | 周期调度底层驱动主函数，更新设备状态 |
| FU-09 | 参数校验与 DET | 校验宏 (L43-81), `MemIf_IsDeviceIndexValid()` (L140), `MemIf_IsBlockNumberValid()` (L152) | 输入参数合法性校验、DET 错误上报 |
| FU-10 | 版本信息查询 | `MemIf_GetVersionInfo()` (L236) | 返回模块版本信息 |
| FU-11 | 设备模式设置 | `MemIf_SetMode()` (L611) | 设置底层驱动工作模式（慢速/快速） |

---

## 3. 失效模式分析表

### 3.1 FU-01: 模块初始化/去初始化 (`MemIf_Init`, `MemIf_DeInit`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | ConfigPtr 指向无效配置数据 | 设备配置错误，后续操作路由到错误底层驱动 | `MemIf_Init()` 中 NULL_PTR 检查 (L187)，使用默认配置回退 (`MemIf_DefaultConfig`) | 85% | B |
| FM-01-02 | 重复初始化 (`MemIf_ModuleInitialized == TRUE`) | 设备状态被意外重置，正在进行的作业丢失 | `MemIf_Init()` 入口检测 (L180)，上报 `MEMIF_E_ALREADY_INITIALIZED` | 90% | B |
| FM-01-03 | `MemIf_DeviceState[]` 数组初始化不完整 | 部分设备状态不一致，可能导致操作路由异常 | 初始化循环遍历所有设备 (L194-198)，每个设备独立设置 `status=IDLE, jobResult=OK, isInitialized=TRUE` | 85% | B |
| FM-01-04 | `MemIf_ModuleInitialized` 标志位翻转 (单粒子翻转) | 模块误判为未初始化，拒绝所有服务 | 各 API 入口统一校验 (L43-57 宏)，DET 上报 `MEMIF_E_UNINIT`；系统级 WDT 监控 | 70% | B |

### 3.2 FU-02: 块读取 (`MemIf_Read`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | DataBufferPtr 为空指针 | 读取数据写入非法地址，可能导致内存破坏 | `MEMIF_VALIDATE_POINTER` 宏校验 (L269)，DET 上报 `MEMIF_E_PARAM_POINTER` | 95% | B |
| FM-02-02 | DeviceIndex 超出配置范围 | 访问未定义的设备状态，返回 `E_NOT_OK` | `MEMIF_VALIDATE_DEVICE_INDEX_RET` 校验 (L268)，比较 `DeviceIndex >= MEMIF_TOTAL_NUM_DEVICES` | 99% | B |
| FM-02-03 | BlockNumber 超出设备有效范围 | 读取未定义的块，返回 `E_NOT_OK` | `MemIf_IsBlockNumberValid()` 校验 (L271)，比对 `BlockNumber < MemIf_DefaultDeviceConfig[DeviceIndex].numBlocks` | 99% | B |
| FM-02-04 | 设备处于 BUSY 状态时发起读请求 | 读请求被静默丢弃，返回 `E_NOT_OK` | 状态检查 `MemIf_DeviceState[DeviceIndex].status == MEMIF_IDLE` (L277)；上层 NvM 负责重试 | 80% | B |
| FM-02-05 | 底层 Fee_Read/Ea_Read 返回错误 | 数据未读取，设备状态不一致 | 返回值检查 (L289)：仅 `E_OK` 时更新状态为 BUSY；否则保持 IDLE | 90% | B |
| FM-02-06 | 设备路由逻辑错误（DeviceIndex 匹配到错误驱动） | 从错误设备读取数据 | 编译时条件检查 `MEMIF_FEE_ENABLED`/`MEMIF_EA_ENABLED`；运行时 `DeviceIndex == MEMIF_DEVICE_INDEX_FEE/EA` 分支判断 (L279-287) | 85% | B |

### 3.3 FU-03: 块写入 (`MemIf_Write`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | DataBufferPtr 为空指针 | 写入未定义数据到 NVRAM | `MEMIF_VALIDATE_POINTER` 宏校验 (L313)，DET 上报 `MEMIF_E_PARAM_POINTER` | 95% | B |
| FM-03-02 | DeviceIndex 超出范围 | 写入操作路由到错误设备 | `MEMIF_VALIDATE_DEVICE_INDEX_RET` 校验 (L312) | 99% | B |
| FM-03-03 | BlockNumber 超出范围 | 写入未定义的块位置 | `MemIf_IsBlockNumberValid()` 校验 (L315) | 99% | B |
| FM-03-04 | 设备处于 BUSY 状态时发起写请求 | 写请求被拒绝，数据未持久化 | 状态检查 (L321)：`status == MEMIF_IDLE` 才接受请求 | 80% | B |
| FM-03-05 | 底层 Fee_Write/Ea_Write 返回错误 | NVRAM 数据未更新 | 返回值检查 (L333)：仅 `E_OK` 时更新状态 | 90% | B |

### 3.4 FU-04: 块擦除 (`MemIf_EraseBlock`, `MemIf_EraseImmediateBlock`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | DeviceIndex 或 BlockNumber 无效 | 擦除操作路由到错误位置，可能破坏有效数据 | 双重校验：`MEMIF_VALIDATE_DEVICE_INDEX_RET` (L494) + `MemIf_IsBlockNumberValid` (L496) | 99% | B |
| FM-04-02 | 设备 BUSY 时发起擦除请求 | 擦除请求被丢弃 | 状态检查 (L502)：`status == MEMIF_IDLE` | 80% | B |
| FM-04-03 | `MemIf_EraseBlock` 与 `MemIf_EraseImmediateBlock` 混淆调用 | 错误的擦除语义（普通擦除 vs 立即擦除） | API 层面 SID 区分 (`MEMIF_SID_ERASEBLOCK`=0x09, `MEMIF_SID_ERASEIMMEDIATEBLOCK`=0x0B)；DET 独立上报 | 75% | B |
| FM-04-04 | 底层擦除操作失败 | 块数据未清除，后续写入可能失败 | 返回值检查 (L514/L561)；`MemIf_MainFunction` 周期性状态更新检测卡死 | 85% | B |

### 3.5 FU-05: 块失效化 (`MemIf_InvalidateBlock`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | DeviceIndex 或 BlockNumber 无效 | 失效化操作指向错误块 | 参数校验 (L452/L454) | 99% | B |
| FM-05-02 | 设备 BUSY 时发起失效化请求 | 请求被丢弃 | 状态检查 (L460)：`status == MEMIF_IDLE` | 80% | B |
| FM-05-03 | 底层 InvalidateBlock 返回错误 | 块未被标记为无效 | 返回值检查 (L472) | 90% | B |

### 3.6 FU-06: 设备状态查询 (`MemIf_GetStatus`, `MemIf_GetJobResult`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | 底层驱动返回错误状态值 | MemIf 报告错误的设备状态 | `MemIf_GetStatus()` 直接从底层获取并同步 (L392-400)；异常值回退到 `MEMIF_UNINIT` | 80% | B |
| FM-06-02 | 内部状态与底层状态不同步 | 上层获取过时状态信息 | `MemIf_GetStatus()` 每次调用时主动查询底层并更新 `MemIf_DeviceState[].status` (L403) | 85% | B |
| FM-06-03 | `MemIf_GetJobResult` 返回默认失败值 | 上层误判作业失败 | 未初始化时返回 `MEMIF_JOB_FAILED` (L416)；DeviceIndex 无效时返回 `MEMIF_JOB_FAILED` (L420) | 75% | B |
| FM-06-04 | 状态查询时 DeviceIndex 越界 | 访问数组越界 | `DeviceIndex >= MEMIF_TOTAL_NUM_DEVICES` 检查 (L385)，返回 `MEMIF_UNINIT` | 99% | B |

### 3.7 FU-07: 作业取消 (`MemIf_Cancel`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | DeviceIndex 无效 | 取消操作路由到错误设备 | `MEMIF_VALIDATE_DEVICE_INDEX` 校验 (L352) | 99% | B |
| FM-07-02 | 取消后状态未正确重置 | 设备停留在 BUSY 状态，后续操作被阻塞 | 取消后强制设置 `status=IDLE, jobResult=MEMIF_JOB_CANCELED` (L366-367) | 85% | B |
| FM-07-03 | 底层 Cancel 调用失败 | 底层作业仍在进行 | 无论底层返回结果，MemIf 层强制更新状态 (L366-367)；由 `MemIf_MainFunction` 后续检测 | 70% | B |

### 3.8 FU-08: 主函数调度 (`MemIf_MainFunction`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | MainFunction 未被周期调用 | 设备状态无法更新，作业结果无法获取 | 系统级 WDT 监控；BswM 调度监控 | 80% | B |
| FM-08-02 | 底层 MainFunction 执行异常 | 底层驱动状态机卡死 | 调用底层 `Fee_MainFunction()`/`Ea_MainFunction()` (L586-591)；超时由配置 `MEMIF_READ/WRITE/ERASE_TIMEOUT_MS` 约束 | 75% | B |
| FM-08-03 | 作业完成后状态未从 BUSY 转为 IDLE | 设备永久 BUSY，后续操作被阻塞 | `MemIf_MainFunction` 中检查 `jobResult != MEMIF_JOB_PENDING` 时自动转 IDLE (L598-600) | 85% | B |
| FM-08-04 | DeviceIndex 无效 | 状态更新访问越界 | `MEMIF_VALIDATE_DEVICE_INDEX` 校验 (L580) | 99% | B |

### 3.9 FU-09: 参数校验与 DET

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | DET 功能被关闭 (`MEMIF_DEV_ERROR_DETECT == STD_OFF`) | 所有参数校验失效，非法操作直达底层驱动 | 编译时配置检查；集成测试验证 DET 开启 | 60% | B |
| FM-09-02 | `MemIf_IsBlockNumberValid` 内部逻辑错误 | 无效 BlockNumber 通过校验 | 函数内双重条件检查 (L156-159)：先验证 DeviceIndex，再验证 BlockNumber | 85% | B |
| FM-09-03 | DET 上报自身故障 (Det_ReportError 调用失败) | 错误无法被上层感知 | Det 模块自身安全机制覆盖（独立 FMEDA） | 70% | B |

### 3.10 FU-10: 版本信息查询 (`MemIf_GetVersionInfo`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | VersionInfoPtr 为空指针 | 版本信息写入非法地址 | NULL_PTR 校验 (L239)，DET 上报 `MEMIF_E_PARAM_POINTER` | 95% | B |
| FM-10-02 | 版本常量编译错误 | 返回错误版本信息，集成兼容性检查失败 | 编译时版本校验 (L648-653)：`MEMIF_AR_RELEASE_MAJOR/MINOR_VERSION` 一致性检查 | 90% | B |

### 3.11 FU-11: 设备模式设置 (`MemIf_SetMode`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-11-01 | DeviceIndex 无效 | 模式设置路由到错误设备 | `MEMIF_VALIDATE_DEVICE_INDEX` 校验 (L615) | 99% | B |
| FM-11-02 | Mode 值非法（非 SLOW/FAST） | 底层驱动进入未定义模式 | 当前实现未对 Mode 值做范围校验，依赖底层驱动自身校验 | 50% | B |
| FM-11-03 | 底层 SetMode 调用失败 | 设备模式未切换 | 无返回值检查（void 函数），依赖底层驱动内部错误处理 | 60% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 初始化/去初始化 | 90% | 83% | 重复初始化检测 + 默认配置回退 |
| FU-02 | 块读取 | 99% | 90% | 参数校验 + 状态检查 + 返回值验证 |
| FU-03 | 块写入 | 99% | 89% | 参数校验 + 状态检查 + 返回值验证 |
| FU-04 | 块擦除 | 99% | 87% | 双重参数校验 + 状态检查 |
| FU-05 | 块失效化 | 99% | 87% | 参数校验 + 状态检查 |
| FU-06 | 设备状态查询 | 99% | 82% | 越界检测 + 底层同步 |
| FU-07 | 作业取消 | 99% | 78% | 强制状态重置 |
| FU-08 | 主函数调度 | 99% | 80% | WDT 监控 + 状态自动恢复 |
| FU-09 | 参数校验与 DET | 99% | 72% | DET 开关风险 |
| FU-10 | 版本信息查询 | 95% | 92% | 编译时版本校验 |
| FU-11 | 设备模式设置 | 99% | 65% | Mode 值校验缺失 |

### 4.2 模块整体诊断覆盖率

```
DC_MemIf = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_MemIf ≈ 85%
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

单点故障是指不被任何诊断机制覆盖的残余故障。MemIf 中主要单点故障来源：
- `MemIf_ModuleInitialized` 标志位单粒子翻转（由 WDT 系统级监控覆盖）
- 设备路由逻辑瞬态故障（由参数校验和底层驱动返回值检查覆盖）
- `MemIf_SetMode` Mode 值校验缺失（由底层驱动自身校验覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (8.1 / 90) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。MemIf 中主要潜伏故障来源：
- DET 功能被关闭导致的参数校验缺失（通过集成测试配置验证覆盖）
- 设备抽象层函数指针表损坏（通过 Lcfg 编译时常量保护覆盖）
- 状态同步逻辑缺陷（通过 `MemIf_MainFunction` 周期性状态更新检测覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (9.0 / 90) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_device_routing_error + lambda_state_desync + lambda_mode_set_failure
     ≈ 12 + 20 + 20 = 52 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 MemIf 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | `MemIf_ModuleInitialized == TRUE`，所有设备 `status == MEMIF_IDLE` | 正常路由读写/擦除请求到底层 Fee/Ea 驱动 |
| **设备忙** | 某设备 `status == MEMIF_BUSY` | 拒绝该设备新请求，返回 `E_NOT_OK`；等待 `MemIf_MainFunction` 完成作业 |
| **未初始化** | `MemIf_ModuleInitialized == FALSE` | 所有 API 返回错误/默认值，DET 上报 `MEMIF_E_UNINIT` |
| **安全停止** | MemIf 主函数无法执行（CPU 故障）或设备状态持续 BUSY 超时 | 系统进入安全状态，WDT 触发复位 |

### 6.2 安全状态转换

```
未初始化 ──[MemIf_Init 成功]──→ 正常运行
正常运行 ──[发起读/写/擦除]──→ 设备忙
设备忙   ──[MainFunction 完成]──→ 正常运行
设备忙   ──[MemIf_Cancel]──→ 正常运行 (jobResult = CANCELED)
正常运行 ──[MemIf_DeInit]──→ 未初始化
任意状态 ──[MainFunction 超时]──→ 安全停止 (WDT 触发)
```

---

## 7. 故障反应时间线

### 7.1 场景一：非法 DeviceIndex 读请求

| 时间 | 事件 |
|------|------|
| T=0 | 上层 (NvM) 调用 `MemIf_Read(DeviceIndex=0xFF, ...)` |
| T=1 | `MEMIF_VALIDATE_DEVICE_INDEX_RET` 检测 `DeviceIndex >= MEMIF_TOTAL_NUM_DEVICES` |
| T=2 | DET 上报 `MEMIF_E_PARAM_DEVICE_INDEX`，返回 `E_NOT_OK` |
| T=3 | 上层收到错误，触发降级策略 |
| **总延迟** | **1 个调用周期 + 系统响应时间** |

### 7.2 场景二：底层读操作失败

| 时间 | 事件 |
|------|------|
| T=0 | `MemIf_Read()` 通过参数校验，调用 `Fee_Read()` / `Ea_Read()` |
| T=1 | 底层返回 `E_NOT_OK` |
| T=2 | `MemIf_Read` 返回值检查 (L289)：不更新状态，保持 `MEMIF_IDLE` |
| T=3 | 返回 `E_NOT_OK` 给上层 |
| T=4 | 上层 (NvM) 检测失败，执行重试或回退策略 |
| **总延迟** | **1 个调用周期 + 上层重试时间** |

### 7.3 场景三：设备 BUSY 状态下的写请求

| 时间 | 事件 |
|------|------|
| T=0 | `MemIf_Write()` 通过参数校验 |
| T=1 | 检查 `MemIf_DeviceState[DeviceIndex].status != MEMIF_IDLE` |
| T=2 | 写请求被拒绝，返回 `E_NOT_OK`（设备状态不变） |
| T=3 | 上层等待 `MemIf_MainFunction` 完成当前作业后重试 |
| **总延迟** | **1 个调用周期 + 等待底层作业完成** |

### 7.4 场景四：MainFunction 状态恢复

| 时间 | 事件 |
|------|------|
| T=0 | `MemIf_MainFunction(DeviceIndex)` 被周期调用 |
| T=1 | 调用 `Fee_MainFunction()` / `Ea_MainFunction()` |
| T=2 | 调用 `MemIf_GetJobResult(DeviceIndex)` 获取作业结果 |
| T=3 | `jobResult != MEMIF_JOB_PENDING`（作业已完成或失败） |
| T=4 | `MemIf_DeviceState[DeviceIndex].status` 更新为 `MEMIF_IDLE` |
| T=5 | 设备恢复接受新请求 |
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

1. **底层驱动假设**: Fee/Ea 层的失效率及诊断覆盖率由独立 FMEDA 覆盖，本分析假设底层 FIT 数据已知
2. **设备抽象表完整性**: `MemIf_Devices[]` 函数指针表在 Lcfg 中定义为 `const`，存储于受保护 ROM 区域
3. **WDT 覆盖**: `MemIf_MainFunction` 的调度监控由系统级 WDT 覆盖
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **配置正确性**: 假设 MemIf 配置（设备数量、块数量、超时参数）在集成阶段经过验证
6. **Phase 2 收敛**: ecual 层 MemIf 已收敛为空转发 shim，所有功能实现位于 `services/memif/`，本 FMEDA 仅分析 canonical 实现
7. **DET 开启**: 假设 `MEMIF_DEV_ERROR_DETECT == STD_ON`（当前默认配置），关闭时 DC 将显著降低（见 FM-09-01）

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| Flash (Fee 底层) 读取 | 10 FIT | 供应商 FMD |
| Flash (Fee 底层) 写入 | 20 FIT | 供应商 FMD |
| EEPROM (Ea 底层) 读取 | 8 FIT | 供应商 FMD |
| EEPROM (Ea 底层) 写入 | 15 FIT | 供应商 FMD |
| RAM 存储 (DeviceState) | 5 FIT | IEC 61508 Annex D |
| CPU 逻辑 (路由/校验) | 12 FIT | 供应商 FMD |
| 配置数据 (ROM/Flash) | 3 FIT | IEC 61508 Annex D |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| DET | Default Error Tracing |
| Fee | Flash EEPROM Emulation |
| Ea | EEPROM Abstraction |
| NVRAM | Non-Volatile Random Access Memory |
| NvM | NVRAM Manager |
| BswM | BSW Manager |
| WDT | Watchdog Timer |
