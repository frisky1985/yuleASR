# 规格确认文档

## 1. 模块概览

### 1.1 待实现模块列表

| 模块 | 类型 | 优先级 | 依赖 | 估计工时 |
|------|------|---------|------|---------|
| Eth | MCAL | 高 | Det, EcuM, Port | 32h |
| Icu | MCAL | 中 | Det, Gpt, Port | 30h |
| FrTp | ECUAL | 中 | FrIf, Det, PduR | 36h |
| Ocu | MCAL | 低 | Gpt, Port, Det | 19h |

### 1.2 模块间依赖图
```
Eth ← Det, EcuM, Port
Icu ← Det, Gpt, Port
FrTp ← FrIf, Det, PduR
Ocu ← Gpt, Port, Det
```

## 2. 技术规格

### 2.1 编码规范
- 语言: C99
- 编码风格: MISRA C:2012 合规
- 安全等级: ASIL-D 兼容
- 注释: Doxygen 风格

### 2.2 安全要求
- 所有公共API必须进行参数验证
- 使用溟动检查检测错误
- 关键数据使用 volatile 限定符
- 中断服务程序最小化

### 2.3 测试要求
- 单元测试覆盖率 >= 80%
- MC/DC 覆盖率 >= 100% (安全相关代码)
- 边界条件测试
- 错误注入测试

## 3. 接口规范

### 3.1 Eth 接口
```c
/* 初始化 */
void Eth_Init(const Eth_ConfigType* CfgPtr);
Std_ReturnType Eth_ControllerInit(uint8 CtrlIdx, uint8 CfgIdx);

/* 模式管理 */
Std_ReturnType Eth_SetControllerMode(uint8 CtrlIdx, Eth_ModeType CtrlMode);
Std_ReturnType Eth_GetControllerMode(uint8 CtrlIdx, Eth_ModeType* CtrlModePtr);

/* PHY接口 */
Std_ReturnType Eth_WriteMII(uint8 CtrlIdx, uint8 TrcvIdx, uint8 RegIdx, uint16 RegVal);
Std_ReturnType Eth_ReadMII(uint8 CtrlIdx, uint8 TrcvIdx, uint8 RegIdx, uint16* RegValPtr);
uint8 Eth_GetPhyAddress(uint8 CtrlIdx, uint8 TrcvIdx);

/* 数据传输 */
BufReq_ReturnType Eth_ProvideTxBuffer(uint8 CtrlIdx, uint16 Len, uint8** BufPtr);
Std_ReturnType Eth_Transmit(uint8 CtrlIdx, uint8* BufPtr, uint16 Len);
Std_ReturnType Eth_Receive(uint8 CtrlIdx, uint8** BufPtr, uint16* LenPtr);

/* 回调 */
void Eth_TxConfirmation(uint8 CtrlIdx);
void Eth_RxIndication(uint8 CtrlIdx, uint8* BufPtr, uint16 Len);
```

### 3.2 Icu 接口
```c
/* 初始化 */
void Icu_Init(const Icu_ConfigType* ConfigPtr);
void Icu_DeInit(void);

/* 模式 */
void Icu_SetMode(Icu_ModeType Mode);
void Icu_DisableWakeup(Icu_ChannelType Channel);
void Icu_EnableWakeup(Icu_ChannelType Channel);

/* 边沿检测 */
void Icu_SetActivationCondition(Icu_ChannelType Channel, Icu_ActivationType Activation);
void Icu_DisableNotification(Icu_ChannelType Channel);
void Icu_EnableNotification(Icu_ChannelType Channel);
Icu_InputStateType Icu_GetInputState(Icu_ChannelType Channel);

/* 时间戳 */
void Icu_StartTimestamp(Icu_ChannelType Channel, Icu_ValueType* BufferPtr, uint16 BufferSize, uint16 NotifyInterval);
void Icu_StopTimestamp(Icu_ChannelType Channel);
Icu_IndexType Icu_GetTimestampIndex(Icu_ChannelType Channel);

/* 边沿计数 */
void Icu_ResetEdgeCount(Icu_ChannelType Channel);
void Icu_EnableEdgeCount(Icu_ChannelType Channel);
void Icu_DisableEdgeCount(Icu_ChannelType Channel);
Icu_EdgeNumberType Icu_GetEdgeNumbers(Icu_ChannelType Channel);

/* 信号测量 */
void Icu_StartSignalMeasurement(Icu_ChannelType Channel);
void Icu_StopSignalMeasurement(Icu_ChannelType Channel);
Icu_ValueType Icu_GetTimeElapsed(Icu_ChannelType Channel);
void Icu_GetDutyCycleValues(Icu_ChannelType Channel, Icu_DutyCycleType* DutyCycleValues);
```

### 3.3 FrTp 接口
```c
/* 初始化 */
void FrTp_Init(const FrTp_ConfigType* CfgPtr);

/* 传输 */
Std_ReturnType FrTp_Transmit(PduIdType FrTpTxSduId, const PduInfoType* FrTpTxInfoPtr);
Std_ReturnType FrTp_CancelTransmit(PduIdType FrTpTxSduId);
Std_ReturnType FrTp_CancelReceive(PduIdType FrTpRxSduId);
Std_ReturnType FrTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value);

/* 回调 */
void FrTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void FrTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/* 主函数 */
void FrTp_MainFunction(void);
```

### 3.4 Ocu 接口
```c
/* 初始化 */
void Ocu_Init(const Ocu_ConfigType* ConfigPtr);
void Ocu_DeInit(void);

/* 通道控制 */
void Ocu_StartChannel(Ocu_ChannelType ChannelNumber);
void Ocu_StopChannel(Ocu_ChannelType ChannelNumber);

/* 引脚操作 */
void Ocu_SetPinState(Ocu_ChannelType ChannelNumber, Ocu_PinStateType PinState);
void Ocu_SetPinAction(Ocu_ChannelType ChannelNumber, Ocu_PinActionType PinAction);

/* 阈值设置 */
Ocu_ReturnType Ocu_SetAbsoluteThreshold(Ocu_ChannelType ChannelNumber, Ocu_ValueType ReferenceValue, Ocu_ValueType AbsoluteValue);
Ocu_ReturnType Ocu_SetRelativeThreshold(Ocu_ChannelType ChannelNumber, Ocu_ValueType RelativeValue);
Ocu_ValueType Ocu_GetCounter(Ocu_ChannelType ChannelNumber);

/* 通知 */
void Ocu_DisableNotification(Ocu_ChannelType ChannelNumber);
void Ocu_EnableNotification(Ocu_ChannelType ChannelNumber);
```

## 4. 验收标准

### 4.1 功能验收
- [ ] 所有API按照AUTOSAR规范实现
- [ ] 所有配置参数可配置
- [ ] 正确的错误检测和报告

### 4.2 质量验收
- [ ] 单元测试通过
- [ ] 静态分析无警告
- [ ] MISRA合规
- [ ] 代码审查通过

### 4.3 文档验收
- [ ] API文档完整
- [ ] 设计文档更新
- [ ] 配置指南完成

---
版本: 1.0
日期: 2026-04-29
状态: 待审批
