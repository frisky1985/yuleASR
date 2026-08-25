# Flash Design Document

> **Module ID**: 0x19  
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_FlashDriver  
> **Source Path**: `src/bsw/mcal/flash/`  
> **Reference Document**: `docs/modules/FLASH.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Flash（Flash Driver）位于 MCAL 层，直接操作片上 Flash 控制器，提供 Flash 的擦除、写入、读取、比较、空白检查、写保护配置以及异步作业管理功能。该模块向上层 Fee、Fls 或 Bootloader 提供底层 Flash 访问能力，并支持 NORMAL/FAST 两种操作模式。

主要上下游模块：
- 上层：Fee、Fls、Bootloader、Calibration/刷写栈
- 下层：MCU Flash Controller、时钟、中断控制器
- 公共：Det（开发/运行时错误检测）

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Flash Driver | 4.4.0 | Flash 驱动软件规范 |
| AUTOSAR Classic Platform | 4.x | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Fee / Fls / Bootloader | 调用 Flash API 进行 NV 存储或刷写 |
| 下层 | MCU Flash Controller | 硬件寄存器操作 |
| 同层 | - | - |
| 公共 | Det | 开发与运行时错误检测（可选） |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│           Upper Layers              │
│   Fee / Fls / Bootloader / NvM      │
├─────────────────────────────────────┤
│          Flash (MCAL)               │
├─────────────────────────────────────┤
│      MCU Flash Controller           │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **硬件抽象子组件**：`Flash_Unlock` / `Flash_Lock` / `Flash_WaitForOperation` / `Flash_ProgramWord` / `Flash_ProgramDoubleWord` / `Flash_EraseSector`。
- **作业处理子组件**：`Flash_ProcessEraseJob` / `Flash_ProcessWriteJob` / `Flash_ProcessReadJob` / `Flash_ProcessCompareJob` / `Flash_ProcessBlankCheckJob`。
- **写保护子组件**：`Fls_ConfigureWriteProtection` 设置运行期写保护掩码，`Flash_EraseSector` / `Flash_ProgramWord` 操作前检查。
- **状态管理子组件**：维护 `Flash_DriverState` 中的状态、作业类型、结果等。

### 3.3 文件结构

```
src/bsw/mcal/flash/
├── include/
│   ├── Flash.h          # 公共 API、类型、错误码
│   └── Flash_Cfg.h      # 预编译配置宏与配置结构
├── src/
│   ├── Flash.c          # 主实现
│   └── Flash_Lcfg.c     # 链接时配置
└── Flash_MemMap.h       # MemMap 分区
```

---

## 4. 状态机

### 4.1 驱动状态

```
[FLASH_STATE_UNINIT] -- Flash_Init --> [FLASH_STATE_IDLE]
[FLASH_STATE_IDLE]   -- Flash_Erase/Write/Read/Compare/BlankCheck --> [FLASH_STATE_BUSY]
[FLASH_STATE_BUSY]   -- 作业完成 --> [FLASH_STATE_IDLE]
[FLASH_STATE_BUSY]   -- Flash_Cancel --> [FLASH_STATE_IDLE]
[FLASH_STATE_IDLE]   -- Flash_DeInit --> [FLASH_STATE_UNINIT]
```

### 4.2 作业类型

| 作业 | 说明 |
|------|------|
| `FLASH_JOB_ERASE` | 扇区擦除 |
| `FLASH_JOB_WRITE` | 数据写入 |
| `FLASH_JOB_READ` | 数据读取 |
| `FLASH_JOB_COMPARE` | 数据比较 |
| `FLASH_JOB_BLANK_CHECK` | 空白检查 |

---

## 5. 核心数据结构

| 类型 | 说明 |
|------|------|
| `Flash_AddressType` | 地址类型，`uint32` |
| `Flash_LengthType` | 长度类型，`uint32` |
| `Flash_OpModeType` | 操作模式：`FLASH_MODE_NORMAL` / `FLASH_MODE_FAST` |
| `Flash_JobResultType` | 作业结果：`OK` / `PENDING` / `FAILED` / `CANCELLED` / `SUSPENDED` |
| `Flash_StatusType` | 驱动状态：`UNINIT` / `IDLE` / `BUSY` / `BUSY_ERASING` / `BUSY_WRITING` / `BUSY_READING` |
| `Flash_SectorInfoType` | Sector 信息：起始地址、大小、页大小、索引、Bank、写保护、已擦除、空白检查标志 |
| `Flash_ConfigType` | 模块总配置：Sector 数组、数量、模式、地址范围、最大读写块、编程/擦除单元、通知回调 |
| `Flash_DriverStateType` | 运行时驱动状态：state、jobType、opMode、currentAddr、remainingLength、dataPtr、jobResult、initState |

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 |
|-----|------|------|------|
| Flash_Init | `void Flash_Init(const Flash_ConfigType* ConfigPtr)` | 初始化 Flash 驱动 | 使用传入配置或默认配置 |
| Flash_DeInit | `void Flash_DeInit(void)` | 反初始化 | 忙碌时拒绝 |
| Flash_Erase | `Std_ReturnType Flash_Erase(Flash_AddressType TargetAddress, Flash_LengthType Length)` | 异步擦除 | 按 Sector 逐步处理 |
| Flash_Write | `Std_ReturnType Flash_Write(Flash_AddressType TargetAddress, const uint8* SourceAddressPtr, Flash_LengthType Length)` | 异步写入 | 按 `programUnit` 对齐 |
| Flash_Read | `Std_ReturnType Flash_Read(Flash_AddressType SourceAddress, uint8* TargetAddressPtr, Flash_LengthType Length)` | 异步读取 | - |
| Flash_Compare | `Std_ReturnType Flash_Compare(Flash_AddressType SourceAddress, const uint8* TargetAddressPtr, Flash_LengthType Length)` | 异步比较 | 受 `FLASH_COMPARE_API` 控制 |
| Flash_BlankCheck | `Std_ReturnType Flash_BlankCheck(Flash_AddressType TargetAddress, Flash_LengthType Length)` | 异步空白检查 | 受 `FLASH_BLANK_CHECK_API` 控制 |
| Flash_SetMode | `void Flash_SetMode(Flash_OpModeType Mode)` | 设置操作模式 | 受 `FLASH_SET_MODE_API` 控制 |
| Flash_Cancel | `void Flash_Cancel(void)` | 取消当前作业 | 受 `FLASH_CANCEL_API` 控制 |
| Flash_Suspend | `Std_ReturnType Flash_Suspend(void)` | 挂起擦除/写入 | 受 `FLASH_SUSPEND_RESUME_API` 控制 |
| Flash_Resume | `Std_ReturnType Flash_Resume(void)` | 恢复挂起作业 | - |
| Flash_GetStatus | `Flash_StatusType Flash_GetStatus(void)` | 获取驱动状态 | - |
| Flash_GetJobResult | `Flash_JobResultType Flash_GetJobResult(void)` | 获取作业结果 | - |
| Flash_MainFunction | `void Flash_MainFunction(void)` | 主函数 | 分步处理异步作业 |
| Flash_GetVersionInfo | `void Flash_GetVersionInfo(Std_VersionInfoType*)` | 版本信息 | 受 `FLASH_VERSION_INFO_API` 控制 |
| Flash_GetSectorInfo | `const Flash_SectorInfoType* Flash_GetSectorInfo(Flash_AddressType Address)` | 获取地址所在 Sector 信息 | - |
| Flash_IsAddressValid | `boolean Flash_IsAddressValid(Flash_AddressType Address)` | 地址有效性检查 | - |
| Fls_ConfigureWriteProtection | `Std_ReturnType Fls_ConfigureWriteProtection(uint32 SectorMask, boolean Enable)` | 运行期写保护配置 | 位 N 对应 Sector N |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| `jobEndNotification` | 配置结构中的回调，作业成功完成时调用 |
| `jobErrorNotification` | 配置结构中的回调，作业失败时调用 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 1 | Flash_Init | `FLASH_E_ALREADY_INITIALIZED` / `FLASH_E_PARAM_CONFIG` |
| 2 | Flash_DeInit | `FLASH_E_UNINIT` / `FLASH_E_BUSY` |
| 4 | Flash_Erase | `FLASH_E_UNINIT` / `FLASH_E_BUSY` / `FLASH_E_PARAM_ADDRESS` / `FLASH_E_PARAM_LENGTH` |
| 5 | Flash_Write | `FLASH_E_UNINIT` / `FLASH_E_BUSY` / `FLASH_E_PARAM_POINTER` / `FLASH_E_PARAM_ADDRESS` / `FLASH_E_PARAM_LENGTH` |
| 6 | Flash_Read | 同 Write |
| 7 | Flash_Cancel | `FLASH_E_UNINIT` |
| 8 | Flash_GetJobResult | `FLASH_E_UNINIT` |
| 9 | Flash_SetMode | `FLASH_E_UNINIT` / `FLASH_E_PARAM_CONFIG` |
| 10 | Flash_Compare | `FLASH_E_UNINIT` / `FLASH_E_BUSY` / `FLASH_E_PARAM_POINTER` / `FLASH_E_PARAM_ADDRESS` / `FLASH_E_PARAM_LENGTH` |
| 11 | Flash_BlankCheck | `FLASH_E_UNINIT` / `FLASH_E_BUSY` / `FLASH_E_PARAM_ADDRESS` / `FLASH_E_PARAM_LENGTH` |
| 12 | Flash_Suspend | `FLASH_E_UNINIT` |
| 13 | Flash_Resume | `FLASH_E_UNINIT` |
| 3 | Flash_GetVersionInfo | `FLASH_E_PARAM_POINTER` |

---

## 7. 处理流程

### 7.1 初始化流程

1. `Flash_Init` 检查重复初始化，验证配置指针。
2. 选择传入配置或 `Flash_DefaultConfig`，保存到 `Flash_ConfigPtr`。
3. 初始化 `Flash_DriverState`：state=IDLE、jobType=NONE、mode=default、initState=INITIALIZED。
4. 从 `Fls_ProtectionConfig.WriteProtectionMask` 加载默认写保护掩码。
5. 解锁 Flash 控制寄存器并清除错误标志。

### 7.2 异步擦除流程

1. `Flash_Erase` 校验地址、长度与对齐（按 `FLASH_ERASE_UNIT` 对齐）。
2. 设置作业参数，state=BUSY，jobType=ERASE，result=PENDING。
3. `Flash_MainFunction` 调用 `Flash_ProcessEraseJob`。
4. `Flash_ProcessEraseJob` 找到当前地址所在 Sector，调用 `Flash_EraseSector`。
5. 每完成一个 Sector，更新 `currentAddr` 与 `remainingLength`，直至全部完成。

### 7.3 异步写入流程

1. `Flash_Write` 校验地址、长度、源指针与对齐（按 `FLASH_PROGRAM_UNIT`）。
2. 设置作业参数，state=BUSY，jobType=WRITE。
3. `Flash_ProcessWriteJob` 按模式选择 `maxWriteFastMode` / `maxWriteNormalMode` 分块。
4. 每块按 `FLASH_PROGRAM_UNIT` 组装为 `uint64`，调用 `Flash_ProgramDoubleWord`。
5. 写入完成后调用 `jobEndNotification`。

### 7.4 写保护流程

1. `Fls_ConfigureWriteProtection` 设置 `Flash_WriteProtectMask`。
2. `Flash_EraseSector` / `Flash_ProgramWord` 在操作前检查目标 Sector 掩码位。
3. 若受保护，则清除错误标志并返回 `E_NOT_OK`。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `FLASH_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `FLASH_RUNTIME_ERROR_DETECT` | STD_OFF | 运行时错误检测 |
| `FLASH_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `FLASH_SET_MODE_API` | STD_ON | SetMode API 开关 |
| `FLASH_SUSPEND_RESUME_API` | STD_OFF | 挂起/恢复 API 开关 |
| `FLASH_CANCEL_API` | STD_OFF | Cancel API 开关 |
| `FLASH_COMPARE_API` | STD_OFF | Compare API 开关 |
| `FLASH_BLANK_CHECK_API` | STD_OFF | BlankCheck API 开关 |
| `FLASH_JOB_END_NOTIFICATION` | STD_OFF | 作业结束通知 |
| `FLASH_JOB_ERROR_NOTIFICATION` | STD_OFF | 作业错误通知 |
| `FLASH_USE_ACCESS_CODE` | STD_OFF | 访问代码支持 |
| `FLASH_TOTAL_SIZE` | 1048576U | Flash 总大小 |
| `FLASH_BASE_ADDRESS` | 0x08000000U | Flash 基地址 |
| `FLASH_PROGRAM_UNIT` | 8U | 编程单元（字节） |
| `FLASH_ERASE_UNIT` | 8192U | 擦除单元（字节） |
| `FLASH_TIMEOUT_MS` | 1000U | 操作超时 |
| `FLASH_MAX_READ_NORMAL_MODE` | 100U | 正常模式每周期最大读取字节 |
| `FLASH_MAX_READ_FAST_MODE` | 50U | 快速模式每周期最大读取字节 |
| `FLASH_MAX_WRITE_NORMAL_MODE` | 500U | 正常模式每周期最大写入字节 |
| `FLASH_MAX_WRITE_FAST_MODE` | 200U | 快速模式每周期最大写入字节 |
| `FLASH_MAX_COMPARE_MODE` | 200U | 每周期最大比较字节 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| `Flash_Lcfg.c` | `Flash_SectorConfig[]`、`Fls_SectorConfig[]`、`Fls_TimingConfig`、`Fls_ProtectionConfig`、`Fls_GeneralConfig` |

### 8.3 构建后配置

当前实现未使用 Post-Build 配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 1 | `FLASH_E_UNINIT` | 模块未初始化 |
| 2 | `FLASH_E_BUSY` | 当前有作业运行 |
| 3 | `FLASH_E_ALREADY_INITIALIZED` | 重复初始化 |
| 4 | `FLASH_E_PARAM_POINTER` | 空指针 |
| 5 | `FLASH_E_PARAM_CONFIG` | 配置参数无效 |
| 6 | `FLASH_E_PARAM_ADDRESS` | 地址无效或未对齐 |
| 7 | `FLASH_E_PARAM_LENGTH` | 长度无效或未对齐 |
| 8 | `FLASH_E_WRITE_FAILED` | 写入失败 |
| 9 | `FLASH_E_ERASE_FAILED` | 擦除失败 |
| 10 | `FLASH_E_COMPARE_FAILED` | 比较失败 |
| 11 | `FLASH_E_BLANK_CHECK_FAILED` | 空白检查失败 |

### 9.2 运行时错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| - | `FLASH_E_ERASE_FAILED` | 擦除硬件失败时报告 RuntimeError |
| - | `FLASH_E_WRITE_FAILED` | 写入硬件失败时报告 RuntimeError |
| - | `FLASH_E_COMPARE_FAILED` | 比较不匹配时报告 RuntimeError |
| - | `FLASH_E_BLANK_CHECK_FAILED` | 空白检查失败时报告 RuntimeError |

### 9.3 安全机制

- 地址范围与对齐校验。
- 写保护掩码运行期检查，防止 Bootloader 等关键扇区被误擦写。
- 硬件错误标志清除与检查。
- 超时等待机制。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| `FLASH_START_SEC_CONST_UNSPECIFIED` | 常量配置数据 |
| `FLASH_STOP_SEC_CONST_UNSPECIFIED` | - |
| `FLASH_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化变量 |
| `FLASH_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | - |
| `FLASH_START_SEC_CODE` | 代码段 |
| `FLASH_STOP_SEC_CODE` | - |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | 较小 | 驱动状态、作业控制结构 |
| ROM | 中等 | 代码 + Sector 配置表 |
| 堆栈 | 中等 | 主函数分块处理与硬件轮询 |

---

## 11. 集成指南

- 上层 Fee/Fls 调用 Flash API 时，需确保目标地址位于 `FLASH_BASE_ADDRESS` ~ `FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE` 范围内。
- 初始化顺序：MCU/Port → Flash（在 Fee/Fls 之前）。
- 异步 API 返回 `E_OK` 后，需周期调用 `Flash_MainFunction` 推进作业。
- 写保护默认从 `Fls_ProtectionConfig.WriteProtectionMask` 加载，Bootloader 可在启动早期重新锁定关键扇区。
- 当前寄存器定义偏向 STM32F4 风格（`FLASH_CR`、`FLASH_SR`、`FLASH_KEYR`），若目标芯片为 S32K312 需相应调整。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `test_Flash.c` | 初始化、读写擦除、比较、空白检查、模式切换、取消、写保护、错误注入 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| Flash over MCU | 在目标芯片上验证真实擦写 |
| 写保护测试 | 验证受保护 Sector 拒绝擦写 |
| 大段写入 | 验证分块写入与进度推进 |

---

## 13. 实现说明 / TODO

- 源码头文件 `Flash.h` 与 `Flash_Cfg.h` 同时存在 `FLS_*` 与 `FLASH_*` 两套命名空间，存在重复与不一致，建议统一为 `FLASH_*` 或拆分为 Fls 与 Flash 两个独立模块。
- `Flash.h` 中 `FLS_MODULE_ID` 定义为 `0x5C`（92）；本设计文档按任务要求使用 `0x19`，二者不一致，需统一。
- 当前 `Flash_ProgramWord` 与 `Flash_EraseSector` 已实现基于 STM32F4 风格寄存器的操作，但 `Flash_ProgramDoubleWord` 未做 WRP 检查，建议补充。
- `Flash_WaitForOperation` 使用空转轮询，生产环境建议替换为 OS 延时或中断驱动。
- 当前 AUTOSAR 版本标记为 R22-11，与项目要求的 Classic Platform 4.4.0 存在差异。
- `Flash_Cfg.h` 包含大量硬件寄存器位掩码宏，跨平台迁移时需重点审查。

---

## 14. 参考资料

1. AUTOSAR_SWS_FlashDriver.pdf
2. `docs/modules/FLASH.md`
3. `src/bsw/mcal/flash/`
