---
title: MCU Driver Module
sidebar_label: mcu
description: "MCU Driver (Microcontroller Driver) 是 AutoSAR MCAL 层的核心模块之一，负责微控制器的基本初始化、时钟配置、复位管理和低功耗模式控制。"
sidebar_position: 14
---

# MCU Driver Module

## 概述

MCU Driver (Microcontroller Driver) 是 AutoSAR MCAL 层的核心模块之一，负责微控制器的基本初始化、时钟配置、复位管理和低功耗模式控制。

## 模块信息

| 属性 | 值 |
|:-----|:---|
| **模块名称** | MCU Driver |
| **模块 ID** | 0x64 (100) |
| **供应商 ID** | 0x55 (YuleTech) |
| **软件版本** | 1.0.0 |
| **所在层级** | MCAL (Microcontroller Driver Layer) |
| **状态** | ✅ 已完成 |

## 功能特性

- **微控制器初始化**: 提供系统级的初始化功能
- **时钟配置管理**: 支持多种时钟源配置，包括 PLL 锁定和切换
- **复位管理**: 识别和读取复位原因
- **低功耗模式**: 支持 RUN、SLEEP、DEEP_SLEEP 等模式切换
- **RAM 管理**: 支持 RAM 初始化和状态检查

## 文件结构

```
src/bsw/mcal/mcu/
├── include/
│   ├── Mcu.h              # 公共接口头文件
│   ├── Mcu_Cfg.h          # 配置头文件
│   └── Mcu_Reg.h          # 寄存器定义
├── src/
│   └── Mcu.c              # 模块实现代码
tests/unit/autosar/mcal/
└── test_mcu.c           # 单元测试代码
docs/modules/
└── mcu.md               # 本文档
```

## API 参考

### 核心 API 列表

| API 函数 | 功能描述 | 参数 | 返回值 |
|---------|---------|------|--------|
| `Mcu_Init` | 初始化 MCU 模块 | `ConfigPtr`: 配置结构指针 | `void` |
| `Mcu_InitClock` | 初始化时钟系统 | `ClockSetting`: 时钟配置索引 | `Std_ReturnType` |
| `Mcu_DistributePllClock` | 分发 PLL 时钟 | 无 | `void` |
| `Mcu_GetPllStatus` | 获取 PLL 锁定状态 | 无 | `Mcu_PllStatusType` |
| `Mcu_SetMode` | 设置 MCU 模式 | `McuMode`: 目标模式 | `void` |
| `Mcu_GetResetReason` | 获取复位原因 | 无 | `Mcu_ResetType` |
| `Mcu_GetResetRawValue` | 获取复位寄存器原始值 | 无 | `Mcu_RawResetType` |
| `Mcu_PerformReset` | 执行软件复位 | 无 | `void` |
| `Mcu_GetVersionInfo` | 获取版本信息 | `versioninfo`: 版本信息结构指针 | `void` |
| `Mcu_InitRamSection` | 初始化 RAM 区段 | `RamSection`: RAM 区段索引 | `Std_ReturnType` |
| `Mcu_GetRamState` | 获取 RAM 状态 | 无 | `Mcu_RamStateType` |

### 详细 API 说明

#### Mcu_Init

```c
void Mcu_Init(const Mcu_ConfigType* ConfigPtr);
```

**功能**: 初始化 MCU 驱动模块

**参数**:
- `ConfigPtr`: 指向 MCU 配置结构的指针，包含时钟、RAM 和模式配置

**错误处理**:
- 如果 `ConfigPtr` 为 `NULL`，报告 `MCU_E_PARAM_CONFIG`
- 如果已经初始化，报告 `MCU_E_ALREADY_INITIALIZED`

**使用示例**:
```c
const Mcu_ConfigType Mcu_Config = {
    .ClockConfigs = Mcu_ClockConfigs,
    .NumClockConfigs = 1,
    .RamSections = Mcu_RamSections,
    .NumRamSections = 1,
    .ModeConfigs = Mcu_ModeConfigs,
    .NumModes = 4
};

void EcuM_Init(void) {
    Mcu_Init(&Mcu_Config);
}
```

---

#### Mcu_InitClock

```c
Std_ReturnType Mcu_InitClock(Mcu_ClockType ClockSetting);
```

**功能**: 初始化 MCU 时钟系统

**参数**:
- `ClockSetting`: 时钟配置设置索引

**返回值**:
- `E_OK`: 时钟初始化成功
- `E_NOT_OK`: 时钟初始化失败

**错误处理**:
- 未初始化时调用，报告 `MCU_E_UNINIT`
- 无效的时钟设置，报告 `MCU_E_PARAM_CLOCK`

**使用示例**:
```c
Std_ReturnType status;
status = Mcu_InitClock(0);  /* 使用第一个时钟配置 */
if (status == E_OK) {
    /* 等待 PLL 锁定 */
    while (Mcu_GetPllStatus() != MCU_PLL_STATUS_LOCKED) {
        /* 等待 */
    }
    Mcu_DistributePllClock();
}
```

---

#### Mcu_SetMode

```c
void Mcu_SetMode(Mcu_ModeType McuMode);
```

**功能**: 设置 MCU 的工作模式

**参数**:
- `McuMode`: 目标模式
  - `MCU_MODE_RUN`: 运行模式
  - `MCU_MODE_SLEEP`: 睡眠模式
  - `MCU_MODE_DEEP_SLEEP`: 深度睡眠模式

**错误处理**:
- 未初始化时调用，报告 `MCU_E_UNINIT`
- 无效的模式，报告 `MCU_E_PARAM_MODE`

**使用示例**:
```c
/* 进入低功耗模式 */
Mcu_SetMode(MCU_MODE_SLEEP);

/* 恢复正常运行 */
Mcu_SetMode(MCU_MODE_RUN);
```

---

#### Mcu_GetResetReason

```c
Mcu_ResetType Mcu_GetResetReason(void);
```

**功能**: 获取上次复位的原因

**返回值**:
- `MCU_RESET_POWER_ON_RESET`: 上电复位
- `MCU_RESET_WATCHDOG_RESET`: 看门狗复位
- `MCU_RESET_SW_RESET`: 软件复位
- `MCU_RESET_EXTERNAL_RESET`: 外部复位
- `MCU_RESET_UNDEFINED`: 未定义

**错误处理**:
- 未初始化时调用，报告 `MCU_E_UNINIT`

**使用示例**:
```c
Mcu_ResetType resetReason = Mcu_GetResetReason();

switch (resetReason) {
    case MCU_RESET_POWER_ON_RESET:
        /* 执行上电复位处理 */
        break;
    case MCU_RESET_WATCHDOG_RESET:
        /* 记录看门狗复位 */
        Dem_ReportErrorStatus(WDG_RESET_EVENT, DEM_EVENT_STATUS_FAILED);
        break;
    /* ... */
}
```

---

## 配置参数

### 预编译配置 (Pre-compile Configuration)

| 参数名 | 默认值 | 说明 |
|--------|--------|------|
| `MCU_DEV_ERROR_DETECT` | `STD_ON` | 开启/关闭开发错误检测 |
| `MCU_VERSION_INFO_API` | `STD_ON` | 开启/关闭版本信息 API |
| `MCU_GET_RAM_STATE_API` | `STD_OFF` | 开启/关闭 RAM 状态 API |
| `MCU_PERFORM_RESET_API` | `STD_ON` | 开启/关闭执行复位 API |
| `MCU_INIT_CLOCK_API` | `STD_ON` | 开启/关闭时钟初始化 API |
| `MCU_NO_PLL` | `STD_OFF` | 无 PLL 时钟配置 |

### 时钟配置 (Clock Configuration)

| 参数名 | 默认值 | 说明 |
|--------|--------|------|
| `MCU_XTAL_FREQUENCY_HZ` | 24000000U | 晶振频率 (24MHz) |
| `MCU_SYSTEM_CLOCK_HZ` | 1000000000U | 系统时钟 (1GHz) |
| `MCU_BUS_CLOCK_HZ` | 500000000U | 总线时钟 (500MHz) |
| `MCU_FLASH_CLOCK_HZ` | 100000000U | Flash 时钟 (100MHz) |

### PLL 配置

| 参数名 | 默认值 | 说明 |
|--------|--------|------|
| `MCU_PLL_PREDIV` | 1 | PLL 预分频器 |
| `MCU_PLL_MULTIPLIER` | 125 | PLL 倍频器 |
| `MCU_PLL_POSTDIV` | 3 | PLL 后分频器 |

## 数据类型

### Mcu_ConfigType

```c
typedef struct {
    const Mcu_RamSectionConfigType* RamSections;
    uint8 NumRamSections;
    const Mcu_ClockConfigType* ClockConfigs;
    uint8 NumClockConfigs;
    const Mcu_ModeConfigType* ModeConfigs;
    uint8 NumModes;
} Mcu_ConfigType;
```

### Mcu_PllStatusType

```c
typedef enum {
    MCU_PLL_STATUS_UNDEFINED = 0,
    MCU_PLL_STATUS_LOCKED,
    MCU_PLL_STATUS_UNLOCKED
} Mcu_PllStatusType;
```

### Mcu_ResetType

```c
typedef enum {
    MCU_RESET_UNDEFINED = 0,
    MCU_RESET_POWER_ON_RESET,
    MCU_RESET_WATCHDOG_RESET,
    MCU_RESET_SW_RESET,
    MCU_RESET_EXTERNAL_RESET
} Mcu_ResetType;
```

### Mcu_ModeType

```c
typedef enum {
    MCU_MODE_RUN = 0,
    MCU_MODE_SLEEP = 1,
    MCU_MODE_DEEP_SLEEP = 2,
    MCU_MODE_NORMAL = 3
} Mcu_ModeType;
```

## 错误码

| 错误码 | 值 | 说明 |
|---------|------|------|
| `MCU_E_PARAM_CONFIG` | 0x0A | 无效的配置指针 |
| `MCU_E_PARAM_CLOCK` | 0x0B | 无效的时钟设置 |
| `MCU_E_PARAM_MODE` | 0x0C | 无效的模式 |
| `MCU_E_PLL_NOT_LOCKED` | 0x0D | PLL 未锁定 |
| `MCU_E_UNINIT` | 0x0E | 模块未初始化 |
| `MCU_E_PARAM_POINTER` | 0x0F | 无效的指针参数 |
| `MCU_E_PARAM_RAMSECTION` | 0x10 | 无效的 RAM 区段 |
| `MCU_E_ALREADY_INITIALIZED` | 0x11 | 模块已初始化 |

## 单元测试

### 测试覆盖

| 测试类别 | 测试用例数 | 覆盖率 |
|---------|----------|--------|
| 正向测试 | 15 | - |
| 负向测试 | 16 | - |
| 边界测试 | 3 | - |
| 状态测试 | 2 | - |
| **总计** | **36** | **&gt;80%** |

### 测试文件

- **测试路径**: `tests/unit/autosar/mcal/test_mcu.c`
- **测试框架**: YuleTech BSW Test Framework
- **运行方式**: 单元测试可独立编译运行

### 测试用例列表

#### 初始化测试
- `mcu_init_valid_config` - 有效配置初始化
- `mcu_init_null_config` - NULL 配置指针错误处理
- `mcu_init_already_initialized` - 重复初始化错误处理

#### 时钟测试
- `mcu_init_clock_valid` - 有效时钟配置
- `mcu_init_clock_not_initialized` - 未初始化时调用
- `mcu_init_clock_invalid_setting` - 无效时钟设置
- `mcu_distribute_pll_clock_valid` - PLL 时钟分发
- `mcu_distribute_pll_not_initialized` - 未初始化时调用
- `mcu_distribute_pll_not_locked` - PLL 未锁定
- `mcu_get_pll_status_locked` - PLL 锁定状态
- `mcu_get_pll_status_unlocked` - PLL 未锁定状态

#### 模式测试
- `mcu_set_mode_run` - RUN 模式设置
- `mcu_set_mode_sleep` - SLEEP 模式设置
- `mcu_set_mode_deep_sleep` - DEEP_SLEEP 模式设置
- `mcu_set_mode_not_initialized` - 未初始化时调用
- `mcu_set_mode_invalid` - 无效模式

#### 复位测试
- `mcu_get_reset_reason_power_on` - 上电复位检测
- `mcu_get_reset_reason_watchdog` - 看门狗复位检测
- `mcu_get_reset_reason_software` - 软件复位检测
- `mcu_get_reset_reason_external` - 外部复位检测
- `mcu_get_reset_raw_value` - 复位原始值读取
- `mcu_get_reset_reason_not_initialized` - 未初始化时调用

#### RAM 测试
- `mcu_init_ram_section_valid` - 有效 RAM 初始化
- `mcu_init_ram_section_not_initialized` - 未初始化时调用
- `mcu_init_ram_section_invalid` - 无效 RAM 区段
- `mcu_get_ram_state_valid` - RAM 状态读取
- `mcu_get_ram_state_not_initialized` - 未初始化时调用

#### 版本测试
- `mcu_get_version_info_valid` - 有效版本信息获取
- `mcu_get_version_info_null` - NULL 指针错误处理

### 运行测试

```bash
# 编译测试
cd tests/build
make test_mcu

# 运行测试
./test_mcu
```

### 预期输出

```
========================================
      MCU Driver Unit Tests
========================================
  Target Coverage: 80%+
  Test Categories:
    - Positive Tests (15)
    - Negative Tests (16)
    - Boundary Tests (3)
    - State Tests (2)
========================================

=== Test Suite: mcu ===
  [mcu] mcu_init_valid_config                  ... PASSED
  [mcu] mcu_init_clock_valid                   ... PASSED
  ...

========================================
  Test Results:
    Total:   36
    Passed:  36
    Failed:  0
========================================
  ALL TESTS PASSED!
```

## 应用笔记

### 初始化顺序

MCU 驱动必须在其他 MCAL 驱动之前初始化:

```c
void EcuM_StartupOne(void) {
    /* 1. 初始化 MCU */
    Mcu_Init(&Mcu_Config);
    
    /* 2. 初始化时钟 */
    Mcu_InitClock(0);
    
    /* 3. 等待 PLL 锁定并分发 */
    while (Mcu_GetPllStatus() != MCU_PLL_STATUS_LOCKED) {
        /* 等待 */
    }
    Mcu_DistributePllClock();
    
    /* 4. 初始化其他 MCAL 驱动 */
    Port_Init(&Port_Config);
    Dio_Init(&Dio_Config);
    /* ... */
}
```

### 低功耗管理

```c
void EnterLowPowerMode(void) {
    /* 保存状态 */
    NvM_WriteBlock(NVM_BLOCK_SYSTEM_STATE, &systemState);
    
    /* 进入睡眠模式 */
    Mcu_SetMode(MCU_MODE_SLEEP);
    
    /* 唤醒后恢复 */
    Mcu_SetMode(MCU_MODE_RUN);
    NvM_ReadBlock(NVM_BLOCK_SYSTEM_STATE, &systemState);
}
```

### 复位处理

```c
void CheckAndHandleReset(void) {
    Mcu_ResetType resetReason = Mcu_GetResetReason();
    
    if (resetReason == MCU_RESET_WATCHDOG_RESET) {
        /* 记录看门狗复位事件 */
        Dem_ReportErrorStatus(
            WDG_RESET_EVENT, 
            DEM_EVENT_STATUS_FAILED
        );
    }
    
    /* 执行系统复位 */
    Mcu_PerformReset();  /* 不会返回 */
}
```

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-04-14 | 首次发布，完成所有 AutoSAR 规范 API |

## 参考文档

- **AutoSAR 规范**: MCU Driver SWS (Software Specification)
- **平台手册**: i.MX8M Mini Reference Manual
- **相关模块**: 
  - `Mcu_Cfg.h` - 配置定义
  - `Det.h` - 开发错误跟踪
  - `Std_Types.h` - 标准类型

## 联系信息

- **公司**: 上海予乐电子科技有限公司
- **项目**: YuleTech AutoSAR BSW Platform
- **版权所有**: Copyright (c) 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
