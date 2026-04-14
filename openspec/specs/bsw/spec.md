# OpenSpec: Yule BSW 基础软件规范

> **版本**: v1.0  
> **状态**: 活跃  
> **最后更新**: 2026-04-13

## 概述

Yule BSW (Basic Software) 是予乐科技基于 AutoSAR 标准开发的汽车基础软件平台，提供从微控制器驱动到服务层的完整基础软件栈。

## 架构规范

### 分层架构

```
┌─────────────────────────────────────────────┐
│  Application Layer (ASW)                    │
│  - 应用层软件 (客户实现)                      │
├─────────────────────────────────────────────┤
│  Runtime Environment (RTE)                  │
│  - 组件间接口通信                            │
├─────────────────────────────────────────────┤
│  Basic Software (BSW)                       │
│  ┌─────────────────────────────────────┐   │
│  │  Service Layer                      │   │
│  │  - Communication Services           │   │
│  │  - Memory Services                  │   │
│  │  - Diagnostic Services              │   │
│  ├─────────────────────────────────────┤   │
│  │  ECU Abstraction Layer              │   │
│  │  - I/O Hardware Abstraction         │   │
│  │  - Communication Hardware Abstraction│  │
│  │  - Memory Hardware Abstraction      │   │
│  │  - Onboard Device Abstraction       │   │
│  ├─────────────────────────────────────┤   │
│  │  Microcontroller Driver Layer (MCAL)│   │
│  │  - Microcontroller Unit (MCU)       │   │
│  │  - General Purpose I/O (GPIO)       │   │
│  │  - Analog to Digital Converter (ADC)│   │
│  │  - Pulse Width Modulation (PWM)     │   │
│  │  - Controller Area Network (CAN)    │   │
│  │  - Serial Communication (SPI/UART)  │   │
│  └─────────────────────────────────────┘   │
├─────────────────────────────────────────────┤
│  Microcontroller Hardware                   │
└─────────────────────────────────────────────┘
```

## 模块规范

### 1. MCAL (Microcontroller Driver Layer)

#### 1.1 MCU Driver

**功能要求**:
- 微控制器时钟初始化与配置
- 复位原因检测
- 低功耗模式管理
- 芯片版本识别

**接口定义**:
```c
Std_ReturnType Mcu_Init(const Mcu_ConfigType* ConfigPtr);
Std_ReturnType Mcu_InitClock(Mcu_ClockType ClockSetting);
void Mcu_SetMode(Mcu_ModeType McuMode);
Mcu_RawResetType Mcu_GetResetReason(void);
```

**Scenarios**:
- ✅ 正常初始化流程
- ✅ 时钟切换
- ✅ 复位原因读取
- ✅ 低功耗模式进入/退出

#### 1.2 GPIO Driver

**功能要求**:
- 引脚方向配置 (输入/输出)
- 引脚电平设置/读取
- 中断触发配置
- 复用功能配置

**接口定义**:
```c
void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level);
Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId);
void Dio_WritePort(Dio_PortType PortId, Dio_PortLevelType Level);
Dio_PortLevelType Dio_ReadPort(Dio_PortType PortId);
```

**Scenarios**:
- ✅ 数字输出控制
- ✅ 数字输入读取
- ✅ 端口级操作
- ✅ 中断触发

#### 1.3 CAN Driver

**功能要求**:
- CAN 控制器初始化
- 报文发送/接收
- 中断处理
- 错误处理
- CAN-FD 支持 (可选)

**接口定义**:
```c
void Can_Init(const Can_ConfigType* Config);
Std_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo);
void Can_MainFunction_Write(void);
void Can_MainFunction_Read(void);
```

**Scenarios**:
- ✅ 标准帧发送
- ✅ 标准帧接收
- ✅ 扩展帧支持
- ✅ 错误处理
- ✅ CAN-FD 报文 (如支持)

### 2. ECU Abstraction Layer

#### 2.1 I/O Hardware Abstraction

**功能要求**:
- 抽象底层 GPIO 操作
- 提供统一的 I/O 接口
- 支持数字/模拟 I/O

#### 2.2 Communication Hardware Abstraction

**功能要求**:
- CAN 通信抽象
- LIN 通信抽象 (可选)
- Ethernet 通信抽象 (可选)

### 3. Service Layer

#### 3.1 Communication Services (Com)

**功能要求**:
- 信号路由
- 报文发送/接收管理
- 信号网关功能
- 大数据类型支持

**接口定义**:
```c
void Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);
uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr);
void Com_MainFunctionTx(void);
void Com_MainFunctionRx(void);
```

#### 3.2 Diagnostic Services (Dcm)

**功能要求**:
- UDS 协议实现 (ISO 14229)
- 诊断会话管理
- 安全访问
- DTC 管理

**支持的 UDS 服务**:
- 0x10 - Diagnostic Session Control
- 0x11 - ECU Reset
- 0x22 - Read Data By Identifier
- 0x2E - Write Data By Identifier
- 0x31 - Routine Control
- 0x34 - Request Download
- 0x36 - Transfer Data
- 0x37 - Request Transfer Exit

**接口定义**:
```c
void Dcm_Init(const Dcm_ConfigType* ConfigPtr);
void Dcm_MainFunction(void);
Std_ReturnType Dcm_GetSecurityLevel(Dcm_SecLevelType* SecLevel);
```

#### 3.3 Memory Services (NvM)

**功能要求**:
- 非易失性数据管理
- 块读写操作
- 冗余管理
- Wear Leveling 支持

## 配置规范

### ARXML 配置

所有 BSW 模块必须通过 ARXML 文件配置，支持以下配置参数：

```xml
<AUTOSAR>
  <AR-PACKAGE>
    <SHORT-NAME>YuleBswConfig</SHORT-NAME>
    <ELEMENTS>
      <!-- ECU Configuration -->
      <ECUC-MODULE-CONFIGURATION>
        <SHORT-NAME>Mcu</SHORT-NAME>
        <DEFINITION-REF DEST="ECUC-MODULE-DEF">/Mcu</DEFINITION-REF>
        <CONTAINERS>
          <!-- MCU Configuration -->
        </CONTAINERS>
      </ECUC-MODULE-CONFIGURATION>
    </ELEMENTS>
  </AR-PACKAGE>
</AUTOSAR>
```

### 代码生成

配置工具必须能够：
1. 解析 ARXML 配置文件
2. 生成 C 代码和头文件
3. 生成配置验证报告

## 测试规范

### 单元测试

每个模块必须提供单元测试，覆盖率要求：
- 语句覆盖率 > 90%
- 分支覆盖率 > 85%
- MC/DC 覆盖率 > 100% (安全相关)

### 集成测试

模块间集成测试必须覆盖：
- 正常操作流程
- 错误处理流程
- 边界条件
- 并发场景

## 质量属性

### 性能

| 指标 | 目标值 |
|:-----|:-------|
| 中断响应延迟 | < 10μs |
| 任务调度延迟 | < 50μs |
| CAN 报文处理 | < 100μs |
| Flash 写操作 | 按芯片规格 |

### 可靠性

- 代码必须通过 MISRA C:2012 检查
- 静态分析警告为 0
- 运行时错误必须通过防御式编程处理

### 可移植性

- 支持主流芯片平台 (NXP、Infineon、Renesas)
- 移植工作量 < 2 人周
- 硬件抽象层清晰分离

## 版本历史

| 版本 | 日期 | 变更内容 |
|:-----|:-----|:---------|
| v1.0 | 2026-04-13 | 初始版本，定义基础架构和核心模块 |

## 参考标准

- AutoSAR Classic Platform 4.4
- MISRA C:2012
- ISO 14229 (UDS)
- ISO 26262 (功能安全)
