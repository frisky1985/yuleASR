# Ocu (Output Compare Unit) 设计规格

## 1. 模块概述

### 1.1 功能说明
OCU (Output Compare Unit) 模块提供定时器输出比较功能:
- 输出比较触发
- PWM信号生成
- 可编程脉冲输出
- 波形生成

### 1.2 AUTOSAR 版本
基于 AUTOSAR Classic Platform 4.4.0, OCU 驱动规范

### 1.3 依赖模块
| 模块 | 作用 |
|------|------|
| Gpt  | 定时器基础 |
| Port | Pin配置 |
| Det  | 错误检测 |

## 2. 架构设计

### 2.1 文件结构
```
src/bsw/mcal/ocu/
├── include/
│   ├── Ocu.h          # 外部API头文件
│   ├── Ocu_Cfg.h      # 配置头文件
│   ├── Ocu_Lcfg.h     # 链接配置头文件
│   └── Ocu_Private.h  # 私有头文件
└── src/
    ├── Ocu.c          # 主实现
    └── Ocu_Irq.c      # 中断处理
```

### 2.2 核心数据结构
```c
/* OCU通道配置 */
typedef struct {
    Ocu_ChannelType channel;           /* 通道编号 */
    Ocu_OutputPinStateType pinState;   /* 默认引脚状态 */
    Ocu_ValueType threshold;           /* 比较阈值 */
    Ocu_NotificationType notification; /* 通知回调 */
    boolean runningInBackground;       /* 后台运行 */
} Ocu_ChannelConfigType;

/* OCU状态 */
typedef struct {
    Ocu_StateType state;               /* 通道状态 */
    Ocu_OutputPinStateType currentPinState; /* 当前引脚状态 */
    Ocu_ValueType compareValue;        /* 当前比较值 */
    boolean isRunning;                 /* 运行标志 */
} Ocu_ChannelStateType;

/* OCU引脚状态 */
typedef enum {
    OCU_HIGH = 0,
    OCU_LOW
} Ocu_OutputPinStateType;
```

## 3. API 设计

| 函数名 | 功能 | 说明 |
|--------|------|------|
| Ocu_Init | 初始化OCU模块 | |
| Ocu_DeInit | 反初始化 | |
| Ocu_StartChannel | 启动通道 | 开始比较 |
| Ocu_StopChannel | 停止通道 | |
| Ocu_SetPinState | 设置引脚状态 | 手动控制 |
| Ocu_SetPinAction | 设置引脚动作 | 比较触发时动作 |
| Ocu_GetCounter | 获取计数器值 | |
| Ocu_SetAbsoluteThreshold | 设置绝对阈值 | |
| Ocu_SetRelativeThreshold | 设置相对阈值 | |
| Ocu_DisableNotification | 禁用通知 | |
| Ocu_EnableNotification | 使能通知 | |

## 4. 功能演示

### 4.1 基本比较功能
```
定时器:    0---->threshold---->MAX---->0
                  ↑
输出:     _______/┛━━━━━━┗\_______
               比较匹配触发翻转
```

### 4.2 PWM生成
```
周期值:   0------------------->Period
占空比:    |<----High---->|<---Low--->|
输出:     ____/┛━━━━━━┗\___________/
```

## 5. 实现计划

| 阶段 | 任务 | 估计工时 |
|-----|------|---------|
| 1 | 头文件创建 | 3h |
| 2 | 数据结构定义 | 2h |
| 3 | 初始化/反初始化 | 3h |
| 4 | 基本比较功能 | 4h |
| 5 | PWM功能 | 4h |
| 6 | 中断处理 | 3h |
| 7 | 单元测试 | 4h |
| **合计** | | **23h (~3天)** |

---
设计版本: 1.0
备注: 低优先级/可选模块
