# AutoSAR DLT (Diagnostic and Logging Trace) 设计规格

## 1. 概述

DLT是AutoSAR标准中的诊断日志和跟踪模块，基于AUTOSAR DLT规范 (AUTOSAR SWS DLT, R21-11)。

## 2. 架构设计

```
├──────────────────────────────────────────────────────────────────────────────────────────┐
┌──────────────────────────────────────────────────────────────────────────────────────────┘
│                                   DLT 核心模块                                                  │
│  ├───────────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                              │
│   ┌─────────────────────┐  ┌─────────────────────┐  ┌─────────────────────┐  ┌─────────────────────┐   │
│   │   DLT上下文管理   │  │  日志缓冲区    │  │  消息编码器   │  │   通信接口    │   │
│   │   (Context)    │  │  (Ring Buffer) │  │   (Encoder)   │  │ (UDP/TCP/Serial)│   │
│   └─────────────────────┘  └─────────────────────┘  └─────────────────────┘  └─────────────────────┘   │
│                                                                                              │
├──────────────────────────────────────────────────────────────────────────────────────────┘
                                         │
           ┌──────────────────────────────────────────────────────────────────────────────────────────┐
           │                           输出通道                                         │
           ├───────────────────────────────────────────────────────────────────────────────────────┘
           │                                                                              │
    ┌──────────────────┐      ┌──────────────────┐      ┌──────────────────┐      ┌──────────────────┐
    │   DLT查看器    │      │  DLT缓存    │      │  远程诊断   │      │   文件记录   │
    │ (PC端工具)   │      │ (Non-volatile)│      │ (DoIP通道)  │      │ (Flash/SD)  │
    └──────────────────┘      └──────────────────┘      └──────────────────┘      └──────────────────┘
```

## 3. 核心组件

### 3.1 DLT上下文 (Context)
- 每个SWC拥有独立的DLT上下文
- 包含Application ID和Context ID
- 支持日志级别控制

### 3.2 日志缓冲区
- 采用双缓冲区机制
- 支持丢弃策略：新旧日志丢弃或覆盖
- 可配置缓冲区大小

### 3.3 消息格式
遵循AutoSAR DLT标准报文格式：
- Standard Header (4 bytes)
- Extended Header (optional, 10 bytes)
- Payload (variable)

## 4. 与Telemetry模块的关系

```
Telemetry 埋点数据 ─────────────┐
                                      │
                                      ▼
                              ┌──────────────────────────────────────┐
                              │    DLT转换层 (Adapter)       │
                              │  - 埋点事件映射到DLT消息   │
                              │  - 时间戳格式转换       │
                              │  - 日志级别映射       │
                              └──────────────────────────────────────┘
                                      │
                                      ▼
                              DLT标准消息流
```

## 5. 配置参数

| 参数 | 说明 | 默认值 |
|-----|-----|-------|
| DLT_BUFFER_SIZE | 主缓冲区大小 | 64KB |
| DLT_MAX_CONTEXTS | 最大上下文数 | 32 |
| DLT_ENABLE_UDP | 启用UDP输出 | ON |
| DLT_UDP_PORT | UDP目标端口 | 3490 |
| DLT_ENABLE_FILE | 启用文件记录 | ON |

## 6. API设计

```c
/* 基础日志记录 */
Dlt_ReturnType Dlt_LogInfo(const DltContext *ctx, const char *msg);
Dlt_ReturnType Dlt_LogWarning(const DltContext *ctx, const char *msg);
Dlt_ReturnType Dlt_LogError(const DltContext *ctx, const char *msg);
Dlt_ReturnType Dlt_LogDebug(const DltContext *ctx, const char *msg);

/* 格式化日志 */
Dlt_ReturnType Dlt_LogFormatInfo(const DltContext *ctx, const char *fmt, ...);

/* 追踪点 */
Dlt_ReturnType Dlt_TracePoint(const DltContext *ctx, uint8_t trace_id);

/* 生成快照 */
Dlt_ReturnType Dlt_Snapshot(const DltContext *ctx);
```

## 7. 与诊断通信

- 通过DoIP (Diagnostic over IP) 发送DLT消息
- 支持UDS会话中的日志传输
- DLT查看器通过TCP连接ECU

## 8. 安全考虑

- 日志中不包含敏感数据（密钥、VIN等）
- 支持日志加密传输
- 访问控制：诊断会话必须已授权
