# S32K312 Lockstep 集成指南

本文档描述如何将S32K312的Lockstep（锁步）功能集成到yuleASR项目中。

---

## 概述

### 什么是Lockstep？

Lockstep（锁步）是一种硬件级安全机制，用于检测CPU内部的随机故障。

S32K312的Cortex-M7内核支持两种运行模式：
- **Lockstep模式**: 两个CPU核心执行相同的指令，硬件比较结果
- **Split模式**: 独立运行 (用于调试)

### ASIL-D安全等级

Lockstep是达到ASIL-D安全等级的关键技术，用于：
- 检测CPU内部结构的随机故障
- 检测内部数据路径中的组合逻辑故障
- 提供诊断覆盖率(DC)

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                    yuleASR BSW 层                    │
│  ┌─────────────────────────────────────────────┐          │
│  │              Lockstep Module                    │          │
│  │  - 状态管理                                      │          │
│  │  - 错误检测                                      │          │
│  │  - 统计收集                                      │          │
│  │  - 事件通知                                      │          │
│  └─────────────────────────────────────────────┘          │
│              │                                        │          │
│              │ 调用 Platform_Lockstep_xxx API         │          │
│              ↓                                        │          │
└─────────────────────────────────────────────────────────────────┘
│                                        │
│              S32K312 Platform Layer     │
│  ┌─────────────────────────────────────────────┐       │
│  │           Platform_Lockstep.c                  │       │
│  │  - MSCM寄存器操作                                │       │
│  │  - 硬件状态检查                                    │       │
│  │  - FCCU集成                                      │       │
│  └─────────────────────────────────────────────┘       │
│              │                              │
│              │ 访问硬件寄存器                    │
│              ↓                              │
└─────────────────────────────────────────────────────────────────┘
│                    │
└─────────────────────────────────────────────────────────────────┘
              S32K312 硬件
     ┌─────────────────────────────────────────────┐
     │        Cortex-M7 双核 (锁步)           │
     │  ┌─────────┐    ┌─────────┐         │
     │  │ CPU0   │    │ CPU0'  │ 比较器      │
     │  │ (主核) │ ────│ (镜像)│─────────┐     │
     │  └─────────┘    └─────────┘        │     │
     │       │              │                │     │
     └─────────────────────────────────────────────────────────────────┘
                                          │
                                          ↓
                                   ┌───────────────────┐
                                   │     FCCU        │
                                   │  (故障管理)    │
                                   └───────────────────┘
```

---

## 文件结构

```
src/
├── bsw/services/lockstep/
│   ├── include/
│   │   ├── Lockstep.h          # 主模块头文件
│   │   └── Lockstep_Cfg.h      # 配置头文件
│   └── src/
│       ├── Lockstep.c          # 主模块实现
│       └── Lockstep_Cfg.c      # 配置实现
└── platform/s32k312/
    ├── include/
    │   └── Platform_Lockstep.h   # 平台抽象层头文件
    └── src/
        └── Platform_Lockstep.c   # S32K312硬件实现
```

---

## 快速开始

### 1. 初始化Lockstep

```c
#include "Lockstep.h"

void SystemInit(void)
{
    Std_ReturnType result;
    
    /* 初始化Lockstep (使能锁步模式) */
    result = Lockstep_Init(&Lockstep_Config);
    
    if (E_OK != result)
    {
        /* 初始化失败 - 进入安全状态 */
        Lockstep_EnterSafeState(LOCKSTEP_E_INIT_FAILED);
    }
    
    /* 注册事件回调 */
    Lockstep_RegisterCallback(Lockstep_EventCallback, NULL_PTR);
}
```

### 2. 主循环监控

```c
void MainFunction(void)
{
    /* 定期调用 (10ms周期) */
    Lockstep_MainFunction();
}
```

### 3. 手动检查

```c
void CriticalSectionCheck(void)
{
    /* 在关键代码段检查Lockstep状态 */
    if (E_OK != Lockstep_TriggerCheck())
    {
        /* 检测到错误 */
        HandleLockstepError();
    }
}
```

---

## 配置选项

### 默认配置 (ASIL-D)

```c
const Lockstep_ConfigType Lockstep_Config =
{
    .mode = LOCKSTEP_MODE_ENABLED,          /* 使能锁步模式 */
    .enableBist = TRUE,                     /* 启动BIST */
    .enableEout = TRUE,                     /* 错误输出 */
    .monitorPeriodMs = 10,                  /* 10ms监控 */
    .errorThreshold = 3,                    /* 3次错误触发安全状态 */
    .numMonitorRegions = 4,                 /* 4个监控区域 */
    .monitorRegions = Lockstep_MonitorRegions
};
```

### 调试配置

```c
const Lockstep_ConfigType Lockstep_ConfigDebug =
{
    .mode = LOCKSTEP_MODE_DEBUG,            /* 分离模式 */
    .enableBist = FALSE,                    /* 关闭BIST */
    .enableEout = FALSE,                    /* 关闭EOUT */
    .monitorPeriodMs = 100,                 /* 较长周期 */
    .errorThreshold = 10,
    .numMonitorRegions = 0
};
```

---

## 与其他模块集成

### 与Dem集成

```c
#include "Dem.h"

void Lockstep_EventCallback(Lockstep_EventType event, uint32 errorCode, const void* context)
{
    switch (event)
    {
        case LOCKSTEP_EVENT_MISMATCH:
            Dem_ReportErrorStatus(
                LOCKSTEP_MISMATCH_EVENT_ID, 
                DEM_EVENT_STATUS_FAILED
            );
            break;
            
        case LOCKSTEP_EVENT_BIST_FAILURE:
            Dem_ReportErrorStatus(
                LOCKSTEP_BIST_FAIL_EVENT_ID,
                DEM_EVENT_STATUS_FAILED
            );
            break;
    }
}
```

### 与FCCU集成

FCCU (Fault Collection and Control Unit) 是S32K312的故障管理模块。

Lockstep检测到错误时会：
1. 通知FCCU
2. FCCU根据配置决定响应策略
3. 可触发复位或进入安全状态

### 与EcuM集成

```c
#include "EcuM.h"

void Lockstep_EnterSafeState(uint32 reason)
{
    /* 通知EcuM进入安全状态 */
    EcuM_SelectShutdownTarget(ECUM_STATE_RESET, 0);
    
    /* 记录复位原因 */
    (void)reason;
}
```

---

## API参考

### 初始化函数

| 函数 | 说明 |
|------|------|
| `Lockstep_Init()` | 初始化Lockstep模块 |
| `Lockstep_DeInit()` | 去初始化 |
| `Lockstep_RegisterCallback()` | 注册事件回调 |

### 状态函数

| 函数 | 说明 |
|------|------|
| `Lockstep_GetState()` | 获取当前状态 |
| `Lockstep_GetMode()` | 获取当前模式 |
| `Lockstep_SetMode()` | 设置模式 |
| `Lockstep_TriggerCheck()` | 手动检查 |

### 维护函数

| 函数 | 说明 |
|------|------|
| `Lockstep_RunBist()` | 运行内建自测试 |
| `Lockstep_MainFunction()` | 主循环处理 |
| `Lockstep_EnterSafeState()` | 进入安全状态 |

---

## 安全考虑

### ASIL-D要求

1. **FMEDA分析**
   - Lockstep覆盖CPU内部的单点故障
   - 需要配合其他安全机制 (看门狗、ECC等)

2. **诊断覆盖率**
   - Lockstep检测率优于99%
   - 剩余故障由FCCU处理

3. **FTA分析**
   - Lockstep失效 -> FCCU检测 -> 系统复位

### 建议的安全机制组合

```
┌─────────────────────────────────────────────────────────────────┐
│                    ASIL-D 安全机制                     │
├─────────────────────────────────────────────────────────────────┤
│  ✓ Lockstep           - CPU双核锁步                    │
│  ✓ RAM ECC            - 内存错误检测和修正            │
│  ✓ Flash ECC          - 代码完整性检查              │
│  ✓ WdgM               - 软件看门狗                  │
│  ✓ FCCU               - 硬件故障管理                │
│  ✓ Safe Monitor        - 软件安全监控                │
└─────────────────────────────────────────────────────────────────┘
```

---

## 调试指南

### 检查Lockstep状态

```c
void Debug_LockstepStatus(void)
{
    Lockstep_StateType state = Lockstep_GetState();
    Lockstep_ModeType mode = Lockstep_GetMode();
    Lockstep_StatisticsType stats;
    
    Lockstep_GetStatistics(&stats);
    
    printf("Lockstep State: %d\n", state);
    printf("Lockstep Mode: %d\n", mode);
    printf("Mismatch Count: %lu\n", stats.mismatchCount);
    printf("BIST Pass: %lu\n", stats.bistPassCount);
    printf("BIST Fail: %lu\n", stats.bistFailCount);
}
```

### 注入故障测试

```c
void Test_LockstepFault(void)
{
    /* 通过FCCU注入测试故障 */
    FCCU->CTRLK = FCCU_CTRLK_KEY;
    FCCU->NCF_S0 = 0x00000001U;  /* 注入NCF0 */
}
```

---

## 常见问题

### Q: 为什么Lockstep_Init返回失败?

**A**: 可能原因:
1. 之前的复位是由于Lockstep错误
2. MSCM寄存器访问权限问题
3. 缺少安全启动过程

### Q: 如何切换到调试模式?

**A**: 在调试时可以使用Split模式:
```c
Lockstep_SetMode(LOCKSTEP_MODE_DEBUG);
```

### Q: Lockstep不匹配会发生什么?

**A**: 系统会:
1. 记录错误
2. 通知FCCU
3. 触发复位或进入安全状态

---

## 参考资料

- [S32K3xx参考手册 - MSCM章节]
- [S32K3xx参考手册 - FCCU章节]
- [ARM Cortex-M7技术参考手册]
- [ISO 26262 功能安全标准]

---

**现在您可以在yuleASR中使用S32K312的Lockstep功能了！** 🚀
