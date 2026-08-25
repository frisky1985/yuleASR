# CanTrcv Design Document

> **Module ID**: 0x43  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_CANTransceiverDriver  
> **Source Path**: `src/bsw/ecual/cantrcv/`  
> **Reference Document**: `docs/modules/cantrcv.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

CanTrcv（CAN Transceiver Driver）是 AUTOSAR ECU 抽象层中负责管理 CAN 收发器硬件的驱动模块。该模块通过 DIO 引脚或 SPI 接口控制 CAN 收发器的工作模式（Normal/Standby/Sleep），并管理唤醒事件的检测和上报。

主要功能：
- 多通道 CAN 收发器管理（最多 CANTRCV_MAX_CHANNELS = 2）
- 收发器模式控制（Normal/Standby/Sleep）
- 通过 DIO 引脚（STB、EN、NERR）控制收发器
- 支持多种硬件型号：TJA1043、TJA1042、TLE6250、UJA1168 及通用型
- 唤醒事件检测（总线唤醒、引脚唤醒）
- 与 EcuM 集成，上报唤醒事件
- 支持 SPI 控制的收发器（条件编译）
- 唤醒/错误通知回调

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS CANTransceiverDriver | 4.4.0 | CAN 收发器驱动规范 |
| ISO 11898 | - | CAN 物理层标准 |

### 2.2 模块依赖

| 模块 | 依赖方向 | 说明 |
|------|----------|------|
| Dio | 调用 | DIO 驱动，控制 STB/EN/NERR 引脚 |
| Spi | 调用 | SPI 驱动（条件编译 CANTRCV_SPI_USED） |
| EcuM | 调用 | ECU 状态管理器，上报唤醒事件 |
| Det | 调用 | 默认错误追踪 |
| Std_Types | 包含 | AUTOSAR 标准类型 |
| ComStack_Types | 包含 | 通信栈类型 |

## 3. 架构设计

### 3.1 分层位置

```
+-----------------------------------+
|       BSW Services (CanSM, ComM)  |
+-----------------------------------+
|    ECUAL Layer (CanTrcv)          |  <-- 本模块
+-----------------------------------+
|    MCAL (Dio, Spi)                |
+-----------------------------------+
|    Hardware (TJA1043/TJA1042)     |
+-----------------------------------+
```

### 3.2 内部组件

- **模式控制器**：通过 DIO 引脚控制收发器模式
  - Normal: STB=HIGH（或反相LOW）, EN=HIGH
  - Standby: STB=LOW（或反相HIGH）, EN=HIGH
  - Sleep: STB=LOW（或反相HIGH）, EN=LOW
- **唤醒检测器**：通过 NERR 引脚检测唤醒事件（低电平有效）
- **EcuM 通知器**：检测到唤醒后调用 EcuM_SetWakeupEvent
- **通知回调**：唤醒通知和错误通知回调（条件编译）

### 3.3 文件结构

```
src/bsw/ecual/cantrcv/
├── include/
│   ├── CanTrcv.h          # 公共 API 头文件
│   └── CanTrcv_Cfg.h      # 预编译配置头文件
└── src/
    ├── CanTrcv.c           # 模块实现
    └── CanTrcv_Lcfg.c      # 链接时配置表
```

## 4. 状态机

### 4.1 模块状态

```
    UNINIT ──── Init() ────> INIT
                  │
    DeInit() ─────┘
```

### 4.2 收发器模式状态

```
    SLEEP ←──── Standby ←──── NORMAL
      │            │              │
      └────────────┴──────────────┘
           SetOpMode() 控制
```

**DIO 引脚与模式对应关系（TJA1043/TJA1042）：**

| 模式 | STB 引脚 | EN 引脚 | 说明 |
|------|----------|---------|------|
| NORMAL | HIGH（反相:LOW） | HIGH | 完全通信 |
| STANDBY | LOW（反相:HIGH） | HIGH | 低功耗，可唤醒 |
| SLEEP | LOW（反相:HIGH） | LOW | 最低功耗，可唤醒 |

## 5. 核心数据结构

### 5.1 运行时状态

```c
typedef struct {
    CanTrcv_TrcvModeType CurrentMode;          /* 当前操作模式 */
    CanTrcv_TrcvWakeupModeType WakeupMode;     /* 唤醒模式 */
    CanTrcv_TrcvWakeupReasonType WakeupReason; /* 唤醒原因 */
    boolean WakeupPending;                     /* 唤醒待处理 */
    boolean WakeupByBusEnabled;                /* 总线唤醒使能 */
} CanTrcv_RuntimeType;
```

### 5.2 通道配置

```c
typedef struct {
    uint8 ChannelId;                           /* 通道 ID */
    CanTrcv_HwType TransceiverType;            /* 硬件类型 */
    CanTrcv_PinConfigType PinConfig;           /* 引脚配置 */
    boolean UsesSpi;                           /* SPI 控制 */
    uint8 SpiSequence;                         /* SPI 序列 ID */
    uint8 SpiChannel;                          /* SPI 通道 ID */
    boolean WakeupByBus;                       /* 总线唤醒 */
    boolean WakeupByPin;                       /* 引脚唤醒 */
    uint8 WakeupSource;                        /* EcuM 唤醒源 */
    uint16 ModeTransitionDelay;                /* 模式转换延迟 */
    uint8 DebounceCount;                       /* 去抖计数 */
} CanTrcv_ChannelConfigType;
```

### 5.3 引脚配置

```c
typedef struct {
    Dio_ChannelType StbPin;          /* STB 引脚 */
    Dio_ChannelType EnPin;           /* EN 引脚 */
    Dio_ChannelType ErrPin;          /* NERR/ERR 引脚 */
    boolean         StbPinInverted;  /* STB 是否反相 */
} CanTrcv_PinConfigType;
```

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `CanTrcv_Init(ConfigPtr)` | 0x01 | 初始化模块 |
| `CanTrcv_DeInit()` | 0x02 | 去初始化 |
| `CanTrcv_SetOpMode(Transceiver, OpMode)` | 0x03 | 设置操作模式 |
| `CanTrcv_GetOpMode(Transceiver, *OpMode)` | 0x04 | 获取操作模式 |
| `CanTrcv_GetBusWuReason(Transceiver, *Reason)` | 0x05 | 获取唤醒原因 |
| `CanTrcv_SetWakeupMode(Transceiver, TrcvWakeupMode)` | 0x06 | 设置唤醒模式 |
| `CanTrcv_GetVersionInfo(*versioninfo)` | 0x07 | 获取版本信息 |
| `CanTrcv_MainFunction()` | 0x08 | 周期处理 |
| `CanTrcv_CheckWakeup(WakeupSource)` | 0x09 | 检查唤醒（EcuM 调用） |
| `CanTrcv_CheckWakeupByTransceiver(Transceiver)` | 0x0A | 检查特定收发器唤醒 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| `CanTrcv_WakeupNotification(Transceiver)` | 唤醒通知（条件编译 CANTRCV_WAKEUP_NOTIFICATION_ENABLED） |
| `CanTrcv_ErrorNotification(Transceiver, ErrorCode)` | 错误通知（条件编译 CANTRCV_ERROR_NOTIFICATION_ENABLED） |

### 6.3 服务 ID 与错误码

**DET 错误码：**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| CANTRCV_E_INVALID_TRANSCEIVER | 0x01 | 无效收发器 |
| CANTRCV_E_PARAM_POINTER | 0x02 | 空指针 |
| CANTRCV_E_INVALID_TRCVMODE | 0x03 | 无效模式 |
| CANTRCV_E_INVALID_TRCV_WAKEUP_MODE | 0x04 | 无效唤醒模式 |
| CANTRCV_E_INIT_FAILED | 0x05 | 初始化失败 |
| CANTRCV_E_UNINIT | 0x11 | 未初始化 |
| CANTRCV_E_PARAM_WAKEUPREASON | 0x06 | 无效唤醒原因 |
| CANTRCV_E_INVALID_CONFIGURATION | 0x07 | 无效配置 |

**唤醒原因枚举：**

| 枚举值 | 说明 |
|--------|------|
| CANTRCV_WU_ERROR | 错误 |
| CANTRCV_WU_BY_BUS | 总线唤醒 |
| CANTRCV_WU_BY_PIN | 引脚唤醒 |
| CANTRCV_WU_INTERNALLY | 内部唤醒 |
| CANTRCV_WU_NOT_SUPPORTED | 不支持 |
| CANTRCV_WU_POWER_ON | 上电唤醒 |
| CANTRCV_WU_BY_SYSERR | 系统错误唤醒 |

## 7. 处理流程

### 7.1 初始化流程

1. 检查配置指针有效性
2. 保存配置指针到全局
3. 遍历所有通道：
   a. 设置初始模式为 SLEEP
   b. 设置唤醒模式为 ENABLE
   c. 设置唤醒原因为 NOT_SUPPORTED
   d. 调用 HwSetMode 设置硬件为 SLEEP
4. 标记模块已初始化

### 7.2 模式设置流程

```
SetOpMode(Transceiver, OpMode)
  │
  ├── 验证初始化状态
  ├── 验证通道索引
  ├── 验证模式值
  │
  ├── HwSetMode(channelIndex, Mode)
  │     ├── NORMAL: STB=Active, EN=HIGH
  │     ├── STANDBY: STB=Inactive, EN=HIGH
  │     └── SLEEP: STB=Inactive, EN=LOW
  │
  └── 如果进入 NORMAL 模式
        └── 清除唤醒待处理和唤醒原因
```

### 7.3 MainFunction 周期处理

每 10ms 调用一次，遍历所有通道：
1. 检查 NERR 引脚电平
2. NERR=LOW 且唤醒使能 → 设置唤醒待处理
3. 设置唤醒原因为 WU_BY_BUS
4. 调用 EcuM_SetWakeupEvent 通知 ECU 状态管理器

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| CANTRCV_VERSION_INFO_API | STD_ON | 版本信息 API |
| CANTRCV_DEV_ERROR_DETECT | STD_ON | DET 使能 |
| CANTRCV_MAX_CHANNELS | 2 | 最大通道数 |
| CANTRCV_WAKEUP_BY_BUS_USED | STD_ON | 总线唤醒 |
| CANTRCV_WAKEUP_BY_PIN_USED | STD_ON | 引脚唤醒 |
| CANTRCV_SPI_USED | STD_OFF | SPI 控制 |
| CANTRCV_DIO_USED | STD_ON | DIO 控制 |
| CANTRCV_MODE_TRANSITION_TIMEOUT_MS | 100 | 模式转换超时 |
| CANTRCV_MAIN_FUNCTION_PERIOD_MS | 10 | 主函数周期 |
| CANTRCV_WAKEUP_NOTIFICATION_ENABLED | STD_ON | 唤醒通知 |
| CANTRCV_ERROR_NOTIFICATION_ENABLED | STD_ON | 错误通知 |

### 8.2 链接时配置

链接时配置定义在 `CanTrcv_Lcfg.c` 中，包括：
- CH0: STB=DIO_CHANNEL_10, EN=DIO_CHANNEL_11, NERR=DIO_CHANNEL_12
- CH1: STB=DIO_CHANNEL_20, EN=DIO_CHANNEL_21, NERR=DIO_CHANNEL_22
- 唤醒源: ECUM_WKSOURCE_CAN / ECUM_WKSOURCE_CAN1

### 8.3 构建后配置

本实现不支持构建后配置。

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 均进行：
- 初始化状态检查（CANTRCV_E_UNINIT）
- 通道索引范围检查（CANTRCV_E_INVALID_CHANNEL）
- 指针有效性检查（CANTRCV_E_PARAM_POINTER）
- 模式值范围检查（CANTRCV_E_PARAM_TRCV_OPMODE）

### 9.2 DEM 错误

本实现未集成 DEM 事件上报，但条件编译了 CANTRCV_RUNTIME_ERROR_REPORTING 和 CANTRCV_PROD_ERROR_REPORTING 开关。

### 9.3 安全机制

- **去初始化时安全关闭**：DeInit 将所有收发器设置为 SLEEP 模式
- **STB 引脚反相支持**：通过 StbPinInverted 配置支持不同硬件设计
- **DIO_INVALID_CHANNEL 保护**：检查引脚配置有效性后再操作

## 10. 内存与性能

### 10.1 MemMap 分区

本实现未使用 MemMap 分区宏。

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| CanTrcv_Runtime[2] | ~14 bytes | 每通道 ~7 bytes |
| CanTrcv_InitStatus | 1 byte | 初始化状态 |
| 配置表 | ~60 bytes | 只读配置 |
| **总计 RAM** | **~15 bytes** | 运行时变量 |
| **总计 ROM** | **~60 bytes** | 配置常量 |

## 11. 集成指南

### 集成步骤

1. 在 `CanTrcv_Cfg.h` 中配置通道数、硬件类型和 DIO 引脚
2. 准备 `CanTrcv_ConfigType` 配置结构体
3. 调用 `CanTrcv_Init(&config)` 初始化
4. 将 `CanTrcv_MainFunction()` 加入 10ms 周期任务
5. 配置 EcuM 唤醒源与 CanTrcv 通道的对应关系
6. 实现唤醒/错误通知回调（如果使能）

### 注意事项

- 初始化后所有收发器默认进入 SLEEP 模式
- 进入 NORMAL 模式时自动清除唤醒事件
- NERR 引脚为低电平有效，用于检测唤醒和错误

## 12. 测试策略

### 12.1 单元测试

| 测试场景 | 预期结果 |
|----------|----------|
| Init 后状态检查 | 所有通道为 SLEEP 模式 |
| SetOpMode(NORMAL) | STB/EN 引脚正确设置 |
| SetOpMode(SLEEP) | 所有引脚进入低功耗状态 |
| GetOpMode 查询 | 返回当前运行时模式 |
| 唤醒检测（NERR=LOW） | WakeupPending=TRUE，EcuM 收到事件 |
| SetWakeupMode(DISABLE) | 不再检测唤醒 |
| SetWakeupMode(CLEAR) | 清除唤醒事件 |
| DeInit | 所有通道进入 SLEEP |
| 无效通道索引 | 返回 E_NOT_OK，触发 DET |

### 12.2 集成测试

| 测试场景 | 验证点 |
|----------|--------|
| CanTrcv + CanSM 联合测试 | 模式请求正确传递 |
| CanTrcv + EcuM 联合测试 | 唤醒事件正确上报 |
| CanTrcv + Can 联合测试 | 总线唤醒触发正确 |
| 多通道并发测试 | 两个通道独立运行 |

## 13. 实现说明 / TODO

### 当前实现特点

- 支持 DIO 控制的 TJA1043/TJA1042 收发器
- STB 引脚反相逻辑支持
- 完整的唤醒检测和 EcuM 通知
- 唤醒/错误通知回调

### 待实现项

- [ ] SPI 控制收发器的实际实现
- [ ] 模式转换延迟处理（当前未实现延迟等待）
- [ ] 硬件模式回读验证（HwGetMode 已定义但 MainFunction 中注释）
- [ ] DEM 事件上报
- [ ] MemMap 内存分区
- [ ] UJA1168 System Basis Chip 特定功能

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| AUTOSAR_SWS_CANTransceiverDriver.pdf | AUTOSAR CanTrcv 规范 |
| NXP TJA1043 Datasheet | 高速 CAN 收发器数据手册 |
| NXP TJA1042 Datasheet | 高速 CAN 收发器数据手册 |
| ISO 11898-2 | CAN 物理层标准 |
| CanTrcv.h | 模块公共接口定义 |
| CanTrcv_Cfg.h | 预编译配置定义 |
| CanTrcv.c | 模块实现源码 |
