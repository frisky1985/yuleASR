# CanIf CAN接口API

<cite>
**本文档引用的文件**
- [CanIf.h](file://src/bsw/ecual/CanIf/include/CanIf.h)
- [CanIf.c](file://src/bsw/ecual/CanIf/src/CanIf.c)
- [CanIf_Cfg.h](file://src/bsw/ecual/CanIf/include/CanIf_Cfg.h)
- [Can.h](file://src/bsw/mcal/can/include/Can.h)
- [PduR.h](file://src/bsw/services/pdur/include/PduR.h)
- [Std_Types.h](file://src/bsw/os/include/Std_Types.h)
- [ComStack_Types.h](file://src/bsw/ecual/include/ComStack_Types.h)
- [Det.h](file://src/bsw/services/det/include/Det.h)
- [MemMap.h](file://src/bsw/general/inc/MemMap.h)
- [main.c](file://examples/can_demo/main.c)
</cite>

## 目录
1. [简介](#简介)
2. [项目结构](#项目结构)
3. [核心组件](#核心组件)
4. [架构概览](#架构概览)
5. [详细组件分析](#详细组件分析)
6. [依赖关系分析](#依赖关系分析)
7. [性能考虑](#性能考虑)
8. [故障排除指南](#故障排除指南)
9. [结论](#结论)
10. [附录](#附录)

## 简介

CanIf（Controller Area Network Interface）是AUTOSAR经典平台ECUAL层的CAN接口模块，遵循AUTOSAR 4.x标准。该模块作为应用层与MCAL层之间的抽象接口，提供了标准化的CAN通信服务。

CanIf模块的主要功能包括：
- CAN控制器初始化和配置
- 控制器模式管理和状态监控
- PDU（Protocol Data Unit）传输和路由
- 动态ID设置和管理
- 收发器（Transceiver）模式控制
- 错误检测和处理机制

## 项目结构

CanIf模块位于AUTOSAR分层架构中的ECUAL层，与上层应用软件组件（ASW）和下层MCAL驱动程序进行交互。

```mermaid
graph TB
subgraph "应用层 (Application Layer)"
ASW[应用软件组件<br/>Swc_*]
COM[通信模块<br/>Com]
end
subgraph "ECUAL层 (ECU Abstraction Layer)"
CANIF[CanIf模块<br/>CAN接口]
PDUR[PduR模块<br/>PDU路由器]
end
subgraph "MCAL层 (Microcontroller Abstraction Layer)"
CAN[Can驱动<br/>CAN Driver]
HW[硬件抽象<br/>MCAL Drivers]
end
subgraph "硬件层"
CANBUS[CAN总线<br/>物理层]
TRANS[收发器<br/>Transceiver]
end
ASW --> CANIF
COM --> CANIF
CANIF --> PDUR
PDUR --> CAN
CANIF --> CAN
CAN --> HW
HW --> CANBUS
HW --> TRANS
```

**图表来源**
- [CanIf.h:1-403](file://src/bsw/ecual/CanIf/include/CanIf.h#L1-L403)
- [Can.h:1-269](file://src/bsw/mcal/can/include/Can.h#L1-L269)
- [PduR.h:1-282](file://src/bsw/services/pdur/include/PduR.h#L1-L282)

**章节来源**
- [CanIf.h:1-403](file://src/bsw/ecual/CanIf/include/CanIf.h#L1-L403)
- [CanIf_Cfg.h:1-84](file://src/bsw/ecual/CanIf/include/CanIf_Cfg.h#L1-L84)

## 核心组件

### 主要API函数

CanIf模块提供以下核心API函数：

#### 初始化和配置
- `CanIf_Init()` - 初始化CAN接口
- `CanIf_DeInit()` - 反初始化CAN接口
- `CanIf_GetVersionInfo()` - 获取版本信息

#### 控制器管理
- `CanIf_SetControllerMode()` - 设置控制器模式
- `CanIf_GetControllerMode()` - 获取控制器模式
- `CanIf_CheckWakeup()` - 检查唤醒事件

#### PDU传输管理
- `CanIf_Transmit()` - 发送PDU数据
- `CanIf_CancelTransmit()` - 取消传输请求
- `CanIf_SetPduMode()` - 设置PDU模式
- `CanIf_GetPduMode()` - 获取PDU模式

#### 动态ID管理
- `CanIf_SetDynamicTxId()` - 设置动态TX ID

#### 收发器管理
- `CanIf_SetTrcvMode()` - 设置收发器模式
- `CanIf_GetTrcvMode()` - 获取收发器模式
- `CanIf_GetTrcvWakeupReason()` - 获取收发器唤醒原因
- `CanIf_SetTrcvWakeupMode()` - 设置收发器唤醒模式

**章节来源**
- [CanIf.h:267-403](file://src/bsw/ecual/CanIf/include/CanIf.h#L267-L403)
- [CanIf.c:29-487](file://src/bsw/ecual/CanIf/src/CanIf.c#L29-L487)

### 核心数据结构

#### 返回类型定义
```mermaid
classDiagram
class CanIf_ReturnType {
<<enumeration>>
+CANIF_OK
+CANIF_NOT_OK
+CANIF_BUSY
+CANIF_WAKEUP_VALID
+CANIF_WAKEUP_INVALID
+CANIF_WAKEUP_NOT_SUPPORTED
+CANIF_WAKEUP_CHECK_SAFETY_B_REQ
}
class Std_ReturnType {
<<typedef>>
+E_OK
+E_NOT_OK
+E_BUSY
}
CanIf_ReturnType --|> Std_ReturnType : "扩展"
```

**图表来源**
- [CanIf.h:95-103](file://src/bsw/ecual/CanIf/include/CanIf.h#L95-L103)
- [Std_Types.h:23-31](file://src/bsw/os/include/Std_Types.h#L23-L31)

#### 模式类型定义
```mermaid
classDiagram
class CanIf_ControllerModeType {
<<enumeration>>
+CANIF_CS_UNINIT
+CANIF_CS_SLEEP
+CANIF_CS_STARTED
+CANIF_CS_STOPPED
}
class CanIf_PduModeType {
<<enumeration>>
+CANIF_OFFLINE
+CANIF_TX_OFFLINE
+CANIF_TX_OFFLINE_ACTIVE
+CANIF_ONLINE
}
class CanIf_TransceiverModeType {
<<enumeration>>
+CANIF_TRCV_MODE_NORMAL
+CANIF_TRCV_MODE_STANDBY
+CANIF_TRCV_MODE_SLEEP
}
class CanIf_TrcvWakeupReasonType {
<<enumeration>>
+CANIF_TRCV_WU_ERROR
+CANIF_TRCV_WU_BY_BUS
+CANIF_TRCV_WU_BY_PIN
+CANIF_TRCV_WU_INTERNALLY
+CANIF_TRCV_WU_NOT_SUPPORTED
+CANIF_TRCV_WU_POWER_ON
+CANIF_TRCV_WU_RESET
+CANIF_TRCV_WU_BY_SYSERR
}
```

**图表来源**
- [CanIf.h:126-163](file://src/bsw/ecual/CanIf/include/CanIf.h#L126-L163)

#### 配置结构体
```mermaid
classDiagram
class CanIf_ConfigType {
+Controllers : CanIf_ControllerConfigType*
+NumControllers : uint8
+HrhConfigs : CanIf_HrhConfigType*
+NumHrhConfigs : uint8
+HthConfigs : CanIf_HthConfigType*
+NumHthConfigs : uint8
+TxPdus : CanIf_TxPduConfigType*
+NumTxPdus : uint8
+RxPdus : CanIf_RxPduConfigType*
+NumRxPdus : uint8
+DevErrorDetect : boolean
+VersionInfoApi : boolean
+DLCCheck : boolean
+SoftwareFilter : boolean
+ReadRxPduDataApi : boolean
+ReadTxPduNotifyStatusApi : boolean
+ReadRxPduNotifyStatusApi : boolean
}
class CanIf_ControllerConfigType {
+ControllerId : uint8
+BaudRate : uint32
+BaudRateConfig : uint32
+DefaultMode : CanIf_ControllerModeType
+WakeupSupport : boolean
+WakeupNotification : boolean
+BusOffNotification : boolean
+ErrorNotification : boolean
}
class CanIf_TxPduConfigType {
+PduId : PduIdType
+CanId : Can_IdType
+CanIdType : CanIf_CanIdTypeType
+Hth : Can_HwHandleType
+ControllerId : uint8
+Length : uint8
+TxConfirmation : boolean
+UserType : boolean
}
class CanIf_RxPduConfigType {
+PduId : PduIdType
+CanId : Can_IdType
+CanIdMask : Can_IdType
+CanIdType : CanIf_CanIdTypeType
+Hrh : Can_HwHandleType
+ControllerId : uint8
+Length : uint8
+RxIndication : boolean
}
CanIf_ConfigType --> CanIf_ControllerConfigType : "包含"
CanIf_ConfigType --> CanIf_TxPduConfigType : "包含"
CanIf_ConfigType --> CanIf_RxPduConfigType : "包含"
```

**图表来源**
- [CanIf.h:227-245](file://src/bsw/ecual/CanIf/include/CanIf.h#L227-L245)
- [CanIf.h:213-222](file://src/bsw/ecual/CanIf/include/CanIf.h#L213-L222)
- [CanIf.h:168-191](file://src/bsw/ecual/CanIf/include/CanIf.h#L168-L191)

**章节来源**
- [CanIf.h:95-245](file://src/bsw/ecual/CanIf/include/CanIf.h#L95-L245)

## 架构概览

CanIf模块采用分层架构设计，实现了应用层与MCAL层的有效解耦。

```mermaid
sequenceDiagram
participant APP as 应用软件组件
participant CANIF as CanIf模块
participant PDUR as PduR模块
participant CAN as Can驱动
participant HW as 硬件
APP->>CANIF : CanIf_Init(Config)
CANIF->>CAN : Can_Init()
CAN->>HW : 硬件初始化
APP->>CANIF : CanIf_SetControllerMode(STARTED)
CANIF->>CAN : Can_SetControllerMode(STARTED)
CAN->>HW : 启动控制器
APP->>CANIF : CanIf_Transmit(TxPduId, PduInfo)
CANIF->>CANIF : 验证PDU配置
CANIF->>CAN : Can_Write(Hth, CanPdu)
CAN->>HW : 写入发送缓冲区
HW-->>CAN : 发送完成中断
CAN->>CANIF : CanIf_TxConfirmation()
CANIF->>PDUR : PduR_TxConfirmation()
PDUR->>APP : 传输确认回调
```

**图表来源**
- [CanIf.c:29-185](file://src/bsw/ecual/CanIf/src/CanIf.c#L29-L185)
- [CanIf.c:259-271](file://src/bsw/ecual/CanIf/src/CanIf.c#L259-L271)

**章节来源**
- [CanIf.c:29-185](file://src/bsw/ecual/CanIf/src/CanIf.c#L29-L185)

## 详细组件分析

### 初始化流程

CanIf初始化过程确保了模块的正确配置和状态管理。

```mermaid
flowchart TD
Start([初始化开始]) --> CheckConfig["检查配置指针"]
CheckConfig --> ConfigValid{"配置有效?"}
ConfigValid --> |否| ReportError["报告DET错误"]
ConfigValid --> |是| CheckInit["检查是否已初始化"]
CheckInit --> AlreadyInit{"已初始化?"}
AlreadyInit --> |是| ReportAlready["报告重复初始化错误"]
AlreadyInit --> |否| SetConfig["设置配置指针"]
SetConfig --> InitModes["初始化控制器和PDU模式"]
InitModes --> MarkReady["标记模块就绪"]
MarkReady --> End([初始化完成])
ReportError --> End
ReportAlready --> End
```

**图表来源**
- [CanIf.c:29-50](file://src/bsw/ecual/CanIf/src/CanIf.c#L29-L50)

#### 初始化参数验证
- 配置指针不能为空
- 防止重复初始化
- 检查开发错误检测配置

**章节来源**
- [CanIf.c:29-50](file://src/bsw/ecual/CanIf/src/CanIf.c#L29-L50)

### 控制器模式管理

控制器模式管理实现了CAN控制器状态的统一控制。

```mermaid
stateDiagram-v2
[*] --> UNINIT
UNINIT --> SLEEP : SetControllerMode(SLEEP)
UNINIT --> STARTED : SetControllerMode(STARTED)
UNINIT --> STOPPED : SetControllerMode(STOPPED)
SLEEP --> STARTED : SetControllerMode(STARTED)
STARTED --> STOPPED : SetControllerMode(STOPPED)
STOPPED --> SLEEP : SetControllerMode(SLEEP)
STARTED --> SLEEP : SetControllerMode(SLEEP)
note right of UNINIT : 未初始化状态
note right of SLEEP : 睡眠模式
note right of STARTED : 已启动模式
note right of STOPPED : 停止模式
```

**图表来源**
- [CanIf.c:69-119](file://src/bsw/ecual/CanIf/src/CanIf.c#L69-L119)

#### 模式转换逻辑
- STARTED模式：激活CAN控制器
- STOPPED模式：停止CAN控制器
- SLEEP模式：进入低功耗状态

**章节来源**
- [CanIf.c:69-119](file://src/bsw/ecual/CanIf/src/CanIf.c#L69-L119)

### PDU传输流程

PDU传输流程展示了从应用层到硬件层的数据传输路径。

```mermaid
sequenceDiagram
participant APP as 应用软件组件
participant CANIF as CanIf模块
participant PDUR as PduR模块
participant CAN as Can驱动
participant HW as 硬件
APP->>CANIF : CanIf_Transmit(TxPduId, PduInfo)
CANIF->>CANIF : 验证PDU ID范围
CANIF->>CANIF : 检查控制器状态
CANIF->>CANIF : 检查PDU模式
alt 控制器已启动且PDU在线
CANIF->>CAN : Can_Write(Hth, CanPdu)
alt 发送成功
CANIF-->>APP : E_OK
else 硬件忙
CANIF-->>APP : E_NOT_OK
else 发送失败
CANIF-->>APP : E_NOT_OK
end
else 控制器未启动或PDU离线
CANIF-->>APP : E_NOT_OK
end
Note over CAN,HW : 硬件发送完成
HW->>CAN : 发送完成中断
CAN->>CANIF : CanIf_TxConfirmation()
CANIF->>PDUR : PduR_TxConfirmation()
PDUR->>APP : 传输确认回调
```

**图表来源**
- [CanIf.c:142-185](file://src/bsw/ecual/CanIf/src/CanIf.c#L142-L185)
- [CanIf.c:259-271](file://src/bsw/ecual/CanIf/src/CanIf.c#L259-L271)

#### 传输验证规则
- PDU ID必须在有效范围内
- 控制器必须处于STARTED状态
- PDU模式不能为OFFLINE
- DLC长度检查（可选）

**章节来源**
- [CanIf.c:142-185](file://src/bsw/ecual/CanIf/src/CanIf.c#L142-L185)

### 收发器管理

收发器管理提供了对CAN总线收发器的控制能力。

```mermaid
classDiagram
class CanIf_TransceiverModeType {
<<enumeration>>
+NORMAL
+STANDBY
+SLEEP
}
class CanIf_TrcvWakeupReasonType {
<<enumeration>>
+ERROR
+BUS_WAKEUP
+PIN_WAKEUP
+INTERNAL_WAKEUP
+NOT_SUPPORTED
+POWER_ON
+RESET
+SYSTEM_ERROR
}
class CanIf_TrcvWakeupModeType {
<<enumeration>>
+ENABLE
+DISABLE
+CLEAR
}
class CanIf_TransceiverConfig {
+TransceiverId : uint8
+DefaultMode : CanIf_TransceiverModeType
+WakeupSupport : boolean
+WakeupNotification : boolean
}
```

**图表来源**
- [CanIf.h:136-163](file://src/bsw/ecual/CanIf/include/CanIf.h#L136-L163)

#### 收发器操作流程
- 模式设置：NORMAL/STANDBY/SLEEP
- 唤醒检测：总线唤醒/PIN唤醒/内部唤醒
- 唤醒模式：启用/禁用/清除

**章节来源**
- [CanIf.c:368-444](file://src/bsw/ecual/CanIf/src/CanIf.c#L368-L444)

## 依赖关系分析

### 外部依赖

CanIf模块依赖于多个AUTOSAR标准模块：

```mermaid
graph TB
subgraph "CanIf模块"
CANIF[CanIf模块]
end
subgraph "标准依赖"
STD[Std_Types.h]
CST[ComStack_Types.h]
DET[Det.h]
MEM[MemMap.h]
end
subgraph "MCAL驱动"
CAN[Can.h]
end
subgraph "服务层"
PDUR[PduR.h]
end
CANIF --> STD
CANIF --> CST
CANIF --> DET
CANIF --> MEM
CANIF --> CAN
CANIF --> PDUR
```

**图表来源**
- [CanIf.h:18-22](file://src/bsw/ecual/CanIf/include/CanIf.h#L18-L22)
- [CanIf.c:9-14](file://src/bsw/ecual/CanIf/src/CanIf.c#L9-L14)

### 内部依赖关系

```mermaid
graph LR
subgraph "配置层"
CFG[CanIf_Cfg.h]
STD[Std_Types.h]
CST[ComStack_Types.h]
end
subgraph "实现层"
IFH[CanIf.h]
IFC[CanIf.c]
end
subgraph "MCAL层"
CANH[Can.h]
end
subgraph "服务层"
PDURH[PduR.h]
end
CFG --> IFH
STD --> IFH
CST --> IFH
IFH --> IFC
CANH --> IFC
PDURH --> IFC
```

**图表来源**
- [CanIf_Cfg.h:1-84](file://src/bsw/ecual/CanIf/include/CanIf_Cfg.h#L1-L84)
- [CanIf.h:18-245](file://src/bsw/ecual/CanIf/include/CanIf.h#L18-L245)

**章节来源**
- [CanIf_Cfg.h:1-84](file://src/bsw/ecual/CanIf/include/CanIf_Cfg.h#L1-L84)
- [CanIf.h:18-245](file://src/bsw/ecual/CanIf/include/CanIf.h#L18-L245)

## 性能考虑

### 内存管理

CanIf模块采用了AUTOSAR标准的内存映射机制：

- **静态内存分配**：控制器和PDU模式状态存储
- **配置常量存储**：配置信息存储在只读段
- **内存分区**：使用MemMap.h进行精确的内存段管理

### 中断处理

```mermaid
flowchart TD
HW_INT[硬件中断] --> CAN_ISR[Can驱动ISR]
CAN_ISR --> CANIF_CB[CanIf回调]
CANIF_CB --> PDUR_CB[PduR回调]
PDUR_CB --> APP_CB[应用回调]
CAN_ISR --> CANIF_STAT[状态更新]
CANIF_STAT --> CANIF_MODE[模式指示]
```

**图表来源**
- [CanIf.c:273-325](file://src/bsw/ecual/CanIf/src/CanIf.c#L273-L325)

### 性能优化建议

1. **批处理传输**：合并多个小PDU以减少中断开销
2. **优先级调度**：为关键消息设置更高的传输优先级
3. **缓冲区管理**：合理配置发送和接收缓冲区大小
4. **错误恢复**：实现快速错误检测和恢复机制

## 故障排除指南

### 常见错误代码

| 错误代码 | 描述 | 可能原因 | 解决方案 |
|---------|------|----------|----------|
| CANIF_E_UNINIT | 未初始化 | 模块未正确初始化 | 调用CanIf_Init() |
| CANIF_E_PARAM_POINTER | 参数指针无效 | 传入NULL指针 | 检查传入参数 |
| CANIF_E_INVALID_TXPDUID | 无效的TX PDU ID | PDU ID超出范围 | 验证PDU ID配置 |
| CANIF_E_PARAM_CONTROLLER | 无效的控制器ID | 控制器ID越界 | 检查控制器配置 |
| CANIF_E_NOT_OK | 一般性失败 | 硬件或驱动错误 | 检查硬件连接 |

### 错误检测机制

```mermaid
flowchart TD
API_CALL[API调用] --> DET_CHECK[DET错误检测]
DET_CHECK --> VALIDATE[参数验证]
VALIDATE --> VALID{"验证通过?"}
VALID --> |否| REPORT_ERROR[报告DET错误]
VALID --> |是| EXECUTE[执行操作]
EXECUTE --> RESULT[返回结果]
REPORT_ERROR --> RESULT
```

**图表来源**
- [CanIf.c:31-40](file://src/bsw/ecual/CanIf/src/CanIf.c#L31-L40)

### 调试技巧

1. **启用开发错误检测**：在配置中启用CANIF_DEV_ERROR_DETECT
2. **日志记录**：记录关键状态变化和错误信息
3. **单元测试**：编写针对各种边界条件的测试用例
4. **硬件验证**：使用示波器验证CAN总线信号质量

**章节来源**
- [CanIf.c:31-40](file://src/bsw/ecual/CanIf/src/CanIf.c#L31-L40)

## 结论

CanIf模块为AUTOSAR系统提供了标准化的CAN通信接口，具有以下特点：

1. **标准化接口**：完全符合AUTOSAR 4.x标准
2. **模块化设计**：清晰的层次结构和职责分离
3. **错误处理**：完善的错误检测和报告机制
4. **可配置性**：支持多种配置选项和运行时调整

该模块为上层应用提供了简单易用的CAN通信接口，同时保持了与底层硬件的高度解耦，便于系统的维护和扩展。

## 附录

### API参考表

#### 初始化相关
| 函数名 | 参数 | 返回值 | 描述 |
|--------|------|--------|------|
| CanIf_Init | ConfigPtr: 配置指针 | void | 初始化CAN接口 |
| CanIf_DeInit | 无 | void | 反初始化CAN接口 |
| CanIf_GetVersionInfo | versioninfo: 版本信息指针 | void | 获取版本信息 |

#### 控制器管理
| 函数名 | 参数 | 返回值 | 描述 |
|--------|------|--------|------|
| CanIf_SetControllerMode | ControllerId: 控制器ID<br/>ControllerMode: 模式 | Std_ReturnType | 设置控制器模式 |
| CanIf_GetControllerMode | ControllerId: 控制器ID<br/>ControllerModePtr: 模式指针 | Std_ReturnType | 获取控制器模式 |
| CanIf_CheckWakeup | WakeupSource: 唤醒源 | Std_ReturnType | 检查唤醒事件 |

#### PDU传输
| 函数名 | 参数 | 返回值 | 描述 |
|--------|------|--------|------|
| CanIf_Transmit | TxPduId: TX PDU ID<br/>PduInfoPtr: PDU信息指针 | Std_ReturnType | 发送PDU数据 |
| CanIf_CancelTransmit | TxPduId: TX PDU ID | Std_ReturnType | 取消传输请求 |
| CanIf_SetPduMode | ControllerId: 控制器ID<br/>PduModeRequest: PDU模式 | Std_ReturnType | 设置PDU模式 |
| CanIf_GetPduMode | ControllerId: 控制器ID<br/>PduModePtr: 模式指针 | Std_ReturnType | 获取PDU模式 |

#### 动态ID管理
| 函数名 | 参数 | 返回值 | 描述 |
|--------|------|--------|------|
| CanIf_SetDynamicTxId | CanTxPduId: TX PDU ID<br/>CanId: CAN ID | Std_ReturnType | 设置动态TX ID |

#### 收发器管理
| 函数名 | 参数 | 返回值 | 描述 |
|--------|------|--------|------|
| CanIf_SetTrcvMode | TransceiverId: 收发器ID<br/>TransceiverMode: 模式 | Std_ReturnType | 设置收发器模式 |
| CanIf_GetTrcvMode | TransceiverId: 收发器ID<br/>TransceiverModePtr: 模式指针 | Std_ReturnType | 获取收发器模式 |
| CanIf_GetTrcvWakeupReason | TransceiverId: 收发器ID<br/>TrcvWuReasonPtr: 原因指针 | Std_ReturnType | 获取收发器唤醒原因 |
| CanIf_SetTrcvWakeupMode | TransceiverId: 收发器ID<br/>TrcvWakeupMode: 唤醒模式 | Std_ReturnType | 设置收发器唤醒模式 |

### 配置示例

#### 基本配置结构
```c
const CanIf_ConfigType CanIf_Config = {
    .Controllers = g_CanIfControllers,
    .NumControllers = CANIF_NUM_CONTROLLERS,
    .TxPdus = g_CanIfTxPdus,
    .NumTxPdus = CANIF_NUM_TX_PDUS,
    .RxPdus = g_CanIfRxPdus,
    .NumRxPdus = CANIF_NUM_RX_PDUS,
    .DevErrorDetect = CANIF_DEV_ERROR_DETECT,
    .VersionInfoApi = CANIF_VERSION_INFO_API,
    .DLCCheck = CANIF_DLC_CHECK,
    .SoftwareFilter = CANIF_SOFTWARE_FILTER_TYPE
};
```

#### PDU配置示例
```c
const CanIf_TxPduConfigType g_CanIfTxPdus[] = {
    {
        .PduId = CANIF_TXPDU_ENGINE_STATUS,
        .CanId = CANIF_CANID_ENGINE_STATUS,
        .CanIdType = CANIF_CANID_TYPE_STANDARD,
        .Hth = 0,
        .ControllerId = CANIF_CONTROLLER_0,
        .Length = 8,
        .TxConfirmation = TRUE,
        .UserType = FALSE
    }
};
```

**章节来源**
- [CanIf_Cfg.h:1-84](file://src/bsw/ecual/CanIf/include/CanIf_Cfg.h#L1-L84)
- [CanIf.h:227-245](file://src/bsw/ecual/CanIf/include/CanIf.h#L227-L245)