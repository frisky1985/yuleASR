# RamTst Design Document

> **Module ID**: 0x27  
> **AUTOSAR Layer**: MCAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_RamTst  
> **Source Path**: `src/bsw/mcal/RamTst/`  
> **Reference Document**: `docs/modules/RamTst.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

RamTst（RAM Test）模块用于对片上 SRAM 进行非破坏性测试，检测 stuck-at、transition、coupling 等故障。本实现支持：

- March-C / March-C- / March 13N
- GALPAT
- Walkpath
- Checkerboard

测试通过 `RamTst_MainFunction` 分步执行，避免长时间阻塞 CPU，适用于由 OS/SchM 周期调度的场景。

上层可由 EcuM、Safety Monitor 或诊断服务调用；下层直接访问目标 RAM 地址。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 | |
|------|------|------|
| AUTOSAR SWS RamTst | 4.4.0 | RAM Test 软件规范 | |
| AUTOSAR Classic Platform | 4.x | 经典平台 | |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 | |
|----------|------|------|
| 上层 | EcuM、Safety Monitor、Diagnostic | 触发测试与读取结果 | |
| 下层 | 目标 RAM 区域 | 通过绝对地址访问 | |
| 同层 | Mcu | 提供 RAM 映射与保护配置 | |
| 公共 | Det | 开发错误检测（可选） | |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│      EcuM / Safety / Diagnostic     │
├─────────────────────────────────────┤
│          RamTst (MCAL)              │
├─────────────────────────────────────┤
│          On-Chip SRAM               │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **算法执行引擎**：March-C、Checkerboard、GALPAT、Walkpath 分步执行函数。
- **错误记录器**：记录失败地址、期望值、实际值、错误掩码、算法步号。
- **状态机管理**：UNINIT / IDLE / RUNNING / COMPLETED / ERROR。
- **超时监控**：通过 `TickCount` 在 `MainFunction` 中累加，超过 `TimeoutMs` 则返回 TIMEOUT。

### 3.3 文件结构

```
src/bsw/mcal/RamTst/
├── include/
│   ├── RamTst.h
│   └── RamTst_Cfg.h
└── src/
    ├── RamTst.c
    └── RamTst_Lcfg.c
```

---

## 4. 状态机

```
[UNINIT] -- RamTst_Init() --> [IDLE]
[IDLE]   -- RamTst_Run()  --> [RUNNING]
[RUNNING]-- 完成/失败/超时 --> [COMPLETED]
[RUNNING]-- RamTst_Stop() --> [IDLE] (Result=ABORTED)
[RUNNING]-- 未知算法/异常  --> [ERROR]
[COMPLETED] -- RamTst_Run() --> [RUNNING]
```

---

## 5. 核心数据结构

| 类型 | 说明 | |
|------|------|
| `RamTst_AlgType` | 测试算法枚举 | |
| `RamTst_TestResultType` | `NOT_TESTED` / `OK` / `FAILED` / `ABORTED` / `TIMEOUT` | |
| `RamTst_StatusType` | `UNINIT` / `IDLE` / `RUNNING` / `COMPLETED` / `ERROR` | |
| `RamTst_ErrorRecordType` | 失败地址、期望/实际值、位掩码、算法步、错误计数 | |
| `RamTst_ConfigType` | 测试区域、算法、调用周期、超时、StopOnError、PatternSeed | |
| `RamTst_InternalType` | 内部运行态（当前地址、March 步、位索引、错误记录等） | |

```c
typedef struct {
    uint32 StartAddress;
    uint32 Size;
    RamTst_AlgType Algorithm;
    uint32 CallCycle;
    uint32 TimeoutMs;
    boolean StopOnError;
    uint32 PatternSeed;
} RamTst_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|----------------|
| `RamTst_Init` | `void RamTst_Init(const RamTst_ConfigType* ConfigPtr)` | 初始化模块 | 必须先调用 | |
| `RamTst_DeInit` | `void RamTst_DeInit(void)` | 反初始化，回到 UNINIT | | |
| `RamTst_Run` | `Std_ReturnType RamTst_Run(void)` | 启动一次 RAM 测试 | 仅在 IDLE 状态 | |
| `RamTst_Stop` | `void RamTst_Stop(void)` | 中止当前测试 | | |
| `RamTst_GetTestResult` | `RamTst_TestResultType RamTst_GetTestResult(void)` | 获取最近测试结果 | | |
| `RamTst_GetErrorRecord` | `Std_ReturnType RamTst_GetErrorRecord(RamTst_ErrorRecordType* ErrorRecord)` | 获取详细错误记录 | | |
| `RamTst_GetTestStatus` | `RamTst_StatusType RamTst_GetTestStatus(void)` | 获取当前状态 | | |
| `RamTst_MainFunction` | `void RamTst_MainFunction(void)` | 周期执行测试步 | 由 OS/SchM 调用 | |
| `RamTst_GetVersionInfo` | `void RamTst_GetVersionInfo(Std_VersionInfoType* versioninfo)` | 获取版本信息 | 受 `RAMTST_VERSION_INFO_API` 控制 | |
| `RamTst_SetMode` | `Std_ReturnType RamTst_SetMode(RamTst_ModeType Mode)` | 设置模式 | 受 `RAMTST_SET_MODE_API` 控制 | |
| `RamTst_GetMode` | `RamTst_ModeType RamTst_GetMode(void)` | 获取模式 | 受 `RAMTST_GET_MODE_API` 控制 | |

### 6.2 回调函数

本模块无回调函数。

### 6.3 Service ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x01 | `RamTst_Init` | `RAMTST_E_PARAM_POINTER` | |
| 0x02 | `RamTst_DeInit` | 无 | |
| 0x03 | `RamTst_Run` | `RAMTST_E_UNINIT`、`RAMTST_E_BUSY` | |
| 0x04 | `RamTst_Stop` | 无 | |
| 0x05 | `RamTst_GetTestResult` | 无 | |
| 0x06 | `RamTst_GetTestStatus` | 无 | |
| 0x07 | `RamTst_MainFunction` | 无 | |
| 0x08 | `RamTst_GetVersionInfo` | `RAMTST_E_PARAM_POINTER` | |
| 0x09 | `RamTst_SetMode` | `RAMTST_E_UNINIT` | |

| 错误码 | 名称 | 说明 | |
|--------|------|------|
| 0x00 | `RAMTST_E_NO_ERROR` | 无错误 | |
| 0x01 | `RAMTST_E_PARAM_POINTER` | 空指针 | |
| 0x02 | `RAMTST_E_UNINIT` | 模块未初始化 | |
| 0x03 | `RAMTST_E_BUSY` | 测试正在进行 | |
| 0x04 | `RAMTST_E_PARAM_CONFIG` | 无效配置 | |
| 0x05 | `RAMTST_E_PARAM_ADDRESS` | 无效地址范围 | |
| 0x06 | `RAMTST_E_PARAM_ALGORITHM` | 无效算法 | |
| 0x07 | `RAMTST_E_TIMEOUT` | 测试超时 | |

---

## 7. 处理流程

### 7.1 初始化流程

1. 检查 `ConfigPtr` 非空。
2. 调用 `RamTst_ResetInternalState` 清零内部状态与错误记录。
3. 拷贝配置到 `RamTst_State.Config`。
4. 状态置为 `RAMTST_STATUS_IDLE`，结果置为 `RAMTST_RESULT_NOT_TESTED`。

### 7.2 测试启动流程

1. 检查状态为 IDLE，且 `Size != 0`。
2. 对齐起始/结束地址到 4 字节。
3. 根据算法初始化 `WritePattern`、`ReadPattern`、`CurrentBit`。
4. 状态置为 `RAMTST_STATUS_RUNNING`，返回 `E_OK`。

### 7.3 MainFunction 执行流程

1. 若状态非 RUNNING 直接返回。
2. `TickCount++`。
3. 根据 `Algorithm` 分派：
   - March-C / March-C- / March 13N -> `RamTst_ExecuteMarchC`
   - Checkerboard -> `RamTst_ExecuteCheckerboard`
   - GALPAT -> `RamTst_ExecuteGALPAT`
   - Walkpath -> `RamTst_ExecuteWalkpath`
4. 每一步检查超时，超时时结果置 TIMEOUT，状态回 IDLE。

### 7.4 March-C 算法流程

共 6 个阶段（非阻塞分步）：

1. M0：写背景值（全 0）升序。
2. M1：读 0 写 1 升序。
3. M2：读 1 写 0 升序。
4. M3：读 0 写 1 降序。
5. M4：读 1 写 0 降序。
6. M5：读 0 升序。

每次读比较失败即记录错误。

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 | |
|----|--------|------|
| `RAMTST_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 | |
| `RAMTST_VERSION_INFO_API` | STD_ON | 版本信息 API | |
| `RAMTST_SET_MODE_API` | STD_OFF | SetMode API | |
| `RAMTST_GET_MODE_API` | STD_OFF | GetMode API | |
| `RAMTST_TIMEOUT_MS` | 5000U | 默认超时 | |
| `RAMTST_START_ADDRESS` | 536870912U | 测试起始地址 | |
| `RAMTST_SIZE` | 131072U | 测试区域大小（字节） | |
| `RAMTST_ALGORITHM` | `RAMTST_ALGORITHM_MARCH_C` | 默认算法 | |
| `RAMTST_CALL_CYCLE` | 10U | MainFunction 调用周期（ms） | |
| `RAMTST_STOP_ON_ERROR` | STD_OFF | 首次错误即停止 | |
| `RAMTST_PATTERN_SEED` | 2779096485U | 模式种子 | |

### 8.2 链接时配置

`RamTst_Lcfg.c` 提供 `const RamTst_ConfigType RamTst_Config`，当前已填充部分字段（StartAddress、Size、Algorithm、CallCycle），建议完整填充所有字段。

---

## 9. 错误处理与安全

### 9.1 DET 错误

在 `RAMTST_DEV_ERROR_DETECT == STD_ON` 时：

- `RamTst_Init` 空指针 -> `RAMTST_E_PARAM_POINTER`
- `RamTst_Run` 未初始化 -> `RAMTST_E_UNINIT`
- `RamTst_Run` 重复启动 -> `RAMTST_E_BUSY`
- `RamTst_GetErrorRecord` / `GetVersionInfo` 空指针 -> `RAMTST_E_PARAM_POINTER`

### 9.2 DEM 错误

本模块未使用 DEM。

### 9.3 安全机制

- 测试区域地址按 4 字节对齐，避免非对齐访问。
- 支持 `StopOnError` 快速失败。
- 超时机制防止测试无限挂起。
- 测试运行期间会改写目标 RAM，调用方需确保测试区域未被关键任务/栈使用。

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 | |
|------|------|
| `RAMTST_START_SEC_VAR_CLEARED_UNSPECIFIED` | `RamTst_State` 内部状态 | |
| `RAMTST_START_SEC_CONFIG_DATA_UNSPECIFIED` | `RamTst_Config` | |
| `RAMTST_START_SEC_CODE` | 代码段 | |

### 10.2 资源估算

| 资源 | 估算值 | 说明 | |
|------|--------|------|
| RAM | ~200 B | `RamTst_InternalType` + 16 条错误记录 | |
| ROM | 配置表 + 算法代码 | 与代码大小相关 | |
| 堆栈 | 低 | 无递归，MainFunction 开销小 | |
| 执行时间 | 与区域大小成正比 | 分步执行，单次 MainFunction 处理 4 字节 | |

---

## 11. 集成指南

- 与 EcuM 集成：在启动自检阶段调用 `RamTst_Run()`，周期调用 `RamTst_MainFunction()` 直至完成。
- 与 OS 集成：将 `RamTst_MainFunction` 配置为 10 ms 周期任务。
- 与 Mcu 集成：测试区域地址与大小需与 Mcu RAM 映射一致。
- 安全注意：测试会覆盖目标 RAM，避免测试程序自身栈、全局变量或 DMA 缓冲区所在的区域。
- 初始化顺序：Mcu -> 其他不依赖 RamTst 的模块 -> RamTst。

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 覆盖内容 | |
|--------|----------|
| 初始化 | 空配置、状态转移 | |
| 启动 | IDLE->RUNNING、重复启动返回 BUSY | |
| 各算法 | March-C、Checkerboard、GALPAT、Walkpath 通过 | |
| 错误注入 | 注入 bit 翻转，验证错误记录 | |
| 停止/超时 | 中止后状态为 IDLE，超时返回 TIMEOUT | |

### 12.2 集成测试

| 场景 | 说明 | |
|------|------|
| 启动自检 | EcuM 调用 RamTst 完成上电自检 | |
| 后台测试 | OS 周期调度 MainFunction 完成全 RAM 测试 | |
| 故障处理 | 检测到错误后进入安全状态 | |

---

## 13. 实现说明 / TODO

- **Module ID 差异**：头文件中 `RAMTST_MODULE_ID` 定义为 `0x64`（十进制 100），与 AUTOSAR 标准 RamTst Module ID `0x27` 不一致。设计文档按项目约定使用 `0x27`，实际代码需统一。
- **March 13N 复用 March-C**：当前 `RAMTST_ALGORITHM_MARCH_13N` 直接复用 `RamTst_ExecuteMarchC`，未实现完整 13N 步骤，若需认证需补充。
- **模式 API 占位**：`RamTst_SetMode` / `GetMode` 实际未实现功能。
- **时间戳依赖**：`RamTst_GetTickMs` 仅返回内部 `TickCount`，未与真实系统 tick 关联。
- **Lcfg 不完整**：`RamTst_Lcfg.c` 未填充 `TimeoutMs`、`StopOnError`、`PatternSeed` 等字段。
- **地址安全**：缺少对测试区域是否覆盖自身数据/栈的检查。

---

## 14. 参考资料

1. AUTOSAR_SWS_RamTst.pdf
2. `docs/modules/RamTst.md`
3. `src/bsw/mcal/RamTst/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_RamTst | — | RAMTST 模块级需求归属 |
| SWS_RamTst_00001 | `RamTst` | 测试 test_RamTst_Init_NullConfig 覆盖: RamTst_Init_NullConfig 场景 |
| SWS_RamTst_00002 | `RamTst_DeInit` | 测试 test_RamTst_DeInit_AfterInit 覆盖: RamTst_DeInit_AfterInit 场景 |
| SWS_RamTst_00003 | `RamTst_Run_Start` | 测试 test_RamTst_Run_Start 覆盖: RamTst_Run_Start 场景 |
| SWS_RamTst_00004 | `RamTst_Stop_Running` | 测试 test_RamTst_Stop_Running 覆盖: RamTst_Stop_Running 场景 |
| SWS_RamTst_00005 | `RamTst_GetTestResult_BeforeRun` | 测试 test_RamTst_GetTestResult_BeforeRun 覆盖: RamTst_GetTestResult_BeforeRun 场景 |
| SWS_RamTst_00006 | `RamTst_Abort` | 测试 test_RamTst_Abort_AfterInit_ShouldReturnResult 覆盖: RamTst_Abort_AfterInit_ShouldReturnResult 场景 |
| SWS_RamTst_00007 | `RamTst_MainFunction` | 测试 test_RamTst_MainFunction_AfterInit_ShouldNotCrash 覆盖: RamTst_MainFunction_AfterInit_ShouldNotCrash 场景 |
| SWS_RamTst_00008 | `RamTst_MainFunction_CompletesTest` | 测试 test_RamTst_MainFunction_CompletesTest 覆盖: RamTst_MainFunction_CompletesTest 场景 |
| SWS_RamTst_00201 | `setup` | 测试 test_setup 覆盖: setup 场景 |
| SWS_RamTst_00202 | `init_with_valid_config` | 测试 test_init_with_valid_config 覆盖: init_with_valid_config 场景 |
| SWS_RamTst_00203 | `init_with_null_config` | 测试 test_init_with_null_config 覆盖: init_with_null_config 场景 |
| SWS_RamTst_00204 | `init_multiple_times` | 测试 test_init_multiple_times 覆盖: init_multiple_times 场景 |
| SWS_RamTst_00205 | `deinit_after_init` | 测试 test_deinit_after_init 覆盖: deinit_after_init 场景 |
| SWS_RamTst_00206 | `deinit_without_init` | 测试 test_deinit_without_init 覆盖: deinit_without_init 场景 |
| SWS_RamTst_00207 | `run_after_init` | 测试 test_run_after_init 覆盖: run_after_init 场景 |
| SWS_RamTst_00208 | `run_without_init` | 测试 test_run_without_init 覆盖: run_without_init 场景 |
| SWS_RamTst_00209 | `run_while_already_running` | 测试 test_run_while_already_running 覆盖: run_while_already_running 场景 |
| SWS_RamTst_00210 | `stop_while_running` | 测试 test_stop_while_running 覆盖: stop_while_running 场景 |
| SWS_RamTst_00211 | `stop_while_idle` | 测试 test_stop_while_idle 覆盖: stop_while_idle 场景 |
| SWS_RamTst_00212 | `stop_while_uninit` | 测试 test_stop_while_uninit 覆盖: stop_while_uninit 场景 |
| SWS_RamTst_00213 | `get_status_uninit` | 测试 test_get_status_uninit 覆盖: get_status_uninit 场景 |
| SWS_RamTst_00214 | `get_status_idle` | 测试 test_get_status_idle 覆盖: get_status_idle 场景 |
| SWS_RamTst_00215 | `get_status_running` | 测试 test_get_status_running 覆盖: get_status_running 场景 |
| SWS_RamTst_00216 | `get_result_not_tested` | 测试 test_get_result_not_tested 覆盖: get_result_not_tested 场景 |
| SWS_RamTst_00217 | `get_result_after_run` | 测试 test_get_result_after_run 覆盖: get_result_after_run 场景 |
| SWS_RamTst_00218 | `mainfunction_when_not_running` | 测试 test_mainfunction_when_not_running 覆盖: mainfunction_when_not_running 场景 |
| SWS_RamTst_00219 | `mainfunction_completes_test` | 测试 test_mainfunction_completes_test 覆盖: mainfunction_completes_test 场景 |
| SWS_RamTst_00220 | `mainfunction_while_uninit` | 测试 test_mainfunction_while_uninit 覆盖: mainfunction_while_uninit 场景 |
| SWS_RamTst_00221 | `full_test_cycle` | 测试 test_full_test_cycle 覆盖: full_test_cycle 场景 |
| SWS_RamTst_00222 | `multiple_test_cycles` | 测试 test_multiple_test_cycles 覆盖: multiple_test_cycles 场景 |
| SWS_RamTst_00223 | `stop_during_test` | 测试 test_stop_during_test 覆盖: stop_during_test 场景 |
| SWS_RamTst_00224 | `algorithm_march` | 测试 test_algorithm_march 覆盖: algorithm_march 场景 |
| SWS_RamTst_00225 | `algorithm_galpat` | 测试 test_algorithm_galpat 覆盖: algorithm_galpat 场景 |
| SWS_RamTst_00226 | `algorithm_walkpath` | 测试 test_algorithm_walkpath 覆盖: algorithm_walkpath 场景 |
