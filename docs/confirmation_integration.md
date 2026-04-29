# COM 传输确认处理和重传逻辑集成说明

## 概述

本文档描述了 T011 传输确认处理和重传逻辑的集成说明。该实现符合 AUTOSAR COM 规范 SWS_Com_00450 和 SWS_Com_00455。

## 功能特性

### 1. 传输确认处理 (Com_TxConfirmation)
- **回调函数**: `Com_TxConfirmation()` 由 PduR 调用以确认传输结果
- **状态管理**: 跟踪传输状态 (IDLE -> PENDING -> CONFIRMED/ERROR)
- **回调支持**: 支持 `ComTxConfirmation` 和 `ComTxErrorNotification` 回调

### 2. 传输状态机
```
COM_TX_IDLE -> COM_TX_PENDING -> COM_TX_CONFIRMED
                        |
                        +-------> COM_TX_ERROR
                        |
                        +-------> COM_TX_RETRY_PENDING
```

### 3. 超时检测 (ComTxTimeout)
- 可配置的传输超时时间
- 超时通知回调 `ComTxTimeoutNotification`
- 自动重传机制触发

### 4. 重传机制 (ComTxRetries)
- 可配置的最大重试次数
- 重传队列管理
- 可配置的重试延迟时间

### 5. 传输模式切换处理
- 支持在确认挂起期间切换传输模式
- 切换到 NONE 模式时自动取消确认监控

## 文件清单

### 新增文件
1. `include/autosar/classic/com/Com_Confirmation.h` - 确认模块公共接口
2. `src/autosar/classic/com/Com_Confirmation.c` - 确认模块实现
3. `tests/unit/com/test_com_confirmation.c` - 单元测试

### 修改文件
1. `include/autosar/classic/com/Com_Types.h` - 添加确认配置类型
2. `include/autosar/classic/com/Com_Cfg.h` - 添加确认配置常量
3. `include/autosar/classic/com/Com.h` - 添加确认API声明
4. `src/autosar/classic/com/Com_Private.h` - 添加运行时数据类型
5. `src/autosar/classic/com/Com.c` - 初始化确认模块
6. `src/autosar/classic/com/Com_Main.c` - 集成超时和重传处理

## 配置说明

### 1. 传输确认配置结构
```c
typedef struct {
    boolean EnableConfirmation;         /* 启用传输确认 */
    uint32 TxTimeout;                   /* 传输超时时间 (ms) */
    uint8 MaxRetries;                   /* 最大重试次数 */
    void (*ComTxConfirmation)(void);    /* 成功回调 */
    void (*ComTxErrorNotification)(void);   /* 错误回调 */
    void (*ComTxTimeoutNotification)(void); /* 超时回调 */
} Com_TxConfirmationConfigType;
```

### 2. IPdu配置示例
```c
const Com_IPduConfigType MyIPduConfig = {
    .IPduId = 0,
    .DataPtr = MyBuffer,
    .Length = 8,
    .Direction = COM_SEND,
    .TxMode = {
        .Mode = COM_DIRECT,
        .Period = 0,
        .RepetitionPeriod = 0,
        .NumRepetitions = 0,
        .TimeOffset = 0
    },
    .TxConfirmation = {
        .EnableConfirmation = TRUE,
        .TxTimeout = 100,           /* 100ms 超时 */
        .MaxRetries = 3,            /* 最多重试3次 */
        .ComTxConfirmation = MyTxConfirmationCallback,
        .ComTxErrorNotification = MyTxErrorCallback,
        .ComTxTimeoutNotification = MyTxTimeoutCallback
    }
};
```

### 3. 配置常量
```c
#define COM_MAX_RETRY_QUEUE_SIZE    16u  /* 最大重传队列大小 */
#define COM_DEFAULT_TX_TIMEOUT      100u /* 默认传输超时 (ms) */
#define COM_DEFAULT_MAX_RETRIES     3u   /* 默认最大重试次数 */
#define COM_RETRY_DELAY_MS          10u  /* 重试延迟 (ms) */
```

## API 接口

### 回调函数 (PduR -> COM)
```c
void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
```

### 内部API
```c
/* 初始化 */
void Com_InitConfirmation(void);
void Com_DeInitConfirmation(void);

/* 确认管理 */
Std_ReturnType Com_StartTxConfirmation(Com_IPduIdType PduId);
void Com_CancelTxConfirmation(Com_IPduIdType PduId);
void Com_HandleTxTimeout(Com_IPduIdType PduId);

/* 状态查询 */
Com_TxStatusType Com_GetTxStatus(Com_IPduIdType PduId);
Com_TxResultType Com_GetTxResult(Com_IPduIdType PduId);

/* 重传机制 */
Std_ReturnType Com_AddToRetryQueue(Com_IPduIdType PduId, uint8 RetryCount);
void Com_RemoveFromRetryQueue(Com_IPduIdType PduId);
void Com_ProcessRetryQueue(void);
boolean Com_IsInRetryQueue(Com_IPduIdType PduId);
uint8 Com_GetRemainingRetries(Com_IPduIdType PduId);
Std_ReturnType Com_PerformRetry(Com_IPduIdType PduId);

/* 超时处理 */
void Com_ProcessTxTimeouts(void);
void Com_ResetTxTimeout(Com_IPduIdType PduId);
boolean Com_IsTxTimedOut(Com_IPduIdType PduId);

/* 模式切换 */
void Com_HandleModeSwitchConfirmation(Com_IPduIdType PduId, 
                                      Com_TransferModeType OldMode,
                                      Com_TransferModeType NewMode);
boolean Com_CanSwitchModeDuringPending(Com_IPduIdType PduId);
```

## 集成步骤

### 1. 配置确认参数
在 IPdu 配置中启用确认并设置参数：
```c
.TxConfirmation = {
    .EnableConfirmation = TRUE,
    .TxTimeout = 100,
    .MaxRetries = 3,
    .ComTxConfirmation = MySuccessCallback,
    .ComTxErrorNotification = MyErrorCallback,
    .ComTxTimeoutNotification = MyTimeoutCallback
}
```

### 2. 确保 PduR 接口正确调用
PduR 在传输完成后调用：
```c
PduR_ComTxConfirmation(TxPduId, result);  /* result = E_OK 或 E_NOT_OK */
```

### 3. 主循环集成
`Com_MainFunctionTx()` 已自动集成以下功能：
- 重传队列处理 (`Com_ProcessRetryQueue()`)
- 超时监控 (`Com_ProcessTxTimeouts()`)

### 4. 传输触发
在 `Com_TransmitIPdu()` 中自动启动确认监控：
```c
Std_ReturnType result = PduR_IfTransmit(ipduConfig->IPduId, &pduInfo);
if (result == E_OK && ipduConfig->TxConfirmation.EnableConfirmation) {
    Com_StartTxConfirmation(PduId);
}
```

## 状态机说明

### 传输状态 (Com_TxStatusType)
| 状态 | 说明 |
|------|------|
| COM_TX_IDLE | 无传输挂起 |
| COM_TX_PENDING | 传输进行中，等待确认 |
| COM_TX_CONFIRMED | 传输已确认成功 |
| COM_TX_ERROR | 传输错误 |
| COM_TX_RETRY_PENDING | 重试挂起中 |

### 传输结果 (Com_TxResultType)
| 结果 | 说明 |
|------|------|
| COM_TX_RES_NONE | 无结果 |
| COM_TX_RES_OK | 传输成功 |
| COM_TX_RES_TIMEOUT | 传输超时 |
| COM_TX_RES_NOT_OK | 传输失败 |
| COM_TX_RES_CANCELLED | 传输取消 |

## 错误处理

### 错误码
```c
#define COM_E_CONFIRMATION_TIMEOUT      0x30u
#define COM_E_MAX_RETRIES_EXCEEDED      0x31u
#define COM_E_RETRY_QUEUE_FULL          0x32u
#define COM_E_INVALID_RETRY             0x33u
```

### 错误场景处理
1. **超时**: 调用 `ComTxTimeoutNotification`，触发重传
2. **传输失败**: 调用 `ComTxErrorNotification`，根据重试次数决定是否重传
3. **重传队列满**: 报告错误 `COM_E_RETRY_QUEUE_FULL`
4. **超过最大重试次数**: 报告错误 `COM_E_MAX_RETRIES_EXCEEDED`

## 测试覆盖

### 单元测试用例
1. 确认回调测试（成功/失败/禁用/无效PDU）
2. 超时检测测试
3. 重传队列管理测试
4. 状态机转换测试
5. 传输模式切换测试
6. 最大重试次数测试

### 运行测试
```bash
cd /home/admin/eth-dds-integration
make test_com_confirmation
```

## 性能考虑

1. **内存使用**:
   - 每个 IPdu 增加 ~24 字节运行时数据
   - 重传队列固定大小: `COM_MAX_RETRY_QUEUE_SIZE * sizeof(Com_RetryQueueEntryType)`

2. **CPU 负载**:
   - `Com_MainFunctionTx()` 增加重传队列处理和超时监控
   - 建议在 10ms 周期调用

3. **栈使用**:
   - 回调函数使用应用层栈空间
   - 注意递归调用深度（重传时）

## 限制和注意事项

1. 重传队列大小固定，需要合理配置 `COM_MAX_RETRY_QUEUE_SIZE`
2. 超时时间单位为毫秒，基于 `Com_MainFunctionTx()` 调用周期
3. 重传延迟时间单位为毫秒
4. 模式切换为 NONE 时会取消确认监控，但已发送的数据可能仍在传输中
5. 不支持 TP (Transport Protocol) 的确认处理（仅支持 IF 接口）

## 兼容性

- AUTOSAR 版本: 4.4.0
- 支持平台: Classic Platform
- 相关模块: PduR, Det (可选)

## 版本历史

| 版本 | 日期 | 修改内容 |
|------|------|----------|
| 1.0.0 | 2024-04-28 | 初始版本 - 实现 T011 |
