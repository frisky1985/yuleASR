# COM 死线监控 (Deadline Monitoring) 集成说明

## 概述

本文档描述 AUTOSAR COM 模块的死线监控功能 (T012) 的实现和集成方法。

## 参考规范

- **AUTOSAR SWS_Com_00500**: Deadline Monitoring
- **ComIPduRxTimeout**: 接收超时配置参数
- **ComErrorHook**: 错误回调机制
- **ComIPduRxDefaultValue**: 接收默认值替代

## 功能特性

### 1. 接收超时计时器管理

```c
Com_Dm_StartTimer(PduId, Timeout);   // 启动/重新启动计时器
Com_Dm_StopTimer(PduId);              // 停止计时器
Com_Dm_ProcessTimers();               // 在 Com_MainFunctionRx 中处理
```

### 2. 超时检测逻辑

- 计时器在每个 `Com_MainFunctionRx` 调用中递减
- 当计时器达到零时，检测到超时
- 状态机从 `COM_DM_STATE_RUNNING` 转换到 `COM_DM_STATE_EXPIRED`

### 3. ErrorHook 调用集成

```c
void ComErrorHook(Com_IPduIdType PduId)
{
    // 用户自定义的错误处理
    // 例如：记录日志、设置故障码
}
```

### 4. 默认值替代机制

当超时发生时，可以配置替换 I-PDU 缓冲区为预定义的默认值：

```c
const uint8 DefaultValue[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x00, 0x00, 0x00};

Com_DmRxConfigType dmConfig = {
    .ComIPduRxTimeout = 100,           // 100ms 超时
    .ComIPduRxDefaultValue = DefaultValue,
    .DefaultValueLength = 4,           // 只替换前4个字节
    .TimeoutAction = COM_DM_ACTION_BOTH,  // 调用Hook + 替换默认值
    .ComErrorHook = MyErrorHook,
    .EnableDeadlineMonitoring = TRUE
};
```

## 代码结构

### 新增文件

```
src/autosar/classic/com/
├── Com_DeadlineMon.h    # 死线监控头文件
├── Com_DeadlineMon.c    # 死线监控实现
```

### 修改的文件

1. **Com_Types.h**: 添加 `Com_DmRxConfigType`, `Com_DmStateType`, `Com_DmActionType`
2. **Com_Private.h**: 包含 `Com_DeadlineMon.h`
3. **Com_Main.c**: 集成计时器处理到 `Com_MainFunctionRx`
4. **Com.c**: 在 `Com_Init` 中初始化死线监控

## 集成步骤

### 步骤 1: 配置超时参数

在 Com_Cfg.h 或 ARXML 配置中设置：

```c
#define COM_DM_DEFAULT_RX_TIMEOUT    100u  // 默认接收超时(ms)
```

### 步骤 2: 初始化

`Com_Init()` 自动初始化死线监控：

```c
void Com_Init(const Com_ConfigType* config)
{
    // ... 其他初始化 ...
    
    /* T012: Initialize deadline monitoring (ASIL-D) */
    Com_Dm_Init();
    
    Com_GlobalState.Status = COM_READY;
}
```

### 步骤 3: 在 Com_MainFunctionRx 中处理

```c
void Com_MainFunctionRx(void)
{
    /* T012: Process deadline monitoring timers (ASIL-D) */
    COM_DM_PROCESS_IN_MAINFUNCTIONRX();
    
    /* ... 其他处理 ... */
}
```

### 步骤 4: 在 PduR_ComRxIndication 中重置计时器

```c
void PduR_ComRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    /* ... 数据复制 ... */
    
    /* T012: Restart deadline monitoring timer */
    if (ipduConfig->Timeout > 0u) {
        Com_Dm_StartTimer(comPduId, ipduConfig->Timeout);
    }
}
```

## ASIL-D 安全特性

### 1. 冗余检查

```c
/* 双重初始化标志 */
boolean Com_DmInitialized;
boolean Com_DmInitialized_Redundant;

/* 验证函数 */
static boolean Com_Dm_VerifyRedundancy(void)
{
    return (Com_DmInitialized == Com_DmInitialized_Redundant);
}
```

### 2. 运行时完整性检查

```c
Std_ReturnType Com_Dm_ValidateIntegrity(void)
{
    /* 检查状态一致性 */
    /* 检查计时器范围 */
    /* 检查内存腐败 */
}
```

### 3. 双重验证

- 计时器写入后读回验证
- 默认值复制后验证
- 回调执行计数器检查

## API 参考

### Com_Dm_Init()

```c
void Com_Dm_Init(void);
```

初始化所有死线监控计时器和状态。

### Com_Dm_StartTimer()

```c
void Com_Dm_StartTimer(Com_IPduIdType PduId, uint32 Timeout);
```

启动或重新启动指定 I-PDU 的超时计时器。

### Com_Dm_ProcessTimers()

```c
void Com_Dm_ProcessTimers(void);
```

在 `Com_MainFunctionRx` 中调用，处理所有活动的计时器。

### Com_Dm_HandleTimeout()

```c
void Com_Dm_HandleTimeout(Com_IPduIdType PduId, const Com_DmRxConfigType* DmConfig);
```

执行配置的超时操作（ErrorHook、默认值替换等）。

### Com_Dm_ApplyDefaultValue()

```c
Std_ReturnType Com_Dm_ApplyDefaultValue(Com_IPduIdType PduId, const Com_DmRxConfigType* DmConfig);
```

将配置的默认值复制到 I-PDU 缓冲区。

## 配置示例

### 基本配置

```c
/* 定义默认值 */
const uint8 EngineData_Default[8] = {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};

/* 死线监控配置 */
Com_DmRxConfigType EngineData_DmConfig = {
    .ComIPduRxTimeout = 100,           // 100ms 超时
    .ComIPduRxDefaultValue = EngineData_Default,
    .DefaultValueLength = 8,
    .TimeoutAction = COM_DM_ACTION_BOTH,
    .ComErrorHook = EngineData_TimeoutCallback,
    .EnableDeadlineMonitoring = TRUE
};
```

### ErrorHook 实现示例

```c
void EngineData_TimeoutCallback(Com_IPduIdType PduId)
{
    /* 记录诊断码 */
    Dem_ReportErrorStatus(DEM_EVENT_ID_ENGINE_DATA_TIMEOUT, DEM_EVENT_STATUS_FAILED);
    
    /* 设置警告 */
    SetWarningFlag(WARNING_ENGINE_DATA_TIMEOUT);
    
    /* 记录日志 */
    Log_Error("EngineData I-PDU timeout detected, PduId=%d", PduId);
}
```

## 测试覆盖

单元测试文件: `tests/unit/test_com_deadline_monitor.c`

测试用例涵盖：
- TC_DM_001: 初始化验证
- TC_DM_002: 去初始化验证
- TC_DM_003-005: 计时器管理
- TC_DM_006: Rx指示重启
- TC_DM_007: ErrorHook调用
- TC_DM_008-009: 默认值替换
- TC_DM_010-016: 边缘情况

## 注意事项

1. **ASIL-D 安全级别**: 代码中包含多处冗余检查，请勿删除
2. **计时器精度**: 计时器基于 `Com_MainFunctionRx` 调用周期，通常为 10ms
3. **零超时处理**: 设置超时为0会禁用该 I-PDU 的死线监控
4. **并发安全**: 计时器操作在 `Com_MainFunctionRx` 上下文中执行，线程安全

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2024-XX-XX | 初始版本 - T012 实现 |

---

*由 YuleTech AutoSAR 平台生成*
