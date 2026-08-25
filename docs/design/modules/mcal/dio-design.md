# Dio Design Document

> **Module ID**: 0x01
> **AUTOSAR Layer**: MCAL
> **AUTOSAR Version**: Classic Platform 4.4.0
> **SWS Reference**: AUTOSAR_SWS_Dio
> **Source Path**: `src/bsw/mcal/dio/`
> **Reference Document**: `docs/modules/dio.md`
> **Doc Version**: 1.0
> **Status**: Draft

---

## 1. 模块概述

Dio（Digital Input/Output Driver）位于 MCAL 层，提供对 MCU GPIO 数字通道、端口和通道组的读写服务。模块以直接寄存器访问方式操作 GPIO，强调低开销；引脚方向配置由同层 Port 模块负责。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Dio | 4.4.0 | DIO 软件规范 |
| AUTOSAR Classic Platform | 4.x | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | IoHwAb / Application / 其他 BSW | 数字 IO 读写 |
| 下层 | GPIO HW | 直接寄存器访问 |
| 同层 | Port | 引脚方向/复用配置 |
| 公共 | Det | 开发错误检测 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   IoHwAb / Application / Other BSW  │
├─────────────────────────────────────┤
│           Dio (MCAL)                │
├─────────────────────────────────────┤
│      Port / GPIO Hardware           │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Channel Manager**：按 `ChannelId` 解析 Port/Pin，读写单通道。
- **Port Manager**：读写整个端口。
- **Channel Group Manager**：按掩码和偏移读写通道组。
- **Platform Abstraction**：`Dio_GetGpioBaseAddr` 把逻辑 Port 映射到 GPIO 基址。

### 3.3 文件结构

```
src/bsw/mcal/dio/
├── include/
│   ├── Dio.h
│   └── Dio_Cfg.h
└── src/
    ├── Dio.c
    └── Dio_Lcfg.c
```

---

## 4. 状态机

Dio 本身不维护复杂状态机，仅通过 `Dio_DriverInitialized` 标志驱动初始化状态：

```
UNINIT -- Dio_Init() --> INITIALIZED
```

通道/端口电平直接反映 GPIO 寄存器状态。

---

## 5. 核心数据结构

```c
typedef uint16 Dio_ChannelType;     /* 高 8 bit = Port, 低 8 bit = Pin */
typedef uint8  Dio_PortType;
typedef uint32 Dio_PortLevelType;

typedef enum {
    STD_LOW = 0,
    STD_HIGH = 1
} Dio_LevelType;

typedef struct {
    Dio_PortType      port;
    uint8             offset;
    Dio_PortLevelType mask;
} Dio_ChannelGroupType;

typedef struct {
    uint8 dummy;
} Dio_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 | SWS ID |
|-----|------|------|------|----------|--------|
| Dio_Init | `void Dio_Init(const Dio_ConfigType* ConfigPtr)` | 初始化 Dio 驱动 | 本实现仅标记初始化标志 | SWS_Dio_00001 | SWS_Dio_00001 |
| Dio_ReadChannel | `Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId)` | 读单个通道电平 | | SWS_Dio_00002 | SWS_Dio_00002 |
| Dio_WriteChannel | `void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level)` | 写单个通道电平 | | SWS_Dio_00003 | SWS_Dio_00003 |
| Dio_ReadPort | `Dio_PortLevelType Dio_ReadPort(Dio_PortType PortId)` | 读整个端口 | | SWS_Dio_00004 | SWS_Dio_00004 |
| Dio_WritePort | `void Dio_WritePort(Dio_PortType PortId, Dio_PortLevelType Level)` | 写整个端口 | | SWS_Dio_00005 | SWS_Dio_00005 |
| Dio_ReadChannelGroup | `Dio_PortLevelType Dio_ReadChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr)` | 读通道组 | | SWS_Dio_00006 | SWS_Dio_00006 |
| Dio_WriteChannelGroup | `void Dio_WriteChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr, Dio_PortLevelType Level)` | 写通道组 | | SWS_Dio_00007 | SWS_Dio_00007 |
| Dio_GetVersionInfo | `void Dio_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 版本信息 | | SWS_Dio_00008 | SWS_Dio_00008 |
| Dio_FlipChannel | `Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId)` | 翻转通道电平 | 受 `DIO_FLIP_CHANNEL_API` 控制 | SWS_Dio_00009 | SWS_Dio_00009 |
| Dio_MaskedWritePort | `void Dio_MaskedWritePort(Dio_PortType PortId, Dio_PortLevelType Level, Dio_PortLevelType Mask)` | 按掩码写端口 | 受 `DIO_MASKED_WRITE_PORT_API` 控制 | SWS_Dio_00010 | SWS_Dio_00010 |

### 6.2 回调函数

Dio 模块无回调函数。

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 | SWS ID |
|-----|-----|------------|--------|
| 0x00 | Dio_Init | DIO_E_PARAM_CONFIG | SWS_Dio_00011 |
| 0x01 | Dio_ReadChannel | DIO_E_UNINIT / DIO_E_PARAM_INVALID_CHANNEL_ID | SWS_Dio_00012 |
| 0x02 | Dio_WriteChannel | DIO_E_UNINIT / DIO_E_PARAM_INVALID_CHANNEL_ID | SWS_Dio_00013 |
| 0x03 | Dio_ReadPort | DIO_E_UNINIT / DIO_E_PARAM_INVALID_PORT_ID | SWS_Dio_00014 |
| 0x04 | Dio_WritePort | DIO_E_UNINIT / DIO_E_PARAM_INVALID_PORT_ID | SWS_Dio_00015 |
| 0x05 | Dio_ReadChannelGroup | DIO_E_UNINIT / DIO_E_PARAM_POINTER | SWS_Dio_00016 |
| 0x06 | Dio_WriteChannelGroup | DIO_E_UNINIT / DIO_E_PARAM_POINTER | SWS_Dio_00017 |
| 0x12 | Dio_GetVersionInfo | DIO_E_PARAM_POINTER | SWS_Dio_00018 |
| 0x11 | Dio_FlipChannel | DIO_E_UNINIT / DIO_E_PARAM_INVALID_CHANNEL_ID | SWS_Dio_00019 |
| 0x13 | Dio_MaskedWritePort | DIO_E_UNINIT / DIO_E_PARAM_INVALID_PORT_ID | SWS_Dio_00020 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 `ConfigPtr` 非空，否则报 `DIO_E_PARAM_CONFIG`。
2. 标记 `Dio_DriverInitialized = TRUE`。
3. 配置指针未实际使用（`Dio_ConfigType` 当前为空占位）。

### 7.2 通道读写流程

1. 校验驱动已初始化、通道 ID 合法。
2. `port = ChannelId >> 8`，`pin = ChannelId & 0xFF`。
3. 通过 `Dio_GetGpioBaseAddr(port)` 获取 GPIO 基址。
4. 读：读取 `PDIR`（`DIO_GPIO_PSR`）对应 bit。
5. 写：读取 `PDOR`（`DIO_GPIO_DR`），修改对应 bit，写回 `PDOR`。

### 7.3 端口读写流程

1. 校验初始化与 PortId。
2. 获取 GPIO 基址。
3. 读：直接返回 `PDIR` 值。
4. 写：直接写 `PDOR`。

### 7.4 通道组读写流程

1. 校验初始化与指针。
2. 获取 GPIO 基址。
3. 读：`(PDIR & mask) >> offset`。
4. 写：`PDOR = (PDOR & ~mask) | ((Level << offset) & mask)`。

### 7.5 翻转通道流程

1. 校验后读取 `PDOR`。
2. 若 bit 为 1 则清 0，否则置 1。
3. 写回 `PDOR` 并返回新电平。

### 7.6 按掩码写端口流程

1. 校验后读取 `PDOR`。
2. `PDOR = (PDOR & ~Mask) | (Level & Mask)`。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值/示例 | 说明 |
|----|-------------|------|
| `DIO_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `DIO_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `DIO_NUM_PORTS` | 8U | 逻辑端口数量 |
| `DIO_NUM_CHANNELS_PER_PORT` | 32U | 每端口通道数 |
| `DIO_NUM_CHANNEL_GROUPS` | 4U | 通道组数量 |
| `DIO_FLIP_CHANNEL_API` | STD_ON | FlipChannel API |
| `DIO_MASKED_WRITE_PORT_API` | STD_ON | MaskedWritePort API |
| `DIO_PORT_A~H` | 0~7 | 端口 ID |
| `DIO_CHANNEL_A0~A7` | 0~7 | A 口通道 |
| `DIO_CHANNEL_B0~B7` | 256~263 | B 口通道 |
| ... | ... | 其余端口通道 |
| `DIO_CHANNEL_CAN0_STB/EN/ERR/WAK` | 1280~1283 | 功能命名通道 |
| `DIO_INVALID_CHANNEL` | 65535U | 无效通道 |

### 8.2 链接时配置

`Dio_Lcfg.c` 中定义：

```c
const Dio_ConfigType Dio_Config = { 0U };
const Dio_ConfigType* const Dio_ConfigPtr = &Dio_Config;
```

当前 `Dio_ConfigType` 为空结构体，主要用于保持接口一致性。

### 8.3 构建后配置

当前实现不支持 Post-Build 配置变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x0A | DIO_E_PARAM_INVALID_CHANNEL_ID | 通道 ID 越界 |
| 0x14 | DIO_E_PARAM_INVALID_PORT_ID | 端口 ID 越界 |
| 0x1F | DIO_E_PARAM_INVALID_GROUP | 通道组无效 |
| 0x20 | DIO_E_PARAM_POINTER | 空指针入参 |
| 0x21 | DIO_E_PARAM_CONFIG | 配置指针为空 |
| 0x22 | DIO_E_UNINIT | 模块未初始化 |

### 9.2 DEM 错误

当前未定义 DEM 事件。

### 9.3 安全机制

- 所有写操作采用读-修改-写方式，需注意多核/中断并发下的原子性。
- 通道 ID 编码为 `Port << 8 | Pin`，限制最大 256 个通道/端口。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| `DIO_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化变量（`Dio_DriverInitialized`） |
| `DIO_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | |
| `DIO_START_SEC_CODE` | 代码段 |
| `DIO_STOP_SEC_CODE` | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | 极小 | 仅一个初始化标志 |
| ROM | 小 | 代码与配置表 |
| 执行时间 | 极短 | 直接寄存器访问，无中断开关 |

---

## 11. 集成指南

- 在使用 Dio 之前，先完成 `Port` 模块初始化以配置引脚方向与复用。
- 调用 `Dio_Init(&Dio_Config)` 标记驱动就绪。
- 通道 ID 使用配置工具生成的宏（如 `DIO_CHANNEL_CAN0_STB`）。
- 对于输入通道，确保 `Port` 已配置为输入；输出通道同理。
- 若在中断中调用 Dio API，需自行保证读-修改-写的原子性，或使用硬件原子置位/清零寄存器。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `test_dio.c` | 初始化、通道读写、端口读写、通道组读写、翻转、掩码写、错误检测 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 与 Port 集成 | 验证引脚方向配置后 Dio 读写正确 |
| 与 IoHwAb 集成 | 验证上层通过 Dio 控制 LED/读取按键 |
| 并发测试 | 验证中断与主循环同时访问同一端口的安全性 |

---

## 13. 实现说明 / TODO

- 源码中 `DIO_MODULE_ID` 定义为 `0x29U`，与 AUTOSAR 标准 `0x01` 不一致；建议后续统一。
- `Dio_Init` 为项目扩展，标准 AUTOSAR Dio 通常无 Init API；本实现仅设置初始化标志。
- `Dio_ConfigType` 当前为空结构体，配置数据依赖预编译宏。
- GPIO 基址映射仅覆盖 A~E 五个逻辑端口（对应 GPIO1~GPIO5），F~H 端口映射需补充。
- 当前仅支持 i.MX8M Mini GPIO 寄存器布局与 S32K312 SIUL2 可选布局。

---

## 14. 参考资料

1. AUTOSAR_SWS_DIO.pdf
2. `docs/modules/dio.md`
3. `src/bsw/mcal/dio/`
