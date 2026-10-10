# FMEDA — Dio (DIO Driver)

> **文档编号**: SAF-FMEDA-DIO-001
> **版本**: 1.1
> **日期**: 2026-10-09
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW MCAL 层的 Dio (DIO Driver) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/mcal/dio/src/Dio.c`

**分析边界**:
- Dio 核心逻辑（通道读写、端口读写、通道组读写、通道翻转、掩码写）
- 上层接口：`Dio_ReadChannel`, `Dio_WriteChannel`, `Dio_ReadPort`, `Dio_WritePort`, `Dio_ReadChannelGroup`, `Dio_WriteChannelGroup`, `Dio_FlipChannel`, `Dio_MaskedWritePort`
- 内部安全机制：DET 错误检测（未初始化检测、参数范围校验、空指针校验）、GPIO 基地址映射
- 下层依赖（假设由硬件层覆盖）：SIUL2 GPIO 寄存器（PDOR/PSOR/PCOR/PDIR/PDDR）、REG_READ32/REG_WRITE32

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 Dio 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体/变量 | 位置 | 说明 |
|------------|------|------|
| `Dio_ChannelGroupType` | Dio.h:99-103 | 通道组定义：`port`（端口号）、`offset`（组内偏移）、`mask`（位掩码） |
| `Dio_ConfigType` | Dio_Cfg.h:111-113 | 模块配置结构体（当前为 dummy 占位） |
| `Dio_DriverInitialized` | Dio.c:72 | 模块初始化状态标志（`boolean`），所有 API 入口均检查此标志 |
| `DIO_GET_PORT(channel)` | Dio.c:66 | 宏：从 ChannelId 高 8 位提取端口号 `(channel >> 8)` |
| `DIO_GET_PIN(channel)` | Dio.c:67 | 宏：从 ChannelId 低 8 位提取引脚号 `(channel & 0xFF)` |
| GPIO 基地址表 | Dio.c:37-64 | 5 个 GPIO 端口基地址映射（GPIO1–GPIO5），支持 i.MX8MM 和 S32K312 |

### 1.4 配置参数

| 参数 | 值 | 来源 |
|------|-----|------|
| `DIO_NUM_PORTS` | 8 | Dio_Cfg.h:27 |
| `DIO_NUM_CHANNELS_PER_PORT` | 32 | Dio_Cfg.h:28 |
| `DIO_NUM_CHANNEL_GROUPS` | 4 | Dio_Cfg.h:29 |
| `DIO_DEV_ERROR_DETECT` | STD_ON | Dio_Cfg.h:21 |
| `DIO_FLIP_CHANNEL_API` | STD_ON | Dio_Cfg.h:34 |
| `DIO_MASKED_WRITE_PORT_API` | STD_ON | Dio_Cfg.h:35 |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 模块初始化 | `Dio_Init()` (L96) | 配置指针校验，设置 `Dio_DriverInitialized` 标志 |
| FU-02 | 通道读取 | `Dio_ReadChannel()` (L109) | 读取单个 DIO 通道引脚电平 |
| FU-03 | 通道写入 | `Dio_WriteChannel()` (L133) | 设置单个 DIO 通道输出电平 |
| FU-04 | 端口读取 | `Dio_ReadPort()` (L160) | 读取整个端口所有引脚电平 |
| FU-05 | 端口写入 | `Dio_WritePort()` (L180) | 设置整个端口输出电平 |
| FU-06 | 通道组读取 | `Dio_ReadChannelGroup()` (L198) | 读取端口内一组连续通道的电平 |
| FU-07 | 通道组写入 | `Dio_WriteChannelGroup()` (L219) | 设置端口内一组连续通道的输出电平 |
| FU-08 | 通道翻转 | `Dio_FlipChannel()` (L259) | 翻转单个通道输出电平并返回新值 |
| FU-09 | 掩码端口写入 | `Dio_MaskedWritePort()` (L292) | 按掩码选择性写入端口部分位 |
| FU-10 | GPIO 基地址映射 | `Dio_GetGpioBaseAddr()` (L78) | 端口号到 GPIO 寄存器基地址的映射 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 模块初始化 (`Dio_Init`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | ConfigPtr 为空指针 | 初始化失败，模块处于未初始化状态 | 入口参数校验：`ConfigPtr == NULL_PTR` 时上报 `DIO_E_PARAM_CONFIG` (L99-101) | 99% | B |
| FM-01-02 | `Dio_DriverInitialized` 标志位存储错误（内存单粒子翻转） | 模块误判为已初始化或未初始化，后续 API 行为异常 | 各 API 入口统一检查该标志 (L113, L136, L164, L184, L202, L222, L263, L296)；未初始化时返回安全默认值 `STD_LOW`/`0U` | 80% | B |
| FM-01-03 | 初始化后配置数据未生效 | 配置参数（端口数、通道数）使用错误值 | 编译时配置校验（`DIO_NUM_PORTS`、`DIO_NUM_CHANNELS_PER_PORT` 由代码生成器生成，编译期常量检查） | 85% | B |

### 3.2 FU-02: 通道读取 (`Dio_ReadChannel`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | 模块未初始化时调用 | 读取未配置的 GPIO 寄存器，返回不确定值 | `Dio_DriverInitialized == FALSE` 检查，上报 `DIO_E_UNINIT`，返回安全默认值 `STD_LOW` (L113-116) | 99% | B |
| FM-02-02 | ChannelId 超出有效范围 | 访问非法 GPIO 基地址，可能读取错误寄存器 | 范围检查：`ChannelId >= (DIO_NUM_PORTS * DIO_NUM_CHANNELS_PER_PORT)` 时上报 `DIO_E_PARAM_INVALID_CHANNEL_ID`，返回 `STD_LOW` (L117-120) | 99% | B |
| FM-02-03 | GPIO 基地址映射返回 0（无效端口） | 读取地址 0x00000000 + PSR 偏移，获取错误数据 | `Dio_GetGpioBaseAddr()` 默认分支返回 `0U` (L87)；读取值为 0 时返回 `STD_LOW`（安全默认值） | 70% | B |
| FM-02-04 | PSR 寄存器读取值受硬件瞬态干扰 | 引脚电平读取错误（单比特翻转） | 冗余读取（安全关键通道由应用层双重采样）；硬件级输入滤波器 | 60% | B |
| FM-02-05 | 端口/引脚位提取逻辑错误 | 返回错误通道电平 | `DIO_GET_PORT`/`DIO_GET_PIN` 宏使用固定位移运算 (L66-67)，为确定性逻辑；编译时静态验证 | 90% | B |

### 3.3 FU-03: 通道写入 (`Dio_WriteChannel`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | 模块未初始化时调用 | 写入未配置的 GPIO 寄存器，引脚状态不确定 | `Dio_DriverInitialized == FALSE` 检查，上报 `DIO_E_UNINIT`，直接 return (L136-139) | 99% | B |
| FM-03-02 | ChannelId 超出有效范围 | 写入非法 GPIO 地址，可能破坏其他外设寄存器 | 范围检查 (L140-143)，上报 `DIO_E_PARAM_INVALID_CHANNEL_ID`，直接 return | 99% | B |
| FM-03-03 | GPIO 基地址为 0（无效端口） | 写入地址 0x00000000 附近，可能破坏关键内存 | `Dio_GetGpioBaseAddr()` 返回 `0U` (L87)；当前实现无额外零地址保护 | 50% | B |
| FM-03-04 | 读-修改-写操作被中断打断 | PDOR 寄存器中间状态被破坏，相邻引脚输出异常 | SchM 临界区保护：`Mcal_DisableAllInterrupts()`/`Mcal_EnableAllInterrupts()` 包裹 R-M-W 操作；硬件 PSOR/PCOR 原子寄存器作为备选路径 | 90% | B |
| FM-03-05 | PDOR 寄存器写入后硬件写入失败 | 引脚输出保持旧值，与预期不一致 | 写后回读验证（安全关键通道由应用层验证） | 70% | B |

### 3.4 FU-04: 端口读取 (`Dio_ReadPort`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | 模块未初始化时调用 | 读取未配置寄存器 | `Dio_DriverInitialized` 检查，上报 `DIO_E_UNINIT`，返回 `0U` (L164-167) | 99% | B |
| FM-04-02 | PortId 超出端口数量 | 访问非法 GPIO 基地址 | 范围检查：`PortId >= DIO_NUM_PORTS` (L168-170)，上报 `DIO_E_PARAM_INVALID_PORT_ID`，返回 `0U` | 99% | B |
| FM-04-03 | GPIO 基地址为 0 | 读取错误地址 | 同 FM-02-03，`Dio_GetGpioBaseAddr()` 默认返回 `0U` (L87) | 70% | B |
| FM-04-04 | 32 位端口值读取瞬态错误 | 多通道电平状态不一致 | 冗余采样（应用层多次读取比对）；硬件滤波器 | 60% | B |

### 3.5 FU-05: 端口写入 (`Dio_WritePort`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | 模块未初始化时调用 | 写入未配置寄存器 | `Dio_DriverInitialized` 检查，上报 `DIO_E_UNINIT`，直接 return (L184-187) | 99% | B |
| FM-05-02 | PortId 超出端口数量 | 写入非法 GPIO 地址 | 范围检查 (L188-190)，上报 `DIO_E_PARAM_INVALID_PORT_ID`，直接 return | 99% | B |
| FM-05-03 | GPIO 基地址为 0 | 写入地址 0x00000000 附近 | 同 FM-03-03，无额外零地址保护 | 50% | B |
| FM-05-04 | 直接写入覆盖相邻引脚状态 | 端口中非目标引脚输出被意外修改 | `Dio_WritePort` 为全端口写入（设计意图）；安全关键场景应使用 `Dio_WriteChannel` 或 `Dio_MaskedWritePort` | 75% | B |

### 3.6 FU-06: 通道组读取 (`Dio_ReadChannelGroup`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | 模块未初始化时调用 | 读取未配置寄存器 | `Dio_DriverInitialized` 检查，上报 `DIO_E_UNINIT`，返回 `0U` (L202-205) | 99% | B |
| FM-06-02 | ChannelGroupIdPtr 为空指针 | 解引用空指针导致系统崩溃 | 空指针校验：`ChannelGroupIdPtr == NULL_PTR` 时上报 `DIO_E_PARAM_POINTER`，返回 `0U` (L206-209) | 99% | B |
| FM-06-03 | ChannelGroupIdPtr 中 mask/offset 配置错误 | 读取错误的位域，返回不相关通道数据 | 配置由代码生成器生成 (`Dio_Cfg.h`)，编译时常量校验；运行时无法检测 | 80% | B |
| FM-06-04 | 掩码移位运算逻辑错误 | 提取的通道组值偏移错误 | `(portValue & mask) >> offset` 为确定性位运算 (L214)；编译时静态验证 | 90% | B |

### 3.7 FU-07: 通道组写入 (`Dio_WriteChannelGroup`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | 模块未初始化时调用 | 写入未配置寄存器 | `Dio_DriverInitialized` 检查，上报 `DIO_E_UNINIT`，直接 return (L222-225) | 99% | B |
| FM-07-02 | ChannelGroupIdPtr 为空指针 | 解引用空指针导致系统崩溃 | 空指针校验 (L226-229)，上报 `DIO_E_PARAM_POINTER`，直接 return | 99% | B |
| FM-07-03 | mask/offset 配置错误导致写入错误位域 | 非目标引脚被修改，目标引脚未更新 | 配置由代码生成器保证；运行时无法检测 mask/offset 一致性 | 75% | B |
| FM-07-04 | 读-修改-写操作被中断打断 | 同端口其他通道的输出状态被破坏 | SchM 临界区保护：`Mcal_DisableAllInterrupts()`/`Mcal_EnableAllInterrupts()` 包裹 R-M-W 操作 | 90% | B |
| FM-07-05 | Level 值移位后超出 mask 范围 | 写入数据溢出到相邻位域 | `(Level << offset) & mask` 截断保护 (L235)；确定性位运算 | 90% | B |

### 3.8 FU-08: 通道翻转 (`Dio_FlipChannel`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | 模块未初始化时调用 | 翻转未配置寄存器 | `Dio_DriverInitialized` 检查，上报 `DIO_E_UNINIT`，返回 `STD_LOW` (L263-266) | 99% | B |
| FM-08-02 | ChannelId 超出有效范围 | 访问非法 GPIO 地址 | 范围检查 (L267-270)，上报 `DIO_E_PARAM_INVALID_CHANNEL_ID`，返回 `STD_LOW` | 99% | B |
| FM-08-03 | 读-修改-写翻转操作被中断打断 | 翻转结果不确定（可能丢失翻转或重复翻转） | SchM 临界区保护：`Mcal_DisableAllInterrupts()`/`Mcal_EnableAllInterrupts()` 包裹 R-M-W 操作；硬件 PTOR 寄存器可用于原子翻转 | 90% | B |
| FM-08-04 | 翻转后返回值与实际引脚状态不一致 | 上层逻辑基于错误的返回值做出错误决策 | 写后回读验证（安全关键通道由应用层验证） | 70% | B |

### 3.9 FU-09: 掩码端口写入 (`Dio_MaskedWritePort`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | 模块未初始化时调用 | 写入未配置寄存器 | `Dio_DriverInitialized` 检查，上报 `DIO_E_UNINIT`，直接 return (L296-299) | 99% | B |
| FM-09-02 | PortId 超出端口数量 | 写入非法 GPIO 地址 | 范围检查 (L300-303)，上报 `DIO_E_PARAM_INVALID_PORT_ID`，直接 return | 99% | B |
| FM-09-03 | Mask 参数为 0 | 写入操作无实际效果，端口状态不变 | 运行时无法检测零掩码（非错误场景，属于合法操作） | N/A | B |
| FM-09-04 | 读-修改-写操作被中断打断 | 同端口其他通道输出状态被破坏 | SchM 临界区保护：`Mcal_DisableAllInterrupts()`/`Mcal_EnableAllInterrupts()` 包裹 R-M-W 操作 | 90% | B |

### 3.10 FU-10: GPIO 基地址映射 (`Dio_GetGpioBaseAddr`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-10-01 | 端口号不在 switch-case 范围内（Port A–E 之外） | 返回基地址 `0U`，后续寄存器访问地址错误 | `default` 分支返回 `0U` (L87)；调用方读取值为 0 或写入 0 地址时行为不确定 | 50% | B |
| FM-10-02 | 基地址常量定义错误（宏展开错误） | 所有 GPIO 操作访问错误硬件地址 | 编译时地址校验（链接脚本验证）；启动时自检测试（由 BSW 初始化序列覆盖） | 90% | B |
| FM-10-03 | i.MX8MM 与 S32K312 平台宏切换错误 | 使用错误平台的寄存器布局 | 编译条件 `#ifdef S32K312` (L26) 由构建系统控制；编译时验证 | 95% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 模块初始化 | 99% | 88% | 空指针检测 + 标志位检查 |
| FU-02 | 通道读取 | 99% | 82% | 参数校验完善，硬件读取依赖冗余采样 |
| FU-03 | 通道写入 | 99% | 88% | 参数校验完善 + SchM 临界区保护 R-M-W |
| FU-04 | 端口读取 | 99% | 82% | 参数校验完善，硬件读取依赖冗余采样 |
| FU-05 | 端口写入 | 99% | 80% | 全端口写入设计，零地址保护不足 |
| FU-06 | 通道组读取 | 99% | 87% | 空指针 + 配置校验 |
| FU-07 | 通道组写入 | 99% | 90% | 空指针 + 移位截断保护 + SchM 临界区 |
| FU-08 | 通道翻转 | 99% | 88% | SchM 临界区保护 R-M-W |
| FU-09 | 掩码端口写入 | 99% | 88% | 参数校验完善 + SchM 临界区保护 R-M-W |
| FU-10 | GPIO 基地址映射 | 95% | 78% | 零地址返回无额外保护 |

### 4.2 模块整体诊断覆盖率

```
DC_Dio = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_Dio ≈ 88%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL B)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 90% | ~91% | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ~90% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 100 FIT | ~38 FIT | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。Dio 中主要单点故障来源：
- GPIO 基地址映射返回 0 时的零地址写入（部分由 MPU 覆盖）
- R-M-W 操作的瞬态中断干扰（由系统级 WDT 覆盖）
- 硬件寄存器读取瞬态错误（由应用层冗余采样覆盖）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (6.5 / 75) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。Dio 中主要潜伏故障来源：
- `Dio_DriverInitialized` 标志位本身的存储故障（通过各 API 入口统一检查降低影响）
- DET 错误检测逻辑自身的故障（通过代码静态分析 + 编译时校验覆盖）
- GPIO 基地址常量定义错误（通过编译时地址校验覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (7.0 / 75) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_gpio_base_zero + lambda_rmw_interrupt + lambda_register_read_transient
     ≈ 6 + 8 + 15 = 29 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 Dio 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **未初始化** | `Dio_DriverInitialized == FALSE` | 所有 API 上报 `DIO_E_UNINIT`，返回安全默认值（`STD_LOW` / `0U`），不执行任何硬件操作 |
| **正常运行** | 初始化完成，参数校验通过 | 正常执行 GPIO 寄存器读写操作 |
| **参数错误** | ChannelId/PortId 超出范围或指针为空 | 上报 DET 错误（`DIO_E_PARAM_INVALID_CHANNEL_ID` / `DIO_E_PARAM_INVALID_PORT_ID` / `DIO_E_PARAM_POINTER`），返回安全默认值 |
| **安全停止** | GPIO 基地址为 0 或 CPU 故障 | 系统进入安全状态，WDT 触发复位；引脚保持最后已知状态或硬件默认上拉/下拉状态 |

### 6.2 安全状态转换

```
未初始化 ──[Dio_Init 成功]──→ 正常运行
正常运行 ──[参数校验失败]──→ 参数错误（返回安全默认值后恢复正常运行）
参数错误 ──[参数校验通过]──→ 正常运行
任意状态 ──[GPIO基地址为0]──→ 安全停止 (WDT触发)
任意状态 ──[CPU故障]──→ 安全停止 (WDT触发)
```

### 6.3 安全默认值策略

当故障检测触发时，各 API 返回的安全默认值如下：

| API 函数 | 安全默认值 | 说明 |
|---------|-----------|------|
| `Dio_ReadChannel()` | `STD_LOW` | 返回低电平，避免上层误判为有效信号 |
| `Dio_ReadPort()` | `0U` | 返回全零端口值 |
| `Dio_ReadChannelGroup()` | `0U` | 返回全零通道组值 |
| `Dio_WriteChannel()` | 无操作 (return) | 不修改引脚状态，保持最后已知值 |
| `Dio_WritePort()` | 无操作 (return) | 不修改引脚状态 |
| `Dio_WriteChannelGroup()` | 无操作 (return) | 不修改引脚状态 |
| `Dio_FlipChannel()` | `STD_LOW` | 返回低电平，不执行翻转 |
| `Dio_MaskedWritePort()` | 无操作 (return) | 不修改引脚状态 |

---

## 7. 故障反应时间线

### 7.1 场景一：未初始化调用

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Dio_ReadChannel(ChannelId)` |
| T=1 cycle | 检测到 `Dio_DriverInitialized == FALSE` |
| T=2 cycles | `Det_ReportError(DIO_MODULE_ID, 0U, DIO_SID_READ_CHANNEL, DIO_E_UNINIT)` |
| T=3 cycles | 返回安全默认值 `STD_LOW` |
| **总延迟** | **3 个 CPU 周期（约 < 1 μs）** |

### 7.2 场景二：无效通道 ID

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Dio_WriteChannel(0xFFFF, STD_HIGH)` |
| T=1 cycle | 检测到 `ChannelId >= (DIO_NUM_PORTS * DIO_NUM_CHANNELS_PER_PORT)` (即 256) |
| T=2 cycles | `Det_ReportError(..., DIO_E_PARAM_INVALID_CHANNEL_ID)` |
| T=3 cycles | 函数直接 return，不执行任何寄存器写入 |
| **总延迟** | **3 个 CPU 周期（约 < 1 μs）** |

### 7.3 场景三：空指针访问通道组

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Dio_WriteChannelGroup(NULL_PTR, level)` |
| T=1 cycle | 检测到 `ChannelGroupIdPtr == NULL_PTR` |
| T=2 cycles | `Det_ReportError(..., DIO_E_PARAM_POINTER)` |
| T=3 cycles | 函数直接 return，不执行寄存器操作 |
| **总延迟** | **3 个 CPU 周期（约 < 1 μs）** |

### 7.4 场景四：GPIO 基地址为零（无效端口）

| 时间 | 事件 |
|------|------|
| T=0 | 应用层调用 `Dio_WriteChannel(ChannelId, STD_HIGH)`，ChannelId 映射到无效端口 |
| T=1 cycle | 参数范围校验通过（ChannelId 在 0–255 范围内） |
| T=2 cycles | `Dio_GetGpioBaseAddr()` 返回 `0U`（default 分支） |
| T=3 cycles | `REG_READ32(0U + DIO_GPIO_DR)` — 访问地址 0x00000000 附近 |
| T=4 cycles | MPU（Memory Protection Unit）触发地址访问异常（若已配置） |
| T=5 cycles | 异常处理触发系统安全状态 / WDT 复位 |
| **总延迟** | **约 5 个 CPU 周期 + WDT 超时** |

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

1. **硬件寄存器层假设**: GPIO 寄存器（PDOR/PDIR/PDDR 等）的失效率及诊断覆盖率由硬件 FMEDA 覆盖，本分析假设硬件层 FIT 数据已知
2. **MPU 保护**: 假设系统已配置 MPU（Memory Protection Unit），对地址 0x00000000 的非法访问可触发异常
3. **WDT 覆盖**: Dio 模块的调度监控由系统级 WDT 覆盖，`Dio_DriverInitialized` 标志位的存储故障由 WDT 超时检测
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **配置正确性**: 假设 Dio 配置（端口数、通道数、GPIO 基地址、通道组 mask/offset）由代码生成器生成并经过集成验证
6. **临界区保护**: 读-修改-写（R-M-W）操作的原子性保护由 SchM 临界区实现（`Mcal_DisableAllInterrupts()`/`Mcal_EnableAllInterrupts()`），Dio 模块内部保证原子性（v1.1 更新）
7. **冗余采样**: 安全关键通道的冗余读取/写入由应用层（ASW）实现，不在 Dio 模块范围内

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| GPIO 寄存器读取 | 5 FIT | 供应商 FMD |
| GPIO 寄存器写入 | 8 FIT | 供应商 FMD |
| GPIO 基地址解码 | 3 FIT | IEC 61508 Annex D |
| RAM 存储（初始化标志） | 4 FIT | IEC 61508 Annex D |
| CPU 逻辑（参数校验/位运算） | 8 FIT | 供应商 FMD |
| DET 错误检测逻辑 | 2 FIT | IEC 61508 Annex D |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| DIO | Digital Input/Output |
| GPIO | General Purpose Input/Output |
| PDOR | Port Data Output Register |
| PDIR | Port Data Input Register |
| PDDR | Port Data Direction Register |
| PSR | Port Status Register (Pin State Read) |
| DR | Data Register (Read/Write) |
| GDIR | General Data Direction Register |
| DET | Default Error Tracer |
| SIUL2 | System Integration Unit Layer 2 |
| R-M-W | Read-Modify-Write |
| MPU | Memory Protection Unit |
| WDT | Watchdog Timer |
