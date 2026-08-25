# Fls Design Document

> **Module ID**: 0x1A  
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_FlashDriver  
> **Source Path**: `src/bsw/mcal/fls/`  
> **Reference Document**: `docs/modules/FLS.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Fls（Flash Driver）位于 MCAL 层，是 AUTOSAR 标准 Flash 驱动模块，为上层 Fee、NvM、Bootloader 等提供异步 Flash 擦除、写入、读取、比较服务。Fls 通过维护作业控制结构，在 `Fls_MainFunction` 中分步完成大块数据的 Flash 操作，支持 NORMAL/FAST 模式切换与作业取消。

主要上下游模块：
- 上层：Fee、NvM、Bootloader
- 下层：MCU Flash Controller
- 公共：Det（开发与运行时错误检测）、MemIf（共享状态与结果类型）

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS Flash Driver | 4.4.0 | Flash 驱动软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | Fee / NvM / Bootloader | 调用 Fls 进行 NV 存储 | |
| 下层 | MCU Flash Controller | 硬件寄存器访问 | |
| 同层 | MemIf | 共享 `MemIf_StatusType`、`MemIf_JobResultType`、`MemIf_ModeType` | |
| 公共 | Det | 开发与运行时错误检测 | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│           Upper Layers              │
│        Fee / NvM / Bootloader       │
├─────────────────────────────────────┤
│           Fls (MCAL)                │
├─────────────────────────────────────┤
│      MCU Flash Controller           │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **API 入口子组件**：`Fls_Init`、`Fls_Erase`、`Fls_Write`、`Fls_Read`、`Fls_Compare`、`Fls_SetMode`、`Fls_Cancel`、`Fls_GetVersionInfo`。
- **作业调度子组件**：`Fls_MainFunction` 根据当前状态调用 `Fls_ProcessErase` / `Fls_ProcessWrite` / `Fls_ProcessRead` / `Fls_ProcessCompare`。
- **硬件抽象子组件**：`Fls_UnlockFlash`、`Fls_LockFlash`、`Fls_EraseSector`、`Fls_WritePage`、`Fls_ReadData`（当前为简化桩实现）。
- **同步读子组件**：`Fls_ReadSync` 在 `FLS_USE_ISR == STD_OFF` 时提供同步读取。

### 3.3 文件结构

```
src/bsw/mcal/fls/
├── include/
│   ├── Fls.h          # 公共 API、类型、错误码
│   ├── Fls_Cfg.h      # 预编译配置宏
│   ├── Fls_Hw.h       # 硬件抽象接口
│   └── Fls_MemMap.h   # MemMap 分区
└── src/
    ├── Fls.c          # 主实现
    └── Fls_Hw.c       # 硬件相关实现
```

---

## 4. 状态机

### 4.1 驱动状态

```
[FLS_UNINIT] -- Fls_Init --> [FLS_IDLE]
[FLS_IDLE]   -- Fls_Erase/Write/Read/Compare --> [FLS_BUSY]
[FLS_BUSY]   -- 作业完成/Cancel --> [FLS_IDLE]
[FLS_IDLE]   -- 反初始化需求 --> [FLS_UNINIT]（当前未实现 Fls_DeInit）
```

### 4.2 内部作业状态

| 状态 | 说明 | |
|------|------|
| `FLS_STATE_IDLE` | 空闲 | |
| `FLS_STATE_ERASING` | 擦除中 | |
| `FLS_STATE_WRITING` | 写入中 | |
| `FLS_STATE_READING` | 读取中 | |
| `FLS_STATE_COMPARING` | 比较中 | |

---

## 5. 核心数据结构

| 类型 | 说明 | |
|------|------|
| `Fls_AddressType` | 地址类型，`uint32` | |
| `Fls_LengthType` | 长度类型，`uint32` | |
| `Fls_StatusType` | 驱动状态：`FLS_UNINIT` / `FLS_IDLE` / `FLS_BUSY` | |
| `Fls_JobResultType` | 作业结果：`MemIf_JobResultType` 别名 | |
| `Fls_JobType` | 作业类型标量：`FLS_JOB_ERASE` / `WRITE` / `READ` / `COMPARE` 等 | |
| `Fls_OpModeType` | 操作模式：`FLS_MODE_NORMAL` / `FLS_MODE_FAST` | |
| `Fls_SectorType` | Sector 配置：起始地址、大小、页大小、解锁掩码、可写/可擦标志 | |
| `Fls_ConfigType` | 模块总配置：Sector 数组、数量、默认模式、最大读写块、通知开关 | |
| `Fls_JobControlType` | 运行时作业控制：jobType、address、读写指针、length、processed、result | |

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 | SWS ID |
|-----|------|------|----------------|--------|
| Fls_Init | `void Fls_Init(const Fls_ConfigType* ConfigPtr)` | 初始化 Fls | 检查重复初始化与空指针 | SWS_Fls_00001 | SWS_Fls_00001 |
| Fls_Erase | `Std_ReturnType Fls_Erase(Fls_AddressType TargetAddress, Fls_LengthType Length)` | 异步擦除 | Sector 对齐检查 | SWS_Fls_00002 | SWS_Fls_00002 |
| Fls_Write | `Std_ReturnType Fls_Write(Fls_AddressType TargetAddress, const uint8* SourceAddress, Fls_LengthType Length)` | 异步写入 | 拒绝只读 Sector | SWS_Fls_00003 | SWS_Fls_00003 |
| Fls_Read | `void Fls_Read(Fls_AddressType SourceAddress, uint8* TargetAddressPtr, Fls_LengthType Length)` | 异步读取 | - | SWS_Fls_00004 | SWS_Fls_00004 |
| Fls_ReadSync | `Std_ReturnType Fls_ReadSync(Fls_AddressType SourceAddress, uint8* TargetAddressPtr, Fls_LengthType Length)` | 同步读取 | `FLS_USE_ISR == STD_OFF` 时可用 | SWS_Fls_00005 | SWS_Fls_00005 |
| Fls_Compare | `void Fls_Compare(Fls_AddressType SourceAddress, const uint8* TargetAddressPtr, Fls_LengthType Length)` | 异步比较 | - | SWS_Fls_00006 | SWS_Fls_00006 |
| Fls_SetMode | `void Fls_SetMode(MemIf_ModeType Mode)` | 设置 NORMAL/FAST 模式 | - | SWS_Fls_00007 | SWS_Fls_00007 |
| Fls_GetStatus | `Fls_StatusType Fls_GetStatus(void)` | 获取驱动状态 | - | SWS_Fls_00008 | SWS_Fls_00008 |
| Fls_GetJobResult | `Fls_JobResultType Fls_GetJobResult(void)` | 获取作业结果 | - | SWS_Fls_00009 | SWS_Fls_00009 |
| Fls_Cancel | `void Fls_Cancel(void)` | 取消当前作业 | - | SWS_Fls_00010 | SWS_Fls_00010 |
| Fls_MainFunction | `void Fls_MainFunction(void)` | 主函数 | 周期处理异步作业 | SWS_Fls_00011 | SWS_Fls_00011 |
| Fls_GetVersionInfo | `void Fls_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 版本信息 | 受 `FLS_VERSION_INFO_API` 控制 | SWS_Fls_00012 | SWS_Fls_00012 |

### 6.2 回调函数

| 回调 | 说明 | |
|------|------|
| `Fls_JobEndNotification` | 配置使能时作业成功完成调用 | |
| `Fls_JobErrorNotification` | 配置使能时作业失败调用 | |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 | SWS ID |
|-----|-----|------------|--------|
| 0x00 | Fls_Init | `FLS_E_ALREADY_INITIALIZED` / `FLS_E_PARAM_CONFIG` | SWS_Fls_00001 | SWS_Fls_00013 |
| 0x01 | Fls_Erase | `FLS_E_UNINIT` / `FLS_E_BUSY` / `FLS_E_PARAM_ADDRESS` / `FLS_E_INVALID_ADDRESS` | SWS_Fls_00002 | SWS_Fls_00014 |
| 0x02 | Fls_Write | `FLS_E_UNINIT` / `FLS_E_BUSY` / `FLS_E_PARAM_DATA` / `FLS_E_PARAM_ADDRESS` | SWS_Fls_00003 | SWS_Fls_00015 |
| 0x03 | Fls_Read | `FLS_E_UNINIT` / `FLS_E_PARAM_DATA` / `FLS_E_PARAM_ADDRESS` | SWS_Fls_00004 | SWS_Fls_00016 |
| 0x04 | Fls_Compare | `FLS_E_UNINIT` / `FLS_E_PARAM_DATA` / `FLS_E_PARAM_ADDRESS` | SWS_Fls_00006 | SWS_Fls_00017 |
| 0x05 | Fls_SetMode | `FLS_E_UNINIT` | SWS_Fls_00007 | SWS_Fls_00018 |
| 0x06 | Fls_Cancel | `FLS_E_UNINIT` | SWS_Fls_00010 | SWS_Fls_00019 |
| 0x07 | Fls_GetStatus | - | SWS_Fls_00008 | SWS_Fls_00020 |
| 0x08 | Fls_GetJobResult | `FLS_E_UNINIT` | SWS_Fls_00009 | SWS_Fls_00021 |
| 0x09 | Fls_GetVersionInfo | `FLS_E_PARAM_POINTER` | SWS_Fls_00012 | SWS_Fls_00022 |

---

## 7. 处理流程

### 7.1 初始化流程

1. `Fls_Init` 检查重复初始化与空配置指针。
2. 保存配置指针，设置 `Fls_Status = FLS_IDLE`，`Fls_State = FLS_STATE_IDLE`，默认模式 `MEMIF_MODE_SLOW`。
3. 清空 `Fls_JobControl`。

### 7.2 异步擦除流程

1. `Fls_Erase` 校验初始化、忙碌状态、地址范围与 Sector 对齐。
2. 设置作业控制：`jobType = FLS_JOB_ERASE`，记录 address 与 length，result=PENDING，state=ERASING。
3. `Fls_MainFunction` 调用 `Fls_ProcessErase`。
4. `Fls_ProcessErase` 查找当前地址所在 Sector，调用 `Fls_EraseSector`。
5. 完成后 `processed += sectorSize`，若 `processed >= length` 则设置结果并调用 `Fls_FinishJob`。

### 7.3 异步写入流程

1. `Fls_Write` 校验参数，拒绝写入 `sectorWritable == FALSE` 的 Sector。
2. 设置作业控制：`jobType = FLS_JOB_WRITE`。
3. `Fls_ProcessWrite` 按当前模式选择 `maxWriteFastMode` / `maxWriteNormalMode` 分块。
4. 调用 `Fls_WritePage` 写入当前块。
5. 完成后推进 `processed`，全部完成则结束作业。

### 7.4 超时处理

- `Fls_MainFunction` 每次调用递减 `Fls_TimeoutCounter`。
- 超时后设置作业结果为 `MEMIF_JOB_FAILED`，状态回到 IDLE，并报告 `FLS_E_ERASE_FAILED` 运行时错误。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 | |
|----|--------|------|
| `FLS_VERSION_INFO_API` | STD_ON | 版本信息 API 开关 | |
| `FLS_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 | |
| `FLS_RUNTIME_ERROR_DETECT` | STD_ON | 运行时错误检测 | |
| `FLS_NUM_OF_SECTORS` | 4U | 配置 Sector 数量 | |
| `FLS_MAIN_FUNCTION_PERIOD` | 10U | 主函数周期 | |
| `FLS_TIMEOUT_VALUE` | 1000U | 超时计数 | |
| `FLS_JOB_END_NOTIFICATION` | STD_ON | 作业结束通知 | |
| `FLS_JOB_ERROR_NOTIFICATION` | STD_ON | 作业错误通知 | |
| `FLS_USE_ISR` | STD_OFF | 是否使用中断 | |
| `FLS_READ_SYNC_API` | STD_OFF | 同步读 API | |
| `FLS_COMPARE_API` | STD_ON | 比较 API | |
| `FLS_CANCEL_API` | STD_ON | 取消 API | |
| `FLS_SET_MODE_API` | STD_ON | 模式设置 API | |
| `FLS_TOTAL_SIZE` | 1048576U | Flash 总大小 | |
| `FLS_BASE_ADDRESS` | 0x08000000U | Flash 基地址 | |
| `FLS_MAX_READ_NORMAL_MODE` | 256U | 正常模式每周期最大读取字节 | |
| `FLS_MAX_READ_FAST_MODE` | 512U | 快速模式每周期最大读取字节 | |
| `FLS_MAX_WRITE_NORMAL_MODE` | 32U | 正常模式每周期最大写入字节 | |
| `FLS_MAX_WRITE_FAST_MODE` | 64U | 快速模式每周期最大写入字节 | |

### 8.2 链接时配置

| 配置表 | 说明 | |
|--------|------|
| `Fls_Lcfg.c` | `Fls_SectorConfig[]`、`Fls_Config` | |

### 8.3 构建后配置

当前实现未使用 Post-Build 配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| 0x01 | `FLS_E_PARAM_CONFIG` | 配置参数无效 | |
| 0x02 | `FLS_E_PARAM_ADDRESS` | 地址无效 | |
| 0x03 | `FLS_E_PARAM_LENGTH` | 长度无效 | |
| 0x04 | `FLS_E_PARAM_DATA` | 数据指针无效 | |
| 0x05 | `FLS_E_UNINIT` | 模块未初始化 | |
| 0x06 | `FLS_E_BUSY` | 当前有作业运行 | |
| 0x07 | `FLS_E_INVALID_LENGTH` | 长度未对齐 | |
| 0x08 | `FLS_E_INVALID_ADDRESS` | Sector 对齐失败或地址非法 | |
| 0x09 | `FLS_E_PARAM_POINTER` | 空指针 | |
| 0x0A | `FLS_E_ALREADY_INITIALIZED` | 重复初始化 | |

### 9.2 运行时错误

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| 0x01 | `FLS_E_ERASE_FAILED` | 擦除超时或失败 | |
| 0x02 | `FLS_E_WRITE_FAILED` | 写入失败 | |
| 0x03 | `FLS_E_READ_FAILED` | 读取失败 | |
| 0x04 | `FLS_E_COMPARE_FAILED` | 比较失败 | |
| 0x05 | `FLS_E_UNEXPECTED_FLASH_ID` | Flash ID 异常 | |

### 9.3 安全机制

- 地址范围校验与 Sector 对齐校验。
- 只读 Sector 写入拒绝。
- 超时监控与运行时错误报告。
- 关键区保护宏 `FLS_ENTER_CRITICAL_SECTION` / `FLS_EXIT_CRITICAL_SECTION`（当前为空，需对接 OS）。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 | |
|------|------|
| `FLS_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化变量 | |
| `FLS_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | - | |
| `FLS_START_SEC_CODE` | 代码段 | |
| `FLS_STOP_SEC_CODE` | - | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | 较小 | 配置指针、状态、作业控制、超时计数 | |
| ROM | 中等 | 代码 + Sector 配置表 | |
| 堆栈 | 中等 | 主函数分块处理 | |

---

## 11. 集成指南

- 上层 Fee/NvM 调用 Fls 异步 API 后，需由 BSW Scheduler 周期调用 `Fls_MainFunction`。
- 初始化顺序：MCU/Port → Fls（在 Fee 之前）。
- `Fls_ReadSync` 仅在 `FLS_USE_ISR == STD_OFF` 时编译，使用时需确保无其他作业在进行。
- 当前 `Fls_EraseSector` / `Fls_WritePage` / `Fls_ReadData` 为简化桩实现（`Fls_WritePage` 使用 `REG_WRITE8`、`Fls_ReadData` 使用 `REG_READ8`），生产环境需对接真实 Flash 控制器。
- `FLS_ENTER_CRITICAL_SECTION` / `FLS_EXIT_CRITICAL_SECTION` 当前为空，需根据 OS 或中断控制器实现。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 | |
|----------|----------|
| `test_Fls.c` | 初始化、擦除、写入、读取、比较、模式切换、取消、同步读、超时 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| Fls over Fee | 验证 Fee 通过 Fls 完成 NV Block 读写 | |
| 只读 Sector 拒绝 | 验证 `sectorWritable == FALSE` 时写入被拒绝 | |
| 超时恢复 | 验证超时后驱动回到 IDLE 并报告错误 | |

---

## 13. 实现说明 / TODO

- 源码头文件 `Fls.h` 中 `FLS_MODULE_ID` 定义为 `92u`（0x5C）；本设计文档按任务要求使用 `0x1A`，二者不一致，需统一。
- 当前 AUTOSAR 版本标记为 R22-11，与项目要求的 Classic Platform 4.4.0 存在差异。
- `Fls_EraseSector` / `Fls_WritePage` 为硬件抽象桩，未操作真实 Flash 寄存器；`Fls_WritePage` 使用 `REG_WRITE8` 便于 Host Test Mock，但不代表真实 Flash 编程时序。
- `Fls.h` 在 `MEMIF_H` 未定义时自行定义 `MemIf_ModeType`，与标准 MemIf 模块存在潜在冲突。
- 未实现 `Fls_DeInit` API。
- `Fls_MainFunction` 超时错误统一报告为 `FLS_E_ERASE_FAILED`，建议按作业类型区分。

---

## 14. 参考资料

1. AUTOSAR_SWS_FlashDriver.pdf
2. `docs/modules/FLS.md`
3. `src/bsw/mcal/fls/`
