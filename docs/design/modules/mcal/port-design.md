# Port Design Document

> **Module ID**: 0x02  
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Port  
> **Source Path**: `src/bsw/mcal/Port/`  
> **Reference Document**: `docs/modules/Port.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Port Driver 是 MCAL 层最底层驱动之一，负责微控制器引脚的初始化与运行时配置，包括：

- 引脚复用（MUX）配置：GPIO、CAN、SPI、UART、I2C、PWM、ADC 等。
- 引脚电气属性配置：slew rate、驱动能力、上拉/下拉、开漏等。
- GPIO 方向与初始电平设置。
- 运行时方向/模式刷新与修改。

上游调用方为 ECUAL 层的 Dio、ICU、PWM 等模块，或直接与 RTE/应用层交互；下层直接访问 SoC IOMUXC/GPIO 寄存器。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS Port | 4.4.0 | Port Driver 软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | Dio、Pwm、Spi、Uart、Can 等 | 使用已配置引脚 | |
| 下层 | SoC IOMUXC/GPIO 寄存器 | i.MX8M Mini / S32K312 平台 | |
| 同层 | Mcu | 时钟使能（由 Mcu 模块统一负责） | |
| 公共 | Det | 开发错误检测（可选） | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│  RTE / Application / ECUAL (Dio...) │
├─────────────────────────────────────┤
│           Port (MCAL)               │
├─────────────────────────────────────┤
│      SoC IOMUXC / GPIO / SIUL2      │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Pin Mux 配置**：根据 `Port_PinModeType` 写 IOMUXC `SW_MUX_CTL_PAD` 寄存器。
- **Pad 属性配置**：配置 `SW_PAD_CTL_PAD` 的 slew rate、驱动强度、上下拉等。
- **GPIO 方向/电平配置**：针对 GPIO 模式设置方向与初始电平。
- **运行时管理**：提供 `SetPinDirection`、`SetPinMode`、`RefreshPortDirection`。

### 3.3 文件结构

```
src/bsw/mcal/Port/
├── include/
│   ├── Port.h
│   └── Port_Cfg.h
└── src/
    ├── Port.c
    └── Port_Lcfg.c
```

---

## 4. 状态机

模块级状态由 `Port_DriverState` 维护：

```
[UNINIT] -- Port_Init() --> [INITIALIZED]
[INITIALIZED] -- Port_DeInit() --> [UNINIT]
```

每个引脚无独立运行时状态机；运行时可变属性由配置表中的 `DirectionChangeable` / `ModeChangeable` 控制。

---

## 5. 核心数据结构

| 类型 | 说明 | |
|------|------|
| `Port_PinType` | `uint16`，引脚全局 ID，格式为 `(port << 8) \| pinNum` | |
| `Port_PinDirectionType` | `PORT_PIN_IN` / `PORT_PIN_OUT` | |
| `Port_PinModeType` | `uint8`，引脚复用模式 | |
| `Port_PinLevelType` | `PORT_PIN_LEVEL_LOW` / `PORT_PIN_LEVEL_HIGH` | |
| `Port_PinConfigType` | 单引脚配置：Pin、Direction、Mode、可变性、初始电平、上下拉 | |
| `Port_ConfigType` | 全局配置：`NumPins` + `PinConfigs` 数组 | |

```c
typedef struct {
    Port_PinType Pin;
    Port_PinDirectionType Direction;
    Port_PinModeType Mode;
    boolean DirectionChangeable;
    boolean ModeChangeable;
    Port_PinLevelType InitialLevel;
    boolean PullUpEnable;
    boolean PullDownEnable;
} Port_PinConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|----------------|----------|
| `Port_Init` | `void Port_Init(const Port_ConfigType* ConfigPtr)` | 初始化 Port 驱动 | 必须先调用 | SWS_Port_00001 |
| `Port_DeInit` | `void Port_DeInit(void)` | 反初始化 | 受 `PORT_DE_INIT_API` 控制 | SWS_Port_00002 |
| `Port_SetPinDirection` | `void Port_SetPinDirection(Port_PinType Pin, Port_PinDirectionType Direction)` | 运行时设置方向 | 受 `PORT_SET_PIN_DIRECTION_API` 控制 | SWS_Port_00003 |
| `Port_RefreshPortDirection` | `void Port_RefreshPortDirection(void)` | 按配置刷新所有 GPIO 方向 | 安全相关 | SWS_Port_00004 |
| `Port_GetVersionInfo` | `void Port_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 获取版本信息 | 受 `PORT_VERSION_INFO_API` 控制 | SWS_Port_00005 |
| `Port_SetPinMode` | `void Port_SetPinMode(Port_PinType Pin, Port_PinModeType Mode)` | 运行时设置引脚模式 | 受 `PORT_SET_PIN_MODE_API` 控制 | SWS_Port_00006 |

### 6.2 回调函数

本模块无回调函数。

### 6.3 Service ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | `Port_Init` | `PORT_E_PARAM_CONFIG`、`PORT_E_ALREADY_INITIALIZED` | |
| 0x01 | `Port_SetPinDirection` | `PORT_E_UNINIT`、`PORT_E_PARAM_PIN`、`PORT_E_DIRECTION_UNCHANGEABLE` | |
| 0x02 | `Port_RefreshPortDirection` | `PORT_E_UNINIT` | |
| 0x03 | `Port_GetVersionInfo` | `PORT_E_PARAM_POINTER` | |
| 0x04 | `Port_SetPinMode` | `PORT_E_UNINIT`、`PORT_E_PARAM_PIN`、`PORT_E_PARAM_INVALID_MODE`、`PORT_E_MODE_UNCHANGEABLE` | |
| 0x05 | `Port_DeInit` | `PORT_E_UNINIT` | |

| 错误码 | 名称 | 说明 | |
|--------|------|------|
| 0x0A | `PORT_E_PARAM_PIN` | 引脚 ID 越界 | |
| 0x0B | `PORT_E_DIRECTION_UNCHANGEABLE` | 该引脚方向不可变 | |
| 0x0C | `PORT_E_PARAM_CONFIG` | 配置指针为空 | |
| 0x0D | `PORT_E_PARAM_INVALID_MODE` | 无效模式 | |
| 0x0E | `PORT_E_MODE_UNCHANGEABLE` | 该引脚模式不可变 | |
| 0x0F | `PORT_E_UNINIT` | 模块未初始化 | |
| 0x10 | `PORT_E_PARAM_POINTER` | 空指针参数 | |
| 0x11 | `PORT_E_ALREADY_INITIALIZED` | 重复初始化 | |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 `ConfigPtr` 非空与未初始化状态（DET 开启时）。
2. 遍历 `PinConfigs`：
   - 配置 IOMUXC MUX 寄存器；
   - 配置 Pad 寄存器（slew rate、驱动强度、上下拉）；
   - 若为 GPIO 模式，设置 `GDIR` 方向与 `DR` 初始电平。
3. 保存配置指针，置 `initialized = TRUE`。

### 7.2 运行时方向/模式修改流程

1. 检查模块已初始化、引脚 ID 有效。
2. 在配置表中查找对应 `PinConfig`，确认 `DirectionChangeable`/`ModeChangeable` 为 TRUE。
3. 更新 GPIO `GDIR` 或 IOMUXC MUX 寄存器。

### 7.3 Refresh 流程

1. 检查初始化状态。
2. 遍历所有配置引脚，仅对 GPIO 模式引脚重新写 `GDIR` 为配置方向。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 | |
|----|--------|------|
| `PORT_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 | |
| `PORT_VERSION_INFO_API` | STD_ON | 版本信息 API | |
| `PORT_SET_PIN_DIRECTION_API` | STD_ON | 方向修改 API | |
| `PORT_SET_PIN_MODE_API` | STD_ON | 模式修改 API | |
| `PORT_DE_INIT_API` | STD_ON | 反初始化 API | |
| `PORT_NUM_PORTS` | 8U | 端口数量 | |
| `PORT_NUM_PINS_PER_PORT` | 32U | 每端口引脚数 | |
| `PORT_TOTAL_NUM_PINS` | 256U | 总引脚数 | |

### 8.2 引脚模式宏

| 宏 | 值 | 说明 | |
|----|----|------|
| `PORT_PIN_MODE_GPIO` | 0 | GPIO 模式 | |
| `PORT_PIN_MODE_CAN` | 1 | CAN 复用 | |
| `PORT_PIN_MODE_SPI` | 2 | SPI 复用 | |
| `PORT_PIN_MODE_UART` | 3 | UART 复用 | |
| `PORT_PIN_MODE_I2C` | 4 | I2C 复用 | |
| `PORT_PIN_MODE_PWM` | 5 | PWM 复用 | |
| `PORT_PIN_MODE_ADC` | 6 | ADC 复用 | |
| `PORT_PIN_MODE_ETH` | 7 | ETH 复用 | |
| `PORT_PIN_MODE_USB` | 8 | USB 复用 | |
| `PORT_PIN_MODE_FLEXIO` | 9 | FlexIO 复用 | |
| `PORT_PIN_MODE_DISABLED` | 15 | 禁用 | |

### 8.3 链接时配置

`Port_Lcfg.c` 提供 `const Port_ConfigType Port_Config`，当前为占位结构 `{ 0U }`，实际项目需由配置工具生成完整引脚表。

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 在 `PORT_DEV_ERROR_DETECT == STD_ON` 时调用 `Det_ReportError`。

| 错误码 | 触发场景 | |
|--------|----------|
| `PORT_E_PARAM_CONFIG` | `Port_Init` 空指针 | |
| `PORT_E_ALREADY_INITIALIZED` | 重复初始化 | |
| `PORT_E_UNINIT` | 未初始化调用其他 API | |
| `PORT_E_PARAM_PIN` | 引脚 ID 超出 `PORT_TOTAL_NUM_PINS` | |
| `PORT_E_DIRECTION_UNCHANGEABLE` | 尝试修改不可变方向 | |
| `PORT_E_MODE_UNCHANGEABLE` | 尝试修改不可变模式 | |
| `PORT_E_PARAM_INVALID_MODE` | 模式值超过 `PORT_PIN_MODE_DISABLED` | |
| `PORT_E_PARAM_POINTER` | `GetVersionInfo` 空指针 | |

### 9.2 DEM 错误

本模块未使用 DEM。

### 9.3 安全机制

- `Port_RefreshPortDirection` 用于安全关键场景下恢复配置方向。
- 建议在 EcuM 初始化早期调用 `Port_Init`，确保外设使用前引脚状态已固定。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 | |
|------|------|
| `PORT_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化运行态变量 `Port_DriverState` | |
| `PORT_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据 `Port_Config` | |
| `PORT_START_SEC_CODE` | 代码段 | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | ~8 B | `Port_DriverState` | |
| ROM | 配置表大小 + 代码 | 与引脚数量成正比 | |
| 堆栈 | 中等 | 初始化时遍历配置表 | |

---

## 11. 集成指南

- 与 Mcu 集成：Port 不直接开关时钟，依赖 Mcu 初始化后 IOMUXC/GPIO 时钟可用。
- 与 Dio 集成：Dio 读取/写入 GPIO 数据寄存器，Port 负责方向与复用配置。
- 初始化顺序：Mcu -> Port -> Dio/Pwm/Spi/Uart 等依赖引脚的模块。
- 配置工具：应由 yuleASR Configurator 生成 `Port_Lcfg.c` 与 `Port_Cfg.h`。

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 覆盖内容 | |
|--------|----------|
| 初始化 | 空配置、重复初始化、正常初始化 | |
| 方向/模式设置 | 可变与不可变引脚 | |
| Refresh | 刷新后方向恢复 | |
| DET | 各错误码触发路径 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 外设引脚复用 | 验证 SPI/CAN/UART 等复用模式生效 | |
| GPIO 输入输出 | 配合 Dio 完成读写 | |
| 安全刷新 | 运行中调用 Refresh 恢复方向 | |

---

## 13. 实现说明 / TODO

- **Module ID 差异**：本模块头文件中定义 `PORT_MODULE_ID` 为 `0x002A`（十进制 42），与 AUTOSAR 标准 Port Module ID `0x02` 不一致。设计文档按项目约定使用 `0x02`，实际代码需统一。
- **Lcfg 占位**：`Port_Lcfg.c` 中 `Port_Config` 为 `{ 0U }`，未包含真实引脚配置，需由配置工具生成。
- **平台适配**：代码同时包含 i.MX8M Mini IOMUXC 与 S32K312 SIUL2 的宏定义，实际编译通过 `S32K312` 宏区分。
- **重复定义**：`Port.c` 中 `PORT_IOMUXC_SW_MUX_CTL_PAD_OFFSET`、`PORT_MUX_MODE_GPIO` 等存在条件编译内外重复定义，建议清理。

---

## 14. 参考资料

1. AUTOSAR_SWS_Port.pdf
2. `docs/modules/Port.md`
3. `src/bsw/mcal/Port/`
