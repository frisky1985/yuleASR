# LnTm (LinTp) Design Document

> **Module ID**: 0x38 (56)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_LINTransport  
> **Source Path**: `src/bsw/services/lntm/`  
> **Reference Document**: `docs/modules/lntm.md`  
> **Doc Version**: 1.0  
> **Status**: Draft

---

## 1. 模块概述

LnTm（LIN Transport Layer，源码中命名为 LinTp）是 AUTOSAR BSW 服务层的 LIN 传输层模块。该模块实现了基于 ISO 15765-2 的 LIN 传输协议，负责将上层 PDU（如诊断消息）分段为 LIN 帧大小的数据单元进行传输，并在接收端进行重组。

LinTp 支持单帧（Single Frame, SF）、首帧（First Frame, FF）和连续帧（Consecutive Frame, CF）三种帧类型，通过 NAD（Node Address）寻址，管理 N_As/N_Cr 超时定时器和 STmin（最小分离时间）间隔控制。模块支持最多 1 个通道、2 个连接（诊断连接和功能连接）。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS LINTransport | 4.4.0 | LIN 传输层模块规范 |
| ISO 15765-2 | - | 道路车辆 - 通过 CAN 和 LIN 的传输协议 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Dcm | 诊断通信管理器 |
| 上层 | PduR | PDU 路由 |
| 下层 | LinIf | LIN 接口 |
| 下层 | Lin | LIN 驱动 |
| 下层 | Det | 开发错误检测 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│   Dcm / PduR (上层)                  │
├─────────────────────────────────────┤
│   LinTp (LIN 传输层)                 │
├─────────────────────────────────────┤
│   LinIf (LIN 接口)                   │
├─────────────────────────────────────┤
│   Lin Driver (LIN 驱动)              │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **分段/重组**: 将上层 PDU 分段为 SF/FF/CF 帧，接收端进行重组
- **PCI 处理**: 构建和解析协议控制信息（Protocol Control Information）
- **序列号管理**: CF 帧的序列号（SN）管理，范围 0-15 循环
- **超时管理**: N_As（发送确认超时）、N_Cr（接收超时）定时器
- **STmin 控制**: 连续帧之间的最小分离时间控制
- **连接管理**: 管理诊断连接（NAD=1）和功能连接（NAD=126）

### 3.3 文件结构

```
src/bsw/services/lntm/
├── include/
│   ├── LinTp.h         # 公共 API 与类型声明
│   └── LinTp_Cfg.h     # 预编译配置（自动生成）
└── src/
    └── LinTp.c          # 核心实现
```

---

## 4. 状态机

### 4.1 连接状态

```
              Transmit(SF)
   IDLE ──────────────────► TX_BUSY
     ▲                        │
     │  TxConfirmation(OK)    │ TxConfirmation(OK)
     │  + 所有数据已发送       │ + 还有数据
     │                        ▼
     │                   WAIT_STMIN
     │                        │
     │                   STmin 到期
     │                        │
     └────────────────────────┘
```

| 状态 | 枚举值 | 说明 |
|------|--------|------|
| `LINTP_STATE_UNINIT` | 0 | 未初始化 |
| `LINTP_STATE_IDLE` | 1 | 空闲 |
| `LINTP_STATE_TX_READY` | 2 | 发送就绪 |
| `LINTP_STATE_TX_BUSY` | 3 | 发送中 |
| `LINTP_STATE_RX_READY` | 4 | 接收就绪 |
| `LINTP_STATE_RX_BUSY` | 5 | 接收中 |
| `LINTP_STATE_WAIT_STMIN` | 6 | 等待 STmin |
| `LINTP_STATE_WAIT_FC` | 7 | 等待流控（LIN 不使用） |

---

## 5. 核心数据结构

### 5.1 连接运行时状态 `LinTp_ConnectionStateType`

```c
typedef struct {
    LinTp_StateType State;         // 当前状态
    PduIdType TxPduId;             // 发送 PDU ID
    PduIdType RxPduId;             // 接收 PDU ID
    uint16 DataLength;             // 数据总长度
    uint16 DataIndex;              // 当前数据索引
    uint8 SequenceNumber;          // 序列号 (0-15)
    uint8 STmin;                   // 最小分离时间
    uint16 N_AsTimer;              // N_As 超时定时器
    uint16 N_CrTimer;              // N_Cr 超时定时器
    uint8 STminTimer;              // STmin 定时器
    LinTp_NADType NAD;             // 节点地址
    boolean TxBusy;                // 发送忙标志
    boolean RxBusy;                // 接收忙标志
} LinTp_ConnectionStateType;
```

### 5.2 LIN TP 帧格式

**PCI 字节格式**:

| 帧类型 | PCI 高 4 位 | PCI 低 4 位 | 说明 |
|--------|-------------|-------------|------|
| SF | 0x0 | DL (数据长度) | 单帧，数据长度 0-6 |
| FF | 0x1 | DL 高 4 位 | 首帧，后续字节编码总长度 |
| CF | 0x2 | SN (序列号) | 连续帧 |

**帧数据布局** (8 字节 LIN 帧):

| 字节 | SF | FF | CF |
|------|----|----|-----|
| 0 | PCI (SF\|DL) | PCI (FF\|DL_hi) | PCI (CF\|SN) |
| 1-6 | Data[0-5] | Length_lo + Data[0-4] | Data[0-5] |
| 7 | Padding | Data[5] | Data[5] |

---

## 6. API 设计

### 6.1 公共接口

| 函数 | SID | 说明 |
|------|-----|------|
| `LinTp_Init(ConfigPtr)` | 0x01 | 初始化模块 |
| `LinTp_DeInit()` | 0x02 | 反初始化 |
| `LinTp_GetVersionInfo(VersionInfo)` | 0x03 | 获取版本信息 |
| `LinTp_Transmit(TxPduId, PduInfoPtr)` | 0x49 | 发送数据 |
| `LinTp_CancelReceive(RxPduId)` | 0x4C | 取消接收 |
| `LinTp_CancelTransmit(TxPduId)` | 0x4D | 取消发送 |
| `LinTp_ChangeParameter(id, parameter, value)` | 0x4B | 修改参数 |
| `LinTp_MainFunction()` | 0x06 | 主函数 |

### 6.2 回调函数

| 函数 | SID | 说明 |
|------|-----|------|
| `LinTp_RxIndication(RxPduId, PduInfoPtr)` | 0x42 | 接收指示 |
| `LinTp_TxConfirmation(TxPduId, result)` | 0x40 | 发送确认 |

### 6.3 错误码

| 错误码 | 宏名 | 说明 |
|--------|------|------|
| 0x01 | `LINTP_E_NOT_INITIALIZED` | 未初始化 |
| 0x02 | `LINTP_E_INVALID_PARAMETER` | 无效参数 |
| 0x03 | `LINTP_E_INVALID_POINTER` | 无效指针 |
| 0x04 | `LINTP_E_INVALID_PDU_SDU_ID` | 无效 PDU/SDU ID |
| 0x05 | `LINTP_E_PARAM_CONFIG` | 无效配置 |

---

## 7. 处理流程

### 7.1 发送流程

1. `LinTp_Transmit()` 被上层调用
2. 根据 TxPduId 查找对应的连接
3. 检查连接不忙（TxBusy = FALSE）
4. 根据数据长度决定帧类型:
   - ≤ 6 字节: 发送单帧（SF）
   - > 6 字节且 ≤ 4095 字节: 发送首帧（FF）
5. SF 处理:
   - 构建 PCI 字节（0x00 | 数据长度）
   - 复制数据到帧缓冲区
   - 填充 0xFF
   - 启动 N_As 定时器
   - 状态转为 TX_BUSY
6. FF 处理:
   - 存储数据长度和索引
   - 设置序列号为 1
   - 启动 N_As 定时器
   - 状态转为 TX_BUSY

### 7.2 发送确认处理

`LinTp_TxConfirmation()` 被 LinIf 调用:

1. 根据 TxPduId 查找连接
2. 成功确认:
   - 清除 N_As 定时器
   - 如果还有数据: 发送下一 CF，更新 SN 和 DataIndex
   - 如果数据发送完毕: 状态转为 IDLE，TxBusy = FALSE
3. 失败确认:
   - TxBusy = FALSE，状态转为 IDLE

### 7.3 接收处理

`LinTp_RxIndication()` 被 LinIf 调用:

1. 解析 PCI 字节
2. 根据 PCI 类型分发:
   - SF (0x00): 提取数据长度和数据
   - FF (0x10): 提取总长度，准备接收 CF
   - CF (0x20): 验证 SN，追加数据

### 7.4 主函数处理

`LinTp_MainFunction()` 周期调用（默认 5ms）:

1. 遍历所有通道和连接
2. 处理定时器:
   - N_As 定时器到期: 发送超时，状态转 IDLE
   - N_Cr 定时器到期: 接收超时，状态转 IDLE
   - STmin 定时器到期: 状态从 WAIT_STMIN 转 TX_BUSY
3. 处理 TX/RX 状态机

---

## 8. 配置设计

### 8.1 预编译配置

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `LINTP_DEV_ERROR_DETECT` | STD_ON | 开发错误检测 |
| `LINTP_VERSION_INFO_API` | STD_ON | 版本信息 API |
| `LINTP_NUMBER_OF_CHANNELS` | 1 | 通道数量 |
| `LINTP_NUMBER_OF_CONNECTIONS` | 2 | 连接数量 |
| `LINTP_NUMBER_OF_PDUS` | 4 | PDU 数量 |
| `LINTP_NAD_DIAGNOSTIC` | 1 | 诊断 NAD |
| `LINTP_NAD_FUNCTIONAL` | 126 | 功能 NAD |
| `LINTP_NAD_BROADCAST` | 127 | 广播 NAD |
| `LINTP_DEFAULT_N_AS_MS` | 100 | 默认 N_As 超时 (ms) |
| `LINTP_DEFAULT_N_CR_MS` | 100 | 默认 N_Cr 超时 (ms) |
| `LINTP_DEFAULT_STMIN_MS` | 10 | 默认 STmin (ms) |
| `LINTP_MAX_MESSAGE_LENGTH` | 4095 | 最大消息长度 |
| `LINTP_SF_MAX_DATA_LENGTH` | 6 | SF 最大数据长度 |
| `LINTP_FRAME_SIZE` | 8 | LIN 帧大小 |
| `LINTP_MAIN_FUNCTION_PERIOD_MS` | 5 | 主函数周期 (ms) |

---

## 9. 错误处理与安全

### 9.1 DET 错误

所有公共 API 入口进行初始化状态、指针非空、通道/连接有效性检查。

### 9.2 DEM 错误

模块不直接报告 DEM 事件。超时错误通过状态机内部处理。

### 9.3 安全机制

- **N_As/N_Cr 超时保护**: 防止发送/接收无限等待
- **序列号验证**: CF 帧的 SN 递增验证
- **最大消息长度限制**: 不超过 4095 字节
- **编译时版本检查**: `#error` 宏确保 AR 版本一致性

---

## 10. 内存与性能

### 10.2 资源估算

| 资源 | 大小 | 说明 |
|------|------|------|
| 每连接 RAM | ~30 bytes | ConnectionStateType |
| 每通道 RAM | ~70 bytes | ChannelStateType（含 2 连接） |
| ROM（代码） | ~4 KB | 分段/重组 + API |

---

## 11. 集成指南

1. **Dcm 集成**: Dcm 通过 `LinTp_Transmit()` 发送诊断消息
2. **PduR 集成**: PDU 路由配置 TxPduId/RxPduId
3. **LinIf 集成**: 注册 RxIndication/TxConfirmation 回调
4. **LinM 集成**: 诊断调度表用于 LinTp 帧传输
5. **EcuM 集成**: 在启动阶段调用 `LinTp_Init()`
6. **SchM 集成**: 配置 `LinTp_MainFunction()` 调用周期（5ms）

---

## 12. 测试策略

### 12.1 单元测试

| 测试项 | 说明 |
|--------|------|
| 初始化测试 | NULL 指针检测、连接初始化验证 |
| SF 发送 | ≤ 6 字节数据的单帧发送 |
| FF/CF 发送 | > 6 字节数据的多帧发送 |
| SF 接收 | 单帧接收和数据提取 |
| 序列号测试 | CF 序列号 0→15→0 循环 |
| 超时测试 | N_As/N_Cr 超时触发状态回退 |
| STmin 测试 | 连续帧间隔满足 STmin 要求 |
| 取消操作 | CancelReceive/CancelTransmit 功能 |

### 12.2 集成测试

| 测试项 | 说明 |
|--------|------|
| Dcm-LinTp 集成 | 诊断消息的端到端传输 |
| LinTp-LinIf 集成 | 帧传输的回调验证 |
| 多连接测试 | 诊断连接和功能连接并行工作 |

---

## 13. 实现说明 / TODO

- `LinTp_ResetToDefaultParameters()` 中引用了未定义的 `LINTM_DEV_ERROR_DETECT` 宏（编译错误风险）
- `LinTp_MainFunction()` 和 `LinTp_RxIndication()` 中同样引用了 `LINTM_*` 宏
- `LinTp_SendSingleFrame()` 中帧填充 0xFF 的代码被注释掉
- `LinTp_SendFirstFrame()` 仅存储元数据，未实际构建 FF 帧
- `LinTp_RxIndication()` 中 SF/FF/CF 的处理逻辑为空（仅解析 PCI 类型）
- `LinTp_CancelReceive()` 和 `LinTp_CancelTransmit()` 为桩实现
- `LinTp_ChangeParameter()` 仅处理 TP_STMIN 和 TP_BS 参数
- 接收端的数据重组逻辑尚未完整实现

---

## 14. 参考资料

- AUTOSAR SWS LINTransport (AUTOSAR_SWS_LINTransport.pdf)
- ISO 15765-2 (Road vehicles - Diagnostic communication over Controller Area Network)
- LIN Protocol Specification V2.1
- 源码: `src/bsw/services/lntm/`
