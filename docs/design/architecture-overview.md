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

## 代码质量与未定义行为防护

CI 门禁集成 **AddressSanitizer (ASan)** 与 **UndefinedBehaviorSanitizer (UBSan)** 两个 native 构建 job，二者均为阻断型：出现内存越界或未定义行为即合并失败。构建选项由 [SanitizerFlags.cmake](file:///workspace/cmake/modules/SanitizerFlags.cmake) 提供（`YULE_ENABLE_ASAN` / `YULE_ENABLE_UBSAN`）。

### UBSan 9 处既有 UB 修复（2026-11）

| # | 位置 | UB 类型 | 修复 |
|:--|:-----|:--------|:-----|
| 1 | [blake2s.c](file:///workspace/third_party/crypto/blake2/src/blake2s.c#L191) | 参数块 XOR 循环上限 `sizeof(blake2s_param_t)`(42B) 越界访问 `h[8]`（`uint32[8]`） | 循环上限改为 `sizeof(S->h)`(32B)，匹配 BLAKE2s 32 字节参数块 |
| 2 | [RamTst.c](file:///workspace/src/bsw/mcal/ramtst/src/RamTst.c#L462) | `1U << CurrentBit`，`CurrentBit` 可达 32，32 位移位为 UB | 将 `CurrentBit > 31` 的提前返回移到移位运算之前，保证移位量 ∈ [0,31] |
| 3 | [Xcp.c](file:///workspace/src/bsw/services/xcp/src/Xcp.c#L1693) | `memcpy(dst, Data, len)` 中 `Data` 可能为 NULL | 增加 `Data != NULL_PTR && Length > 0U` 守卫 |
| 4 | [stack_protection.c](file:///workspace/src/safety/saferam/stack_protection.c#L364) | 金丝雀 `uint64_t` 读写地址可能非 8 字节对齐 | 写/读均改用 `memcpy` |
| 5 | [stack_protection.c](file:///workspace/src/safety/saferam/stack_protection.c#L925) | `FillStackPattern`/`FindHighWatermark` 对 `uint32_t*` 非对齐访问 | 改用 `memcpy` 逐字填充/比对水印 |
| 6 | [dds_auth.c](file:///workspace/src/middleware/dds/security/dds_auth.c#L617) | 输出参数 `*sig_len = 32` 中 `sig_len` 可能非 4 字节对齐 | 改用 `memcpy(sig_len, &out_len, sizeof(uint32_t))` |
| 7 | [fault_injection.c](file:///workspace/src/safety/saferam/fault_injection.c#L160) | 位翻转对 `volatile uint32_t*` 非对齐读写 | 改用 `volatile uint8_t*` 逐字节访问，保留 volatile 语义 |
| 8 | [fault_injection.c](file:///workspace/src/safety/saferam/fault_injection.c#L482) | OOB 读写测试对任意地址 `uint32_t` 访问 | 同上，逐字节访问 |
| 9 | [test_stack_protection.c](file:///workspace/tests/unit/safety/test_stack_protection.c#L672) | 测试故意破坏非对齐地址的金丝雀 | 两处 `*canary = ...` 改用 `memcpy` |

### 通用修复原则

- **非对齐访问**：禁止将任意整数地址强转为 `uint16_t*/uint32_t*/uint64_t*` 后直接解引用；统一用 `memcpy`（普通内存）或逐字节 `volatile uint8_t*` 访问（MMIO/故障注入）。
- **移位**：移位量必须先经 `% 位宽` 或范围守卫，禁止 ≥ 操作数位宽。
- **指针参数**：对外 API 入口对 `NULL` 指针做显式检查，不依赖调用方保证。
- **数组下标**：循环上限用 `sizeof(数组)` 而非可能含 padding 的结构体大小。

### 验证状态

三套门禁均 **193/193 通过**：native（功能）、ASan（0 内存越界）、UBSan（0 runtime error）。
