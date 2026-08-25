# EthTrcv Design Document

> **Module ID**: 0x9F  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_EthernetTransceiverDriver  
> **Source Path**: `src/bsw/ecual/ethtrcv/`  
> **Reference Document**: `docs/modules/ethtrcv.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

EthTrcv（Ethernet Transceiver Driver）是 AUTOSAR ECU 抽象层中负责管理以太网物理层收发器（PHY）的驱动模块。该模块支持多种 PHY 芯片型号，提供 PHY 初始化、模式控制、链路状态监控、自动协商、唤醒检测和线缆诊断等功能。

主要功能：
- 多 PHY 收发器管理（最多 ETHTRCV_NUMBER_OF_TRCVS = 2）
- 自动 PHY 检测（通过 PHY ID 寄存器识别芯片型号）
- 支持多种 PHY 芯片：TJA1100/TJA1101（NXP）、RTL8211（Realtek）、LAN8720（SMSC）、KSZ8081（Microchip）、DP83848（TI）等
- 多接口支持：MII/SMI、SPI、I2C PHY 寄存器访问
- 链路状态机（含去抖机制）
- 模式控制（DOWN/ACTIVE/STANDBY/SLEEP）
- 唤醒检测与指示
- 信号质量评估
- 线缆诊断
- PHY 寄存器缓存（32 个寄存器）

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS EthernetTransceiverDriver | 4.4.0 | 以太网收发器驱动规范 |
| IEEE 802.3 | - | 以太网物理层标准 |
| IEEE 802.3u | - | 快速以太网标准 |
| IEEE 802.3ab | - | 千兆以太网标准 |
| IEEE 802.3bw | - | 100BASE-T1 标准 |

### 2.2 模块依赖

| 模块 | 依赖方向 | 说明 |
|------|----------|------|
| Eth | 调用 | 以太网 MCAL，提供 Eth_ReadMii / Eth_WriteMii |
| Spi | 调用 | SPI 驱动（TJA1100 SPI PHY 访问） |
| I2c | 调用 | I2C 驱动（I2C PHY 访问） |
| Det | 调用 | 默认错误追踪 |
| Dem | 调用 | 诊断事件管理 |
| Mcu | 调用 | MCU 驱动（延迟等） |
| Std_Types | 包含 | AUTOSAR 标准类型 |

## 3. 架构设计

### 3.1 分层位置

```
+-----------------------------------+
|       ECUAL (EthSM, EthIf)        |
+-----------------------------------+
|    ECUAL Layer (EthTrcv)          |  <-- 本模块
+-----------------------------------+
|    MCAL (Eth, Spi, I2c)           |
+-----------------------------------+
|    Hardware (PHY: TJA1100/RTL8211)|
+-----------------------------------+
```

### 3.2 内部组件

- **PHY 检测器**：读取 PHYIDR1/PHYIDR2 寄存器，通过 OUI + Model 识别 PHY 类型
- **PHY 访问层**：统一的寄存器读写接口，支持 MII/SPI/I2C 三种访问方式
- **链路状态机**：DOWN → WAIT_UP → UP → WAIT_DOWN，含去抖计数器
- **模式控制器**：PHY 特定模式设置（TJA1100 扩展控制寄存器、RTL8211 BMCR 寄存器）
- **唤醒检测器**：检查 PHY 特定唤醒状态寄存器
- **信号质量评估器**：基于链路失败计数器评估信号质量
- **线缆诊断器**：触发 PHY 线缆测试并解析结果
- **寄存器缓存**：32 个 16 位 PHY 寄存器镜像

### 3.3 文件结构

```
src/bsw/ecual/ethtrcv/
├── include/
│   ├── EthTrcv.h          # 公共 API 头文件
│   ├── EthTrcv_Cfg.h      # 预编译配置头文件
│   └── EthTrcv_MemMap.h   # MemMap 内存分区宏
└── src/
    ├── EthTrcv.c           # 模块实现
    └── EthTrcv_Lcfg.c      # 链接时配置表
```

## 4. 状态机

### 4.1 模块状态

```
    STATE_UNINIT (0x00) ──── Init() ────> STATE_INIT (0x01)
         ^                                    │
         └──────── DeInit() ─────────────────┘
```

### 4.2 链路状态机（每 PHY）

```
    LINK_SM_DOWN (0x00)
         │
    BMSR.Link_Status = 1 (去抖后)
         │
         v
    LINK_SM_WAIT_UP (0x01)
         │
    去抖计数 >= ETHTRCV_LINK_DEBOUNCE_COUNT
         │
         v
    LINK_SM_UP (0x02)
         │
    BMSR.Link_Status = 0
         │
         v
    LINK_SM_WAIT_DOWN (0x03)
         │
    确认链路断开
         │
         v
    LINK_SM_DOWN
```

### 4.3 PHY 访问状态

```
    PHY_STATE_IDLE (0x00)
         │
    读请求 ────> PHY_STATE_READ_PENDING (0x01)
    写请求 ────> PHY_STATE_WRITE_PENDING (0x02)
```

## 5. 核心数据结构

### 5.1 PHY 运行时状态

```c
typedef struct {
    EthTrcv_ModeType CurrentMode;           /* 当前模式 */
    EthTrcv_TypeType DetectedType;          /* 检测到的 PHY 类型 */
    EthTrcv_LinkStateType LinkState;        /* 链路状态 */
    EthTrcv_BaudRateType BaudRate;          /* 协商速率 */
    EthTrcv_DuplexModeType DuplexMode;      /* 协商双工 */
    uint8 LinkStateMachine;                 /* 链路状态机状态 */
    uint8 LinkDebounceCounter;              /* 去抖计数器 */
    uint8 PhyAccessState;                   /* PHY 访问状态 */
    uint16 PhyRegCache[32];                 /* PHY 寄存器缓存 */
    boolean IsInitialized;                  /* 初始化标志 */
    boolean LinkChangePending;              /* 链路变化待处理 */
    uint16 LinkChangeCounter;               /* 链路变化计数器 */
} EthTrcv_TrcvRuntimeType;
```

### 5.2 PHY 配置

```c
typedef struct {
    uint8 TrcvIdx;                          /* 收发器索引 */
    uint8 CtrlIdx;                          /* 控制器索引 */
    uint8 PhyAddress;                       /* MDIO PHY 地址 */
    EthTrcv_TypeType TrcvType;              /* 配置的 PHY 类型 */
    uint8 InterfaceType;                    /* MII/RMII/RGMII */
    uint8 AccessInterface;                  /* MII/SPI/I2C */
    EthTrcv_ModeType DefaultMode;           /* 默认模式 */
    boolean AutoNegotiationEnable;          /* 自协商使能 */
    EthTrcv_BaudRateType FixedSpeed;        /* 固定速率 */
    EthTrcv_DuplexModeType FixedDuplexMode; /* 固定双工 */
    boolean WakeupSupport;                  /* 唤醒支持 */
    uint8 WakeupMode;                       /* 唤醒模式 */
    uint32 WakeupSource;                    /* EcuM 唤醒源 */
    uint16 ResetDelayUs;                    /* PHY 复位延迟 */
    uint16 LinkUpDelayMs;                   /* 链路建立延迟 */
} EthTrcv_TrcvConfigType;
```

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `EthTrcv_Init(CfgPtr)` | 0x01 | 初始化所有收发器 |
| `EthTrcv_DeInit()` | 0x02 | 去初始化 |
| `EthTrcv_SetTransceiverMode(TrcvIdx, CtrlIdx, Mode)` | 0x03 | 设置收发器模式 |
| `EthTrcv_GetTransceiverMode(TrcvIdx, CtrlIdx, *Mode)` | 0x04 | 获取收发器模式 |
| `EthTrcv_GetLinkState(TrcvIdx, CtrlIdx, *LinkStatePtr)` | 0x05 | 获取链路状态 |
| `EthTrcv_GetBaudRate(TrcvIdx, CtrlIdx, *BaudRatePtr)` | 0x06 | 获取速率 |
| `EthTrcv_GetDuplexMode(TrcvIdx, CtrlIdx, *DuplexModePtr)` | 0x07 | 获取双工模式 |
| `EthTrcv_MainFunction()` | 0x08 | 周期处理 |
| `EthTrcv_GetVersionInfo(*VersionInfoPtr)` | 0x09 | 获取版本信息 |
| `EthTrcv_CheckWakeup(WakeupSource)` | 0x0A | 检查唤醒事件 |
| `EthTrcv_ReadMiiIndication(TrcvIdx, RegIdx, *RegValPtr)` | 0x0B | PHY 寄存器读指示 |
| `EthTrcv_WriteMiiIndication(TrcvIdx, RegIdx, RegVal)` | 0x0C | PHY 寄存器写指示 |
| `EthTrcv_GetSignalQuality(TrcvIdx, *SignalQualityPtr)` | 0x0D | 获取信号质量 |
| `EthTrcv_GetCableDiagnosticsResult(TrcvIdx, *ResultPtr)` | 0x0E | 获取线缆诊断结果 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| `EthTrcv_LinkStateChgCbkType(CtrlIdx, LinkState)` | 链路状态变化通知 |
| `EthTrcv_WakeupIndicationCbkType(TrcvIdx)` | 唤醒指示 |
| `EthTrcv_PhyRegReadCbkType(TrcvIdx, RegIdx, RegVal)` | PHY 寄存器读完成 |
| `EthTrcv_PhyRegWriteCbkType(TrcvIdx, RegIdx)` | PHY 寄存器写完成 |

### 6.3 服务 ID 与错误码

**DET 错误码：**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| ETHTRCV_E_NO_ERROR | 0x00 | 无错误 |
| ETHTRCV_E_NOT_INITIALIZED | 0x01 | 未初始化 |
| ETHTRCV_E_INV_TRCV_IDX | 0x02 | 无效收发器索引 |
| ETHTRCV_E_INV_TRCV_MODE | 0x03 | 无效模式 |
| ETHTRCV_E_INV_POINTER | 0x04 | 空指针 |
| ETHTRCV_E_INVALID_PHY_ADDR | 0x05 | 无效 PHY 地址 |
| ETHTRCV_E_INVALID_REG_IDX | 0x06 | 无效寄存器索引 |
| ETHTRCV_E_INVALID_REG_VAL | 0x07 | 无效寄存器值 |
| ETHTRCV_E_TIMEOUT | 0x08 | 超时 |
| ETHTRCV_E_INIT_FAILED | 0x09 | 初始化失败 |
| ETHTRCV_E_ALREADY_INITIALIZED | 0x0A | 重复初始化 |

## 7. 处理流程

### 7.1 初始化流程

1. 检查是否已初始化
2. 保存配置指针
3. 遍历所有收发器：
   a. 清零运行时数据
   b. 读取 PHYIDR1/PHYIDR2，检测 PHY 类型
   c. PHY 复位（BMCR_RESET）
   d. 等待复位完成
   e. 根据检测到的 PHY 类型执行特定初始化

### 7.2 PHY 检测流程

```
读取 PHYIDR1 + PHYIDR2
  │
  ├── 计算 OUI = (PHYIDR1 << 6) | (PHYIDR2 >> 10)
  ├── 计算 Model = (PHYIDR2 >> 4) & 0x3F
  │
  ├── OUI = 0x0001C1 (NXP) → TJA1100/TJA1101
  ├── OUI = 0x0010A0 (Realtek) → RTL8211
  ├── OUI = 0x00005C (Microchip) → LAN8720/KSZ8081
  ├── OUI = 0x080017 (TI) → DP83848
  └── 其他 → GENERIC
```

### 7.3 MainFunction 周期处理

每 10ms 调用一次，遍历所有收发器：
1. 读取 BMSR 更新链路状态（含去抖）
2. 链路建立时读取 PHY 特定寄存器获取速率/双工
3. 处理链路状态变化回调（延迟 2 个周期）
4. 检查唤醒事件（如果支持）

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| ETHTRCV_DEV_ERROR_DETECT | STD_ON | DET 使能 |
| ETHTRCV_VERSION_INFO_API | STD_ON | 版本信息 API |
| ETHTRCV_NUMBER_OF_TRCVS | 2 | 收发器数量 |
| ETHTRCV_MAIN_FUNCTION_PERIOD_MS | 10 | 主函数周期 |
| ETHTRCV_PHY_ACCESS_TIMEOUT_MS | 100 | PHY 访问超时 |
| ETHTRCV_LINK_DEBOUNCE_COUNT | 3 | 链路去抖计数 |
| ETHTRCV_WAKEUP_SUPPORT | STD_ON | 唤醒支持 |
| ETHTRCV_CABLE_DIAGNOSTICS_SUPPORT | STD_ON | 线缆诊断 |
| ETHTRCV_SIGNAL_QUALITY_SUPPORT | STD_ON | 信号质量 |
| ETHTRCV_AUTO_NEGOTIATION_SUPPORT | STD_ON | 自协商 |

### 8.2 链接时配置

配置包含 TRCV0（TJA1100, RMII, 100Mbps）和 TRCV1（RTL8211, RGMII, 1000Mbps）的详细 PHY 参数。

### 8.3 构建后配置

本实现不支持构建后配置。

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 均进行初始化状态、索引范围和指针有效性检查。

### 9.2 DEM 错误

模块包含 Dem.h 头文件引用，可用于上报 PHY 初始化失败等生产错误。

### 9.3 安全机制

- **PHY 复位超时保护**：复位后轮询 BMCR_RESET 位直到清零
- **链路去抖**：连续 3 次检测到链路状态一致才确认变化
- **链路变化回调延迟**：延迟 2 个 MainFunction 周期后触发回调
- **MemMap 分区**：使用 EthTrcv_MemMap.h 进行内存段保护
- **寄存器缓存**：减少重复 PHY 访问，提高性能

## 10. 内存与性能

### 10.1 MemMap 分区

| 段名 | 类型 | 说明 |
|------|------|------|
| ETHTRCV_START_SEC_CONFIG_DATA_UNSPECIFIED | ROM | 配置数据 |
| ETHTRCV_START_SEC_VAR_CLEARED_UNSPECIFIED | RAM (清零) | 运行时变量 |
| ETHTRCV_START_SEC_VAR_INIT_UNSPECIFIED | RAM (初始化) | 初始化变量 |
| ETHTRCV_START_SEC_CODE | RAM | 代码段 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| Trcv[2] 运行时 | ~160 bytes | 每 PHY ~80 bytes（含 32×2=64 bytes 寄存器缓存） |
| 配置表 | ~120 bytes | 只读配置 |
| **总计 RAM** | **~160 bytes** | 运行时变量 |
| **总计 ROM** | **~120 bytes** | 配置常量 |

## 11. 集成指南

### 集成步骤

1. 在 `EthTrcv_Cfg.h` 中配置 PHY 类型、接口类型和访问方式
2. 准备 `EthTrcv_TrcvConfig[]` 配置数组
3. 调用 `EthTrcv_Init(&EthTrcv_Config)` 初始化
4. 将 `EthTrcv_MainFunction()` 加入 10ms 周期任务
5. 注册链路状态变化回调 `EthTrcv_LinkStateChgCallback`
6. 注册唤醒指示回调 `EthTrcv_WakeupIndicationCallback`

### 注意事项

- SPI 和 I2C PHY 访问当前为桩实现（返回 E_NOT_OK）
- TJA1100 的线缆诊断会阻塞等待测试完成（最多 1000 次轮询）
- 信号质量评估仅支持 TJA1100/TJA1101

## 12. 测试策略

### 12.1 单元测试

| 测试场景 | 预期结果 |
|----------|----------|
| Init 检测 PHY 类型 | DetectedType 正确设置 |
| SetTransceiverMode(ACTIVE) | PHY BMCR 寄存器正确配置 |
| SetTransceiverMode(SLEEP) - TJA1100 | 扩展控制寄存器设置 PWR_SLEEP |
| 链路状态去抖 | 去抖计数达到阈值后才更新状态 |
| 链路变化回调 | 延迟 2 个周期后触发 |
| GetBaudRate - RTL8211 | 从 PHYSR 寄存器正确解析速率 |
| CheckWakeup - TJA1100 | 检查 COMM_STATUS 的 REM_WUR 位 |
| GetSignalQuality | 基于 LinkFailCounter 返回质量等级 |

### 12.2 集成测试

| 测试场景 | 验证点 |
|----------|--------|
| EthTrcv + Eth 联合测试 | MII 寄存器读写正确 |
| EthTrcv + EthSM 联合测试 | 链路状态变化触发 EthSM 状态转换 |
| 唤醒功能测试 | 从 Sleep 模式检测唤醒事件 |
| 线缆诊断测试 | 正确识别开路/短路/正常 |

## 13. 实现说明 / TODO

### 当前实现特点

- 支持 10 种 PHY 芯片型号的自动检测
- 三种 PHY 访问接口（MII/SPI/I2C）
- 完整的链路状态机（含去抖）
- PHY 特定模式控制（TJA1100 扩展寄存器、RTL8211 BMCR）
- 信号质量和线缆诊断功能

### 待实现项

- [ ] SPI PHY 访问的实际实现
- [ ] I2C PHY 访问的实际实现
- [ ] 异步 PHY 寄存器访问（当前为同步轮询）
- [ ] 更多 PHY 型号的特定优化
- [ ] 线缆诊断的异步化（避免阻塞）

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| AUTOSAR_SWS_EthernetTransceiverDriver.pdf | AUTOSAR EthTrcv 规范 |
| NXP TJA1100 Datasheet | 100BASE-T1 PHY 数据手册 |
| Realtek RTL8211 Datasheet | 千兆以太网 PHY 数据手册 |
| IEEE 802.3 Clause 22 | MII 寄存器定义 |
| EthTrcv.h | 模块公共接口定义 |
| EthTrcv_Cfg.h | 预编译配置定义 |
| EthTrcv.c | 模块实现源码 |
