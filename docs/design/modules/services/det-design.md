# Det Design Document

> **Module ID**: 0x0F (15)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS Default Error Tracer  
> **Source Path**: `src/bsw/services/det/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

Det (Default Error Tracer) 是 AUTOSAR BSW 的基础错误追踪模块，为所有 BSW 模块提供统一的运行时错误报告接口。当 BSW 模块检测到开发错误（如参数越界、未初始化调用等）时，通过 `Det_ReportError()` 将错误信息记录到内部环形缓冲区。Det 是 ASIL-D 安全链路的关键组件，为 Dem 事件管理和调试分析提供数据源。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS DefaultErrorTracer | 4.4.0 | Det 模块规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | 全部 BSW 模块 | 调用 Det_ReportError 报告开发错误 |
| 下层 | Dem | 消费 Det 缓冲区的错误记录，转为 DTC 事件 |
| 下层 | EcuM | 初始化阶段调用 Det_Init |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│      All BSW Modules (callers)      │
├─────────────────────────────────────┤
│          Det (Services)             │
├─────────────────────────────────────┤
│     Dem / EcuM (consumers)          │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Error Ring Buffer**: 环形缓冲区，存储 (ModuleId, InstanceId, ApiId, ErrorId) 四元组
- **Filter**: 可选的错误过滤，按模块/错误码屏蔽
- **Notification**: 缓冲区满或新条目时的回调通知

### 3.3 文件结构

```
src/bsw/services/det/
├── include/
│   ├── Det.h           # 公共 API 声明
│   ├── Det_Cfg.h       # 预编译配置
│   └── Det_MemMap.h    # 内存段映射
└── src/
    ├── Det.c            # 核心实现
    └── Det_Lcfg.c       # 链接时配置
```

---

## 4. 状态机

```
          Det_Init()
UNINIT ──────────────► READY
                         │
            ┌────────────┴────────────┐
            │                         │
     ReportError()             GetEntries()
     (写入缓冲区)              (Dem 读取)
```

Det 只有两个状态：UNINIT 和 READY。初始化后持续运行，无复杂状态转换。

---

## 5. 数据结构

### 5.1 错误记录

```c
typedef struct {
    uint16 ModuleId;
    uint8  InstanceId;
    uint8  ApiId;
    uint8  ErrorId;
} Det_ErrorEntryType;
```

### 5.2 配置类型

```c
typedef struct {
    uint16 BufferSize;          /* 环形缓冲区大小 */
    boolean OverwriteMode;      /* TRUE=满时覆盖最旧条目 */
    void (*NotificationCb)(void); /* 缓冲区满回调 */
} Det_ConfigType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void Det_Init(const Det_ConfigType* Config)` | 初始化 Det 模块和环形缓冲区 | SWS_Det_00001 |
| `void Det_DeInit(void)` | 反初始化，清除缓冲区 | SWS_Det_00002 |
| `Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)` | 记录一条错误到缓冲区 | SWS_Det_00003 |
| `uint16 Det_GetEntries(Det_ErrorEntryType* Buffer, uint16 MaxEntries)` | 读取并消费缓冲区中的错误条目 | SWS_Det_00004 |
| `uint16 Det_GetErrorCount(void)` | 返回当前缓冲区中的错误数量 | SWS_Det_00005 |
| `void Det_GetVersionInfo(Std_VersionInfoType* VersionInfo)` | 返回版本信息 | SWS_Det_00006 |

---

## 7. 处理流程

### 7.1 ReportError 流程

1. 检查模块是否已初始化，未初始化直接返回
2. 写入 (ModuleId, InstanceId, ApiId, ErrorId) 到环形缓冲区写指针位置
3. 写指针递增（取模 BufferSize）
4. 若缓冲区满且 OverwriteMode=TRUE，覆盖最旧条目
5. 若注册了 NotificationCb 且缓冲区达到阈值，触发回调

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `DET_BUFFER_SIZE` | 64U | 环形缓冲区条目数 |
| `DET_OVERWRITE_MODE` | TRUE | 满时覆盖模式 |
| `DEV_ERROR_DETECT` | STD_ON | 编译时启用/禁用 DET 报告 |

---

## 9. 错误处理

Det 本身作为错误追踪基础设施，其错误处理极简：
- `Det_Init` 收到 NULL 指针 → 不初始化，后续 ReportError 静默返回
- `Det_GetEntries` 缓冲区为空 → 返回 0 条目
- 缓冲区溢出 → 根据 OverwriteMode 决定覆盖或丢弃

---

## 10. 内存与性能

- **RAM**: 环形缓冲区 = BufferSize × sizeof(Det_ErrorEntryType) = 64 × 4 = 256 字节
- **ROM**: ~1 KB（代码段）
- **性能**: ReportError 为 O(1) 操作，无锁写入（单任务上下文）

---

## 11. 集成指南

- 所有 BSW 模块通过 `Det_ReportError(MODULE_ID, instanceId, apiId, errorId)` 报告错误
- Dem 在 MainFunction 中调用 `Det_GetEntries()` 消费错误并映射为 DTC 事件
- EcuM 在启动序列中调用 `Det_Init()` 早于所有其他 BSW 模块初始化

---

## 12. 测试策略

- 初始化/反初始化测试
- 缓冲区写入/读取环形行为测试
- 溢出覆盖行为测试
- NULL 指针参数测试
- 多线程安全测试（若支持）

---

## 13. 实现说明

- 环形缓冲区使用 volatile 索引保证 ISR 安全
- 支持 MemMap 段放置（DET_START_SEC_VAR / DET_STOP_SEC_VAR）
- 编译时可通过 DET_DEV_ERROR_DETECT=STD_OFF 完全移除 Det 调用

---

## 14. 参考文献

- AUTOSAR_SWS_DefaultErrorTracer.pdf (R4.4.0)
- yuleASR Det 模块源码: `src/bsw/services/det/`
