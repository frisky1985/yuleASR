# 模块清单

## 概述

本文档提供 YuleTech AutoSAR BSW Platform 所有模块的详细说明。

## 模块统计

- **MCAL 层**: 9 个模块
- **ECUAL 层**: 9 个模块
- **Service 层**: 5 个模块
- **RTE 层**: 1 个模块
- **总计**: 24 个模块

---

## MCAL 层

### Mcu (Microcontroller Driver)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x01 |
| **功能** | 微控制器驱动 |
| **描述** | 提供微控制器初始化、时钟配置、复位控制等功能 |
| **主要 API** | Mcu_Init, Mcu_DistributePllClock, Mcu_SetMode |
| **状态** | ✅ 已完成 |

**功能特性：**
- 微控制器初始化
- PLL 时钟配置
- 复位原因获取
- 低功耗模式控制
- RAM 初始化

### Port (Port Driver)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x02 |
| **功能** | 端口驱动 |
| **描述** | 提供端口引脚配置和控制 |
| **主要 API** | Port_Init, Port_SetPinDirection, Port_RefreshPortDirection |
| **状态** | ✅ 已完成 |

### Dio (Digital I/O Driver)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x03 |
| **功能** | 数字 I/O 驱动 |
| **描述** | 提供数字输入/输出控制 |
| **主要 API** | Dio_ReadChannel, Dio_WriteChannel, Dio_FlipChannel |
| **状态** | ✅ 已完成 |

### Can (CAN Driver)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x04 |
| **功能** | CAN 驱动 |
| **描述** | 提供 CAN 控制器初始化和数据传输 |
| **主要 API** | Can_Init, Can_Write, Can_SetControllerMode |
| **状态** | ✅ 已完成 |

**功能特性：**
- CAN 控制器初始化
- 消息发送/接收
- 中断处理
- 总线关闭恢复
- 波特率配置

### Spi (SPI Driver)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x05 |
| **功能** | SPI 驱动 |
| **描述** | 提供 SPI 通信接口 |
| **主要 API** | Spi_Init, Spi_AsyncTransmit, Spi_GetHWUnitStatus |
| **状态** | ✅ 已完成 |

### Gpt (General Purpose Timer Driver)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x06 |
| **功能** | 通用定时器驱动 |
| **描述** | 提供定时器功能 |
| **主要 API** | Gpt_Init, Gpt_StartTimer, Gpt_StopTimer |
| **状态** | ✅ 已完成 |

### Pwm (PWM Driver)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x07 |
| **功能** | PWM 驱动 |
| **描述** | 提供 PWM 信号生成 |
| **主要 API** | Pwm_Init, Pwm_SetDutyCycle, Pwm_SetPeriodAndDuty |
| **状态** | ✅ 已完成 |

### Adc (ADC Driver)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x08 |
| **功能** | ADC 驱动 |
| **描述** | 提供模数转换功能 |
| **主要 API** | Adc_Init, Adc_StartGroupConversion, Adc_ReadGroup |
| **状态** | ✅ 已完成 |

### Wdg (Watchdog Driver)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x09 |
| **功能** | 看门狗驱动 |
| **描述** | 提供看门狗监控功能 |
| **主要 API** | Wdg_Init, Wdg_SetTriggerCondition, Wdg_GetVersionInfo |
| **状态** | ✅ 已完成 |

---

## ECUAL 层

### CanIf (CAN Interface)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x0A |
| **功能** | CAN 接口 |
| **描述** | 提供 CAN 硬件抽象和 PDU 路由 |
| **主要 API** | CanIf_Init, CanIf_Transmit, CanIf_SetControllerMode |
| **状态** | ✅ 已完成 |

**功能特性：**
- CAN 控制器管理
- PDU 发送/接收
- 总线关闭处理
- 控制器模式控制
- 动态 CAN ID 支持

### IoHwAb (I/O Hardware Abstraction)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x0B |
| **功能** | I/O 硬件抽象 |
| **描述** | 提供 I/O 信号抽象和处理 |
| **主要 API** | IoHwAb_Init, IoHwAb_AnalogRead, IoHwAb_DigitalRead |
| **状态** | ✅ 已完成 |

### CanTp (CAN Transport Protocol)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x0C |
| **功能** | CAN 传输协议 |
| **描述** | 提供 ISO-TP 传输协议支持 |
| **主要 API** | CanTp_Init, CanTp_Transmit, CanTp_RxIndication |
| **状态** | ✅ 已完成 |

### EthIf (Ethernet Interface)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x0D |
| **功能** | 以太网接口 |
| **描述** | 提供以太网通信接口 |
| **主要 API** | EthIf_Init, EthIf_Transmit, EthIf_RxIndication |
| **状态** | ✅ 已完成 |

### MemIf (Memory Interface)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x0E |
| **功能** | 存储器接口 |
| **描述** | 提供统一的存储器访问接口 |
| **主要 API** | MemIf_Init, MemIf_Read, MemIf_Write |
| **状态** | ✅ 已完成 |

### Fee (Flash EEPROM Emulation)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x0F |
| **功能** | Flash EEPROM 仿真 |
| **描述** | 在 Flash 上模拟 EEPROM |
| **主要 API** | Fee_Init, Fee_Read, Fee_Write |
| **状态** | ✅ 已完成 |

### Ea (EEPROM Abstraction)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x10 |
| **功能** | EEPROM 抽象 |
| **描述** | 提供 EEPROM 访问抽象 |
| **主要 API** | Ea_Init, Ea_Read, Ea_Write |
| **状态** | ✅ 已完成 |

### FrIf (FlexRay Interface)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x11 |
| **功能** | FlexRay 接口 |
| **描述** | 提供 FlexRay 通信接口 |
| **主要 API** | FrIf_Init, FrIf_Transmit, FrIf_RxIndication |
| **状态** | ✅ 已完成 |

### LinIf (LIN Interface)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x12 |
| **功能** | LIN 接口 |
| **描述** | 提供 LIN 通信接口 |
| **主要 API** | LinIf_Init, LinIf_Transmit, LinIf_RxIndication |
| **状态** | ✅ 已完成 |

---

## Service 层

### Com (Communication Services)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x64 |
| **功能** | 通信服务 |
| **描述** | 提供基于信号的通信服务 |
| **主要 API** | Com_Init, Com_SendSignal, Com_ReceiveSignal |
| **状态** | ✅ 已完成 |

**功能特性：**
- 信号发送/接收
- 信号组管理
- IPDU 传输控制
- 过滤和转换
- 更新位支持
- 死线监控

### PduR (PDU Router)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x69 |
| **功能** | PDU 路由器 |
| **描述** | 提供 PDU 路由功能 |
| **主要 API** | PduR_Init, PduR_Transmit, PduR_RxIndication |
| **状态** | ✅ 已完成 |

**功能特性：**
- 路由表管理
- 向上/向下路由
- 网关功能
- FIFO 队列管理
- 多目标路由

### NvM (NVRAM Manager)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x14 |
| **功能** | NVRAM 管理器 |
| **描述** | 提供非易失性存储管理 |
| **主要 API** | NvM_Init, NvM_ReadBlock, NvM_WriteBlock |
| **状态** | ✅ 已完成 |

**功能特性：**
- 块管理
- 读写操作
- CRC 校验
- 作业队列
- 冗余存储
- 数据恢复

### Dcm (Diagnostic Communication Manager)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x29 |
| **功能** | 诊断通信管理器 |
| **描述** | 提供 UDS 诊断服务 |
| **主要 API** | Dcm_Init, Dcm_MainFunction, Dcm_RxIndication |
| **状态** | ✅ 已完成 |

**功能特性：**
- UDS 服务支持
- 会话管理
- 安全访问控制
- DID/RID 处理
- DTC 管理

### Dem (Diagnostic Event Manager)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x54 |
| **功能** | 诊断事件管理器 |
| **描述** | 提供诊断事件和 DTC 管理 |
| **主要 API** | Dem_Init, Dem_SetEventStatus, Dem_GetDTCStatus |
| **状态** | ✅ 已完成 |

**功能特性：**
- 事件状态管理
- DTC 管理
- 去抖算法
- 老化机制
- 冻结帧支持

---

## RTE 层

### Rte (Runtime Environment)

| 属性 | 值 |
|:-----|:---|
| **模块 ID** | 0x70 |
| **功能** | 运行时环境 |
| **描述** | 提供组件间通信接口和调度 |
| **主要 API** | Rte_Init, Rte_Read, Rte_Write, Rte_Start |
| **状态** | ✅ 已完成 |

**功能特性：**
- 组件通信接口
- 数据类型定义
- 任务调度器
- COM/NVM 接口
- 模式管理
- 事件处理

---

## 模块依赖关系

### 垂直依赖

```
RTE
 ↑ 依赖
Service Layer (Com, PduR, NvM, Dcm, Dem)
 ↑ 依赖
ECUAL Layer (CanIf, IoHwAb, CanTp, ...)
 ↑ 依赖
MCAL Layer (Mcu, Port, Dio, Can, ...)
 ↑ 依赖
Hardware
```

### 水平依赖

#### COM 依赖
- PduR (发送/接收 PDU)

#### PduR 依赖
- CanIf (CAN 传输)
- Com (信号接收)
- Dcm (诊断传输)

#### NvM 依赖
- MemIf (存储器访问)

#### Dcm 依赖
- PduR (诊断 PDU 传输)
- Dem (DTC 管理)

#### Dem 依赖
- NvM (DTC 存储)

---

## 模块配置

### 配置参数

每个模块通过 `<Module>_Cfg.h` 进行配置：

```c
/* 开发错误检测 */
#define MODULE_DEV_ERROR_DETECT     (STD_ON)

/* 版本信息 API */
#define MODULE_VERSION_INFO_API     (STD_ON)

/* 模块特定配置 */
#define MODULE_MAX_INSTANCES        (4U)
#define MODULE_BUFFER_SIZE          (64U)
```

### 配置工具

配置可通过以下方式生成：
1. 手动编辑配置文件
2. 使用配置工具生成
3. 从 ARXML 导入

---

## 版本历史

| 版本 | 日期 | 变更描述 |
|:-----|:-----|:---------|
| 1.0.0 | 2026-04-15 | 初始版本，完整模块清单 |

---

**文档版本**: 1.0.0
**最后更新**: 2026-04-15
**作者**: YuleTech AutoSAR Team
