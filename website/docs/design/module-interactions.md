---
title: 模块交互设计
description: "1. **层次化调用**: 层次间通过标准接口调用，避免跨层直接访问"
sidebar_position: 38
---

# 模块交互设计

## 交互原则

1. **层次化调用**: 层次间通过标准接口调用，避免跨层直接访问
2. **回调机制**: 低层向高层通知通过回调函数
3. **配置驱动**: 所有模块行为由配置决定

## 核心交互流程

### 诊断会话

```
DCM → PDUR → CANIF → CAN
 ^                       |
 |←←←←←←←←←←←←←←←←←←←←←←←←┘
```

**流程说明**:
1. DCM 接收上层诊断请求
2. DCM 通过 PDUR 路由到 CANIF
3. CANIF 通过 CAN 驱动发送
4. 响应通过相反路径返回

### 通信数据发送

```
COM → PDUR → CANIF → CAN
```

### NVM 数据访问

```
Application → NVM → MEMIF → FEE → FLS
```

## 交互接口规范

### 标准接口命名

```
<ModuleName>_<FunctionName>
```

例如:
- `CanIf_Transmit()`
- `PduR_ComTransmit()`
- `Dem_SetEventStatus()`

### 回调接口命名

```
<CallerModule>_<CalleeModule><Notification>
```

例如:
- `PduR_CanIfRxIndication()`
- `Com_TxConfirmation()`
