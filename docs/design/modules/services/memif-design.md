# MemIf Design Document

> **Module ID**: 0x1C  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_MemoryInterface  
> **Source Path**: `src/bsw/services/memif/`  
> **Reference Document**: `docs/modules/memif.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

MemIf (Memory Interface) 是 AUTOSAR BSW 服务层的存储器接口抽象模块，为上层模块（NvM）提供统一的存储器设备访问接口。MemIf 通过设备索引路由读写/擦除请求到底层存储器驱动（Fee 或 Ea），实现存储器硬件抽象。上层模块无需关心底层是 Flash EEPROM Emulation (Fee) 还是 EEPROM Abstraction (Ea)，通过统一的 MemIf API 即可完成所有存储器操作。

MemIf 模块支持以下核心能力：
- 基于设备索引的读写请求路由（Fee/Ea）
- 异步作业管理（Job 状态跟踪）
- 设备模式切换（Slow/Fast）
- 块无效化和擦除操作
- 作业取消功能

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS MemoryInterface | 4.4.0 | MemIf 模块规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | NvM | 调用 MemIf_Read/Write 进行 NV 数据存取 |
| 下层 | Fee | Flash EEPROM Emulation 驱动 |
| 下层 | Ea | EEPROM Abstraction 驱动 |
| 下层 | Det | 开发错误报告 |
| 下层 | EcuM | 初始化阶段调用 MemIf_Init |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│         NvM (NV Data Manager)       │
├─────────────────────────────────────┤
│      MemIf (Services Layer)         │
├──────────────────┬──────────────────┤
│   Fee (Flash     │  Ea (EEPROM      │
│   EEPROM Emul.)  │  Abstraction)    │
└──────────────────┴──────────────────┘
```

### 3.2 内部组件

- **Device Router**: 根据 DeviceIndex 将请求路由到 Fee 或 Ea 驱动
- **Device State Tracker**: 跟踪每个设备的状态（UNINIT/IDLE/BUSY）和作业结果
- **Validation Layer**: 验证设备索引和块号的有效性

### 3.3 文件结构

```
src/bsw/services/memif/
├── include/
│   ├── MemIf.h           # 公共 API 声明、类型定义
│   ├── MemIf_Cfg.h       # 预编译配置
│   ├── MemIf_MemMap.h    # 内存段映射
│   └── Rte_MemIf.h       # RTE 接口（可选）
└── src/
    ├── MemIf.c            # 核心实现
    └── MemIf_Lcfg.c       # 链接时配置
```

---

## 4. 状态机

### 4.1 模块状态

```
          MemIf_Init()
UNINIT ──────────────► IDLE
  ▲                      │
  │    MemIf_DeInit()    │
  └──────────────────────┘
```

### 4.2 设备作业状态

```
     Read/Write/Invalidate
IDLE ──────────────────────► BUSY
  ▲                            │
  │   Job Complete / Cancel    │
  └────────────────────────────┘
```

每个设备独立跟踪以下状态：
- **MEMIF_UNINIT (0)**: 未初始化
- **MEMIF_IDLE (1)**: 空闲，可接受新请求
- **MEMIF_BUSY (2)**: 正在执行操作

作业结果：
- **MEMIF_JOB_OK (0)**: 操作成功
- **MEMIF_JOB_PENDING (1)**: 操作进行中
- **MEMIF_JOB_CANCELED (2)**: 操作被取消
- **MEMIF_JOB_FAILED (3)**: 操作失败

---

## 5. 核心数据结构

### 5.1 设备状态类型

```c
typedef struct {
    MemIf_StatusType status;        /* 设备状态 */
    MemIf_JobResultType jobResult;  /* 作业结果 */
    boolean isInitialized;          /* 初始化标志 */
} MemIf_DeviceStateType;
```

### 5.2 设备配置类型

```c
typedef struct {
    MemIf_DeviceIndexType DeviceId;  /* 设备 ID */
    uint8 DeviceType;                /* 设备类型 (0=FEE, 1=EA) */
    uint32 BlockSize;                /* 块大小 */
    uint32 NumberOfBlocks;           /* 块数量 */
    uint32 numBlocks;                /* 块数量（冗余字段） */
} MemIf_DeviceConfigType;
```

### 5.3 全局配置类型

```c
typedef struct {
    const MemIf_DeviceConfigType* Devices;  /* 设备配置数组 */
    uint8 NumDevices;                       /* 设备数量 */
} MemIf_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | Service ID | 说明 | SWS 需求 |
|-----|-----------|------|----------|
| `void MemIf_Init(const MemIf_ConfigType* ConfigPtr)` | 0x01 | 初始化 MemIf 模块 | SWS_MemIf_00001 |
| `void MemIf_DeInit(void)` | 0x02 | 反初始化 | SWS_MemIf_00002 |
| `Std_ReturnType MemIf_Read(uint8 DeviceIndex, uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length)` | 0x03 | 从设备读取数据 | SWS_MemIf_00004 |
| `Std_ReturnType MemIf_Write(uint8 DeviceIndex, uint16 BlockNumber, const uint8* DataBufferPtr)` | 0x04 | 向设备写入数据 | SWS_MemIf_00005 |
| `void MemIf_Cancel(uint8 DeviceIndex)` | 0x05 | 取消进行中的操作 | SWS_MemIf_00006 |
| `void MemIf_MainFunction(uint8 DeviceIndex)` | 0x06 | 周期性主函数 | SWS_MemIf_00011 |
| `MemIf_StatusType MemIf_GetStatus(uint8 DeviceIndex)` | 0x07 | 获取设备状态 | SWS_MemIf_00007 |
| `MemIf_JobResultType MemIf_GetJobResult(uint8 DeviceIndex)` | 0x08 | 获取作业结果 | SWS_MemIf_00008 |
| `Std_ReturnType MemIf_EraseBlock(uint8 DeviceIndex, uint16 BlockNumber)` | 0x09 | 擦除块 | SWS_MemIf_00010 |
| `void MemIf_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 0x0A | 获取版本信息 | SWS_MemIf_00003 |
| `Std_ReturnType MemIf_InvalidateBlock(uint8 DeviceIndex, uint16 BlockNumber)` | 0x0B | 无效化块 | SWS_MemIf_00009 |
| `void MemIf_SetMode(uint8 DeviceIndex, MemIf_ModeType Mode)` | 0x0C | 设置设备模式 | SWS_MemIf_00012 |
| `uint8 MemIf_GetNumberOfDevices(void)` | 0x0D | 获取设备数量 | SWS_MemIf_00013 |

### 6.2 回调函数

MemIf 不定义回调接口。上层通过 `MemIf_GetJobResult()` 轮询作业完成状态。

### 6.3 服务 ID 与错误码

**DET Error Codes:**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| MEMIF_E_PARAM_DEVICE | 0x01 | 无效设备索引 |
| MEMIF_E_PARAM_POINTER | 0x02 | NULL 指针 |
| MEMIF_E_PARAM_BLOCK | 0x03 | 无效块号 |
| MEMIF_E_NOT_INITIALIZED | 0x04 | 模块未初始化 |
| MEMIF_E_PARAM_MODE | 0x05 | 无效模式 |
| MEMIF_E_UNINIT | 0x03 | 模块未初始化（内部） |
| MEMIF_E_PARAM_DEVICE_INDEX | 0x04 | 无效设备索引（内部） |
| MEMIF_E_ALREADY_INITIALIZED | 0x05 | 重复初始化 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查是否已初始化（重复初始化报 DET 错误）
2. 若 ConfigPtr 为 NULL，使用默认配置
3. 初始化所有设备状态为 IDLE、jobResult 为 JOB_OK
4. 设置 MemIf_ModuleInitialized = TRUE

### 7.2 读操作流程

1. 检查模块已初始化、设备索引有效、数据指针非 NULL、块号有效
2. 检查设备状态为 IDLE
3. 根据 DeviceIndex 路由到 Fee_Read() 或 Ea_Read()
4. 若底层返回 E_OK，设置设备状态为 BUSY、jobResult 为 PENDING
5. 返回 E_OK 表示请求已接受

### 7.3 MainFunction 流程

1. 检查模块已初始化、设备索引有效
2. 调用底层驱动的 MainFunction（Fee_MainFunction / Ea_MainFunction）
3. 若设备状态为 BUSY，轮询 GetJobResult
4. 若作业完成（非 PENDING），将设备状态恢复为 IDLE

---

## 8. 配置设计

### 8.1 预编译配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `MEMIF_DEV_ERROR_DETECT` | STD_ON | 启用开发错误检测 |
| `MEMIF_VERSION_INFO_API` | STD_ON | 启用版本信息 API |
| `MEMIF_NUMBER_OF_DEVICES` | 2U | 设备数量 |
| `MEMIF_FEE_DEVICE_INDEX` | 0U | Fee 设备索引 |
| `MEMIF_EA_DEVICE_INDEX` | 1U | Ea 设备索引 |
| `MEMIF_READ_TIMEOUT_MS` | 100U | 读超时 |
| `MEMIF_WRITE_TIMEOUT_MS` | 1000U | 写超时 |
| `MEMIF_ERASE_TIMEOUT_MS` | 5000U | 擦除超时 |
| `MEMIF_MAX_BLOCK_NUMBER` | 256U | 最大块号 |
| `MEMIF_MAX_BLOCK_SIZE` | 4096U | 最大块大小 |
| `MEMIF_FEE_USED` | STD_ON | Fee 驱动使能 |
| `MEMIF_EA_USED` | STD_ON | Ea 驱动使能 |

### 8.2 链接时配置

通过 Lcfg 提供设备配置表，定义 Fee 和 Ea 设备的块数量和大小。

### 8.3 构建后配置

不支持构建后配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 场景 | API | 错误码 |
|------|-----|--------|
| 重复初始化 | MemIf_Init | MEMIF_E_ALREADY_INITIALIZED |
| 模块未初始化 | 所有 API | MEMIF_E_UNINIT |
| 设备索引越界 | Read/Write/Cancel | MEMIF_E_PARAM_DEVICE_INDEX |
| 数据指针为 NULL | Read/Write | MEMIF_E_PARAM_POINTER |
| 块号无效 | Read/Write | MEMIF_E_PARAM_BLOCK |

### 9.2 DEM 错误

MemIf 不直接报告 DEM 事件。存储器操作失败可通过 NvM 上报 DEM。

### 9.3 安全机制

- 设备状态检查确保 BUSY 时不接受新请求
- 块号有效性验证防止越界访问
- 超时机制（Read/Write/Erase Timeout）防止无限等待
- MemMap 段放置确保变量在正确内存区域

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 段 | 变量 | 说明 |
|----|------|------|
| MEMIF_START_SEC_VAR_CLEARED_UNSPECIFIED | MemIf_DeviceState[], MemIf_ConfigPtr, MemIf_ModuleInitialized | 运行时变量 |
| MEMIF_START_SEC_CONST_UNSPECIFIED | MemIf_DefaultDeviceConfig, MemIf_DefaultConfig | 常量配置 |
| MEMIF_START_SEC_CODE | 所有函数 | 代码段 |

### 10.2 资源估算

- **RAM**: MemIf_DeviceState[2] ≈ 2 × 6 = 12 字节 + 配置指针 4 字节 + 初始化标志 1 字节 ≈ 20 字节
- **ROM**: ~3 KB（代码段 + 默认配置）
- **性能**: Read/Write 为 O(1) 路由操作；实际延迟取决于底层 Fee/Ea 驱动

---

## 11. 集成指南

- NvM 通过 `MemIf_Read(DeviceIndex, BlockNumber, ...)` 读取 NV 数据
- NvM 通过 `MemIf_Write(DeviceIndex, BlockNumber, ...)` 写入 NV 数据
- DeviceIndex 0 = Fee（Flash），DeviceIndex 1 = Ea（EEPROM）
- SCHM 以适当周期调用 `MemIf_MainFunction(DeviceIndex)` 处理异步操作
- 超时处理由 NvM 层负责，MemIf 仅提供状态查询

---

## 12. 测试策略

### 12.1 单元测试

- 初始化/反初始化测试（重复初始化、NULL 配置）
- 设备路由测试（Fee 设备 → Fee 驱动，Ea 设备 → Ea 驱动）
- 设备状态转换测试（IDLE → BUSY → IDLE）
- 块号边界值测试
- 作业取消测试
- 未初始化状态下的 API 调用测试

### 12.2 集成测试

- NvM → MemIf → Fee 完整读写链路
- NvM → MemIf → Ea 完整读写链路
- 并发操作不同设备的正确性
- 超时和错误恢复场景

---

## 13. 实现说明 / TODO

- MemIf_EraseBlock 和 MemIf_EraseImmediateBlock 在头文件中均有声明，需明确使用场景差异
- MemIf_SetMode 的 DET 报告使用了错误的 SID（MEMIF_SID_MAINFUNCTION），应修正
- Fee/Ea 的条件编译使用 `MEMIF_FEE_ENABLED` / `MEMIF_FEE_USED` 两种宏名，需统一
- 当前 Read/Write 仅支持异步模式，可考虑增加同步轮询模式

---

## 14. 参考资料

- AUTOSAR_SWS_MemoryInterface.pdf (R4.4.0)
- AUTOSAR_SWS_FlashEEPROMEmulation.pdf
- yuleASR MemIf 模块源码: `src/bsw/services/memif/`
