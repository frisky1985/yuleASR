# Xcp Design Document

> **Module ID**: 0xA5  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_XCP  
> **Source Path**: `src/bsw/services/xcp/`  
> **Reference Document**: `docs/modules/xcp.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Xcp（Universal Measurement and Calibration Protocol）模块实现了 ASAM XCP 1.1 标准协议，为 ECU 提供通用的测量与标定功能。该模块支持多种传输层（CAN、Ethernet UDP/TCP、FlexRay），允许外部工具（如 INCA、CANape）通过标准命令进行 ECU 内部变量的实时读取（Measurement）、参数修改（Calibration）和闪存编程（Programming）。

主要职责：
- XCP 协议连接管理（Connect/Disconnect）
- 标准命令处理（Upload/Download/SetMTA 等）
- DAQ（Data Acquisition）列表管理与采样
- STIM（Stimulation）数据处理
- PGM（Programming）闪存编程流程
- 资源保护与 Seed/Key 安全机制
- 内存访问验证

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| ASAM XCP 1.1 | 1.1 | 通用测量标定协议 |
| AUTOSAR SWS XCP | 4.4.0 | XCP 模块软件规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | RTE / ASWC | 测量标定变量访问 |
| 下层 | CanIf / SoAd / FrIf | 传输层接口 |
| 下层 | PduR | PDU 路由 |
| 公共 | Det | 开发错误追踪 |
| 公共 | Dem | 诊断事件管理 |
| 公共 | Crc | 校验和计算 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        Application Layer            │
├─────────────────────────────────────┤
│        Xcp (Services Layer)         │
├─────────────────────────────────────┤
│     CanIf / SoAd / FrIf / PduR     │
├─────────────────────────────────────┤
│     MCAL (Can/Eth/FlexRay)          │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **命令处理器（Command Processor）**：分发和处理 XCP 命令（标准/DAQ/PGM）
- **连接管理器（Connection Manager）**：管理多通道连接状态
- **DAQ 引擎（DAQ Engine）**：DAQ 列表配置、采样与 DTO 传输
- **STIM 处理器（STIM Processor）**：处理激励数据队列
- **PGM 引擎（Programming Engine）**：闪存编程流程管理
- **内存访问管理器（Memory Access Manager）**：验证和执行内存读写
- **安全管理器（Security Manager）**：Seed/Key 资源保护

### 3.3 文件结构

```
src/bsw/services/xcp/
├── include/
│   ├── Xcp.h
│   ├── Xcp_Cfg.h
│   └── Xcp_MemMap.h
├── src/
│   └── Xcp.c
└── legacy/
```

---

## 4. 状态机

连接状态机：

```
[XCP_STATE_DISCONNECTED]
    │ CONNECT command
    ▼
[XCP_STATE_CONNECTED]
    │ DISCONNECT command
    ▼
[XCP_STATE_DISCONNECTED]
```

PGM 状态机：

```
[XCP_PGM_STATE_IDLE]
    │ PROGRAM_START
    ▼
[XCP_PGM_STATE_STARTED]
    │ PROGRAM command
    ▼
[XCP_PGM_STATE_PROGRAMMING]
    │ PROGRAM_RESET
    ▼
[XCP_PGM_STATE_IDLE]
```

DAQ 列表状态：

```
[XCP_DAQ_STATE_STOPPED]
    │ START_STOP_DAQ_LIST (mode=0x01)
    ▼
[XCP_DAQ_STATE_RUNNING]
    │ START_STOP_DAQ_LIST (mode=0x00)
    ▼
[XCP_DAQ_STATE_STOPPED]
```

---

## 5. 核心数据结构

```c
/* 通道状态 */
typedef struct {
    uint8 SequenceNumber;
    boolean Connected;
    Xcp_ConnectionStateType State;
    Xcp_SessionStatusType SessionStatus;
    Xcp_MtaType Mta;
    uint8 ResourceProtection;
    boolean ResourcesLocked[XCP_MAX_SEEDS];
    uint8 CurrentSeed[XCP_MAX_SEEDS][4];
    uint8 MaxCto;
    uint16 MaxDto;
    Xcp_CommModeType CommMode;
} Xcp_ChannelStateType;

/* MTA（内存传输地址） */
typedef struct {
    uint32 Address;
    Xcp_AddressExtensionType Extension;
} Xcp_MtaType;

/* DAQ 列表 */
typedef struct {
    uint16 ListNumber;
    Xcp_OdtType* OdtList;
    uint8 NumOdts;
    uint8 Mode;
    uint16 Prescaler;
    uint16 EventChannel;
    uint8 Priority;
    Xcp_DaqStateType State;
    boolean IsAllocated;
    uint32 CurrentTimestamp;
    uint16 CurrentOdt;
} Xcp_DaqListType;

/* ODT 条目 */
typedef struct {
    uint32 BitOffset;
    uint32 EleLength;
    uint8 AddrExt;
    uint32 Addr;
    boolean IsValid;
} Xcp_OdtEntryType;

/* 全局配置 */
typedef struct {
    const Xcp_ChannelConfigType* ChannelConfigs;
    uint8 NumChannels;
    const Xcp_SessionConfigType* SessionConfig;
    Xcp_DaqListType* DaqLists;
    uint8 NumDaqLists;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean BlockDownloadSupported;
    boolean InterleavedModeSupported;
    uint16 MainFunctionPeriod;
} Xcp_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| Xcp_Init | `void Xcp_Init(const Xcp_ConfigType* ConfigPtr)` | 初始化 | | SWS_Xcp_00001 |
| Xcp_DeInit | `void Xcp_DeInit(void)` | 反初始化 | | SWS_Xcp_00002 |
| Xcp_GetVersionInfo | `void Xcp_GetVersionInfo(Std_VersionInfoType*)` | 版本信息 | 条件编译 | SWS_Xcp_00003 |
| Xcp_MainFunction | `void Xcp_MainFunction(void)` | 周期处理 | DAQ 处理 | SWS_Xcp_00004 |
| Xcp_RxIndication | `void Xcp_RxIndication(uint8, PduIdType, const PduInfoType*)` | 接收指示 | 下层回调 | SWS_Xcp_00005 |
| Xcp_TxConfirmation | `void Xcp_TxConfirmation(uint8, PduIdType)` | 发送确认 | | SWS_Xcp_00006 |
| Xcp_TriggerTransmit | `Std_ReturnType Xcp_TriggerTransmit(uint8, PduIdType, PduInfoType*)` | 触发发送 | | SWS_Xcp_00007 |
| Xcp_SetTransmissionMode | `void Xcp_SetTransmissionMode(uint8, uint8)` | 设置传输模式 | | SWS_Xcp_00008 |
| Xcp_GetSessionStatus | `Xcp_SessionStatusType Xcp_GetSessionStatus(void)` | 获取会话状态 | |  |
| Xcp_ProcessCommand | `void Xcp_ProcessCommand(uint8, const uint8*, uint8)` | 命令处理 | | SWS_Xcp_00009 |
| Xcp_ReadMemory | `Std_ReturnType Xcp_ReadMemory(uint32, uint8, uint8*, uint32)` | 内存读取 | | SWS_Xcp_00047 |
| Xcp_WriteMemory | `Std_ReturnType Xcp_WriteMemory(uint32, uint8, const uint8*, uint32)` | 内存写入 | | SWS_Xcp_00048 |
| Xcp_SetResourceProtection | `void Xcp_SetResourceProtection(uint8, boolean)` | 设置资源保护 | | SWS_Xcp_00049 |
| Xcp_IsResourceProtected | `boolean Xcp_IsResourceProtected(uint8)` | 检查保护状态 | | SWS_Xcp_00050 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| Xcp_RxIndication | 下层传输层接收到 XCP 命令后调用 |
| Xcp_TxConfirmation | 下层传输层确认发送完成后调用 |
| Xcp_TriggerTransmit | 下层传输层请求发送 DTO 数据 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | Init | XCP_E_PARAM_POINTER |
| 0x01 | DeInit | — |
| 0x02 | GetVersionInfo | XCP_E_PARAM_POINTER |
| 0x03 | MainFunction | — |
| 0x04 | RxIndication | XCP_E_NOT_INITIALIZED, XCP_E_PARAM_POINTER, XCP_E_PARAM_CHANNEL |
| 0x05 | TxConfirmation | XCP_E_NOT_INITIALIZED, XCP_E_PARAM_CHANNEL |
| 0x06 | TriggerTransmit | XCP_E_NOT_INITIALIZED, XCP_E_PARAM_POINTER, XCP_E_PARAM_CHANNEL |
| 0x07 | SetTransmissionMode | XCP_E_NOT_INITIALIZED, XCP_E_PARAM_CHANNEL |
| 0x08 | GetSessionStatus | — |

---

## 7. 处理流程

### 7.1 命令处理流程

1. `Xcp_RxIndication` 接收 PDU 数据
2. 调用 `Xcp_ProcessCommand` 解析命令码
3. 根据命令码路由到对应处理器：
   - 标准命令（CONNECT/DISCONNECT/UPLOAD/DOWNLOAD 等）→ `Xcp_ProcessStandardCommand`
   - DAQ 命令（CLEAR_DAQ/WRITE_DAQ/START_STOP 等）→ `Xcp_ProcessDaqCommand`
   - PGM 命令（PROGRAM_START/PROGRAM/PROGRAM_RESET 等）→ `Xcp_ProcessPgmCommand`
4. 执行命令后通过 `Xcp_SendResponse` 或 `Xcp_SendError` 返回结果

### 7.2 DAQ 采样流程

1. `Xcp_MainFunction` 周期调用 `Xcp_DaqProcessor`
2. 遍历所有 RUNNING 状态的 DAQ 列表
3. 检查 Prescaler 条件，调用 `Xcp_DaqSample`
4. 对每个 ODT 的每个有效条目从内存读取数据
5. 组装 DTO 报文（含 PID、可选时间戳）
6. 调用 `Xcp_DaqTransmit` 发送

### 7.3 编程流程

1. PROGRAM_START → 进入 PGM 状态
2. PROGRAM_CLEAR → 擦除闪存扇区
3. PROGRAM → 写入数据（通过 SetMTA + Download）
4. PROGRAM_VERIFY → 校验编程数据
5. PROGRAM_RESET → 退出 PGM 状态

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| XCP_DEV_ERROR_DETECT | STD_ON | 开发错误检测 |
| XCP_VERSION_INFO_API | STD_ON | 版本信息 API |
| XCP_NUMBER_OF_CHANNELS | 配置 | 通道数量 |
| XCP_MAX_CTO_SIZE | 配置 | 最大 CTO 大小 |
| XCP_MAX_DTO_SIZE | 配置 | 最大 DTO 大小 |
| XCP_MAX_DAQ_LISTS | 配置 | 最大 DAQ 列表数 |
| XCP_MAX_ODTS_PER_DAQ | 配置 | 每 DAQ 最大 ODT 数 |
| XCP_MAX_ODT_ENTRIES_PER_ODT | 配置 | 每 ODT 最大条目数 |
| XCP_DAQ_SUPPORTED | STD_ON | DAQ 功能支持 |
| XCP_PROGRAMMING_SUPPORTED | STD_ON | PGM 功能支持 |
| XCP_NUMBER_OF_MEMORY_RANGES | 配置 | 内存区域数量 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| Xcp_Config (const) | 全局配置常量，包含通道/DAQ/会话配置 |
| Xcp_MemoryRanges (const) | 内存访问范围定义 |

### 8.3 构建后配置

不适用。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x00 | XCP_E_NO_ERROR | 无错误 |
| 0x01 | XCP_E_PARAM_POINTER | 空指针入参 |
| 0x02 | XCP_E_PARAM_CHANNEL | 无效通道 ID |
| 0x03 | XCP_E_PARAM_DAQ | 无效 DAQ 参数 |
| 0x04 | XCP_E_PARAM_STIM | 无效 STIM 参数 |
| 0x05 | XCP_E_NOT_INITIALIZED | 模块未初始化 |
| 0x06 | XCP_E_INVALID_SEQUENCE | 无效命令序列 |
| 0x07 | XCP_E_PDU_LENGTH | PDU 长度错误 |
| 0x08 | XCP_E_OUT_OF_RANGE | 参数超范围 |
| 0x09 | XCP_E_BUSY | 模块忙 |

### 9.2 XCP 协议错误码

| 错误码 | 名称 | 说明 |
|--------|------|------|
| 0x00 | ERR_CMD_SYNCH | 命令同步错误 |
| 0x10 | ERR_CMD_BUSY | 命令忙 |
| 0x11 | ERR_DAQ_ACTIVE | DAQ 活跃中 |
| 0x12 | ERR_PGM_ACTIVE | 编程活跃中 |
| 0x20 | ERR_CMD_UNKNOWN | 未知命令 |
| 0x21 | ERR_CMD_SYNTAX | 命令语法错误 |
| 0x22 | ERR_OUT_OF_RANGE | 参数超范围 |
| 0x23 | ERR_WRITE_PROTECTED | 写保护 |
| 0x24 | ERR_ACCESS_DENIED | 访问拒绝 |
| 0x25 | ERR_ACCESS_LOCKED | 资源锁定 |
| 0x30 | ERR_MEMORY_OVERFLOW | 内存溢出 |
| 0x31 | ERR_GENERIC | 通用错误 |

### 9.3 安全机制

- Seed/Key 资源保护机制（CAL/PAG、DAQ、STIM、PGM）
- 内存访问范围验证（RAM/Flash/EEPROM 区域检查）
- 命令序列验证
- ASIL 等级：QM

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| XCP_START_SEC_CODE / XCP_STOP_SEC_CODE | 代码段 |
| XCP_START_SEC_VAR_INIT_UNSPECIFIED | 初始化变量（Xcp_Initialized） |
| XCP_START_SEC_VAR_NOINIT_UNSPECIFIED | 未初始化变量（通道状态、DAQ 列表、缓冲区） |
| XCP_START_SEC_CONST_UNSPECIFIED | 常量（协议版本、内存范围） |
| XCP_START_SEC_CONFIG_DATA_UNSPECIFIED | 配置数据（Xcp_Config） |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~8-16 KB | DAQ 列表 + ODT 条目 + TX/DAQ 缓冲区 + STIM 队列 |
| ROM | ~12 KB | 命令处理器 + 所有命令处理函数 |
| 堆栈 | ~1 KB | 命令处理调用栈 |

---

## 11. 集成指南

- 与上层集成：通过 RTE 访问测量标定变量
- 与下层集成：通过 CanIf/SoAd 传输 XCP CTO/DTO 报文
- 初始化顺序：Det → CanIf/SoAd → Xcp_Init
- MainFunction 周期建议：1-10ms（影响 DAQ 采样精度）
- A2L 文件需与 XCP 配置一致

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| test_xcp.c | 初始化、CONNECT/DISCONNECT、UPLOAD/DOWNLOAD、DAQ 配置与启动、PGM 流程 |
| test_xcp_memory.c | 内存读写、访问验证 |
| test_xcp_security.c | Seed/Key、资源保护 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 端到端标定 | 使用 XCP 工具进行变量读取和修改 |
| DAQ 采样 | 验证 DAQ 列表采样和 DTO 传输 |
| 闪存编程 | 完整 PGM 流程测试 |
| 安全机制 | Seed/Key 解锁流程 |

---

## 13. 实现说明 / TODO

- 命令路由和标准命令处理已完整实现
- DAQ 命令处理已实现（ClearDaqList/SetDaqPtr/WriteDaq/StartStop 等）
- PGM 命令处理已实现（ProgramStart/Program/ProgramReset/ProgramVerify）
- DAQ 采样引擎已实现（Xcp_DaqProcessor/Xcp_DaqSample）
- 内存访问验证已实现（基于内存范围表）
- `Xcp_SendResponse`/`Xcp_SendError` 中实际传输调用被注释，需集成传输层
- `Xcp_GetTimestamp` 使用简单计数器，需替换为硬件定时器
- STIM 处理器为简化实现
- CRC 校验和函数已实现但未在 BUILD_CHECKSUM 命令中使用

---

## 14. 参考资料

1. ASAM XCP Version 1.1 Specification
2. AUTOSAR_SWS_XCP.pdf
3. `docs/modules/xcp.md`
4. `src/bsw/services/xcp/`
