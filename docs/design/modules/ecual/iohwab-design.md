# IoHwAb Design Document

> **Module ID**: 0x40  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_IOHardwareAbstraction  
> **Source Path**: `src/bsw/ecual/iohwab/`  
> **Reference Document**: `docs/modules/iohwab.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

IoHwAb（I/O Hardware Abstraction）模块是 AUTOSAR ECUAL 层的核心组件，为上层 BSW 模块和应用层提供统一的 I/O 硬件抽象接口。该模块屏蔽了底层 Dio、Adc、Pwm 等 MCAL 驱动的差异，提供通道级别的模拟量读取、数字量读写功能。

主要职责：
- I/O 通道配置与初始化
- 模拟量读取（Adc 通道）
- 数字量读取与写入（Dio 通道）
- 通道反查与类型验证
- 信号反转处理（Inverted 标志）
- 版本信息查询

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS IOHardwareAbstraction | 4.4.0 | I/O 硬件抽象模块规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | ASWC / BSW | 需要 I/O 访问的应用和模块 |
| 下层 | Dio | 数字量 I/O 驱动 |
| 下层 | Adc | 模数转换器驱动 |
| 下层 | Pwm | PWM 输出驱动 |
| 公共 | Det | 开发错误追踪 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        Application / BSW            │
├─────────────────────────────────────┤
│     IoHwAb (ECUAL Layer)            │
├─────────────────────────────────────┤
│       Dio / Adc / Pwm (MCAL)        │
├─────────────────────────────────────┤
│        Hardware (GPIO/ADC/PWM)      │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **通道注册表（Channel Registry）**：维护所有已配置 I/O 通道的映射关系
- **通道查找器（Channel Finder）**：根据 ChannelId 快速定位通道条目
- **信号处理器（Signal Processor）**：处理信号反转、分辨率转换

### 3.3 文件结构

```
src/bsw/ecual/iohwab/
├── include/
│   ├── IoHwAb.h
│   └── IoHwAb_Cfg.h
└── src/
    ├── IoHwAb.c
    └── IoHwAb_Lcfg.c
```

---

## 4. 状态机

模块内部状态机：

```
[IOHWAB_INTERNAL_UNINIT]
    │ IoHwAb_Init
    ▼
[IOHWAB_INTERNAL_INIT]
    │ channels loaded
    ▼
[IOHWAB_INTERNAL_READY]
    │ IoHwAb_DeInit
    ▼
[IOHWAB_INTERNAL_UNINIT]
```

---

## 5. 核心数据结构

```c
/* 通道类型枚举 */
typedef enum {
    IOHWAB_CHANNEL_DIGITAL = 0,
    IOHWAB_CHANNEL_ANALOG,
    IOHWAB_CHANNEL_PWM
} IoHwAb_ChannelTypeEnum;

/* 通道配置 */
typedef struct {
    IoHwAb_ChannelType    ChannelId;
    IoHwAb_ChannelTypeEnum Type;
    uint16                DioChannelId;
    uint8                 AdcChannelId;
    uint8                 PwmChannelId;
    boolean               Inverted;
    uint16                Resolution;
} IoHwAb_ChannelConfigType;

/* 内部通道条目 */
typedef struct {
    uint16                     id;
    uint8                      value;
    IoHwAb_ChannelTypeEnum     type;
    uint16                     dioChannel;
    uint8                      adcChannel;
    uint8                      pwmChannel;
    boolean                    inverted;
} IoHwAb_ChannelEntryType;

/* 全局配置 */
typedef struct {
    uint8 NumChannels;
    const IoHwAb_ChannelConfigType* Channels;
} IoHwAb_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| IoHwAb_Init | `void IoHwAb_Init(const IoHwAb_ConfigType* ConfigPtr)` | 初始化 | 加载通道配置 | SWS_IoHwAb_00001 |
| IoHwAb_DeInit | `void IoHwAb_DeInit(void)` | 反初始化 | | SWS_IoHwAb_00002 |
| IoHwAb_AnalogRead | `IoHwAb_ReturnType IoHwAb_AnalogRead(IoHwAb_ChannelType Channel, uint16* Value)` | 模拟量读取 | |  |
| IoHwAb_DigitalRead | `IoHwAb_ReturnType IoHwAb_DigitalRead(IoHwAb_ChannelType Channel, uint8* Value)` | 数字量读取 | 支持反转 |  |
| IoHwAb_DigitalWrite | `IoHwAb_ReturnType IoHwAb_DigitalWrite(IoHwAb_ChannelType Channel, uint8 Value)` | 数字量写入 | 支持反转 |  |
| IoHwAb_MainFunction | `void IoHwAb_MainFunction(void)` | 周期处理 | | SWS_IoHwAb_00004 |
| IoHwAb_GetVersionInfo | `void IoHwAb_GetVersionInfo(Std_VersionInfoType*)` | 版本信息 | | SWS_IoHwAb_00003 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| — | 当前无回调接口 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x01 | Init | IOHWAB_E_PARAM_POINTER |
| 0x02 | DeInit | — |
| 0x03 | AnalogRead | IOHWAB_E_PARAM_CHANNEL, IOHWAB_E_PARAM_VALUE |
| 0x04 | DigitalRead | IOHWAB_E_PARAM_CHANNEL, IOHWAB_E_PARAM_VALUE |
| 0x05 | DigitalWrite | IOHWAB_E_PARAM_CHANNEL, IOHWAB_E_PARAM_VALUE |
| 0x0A | GetVersionInfo | — |
| 0x0B | MainFunction | — |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 ConfigPtr 有效性
2. 重置内部状态为 UNINIT
3. 遍历配置通道数组，填充内部通道条目
4. 每个通道映射 ChannelId → DioChannel/AdcChannel/PwmChannel
5. 设置通道计数，状态转为 INIT

### 7.2 模拟量读取流程

1. 检查初始化状态
2. 检查 Value 指针有效性
3. 通过 `IoHwAb_FindChannel` 查找通道
4. 验证通道类型为 ANALOG
5. 返回通道当前值

### 7.3 数字量读取流程（含反转）

1. 检查初始化状态和指针有效性
2. 查找通道并验证类型为 DIGITAL
3. 若 `inverted == TRUE`，返回 `1 - value`
4. 否则直接返回 `value`

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| IOHWAB_DEV_ERROR_DETECT | STD_ON | 开发错误检测 |
| IOHWAB_MAX_CHANNELS | 64 | 最大通道数 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| IoHwAb_Lcfg.c | 通道配置数据 |
| IoHwAb_Cfg.h | 预编译配置参数 |

### 8.3 构建后配置

不适用。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x01 | IOHWAB_E_PARAM_POINTER | 空指针入参 |
| 0x02 | IOHWAB_E_PARAM_CHANNEL | 无效通道 ID |
| 0x03 | IOHWAB_E_PARAM_VALUE | 无效值 |
| 0x04 | IOHWAB_E_UNINIT | 模块未初始化 |

### 9.2 DEM 错误

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| — | — | 当前未定义 DEM 事件 |

### 9.3 安全机制

- 通道类型验证（防止对模拟通道执行数字操作）
- 信号反转保护
- 通道数量上限保护（IOHWAB_MAX_CHANNELS = 64）

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| 默认代码段 | IoHwAb.c 全部函数 |
| 默认数据段 | 内部状态结构体 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~640 bytes | 64 通道 × 10 bytes/通道 |
| ROM | ~1.5 KB | 代码段 |
| 堆栈 | ~128 bytes | 函数调用栈 |

---

## 11. 集成指南

- 与上层集成：ASWC/BSW 通过 `IoHwAb_AnalogRead`/`IoHwAb_DigitalRead`/`IoHwAb_DigitalWrite` 访问 I/O
- 与下层集成：依赖 Dio/Adc/Pwm MCAL 驱动
- 初始化顺序：Mcal(Dio/Adc/Pwm) → Det → IoHwAb_Init
- 通道 ID 需与 IoHwAb_Cfg.h 中的定义一致

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| test_iohwab.c | 初始化/反初始化、模拟量读取、数字量读写、反转逻辑、通道类型验证、未初始化保护 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| ADC 读取 | 验证模拟量读取与 Adc 驱动集成 |
| GPIO 读写 | 验证数字量读写与 Dio 驱动集成 |
| 反转通道 | 验证 Inverted 标志的正确处理 |

---

## 13. 实现说明 / TODO

- 当前实现为内部值缓存模式，未实际调用 Dio/Adc/Pwm MCAL 驱动
- `IoHwAb_MainFunction` 为空实现，需要添加 MCAL 数据刷新逻辑
- 需要添加 PWM 通道写入 API
- 需要添加 ADC 原始值到工程单位的转换
- 需要添加电压/电流越界诊断（DEM 事件）
- 编译时版本检查已实现（AR major/minor mismatch）

---

## 14. 参考资料

1. AUTOSAR_SWS_IOHardwareAbstraction.pdf
2. `docs/modules/iohwab.md`
3. `src/bsw/ecual/iohwab/`
