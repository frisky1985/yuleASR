# I2c Design Document

> **Module ID**: 0x57
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_I2c  
> **Source Path**: `src/bsw/mcal/i2c/`  
> **Reference Document**: `docs/modules/i2c.md`  
> **Doc Version**: 1.0  
> **Status**: 已完成

---

## 1. 模块概述

I2C（I2C总线驱动）是 AUTOSAR MCAL 层驱动模块。提供I2C主/从模式下的初始化、发送、接收和状态管理服务。

在 AUTOSAR 分层架构中，I2C 位于微控制器驱动层（MCAL），直接操作芯片硬件寄存器，向上为 ECUAL 层和 Services 层提供标准化硬件抽象接口。主要交互模块包括：

- **上层**：EcuM（初始化管理）、SchM（调度）以及使用该驱动的 ECUAL/Services 模块（如 CanIf、LinIf、IoHwAb 等）。
- **同层**：Port、DIO、MCU、GPT 等其它 MCAL 驱动（部分模块存在硬件依赖或时序依赖）。
- **下层**：微控制器硬件寄存器、中断控制器、DMA（若支持）。
- **公共**：Det（开发错误检测，可选）、Dem（诊断事件，可选）。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS I2c | 4.4.0 | 模块软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | CanIf / LinIf / IoHwAb 等 | 使用 I2c 提供硬件服务 | |
| 下层 | 微控制器硬件 | 直接操作芯片寄存器 | |
| 同层 | Port / Mcu / Gpt 等 | 时序或引脚依赖 | |
| 公共 | Det, Dem | 错误追踪与诊断事件（可选） | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        ECUAL / Services / ASW       │
├─────────────────────────────────────┤
│           I2c Driver (MCAL)        │
├─────────────────────────────────────┤
│      Microcontroller Hardware       │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **公共头文件 `I2c.h`**：定义模块 API、服务 ID、错误码、数据类型。
- **配置文件 `I2c_Cfg.h`**：预编译配置开关、数量宏、外部配置表声明。
- **实现文件 `I2c.c`**：API 实现、硬件访问、状态管理、错误处理。
- **链接时配置 `I2c_Lcfg.c`**（若存在）：由配置工具生成的链接时配置数据。
- **中断/硬件抽象文件**（若存在）：处理中断分发或平台相关硬件访问。

### 3.3 文件结构

```
src/bsw/mcal/i2c/
include
    ├── I2c.h
    ├── I2c_Cfg.h
src
    ├── I2c.c
```

---

## 4. 状态机

I2c 模块在初始化后从 `UNINIT` 状态转换为 `INIT` 状态；各通道/实例在启用后进入运行状态。

```
[UNINIT] -- Init() --> [INIT]
[INIT]   -- Enable/Start --> [RUNNING]
[RUNNING]-- Disable/Stop  --> [STOPPED]
```

对于本模块，状态机相对简单，未实现复杂分层状态转换时，本节仍保留以说明基本生命周期。

---

## 5. 核心数据结构

关键类型定义如下：

```c
typedef struct {...} I2c_SlaveAddressConfigType;
```

```c
typedef struct {...} I2c_MasterConfigType;
```

```c
typedef struct {...} I2c_SlaveConfigType;
```

```c
typedef struct {...} I2c_SmbusConfigType;
```

```c
typedef struct {...} I2c_DmaConfigType;
```

```c
typedef struct {...} I2c_InterruptConfigType;
```

```c
typedef struct {...} I2c_ChannelConfigType;
```

```c
typedef struct {...} I2c_ConfigType;
```

```c
typedef struct {...} I2c_DataBufferType;
```

```c
typedef struct {...} I2c_TransferRequestType;
```

```c
typedef enum {...} I2c_StatusType;
```

```c
typedef enum {...} I2c_OpModeType;
```


---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 | SWS ID |
|-----|------|------|----------------|--------|
| I2c_Init | `void I2c_Init(const I2c_ConfigType* Config)` | _Init | 见 I2c.h | SWS_I2c_00001 | SWS_I2c_00001 |
| I2c_DeInit | `Std_ReturnType I2c_DeInit(void)` | _DeInit | 见 I2c.h | SWS_I2c_00002 | SWS_I2c_00002 |
| I2c_WriteBytes | `Std_ReturnType I2c_WriteBytes(I2c_ChannelType Channel, I2c_AddressType SlaveAddress, const I2c_DataType* DataBuffer, I2c_LengthType Length, I2c_AddrModeType AddrMode)` | _WriteBytes | 见 I2c.h | SWS_I2c_00003 | SWS_I2c_00003 |
| I2c_ReadBytes | `Std_ReturnType I2c_ReadBytes(I2c_ChannelType Channel, I2c_AddressType SlaveAddress, I2c_DataType* DataBuffer, I2c_LengthType Length, I2c_AddrModeType AddrMode)` | _ReadBytes | 见 I2c.h | SWS_I2c_00004 | SWS_I2c_00004 |
| I2c_WriteRead | `Std_ReturnType I2c_WriteRead(I2c_ChannelType Channel, I2c_AddressType SlaveAddress, const I2c_DataType* TxBuffer, I2c_LengthType TxLength, I2c_DataType* RxBuffer, I2c_LengthType RxLength, I2c_AddrModeType AddrMode)` | _WriteRead | 见 I2c.h | SWS_I2c_00005 | SWS_I2c_00005 |
| I2c_GetStatus | `I2c_StatusType I2c_GetStatus(void)` | _GetStatus | 见 I2c.h | SWS_I2c_00006 | SWS_I2c_00006 |
| I2c_GetVersionInfo | `void I2c_GetVersionInfo(Std_VersionInfoType* versioninfo)` | _GetVersionInfo | 见 I2c.h | SWS_I2c_00007 | SWS_I2c_00007 |
| I2c_SetClockMode | `Std_ReturnType I2c_SetClockMode(I2c_ChannelType Channel, I2c_ClockModeType ClockMode)` | _SetClockMode | 见 I2c.h | SWS_I2c_00008 | SWS_I2c_00008 |
| I2c_EnableInterrupt | `Std_ReturnType I2c_EnableInterrupt(I2c_ChannelType Channel)` | _EnableInterrupt | 见 I2c.h | SWS_I2c_00009 | SWS_I2c_00009 |
| I2c_DisableInterrupt | `Std_ReturnType I2c_DisableInterrupt(I2c_ChannelType Channel)` | _DisableInterrupt | 见 I2c.h | SWS_I2c_00010 | SWS_I2c_00010 |
| I2c_SetSlaveAddress | `Std_ReturnType I2c_SetSlaveAddress(I2c_ChannelType Channel, I2c_AddressType SlaveAddress, I2c_AddrModeType AddrMode)` | _SetSlaveAddress | 见 I2c.h | SWS_I2c_00011 | SWS_I2c_00011 |
| I2c_GetBusState | `I2c_BusStateType I2c_GetBusState(I2c_ChannelType Channel)` | _GetBusState | 见 I2c.h | SWS_I2c_00012 | SWS_I2c_00012 |
| I2c_ClearBus | `Std_ReturnType I2c_ClearBus(I2c_ChannelType Channel)` | _ClearBus | 见 I2c.h | SWS_I2c_00013 | SWS_I2c_00013 |
| I2c_SoftwareReset | `Std_ReturnType I2c_SoftwareReset(I2c_ChannelType Channel)` | _SoftwareReset | 见 I2c.h | SWS_I2c_00014 | SWS_I2c_00014 |
| I2c_SetTransferMode | `Std_ReturnType I2c_SetTransferMode(I2c_ChannelType Channel, I2c_TransferModeType TransferMode)` | _SetTransferMode | 见 I2c.h | SWS_I2c_00015 | SWS_I2c_00015 |
| I2c_CancelTransfer | `Std_ReturnType I2c_CancelTransfer(I2c_ChannelType Channel)` | _CancelTransfer | 见 I2c.h | SWS_I2c_00016 | SWS_I2c_00016 |
| I2c_PrepareSlaveBuffer | `Std_ReturnType I2c_PrepareSlaveBuffer(I2c_ChannelType Channel, I2c_DataType* Buffer, I2c_LengthType Length)` | _PrepareSlaveBuffer | 见 I2c.h | SWS_I2c_00017 | SWS_I2c_00017 |
| I2c_SlaveWriteBuffer | `Std_ReturnType I2c_SlaveWriteBuffer(I2c_ChannelType Channel, const I2c_DataType* Buffer, I2c_LengthType Length)` | _SlaveWriteBuffer | 见 I2c.h | SWS_I2c_00018 | SWS_I2c_00018 |
| I2c_SlaveReadBuffer | `Std_ReturnType I2c_SlaveReadBuffer(I2c_ChannelType Channel, I2c_DataType* Buffer, I2c_LengthType Length)` | _SlaveReadBuffer | 见 I2c.h | SWS_I2c_00019 | SWS_I2c_00019 |
| I2c_MainFunction | `void I2c_MainFunction(void)` | _MainFunction | 见 I2c.h | SWS_I2c_00020 | SWS_I2c_00020 |

### 6.2 回调函数

| 回调 | 说明 | |
|------|------|
| I2c_Notification / I2c_Cbk | 若配置启用，由中断或状态变化触发 | |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 | SWS ID |
|-----|-----|------------|--------|
| (0x00U) | INIT | 见实现 | |
| (0x01U) | DEINIT | 见实现 | |
| (0x02U) | WRITEBYTES | 见实现 | |
| (0x03U) | READBYTES | 见实现 | |
| (0x04U) | WRITEREAD | 见实现 | |
| (0x05U) | GETSTATUS | 见实现 | |
| (0x06U) | GETVERSIONINFO | 见实现 | |
| (0x07U) | SETCLOCKMODE | 见实现 | |
| (0x08U) | ENABLEINTERRUPT | 见实现 | |
| (0x09U) | DISABLEINTERRUPT | 见实现 | |
| (0x0AU) | SETSLAVEADDRESS | 见实现 | |
| (0x0BU) | GETBUSSTATE | 见实现 | |
| (0x0CU) | CLEARBUS | 见实现 | |
| (0x0DU) | SOFTWARERESET | 见实现 | |
| (0x0EU) | SETTRANSFERMODE | 见实现 | |
| (0x0FU) | MAINFUNCTION | 见实现 | |
| (0x10U) | CANCELTRANSFER | 见实现 | |
| (0x11U) | PREPARESLAVEBUFFER | 见实现 | |
| (0x12U) | SLAVEWRITEBUFFER | 见实现 | |
| (0x13U) | SLAVEREADBUFFER | 见实现 | |

开发错误码定义：

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| (0x01U) | PARAM_CHANNEL | 见 I2c.h | |
| (0x02U) | PARAM_POINTER | 见 I2c.h | |
| (0x03U) | PARAM_LENGTH | 见 I2c.h | |
| (0x04U) | PARAM_ADDRESS | 见 I2c.h | |
| (0x05U) | PARAM_MODE | 见 I2c.h | |
| (0x06U) | PARAM_CONFIG | 见 I2c.h | |
| (0x07U) | ALREADY_INITIALIZED | 见 I2c.h | |
| (0x08U) | UNINIT | 见 I2c.h | |
| (0x09U) | BUSY | 见 I2c.h | |
| (0x0AU) | TIMEOUT | 见 I2c.h | |
| (0x0BU) | ARBITRATION_LOST | 见 I2c.h | |
| (0x0CU) | BUS_ERROR | 见 I2c.h | |

---

## 7. 处理流程

### 7.1 初始化流程

1. EcuM 在启动阶段调用 `I2c_Init(ConfigPtr)`。
2. 若 `DEV_ERROR_DETECT == STD_ON`，校验 `ConfigPtr` 非空及版本信息。
3. 初始化硬件寄存器、全局状态变量和运行时数据结构。
4. 设置模块初始化标志，模块进入可操作状态。

### 7.2 正常操作/数据处理流程

1. 上层通过标准 API 请求服务（如读取、写入、发送、接收）。
2. 模块校验初始化状态、参数范围和指针有效性（DET 开启时）。
3. 访问硬件寄存器或调用硬件抽象层完成请求。
4. 同步 API 直接返回结果；异步 API 更新作业状态并通过中断/轮询完成。
5. 若启用通知，调用配置的回调函数。

### 7.3 错误处理流程

1. API 入口进行参数和状态检查。
2. 检测到错误时，通过 `Det_ReportError()` 报告开发错误。
3. 函数通常提前返回安全默认值或错误状态，避免影响硬件。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 | |
|----|--------|------|
| I2C_DEV_ERROR_DETECT | STD_ON | 配置开关 | |
| I2C_VERSION_INFO_API | STD_ON | 配置开关 | |
| I2C_INTERRUPT_SUPPORTED | STD_ON | 配置开关 | |
| I2C_DMA_SUPPORTED | STD_ON | 配置开关 | |
| I2C_10BIT_ADDRESS_SUPPORTED | STD_ON | 配置开关 | |
| I2C_16BIT_ADDRESS_SUPPORTED | STD_OFF | 配置开关 | |
| I2C_SMBUS_SUPPORTED | STD_ON | 配置开关 | |
| I2C_MULTI_MASTER_SUPPORTED | STD_ON | 配置开关 | |
| I2C_CLOCK_STRETCHING_SUPPORTED | STD_ON | 配置开关 | |
| I2C_SLAVE_MODE_SUPPORTED | STD_ON | 配置开关 | |
| I2C_GENERAL_CALL_SUPPORTED | STD_ON | 配置开关 | |
| I2C_SW_RESET_API | STD_ON | 配置开关 | |
| I2C_NUM_CHANNELS | (8U) | 数量/ID 宏 | |
| I2C_NUM_HW_UNITS | (4U) | 数量/ID 宏 | |
| I2C_NUM_DMA_CHANNELS | (8U) | 数量/ID 宏 | |
| I2C_INTERRUPT_PRIORITY | (5U) | 其它配置 | |
| I2C_ERROR_INTERRUPT_PRIORITY | (4U) | 其它配置 | |
| I2C_DMA_TX_INTERRUPT_PRIORITY | (6U) | 其它配置 | |
| I2C_DMA_RX_INTERRUPT_PRIORITY | (6U) | 其它配置 | |
| I2C_BUS_BUSY_TIMEOUT_MS | (100U) | 其它配置 | |
| I2C_TRANSFER_TIMEOUT_MS | (1000U) | 其它配置 | |
| I2C_CLOCK_STRETCH_TIMEOUT_MS | (50U) | 其它配置 | |
| I2C_SMBUS_TIMEOUT_MS | (35U) | 其它配置 | |

### 8.2 链接时配置

| 配置表 | 说明 | |
|--------|------|
| `I2c_Lcfg.c` | 由 yuleASR Configurator 生成的链接时配置数据 | |
| `I2c_Config` | 外部配置根结构体声明（位于 `I2c_Cfg.h`） | |

### 8.3 构建后配置

本模块当前未使用 Post-Build 配置；所有配置在编译/链接时确定。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| (0x01U) | PARAM_CHANNEL | 参数或状态错误 | |
| (0x02U) | PARAM_POINTER | 参数或状态错误 | |
| (0x03U) | PARAM_LENGTH | 参数或状态错误 | |
| (0x04U) | PARAM_ADDRESS | 参数或状态错误 | |
| (0x05U) | PARAM_MODE | 参数或状态错误 | |
| (0x06U) | PARAM_CONFIG | 参数或状态错误 | |
| (0x07U) | ALREADY_INITIALIZED | 参数或状态错误 | |
| (0x08U) | UNINIT | 参数或状态错误 | |
| (0x09U) | BUSY | 参数或状态错误 | |
| (0x0AU) | TIMEOUT | 参数或状态错误 | |
| (0x0BU) | ARBITRATION_LOST | 参数或状态错误 | |
| (0x0CU) | BUS_ERROR | 参数或状态错误 | |

### 9.2 DEM 错误

本模块当前未定义专用 DEM 事件；相关硬件故障可通过下层或上层模块上报。

### 9.3 安全机制

- ASIL 等级：视具体安全项目分配，通常与使用该模块的上层安全相关功能一致。
- 参数校验：在 `DEV_ERROR_DETECT == STD_ON` 时执行空指针、越界、未初始化检查。
- 安全相关模块（如 Wdg、RamTst）需结合项目安全手册进行额外分析。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 | |
|------|------|
| I2C_START_SEC_CODE | MemMap 代码/数据段 | |
| I2C_START_SEC_VAR_CLEARED_UNSPECIFIED | MemMap 代码/数据段 | |
| I2C_STOP_SEC_CODE | MemMap 代码/数据段 | |
| I2C_STOP_SEC_VAR_CLEARED_UNSPECIFIED | MemMap 代码/数据段 | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | 依赖配置 | 运行时状态变量、通道/作业句柄 | |
| ROM | 依赖配置 | 代码段 + 链接时配置表 | |
| 堆栈 | 依赖调用深度 | 通常为浅层调用，中断处理另计 | |

---

## 11. 集成指南

- **与上层模块集成**：I2c 向上层提供标准 API；上层模块（如 CanIf、LinIf、IoHwAb）在初始化后调用 I2c 服务。
- **与下层硬件集成**：直接访问微控制器外设寄存器，具体寄存器映射与目标平台（NXP S32K312 / i.MX8M Mini 等）相关。
- **初始化顺序**：I2c 通常在 EcuM 的驱动初始化阶段调用；若依赖 Port/Mcu/Gpt，应确保这些模块先完成初始化。
- **中断配置**：若模块使用中断，需在 OS/启动代码中配置对应中断向量，并在 I2c_Irq.c（若存在）中实现 ISR。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 | |
|----------|----------|
| `test_i2c.c` | 初始化、API 参数边界、错误处理、MemMap 段 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 与上层模块联调 | 验证 I2c 与 CanIf/LinIf/IoHwAb 等接口行为 | |
| 中断时序测试 | 验证异步操作完成与回调触发时序 | |
| 错误注入测试 | 验证 DET 报告和错误恢复行为 | |

---

## 13. 实现说明 / TODO

- 本设计文档基于当前 `src/bsw/mcal/i2c/` 源码自动生成并人工校对。
- 若源码与 AUTOSAR SWS 存在偏差，以源码实现为准并在实现注释中说明。
- 平台相关寄存器定义可能分散在平台头文件或条件编译块中，集成时需结合具体目标芯片手册。

---

## 14. 参考资料

1. AUTOSAR_SWS_I2c.pdf
2. `docs/modules/i2c.md`
3. `src/bsw/mcal/i2c/`
4. `docs/design/modules/TEMPLATE.md`
