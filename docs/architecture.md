# YuleTech AutoSAR BSW 架构文档

## 1. 概述

### 1.1 文档目的

本文档描述 YuleTech AutoSAR BSW Platform 的系统架构设计，包括分层架构、模块交互、接口定义和设计原则。

### 1.2 适用范围

本文档适用于以下读者：
- 软件开发工程师
- 系统架构师
- 测试工程师
- 项目经理

### 1.3 参考文档

- AutoSAR Classic Platform 4.4 规范
- AutoSAR Layered Software Architecture
- NXP i.MX8M Mini Reference Manual

## 2. 系统架构

### 2.1 分层架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application Layer                         │
│                    (SWC - Software Components)                   │
├─────────────────────────────────────────────────────────────────┤
│                         RTE (Runtime Environment)                │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  - Component Communication Interface                       │  │
│  │  - Data Type Definitions                                   │  │
│  │  - Scheduler                                               │  │
│  │  - COM/NVM Interface                                       │  │
│  └───────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                        Service Layer                             │
│  ┌─────────────┬─────────────┬─────────────┬─────────────┐      │
│  │     Com     │    PduR     │     NvM     │  Dcm/Dem    │      │
│  │  通信服务    │  PDU路由器   │ NVRAM管理器 │  诊断服务    │      │
│  └─────────────┴─────────────┴─────────────┴─────────────┘      │
├─────────────────────────────────────────────────────────────────┤
│                        ECUAL Layer                               │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐       │
│  │  CanIf   │  CanTp   │  EthIf   │  FrIf    │  LinIf   │       │
│  │ CAN接口  │ CAN传输协议│ 以太网接口│ FlexRay  │ LIN接口  │       │
│  ├──────────┼──────────┼──────────┼──────────┼──────────┤       │
│  │  IoHwAb  │  MemIf   │   Fee    │   Ea     │          │       │
│  │ I/O硬件抽象│ 存储器接口│ Flash仿真 │ EEPROM抽象│          │       │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘       │
├─────────────────────────────────────────────────────────────────┤
│                        MCAL Layer                                │
│  ┌────────┬────────┬────────┬────────┬────────┬────────┐        │
│  │  Mcu   │  Port  │  Dio   │  Can   │  Spi   │  Gpt   │        │
│  │ 微控制器│  端口  │ 数字I/O│  CAN   │  SPI   │ 定时器 │        │
│  ├────────┼────────┼────────┼────────┼────────┼────────┤        │
│  │  Pwm   │  Adc   │  Wdg   │        │        │        │        │
│  │  PWM   │  ADC   │ 看门狗 │        │        │        │        │
│  └────────┴────────┴────────┴────────┴────────┴────────┘        │
├─────────────────────────────────────────────────────────────────┤
│                     Hardware (i.MX8M Mini)                       │
│              ARM Cortex-A53, CAN, Ethernet, LIN                  │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 依赖规则

```
RTE
 ↑ 可以调用 ↓
Service Layer
 ↑ 可以调用 ↓
ECUAL Layer
 ↑ 可以调用 ↓
MCAL Layer
 ↑ 可以调用 ↓
Hardware
```

**约束：**
- 上层可以调用下层
- 下层不能调用上层
- 同层之间不能直接调用（通过 Service 层路由）

## 3. 模块架构

### 3.1 MCAL 层架构

#### 3.1.1 微控制器驱动 (Mcu)

```
┌─────────────────────────────────────┐
│           Mcu Driver                │
├─────────────────────────────────────┤
│  - Mcu_Init()                       │
│  - Mcu_DistributePllClock()         │
│  - Mcu_GetPllStatus()               │
│  - Mcu_SetMode()                    │
├─────────────────────────────────────┤
│  Clock Configuration                │
│  Reset Control                      │
│  Power Management                   │
└─────────────────────────────────────┘
```

#### 3.1.2 CAN 驱动 (Can)

```
┌─────────────────────────────────────┐
│           Can Driver                │
├─────────────────────────────────────┤
│  - Can_Init()                       │
│  - Can_Write()                      │
│  - Can_MainFunction_Write()         │
│  - Can_MainFunction_Read()          │
├─────────────────────────────────────┤
│  Controller Management              │
│  Message Buffer Management          │
│  Interrupt Handling                 │
└─────────────────────────────────────┘
```

### 3.2 ECUAL 层架构

#### 3.2.1 CAN 接口 (CanIf)

```
┌─────────────────────────────────────┐
│           CanIf Module              │
├─────────────────────────────────────┤
│  - CanIf_Init()                     │
│  - CanIf_Transmit()                 │
│  - CanIf_RxIndication()             │
│  - CanIf_TxConfirmation()           │
├─────────────────────────────────────┤
│  PDU Routing                        │
│  Controller Mode Control            │
│  BusOff Handling                    │
└─────────────────────────────────────┘
```

#### 3.2.2 I/O 硬件抽象 (IoHwAb)

```
┌─────────────────────────────────────┐
│         IoHwAb Module               │
├─────────────────────────────────────┤
│  - IoHwAb_Init()                    │
│  - IoHwAb_AnalogRead()              │
│  - IoHwAb_DigitalRead()             │
│  - IoHwAb_DigitalWrite()            │
├─────────────────────────────────────┤
│  Analog Signal Processing           │
│  Digital Signal Processing          │
│  Signal Conditioning                │
└─────────────────────────────────────┘
```

### 3.3 Service 层架构

#### 3.3.1 PDU 路由器 (PduR)

```
┌─────────────────────────────────────┐
│           PduR Module               │
├─────────────────────────────────────┤
│  - PduR_Init()                      │
│  - PduR_Transmit()                  │
│  - PduR_RxIndication()              │
│  - PduR_TxConfirmation()            │
├─────────────────────────────────────┤
│  Routing Table                      │
│  ┌─────────┬─────────┬─────────┐   │
│  │  COM    │  DCM    │  CanIf  │   │
│  │   ↑     │   ↑     │   ↓     │   │
│  └────┬────┴────┬────┴────┬────┘   │
│       │         │         │        │
│  ┌────┴─────────┴─────────┴────┐   │
│  │      Routing Engine          │   │
│  └─────────────────────────────┘   │
└─────────────────────────────────────┘
```

#### 3.3.2 NVRAM 管理器 (NvM)

```
┌─────────────────────────────────────┐
│           NvM Module                │
├─────────────────────────────────────┤
│  - NvM_Init()                       │
│  - NvM_ReadBlock()                  │
│  - NvM_WriteBlock()                 │
│  - NvM_MainFunction()               │
├─────────────────────────────────────┤
│  Job Queue Management               │
│  ┌─────────────────────────────┐   │
│  │  Standard Queue (16 slots)  │   │
│  │  Immediate Queue (4 slots)  │   │
│  └─────────────────────────────┘   │
│  Block Management                   │
│  CRC Calculation (CRC8/16/32)       │
└─────────────────────────────────────┘
```

#### 3.3.3 诊断通信管理器 (Dcm)

```
┌─────────────────────────────────────┐
│           Dcm Module                │
├─────────────────────────────────────┤
│  - Dcm_Init()                       │
│  - Dcm_MainFunction()               │
│  - Dcm_RxIndication()               │
│  - Dcm_TxConfirmation()             │
├─────────────────────────────────────┤
│  UDS Services                       │
│  ┌─────────────────────────────┐   │
│  │  0x10 - Session Control     │   │
│  │  0x11 - ECU Reset           │   │
│  │  0x22 - Read Data By ID     │   │
│  │  0x2E - Write Data By ID    │   │
│  │  0x31 - Routine Control     │   │
│  │  0x27 - Security Access     │   │
│  └─────────────────────────────┘   │
│  Session Management                 │
│  Security Access Control            │
└─────────────────────────────────────┘
```

### 3.4 RTE 层架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        RTE Layer                                 │
├─────────────────────────────────────────────────────────────────┤
│  RTE Core                                                        │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  - Rte_Init() / Rte_DeInit()                               │  │
│  │  - Rte_Start() / Rte_Stop()                                │  │
│  │  - Rte_Read() / Rte_Write()                                │  │
│  │  - Rte_IrvRead() / Rte_IrvWrite()                          │  │
│  │  - Rte_Switch() / Rte_Mode()                               │  │
│  └───────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│  RTE Scheduler                                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  - Task Management                                         │  │
│  │  - Priority Scheduling                                     │  │
│  │  - Event Handling                                          │  │
│  │  - Periodic Runnable Execution                             │  │
│  └───────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│  RTE Interfaces                                                  │
│  ┌───────────────┬───────────────┬───────────────┐              │
│  │  COM Interface│  NVM Interface│  DCM Interface│              │
│  │  - Signal Map │  - Block Map  │  - Diag Map   │              │
│  │  - Send/Recv  │  - Read/Write │  - Request    │              │
│  └───────────────┴───────────────┴───────────────┘              │
└─────────────────────────────────────────────────────────────────┘
```

## 4. 数据流

### 4.1 发送数据流

```
Application (SWC)
       ↓
   Rte_Write()
       ↓
      RTE
       ↓
   Com_SendSignal()
       ↓
      COM
       ↓
   PduR_Transmit()
       ↓
      PduR
       ↓
   CanIf_Transmit()
       ↓
     CanIf
       ↓
    Can_Write()
       ↓
      CAN
       ↓
   Hardware
```

### 4.2 接收数据流

```
   Hardware
       ↓
      CAN
       ↓
   Can_RxIndication()
       ↓
     CanIf
       ↓
   CanIf_RxIndication()
       ↓
      PduR
       ↓
   PduR_RxIndication()
       ↓
      COM
       ↓
   Com_RxIndication()
       ↓
      RTE
       ↓
   Rte_ComCallbackRx()
       ↓
   Rte_Read()
       ↓
Application (SWC)
```

### 4.3 诊断数据流

```
Diagnostic Tool (UDS)
       ↓
      CAN
       ↓
     CanIf
       ↓
      PduR
       ↓
      DCM
       ↓
   Process Request
       ↓
   ┌─────────────┐
   │ Read DIDs   │
   │ Write DIDs  │
   │ Routine Ctrl│
   │ Session Ctrl│
   └─────────────┘
       ↓
   Send Response
       ↓
      DCM
       ↓
      PduR
       ↓
     CanIf
       ↓
      CAN
       ↓
Diagnostic Tool
```

## 5. 接口定义

### 5.1 标准接口

所有模块遵循 AutoSAR 标准接口定义：

```c
/* 初始化接口 */
void Module_Init(const Module_ConfigType* ConfigPtr);

/* 反初始化接口 */
void Module_DeInit(void);

/* 主函数接口 */
void Module_MainFunction(void);

/* 版本信息接口 */
void Module_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

### 5.2 回调接口

```c
/* 发送确认回调 */
void Module_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/* 接收指示回调 */
void Module_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/* 触发发送回调 */
Std_ReturnType Module_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);
```

## 6. 设计原则

### 6.1 模块化设计

- 每个模块职责单一
- 模块间通过标准接口通信
- 支持独立测试和验证

### 6.2 可配置性

- 所有模块支持配置工具生成
- 配置参数通过 Module_Cfg.h 定义
- 支持编译时和运行时配置

### 6.3 错误处理

- 统一的错误检测机制 (DET)
- 开发时错误检测 (STD_ON)
- 运行时错误处理

### 6.4 性能优化

- 静态内存分配
- 零拷贝数据传输
- 优化的查找算法

## 7. 安全设计

### 7.1 内存保护

- 使用 MemMap.h 进行内存分区
- 代码段和数据段分离
- 配置数据只读保护

### 7.2 错误检测

- 参数范围检查
- 空指针检查
- 状态机完整性检查

### 7.3 故障安全

- 看门狗监控
- 安全模式切换
- 故障恢复机制

## 8. 扩展性设计

### 8.1 新模块添加

1. 创建模块目录结构
2. 实现标准接口
3. 添加配置参数
4. 编写验证测试
5. 更新文档

### 8.2 新硬件支持

1. 实现 MCAL 驱动
2. 配置 ECUAL 模块
3. 验证硬件抽象层
4. 更新构建配置

## 9. 工具链集成

### 9.1 代码生成

```
ARXML Configuration
       ↓
Code Generator (Python/Jinja2)
       ↓
Generated Code (Rte_*.c/h)
       ↓
Integration with BSW
```

### 9.2 构建系统

```
CMake Configuration
       ↓
Toolchain Selection
       ↓
Module Compilation
       ↓
Linking
       ↓
Binary Generation
```

## 10. 版本历史

| 版本 | 日期 | 描述 |
|:-----|:-----|:-----|
| 1.0.0 | 2026-04-15 | 初始版本，完整 BSW 实现 |

---

**文档版本**: 1.0.0
**最后更新**: 2026-04-15
**作者**: YuleTech AutoSAR Team
