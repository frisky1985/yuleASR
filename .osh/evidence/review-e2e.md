# E2E 模块审查证据

| 属性 | 值 |
|------|-----|
| 审查时间 | 2026-07-21 08:28 |
| 审查人 | 小马 🐴 (质量架构师, subagent) |
| 审查范围 | E2E (End-to-End Protection) 模块 |
| 审查类型 | 证据链补充审查 |
| 相关文件 | docs/modules/e2e.md, src/bsw/services/e2e/ |

## 审查项

### ✅ 1. E2E 配置文件
- E2E_P01: 单发送者单接收者, CRC-8 + 计数器 + DataID
- E2E_P02: 单发送者单接收者, CRC-16 + 计数器 + DataID
- 配置通过 arxml 仿真形态的 struct cfg

### ✅ 2. 保护机制
- CRC 保护: 使用 AUTOSAR CRC 引擎 (CRC-8-SAE J1850, CRC-16-IBM)
- 计数器: 序列号单调递增检测缺失/重复
- DataID: 16-bit 消息源标识
- 超时检测: 接收窗口超时

### ✅ 3. E2E 状态机
- E2E_P01State 和 E2E_P02State 结构体
- 状态: E2E_P01STATUS_OK, E2E_P01STATUS_NONOK, E2E_P01STATUS_FATAL
- 错误计数器 E2E_P01MinErrorThresholdInit = 3

### ✅ 4. API 实现
- E2E_P01_CheckInit(), E2E_P01_Check(), E2E_P01_Protect()
- E2E_P02_CheckInit(), E2E_P02_Check(), E2E_P02_Protect()
- E2E_P01_MapStatusToSM(), E2E_P02_MapStatusToSM()

### ✅ 5. 配置要求
- Profile 明确且符合 AUTOSAR SWS E2E 规范
- 每个受保护数据路径单独实例化 E2E 状态
- 数据传输完整性由上层 (Com/SecOC) 连接

### ⚠️ 6. 发现

| 发现 | 严重度 | 建议 |
|------|--------|------|
| E2E 配置实例化未通过自动化生成 | 低 | 目前手动 struct cfg，后续可过渡到 arxml→C |
| 无 E2E SMI (State Machine Interface) 监控 | 中 | 建议在 BswM 中增加 E2E 故障反应 |

## 结论

**通过** — E2E 模块 Profile P01/P02 均已实现，CRC+计数器+DataID 三要素完整。  
状态机符合 AUTOSAR 规范要求，可通过 E2E_Check 进行端到端数据完整性验证。
