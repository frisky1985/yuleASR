---
title: COM 模块 故障排除指南
description: "> **使用对象**: 软件开发工程师1、系统集成工程师1、支持工程师1"
sidebar_position: 21
---

# COM 模块 故障排除指南

> **版本**: v1.0  
> **使用对象**: 软件开发工程师1、系统集成工程师1、支持工程师1  
> **更新日期**: 2024年

***

## 目录

1. [累计问题索引](#累计问题索引)
2. [初始化问题](#初始化问题)
3. [发送问题](#发送问题)
4. [接收问题](#接收问题)
5. [传输模式问题](#传输模式问题)
6. [队列溢出问题](#队列溢出问题)
7. [超时监控问题](#超时监控问题)
8. [性能问题](#性能问题)
9. [调试工具与方法](#调试工具与方法)
10. [诊断检查清单](#诊断检查清单)

***

## 累计问题索引

按照症状快速定位问题：

| 症状 | 可能原因 | 参见章节 |
|******|*********|*********|
| 模块初始化失败 | 配置错误、内存不足 | [初始化问题](#初始化问题) |
| 信号发送失败 | 未初始化、队列满 | [发送问题](#发送问题) |
| 收不到数据 | I-PDU组未启动、路由错误 | [接收问题](#接收问题) |
| 数据不变化 | 传输属性配置错误 | [传输模式问题](#传输模式问题) |
| 定时发送失败 | 主函数未调用、周期配置错误 | [传输模式问题](#传输模式问题) |
| 队列溢出报错 | 负载过高、消费缓慢 | [队列溢出问题](#队列溢出问题) |
| 超时报错 | 通信中断、对端故障 | [超时监控问题](#超时监控问题) |
| 系统响应慢 | 主函数周期太长、负载过重 | [性能问题](#性能问题) |

***

## 初始化问题

### 问题 1.1: Com_Init 返回后状态仍为 UNINIT

**症状**:
```c
Com_Init(&ComConfig);
if (Com_GetStatus() == COM_UNINIT) {
    /* 状态未改变 */
}
```

**可能原因**:

1. **配置指针为 NULL**
```c
/* 错误 */
Com_Init(NULL);

/* 正确 */
Com_Init(&ComConfig);
```

2. **重复初始化**
```c
Com_Init(&ComConfig);
Com_Init(&ComConfig);  /* 第二次调用会失败，报 COM_E_ALREADY_INITIALIZED */
```

3. **配置结构损坏**
```c
/* 检查配置数据完整性 */
if (ComConfig.NumSignals == 0 || ComConfig.Signals == NULL) {
    /* 配置结构异常 */
}
```

**诊断步骤**:

```c
void Diagnose_InitFailure(void) {
    /* 步骤1: 检查配置指针 */
    if (&ComConfig == NULL) {
        LogError("ComConfig is NULL");
        return;
    }
    
    /* 步骤2: 检查配置数据 */
    printf("Config: Signals=%d, IPdus=%d, Groups=%d\n",
           ComConfig.NumSignals,
           ComConfig.NumIPdus,
           ComConfig.NumIPduGroups);
    
    /* 步骤3: 检查状态转换 */
    printf("Before Init: Status=%d\n", Com_GetStatus());
    Com_Init(&ComConfig);
    printf("After Init: Status=%d\n", Com_GetStatus());
}
```

**解决方法**:
1. 确保配置指针有效
2. 确保只调用一次 Com_Init
3. 验证配置数据完整性
4. 检查 Det 错误日志

***

### 问题 1.2: I-PDU 组启动失败

**症状**:
```c
Com_IpduGroupStart(ComConf_ComIPduGroup_EngineGroup, TRUE);
/* 信号发送仍失败 */
```

**诊断步骤**:

```c
void Diagnose_IpduGroupIssue(void) {
    /* 步骤1: 确认模块已初始化 */
    if (Com_GetStatus() != COM_READY) {
        LogError("COM not initialized");
        return;
    }
    
    /* 步骤2: 检查 I-PDU ID */
    printf("Group ID: %d\n", ComConf_ComIPduGroup_EngineGroup);
    
    /* 步骤3: 确认配置中存在该组 */
    if (ComConf_ComIPduGroup_EngineGroup >= ComConfig.NumIPduGroups) {
        LogError("Invalid group ID");
    }
    
    /* 步骤4: 检查组包含的 I-PDU */
    const Com_IPduGroupConfigType* group = &ComConfig.IPduGroups[ComConf_ComIPduGroup_EngineGroup];
    printf("Group contains %d IPdus\n", group->NumIPdus);
}
```

**解决方法**:
1. 确保在 Com_Init 之后启动组
2. 验证 I-PDU Group ID 有效
3. 确认组包含正确的 I-PDU
4. 检查配置工具生成的配置

***

## 发送问题

### 问题 2.1: Com_SendSignal 返回 E_NOT_OK

**症状**:
```c
uint8 result = Com_SendSignal(ComConf_ComSignal_EngineSpeed, &speed);
if (result != E_OK) {
    /* 发送失败 */
}
```

**可能原因和解决方法**:

#### 原因 A: 模块未初始化

```c
/* 检查方法 */
if (Com_GetStatus() != COM_READY) {
    LogError("COM not initialized before SendSignal");
    /* 解决: 先调用 Com_Init */
}
```

#### 原因 B: 信号 ID 无效

```c
/* 检查方法 */
if (SignalId >= COM_MAX_SIGNALS) {
    LogError("Invalid signal ID: %d", SignalId);
    /* 解决: 使用正确的信号 ID */
}

/* 正确用法 */
Com_SendSignal(ComConf_ComSignal_EngineSpeed, &speed);  /* 使用 Com_Cfg.h 中定义的宏 */
```

#### 原因 C: 数据指针为 NULL

```c
/* 错误 */
Com_SendSignal(ComConf_ComSignal_EngineSpeed, NULL);

/* 正确 */
if (SignalDataPtr != NULL) {
    Com_SendSignal(ComConf_ComSignal_EngineSpeed, SignalDataPtr);
}
```

#### 原因 D: I-PDU 组未启动

```c
/* 诊断 */
void Diagnose_SendFailure(Com_SignalIdType signalId) {
    /* 查找信号所在的 I-PDU */
    for (uint16 i = 0; i < ComConfig.NumIPdus; i++) {
        for (uint8 j = 0; j < ComConfig.IPdus[i].NumSignals; j++) {
            if (ComConfig.IPdus[i].SignalRefs[j] == signalId) {
                printf("Signal belongs to IPdu %d\n", i);
                /* 检查该 IPdu 的组是否启动 */
            }
        }
    }
}
```

**完整诊断代码**:

```c
Std_ReturnType Diagnose_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr) {
    /* 检查1: 模块状态 */
    if (Com_GetStatus() != COM_READY) {
        LogError("[Send] Module not initialized");
        return E_NOT_OK;
    }
    
    /* 检查2: 信号 ID */
    if (SignalId >= COM_MAX_SIGNALS) {
        LogError("[Send] Invalid signal ID: %d", SignalId);
        return E_NOT_OK;
    }
    
    /* 检查3: 数据指针 */
    if (SignalDataPtr == NULL) {
        LogError("[Send] Null data pointer");
        return E_NOT_OK;
    }
    
    /* 检查4: 信号配置 */
    if (SignalId >= ComConfig.NumSignals) {
        LogError("[Send] Signal not configured: %d", SignalId);
        return E_NOT_OK;
    }
    
    LogInfo("[Send] All checks passed for signal %d", SignalId);
    return E_OK;
}
```

***

### 问题 2.2: 数据发送但对端收不到

**症状**: 调用 Com_SendSignal 成功，但对端未收到数据。

**诊断步骤**:

#### 步骤 1: 确认主函数调用

```c
/* 确保 Com_MainFunctionTx 正确调用 */
TASK(ComTxTask) {
    /* 正确: 调用主函数 */
    Com_MainFunctionTx();
    
    TerminateTask();
}

/* 验证周期 */
/* 如果传输模式周期为 100ms，则 Com_MainFunctionTx 应每 10ms 调用一次 */
```

#### 步骤 2: 检查传输属性

```c
/* 如果传输属性为 PENDING，数据不会立即发送 */
/* 检查信号配置 */
const Com_SignalConfigType* sigConfig = &ComConfig.Signals[signalId];
switch (sigConfig->TransferProperty) {
    case COM_PENDING:
        printf("Signal is PENDING - data only sent on periodic trigger\n");
        break;
    case COM_TRIGGERED:
    case COM_TRIGGERED_ON_CHANGE:
        printf("Signal will trigger transmission\n");
        break;
}
```

#### 步骤 3: 检查下层通信

```c
/* 确保 PduR_ComTransmit 被调用 */
/* 在 PduR 中添加日志 */
void PduR_ComTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    LogInfo("PduR_ComTransmit called for PduId=%d, length=%d", 
            TxPduId, PduInfoPtr->SduLength);
    /* ... */
}
```

#### 步骤 4: 确认发送确认

```c
/* 检查发送确认回调 */
void PduR_ComTxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    if (result == E_OK) {
        LogInfo("Tx confirmation success for PduId=%d", TxPduId);
    } else {
        LogError("Tx confirmation failed for PduId=%d", TxPduId);
    }
}
```

***

## 接收问题

### 问题 3.1: 收不到数据

**症状**: Com_ReceiveSignal 返回成功，但数据始终为初始值。

**诊断步骤**:

#### 步骤 1: 验证数据发送方

```c
/* 在发送端确认数据正确发出 */
void SendDiagnostics(void) {
    uint16 testData = 0x1234;
    uint8 result = Com_SendSignal(ComConf_ComSignal_EngineSpeed, &testData);
    LogInfo("Send result=%d, data=0x%04X", result, testData);
}
```

#### 步骤 2: 检查 Rx 主函数

```c
/* 确保 Com_MainFunctionRx 正确调用 */
TASK(ComRxTask) {
    /* 调用主函数处理接收 */
    Com_MainFunctionRx();
    
    TerminateTask();
}
```

#### 步骤 3: 验证接收回调

```c
/* 在 PduR_ComRxIndication 中添加日志 */
void PduR_ComRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    LogInfo("RxIndication: PduId=%d, length=%d", RxPduId, PduInfoPtr->SduLength);
    
    /* 打印接收到的数据 */
    for (uint8 i = 0; i < PduInfoPtr->SduLength && i < 8; i++) {
        printf("Data[%d]=0x%02X ", i, PduInfoPtr->SduDataPtr[i]);
    }
    printf("\n");
}
```

#### 步骤 4: 检查信号方向

```c
/* 确认信号配置为接收方向 */
const Com_IPduConfigType* ipdu = &ComConfig.IPdus[ipduId];
if (ipdu->Direction != COM_RECEIVE) {
    LogError("IPdu direction is not RECEIVE");
}
```

***

### 问题 3.2: 数据解析错误

**症状**: 收到数据但值不正确。

**可能原因**:

#### 原因 A: 字节序不匹配

```c
/* 问题: 发送端使用小端序，接收端配置为大端序 */

/* 诊断方法 */
void CheckEndianness(void) {
    uint16 testValue = 0x1234;
    uint8* ptr = (uint8*)&testValue;
    
    if (ptr[0] == 0x34) {
        LogInfo("System is LITTLE_ENDIAN");
    } else {
        LogInfo("System is BIG_ENDIAN");
    }
    
    /* 检查配置 */
    printf("Signal configured endianness: %d\n", 
           ComConfig.Signals[signalId].Endianness);
}
```

#### 原因 B: 位置偏移错误

```c
/* 检查信号在 I-PDU 中的位置 */
void CheckSignalPosition(Com_SignalIdType signalId) {
    const Com_SignalConfigType* sig = &ComConfig.Signals[signalId];
    printf("Signal %d: BitPosition=%d, BitSize=%d\n",
           signalId, sig->BitPosition, sig->BitSize);
    
    /* 验证是否在 I-PDU 范围内 */
    if (sig->BitPosition + sig->BitSize > (COM_MAX_IPDU_LENGTH * 8)) {
        LogError("Signal exceeds IPdu size!");
    }
}
```

#### 原因 C: 数据类型不匹配

```c
/* 确保使用正确的 C 类型 */
/* 错误 */
sint8 temp;  /* 应使用 uint8 */
Com_ReceiveSignal(ComConf_ComSignal_CoolantTemp, &temp);

/* 正确 */
uint8 temp;
Com_ReceiveSignal(ComConf_ComSignal_CoolantTemp, &temp);
```

***

## 传输模式问题

### 问题 4.1: 定时传输不工作

**症状**: 配置为周期性传输但数据不定时发送。

**诊断步骤**:

```c
void Diagnose_PeriodicTx(void) {
    /* 步骤1: 检查传输模式配置 */
    const Com_IPduConfigType* ipdu = &ComConfig.IPdus[ComConf_ComIPdu_EngineData];
    printf("Tx Mode: %d\n", ipdu->TxMode.TxModeTrue.Mode);
    printf("CycleTime: %d ms\n", ipdu->TxMode.TxModeTrue.CycleTime);
    
    /* 步骤2: 验证周期时间 */
    if (ipdu->TxMode.TxModeTrue.CycleTime == 0) {
        LogError("CycleTime is 0 - periodic transmission disabled");
    }
    
    /* 步骤3: 检查主函数周期 */
    printf("MainFunctionTx period: %d ms\n", COM_MAIN_FUNCTION_TX_PERIOD);
    
    /* 步骤4: CycleTime 应为主函数周期的整数倍 */
    if (ipdu->TxMode.TxModeTrue.CycleTime % COM_MAIN_FUNCTION_TX_PERIOD != 0) {
        LogWarning("CycleTime is not multiple of MainFunction period");
    }
}
```

**解决方法**:
1. 确保 CycleTime > 0
2. 确保 CycleTime 是主函数周期的整数倍
3. 确保主函数正确调用

***

### 问题 4.2: 事件触发不工作

**症状**: 使用 TRIGGERED 属性但数据不发送。

**诊断步骤**:

```c
void Diagnose_TriggeredTx(void) {
    /* 检查信号传输属性 */
    const Com_SignalConfigType* sig = &ComConfig.Signals[ComConf_ComSignal_EngineSpeed];
    printf("Transfer Property: %d\n", sig->TransferProperty);
    
    switch (sig->TransferProperty) {
        case COM_PENDING:
            LogInfo("Property is PENDING - won't trigger transmission");
            break;
        case COM_TRIGGERED:
            LogInfo("Property is TRIGGERED - should trigger");
            break;
        case COM_TRIGGERED_ON_CHANGE:
            LogInfo("Property is TRIGGERED_ON_CHANGE - only on data change");
            break;
    }
    
    /* 检查传输模式 */
    const Com_IPduConfigType* ipdu = &ComConfig.IPdus[ComConf_ComIPdu_EngineData];
    if (ipdu->TxMode.TxModeTrue.Mode == COM_MODE_NONE) {
        LogError("Tx Mode is NONE - transmission disabled");
    }
}
```

***

## 队列溢出问题

### 问题 5.1: 频繁报 Tx 队列溢出

**症状**: DET 报告 COM_E_TX_QUEUE_OVERFLOW 错误。

**诊断步骤**:

```c
void Diagnose_QueueOverflow(void) {
    Com_TxQueueStatusType status;
    Com_GlobalErrorStatsType stats;
    
    /* 步骤1: 获取队列状态 */
    if (Com_Eh_GetTxQueueStatus(&status) == E_OK) {
        printf("Queue Fill Level: %d/%d\n", status.FillLevel, status.MaxFillLevel);
        printf("Is Full: %s\n", status.IsFull ? "Yes" : "No");
    }
    
    /* 步骤2: 获取错误统计 */
    if (Com_Eh_GetErrorStats(&stats) == E_OK) {
        printf("Overflow Count: %lu\n", stats.TxQueueOverflowCount);
        printf("Reject Count: %lu\n", stats.TxQueueRejectCount);
        printf("Max Fill Level: %d\n", stats.MaxQueueFillLevel);
    }
    
    /* 步骤3: 检查发送频率 */
    /* 如果发送频率高于处理能力，会导致队列溢出 */
    printf("Queue Size: %d\n", COM_MAX_TX_REQUESTS);
    printf("MainFunction Period: %d ms\n", COM_MAIN_FUNCTION_TX_PERIOD);
}
```

**解决方法**:

#### 方案 A: 增大队列大小

```c
/* Com_Cfg.h */
#define COM_MAX_TX_REQUESTS    64u  /* 原先为 32 */
```

#### 方案 B: 调整溢出策略

```c
/* 选择适合的溢出策略 */
#define COM_DEFAULT_OVERFLOW_STRATEGY    COM_TXQUEUE_DROP_OLDEST

/* 策略选择**:
 * - COM_TXQUEUE_REJECT_NEWEST: 拒绝新请求（保留旧数据）
 * - COM_TXQUEUE_DROP_OLDEST: 丢弃最旧数据（保持最新）
 * - COM_TXQUEUE_DROP_NEWEST: 丢弃新数据
 * - COM_TXQUEUE_REJECT_OLDEST: 拒绝旧数据
 */
```

#### 方案 C: 优化发送逻辑

```c
/* 减少不必要的发送 */
void OptimizedSend(void) {
    static uint16 lastSpeed = 0;
    uint16 currentSpeed = ReadEngineSpeed();
    
    /* 只有数据变化时才发送 */
    if (currentSpeed != lastSpeed) {
        Com_SendSignal(ComConf_ComSignal_EngineSpeed, &currentSpeed);
        lastSpeed = currentSpeed;
    }
}
```

***

### 问题 5.2: 队列处理卡顿

**症状**: 队列占用率高但数据不发送。

**可能原因**:

1. **主函数未调用**
2. **下层通信故障**
3. **发送确认丢失**

**诊断步骤**:

```c
void Diagnose_QueueStall(void) {
    /* 检查1: 主函数调用 */
    static uint32 lastProcessed = 0;
    uint32 currentCount = Com_GlobalErrorStats.TxQueueProcessCount;
    
    if (currentCount == lastProcessed) {
        LogError("MainFunctionTx not being called!");
    }
    lastProcessed = currentCount;
    
    /* 检查2: 发送确认 */
    printf("TxConfirmation received: %lu\n", Com_GlobalTxConfirmationCount);
    printf("TxTimeout occurred: %lu\n", Com_GlobalErrorStats.TxTimeoutCount);
}
```

***

## 超时监控问题

### 问题 6.1: 频繁超时报错

**症状**: 接收超时监控频繁触发。

**可能原因**:

1. **对端发送周期长于超时时间**
2. **通信中断**
3. **消息丢失**

**诊断步骤**:

```c
void Diagnose_Timeout(void) {
    /* 检查超时配置 */
    const Com_IPduConfigType* ipdu = &ComConfig.IPdus[ComConf_ComIPdu_EngineData];
    printf("Configured Timeout: %d ms\n", ipdu->Timeout);
    
    /* 检查对端发送周期 */
    printf("Expected Rx period: %d ms\n", ipdu->TxMode.TxModeTrue.CycleTime);
    
    /* 检查是否 Timeout < 期望周期 */
    if (ipdu->Timeout <= ipdu->TxMode.TxModeTrue.CycleTime) {
        LogWarning("Timeout may be too short");
    }
    
    /* 检查 Rx 确认 */
    printf("RxIndication count: %lu\n", Com_RxIndicationCount);
    printf("Timeout count: %lu\n", Com_GlobalErrorStats.RxTimeoutCount);
}
```

**解决方法**:

```c
/* 调整超时时间 */
/* 建议: Timeout >= 2 * 期望接收周期 */
#define COM_DEFAULT_RX_TIMEOUT    200u  /* 如果周期为 100ms */
```

***

## 性能问题

### 问题 7.1: CPU 占用率高

**症状**: Com_MainFunctionTx/Rx 执行时间长，CPU 负载高。

**诊断步骤**:

```c
void Profile_Performance(void) {
    uint32 startTime, endTime;
    
    /* 测量 MainFunctionTx 执行时间 */
    startTime = GetMicroseconds();
    Com_MainFunctionTx();
    endTime = GetMicroseconds();
    printf("MainFunctionTx: %d us\n", endTime - startTime);
    
    /* 测量 MainFunctionRx 执行时间 */
    startTime = GetMicroseconds();
    Com_MainFunctionRx();
    endTime = GetMicroseconds();
    printf("MainFunctionRx: %d us\n", endTime - startTime);
}
```

**优化建议**:

1. **减少主函数调用频率**
```c
/* 如果数据变化不频繁，可以降低调用频率 */
#define COM_MAIN_FUNCTION_TX_PERIOD    20u  /* 原先为 10ms */
```

2. **优化传输模式**
```c
/* 使用 TRIGGERED_ON_CHANGE 而非 TRIGGERED */
/* 减少不必要的传输 */
```

3. **合理分配 I-PDU 组**
```c
/* 按照优先级分组 */
/* 高优先级组: 10ms 周期 */
/* 低优先级组: 100ms 周期 */
```

***

## 调试工具与方法

### 内存调试

```c
/**
 * @brief 打印 I-PDU 内存内容
 */
void Dump_IpduMemory(Com_IPduIdType pduId) {
    const Com_IPduConfigType* ipdu = &ComConfig.IPdus[pduId];
    printf("IPdu %d Memory Dump:\n", pduId);
    
    for (uint8 i = 0; i < ipdu->Length; i++) {
        printf("%02X ", ipdu->DataPtr[i]);
        if ((i + 1) % 8 == 0) printf("\n");
    }
    printf("\n");
}

/**
 * @brief 打印所有信号值
 */
void Dump_AllSignals(void) {
    printf("=== Signal Values ===\n");
    
    for (uint16 i = 0; i < ComConfig.NumSignals; i++) {
        const Com_SignalConfigType* sig = &ComConfig.Signals[i];
        printf("Signal %d (%s): ", i, GetSignalName(i));
        
        switch (sig->SignalType) {
            case COM_UINT8:
                printf("%u\n", *(uint8*)sig->DataPtr);
                break;
            case COM_UINT16:
                printf("%u\n", *(uint16*)sig->DataPtr);
                break;
            case COM_UINT32:
                printf("%lu\n", *(uint32*)sig->DataPtr);
                break;
            default:
                printf("(unsupported type)\n");
                break;
        }
    }
}
```

### 日志跟踪

```c
/**
 * @brief 详细日志记录
 */
#define COM_LOG_LEVEL_NONE     0
#define COM_LOG_LEVEL_ERROR    1
#define COM_LOG_LEVEL_WARNING  2
#define COM_LOG_LEVEL_INFO     3
#define COM_LOG_LEVEL_DEBUG    4

#ifndef COM_LOG_LEVEL
#define COM_LOG_LEVEL    COM_LOG_LEVEL_INFO
#endif

#define COM_LOG_ERROR(...)    if (COM_LOG_LEVEL >= COM_LOG_LEVEL_ERROR) printf(__VA_ARGS__)
#define COM_LOG_WARNING(...)  if (COM_LOG_LEVEL >= COM_LOG_LEVEL_WARNING) printf(__VA_ARGS__)
#define COM_LOG_INFO(...)     if (COM_LOG_LEVEL >= COM_LOG_LEVEL_INFO) printf(__VA_ARGS__)
#define COM_LOG_DEBUG(...)    if (COM_LOG_LEVEL >= COM_LOG_LEVEL_DEBUG) printf(__VA_ARGS__)
```

### 轮询跟踪

```c
/**
 * @brief 跟踪发送过程
 */
void Trace_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr) {
    COM_LOG_DEBUG("[SEND] Signal=%d\n", SignalId);
    
    /* 步骤1: 复制到缓冲区 */
    COM_LOG_DEBUG("[SEND] Step1: Copy to buffer\n");
    
    /* 步骤2: 检查触发条件 */
    COM_LOG_DEBUG("[SEND] Step2: Check trigger\n");
    
    /* 步骤3: 添加到队列 */
    COM_LOG_DEBUG("[SEND] Step3: Add to queue\n");
}
```

***

## 诊断检查清单

### 基础检查清单

- [ ] COM 模块已正确初始化
- [ ] I-PDU 组已正确启动
- [ ] 主函数按正确周期调用
- [ ] 信号 ID 配置正确
- [ ] 数据指针有效
- [ ] 传输属性配置正确
- [ ] 传输模式配置正确
- [ ] PduR 路由配置正确
- [ ] 下层驱动正常工作

### 高级检查清单

- [ ] 字节序配置一致
- [ ] 位置偏移计算正确
- [ ] 数据类型匹配
- [ ] 队列大小足够
- [ ] 超时时间配置合理
- [ ] 主函数执行时间合适
- [ ] 错误处理策略正确
- [ ] 统计数据正常

### 常用调试命令

```bash
# 检查模块状态
grep "COM_UNINIT\|COM_READY" debug.log

# 检查错误计数
grep "COM_E_" debug.log | wc -l

# 检查发送确认
grep "TxConfirmation" debug.log | tail -20

# 检查接收指示
grep "RxIndication" debug.log | tail -20
```

***

## 相关文档

- [API 参考](../api/com-api.md)
- [用户手册](com-user-manual.md)

***

## 版本历史

| 版本 | 日期 | 描述 |
|******|******|******|
| v1.0 | 2024-04 | 初始版本 |
