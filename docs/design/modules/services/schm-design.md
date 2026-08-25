# SchM Design Document

> **Module ID**: 0x3A (58)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS BSW Scheduler Manager  
> **Source Path**: `src/bsw/services/schm/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

SchM (BSW Scheduler Manager) 管理所有 BSW 模块的周期调度，确保 MainFunction 按配置的时序和优先级被调用。SchM 提供统一的调度入口，协调 BSW 模块的执行顺序、监控超时和检测调度冲突。SchM 与 Os 任务配合，在固定周期任务中调用各模块的 MainFunction。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS BSW Scheduler Manager | 4.4.0 | SchM 规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | EcuM | 启动调度 |
| 下层 | 全部 BSW 模块 | 调度各模块 MainFunction |
| 同层 | Os | 任务/报警调度 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│           EcuM                      │
├─────────────────────────────────────┤
│          SchM (Services)            │
├─────────────────────────────────────┤
│  All BSW MainFunctions + Os         │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Schedule Table**: 预定义的调度时间表（周期 + 偏移 + 模块列表）
- **Timing Monitor**: 监控 MainFunction 执行时间是否超时
- **Exclusive Area Manager**: 管理 BSW 模块的临界区保护

### 3.3 文件结构

```
src/bsw/services/schm/
├── include/
│   ├── SchM.h          # 公共 API
│   ├── SchM_Cfg.h      # 调度表配置
│   └── SchM_<Module>.h # 各模块调度接口
└── src/
    ├── SchM.c           # 核心调度器
    └── SchM_Lcfg.c      # 链接时配置
```

---

## 4. 状态机

```
           SchM_Init()
  UNINIT ──────────────► IDLE
                           │
              EcuM Start → SchM_Start()
                           │
                           ▼
                       RUNNING
                  (周期调度执行中)
```

---

## 5. 数据结构

```c
typedef struct {
    uint32 PeriodMs;          /* 调度周期 */
    uint32 OffsetMs;          /* 起始偏移 */
    void (*MainFunctions[])(void);  /* 该周期内的 MainFunction 列表 */
    uint8  NumFunctions;
} SchM_ScheduleEntryType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void SchM_Init(void)` | 初始化调度器 | SWS_SchM_00001 |
| `void SchM_DeInit(void)` | 反初始化 | SWS_SchM_00002 |
| `void SchM_Start(void)` | 启动调度 | SWS_SchM_00005 |
| `void SchM_Stop(void)` | 停止调度 | SWS_SchM_00006 |
| `void SchM_MainFunction(void)` | 调度器主入口 | SWS_SchM_00004 |
| `void SchM_GetVersionInfo(Std_VersionInfoType* VersionInfo)` | 版本信息 | SWS_SchM_00003 |

### 各模块 Exclusive Area 接口

| API | 说明 |
|-----|------|
| `void SchM_Enter_<Module>(void)` | 进入模块临界区 |
| `void SchM_Exit_<Module>(void)` | 退出模块临界区 |

---

## 7. 处理流程

### 7.1 MainFunction 调度流程

1. Os 周期任务触发 `SchM_MainFunction()`
2. SchM 根据当前 Tick 查找匹配的调度条目
3. 按优先级顺序调用各 BSW 模块的 MainFunction
4. 记录每个 MainFunction 的执行时间
5. 若超时 → 报告 DET 错误

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `SCHM_MAIN_FUNCTION_PERIOD` | 5U | 调度器主周期 (ms) |
| `SCHM_MAX_SCHEDULE_ENTRIES` | 32U | 最大调度条目数 |
| `SCHM_TIMING_MONITORING` | STD_ON | 启用执行时间监控 |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `SCHM_E_UNINIT` | 初始化前调用 |
| `SCHM_E_TIMEOUT` | MainFunction 执行超时 |
| `SCHM_E_OVERLAP` | 调度冲突（上一周期未完成） |

---

## 10. 内存与性能

- **RAM**: 调度表 ~32 × 16B = 512 字节
- **ROM**: ~2 KB 代码
- **性能**: 调度开销 ~1 µs/条目

---

## 11. 集成指南

- EcuM 在启动序列中调用 `SchM_Init` + `SchM_Start`
- Os 任务中调用 `SchM_MainFunction`
- 每个 BSW 模块的 MainFunction 在 Lcfg 中注册
- Exclusive Area 保护多任务环境下的数据一致性

---

## 12. 测试策略

- 调度时序正确性测试
- MainFunction 超时检测测试
- Exclusive Area 并发保护测试
- 调度表动态切换测试

---

## 13. 实现说明

- 调度表为静态配置（预编译）
- 使用 Os Counter 作为时间基准
- Exclusive Area 通过禁用中断或自旋锁实现

---

## 14. 参考文献

- AUTOSAR_SWS_BSWSchedulerManager.pdf (R4.4.0)
- yuleASR SchM 源码: `src/bsw/services/schm/`
