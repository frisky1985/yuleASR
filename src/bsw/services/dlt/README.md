# DLT (Diagnostic Log and Trace) 模块

## 概述

DLT 模块实现遵循 AUTOSAR Classic Platform 4.x 标准，提供服务层的诊断日志和跟踪功能。

## 功能特性

### 1. 日志级别支持
- `DLT_LOG_VERBOSE` - 最详细的日志
- `DLT_LOG_DEBUG` - 调试信息
- `DLT_LOG_INFO` - 一般信息
- `DLT_LOG_WARN` - 警告信息
- `DLT_LOG_ERROR` - 错误信息
- `DLT_LOG_FATAL` - 致命错误

### 2. 跟踪类型
- `DLT_TRACE_TYPE_VARIABLE` - 变量跟踪
- `DLT_TRACE_TYPE_FUNCTION_IN` - 函数进入
- `DLT_TRACE_TYPE_FUNCTION_OUT` - 函数退出
- `DLT_TRACE_TYPE_STATE` - 状态跟踪
- `DLT_TRACE_TYPE_VFB` - 虚拟功能总线跟踪

### 3. 输出模式
- `DLT_OUTPUT_MODE_NONE` - 无输出
- `DLT_OUTPUT_MODE_SERIAL` - 串口输出
- `DLT_OUTPUT_MODE_NETWORK` - 网络输出
- `DLT_OUTPUT_MODE_BOTH` - 串口和网络同时输出
- `DLT_OUTPUT_MODE_BUFFER` - 仅缓冲区

## 文件结构

```
src/bsw/services/dlt/
├── include/
│   ├── Dlt.h         # 公共头文件
│   └── Dlt_Cfg.h     # 配置文件
├── src/
│   └── Dlt.c         # 实现文件
└── README.md         # 本文件
```

## 核心 API

### 初始化/反初始化
```c
void Dlt_Init(const Dlt_ConfigType* ConfigPtr);
void Dlt_DeInit(void);
void Dlt_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

### 日志功能
```c
// 简单日志
Dlt_ReturnType Dlt_LogMessage(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel,
    const char* message
);

// 格式化日志 (类似 printf)
Dlt_ReturnType Dlt_LogMessageWithArg(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel,
    const char* format,
    ...
);
```

### 跟踪功能
```c
// 跟踪点
Dlt_ReturnType Dlt_TracePoint(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_TraceType traceType,
    uint32 traceInfo
);

// 变量跟踪
Dlt_ReturnType Dlt_TraceVariable(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    const char* variableName,
    sint32 variableValue
);
```

### 上下文管理
```c
// 注册上下文
Dlt_ReturnType Dlt_RegisterContext(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    const char* description
);

// 注销上下文
Dlt_ReturnType Dlt_UnregisterContext(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId
);
```

### 日志级别管理
```c
// 设置日志级别
Dlt_ReturnType Dlt_SetLogLevel(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType newLogLevel
);

// 获取日志级别
Dlt_ReturnType Dlt_GetLogLevel(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType* logLevel
);
```

### 回调注册
```c
// 注册串口输出回调
Dlt_ReturnType Dlt_RegisterSerialOutputCbk(Dlt_SerialOutputCbkType callback);

// 注册网络输出回调
Dlt_ReturnType Dlt_RegisterNetworkOutputCbk(Dlt_NetworkOutputCbkType callback);

// 注册时间戳回调
Dlt_ReturnType Dlt_RegisterGetTimestampCbk(Dlt_GetTimestampCbkType callback);
```

### 主函数
```c
void Dlt_MainFunction(void);  // 需周期性调用
```

## 使用示例

### 基本使用
```c
#include "Dlt.h"

// 定义应用ID和上下文ID
static const Dlt_ApplicationIdType appId = {'M', 'A', 'I', 'N', '\0'};
static const Dlt_ContextIdType ctxId = {'D', 'E', 'M', 'O', '\0'};

void example_init(void)
{
    // 初始化 DLT
    Dlt_Init(NULL_PTR);  // 使用默认配置
    
    // 注册上下文
    Dlt_RegisterContext(appId, ctxId, "Demo Context");
    
    // 设置日志级别
    Dlt_SetLogLevel(appId, ctxId, DLT_LOG_DEBUG);
}

void example_logging(void)
{
    // 简单日志
    Dlt_LogMessage(appId, ctxId, DLT_LOG_INFO, "System started");
    
    // 格式化日志
    Dlt_LogMessageWithArg(appId, ctxId, DLT_LOG_DEBUG, 
                          "Temperature: %d C", 25);
    
    // 错误日志
    Dlt_LogMessage(appId, ctxId, DLT_LOG_ERROR, "Sensor failure");
}

void example_tracing(void)
{
    // 变量跟踪
    sint32 speed = 100;
    Dlt_TraceVariable(appId, ctxId, "VehicleSpeed", speed);
    
    // 函数进入跟踪
    Dlt_TracePoint(appId, ctxId, DLT_TRACE_TYPE_FUNCTION_IN, 0);
    
    // ... 函数体 ...
    
    // 函数退出跟踪
    Dlt_TracePoint(appId, ctxId, DLT_TRACE_TYPE_FUNCTION_OUT, 0);
}
```

### 自定义输出回调
```c
// 串口输出回调示例
Dlt_ReturnType my_serial_output(const uint8* data, uint16 length)
{
    // 发送到 UART
    for (uint16 i = 0; i < length; i++) {
        UART_SendByte(data[i]);
    }
    return E_OK;
}

// 注册回调
void setup_output(void)
{
    Dlt_RegisterSerialOutputCbk(my_serial_output);
}
```

## 配置选项

### Dlt_Cfg.h 主要配置

| 配置项 | 默认值 | 描述 |
|--------|--------|------|
| DLT_DEV_ERROR_DETECT | STD_ON | 开发错误检测 |
| DLT_VERSION_INFO_API | STD_ON | 版本信息 API |
| DLT_USE_TIMESTAMP | STD_ON | 使用时间戳 |
| DLT_USE_ECU_ID | STD_ON | 使用 ECU ID |
| DLT_RING_BUFFER_SIZE | 128 | 环形缓冲区条目数 |
| DLT_BUFFER_ENTRY_SIZE | 256 | 单个缓冲区条目大小 |
| DLT_MAX_CONTEXTS | 32 | 最大上下文数 |
| DLT_DEFAULT_LOG_LEVEL | DLT_LOG_DEBUG | 默认日志级别 |
| DLT_SERIAL_OUTPUT_ENABLED | STD_ON | 串口输出使能 |
| DLT_NETWORK_OUTPUT_ENABLED | STD_ON | 网络输出使能 |
| DLT_NON_BLOCKING_WRITE | STD_ON | 非阻塞写入 |
| DLT_DROP_ON_OVERFLOW | STD_ON | 溢出时丢弃 |

## 技术规范

### 协议兼容性
- 符合 AUTOSAR DLT 协议规范
- 支持标准 DLT 头部格式
- 支持扩展头部

### 性能特性
- 环形缓冲区管理，支持溢出处理
- 非阻塞写入模式
- 支持多种输出通道（串口/网络/缓冲区）
- 日志级别过滤

### 内存使用
- 静态内存分配
- 可配置的缓冲区大小
- 支持 32 个上下文

## 集成说明

### 与其他模块的关系
- 依赖: Std_Types, ComStack_Types
- 可选依赖: Det (开发错误跟踪)
- 调用关系: 可被 ASW 组件和 BSW 模块调用

### 初始化序列
1. 系统启动后调用 `Dlt_Init()`
2. 注册需要的上下文
3. 设置日志级别
4. 注册输出回调（如需要）
5. 周期性调用 `Dlt_MainFunction()`

## 版本信息
- 版本: 1.0.0
- AUTOSAR 版本: 4.4.0
- 供应商: Shanghai Yule Electronics Technology Co., Ltd.
