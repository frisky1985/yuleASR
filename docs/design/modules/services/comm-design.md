# ComM Design Document

> **Module ID**: 0x12 (18)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS Communication Manager  
> **Source Path**: `src/bsw/services/comm/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

ComM (Communication Manager) 是 AUTOSAR BSW 中管理通信状态的核心模块。ComM 协调 ECU 的通信网络状态（FULL_COMMUNICATION / PARTIAL_COMMUNICATION / NO_COMMUNICATION），控制 CAN/LIN/Ethernet 等总线网络的启动和睡眠。所有需要通信的 SWC 和 BSW 模块通过 ComM 请求/释放通信资源。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS Communication Manager | 4.4.0 | ComM 规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | RTE, SWC, Dcm | 通信请求 |
| 下层 | CanSM, LinSM, EthSM | 总线状态管理 |
| 下层 | Nm (CanNm, LinNm) | 网络管理 |
| 下层 | EcuM | 启动/睡眠协调 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│      RTE / SWC / Dcm                │
├─────────────────────────────────────┤
│         ComM (Services)             │
├─────────────────────────────────────┤
│   CanSM / LinSM / EthSM / NM        │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Channel Manager**: 管理多个通信通道（CAN/LIN/Eth）
- **Request Manager**: 管理通信请求计数和来源
- **State Machine**: 通道状态机（NO_COM → PARTIAL → FULL）
- **Inhibit Manager**: 睡眠抑制管理

### 3.3 文件结构

```
src/bsw/services/comm/
├── include/
│   ├── ComM.h       # 公共 API
│   └── ComM_Cfg.h   # 通道配置
└── src/
    ├── ComM.c        # 核心实现
    └── ComM_Lcfg.c   # 链接时配置
```

---

## 4. 状态机

```
              ComM_RequestCommunication()
  NO_COMMUNICATION ──────────────────────► PARTIAL_COMMUNICATION
                                                │
                              All SM Ready + NM Active
                                                │
                                                ▼
                                        FULL_COMMUNICATION
                                                │
                              All Requests Released + NM Timeout
                                                │
                                                ▼
                                        NO_COMMUNICATION
```

---

## 5. 数据结构

```c
typedef enum {
    COMM_NO_COMMUNICATION = 0,
    COMM_PARTIAL_COMMUNICATION,
    COMM_FULL_COMMUNICATION
} ComM_CommunicationStateType;

typedef enum {
    COMM_CHANNEL_CAN = 0,
    COMM_CHANNEL_LIN,
    COMM_CHANNEL_ETH
} ComM_ChannelType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void ComM_Init(const ComM_ConfigType* Config)` | 初始化 | SWS_ComM_00001 |
| `void ComM_DeInit(void)` | 反初始化 | SWS_ComM_00002 |
| `Std_ReturnType ComM_RequestCommunication(ComM_ChannelType Channel)` | 请求通信 |  |
| `Std_ReturnType ComM_RequestCommunicationMode(ComM_ChannelType Channel, ComM_CommunicationStateType Mode)` | 请求特定模式 |  |
| `Std_ReturnType ComM_PreventSleep(ComM_ChannelType Channel)` | 阻止睡眠 |  |
| `void ComM_MainFunction(void)` | 周期主函数 | SWS_ComM_00004 |
| `ComM_CommunicationStateType ComM_GetCommunicationState(ComM_ChannelType Channel)` | 获取通信状态 |  |

---

## 7. 处理流程

### 7.1 通信启动流程

1. SWC 或 Dcm 调用 `ComM_RequestCommunication(Channel)`
2. ComM 递增该通道的请求计数
3. 若从 NO_COMMUNICATION 转换 → 通知 CanSM/LinSM 启动网络
4. CanSM 启动 CAN 控制器 → CanNm 进入 Normal 模式
5. 所有 SM 就绪 + NM 活跃 → 进入 FULL_COMMUNICATION
6. 通知上层通信可用

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `COMM_NUM_CHANNELS` | 3U | 通信通道数 (CAN/LIN/Eth) |
| `COMM_MAIN_FUNCTION_PERIOD` | 10U | 主函数周期 (ms) |
| `COMM_T_TIME_IND` | 500U | 状态指示延迟 (ms) |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `COMM_E_UNINIT` | 初始化前调用 |
| `COMM_E_INV_CHANNEL` | 通道号越界 |

---

## 10. 内存与性能

- **RAM**: 每通道 ~48 字节（状态 + 请求计数 + 定时器）
- **ROM**: ~4 KB 代码
- **性能**: MainFunction ~3 µs/通道

---

## 11. 集成指南

- SWC 通过 RTE 间接调用 ComM
- Dcm 通过 ComM 控制诊断期间的通信保持
- CanSM/LinSM 通过 `ComM_BusSMModeIndication` 报告状态

---

## 12. 测试策略

- 通信请求/释放状态转换测试
- 多请求者并发测试
- 部分通信模式测试
- 睡眠抑制测试
- 超时自动释放测试

---

## 13. 实现说明

- 请求计数使用引用计数（多个请求者）
- 状态转换有延迟保护（防止抖动）
- 支持 Global Passive 模式（ECU 全局不通信）

---

## 14. 参考文献

- AUTOSAR_SWS_CommunicationManager.pdf (R4.4.0)
- yuleASR ComM 源码: `src/bsw/services/comm/`

## 需求追溯表

> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。

| 需求 ID | 对应 API | 功能描述 |
|---------|----------|----------|
| SWS_ComM_00003 | `ComM_GetStatus` | 测试 test_ComM_GetStatus_ValidCall_ShouldReturnStatus 覆盖: ComM_GetStatus_ValidCall_ShouldReturnStatus 场景 |
| SWS_ComM_00005 | `ComM_RequestComMode` | 测试 test_ComM_RequestComMode 覆盖: ComM_RequestComMode 场景 |
| SWS_ComM_00006 | `ComM_GetMaxComMode` | 测试 test_ComM_GetMaxComMode 覆盖: ComM_GetMaxComMode 场景 |
| SWS_ComM_00007 | `ComM_GetInhibitionStatus` | 测试 test_ComM_GetInhibitionStatus_ValidCall_ShouldReturnStatus 覆盖: ComM_GetInhibitionStatus_ValidCall_ShouldReturnStatus 场景 |
| SWS_ComM_00008 | `ComM_GetCurrentComMode` | 测试 test_ComM_GetCurrentComMode 覆盖: ComM_GetCurrentComMode 场景 |
| SWS_ComM_00009 | `ComM_MainFunction` | 测试 test_ComM_MainFunction_ValidCall_ShouldSucceed 覆盖: ComM_MainFunction_ValidCall_ShouldSucceed 场景 |
| SWS_ComM_00010 | `ComM_RequestComMode` | 测试 test_ComM_RequestComMode_ValidCall_ShouldSucceed 覆盖: ComM_RequestComMode_ValidCall_ShouldSucceed 场景 |
