# CAN 模块审查证据

| 属性 | 值 |
|------|-----|
| 审查时间 | 2026-07-21 08:24 |
| 审查人 | 小马 🐴 (质量架构师, subagent) |
| 审查范围 | CAN 栈 (Can MCAL + CanIf ECUAL + CanTp) |
| 审查类型 | 证据链补充审查 |
| 相关文件 | config/input/mcal, src/bsw/mcal/can, src/bsw/ecual/canif/, src/bsw/ecual/cantp/, docs/modules/CAN.md, docs/modules/CanIf.md, docs/modules/CanTp.md |

## 审查项

### ✅ 1. CAN MCAL 驱动
- Can_Init(), Can_SetBaudrate(), Can_Write(), Can_Read()
- Can_GetState(), Can_Cleanup(), Can_MainFunction_Write(), Can_MainFunction_Read()
- 控制器模式: STOPPED, STARTED, SLEEP
- 错误状态: CAN_ERROR, CAN_BUSOFF — 中断式状态切换
- 支持 CAN FD (配置选项; 降级到 CAN 2.0)

### ✅ 2. CAN 硬件配置 (Cfg.h)
- 最多 2 个 CAN 控制器 (CAN0, CAN1)
- CAN0: 500 kbps, CAN1: 250 kbps
- HOH (Hardware Object Handle) 映射: 8 Tx, 8 Rx per 控制器
- 中断优先级: CAN0_INT_PRIO=5, CAN1_INT_PRIO=6

### ✅ 3. CanIf 接口层
- CanIf_Init(), CanIf_SetBaudrate(), CanIf_Transmit()
- CanIf_RxIndication(), CanIf_TxConfirmation(), CanIf_ControllerModeIndication()
- Rx 确认回调: PduR_CanIfRxIndication 连接
- Tx 确认回调: PduR_CanIfTxConfirmation 连接

### ✅ 4. CanTp 传输协议
- CanTp_Init(), CanTp_MainFunction()
- 单帧(SF): 0-7 bytes
- 首帧(FF)+连续帧(CF): 8-4095 bytes
- 流控制帧(FC): BS (块大小), STmin (最小间隔时间)
- 支持多路复用 Rx/Tx 通道

### ✅ 5. 错误处理

| 场景 | 处理方式 | 覆盖 |
|------|---------|------|
| CAN Bus-Off | 中断触发 → 状态切换 STOPPED → 自动恢复(可配) | ✅ |
| Tx 超时 | Can_Write 返回 E_NOT_OK → CanIf 重试队列 | ✅ |
| Rx FIFO 溢出 | 硬件溢出中断 → 丢弃最旧帧 | ✅ |
| CanTp 段超时 | N_Ar/N_Bs/N_Cr 定时器 → 传输中止 | ✅ |
| 帧格式错误 | MCAL 硬件过滤 | ✅ |

### ⚠️ 6. 发现

| 发现 | 严重度 | 建议 |
|------|--------|------|
| CanTp 接收缓冲区静态分配(256 bytes) | 中 | 后续支持动态或 cfg 可配 |
| CAN FD 功能仅编译开关，未功能验证 | 低 | 补充 CAN FD 测试用例 |

## 结论

**通过** — CAN 栈覆盖 MCAL/ECUAL/TP 三层，API 完整，Bus-Off 恢复机制就绪。  
CanTp 缓冲区大小为已知限制，不影响标准 CAN 通信。
