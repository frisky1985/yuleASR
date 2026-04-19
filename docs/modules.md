# 模块清单

## 概述

本文档提供 YuleTech AutoSAR BSW Platform 所有模块的详细说明。

## 模块统计

- **MCAL 层**: 9 个模块
- **ECUAL 层**: 9 个模块
- **Service 层**: 5 个模块
- **RTE 层**: 1 个模块
- **ASW 层**: 8 个组件
- **总计**: 32 个模块/组件

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

## ASW 层

### EngineControl (发动机控制组件)

| 属性 | 值 |
|:-----|:---|
| **组件 ID** | 0x80 |
| **功能** | 发动机控制 |
| **描述** | 提供发动机状态管理、燃油喷射、点火控制等功能 |
| **主要 Runnable** | Init, 10ms, 100ms, StateMachine |
| **状态** | ✅ 已完成 |

**功能特性：**
- 发动机状态机 (OFF/CRANKING/RUNNING/STOPPING/FAULT)
- 燃油喷射计算
- 点火正时计算
- 温度补偿
- 故障检测

### VehicleDynamics (车辆动力学组件)

| 属性 | 值 |
|:-----|:---|
| **组件 ID** | 0x81 |
| **功能** | 车辆动力学控制 |
| **描述** | 提供 VDC、牵引力控制、稳定性控制等功能 |
| **主要 Runnable** | Init, 10ms, 20ms |
| **状态** | ✅ 已完成 |

**功能特性：**
- VDC 状态管理
- 滑移率计算
- 横摆角速度控制
- 制动干预
- 牵引力控制

### DiagnosticManager (诊断管理组件)

| 属性 | 值 |
|:-----|:---|
| **组件 ID** | 0x82 |
| **功能** | 诊断服务管理 |
| **描述** | 提供诊断会话管理、安全访问、DTC 管理等功能 |
| **主要 Runnable** | Init, 50ms, ProcessRequest |
| **状态** | ✅ 已完成 |

**功能特性：**
- 诊断会话控制 (0x10)
- 安全访问 (0x27)
- DTC 信息管理 (0x19)
- 清除诊断信息 (0x14)
- 测试仪在线 (0x3E)

### CommunicationManager (通信管理组件)

| 属性 | 值 |
|:-----|:---|
| **组件 ID** | 0x83 |
| **功能** | 通信管理 |
| **描述** | 提供信号和 PDU 的收发管理 |
| **主要 Runnable** | Init, 10ms, RxProcess, TxProcess |
| **状态** | ✅ 已完成 |

**功能特性：**
- 信号发送/接收
- PDU 发送/接收
- 超时检测
- 统计信息

### StorageManager (存储管理组件)

| 属性 | 值 |
|:-----|:---|
| **组件 ID** | 0x84 |
| **功能** | 存储管理 |
| **描述** | 提供非易失性数据块管理 |
| **主要 Runnable** | Init, 100ms, WriteCycle |
| **状态** | ✅ 已完成 |

**功能特性：**
- 存储块读写
- CRC 校验
- 写周期计数
- 写保护
- 块擦除

### IOControl (IO 控制组件)

| 属性 | 值 |
|:-----|:---|
| **组件 ID** | 0x85 |
| **功能** | IO 控制 |
| **描述** | 提供数字/模拟/PWM IO 控制 |
| **主要 Runnable** | Init, 10ms, 50ms |
| **状态** | ✅ 已完成 |

**功能特性：**
- 数字输入/输出
- 模拟输入/输出
- PWM 输入/输出
- 输入消抖

### ModeManager (模式管理组件)

| 属性 | 值 |
|:-----|:---|
| **组件 ID** | 0x86 |
| **功能** | 系统模式管理 |
| **描述** | 提供系统模式管理和状态机协调 |
| **主要 Runnable** | Init, 50ms, ModeSwitch |
| **状态** | ✅ 已完成 |

**功能特性：**
- 系统模式管理
- 模式转换验证
- 组件通知机制
- 强制模式转换

### WatchdogManager (看门狗管理组件)

| 属性 | 值 |
|:-----|:---|
| **组件 ID** | 0x87 |
| **功能** | 看门狗管理 |
| **描述** | 提供看门狗监控和 Alive 监控 |
| **主要 Runnable** | Init, 10ms, Trigger |
| **状态** | ✅ 已完成 |

**功能特性：**
- 实体注册/注销
- Alive 监控
- Checkpoint 机制
- 超时检测
- 硬件看门狗触发

---

## ASW 组件接口

### 接口定义文件

所有 ASW 组件共享的接口定义位于：
- `src/asw/asw_interfaces.h` - 数据元素定义

### RTE 接口

每个组件通过 RTE 接口与其他组件通信：
- **Sender-Receiver 接口**: 用于数据交换
- **Client-Server 接口**: 用于服务调用
- **Mode Switch 接口**: 用于模式切换

---

## 完整架构图

```
┌─────────────────────────────────────────────────────────────┐
│                        ASW 层                                │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │
│  │ Engine  │ │Vehicle  │ │Diagnostic│ │Communication│      │
│  │ Control │ │Dynamics │ │ Manager │ │  Manager   │       │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘         │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │
│  │ Storage │ │  IO     │ │  Mode   │ │ Watchdog │         │
│  │ Manager │ │ Control │ │ Manager │ │ Manager  │         │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘          │
└───────┼───────────┼───────────┼───────────┼────────────────┘
        │           │           │           │
        └───────────┴─────┬─────┴───────────┘
                          │
┌─────────────────────────┼──────────────────────────────────┐
│                        RTE 层                               │
│              ┌──────────┴──────────┐                       │
│              │    Runtime Environment │                    │
│              │   (Rte/Rte_Scheduler)  │                    │
│              └──────────┬──────────┘                       │
└─────────────────────────┼──────────────────────────────────┘
                          │
┌─────────────────────────┼──────────────────────────────────┐
│                     Service 层                              │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │
│  │   Com   │ │  PduR   │ │   NvM   │ │   Dcm   │          │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘          │
│  ┌─────────┐                                                │
│  │   Dem   │                                                │
│  └─────────┘                                                │
└─────────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────┼──────────────────────────────────┐
│                      ECUAL 层                               │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │
│  │  CanIf  │ │ IoHwAb  │ │  CanTp  │ │  EthIf  │          │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │
│  │  MemIf  │ │   Fee   │ │   Ea    │ │  FrIf   │          │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘          │
│  ┌─────────┐                                                │
│  │  LinIf  │                                                │
│  └─────────┘                                                │
└─────────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────┼──────────────────────────────────┐
│                       MCAL 层                               │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │
│  │   Mcu   │ │   Port  │ │   Dio   │ │   Can   │          │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │
│  │   Spi   │ │   Gpt   │ │   Pwm   │ │   Adc   │          │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘          │
│  ┌─────────┐                                                │
│  │   Wdg   │                                                │
│  └─────────┘                                                │
└─────────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────┼──────────────────────────────────┐
│                      Hardware                               │
│              NXP i.MX8M Mini                                │
└─────────────────────────────────────────────────────────────┘
```

---

## 版本历史

| 版本 | 日期 | 变更描述 |
|:-----|:-----|:---------|
| 1.0.0 | 2026-04-15 | 初始版本，完整模块清单 |
| 1.1.0 | 2026-04-19 | 添加 ASW 层 8 个组件 |

---

**文档版本**: 1.1.0
**最后更新**: 2026-04-19
**作者**: YuleTech AutoSAR Team
