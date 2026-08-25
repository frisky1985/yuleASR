# Adc Design Document

> **Module ID**: 0x0E
> **AUTOSAR Layer**: MCAL
> **AUTOSAR Version**: Classic Platform 4.4.0
> **SWS Reference**: AUTOSAR_SWS_Adc
> **Source Path**: `src/bsw/mcal/adc/`
> **Reference Document**: `docs/modules/ADC.md`
> **Doc Version**: 1.0
> **Status**: Draft

---

## 1. 模块概述

Adc（Analog-to-Digital Converter Driver）位于 MCAL 层，负责把 MCU 的模拟输入转换为数字量。模块向上层（如 IoHwAb 或应用）提供按 Group 组织的采样服务，支持软件触发、硬件触发、组通知（Notification）和流式采样；向下依赖 Mcu 提供时钟、Port 提供引脚配置。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS Adc | 4.4.0 | ADC 软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | IoHwAb / Application | 读取 ADC 结果 | |
| 下层 | Mcu | 时钟使能/门控 | |
| 同层 | Port | ADC 引脚复用配置 | |
| 公共 | Det | 开发错误检测 | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        IoHwAb / Application         │
├─────────────────────────────────────┤
│           Adc (MCAL)                │
├─────────────────────────────────────┤
│     Mcu / Port / GPIO / ADC HW      │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **HW Unit Manager**：管理 ADC 硬件单元（时钟、寄存器初始化、Calibration）。
- **Group Conversion Engine**：按 Group 启动/停止转换，遍历 Channel 完成采样。
- **Result Buffer**：保存每组各通道的转换结果。
- **Notification Handler**：转换完成后调用组级回调。
- **Power State Manager**：提供 Set/Get/Prepare PowerState 接口（当前为 stub）。

### 3.3 文件结构

```
src/bsw/mcal/adc/
├── include/
│   ├── Adc.h
│   └── Adc_Cfg.h
└── src/
    └── Adc.c
```

---

## 4. 状态机

### 4.1 驱动初始化状态

```
UNINIT -- Adc_Init() --> INITIALIZED
INITIALIZED -- Adc_DeInit() --> UNINIT
```

### 4.2 Group 转换状态

```
IDLE -- StartGroupConversion() --> BUSY
BUSY -- 转换完成 --> STREAM_COMPLETED
BUSY -- StopGroupConversion() --> IDLE
STREAM_COMPLETED -- StartGroupConversion() --> BUSY
STREAM_COMPLETED -- StopGroupConversion() --> IDLE
```

---

## 5. 核心数据结构

```c
typedef uint8  Adc_HWUnitType;
typedef uint8  Adc_ChannelType;
typedef uint8  Adc_GroupType;
typedef uint16 Adc_ValueGroupType;

typedef enum {
    ADC_IDLE = 0,
    ADC_BUSY,
    ADC_STREAM_COMPLETED
} Adc_StatusType;

typedef enum {
    ADC_TRIGG_SRC_SW = 0,
    ADC_TRIGG_SRC_HW
} Adc_TriggerSourceType;

typedef enum {
    ADC_CONV_MODE_ONESHOT = 0,
    ADC_CONV_MODE_CONTINUOUS
} Adc_ConversionModeType;

typedef enum {
    ADC_STREAM_BUFFER_LINEAR = 0,
    ADC_STREAM_BUFFER_CIRCULAR
} Adc_StreamBufferModeType;

typedef enum {
    ADC_ACCESS_MODE_SINGLE = 0,
    ADC_ACCESS_MODE_STREAMING
} Adc_GroupAccessModeType;

typedef enum {
    ADC_RESOLUTION_6BIT = 0,
    ADC_RESOLUTION_8BIT,
    ADC_RESOLUTION_10BIT,
    ADC_RESOLUTION_12BIT
} Adc_ResolutionType;

typedef enum {
    ADC_FULL_POWER = 0,
    ADC_LOW_POWER
} Adc_PowerStateType;

typedef struct {
    Adc_ChannelType    ChannelId;
    Adc_SamplingTimeType SamplingTime;
    uint8              ChannelInput;
} Adc_ChannelConfigType;

typedef struct {
    Adc_GroupType        GroupId;
    Adc_HWUnitType       HwUnit;
    const Adc_ChannelType* Channels;
    uint8                NumChannels;
    Adc_TriggerSourceType TriggerSource;
    Adc_ConversionModeType ConversionMode;
    Adc_GroupAccessModeType AccessMode;
    Adc_StreamBufferModeType BufferMode;
    Adc_StreamNumSampleType NumSamples;
    Adc_ResolutionType   Resolution;
    boolean              GroupNotification;
    void (*NotificationFn)(void);
} Adc_GroupConfigType;

typedef struct {
    Adc_HWUnitType HwUnitId;
    uint32         BaseAddress;
    uint32         ClockFrequency;
    Adc_ResolutionType DefaultResolution;
} Adc_HWUnitConfigType;

typedef struct {
    const Adc_HWUnitConfigType* HwUnits;
    uint8              NumHwUnits;
    const Adc_GroupConfigType* Groups;
    uint8              NumGroups;
    const Adc_ChannelConfigType* Channels;
    uint8              NumChannels;
    boolean            DevErrorDetect;
    boolean            VersionInfoApi;
    boolean            DeInitApi;
    boolean            PowerStateSupported;
} Adc_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 | SWS ID |
|-----|------|------|----------------|--------|
| Adc_Init | `void Adc_Init(const Adc_ConfigType* ConfigPtr)` | 初始化 ADC 驱动 | 需传入有效配置指针 | SWS_Adc_00001 | SWS_Adc_00001 |
| Adc_DeInit | `void Adc_DeInit(void)` | 反初始化 | 需无 Group 处于 BUSY | SWS_Adc_00002 | SWS_Adc_00002 |
| Adc_StartGroupConversion | `void Adc_StartGroupConversion(Adc_GroupType Group)` | 启动组转换 | 软件触发 | SWS_Adc_00003 | SWS_Adc_00003 |
| Adc_StopGroupConversion | `void Adc_StopGroupConversion(Adc_GroupType Group)` | 停止组转换 | | SWS_Adc_00004 | SWS_Adc_00004 |
| Adc_ReadGroup | `Std_ReturnType Adc_ReadGroup(Adc_GroupType Group, Adc_ValueGroupType* DataBufferPtr)` | 读取组结果 | | SWS_Adc_00005 | SWS_Adc_00005 |
| Adc_EnableHardwareTrigger | `void Adc_EnableHardwareTrigger(Adc_GroupType Group)` | 使能硬件触发 | 仅当 TriggerSource=HW | SWS_Adc_00006 | SWS_Adc_00006 |
| Adc_DisableHardwareTrigger | `void Adc_DisableHardwareTrigger(Adc_GroupType Group)` | 禁用硬件触发 | | SWS_Adc_00007 | SWS_Adc_00007 |
| Adc_EnableGroupNotification | `void Adc_EnableGroupNotification(Adc_GroupType Group)` | 使能组通知 | 当前为 stub | SWS_Adc_00008 | SWS_Adc_00008 |
| Adc_DisableGroupNotification | `void Adc_DisableGroupNotification(Adc_GroupType Group)` | 禁用组通知 | 当前为 stub | SWS_Adc_00009 | SWS_Adc_00009 |
| Adc_GetGroupStatus | `Adc_StatusType Adc_GetGroupStatus(Adc_GroupType Group)` | 获取组状态 | | SWS_Adc_00010 | SWS_Adc_00010 |
| Adc_GetVersionInfo | `void Adc_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 获取版本信息 | | SWS_Adc_00011 | SWS_Adc_00011 |
| Adc_GetStreamLastPointer | `Adc_StreamNumSampleType Adc_GetStreamLastPointer(Adc_GroupType Group, Adc_ValueGroupType** PtrToSamplePtr)` | 获取流式采样指针 | | SWS_Adc_00012 | SWS_Adc_00012 |
| Adc_SetupResultBuffer | `Std_ReturnType Adc_SetupResultBuffer(Adc_GroupType Group, Adc_ValueGroupType* DataBufferPtr)` | 设置结果缓冲区 | 当前为 stub | SWS_Adc_00013 | SWS_Adc_00013 |
| Adc_SetPowerState | `void Adc_SetPowerState(Adc_PowerStateType PowerState, Adc_PowerStateRequestResultType* Result)` | 设置电源状态 | 当前为 stub | SWS_Adc_00014 | SWS_Adc_00014 |
| Adc_GetTargetPowerState | `void Adc_GetTargetPowerState(Adc_PowerStateType* TargetPowerState, Adc_PowerStateRequestResultType* Result)` | 获取目标电源状态 | 当前为 stub | SWS_Adc_00015 | SWS_Adc_00015 |
| Adc_GetCurrentPowerState | `void Adc_GetCurrentPowerState(Adc_PowerStateType* CurrentPowerState, Adc_PowerStateRequestResultType* Result)` | 获取当前电源状态 | 当前为 stub | SWS_Adc_00016 | SWS_Adc_00016 |
| Adc_PreparePowerState | `void Adc_PreparePowerState(Adc_PowerStateType PowerState, Adc_PowerStateRequestResultType* Result)` | 准备电源状态切换 | 当前为 stub | SWS_Adc_00017 | SWS_Adc_00017 |

### 6.2 回调函数

| 回调 | 说明 | |
|------|------|
| `NotificationFn`（GroupConfig 中注册） | 组转换完成后调用，由用户配置提供 | |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 | SWS ID |
|-----|-----|------------|--------|
| 0x00 | Adc_Init | ADC_E_PARAM_CONFIG / ADC_E_ALREADY_INITIALIZED | SWS_Adc_00001 | SWS_Adc_00018 |
| 0x01 | Adc_DeInit | ADC_E_UNINIT | SWS_Adc_00002 | SWS_Adc_00019 |
| 0x02 | Adc_StartGroupConversion | ADC_E_UNINIT / ADC_E_PARAM_GROUP | SWS_Adc_00003 | SWS_Adc_00020 |
| 0x03 | Adc_StopGroupConversion | ADC_E_UNINIT / ADC_E_PARAM_GROUP | SWS_Adc_00004 | SWS_Adc_00021 |
| 0x04 | Adc_ReadGroup | ADC_E_UNINIT / ADC_E_PARAM_GROUP / ADC_E_PARAM_POINTER | SWS_Adc_00005 | SWS_Adc_00022 |
| 0x05/0x06 | Adc_Enable/DisableHardwareTrigger | ADC_E_UNINIT / ADC_E_PARAM_GROUP | | SWS_Adc_00023 |
| 0x07/0x08 | Adc_Enable/DisableGroupNotification | ADC_E_UNINIT / ADC_E_PARAM_GROUP | | SWS_Adc_00024 |
| 0x09 | Adc_GetGroupStatus | ADC_E_UNINIT / ADC_E_PARAM_GROUP | SWS_Adc_00010 | SWS_Adc_00025 |
| 0x0A | Adc_GetVersionInfo | ADC_E_PARAM_POINTER | SWS_Adc_00011 | SWS_Adc_00026 |
| 0x0B | Adc_GetStreamLastPointer | ADC_E_UNINIT / ADC_E_PARAM_GROUP / ADC_E_PARAM_POINTER | SWS_Adc_00012 | SWS_Adc_00027 |
| 0x0C | Adc_SetupResultBuffer | ADC_E_UNINIT / ADC_E_PARAM_GROUP / ADC_E_PARAM_POINTER | SWS_Adc_00013 | SWS_Adc_00028 |
| 0x0E~0x11 | Power State APIs | ADC_E_UNINIT / ADC_E_PARAM_POINTER | |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 `ConfigPtr` 非空，否则报 `ADC_E_PARAM_CONFIG`。
2. 检查未重复初始化，否则报 `ADC_E_ALREADY_INITIALIZED`。
3. 保存配置指针到 `Adc_ConfigPtr`。
4. 遍历每个 HW Unit：
   - 获取 base address；
   - 使能时钟（当前为平台 stub）；
   - 写 `ADC_CFG`（IPG clock、12-bit mode、clock divide by 2）；
   - 写 `ADC_GC` 使能模块并执行 Calibration（等待 `ADACT` 清零）。
5. 所有 Group 状态置为 `ADC_IDLE`。
6. `Adc_DriverInitialized = TRUE`。

### 7.2 组转换流程

1. 检查驱动已初始化、Group 索引合法。
2. 若 Group 已在 `ADC_BUSY` 则直接返回。
3. 获取对应 HW Unit base address，状态置 `ADC_BUSY`。
4. 清 `ADC_GC.ADTRG` 选择软件触发。
5. 遍历组内 Channel：
   - 写 `ADC_HC0` 选择通道；
   - 置 `ADC_GC.ADCONV` 启动转换；
   - 轮询 `ADC_HS.COCO0` 等待完成；
   - 读 `ADC_R0` 低 12bit 保存到 `Adc_GroupResults[Group][i]`。
6. 状态置 `ADC_STREAM_COMPLETED`。
7. 若 `GroupNotification` 使能且回调非空，调用 `NotificationFn()`。

### 7.3 读取结果流程

1. 校验初始化、Group 索引、结果指针。
2. 按组内通道数把 `Adc_GroupResults[Group][i]` 拷贝到用户缓冲区。
3. 返回 `E_OK`。

### 7.4 反初始化流程

1. 检查驱动已初始化。
2. 检查无 Group 处于 `ADC_BUSY`，否则直接返回。
3. 遍历 HW Unit，写 `ADC_GC=0` 关闭 ADC，禁用时钟 stub。
4. `Adc_DriverInitialized = FALSE`。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值/示例 | 说明 | |
|----|-------------|------|
| `ADC_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 | |
| `ADC_VERSION_INFO_API` | STD_ON | 版本信息 API | |
| `ADC_NUM_HW_UNITS` | 2U | HW Unit 数量 | |
| `ADC_NUM_GROUPS` | 8U | Group 数量 | |
| `ADC_NUM_CHANNELS` | 16U | Channel 数量 | |
| `ADC_STREAM_NUM_SAMPLES` | 1U | 流采样数 | |
| `ADC_MAIN_FUNCTION_PERIOD_MS` | 10U | 主函数周期 | |
| `ADC_DE_INIT_API` | STD_ON | DeInit API 开关 | |
| `ADC_ENABLE_START_STOP_GROUP_API` | STD_ON | 启停 Group API | |
| `ADC_HW_TRIGGER_API` | STD_ON | 硬件触发 API | |
| `ADC_READ_GROUP_API` | STD_ON | ReadGroup API | |
| `ADC_GET_STREAM_LAST_POINTER_API` | STD_ON | StreamLastPointer API | |
| `ADC_ENABLE_QUEUING` | STD_ON | 队列支持 | |
| `ADC_GRP_NOTIF_CAPABILITY` | STD_ON | Group 通知能力 | |
| `ADC_POWER_STATE_SUPPORTED` | STD_OFF | 电源状态支持 | |
| `ADC_DEFAULT_RESOLUTION` | ADC_RESOLUTION_12BIT | 默认分辨率 | |
| `ADC_DEFAULT_SAMPLING_TIME` | ADC_SAMPLING_TIME_15CYCLES | 默认采样时间 | |
| `ADC_CLOCK_FREQUENCY_HZ` | 24000000U | ADC 时钟频率 | |

### 8.2 链接时配置

`Adc_ConfigType Adc_Config` 在配置生成阶段实例化，并通过 `Adc.h` 以 `extern const Adc_ConfigType Adc_Config;` 暴露给驱动。

### 8.3 构建后配置

当前实现面向预编译配置，不支持 Post-Build 配置变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| 0x0A | ADC_E_UNINIT | 模块未初始化时调用 API | |
| 0x0B | ADC_E_BUSY | Group 正忙 | |
| 0x0C | ADC_E_IDLE | Group 空闲 | |
| 0x0D | ADC_E_ALREADY_INITIALIZED | 重复初始化 | |
| 0x0E | ADC_E_PARAM_POINTER | 空指针入参 | |
| 0x0F | ADC_E_PARAM_GROUP | Group 索引越界 | |
| 0x10 | ADC_E_PARAM_CHANNEL | 通道索引无效 | |
| 0x11 | ADC_E_PARAM_CONFIG | 配置指针为空 | |
| 0x12 | ADC_E_PARAM_TRIGGERSOURCE | 触发源不匹配 | |
| 0x13 | ADC_E_PARAM_BUFFER | 缓冲区无效 | |
| 0x14 | ADC_E_NOTIF_CAPABILITY | 通知能力不支持 | |
| 0x15 | ADC_E_POWER_STATE_NOT_SUPPORTED | 电源状态不支持 | |
| 0x16 | ADC_E_TRANSITION_NOT_POSSIBLE | 电源状态切换不可行 | |
| 0x17 | ADC_E_PERIPHERAL_NOT_PREPARED | 外设未准备好 | |

### 9.2 DEM 错误

当前未定义 DEM 事件。

### 9.3 安全机制

- 所有 API 在 `ADC_DEV_ERROR_DETECT == STD_ON` 时进行参数与状态校验。
- 转换过程为轮询等待，未启用中断，需注意看门狗喂狗。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 | |
|------|------|
| `ADC_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据段（`Adc_Config`） | |
| `ADC_STOP_SEC_CONFIG_DATA_UNSPECIFIED` | | |
| `ADC_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化变量（`Adc_DriverInitialized`、`Adc_GroupStatus`、`Adc_GroupResults`、`Adc_ConfigPtr`） | |
| `ADC_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | | |
| `ADC_START_SEC_CODE` | 代码段 | |
| `ADC_STOP_SEC_CODE` | | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | ~`(ADC_NUM_GROUPS * ADC_NUM_CHANNELS * 2) + (ADC_NUM_GROUPS * 1) + 指针` 字节 | 结果数组与状态数组 | |
| ROM | 配置结构 + 代码 | 随通道/组规模线性增长 | |
| 堆栈 | 较小 | 无递归，局部变量少 | |

---

## 11. 集成指南

- 在 `EcuM`/`Mcu` 初始化完成后、调用任何采样 API 前执行 `Adc_Init(&Adc_Config)`。
- ADC 引脚需在 `Port` 模块中配置为模拟输入。
- 若使用硬件触发，需同步配置 `Icu`/`Gpt` 等触发源。
- 通过 `Det` 使能开发错误检测以捕获参数错误。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 | |
|----------|----------|
| `test_adc.c` | 初始化、重复初始化、空指针、Group 越界、转换与读取、通知回调 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 与 IoHwAb 集成 | 验证上层通过 IoHwAb 读取 ADC 值 | |
| 与 Port 集成 | 确认引脚配置为模拟功能 | |
| 与 Mcu 集成 | 验证时钟使能后 ADC 能正常工作 | |

---

## 13. 实现说明 / TODO

- 当前源码中 `ADC_MODULE_ID` 定义为 `0x2CU`，与设计文档采用的 AUTOSAR 标准 ID `0x0E` 不一致；建议后续统一为标准 Module ID。
- `Adc_EnableClock`/`Adc_DisableClock` 为平台占位实现，需根据具体 MCU 时钟树补充。
- `ADC_POWER_STATE_SUPPORTED` 当前关闭，电源状态相关 API 为 stub。
- Group 通知、结果缓冲设置（`Adc_SetupResultBuffer`）为 stub，未真正改变运行时行为。
- 转换采用轮询等待 `COCO0`，未使用中断/DMA，实时性需评估。

---

## 14. 参考资料

1. AUTOSAR_SWS_ADC.pdf
2. `docs/modules/ADC.md`
3. `src/bsw/mcal/adc/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Adc | — | ADC 模块级需求归属 |
| SWS_Adc_00212 | `resolution` | 测试 test_resolution 覆盖: resolution 场景 |
| SWS_Adc_00213 | `sampling_time` | 测试 test_sampling_time 覆盖: sampling_time 场景 |
