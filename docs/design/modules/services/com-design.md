# COM 模块设计文档

> **Module ID**: 0x1E  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_COM  
> **Source Path**: `src/bsw/services/com/`  
> **Reference Document**: `docs/modules/COM.md`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

COM（Communication Module）是 AUTOSAR 服务层通信模块，负责基于信号（Signal）和信号组（Signal Group）的整车网络数据交换。COM 向上通过 RTE 为 SWC 提供 `Com_SendSignal`、`Com_ReceiveSignal` 等接口，向下通过 PduR 将 I-PDU 路由到 CAN、LIN、FlexRay 或以太网等总线协议。

COM 在 AUTOSAR 分层中的位置：

```
┌─────────────────────────────────────┐
│  ASW (Application Software)         │
├─────────────────────────────────────┤
│  RTE                                │
├─────────────────────────────────────┤
│  COM (Services)                     │
├─────────────────────────────────────┤
│  PduR (Services)                    │
├─────────────────────────────────────┤
│  CanIf / LinIf / EthIf (ECUAL)      │
├─────────────────────────────────────┤
│  Can / Lin / Eth (MCAL)             │
└─────────────────────────────────────┘
```

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS COM | 4.4.0 | AUTOSAR 通信模块软件规范 |
| AUTOSAR Classic Platform | 4.x | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | RTE / ASW | 通过信号接口读写数据 |
| 下层 | PduR | I-PDU 的发送、接收、触发发送确认 |
| 公共 | Det | 开发错误检测（COM_DEV_ERROR_DETECT） |
| 公共 | Dem | 诊断事件（预留，当前未实现具体事件） |

---

## 3. 架构设计

### 3.1 分层位置

COM 属于服务层，位于 RTE 与 PduR 之间，是所有信号级通信的汇聚点。

### 3.2 内部组件

| 组件 | 职责 |
|------|------|
| Signal Pack/Unpack | 将应用层信号按位打包到 I-PDU，或从 I-PDU 解包 |
| Filter Engine | 对发送信号应用过滤算法，决定是否触发发送 |
| Tx Manager | 管理周期发送、重复发送、事件触发发送 |
| Rx Manager | 接收 PduR 数据并写入 I-PDU 缓冲区 |
| Signal Group Manager | 通过 Shadow Buffer 处理一致性信号组 |
| I-PDU Group Manager | 按 I-PDU Group 控制启动/停止 |
| Deadline Monitoring | 接收超时监控（接口存在，当前简化实现） |

### 3.3 文件结构

```
src/bsw/services/com/
├── include/
│   ├── Com.h           # 公共 API、类型、错误码、SID
│   └── Com_Cfg.h       # 预编译配置（代码生成）
└── src/
    ├── Com.c           # 核心实现
    ├── Com_Lcfg.c      # 链接时配置表（信号、I-PDU）
    └── Com_test.c      # 单元测试
```

---

## 4. 状态机

### 4.1 模块状态

```
        ┌─────────────┐
  Init  │   UNINIT    │
───────►│             │
        └──────┬──────┘
               │ DeInit
               ▼
        ┌─────────────┐
        │    INIT     │
        └─────────────┘
```

- `COM_UNINIT`：模块未初始化，除 `Com_Init` 与 `Com_GetVersionInfo` 外所有 API 返回错误。
- `COM_INIT`：模块已初始化，可执行信号收发与主函数处理。

### 4.2 I-PDU 发送状态

| 状态 | 含义 |
|------|------|
| `COM_TX_IDLE` | 空闲，可发送 |
| `COM_TX_PENDING` | 已调用 `PduR_Transmit`，等待 `Com_TxConfirmation` |
| `COM_TX_ACTIVE` | 保留 |

### 4.3 信号更新状态

| 标志 | 含义 |
|------|------|
| `Updated` | 信号/I-PDU 有新数据待处理 |
| `FilterPassed` | 信号通过过滤算法 |

---

## 5. 核心数据结构

### 5.1 运行时内部状态

```c
typedef struct
{
    uint8 State;                                    /* COM_UNINIT / COM_INIT */
    const Com_ConfigType* ConfigPtr;                /* 当前配置指针 */
    Com_IPduStateType IPduStates[COM_NUM_OF_IPDUS]; /* 每个 I-PDU 状态 */
    Com_SignalStateType SignalStates[COM_NUM_OF_SIGNALS]; /* 每个信号状态 */
    uint8 IPduBuffer[COM_NUM_OF_IPDUS][COM_MAX_IPDU_BUFFER_SIZE]; /* I-PDU 数据缓冲 */
    uint8 ShadowBuffer[COM_MAX_IPDU_BUFFER_SIZE];   /* 信号组影子缓冲 */
    Com_IpduGroupVector IPduGroupVector;            /* I-PDU 组使能向量 */
} Com_InternalStateType;
```

### 5.2 I-PDU 运行时状态

```c
typedef struct
{
    uint8 TxState;          /* COM_TX_IDLE / COM_TX_PENDING */
    uint8 RepetitionCount;  /* 当前重复计数 */
    uint32 TimeCounter;     /* 周期/重复发送倒计时 */
    boolean Updated;        /* 是否有新数据 */
    boolean GroupEnabled;   /* 所属 I-PDU Group 是否使能 */
} Com_IPduStateType;
```

### 5.3 信号运行时状态

```c
typedef struct
{
    boolean Updated;        /* 信号是否被更新 */
    boolean FilterPassed;   /* 是否通过过滤 */
    uint32 LastValue;       /* 上一次发送值（用于过滤算法） */
} Com_SignalStateType;
```

### 5.4 信号配置

```c
typedef struct {
    Com_SignalIdType SignalId;    /* 信号 ID */
    uint16 BitPosition;           /* 在 I-PDU 中的起始位 */
    uint8 BitSize;                /* 位长度 */
    uint8 Endianness;             /* COM_LITTLE_ENDIAN / COM_BIG_ENDIAN */
    uint8 TransferProperty;       /* 触发属性 */
    uint8 FilterAlgorithm;        /* 过滤算法 */
    uint32 FilterMask;            /* 过滤掩码 */
    uint32 FilterX;               /* 过滤参考值 */
    uint16 SignalGroupRef;        /* 所属 I-PDU ID */
} Com_SignalConfigType;
```

### 5.5 I-PDU 配置

```c
typedef struct {
    PduIdType PduId;              /* PduR 层 PDU ID */
    uint16 DataLength;            /* I-PDU 数据长度 */
    boolean RepeatingEnabled;     /* 是否使能重复发送 */
    uint8 NumRepetitions;         /* 重复次数 */
    uint16 TimeBetweenRepetitions; /* 重复间隔 */
    uint16 TimePeriod;            /* 周期发送周期 */
} Com_IPduConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | SWS 需求 |
|-----|------|------|----------|
| Com_Init | `void Com_Init(const Com_ConfigType* config)` | 初始化模块、缓冲区、状态 | SWS_Com_00001 |
| Com_DeInit | `void Com_DeInit(void)` | 反初始化 | SWS_Com_00002 |
| Com_SendSignal | `uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)` | 发送信号，打包并触发发送 | SWS_Com_00003 |
| Com_ReceiveSignal | `uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr)` | 从 I-PDU 解包信号 | SWS_Com_00004 |
| Com_SendSignalGroup | `uint8 Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId)` | 发送信号组（Shadow Buffer -> I-PDU） | SWS_Com_00005 |
| Com_ReceiveSignalGroup | `uint8 Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId)` | 接收信号组（I-PDU -> Shadow Buffer） | SWS_Com_00006 |
| Com_UpdateShadowSignal | `uint8 Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)` | 更新影子信号 | SWS_Com_00007 |
| Com_ReceiveShadowSignal | `uint8 Com_ReceiveShadowSignal(Com_SignalIdType SignalId, void* SignalDataPtr)` | 读取影子信号 | SWS_Com_00008 |
| Com_TriggerIPDUSend | `Std_ReturnType Com_TriggerIPDUSend(PduIdType PduId)` | 立即触发 I-PDU 发送 | SWS_Com_00010 |
| Com_IpduGroupControl | `void Com_IpduGroupControl(Com_IpduGroupVector IpduGroupVector, boolean enable)` | 控制 I-PDU Group 启停 | SWS_Com_00021 |
| Com_MainFunctionRx | `void Com_MainFunctionRx(void)` | 接收主函数 | SWS_Com_00013 |
| Com_MainFunctionTx | `void Com_MainFunctionTx(void)` | 发送主函数（周期/重复调度） | SWS_Com_00014 |
| Com_MainFunctionRouteSignals | `void Com_MainFunctionRouteSignals(void)` | 信号网关路由（COM_GATEWAY_SUPPORT 开关） | SWS_Com_00015 |
| Com_GetVersionInfo | `void Com_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 获取版本信息 | SWS_Com_00017 |

### 6.2 PduR 回调

| 回调 | 签名 | 功能 | SWS 需求 |
|------|------|------|----------|
| Com_TriggerTransmit | `Std_ReturnType Com_TriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)` | PduR 请求当前 I-PDU 数据 | SWS_Com_00009 |
| Com_TxConfirmation | `void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)` | 发送完成确认 | SWS_Com_00011 |
| Com_RxIndication | `void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)` | PduR 通知接收到数据 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x01 | Com_Init | COM_E_PARAM_POINTER |
| 0x02 | Com_DeInit | COM_E_UNINIT |
| 0x08 | Com_SendSignal | COM_E_UNINIT / COM_E_PARAM_POINTER / COM_E_INVALID_SIGNAL_ID |
| 0x09 | Com_ReceiveSignal | COM_E_UNINIT / COM_E_PARAM_POINTER / COM_E_INVALID_SIGNAL_ID |
| 0x1E | Com_MainFunctionRx | COM_E_UNINIT |
| 0x1F | Com_MainFunctionTx | COM_E_UNINIT |

主要错误码：

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x01 | COM_E_PARAM | 通用参数错误 |
| 0x02 | COM_E_UNINIT | 模块未初始化 |
| 0x03 | COM_E_PARAM_POINTER | 空指针入参 |
| 0x06 | COM_E_INVALID_SIGNAL_ID | 信号 ID 越界 |
| 0x08 | COM_E_INVALID_IPDU_ID | I-PDU ID 越界 |

---

## 7. 处理流程

### 7.1 信号发送流程

1. SWC/RTE 调用 `Com_SendSignal(SignalId, DataPtr)`。
2. COM 检查模块状态、参数合法性。
3. 将信号值转换为 `uint32`：`Com_GetSignalValueAsUint32`。
4. 应用过滤算法：`Com_ApplyFilter`。
5. 若过滤通过，调用 `Com_PackSignal` 将信号按位写入 I-PDU 缓冲区（支持大端/小端）。
6. 更新信号状态与 I-PDU 更新标志。
7. 若触发属性为 `COM_TRIGGERED` 或 `COM_TRIGGERED_ON_CHANGE`，立即调用 `Com_TransmitIPdu` 通过 PduR 发送。

### 7.2 信号接收流程

1. PduR 在收到总线数据后调用 `Com_RxIndication(PduId, PduInfo)`。
2. COM 校验状态与指针，将接收数据复制到对应 I-PDU 缓冲区。
3. 设置 I-PDU 的 `Updated` 标志。
4. SWC/RTE 调用 `Com_ReceiveSignal(SignalId, DataPtr)`。
5. COM 调用 `Com_UnpackSignal` 从 I-PDU 缓冲区解包信号值。
6. 清除该信号的 `Updated` 标志。

### 7.3 周期发送流程（Com_MainFunctionTx）

1. 遍历所有已配置 I-PDU。
2. 跳过未使能的 I-PDU Group。
3. 对每个配置 `TimePeriod > 0` 的 I-PDU：
   - 若 `TimeCounter == 0`，调用 `Com_TransmitIPdu` 并重新装载计数器；
   - 否则 `TimeCounter--`。
4. 对配置重复发送且仍有剩余次数的 I-PDU，在 `TimeCounter == 0` 时再次发送。

### 7.4 PduR 触发发送流程

1. PduR 调用 `Com_TriggerTransmit(TxPduId, PduInfoPtr)`。
2. COM 将 `PduInfoPtr->SduDataPtr` 指向当前 I-PDU 缓冲区。
3. 设置 `SduLength` 为配置的数据长度，返回 `E_OK`。

---

## 8. 配置设计

### 8.1 预编译配置（Com_Cfg.h）

| 宏 | 当前值 | 说明 |
|----|--------|------|
| COM_DEV_ERROR_DETECT | STD_ON | 开发错误检测开关 |
| COM_VERSION_INFO_API | STD_ON | 版本信息 API 开关 |
| COM_NUM_OF_IPDUS | 64 | 最大 I-PDU 数量 |
| COM_NUM_OF_SIGNALS | 256 | 最大信号数量 |
| COM_NUM_OF_IPDU_GROUPS | 16 | 最大 I-PDU 组数量 |
| COM_MAX_IPDU_BUFFER_SIZE | 128 | 每个 I-PDU 最大缓冲区字节数 |
| COM_MAX_SIGNAL_LENGTH | 64 | 信号最大字节长度 |

### 8.2 链接时配置（Com_Lcfg.c）

- `Com_Signals[]`：信号配置表，包含位位置、长度、端序、触发属性、过滤参数、所属 I-PDU。
- `Com_IPdus[]`：I-PDU 配置表，包含 PduId、数据长度、重复参数、周期。
- `Com_Config`：顶层配置结构，聚合以上两张表。

### 8.3 构建后配置

当前实现采用预编译 + 链接时配置，未支持构建后（Post-Build）配置变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 在 `COM_DEV_ERROR_DETECT == STD_ON` 时检查：
- 模块是否已初始化；
- 入参指针是否为空；
- ID 是否越界。

### 9.2 DEM 错误

当前实现未定义具体 DEM 事件，接口为预留。

### 9.3 安全机制

- 信号打包/解包在位索引时未做越界写保护，依赖配置工具生成合法配置。
- 接收数据复制长度取 `min(SduLength, COM_MAX_IPDU_BUFFER_SIZE)`，防止缓冲区溢出。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| COM_START_SEC_VAR_CLEARED_UNSPECIFIED | 零初始化全局变量（Com_InternalState、Com_DMEnabled） |
| COM_START_SEC_CODE | 代码段 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~9 KB | 64 I-PDU × 128 B + 256 信号状态 + 内部状态 |
| ROM | 依赖配置 | 配置表 + 代码 |
| 堆栈 | 较小 | 无递归，局部变量少 |

---

## 11. 集成指南

### 11.1 初始化顺序

1. PduR 初始化
2. 下层 ECUAL 接口初始化（CanIf / LinIf / EthIf）
3. `Com_Init(&Com_Config)`
4. 使能需要的 I-PDU Group：`Com_IpduGroupControl(..., TRUE)`

### 11.2 与 PduR 集成

- 发送：`Com_TransmitIPdu` -> `PduR_Transmit`
- 发送确认：`PduR` -> `Com_TxConfirmation`
- 接收：`PduR` -> `Com_RxIndication`
- 触发发送：`PduR` -> `Com_TriggerTransmit`

### 11.3 与 RTE 集成

RTE 通过 `Com_SendSignal` / `Com_ReceiveSignal` 读写信号；对于信号组，需先 `Com_UpdateShadowSignal` / `Com_ReceiveShadowSignal`，再 `Com_SendSignalGroup` / `Com_ReceiveSignalGroup`。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| Com_test.c | 初始化、信号打包/解包、大端/小端、过滤、发送/接收、I-PDU Group 控制 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 周期发送 | 验证 `Com_MainFunctionTx` 按 TimePeriod 调度 |
| 事件触发发送 | 验证 `COM_TRIGGERED` 信号立即发送 |
| 接收解包 | 验证 `Com_RxIndication` + `Com_ReceiveSignal` 数据一致性 |
| 信号组一致性 | 验证 Shadow Buffer 与 I-PDU 的同步 |

---

## 13. 实现说明 / TODO

- 当前实现支持 8/16/32 位信号打包，未支持大于 32 位的信号。
- `Com_MainFunctionRouteSignals` 为网关路由预留，仅在 `COM_GATEWAY_SUPPORT == STD_ON` 时展开循环，实际映射逻辑待实现。
- 动态信号 `Com_SendDynSignal` / `Com_ReceiveDynSignal` 当前返回 `COM_SERVICE_NOT_OK`。
- Deadline Monitoring 使能标志 `Com_DMEnabled` 已存储，但未在接收主函数中实现超时判定。
- `Com_IpduGroupControl` 当前简单使能/禁用所有 I-PDU，未按 `IpduGroupVector` 逐位解析。

---

## 14. 参考资料

1. AUTOSAR_SWS_COM.pdf 4.4.0
2. `docs/modules/COM.md`
3. `src/bsw/services/com/include/Com.h`
4. `src/bsw/services/com/src/Com.c`
5. `src/bsw/services/com/src/Com_Lcfg.c`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Com | — | COM 模块级需求归属 |
| SWS_Com_00022 | `—` |  |
| SWS_Com_00024 | `—` |  |
| SWS_Com_00025 | `—` |  |
