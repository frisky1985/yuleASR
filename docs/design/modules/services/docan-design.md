# DoCan Design Document

> **Module ID**: 0x4D (77)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS Diagnostics over CAN  
> **Source Path**: `src/bsw/services/docan/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

DoCan (Diagnostics over CAN) 实现基于 CAN 总线的 UDS 诊断传输层，是 ISO 15765-2 (CAN TP) 的 AUTOSAR 诊断适配层。DoCan 将 Dcm 的诊断消息通过 CanTp 进行多帧传输，管理诊断连接的建立/释放、流控参数配置和传输超时监控。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Diagnostics over CAN | 4.4.0 | DoCan 规范 |
| ISO 15765-2 | — | CAN 传输协议 |
| ISO 14229 (UDS) | — | 统一诊断服务 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Dcm | 诊断通信管理器 |
| 下层 | CanTp | CAN 传输协议 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│            Dcm                      │
├─────────────────────────────────────┤
│          DoCan (Services)           │
├─────────────────────────────────────┤
│          CanTp / CanIf              │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Connection Manager**: 诊断连接生命周期
- **PDU Router**: Dcm ↔ CanTp 消息路由
- **Timing Supervisor**: N_As/N_Bs/N_Cr 超时监控
- **Flow Control Config**: BS/STmin 参数管理

### 3.3 文件结构

```
src/bsw/services/docan/
├── include/
│   ├── DoCan.h       # 公共 API
│   └── DoCan_Cfg.h   # 连接配置
└── src/
    ├── DoCan.c        # 核心实现
    └── DoCan_Lcfg.c   # 链接时配置
```

---

## 4. 状态机

```
           DoCan_Init()
  CLOSED ──────────────► IDLE
                           │
              Dcm Start Communication
                           │
                           ▼
                        CONNECTED
                  (诊断传输可用)
```

---

## 5. 数据结构

```c
typedef struct {
    PduIdType TxPduId;
    PduIdType RxPduId;
    uint8     MaxBlockSize;
    uint8     STmin;
    uint16    N_As_Timeout;
    uint16    N_Bs_Timeout;
    uint16    N_Cr_Timeout;
} DoCan_ConnectionConfigType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void DoCan_Init(const DoCan_ConfigType* Config)` | 初始化 | SWS_DoCan_00001 |
| `void DoCan_DeInit(void)` | 反初始化 | SWS_DoCan_00002 |
| `Std_ReturnType DoCan_Transmit(PduIdType TxPduId, const PduInfoType* Info)` | 发送诊断消息 | SWS_DoCan_00005 |
| `void DoCan_RxIndication(PduIdType RxPduId, const PduInfoType* Info)` | CanTp 接收回调 | SWS_DoCan_00006 |
| `void DoCan_TxConfirmation(PduIdType TxPduId, Std_ReturnType Result)` | CanTp 发送确认 | SWS_DoCan_00007 |
| `void DoCan_MainFunction(void)` | 周期主函数 | SWS_DoCan_00004 |

---

## 7. 处理流程

### 7.1 诊断消息传输流程

1. Dcm 调用 `DoCan_Transmit(TxPduId, PduInfo)` 发送 UDS 响应
2. DoCan 路由到 CanTp_Transmit
3. CanTp 处理分段（SF/FF/CF）+ 流控
4. CanTp 完成后回调 `DoCan_TxConfirmation`
5. DoCan 通知 Dcm 传输完成

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `DOCAN_MAX_CONNECTIONS` | 2U | 最大诊断连接数 |
| `DOCAN_N_AS_TIMEOUT` | 5000U | N_As 超时 (ms) |
| `DOCAN_N_BS_TIMEOUT` | 5000U | N_Bs 超时 (ms) |
| `DOCAN_N_CR_TIMEOUT` | 5000U | N_Cr 超时 (ms) |
| `DOCAN_MAIN_FUNCTION_PERIOD` | 10U | 主函数周期 (ms) |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `DOCAN_E_UNINIT` | 初始化前调用 |
| `DOCAN_E_INV_CONNECTION` | 无效连接 |
| `DOCAN_E_TIMEOUT` | 传输超时 |

---

## 10. 内存与性能

- **RAM**: 每连接 ~32 字节
- **ROM**: ~3 KB 代码
- **性能**: 消息路由 ~5 µs

---

## 11. 集成指南

- Dcm 通过 DoCan 发送/接收 UDS 诊断消息
- CanTp 提供 CAN 多帧传输能力
- 功能寻址使用独立 RxPduId

---

## 12. 测试策略

- 单帧/多帧诊断消息测试
- 流控参数配置测试
- 超时处理测试
- 功能寻址 vs 物理寻址测试

---

## 13. 实现说明

- 支持物理寻址和功能寻址双通道
- 流控参数可运行时通过 ChangeParameter 修改
- 与 CanTp 通过标准 TP 回调接口交互

---

## 14. 参考文献

- AUTOSAR_SWS_DiagnosticsOverCAN.pdf (R4.4.0)
- ISO 15765-2 (CAN TP)
- yuleASR DoCan 源码: `src/bsw/services/docan/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_DoCan_00003 | `DoCan_GetVersionInfo` | 测试 test_DoCan_GetVersionInfo_ValidPtr_ShouldSucceed 覆盖: DoCan_GetVersionInfo_ValidPtr_ShouldSucceed 场景 |
| SWS_DoCan_00008 | `DoCan_TxConfirmation` | 测试 test_DoCan_TxConfirmation_ValidCall_ShouldSucceed 覆盖: DoCan_TxConfirmation_ValidCall_ShouldSucceed 场景 |
| SWS_DoCan_00009 | `DoCan_GetState` | 测试 test_DoCan_GetState_ValidCall_ShouldSucceed 覆盖: DoCan_GetState_ValidCall_ShouldSucceed 场景 |
