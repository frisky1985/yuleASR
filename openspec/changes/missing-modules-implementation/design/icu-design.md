# Icu (Input Capture Unit) 设计规格

## 1. 模块概述

### 1.1 功能说明
ICU (Input Capture Unit) 模块提供输入信号捕获功能:
- 信号边沿检测 (上升沿/下降沿/双边沿)
- 脉冲宽度测量
- 脉冲周期计数
- 信号计数 (Edge Counting)
- 时间戳捕获 (若硬件支持)

### 1.2 AUTOSAR 版本
基于 AUTOSAR Classic Platform 4.4.0, ICU 驱动规范

### 1.3 依赖模块
| 模块 | 作用 |
|------|------|
| Det  | 错误检测 |
| Gpt  | 定时器基础 (共享时钟源) |
| Port | Pin配置 |

## 2. 架构设计

### 2.1 文件结构
```
src/bsw/mcal/icu/
├── include/
│   ├── Icu.h          # 外部API头文件
│   ├── Icu_Cfg.h      # 配置头文件
│   ├── Icu_Lcfg.h     # 链接配置头文件
│   └── Icu_Private.h  # 私有头文件
└── src/
    ├── Icu.c          # 主实现
    └── Icu_Irq.c      # 中断处理
```

### 2.2 核心数据结构
```c
/* ICU通道配置 */
typedef struct {
    Icu_ChannelType channel;       /* 通道编号 */
    Icu_MeasurementModeType mode;  /* 测量模式 */
    Icu_SignalEdgeType edge;       /* 触发边沿 */
    Icu_SignalMeasurementPropertyType property; /* 测量属性 */
    Icu_NotificationType notification; /* 通知回调 */
    boolean timestampEnabled;      /* 时间戳使能 */
} Icu_ChannelConfigType;

/* ICU运行时状态 */
typedef struct {
    Icu_StateType state;           /* 通道状态 */
    Icu_InputStateType inputState; /* 输入状态 */
    Icu_ValueType capturedValue;   /* 捕获值 */
    uint16 edgeCount;              /* 边沿计数 */
    Icu_IndexType bufferIndex;     /* 时间戳缓冲区索引 */
} Icu_ChannelStateType;

/* 测量模式定义 */
typedef enum {
    ICU_MODE_SIGNAL_EDGE_DETECT = 0,    /* 边沿检测 */
    ICU_MODE_SIGNAL_MEASUREMENT,        /* 信号测量 */
    ICU_MODE_TIMESTAMP,                 /* 时间戳 */
    ICU_MODE_EDGE_COUNTER               /* 边沿计数 */
} Icu_MeasurementModeType;
```

## 3. API 设计

### 3.1 核心函数

| 函数名 | 功能 | 说明 |
|--------|------|------|
| Icu_Init | 初始化ICU模块 | 配置所有通道 |
| Icu_DeInit | 反初始化 | 释放资源 |
| Icu_SetMode | 设置模块模式 | NORMAL/SLEEP |
| Icu_DisableWakeup | 禁用唤醒源 | |
| Icu_EnableWakeup | 使能唤醒源 | |
| Icu_SetActivationCondition | 设置触发条件 | RISING/FALLING/BOTH |
| Icu_DisableNotification | 禁用通知 | |
| Icu_EnableNotification | 使能通知 | |
| Icu_GetInputState | 获取输入状态 | |
| Icu_StartTimestamp | 开始时间戳捕获 | |
| Icu_StopTimestamp | 停止时间戳捕获 | |
| Icu_GetTimestampIndex | 获取时间戳索引 | |
| Icu_ResetEdgeCount | 重置边沿计数 | |
| Icu_EnableEdgeCount | 使能边沿计数 | |
| Icu_DisableEdgeCount | 禁用边沿计数 | |
| Icu_GetEdgeNumbers | 获取边沿数 | |
| Icu_StartSignalMeasurement | 开始信号测量 | |
| Icu_StopSignalMeasurement | 停止信号测量 | |
| Icu_GetTimeElapsed | 获取已过时间 | 脉冲宽度/周期 |
| Icu_GetDutyCycleValues | 获取占空比值 | |

## 4. 实现计划

| 阶段 | 任务 | 估计工时 |
|-----|------|---------|
| 1 | 头文件创建 (Icu.h, Icu_Cfg.h) | 4h |
| 2 | 数据结构定义 | 3h |
| 3 | 初始化/反初始化 | 4h |
| 4 | 边沿检测功能 | 6h |
| 5 | 信号测量功能 | 6h |
| 6 | 时间戳功能 | 4h |
| 7 | 边沿计数功能 | 4h |
| 8 | 中断处理 | 4h |
| 9 | 单元测试 | 6h |
| **合计** | | **41h (~5天)** |

---
设计版本: 1.0
