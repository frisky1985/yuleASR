# LdCom Design Document

> **Module ID**: 0x3D  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_LdCom  
> **Source Path**: `src/bsw/services/ldcom/`  
> **Reference Document**: `docs/modules/ldcom.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

LdCom（Loadable Communication）模块是 AUTOSAR 服务层的一部分，负责为超出单帧 CAN/LIN/Ethernet MTU 的大型 PDU 提供分段与重组（Segmentation and Reassembly）服务。该模块位于 Com 模块之上，为上层应用提供透明的大数据传输能力，同时与下层通信驱动（CanTp、LinTp、SoAd 等）协作完成实际的分段传输。

主要职责：
- 大型 PDU 的发送端分段（Segmentation）
- 大型 PDU 的接收端重组（Reassembly）
- 分段传输的进度跟踪与状态查询
- 段间间隔控制，避免总线过载
- 传输取消与错误恢复

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS LdCom | R21-11 §12.11 | 大数据通信模块软件规范 |
| AUTOSAR Classic Platform | 4.4.0 | 经典平台 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Com | 信号/PDU 路由 |
| 上层 | ASWC | 应用层软件组件 |
| 下层 | CanTp / LinTp / SoAd | 传输协议层，提供分段传输通道 |
| 下层 | PduR | PDU 路由器 |
| 公共 | Det | 开发错误追踪 |
| 公共 | Dem | 诊断事件管理（可选） |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│        Application Layer            │
├─────────────────────────────────────┤
│              Com                    │
├─────────────────────────────────────┤
│          LdCom (Services)           │
├─────────────────────────────────────┤
│     CanTp / LinTp / SoAd / PduR    │
├─────────────────────────────────────┤
│          MCAL (Can/Lin/Eth)         │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **分段管理器（Segmentation Manager）**：负责将大型 PDU 拆分为适合传输层 MTU 的段
- **重组管理器（Reassembly Manager）**：负责将接收到的段重组为完整 PDU
- **状态跟踪器（Status Tracker）**：跟踪每个 PDU 的分段/重组状态与进度
- **调度器（Scheduler）**：控制段间发送间隔，管理 MainFunction 周期处理

### 3.3 文件结构

```
src/bsw/services/ldcom/
├── include/
│   └── LdCom.h
└── src/
    └── LdCom.c
```

---

## 4. 状态机

LdCom 分段状态机描述每个 PDU 的传输进度：

```
[LDCOM_SEG_IDLE]
    │ Transmit request
    ▼
[LDCOM_SEG_IN_PROGRESS] ──── abort/cancel ────► [LDCOM_SEG_ABORTED]
    │ all segments sent           │
    ▼                             │ error
[LDCOM_SEG_COMPLETE]              ▼
                          [LDCOM_SEG_ERROR]
```

接收方向状态机类似，从 IDLE 进入 IN_PROGRESS，收到完整数据后转为 COMPLETE。

---

## 5. 核心数据结构

```c
/* PDU 方向类型 */
typedef enum {
    LDCOM_DIR_TX,
    LDCOM_DIR_RX
} LdCom_DirectionType;

/* 分段状态类型 */
typedef enum {
    LDCOM_SEG_IDLE,
    LDCOM_SEG_IN_PROGRESS,
    LDCOM_SEG_COMPLETE,
    LDCOM_SEG_ABORTED,
    LDCOM_SEG_ERROR
} LdCom_SegmentStatusType;

/* 配置结构 */
typedef struct {
    PduIdType pduId;
    uint16 maxSegmentSize;
    uint16 interSegmentInterval;
    LdCom_DirectionType direction;
} LdCom_ConfigType;
```

---

## 6. API 设计

### 6.1 公共接口

| API | 签名 | 功能 | 备注 | SWS 需求 |
|-----|------|------|------|----------|
| LdCom_Init | `Std_ReturnType LdCom_Init(const LdCom_ConfigType* config)` | 初始化模块 | 必须首先调用 | SWS_LdCom_00001 |
| LdCom_DeInit | `void LdCom_DeInit(void)` | 反初始化 | 恢复模块至未初始化状态 | SWS_LdCom_00002 |
| LdCom_MainFunction | `void LdCom_MainFunction(void)` | 周期处理函数 | 由调度器周期调用 | SWS_LdCom_00003 |
| LdCom_Transmit | `Std_ReturnType LdCom_Transmit(PduIdType pduId, const PduInfoType* pduInfo)` | 发送大型 PDU | 自动分段 | SWS_LdCom_00004 |
| LdCom_CancelTransmit | `Std_ReturnType LdCom_CancelTransmit(PduIdType pduId)` | 取消发送 | 中止正在进行的分段传输 | SWS_LdCom_00005 |
| LdCom_RxIndication | `Std_ReturnType LdCom_RxIndication(PduIdType pduId, const PduInfoType* pduInfo)` | 接收指示 | 下层回调 | SWS_LdCom_00006 |
| LdCom_GetSegmentStatus | `Std_ReturnType LdCom_GetSegmentStatus(PduIdType pduId, LdCom_SegmentStatusType* status)` | 查询分段状态 | | SWS_LdCom_00007 |
| LdCom_GetProgress | `Std_ReturnType LdCom_GetProgress(PduIdType pduId, uint16* bytesSent, uint16* totalBytes)` | 查询传输进度 | | SWS_LdCom_00008 |
| LdCom_TriggerTransmit | `Std_ReturnType LdCom_TriggerTransmit(PduIdType pduId, PduInfoType* pduInfo)` | 触发发送 | 传输层请求 | SWS_LdCom_00009 |

### 6.2 回调函数

| 回调 | 说明 |
|------|------|
| LdCom_RxIndication | 下层传输协议层接收到段数据后调用 |
| LdCom_TriggerTransmit | 下层传输层请求发送下一段数据 |

### 6.3 服务 ID 与错误码

| SID | API | 主要错误码 |
|-----|-----|------------|
| 0x00 | Init | DET_E_PARAM_POINTER |
| 0x01 | Transmit | DET_E_UNINIT, DET_E_PARAM_POINTER |
| 0x02 | CancelTransmit | DET_E_UNINIT |
| 0x03 | RxIndication | DET_E_UNINIT |
| 0x04 | GetSegmentStatus | DET_E_UNINIT |
| 0x05 | GetProgress | DET_E_UNINIT |
| 0x06 | TriggerTransmit | DET_E_UNINIT |

---

## 7. 处理流程

### 7.1 发送流程

1. 上层调用 `LdCom_Transmit`，传入 PDU ID 和数据
2. 模块检查初始化状态和参数有效性
3. 根据 `maxSegmentSize` 计算分段数量
4. 进入 `LDCOM_SEG_IN_PROGRESS` 状态
5. `MainFunction` 周期触发，按 `interSegmentInterval` 间隔逐段调用下层传输
6. 每段发送后更新进度（`bytesSent`）
7. 所有段发送完成后转为 `LDCOM_SEG_COMPLETE`

### 7.2 接收流程

1. 下层调用 `LdCom_RxIndication`，传入接收到的段数据
2. 模块将段数据写入重组缓冲区
3. 检查是否所有段已接收完成
4. 完成后通知上层 Com 模块

---

## 8. 配置设计

### 8.1 预编译配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| LDCOM_DEV_ERROR_DETECT | STD_ON | 开发错误检测开关 |
| LDCOM_MODULE_ID | 0x0B | 模块标识符 |

### 8.2 链接时配置

| 配置表 | 说明 |
|--------|------|
| LdCom_ConfigType | 通过 `LdCom_Init` 参数传入配置 |

### 8.3 构建后配置

不适用。

---

## 9. 错误处理与安全

### 9.1 DET 错误

| 错误码 | 名称 | 触发场景 |
|--------|------|----------|
| DET_E_PARAM_POINTER | 空指针 | Config 或 pduInfo 为 NULL_PTR |
| DET_E_UNINIT | 未初始化 | 模块未初始化时调用 API |

### 9.2 DEM 错误

| 事件 ID | 名称 | 说明 |
|---------|------|------|
| — | — | 当前实现未定义 DEM 事件 |

### 9.3 安全机制

- ASIL 等级：QM（质量管理）
- 安全机制：初始化状态检查、参数有效性验证

---

## 10. 内存与性能

### 10.1 MemMap 分区

| 分区 | 用途 |
|------|------|
| 默认代码段 | LdCom.c 全部函数 |
| 默认数据段 | 配置指针、初始化标志 |

### 10.2 资源估算

| 资源 | 估算值 | 说明 |
|------|--------|------|
| RAM | ~64 bytes | 配置指针 + 初始化标志 |
| ROM | ~2 KB | 代码段（桩实现） |
| 堆栈 | ~128 bytes | 函数调用栈 |

---

## 11. 集成指南

- 与上层集成：通过 Com 模块调用 `LdCom_Transmit` 发送大数据 PDU
- 与下层集成：依赖 CanTp/LinTp/SoAd 提供分段传输通道
- 初始化顺序：Ecuc 初始化 → Det 初始化 → LdCom_Init → Com 初始化
- MainFunction 周期建议：10ms

---

## 12. 测试策略

### 12.1 单元测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| test_ldcom.c | 初始化/反初始化、空指针检测、重复初始化、发送/接收流程、状态查询 |

### 12.2 集成测试

| 场景 | 说明 |
|------|------|
| 大数据 PDU 传输 | 验证超过 MTU 的 PDU 能正确分段和重组 |
| 传输取消 | 验证 CancelTransmit 能正确中止传输 |
| 错误恢复 | 验证传输错误后的状态恢复 |

---

## 13. 实现说明 / TODO

- 当前为桩实现（Stub），`LdCom_Transmit`、`LdCom_RxIndication` 等核心函数尚未实现分段逻辑
- `LdCom_MainFunction` 为空实现，需要添加分段调度逻辑
- `LdCom_TriggerTransmit` 返回 `E_NOT_OK`，需要实现实际的段数据填充
- 需要添加重组缓冲区管理
- 需要实现段间间隔控制定时器

---

## 14. 参考资料

1. AUTOSAR_SWS_LdCom.pdf (R21-11 §12.11)
2. `docs/modules/ldcom.md`
3. `src/bsw/services/ldcom/`
