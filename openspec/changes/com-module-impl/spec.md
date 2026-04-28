# COM模块技术规格

## 概述

### 范围

本规格定义AUTOSAR COM模块的实现要求，包括：
- 信号和信号组管理
- I-PDU打包和解包
- 传输模式实现
- 与PduR的接口

### 参考标准

- AUTOSAR SWS COM 4.4.0
- AUTOSAR RTE 4.4.0
- ISO 17356-6

## 功能规格

### 1. 信号管理

#### 1.1 数据类型支持

```c
┌──────────────────────────────────────────────────┐
│  信号类型              │  位大小    │  说明          │
├──────────────────────────────────────────────────┤
│  COM_BOOLEAN             │  1 bit     │  布尔值        │
│  COM_UINT8               │  8 bits    │  无符号8位     │
│  COM_UINT16              │  16 bits   │  无符号16位    │
│  COM_UINT32              │  32 bits   │  无符号32位    │
│  COM_UINT64              │  64 bits   │  无符号64位    │
│  COM_SINT8               │  8 bits    │  有符号8位     │
│  COM_SINT16              │  16 bits   │  有符号16位    │
│  COM_SINT32              │  32 bits   │  有符号32位    │
│  COM_SINT64              │  64 bits   │  有符号64位    │
│  COM_FLOAT32             │  32 bits   │  IEEE 754单精度 │
│  COM_FLOAT64             │  64 bits   │  IEEE 754双精度 │
│  COM_UINT8_N             │  N*8 bits  │  字节数组      │
└──────────────────────────────────────────────────┘
```

#### 1.2 字节序

- **LITTLE_ENDIAN**: 小端字节序（ARM默认）
- **BIG_ENDIAN**: 大端字节序（Motorola）
- 位级字节序转换由Com自动处理

### 2. I-PDU管理

#### 2.1 方向

```c
typedef enum {
    COM_SEND,       /* 发送 */
    COM_RECEIVE     /* 接收 */
} Com_IPduDirectionType;
```

#### 2.2 类型

```c
typedef enum {
    COM_NORMAL,     /* 普通PDU */
    COM_TP          /* 大数据传输(本版本不支持) */
} Com_IPduType;
```

#### 2.3 信号处理模式

```c
typedef enum {
    COM_IMMEDIATE,  /* 立即通知RTE */
    COM_DEFERRED    /* 延迟到MainFunction */
} Com_IPduSignalProcessingType;
```

### 3. 传输模式

#### 3.1 周期模式 (PERIODIC)

```
┌─────────┐      Period       ┌─────────┐      Period
│  PDU 1   │ ───────────────→ │  PDU 2   │ ───────────→ ...
└─────────┘               └─────────┘
```

- 固定周期发送
- 不响应信号触发

#### 3.2 事件模式 (EVENT)

```
┌──────┐              ┌──────┐
│Signal│ ───────────→ │  PDU  │ ──→ (immediate)
└──────┘              └──────┘
```

- 信号触发时立即发送
- 无周期发送

#### 3.3 混合模式 (MIXED)

```
┌──────┐    ┌──────┐     Period      ┌──────┐
│Signal│ ───→ │ PDU  │ ──────────→ │ PDU  │ ───────→ ...
└──────┘    └──────┘ (immediate) └──────┘ (periodic)
```

- 周期发送保活性
- 信号变化立即发送
- 支持重复发送

### 4. 死线监控

#### 4.1 接收超时

```c
┌──────────────────────────────────────────────┐
│  传感器接收  │  Timeout  │  传感器接收  │  Timeout  │┊
│     OK      ──────→    OK    ────────→  TIMEOUT  │
│   (重置计时器) │  倒计时   │  (重置计时器) │  报告超时 │
└──────────────────────────────────────────────┘
```

- 基于PDU级别的超时检测
- 超时后调用ErrorHook

#### 4.2 发送确认

- TxConfirmation超时检测
- 确认失败报告错误

### 5. 接口规格

#### 5.1 Com_SendSignal

```c
Std_ReturnType Com_SendSignal(
    Com_SignalIdType SignalId,
    const void* SignalDataPtr
);
```

**输入**
- SignalId: 信号标识符
- SignalDataPtr: 信号数据指针

**返回值**
- E_OK: 成功
- COM_SERVICE_NOT_AVAILABLE: 服务不可用
- COM_BUSY: 繁忙

**行为**
1. 检查初始化状态
2. 检查信号ID有效性
3. 拷贝数据到I-PDU缓冲区
4. 根据TransferProperty决定是否触发发送
5. 返回状态

#### 5.2 Com_ReceiveSignal

```c
Std_ReturnType Com_ReceiveSignal(
    Com_SignalIdType SignalId,
    void* SignalDataPtr
);
```

**行为**
1. 检查初始化状态
2. 检查信号ID有效性
3. 从I-PDU缓冲区提取数据
4. 转换为应用层格式
5. 写入SignalDataPtr

### 6. 性能规格

| 指标 | 最小值 | 目标值 | 说明 |
|------|--------|--------|------|
| 信号发送延迟 | - | <10us | Com_SendSignal执行时间 |
| I-PDU发送周期 | 1ms | - | 最小可配置周期 |
| 支持信号数 | 64 | 128 | 单个I-PDU |
| 支持I-PDU数 | 32 | 64 | 全局 |
| RAM占用 | - | <10KB | 含所有缓冲区 |
| 代码大小 | - | <20KB | Flash占用 |

### 7. 错误处理

#### 7.1 生产错误

| 错误码 | 条件 | 行为 |
|---------|------|------|
| COM_E_UNINIT | API调用前未初始化 | 返回E_NOT_OK，调用Det_ReportError |
| COM_E_PARAM_POINTER | 空指针参数 | 返回E_NOT_OK |
| COM_E_PARAM | 无效参数 | 返回E_NOT_OK |

#### 7.2 运行时错误

| 错误 | 处理 |
|------|------|
| 接收超时 | 调用ComErrorHook，可配置默认值 |
| 发送确认失败 | 调用ComErrorHook |
| 过载检测 | 丢弃新数据或覆盖旧数据(可配置) |

## 配置规格

### ARXML配置示例

```xml
<COM-SIGNAL>
    <SHORT-NAME>EngineSpeed</SHORT-NAME>
    <DATA-TYPE>uint16</DATA-TYPE>
    <LENGTH>16</LENGTH>
    <START-BIT>0</START-BIT>
    <ENDIANNESS>LITTLE_ENDIAN</ENDIANNESS>
    <TRANSFER-PROPERTY>TRIGGERED</TRANSFER-PROPERTY>
    <INIT-VALUE>0</INIT-VALUE>
</COM-SIGNAL>

<COM-IPDU>
    <SHORT-NAME>EngineData</SHORT-NAME>
    <LENGTH>8</LENGTH>
    <DIRECTION>SEND</DIRECTION>
    <TRANSMISSION-MODE>
        <MODE>MIXED</MODE>
        <PERIOD>100</PERIOD>  <!-- ms -->
        <REPETITION-PERIOD>10</REPETITION-PERIOD>
        <NUM-REPETITIONS>3</NUM-REPETITIONS>
    </TRANSMISSION-MODE>
</COM-IPDU>
```

---

*由 OSH Orchestrator 生成*
