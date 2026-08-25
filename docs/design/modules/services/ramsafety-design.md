# RamSafety Design Document

> **Module ID**: 0x99  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_RAMTest  
> **Source Path**: `src/bsw/services/ramsafety/`  
> **Reference Document**: `docs/modules/ramsafety.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

RamSafety (RAM Safety) 是 AUTOSAR BSW 服务层的 RAM 安全检查模块，提供启动时和运行时 RAM 完整性检测功能。该模块实现 March C- 算法、Walking Pattern 检查、地址线/数据线测试和 CRC 验证等多种 RAM 测试方法，确保 RAM 存储器的可靠性。RamSafety 是 ASIL-D 安全链路的关键组件，满足 ISO 26262 对 RAM 故障检测的要求。

RamSafety 模块支持以下核心能力：
- 启动时完整 RAM 测试（March C- + Walking Pattern）
- 运行时周期性 RAM 监控（CRC 验证 + 抽样检查）
- 多区域独立配置和检查
- 硬件 ECC 状态监控
- 测试统计信息收集
- 安全状态响应（检测到故障时进入安全状态）
- 按需触发测试

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS RAMTest | 4.4.0 | RamSafety 模块规范 |
| ISO 26262-5 | 2018 | ASIL-D RAM 故障检测要求 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | EcuM / SchM | 启动时调用 RunStartupTest，运行时调用 MainFunction |
| 下层 | Platform_RamSafety | 平台抽象层（CRC 计算、ECC 检查、安全状态进入） |
| 下层 | Det | 开发错误报告 |
| 下层 | Mcal | 中断控制（DisableAllInterrupts/EnableAllInterrupts） |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│     EcuM / SchM / Safety Monitor    │
├─────────────────────────────────────┤
│    RamSafety (Services Layer)       │
├─────────────────────────────────────┤
│  Platform_RamSafety | Det | Mcal    │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **March C- Engine**: 实现 6 阶段 March C- 算法，检测 stuck-at 和 coupling 故障
- **Walking Pattern Engine**: 使用 8 种数据模式（0x00, 0xFF, 0x55, 0xAA, 0x01, 0xFE, 0x80, 0x7F）进行写读验证
- **Address/Data Line Tester**: 地址线和数据线独立测试
- **CRC Verifier**: 运行时 CRC 校验，与存储的基准 CRC 比较
- **Statistics Collector**: 测试通过/失败计数、错误地址记录
- **Safety State Manager**: 检测到严重故障时触发安全状态

### 3.3 文件结构

```
src/bsw/services/ramsafety/
├── include/
│   ├── RamSafety.h        # 公共 API 声明、类型定义
│   ├── RamSafety_Cfg.h    # 预编译配置
│   └── RamSafety_MemMap.h # 内存段映射
└── src/
    ├── RamSafety.c         # 核心实现
    └── RamSafety_Cfg.c     # 配置数据
```

---

## 4. 状态机

```
                    RamSafety_Init()
      UNINIT ──────────────────────► INIT
        ▲                               │
        │                               │ RunStartupTest()
        │                               ▼
        │                         STARTUP_TEST
        │                           │       │
        │               (pass)──────┘       └──(fail)──┐
        │                                               │
        │              ┌────────────────────────────────┘
        │              │
        │              ▼
        │          ERROR ◄─── (runtime fail)
        │           │
        │    DeInit()│
        └───────────┘

     STARTUP_TEST ──(pass)──► ACTIVE
                                │
                    MainFunction()
                    (周期性CRC检查)
```

RamSafety 模块有 5 个状态：
- **RAMSAFETY_STATE_UNINIT (0)**: 未初始化
- **RAMSAFETY_STATE_INIT (1)**: 初始化完成，等待启动测试
- **RAMSAFETY_STATE_STARTUP_TEST (2)**: 启动测试进行中
- **RAMSAFETY_STATE_ACTIVE (3)**: 运行时监控活跃
- **RAMSAFETY_STATE_ERROR (4)**: 检测到错误，进入安全状态

---

## 5. 核心数据结构

### 5.1 RAM 区域配置

```c
typedef struct {
    uint32 startAddress;       /* 起始地址 (4字节对齐) */
    uint32 size;               /* 大小 (字节) */
    uint8 priority;            /* 优先级 (0-255) */
    boolean startupTest;       /* 启动时检查使能 */
    boolean runtimeTest;       /* 运行时检查使能 */
    boolean eccEnabled;        /* 硬件 ECC 使能 */
    uint32 crcSeed;            /* CRC 初始值 */
} RamSafety_RegionType;
```

### 5.2 测试统计信息

```c
typedef struct {
    uint32 testsPassed;          /* 通过次数 */
    uint32 testsFailed;          /* 失败次数 */
    uint32 lastErrorAddress;     /* 最后错误地址 */
    uint8 lastErrorPattern;      /* 最后错误模式 */
    RamSafety_TestType lastTestType; /* 最后测试类型 */
    uint32 totalBytesTested;     /* 总测试字节数 */
} RamSafety_StatisticsType;
```

### 5.3 全局配置

```c
typedef struct {
    const RamSafety_RegionType* regions;  /* 区域配置数组 */
    uint8 numRegions;                     /* 区域数量 */
    uint16 runtimePeriodMs;               /* 运行时检查周期 */
    boolean useHardwareEcc;               /* 使用硬件 ECC */
    uint8 maxRuntimeRegionsPerCycle;      /* 每周期最多检查区域数 */
} RamSafety_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | Service ID | SWS 需求 | 说明 |
|-----|-----------|----------|------|
| `Std_ReturnType RamSafety_Init(const RamSafety_ConfigType* config)` | 0x01 | SWS_RamSafety_00001 | 初始化 |
| `Std_ReturnType RamSafety_DeInit(void)` | 0x02 | SWS_RamSafety_00002 | 反初始化 |
| `RamSafety_StateType RamSafety_GetState(void)` | - | SWS_RamSafety_00003 | 获取状态 |
| `Std_ReturnType RamSafety_RunStartupTest(RamSafety_ProgressCallbackType progressCb)` | 0x03 | SWS_RamSafety_00010 | 启动测试 |
| `void RamSafety_MainFunction(void)` | 0x04 | SWS_RamSafety_00011 | 运行时主函数 |
| `RamSafety_ResultType RamSafety_TriggerTest(RamSafety_TestType, uint8, RamSafety_ErrorCallbackType)` | 0x05 | SWS_RamSafety_00012 | 按需触发测试 |
| `Std_ReturnType RamSafety_VerifyRegion(uint8 regionId)` | 0x06 | SWS_RamSafety_00020 | CRC 验证区域 |
| `Std_ReturnType RamSafety_VerifyRange(uint32 startAddr, uint32 size)` | - | SWS_RamSafety_00021 | 验证地址范围 |
| `Std_ReturnType RamSafety_GetStatistics(RamSafety_StatisticsType* stats)` | - | SWS_RamSafety_00030 | 获取统计 |
| `Std_ReturnType RamSafety_ClearStatistics(void)` | - | SWS_RamSafety_00031 | 清除统计 |
| `Std_ReturnType RamSafety_CheckEccStatus(uint8, boolean*, uint32*)` | - | SWS_RamSafety_00032 | ECC 状态检查 |
| `void RamSafety_EnterSafeState(uint8 reason)` | - | SWS_RamSafety_00040 | 进入安全状态 |
| `void RamSafety_GetVersionInfo(Std_VersionInfoType* versioninfo)` | - | SWS_RamSafety_00050 | 版本信息 |

### 6.2 回调函数

```c
/* 进度回调 */
typedef void (*RamSafety_ProgressCallbackType)(
    uint8 percent,
    const RamSafety_RegionType* region
);

/* 错误回调 */
typedef void (*RamSafety_ErrorCallbackType)(
    RamSafety_TestType testType,
    uint32 address,
    uint8 expected,
    uint8 actual
);
```

### 6.3 服务 ID 与错误码

**DET Error Codes:**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| RAMSAFETY_E_NO_ERROR | 0x00 | 无错误 |
| RAMSAFETY_E_INIT_FAILED | 0x01 | 初始化失败 |
| RAMSAFETY_E_INVALID_REGION | 0x02 | 无效区域 |
| RAMSAFETY_E_TEST_FAILED | 0x03 | 测试失败 |
| RAMSAFETY_E_MARCH_FAILED | 0x04 | March C- 失败 |
| RAMSAFETY_E_WALK_FAILED | 0x05 | Walking Pattern 失败 |
| RAMSAFETY_E_ECC_ERROR | 0x06 | ECC 错误 |
| RAMSAFETY_E_TIMEOUT | 0x07 | 超时 |
| RAMSAFETY_E_INVALID_STATE | 0x08 | 无效状态 |

**测试类型:**

| 类型 | 说明 |
|------|------|
| RAMSAFETY_TEST_MARCH_C | March C- 完整检查 |
| RAMSAFETY_TEST_WALK_PATTERN | Walking Pattern |
| RAMSAFETY_TEST_ADDR_LINE | 地址线测试 |
| RAMSAFETY_TEST_DATA_LINE | 数据线测试 |
| RAMSAFETY_TEST_QUICK | 快速检查 |
| RAMSAFETY_TEST_FULL | 完整检查 (March C- + Walk) |

---

## 7. 处理流程

### 7.1 启动测试流程

1. 检查状态为 INIT，配置指针有效
2. 设置状态为 STARTUP_TEST
3. 遍历所有配置为 startupTest=TRUE 的区域
4. 对每个区域：
   a. 通知进度回调
   b. 执行 March C- 算法（6 阶段）
   c. 执行 Walking Pattern 检查（8 种模式）
   d. 若 ECC 使能，检查硬件 ECC 状态
   e. 更新统计信息
5. 通知最终进度 100%
6. 若全部通过，状态转为 ACTIVE
7. 若任一失败，状态转为 ERROR，调用 EnterSafeState

### 7.2 运行时 MainFunction 流程

1. 检查状态为 ACTIVE
2. 递增计时器，检查是否到达周期
3. 确定本周期要检查的区域（maxRuntimeRegionsPerCycle）
4. 对每个区域：
   a. 执行 CRC 验证（RamSafety_VerifyRegion）
   b. 若 CRC 验证失败，执行更详细的 March C- 检查
5. 循环检查所有使能运行时检查的区域

### 7.3 March C- 算法

6 阶段测试序列：
1. ↑ (w0) - 从低地址到高地址写 0x00
2. ↑ (r0,w1,r1) - 读 0x00, 写 0xFF, 读 0xFF
3. ↑ (r1,w0,r0) - 读 0xFF, 写 0x00, 读 0x00
4. ↓ (r0,w1,r1) - 从高到低读 0x00, 写 0xFF, 读 0xFF
5. ↓ (r1,w0,r0) - 从高到低读 0xFF, 写 0x00, 读 0x00
6. ↓ (r0) - 从高到低读 0x00

---

## 8. 配置设计

### 8.1 预编译配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `RAMSAFETY_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `RAMSAFETY_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `RAMSAFETY_MAX_REGIONS` | 16U | 最大区域数量 |
| `RAMSAFETY_DEFAULT_PERIOD_MS` | 100U | 默认检查周期 |
| `RAMSAFETY_CFG_NUM_REGIONS` | 4U | 配置区域数 |
| `RAMSAFETY_CFG_STARTUP_TIMEOUT_MS` | 5000U | 启动测试超时 |
| `RAMSAFETY_CFG_RUNTIME_PERIOD_MS` | 100U | 运行时周期 |
| `RAMSAFETY_CFG_HARDWARE_ECC` | STD_ON | 硬件 ECC 使能 |
| `RAMSAFETY_CFG_CRC_VERIFICATION` | STD_ON | CRC 验证使能 |
| `RAMSAFETY_CFG_CRC_POLYNOMIAL` | 79764919U | CRC 多项式 |
| `RAMSAFETY_CFG_MAX_REGIONS_PER_CYCLE` | 1U | 每周期最多区域数 |
| `RAMSAFETY_CFG_FAILURE_THRESHOLD` | 3U | 失败阈值 |

**预定义区域:**

| 区域 ID | 名称 | 基地址 | 大小 |
|---------|------|--------|------|
| 0 | DTCM | 0x20000000 | 128 KB |
| 1 | SRAM | 0x20400000 | 256 KB |
| 2 | FlexRAM | 0x14000000 | 512 KB |
| 3 | Stack | 0x20020000 | 32 KB |

### 8.2 链接时配置

通过 `RamSafety_Cfg.c` 提供区域配置数组，定义各 RAM 区域的地址、大小和测试使能。

### 8.3 构建后配置

不支持构建后配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 场景 | API | 错误码 |
|------|-----|--------|
| config 为 NULL | RamSafety_Init | RAMSAFETY_E_INIT_FAILED |
| 重复初始化 | RamSafety_Init | RAMSAFETY_E_INVALID_STATE |
| 未初始化时调用 | DeInit | RAMSAFETY_E_INVALID_STATE |
| 无效区域 ID | VerifyRegion | RAMSAFETY_E_INVALID_REGION |
| 统计指针为 NULL | GetStatistics | RAMSAFETY_E_PARAM_POINTER |

### 9.2 DEM 错误

RamSafety 检测到 RAM 故障时通过 `RamSafety_EnterSafeState()` 触发安全响应，可联动 EcuM/FCCU 报告 DEM 事件。

### 9.3 安全机制

- **ASIL-D 安全等级**: 所有关键函数标注 ASIL-D
- **安全魔数验证**: SafetyMagic 变量（INIT=0xA55A3CC3, ACTIVE=0x3CC3A55A）用于验证模块状态完整性
- **中断保护**: 初始化和去初始化使用 Mcal_DisableAllInterrupts/EnableAllInterrupts
- **volatile 状态变量**: State 和 SafetyMagic 使用 volatile 修饰防止编译器优化
- **March C- 算法**: 检测 stuck-at fault 和 coupling fault
- **安全状态响应**: March C- 失败直接触发 EnterSafeState
- **平台抽象**: 通过 Platform_RamSafety 接口支持不同硬件平台

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 段 | 变量 | 说明 |
|----|------|------|
| RAMSAFETY_START_SEC_VAR_INIT_UNSPECIFIED | RamSafety_State, RamSafety_CurrentConfig, RamSafety_SafetyMagic | 初始化变量 |
| RAMSAFETY_START_SEC_VAR_CLEARED_UNSPECIFIED | RamSafety_Stats, RamSafety_NextRegionIndex, RamSafety_Timer | 清除变量 |
| RAMSAFETY_START_SEC_CODE | 所有函数 | 代码段 |
| RAMSAFETY_START_SEC_CONFIG_DATA_UNSPECIFIED | RamSafety_Config, RamSafety_RegionConfig | 配置数据 |

### 10.2 资源估算

- **RAM**: RamSafety_Stats (24B) + 内部变量 (~20B) ≈ 44 字节（测试区域数据不在 MemIf 管理范围）
- **ROM**: ~5 KB（代码段，含 March C- 算法和 Walking Pattern）
- **性能**: March C- 为 O(6N) N=区域大小；Walking Pattern 为 O(8×2) 仅检查两端；CRC 验证为 O(N)

---

## 11. 集成指南

- EcuM 在启动序列中调用 `RamSafety_Init()` → `RamSafety_RunStartupTest()`
- SCHM 以 100ms 周期调用 `RamSafety_MainFunction()`
- 启动测试期间系统处于阻塞状态，需通过进度回调通知上层
- 运行时 CRC 验证失败时自动触发 March C- 详细检查
- March C- 失败将直接触发安全状态，系统需准备安全停机处理
- 区域配置需覆盖所有安全关键 RAM（DTCM、SRAM、Stack）

---

## 12. 测试策略

### 12.1 单元测试

- 初始化/反初始化测试
- March C- 算法正确性（注入故障验证检测能力）
- Walking Pattern 各模式测试
- 地址线/数据线测试
- CRC 验证正确性
- 区域配置验证（对齐、大小）
- 统计信息收集测试

### 12.2 集成测试

- 启动测试 → 运行时监控完整流程
- RAM 故障注入 → 安全状态响应链路
- 多区域并行检查的性能影响
- ECC 错误检测和报告

---

## 13. 实现说明 / TODO

- March C- 算法按字节操作，对大区域效率较低，可优化为按字（32-bit）操作
- Walking Pattern 当前仅检查区域两端，生产版本应增加全区域扫描模式
- Platform_RamSafety 接口需根据实际硬件平台实现
- CRC 多项式和初始值需与平台 CRC 库对齐
- 安全魔数机制可增强为 CRC 保护

---

## 14. 参考资料

- AUTOSAR_SWS_RAMTest.pdf (R4.4.0)
- ISO 26262-5:2018 Clause 8 (RAM requirements)
- March C- Algorithm Reference (van de Goor, 1991)
- yuleASR RamSafety 模块源码: `src/bsw/services/ramsafety/`
