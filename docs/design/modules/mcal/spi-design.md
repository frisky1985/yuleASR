# Spi Design Document

> **Module ID**: 0x0A  
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_Spi  
> **Source Path**: `src/bsw/mcal/Spi/`  
> **Reference Document**: `docs/modules/Spi.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

SPI Driver 是 MCAL 层串行外设接口驱动，提供：

- 多通道 SPI 主机模式。
- 同步与异步传输。
- DMA 与中断支持。
- 多从机片选管理。

上层可由 CanTrcv、Eeprom、Sensor 等 ECUAL 模块调用；下层直接访问 SoC ECSPI 外设寄存器。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS Spi | 4.4.0 | SPI Driver 软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | CanTrcv、Eeprom、传感器驱动等 | SPI 数据消费方 | |
| 下层 | SoC ECSPI 外设 | i.MX8M Mini | |
| 同层 | Mcu、Port、Dma、Gpt | 时钟、引脚、DMA、时间基准 | |
| 公共 | Det | 开发错误检测（可选） | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   RTE / Application / ECUAL (Eeprom...) │
├─────────────────────────────────────┤
│           Spi (MCAL)                │
├─────────────────────────────────────┤
│      SoC ECSPI / DMA / IRQ          │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **通道配置管理**：每个通道对应一个 ECSPI 控制器。
- **同步传输引擎**：轮询 TX/RX FIFO 完成收发。
- **异步传输引擎**：通过 DMA 或中断完成收发，由 `Spi_MainFunction` 监控超时。
- **从机设备管理**：`Spi_ExternalDeviceType` 描述片选、通道、波特率。
- **中断处理**：`Spi_IsrHandler` 处理 RX/TX 中断并推进传输。

### 3.3 文件结构

```
src/bsw/mcal/Spi/
├── include/
│   ├── Spi.h
│   └── Spi_Cfg.h
└── src/
    ├── Spi.c
    └── Spi_Lcfg.c
```

---

## 4. 状态机

### 4.1 驱动状态

```
[UNINIT] -- Spi_Init() --> [IDLE]
[IDLE]   -- 开始传输  --> [BUSY]
[BUSY]   -- 传输完成  --> [IDLE]
[IDLE]   -- Spi_DeInit() --> [UNINIT]
```

### 4.2 任务结果

| 状态 | 说明 | |
|------|------|
| `SPI_JOB_OK` | 任务完成/无任务 | |
| `SPI_JOB_PENDING` | 任务进行中 | |
| `SPI_JOB_FAILED` | 任务失败 | |

---

## 5. 核心数据结构

| 类型 | 说明 | |
|------|------|
| `Spi_ChannelType` | `uint8`，通道 ID | |
| `Spi_SequenceType` | `uint8`，序列 ID（当前未使用） | |
| `Spi_StatusType` | `SPI_UNINIT` / `SPI_IDLE` / `SPI_BUSY` | |
| `Spi_JobResultType` | `SPI_JOB_OK` / `PENDING` / `FAILED` / `QUEUED` | |
| `Spi_SeqResultType` | 序列结果枚举 | |
| `Spi_DataModeType` | `8BIT` / `16BIT` / `32BIT` | |
| `Spi_ClockModeType` | CPOL/CPHA 组合（Mode 0~3） | |
| `Spi_TransferType` | `FULL_DUPLEX` / `HALF_DUPLEX_TX` / `HALF_DUPLEX_RX` | |
| `Spi_TransferResultType` | 传输结果枚举 | |
| `Spi_ChannelConfigType` | 单通道配置：数据模式、时钟模式、波特率、DMA/中断 | |
| `Spi_ExternalDeviceType` | 外部设备配置：片选引脚、通道、波特率 | |
| `Spi_BufferType` | 缓冲区描述符 | |
| `Spi_ConfigType` | 全局配置 | |

```c
typedef struct {
    uint8 ChannelCount;
    const Spi_ChannelConfigType* ChannelConfig;
    const Spi_ExternalDeviceType* DeviceConfig;
    uint8 DeviceCount;
} Spi_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|----------------|
| `Spi_Init` | `void Spi_Init(const Spi_ConfigType* Config)` | 初始化 SPI 驱动 | 必须先调用 | |
| `Spi_DeInit` | `Std_ReturnType Spi_DeInit(void)` | 反初始化 | | |
| `Spi_SyncTransmit` | `Std_ReturnType Spi_SyncTransmit(uint8 DeviceId, const uint8* TxData, uint8* RxData, uint32 Length)` | 同步收发 | 阻塞式 | |
| `Spi_AsyncTransmit` | `Std_ReturnType Spi_AsyncTransmit(uint8 DeviceId, const uint8* TxData, uint8* RxData, uint32 Length)` | 异步收发 | DMA 或中断 | |
| `Spi_GetStatus` | `Spi_StatusType Spi_GetStatus(void)` | 获取驱动状态 | | |
| `Spi_GetJobResult` | `Spi_JobResultType Spi_GetJobResult(void)` | 获取任务结果 | | |
| `Spi_MainFunction` | `void Spi_MainFunction(void)` | 周期检查超时 | | |
| `Spi_IsrHandler` | `void Spi_IsrHandler(uint8 Channel)` | 中断处理 | | |
| `Spi_GetVersionInfo` | `void Spi_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 获取版本信息 | 受 `SPI_VERSION_INFO_API` 控制 | |

### 6.2 回调函数

本模块未使用回调函数，异步完成通过轮询 `Spi_GetJobResult` 或中断上下文更新状态。

### 6.3 Service ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0 | `Spi_Init` | `SPI_E_PARAM_POINTER`、`SPI_E_ALREADY_INITIALIZED` | |
| 1 | `Spi_DeInit` | `SPI_E_UNINIT` | |
| 2 | `Spi_WriteIB`（未实现） | - | |
| 3 | `Spi_AsyncTransmit` | `SPI_E_UNINIT`、`SPI_E_PARAM_CHANNEL` | |
| 6 | `Spi_GetStatus` | `SPI_E_UNINIT` | |
| 7 | `Spi_GetJobResult` | `SPI_E_UNINIT` | |
| 9 | `Spi_GetVersionInfo` | `SPI_E_PARAM_POINTER` | |
| 10 | `Spi_SyncTransmit` | `SPI_E_UNINIT`、`SPI_E_PARAM_CHANNEL` | |
| 12 | `Spi_Cancel`（未实现） | - | |

| 错误码 | 名称 | 说明 | |
|--------|------|------|
| 0x0A | `SPI_E_PARAM_CHANNEL` | 无效通道 | |
| 0x0B | `SPI_E_PARAM_JOB` | 无效 Job | |
| 0x0C | `SPI_E_PARAM_SEQ` | 无效 Sequence | |
| 0x10 | `SPI_E_PARAM_POINTER` | 空指针 | |
| 0x11 | `SPI_E_PARAM_LENGTH` | 无效长度 | |
| 0x12 | `SPI_E_PARAM_UNIT` | 无效硬件单元 | |
| 0x1A | `SPI_E_UNINIT` | 未初始化 | |
| 0x1B | `SPI_E_ALREADY_INITIALIZED` | 已初始化 | |
| 0x20 | `SPI_E_SEQ_PENDING` | 序列挂起 | |
| 0x21 | `SPI_E_SEQ_INPROCESS` | 序列执行中 | |
| 0x22 | `SPI_E_JOB_PENDING` | Job 挂起 | |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 `Config` 非空。
2. 遍历所有通道：
   - 禁用 ECSPI。
   - 配置 CONREG（主机模式、通道号）。
   - 配置 CONFIGREG（时钟模式、数据位宽）。
   - 配置 DMAREG（若 DMA 使能）。
   - 配置 INTREG（若中断使能）。
   - 调用 `Spi_SetBaudRateInternal` 设置波特率。
3. 置 `Spi_Initialized = TRUE`，状态为 IDLE。

### 7.2 同步传输流程

1. 检查初始化、DeviceId 有效、当前不忙。
2. 根据 DeviceId 获取通道，设置状态为 BUSY。
3. 配置 CONREG 通道选择、设置波特率。
4. 轮询发送每个字节并等待 RX FIFO 就绪后读取。
5. 超时返回 `E_NOT_OK`，成功返回 `E_OK`，状态回 IDLE。

### 7.3 异步传输流程

1. 检查初始化、DeviceId 有效、当前不忙。
2. 配置通道与波特率。
3. 若 DMA 使能且长度大于阈值：
   - 配置 TX/RX DMA 通道；
   - 启动 DMA；
   - 置 `XCH` 启动传输。
4. 否则使用中断：
   - 预填充 TX FIFO；
   - 使能 TX/RX 中断；
   - 置 `XCH` 启动传输。
5. `Spi_MainFunction` 监控超时；`Spi_IsrHandler` 推进收发。

### 7.4 中断处理流程

1. 读 STATREG。
2. RX 就绪时读取 RXDATA 到接收缓冲区。
3. TX 空时继续填充 TXDATA。
4. 传输完成后清中断、状态置 IDLE。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 | |
|----|--------|------|
| `SPI_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 | |
| `SPI_VERSION_INFO_API` | STD_ON | 版本信息 API | |
| `SPI_CHANNEL_COUNT` | 4U | 通道数量 | |
| `SPI_MAX_EXTERNAL_DEV` | 16U | 最大外部设备数 | |
| `SPI_INTERRUPT_SUPPORT` | STD_ON | 中断支持 | |
| `SPI_DMA_SUPPORT` | STD_ON | DMA 支持 | |
| `SPI_MULTI_SLAVE_SUPPORT` | STD_ON | 多从机支持 | |
| `SPI_FIFO_SUPPORT` | STD_ON | FIFO 支持 | |
| `SPI_MAX_JOB` | 16U | 最大 Job 数 | |
| `SPI_MAX_SEQUENCE` | 8U | 最大 Sequence 数 | |
| `SPI_REF_CLOCK_HZ` | 80000000U | 参考时钟 | |
| `SPI_TRANSFER_TIMEOUT_MS` | 1000U | 传输超时 | |
| `SPI_FIFO_DEPTH` | 64U | FIFO 深度 | |
| `SPI_DMA_TX_CHANNEL_BASE` | 8U | DMA TX 通道基号 | |
| `SPI_DMA_RX_CHANNEL_BASE` | 12U | DMA RX 通道基号 | |
| `SPI_TX_FIFO_THRESHOLD` | 32U | TX FIFO 阈值 | |
| `SPI_RX_FIFO_THRESHOLD` | 32U | RX FIFO 阈值 | |

### 8.2 链接时配置

`Spi_Lcfg.c` 提供 `const Spi_ConfigType Spi_Config`，当前为占位结构 `{ 0U }`，需配置工具生成通道与从机设备表。

---

## 9. 错误处理与安全

### 9.1 DET 错误

在 `SPI_DEV_ERROR_DETECT == STD_ON` 时：

- `Spi_Init` 空指针 -> `SPI_E_PARAM_POINTER`
- 未初始化调用 API -> `SPI_E_UNINIT`
- 通道越界 -> `SPI_E_PARAM_CHANNEL`

### 9.2 DEM 错误

本模块未使用 DEM。

### 9.3 安全机制

- 同步传输带超时保护，避免死等。
- `Spi_MainFunction` 监控异步传输超时。
- `Spi_DeInit` 增加非 DET 条件下的防御检查，防止空指针访问。

---

## 10. 内存与性能

### 10.1 MemMap 分区

本实现未显式使用 MemMap 宏包围全局变量。建议后续补充：

| 分区 | 用途 | |
|------|------|
| `SPI_START_SEC_VAR_CLEARED_UNSPECIFIED` | `Spi_ChannelState`、`Spi_Initialized` 等 | |
| `SPI_START_SEC_CONFIG_DATA_UNSPECIFIED` | `Spi_Config` | |
| `SPI_START_SEC_CODE` | 代码段 | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | ~200 B | 通道状态数组 + 全局变量 | |
| ROM | 配置表 + 代码 | 与通道/设备数成正比 | |
| 堆栈 | 中等 | 同步传输轮询占用 | |

---

## 11. 集成指南

- 与 Mcu 集成：依赖 Mcu 使能 ECSPI 时钟。
- 与 Port 集成：SPI 引脚需配置为对应复用模式。
- 与 Dma 集成：异步传输调用 `Dma_ConfigTx`/`Dma_ConfigRx`/`Dma_EnableChannel`，需 DMA 驱动提供这些外部符号。
- 与 Gpt 集成：通过 `Gpt_GetTimeElapsed` 获取时间基准。
- 与 ISR 集成：将 `Spi_IsrHandler` 注册到对应中断向量。
- 初始化顺序：Mcu -> Port -> Dma/Gpt -> Spi。

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 覆盖内容 | |
|--------|----------|
| 初始化 | 空配置、重复初始化、寄存器配置 | |
| 同步传输 | 全双工收发、超时、长度边界 | |
| 异步传输 | DMA 路径、中断路径、超时 | |
| 中断处理 | FIFO 读写、传输完成 | |
| DET | 错误码路径 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 连接真实 SPI 从机 | 验证 CPOL/CPHA、波特率 | |
| 多从机切换 | 通过 DeviceId 切换通道与片选 | |
| DMA 大数据传输 | 验证连续大数据块收发 | |

---

## 13. 实现说明 / TODO

- **Module ID 差异**：头文件中 `SPI_MODULE_ID` 定义为 `0x7A`（十进制 122），`Spi_Cfg.h` 中 `SPI_MODULE_ID_CFG` 为 122，与 AUTOSAR 标准 SPI Module ID `0x0A` 不一致。设计文档按项目约定使用 `0x0A`，实际代码需统一。
- **AUTOSAR API 偏差**：未实现 `Spi_WriteIB`、`Spi_ReadIB`、`Spi_SetupEB`、`Spi_GetSequenceResult`、`Spi_Cancel`、`Spi_SetClock` 等标准 API。
- **Sequence/Job 抽象缺失**：当前仅支持按 DeviceId 直接收发，未实现 AUTOSAR 的 Job/Sequence 调度模型。
- **片选控制**：`ChipSelectPin` 保存在配置中，但代码未主动拉低/拉高 GPIO 片选，需上层或外部逻辑处理。
- **DMA 地址计算**：`Dma_ConfigTx` 目的地址使用固定 `UART1_BASE_ADDR` 偏移公式，命名与基址需核对。
- **STATREG 位重复**：`STATREG_TC` 与 `STATREG_RO` 都定义为 `(1u << 7)`，存在冲突。
- **Lcfg 占位**：`Spi_Lcfg.c` 中 `Spi_Config` 为 `{ 0U }`，需配置工具生成。

---

## 14. 参考资料

1. AUTOSAR_SWS_Spi.pdf
2. `docs/modules/Spi.md`
3. `src/bsw/mcal/Spi/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Spi | — | SPI 模块级需求归属 |
| SWS_Spi_00001 | `spi_init_null` | 测试 spi_init_null 覆盖: spi_init_null 场景 |
| SWS_Spi_00002 | `spi_deinit_busy` | 测试 spi_deinit_busy 覆盖: spi_deinit_busy 场景 |
| SWS_Spi_00003 | `spi_sync_transmit_valid` | 测试 spi_sync_transmit_valid 覆盖: spi_sync_transmit_valid 场景 |
| SWS_Spi_00004 | `spi_async_transmit_busy` | 测试 spi_async_transmit_busy 覆盖: spi_async_transmit_busy 场景 |
| SWS_Spi_00005 | `spi_get_status_uninit` | 测试 spi_get_status_uninit 覆盖: spi_get_status_uninit 场景 |
| SWS_Spi_00006 | `spi_get_job_result` | 测试 spi_get_job_result 覆盖: spi_get_job_result 场景 |
| SWS_Spi_00007 | `Spi_GetVersionInfo` | 测试 test_Spi_GetVersionInfo_NullPtr_ShouldReportDet 覆盖: Spi_GetVersionInfo_NullPtr_ShouldReportDet 场景 |
| SWS_Spi_00009 | `spi_get_version_info` | 测试 spi_get_version_info 覆盖: spi_get_version_info 场景 |
