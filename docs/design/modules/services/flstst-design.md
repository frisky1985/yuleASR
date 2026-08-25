# FlsStst Design Document

> **Module ID**: 0x9A  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_FlashTest  
> **Source Path**: `src/bsw/services/flstst/`  
> **Reference Document**: `docs/modules/flstst.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

FlsStst (Flash State / Flash Test) 是 AUTOSAR BSW 服务层的 Flash 测试模块，负责对 Flash 存储器执行 March C 算法检测和擦除/编程验证。该模块通过分步执行（deferred execution）方式在 MainFunction 中完成长时间的 Flash 测试，避免阻塞系统。FlsStst 检测 Flash 存储器的 stuck-at 故障和 coupling 故障，确保 NV 数据存储的可靠性。

FlsStst 模块支持以下核心能力：
- March C 算法测试（分步执行，每次处理 BYTES_PER_CYCLE 字节）
- 擦除验证（读回值 = 0xFF 检查）
- 编程验证（读回值与期望数据比较）
- 测试中止功能
- 多扇区支持
- 背景模式可配置（0x55/0xAA）

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS FlashTest | 4.4.0 | FlsStst 模块规范（部分实现） |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | NvM / Fee | 调用 FlsStst_RunTest 测试 Flash 扇区 |
| 下层 | Flash HAL | 底层 Flash 读写操作（当前为 stub） |
| 下层 | Det | 开发错误报告 |
| 下层 | EcuM | 初始化阶段调用 FlStSt_Init |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│       NvM / Fee (Flash Users)       │
├─────────────────────────────────────┤
│     FlsStst (Services Layer)        │
├─────────────────────────────────────┤
│     Flash HAL (Hardware Abstraction)│
│     Det (Error Tracing)             │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **March C Engine**: 分步执行 March C 算法，每周期处理有限字节以控制延迟
- **Erase Verifier**: 逐块验证扇区擦除状态（所有字节 = 0xFF）
- **Program Verifier**: 逐字节比较编程数据与期望值
- **Test Run Manager**: 管理测试运行的生命周期（启动、步进、完成、中止）

### 3.3 文件结构

```
src/bsw/services/flstst/
├── include/
│   ├── FlStSt.h        # 公共 API 声明、类型定义
│   └── FlStSt_Cfg.h    # 预编译配置
└── src/
    └── FlStSt.c         # 核心实现
```

---

## 4. 状态机

### 4.1 模块状态

```
          FlStSt_Init()
UNINIT ──────────────► INIT ◄──── Abort() / Test Complete
  ▲                     │
  │    FlStSt_DeInit()  │  RunTest()
  └─────────────────────┼──────► BUSY
                        │         │
                        │         │ Test Complete / Abort
                        │         ▼
                        └────── INIT
```

模块状态：
- **FLSTST_STATE_UNINIT (0x00)**: 未初始化
- **FLSTST_STATE_INIT (0x01)**: 已初始化，空闲
- **FLSTST_STATE_BUSY (0x02)**: 测试进行中

### 4.2 测试阶段状态

```
IDLE → WRITE_BACKGROUND → MARCH_C_UP → MARCH_C_DOWN → VERIFY → COMPLETE
                                                                  │
                                                     ABORTED ◄────┘ (abort)
                                                     ERROR   ◄────┘ (fail)
```

测试阶段：
- **FLSTST_PHASE_IDLE (0x00)**: 空闲
- **FLSTST_PHASE_MARCH_C_UP (0x01)**: March C 上升阶段
- **FLSTST_PHASE_MARCH_C_DOWN (0x02)**: March C 下降阶段
- **FLSTST_PHASE_WRITE_BACKGROUND (0x03)**: 写背景模式
- **FLSTST_PHASE_VERIFY (0x04)**: 验证阶段
- **FLSTST_PHASE_COMPLETE (0x05)**: 完成
- **FLSTST_PHASE_ABORTED (0x06)**: 已中止
- **FLSTST_PHASE_ERROR (0x07)**: 错误

---

## 5. 核心数据结构

### 5.1 扇区描述符

```c
typedef struct {
    uint32  StartAddr;     /* 扇区起始地址 */
    uint32  Size;          /* 扇区大小 (字节) */
    uint16  SectorId;      /* 逻辑扇区 ID */
    uint16  PageSize;      /* Flash 页大小 (字节) */
} FlStSt_SectorType;
```

### 5.2 测试运行类型

```c
typedef struct {
    boolean               Active;          /* 测试激活标志 */
    uint16                SectorId;        /* 目标扇区 ID */
    const FlStSt_SectorType* Sector;       /* 扇区描述符指针 */
    FlStSt_PhaseType      Phase;           /* 当前阶段 */
    FlStSt_AlgorithmType  Algorithm;       /* 算法类型 */
    uint32                CurrentOffset;   /* 当前字偏移 */
    uint8                 MarchStep;       /* March C 步骤 (0-3) */
    uint8                 BackgroundValue; /* 背景模式值 */
    boolean               MarchDirectionUp; /* 方向 (TRUE=上升) */
    FlStSt_ResultType     Result;          /* 测试结果 */
    uint32                FailedAddress;   /* 失败地址 */
    uint8                 ExpectedValue;   /* 期望值 */
    uint8                 ActualValue;     /* 实际值 */
} FlStSt_TestRunType;
```

### 5.3 全局配置类型

```c
typedef struct {
    uint8                 NumSectors;      /* 扇区数量 */
    const FlStSt_SectorType* Sectors;      /* 扇区配置数组 */
    FlStSt_AlgorithmType  Algorithm;       /* 默认算法 */
    boolean               RunOnInit;       /* 初始化时运行测试 */
    boolean               DevErrorDetect;  /* 开发错误检测 */
    boolean               VersionInfoApi;  /* 版本信息 API */
} FlStSt_ConfigType;
```

### 5.4 内部状态类型

```c
typedef struct {
    uint8                 State;           /* 模块状态 */
    const FlStSt_ConfigType* ConfigPtr;    /* 配置指针 */
    FlStSt_TestRunType    CurrentTest;     /* 当前测试运行 */
    FlStSt_ResultType     LastResult;      /* 上次测试结果 */
} FlStSt_InternalStateType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | Service ID | 说明 | SWS 需求 |
|-----|-----------|------|----------|
| `void FlStSt_Init(const FlStSt_ConfigType* ConfigPtr)` | 0x01 | 初始化 | SWS_FlStSt_00001 |
| `void FlStSt_DeInit(void)` | 0x02 | 反初始化 | SWS_FlStSt_00002 |
| `void FlStSt_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 0x03 | 版本信息 | SWS_FlStSt_00003 |
| `Std_ReturnType FlStSt_RunTest(uint16 SectorId)` | 0x10 | 运行 March C 测试 | SWS_FlStSt_00005 |
| `Std_ReturnType FlStSt_VerifyErase(uint16 SectorId, boolean* Result)` | 0x11 | 验证擦除 | SWS_FlStSt_00006 |
| `Std_ReturnType FlStSt_VerifyProgram(uint16 SectorId, const uint8* ExpectedData, uint16 Length, boolean* Result)` | 0x12 | 验证编程 | SWS_FlStSt_00007 |
| `Std_ReturnType FlStSt_GetResult(FlStSt_ResultType* Result)` | 0x13 | 获取测试结果 | SWS_FlStSt_00008 |
| `Std_ReturnType FlStSt_Abort(void)` | 0x14 | 中止测试 | SWS_FlStSt_00009 |
| `void FlStSt_MainFunction(void)` | 0x15 | 主函数（步进执行） | SWS_FlStSt_00004 |

### 6.2 回调函数

FlsStst 不定义回调接口。上层通过 `FlStSt_GetResult()` 轮询测试完成状态。

### 6.3 服务 ID 与错误码

**DET Error Codes:**

| 错误码 | 值 | 说明 |
|--------|-----|------|
| FLSTST_E_PARAM_POINTER | 0x01 | NULL 指针 |
| FLSTST_E_PARAM_CONFIG | 0x02 | 配置参数无效 |
| FLSTST_E_UNINIT | 0x03 | 模块未初始化 |
| FLSTST_E_ALREADY_INITIALIZED | 0x04 | 重复初始化 |
| FLSTST_E_INVALID_SECTOR | 0x05 | 无效扇区 ID |
| FLSTST_E_TEST_FAILED | 0x06 | 测试失败 |
| FLSTST_E_TEST_ABORTED | 0x07 | 测试被中止 |
| FLSTST_E_BUSY | 0x08 | 模块忙碌 |
| FLSTST_E_INIT_FAILED | 0x09 | 初始化失败 |
| FLSTST_E_NOT_SUPPORTED | 0x0A | 不支持的操作 |

**测试结果:**

| 结果 | 值 | 说明 |
|------|-----|------|
| FLSTST_RESULT_NOT_RUN | 0x00 | 未运行 |
| FLSTST_RESULT_PASSED | 0x01 | 通过 |
| FLSTST_RESULT_FAILED | 0x02 | 失败 |
| FLSTST_RESULT_ABORTED | 0x03 | 已中止 |

**算法类型:**

| 算法 | 值 | 说明 |
|------|-----|------|
| FLSTST_ALGO_MARCH_C | 0x00 | March C |
| FLSTST_ALGO_MARCH_C_MINUS | 0x01 | March C- |
| FLSTST_ALGO_CHECKERBOARD | 0x02 | 棋盘格 |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查是否已初始化（重复初始化报 DET 错误）
2. 检查 ConfigPtr 非 NULL
3. 存储配置指针
4. 设置状态为 FLSTST_STATE_INIT
5. 清除 CurrentTest 和 LastResult
6. 若 RunOnInit=TRUE，自动启动第一个扇区的测试

### 7.2 March C 测试启动流程

1. 检查模块已初始化
2. 检查当前无活跃测试（CurrentTest.Active == FALSE）
3. 根据 SectorId 查找扇区配置
4. 初始化测试运行参数：
   - Active = TRUE
   - Phase = WRITE_BACKGROUND
   - MarchStep = 0 (W0)
   - BackgroundValue = 0x55
   - MarchDirectionUp = TRUE
5. 设置模块状态为 BUSY

### 7.3 MainFunction 步进执行流程

1. 检查状态为 BUSY
2. 检查 CurrentTest.Active
3. 若非活跃，保存结果到 LastResult，状态恢复为 INIT
4. 调用 FlStSt_LocalRunMarchCStep() 执行一步
5. 每步处理最多 FLSTST_BYTES_PER_CYCLE (4) 字节
6. March C 步骤序列：
   - Step 0 (W0): 写背景模式 (0x55)
   - Step 1 (R0_W1): 读 0x55 写 0xAA
   - Step 2 (R1_W0): 读 0xAA 写 0x55
   - Step 3 (R0): 读 0x55
7. 每步完成后进入下一步
8. 所有步骤完成后设置 Phase = COMPLETE, Result = PASSED

### 7.4 擦除验证流程

1. 检查模块已初始化、Result 指针非 NULL
2. 查找扇区配置
3. 以 FLSTST_VERIFY_CHUNK_SIZE (256) 字节为块遍历扇区
4. 对每个字节调用 FlStSt_LocalReadByte() 检查是否为 0xFF
5. 返回验证结果

### 7.5 编程验证流程

1. 检查模块已初始化、ExpectedData 和 Result 指针非 NULL
2. 查找扇区配置
3. 检查 Length 不超过扇区大小
4. 逐字节比较读回值与期望数据
5. 返回验证结果

---

## 8. 配置设计

### 8.1 预编译配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `FLSTST_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `FLSTST_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `FLSTST_MAIN_FUNCTION_PERIOD_MS` | 10U | 主函数周期 |
| `FLSTST_MAX_SECTORS` | 8U | 最大扇区数 |
| `FLSTST_MARCH_BACKGROUND_PATTERN` | 0x55U | 背景模式 |
| `FLSTST_MARCH_BACKGROUND_INVERT` | 0xAAU | 反转背景模式 |
| `FLSTST_MARCH_CHECKERBOARD_A` | 0xAAU | 棋盘格模式 A |
| `FLSTST_MARCH_CHECKERBOARD_B` | 0x55U | 棋盘格模式 B |
| `FLSTST_BYTES_PER_CYCLE` | 4U | 每周期处理字节数 |
| `FLSTST_ERASE_VALUE` | 0xFFU | 擦除值 |
| `FLSTST_VERIFY_CHUNK_SIZE` | 256U | 验证块大小 |

### 8.2 链接时配置

通过配置工具定义 Flash 扇区表（起始地址、大小、页大小）。

### 8.3 构建后配置

不支持构建后配置。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 场景 | API | 错误码 |
|------|-----|--------|
| 重复初始化 | FlStSt_Init | FLSTST_E_ALREADY_INITIALIZED |
| ConfigPtr 为 NULL | FlStSt_Init | FLSTST_E_PARAM_POINTER |
| 模块未初始化 | RunTest/Abort | FLSTST_E_UNINIT |
| 测试进行中再次 RunTest | FlStSt_RunTest | 返回 E_NOT_OK |
| 无效扇区 ID | RunTest/Verify | 返回 E_NOT_OK |
| Result/Data 指针为 NULL | Verify/GetResult | FLSTST_E_PARAM_POINTER |

### 9.2 DEM 错误

FlsStst 不直接报告 DEM 事件。Flash 测试失败可通过 Fee/NvM 上报。

### 9.3 安全机制

- 分步执行确保 MainFunction 延迟可控（每次最多 4 字节）
- 测试中止功能确保长时间测试可被安全中断
- DeInit 时自动中止活跃测试
- 失败时记录 FailedAddress、ExpectedValue、ActualValue 用于诊断
- 背景模式可配置（0x55/0xAA），支持交替模式检测

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 段 | 变量 | 说明 |
|----|------|------|
| 静态 | FlStSt_InternalState | 模块内部状态 |
| CODE | 所有函数 | 代码段 |

### 10.2 资源估算

- **RAM**: FlStSt_InternalStateType ≈ 4 + 4 + sizeof(FlStSt_TestRunType) + 1 ≈ 40 字节
- **ROM**: ~3 KB（代码段 + March C 逻辑）
- **性能**: 每个 MainFunction 调用处理 4 字节；一个 4KB 扇区的完整 March C 测试需要 4×4×(4096/4) = 16384 次 MainFunction 调用，以 10ms 周期约需 164 秒

---

## 11. 集成指南

- Fee 模块在 Flash 操作前/后调用 `FlStSt_RunTest()` 验证扇区健康
- `FlStSt_VerifyErase()` 在擦除操作后验证所有字节为 0xFF
- `FlStSt_VerifyProgram()` 在编程操作后验证数据正确性
- SCHM 以 10ms 周期调用 `FlStSt_MainFunction()` 步进执行测试
- 测试进行中不接受新的测试请求
- 当前 Flash HAL 为 stub 实现（FlStSt_LocalReadByte 返回 0xFF，FlStSt_LocalWriteByte 为 no-op），需对接实际 Flash 驱动

---

## 12. 测试策略

### 12.1 单元测试

- 初始化/反初始化测试
- March C 算法步进执行测试（验证各步骤正确转换）
- 测试中止和恢复测试
- 擦除验证测试（全 0xFF、部分非 0xFF）
- 编程验证测试（数据匹配/不匹配）
- 无效扇区 ID 测试
- 忙碌状态下的请求拒绝测试

### 12.2 集成测试

- Fee → FlsStst 完整 Flash 测试链路
- MainFunction 长时间运行的稳定性
- 多扇区顺序测试

---

## 13. 实现说明 / TODO

- Flash HAL 接口（FlStSt_LocalReadByte/FlStSt_LocalWriteByte）当前为 stub，需替换为实际 Flash 驱动调用
- March C 算法的方向切换逻辑（MarchDirectionUp）在步骤转换时存在潜在问题，需验证
- FLSTST_RUN_ON_INIT 宏在代码中使用但未在 Cfg.h 中定义，需补充
- Checkerboard 算法（FLSTST_ALGO_CHECKERBOARD）已定义但未实现
- 测试耗时较长（4KB 扇区约 164 秒），生产版本应增加 FLSTST_BYTES_PER_CYCLE 或优化算法

---

## 14. 参考资料

- AUTOSAR_SWS_FlashTest.pdf (R4.4.0)
- March C Algorithm Reference
- yuleASR FlsStst 模块源码: `src/bsw/services/flstst/`
