# Eep Design Document

> **Module ID**: 0x12
> **AUTOSAR Layer**: MCAL
> **AUTOSAR Version**: Classic Platform 4.4.0
> **SWS Reference**: AUTOSAR_SWS_Eep
> **Source Path**: `src/bsw/mcal/eep/`
> **Reference Document**: `docs/modules/eep.md`
> **Doc Version**: 1.0
> **Status**: Draft

---

## 1. 模块概述

Eep（EEPROM Driver）位于 MCAL 层，提供对非易失性 EEPROM 存储的异步读、写、擦除服务。本实现采用 Flash/RAM backing store 模拟 EEPROM，支持轮询模式，向上层（如 Ea / NvM）提供统一的 EEPROM 抽象接口。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS Eep | 4.4.0 | EEPROM 软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | Ea / NvM / Application | EEPROM 数据存取 | |
| 下层 | Fls（设计意图） | Flash-backed 模拟 | |
| 公共 | Det | 开发错误检测 | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│         NvM / Ea / Application      │
├─────────────────────────────────────┤
│           Eep (MCAL)                │
├─────────────────────────────────────┤
│    Fls / Flash or RAM Backing Store │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Job Manager**：接收 Read/Write/Erase 请求并维护当前作业。
- **State Manager**：维护 `EEP_UNINIT/IDLE/BUSY` 状态与 `JobResult`。
- **Address Validator**：检查地址与长度是否越界。
- **Operation Executor**：`Eep_ProcessRead/Write/Erase` 执行实际内存访问。
- **Main Function**：轮询模式下在周期任务中推进作业完成。

### 3.3 文件结构

```
src/bsw/mcal/eep/
├── include/
│   ├── Eep.h
│   └── Eep_Cfg.h
└── src/
    ├── Eep.c
    └── Eep_Lcfg.c
```

---

## 4. 状态机

### 4.1 模块状态

```
UNINIT -- Eep_Init() --> IDLE
IDLE -- Eep_Read/Write/Erase() --> BUSY
BUSY -- 作业完成 --> IDLE
BUSY -- Eep_Cancel() --> IDLE (JobResult=CANCELED)
IDLE -- Eep_DeInit() --> UNINIT
```

### 4.2 作业结果

```
PENDING -- 完成 --> JOB_OK
PENDING -- 取消 --> JOB_CANCELED
PENDING -- 错误 --> JOB_FAILED
```

---

## 5. 核心数据结构

```c
typedef uint32 Eep_AddressType;
typedef uint32 Eep_LengthType;
typedef uint8  Eep_ModeType;

typedef enum {
    EEP_JOB_OK       = 0x00U,
    EEP_JOB_PENDING  = 0x01U,
    EEP_JOB_FAILED   = 0x02U,
    EEP_JOB_CANCELED = 0x03U
} Eep_JobResultType;

typedef enum {
    EEP_UNINIT = 0x00U,
    EEP_IDLE   = 0x01U,
    EEP_BUSY   = 0x02U
} Eep_StatusType;

typedef struct {
    Eep_AddressType BaseAddress;
    Eep_LengthType  Size;
    uint32          JobCallCycle;
    uint8           PageSize;
    uint32          WriteCycleTimeMs;
    uint32          EraseCycleTimeMs;
    boolean         PollingMode;
} Eep_ConfigType;
```

内部状态：

```c
typedef enum {
    EEP_OP_NONE  = 0x00U,
    EEP_OP_READ  = 0x01U,
    EEP_OP_WRITE = 0x02U,
    EEP_OP_ERASE = 0x03U,
    EEP_OP_GC    = 0x04U
} Eep_InternalOpType;

typedef struct {
    uint32              PhysicalPage;
    uint16              VirtualPageId;
    uint16              Sequence;
    uint32              PageStatus;
} Eep_VirtualPageType;

typedef struct {
    Eep_StatusType      Status;
    Eep_JobResultType   JobResult;
    Eep_InternalOpType  CurrentOp;
    Eep_AddressType     CurrentAddress;
    uint8*              CurrentDataPtr;
    Eep_LengthType      CurrentLength;
    Eep_LengthType      ProcessedLength;
    Eep_AddressType     BaseAddress;
    Eep_LengthType      TotalSize;
    uint8               PageSize;
    const Eep_ConfigType* ConfigPtr;
    uint32              JobStartTick;
    Eep_VirtualPageType PageTable[EEP_MAX_VIRTUAL_PAGES];
    uint16              ActivePageCount;
    uint8               WriteBuffer[EEP_PAGE_SIZE];
    boolean             WriteBufferValid;
} Eep_InternalType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 | SWS ID |
|-----|------|------|----------------|--------|
| Eep_Init | `void Eep_Init(const Eep_ConfigType* ConfigPtr)` | 初始化 EEPROM 驱动 | | SWS_Eep_00001 | SWS_Eep_00001 |
| Eep_DeInit | `void Eep_DeInit(void)` | 反初始化 | | SWS_Eep_00002 | SWS_Eep_00002 |
| Eep_Read | `Std_ReturnType Eep_Read(Eep_AddressType Address, uint8* DataPtr, Eep_LengthType Length)` | 异步读 | | SWS_Eep_00003 | SWS_Eep_00003 |
| Eep_Write | `Std_ReturnType Eep_Write(Eep_AddressType Address, const uint8* DataPtr, Eep_LengthType Length)` | 异步写 | | SWS_Eep_00004 | SWS_Eep_00004 |
| Eep_Erase | `Std_ReturnType Eep_Erase(Eep_AddressType Address, Eep_LengthType Length)` | 异步擦除 | | SWS_Eep_00005 | SWS_Eep_00005 |
| Eep_Cancel | `void Eep_Cancel(void)` | 取消当前作业 | 受 `EEP_CANCEL_API` 控制 | SWS_Eep_00006 | SWS_Eep_00006 |
| Eep_GetStatus | `Eep_StatusType Eep_GetStatus(void)` | 获取模块状态 | | SWS_Eep_00007 | SWS_Eep_00007 |
| Eep_GetJobResult | `Eep_JobResultType Eep_GetJobResult(void)` | 获取作业结果 | | SWS_Eep_00008 | SWS_Eep_00008 |
| Eep_MainFunction | `void Eep_MainFunction(void)` | 周期处理函数 | | SWS_Eep_00009 | SWS_Eep_00009 |
| Eep_GetVersionInfo | `void Eep_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 版本信息 | | SWS_Eep_00010 | SWS_Eep_00010 |

### 6.2 回调函数

当前未实现回调机制，作业结果通过 `Eep_GetJobResult` 查询。

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 | SWS ID |
|-----|-----|------------|--------|
| 0x01 | Eep_Init | EEP_E_PARAM_POINTER | SWS_Eep_00001 | SWS_Eep_00011 |
| 0x02 | Eep_DeInit | - | SWS_Eep_00002 | SWS_Eep_00012 |
| 0x03 | Eep_Read | EEP_E_UNINIT / EEP_E_PARAM_POINTER / EEP_E_PARAM_LENGTH / EEP_E_PARAM_ADDRESS | SWS_Eep_00003 | SWS_Eep_00013 |
| 0x04 | Eep_Write | EEP_E_UNINIT / EEP_E_PARAM_POINTER / EEP_E_PARAM_LENGTH / EEP_E_PARAM_ADDRESS | SWS_Eep_00004 | SWS_Eep_00014 |
| 0x05 | Eep_Erase | EEP_E_UNINIT / EEP_E_PARAM_ADDRESS | SWS_Eep_00005 | SWS_Eep_00015 |
| 0x06 | Eep_Cancel | - | SWS_Eep_00006 | SWS_Eep_00016 |
| 0x07 | Eep_GetStatus | - | SWS_Eep_00007 | SWS_Eep_00017 |
| 0x08 | Eep_GetJobResult | - | SWS_Eep_00008 | SWS_Eep_00018 |
| 0x09 | Eep_MainFunction | - | SWS_Eep_00009 | SWS_Eep_00019 |
| 0x0A | Eep_GetVersionInfo | EEP_E_PARAM_POINTER | SWS_Eep_00010 | SWS_Eep_00020 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 校验 `ConfigPtr` 非空，否则报 `EEP_E_PARAM_POINTER`。
2. 调用 `Eep_ResetInternalState()` 重置内部状态。
3. 从配置拷贝 `BaseAddress`、`Size`、`PageSize`、`WriteCycleTimeMs`、`EraseCycleTimeMs`。
4. 保存配置指针。
5. 状态置 `EEP_IDLE`，`JobResult` 置 `EEP_JOB_OK`。

### 7.2 读/写/擦除请求流程

1. 校验模块已初始化、数据指针非空（擦除除外）、长度非 0。
2. `Eep_ValidateAddress(Address, Length)` 检查地址范围。
3. 若状态非 `EEP_IDLE`，返回 `E_NOT_OK`。
4. 设置当前作业：`CurrentOp`、`CurrentAddress`、`CurrentDataPtr`、`CurrentLength`、`ProcessedLength=0`。
5. `JobResult = EEP_JOB_PENDING`，`Status = EEP_BUSY`。
6. 若 `PollingMode == TRUE`，立即调用对应 `Eep_ProcessXxx()` 完成。
7. 返回 `E_OK`。

### 7.3 主函数处理流程

1. 若状态不是 `EEP_BUSY` 直接返回。
2. 根据 `CurrentOp` 分发：
   - `EEP_OP_READ` -> `Eep_ProcessRead()`
   - `EEP_OP_WRITE` -> `Eep_ProcessWrite()`
   - `EEP_OP_ERASE` -> `Eep_ProcessErase()`
3. 处理完成后状态回到 `EEP_IDLE`，`JobResult` 更新为 `EEP_JOB_OK`。

### 7.4 反初始化流程

1. 调用 `Eep_ResetInternalState()`。
2. 状态回到 `EEP_UNINIT`。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值/示例 | 说明 | |
|----|-------------|------|
| `EEP_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 | |
| `EEP_VERSION_INFO_API` | STD_ON | 版本信息 API | |
| `EEP_POLLING_MODE` | STD_ON | 轮询模式 | |
| `EEP_CANCEL_API` | STD_ON | Cancel API | |
| `EEP_BASE_ADDRESS` | 134742016U (0x08080000) | EEPROM 区域基址 | |
| `EEP_SIZE` | 65536U | EEPROM 区域大小 | |
| `EEP_PAGE_SIZE` | 8U | 页大小 | |
| `EEP_WRITE_CYCLE_TIME` | 10U | 写周期时间 ms | |
| `EEP_ERASE_CYCLE_TIME` | 20U | 擦除周期时间 ms | |
| `EEP_JOB_CALL_CYCLE` | 10U | MainFunction 调用周期 ms | |

### 8.2 链接时配置

`Eep_Lcfg.c` 中定义：

```c
static const Eep_ConfigType Eep_Config = {
    .BaseAddress = EEP_BASE_ADDRESS,
    .Size = EEP_SIZE,
    .JobCallCycle = EEP_JOB_CALL_CYCLE
};
```

### 8.3 构建后配置

当前实现不支持 Post-Build 配置变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 | |
|--------|------|----------|
| 0x00 | EEP_E_NO_ERROR | 无错误 | |
| 0x01 | EEP_E_PARAM_POINTER | 空指针入参 | |
| 0x02 | EEP_E_PARAM_ADDRESS | 地址或地址+长度越界 | |
| 0x03 | EEP_E_PARAM_LENGTH | 长度为 0 或非法 | |
| 0x04 | EEP_E_UNINIT | 模块未初始化 | |
| 0x05 | EEP_E_BUSY | 模块忙 | |
| 0x06 | EEP_E_WRITE_PROTECTED | 写保护（未实现） | |
| 0x07 | EEP_E_COMPARE_FAILED | 比较失败（未实现） | |
| 0x08 | EEP_E_ERASE_FAILED | 擦除失败（未实现） | |
| 0x09 | EEP_E_TIMEOUT | 超时（未实现） | |
| 0x0A | EEP_E_PARAM_CONFIG | 配置无效（未实现） | |

### 9.2 DEM 错误

当前未定义 DEM 事件。

### 9.3 安全机制

- 所有写/擦除请求前校验地址范围。
- 异步作业通过状态机防止并发请求。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 | |
|------|------|
| `EEP_START_SEC_VAR_CLEARED_UNSPECIFIED` | 零初始化变量（`Eep_State`） | |
| `EEP_STOP_SEC_VAR_CLEARED_UNSPECIFIED` | | |
| `EEP_START_SEC_CODE` | 代码段 | |
| `EEP_STOP_SEC_CODE` | | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | ~`EEP_MAX_VIRTUAL_PAGES * sizeof(Eep_VirtualPageType) + EEP_PAGE_SIZE + 状态` | 内部页表与写缓冲 | |
| ROM | 配置表 + 代码 | | |
| 堆栈 | 小 | 无递归 | |

---

## 11. 集成指南

- 在 `Fls` 初始化完成后调用 `Eep_Init(&Eep_Config)`（本实现直接访问 backing memory，Fls 仅为设计意图）。
- 上层通过 `Eep_Read/Eep_Write/Eep_Erase` 发起异步操作，并在 `Eep_MainFunction` 周期调度下完成。
- 轮询模式下，请求会立即执行；中断模式当前未实现。
- 通过 `Eep_GetStatus` 与 `Eep_GetJobResult` 查询作业状态。

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 | |
|----------|----------|
| `test_eep.c` | 初始化、读写擦除、地址越界、取消、状态查询、版本信息 | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 与 Ea/NvM 集成 | 验证上层通过 Eep 读写 NV 数据 | |
| 与 Fls 集成 | 验证真正的 Flash 编程流程（需补充 Fls 调用） | |
| 掉电恢复 | 验证 EEPROM 模拟的页状态与垃圾回收 | |

---

## 13. 实现说明 / TODO

- 源码中 `EEP_MODULE_ID` 定义为 `0x5FU`，与 AUTOSAR 标准 `0x12` 不一致；建议后续统一。
- 当前 `Eep_ProcessRead/Write/Erase` 直接对 `BaseAddress + Address` 进行内存读写/填充，尚未调用 `Fls` 驱动完成真正的 Flash 编程与擦除。
- `Eep_GetTick()` 当前返回 0，超时检测、写周期等待、擦除周期等待均未实现。
- 虚拟页表、垃圾回收（GC）、写缓冲、`EEP_OP_GC` 等机制在代码中保留但未真正使用。
- `EEP_E_WRITE_PROTECTED`、`EEP_E_COMPARE_FAILED`、`EEP_E_ERASE_FAILED`、`EEP_E_TIMEOUT` 等错误码未在实现中触发。
- 反初始化不检查当前是否有进行中的作业，直接重置状态。

---

## 14. 参考资料

1. AUTOSAR_SWS_EEPROMDriver.pdf
2. `docs/modules/eep.md`
3. `src/bsw/mcal/eep/`
