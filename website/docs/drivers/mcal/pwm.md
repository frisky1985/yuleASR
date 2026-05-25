---
title: PWM (脉宽调制) 模块文档
sidebar_label: pwm
description: "PWM (Pulse Width Modulation) 模块是AutoSAR MCAL层的脉宽调制驱动，提供精确的PWM信号生成能力，支持多通道、可变占空比、可变周期等特性，广泛应用于电机控制、LED调光、电源管理等场景。"
sidebar_position: 17
---

# PWM (脉宽调制) 模块文档

## 概述

PWM (Pulse Width Modulation) 模块是AutoSAR MCAL层的脉宽调制驱动，提供精确的PWM信号生成能力，支持多通道、可变占空比、可变周期等特性，广泛应用于电机控制、LED调光、电源管理等场景。

---

## 功能特性

### 核心功能
- **多通道支持**: 最大8个独立PWM通道
- **可变占空比**: 支持0-100%占空比调节
- **可变周期**: 支持可变周期、固定周期和带偏移的固定周期三种模式
- **边沿通知**: 支持上升沿、下降沿、双边沿触发中断
- **电源管理**: 支持低功耗模式切换

### 通道类型
| 类型 | 说明 |
|-------|------|
| PWM_VARIABLE_PERIOD | 可变周期 - 支持运行时修改周期和占空比 |
| PWM_FIXED_PERIOD | 固定周期 - 只支持修改占空比 |
| PWM_FIXED_PERIOD_SHIFTED | 带偏移的固定周期 - 用于同步多通道PWM |

---

## 架构设计

### 层级结构
```
┌───────────────────────────────────────────────────────┐
│        Application Layer (ASW)                   │
├───────────────────────────────────────────────────────┤
│         RTE (Runtime Environment)                 │
├───────────────────────────────────────────────────────┤
│    Service Layer (ECU State Manager, etc.)       │
├───────────────────────────────────────────────────────┤
│       ECU Abstraction Layer (ECUAL)               │
├───────────────────────────────────────────────────────┤
│   ┌───────────────────────────────────────────────┐   │
│   │    PWM Driver (This Module)                   │   │
│   │         │                                      │   │
│   │    ┌────────────┐    ┌──────────┐     │   │
│   │    │ Channel   │    │  PWM HW     │     │   │
│   │    │    0-7     │    │  Registers  │     │   │
│   │    └────────────┘    └──────────┘     │   │
│   └───────────────────────────────────────────────┘   │
└───────────────────────────────────────────────────────┘
┌───────────────────────────────────────────────────────┐
│         Microcontroller Hardware                  │
└───────────────────────────────────────────────────────┘
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

#### Pwm_Init
```c
void Pwm_Init(const Pwm_ConfigType* ConfigPtr)
```初始化PWM驱动。

| 参数 | 方向 | 描述 |
|------|------|------|
| ConfigPtr | IN | 指向配置结构的指针 |

**DET错误检测:**
- `PWM_E_PARAM_CONFIG`: 配置指针为NULL
- `PWM_E_ALREADY_INITIALIZED`: 驱动已初始化

---

#### Pwm_DeInit
```c
void Pwm_DeInit(void)
```反初始化PWM驱动。

**DET错误检测:**
- `PWM_E_UNINIT`: 驱动未初始化

---

### 占空比控制

#### Pwm_SetDutyCycle
```c
void Pwm_SetDutyCycle(Pwm_ChannelType Channel, uint16 DutyCycle)
```设置指定通道的占空比。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | PWM通道号 (0-7) |
| DutyCycle | IN | 占空比值 (0-0x8000, 0x8000=100%) |

**DET错误检测:**
- `PWM_E_UNINIT`: 驱动未初始化
- `PWM_E_PARAM_CHANNEL`: 无效通道号

---

#### Pwm_SetPeriodAndDuty
```c
void Pwm_SetPeriodAndDuty(Pwm_ChannelType Channel, Pwm_PeriodType Period, uint16 DutyCycle)
```同时设置指定通道的周期和占空比。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | PWM通道号 (0-7) |
| Period | IN | 周期值 (时钟周期数) |
| DutyCycle | IN | 占空比值 |

**DET错误检测:**
- `PWM_E_UNINIT`: 驱动未初始化
- `PWM_E_PARAM_CHANNEL`: 无效通道号
- `PWM_E_PERIOD_UNCHANGEABLE`: 尝试修改固定周期通道的周期

**限制:** 仅对PWM_VARIABLE_PERIOD类型的通道有效。

---

### 输出控制

#### Pwm_SetOutputToIdle
```c
void Pwm_SetOutputToIdle(Pwm_ChannelType Channel)
```将指定通道的输出设置为空闲状态。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | PWM通道号 (0-7) |

**说明:** 空闲状态由配置中的`IdleState`参数决定。

---

#### Pwm_GetOutputState
```c
Pwm_OutputStateType Pwm_GetOutputState(Pwm_ChannelType Channel)
```获取指定通道的当前输出状态。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | PWM通道号 (0-7) |

**返回值:**
- `PWM_HIGH`: 高电平
- `PWM_LOW`: 低电平

---

### 通知管理

#### Pwm_EnableNotification
```c
void Pwm_EnableNotification(Pwm_ChannelType Channel, Pwm_EdgeNotificationType Notification)
```使能指定通道的边沿通知中断。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | PWM通道号 (0-7) |
| Notification | IN | 触发边沿类型 |

**触发边沿类型:**
- `PWM_RISING_EDGE`: 上升沿触发
- `PWM_FALLING_EDGE`: 下降沿触发
- `PWM_BOTH_EDGES`: 双边沿触发

---

#### Pwm_DisableNotification
```c
void Pwm_DisableNotification(Pwm_ChannelType Channel)
```禁用指定通道的通知中断。

| 参数 | 方向 | 描述 |
|------|------|------|
| Channel | IN | PWM通道号 (0-7) |

---

### 版本信息

#### Pwm_GetVersionInfo
```c
void Pwm_GetVersionInfo(Std_VersionInfoType* versioninfo)
```获取驱动版本信息。

| 参数 | 方向 | 描述 |
|------|------|------|
| versioninfo | OUT | 版本信息结构体指针 |

**DET错误检测:**
- `PWM_E_PARAM_POINTER`: 指针为NULL

---

### 电源模式

#### Pwm_SetPowerState
```c
void Pwm_SetPowerState(Pwm_PowerStateType PowerState, Pwm_PowerStateRequestResultType* Result)
```设置PWM驱动的电源状态。

| 参数 | 方向 | 描述 |
|------|------|------|
| PowerState | IN | 目标电源状态 |
| Result | OUT | 操作结果 |

---

#### Pwm_GetTargetPowerState
```c
void Pwm_GetTargetPowerState(Pwm_PowerStateType* TargetPowerState, Pwm_PowerStateRequestResultType* Result)
```获取目标电源状态。

---

#### Pwm_GetCurrentPowerState
```c
void Pwm_GetCurrentPowerState(Pwm_PowerStateType* CurrentPowerState, Pwm_PowerStateRequestResultType* Result)
```获取当前电源状态。

---

#### Pwm_PreparePowerState
```c
void Pwm_PreparePowerState(Pwm_PowerStateType PowerState, Pwm_PowerStateRequestResultType* Result)
```准备电源状态转换。

---

## 配置参数

### 通用配置
| 参数 | 说明 | 默认值 |
|------|------|---------|
| PWM_DEV_ERROR_DETECT | 错误检测使能 | STD_ON |
| PWM_VERSION_INFO_API | 版本信息API使能 | STD_ON |
| PWM_DE_INIT_API | 反初始化API使能 | STD_ON |
| PWM_SET_DUTY_CYCLE_API | 设置占空比API使能 | STD_ON |
| PWM_SET_PERIOD_AND_DUTY_API | 设置周期和占空比API使能 | STD_ON |
| PWM_SET_OUTPUT_TO_IDLE_API | 设置输出到空闲API使能 | STD_ON |
| PWM_GET_OUTPUT_STATE_API | 获取输出状态API使能 | STD_ON |
| PWM_NOTIFICATION_SUPPORTED | 通知功能使能 | STD_ON |
| PWM_POWER_STATE_SUPPORTED | 电源状态功能使能 | STD_OFF |

### 通道配置示例
```c
const Pwm_ChannelConfigType Pwm_ChannelConfig[] = {
    {
        .ChannelId = PWM_CHANNEL_0,
        .BaseAddress = 0x30660000,
        .ChannelClass = PWM_VARIABLE_PERIOD,
        .DefaultPeriod = 1000,        /* 1ms @ 1MHz */
        .DefaultDutyCycle = 0x4000,   /* 50% */
        .IdleState = PWM_IDLE_LOW,
        .Polarity = PWM_POLARITY_HIGH,
        .ClockSource = PWM_CLOCK_SYSTEM,
        .ClockPrescaler = 1,
        .NotificationSupported = TRUE,
        .NotificationFn = &Pwm_Channel0_Notification
    },
    /* 更多通道配置... */
};
```

---

## 占空比计算

PWM驱动使用固定点数表示占空比:
- 分辨率: 0x8000 (32768)
- 0x0000 = 0%
- 0x4000 = 50%
- 0x8000 = 100%

**计算公式:**
```
DutyCycleValue = (DesiredPercent * 0x8000) / 100
```

**例如:**
| 百分比 | 寄存器值 |
|-------|----------|
| 0%    | 0x0000   |
| 25%   | 0x2000   |
| 50%   | 0x4000   |
| 75%   | 0x6000   |
| 100%  | 0x8000   |

---

## 使用示例

### 基本用法
```c
#include "Pwm.h"

/* 初始化PWM */
void Pwm_InitExample(void)
{
    Pwm_Init(&Pwm_Config);
}

/* 设置占空比为50% */
void Pwm_Set50Percent(Pwm_ChannelType channel)
{
    Pwm_SetDutyCycle(channel, 0x4000);
}

/* 逐渐增加占空比 */
void Pwm_FadeIn(Pwm_ChannelType channel)
{
    for (uint16 duty = 0; duty <= 0x8000; duty += 0x100) {
        Pwm_SetDutyCycle(channel, duty);
        Delay_ms(10);
    }
}
```

### 电机控制示例
```c
/* 电机速度控制: 0-100% */
void Motor_SetSpeed(uint8 speedPercent)
{
    uint16 dutyCycle;
    
    if (speedPercent > 100) {
        speedPercent = 100;
    }
    
    /* 转换百分比到PWM值 */
    dutyCycle = (uint16)((speedPercent * 0x8000) / 100);
    
    Pwm_SetDutyCycle(PWM_CHANNEL_0, dutyCycle);
}
```

### 通知回调示例
```c
volatile uint32 pwmEdgeCount = 0;

void Pwm_Channel0_Notification(void)
{
    pwmEdgeCount++;
}

void SetupPwmWithNotification(void)
{
    /* 使能上升沿通知 */
    Pwm_EnableNotification(PWM_CHANNEL_0, PWM_RISING_EDGE);
}
```

---

## 测试覆盖

### 单元测试
单元测试文件位于: `tests/unit/autosar/mcal/test_pwm.c`

**测试覆盖范围:**
- ✓ 初始化测试 (Pwm_Init)
- ✓ 反初始化测试 (Pwm_DeInit)
- ✓ 占空比设置 (Pwm_SetDutyCycle)
- ✓ 周期和占空比设置 (Pwm_SetPeriodAndDuty)
- ✓ 输出到空闲 (Pwm_SetOutputToIdle)
- ✓ 获取输出状态 (Pwm_GetOutputState)
- ✓ 通知管理 (Pwm_EnableNotification, Pwm_DisableNotification)
- ✓ 版本信息 (Pwm_GetVersionInfo)
- ✓ 电源状态 (Pwm_SetPowerState, Pwm_GetTargetPowerState, Pwm_GetCurrentPowerState, Pwm_PreparePowerState)
- ✓ 多通道测试
- ✓ 边界条件测试
- ✓ DET错误检测

**运行测试:**
```bash
cd /home/admin/yuleASR/tests/unit/autosar/mcal
gcc -I../../../../src/bsw/mcal/pwm/include test_pwm.c -o test_pwm
./test_pwm
```

### 测试结果示例
```
========================================
    PWM Driver Unit Tests              
========================================

=== Initialization Tests ===
  [PASS] Pwm_DriverInitialized == FALSE
  [PASS] Pwm_DriverInitialized == TRUE
  [PASS] det_call_count == 0
  [PASS] det_call_count == 1
  ...

========================================
    Test Summary                        
========================================
  Total:   150+
  Passed:  150+
  Failed:  0
  Coverage: 100.0%
========================================
```

---

## 常见问题

### Q: 占空比设置不生效
**A:** 检查:
1. 驱动是否已正确初始化
2. 通道号是否在有效范围内
3. PWM硬件时钟是否已使能

### Q: 通知不触发
**A:** 检查:
1. 通知是否已使能 (Pwm_EnableNotification)
2. 中断是否已在中断控制器中使能
3. 回调函数指针是否正确设置

### Q: 周期设置报错
**A:** 检查通道配置中的`ChannelClass`:
- `PWM_FIXED_PERIOD`: 不能在运行时修改周期
- `PWM_VARIABLE_PERIOD`: 可以在运行时修改周期

---

## 相关文档

- [AutoSAR PWM驱动规范](https://www.autosar.org/standards/classic-platform/)
- [i.MX8M Mini PWM 寄存器参考手册](https://www.nxp.com/docs/en/reference-manual/IMX8MMRM.pdf)
- [MCAL开发指南](../development-guide.md)

---

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-04-14 | 初始版本 |

---

## 版权信息

Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
All rights reserved.
