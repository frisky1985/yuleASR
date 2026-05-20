# GPT (General Purpose Timer) 模块文档

## 概述

GPT (General Purpose Timer) 模块是AutoSAR MCAL层的通用定时器驱动，提供基于硬件定时器的精确时间服务。该模块支持多通道定时器管理、通知回调、睡眠唤醒等功能。

---

## 功能特性

### 核心功能
- **多通道支持**: 最大8个独立定时器通道
- **两种工作模式**: 单次触发(One-shot)和连续(Continuous)模式
- **灵活分频**: 支持1/2/4/8/16/32/64/128分频
- **中断通知**: 定时器到期中断回调支持
- **唤醒功能**: 支持从睡眠模式唤醒

### 预定义定时器
- **1微秒 16位**: 高精度短时定时
- **1微秒 24位**: 中等时长定时
- **1微秒 32位**: 高精度长时定时
- **100微秒 32位**: 低精度长时定时

---

## 架构设计

### 层级结构
```
┌──────────────────────────────────────────────────┐
│        Application Layer (ASW)               │
├──────────────────────────────────────────────────┤
│         RTE (Runtime Environment)            │
├──────────────────────────────────────────────────┤
│    Service Layer (NvM, Dcm, Dem, etc.)       │
├──────────────────────────────────────────────────┤
│       ECU Abstraction Layer (ECUAL)          │
├──────────────────────────────────────────────────┤
│   ┌────────────────────────────────────────────┐   │
│   │    GPT Driver (This Module)              │   │
│   │         │                              │   │
│   │    ┌──────────┐    ┌────────────┐  │   │
│   │    │ Channel │    │  GPT1/GPT2   │  │   │
│   │    │    0-3   │    │  Hardware    │  │   │
│   │    └──────────┘    └────────────┘  │   │
│   └────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────┘
┌──────────────────────────────────────────────────┐
│         Microcontroller Hardware             │
└──────────────────────────────────────────────────┘
```

### 模块依赖
| 模块 | 说明 |
|------|------|
| Std_Types | 标准类型定义 |
| Det | 运行时错误检测 |
| MemMap | 内存映射 |
| Mcu | 时钟配置管理 |

---

## API接口

### 初始化与反初始化

#### Gpt_Init
```c
void Gpt_Init(const Gpt_ConfigType* ConfigPtr)
```
初始化GPT驱动。

| 参数 | 方向 | 描述 |
|------|------|------|
| ConfigPtr | IN | 指向配置结构的指针 |

**DET错误检测:**
- `GPT_E_PARAM_POINTER`: 配置指针为NULL
- `GPT_E_ALREADY_INITIALIZED`: 驱动已初始化

---

#### Gpt_DeInit
```c
void Gpt_DeInit(void)
```
反初始化GPT驱动。

**DET错误检测:**
- `GPT_E_UNINIT`: 驱动未初始化

**限制:** 当有通道正在运行时，DeInit 将不会执行。

---

### 定时器控制

#### Gpt_StartTimer
```c
void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value)
```
启动指定通道的定时器。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | 定时器通道号 (0-7) |
| Value | IN | 计时目标值 (时钟周期数) |

**DET错误检测:**
- `GPT_E_UNINIT`: 驱动未初始化
- `GPT_E_PARAM_CHANNEL`: 无效通道号
- `GPT_E_PARAM_VALUE`: 计时值为0
- `GPT_E_CHANNEL_BUSY`: 通道正在运行

---

#### Gpt_StopTimer
```c
void Gpt_StopTimer(Gpt_ChannelType Channel)
```
停止指定通道的定时器。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | 定时器通道号 (0-7) |

---

### 时间查询

#### Gpt_GetTimeElapsed
```c
Gpt_ValueType Gpt_GetTimeElapsed(Gpt_ChannelType Channel)
```
获取指定通道已经过的时间。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | 定时器通道号 (0-7) |

**返回值:** 已计数的时钟周期数

---

#### Gpt_GetTimeRemaining
```c
Gpt_ValueType Gpt_GetTimeRemaining(Gpt_ChannelType Channel)
```
获取指定通道剩余的时间。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | 定时器通道号 (0-7) |

**返回值:** 剩余时钟周期数 (通道未运行时返回0)

---

### 通知管理

#### Gpt_EnableNotification
```c
void Gpt_EnableNotification(Gpt_ChannelType Channel)
```
使能指定通道的中断通知。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | 定时器通道号 (0-7) |

---

#### Gpt_DisableNotification
```c
void Gpt_DisableNotification(Gpt_ChannelType Channel)
```禁用指定通道的中断通知。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | 定时器通道号 (0-7) |

---

### 电源模式

#### Gpt_SetMode
```c
void Gpt_SetMode(Gpt_ModeType Mode)
```
设置GPT驱动的操作模式。

| 参数 | 方向 | 描述 |
|------|------|------|
| Mode | IN | 模式 (`GPT_MODE_NORMAL` 或 `GPT_MODE_SLEEP`) |

**说明:** 进入睡眠模式时，所有运行的定时器将被停止。

---

### 唤醒功能

#### Gpt_EnableWakeup
```c
void Gpt_EnableWakeup(Gpt_ChannelType Channel)
```
使能指定通道的唤醒功能。

#### Gpt_DisableWakeup
```c
void Gpt_DisableWakeup(Gpt_ChannelType Channel)
```
禁用指定通道的唤醒功能。

#### Gpt_CheckWakeup
```c
Std_ReturnType Gpt_CheckWakeup(Gpt_ChannelType Channel)
```
检查指定通道的唤醒状态。

**返回值:**
- `E_OK`: 检测到唤醒事件
- `E_NOT_OK`: 未检测到唤醒事件

---

### 预定义定时器

#### Gpt_GetPredefTimerValue
```c
Std_ReturnType Gpt_GetPredefTimerValue(
    Gpt_PredefTimerType PredefTimer,
    uint32* TimeValuePtr
)
```
读取预定义定时器的当前值。

| 参数 | 方向 | 描述 |
|------|------|------|
| PredefTimer | IN | 预定义定时器类型 |
| TimeValuePtr | OUT | 存储时间值的指针 |

**预定义定时器类型:**
- `GPT_PREDEF_TIMER_1US_16BIT`: 1μs分辨率, 16位
- `GPT_PREDEF_TIMER_1US_24BIT`: 1μs分辨率, 24位
- `GPT_PREDEF_TIMER_1US_32BIT`: 1μs分辨率, 32位
- `GPT_PREDEF_TIMER_100US_32BIT`: 100μs分辨率, 32位

---

### 版本信息

#### Gpt_GetVersionInfo
```c
void Gpt_GetVersionInfo(Std_VersionInfoType* versioninfo)
```
获取GPT模块的版本信息。

| 参数 | 方向 | 描述 |
|------|------|------|
| versioninfo | OUT | 版本信息结构指针 |

---

## 配置参数

### 通用配置

| 参数 | 类型 | 说明 | 默认值 |
|------|------|------|--------|
| `GptDevErrorDetect` | Boolean | 使能DET错误检测 | TRUE |
| `GptVersionInfoApi` | Boolean | 使能版本信息API | TRUE |
| `GptDeInitApi` | Boolean | 使能反初始化API | TRUE |
| `GptTimeElapsedApi` | Boolean | 使能获取已过时间API | TRUE |
| `GptTimeRemainingApi` | Boolean | 使能获取剩余时间API | TRUE |
| `GptEnableDisableNotificationApi` | Boolean | 使能通知管理API | TRUE |
| `GptWakeupFunctionalityApi` | Boolean | 使能唤醒功能API | FALSE |
| `GptPredefTimer1usEnablingGrade` | Boolean | 使能1μs预定义定时器 | TRUE |
| `GptPredefTimer100us32bitEnable` | Boolean | 使能100μs预定义定时器 | TRUE |

### 通道配置

| 参数 | 类型 | 说明 |
|------|------|------|
| `GptChannelId` | uint8 | 通道号 (0-7) |
| `GptChannelMode` | Enum | 工作模式 (One-shot/Continuous) |
| `GptClockPrescaler` | Enum | 时钟分频 (1/2/4/8/16/32/64/128) |
| `GptMaxTickValue` | uint32 | 最大计数值 |
| `GptNotification` | FunctionPtr | 通知回调函数指针 |

---

## 使用示例

### 基本定时器使用
```c
#include "Gpt.h"

/* 通知回调函数 */
void MyTimerNotification(void)
{
    /* 定时器到期处理 */
}

/* 初始化并启动定时器 */
void InitTimer(void)
{
    /* 初始化GPT驱动 */
    Gpt_Init(&Gpt_Config);
    
    /* 启动通道0，8ms超时 (1μs/tick, 8000 ticks) */
    Gpt_StartTimer(GPT_CHANNEL_0, 8000);
    
    /* 使能中断通知 */
    Gpt_EnableNotification(GPT_CHANNEL_0);
}

/* 轮询检查定时器 */
void CheckTimer(void)
{
    Gpt_ValueType elapsed = Gpt_GetTimeElapsed(GPT_CHANNEL_0);
    Gpt_ValueType remaining = Gpt_GetTimeRemaining(GPT_CHANNEL_0);
}
```

### 唤醒功能使用
```c
/* 使能睡眠唤醒 */
void SetupWakeup(void)
{
    /* 初始化 */
    Gpt_Init(&Gpt_Config);
    
    /* 使能通道1的唤醒功能 */
    Gpt_EnableWakeup(GPT_CHANNEL_1);
    
    /* 启动定时器 (500ms后唤醒) */
    Gpt_StartTimer(GPT_CHANNEL_1, 500000);
    
    /* 进入睡眠模式 */
    Gpt_SetMode(GPT_MODE_SLEEP);
}

/* 检查唤醒源 */
void CheckWakeupSource(void)
{
    if (Gpt_CheckWakeup(GPT_CHANNEL_1) == E_OK) {
        /* 处理唤醒事件 */
        Gpt_SetMode(GPT_MODE_NORMAL);
    }
}
```

### 预定义定时器使用
```c
/* 获取高精度时间戳 */
void GetTimestamp(void)
{
    uint32 timestamp;
    
    if (Gpt_GetPredefTimerValue(GPT_PREDEF_TIMER_1US_32BIT, &timestamp) == E_OK) {
        /* timestamp 包含 1μs 分辨率的时间值 */
    }
}
```

---

## 测试覆盖

### 单元测试覆盖

| 测试类别 | 测试项数 | 覆盖API |
|---------|---------|---------|
| 初始化测试 | 3 | `Gpt_Init` |
| 反初始化测试 | 3 | `Gpt_DeInit` |
| 时间查询测试 | 8 | `Gpt_GetTimeElapsed`, `Gpt_GetTimeRemaining` |
| 定时器控制测试 | 9 | `Gpt_StartTimer`, `Gpt_StopTimer` |
| 通知管理测试 | 6 | `Gpt_EnableNotification`, `Gpt_DisableNotification` |
| 版本信息测试 | 2 | `Gpt_GetVersionInfo` |
| 模式管理测试 | 3 | `Gpt_SetMode` |
| 唤醒功能测试 | 9 | `Gpt_EnableWakeup`, `Gpt_DisableWakeup`, `Gpt_CheckWakeup` |
| 预定义定时器测试 | 4 | `Gpt_GetPredefTimerValue` |
| 集成测试 | 4 | 组合API |

**总测试项:** 51项
**API覆盖率:** 100% (13/13 公开API)
**代码覆盖率:** >80%

---

## 性能特性

### 时序特性
| 操作 | 执行时间 | 说明 |
|------|----------|------|
| `Gpt_Init` | <100μs | 一次性初始化 |
| `Gpt_StartTimer` | <5μs | 启动定时器 |
| `Gpt_StopTimer` | <5μs | 停止定时器 |
| `Gpt_GetTimeElapsed` | <2μs | 读取当前计数 |
| `Gpt_GetTimeRemaining` | <3μs | 计算并返回剩余时间 |

### 资源占用
| 资源 | 占用 |
|------|------|
| RAM | ~128 bytes (状态管理) + 8×通道配置 |
| ROM | ~4KB (代码) |
| 中断 | 2 (GPT1/GPT2) |

---

## 错误处理

### DET错误代码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `GPT_E_PARAM_CHANNEL` | 0x0A | 无效通道号 |
| `GPT_E_PARAM_VALUE` | 0x0B | 无效计时值 |
| `GPT_E_PARAM_POINTER` | 0x0C | NULL指针参数 |
| `GPT_E_PARAM_MODE` | 0x0D | 无效模式 |
| `GPT_E_PARAM_PREDEF_TIMER` | 0x0E | 无效预定义定时器 |
| `GPT_E_ALREADY_INITIALIZED` | 0x0F | 重复初始化 |
| `GPT_E_CHANNEL_BUSY` | 0x10 | 通道正忙 |
| `GPT_E_UNINIT` | 0x11 | 驱动未初始化 |
| `GPT_E_INIT_FAILED` | 0x12 | 初始化失败 |

---

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-04-14 | 首次发布，支持i.MX8M Mini GPT硬件 |

---

## 参考资料

1. AutoSAR Classic Platform Specification - GPT Driver
2. NXP i.MX 8M Mini Reference Manual - GPT Chapter
3. YuleTech MCAL Development Guidelines

---

## 联系支持

- **作者**: 上海予乐电子科技有限公司
- **项目**: YuleTech AutoSAR BSW Platform
- **版本**: v1.0.0
