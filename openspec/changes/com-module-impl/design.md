# COM模块设计文档

## 架构概览

```
┌───────────────────────────────────────────────────────┐
│                    Application Layer (ASW)                  │
│  ┌────────┐  ┌────────┐  ┌────────┐                     │
│  │   SWC1   │  │   SWC2   │  │   SWC3   │                     │
│  └───┼──────┘  └───┼──────┘  └───┼──────┘                     │
│       │           │           │                              │
└───────┼───────────┼───────────┼───────────────────────────────────┘
        │           │           │
        ▼           ▼           ▼
┌───────────────────────────────────────────────────────┐
│                    RTE (Runtime Environment)                 │
│  ┌────────┐  ┌────────┐  ┌────────┐                     │
│  │  RTE1   │  │  RTE2   │  │  RTE3   │                     │
│  └───┼──────┘  └───┼──────┘  └───┼──────┘                     │
└───────┼───────────┼───────────┼───────────────────────────────────┘
        │           │           │
        ▼           ▼           ▼
┌───────────────────────────────────────────────────────┐
│  🔵 COM (Communication) - 本Change重点                   │
│  ┌───────────────────────────────────────────────────────┐
│  │  ┌──────────────────────────────────────────────────┐  │
│  │  │  Signal Management  │  I-PDU Handling  │  Deadline Mon  │  │
│  │  └──────────────────────────────────────────────────┘  │
│  └───────────────────────────────────────────────────────┘
│                              │                              │
└───────────────────────┼──────────────────────────────────────────────────┘
                              │
                              ▼
┌───────────────────────────────────────────────────────┐
│                  PDU Router (PduR) - 已实现                   │
└───────────────────────────────────────────────────────┘
```

## 核心设计

### 1. 数据结构

```c
/* 信号配置 */
typedef struct {
    Com_SignalIdType SignalId;
    uint8* DataPtr;              /* 指向IPDU数据区 */
    uint8 BitPosition;
    uint8 BitSize;
    Com_SignalEndiannessType Endianness;
    Com_SignalTypeType SignalType;
    Com_TransferPropertyType TransferProperty;
} Com_SignalConfigType;

/* 信号组配置 */
typedef struct {
    Com_SignalGroupIdType SignalGroupId;
    Com_SignalIdType* SignalRefs;   /* 引用的信号 */
    uint8 NumSignals;
} Com_SignalGroupConfigType;

/* I-PDU配置 */
typedef struct {
    PduIdType PduId;
    uint8* DataPtr;
    uint8 Length;
    Com_IPduDirectionType Direction;
    Com_IPduType Type;
    Com_IPduSignalProcessingType SignalProcessing;
    Com_SignalIdType* SignalRefs;
    uint8 NumSignals;
    Com_SignalGroupIdType* SignalGroupRefs;
    uint8 NumSignalGroups;
} Com_IPduConfigType;

/* 传输模式配置 */
typedef struct {
    Com_IPduIdType IPduId;
    Com_TransferModeType Mode;          /* PERIODIC, EVENT, MIXED */
    uint32 Period;                       /* 周期(ms) */
    uint32 RepetitionPeriod;             /* 重复周期 */
    uint8 NumRepetitions;                /* 重复次数 */
    uint32 TimeOffset;                   /* 起始偏移 */
} Com_IPduTxModeConfigType;
```

### 2. 状态机

```
Com_Init() → COM_UNINIT → COM_READY → Com_DeInit() → COM_UNINIT

I-PDU状态:
  STOPPED ←──────→ STARTED
    ↑                    ↓
    └─── Com_IpduGroupStop()  Com_IpduGroupStart()
```

### 3. 信号打包/解包流程

```
发送流程:
Com_SendSignal() → 更新信号缓冲区 → 检查触发条件 → 
    ├── 立即发送: Com_TriggerIPDUSend()
    └── 延迟发送: 标记Dirty位

接收流程:
PduR_ComRxIndication() → 解包所有信号 → 更新信号数据 → 
    ├── IMMEDIATE: 通知RTE
    └── DEFERRED: 延迟到Com_MainFunctionRx()
```

## 关键算法

### 1. 位置映射

```c
/* 大端/小端转换和位提取 */
static uint64 Com_ExtractSignal(uint8* data, uint16 bitPos, uint8 bitSize, 
                                 Com_SignalEndiannessType endianness);
static void Com_InsertSignal(uint8* data, uint16 bitPos, uint8 bitSize,
                              Com_SignalEndiannessType endianness, uint64 value);
```

### 2. 传输模式调度

```c
/* MainFunctionTx 中的调度逻辑 */
for each I-PDU in TxMode:
    if Mode == PERIODIC:
        if TimerExpired:
            SendIPDU()
            ResetTimer(Period)
    else if Mode == EVENT:
        if Triggered:
            SendIPDU()
    else if Mode == MIXED:
        if Triggered:
            SendIPDU()
            StartRepetitionTimer()
        else if TimerExpired:
            SendIPDU()
```

## 接口设计

### 标准接口

```c
/* 初始化 */
void Com_Init(const Com_ConfigType* config);
void Com_DeInit(void);

/* 信号操作 */
uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);
uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr);

/* 信号组操作 */
uint8 Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId);
uint8 Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId);
void Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);

/* I-PDU组控制 */
void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean Initialize);
void Com_IpduGroupStop(Com_IpduGroupIdType IpduGroupId);

/* 主函数 */
void Com_MainFunctionRx(void);
void Com_MainFunctionTx(void);
```

### 回调接口

```c
/* 从PduR接收 */
void PduR_ComRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void PduR_ComTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
Std_ReturnType PduR_ComTriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr);
```

## 配置示例

```c
/* Com_Cfg.c */
const Com_SignalConfigType ComSignals[] = {
    {
        .SignalId = ComConf_ComSignal_EngineSpeed,
        .DataPtr = &ComIPDU_Engine[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED
    },
    /* ... 更多信号 */
};

const Com_IPduConfigType ComIPdus[] = {
    {
        .PduId = ComConf_ComIPdu_EngineData,
        .DataPtr = ComIPDU_Engine,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = EngineSignals,
        .NumSignals = 3
    }
};
```

## 测试策略

1. **单元测试**
   - 信号打包/解包正确性
   - 传输模式验证
   - 时序测试

2. **集成测试**
   - 与PduR集成
   - 与RTE集成
   - 端到端数据流

3. **性能测试**
   - 信号发送延迟
   - I-PDU处理吞吐量

## 关键指标

| 指标 | 目标值 | 说明 |
|------|--------|------|
| 代码覆盖率 | >90% | 单元测试 |
| 信号发送延迟 | <10us | 从调用到缓冲区更新 |
| 支持信号数 | >100 | 单个I-PDU |
| RAM占用 | <10KB | 含缓冲区和状态 |

---

*由 OSH Orchestrator 生成*
