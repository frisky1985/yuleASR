# Dlt Design Document

> **Module ID**: 0xFE (254)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS Diagnostic Log and Trace  
> **Source Path**: `src/bsw/services/dlt/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

Dlt (Diagnostic Log and Trace) 提供 ECU 运行时日志和追踪信息的输出机制。Dlt 支持多种日志级别（Fatal/Error/Warn/Info/Debug/Verbose），通过多种输出通道（UART、以太网、内存缓冲区）传输日志消息。Dlt 是开发调试、产线测试和售后诊断的重要工具。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Diagnostic Log and Trace | 4.4.0 | DLT 规范 |
| GENIVI DLT | 2.x | DLT 协议基础 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | 全部 BSW/SWC | 日志输出 |
| 下层 | SoAd / Uart | 传输通道 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│      All BSW / SWC (loggers)        │
├─────────────────────────────────────┤
│          Dlt (Services)             │
├─────────────────────────────────────┤
│      SoAd / Uart (transports)       │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Log Buffer**: 环形日志消息缓冲区
- **Context Manager**: 管理日志上下文（ECU ID / Application ID / Context ID）
- **Filter**: 按级别/上下文过滤日志
- **Output Router**: 路由到 UART / Ethernet / Memory

### 3.3 文件结构

```
src/bsw/services/dlt/
├── include/
│   ├── Dlt.h          # 公共 API + 宏
│   ├── Dlt_Cfg.h      # 配置
│   ├── Dlt_Types.h    # 类型定义
│   └── Dlt_Internal.h # 内部定义
└── src/
    ├── Dlt.c           # 核心实现
    └── Dlt_Lcfg.c      # 链接时配置
```

---

## 4. 状态机

```
           Dlt_Init()
  UNINIT ──────────────► READY
                           │
              Log Message   │
              (buffered)    │
                           ▼
                        FLUSHING
                  (输出到传输通道)
```

---

## 5. 数据结构

```c
typedef enum {
    DLT_LOG_FATAL   = 1,
    DLT_LOG_ERROR   = 2,
    DLT_LOG_WARN    = 3,
    DLT_LOG_INFO    = 4,
    DLT_LOG_DEBUG   = 5,
    DLT_LOG_VERBOSE = 6
} Dlt_LogLevelType;

typedef struct {
    char EcuId[4];
    char AppId[4];
    char CtxId[4];
    Dlt_LogLevelType Level;
    uint32 Timestamp;
    uint8* PayloadPtr;
    uint16 PayloadLength;
} Dlt_MessageType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void Dlt_Init(const Dlt_ConfigType* Config)` | 初始化 | SWS_Dlt_00001 |
| `void Dlt_DeInit(void)` | 反初始化 | SWS_Dlt_00002 |
| `Std_ReturnType Dlt_Log(Dlt_LogLevelType Level, const char* AppId, const char* CtxId, const uint8* Payload, uint16 Len)` | 记录日志 |  |
| `void Dlt_MainFunction(void)` | 周期刷新缓冲区 | SWS_Dlt_00004 |
| `void Dlt_SetLogLevel(Dlt_LogLevelType Level)` | 运行时设置日志级别 | SWS_Dlt_00014 |

### 便捷宏

```c
#define DLT_LOG_FATAL(AppId, CtxId, Msg, Len)  Dlt_Log(DLT_LOG_FATAL, AppId, CtxId, Msg, Len)
#define DLT_LOG_ERROR(AppId, CtxId, Msg, Len)  Dlt_Log(DLT_LOG_ERROR, AppId, CtxId, Msg, Len)
#define DLT_LOG_INFO(AppId, CtxId, Msg, Len)   Dlt_Log(DLT_LOG_INFO, AppId, CtxId, Msg, Len)
```

---

## 7. 处理流程

### 7.1 日志记录流程

1. BSW/SWC 调用 `DLT_LOG_INFO("APP1", "CTX1", data, len)`
2. Dlt 检查日志级别过滤 → 低于阈值则丢弃
3. 构造 DLT 消息头（ECU ID + App ID + Ctx ID + 时间戳 + 级别）
4. 写入环形缓冲区
5. MainFunction 从缓冲区取出 → 路由到输出通道

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `DLT_BUFFER_SIZE` | 4096U | 日志缓冲区大小 (字节) |
| `DLT_DEFAULT_LOG_LEVEL` | DLT_LOG_INFO | 默认日志级别 |
| `DLT_MAIN_FUNCTION_PERIOD` | 10U | 刷新周期 (ms) |
| `DLT_UART_OUTPUT` | STD_ON | UART 输出通道 |
| `DLT_ETH_OUTPUT` | STD_ON | 以太网输出通道 |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `DLT_E_UNINIT` | 初始化前调用 |
| `DLT_E_BUFFER_FULL` | 缓冲区满，日志丢弃 |
| `DLT_E_TRANSPORT_ERROR` | 输出通道发送失败 |

---

## 10. 内存与性能

- **RAM**: 日志缓冲区 4 KB + 上下文注册表 ~256 字节
- **ROM**: ~3 KB 代码
- **性能**: 日志写入 ~2 µs，MainFunction 刷新 ~10 µs

---

## 11. 集成指南

- 所有 BSW 模块通过 DLT 宏输出日志
- 产线使用 DLT Viewer 工具通过以太网接收日志
- 日志级别可在运行时通过 DCM 动态调整

---

## 12. 测试策略

- 各级别日志输出测试
- 缓冲区满时丢弃行为测试
- 日志级别过滤测试
- 多输出通道路由测试

---

## 13. 实现说明

- 日志消息格式遵循 GENIVI DLT 协议
- 环形缓冲区支持 ISR 安全写入
- 时间戳使用 Os Counter 或 StbM

---

## 14. 参考文献

- AUTOSAR_SWS_DiagnosticLogAndTrace.pdf (R4.4.0)
- GENIVI DLT Daemon Specification
- yuleASR Dlt 源码: `src/bsw/services/dlt/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_Dlt_00003 | `Dlt_GetVersionInfo` | 测试 test_Dlt_GetVersionInfo_ValidPtr_ShouldSucceed 覆盖: Dlt_GetVersionInfo_ValidPtr_ShouldSucceed 场景 |
| SWS_Dlt_00005 | `Dlt_Log` | 测试 test_Dlt_Log_ValidCall_ShouldSucceed 覆盖: Dlt_Log_ValidCall_ShouldSucceed 场景 |
| SWS_Dlt_00006 | `Dlt_SendLogMessage` | 测试 test_Dlt_SendLogMessage 覆盖: Dlt_SendLogMessage 场景 |
| SWS_Dlt_00007 | `Dlt_SendTraceMessage` | 测试 test_Dlt_SendTraceMessage 覆盖: Dlt_SendTraceMessage 场景 |
| SWS_Dlt_00008 | `Dlt_RegisterContext` | 测试 test_Dlt_RegisterContext_ValidCall_ShouldSucceed 覆盖: Dlt_RegisterContext_ValidCall_ShouldSucceed 场景 |
