# Fee Design Document

> **Module ID**: 0x1C  
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_FlashEEPROMEmulation  
> **Source Path**: `src/bsw/mcal/fee/`  
> **Reference Document**: `docs/modules/FEE.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

Fee（Flash EEPROM Emulation）位于 MCAL 层，通过在 Flash 上模拟 EEPROM 的随机字节可写特性，为上层 NvM（NVRAM Manager）提供非易失性存储服务。Fee 将逻辑 Block 映射到 Flash 的虚拟页中，支持读写、擦除、比较、空白检查、磨损均衡、垃圾回收（Garbage Collection）以及擦除挂起/恢复等机制。

主要上下游模块：
- 上层：NvM（NVRAM Manager）
- 下层：Fls（Flash Driver）
- 公共：Det（开发错误检测）、SchM（独占区保护）

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Flash EEPROM Emulation | 4.4.0 | Fee 软件规范 |
| AUTOSAR Classic Platform | 4.x | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | NvM | 调用 Fee 读写 NV Block |
| 下层 | Fls | Fee 通过 Fls 访问物理 Flash |
| 同层 | SchM | 临界区保护（`SchM_Enter_Fee_*` / `SchM_Exit_Fee_*`） |
| 公共 | Det | 开发错误检测（可选） |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│           Upper Layers              │
│              NvM                    │
├─────────────────────────────────────┤
│           Fee (MCAL)                │
├─────────────────────────────────────┤
│              Fls                    │
│        Flash Controller / Memory    │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **作业管理子组件**：接收并记录 NvM 的读写擦除等请求，在 `Fee_MainFunction` 中分步处理。
- **地址与长度校验子组件**：确保目标地址与长度位于已配置 Sector 内且对齐。
- **状态机子组件**：维护 Fee 内部状态转换（Idle / Read / Write / Erase / GC 等）。
- **磨损均衡与 GC 子组件**：跟踪擦写次数，选择目标页进行垃圾回收。
- **通知子组件**：作业完成或失败时调用 `Fee_JobEndNotification` / `Fee_JobErrorNotification`。

### 3.3 文件结构

```
src/bsw/mcal/fee/
├── include/
│   ├── Fee.h          # 公共 API、类型、错误码
│   ├── Fee_Cfg.h      # 预编译配置宏
│   ├── Fee_MemMap.h   # MemMap 分区
│   └── SchM_Fee.h     # 独占区接口
└── src/
    ├── Fee.c          # 主实现
    └── Fee_Lcfg.c     # 链接时配置
```

---

## 4. 状态机

### 4.1 驱动状态

```
[FEE_UNINIT] -- Fee_Init --> [FEE_IDLE]
[FEE_IDLE]   -- 新作业 --> [FEE_BUSY]
[FEE_BUSY]   -- 作业完成 --> [FEE_IDLE]
[FEE_BUSY]   -- Fee_DeInit --> [FEE_UNINIT]
```

### 4.2 内部作业状态

| 状态 | 说明 |
|------|------|
| `FEE_STATE_IDLE` | 空闲 |
| `FEE_STATE_READ_HEADER` | 读取 Block Header |
| `FEE_STATE_READ_DATA` | 读取 Block 数据 |
| `FEE_STATE_WRITE_HEADER` | 写入 Block Header |
| `FEE_STATE_WRITE_DATA` | 写入 Block 数据 |
| `FEE_STATE_ERASE_IMMEDIATE` | 立即擦除 |
| `FEE_STATE_GC_COPY` | 垃圾回收拷贝 |
| `FEE_STATE_GC_ERASE` | 垃圾回收擦除 |

---

## 5. 核心数据结构

| 类型 | 说明 |
|------|------|
| `Fee_AddressType` | 地址类型，`uint32` |
| `Fee_LengthType` | 长度类型，`uint32` |
| `Fee_StateType` | 驱动状态：`FEE_UNINIT` / `FEE_IDLE` / `FEE_BUSY` |
| `Fee_JobResultType` | 作业结果：`OK` / `FAILED` / `PENDING` / `CANCELLED` / `BLOCK_INCONSISTENT` / `BLOCK_INVALID` |
| `Fee_ModeType` | 操作模式：`FEE_MODE_NORMAL` / `FEE_MODE_FAST` |
| `Fee_JobType` | 作业类型：`READ` / `WRITE` / `ERASE_IMMEDIATE` / `GC_PAGE` / `NONE` |
| `Fee_InternalStateType` | 内部状态机枚举 |
| `Fee_SectorType` | Sector 配置：起始地址、大小、页大小、擦写寿命、可写/可擦标志 |
| `Fee_BlockType` | Block 配置：编号、地址、大小、寿命、立即数据标志 |
| `Fee_BlockConfigType` | 链接时 Block 配置：编号、大小、立即标志、设备索引、寿命、对齐 |
| `Fee_PageConfigType` | Page 配置：起始地址、大小、页号 |
| `Fee_ConfigType` | 模块总配置：Sector/Block/Page 数组、数量、模式、每周期最大读写字节、功能开关等 |

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 |
|-----|------|------|------|
| Fee_Init | `Std_ReturnType Fee_Init(const Fee_ConfigType* ConfigPtr)` | 初始化 Fee | NULL 配置返回 `E_NOT_OK` |
| Fee_DeInit | `Std_ReturnType Fee_DeInit(void)` | 反初始化 | 忙碌时拒绝 |
| Fee_SetMode | `Std_ReturnType Fee_SetMode(Fee_ModeType Mode)` | 设置 NORMAL/FAST 模式 | - |
| Fee_Read | `Std_ReturnType Fee_Read(Fee_AddressType SourceAddress, Fee_LengthType Length, uint8* DestPtr)` | 异步读 | 返回 `E_OK` 表示作业已接受 |
| Fee_Write | `Std_ReturnType Fee_Write(Fee_AddressType TargetAddress, Fee_LengthType Length, const uint8* SourcePtr)` | 异步写 | - |
| Fee_Erase | `Std_ReturnType Fee_Erase(Fee_AddressType TargetAddress, Fee_LengthType Length)` | 异步擦除 | - |
| Fee_Compare | `Std_ReturnType Fee_Compare(Fee_AddressType SourceAddress, Fee_LengthType Length, const uint8* DataPtr)` | 异步比较 | - |
| Fee_BlankCheck | `Std_ReturnType Fee_BlankCheck(Fee_AddressType TargetAddress, Fee_LengthType Length)` | 异步空白检查 | - |
| Fee_GetStatus | `Fee_StateType Fee_GetStatus(void)` | 获取驱动状态 | - |
| Fee_GetJobResult | `Fee_JobResultType Fee_GetJobResult(void)` | 获取最后作业结果 | - |
| Fee_Cancel | `Std_ReturnType Fee_Cancel(void)` | 取消当前作业 | 受 `FEE_CANCEL_SUPPORT` 控制 |
| Fee_Suspend | `Std_ReturnType Fee_Suspend(void)` | 挂起擦除作业 | 受 `FEE_ERASE_SUSPEND_SUPPORT` 控制 |
| Fee_Resume | `Std_ReturnType Fee_Resume(void)` | 恢复擦除作业 | - |
| Fee_GetVersionInfo | `void Fee_GetVersionInfo(Std_VersionInfoType*)` | 版本信息 | 受 `FEE_VERSION_INFO_API` 控制 |
| Fee_MainFunction | `void Fee_MainFunction(void)` | 主函数，分步处理异步作业 | 由 BSW Scheduler 周期调用 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| `Fee_JobEndNotification` | 下层/主函数在作业成功完成时调用 |
| `Fee_JobErrorNotification` | 下层/主函数在作业失败时调用 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | Fee_Init | `FEE_E_PARAM_CONFIG` / `FEE_E_ALREADY_INITIALIZED` |
| 0x01 | Fee_DeInit | `FEE_E_UNINIT` / `FEE_E_BUSY` |
| 0x02 | Fee_SetMode | `FEE_E_UNINIT` / `FEE_E_INVALID_MODE` |
| 0x03 | Fee_Read | `FEE_E_UNINIT` / `FEE_E_PARAM_POINTER` / `FEE_E_INVALID_LENGTH` / `FEE_E_INVALID_ADDRESS` / `FEE_E_BUSY` |
| 0x04 | Fee_Write | 同上 |
| 0x05 | Fee_Erase | `FEE_E_UNINIT` / `FEE_E_INVALID_LENGTH` / `FEE_E_INVALID_ADDRESS` / `FEE_E_BUSY` |
| 0x06 | Fee_Compare | 同 Read |
| 0x07 | Fee_BlankCheck | 同 Erase |
| 0x08 | Fee_GetStatus | `FEE_E_UNINIT` |
| 0x09 | Fee_GetJobResult | `FEE_E_UNINIT` |
| 0x0A | Fee_GetVersionInfo | `FEE_E_PARAM_POINTER` |
| 0x0B | Fee_Cancel | `FEE_E_UNINIT` / `FEE_E_INVALID_CANCEL` |
| 0x0C | Fee_Suspend | `FEE_E_UNINIT` / `FEE_E_INVALID_SUSPEND` |
| 0x0D | Fee_Resume | `FEE_E_UNINIT` / `FEE_E_INVALID_RESUME` |
| 0x0E | Fee_MainFunction | `FEE_E_UNINIT` |

---

## 7. 处理流程

### 7.1 初始化流程

1. `Fee_Init` 校验配置指针与重复初始化。
2. 保存配置指针，设置驱动状态为 `FEE_IDLE`。
3. 调用 `Fee_InitSectors` 初始化每个 Sector 的状态与下一次写入地址。

### 7.2 异步写流程

1. `Fee_Write` 校验地址、长度、数据指针与忙碌状态。
2. 在 SchM 临界区内记录作业信息，状态置为 `FEE_BUSY`，结果置为 `PENDING`。
3. `Fee_MainFunction` 周期调用 `Fee_ProcessWrite`。
4. `Fee_ProcessWrite` 按模式分块写入，每周期最多写 `maxWriteNormalMode` / `maxWriteFastMode` 字节。
5. 写完后设置结果并调用 `Fee_JobEndNotification`。

### 7.3 擦除挂起/恢复流程

1. `Fee_Suspend` 在作业运行时设置 `FEE_FLAG_SUSPENDED`。
2. `Fee_MainFunction` 检测到 SUSPENDED 标志后直接返回，暂停处理。
3. `Fee_Resume` 清除 SUSPENDED 标志，主函数继续处理。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `FEE_DEV_ERROR_DETECT` | STD_ON | 开发错误检测开关 |
| `FEE_VERSION_INFO_API` | STD_ON | 版本信息 API 开关 |
| `FEE_NUM_SECTORS` | 4U | 配置 Sector 数量 |
| `FEE_NUM_BLOCKS` | 16U | 配置 Block 数量 |
| `FEE_ERASE_TIMEOUT_US` | 5000000U | 擦除超时 |
| `FEE_WRITE_TIMEOUT_US` | 100000U | 写入超时 |
| `FEE_READ_TIMEOUT_US` | 10000U | 读取超时 |
| `FEE_ERASE_SUSPEND_SUPPORT` | STD_ON | 擦除挂起支持 |
| `FEE_WRITE_VERIFY_SUPPORT` | STD_ON | 写校验支持 |
| `FEE_COMPARE_SUPPORT` | STD_ON | 比较支持 |
| `FEE_BLANK_CHECK_SUPPORT` | STD_ON | 空白检查支持 |
| `FEE_CANCEL_SUPPORT` | STD_ON | 取消支持 |
| `FEE_ECC_CHECK_ENABLED` | STD_ON | ECC 检查 |
| `FEE_HW_ERROR_RECOVERY` | STD_ON | 硬件错误恢复 |
| `FEE_FLASH_BASE_ADDR` | 0x10000000U | Flash 基地址 |
| `FEE_VIRTUAL_PAGE_SIZE` | 8U | 虚拟页大小 |
| `FEE_GC_THRESHOLD_PERCENT` | 80U | GC 阈值百分比 |
| `FEE_NUMBER_OF_BLOCKS` | 10U | 链接时 Block 数量 |
| `FEE_NUMBER_OF_PAGES` | 2U | 链接时 Page 数量 |
| `FEE_GC_REPETITIONS` | 3U | GC 重复次数 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| `Fee_Lcfg.c` | `Fee_SectorType[]`、`Fee_BlockConfigType[]`、`Fee_PageConfigType[]`、`Fee_Config` |

### 8.3 构建后配置

当前实现未使用 Post-Build 配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| 0x01 | `FEE_E_PARAM_CONFIG` | 配置指针无效 |
| 0x02 | `FEE_E_PARAM_ADDRESS` | 地址无效 |
| 0x03 | `FEE_E_PARAM_LENGTH` | 长度无效 |
| 0x04 | `FEE_E_PARAM_DATA` | 数据参数无效 |
| 0x05 | `FEE_E_UNINIT` | 模块未初始化 |
| 0x06 | `FEE_E_BUSY` | 当前有作业运行 |
| 0x07 | `FEE_E_INVALID_LENGTH` | 长度未对齐或为零 |
| 0x08 | `FEE_E_INVALID_ADDRESS` | 地址不在配置 Sector 内 |
| 0x09 | `FEE_E_PARAM_POINTER` | 空指针 |
| 0x0A | `FEE_E_ALREADY_INITIALIZED` | 重复初始化 |
| 0x0B | `FEE_E_ERASE_FAILED` | 擦除失败 |
| 0x0C | `FEE_E_WRITE_FAILED` | 写入失败 |
| 0x0D | `FEE_E_READ_FAILED` | 读取失败 |
| 0x0E | `FEE_E_COMPARE_FAILED` | 比较失败 |
| 0x0F | `FEE_E_INVALID_MODE` | 模式参数非法 |
| 0x10 | `FEE_E_INVALID_SUSPEND` | 挂起条件不满足 |
| 0x11 | `FEE_E_SUSPENDED` | 已挂起 |
| 0x12 | `FEE_E_INVALID_RESUME` | 恢复条件不满足 |

### 9.2 DEM 错误

当前实现未定义 Dem 事件。

### 9.3 安全机制

- 地址范围校验、长度对齐校验、虚拟页对齐校验。
- SchM 独占区保护关键数据结构。
- 擦写寿命与磨损均衡跟踪（接口保留，待完善）。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| `FEE_START_SEC_CONFIG_DATA_UNSPECIFIED` | 配置数据段 |
| `FEE_STOP_SEC_CONFIG_DATA_UNSPECIFIED` | - |
| `FEE_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化变量 |
| `FEE_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | - |
| `FEE_START_SEC_CODE` | 代码段 |
| `FEE_STOP_SEC_CODE` | - |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | 较小 | 驱动状态、作业信息、Sector 状态数组 |
| ROM | 中等 | 代码 + Fee 配置表 + Block/Page 配置 |
| 堆栈 | 中等 | `Fee_MainFunction` 分块处理 |

---

## 11. 集成指南

- 上层 NvM 通过 Fee 的异步 API 发起请求，并在 `Fee_JobEndNotification` 中继续后续状态机。
- 下层 Fls 需在 Fee 之前初始化，并确保 `Fls_MainFunction` 与 `Fee_MainFunction` 调度周期匹配。
- 初始化顺序：Fls → Fee → NvM。
- 必须在 `Fee_Read`/`Fee_Write` 等异步 API 返回 `E_OK` 后周期调用 `Fee_MainFunction`。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `test_Fee.c` | 初始化、读写擦除比较、取消、挂起恢复、错误注入、地址/长度校验 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| Fee  over Fls | 验证 Fee 读写最终正确写入物理 Flash |
| GC 触发 | 模拟页写满，验证垃圾回收流程 |
| 擦除挂起 | 验证挂起/恢复不影响后续作业 |

---

## 13. 实现说明 / TODO

- `Fee_FlashRead` / `Fee_FlashWrite` / `Fee_FlashErase` 为硬件抽象桩，当前直接返回 `E_OK`，未对接真实 Fls。
- `Fee_ProcessCompare` 与 `Fee_ProcessBlankCheck` 为简化实现，未真正读取 Flash。
- `Fee_UpdateWearLeveling`、`Fee_GetPreferredPageForGc` 为保留接口，未实现完整磨损均衡算法。
- 源码头文件中 `FEE_MODULE_ID` 定义为 `30u`（0x1E）；本设计文档按任务要求使用 `0x1C`，二者不一致，需统一。
- 当前 AUTOSAR 版本标记为 R22-11，与项目要求的 Classic Platform 4.4.0 存在差异。

---

## 14. 参考资料

1. AUTOSAR_SWS_FlashEEPROMEmulation.pdf
2. `docs/modules/FEE.md`
3. `src/bsw/mcal/fee/`
