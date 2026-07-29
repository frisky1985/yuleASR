# RamSafety 模块集成指南

## 概述

RamSafety模块提供通用RAM安全检查功能，支持:
- **March C-** 算法: 检测存储器占位和耦合故障
- **Walking Pattern**: 检测固定和浮动数据线故障
- **Address Line Test**: 检测地址线故障
- **Data Line Test**: 检测数据线故障
- **CRC验证**: 运行时快速验证
- **ECC监控**: 硬件ECC错误检测

## 架构

```
+----------------------------------------+
|              RamSafety (BSW)           |
|  +----------------------------------+  |
|  |  March C- | Walking | CRC | ECC  |  |
|  +----------------------------------+  |
+----------------------------------------+
                   |
+----------------------------------------+
|       Platform_RamSafety (HAL)         |
|  +----------------------------------+  |
|  |  MSCM操作 | CRC计算 | FCCU     |  |
|  +----------------------------------+  |
+----------------------------------------+
                   |
+----------------------------------------+
|           S32K312 硬件                 |
|  +----------------------------------+  |
|  |  DTCM | SRAM | FlexRAM | MSCM    |  |
|  +----------------------------------+  |
+----------------------------------------+
```

## 文件组织

```
src/
├── bsw/
│   └── services/
│       └── ramsafety/
│           ├── include/
│           │   ├── RamSafety.h        # BSW API
│           │   └── RamSafety_Cfg.h    # 配置头文件
│           ├── src/
│           │   ├── RamSafety.c        # BSW实现
│           │   └── RamSafety_Cfg.c    # 配置数据
│           └── test/
│               └── RamSafety_test.c   # 单元测试
└── platform/
    └── s32k312/
        ├── include/
        │   └── Platform_RamSafety.h   # 平台抽象层
        └── src/
            └── Platform_RamSafety.c   # S32K312实现
```

## 配置

### 基本配置 (RamSafety_Cfg.h)

```c
#define RAMSAFETY_CFG_NUM_REGIONS           4U      /* 4个RAM区域 */
#define RAMSAFETY_CFG_RUNTIME_PERIOD_MS     100U    /* 100ms检查周期 */
#define RAMSAFETY_CFG_HARDWARE_ECC          STD_ON  /* 使能硬件ECC */
```

### 区域配置 (RamSafety_Cfg.c)

| 区域 | 地址 | 大小 | 优先级 | 启动检查 | 运行检查 | ECC |
|-----|------|------|--------|----------|----------|-----|
| DTCM | 0x20000000 | 256KB | 最高 | ✓ | ✓ | ✓ |
| SRAM | 0x20400000 | 512KB | 高 | ✓ | ✓ | ✓ |
| FlexRAM | 0x20480000 | 256KB | 中 | ✓ | ✓ | ✓ |
| Stack | 0x2003F000 | 4KB | 低 | ✓ | ✗ | ✓ |

## 使用示例

### 基本用法

```c
#include "RamSafety.h"

void SystemInit(void)
{
    Std_ReturnType result;
    
    /* 初始化RamSafety */
    result = RamSafety_Init(&RamSafety_Config);
    if (result != E_OK)
    {
        /* 初始化失败 */
        EnterSafeState();
        return;
    }
    
    /* 启动时完整检查 */
    result = RamSafety_RunStartupTest(ProgressCallback);
    if (result != E_OK)
    {
        /* RAM检查失败 */
        EnterSafeState();
        return;
    }
    
    /* 系统正常启动 */
}

void MainLoop(void)
{
    while (1)
    {
        /* 应用代码 */
        
        /* 定期调用RamSafety主函数 (10ms周期) */
        RamSafety_MainFunction();
        
        Delay(10);
    }
}
```

### 手动触发检查

```c
void ErrorCallback(RamSafety_TestType testType, uint32 address, 
                   uint8 expected, uint8 actual)
{
    /* 记录错误 */
    LogError("RAM Error: Test=%d, Addr=0x%08X, Exp=0x%02X, Act=0x%02X",
             testType, address, expected, actual);
}

void OnDemandTest(void)
{
    RamSafety_ResultType result;
    
    /* 触发March C- 检查 */
    result = RamSafety_TriggerTest(
        RAMSAFETY_TEST_MARCH_C,
        0U,  /* 区域0 (DTCM) */
        ErrorCallback
    );
    
    if (result != RAMSAFETY_RESULT_PASS)
    {
        /* 处理失败 */
    }
}
```

### CRC验证

```c
void RuntimeVerification(void)
{
    Std_ReturnType result;
    
    /* 定期验证一个区域 */
    result = RamSafety_VerifyRegion(0U);
    if (result != E_OK)
    {
        /* CRC验证失败，执行更详细检查 */
        RamSafety_TriggerTest(RAMSAFETY_TEST_MARCH_C, 0U, NULL_PTR);
    }
}
```

### 统计信息

```c
void PrintStatistics(void)
{
    RamSafety_StatisticsType stats;
    
    if (RamSafety_GetStatistics(&stats) == E_OK)
    {
        printf("Tests Passed: %lu\n", stats.testsPassed);
        printf("Tests Failed: %lu\n", stats.testsFailed);
        printf("Last Error Address: 0x%08X\n", stats.lastErrorAddress);
        printf("Total Bytes Tested: %lu\n", stats.totalBytesTested);
    }
}
```

## 与其他模块的集成

### 与Dem模块集成

```c
void RamSafety_EnterSafeState(uint8 reason)
{
    /* 报告RAM错误到Dem */
    Dem_ReportErrorStatus(
        DemConf_DemEventParameter_RamSafety_Error,
        DEM_EVENT_STATUS_FAILED
    );
    
    /* 通知FCCU */
    Platform_Fccu_NonFaultyFault(PLATFORM_FCCU_FAULT_RAM_ECC);
}
```

### 与Lockstep模块集成

```c
void SafetyErrorHandler(void)
{
    /* Lockstep错误 */
    if (Lockstep_GetStatus() == LOCKSTEP_STATUS_ERROR)
    {
        /* 检查RAM是否安全 */
        RamSafety_TriggerTest(RAMSAFETY_TEST_QUICK, 0xFF, NULL_PTR);
    }
    
    /* RAM错误 */
    if (RamSafety_GetState() == RAMSAFETY_STATE_ERROR)
    {
        /* 进入安全状态 */
        EnterSafeState();
    }
}
```

### 与WdgM模块集成

```c
void WdgM_CheckpointReached(WdgM_SupervisedEntityIdType SEID)
{
    /* 在关键检查点验证RAM */
    if (SEID == WdgM_SE_SafetyCritical)
    {
        RamSafety_VerifyRegion(0U);
    }
}
```

## S32K312特定配置

### MSCM配置

S32K312的MSCM提供:
- **ECC支持**: 单位错误纠正 + 双位错误检测
- **错误中断**: 可配置的中断触发
- **错误计数**: 硬件级错误统计

```c
/* MSCM配置示例 */
Platform_MscmConfigType mscmConfig = {
    .baseAddr = 0x401F0000U,
    .eccEnabled = TRUE,
    .interruptEnabled = TRUE
};

Platform_RamSafety_SetMscmConfig(&mscmConfig);
```

### 启动代码集成

```c
void StartupRoutine(void)
{
    /* 1. 初始化时钟 */
    Clock_Init();
    
    /* 2. 初始化RamSafety */
    RamSafety_Init(&RamSafety_Config);
    
    /* 3. 执行RAM启动检查 */
    if (RamSafety_RunStartupTest(NULL_PTR) != E_OK)
    {
        /* 进入安全状态 */
        EnterSafeState();
    }
    
    /* 4. 初始化其他模块 */
    Dem_Init(&Dem_Config);
    WdgM_Init(&WdgM_Config);
    
    /* 5. 启动应用 */
    Os_Start();
}
```

## 性能考虑

### 检查时间

| 测试类型 | 256KB | 512KB | 1MB |
|---------|-------|-------|-----|
| March C- | ~50ms | ~100ms | ~200ms |
| Walking Pattern | ~5ms | ~10ms | ~20ms |
| Quick | ~0.1ms | ~0.2ms | ~0.4ms |
| CRC | ~2ms | ~4ms | ~8ms |

### 建议配置

- **启动检查**: 对所有关键区域执行March C-
- **运行时检查**: 每100ms检查一个区域，使用CRC验证
- **间隔检查**: 每10秒执行一次Walking Pattern

## 故障排除

### 常见问题

| 问题 | 可能原因 | 解决方法 |
|-----|----------|---------|
| 启动检查失败 | RAM硬件故障 | 检查电源/时钟，更换硬件 |
| ECC错误 | 单位翻转/辐照 | 硬件自动纠正，记录记录 |
| CRC不匹配 | 数据变更 | 更新CRC底线 |
| 超时 | 检查范围太大 | 减小区域大小或增加周期 |

### 调试配置

在调试阶段使用减少的检查范围:

```c
/* 使用调试配置 */
RamSafety_Init(&RamSafety_ConfigDebug);  /* 只检查DTCM */
```

## 参考

- S32K312 Reference Manual (MSCM章节)
- AUTOSAR SWS RamSafety (4.7.0)
- IEC 61508 (RAM测试要求)
- ISO 26262-5 (内存安全要求)
