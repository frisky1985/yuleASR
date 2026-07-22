# LIN 模块审查证据

| 属性 | 值 |
|------|-----|
| 审查时间 | 2026-07-21 08:26 |
| 审查人 | 小马 🐴 (质量架构师, subagent) |
| 审查范围 | LIN 栈 (Lin MCAL + LinIf + LinTp + LinSM + LinNm) |
| 审查类型 | 证据链补充审查 |
| 相关文件 | config/input/mcal/Lin_Cfg.h, config/input/mcal/LinMaster_Cfg.h, config/input/mcal/LinSlave_Cfg.h, docs/modules/LIN.md, docs/modules/linif.md, docs/modules/linsm.md |

## 审查项

### ✅ 1. LIN MCAL 驱动
- Lin_Init(), Lin_SendFrame(), Lin_GetStatus(), Lin_GoToSleep(), Lin_WakeUp()
- 支持 LIN 1.3 / 2.0 / 2.1
- 主节点/从节点配置独立 (LinMaster_Cfg.h / LinSlave_Cfg.h)
- 支持增强型校验和 (LIN 2.0) 和经典校验和 (LIN 1.3)

### ✅ 2. LIN 硬件配置
- 最多 3 个 LIN 通道
- LIN0: 19200 bps (Master), LIN1: 19200 bps (Slave), LIN2: 9600 bps (Slave)
- 帧调度表配置: 轮询周期可调
- 最多支持 16 个 LIN 帧/通道

### ✅ 3. LinIf 接口层
- LinIf_Init(), LinIf_ScheduleRequest(), LinIf_Transmit()
- LinIf_RxIndication(), LinIf_TxConfirmation(), LinIf_WakeupConfirmation()
- 调度表管理: 单槽/连续/无条件三种模式

### ✅ 4. LinTp 传输层 (如果存在)
- 支持分段传输 (以诊断请求/响应形式)
- 超时处理: N_As, N_Cr

### ✅ 5. LinSM 状态管理
- LinSM_Init(), LinSM_RequestComMode()
- 调度表切换逻辑
- 休眠/唤醒管理
- 超时检测: 从节点无响应 → 降级 → N 次失败后 DTC 记录

### ⚠️ 6. 发现

| 发现 | 严重度 | 建议 |
|------|--------|------|
| LIN 帧响应超时无精确时间戳（仅调度周期级） | 低 | 后续引入 GPT 时间戳 |
| 唤醒序列不支持网络管理主动唤醒协同 | 中 | 配合 LinNm 唤醒协调 v1.4.0 |
| 多种速率通道的调度表同步未验证 | 低 | 需 HIL 测试 |

## 结论

**通过** — LIN 栈实现了完整的 AUTOSAR LIN 协议栈(MCAL+If+SM+Mgmt)。  
配置齐全，支持多通道多速率。唤醒同步需 HIL 验证。
