# MCAL 桩审计报告：ocu / eth / fee / eep / lin

- **审计日期**: 2026-08-03
- **项目**: yuleASR（分支 `v1.3.0`，最近提交 `4c701d7`）
- **审计范围**: `src/bsw/mcal/` 下 5 个模块：`mcal_ocu`、`mcal_eth`、`mcal_fee`、`mcal_eep`、`mcal_lin`
- **审计性质**: 只读。门禁检查 `git status --porcelain src/` 输出为空 ✓；审计期间未修改 `src/` 下任何文件，仅新建本报告。

## 方法说明

1. **API 面完整性**: 读取各模块 `include/` 主头文件，枚举函数原型，与 AUTOSAR Classic 4.4.0（Fee 为 R22-11）标准 API 名称逐一对比。
2. **依赖缺失清单**: 对 `src/*.c` 提取 `#include`，并在全仓库（`src/`、`include/`、`config/`）查找该头文件是否存在；另检查跨模块调用（如 `Fls_*`、`Eth_GetCurrentTime`）是否有定义。
3. **实现深度分级**:
   - 空桩 = 函数体为空/丢弃参数返回固定值；
   - 浅实现 = 有控制逻辑但不访问寄存器/不调 MCAL；
   - 演示级 = 有完整流程但硬件交互为模拟（RAM 伪寄存器 / printf）；
   - 完整 = 通过 volatile 指针/MMIO 访问真实寄存器。
4. **符号统计**: 任务要求 build-s1，但仓库中 **build-s1 不存在**；经检查 `build-native/`（host 编译器 `/usr/bin/cc`，CMakeCache 确认）已含全部 5 个模块的静态库，且**全部 .c 源码均不新于对应 .a**（时效性验证：`find src/bsw/mcal/$m -name "*.c" -newer libmcal_$m.a` → 0 个文件），故直接对 `build-native/lib/bsw/mcal/libmcal_*.a` 做 `nm` 统计。命令与摘录见各模块小节。
5. **行数**: `wc -l` 各模块 `src/*.c`。

> 注: 终端 `grep` 被 shell 别名到 `rtk` 工具，本审计一律使用原生 `/usr/bin/grep`，所有命令可复现。

---

## 1. Ocu（输出比较单元，Output Compare Unit）

### 1.1 API 面完整性

`Ocu.h` 声明 12 个公共 API（`grep -E "^void Ocu_|^Std_ReturnType Ocu_|^Ocu_ValueType Ocu_" ocu/include/Ocu.h`）：

| 已实现 | 状态 |
|:--|:--|
| Ocu_Init / Ocu_DeInit / Ocu_StartChannel / Ocu_StopChannel | ✅ |
| Ocu_SetPinState / Ocu_SetPinAction | ✅ |
| Ocu_SetAbsoluteThreshold / Ocu_SetRelativeThreshold | ✅ |
| Ocu_GetCounter / Ocu_DisableNotification / Ocu_EnableNotification | ✅ |
| Ocu_GetVersionInfo（受 `OCU_VERSION_INFO_API` 门控） | ✅ |

**缺失（AUTOSAR SWS_Ocu 标准 14 个 API 中的 2 个可选 API）**:

- `Ocu_GetTimeElapsed`（计算自比较匹配以来经过时间）
- `Ocu_GetRemainingTime`（计算距下次匹配剩余时间）

全仓库检索 `grep -rn "Ocu_GetTimeElapsed\|Ocu_GetRemainingTime" src/` → 无任何定义/引用。

### 1.2 依赖缺失清单

`#include` 全集（Ocu.c / Ocu_Irq.c）：`Ocu.h, Ocu_Private.h, Det.h, MemMap.h, Std_Types.h, Ocu_Cfg.h, Ocu_Lcfg.h`。
全部头文件在仓库中存在（Det.h→`src/bsw/services/det/include/`，MemMap.h→`include/autosar/`）。**无缺失 include**。

### 1.3 实现深度分级：**演示级**

- `Ocu.c`（690 行）为完整驱动结构：模块状态机（UNINIT/INITIALIZED）、逐通道初始化、DET 校验宏（`OCU_VALIDATE`/`OCU_REPORT_ERROR`）、阈值范围检查、通知回调调用链。
- `Ocu_Irq.c`（407 行）中 `Ocu_Hw*` 硬件抽象层对寄存器做读改写（`hwRegs->Control |= OCU_CTRL_ENABLE_BIT` 等），**但寄存器基址来自静态 RAM 数组**：

```c
/* Ocu_Irq.c:397-404 */
Ocu_HwRegisterType* Ocu_HwGetRegisterBase(Ocu_ChannelType Channel)
{
    /* Array of hardware base addresses - to be mapped to actual hardware */
    static Ocu_HwRegisterType Ocu_HwRegisters[OCU_NUM_CHANNELS];
    return &Ocu_HwRegisters[Channel];
}
```

即寄存器访问全部落在 RAM 伪寄存器上，**无真实 MMIO 映射**（`Ocu_ChannelConfigType.BaseAddress` 配置项未被用于寻址）。`grep -c volatile ocu/src/*.c` → 仅 Ocu_Irq.c 2 处（结构体字段声明）。

### 1.4 符号统计（nm 证据）

```
$ nm build-native/lib/bsw/mcal/libmcal_ocu.a | grep ' T ' | wc -l
26
$ nm build-native/lib/bsw/mcal/libmcal_ocu.a | grep ' T ' | awk '{print $3}'
_Ocu_Channel0_IrqHandler _Ocu_Channel1_IrqHandler _Ocu_Channel2_IrqHandler _Ocu_Channel3_IrqHandler
_Ocu_DeInit _Ocu_DisableNotification _Ocu_EnableNotification _Ocu_GetCounter _Ocu_GetVersionInfo
_Ocu_HwDeInitChannel _Ocu_HwGetCounter _Ocu_HwGetRegisterBase _Ocu_HwInitChannel _Ocu_HwSetCompareValue
_Ocu_HwSetPinAction _Ocu_HwSetPinState _Ocu_HwStartChannel _Ocu_HwStopChannel _Ocu_Init
_Ocu_ProcessCompareMatch _Ocu_SetAbsoluteThreshold _Ocu_SetPinAction _Ocu_SetPinState
_Ocu_SetRelativeThreshold _Ocu_StartChannel _Ocu_StopChannel
```

### 1.5 行数

```
$ wc -l ocu/src/*.c
690 Ocu.c
407 Ocu_Irq.c
1097 合计
```

---

## 2. Eth（以太网驱动）

### 2.1 API 面完整性

`Eth.h` 声明 23 个 API（含 3 个 ISR）: Eth_Init / Eth_DeInit / Eth_ControllerInit / Eth_GetVersionInfo / Eth_SetControllerMode / Eth_GetControllerMode / Eth_GetControllerIdx / Eth_GetPhysAddr / Eth_SetPhysAddr / Eth_UpdatePhysAddrFilter / Eth_WriteMii / Eth_ReadMii / Eth_ProvideTxBuffer / Eth_TxConfirmation / Eth_Transmit / Eth_Receive / Eth_EnableIrq / Eth_DisableIrq / Eth_InitBuffers / Eth_IsrTx / Eth_IsrRx / Eth_IsrError / Eth_GetCurrentTime。

| 状态 | 说明 |
|:--|:--|
| ✅ 实现 | 上述 21 个（nm 中均有 `_Eth_*` T 符号；另有 Eth_Irq.c 的 Eth_IsrCtrl0/Tx/Rx/Error 等 8 个控制器级 ISR） |
| ❌ **声明未实现** | **`Eth_UpdatePhysAddrFilter`** — Eth.h:234 有 extern 声明，全仓库无定义（`grep -rn "Eth_UpdatePhysAddrFilter" src/` 仅命中头文件声明） |
| ❌ **声明未实现且被上层调用** | **`Eth_GetCurrentTime`** — Eth.h:276 有 extern 声明，无定义；但 **`src/bsw/services/stbm/src/StbM.c:149` 真实调用** `Eth_GetCurrentTime(configPtr->ethControllerId, ...)`。若 StbM 目标链接本库，将产生链接期未解析符号 |
| ⚠️ 可选 API 缺失 | Eth_SetRxQueueState / Eth_GetRxQueueState / Eth_GetTxQueueState / Eth_SetWakeupEvent / Eth_CheckWakeup / Eth_CheckWakeupFlag / Eth_GetWakeupFlag（PN/队列类可选 API，未实现，通常可接受） |

### 2.2 依赖缺失清单

`#include`：`Eth.h, Eth_Cfg.h, Eth_Lcfg.h, Eth_Private.h, MemMap.h` + 头文件内 `Std_Types.h, ComStack_Types.h, Eth_GeneralTypes.h, string.h`。全部存在，**无缺失 include**。

注意：`Eth_Private.h` 定义了完整寄存器偏移表（`ETH_MAC_CR 0x0000u`、`ETH_DMA_BMR 0x1000u` 等 39 处），但 `Eth.c` 中**未使用任何一处**（`grep -c "ETH_MAC_\|ETH_DMA_" eth/src/Eth.c` → 0），寄存器定义成为死代码。

### 2.3 实现深度分级：**浅实现**

- 上层逻辑真实：Tx/Rx 缓冲环管理（`Eth_InitTxBuffers`/`Eth_AllocateTxBuffer`/`Eth_FreeTxBuffer`）、帧校验（`Eth_ValidateFrame`）、MAC 地址更新、`Eth_ProvideTxBuffer` 的 BufReq 协商流程、控制器状态表。
- 但 `Eth_Hw*` 硬件层 6 个函数全部为**注释占位空壳**（Eth.c:184-325）：

```c
static Std_ReturnType Eth_HwWriteMii(...) {
    /* Write PHY address and register address */
    /* Write data */
    /* Wait for completion */
    (void)CtrlIdx; (void)PhyAddr; (void)RegAddr; (void)Data;
    return result;          /* result = E_OK，恒成功 */
}
static Std_ReturnType Eth_HwReadMii(...) {
    ...
    *DataPtr = 0u;          /* 读 MII 恒返回 0 */
    return result;
}
```

`Eth_HwInit`/`Eth_HwSetMode` 只翻转 RAM 状态标志；`Eth_HwTransmit` 只改描述符状态。无任何寄存器/MMIO 访问（`grep -n volatile eth/src/Eth.c` → 0 处）。

### 2.4 符号统计（nm 证据）

```
$ nm build-native/lib/bsw/mcal/libmcal_eth.a | grep ' T ' | wc -l
25
$ nm build-native/lib/bsw/mcal/libmcal_eth.a | grep ' T ' | awk '{print $3}'
_Eth_ControllerInit _Eth_DeInit _Eth_DisableIrq _Eth_EnableIrq _Eth_GetControllerIdx
_Eth_GetControllerMode _Eth_GetPhysAddr _Eth_GetVersionInfo _Eth_Init _Eth_InitBuffers
_Eth_IsrCtrl0 _Eth_IsrCtrl0Error _Eth_IsrCtrl0Rx _Eth_IsrCtrl0Tx _Eth_IsrError _Eth_IsrRx
_Eth_IsrTx _Eth_ProvideTxBuffer _Eth_ReadMii _Eth_Receive _Eth_SetControllerMode _Eth_SetPhysAddr
_Eth_Transmit _Eth_TxConfirmation _Eth_WriteMii
（对比头文件：Eth_UpdatePhysAddrFilter、Eth_GetCurrentTime 缺失于符号表）
```

### 2.5 行数

```
$ wc -l eth/src/*.c
882 Eth.c
339 Eth_Irq.c
1221 合计
```

---

## 3. Fee（Flash EEPROM 模拟，Flash EEPROM Emulation）

> 注：AUTOSAR 分层中 Fee 属 ECUAL/MemIf 层且**必须基于 Fls 驱动**；本项目将其放在 `mcal/` 下。

### 3.1 API 面完整性

`Fee.h` 声明 23 个 API（17 个标准 + 6 个 helper），AUTOSAR R22-11 标准 API **全覆盖**：

| 已实现 | 状态 |
|:--|:--|
| Fee_Init / Fee_DeInit / Fee_SetMode / Fee_Read / Fee_Write / Fee_Erase | ✅ |
| Fee_Compare / Fee_BlankCheck / Fee_GetStatus / Fee_GetJobResult | ✅ |
| Fee_Cancel / Fee_Suspend / Fee_Resume / Fee_MainFunction | ✅ |
| Fee_GetVersionInfo（门控）/ Fee_JobEndNotification / Fee_JobErrorNotification | ✅ |
| helper: Fee_GetNextState / Fee_IsStateTransitionValid / Fee_UpdateWearLeveling / Fee_GetPreferredPageForGc / Fee_GetBlockConfig / Fee_GetPageConfig | ✅ |

**缺失**: 仅 R22-11 可选的 `Fee_ReadSync` / `Fee_WriteSync`（同步读写，通常 NvM 不用，可接受）。

### 3.2 依赖缺失清单

`#include`：`Fee.h, Fee_Cfg.h, Fee_MemMap.h, SchM_Fee.h, Det.h, MemMap.h`。全部存在，**无缺失 include**。

**关键缺口（依赖断裂）**: AUTOSAR 架构中 Fee 依赖 Fls（`Fls_Read/Fls_Write/Fls_Erase`），本模块 `grep -c "Fls_" fee/src/*.c` → **0**，即 Fee 完全绕开 Fls，自实现 `Fee_FlashRead/Write/Erase` 作为底层。而仓库中 Fls 本身是**完整的**（`Fls_Hw.c` 1170 行、`FLS_HW_FLASH_BASE 0x40023C00` 真实寄存器基址、28 个 T 符号）——Fee 与现成 Fls 之间没有接线。

### 3.3 实现深度分级：**浅实现**

- 上层逻辑真实且较厚：作业状态机（`Fee_InternalStateType` 7 态）、扇区/块配置表、磨损均衡（`Fee_UpdateWearLeveling`）、GC 候选页选择、地址/长度校验、JobEnd/JobError 通知链（另导出 `Fee_NvmJobEndNotification`/`Fee_NvmJobErrorNotification` 两个 NvM 侧通知）。
- 但底层 3 个 Flash 函数是**空桩**（Fee.c:391-424）：

```c
static Std_ReturnType Fee_FlashWrite(Fee_AddressType Address, const uint8* SourcePtr, Fee_LengthType Length)
{
    /* Hardware-specific flash write implementation */
    /* This would interface with the actual flash controller */
    (void)Address; (void)SourcePtr; (void)Length;
    return E_OK;      /* 恒成功——写操作静默丢失，不落盘 */
}
```

`Fee_FlashRead`/`Fee_FlashErase` 同型。后果：`Fee_Write`/`Fee_Erase` 走完整个状态机后返回 E_OK，但数据既不写入 Flash 也不写入任何 backing store。

### 3.4 符号统计（nm 证据）

```
$ nm build-native/lib/bsw/mcal/libmcal_fee.a | grep ' T ' | wc -l
25
$ nm build-native/lib/bsw/mcal/libmcal_fee.a | grep ' T ' | awk '{print $3}'
_Fee_BlankCheck _Fee_Cancel _Fee_Compare _Fee_DeInit _Fee_Erase _Fee_GetBlockConfig _Fee_GetJobResult
_Fee_GetNextState _Fee_GetPageConfig _Fee_GetPreferredPageForGc _Fee_GetStatus _Fee_GetVersionInfo
_Fee_Init _Fee_IsStateTransitionValid _Fee_JobEndNotification _Fee_JobErrorNotification _Fee_MainFunction
_Fee_NvmJobEndNotification _Fee_NvmJobErrorNotification _Fee_Read _Fee_Resume _Fee_SetMode _Fee_Suspend
_Fee_UpdateWearLeveling _Fee_Write
```

### 3.5 行数

```
$ wc -l fee/src/*.c
1211 Fee.c
 324 Fee_Lcfg.c
1535 合计
```

---

## 4. Eep（EEPROM 驱动）

### 4.1 API 面完整性

`Eep.h` 声明 10 个 API，AUTOSAR SWS_Eep 核心 API **全覆盖**：Eep_Init / Eep_DeInit / Eep_Read / Eep_Write / Eep_Erase / Eep_Cancel（门控）/ Eep_GetStatus / Eep_GetJobResult / Eep_MainFunction / Eep_GetVersionInfo（门控）。

**缺失（可选 API）**: `Eep_SetMode` / `Eep_ReadExtended` / `Eep_WriteExtended` / `Eep_EraseImmediate` —— 头文件中定义了对应 SID（`EEP_SID_SET_MODE 0x0B` 等）但**无函数原型、无实现**（全仓库 grep 仅命中 `Ea.c:140` 一行注释）。另注：`Eep_DeInit` 非 AUTOSAR 标准 API，属本模块扩展（无害）。

### 4.2 依赖缺失清单

`#include`：`Eep.h, Eep_Cfg.h, Det.h, MemMap.h, Std_Types.h`。全部存在，**无缺失 include**。

**关键缺口（依赖断裂）**: 文件头注释自称 "Dependencies: Fls.h (Flash Driver)"、实现注释自称 "Flash-backed EEPROM Emulation using Fls driver"，但 `grep -c "Fls_" eep/src/*.c` → **0**——未 include Fls.h、未调用任何 Fls API。实际 backing store 是**直接指针内存访问**：

```c
/* Eep.c:198-201 */
srcPtr = (uint8*)(uintptr)(Eep_State.BaseAddress + Eep_State.CurrentAddress);
for (i = 0U; i < Eep_State.CurrentLength; i++) { Eep_State.CurrentDataPtr[i] = srcPtr[i]; }
```

### 4.3 实现深度分级：**演示级**

- 有真实实现：异步作业模型（READ/WRITE/ERASE/GC 四态）、DET 全量参数校验、轮询模式立即执行、`Eep_MainFunction` 周期处理、写缓冲、虚拟页表结构（`Eep_VirtualPageType`/64 虚拟页）。
- 但存在明显桩点：
  - `Eep_GetTick()`（Eep.c:257-261）**恒返回 0**（"For bare-metal: this should use a system tick counter"）→ 写/擦周期计时、超时机制全部失效；
  - 页表与 GC 机制**定义了但作业路径未使用**——`Eep_ProcessRead/Write/Erase` 均为扁平内存拷贝/填充 0xFF，无页状态流转、无磨损均衡；
  - `Eep_Init` 中 "Initialize backing memory to erased state if first boot" 仅为注释，未实现。

### 4.4 符号统计（nm 证据）

```
$ nm build-native/lib/bsw/mcal/libmcal_eep.a | grep ' T ' | wc -l
10
$ nm build-native/lib/bsw/mcal/libmcal_eep.a | grep ' T ' | awk '{print $3}'
_Eep_Cancel _Eep_DeInit _Eep_Erase _Eep_GetJobResult _Eep_GetStatus _Eep_GetVersionInfo
_Eep_Init _Eep_MainFunction _Eep_Read _Eep_Write
```

### 4.5 行数

```
$ wc -l eep/src/*.c
570 Eep.c
 25 Eep_Lcfg.c
595 合计
```

---

## 5. Lin（LIN 总线驱动）

### 5.1 API 面完整性

`Lin.h` 声明 17 个标准 API，**全部实现**：Lin_Init / Lin_DeInit / Lin_GetVersionInfo / Lin_SendFrame / Lin_SendResponse / Lin_DisableResponse / Lin_WakeUp / Lin_WakeUpInternal / Lin_CheckWakeup / Lin_GetStatus / Lin_GoToSleep / Lin_GoToSleepInternal / Lin_WakeUpConfirmation / Lin_WakeUpFrameIndication / Lin_IsrTx / Lin_IsrRx / Lin_IsrErr（ISR 定义于 Lin.c:325-346）。

**缺失（可选 API）**: `Lin_CheckWakeupFlag` / `Lin_GetWakeupFlag`（全仓库无定义）。

**超标准扩展**（非 AUTOSAR Lin 必需，但体量巨大）：Master 侧（LinMaster_*：调度表 25 个导出符号、诊断 21 个、TP 11 个、HAL 10 个）、Slave 侧（LinSlave_*：配置表 24 个、TP 8 个、UDS 18 个、PID/校验和、HAL 8 个）。

### 5.2 依赖缺失清单

`#include`：`Lin.h` + 17 个子模块头（LinMaster_*.h ×7、LinSlave_*.h ×8、Lin_Cfg.h、LinSlave_Hal.h）+ `Det.h, Std_Types.h, <stdio.h>, <string.h>, <time.h>`。全部存在，**无缺失 include**。

### 5.3 实现深度分级：**演示级**

- 协议逻辑极丰富（全模块 4560 行）：LinSlave_Tp 传输协议（SF/FF/CF/FC 状态机）、LinMaster_Tp（ISO 17987 客户端）、UDS 框架（0x10 会话控制/0x11 ECU复位/0x3E TesterPresent、S3 超时、安全等级）、调度表引擎（周期/单次/暂停/切换）、PID 计算与校验和、帧配置表。
- 但**硬件层为 printf 模拟**，`LinSlave_Hal.c` 文件头自述：

```c
/* @note 此为模拟实现，实际项目需要针对具体MCU实现 */
void LinSlave_Hal_UartSendBuffer(const uint8* Buffer, uint8 Length) {
    (void)printf("[HAL] UART TX Buffer [%d bytes]: ", Length); ...   /* 打印到 stdout */
}
uint32_t LinSlave_Hal_GetTimestampMs(void) {
    static uint32_t mockTime = 0; return mockTime++;                  /* 假时间戳 */
}
```

`LinMaster_Hal.c` 同样为模拟寄存器（"模拟硬件寄存器 (实际应用中替换为真实硬件寄存器)"、`MockCurrentTimeMs++`）。`Lin.c` 自身 0 处 printf，协议层与模拟 HAL 解耦良好。
- 另外两个文件 `LinMaster_Tp.c`、`LinSlave_Uds.c` 为**单行压缩文件**（`wc -l` 计 0 行，实际内容完整，含全部 TP/UDS 实现）。

### 5.4 符号统计（nm 证据）

```
$ nm build-native/lib/bsw/mcal/libmcal_lin.a | grep ' T ' | wc -l
149
$ nm build-native/lib/bsw/mcal/libmcal_lin.a | grep ' T ' | awk '{print $3}' | head -30
_LinMaster_CalculateProtectedId _LinMaster_DeInit _LinMaster_Diag_CancelRequest
_LinMaster_Diag_ClearDiagnosticInformation _LinMaster_Diag_CommunicationControl
_LinMaster_Diag_EcuReset _LinMaster_Diag_GetLastError _LinMaster_Diag_GetResponse
_LinMaster_Diag_Init _LinMaster_Diag_IsRequestComplete _LinMaster_Diag_MainFunction
_LinMaster_Diag_ReadDTCInformation _LinMaster_Diag_ReadDataById _LinMaster_Diag_RegisterCallback
...（完整 149 个符号：LinMaster 系 ~67 + LinSlave 系 ~66 + Lin 标准 17，含 -1 个重叠，见下）
$ nm build-native/lib/bsw/mcal/libmcal_lin.a | grep ' T ' | grep -c "_LinMaster"
67
$ nm build-native/lib/bsw/mcal/libmcal_lin.a | grep ' T ' | grep -c "_LinSlave"
66
$ nm build-native/lib/bsw/mcal/libmcal_lin.a | grep ' T ' | grep -cE "_Lin_(Init|DeInit|Send|Get|GoTo|Wake|Check|Disable|Isr|GetVersion)"
17
```

### 5.5 行数

```
$ wc -l lin/src/*.c
807 LinMaster.c              630 LinSlave.c
776 LinMaster_Schedule.c     419 LinSlave_Tp.c
701 LinMaster_Diagnostic.c   401 LinSlave_CfgTable.c
347 Lin.c                    217 LinMaster_Hal.c
101 LinSlave_Checksum.c       83 LinSlave_Pid.c
 78 LinSlave_Hal.c
  0 LinMaster_Tp.c  ← 单行压缩文件，实际内容约 380 行
  0 LinSlave_Uds.c   ← 单行压缩文件，实际内容约 340 行
4560 合计（wc -l 口径；两个单行文件各计 1 行）
```

---

## 6. 汇总对照表

| 模块 | 行数 | nm T 符号 | API 完整性 | 底层硬件交互 | 深度分级 | 关键缺口 |
|:--|--:|--:|:--|:--|:--|:--|
| Ocu | 1097 | 26 | 12/14（缺 GetTimeElapsed/GetRemainingTime） | RAM 伪寄存器（static 数组） | **演示级** | HW 寄存器未映射真实地址 |
| Eth | 1221 | 25 | 21 实现 + **2 声明未实现**（UpdatePhysAddrFilter、GetCurrentTime，后者被 StbM 调用） | 无（Hw 层注释空壳） | **浅实现** | 两个悬空 API + MII/收发全空壳 |
| Fee | 1535 | 25 | 17/17 标准 API 全覆盖（缺可选 ReadSync/WriteSync） | 无（Fee_Flash* 三函数空桩） | **浅实现** | 底层 Flash 读写擦空桩、未接 Fls |
| Eep | 595 | 10 | 10/10 核心 API（缺可选 SetMode/Read/WriteExtended/EraseImmediate） | 直接指针访问 BaseAddress 内存 | **演示级** | 未接 Fls、GetTick 恒 0、页表/GC 未生效 |
| Lin | 4560 | 149 | 17/17 标准 API 全覆盖（缺可选 CheckWakeupFlag/GetWakeupFlag） | printf 模拟 HAL（Slave+Master） | **演示级** | HAL 需重写为真实 UART 寄存器驱动 |

> 参考：仓库中 `mcal_fls`（Fee/Eep 的应依赖对象）已含真实硬件层——`Fls_Hw.c` 1170 行、`FLS_HW_FLASH_BASE 0x40023C00`、157 处寄存器引用、28 个 T 符号，为**完整级**。这使 Fee/Eep 的"绕开 Fls"缺口尤其显眼。

---

## 7. 下一轮 MCAL 补齐优先级排序与理由

### P0 — Eth（最高优先）

**理由**：
1. **存在链接期断裂**：`Eth_UpdatePhysAddrFilter`、`Eth_GetCurrentTime` 头文件已声明但无实现，其中 `Eth_GetCurrentTime` 已被 `StbM.c:149` 真实调用——这是 5 个模块中唯一"上层已消费但下层缺失"的直接断链，一旦 StbM 参与链接即失败；
2. 补齐成本低：Eth_Private.h 的寄存器偏移表（ETH_MAC_CR/ETH_DMA_* 等 39 项）已就绪，只需把 `Eth_HwInit/HwSetMode/HwWriteMii/HwReadMii/HwTransmit/HwReceive` 六个空壳按偏移表落地，即可从"浅实现"跳到"完整"；
3. 与 ASW/服务层耦合高（EthIf/EthSM/StbM 均在其上）。

### P1 — Fee

**理由**：
1. **静默数据错误风险**：`Fee_FlashWrite/Erase` 空桩恒返回 E_OK，NvM→MemIf→Fee 链路会"假成功"——写入数据实际丢失，是比显式报错更危险的缺陷；
2. 上层 1211 行状态机/磨损均衡/GC 逻辑真实且无需返工，只缺 3 个底层函数接线；且仓库 **Fls 已完整**，最优做法是让 `Fee_Flash*` 改为调用 `Fls_Write/Fls_Erase/Fls_Read` 并对接 `Fee_JobEndNotification ← Fls 回调`，投入产出比最高；
3. 依赖关系：Fls 现成可用，无外部阻塞。

### P2 — Eep

**理由**：
1. 与 Fee 同属存储栈，但影响面略小（Ea/NvM 多走 Fee 链；Eep 直连场景少），且其"直接指针访问 BaseAddress"在 RAM/模拟目标上**当前可工作**，无静默假成功问题；
2. 需补：接入 Fls（或至少把 `Eep_GetTick` 接到真实 tick、让页表/GC 进入作业路径）；可顺手补齐 4 个可选 API（头文件 SID 已定义，工作量小）；
3. 行数最少（595 行），改造面可控。

### P3 — Ocu

**理由**：
1. API 缺口最小（仅 2 个可选 API），无上层断链（全仓库无调用者）；
2. 唯一实质缺口是把 `Ocu_HwGetRegisterBase` 从 static RAM 数组改为按 `Ocu_ChannelConfigType.BaseAddress` 映射真实定时器寄存器——中等工作量；
3. 依赖关系简单（无 Fls 类外部依赖），可独立推进。

### P4 — Lin（最低优先）

**理由**：
1. **API 面最完整**（17/17 标准 API + 149 个符号），协议层（TP/UDS/调度表）已经是最厚的，当前 printf 模拟 HAL 在 native 测试/演示场景下**功能可用**，无断链、无静默错误；
2. 补齐 = 重写 `LinSlave_Hal.c`/`LinMaster_Hal.c` 为真实 UART 寄存器驱动（Break 检测、字节时序、时间戳），**工作量 5 模块中最大**（HAL 接口面宽、时序敏感）；
3. 建议依赖真实 UART 驱动（mcal_uart 已有 Dma.h/Uart.c）时一并推进，独立排队。

**排序总览**：`Eth (P0) → Fee (P1) → Eep (P2) → Ocu (P3) → Lin (P4)`
主线逻辑：**先消除上层已消费的断链（Eth），再堵静默数据错误（Fee），随后补存储栈一致性（Eep），最后是低风险 API 补全（Ocu）与最大改造面（Lin）。**
