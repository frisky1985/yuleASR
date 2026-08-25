# StbM Design Document

> **Module ID**: 0xA2 (162)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS Time Base Manager  
> **Source Path**: `src/bsw/services/stbm/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

StbM (Time Base Manager) 提供 ECU 级别的全局时间基准管理，为所有需要精确时间戳的模块（SecOC、gPTP、Dlt、事件记录等）提供统一的时间源。StbM 管理多个时间域（Time Domain），支持全局时间、网络时间（gPTP）和硬件时间戳的同步和转换。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Time Base Manager | 4.4.0 | StbM 规范 |
| IEEE 802.1AS (gPTP) | — | 精确时间协议 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SecOC, Dlt, Dem, Gpt | 时间戳请求 |
| 下层 | Gpt (MCAL) | 硬件定时器 |
| 下层 | Eth (MCAL) | IEEE 1588 硬件时间戳 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   SecOC / Dlt / Dem / Gpt           │
├─────────────────────────────────────┤
│         StbM (Services)             │
├─────────────────────────────────────┤
│    Gpt (MCAL) / Eth HW Timestamp    │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Time Domain Manager**: 管理多个独立时间域
- **Time Sync Engine**: 全局时间同步（gPTP slave/master）
- **Timestamp Converter**: 不同时间域之间的转换
- **Offset Calculator**: 计算和补偿时钟偏移

### 3.3 文件结构

```
src/bsw/services/stbm/
├── include/
│   ├── StbM.h          # 公共 API
│   ├── StbM_Cfg.h      # 时间域配置
│   └── StbM_MemMap.h   # 内存段映射
└── src/
    └── StbM.c           # 核心实现
```

---

## 4. 状态机

```
           StbM_Init()
  UNINIT ──────────────► INITIALIZED
                            │
              StbM_StartTimeSync()
                            │
                            ▼
                     TIME_SYNC_ACTIVE
                   (时间同步运行中)
```

---

## 5. 数据结构

```c
typedef uint8 StbM_TimeDomainType;

typedef struct {
    uint32 Seconds;
    uint32 Nanoseconds;
} StbM_TimeStampType;

typedef enum {
    STBM_STATUS_OK = 0,
    STBM_STATUS_NOT_SYNCED,
    STBM_STATUS_ERROR
} StbM_StatusType;

typedef struct {
    StbM_TimeDomainType DomainId;
    uint32 TickFrequencyHz;
    boolean SyncToGptp;
    StbM_TimeStampType CurrentTime;
} StbM_TimeDomainConfigType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void StbM_Init(const StbM_ConfigType* Config)` | 初始化 | SWS_StbM_00001 |
| `void StbM_DeInit(void)` | 反初始化 | SWS_StbM_00002 |
| `Std_ReturnType StbM_GetTimeStamp(StbM_TimeDomainType Domain, StbM_TimeStampType* TimeStamp)` | 获取时间戳 |  |
| `Std_ReturnType StbM_GetTimeDiff(StbM_TimeDomainType Domain, const StbM_TimeStampType* Start, const StbM_TimeStampType* End, uint32* DiffNs)` | 计算时间差 |  |
| `Std_ReturnType StbM_SetTime(StbM_TimeDomainType Domain, const StbM_TimeStampType* NewTime)` | 设置时间（gPTP 同步） |  |
| `StbM_StatusType StbM_GetStatus(StbM_TimeDomainType Domain)` | 获取同步状态 |  |
| `void StbM_MainFunction(void)` | 周期主函数 | SWS_StbM_00004 |

---

## 7. 处理流程

### 7.1 时间戳获取流程

1. 上层调用 `StbM_GetTimeStamp(Domain, &TimeStamp)`
2. StbM 读取对应时间域的硬件计数器
3. 应用偏移补偿（gPTP offset）
4. 返回全局时间戳（秒 + 纳秒）

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `STBM_NUM_TIME_DOMAINS` | 3U | 时间域数量 |
| `STBM_TICK_FREQUENCY` | 1000000U | 计数器频率 (Hz) |
| `STBM_GPTP_SUPPORT` | STD_ON | gPTP 时间同步支持 |
| `STBM_MAIN_FUNCTION_PERIOD` | 10U | 主函数周期 (ms) |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `STBM_E_UNINIT` | 初始化前调用 |
| `STBM_E_INV_DOMAIN` | 无效时间域 |
| `STBM_E_NOT_SYNCED` | 时间域未同步 |

---

## 10. 内存与性能

- **RAM**: 每时间域 ~24 字节
- **ROM**: ~3 KB 代码
- **性能**: GetTimeStamp ~2 µs

---

## 11. 集成指南

- SecOC 使用 StbM 时间戳进行 freshness counter
- Dlt 使用 StbM 时间戳标记日志消息
- gPTP 通过 StbM_SetTime 同步全局时间
- Eth 硬件时间戳通过 StbM 转换为全局时间

---

## 12. 测试策略

- 时间戳获取正确性测试
- 时间差计算精度测试
- gPTP 同步行为测试
- 时间域切换测试
- 溢出处理测试

---

## 13. 实现说明

- 支持 32-bit 秒 + 32-bit 纳秒时间戳格式
- gPTP 偏移通过 PID 控制器平滑补偿
- 硬件时间戳通过 Eth MAC 的 IEEE 1588 寄存器获取

---

## 14. 参考文献

- AUTOSAR_SWS_TimeBaseManager.pdf (R4.4.0)
- IEEE 802.1AS (gPTP)
- yuleASR StbM 源码: `src/bsw/services/stbm/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_StbM_00003 | `StbM_GetVersionInfo` | 测试 test_StbM_GetVersionInfo_ValidPtr_ShouldSucceed 覆盖: StbM_GetVersionInfo_ValidPtr_ShouldSucceed 场景 |
| SWS_StbM_00005 | `StbM_GetCurrentTime` | 测试 test_StbM_GetCurrentTime 覆盖: StbM_GetCurrentTime 场景 |
| SWS_StbM_00006 | `StbM_SetTime` | 测试 test_StbM_SetTime_ValidCall_ShouldSucceed 覆盖: StbM_SetTime_ValidCall_ShouldSucceed 场景 |
| SWS_StbM_00007 | `StbM_SetGlobalTime` | 测试 test_StbM_SetGlobalTime 覆盖: StbM_SetGlobalTime 场景 |
| SWS_StbM_00008 | `StbM_SyncTime` | 测试 test_StbM_SyncTime_ValidCall_ShouldSucceed 覆盖: StbM_SyncTime_ValidCall_ShouldSucceed 场景 |
| SWS_StbM_00009 | `StbM_GetTimeBaseStatus` | 测试 test_StbM_GetTimeBaseStatus 覆盖: StbM_GetTimeBaseStatus 场景 |
