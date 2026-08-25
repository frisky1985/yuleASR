# Uart Design Document

> **Module ID**: 0x1D  
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Uart  
> **Source Path**: `src/bsw/mcal/Uart/`  
> **Reference Document**: `docs/modules/Uart.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

UART Driver 是 MCAL 层异步串行通信驱动，提供：

- 多通道 UART（最多 4 通道）。
- 轮询、中断、DMA 三种操作模式。
- 可配置波特率、数据位、停止位、校验、硬件流控制、FIFO。
- 发送/接收完成回调与错误通知。

上层可由 COM Stack、XCP、诊断、传感器等模块调用；下层直接访问 SoC UART 外设寄存器。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS Uart | 4.4.0 | UART Driver 软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | COM、XCP、诊断、传感器驱动 | 串口数据消费方 | |
| 下层 | SoC UART 外设 | i.MX8M Mini / S32K312 LPUART | |
| 同层 | Mcu、Port、Dma、Gpt | 时钟、引脚、DMA、时间基准 | |
| 公共 | Det | 开发错误检测（可选） | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   RTE / Application / COM / XCP     │
├─────────────────────────────────────┤
│           Uart (MCAL)               │
├─────────────────────────────────────┤
│      SoC UART / DMA / IRQ           │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **通道配置管理**：每个通道对应一个 UART 控制器。
- **轮询收发引擎**：`Uart_Send` / `Uart_Receive` 阻塞式完成收发。
- **DMA 收发引擎**：`Uart_SendDMA` / `Uart_ReceiveDMA` 配置 DMA 后返回。
- **中断收发引擎**：`Uart_SendInterrupt` / `Uart_ReceiveInterrupt` 使能中断后由 ISR 推进。
- **中断处理**：`Uart_IsrHandler` 分发 TX/RX/错误处理。
- **超时监控**：`Uart_MainFunction` 检查中断/DMA 传输超时并中止。

### 3.3 文件结构

```
src/bsw/mcal/Uart/
├── include/
│   ├── Uart.h
│   ├── Uart_Cfg.h
│   ├── SchM_Uart.h
│   └── Dma.h
└── src/
    └── Uart.c
```

---

## 4. 状态机

### 4.1 通道状态

```
[UNINIT] -- Uart_Init() --> [READY]
[READY]  -- 开始 TX/RX  --> [TX_BUSY] / [RX_BUSY] / [TX_RX_BUSY]
[TX_BUSY] -- 完成/中止 --> [READY]
[RX_BUSY] -- 完成/中止 --> [READY]
[TX_RX_BUSY] -- 完成/中止 --> [READY]
[ERROR]  -- 用户处理 --> [READY]
```

### 4.2 发送/接收状态

| 状态 | 说明 | |
|------|------|
| `UART_TX_IDLE` / `UART_RX_IDLE` | 空闲 | |
| `UART_TX_ACTIVE` / `UART_RX_ACTIVE` | 进行中 | |
| `UART_TX_COMPLETE` / `UART_RX_COMPLETE` | 完成 | |
| `UART_TX_ERROR` / `UART_RX_ERROR` | 错误 | |

---

## 5. 核心数据结构

| 类型 | 说明 | |
|------|------|
| `Uart_ChannelType` | `uint8`，通道 ID | |
| `Uart_StatusType` | 通道状态枚举 | |
| `Uart_OpModeType` | `POLLING` / `INTERRUPT` / `DMA` | |
| `Uart_DataBitsType` | 5/6/7/8 位数据 | |
| `Uart_StopBitsType` | 1/1.5/2 位停止位 | |
| `Uart_ParityType` | `NONE` / `ODD` / `EVEN` | |
| `Uart_HwHandshakeType` | 无/RTS/CTS/RTS_CTS | |
| `Uart_FifoModeType` | FIFO 禁用/启用 | |
| `Uart_TxStatusType` / `Uart_RxStatusType` | 收发子状态 | |
| `Uart_ResultType` | `OK` / `PENDING` / `TIMEOUT` / `ERROR` | |
| `Uart_ChannelConfigType` | 单通道配置 | |
| `Uart_ConfigType` | 全局配置 | |
| `Uart_BufferType` | 缓冲区描述符 | |
| `Uart_TxNotificationType` / `Uart_RxNotificationType` / `Uart_ErrorNotificationType` | 回调类型 | |

```c
typedef struct {
    Uart_ChannelType ChannelId;
    uint32 BaudRate;
    Uart_DataBitsType DataBits;
    Uart_StopBitsType StopBits;
    Uart_ParityType Parity;
    Uart_OpModeType OpMode;
    Uart_HwHandshakeType HwHandshake;
    Uart_FifoModeType FifoMode;
    uint8 TxFifoThreshold;
    uint8 RxFifoThreshold;
    boolean DmaEnabled;
    uint8 DmaTxChannel;
    uint8 DmaRxChannel;
    uint8 IrqPriority;
    uint32 TxTimeout;
    uint32 RxTimeout;
} Uart_ChannelConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|----------------|
| `Uart_Init` | `void Uart_Init(const Uart_ConfigType* Config)` | 初始化 UART 驱动 | | |
| `Uart_DeInit` | `void Uart_DeInit(void)` | 反初始化 | | |
| `Uart_Send` | `Std_ReturnType Uart_Send(Uart_ChannelType Channel, const uint8* Data, uint32 Length)` | 轮询发送 | | |
| `Uart_SendDMA` | `Std_ReturnType Uart_SendDMA(...)` | DMA 发送 | 受 `UART_DMA_SUPPORT` 控制 | |
| `Uart_SendInterrupt` | `Std_ReturnType Uart_SendInterrupt(...)` | 中断发送 | | |
| `Uart_Receive` | `Std_ReturnType Uart_Receive(...)` | 轮询接收 | | |
| `Uart_ReceiveDMA` | `Std_ReturnType Uart_ReceiveDMA(...)` | DMA 接收 | | |
| `Uart_ReceiveInterrupt` | `Std_ReturnType Uart_ReceiveInterrupt(...)` | 中断接收 | | |
| `Uart_GetStatus` | `Uart_StatusType Uart_GetStatus(Uart_ChannelType Channel)` | 获取通道状态 | | |
| `Uart_GetTxResult` | `Uart_ResultType Uart_GetTxResult(...)` | 发送结果 | | |
| `Uart_GetRxResult` | `Uart_ResultType Uart_GetRxResult(...)` | 接收结果 | | |
| `Uart_SetBaudRate` | `Std_ReturnType Uart_SetBaudRate(...)` | 动态设置波特率 | | |
| `Uart_EnableInterrupt` / `Uart_DisableInterrupt` | ... | 中断开关 | | |
| `Uart_EnableDMA` / `Uart_DisableDMA` | ... | DMA 开关 | 受 `UART_DMA_SUPPORT` 控制 | |
| `Uart_ClearFIFO` | `void Uart_ClearFIFO(...)` | 清除 FIFO | 受 `UART_FIFO_SUPPORT` 控制 | |
| `Uart_SetFifoThreshold` | `Std_ReturnType Uart_SetFifoThreshold(...)` | FIFO 阈值 | | |
| `Uart_Abort` | `void Uart_Abort(...)` | 中止传输 | | |
| `Uart_GetVersionInfo` | `void Uart_GetVersionInfo(...)` | 版本信息 | 受 `UART_VERSION_INFO_API` 控制 | |
| `Uart_MainFunction` | `void Uart_MainFunction(void)` | 周期监控超时 | | |
| `Uart_IsrHandler` | `void Uart_IsrHandler(Uart_ChannelType Channel)` | 中断处理 | | |

### 6.2 回调函数

| 回调 | 说明 | |
|------|------|
| `Uart_TxNotification[Channel]` | 发送完成通知 | |
| `Uart_RxNotification[Channel]` | 接收完成通知 | |
| `Uart_ErrorNotification[Channel]` | 错误通知 | |

回调通过全局函数指针数组注册，当前未提供显式注册 API，需直接赋值。

### 6.3 Service ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0 | `Uart_Init` | `UART_E_PARAM_POINTER`、`UART_E_ALREADY_INITIALIZED` | |
| 1 | `Uart_DeInit` | `UART_E_UNINIT` | |
| 2 | `Uart_Send` | `UART_E_PARAM_CHANNEL`、`UART_E_PARAM_POINTER`、`UART_E_UNINIT`、`UART_E_TX_BUSY`、`UART_E_PARAM_LENGTH` | |
| 3 | `Uart_Receive` | `UART_E_PARAM_CHANNEL`、`UART_E_PARAM_POINTER`、`UART_E_UNINIT`、`UART_E_RX_BUSY` | |
| 4 | `Uart_GetStatus` | `UART_E_PARAM_CHANNEL` | |
| 5 | `Uart_GetVersionInfo` | `UART_E_PARAM_POINTER` | |
| 6 | `Uart_SendDMA` | `UART_E_PARAM_CHANNEL`、`UART_E_PARAM_POINTER`、`UART_E_UNINIT` | |
| 7 | `Uart_ReceiveDMA` | `UART_E_PARAM_CHANNEL`、`UART_E_PARAM_POINTER`、`UART_E_UNINIT` | |
| 8 | `Uart_Abort` | 无 | |
| 9/10 | Enable/DisableInterrupt | 无 | |
| 11/12 | Enable/DisableDMA | 无 | |
| 13 | `Uart_ClearFIFO` | 无 | |
| 14 | `Uart_SetBaudRate` | `UART_E_PARAM_CHANNEL`、`UART_E_UNINIT`、`UART_E_PARAM_BAUDRATE` | |

| 错误码 | 名称 | 说明 | |
|--------|------|------|
| 0x00 | `UART_E_NO_ERROR` | 无错误 | |
| 0x01 | `UART_E_PARAM_CHANNEL` | 无效通道 | |
| 0x02 | `UART_E_PARAM_POINTER` | 空指针 | |
| 0x03 | `UART_E_PARAM_CONFIG` | 无效配置 | |
| 0x04 | `UART_E_PARAM_BAUDRATE` | 无效波特率 | |
| 0x08 | `UART_E_PARAM_LENGTH` | 无效长度 | |
| 0x10 | `UART_E_UNINIT` | 未初始化 | |
| 0x11 | `UART_E_ALREADY_INITIALIZED` | 已初始化 | |
| 0x20 | `UART_E_TX_BUSY` | 发送忙 | |
| 0x21 | `UART_E_RX_BUSY` | 接收忙 | |
| 0x32 | `UART_E_OVERRUN` | 接收溢出 | |
| 0x40 | `UART_E_TIMEOUT` | 超时 | |
| 0x50 | `UART_E_DMA_ERROR` | DMA 错误 | |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 `Config` 非空、未初始化。
2. 保存配置指针。
3. 对每个通道：
   - 清零通道状态；
   - 调用 `Uart_HwInit`：
     - 软件复位；
     - 配置 UCR1（UART 使能、DMA/中断使能）；
     - 配置 UCR2（收发使能、校验、停止位、数据位、流控）；
     - 配置 UCR3/UCR4；
     - 配置 UFCR（FIFO 阈值与参考分频）；
     - 设置波特率。
4. 置 `Uart_Initialized = TRUE`。

### 7.2 轮询发送流程

1. 校验通道、指针、初始化状态，长度非零。
2. 检查通道非 TX 忙。
3. 等待 TX FIFO 就绪，逐字节写入 UTXD。
4. 等待传输完成标志 TXDC。
5. 更新状态，调用发送完成回调。

### 7.3 轮询接收流程

1. 校验通道、指针、初始化状态。
2. 检查通道非 RX 忙。
3. 等待 RDR 就绪，逐字节读取 URXD。
4. 检测到 ORE 等错误立即返回 ERROR。
5. 更新状态，调用接收完成回调。

### 7.4 中断处理流程

1. 读 USR1/USR2。
2. `RRDY` -> `Uart_ProcessRxInterrupt` 读取数据。
3. `TRDY` -> `Uart_ProcessTxInterrupt` 写入数据。
4. `ORE`/`BRCD` -> `Uart_ProcessError` 更新错误码并调用错误回调。
5. 写回寄存器清除标志。

### 7.5 MainFunction 流程

1. 检查初始化状态。
2. 遍历所有通道：
   - 若 TX/RX ACTIVE 且超时，置结果 TIMEOUT 并调用 `Uart_Abort`。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 | |
|----|--------|------|
| `UART_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 | |
| `UART_VERSION_INFO_API` | STD_ON | 版本信息 API | |
| `UART_CHANNEL_COUNT` | 4U | 通道数量 | |
| `UART_DMA_SUPPORT` | STD_ON | DMA 支持 | |
| `UART_FIFO_SUPPORT` | STD_ON | FIFO 支持 | |
| `UART_HW_HANDSHAKE_SUPPORT` | STD_ON | 硬件流控支持 | |
| `UART_MULTI_PROCESSOR_MODE` | STD_OFF | 多处理器模式 | |
| `UART_REF_CLOCK_HZ` | 80000000U | 参考时钟 | |
| `UART_FIFO_DEPTH` | 32U | FIFO 深度 | |
| `UART_TX_FIFO_THRESHOLD` | 8U | TX FIFO 阈值 | |
| `UART_RX_FIFO_THRESHOLD` | 8U | RX FIFO 阈值 | |
| `UART_TX_TIMEOUT_MS` / `UART_RX_TIMEOUT_MS` | 1000U | 默认收发超时 | |
| `UART_IRQ_PRIORITY_LEVEL` | 5U | 中断优先级 | |

### 8.2 DMA 通道配置

| 宏 | 值 | |
|----|----|
| `UART0_DMA_TX_CHANNEL` | 0 | |
| `UART0_DMA_RX_CHANNEL` | 1 | |
| `UART1_DMA_TX_CHANNEL` | 2 | |
| `UART1_DMA_RX_CHANNEL` | 3 | |
| `UART2_DMA_TX_CHANNEL` | 4 | |
| `UART2_DMA_RX_CHANNEL` | 5 | |
| `UART3_DMA_TX_CHANNEL` | 6 | |
| `UART3_DMA_RX_CHANNEL` | 7 | |

### 8.3 链接时配置

当前 `Uart.c` 未使用独立 Lcfg 文件，配置通过 `Uart_ConfigType` 传入 `Uart_Init`。建议后续按 AUTOSAR 模式生成 `Uart_Lcfg.c`。

---

## 9. 错误处理与安全

### 9.1 DET 错误

在 `UART_DEV_ERROR_DETECT == STD_ON` 时：

- 空配置/空指针 -> 对应 PARAM 错误
- 未初始化调用 -> `UART_E_UNINIT`
- 重复初始化 -> `UART_E_ALREADY_INITIALIZED`
- 通道越界 -> `UART_E_PARAM_CHANNEL`
- 零长度发送 -> `UART_E_PARAM_LENGTH`
- 零波特率 -> `UART_E_PARAM_BAUDRATE`

### 9.2 DEM 错误

本模块未使用 DEM。

### 9.3 安全机制

- 所有轮询 API 带超时保护。
- `Uart_MainFunction` 监控中断/DMA 传输超时。
- `Uart_Abort` 可安全释放通道。
- SchM 独占区 `UART_EXCLUSIVE_AREA_0` 使用 `Mcal_DisableAllInterrupts`。

---

## 10. 内存与性能

### 10.1 MemMap 分区

本实现未显式使用 MemMap 宏。建议后续补充：

| 分区 | 用途 | |
|------|------|
| `UART_START_SEC_VAR_CLEARED_UNSPECIFIED` | `Uart_ChannelState`、回调指针 | |
| `UART_START_SEC_CONFIG_DATA_UNSPECIFIED` | `Uart_Config` | |
| `UART_START_SEC_CODE` | 代码段 | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | ~500 B | 通道状态 + 回调指针数组 | |
| ROM | 配置表 + 代码 | 与通道数成正比 | |
| 堆栈 | 中等 | 轮询收发循环 | |

---

## 11. 集成指南

- 与 Mcu 集成：依赖 Mcu 使能 UART 时钟。
- 与 Port 集成：UART 引脚需配置为对应复用模式。
- 与 Dma 集成：`Uart_SendDMA` / `Uart_ReceiveDMA` 调用 `Dma_InitChannel` / `Dma_EnableChannel` / `Dma_DisableChannel`。
- 与 Gpt 集成：通过 `Gpt_GetTimeElapsed` 获取时间基准。
- 与 ISR 集成：将 `Uart_IsrHandler` 注册到对应 UART 中断向量。
- 初始化顺序：Mcu -> Port -> Dma/Gpt -> Uart。

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 覆盖内容 | |
|--------|----------|
| 初始化 | 空配置、重复初始化、寄存器默认值 | |
| 轮询收发 | 正常收发、超时、错误注入 | |
| 中断收发 | FIFO 填充/读取、完成回调 | |
| DMA 收发 | DMA 配置正确性 | |
| 波特率设置 | 不同波特率下的 UBIR/UBMR | |
| DET | 各错误码路径 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 与 PC 串口通信 | 验证 8N1 及各种波特率 | |
| 硬件流控 | RTS/CTS 握手 | |
| DMA 大数据量 | 连续发送/接收 KB 级数据 | |
| 错误处理 | 帧错误、溢出、中断信号 | |

---

## 13. 实现说明 / TODO

- **Module ID 差异**：头文件中 `UART_MODULE_ID` 定义为 `0x11`（十进制 17），与 AUTOSAR 标准 UART Module ID `0x1D` 不一致。设计文档按项目约定使用 `0x1D`，实际代码需统一。
- **API 命名偏差**：未严格遵循 AUTOSAR Uart SWS 的 API 列表（如缺少 `Uart_CheckWakeup`、标准 Channel/Length 参数类型等）。
- **回调注册 API 缺失**：发送/接收/错误回调通过全局数组保存，但未提供公开注册函数。
- **DMA 地址计算**：`Uart_SendDMA` / `Uart_ReceiveDMA` 使用 `UART1_BASE_ADDR + offset + Channel * 0x40000U` 计算外设地址，需与实际 UART 基址表一致。
- **GetVersionInfo SID 错误**：实现中调用 `Det_ReportError` 时使用了 SID `0x02`，与配置宏 `UART_SERVICE_ID_GETVERSIONINFO = 5` 不一致。
- **SetFifoThreshold 未实现**：声明存在，但 `Uart.c` 中无实现。
- **无独立 Lcfg**：建议生成 `Uart_Lcfg.c` 存放全局配置实例。

---

## 14. 参考资料

1. AUTOSAR_SWS_Uart.pdf
2. `docs/modules/Uart.md`
3. `src/bsw/mcal/Uart/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Uart_00001 | `uart` | 测试 test_uart_Init_should_initialize_successfully 覆盖: uart_Init_should_initialize_successfully 场景 |
| SWS_Uart_00002 | `uart_DeInit_should_cleanup_successfully` | 测试 test_uart_DeInit_should_cleanup_successfully 覆盖: uart_DeInit_should_cleanup_successfully 场景 |
| SWS_Uart_00003 | `Uart_Transmit` | 测试 test_Uart_Transmit_ValidCall_ShouldSucceed 覆盖: Uart_Transmit_ValidCall_ShouldSucceed 场景 |
| SWS_Uart_00004 | `Uart_Receive` | 测试 test_Uart_Receive_ValidCall_ShouldSucceed 覆盖: Uart_Receive_ValidCall_ShouldSucceed 场景 |
| SWS_Uart_00005 | `Uart_GetStatus` | 测试 test_Uart_GetStatus_ValidCall_ShouldReturnStatus 覆盖: Uart_GetStatus_ValidCall_ShouldReturnStatus 场景 |
| SWS_Uart_00006 | `Uart_GetVersionInfo` | 测试 test_Uart_GetVersionInfo_ValidPtr_ShouldSucceed 覆盖: Uart_GetVersionInfo_ValidPtr_ShouldSucceed 场景 |
| SWS_Uart_00007 | `Uart_SetBaudRate` | 测试 test_Uart_SetBaudRate_ValidCall_ShouldSucceed 覆盖: Uart_SetBaudRate_ValidCall_ShouldSucceed 场景 |
| SWS_Uart_00008 | `Uart_MainFunction` | 测试 test_Uart_MainFunction_ValidCall_ShouldSucceed 覆盖: Uart_MainFunction_ValidCall_ShouldSucceed 场景 |
| SWS_Uart_00009 | `Uart_FlushTxBuffer` | 测试 test_Uart_FlushTxBuffer_ValidCall_ShouldSucceed 覆盖: Uart_FlushTxBuffer_ValidCall_ShouldSucceed 场景 |
| SWS_Uart_00010 | `Uart_FlushRxBuffer` | 测试 test_Uart_FlushRxBuffer_ValidCall_ShouldSucceed 覆盖: Uart_FlushRxBuffer_ValidCall_ShouldSucceed 场景 |
| SWS_Uart_00019 | `uart_GetVersionInfo_should_return_version` | 测试 test_uart_GetVersionInfo_should_return_version 覆盖: uart_GetVersionInfo_should_return_version 场景 |
