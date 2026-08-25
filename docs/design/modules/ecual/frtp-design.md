# FrTp Design Document

> **Module ID**: 0x2D (45)  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS FlexRay Transport Protocol  
> **Source Path**: `src/bsw/ecual/frtp/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

FrTp (FlexRay Transport Protocol) 是 FlexRay 总线的传输协议层模块，提供长 I-PDU 在 FlexRay 帧上的分段和重组功能。FrTp 实现 AUTOSAR TP 协议，处理多帧发送（SF/FF/CF）、流控（FC）、连接管理和超时监督，桥接 PduR 和 FrIf 之间的传输层。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS FrTp | 4.4.0 | FlexRay TP 规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | PduR | PDU 路由，发送/接收请求 |
| 下层 | FrIf | FlexRay 帧收发 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│            PduR                     │
├─────────────────────────────────────┤
│          FrTp (ECUAL)               │
├─────────────────────────────────────┤
│          FrIf                       │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **TX State Machine**: SF/FF/CF 发送状态机
- **RX State Machine**: 帧重组状态机
- **Flow Control**: CTS/WT/OVF 流控处理
- **Timer Manager**: N_As/N_Bs/N_Cs/N_Cr 超时管理
- **PDU Encoder/Decoder**: PCI 字节编解码

### 3.3 文件结构

```
src/bsw/ecual/frtp/
├── include/
│   ├── FrTp.h          # 公共 API
│   ├── FrTp_Cfg.h      # 配置
│   ├── FrTp_Lcfg.h     # 运行时结构
│   └── FrTp_Private.h  # 内部定义
└── src/
    ├── FrTp.c           # 初始化/MainFunction
    ├── FrTp_Tx.c        # 发送逻辑
    ├── FrTp_Rx.c        # 接收逻辑
    ├── FrTp_TxSm.c      # 发送状态机
    ├── FrTp_PrivUtil.c  # PDU 编解码 + 工具函数
    └── FrTp_Lcfg.c      # 链接时配置
```

---

## 4. 状态机

### 4.1 连接级状态

```
  IDLE ──Transmit()──► TX_WAIT_FC ──FC(CTS)──► TX_SENDING_CF ──done──► IDLE
    │                      │                        │
    │                 N_Bs timeout              N_As timeout
    │                      │                        │
    └─────────────────── IDLE ◄─────────────────────┘

  IDLE ──RxInd(FF)──► RX_WAIT_CF ──all CF──► IDLE
                           │
                      N_Cr timeout
                           │
                        IDLE
```

---

## 5. 数据结构

```c
typedef enum {
    FRTP_FRAME_SF = 0x00U,
    FRTP_FRAME_FF = 0x01U,
    FRTP_FRAME_CF = 0x02U,
    FRTP_FRAME_FC = 0x03U
} FrTp_FrameTypeType;

typedef enum {
    FRTP_FS_CTS = 0x00U,
    FRTP_FS_WT  = 0x01U,
    FRTP_FS_OVF = 0x02U
} FrTp_FlowStatusType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void FrTp_Init(const FrTp_ConfigType* CfgPtr)` | 初始化 | SWS_FrTp_00001 |
| `void FrTp_DeInit(void)` | 反初始化 | SWS_FrTp_00002 |
| `Std_ReturnType FrTp_Transmit(PduIdType TxSduId, const PduInfoType* Info)` | 发起 TP 发送 | SWS_FrTp_00005 |
| `Std_ReturnType FrTp_CancelTransmit(PduIdType TxSduId)` | 取消发送 | SWS_FrTp_00006 |
| `Std_ReturnType FrTp_CancelReceive(PduIdType RxSduId)` | 取消接收 | SWS_FrTp_00007 |
| `Std_ReturnType FrTp_ChangeParameter(PduIdType id, TPParameterType param, uint16 value)` | 修改参数 | SWS_FrTp_00008 |
| `void FrTp_RxIndication(PduIdType RxPduId, const PduInfoType* Info)` | FrIf 接收回调 | SWS_FrTp_00009 |
| `void FrTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)` | FrIf 发送确认 | SWS_FrTp_00010 |
| `void FrTp_MainFunction(void)` | 周期主函数 | SWS_FrTp_00004 |

---

## 7. 处理流程

### 7.1 多帧发送流程

1. PduR 调用 `FrTp_Transmit(SduId, PduInfo)`
2. 判断 SDU 长度：≤ 单帧 → 发送 SF；> 单帧 → 发送 FF
3. 发送 FF 后进入 TX_WAIT_FC，启动 N_Bs 定时器
4. 收到 FC(CTS) → 按 BlockSize 发送 CF 帧
5. 每个 Block 结束后等待下一个 FC
6. 全部发送完成 → 通知 PduR

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `FRTP_MAX_CONNECTIONS` | 4U | 最大连接数 |
| `FRTP_MAX_PDU_LENGTH` | 4095U | 最大 SDU 长度 |
| `FRTP_MAIN_FUNCTION_PERIOD` | 10U | 主函数周期 (ms) |
| `FRTP_DEFAULT_N_AS` | 1000U | N_As 超时 (ms) |
| `FRTP_DEFAULT_N_BS` | 1000U | N_Bs 超时 (ms) |
| `FRTP_DEFAULT_N_CR` | 1000U | N_Cr 超时 (ms) |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `FRTP_E_UNINIT` | 初始化前调用 |
| `FRTP_E_TIMEOUT_BS` | N_Bs 超时 |
| `FRTP_E_TIMEOUT_CR` | N_Cr 超时 |
| `FRTP_E_INVALID_FS` | 无效流控状态 |
| `FRTP_E_COM_ERROR` | FrIf 发送失败 |

---

## 10. 内存与性能

- **RAM**: 每连接 ~48 字节 × 4 = 192 字节
- **ROM**: ~6 KB 代码（含编解码器）
- **性能**: SF 发送 ~20 µs, CF 发送 ~15 µs/帧

---

## 11. 集成指南

- PduR 通过 FrTp_Transmit 发起长 PDU 传输
- FrIf 提供底层 FlexRay 帧收发
- N_As/N_Bs/N_Cr 超时值需与对端节点协商一致

---

## 12. 测试策略

- 单帧/多帧发送测试
- 流控 CTS/WT/OVF 测试
- N_Bs/N_Cr 超时测试
- 发送/接收取消测试
- PCI 编解码正确性测试

---

## 13. 实现说明

- PCI 字节编码遵循 ISO 15765-2 格式
- 序列号 4-bit 循环 (0-15)
- 支持编译时裁剪 Cancel/ChangeParameter API

---

## 14. 参考文献

- AUTOSAR_SWS_FrTp.pdf (R4.4.0)
- yuleASR FrTp 源码: `src/bsw/ecual/frtp/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_FrTp_00003 | `FrTp_GetVersionInfo` | 测试 test_FrTp_GetVersionInfo_ValidPtr_ShouldSucceed 覆盖: FrTp_GetVersionInfo_ValidPtr_ShouldSucceed 场景 |
