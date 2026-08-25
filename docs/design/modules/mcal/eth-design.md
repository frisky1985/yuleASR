# Eth Design Document

> **Module ID**: 0x09  
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_EthernetDriver  
> **Source Path**: `src/bsw/mcal/eth/`  
> **Reference Document**: `docs/modules/ETH.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Eth（Ethernet Driver）位于 MCAL 层，负责 Ethernet MAC 控制器的初始化、控制、帧收发管理以及 PHY MII/RMII 寄存器访问。该模块向上层（EthIf / TcpIp / SoAd 等）提供统一的以太网数据链路层服务，支持 10/100/1000 Mbps 速率、全双工/半双工模式、MAC 地址过滤与中断管理。

主要上下游模块：
- 上层：EthIf（Ethernet Interface）
- 同层/公共：Det（开发错误检测）、Dem（诊断事件，可选）、StbM（时间同步，通过 `Eth_GetCurrentTime`）
- 下层：MCU 时钟、Port/Pin 复用、DMA、中断控制器

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Ethernet Driver | 4.4.0 | 以太网驱动软件规范 |
| AUTOSAR Classic Platform | 4.x | 经典平台 |
| IEEE 802.3 | - | 以太网帧格式与 PHY 寄存器定义 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | EthIf | 以太网接口层，调用 Eth 进行收发 |
| 下层 | MCU / Port | 时钟、引脚、中断路由 |
| 同层 | Det | 开发错误检测（可选） |
| 公共 | StbM | 时间戳接口（可选） |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│           Upper Layers              │
│      EthIf / TcpIp / SoAd           │
├─────────────────────────────────────┤
│           Eth (MCAL)                │
├─────────────────────────────────────┤
│     MCU / Port / DMA / Interrupt    │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **控制器管理子组件**：负责 MAC 控制器初始化、模式切换（DOWN / ACTIVE）、MAC 地址设置。
- **缓冲区管理子组件**：维护 TX/RX 描述符与缓冲区池，支持 `Eth_ProvideTxBuffer` 分配与回收。
- **PHY/MII 访问子组件**：通过 `Eth_WriteMii` / `Eth_ReadMii` 访问 PHY 寄存器。
- **中断处理子组件**：Tx/Rx/Error ISR 入口定义于 `Eth_Irq.c`。

### 3.3 文件结构

```
src/bsw/mcal/eth/
├── include/
│   ├── Eth.h              # 公共 API 与类型定义
│   ├── Eth_Cfg.h          # 预编译配置宏
│   ├── Eth_GeneralTypes.h # 通用以太网类型
│   ├── Eth_Lcfg.h         # 链接时配置
│   └── Eth_Private.h      # 模块私有头
└── src/
    ├── Eth.c              # 主实现
    └── Eth_Irq.c          # 中断服务程序
```

---

## 4. 状态机

### 4.1 模块状态

```
[ETH_STATE_UNINIT] -- Eth_Init --> [ETH_STATE_INIT]
[ETH_STATE_INIT]   -- Eth_DeInit --> [ETH_STATE_UNINIT]
```

### 4.2 控制器模式

```
[ETH_MODE_DOWN] -- Eth_SetControllerMode(ACTIVE) --> [ETH_MODE_ACTIVE]
[ETH_MODE_ACTIVE] -- Eth_SetControllerMode(DOWN) --> [ETH_MODE_DOWN]
```

### 4.3 缓冲区状态

```
[ETH_BUF_STATE_FREE] -- Eth_ProvideTxBuffer --> [ETH_BUF_STATE_BUSY]
[ETH_BUF_STATE_BUSY] -- Eth_Transmit --> [ETH_BUF_STATE_TRANSMITTING]
[ETH_BUF_STATE_TRANSMITTING] -- Eth_TxConfirmation/ISR --> [ETH_BUF_STATE_FREE]
```

---

## 5. 核心数据结构

| 类型 | 说明 |
|------|------|
| `Eth_StateType` | 模块状态：`ETH_STATE_UNINIT` / `ETH_STATE_INIT` |
| `Eth_ModeType` | 控制器模式：`ETH_MODE_DOWN` / `ETH_MODE_ACTIVE` |
| `Eth_ControllerType` | 控制器索引类型（`uint8`） |
| `Eth_BufIdxType` | 缓冲区索引类型（`uint8`，`0xFF` 表示无效） |
| `Eth_DataLenType` | 帧长度类型（`uint16`） |
| `Eth_MacAddrType` | MAC 地址类型，`uint8[6]` |
| `Eth_RateType` | 速率：`ETH_RATE_10MBPS` / `100MBPS` / `1000MBPS` |
| `Eth_FilterActionType` | 过滤动作：`ADD` / `REMOVE` |
| `Eth_TimeStampType` | 时间戳：`seconds` + `nanoseconds` |
| `Eth_FrameStructType` | 帧结构：目的 MAC、源 MAC、帧类型、Payload、长度 |
| `Eth_ControllerConfigType` | 单控制器配置：索引、MAC、速率、双工、Checksum Offload、PHY 地址、缓冲区数量与大小 |
| `Eth_ConfigType` | 模块总配置：控制器数组、数量、DevErrorDetect、VersionInfoApi |

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| Eth_Init | `void Eth_Init(const Eth_ConfigType* CfgPtr)` | 初始化 Eth 模块 | 传入 NULL 触发 `ETH_E_INV_POINTER` | SWS_Eth_00001 |
| Eth_DeInit | `void Eth_DeInit(void)` | 反初始化 | 复位所有控制器状态 | SWS_Eth_00002 |
| Eth_ControllerInit | `void Eth_ControllerInit(Eth_ControllerType CtrlIdx, const Eth_ControllerConfigType* CfgPtr)` | 初始化指定控制器 | - | SWS_Eth_00003 |
| Eth_SetControllerMode | `Std_ReturnType Eth_SetControllerMode(Eth_ControllerType CtrlIdx, Eth_ModeType CtrlMode)` | 设置控制器模式 | 仅在 ACTIVE 时可收发 | SWS_Eth_00005 |
| Eth_GetControllerMode | `Std_ReturnType Eth_GetControllerMode(Eth_ControllerType CtrlIdx, Eth_ModeType* CtrlModePtr)` | 获取控制器模式 | - | SWS_Eth_00006 |
| Eth_GetControllerIdx | `uint8 Eth_GetControllerIdx(const uint8* CtrlName)` | 按名称获取控制器索引 | 当前实现固定返回 0 | SWS_Eth_00007 |
| Eth_GetPhysAddr | `void Eth_GetPhysAddr(Eth_ControllerType CtrlIdx, uint8* PhysAddrPtr)` | 获取 MAC 地址 | - | SWS_Eth_00008 |
| Eth_SetPhysAddr | `void Eth_SetPhysAddr(Eth_ControllerType CtrlIdx, const uint8* PhysAddrPtr)` | 设置 MAC 地址 | - | SWS_Eth_00009 |
| Eth_UpdatePhysAddrFilter | `Std_ReturnType Eth_UpdatePhysAddrFilter(...)` | 更新 MAC 地址过滤表 | ADD/REMOVE | SWS_Eth_00010 |
| Eth_WriteMii | `Std_ReturnType Eth_WriteMii(...)` | 写 PHY MII 寄存器 | - | SWS_Eth_00011 |
| Eth_ReadMii | `Std_ReturnType Eth_ReadMii(...)` | 读 PHY MII 寄存器 | - | SWS_Eth_00012 |
| Eth_ProvideTxBuffer | `BufReq_ReturnType Eth_ProvideTxBuffer(...)` | 分配 TX 缓冲区 | 返回 `BUFREQ_E_BUSY` 表示无空闲缓冲 | SWS_Eth_00013 |
| Eth_Transmit | `Std_ReturnType Eth_Transmit(...)` | 发送以太网帧 | - | SWS_Eth_00014 |
| Eth_Receive | `Std_ReturnType Eth_Receive(...)` | 接收以太网帧 | - | SWS_Eth_00015 |
| Eth_TxConfirmation | `void Eth_TxConfirmation(...)` | TX 完成确认回调 | 释放 TX 缓冲 | SWS_Eth_00016 |
| Eth_EnableIrq / Eth_DisableIrq | `void Eth_EnableIrq(void)` / `void Eth_DisableIrq(void)` | 全局中断使能/禁止 | - | SWS_Eth_00017 / SWS_Eth_00018 |
| Eth_InitBuffers | `void Eth_InitBuffers(void)` | 重新初始化缓冲区 | - | SWS_Eth_00019 |
| Eth_GetVersionInfo | `void Eth_GetVersionInfo(Std_VersionInfoType*)` | 版本信息 | 受 `ETH_VERSION_INFO_API` 控制 | SWS_Eth_00004 |
| Eth_GetCurrentTime | `Std_ReturnType Eth_GetCurrentTime(...)` | 获取当前时间戳 | 供 StbM 使用 | SWS_Eth_00020 |
| Eth_MainFunction | `void Eth_MainFunction(void)` | 周期主函数 | TX确认/RX轮询/错误恢复 | SWS_Eth_00020 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| `Eth_IsrTx` / `Eth_IsrRx` / `Eth_IsrError` | TX/RX/错误中断入口（声明于 `Eth.h`） |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x01 | Eth_Init | `ETH_E_INV_POINTER` |
| 0x02 | Eth_DeInit | `ETH_E_NOT_INITIALIZED` |
| 0x03 | Eth_GetVersionInfo | `ETH_E_INV_POINTER` |
| 0x04 | Eth_SetControllerMode | `ETH_E_INV_MODE` / `ETH_E_NOT_INITIALIZED` |
| 0x05 | Eth_GetControllerMode | `ETH_E_INV_POINTER` |
| 0x0E | Eth_ProvideTxBuffer | `ETH_E_INV_POINTER` / `ETH_E_INV_CTRL_INDEX` |
| 0x0F | Eth_Transmit | `ETH_E_INV_BUF_INDEX` / `ETH_E_INV_PARAM` |
| 0x10 | Eth_Receive | `ETH_E_INV_POINTER` |
| 0x11 | Eth_TxConfirmation | `ETH_E_INV_CTRL_INDEX` |
| 0x13 | Eth_EnableIrq | `ETH_E_NOT_INITIALIZED` |
| 0x14 | Eth_DisableIrq | `ETH_E_NOT_INITIALIZED` |
| 0x15 | Eth_InitBuffers | `ETH_E_NOT_INITIALIZED` |

---

## 7. 处理流程

### 7.1 初始化流程

1. `Eth_Init` 检查配置指针非空。
2. 设置全局模块状态为 `ETH_STATE_INIT`。
3. 遍历每个控制器，初始化控制器状态、配置指针、缓冲区描述符与缓冲区池。
4. `Eth_ControllerInit` 可进一步调用 `Eth_HwInit` 完成硬件寄存器配置。

### 7.2 数据发送流程

1. 上层调用 `Eth_ProvideTxBuffer` 分配缓冲区。
2. 上层填充帧数据后调用 `Eth_Transmit`。
3. `Eth_Transmit` 校验控制器模式为 ACTIVE、缓冲区状态为 BUSY，随后调用 `Eth_HwTransmit`。
4. 硬件发送完成后触发 TX ISR，`Eth_TxConfirmation` 释放缓冲区。

### 7.3 数据接收流程

1. 硬件收到帧后触发 RX ISR，将 RX 描述符置为 READY。
2. 上层周期调用 `Eth_Receive`。
3. `Eth_HwReceive` 扫描 READY 的 RX 描述符，返回缓冲区索引与长度。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `ETH_DEV_ERROR_DETECT` | STD_ON | 开发错误检测开关 |
| `ETH_VERSION_INFO_API` | STD_ON | 版本信息 API 开关 |
| `ETH_MAX_CONTROLLERS` | 1U | 最大控制器数量 |
| `ETH_MAX_TX_BUFS` | 8U | TX 缓冲区数量 |
| `ETH_MAX_RX_BUFS` | 8U | RX 缓冲区数量 |
| `ETH_CFG_BUF_SIZE` | 1536U | 缓冲区大小 |
| `ETH_TIMEOUT` | 1000U | 通用超时 |
| `ETH_LINK_TIMEOUT` | 5000U | 链路超时 |
| `ETH_CTRL0_MAC_ADDR` | `{0x00,0x01,0x02,0x03,0x04,0x05}` | 默认 MAC 地址 |
| `ETH_CTRL0_SPEED` | `ETH_RATE_100MBPS` | 默认速率 |
| `ETH_CTRL0_FULL_DUPLEX` | STD_ON | 全双工开关 |
| `ETH_MULTICAST_SUPPORT` | STD_ON | 组播支持 |
| `ETH_PROMISCUOUS_MODE` | STD_OFF | 混杂模式 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| `Eth_Lcfg.c` | 控制器配置数组 `Eth_ControllerConfig[]` 与模块配置 `Eth_Config` |

### 8.3 构建后配置

当前实现未使用 Post-Build 配置，所有配置在链接时确定。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x01 | `ETH_E_NOT_INITIALIZED` | 模块未初始化时调用 API |
| 0x02 | `ETH_E_INV_CTRL_INDEX` | 控制器索引越界 |
| 0x03 | `ETH_E_INV_POINTER` | 空指针入参 |
| 0x04 | `ETH_E_INV_PARAM` | 参数无效（如缓冲区状态错误） |
| 0x05 | `ETH_E_INV_CONFIG` | 配置无效 |
| 0x06 | `ETH_E_INV_MODE` | 控制器模式参数非法 |
| 0x07 | `ETH_E_INV_FRAME_LENGTH` | 帧长度非法 |
| 0x08 | `ETH_E_INV_MAC_ADDR` | MAC 地址非法 |
| 0x09 | `ETH_E_INV_BUF_INDEX` | 缓冲区索引非法 |
| 0x0A | `ETH_E_TIMEOUT` | 操作超时 |
| 0x0B | `ETH_E_BUSY` | 资源忙 |

### 9.2 DEM 错误

当前实现未定义 Dem 事件。

### 9.3 安全机制

- ASIL 等级：源码声明兼容 ASIL-D，但当前为简化实现。
- 安全机制：帧长度校验、MAC 地址有效性校验、控制器索引校验、空指针检测。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| `ETH_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化全局/静态变量 |
| `ETH_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | - |
| `ETH_START_SEC_CODE` | 代码段 |
| `ETH_STOP_SEC_CODE` | - |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | `ETH_MAX_CONTROLLERS * (ETH_MAX_TX_BUFS + ETH_MAX_RX_BUFS) * ETH_MAX_FRAME_SIZE` + 状态结构 | 主要由缓冲区池决定 |
| ROM | 代码 + 配置表 | 与硬件抽象相关 |
| 堆栈 | 中等 | ISR 与 API 调用嵌套较浅 |

---

## 11. 集成指南

- 与上层集成：EthIf 调用 `Eth_Init`、`Eth_ProvideTxBuffer`、`Eth_Transmit`、`Eth_Receive`；需确保 EthIf 配置的帧缓冲大小与 `ETH_CFG_BUF_SIZE` 一致。
- 与下层集成：需根据目标 MCU 补全 `Eth_Hw*` 函数中的寄存器操作；时钟、引脚复用由 MCU/Port 模块预先配置。
- 初始化顺序：MCU → Port → Eth（在 EthIf 之前）。
- 中断路由：将 `Eth_IsrTx` / `Eth_IsrRx` / `Eth_IsrError` 挂接到中断向量表。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `test_Eth.c` | 初始化、控制器模式切换、MAC 地址设置、MII 读写、缓冲区分配与释放、错误注入 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 自环回测试 | 控制器环回模式下发帧并接收 |
| PHY 读写测试 | 验证 MII 接口可正确访问 PHY ID 寄存器 |
| 中断触发测试 | 验证 Tx/Rx ISR 与 `Eth_TxConfirmation` 协同 |

---

## 13. 实现说明 / TODO

- 当前 `Eth_Hw*` 系列函数为桩实现，仅维护软件状态，未操作真实硬件寄存器。
- `Eth_GetControllerIdx` 固定返回 0，未实现按名称查找。
- `Eth_UpdatePhysAddrFilter` 未实际写入硬件过滤表。
- 源码头文件中 `ETH_MODULE_ID` 定义为 `0x53`；本设计文档按任务要求使用 `0x09`，二者不一致，需统一。
- 需根据目标芯片补全 DMA 描述符管理、链路状态检测、时间戳捕获。

---

## 14. 参考资料

1. AUTOSAR_SWS_EthernetDriver.pdf
2. `docs/modules/ETH.md`
3. `src/bsw/mcal/eth/`
