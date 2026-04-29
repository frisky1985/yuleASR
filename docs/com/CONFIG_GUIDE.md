# COM 模块配置指南

> **版本**: v1.0  
> **更新日期**: 2024年

---

## 目录

1. [配置概述](#配置概述)
2. [预编译配置](#预编译配置)
3. [信号配置](#信号配置)
4. [I-PDU 配置](#i-pdu-配置)
5. [I-PDU 组配置](#i-pdu-组配置)
6. [传输模式配置](#传输模式配置)
7. [错误处理配置](#错误处理配置)
8. [配置示例](#配置示例)

---

## 配置概述

COM 模块配置包括以下文件：

| 文件 | 用途 |
|------|------|
| `Com_Cfg.h` | 预编译配置宏 |
| `Com_Lcfg.c` | 链接时配置数据 |
| `Com_PBcfg.c` | 邮编译配置数据 (可选) |

---

## 预编译配置

### 基础配置 (Com_Cfg.h)

```c
#ifndef COM_CFG_H
#define COM_CFG_H

/*==================[开发错误检测]==========================*/
#define COM_DEV_ERROR_DETECT                STD_ON
#define COM_VERSION_INFO_API                STD_ON

/*==================[功能开关]=============================*/
#define COM_ENABLE_SIGNAL_GROUP_ARRAY_API   STD_ON
#define COM_ENABLE_MDT_FOR_CYCLIC_TRANSMISSION STD_OFF

/*==================[优化选项]=============================*/
#define COM_OPTIMIZE_FOR_SIZE               STD_OFF
#define COM_OPTIMIZE_FOR_SPEED              STD_ON

/*==================[资源限制]=============================*/
#define COM_MAX_SIGNALS                     128u
#define COM_MAX_SIGNAL_GROUPS               32u
#define COM_MAX_IPDUS                       64u
#define COM_MAX_IPDU_GROUPS                 16u
#define COM_MAX_IPDU_LENGTH                 64u
#define COM_MAX_SHADOW_BUFFER_SIZE          256u
#define COM_MAX_TX_REQUESTS                 32u

/*==================[主函数周期]=============================*/
#define COM_MAIN_FUNCTION_RX_PERIOD         10u  /* ms */
#define COM_MAIN_FUNCTION_TX_PERIOD         10u  /* ms */
#define COM_MAIN_FUNCTION_SIGNAL_PERIOD     10u  /* ms */

/*==================[符号名定义]=============================*/
/* I-PDU Groups */
#define ComConf_ComIPduGroup_EngineGroup    0u
#define ComConf_ComIPduGroup_ChassisGroup   1u
#define ComConf_ComIPduGroup_BodyGroup      2u

/* I-PDUs */
#define ComConf_ComIPdu_EngineData          0u
#define ComConf_ComIPdu_EngineStatus        1u
#define ComConf_ComIPdu_VehicleSpeed        2u
#define ComConf_ComIPdu_BodyControl         3u

/* Signals */
#define ComConf_ComSignal_EngineSpeed       0u
#define ComConf_ComSignal_CoolantTemp       1u
#define ComConf_ComSignal_ThrottlePosition  2u

/* Signal Groups */
#define ComConf_ComSignalGroup_EngineCoreInfo    0u

#endif /* COM_CFG_H */
```

### 配置选项说明

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| COM_DEV_ERROR_DETECT | STD_ON | 开发阶段建议启用，生产环境可关闭 |
| COM_VERSION_INFO_API | STD_ON | 启用版本信息 API |
| COM_OPTIMIZE_FOR_SPEED | STD_ON | 速度优兆，关闭则优先空间 |

---

## 信号配置

### 信号属性

```c
typedef struct {
    Com_SignalIdType SignalId;           /* 信号标识符 */
    uint8* DataPtr;                      /* 数据缓冲区指针 */
    uint16 BitPosition;                  /* 在 I-PDU 中的位置（位）*/
    uint8 BitSize;                       /* 信号长度（位）*/
    Com_SignalEndiannessType Endianness; /* 字节序 */
    Com_SignalTypeType SignalType;       /* 数据类型 */
    Com_TransferPropertyType TransferProperty; /* 传输属性 */
    void (*ComNotification)(void);       /* 通知回调 */
    uint32 Timeout;                      /* 超时时间 */
    const void* InitValue;               /* 初始值 */
} Com_SignalConfigType;
```

### 数据类型配置

| 类型 | 应用场景 | C 类型 |
|------|---------|--------|
| COM_BOOLEAN | 状态标志 | boolean |
| COM_UINT8 | 温度、百分比 | uint8 |
| COM_UINT16 | 转速、车速 | uint16 |
| COM_UINT32 | 里程、计数器 | uint32 |
| COM_FLOAT32 | 温度、压力 | float32 |

### 传输属性配置

| 属性 | 描述 | 使用场景 |
|------|------|---------|
| COM_PENDING | 不触发传输 | 周期性数据 |
| COM_TRIGGERED | 总是触发 | 紧急事件 |
| COM_TRIGGERED_ON_CHANGE | 变化时触发 | 节省带宽 |
| COM_TRIGGERED_WITHOUT_REPETITION | 触发无重复 | 一次性事件 |

---

## I-PDU 配置

### I-PDU 属性

```c
typedef struct {
    Com_IPduIdType IPduId;               /* I-PDU 标识符 */
    uint8* DataPtr;                      /* 数据缓冲区 */
    uint8 Length;                        /* 长度（字节）*/
    Com_IPduDirectionType Direction;     /* 方向 */
    Com_IPduType Type;                   /* 类型 */
    Com_IPduSignalProcessingType SignalProcessing; /* 信号处理方式 */
    Com_SignalIdType* SignalRefs;        /* 信号引用列表 */
    uint8 NumSignals;                    /* 信号数量 */
    Com_SignalGroupIdType* SignalGroupRefs; /* 信号组引用 */
    uint8 NumSignalGroups;               /* 信号组数量 */
    Com_IPduTxModeConfigType TxMode;     /* 传输模式 */
    Com_IpduGroupIdType* IpduGroupRefs;  /* 所属组 */
    uint8 NumIpduGroups;                 /* 组数量 */
    uint32 Timeout;                      /* 超时时间 */
    void (*ComIPduCallout)(PduIdType, PduInfoType*); /* 调用函数 */
    Com_TxConfirmationConfigType TxConfirmation; /* 发送确认 */
} Com_IPduConfigType;
```

### 方向配置

```c
typedef enum {
    COM_SEND,       /* 发送方向 */
    COM_RECEIVE     /* 接收方向 */
} Com_IPduDirectionType;
```

---

## I-PDU 组配置

### 组配置

```c
typedef struct {
    Com_IpduGroupIdType IpduGroupId;     /* 组标识符 */
    Com_IPduIdType* IPduRefs;            /* I-PDU 引用列表 */
    uint8 NumIPdus;                      /* I-PDU 数量 */
} Com_IPduGroupConfigType;
```

### 配置示例

```c
/* 定义 I-PDU 组 */
const Com_IPduGroupConfigType Com_IPduGroups[3] = {
    {   /* Engine Group */
        .IpduGroupId = ComConf_ComIPduGroup_EngineGroup,
        .IPduRefs = (Com_IPduIdType[]){ComConf_ComIPdu_EngineData, ComConf_ComIPdu_EngineStatus},
        .NumIPdus = 2
    },
    {   /* Chassis Group */
        .IpduGroupId = ComConf_ComIPduGroup_ChassisGroup,
        .IPduRefs = (Com_IPduIdType[]){ComConf_ComIPdu_VehicleSpeed},
        .NumIPdus = 1
    },
    {   /* Body Group */
        .IpduGroupId = ComConf_ComIPduGroup_BodyGroup,
        .IPduRefs = (Com_IPduIdType[]){ComConf_ComIPdu_BodyControl},
        .NumIPdus = 1
    }
};
```

---

## 传输模式配置

### 传输模式类型

```c
typedef enum {
    COM_MODE_DIRECT = 0,    /* 直接传输 */
    COM_MODE_PERIODIC,      /* 周期性传输 */
    COM_MODE_MIXED,         /* 混合模式 */
    COM_MODE_NONE           /* 不传输 */
} ComTxModeModeType;
```

### 传输模式配置

```c
typedef struct {
    ComTxModeModeType Mode;         /* 模式 */
    uint32 CycleTime;               /* 周期（毫秒）*/
    uint32 RepetitionPeriod;        /* 重复间隔（毫秒）*/
    uint8 NumRepetitions;           /* 重复次数 */
    uint32 TimeOffset;              /* 首次偏移（毫秒）*/
    boolean RepeatingEnabled;       /* 重复使能 */
} Com_TxModeType;
```

### 模式配置示例

```c
/* 周期性传输: 每 100ms 发送一次 */
Com_TxModeType periodicMode = {
    .Mode = COM_MODE_PERIODIC,
    .CycleTime = 100,           /* 100ms */
    .NumRepetitions = 0
};

/* 直接传输: 触发时发送，重复 3 次 */
Com_TxModeType directMode = {
    .Mode = COM_MODE_DIRECT,
    .RepetitionPeriod = 20,     /* 20ms 间隔 */
    .NumRepetitions = 3
};

/* 混合模式: 每 100ms + 触发重复 */
Com_TxModeType mixedMode = {
    .Mode = COM_MODE_MIXED,
    .CycleTime = 100,
    .RepetitionPeriod = 20,
    .NumRepetitions = 3
};
```

---

## 错误处理配置

### 溢出策略配置

```c
typedef enum {
    COM_TXQUEUE_REJECT_NEWEST = 0,  /* 拒绝新请求 */
    COM_TXQUEUE_DROP_OLDEST,        /* 丢弃最旧请求 */
    COM_TXQUEUE_DROP_NEWEST,        /* 丢弃新请求 */
    COM_TXQUEUE_REJECT_OLDEST       /* 拒绝旧请求 */
} Com_TxQueueOverflowStrategyType;

/* 默认策略 */
#define COM_DEFAULT_OVERFLOW_STRATEGY    COM_TXQUEUE_REJECT_NEWEST
```

### 错误处理配置

```c
#define COM_ERROR_HANDLING_ENABLE       STD_ON
#define COM_ERROR_STATISTICS_ENABLE     STD_ON
#define COM_MAX_ERROR_LOG_ENTRIES       16u
#define COM_ERROR_LOG_WRAP_MODE         STD_ON
```

---

## 配置示例

### 完整配置示例 (Com_Lcfg.c)

```c
#include "Com.h"
#include "Com_Cfg.h"

/*==================[数据缓冲区]===========================*/
static uint8 EngineData_Buffer[8];
static uint8 EngineStatus_Buffer[8];

static uint16 EngineSpeed_Value;
static uint8 CoolantTemp_Value;
static uint8 ThrottlePosition_Value;

/*==================[信号配置]===========================*/
const Com_SignalConfigType Com_Signals[] = {
    {   /* Engine Speed */
        .SignalId = ComConf_ComSignal_EngineSpeed,
        .DataPtr = (uint8*)&EngineSpeed_Value,
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_BIG_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL,
        .Timeout = 0,
        .InitValue = NULL
    },
    {   /* Coolant Temperature */
        .SignalId = ComConf_ComSignal_CoolantTemp,
        .DataPtr = &CoolantTemp_Value,
        .BitPosition = 16,
        .BitSize = 8,
        .Endianness = COM_BIG_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL,
        .Timeout = 0,
        .InitValue = NULL
    },
    {   /* Throttle Position */
        .SignalId = ComConf_ComSignal_ThrottlePosition,
        .DataPtr = &ThrottlePosition_Value,
        .BitPosition = 24,
        .BitSize = 8,
        .Endianness = COM_BIG_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL,
        .Timeout = 0,
        .InitValue = NULL
    }
};

/*==================[I-PDU 配置]===========================*/
static Com_SignalIdType EngineData_Signals[] = {
    ComConf_ComSignal_EngineSpeed,
    ComConf_ComSignal_CoolantTemp,
    ComConf_ComSignal_ThrottlePosition
};

const Com_IPduConfigType Com_IPdus[] = {
    {   /* Engine Data */
        .IPduId = ComConf_ComIPdu_EngineData,
        .DataPtr = EngineData_Buffer,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_DEFERRED,
        .SignalRefs = EngineData_Signals,
        .NumSignals = 3,
        .SignalGroupRefs = NULL,
        .NumSignalGroups = 0,
        .TxMode = {
            .TxModeTrue = {
                .Mode = COM_MODE_PERIODIC,
                .CycleTime = 100,       /* 100ms */
                .NumRepetitions = 0
            },
            .UseTmc = FALSE
        },
        .IpduGroupRefs = (Com_IpduGroupIdType[]){ComConf_ComIPduGroup_EngineGroup},
        .NumIpduGroups = 1,
        .Timeout = 0,
        .ComIPduCallout = NULL,
        .TxConfirmation = {
            .EnableConfirmation = FALSE
        }
    }
};

/*==================[I-PDU 组配置]========================*/
static Com_IPduIdType EngineGroup_IPdus[] = {
    ComConf_ComIPdu_EngineData
};

const Com_IPduGroupConfigType Com_IPduGroups[] = {
    {
        .IpduGroupId = ComConf_ComIPduGroup_EngineGroup,
        .IPduRefs = EngineGroup_IPdus,
        .NumIPdus = 1
    }
};

/*==================[全局配置]===========================*/
const Com_ConfigType ComConfig = {
    .Signals = Com_Signals,
    .NumSignals = sizeof(Com_Signals) / sizeof(Com_SignalConfigType),
    .SignalGroups = NULL,
    .NumSignalGroups = 0,
    .IPdus = Com_IPdus,
    .NumIPdus = sizeof(Com_IPdus) / sizeof(Com_IPduConfigType),
    .IPduGroups = Com_IPduGroups,
    .NumIPduGroups = sizeof(Com_IPduGroups) / sizeof(Com_IPduGroupConfigType)
};
```

---

## 相关文档

- [API 参考](./API_REFERENCE.md)
- [用户手册](./USER_MANUAL.md)
- [故障排除指南](./TROUBLESHOOTING.md)

---

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| v1.0 | 2024-04 | 初始版本 |
