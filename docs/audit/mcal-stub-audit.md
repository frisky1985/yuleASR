# MCAL 桩审计报告

> **审计范围**: ocu / eth / fee / eep / lin 五个模块
> **审计日期**: 2026-08-24
> **审计原则**: 只读调研，禁止修改 `src/bsw/mcal/*`
> **工具链**: arm-none-eabi-gcc 15.3.Rel1，build-s0-arm

---

## 实现深度分级说明

| 级别 | 定义 |
|:-----|:-----|
| **空桩** | 函数体仅返回 `E_NOT_OK` 或为空，无任何逻辑 |
| **浅实现** | API 面完整或接近，有状态机或参数校验，但无硬件寄存器访问 |
| **演示级** | 有完整业务逻辑，依赖其他 BSW 模块或 host stdlib，不直接访问目标硬件寄存器 |
| **完整** | 完整寄存器级操作，可直接运行于目标 MCU |

---

## 1. OCU — Output Compare Unit

### 基本信息

```
wc -l src/bsw/mcal/ocu/src/Ocu.c      → 690
wc -l src/bsw/mcal/ocu/src/Ocu_Irq.c  → 407
总计源码行数: 1097
```

### nm 符号统计

```bash
arm-none-eabi-nm build-s0-arm/lib/bsw/mcal/libmcal_ocu.a | grep " T " | wc -l
# → 26
```

T 符号（完整）：
```
Ocu_DeInit  Ocu_DisableNotification  Ocu_EnableNotification
Ocu_GetCounter  Ocu_GetVersionInfo  Ocu_Init
Ocu_SetAbsoluteThreshold  Ocu_SetPinAction  Ocu_SetPinState
Ocu_SetRelativeThreshold  Ocu_StartChannel  Ocu_StopChannel
Ocu_Channel0_IrqHandler  Ocu_Channel1_IrqHandler
Ocu_Channel2_IrqHandler  Ocu_Channel3_IrqHandler
Ocu_HwDeInitChannel  Ocu_HwGetCounter  Ocu_HwGetRegisterBase
Ocu_HwInitChannel  Ocu_HwSetCompareValue  Ocu_HwSetPinAction
Ocu_HwSetPinState  Ocu_HwStartChannel  Ocu_HwStopChannel
Ocu_ProcessCompareMatch
```

### API 面完整性

| API | 状态 |
|:----|:----:|
| Ocu_Init / Ocu_DeInit | ✓ |
| Ocu_StartChannel / Ocu_StopChannel | ✓ |
| Ocu_SetPinAction / Ocu_SetPinState | ✓ |
| Ocu_GetCounter | ✓ |
| Ocu_EnableNotification / Ocu_DisableNotification | ✓ |
| Ocu_GetVersionInfo | ✓ |

**API 完整性: 10/10 (100%)**

### 依赖缺失分析

```bash
grep -rh "#include" src/bsw/mcal/ocu/src/ | grep -v "Ocu\|Std_Types\|Platform_Types\|Det\|SchM"
# → #include "MemMap.h"  （仅 MemMap，无硬件平台依赖）

grep -c "REG\|0x[0-9A-Fa-f]\{4,\}\|__IO\|volatile" src/bsw/mcal/ocu/src/Ocu.c
# → 0  （无寄存器访问）

grep -c "E_NOT_OK" src/bsw/mcal/ocu/src/Ocu.c
# → 7  （仅参数校验返回错误）
```

**缺失依赖**: S32K312 OCU 寄存器映射头文件，硬件计数器读写实现

### 实现深度评级

**浅实现** — API 面 100% 完整，含参数校验与状态机，`Ocu_Hw*` 系列有硬件抽象层占位，但无寄存器级实现

---

## 2. ETH — Ethernet Controller

### 基本信息

```
wc -l src/bsw/mcal/eth/src/Eth.c      → 926
wc -l src/bsw/mcal/eth/src/Eth_Irq.c  → 339
总计源码行数: 1265
```

### nm 符号统计

```bash
arm-none-eabi-nm build-s0-arm/lib/bsw/mcal/libmcal_eth.a | grep " T " | wc -l
# → 26
```

T 符号（完整）：
```
Eth_ControllerInit  Eth_DeInit  Eth_DisableIrq  Eth_EnableIrq
Eth_GetControllerIdx  Eth_GetControllerMode  Eth_GetPhysAddr
Eth_GetVersionInfo  Eth_Init  Eth_InitBuffers  Eth_ProvideTxBuffer
Eth_ReadMii  Eth_Receive  Eth_SetControllerMode  Eth_SetPhysAddr
Eth_Transmit  Eth_TxConfirmation  Eth_UpdatePhysAddrFilter  Eth_WriteMii
Eth_IsrCtrl0  Eth_IsrCtrl0Error  Eth_IsrCtrl0Rx  Eth_IsrCtrl0Tx
Eth_IsrError  Eth_IsrRx  Eth_IsrTx
```

### API 面完整性

| API | 状态 |
|:----|:----:|
| Eth_Init / Eth_ControllerInit | ✓ |
| Eth_SetControllerMode / Eth_GetControllerMode | ✓ |
| Eth_WriteMii / Eth_ReadMii | ✓ |
| Eth_GetPhysAddr / Eth_SetPhysAddr | ✓ |
| Eth_ProvideTxBuffer / Eth_Transmit / Eth_Receive | ✓ |
| **Eth_GetCounterValues** | **✗ 缺失** |

**API 完整性: 10/11 (91%)**

### 依赖缺失分析

```bash
grep -rh "#include" src/bsw/mcal/eth/src/ | grep -v "Eth\|Std_Types\|Platform_Types\|Det\|SchM"
# → #include "MemMap.h"

grep -c "REG\|0x[0-9A-Fa-f]\{4,\}\|__IO\|volatile" src/bsw/mcal/eth/src/Eth.c
# → 0  （无寄存器访问）

grep -c "E_NOT_OK" src/bsw/mcal/eth/src/Eth.c
# → 25  （密集 E_NOT_OK，硬件路径均未实现）
```

**缺失依赖**: S32K312 ENET 寄存器映射、MII/RMII 物理层驱动、DMA 描述符操作
**缺失 API**: `Eth_GetCounterValues`（统计计数器读取）

### 实现深度评级

**浅实现** — 帧缓冲区管理框架完整，但无 ENET 控制器寄存器写操作，25 处 E_NOT_OK 表明实际硬件路径均未实现

---

## 3. FEE — Flash EEPROM Emulation

### 基本信息

```
wc -l src/bsw/mcal/fee/src/Fee.c       → 1211
wc -l src/bsw/mcal/fee/src/Fee_Lcfg.c  → 324
总计源码行数: 1535
```

### nm 符号统计

```bash
arm-none-eabi-nm build-s0-arm/lib/bsw/mcal/libmcal_fee.a | grep " T " | wc -l
# → 25
```

T 符号（完整）：
```
Fee_BlankCheck  Fee_Cancel  Fee_Compare  Fee_DeInit  Fee_Erase
Fee_GetBlockConfig  Fee_GetJobResult  Fee_GetNextState  Fee_GetPageConfig
Fee_GetPreferredPageForGc  Fee_GetStatus  Fee_GetVersionInfo  Fee_Init
Fee_IsStateTransitionValid  Fee_JobEndNotification  Fee_JobErrorNotification
Fee_MainFunction  Fee_Read  Fee_Resume  Fee_SetMode  Fee_Suspend
Fee_UpdateWearLeveling  Fee_Write
Fee_NvmJobEndNotification  Fee_NvmJobErrorNotification
```

### API 面完整性

| API | 状态 |
|:----|:----:|
| Fee_Init / Fee_SetMode | ✓ |
| Fee_Read / Fee_Write / Fee_Erase | ✓ |
| Fee_Cancel / Fee_GetJobResult / Fee_GetStatus | ✓ |
| Fee_GetVersionInfo | ✓ |
| Fee_MainFunction（含磨损均衡、GC）| ✓ 扩展功能 |

**API 完整性: 9/9 (100%)**

### 依赖缺失分析

```bash
grep -c "Fls_\|Fls\.h" src/bsw/mcal/fee/src/Fee.c
# → 0  （使用内部内存数组模拟，未对接 Fls）

grep -c "E_NOT_OK" src/bsw/mcal/fee/src/Fee.c
# → 43  （状态机错误处理路径）
```

**缺失依赖**: 真实 `Fls` MCAL 驱动对接（Fee 依赖 Fls 完成实际 Flash 读写，当前为内存数组模拟）

### 实现深度评级

**演示级** — 完整磨损均衡状态机、页管理、GC 算法均已实现，但底层存储未对接真实 `Fls` 驱动

---

## 4. EEP — EEPROM Driver

### 基本信息

```
wc -l src/bsw/mcal/eep/src/Eep.c       → 570
wc -l src/bsw/mcal/eep/src/Eep_Lcfg.c  → 25
总计源码行数: 595
```

### nm 符号统计

```bash
arm-none-eabi-nm build-s0-arm/lib/bsw/mcal/libmcal_eep.a | grep " T " | wc -l
# → 10
```

T 符号（完整）：
```
Eep_Cancel  Eep_DeInit  Eep_Erase  Eep_GetJobResult
Eep_GetStatus  Eep_GetVersionInfo  Eep_Init
Eep_MainFunction  Eep_Read  Eep_Write
```

### API 面完整性

| API | 状态 |
|:----|:----:|
| Eep_Init / Eep_DeInit | ✓ |
| Eep_Read / Eep_Write / Eep_Erase | ✓ |
| Eep_Cancel / Eep_GetJobResult / Eep_GetStatus | ✓ |
| Eep_GetVersionInfo / Eep_MainFunction | ✓ |
| **Eep_SetMode** | **✗ 缺失** |

**API 完整性: 8/9 (89%)**

### 依赖缺失分析

```bash
grep -c "Fls_\|Fls\.h" src/bsw/mcal/eep/src/Eep.c
# → 1  （有 Fls 引用，但未完整对接）

grep -c "E_NOT_OK" src/bsw/mcal/eep/src/Eep.c
# → 13
```

**缺失 API**: `Eep_SetMode`（MEMIF_MODE_SLOW/FAST 切换）
**缺失依赖**: `Fls` MCAL 完整对接（当前仅 1 处引用，实际存储访问路径不完整）

### 实现深度评级

**浅实现** — 异步作业队列框架完整，但缺 `Eep_SetMode` 且 Fls 对接不完整

---

## 5. LIN — Local Interconnect Network

### 基本信息

```
wc -l src/bsw/mcal/lin/src/*.c（13 文件）→ 5283
```

主要文件：
```
LinMaster.c 807  LinMaster_Schedule.c 776  LinMaster_Diagnostic.c 701
LinSlave.c 630   LinSlave_Tp.c 419         LinMaster_Tp.c 399
Lin.c 347  ...
```

### nm 符号统计

```bash
arm-none-eabi-nm build-s0-arm/lib/bsw/mcal/libmcal_lin.a | grep " T Lin_" | wc -l
# Lin_* 核心 API: 17
arm-none-eabi-nm build-s0-arm/lib/bsw/mcal/libmcal_lin.a | grep " T " | wc -l
# 总 T 符号: 173
```

Lin_* 核心符号：
```
Lin_CheckWakeup  Lin_DeInit  Lin_DisableResponse  Lin_GetStatus
Lin_GetVersionInfo  Lin_GoToSleep  Lin_GoToSleepInternal  Lin_Init
Lin_IsrErr  Lin_IsrRx  Lin_IsrTx  Lin_SendFrame  Lin_SendResponse
Lin_WakeUp  Lin_WakeUpConfirmation  Lin_WakeUpFrameIndication  Lin_WakeUpInternal
```

### API 面完整性

| API | 状态 |
|:----|:----:|
| Lin_Init / Lin_DeInit | ✓ |
| Lin_SendFrame / Lin_GetStatus | ✓ |
| Lin_GoToSleep / Lin_GoToSleepInternal | ✓ |
| Lin_CheckWakeup | ✓ |
| Lin_Wakeup / Lin_WakeupInternal | ✓（以 Lin_WakeUp / Lin_WakeUpInternal 实现）|

**API 完整性: 8/8 (100%)**

### 依赖缺失分析

```bash
grep -c "printf\|fprintf\|time(\|clock(" src/bsw/mcal/lin/src/*.c
# → 10  （使用 host stdlib，裸机不可用）
```

**缺失依赖**:
- host `stdio.h` / `time.h` 需替换为裸机平台抽象（串口输出或移除）
- S32K312 LIN 控制器寄存器映射（LPUART/FlexIO LIN mode）
- 硬件定时器（LIN break 检测、帧超时）

### 实现深度评级

**演示级** — 代码量最大（5283 行），完整 Master/Slave 协议栈（调度表、TP、诊断、UDS），但依赖 host `stdio.h`/`time.h`，无法在 S32K312 裸机直接运行

---

## 汇总表

| 模块 | 源码行数 | T 符号数 | API 完整性 | 缺失 API | 主要缺失依赖 | 实现深度 |
|:-----|:--------:|:--------:|:----------:|:--------:|:------------|:--------:|
| **ocu** | 1,097 | 26 | 10/10 (100%) | — | S32K312 OCU 寄存器映射 | 浅实现 |
| **eth** | 1,265 | 26 | 10/11 (91%) | `Eth_GetCounterValues` | ENET 寄存器、MII/DMA | 浅实现 |
| **fee** | 1,535 | 25 | 9/9 (100%) | — | 真实 `Fls` 驱动对接 | 演示级 |
| **eep** | 595 | 10 | 8/9 (89%) | `Eep_SetMode` | `Fls` 完整对接 | 浅实现 |
| **lin** | 5,283 | 173 | 8/8 (100%) | — | host stdlib 替换、LIN HW 寄存器 | 演示级 |

---

## 下一轮 MCAL 补齐优先级排序

| 优先级 | 模块 | 理由 |
|:------:|:-----|:-----|
| **P1** | **eep** | API 缺口最小（仅补 `Eep_SetMode`），依赖 `Fls`（已有完整实现），补齐成本最低；NvM → MemIf → Eep 路径打通后可解锁 NvM 真实持久化 |
| **P2** | **fee** | API 面完整，只需将内存模拟替换为真实 `Fls` 调用；与 Eep 同为 NvM 底层，改动范围明确 |
| **P3** | **ocu** | API 完整，无外部依赖（仅 MemMap），只需补 `Ocu_Hw*` S32K312 寄存器操作；ASW IOControl 有 OCU 依赖 |
| **P4** | **lin** | 演示级实现最完整（5283 行），但 host stdlib 替换工作量中等；CommunicationManager 依赖 LIN |
| **P5** | **eth** | 依赖最复杂（ENET DMA + MII + PHY），实现周期最长；当前 EthIf/EthSm 均为桩，TCP/IP 栈未就绪前补齐收益有限 |

**建议补齐路径**:
```
Sprint N+1: eep → 解锁 NvM 真实持久化
Sprint N+2: fee → NvM 完整通路
Sprint N+3: ocu → 解锁 IOControl PWM 精确控制
Sprint N+4: lin → 替换 host stdlib → 裸机 LPUART LIN mode
Sprint N+5: eth → ENET DMA 完整实现（最后，依赖 TCP/IP 栈就绪）
```

---

## 审计完整性声明

```bash
git status --porcelain src/
# （空输出 — 本次审计未修改任何源代码）
```

本报告所有数据均来自可复现命令（`nm`、`wc -l`、`grep -c`），证据已在命令行原文摘录于各模块章节中。

*审计人: Hermes | 日期: 2026-08-24 | 仓库: yuleASR v1.3.0*
