# Can Design Document

> **Module ID**: 0x14
> **AUTOSAR Layer**: MCAL
> **AUTOSAR Version**: Classic Platform 4.4.0
> **SWS Reference**: AUTOSAR_SWS_Can
> **Source Path**: `src/bsw/mcal/can/`
> **Reference Document**: `docs/modules/CAN.md`
> **Doc Version**: 1.0
> **Status**: Draft

---

## 1. 模块概述

Can（CAN Driver）位于 MCAL 层，负责 MCU CAN 控制器（FlexCAN）的初始化、模式切换、报文发送/接收、中断使能/禁用以及 Bus-Off/Wakeup 等事件处理。模块向上通过 CanIf 与上层通信栈交互，向下操作 FlexCAN 硬件寄存器。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Can | 4.4.0 | CAN 软件规范 |
| AUTOSAR Classic Platform | 4.x | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | CanIf | 发送确认、接收指示、Bus-Off 通知 |
| 下层 | Mcu | 时钟使能 |
| 同层 | - | |
| 公共 | Det | 开发错误检测 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│     CanIf / CanSM / Com / PDU-R     │
├─────────────────────────────────────┤
│           Can (MCAL)                │
├─────────────────────────────────────┤
│        FlexCAN Controller HW        │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Controller Manager**：维护各 CAN 控制器状态（UNINIT/STOPPED/STARTED/SLEEP）。
- **Message Buffer Manager**：初始化并管理最多 `CAN_NUM_HOH` 个硬件报文缓冲区（MB）。
- **Tx/Rx Processor**：`Can_Write` 与 `Can_MainFunction_Read/Write` 中处理收发。
- **Event Processor**：`Can_MainFunction_BusOff`、`Can_CheckWakeup` 检测 Bus-Off/Wakeup。
- **Interrupt Controller**：`Can_Enable/DisableControllerInterrupts` 控制 `IMASK1/2`。

### 3.3 文件结构

```
src/bsw/mcal/can/
├── include/
│   ├── Can.h
│   └── Can_Cfg.h
└── src/
    ├── Can.c
    └── Can_Lcfg.c
```

---

## 4. 状态机

### 4.1 控制器状态

```
UNINIT -- Can_Init() --> STOPPED
STOPPED -- STARTED --> STARTED
STARTED -- STOPPED --> STOPPED
STOPPED -- SLEEP --> 不支持（返回 CAN_NOT_OK）
```

---

## 5. 核心数据结构

```c
typedef enum {
    CAN_CS_UNINIT = 0,
    CAN_CS_STARTED,
    CAN_CS_STOPPED,
    CAN_CS_SLEEP
} Can_ControllerStateType;

typedef enum {
    CAN_HOH_TYPE_RECEIVE = 0,
    CAN_HOH_TYPE_TRANSMIT
} Can_HohTypeType;

typedef enum {
    CAN_ID_TYPE_STANDARD = 0,
    CAN_ID_TYPE_EXTENDED
} Can_IdTypeType;

typedef enum {
    CAN_OK = 0,
    CAN_NOT_OK,
    CAN_BUSY
} Can_ReturnType;

typedef uint16 Can_HwHandleType;
typedef uint32 Can_IdType;

typedef struct {
    Can_IdType      CanId;
    Can_HwHandleType Hoh;
    uint8           ControllerId;
} Can_HwType;

typedef struct {
    Can_IdTypeType idType;
    uint32 CanId;
    uint8  CanDlc;
    const uint8* SduPtr;
} Can_PduType;

typedef struct {
    Can_HwHandleType Hoh;
    Can_HohTypeType  HohType;
    Can_IdTypeType   IdType;
    uint32 FirstId;
    uint32 LastId;
    uint8  ObjectId;
    boolean Filtering;
} Can_HardwareObjectType;

typedef struct {
    uint32 BaudRate;
    uint32 PropSeg;
    uint32 PhaseSeg1;
    uint32 PhaseSeg2;
    uint32 SyncJumpWidth;
    uint32 Prescaler;
} Can_BaudrateConfigType;

typedef struct {
    uint8  ControllerId;
    uint32 BaseAddress;
    const Can_BaudrateConfigType* BaudrateConfigs;
    uint8  NumBaudrateConfigs;
    const Can_HardwareObjectType* HardwareObjects;
    uint8  NumHardwareObjects;
    uint32 RxProcessing;
    uint32 TxProcessing;
    boolean BusOffProcessing;
    boolean WakeupProcessing;
    boolean WakeupSupport;
    uint8  DefaultBaudrateIndex;
} Can_ControllerConfigType;

typedef struct {
    const Can_ControllerConfigType* Controllers;
    uint8  NumControllers;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
} Can_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| Can_Init | `void Can_Init(const Can_ConfigType* Config)` | 初始化 CAN 驱动 | | SWS_Can_00001 |
| Can_GetVersionInfo | `void Can_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 版本信息 | | SWS_Can_00002 |
| Can_SetControllerMode | `Can_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition)` | 控制器模式切换 | 仅支持 STARTED/STOPPED | SWS_Can_00003 |
| Can_DisableControllerInterrupts | `void Can_DisableControllerInterrupts(uint8 Controller)` | 关闭中断 | | SWS_Can_00004 |
| Can_EnableControllerInterrupts | `void Can_EnableControllerInterrupts(uint8 Controller)` | 使能中断 | 仅 BusOff/ERR 掩码 | SWS_Can_00005 |
| Can_Write | `Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo)` | 发送报文 | 标准/扩展 ID 自动处理 | SWS_Can_00006 |
| Can_MainFunction_Write | `void Can_MainFunction_Write(void)` | 轮询发送完成 | | SWS_Can_00007 |
| Can_MainFunction_Read | `void Can_MainFunction_Read(void)` | 轮询接收完成 | | SWS_Can_00008 |
| Can_MainFunction_BusOff | `void Can_MainFunction_BusOff(void)` | 轮询 Bus-Off | | SWS_Can_00009 |
| Can_MainFunction_Wakeup | `void Can_MainFunction_Wakeup(void)` | Wakeup 处理 | 当前为空 | SWS_Can_00010 |
| Can_MainFunction_Mode | `void Can_MainFunction_Mode(void)` | 模式过渡处理 | 当前为空 | SWS_Can_00011 |
| Can_CheckWakeup | `Std_ReturnType Can_CheckWakeup(uint8 Controller)` | 检查 Wakeup 事件 | | SWS_Can_00012 |

### 6.2 回调函数

当前实现中 `CanIf_TxConfirmation`、`CanIf_RxIndication`、`CanIf_ControllerBusOff` 为注释掉的调用点，需上层提供并在集成时解注释或替换为实际接口。

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | Can_Init | CAN_E_PARAM_POINTER / CAN_E_TRANSITION |
| 0x01 | Can_GetVersionInfo | CAN_E_PARAM_POINTER |
| 0x03 | Can_SetControllerMode | CAN_E_UNINIT / CAN_E_PARAM_CONTROLLER / CAN_E_TRANSITION |
| 0x04/0x05 | Disable/Enable Controller Interrupts | CAN_E_UNINIT / CAN_E_PARAM_CONTROLLER |
| 0x06~0x08 | Can_Write（使用旧 SID） | CAN_E_UNINIT / CAN_E_PARAM_POINTER / CAN_E_PARAM_HANDLE |
| 0x09~0x0D | MainFunction 系列 | 无 DET |
| 0x0E | Can_CheckWakeup | CAN_E_UNINIT / CAN_E_PARAM_CONTROLLER |

---

## 7. 处理流程

### 7.1 初始化流程

1. 校验配置指针与重复初始化。
2. 保存 `Can_ConfigPtr`。
3. 对每个控制器：
   - 状态置 `CAN_CS_UNINIT`；
   - 使能时钟（stub）；
   - 写 `CAN_MCR=0` 使能模块；
   - 置 `HALT+FRZ` 进入 Freeze Mode，等待 `FRZACK`；
   - 配置最大 MB 数 `MAXMB = CAN_NUM_HOH-1`；
   - 使用首个 BaudrateConfig 计算 `PRESDIV/RJW/PSEG1/PSEG2/PROPSEG` 并写入 `CAN_CTRL1`；
   - 初始化所有 MB 为 TX inactive；
   - 根据 `BusOffProcessing/WakeupProcessing` 设置 `IMASK1`；
   - 状态置 `CAN_CS_STOPPED`。
4. `Can_DriverInitialized = TRUE`。

### 7.2 发送流程（Can_Write）

1. 校验初始化、指针、HTH 索引。
2. 根据 `CAN_NUM_HOH / CAN_NUM_CONTROLLERS` 计算控制器与 MB 索引。
3. 检查 MB 状态为 `TX_INACTIVE`，否则返回 `CAN_BUSY`。
4. 根据 ID 类型写 ID 到 MB+4。
5. 拷贝数据到 MB+8 / MB+12。
6. 写 Control/Status：code=TX_ACTIVE + DLC。
7. 若定义 `QEMU_CAN_LOOPBACK`，直接调用 `CanIf_RxIndication`。
8. 返回 `CAN_OK`。

### 7.3 接收流程（Can_MainFunction_Read）

1. 遍历所有控制器，仅处理 `STARTED` 状态。
2. 读 `CAN_IFLAG1`。
3. 对后半部分 MB（默认用于接收）检查标志位。
4. 读取 MB 数据并清除标志（调用 `CanIf_RxIndication` 的代码当前被注释）。

### 7.4 Bus-Off 处理

1. 遍历 `STARTED` 控制器。
2. 读 `CAN_ESR1`。
3. 若 `BOFFINT` 置位，清除标志并通知 `CanIf_ControllerBusOff`（注释中）。

### 7.5 模式切换流程

- **STARTED**：清 `MCR.HALT`，等待 `NOT_RDY` 清零，状态改为 `CAN_CS_STARTED`。
- **STOPPED**：置 `MCR.HALT+FRZ`，等待 `FRZACK`，状态改为 `CAN_CS_STOPPED`。
- **SLEEP**：返回 `CAN_NOT_OK`。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值/示例 | 说明 |
|----|-------------|------|
| `CAN_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `CAN_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `CAN_NUM_CONTROLLERS` | 2U | 控制器数量 |
| `CAN_NUM_HOH` | 16U | 硬件对象总数 |
| `CAN_NUM_BAUDRATE_CONFIGS` | 3U | 波特率配置数 |
| `CAN_BAUDRATE_500K/250K/125K` | 0/1/2 | 波特率索引 |
| `CAN_PROCESSING_INTERRUPT` | 0U | 中断处理模式 |
| `CAN_PROCESSING_POLLING` | 1U | 轮询处理模式 |
| `CAN_CONTROLLER_0/1` | 0/1 | 控制器 ID |
| `CAN_HOH_RX_0~3 / CAN_HOH_TX_0~3` | 0~7 | HTH/HRH 句柄 |
| `CAN_TIMEOUT_DURATION` | 10000U | 超时计数 |
| `CAN_MAIN_FUNCTION_PERIOD_MS` | 10U | 主函数周期 |

### 8.2 链接时配置

`Can_Lcfg.c` 中定义：

```c
const Can_ConfigType Can_Config = { 0U };
const Can_ConfigType* const Can_ConfigPtr = &Can_Config;
```

当前生成的链接时配置为零填充，完整配置需由配置工具补充。

### 8.3 构建后配置

当前实现以预编译/链接时配置为主，未启用 Post-Build 变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x01 | CAN_E_PARAM_POINTER | 空指针入参 |
| 0x02 | CAN_E_PARAM_HANDLE | HTH/HRH 句柄无效 |
| 0x03 | CAN_E_PARAM_DLC | 数据长度无效 |
| 0x04 | CAN_E_PARAM_CONTROLLER | 控制器索引越界 |
| 0x05 | CAN_E_UNINIT | 模块未初始化 |
| 0x06 | CAN_E_TRANSITION | 模式切换非法或重复初始化 |
| 0x07 | CAN_E_PARAM_BAUDRATE | 波特率索引无效 |
| 0x08 | CAN_E_ICOM_CONFIG_INVALID | ICOM 配置无效 |
| 0x09 | CAN_E_INIT_FAILED | 初始化失败 |
| 0x0A | CAN_E_FATAL | 致命错误 |

### 9.2 DEM 错误

当前未定义 DEM 事件。

### 9.3 安全机制

- 所有 API 在 `CAN_DEV_ERROR_DETECT == STD_ON` 时进行参数校验。
- 模式切换前检查当前状态，避免非法迁移。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| `CAN_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据段（`Can_Config`） |
| `CAN_STOP_SEC_CONFIG_DATA_UNSPECIFIED` | |
| `CAN_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化变量（驱动状态、控制器状态、配置指针） |
| `CAN_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | |
| `CAN_START_SEC_CODE` | 代码段 |
| `CAN_STOP_SEC_CODE` | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~`CAN_NUM_CONTROLLERS * sizeof(state) + 指针` | 状态数组 |
| ROM | 配置表 + 代码 | 随 HOH 数量增长 |
| 堆栈 | 较小 | 无递归 |

---

## 11. 集成指南

- 在 `Mcu` 初始化后调用 `Can_Init(&Can_Config)`。
- 调用 `Can_SetControllerMode(CAN_CONTROLLER_x, CAN_CS_STARTED)` 启动控制器。
- 上层 CanIf 需提供 `CanIf_TxConfirmation`、`CanIf_RxIndication`、`CanIf_ControllerBusOff`。
- 若使用中断模式，需在 `Can_EnableControllerInterrupts` 后安装对应 NVIC 向量；当前默认仅使能 BusOff/ERR 中断掩码。
- 当使用 QEMU 回环时定义 `QEMU_CAN_LOOPBACK`。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `test_can.c` | 初始化、模式切换、发送、参数校验、版本信息 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 与 CanIf 集成 | 验证 TxConfirmation / RxIndication / BusOff 回调 |
| 与 CanSM 集成 | 验证 STARTED/STOPPED 状态切换 |
| 回环测试 | 定义 `QEMU_CAN_LOOPBACK` 验证自发自收 |

---

## 13. 实现说明 / TODO

- 源码中 `CAN_MODULE_ID` 定义为 `0x50U`，与 AUTOSAR 标准 `0x14` 不一致；建议后续统一。
- `Can_EnableClock`/`Can_DisableClock` 为平台占位实现。
- `CAN_CS_SLEEP` 模式未实现。
- `Can_MainFunction_Read/Write/BusOff` 中对 CanIf 的回调调用当前被注释，集成时需打开。
- `Can_MainFunction_Wakeup`/`Can_MainFunction_Mode` 为空函数，需按项目需求补充。
- 当前 HTH/HRH 到 MB 的映射为简单平均分法，需与配置工具保持一致。

---

## 14. 参考资料

1. AUTOSAR_SWS_CAN.pdf
2. `docs/modules/CAN.md`
3. `src/bsw/mcal/can/`
