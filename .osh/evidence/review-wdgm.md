# WdgM 模块审查证据

| 属性 | 值 |
|------|-----|
| 审查时间 | 2026-07-21 08:30 |
| 审查人 | 小马 🐴 (质量架构师, subagent) |
| 审查范围 | WdgM (Watchdog Manager) 模块 |
| 审查类型 | 证据链补充审查 |
| 相关文件 | src/bsw/services/wdgm, docs/modules/wdgm.md, config/input/mcal/Wdg_Cfg.h |

## 审查项

### ✅ 1. WdgM 管理功能
- WdgM_Init(), WdgM_MainFunction(), WdgM_GetMode(), WdgM_SetMode()
- WdgM_PerformReset(), WdgM_CheckpointReached()
- 监督实体: AliveSupervision, DeadlineSupervision, LogicalSupervision

### ✅ 2. 监督功能

| 监督类型 | 实现 | 说明 |
|---------|------|------|
| AliveSupervision | ✅ | 监督实体周期性"活着"信号 |
| DeadlineSupervision | ✅ | 任务必须在时间窗内执行 |
| LogicalSupervision | ✅ | 编程逻辑序列验证 |

### ✅ 3. WdgM 模式管理
- WdgM_MODE_OFF / WdgM_MODE_SLOW / WdgM_MODE_FAST
- 模式切换触发条件可配置
- 过渡到 OFF 时触发硬件看门狗复位

### ✅ 4. 与硬件 WDG 的接口
- WdgM 调用 Wdg_Trigger(), Wdg_SetMode(), Wdg_GetVersionInfo()
- 使用 MCAL Wdg 驱动的脉冲触发接口
- 硬件 WDG 超时: 100ms (FAST), 500ms (SLOW), OFF (不超时)

### ⚠️ 5. 发现

| 发现 | 严重度 | 建议 |
|------|--------|------|
| Supervision 实体数量硬编码为 4 | 中 | v1.4.0 改为 cfg 可配 |
| 无 WdgM BIST (Built-In Self Test) | 高 | v1.4.0 必须实现 WdgM 自检 |
| 无 WdgM_GetVersionInfo 函数实现 | 低 | 参考 AUTOSAR 补充 |

## 结论

**有条件通过** — WdgM 三个监督类型均实现，模式管理完整。  
BIST 缺失是 v1.3.0 的已知缺口，建议在 v1.4.0 作为 P0 任务补充。
