## EcuM 模块 — 模块审查
- 审查时间: 2026-07-22
- 审查人: 小马 (质量架构师)
- 结论: 通过
- 发现: P2 分类

### 审查范围
- 源文件: `src/bsw/services/ecum/`
- 测试文件: `tests/unit/autosar/services/test_ecum.c`, `tests/unit/ecum/test_ecum.c`, `tests/unit/services/test_ecum.c`, `tests/integration/bsw/test_ecum_bswm_integration.c`
- 规范引用: AUTOSAR_SWS_ECUStateManager R4.0.3

### 审查项

#### ✅ 1. AUTOSAR EcuM 启动流程
- `EcuM.c` 实现完整启动流程: EcuM_Init → EcuM_StartupOne → EcuM_StartupTwo
- 启动阶段状态机: STARTUP → RUN → POST_RUN → SLEEP/SHUTDOWN
- 支持 BSW 模式管理: EcuM_StartBswMode, EcuM_StopBswMode

#### ✅ 2. 模块拆分解耦良好
- 实现拆分为多个 _impl.c 文件，按职责分离：
  - `_ecum_startup_impl.c`: 启动阶段实现
  - `_ecum_run_wakeup_impl.c`: RUN 态唤醒管理
  - `_ecum_run_sleep_impl.c`: 休眠管理
  - `_ecum_shutdown_impl.c`: 关机管理
  - `_ecum_rest_impl.c`: 复位管理

#### ✅ 3. 唤醒源管理
- CAN 唤醒: `EcuM_SetWakeupEvent(EcuConf_EcuM_WakeupSource_CAN)`
- LIN 唤醒: `EcuM_SetWakeupEvent(EcuConf_EcuM_WakeupSource_LIN)`
- 以太网唤醒: `EcuM_SetWakeupEvent(EcuConf_EcuM_WakeupSource_Ethernet)`
- Pin 唤醒: GPIO 中断触发
- 定时器唤醒: RTC 定时唤醒
- 唤醒验证: `EcuM_CheckWakeup()`, `EcuM_ValidateWakeup()`
- 唤醒源使能/禁用: `EcuM_EnableWakeupSources()`, `EcuM_DisableWakeupSources()`

#### ✅ 4. 关机管理
- 目标: EcuM_SelectShutdownTarget 支持 OFF, RESET, SLEEP
- 原因: EcuM_SelectShutdownCause
- 调用链: EcuM_Shutdown → _ecum_shutdown_impl

#### ✅ 5. 测试覆盖
- `test_ecum.c` (cmocka, 1000+ 行): 完整覆盖初始化、状态管理、RUN 请求、休眠唤醒、关机、BSW 模式
- `test_ecum_bswm_integration.c`: EcuM-BswM 集成测试
- 测试用例类型:
  - 正常功能: init, shutdown, sleep, wakeup
  - 边界条件: 双初始化, 未初始化操作
  - 错误路径: NULL 指针, 无效参数

#### ⚠️ 6. 发现项

| ID | 严重度 | 描述 | 位置 |
|----|--------|------|------|
| ECUM-P2-001 | P2 | 多唤醒源同时触发时的优先级协商策略未文档化 | EcuM.c: wakeup arbitration |
| ECUM-P2-002 | P2 | 关机超时（shutdown target transition timeout）的默认值硬编码 | EcuM_Cfg.h |

### 总体评价
EcuM 模块是项目中实现最成熟的模块之一，拆分解耦良好，测试覆盖齐全（cmocka 框架 1000+ 行测试，多文件拆分）。代码结构清晰，严格遵循 AUTOSAR 规范。仅发现 P2 级文档化和可配置性改进项。
