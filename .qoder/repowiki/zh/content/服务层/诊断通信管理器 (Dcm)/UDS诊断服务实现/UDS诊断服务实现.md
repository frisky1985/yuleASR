# UDS诊断服务实现

<cite>
**本文档引用的文件**
- [Swc_DiagnosticManager.h](file://src/asw/diagnostic_manager/include/Swc_DiagnosticManager.h)
- [Swc_DiagnosticManager.c](file://src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c)
- [Dcm.h](file://src/bsw/services/dcm/include/Dcm.h)
- [Dcm.c](file://src/bsw/services/dcm/src/Dcm.c)
- [Dcm_Cfg.h](file://src/bsw/services/dcm/include/Dcm_Cfg.h)
- [bsw_config.json](file://config/bsw_config.json)
</cite>

## 目录
1. [项目概述](#项目概述)
2. [项目结构](#项目结构)
3. [核心组件](#核心组件)
4. [架构概览](#架构概览)
5. [详细组件分析](#详细组件分析)
6. [依赖关系分析](#依赖关系分析)
7. [性能考虑](#性能考虑)
8. [故障排除指南](#故障排除指南)
9. [结论](#结论)

## 项目概述

本项目实现了基于AutoSAR Classic平台的UDS（统一诊断服务）诊断通信管理功能。系统采用分层架构设计，包含应用软件组件（ASW）和基础软件组件（BSW），提供完整的诊断服务处理能力。

该实现支持多种UDS诊断服务，包括诊断会话控制、ECU复位、安全访问、通信控制、测试器存在等核心服务，以及DID（数据标识符）和RID（功能标识符）的读写处理机制。

## 项目结构

项目采用AutoSAR标准的分层架构，主要分为以下几个层次：

```mermaid
graph TB
subgraph "应用软件层 (ASW)"
DM[诊断管理器<br/>Swc_DiagnosticManager]
CM[通信管理器<br/>Swc_CommunicationManager]
IO[IO控制<br/>Swc_IOControl]
end
subgraph "基础软件层 (BSW)"
subgraph "服务层"
DCM[诊断通信管理器<br/>Dcm]
DEM[故障管理<br/>Dem]
COM[通信<br/>Com]
end
subgraph "ECUAL层"
CANIF[CAN接口<br/>CanIf]
CANTP[CAN传输协议<br/>CanTp]
MEMIF[存储接口<br/>MemIf]
end
subgraph "MCAL层"
MCU[微控制器抽象<br/>Mcu]
CAN[CAN控制器<br/>Can]
DIO[DIO端口<br/>Dio]
end
end
subgraph "硬件层"
VCU[车辆控制单元]
TESTER[诊断测试器]
end
TESTER --> DM
DM --> DCM
DCM --> CANIF
CANIF --> CAN
CAN --> VCU
```

**图表来源**
- [Swc_DiagnosticManager.h:1-211](file://src/asw/diagnostic_manager/include/Swc_DiagnosticManager.h#L1-L211)
- [Dcm.h:1-379](file://src/bsw/services/dcm/include/Dcm.h#L1-L379)

**章节来源**
- [Swc_DiagnosticManager.h:1-211](file://src/asw/diagnostic_manager/include/Swc_DiagnosticManager.h#L1-L211)
- [Dcm.h:1-379](file://src/bsw/services/dcm/include/Dcm.h#L1-L379)

## 核心组件

### 诊断管理器组件

诊断管理器是应用软件层的核心组件，负责处理UDS诊断请求和生成响应。其主要职责包括：

- 诊断会话管理
- 安全访问控制
- DTC（故障码）管理
- 与RTE（运行时环境）的接口

```mermaid
classDiagram
class Swc_DiagnosticManager {
+Swc_DiagnosticSessionType currentSession
+Swc_SecurityLevelType securityLevel
+uint8 activeProtocol
+boolean communicationEnabled
+uint32 sessionTimeout
+uint32 securityTimeout
+processRequest()
+changeSession()
+unlockSecurity()
+getDtcStatus()
}
class Swc_DiagnosticRequestType {
+uint8 serviceId
+uint8 subFunction
+uint8 dataLength
+uint8 data[256]
}
class Swc_DiagnosticResponseType {
+uint8 responseId
+uint8 dataLength
+uint8 data[256]
+uint8 negativeResponseCode
}
class Swc_DtcStatusType {
+uint32 dtcCode
+uint8 statusByte
+uint8 faultDetectionCounter
+uint8 occurrenceCounter
+uint32 agingCounter
+uint32 lastOccurrenceTime
}
Swc_DiagnosticManager --> Swc_DiagnosticRequestType
Swc_DiagnosticManager --> Swc_DiagnosticResponseType
Swc_DiagnosticManager --> Swc_DtcStatusType
```

**图表来源**
- [Swc_DiagnosticManager.h:28-87](file://src/asw/diagnostic_manager/include/Swc_DiagnosticManager.h#L28-L87)

### 诊断通信管理器

诊断通信管理器是BSW层的核心组件，遵循AutoSAR标准实现完整的UDS协议栈：

- 支持多种诊断服务
- 协议状态管理
- 错误处理和负响应
- 定时器管理

**章节来源**
- [Swc_DiagnosticManager.c:1-686](file://src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c#L1-L686)
- [Dcm.c:1-1455](file://src/bsw/services/dcm/src/Dcm.c#L1-L1455)

## 架构概览

系统采用分层架构设计，确保各层职责清晰分离：

```mermaid
sequenceDiagram
participant Tester as 诊断测试器
participant ASW as 应用软件层
participant BSW as 基础软件层
participant HW as 硬件层
Tester->>ASW : UDS诊断请求
ASW->>ASW : 参数验证和安全检查
ASW->>BSW : 调用诊断通信管理器
BSW->>BSW : 协议解析和服务处理
BSW->>HW : 硬件接口调用
HW-->>BSW : 硬件响应
BSW-->>ASW : 处理结果
ASW-->>Tester : 诊断响应
Note over ASW,BSW : 分层架构确保职责分离
Note over HW : 硬件抽象层提供统一接口
```

**图表来源**
- [Dcm.c:1046-1111](file://src/bsw/services/dcm/src/Dcm.c#L1046-L1111)
- [Swc_DiagnosticManager.c:471-531](file://src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c#L471-L531)

## 详细组件分析

### 诊断会话控制服务 (0x10)

诊断会话控制服务用于管理系统的工作模式和会话级别。

#### 请求格式
- 服务ID: 0x10
- 数据长度: 至少1字节
- 参数: 会话类型（1字节）

#### 会话类型定义
| 会话类型 | 数值 | 描述 |
|---------|------|------|
| 默认会话 | 0x01 | 基础诊断功能 |
| 扩会展开会话 | 0x03 | 扩展诊断功能 |
| 编程会话 | 0x02 | ECU编程功能 |
| 安全系统会话 | 0x04 | 安全相关诊断 |

#### 处理流程
```mermaid
flowchart TD
Start([接收诊断会话控制请求]) --> ValidateParam["验证参数长度"]
ValidateParam --> ParamValid{"参数有效?"}
ParamValid --> |否| SendInvalid["发送错误响应"]
ParamValid --> |是| CheckSession["检查目标会话类型"]
CheckSession --> SessionType{"会话类型"}
SessionType --> |默认会话| SetDefault["设置默认会话"]
SessionType --> |扩展会话| SetExtended["设置扩展会话"]
SessionType --> |编程会话| CheckSecurity["检查安全级别"]
SessionType --> |安全会话| CheckSecurity2["检查安全级别"]
CheckSecurity --> SecurityOK{"安全级别足够?"}
CheckSecurity2 --> SecurityOK2{"安全级别足够?"}
SecurityOK --> |否| SendSeqError["发送序列错误"]
SecurityOK --> |是| SetProgramming["设置编程会话"]
SecurityOK2 --> |否| SendSeqError
SecurityOK2 --> |是| SetSafety["设置安全会话"]
SetDefault --> BuildResponse["构建响应"]
SetExtended --> BuildResponse
SetProgramming --> BuildResponse
SetSafety --> BuildResponse
BuildResponse --> SendResponse["发送响应"]
SendInvalid --> End([结束])
SendSeqError --> End
SendResponse --> End
```

**图表来源**
- [Swc_DiagnosticManager.c:144-201](file://src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c#L144-L201)
- [Dcm.c:238-280](file://src/bsw/services/dcm/src/Dcm.c#L238-L280)

**章节来源**
- [Swc_DiagnosticManager.c:144-201](file://src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c#L144-L201)
- [Dcm.c:238-280](file://src/bsw/services/dcm/src/Dcm.c#L238-L280)

### ECU复位服务 (0x11)

ECU复位服务用于重新启动或关闭ECU。

#### 请求格式
- 服务ID: 0x11
- 数据长度: 至少1字节
- 参数: 复位类型（1字节）

#### 复位类型定义
| 复位类型 | 数值 | 描述 |
|---------|------|------|
| 硬复位 | 0x01 | 完全重启ECU |
| 点火开关复位 | 0x02 | 模拟点火开关动作 |
| 软复位 | 0x03 | 软件层面重启 |
| 快速断电启用 | 0x04 | 启用快速断电功能 |
| 快速断电禁用 | 0x05 | 禁用快速断电功能 |

#### 处理逻辑
```mermaid
sequenceDiagram
participant Tester as 诊断测试器
participant DCM as 诊断通信管理器
participant MCU as 微控制器
participant OS as 操作系统
Tester->>DCM : ECU复位请求 (0x11)
DCM->>DCM : 验证复位类型
DCM->>DCM : 构建响应
DCM-->>Tester : 正确响应
DCM->>OS : 触发复位操作
OS->>MCU : 执行复位
MCU-->>OS : 复位完成
```

**图表来源**
- [Dcm.c:285-324](file://src/bsw/services/dcm/src/Dcm.c#L285-L324)

**章节来源**
- [Dcm.c:285-324](file://src/bsw/services/dcm/src/Dcm.c#L285-L324)

### 安全访问服务 (0x27)

安全访问服务提供多级别的安全保护机制。

#### 请求格式
- 服务ID: 0x27
- 数据长度: 至少1字节
- 参数: 子功能（1字节）

#### 安全级别定义
| 安全级别 | 数值 | 描述 |
|---------|------|------|
| 锁定状态 | 0x00 | 无权限 |
| 客户级别 | 0x01 | 基础访问权限 |
| 工程级别 | 0x02 | 高级访问权限 |
| 制造商级别 | 0x03 | 最高访问权限 |

#### 安全流程
```mermaid
flowchart TD
Start([接收安全访问请求]) --> CheckSF["检查子功能"]
CheckSF --> IsRequestSeed{"请求种子?"}
IsRequestSeed --> |是| CheckAttempts["检查尝试次数"]
CheckAttempts --> AttemptsOK{"尝试次数充足?"}
AttemptsOK --> |否| GenSeed["生成种子"]
AttemptsOK --> |是| SendExceeded["发送超出次数错误"]
GenSeed --> BuildSeedResp["构建种子响应"]
BuildSeedResp --> SendSeedResp["发送种子"]
IsRequestSeed --> |否| VerifyKey["验证密钥"]
VerifyKey --> KeyValid{"密钥有效?"}
KeyValid --> |是| SetSecurity["设置安全级别"]
KeyValid --> |否| SendInvalidKey["发送无效密钥错误"]
SetSecurity --> BuildKeyResp["构建密钥响应"]
BuildKeyResp --> SendKeyResp["发送密钥响应"]
SendInvalidKey --> End([结束])
SendExceeded --> End
SendSeedResp --> End
SendKeyResp --> End
```

**图表来源**
- [Swc_DiagnosticManager.c:206-259](file://src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c#L206-L259)
- [Dcm.c:329-413](file://src/bsw/services/dcm/src/Dcm.c#L329-L413)

**章节来源**
- [Swc_DiagnosticManager.c:206-259](file://src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c#L206-L259)
- [Dcm.c:329-413](file://src/bsw/services/dcm/src/Dcm.c#L329-L413)

### 通信控制服务 (0x28)

通信控制服务用于启用或禁用特定类型的通信。

#### 请求格式
- 服务ID: 0x28
- 数据长度: 至少2字节
- 参数: 子功能 + 通信类型

#### 通信类型定义
| 通信类型 | 数值 | 描述 |
|---------|------|------|
| 诊断通信 | 0x01 | 诊断相关通信 |
| 生存期通信 | 0x02 | 运行时通信 |
| 所有通信 | 0x03 | 全部通信类型 |

**章节来源**
- [Dcm.c:415-453](file://src/bsw/services/dcm/src/Dcm.c#L415-L453)

### 测试器存在服务 (0x3E)

测试器存在服务用于保持诊断会话活跃状态。

#### 请求格式
- 服务ID: 0x3E
- 数据长度: 至少1字节
- 参数: 子功能（1字节）

#### 子功能定义
| 子功能 | 数值 | 描述 |
|-------|------|------|
| 激活 | 0x00 | 激活测试器存在 |
| 抑制 | 0x80 | 抑制响应（仅心跳） |

**章节来源**
- [Swc_DiagnosticManager.c:346-361](file://src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c#L346-L361)
- [Dcm.c:418-452](file://src/bsw/services/dcm/src/Dcm.c#L418-L452)

### DTC管理服务

系统支持多种DTC（故障码）管理功能：

#### 读取DTC信息 (0x19)
支持多种子功能：
- 0x01: 按状态掩码报告DTC数量
- 0x02: 按状态掩码报告DTC列表
- 0x06: 按DTC号报告扩展数据记录
- 0x0A: 报告支持的DTC

#### 清除DTC信息 (0x14)
- 清除指定DTC
- 清除所有DTC（使用特殊代码0xFFFFFF）

**章节来源**
- [Swc_DiagnosticManager.c:264-341](file://src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c#L264-L341)
- [Dcm.c:598-763](file://src/bsw/services/dcm/src/Dcm.c#L598-L763)

## 依赖关系分析

### 组件间依赖关系

```mermaid
graph TB
subgraph "应用软件组件"
DM[Swc_DiagnosticManager]
CM[Swc_CommunicationManager]
IO[Swc_IOControl]
end
subgraph "基础软件组件"
DCM[Dcm]
DEM[Dem]
COM[Com]
PduR[PduR]
end
subgraph "外部接口"
RTE[RTE]
DET[DET]
OS[操作系统]
end
DM --> DCM
DM --> RTE
DM --> DET
DCM --> PduR
DCM --> DEM
DCM --> COM
DCM --> OS
PduR --> COM
COM --> DET
```

**图表来源**
- [Swc_DiagnosticManager.c:15-17](file://src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c#L15-L17)
- [Dcm.c:20-25](file://src/bsw/services/dcm/src/Dcm.c#L20-L25)

### 配置依赖

系统配置通过JSON文件进行管理：

```mermaid
flowchart LR
Config[bsw_config.json] --> MCU[MCU配置]
Config --> CAN[CAN配置]
MCU --> Clock[时钟频率: 800MHz]
MCU --> Cores[核心数: 4]
CAN --> BaudRate[波特率: 500kbps]
CAN --> Controllers[控制器数: 2]
```

**图表来源**
- [bsw_config.json:1-21](file://config/bsw_config.json#L1-L21)

**章节来源**
- [bsw_config.json:1-21](file://config/bsw_config.json#L1-L21)

## 性能考虑

### 内存管理
- 请求缓冲区大小：256字节
- 响应缓冲区大小：256字节
- 最大DTC数量：50个
- DCM缓冲区大小：4095字节

### 时间管理
- 会话超时：5000ms
- 安全超时：5000ms
- 主函数周期：10ms
- P2定时器：50ms
- P2*定时器：5000ms

### 并发处理
系统采用单线程架构，通过RTE实现任务调度，确保诊断请求的有序处理。

## 故障排除指南

### 常见错误代码

| 错误代码 | 含义 | 可能原因 | 解决方案 |
|---------|------|----------|----------|
| 0x11 | 服务不支持 | 请求的服务未实现 | 检查服务ID是否正确 |
| 0x12 | 子功能不支持 | 子功能超出范围 | 验证子功能参数 |
| 0x13 | 消息长度错误 | 数据长度不足 | 检查请求格式 |
| 0x24 | 请求序列错误 | 安全级别不足 | 先执行安全访问 |
| 0x31 | 请求超出范围 | DTC不存在或越界 | 验证DTC代码 |
| 0x33 | 安全访问被拒绝 | 权限不足 | 检查安全级别 |
| 0x35 | 无效密钥 | 密钥验证失败 | 重新生成密钥 |

### 调试建议

1. **日志记录**：启用DET（诊断事件跟踪）记录错误信息
2. **参数验证**：在每个服务入口处添加参数检查
3. **超时监控**：定期检查会话和安全超时状态
4. **内存检查**：监控缓冲区使用情况，防止溢出

**章节来源**
- [Dcm.h:161-203](file://src/bsw/services/dcm/include/Dcm.h#L161-L203)
- [Swc_DiagnosticManager.c:496-500](file://src/asw/diagnostic_manager/src/Swc_DiagnosticManager.c#L496-L500)

## 结论

本UDS诊断服务实现提供了完整的诊断通信功能，具有以下特点：

1. **标准化实现**：完全符合AutoSAR和ISO 14229标准
2. **模块化设计**：清晰的分层架构，职责分离明确
3. **安全性保障**：多级别的安全访问控制机制
4. **可扩展性**：支持DID和RID的动态配置
5. **可靠性**：完善的错误处理和超时管理

系统能够满足现代汽车诊断需求，为ECU的开发、测试和维护提供可靠的技术支撑。