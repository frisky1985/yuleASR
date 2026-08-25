# LinTrcv Design Document

> **Module ID**: 0x44  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_LINTransceiverDriver  
> **Source Path**: `src/bsw/ecual/lintrcv/`  
> **Reference Document**: `docs/modules/lintrcv.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

LinTrcv（LIN Transceiver Driver）是 AUTOSAR ECU 抽象层中负责管理 LIN 收发器硬件的驱动模块。该模块通过 DIO 引脚控制 LIN 收发器的工作模式，并管理唤醒事件的检测和上报。

主要功能：
- 多通道 LIN 收发器管理（最多 LINTRCV_MAX_CHANNELS = 4）
- 收发器模式控制（Normal/Standby/Sleep）
- 支持 TJA1021/TJA1022/TJA1028 及通用 LIN 收发器
- DIO 引脚控制（EN、NWake、NERR）
- 唤醒事件检测（总线唤醒、引脚唤醒、系统错误）
- 与 EcuM 集成，上报唤醒事件
- 模式转换延迟管理
- 支持 SPI/I2C 控制接口（条件编译）

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS LINTransceiverDriver | 4.4.0 | LIN 收发器驱动规范 |
| ISO 17987 | - | LIN 协议标准 |
| SAE J2602 | - | LIN 物理层标准 |

### 2.2 模块依赖

| 模块 | 依赖方向 | 说明 |
|------|----------|------|
| Dio | 调用 | DIO 驱动，控制 EN/NWake/NERR 引脚 |
| Spi | 调用 | SPI 驱动（条件编译 LINTRCV_SPI_SUPPORT） |
| EcuM | 调用 | ECU 状态管理器，上报唤醒事件 |
| Det | 调用 | 默认错误追踪 |
| Std_Types | 包含 | AUTOSAR 标准类型 |
| MemMap | 包含 | 内存分区宏 |

## 3. 架构设计

### 3.1 分层位置

```
+-----------------------------------+
|       BSW Services (LinSM)        |
+-----------------------------------+
|    ECUAL Layer (LinTrcv)          |  <-- 本模块
+-----------------------------------+
|    MCAL (Dio, Spi)                |
+-----------------------------------+
|    Hardware (TJA1021/TJA1022)     |
+-----------------------------------+
```

### 3.2 内部组件

- **TJA1021 模式控制器**：通过 EN 引脚控制收发器模式
  - Normal: EN=HIGH
  - Standby: EN=LOW（NWake 由外部上拉）
  - Sleep: EN=LOW（NWake=LOW 或超时后自动进入）
- **唤醒检测器**：检查 NWake 引脚和总线活动
- **EcuM 通知器**：检测到唤醒后调用 EcuM_SetWakeupEvent
- **模式转换验证器**：验证模式转换的合法性
- **延迟管理器**：模式转换间的微秒级延迟

### 3.3 文件结构

```
src/bsw/ecual/lintrcv/
├── include/
│   ├── LinTrcv.h          # 公共 API 头文件
│   └── LinTrcv_Cfg.h      # 预编译配置头文件
└── src/
    ├── LinTrcv.c           # 模块实现
    └── LinTrcv_Lcfg.c      # 链接时配置表
```

## 4. 状态机

### 4.1 模块状态

```
    UNINIT ──── Init() ────> INIT
                  │
    DeInit() ─────┘
```

### 4.2 通道状态

```
    LINTRCV_CHANNEL_UNINIT (0) ──── Init ────> LINTRCV_CHANNEL_INIT (1)
```

### 4.3 TJA1021 操作模式

```
    SLEEP ←──→ STANDBY ←──→ NORMAL
      │                        │
      └────────────────────────┘
           SetOpMode() 控制
```

**TJA1021 引脚与模式对应关系：**

| 模式 | EN 引脚 | NWake | 说明 |
|------|---------|-------|------|
| NORMAL | HIGH | - | 正常通信 |
| STANDBY | LOW | HIGH（外部上拉） | 低功耗，可唤醒 |
| SLEEP | LOW | LOW（或超时） | 最低功耗，可唤醒 |

**TJA1021 模式转换延迟：**

| 转换 | 延迟 |
|------|------|
| Sleep → Normal | 1000 us |
| Standby → Normal | 100 us |
| Normal → Standby | 50 us |
| Normal → Sleep | 50 us |

## 5. 核心数据结构

### 5.1 通道运行时状态

```c
typedef struct {
    LinTrcv_ChannelStateType State;           /* 通道初始化状态 */
    LinTrcv_OpmodeType CurrentMode;           /* 当前操作模式 */
    LinTrcv_WakeupReasonType LastWuReason;    /* 最后唤醒原因 */
    boolean WakeupEventPending;               /* 唤醒事件待处理 */
    boolean ModeTransitionPending;            /* 模式转换进行中 */
    uint32 ModeTransitionStartTime;           /* 模式转换开始时间 */
} LinTrcv_ChannelStateStructType;
```

### 5.2 通道配置

```c
typedef struct {
    uint8 ChannelId;                          /* 通道 ID */
    LinTrcv_HardwareType HwType;              /* 硬件类型 */
    LinTrcv_ControlInterfaceType CtrlIf;      /* 控制接口 */
    uint16 EnPinDio;                          /* EN 引脚 */
    uint16 TxDPinDio;                         /* TXD 引脚 */
    uint16 NwadrsPinDio;                      /* NWake 引脚 */
    uint16 NerrPinDio;                        /* NERR 引脚 */
    boolean WakeupByBusEnabled;               /* 总线唤醒使能 */
    boolean WakeupByPinEnabled;               /* 引脚唤醒使能 */
    uint32 WakeupSourceRef;                   /* EcuM 唤醒源 */
    uint32 SleepToNormalDelay;                /* Sleep→Normal 延迟 */
    uint32 StandbyToNormalDelay;              /* Standby→Normal 延迟 */
    uint32 NormalToStandbyDelay;              /* Normal→Standby 延迟 */
    uint32 NormalToSleepDelay;                /* Normal→Sleep 延迟 */
    LinTrcv_OpmodeType InitialMode;           /* 初始模式 */
} LinTrcv_ChannelConfigType;
```

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `LinTrcv_Init(ConfigPtr)` | 0x00 | 初始化模块 |
| `LinTrcv_DeInit()` | 0x01 | 去初始化 |
| `LinTrcv_SetOpMode(Channel, OpMode)` | 0x02 | 设置操作模式 |
| `LinTrcv_GetOpMode(Channel, *OpMode)` | 0x03 | 获取操作模式 |
| `LinTrcv_GetBusWuReason(Channel, *WuReason)` | 0x04 | 获取唤醒原因 |
| `LinTrcv_GetVersionInfo(*VersionInfo)` | 0x05 | 获取版本信息 |
| `LinTrcv_Wakeup(Channel)` | 0x06 | 发起唤醒 |
| `LinTrcv_CheckWakeup(Channel)` | 0x07 | 检查唤醒事件 |
| `LinTrcv_Cbk_WakeupByBus(Channel)` | 0x08 | 总线唤醒回调 |
| `LinTrcv_MainFunction()` | - | 周期处理 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| `LinTrcv_Cbk_WakeupByBus(Channel)` | 总线唤醒通知回调（由 Dio 中断触发） |

### 6.3 服务 ID 与错误码

**DET 错误码：**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| LINTRCV_E_UNINIT | 0x01 | 未初始化 |
| LINTRCV_E_INVALID_CHANNEL | 0x02 | 无效通道 |
| LINTRCV_E_INVALID_OPMODE | 0x03 | 无效操作模式 |
| LINTRCV_E_PARAM_POINTER | 0x04 | 空指针 |
| LINTRCV_E_INIT_FAILED | 0x05 | 初始化失败 |
| LINTRCV_E_BUS_WU_NOT_SUPPORTED | 0x06 | 不支持总线唤醒 |
| LINTRCV_E_PARAM_CONFIG | 0x07 | 无效配置 |
| LINTRCV_E_HW_FAILURE | 0x08 | 硬件故障 |

**唤醒原因枚举：**

| 枚举值 | 说明 |
|--------|------|
| LINTRCV_WU_BY_BUS | 总线活动唤醒 |
| LINTRCV_WU_BY_PIN | NWake 引脚唤醒 |
| LINTRCV_WU_BY_SYSERR | 系统错误唤醒 |
| LINTRCV_WU_RESET | 复位唤醒 |
| LINTRCV_WU_INTERNAL | 内部唤醒 |
| LINTRCV_WU_NOT_SUPPORTED | 不支持 |
| LINTRCV_WU_ERROR | 错误 |
| LINTRCV_WU_POWER_ON | 上电唤醒 |
| LINTRCV_WU_BY_BUS_CS | 总线状态变化唤醒 |

## 7. 处理流程

### 7.1 初始化流程

1. 检查配置指针有效性
2. 保存配置指针
3. 遍历所有通道：
   a. 设置通道状态为 INIT
   b. 设置唤醒原因为 RESET
   c. 根据配置的初始模式调用 SetTja1021Mode
4. 标记模块已初始化

### 7.2 TJA1021 模式设置流程

```
SetTja1021Mode(Channel, OpMode)
  │
  ├── NORMAL 模式
  │     ├── EN = HIGH
  │     ├── 如果从 Sleep → 延迟 SleepToNormalDelay (1000us)
  │     └── 如果从 Standby → 延迟 StandbyToNormalDelay (100us)
  │
  ├── STANDBY 模式
  │     ├── EN = LOW
  │     └── 延迟 NormalToStandbyDelay (50us)
  │
  └── SLEEP 模式
        ├── EN = LOW
        └── 延迟 NormalToSleepDelay (50us)
```

### 7.3 MainFunction 周期处理

遍历所有通道：
1. 如果通道处于 STANDBY 或 SLEEP 模式：
   a. 读取硬件模式（通过 EN 引脚推断）
   b. 如果检测到 NORMAL 模式 → 设置唤醒事件
   c. 通知 EcuM
2. 如果引脚唤醒使能：
   a. 读取 NWake 引脚状态
   b. NWake 活跃 → 设置唤醒事件（原因 = BY_PIN）
   c. 通知 EcuM

### 7.4 唤醒检测流程

```
DetectWakeupReason(Channel)
  │
  ├── 检查 WakeupByBusEnabled
  │     └── WakeupEventPending → WU_BY_BUS
  │
  ├── 检查 WakeupByPinEnabled
  │     └── NWake = ACTIVE → WU_BY_PIN
  │
  └── 检查 NERR 引脚
        └── NERR = ERROR → WU_BY_SYSERR
```

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| LINTRCV_DEV_ERROR_DETECT | STD_ON | DET 使能 |
| LINTRCV_VERSION_INFO_API | STD_ON | 版本信息 API |
| LINTRCV_NUM_CHANNELS | 2 | 配置通道数 |
| LINTRCV_MAX_CHANNELS | 4 | 最大通道数 |
| LINTRCV_WAKEUP_SUPPORTED | STD_ON | 唤醒支持 |
| LINTRCV_WAKEUP_BY_BUS_USED | STD_ON | 总线唤醒 |
| LINTRCV_WAKEUP_BY_PIN_USED | STD_ON | 引脚唤醒 |
| LINTRCV_SPI_SUPPORT | STD_OFF | SPI 支持 |
| LINTRCV_I2C_SUPPORT | STD_OFF | I2C 支持 |
| LINTRCV_TJA1021_SUPPORT | STD_ON | TJA1021 支持 |
| LINTRCV_TJA1022_SUPPORT | STD_ON | TJA1022 支持 |
| LINTRCV_TJA1021_SLEEP_TO_NORMAL_US | 1000 | Sleep→Normal 延迟 |
| LINTRCV_TJA1021_STANDBY_TO_NORMAL_US | 100 | Standby→Normal 延迟 |
| LINTRCV_WAKEUP_DEBOUNCE_US | 50 | 唤醒去抖延迟 |

### 8.2 链接时配置

链接时配置定义在 `LinTrcv_Lcfg.c` 中，包括：
- CH0: EN=DIO_CHANNEL_TJA1021_0_EN, NWake=DIO_CHANNEL_TJA1021_0_NWAKE, NERR=DIO_CHANNEL_TJA1021_0_NERR
- CH1: EN=DIO_CHANNEL_TJA1021_1_EN, NWake=DIO_CHANNEL_TJA1021_1_NWAKE, NERR=DIO_CHANNEL_TJA1021_1_NERR
- 唤醒源: LINTRCV_WAKEUP_SOURCE_0(16) / LINTRCV_WAKEUP_SOURCE_1(32)

### 8.3 构建后配置

本实现不支持构建后配置。

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 均进行：
- 初始化状态检查（LINTRCV_E_UNINIT）
- 通道有效性检查（LINTRCV_E_INVALID_CHANNEL）
- 模式值范围检查（LINTRCV_E_INVALID_OPMODE）
- 指针有效性检查（LINTRCV_E_PARAM_POINTER）
- AUTOSAR 版本一致性检查（编译时 #error）

### 9.2 DEM 错误

本实现未集成 DEM 事件上报。

### 9.3 安全机制

- **MemMap 分区**：使用 MemMap.h 进行内存段保护
- **模式转换验证**：ValidateModeTransition 检查转换合法性
- **去初始化安全**：DeInit 将所有通道设为 Standby 模式
- **版本一致性检查**：编译时检查 DET 与 LinTrcv 的 AUTOSAR 版本匹配
- **延迟管理**：模式转换后等待硬件稳定时间

## 10. 内存与性能

### 10.1 MemMap 分区

| 段名 | 类型 | 说明 |
|------|------|------|
| LINTRCV_START_SEC_VAR_CLEARED_UNSPECIFIED | RAM (清零) | 运行时变量 |
| LINTRCV_START_SEC_CODE | RAM | 代码段 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| LinTrcv_ChannelState[4] | ~48 bytes | 每通道 ~12 bytes |
| LinTrcv_ConfigPtr | 4 bytes | 配置指针 |
| LinTrcv_ModuleInitialized | 1 byte | 初始化标志 |
| 配置表 | ~120 bytes | 只读配置 |
| **总计 RAM** | **~53 bytes** | 运行时变量 |
| **总计 ROM** | **~120 bytes** | 配置常量 |

## 11. 集成指南

### 集成步骤

1. 在 `LinTrcv_Cfg.h` 中配置通道数、硬件类型和 DIO 引脚
2. 准备 `LinTrcv_ConfigType` 配置结构体
3. 调用 `LinTrcv_Init(&config)` 初始化
4. 将 `LinTrcv_MainFunction()` 加入周期任务
5. 配置 EcuM 唤醒源与 LinTrcv 通道的对应关系
6. 连接 Dio 中断到 `LinTrcv_Cbk_WakeupByBus` 回调

### 注意事项

- TJA1021 从 Sleep 到 Normal 需要 1000us 延迟
- NWake 引脚为低电平有效（LINTRCV_TJA1021_NWAKE_ACTIVE = STD_OFF）
- NERR 引脚为低电平有效（LINTRCV_TJA1021_NERR_ERROR = STD_OFF）
- 当前延迟实现使用忙等待循环（LinTrcv_DelayUs），应替换为定时器或 OS 延迟服务

## 12. 测试策略

### 12.1 单元测试

| 测试场景 | 预期结果 |
|----------|----------|
| Init 后状态检查 | 所有通道为 INIT 状态，初始模式已设置 |
| SetOpMode(NORMAL) | EN 引脚为 HIGH |
| SetOpMode(STANDBY) | EN 引脚为 LOW |
| SetOpMode(SLEEP) | EN 引脚为 LOW |
| GetOpMode 查询 | 返回当前模式 |
| 唤醒检测（NWake 活跃） | WakeupEventPending=TRUE |
| CheckWakeup | 检测到唤醒后通知 EcuM |
| Cbk_WakeupByBus | 设置唤醒事件并通知 EcuM |
| DeInit | 所有通道进入 Standby |
| 无效通道索引 | 返回 E_NOT_OK，触发 DET |

### 12.2 集成测试

| 测试场景 | 验证点 |
|----------|--------|
| LinTrcv + LinSM 联合测试 | 模式请求正确传递 |
| LinTrcv + EcuM 联合测试 | 唤醒事件正确上报 |
| LinTrcv + Lin 联合测试 | 总线唤醒触发正确 |
| 多通道并发测试 | 各通道独立运行 |
| 模式转换延迟测试 | 延迟时间满足 TJA1021 规格 |

## 13. 实现说明 / TODO

### 当前实现特点

- 支持 TJA1021/TJA1022/TJA1028 和通用 LIN 收发器
- 完整的 DIO 控制模式管理
- 三种唤醒源检测（总线、引脚、系统错误）
- MemMap 内存分区
- AUTOSAR 版本一致性编译检查

### 待实现项

- [ ] SPI/I2C 控制接口的实际实现
- [ ] 延迟函数替换为定时器/OS 服务（当前为忙等待）
- [ ] 模式转换状态机（异步转换处理）
- [ ] DEM 事件上报
- [ ] TJA1022 双通道特定功能
- [ ] 唤醒去抖机制的实际实现

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| AUTOSAR_SWS_LINTransceiverDriver.pdf | AUTOSAR LinTrcv 规范 |
| NXP TJA1021 Datasheet | LIN 收发器数据手册 |
| NXP TJA1022 Datasheet | 双 LIN 收发器数据手册 |
| ISO 17987 | LIN 协议标准 |
| LinTrcv.h | 模块公共接口定义 |
| LinTrcv_Cfg.h | 预编译配置定义 |
| LinTrcv.c | 模块实现源码 |

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_LinTrcv_00001 | `lintrcv` | 测试 test_lintrcv_Init_should_initialize 覆盖: lintrcv_Init_should_initialize 场景 |
| SWS_LinTrcv_00002 | `LinTrcv_DeInit` | 测试 test_LinTrcv_DeInit_ValidCall_ShouldSucceed 覆盖: LinTrcv_DeInit_ValidCall_ShouldSucceed 场景 |
| SWS_LinTrcv_00003 | `LinTrcv_GetVersionInfo` | 测试 test_LinTrcv_GetVersionInfo_ValidPtr_ShouldSucceed 覆盖: LinTrcv_GetVersionInfo_ValidPtr_ShouldSucceed 场景 |
| SWS_LinTrcv_00004 | `LinTrcv_SetTrcvMode` | 测试 test_LinTrcv_SetTrcvMode_ValidCall_ShouldSucceed 覆盖: LinTrcv_SetTrcvMode_ValidCall_ShouldSucceed 场景 |
| SWS_LinTrcv_00005 | `LinTrcv_GetTrcvMode` | 测试 test_LinTrcv_GetTrcvMode_ValidCall_ShouldReturnMode 覆盖: LinTrcv_GetTrcvMode_ValidCall_ShouldReturnMode 场景 |
| SWS_LinTrcv_00006 | `LinTrcv_GetTrcvWakeupReason` | 测试 test_LinTrcv_GetTrcvWakeupReason_ValidCall_ShouldReturnReason 覆盖: LinTrcv_GetTrcvWakeupReason_ValidCall_ShouldReturnReason 场景 |
| SWS_LinTrcv_00007 | `LinTrcv_MainFunction` | 测试 test_LinTrcv_MainFunction_ValidCall_ShouldSucceed 覆盖: LinTrcv_MainFunction_ValidCall_ShouldSucceed 场景 |
