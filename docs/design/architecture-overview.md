# 系统架构概览

## 概述

yuleASR 是一个基于 AUTOSAR Classic Platform 的基础软件框架，支持多种嵌入式 MCU 平台。

## 层次结构

```
┌──────────────────────────────────────────────────────────────────────┐
│                        应用层 (ASW)                            │
│                    RTE (Runtime Environment)                     │
├──────────────────────────────────────────────────────────────────────┤
│                      基础软件 (BSW)                           │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐     │
│  │   服务层     │  │   ECU 抽象层  │  │   微控制器驱动 │  │   复杂驱动  │     │
│  │  (Services)  │  │    (ECUAL)   │  │    (MCAL)    │  │   (CDD)    │     │
│  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘     │
├──────────────────────────────────────────────────────────────────────┤
│                      微控制器 (MCU)                            │
└──────────────────────────────────────────────────────────────────────┘
```

## 主要组件

### 服务层 (Services Layer)
- **DCM**: 诊断通信管理 (Diagnostic Communication Manager)
- **DEM**: 诊断事件管理 (Diagnostic Event Manager)
- **NVM**: 非易失性存储管理 (Non-Volatile Memory Manager)
- **COM**: 自动化通信管理 (Communication Manager)
- **PDUR**: PDU 路由器 (PDU Router)
- **CSM**: 加密服务管理 (Crypto Services Manager)
- **ECUM**: ECU 状态管理 (ECU State Manager)

### ECU 抽象层 (ECUAL)
- **CANIF**: CAN 接口 (CAN Interface)
- **CANTP**: CAN 传输协议 (CAN Transport Protocol)
- **SOAD**: 套接字适配器 (Socket Adapter)
- **DOIP**: 诊断 over IP (Diagnostic over IP)

### 微控制器驱动 (MCAL)
- **CAN**: CAN 驱动
- **SPI**: SPI 驱动
- **ADC**: ADC 驱动
- **GPT**: 通用定时器 (General Purpose Timer)
- **MCU**: 微控制器驱动

### RTE (运行时环境)

RTE 是 ASW 与 BSW 之间的数据/调用路由层，负责 Sender-Receiver 端口数据缓冲与 Client-Server 调用转发。核心实现位于 [src/middleware/rte/](file:///workspace/src/middleware/rte)。

#### 端口句柄编码

端口句柄 `Rte_PortHandleType` 为 16 位，编码规则：

```
bit 15..8 = componentId   (0..7, 对应 8 个 SWC)
bit  7..4 = 组件内端口号高 nibble (恒等于 componentId，全局唯一编码)
bit  3..0 = 组件内端口索引 (1..7)
```

> 例：EngineControl 的 `ENGINE_PARAMS_P` 句柄 = `(0 << 8) | 0x02`；IOControl 的 `PWM_OUTPUT_P` = `(5 << 8) | 0x57`。

`Rte_Read` / `Rte_Write` / `Rte_ConnectPort` 内部按 **低 nibble**（`portHandle & 0x0F`）索引 `Rte_ComponentStates[componentId].Ports[]`，`componentId` 取高字节（`portHandle >> 8`）。`RTE_MAX_PORTS_PER_COMPONENT = 8` 覆盖最大端口索引 7。

> **历史缺陷**：原实现用 `portHandle & 0xFF`（全字节）作下标，而 `RTE_MAX_PORTS_PER_COMPONENT` 仅 4，导致 ID ≥ 4 的端口（7/8 组件的全部端口）数组越界并被 `Rte_ValidatePortHandle` 拒绝，RTE 数据通路实际只通了 EngineControl 的 3 个端口。修复：索引改 `& 0x0F`，并将 `RTE_MAX_PORTS_PER_COMPONENT` 提升到 8。

#### 端口数据长度（DataLength）

每个端口在连接表 [Rte_SwcPortApi.c](file:///workspace/src/middleware/rte/src/Rte_SwcPortApi.c#L60) 中声明 `DataLength`，作为 `Rte_Read`/`Rte_Write` 的 `memcpy` 长度。长度必须**精确等于该端口数据元素的类型大小**：

- 过小 → 数据被截断
- 过大 → 写入时越界读发送方缓冲、读出时越界写接收方缓冲（ASan 捕获的根因）

设计采用 `sizeof(<数据类型>)` 由编译器在编译期计算（含结构体内 padding），并引入 8 个 SWC 头文件以解析类型：

```c
RTE_SWC_PORT_CONNECTION(0U, 0x02U, 2U, (uint16)sizeof(Swc_EngineParametersType)),   /* ENGINE_PARAMS_P */
RTE_SWC_PORT_CONNECTION(2U, 0x25U, 2U, (uint16)sizeof(Swc_DiagnosticResponseType)), /* DIAG_RESPONSE_P */
...
```

> **历史缺陷**：原表把全部 36 个端口的 `DataLength` 硬编码为 `128U`，而实际结构体仅几字节到几十字节，`Rte_Write` 读 128 字节造成堆/栈越界读。修复：逐端口替换为 `sizeof`。

#### 端口缓冲区

每个端口状态含 `uint8 Data[RTE_MAX_BUFFER_SIZE]`，`RTE_MAX_BUFFER_SIZE = 512`（容纳最大的 `Swc_DiagnosticResponseType` = 259 字节，含 256B 诊断数据缓冲）。`Rte_Write` 内部对 `copyLength` 做 `min(Length, RTE_MAX_BUFFER_SIZE)` 钳制，防止配置错误导致缓冲溢出。

#### 验证

修复后全量测试在 AddressSanitizer 下 **193/193 通过**（含 `s0_smoke` 端到端用例），无数组越界报告。

## 安全机制

- **WDGM**: 看门狗管理 (Watchdog Manager)
- **SECOC**: 安全车载通信 (Secure Onboard Communication)
- **E2E**: 端到端保护 (End-to-End Protection)
