# Service 层规范

## 模块架构

```
┌─────────────────────────────────────────┐
│           RTE (Runtime Environment)      │
├─────────────────────────────────────────┤
│  Com    PduR    NvM    Dcm    Dem       │
│   ↑       ↑      ↑      ↑      ↑        │
├─────────────────────────────────────────┤
│  CanIf  PduR   MemIf   CanTp   Dcm      │
│   ↑              ↑                     │
├─────────────────────────────────────────┤
│           ECUAL Layer                   │
└─────────────────────────────────────────┘
```

## Com 模块规范

### 功能
- 信号发送和接收
- 信号组管理
- IPDU 管理
- 通知机制

### API
```c
Std_ReturnType Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr);
Std_ReturnType Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr);
void Com_MainFunctionRx(void);
void Com_MainFunctionTx(void);
```

## PduR 模块规范

### 功能
- PDU 路由表管理
- 上下层 PDU 转发
- 网关功能
- 多播支持

### API
```c
Std_ReturnType PduR_ComTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
Std_ReturnType PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
Std_ReturnType PduR_CanIfTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
```

## NvM 模块规范

### 功能
- NVRAM 块管理
- 读写操作
- 块保护机制
- 冗余管理

### API
```c
Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr);
Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr);
void NvM_MainFunction(void);
```

## Dcm 模块规范

### 功能
- UDS 服务处理
- 安全访问控制
- DID/RID 管理
- 会话控制

### API
```c
void Dcm_MainFunction(void);
Std_ReturnType Dcm_GetSecurityLevel(Dcm_SecLevelType* SecLevel);
Std_ReturnType Dcm_GetSesCtrlType(Dcm_SesCtrlType* SesCtrlType);
```

## Dem 模块规范

### 功能
- 诊断事件管理
- DTC 处理
- 事件老化
- 冻结帧管理

### API
```c
Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);
Std_ReturnType Dem_GetEventStatus(Dem_EventIdType EventId, Dem_UdsStatusByteType* EventStatusByte);
void Dem_MainFunction(void);
```

## 通用规范

### 错误检测
- 所有 API 必须检查参数有效性
- 使用 DET 报告开发错误
- 生产代码可以关闭 DET

### 内存分区
- 使用 MemMap.h 进行内存分区
- 代码段、数据段分离
- 支持链接时优化

### 版本信息
- 每个模块提供版本信息 API
- 版本格式: Major.Minor.Patch
