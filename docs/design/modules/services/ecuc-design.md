# EcuC Design Document

> **Module ID**: 0x03  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_ECUConfiguration  
> **Source Path**: `src/bsw/services/ecuC/`  
> **Reference Document**: `docs/modules/ecuc.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

EcuC (ECU Configuration) 是 AUTOSAR BSW 服务层的 ECU 配置管理模块，负责提供统一的 ECU 硬件和软件配置信息访问接口。EcuC 管理 ECU 的核心频率、总线频率、存储器大小、波特率等硬件参数，以及 PDU 配置、信号定义和网关路由路径。其他 BSW 模块和 SWC 通过 EcuC 获取系统配置信息，实现配置集中管理。

EcuC 模块支持以下核心能力：
- ECU 硬件参数（频率、存储器、波特率）的读写访问
- PDU 和信号配置管理
- 网关路由路径定义（CAN → Ethernet 等跨网络 PDU 路由）
- 信号属性配置（传输属性、方向、位序）

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS ECUConfiguration | 4.4.0 | EcuC 模块规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Com, PduR, CanSM 等 | 查询 ECU 配置参数 |
| 下层 | Det | 开发错误报告 |
| 下层 | EcuM | 初始化阶段调用 EcuC_Init |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   Com / PduR / CanSM / SWC          │
├─────────────────────────────────────┤
│       EcuC (Services Layer)         │
├─────────────────────────────────────┤
│       Det (Error Tracing)           │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Config Value Manager**: 管理 ECU 硬件参数的读写（频率、存储器大小、波特率）
- **PDU/Signal Registry**: 管理 PDU 和信号的静态配置
- **Gateway Router**: 管理跨网络 PDU 路由路径和信号映射

### 3.3 文件结构

```
src/bsw/services/ecuC/
├── include/
│   ├── EcuC.h           # 公共 API 声明、类型定义
│   └── EcuC_Cfg.h       # 预编译配置
└── src/
    ├── EcuC.c            # 核心实现
    └── EcuC_Lcfg.c       # 链接时配置（信号、PDU、路由路径）
```

---

## 4. 状态机

```
          EcuC_Init()
UNINIT ──────────────► INIT
  ▲                      │
  │    EcuC_DeInit()     │
  └──────────────────────┘
```

EcuC 模块有两个状态：
- **ECUC_UNINIT (0)**: 模块未初始化
- **ECUC_INIT (1)**: 模块已初始化，可接受配置访问请求

---

## 5. 核心数据结构

### 5.1 信号配置类型

```c
typedef struct {
    uint16 SignalId;           /* 信号 ID */
    uint16 SignalSize;         /* 信号大小 (bit) */
    uint16 SignalStartBit;     /* 起始位 */
    uint8  SignalBitOrder;     /* 位序 */
    uint8  TransferProperty;   /* 传输属性 (TRIGGERED/TRIGGERED_ON_CHANGE) */
    uint8  Direction;          /* 方向 (SEND/RECEIVE) */
    uint16 RelatedPduId;       /* 关联 PDU ID */
} EcuC_SignalConfigType;
```

### 5.2 PDU 配置类型

```c
typedef struct {
    uint16 PduId;              /* PDU ID */
    uint16 PduLength;          /* PDU 长度 (字节) */
    uint16 SignalCount;        /* 信号数量 */
    const EcuC_SignalConfigType* Signals; /* 信号数组 */
} EcuC_PduConfigType;
```

### 5.3 路由路径类型

```c
typedef struct {
    uint16 SourcePduId;        /* 源 PDU ID */
    uint16 DestinationPduId;   /* 目标 PDU ID */
    uint8  SignalCount;        /* 信号映射数量 */
    const uint16* SignalMapping; /* 信号映射表 */
} EcuC_RoutingPathType;
```

### 5.4 全局配置类型

```c
typedef struct {
    uint32 CoreFrequency;      /* 核心频率 (Hz) */
    uint32 BusFrequency;       /* 总线频率 (Hz) */
    uint32 RamSize;            /* RAM 大小 (字节) */
    uint32 FlashSize;          /* Flash 大小 (字节) */
    uint32 EepromSize;         /* EEPROM 大小 (字节) */
    uint32 CanBaudrate;        /* CAN 波特率 */
    uint32 LinBaudrate;        /* LIN 波特率 */
    uint32 SpiFrequency;       /* SPI 频率 */
    uint16 PduCount;           /* PDU 总数 */
    uint16 SignalCount;        /* 信号总数 */
    uint16 RoutingPathCount;   /* 路由路径数 */
    const EcuC_PduConfigType* Pdus;       /* PDU 配置数组 */
    const EcuC_SignalConfigType* Signals;  /* 信号配置数组 */
    const EcuC_RoutingPathType* RoutingPaths; /* 路由路径数组 */
} EcuC_ConfigType;
```

### 5.5 内部状态类型

```c
typedef struct {
    EcuC_StateType  state;      /* 模块状态 */
    uint8           variant;    /* 配置变体 */
    EcuC_ConfigType activeConfig; /* 活动配置（可修改副本） */
    const EcuC_ConfigType* configPtr; /* 原始配置指针 */
} EcuC_InternalType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | Service ID | 说明 | SWS 需求 |
|-----|-----------|------|----------|
| `void EcuC_Init(const EcuC_ConfigType* ConfigPtr)` | 0x00 | 初始化 EcuC 模块 | SWS_EcuC_00001 |
| `void EcuC_DeInit(void)` | 0x01 | 反初始化 | SWS_EcuC_00002 |
| `Std_ReturnType EcuC_GetConfigValue(uint16 ConfigId, uint32* Value)` | 0x02 | 获取配置值 | SWS_EcuC_00004 |
| `Std_ReturnType EcuC_SetConfigValue(uint16 ConfigId, uint32 Value)` | 0x03 | 设置配置值 | SWS_EcuC_00005 |
| `void EcuC_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 0x04 | 获取版本信息 | SWS_EcuC_00003 |

### 6.2 回调函数

EcuC 不定义回调接口。

### 6.3 服务 ID 与错误码

**Config IDs:**

| ConfigId | 名称 | 值 |
|----------|------|-----|
| ECUC_CONFIG_ID_CORE_FREQ | 核心频率 | 0x01 |
| ECUC_CONFIG_ID_BUS_FREQ | 总线频率 | 0x02 |
| ECUC_CONFIG_ID_RAM_SIZE | RAM 大小 | 0x03 |
| ECUC_CONFIG_ID_FLASH_SIZE | Flash 大小 | 0x04 |
| ECUC_CONFIG_ID_EEPROM_SIZE | EEPROM 大小 | 0x05 |
| ECUC_CONFIG_ID_CAN_BAUD | CAN 波特率 | 0x06 |
| ECUC_CONFIG_ID_LIN_BAUD | LIN 波特率 | 0x07 |

**DET Error Codes:**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| ECUC_E_NO_ERROR | 0x00 | 无错误 |
| ECUC_E_PARAM_POINTER | 0x01 | NULL 指针 |
| ECUC_E_UNINIT | 0x20 | 模块未初始化 |
| ECUC_E_READ_ONLY | 0x40 | 只读配置 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 ConfigPtr 是否为 NULL，若为 NULL 则报告 DET 错误
2. 存储配置指针到内部状态
3. 拷贝配置到 activeConfig（支持运行时修改）
4. 设置 variant = 1
5. 设置状态为 ECUC_INIT

### 7.2 配置值读取流程

1. 检查模块是否已初始化
2. 检查 Value 指针是否为 NULL
3. 根据 ConfigId 从 activeConfig 中读取对应字段
4. 返回 E_OK 或 E_NOT_OK（无效 ConfigId）

### 7.3 配置值写入流程

1. 检查模块是否已初始化
2. 根据 ConfigId 写入 activeConfig 对应字段
3. 支持运行时动态修改硬件参数（如波特率切换）

---

## 8. 配置设计

### 8.1 预编译配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `ECUC_DEV_ERROR_DETECT` | STD_ON | 启用开发错误检测 |
| `ECUC_VERSION_INFO_API` | STD_ON | 启用版本信息 API |
| `ECUC_MAIN_FUNCTION_PERIOD_MS` | 10U | 主函数周期 (ms) |
| `ECUC_MAX_PDUS` | 32U | 最大 PDU 数量 |
| `ECUC_MAX_SIGNALS` | 128U | 最大信号数量 |
| `ECUC_MAX_ROUTING_PATHS` | 16U | 最大路由路径数 |
| `ECUC_MAX_PDU_LENGTH` | 256U | 最大 PDU 长度 |
| `ECUC_MAX_SIGNAL_SIZE` | 64U | 最大信号大小 |
| `ECUC_GATEWAY_DIRECT` | STD_ON | 直接网关使能 |
| `ECUC_SIGNAL_ADAPTATION_ENABLED` | STD_ON | 信号适配使能 |

### 8.2 链接时配置

通过 `EcuC_Lcfg.c` 提供：
- **信号配置**: Engine 信号（RPM、Throttle、CoolantTemp）、Vehicle 信号（Speed、Gear）、Diag 信号（DTC Status）
- **PDU 配置**: 3 个 PDU（Engine PDU、Vehicle PDU、Diag PDU）
- **路由路径**: CAN→Ethernet 网关路由（Engine PDU → Ethernet Engine PDU, Vehicle PDU → Ethernet Vehicle PDU）

### 8.3 构建后配置

不支持构建后配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 场景 | API | 错误码 |
|------|-----|--------|
| ConfigPtr 为 NULL | EcuC_Init | ECUC_E_PARAM_POINTER |
| 模块未初始化时调用 | Get/Set | ECUC_E_UNINIT |
| Value 指针为 NULL | GetConfigValue | ECUC_E_PARAM_POINTER |
| versioninfo 为 NULL | GetVersionInfo | ECUC_E_PARAM_POINTER |

### 9.2 DEM 错误

EcuC 不直接报告 DEM 事件。

### 9.3 安全机制

- 所有公共 API 在 DEV_ERROR_DETECT 启用时进行参数校验
- activeConfig 为配置副本，支持运行时修改而不影响原始配置
- ConfigId 使用 switch-case 分发，无效 ID 返回 E_NOT_OK

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 段 | 变量 | 说明 |
|----|------|------|
| 默认 | EcuC_State | 模块内部状态（含 activeConfig 副本） |
| CODE | 所有函数 | 代码段 |

### 10.2 资源估算

- **RAM**: EcuC_InternalType ≈ sizeof(EcuC_ConfigType) + 8 ≈ 64 字节（活动配置副本）
- **ROM**: ~2 KB（代码段 + Lcfg 配置数据）
- **性能**: GetConfigValue / SetConfigValue 为 O(1) switch-case 操作

---

## 11. 集成指南

- BSW 模块通过 `EcuC_GetConfigValue(ConfigId, &value)` 获取硬件参数
- Com 模块使用 EcuC 获取 PDU 和信号配置
- 网关模块使用 EcuC 的路由路径配置进行跨网络 PDU 转发
- EcuM 在启动序列中调用 `EcuC_Init()` 初始化
- Lcfg 中需定义完整的信号/PDU/路由配置表

---

## 12. 测试策略

### 12.1 单元测试

- 初始化/反初始化测试
- 各 ConfigId 的读写测试
- 无效 ConfigId 返回 E_NOT_OK 测试
- NULL 指针参数测试
- 未初始化状态下的 API 调用测试

### 12.2 集成测试

- EcuC 配置 → Com PDU 路由完整链路
- 网关路由路径正确性验证
- 运行时配置修改对下游模块的影响

---

## 13. 实现说明 / TODO

- 当前 `EcuC_SetConfigValue` 允许修改所有参数，生产版本应对只读参数（如 FlashSize）返回 ECUC_E_READ_ONLY
- Lcfg 中的路由路径 SignalMapping 使用指针数组，需确保链接时地址正确
- 当前 Module ID 在 EcuC.h 中定义为 0x13U，在 EcuC_Cfg.h 中定义为 150U，存在不一致需统一

---

## 14. 参考资料

- AUTOSAR_SWS_ECUConfiguration.pdf (R4.4.0)
- yuleASR EcuC 模块源码: `src/bsw/services/ecuC/`
