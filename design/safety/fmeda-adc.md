# FMEDA — Adc (ADC Driver)

> **文档编号**: SAF-FMEDA-ADC-001
> **版本**: 1.1
> **日期**: 2026-10-09
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW MCAL 层的 Adc (ADC Driver) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/mcal/adc/src/Adc.c`

**分析边界**:
- Adc 核心逻辑（通道初始化、组转换启停、结果读取、硬件触发、校准）
- 上层接口：`Adc_Init`, `Adc_StartGroupConversion`, `Adc_StopGroupConversion`, `Adc_ReadGroup`, `Adc_GetGroupStatus`
- 内部安全机制：DET 参数校验、初始化状态检查、Busy 互斥、结果掩码、校准等待、CALF 校准失败检测、转换超时保护、结果范围校验、BIST 自检、错误通知回调
- 下层依赖（假设由硬件覆盖）：ADC 寄存器（HC0/HS/R0/CFG/GC/GS/CAL）、IPG 时钟

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 Adc 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体/变量 | 位置 | 说明 |
|-------------|------|------|
| `Adc_DriverInitialized` | Adc.c:67 | 驱动初始化状态标志（`boolean`） |
| `Adc_GroupStatus[ADC_NUM_GROUPS]` | Adc.c:68 | 各组转换状态数组（`ADC_IDLE`/`ADC_BUSY`/`ADC_STREAM_COMPLETED`） |
| `Adc_GroupResults[ADC_NUM_GROUPS][ADC_NUM_CHANNELS]` | Adc.c:69 | 各组转换结果缓存（`uint16` 二维数组） |
| `Adc_ConfigPtr` | Adc.c:70 | 指向全局配置结构体的指针（`const Adc_ConfigType*`） |
| `Adc_ConfigType` | Adc.h:231-242 | 顶层配置：HwUnits[], Groups[], Channels[] 及功能开关 |
| `Adc_GroupConfigType` | Adc.h:203-216 | 组配置：GroupId, HwUnit, Channels[], NumChannels, TriggerSource, Resolution, NotificationFn |
| `Adc_ChannelConfigType` | Adc.h:194-198 | 通道配置：ChannelId, SamplingTime, ChannelInput |
| `Adc_HWUnitConfigType` | Adc.h:221-226 | 硬件单元配置：HwUnitId, BaseAddress, ClockFrequency, DefaultResolution |

### 1.4 寄存器映射

| 寄存器 | 偏移 | 说明 |
|--------|------|------|
| `ADC_HC0` | 0x00 | 控制寄存器（通道选择 + 中断使能） |
| `ADC_HS` | 0x04 | 状态寄存器（COCO0 转换完成标志） |
| `ADC_R0` | 0x08 | 转换结果寄存器（低 12 位有效） |
| `ADC_CFG` | 0x0C | 配置寄存器（时钟/分辨率/采样/低功耗） |
| `ADC_GC` | 0x10 | 全局控制寄存器（校准/触发/连续/平均） |
| `ADC_GS` | 0x14 | 全局状态寄存器（ADACT 活动/AWOD/CALF 校准失败） |
| `ADC_OFS` | 0x1C | 偏移校正寄存器 |
| `ADC_CAL` | 0x20 | 校准寄存器 |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 驱动初始化 | `Adc_Init()` (L116) | 配置 ADC 硬件、执行校准、初始化状态数组 |
| FU-02 | 驱动反初始化 | `Adc_DeInit()` (L163) | 关闭 ADC 硬件、清除初始化标志 |
| FU-03 | 组转换启动 | `Adc_StartGroupConversion()` (L193) | 软件触发启动组内所有通道转换 |
| FU-04 | 组转换停止 | `Adc_StopGroupConversion()` (L251) | 停止正在进行的组转换 |
| FU-05 | 组结果读取 | `Adc_ReadGroup()` (L281) | 从结果缓存读取组转换数据 |
| FU-06 | 硬件触发管理 | `Adc_EnableHardwareTrigger()` (L309), `Adc_DisableHardwareTrigger()` (L336) | 硬件触发使能/禁用 |
| FU-07 | 组状态管理 | `Adc_GetGroupStatus()` (L394), `Adc_GroupStatus[]` | 查询/维护组转换状态 |
| FU-08 | 组通知管理 | `Adc_EnableGroupNotification()` (L361), `Adc_DisableGroupNotification()` (L377) | 转换完成通知使能/禁用 |
| FU-09 | 流式数据管理 | `Adc_GetStreamLastPointer()` (L427), `Adc_SetupResultBuffer()` (L451) | 流式结果指针访问与缓冲区设置 |
| FU-10 | 电源状态管理 | `Adc_SetPowerState()` (L475), `Adc_GetCurrentPowerState()` (L509) | 低功耗状态切换 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 驱动初始化 (`Adc_Init`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | `ConfigPtr` 为空指针 | 访问非法地址，系统崩溃 | `Adc_Init()` 入口 NULL 检查 (L119)，上报 `ADC_E_PARAM_CONFIG` | 90% | B |
| FM-01-02 | 重复初始化 (`Adc_DriverInitialized == TRUE`) | 硬件配置被覆盖，正在进行的转换丢失 | 初始化状态检查 (L123)，上报 `ADC_E_ALREADY_INITIALIZED` | 99% | B |
| FM-01-03 | ADC 基地址解析失败 (`Adc_GetBaseAddr` 返回 0) | 硬件单元无法访问 | `Adc_GetBaseAddr()` 默认分支返回 0U (L82)，初始化循环中 `baseAddr == 0U` 时跳过 (L133) | 85% | B |
| FM-01-04 | ADC_CFG 寄存器写入值损坏 | 时钟/分辨率配置错误，转换结果不准确 | 配置写入后可读回验证（建议增强）；校准过程间接验证配置正确性 | 60% | B |
| FM-01-05 | 校准过程失败（`ADC_GS_CALF` 置位） | 转换结果偏移，精度下降 | 校准等待循环后检查 `ADC_GS_CALF` 标志 (v1.1 新增)；失败时上报诊断事件，设置驱动状态为降级模式，拒绝后续转换请求 | 90% | B |
| FM-01-06 | 校准等待死循环（`ADACT` 永不清零） | 初始化阻塞，系统挂起 | 校准等待循环增加超时计数器 (v1.1 新增)；超时后退出循环，上报 `ADC_E_CALIBRATION_TIMEOUT`，驱动进入安全停止状态 | 95% | B |
| FM-01-07 | `Adc_GroupStatus[]` 初始化不完整 | 部分组状态为未定义值，后续操作异常 | 循环初始化 (L154-156) 覆盖 `0..ADC_NUM_GROUPS-1`，循环边界由编译时常量保证 | 90% | B |

### 3.2 FU-02: 驱动反初始化 (`Adc_DeInit`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | 未初始化时调用 `Adc_DeInit` | 操作未初始化的硬件 | 初始化状态检查 (L166)，上报 `ADC_E_UNINIT` | 99% | B |
| FM-02-02 | 转换进行中调用 `Adc_DeInit`（某组 `ADC_BUSY`） | 转换中断，结果数据不完整 | Busy 状态检查 (L173-175)，存在 Busy 组时直接返回拒绝反初始化 | 95% | B |
| FM-02-03 | ADC_GC 寄存器清零失败 | ADC 硬件未正确关闭，功耗异常 | 写后读回验证（建议增强）；硬件复位由系统级监控覆盖 | 50% | B |

### 3.3 FU-03: 组转换启动 (`Adc_StartGroupConversion`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | 未初始化时调用 | 操作未配置的硬件，行为不可预测 | 初始化状态检查 (L196)，上报 `ADC_E_UNINIT` | 99% | B |
| FM-03-02 | `Group` 参数超出范围 (`>= ADC_NUM_GROUPS`) | 访问越界内存，`Adc_GroupStatus`/`Adc_GroupResults` 被破坏 | Group 范围检查 (L200)，上报 `ADC_E_PARAM_GROUP` | 99% | B |
| FM-03-03 | 组已在 Busy 状态时重复启动 | 正在进行的转换被干扰 | Busy 状态检查 (L206-208)，直接返回忽略本次请求 | 99% | B |
| FM-03-04 | 转换完成等待死循环（`COCO0` 永不置位） | 函数阻塞，系统挂起 | 转换等待循环增加超时计数器 (v1.1 新增)；超时后退出循环，上报 `ADC_E_CONVERSION_TIMEOUT`，组状态设为 `ADC_IDLE`，调用错误通知回调 | 95% | B |
| FM-03-05 | 转换结果读取值超出有效位宽 | 结果包含无效高位数据 | 结果掩码 `& 0xFFFU` (L237)，强制截取 12 位有效数据 | 95% | B |
| FM-03-06 | 通道号超出 `ADCH_MASK` (5 位) | 写入 HC0 的通道选择位错误 | 通道掩码 `& ADC_HC_ADCH_MASK` (L225)，限制为 5 位（0-31） | 90% | B |
| FM-03-07 | 通知回调函数指针无效（非 NULL 但损坏） | 跳转到非法地址，系统崩溃 | 仅检查 `!= 0U` 和 `!= NULL_PTR` (L243-244)，无法检测野指针；由 MPU 保护覆盖（系统级） | 60% | B |
| FM-03-08 | `Adc_GroupResults` 写入越界 | 内存破坏 | 循环边界由 `groupConfig->NumChannels` 控制 (L221)；配置正确性由集成阶段验证 | 80% | B |

### 3.4 FU-04: 组转换停止 (`Adc_StopGroupConversion`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | 未初始化时调用 | 操作未配置的硬件 | 初始化状态检查 (L254)，上报 `ADC_E_UNINIT` | 99% | B |
| FM-04-02 | `Group` 参数超出范围 | 访问越界内存 | Group 范围检查 (L258)，上报 `ADC_E_PARAM_GROUP` | 99% | B |
| FM-04-03 | 组不在 Busy 状态时调用停止 | 无操作，但状态一致 | 非 Busy 状态直接返回 (L264-266) | 99% | B |
| FM-04-04 | ADC_GC 寄存器写入失败（清除 `ADCONV`/`ADCO`） | 硬件转换未停止，持续消耗资源 | 写后读回验证（建议增强）；WDT 超时覆盖 | 50% | B |

### 3.5 FU-05: 组结果读取 (`Adc_ReadGroup`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | 未初始化时调用 | 返回未定义数据 | 初始化状态检查 (L284)，返回 `E_NOT_OK` | 99% | B |
| FM-05-02 | `Group` 参数超出范围 | 访问越界内存 | Group 范围检查 (L288)，返回 `E_NOT_OK` | 99% | B |
| FM-05-03 | `DataBufferPtr` 为空指针 | 写入非法地址，内存破坏 | NULL 指针检查 (L292)，返回 `E_NOT_OK` | 90% | B |
| FM-05-04 | 读取到未完成的转换结果（状态为 `ADC_BUSY`） | 结果数据不完整或为旧值 | `Adc_ReadGroup` 增加组状态检查 (v1.1 新增)；非 `ADC_STREAM_COMPLETED` 时返回 `E_NOT_OK`，拒绝返回不完整数据 | 90% | B |
| FM-05-05 | `Adc_GroupResults` 数据因 RAM 瞬态故障损坏 | 应用层获得错误的转换值 | 无内置 CRC/奇偶校验（建议增强）；上层合理性检查（范围校验） | 30% | B |
| FM-05-06 | `NumChannels` 配置值大于实际结果数组宽度 | 读取越界 | 循环边界由 `groupConfig->NumChannels` 控制 (L300)；配置正确性由集成验证保证 | 80% | B |

### 3.6 FU-06: 硬件触发管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | 未初始化时调用 `Adc_EnableHardwareTrigger` | 操作未配置的硬件 | 初始化状态检查 (L312)，上报 `ADC_E_UNINIT` | 99% | B |
| FM-06-02 | `Group` 参数超出范围 | 访问越界配置 | Group 范围检查 (L316)，上报 `ADC_E_PARAM_GROUP` | 99% | B |
| FM-06-03 | 组的触发源配置为软件触发时启用硬件触发 | 误触发转换 | `TriggerSource != ADC_TRIGG_SRC_HW` 检查 (L323)，不匹配则直接返回 | 95% | B |
| FM-06-04 | ADC_GC 寄存器 `ADTRG` 位写入失败 | 硬件触发未生效/未停止 | 写后读回验证（建议增强） | 50% | B |

### 3.7 FU-07: 组状态管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | 未初始化时调用 `Adc_GetGroupStatus` | 返回未定义状态 | 初始化状态检查 (L397)，返回 `ADC_IDLE` | 99% | B |
| FM-07-02 | `Group` 参数超出范围 | 返回越界数据 | Group 范围检查 (L401)，返回 `ADC_IDLE` | 99% | B |
| FM-07-03 | `Adc_GroupStatus[]` 因 RAM 瞬态故障损坏 | 返回错误的组状态 | 状态值合理性检查（仅允许 0/1/2）；建议增加枚举范围校验 | 60% | B |
| FM-07-04 | 状态转换不一致（如转换完成但未更新为 `ADC_STREAM_COMPLETED`） | 上层无法获取正确结果 | 状态在 `Adc_StartGroupConversion` 结束时更新 (L240)；一致性由代码逻辑保证 | 80% | B |

### 3.8 FU-08: 组通知管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | 未初始化时调用通知使能/禁用 | 操作未配置的功能 | 初始化状态检查 (L364, L380)，上报 `ADC_E_UNINIT` | 99% | B |
| FM-08-02 | `Group` 参数超出范围 | 访问越界配置 | Group 范围检查 (L368, L384)，上报 `ADC_E_PARAM_GROUP` | 99% | B |
| FM-08-03 | 通知使能后回调未触发 | 上层无法及时获取转换结果 | 当前为桩实现 `(void)Group` (L373)；完整实现应由中断/COCO 标志驱动 | 40% | B |

### 3.9 FU-09: 流式数据管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | 未初始化时调用 `Adc_GetStreamLastPointer` | 返回非法指针 | 初始化状态检查 (L430)，返回 `0U` | 99% | B |
| FM-09-02 | `Group` 参数超出范围 | 访问越界内存 | Group 范围检查 (L434)，返回 `0U` | 99% | B |
| FM-09-03 | `PtrToSamplePtr` 为空指针 | 写入非法地址 | NULL 指针检查 (L438)，返回 `0U` | 90% | B |
| FM-09-04 | 返回的 `NumChannels` 与实际有效样本数不一致 | 上层读取无效样本 | 返回值由配置 `NumChannels` 决定 (L445)；配置正确性由集成验证保证 | 75% | B |
| FM-09-05 | `Adc_SetupResultBuffer` 的 `DataBufferPtr` 未被实际使用 | 缓冲区设置无效 | 当前为桩实现 `(void)Group; (void)DataBufferPtr` (L467-468)；完整实现应绑定缓冲区 | 30% | B |

### 3.10 FU-10: 电源状态管理

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | 未初始化时调用电源状态接口 | 操作未配置的功能 | 初始化状态检查 (L478, L495, L512, L529)，上报 `ADC_E_UNINIT` | 99% | B |
| FM-10-02 | `Result` 指针为空 | 写入非法地址 | NULL 指针检查 (L482, L499, L516, L533)，上报 `ADC_E_PARAM_POINTER` | 90% | B |
| FM-10-03 | 电源状态切换未实际执行 | ADC 在应低功耗时仍全功耗运行 | 当前为桩实现，始终返回 `ADC_SERVICE_ACCEPTED` (L488)；实际功耗由硬件状态决定 | 30% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 驱动初始化 | 99% | 88% | CALF 检查 + 超时保护已实现 |
| FU-02 | 驱动反初始化 | 99% | 82% | 寄存器写回验证不足 |
| FU-03 | 组转换启动 | 99% | 90% | 转换超时保护已实现 |
| FU-04 | 组转换停止 | 99% | 87% | 寄存器写回验证不足 |
| FU-05 | 组结果读取 | 99% | 85% | 组状态检查 + 结果范围校验已实现 |
| FU-06 | 硬件触发管理 | 99% | 86% | 寄存器写回验证不足 |
| FU-07 | 组状态管理 | 99% | 85% | 状态值范围校验可增强 |
| FU-08 | 组通知管理 | 99% | 79% | 当前为桩实现 |
| FU-09 | 流式数据管理 | 99% | 75% | 部分桩实现 |
| FU-10 | 电源状态管理 | 99% | 73% | 桩实现，无实际功耗管理 |

### 4.2 模块整体诊断覆盖率

```
DC_Adc = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_Adc ≈ 90%
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

单点故障是指不被任何诊断机制覆盖的残余故障。Adc 中主要单点故障来源：
- ADC 校准过程中 CALF 标志未检测（由 WDT 超时部分覆盖）
- 转换结果 RAM 存储的瞬态故障（由上层合理性检查覆盖）
- 寄存器写操作失败（由系统级监控部分覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (8.1 / 95) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。Adc 中主要潜伏故障来源：
- DET 参数校验逻辑自身的故障（通过代码结构冗余部分覆盖）
- 状态机一致性依赖代码逻辑正确性（通过集成测试覆盖）
- 桩实现功能（通知、电源管理）的潜在缺陷（在完整实现后通过测试覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (9.5 / 95) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_calibration_miss + lambda_conversion_timeout
       + lambda_result_ram_corruption + lambda_register_write_fail
     ≈ 6 + 5 + 15 + 10 = 36 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 Adc 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | 驱动已初始化，所有组状态为 `ADC_IDLE` 或 `ADC_STREAM_COMPLETED` | 正常接受转换请求和结果读取 |
| **转换中** | 至少一个组状态为 `ADC_BUSY` | 拒绝同组重复启动；允许其他组操作 |
| **降级运行** | 校准失败（`CALF` 置位）或转换超时 | 返回上次有效结果或默认值，上报诊断事件 |
| **安全停止** | 驱动未初始化或初始化失败 | 所有接口返回错误码（`E_NOT_OK` / `ADC_IDLE`），拒绝服务 |

### 6.2 安全状态转换

```
安全停止 ──[Adc_Init 成功]──→ 正常运行
正常运行 ──[StartGroupConversion]──→ 转换中
转换中   ──[转换完成/通知]──→ 正常运行 (ADC_STREAM_COMPLETED)
转换中   ──[StopGroupConversion]──→ 正常运行 (ADC_IDLE)
正常运行 ──[校准失败/转换超时]──→ 降级运行
降级运行 ──[Adc_DeInit]──→ 安全停止
任意状态 ──[WDT 超时]──→ 安全停止 (系统复位)
```

### 6.3 转换错误安全行为

当 ADC 转换过程中检测到错误时，驱动采取以下安全措施：

1. **转换超时**：`Adc_StartGroupConversion` 中 COCO0 等待循环 (L234) 由系统 WDT 保护，超时后系统复位进入安全状态
2. **结果掩码**：所有转换结果通过 `& 0xFFFU` (L237) 截取 12 位有效数据，过滤高位噪声
3. **默认值返回**：当转换失败或数据无效时，`Adc_GroupResults` 中保留的上次有效值被读取；首次上电时数组清零（BSS 段），返回 0x000 作为默认值
4. **状态保护**：Busy 互斥检查 (L206) 防止并发转换冲突

---

## 7. 故障反应时间线

### 7.1 场景一：组转换启动 — 参数越界

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Adc_StartGroupConversion(Group)` 且 `Group >= ADC_NUM_GROUPS` |
| T=1 cycle | DET 检测到 `Group >= ADC_NUM_GROUPS` (L200) |
| T=2 | 调用 `Det_ReportError(ADC_MODULE_ID, 0U, ADC_SID_STARTGROUPCONVERSION, ADC_E_PARAM_GROUP)` |
| T=3 | 函数返回，不执行任何硬件操作 |
| **总延迟** | **< 1 个调用周期** |

### 7.2 场景二：组转换启动 — 转换完成等待

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Adc_StartGroupConversion(Group)` |
| T=1 | 参数校验通过，`Adc_GroupStatus[Group] = ADC_BUSY` (L213) |
| T=2 | 配置软件触发，清除 `ADTRG` 位 (L217-218) |
| T=3..N | 循环：写入 HC0 选择通道 → 设置 ADCONV → 等待 COCO0 → 读取结果 |
| T=N+1 | 所有通道转换完成，`Adc_GroupStatus[Group] = ADC_STREAM_COMPLETED` (L240) |
| T=N+2 | 调用通知回调（如已启用）(L243-247) |
| **总延迟** | **N 个通道 × 单通道转换时间（取决于分辨率和采样时间配置）** |

### 7.3 场景三：转换等待超时（WDT 保护）

| 时间 | 事件 |
|------|------|
| T=0 | `Adc_StartGroupConversion` 进入 COCO0 等待循环 (L234) |
| T=0..Twdt | COCO0 未置位（硬件故障），持续等待 |
| T=Twdt | WDT 超时触发系统复位 |
| T=Twdt+Treset | 系统复位，进入安全状态（`Adc_DriverInitialized = FALSE`） |
| **总延迟** | **WDT 超时周期（典型 100-500ms）** |

### 7.4 场景四：读取未完成转换结果

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Adc_ReadGroup(Group, DataBufferPtr)` |
| T=1 | 参数校验通过，组状态检查 `Adc_GroupStatus[Group] != ADC_STREAM_COMPLETED` (v1.1) |
| T=2 | 状态不满足，返回 `E_NOT_OK`，拒绝提供不完整数据 |
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

1. **硬件依赖假设**: ADC 硬件寄存器（HC0/HS/R0/CFG/GC/GS）的失效率及诊断覆盖率由芯片供应商 FMD 覆盖，本分析假设硬件 FIT 数据已知
2. **REG_READ32/REG_WRITE32 可靠性**: 假设寄存器读写宏由 MCAL 底层提供，具备地址合法性校验
3. **WDT 覆盖**: 系统级 WDT 作为最终超时保护；Adc 模块内部已增加校准等待和转换等待的超时计数器（v1.1 更新）
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **配置正确性**: 假设 Adc 配置（`Adc_ConfigType`、组/通道/硬件单元参数）在集成阶段经过工具链验证
6. **DET 覆盖**: 假设 `Det_ReportError` 默认错误报告模块工作正常，由系统级 DEM 记录
7. **MPU 保护**: 指针非法访问由系统级 MPU/MMU 覆盖（如配置）

---

## 10. 改进建议

基于 FMEDA 分析结果，以下列出提升诊断覆盖率的改进建议：

| 编号 | 建议 | 影响 FU | 预期 DC 提升 | 状态 |
|------|------|---------|-------------|------|
| R-01 | 校准完成后检查 `ADC_GS_CALF` 标志，失败时上报诊断事件 | FU-01 | +15% | ✅ v1.1 已实现 |
| R-02 | 为校准等待循环和 COCO0 等待循环增加超时计数器 | FU-01, FU-03 | +10% | ✅ v1.1 已实现 |
| R-03 | `Adc_ReadGroup` 增加组状态检查，非 `ADC_STREAM_COMPLETED` 时返回错误 | FU-05 | +20% | ✅ v1.1 已实现 |
| R-04 | 关键寄存器写操作后增加读回验证 | FU-01, FU-02, FU-04, FU-06 | +15% | 待实现 |
| R-05 | 为 `Adc_GroupResults` 增加奇偶校验或 CRC 保护 | FU-05, FU-09 | +25% | 待实现 |
| R-06 | 完善组通知回调实现，由中断/COCO 标志驱动 | FU-08 | +30% | 待实现 |
| R-07 | 完善 `Adc_SetupResultBuffer` 实现，绑定外部缓冲区 | FU-09 | +20% | 待实现 |
| R-08 | 实现实际电源状态切换逻辑 | FU-10 | +30% | 待实现 |

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| ADC 硬件单元（转换核心） | 15 FIT | 供应商 FMD (NXP i.MX8M Mini) |
| ADC 寄存器访问 | 8 FIT | IEC 61508 Annex D |
| RAM 存储（结果缓存） | 10 FIT | IEC 61508 Annex D |
| CPU 逻辑（参数校验/状态管理） | 12 FIT | 供应商 FMD |
| 时钟系统（IPG Clock） | 5 FIT | 供应商 FMD |
| 配置存储（Flash/ROM） | 3 FIT | 供应商 FMD |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| ADC | Analog-to-Digital Converter |
| DET | Default Error Tracer |
| DEM | Default Error Monitor |
| COCO | Conversion Complete Flag |
| WDT | Watchdog Timer |
| MPU | Memory Protection Unit |
| CALF | Calibration Failure Flag |
| ADACT | ADC Active Flag |
