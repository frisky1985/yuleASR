# SomeIpXF (SOME/IP Transformer) Design Document

> **Module ID**: 0x9C  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_SOMEIPTransformer  
> **Source Path**: `src/bsw/services/someipxf/`  
> **Reference Document**: `docs/modules/someipxf.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

SomeIpXf 模块实现 SOME/IP 数据转换器（Transformer），负责应用层数据与 SOME/IP 线格式之间的序列化（Serialization）和反序列化（Deserialization）。该模块处理 SOME/IP 消息头的构建/解析，以及多种数据类型（布尔、整数、字符串、数组等）的编码/解码。

主要功能：
- **消息头处理**：构建和解析 12 字节 SOME/IP 消息头（ServiceID/MethodID/Length/ProtocolVersion 等）
- **数据序列化**：将应用层数据编码为 SOME/IP 大端序线格式
- **数据反序列化**：将 SOME/IP 线格式数据解码为应用层格式
- **多数据类型支持**：Boolean、UInt8~64、SInt8~64、Float32/64、String、Array、Struct
- **Transformer 配置**：每个 Transformer 实例绑定特定的 Service/Method 和数据元素定义
- **E2E 保护**：可选的端到端保护支持

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS SOME/IP Transformer | R22-11 | 模块软件规范 |
| SOME/IP Protocol Specification | 1.x | SOME/IP 协议规范 |
| AUTOSAR TPS SOME/IP | 4.4.0 | SOME/IP 技术规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | SD / Application | 服务发现与应用层数据转换 |
| 同层 | SomeIpTp | 大消息传输协议（分段/重组） |
| 公共 | Det | 开发错误检测与报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│         SD / Application / SW-C     │
├─────────────────────────────────────┤
│         SomeIpXf (Services)         │
├─────────────────────────────────────┤
│         SomeIpTp (Transport Proto)  │
├─────────────────────────────────────┤
│         SoAd (Socket Adaptor)       │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **序列化引擎**：将应用数据按数据类型编码为大端序线格式
- **反序列化引擎**：将大端序线格式数据解码为应用数据
- **消息头处理器**：构建/解析 12 字节 SOME/IP 消息头
- **Transformer 管理器**：管理多个 Transformer 实例，每个绑定特定 Service/Method
- **数据元素处理器**：根据配置的数据元素类型执行对应的序列化/反序列化
- **对齐管理器**：处理数据元素的位/字节对齐要求

### 3.3 文件结构

```
src/bsw/services/someipxf/
├── include/
│   ├── SomeIpXf.h          -- 公共 API 与类型定义
│   ├── SomeIpXf_Cfg.h      -- 预编译配置参数
│   └── SomeIpXf_MemMap.h   -- MemMap 宏定义
└── src/
    ├── SomeIpXf.c           -- 核心实现
    └── SomeIpXf_Test.c      -- 测试代码
```

---

## 4. 状态机

### 4.1 模块状态

```
SOMEIPXF_STATE_UNINIT -- Init() --> SOMEIPXF_STATE_INIT
SOMEIPXF_STATE_INIT -- DeInit() --> SOMEIPXF_STATE_UNINIT
```

模块状态简单，仅区分初始化和未初始化两种状态。

### 4.2 转换流程

```
Transform():
  1. 验证 TransformerId 和 DataElementId
  2. 如果 HeaderIncluded → 构建 SOME/IP 消息头
  3. 根据 DataType 选择序列化方法
  4. 更新消息头 Length 字段

Detransform():
  1. 验证 TransformerId 和 DataElementId
  2. 如果 HeaderIncluded → 解析并验证 SOME/IP 消息头
  3. 根据 DataType 选择反序列化方法
  4. 返回解码后的数据
```

---

## 5. 核心数据结构

### 5.1 SOME/IP 消息头

```c
typedef struct {
    uint16 ServiceId;          /* 服务 ID */
    uint16 MethodId;           /* 方法 ID */
    uint32 Length;             /* 载荷长度 */
    uint8 ProtocolVersion;     /* 协议版本 (1) */
    uint8 InterfaceVersion;    /* 接口版本 */
    uint8 MessageType;         /* 消息类型 */
    uint8 ReturnCode;          /* 返回码 */
} SomeIpXf_HeaderType;
```

消息头在总线上的格式（12 字节，大端序）：
```
Byte 0-1: Service ID
Byte 2-3: Method ID
Byte 4-7: Length (payload length)
Byte 8:   Protocol Version
Byte 9:   Interface Version
Byte 10:  Message Type
Byte 11:  Return Code
```

### 5.2 数据类型枚举

```c
typedef enum {
    SOMEIPXF_DT_BOOLEAN = 0,
    SOMEIPXF_DT_UINT8,
    SOMEIPXF_DT_UINT16,
    SOMEIPXF_DT_UINT32,
    SOMEIPXF_DT_UINT64,
    SOMEIPXF_DT_SINT8,
    SOMEIPXF_DT_SINT16,
    SOMEIPXF_DT_SINT32,
    SOMEIPXF_DT_SINT64,
    SOMEIPXF_DT_FLOAT32,
    SOMEIPXF_DT_FLOAT64,
    SOMEIPXF_DT_STRING,
    SOMEIPXF_DT_ARRAY,
    SOMEIPXF_DT_STRUCT,
    SOMEIPXF_DT_UNION
} SomeIpXf_DataTypeType;
```

### 5.3 数据元素配置

```c
typedef struct {
    SomeIpXf_DataTypeType DataType;    /* 数据类型 */
    uint16 BitSize;                    /* 位大小 */
    uint16 Alignment;                  /* 对齐要求 */
    boolean IsDynamic;                 /* 动态长度 */
    boolean IsArray;                   /* 是否数组 */
    uint16 ArraySize;                  /* 数组大小 */
    SomeIpXf_ArrayLenType ArrayLenType; /* 数组长度类型 */
    SomeIpXf_StringCodingType StringCoding; /* 字符串编码 */
    SomeIpXf_StringLenType StringLenType;   /* 字符串长度类型 */
    uint16 StringMaxLen;               /* 字符串最大长度 */
    uint16 WireType;                   /* Union 线类型 */
} SomeIpXf_DataElementConfigType;
```

### 5.4 接口配置

```c
typedef struct {
    uint16 ServiceId;              /* 服务 ID */
    uint16 MethodId;               /* 方法 ID */
    uint8 InterfaceVersion;        /* 接口版本 */
    uint8 ProtocolVersion;         /* 协议版本 */
    uint16 DataLength;             /* 数据长度 */
    uint8 MessageType;             /* 消息类型 */
    uint8 ReturnCode;              /* 返回码 */
} SomeIpXf_InterfaceConfigType;
```

### 5.5 Transformer 配置

```c
typedef struct {
    uint16 TransformerId;
    const SomeIpXf_InterfaceConfigType* InterfaceConfig;
    const SomeIpXf_DataElementConfigType* DataElements;
    uint16 NumDataElements;
    boolean HeaderIncluded;
} SomeIpXf_TransformerConfigType;
```

### 5.6 缓冲区类型

```c
typedef struct {
    uint8* Data;       /* 数据指针 */
    uint32 Length;     /* 数据长度 */
    uint32 MaxLength;  /* 最大长度 */
} SomeIpXf_BufferType;
```

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `SomeIpXf_Init(ConfigPtr)` | 0x01 | 初始化模块 |
| `SomeIpXf_DeInit()` | 0x02 | 反初始化模块 |
| `SomeIpXf_GetVersionInfo(versioninfo)` | 0x03 | 获取版本信息 |
| `SomeIpXf_Transform(TransformerId, DataElementId, SourceBuffer, TargetBuffer)` | 0x04 | 序列化转换 |
| `SomeIpXf_Detransform(TransformerId, DataElementId, SourceBuffer, TargetBuffer)` | 0x05 | 反序列化转换 |
| `SomeIpXf_TransformerInit(TransformerId, HeaderPtr)` | 0x06 | 初始化 Transformer |

### 6.2 序列化/反序列化原语

| 函数 | 说明 |
|------|------|
| `SomeIpXf_SerializeBoolean(Value, Buffer, Offset)` | 序列化布尔值 |
| `SomeIpXf_DeserializeBoolean(Buffer, Offset, Value)` | 反序列化布尔值 |
| `SomeIpXf_SerializeUint8(Value, Buffer, Offset)` | 序列化 uint8 |
| `SomeIpXf_DeserializeUint8(Buffer, Offset, Value)` | 反序列化 uint8 |
| `SomeIpXf_SerializeUint16(Value, Buffer, Offset)` | 序列化 uint16 (大端) |
| `SomeIpXf_DeserializeUint16(Buffer, Offset, Value)` | 反序列化 uint16 (大端) |
| `SomeIpXf_SerializeUint32(Value, Buffer, Offset)` | 序列化 uint32 (大端) |
| `SomeIpXf_DeserializeUint32(Buffer, Offset, Value)` | 反序列化 uint32 (大端) |
| `SomeIpXf_SerializeString(StringPtr, StringLen, Buffer, Config)` | 序列化字符串 |
| `SomeIpXf_DeserializeString(Buffer, BufferLen, StringPtr, StringLen, Config)` | 反序列化字符串 |
| `SomeIpXf_SerializeArray(ArrayPtr, ArrayLen, ElementSize, Buffer, Config)` | 序列化数组 |
| `SomeIpXf_DeserializeArray(Buffer, BufferLen, ArrayPtr, ArrayLen, ElementSize, Config)` | 反序列化数组 |
| `SomeIpXf_BuildHeader(Header, Buffer)` | 构建 SOME/IP 消息头 |
| `SomeIpXf_ParseHeader(Buffer, Header)` | 解析 SOME/IP 消息头 |

### 6.3 错误码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `SOMEIPXF_E_PARAM_POINTER` | 0x01 | 空指针 |
| `SOMEIPXF_E_PARAM_CONFIG` | 0x02 | 配置错误 |
| `SOMEIPXF_E_UNINIT` | 0x03 | 未初始化 |
| `SOMEIPXF_E_ALREADY_INITIALIZED` | 0x04 | 重复初始化 |
| `SOMEIPXF_E_INVALID_BUFFER_SIZE` | 0x05 | 无效缓冲区大小 |
| `SOMEIPXF_E_SERIALIZATION_ERROR` | 0x06 | 序列化错误 |
| `SOMEIPXF_E_DESERIALIZATION_ERROR` | 0x07 | 反序列化错误 |
| `SOMEIPXF_E_INVALID_DATA_TYPE` | 0x08 | 无效数据类型 |
| `SOMEIPXF_E_BUFFER_OVERFLOW` | 0x09 | 缓冲区溢出 |
| `SOMEIPXF_E_WRONG_INTERFACE_VERSION` | 0x0A | 接口版本错误 |
| `SOMEIPXF_E_WRONG_MESSAGE_TYPE` | 0x0B | 消息类型错误 |
| `SOMEIPXF_E_UNKNOWN_SERVICE` | 0x0C | 未知服务 |
| `SOMEIPXF_E_UNKNOWN_METHOD` | 0x0D | 未知方法 |
| `SOMEIPXF_E_WRONG_PROTOCOL_VERSION` | 0x0E | 协议版本错误 |

### 6.4 消息类型

| 类型 | 值 | 说明 |
|------|-----|------|
| `SOMEIPXF_MSG_TYPE_REQUEST` | 0x00 | 请求 |
| `SOMEIPXF_MSG_TYPE_REQUEST_NO_RETURN` | 0x01 | 无返回请求 |
| `SOMEIPXF_MSG_TYPE_NOTIFICATION` | 0x02 | 通知 |
| `SOMEIPXF_MSG_TYPE_RESPONSE` | 0x80 | 响应 |
| `SOMEIPXF_MSG_TYPE_ERROR` | 0x81 | 错误 |

### 6.5 返回码

| 返回码 | 值 | 说明 |
|--------|-----|------|
| `SOMEIPXF_RET_CODE_OK` | 0x00 | 成功 |
| `SOMEIPXF_RET_CODE_NOT_OK` | 0x01 | 失败 |
| `SOMEIPXF_RET_CODE_UNKNOWN_SERVICE` | 0x02 | 未知服务 |
| `SOMEIPXF_RET_CODE_UNKNOWN_METHOD` | 0x03 | 未知方法 |
| `SOMEIPXF_RET_CODE_NOT_READY` | 0x04 | 未就绪 |
| `SOMEIPXF_RET_CODE_NOT_REACHABLE` | 0x05 | 不可达 |
| `SOMEIPXF_RET_CODE_TIMEOUT` | 0x06 | 超时 |
| `SOMEIPXF_RET_CODE_MALFORMED_MSG` | 0x09 | 格式错误 |

---

## 7. 处理流程

### 7.1 Transform（序列化）流程

1. `SomeIpXf_Transform()` 被调用
2. 验证 TransformerId、SourceBuffer、TargetBuffer
3. 获取 Transformer 配置
4. 如果 `HeaderIncluded == TRUE`：
   - 构建 SOME/IP 消息头（12 字节）
   - 写入 ServiceId、MethodId、ProtocolVersion、InterfaceVersion、MessageType
   - `headerSize = 12`
5. 根据 DataElementId 获取数据元素配置
6. 按 DataType 执行对应序列化：
   - **BOOLEAN**: 1 字节（0 或 1）
   - **UINT8**: 1 字节直接拷贝
   - **UINT16**: 2 字节大端序编码 `SOMEIPXF_PUT_U16_BE`
   - **UINT32**: 4 字节大端序编码 `SOMEIPXF_PUT_U32_BE`
   - **STRING**: 4 字节长度字段 + 字符串数据
   - **ARRAY**: 4 字节长度字段 + 数组数据
7. 如果 HeaderIncluded → 更新消息头 Length 字段
8. 设置 TargetBuffer->Length

### 7.2 Detransform（反序列化）流程

1. `SomeIpXf_Detransform()` 被调用
2. 验证参数
3. 如果 `HeaderIncluded == TRUE`：
   - 解析 12 字节消息头
   - 验证 ProtocolVersion == SOMEIPXF_PROTOCOL_VERSION
   - 验证 ReturnCode == SOMEIPXF_RET_CODE_OK
   - `headerSize = 12`
4. 根据 DataType 执行对应反序列化：
   - **BOOLEAN**: 1 字节解码
   - **UINT8**: 1 字节直接拷贝
   - **UINT16**: 2 字节大端序解码 `SOMEIPXF_GET_U16_BE`
   - **UINT32**: 4 字节大端序解码 `SOMEIPXF_GET_U32_BE`
   - **STRING**: 读取 4 字节长度 → 拷贝字符串数据
   - **UINT64/SINT64/FLOAT32/FLOAT64**: 当前返回 E_NOT_OK（待实现）
5. 设置 TargetBuffer->Length

### 7.3 消息头格式

```
┌─────────────┬─────────────┬─────────────────────┐
│ Service ID  │ Method ID   │ Length              │
│ (16 bit)    │ (16 bit)    │ (32 bit, big-endian)│
├─────────────┼─────────────┼──────┬──────┬───────┤
│ Protocol    │ Interface  │ Msg  │ Return│
│ Version     │ Version    │ Type │ Code  │
│ (8 bit)     │ (8 bit)    │(8bit)│(8bit) │
└─────────────┴─────────────┴──────┴───────┘
```

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `SOMEIPXF_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `SOMEIPXF_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `SOMEIPXF_NUMBER_OF_TRANSFORMERS` | 8 | Transformer 数量 |
| `SOMEIPXF_NUMBER_OF_DATA_ELEMENTS` | 32 | 数据元素数量 |
| `SOMEIPXF_MAX_BUFFER_SIZE` | 1400 | 最大缓冲区大小 |
| `SOMEIPXF_MAX_STRING_LENGTH` | 256 | 最大字符串长度 |
| `SOMEIPXF_MAX_ARRAY_ELEMENTS` | 64 | 最大数组元素数 |
| `SOMEIPXF_PROTOCOL_VERSION` | 1 | 协议版本 |
| `SOMEIPXF_INTERFACE_VERSION` | 1 | 接口版本 |
| `SOMEIPXF_BIG_ENDIAN` | STD_ON | 大端序 |
| `SOMEIPXF_E2E_ENABLED` | STD_OFF | E2E 保护 |
| `SOMEIPXF_ENABLE_UNION` | STD_OFF | Union 类型支持 |

**数据类型使能：**
- Boolean/UInt8~64/SInt8~64/Float32/Float64/String/Array/Struct: 全部 STD_ON
- Union: STD_OFF

**预定义服务：**
- `SOMEIPXF_SERVICE_ID_ECU_MONITOR` (1)
- `SOMEIPXF_SERVICE_ID_DIAGNOSTICS` (2)
- `SOMEIPXF_SERVICE_ID_VEHICLE_DATA` (3)
- `SOMEIPXF_SERVICE_ID_ENGINE_CTRL` (4)

### 8.2 链接时配置

Transformer 配置通过 `SomeIpXf_Config` 全局结构体引用，包含最多 8 个 Transformer 实例。

### 8.3 构建后配置

不支持构建后配置变体。

---

## 9. 错误处理与安全

### 9.1 DET 错误

- `Init`: 重复初始化 → `SOMEIPXF_E_ALREADY_INITIALIZED`；空指针 → `SOMEIPXF_E_PARAM_POINTER`
- `DeInit`: 未初始化 → `SOMEIPXF_E_UNINIT`
- `Transform`: 未初始化 → `SOMEIPXF_E_UNINIT`；空指针 → `SOMEIPXF_E_PARAM_POINTER`；无效 TransformerId → `SOMEIPXF_E_PARAM_CONFIG`；不支持的数据类型 → `SOMEIPXF_E_INVALID_DATA_TYPE`
- `Detransform`: 同上 + 协议版本错误 → `SOMEIPXF_E_WRONG_PROTOCOL_VERSION`
- `GetVersionInfo`: 空指针 → `SOMEIPXF_E_PARAM_POINTER`

### 9.2 DEM 错误

当前实现未报告 DEM 事件。

### 9.3 安全机制

- **MemMap 保护**：所有变量和代码段使用标准 MemMap 分区
- **协议版本验证**：Detransform 时验证 ProtocolVersion
- **返回码验证**：Detransform 时验证 ReturnCode == OK
- **缓冲区溢出保护**：序列化前验证 TargetBuffer->MaxLength
- **大端序保证**：所有多字节数据使用大端序编码/解码
- **DET 宏封装**：通过 `SOMEIPXF_DET_REPORT_ERROR` 宏统一控制

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 类型 | 说明 |
|------|------|------|
| `SOMEIPXF_START_SEC_VAR_CLEARED_UNSPECIFIED` | 已清零变量 | 内部状态 |
| `SOMEIPXF_START_SEC_CODE` | 代码段 | 所有 API 函数 |

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| `SomeIpXf_InternalState` | ~12 bytes | 模块内部状态 |
| 代码段 | ~4 KB (估算) | 序列化/反序列化逻辑 |
| 栈使用 | ~50 bytes | 消息头构建/解析 |

---

## 11. 集成指南

1. **SomeIpTp 集成**：
   - 大消息通过 SomeIpTp 分段发送
   - 重组完成后 SomeIpTp 调用 `SomeIpXf_RxIndication`
2. **SD 集成**：
   - 服务发现通过 SomeIpXf 序列化/反序列化 SD 消息
3. **数据元素配置**：
   - 每个 Transformer 实例需配置 InterfaceConfig 和 DataElements
   - DataElement 的 DataType 决定序列化方式
4. **字节序**：当前实现固定使用大端序（`SOMEIPXF_BIG_ENDIAN = STD_ON`）

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| Init/DeInit | 状态转换 |
| 消息头构建 | 12 字节头编码验证 |
| 消息头解析 | 12 字节头解码验证 |
| Boolean 序列化 | 1 字节编码 |
| UInt8/16/32 序列化 | 大端序编码验证 |
| String 序列化 | 长度字段 + 数据 |
| Array 序列化 | 长度字段 + 元素数据 |
| Transform 完整流程 | Header + Payload 序列化 |
| Detransform 完整流程 | Header 验证 + Payload 反序列化 |
| 协议版本验证 | 错误版本拒绝 |
| 缓冲区溢出 | MaxLength 保护 |
| 无效数据类型 | DET 错误报告 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| SomeIpTp 集成 | 大消息分段后的序列化/反序列化 |
| 端到端通信 | 序列化 → 传输 → 反序列化 |
| 多 Transformer | 不同 Service/Method 的并发处理 |

---

## 13. 实现说明 / TODO

- **SInt 类型**：SInt8~32 的反序列化未实现（仅 UInt 系列已实现）
- **Float 类型**：Float32/Float64 的序列化/反序列化未实现
- **UINT64/SINT64**：Detransform 路径返回 E_NOT_OK
- **Struct 类型**：序列化/反序列化未实现
- **Union 类型**：`SOMEIPXF_ENABLE_UNION = STD_OFF`，未实现
- **TransformerInit**：头文件声明但 `.c` 文件中未实现
- **序列化原语**：`SomeIpXf_SerializeBoolean`、`SomeIpXf_SerializeUint8` 等函数在头文件声明但 `.c` 文件中未实现（仅在 Transform/Detransform 中内联处理）
- **E2E 保护**：`SOMEIPXF_E2E_ENABLED = STD_OFF`，端到端保护未实现
- **对齐处理**：`SomeIpXf_AlignOffset()` 函数已实现但未被调用

---

## 14. 参考资料

| 文档 | 说明 |
|------|------|
| AUTOSAR_SWS_SOMEIPTransformer | SOME/IP Transformer 模块规范 |
| SOME/IP Protocol Specification | SOME/IP 协议规范 |
| AUTOSAR_TPS_SOMEIP | SOME/IP 技术规范 |
| AUTOSAR_SWS_SOMEIPTransportProtocol | SOME/IP TP 规范 |
| `src/bsw/services/someipxf/` | 源代码目录 |

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_SomeIpXF_00001 | `SomeIpXF` | 测试 test_SomeIpXF_Init_DoubleInit_ShouldSucceed 覆盖: SomeIpXF_Init_DoubleInit_ShouldSucceed 场景 |
| SWS_SomeIpXF_00002 | `SomeIpXF_DeInit` | 测试 test_SomeIpXF_DeInit_ValidCall_ShouldSucceed 覆盖: SomeIpXF_DeInit_ValidCall_ShouldSucceed 场景 |
| SWS_SomeIpXF_00003 | `SomeIpXF_GetVersionInfo` | 测试 test_SomeIpXF_GetVersionInfo_ValidPtr_ShouldSucceed 覆盖: SomeIpXF_GetVersionInfo_ValidPtr_ShouldSucceed 场景 |
| SWS_SomeIpXF_00004 | `SomeIpXF_MainFunction` | 测试 test_SomeIpXF_MainFunction_ValidCall_ShouldSucceed 覆盖: SomeIpXF_MainFunction_ValidCall_ShouldSucceed 场景 |
| SWS_SomeIpXF_00005 | `SomeIpXF_Serialize` | 测试 test_SomeIpXF_Serialize_ValidCall_ShouldSucceed 覆盖: SomeIpXF_Serialize_ValidCall_ShouldSucceed 场景 |
| SWS_SomeIpXF_00006 | `SomeIpXF_Deserialize` | 测试 test_SomeIpXF_Deserialize_ValidCall_ShouldSucceed 覆盖: SomeIpXF_Deserialize_ValidCall_ShouldSucceed 场景 |
| SWS_SomeIpXF_00007 | `SomeIpXF_GetPayloadSize` | 测试 test_SomeIpXF_GetPayloadSize_ValidCall_ShouldReturnSize 覆盖: SomeIpXF_GetPayloadSize_ValidCall_ShouldReturnSize 场景 |
