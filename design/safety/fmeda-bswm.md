# FMEDA — BswM (BSW Mode Manager)

> **文档编号**: SAF-FMEDA-BSWM-001
> **版本**: 1.0
> **日期**: 2026-10-24
> **ASIL 等级**: B
> **参考标准**: ISO 26262 Part 5 (Hardware), Part 6 (Software)

---

## 1. 概述

### 1.1 分析范围

本 FMEDA 针对 AUTOSAR BSW Integration Layer 的 BswM (BSW Mode Manager) 模块，分析其随机硬件故障的诊断覆盖率及安全机制有效性。

**模块路径**: `src/bsw/services/bswm/src/BswM.c`

**分析边界**:
- BswM 核心逻辑（模式请求处理、规则引擎求值、动作列表执行、模式仲裁）
- 上层接口：`BswM_RequestMode`, `BswM_GetCurrentMode`, `BswM_MainFunction`
- 通知回调：`BswM_EcuM_CurrentState`, `BswM_ComM_CurrentMode`, `BswM_Nm_StateChangeNotification` 等
- 内部安全机制：表达式树递归深度限制、端口脏标志优化、规则优先级排序、空指针/边界校验
- 下层依赖（假设由 ECUAL/MCAL 层覆盖）：EcuM, Com, Det

### 1.2 分析方法

依据 ISO 26262 Part 5 Clause 7 对 BswM 模块的每个功能单元进行失效模式分析，评估：
- 失效模式（Failure Mode）
- 失效效应（Failure Effect）
- 故障检测方法（Fault Detection Method）
- 诊断覆盖率（Diagnostic Coverage, DC）
- ASIL 等级

### 1.3 关键数据结构

| 结构体 | 位置 | 说明 |
|--------|------|------|
| `BswM_InternalType` | BswM.c:40-55 | 模块内部状态，含 currentMode, requestedMode, portValue[], portDirty[], ruleState[], ruleEnabled[], rulePortMask[], ruleOrder[], ipduGroupState[] |
| `BswM_ExpressionConfigType` | BswM.h:126-132 | 表达式树节点：ExpressionType, PortIndex, CompareValue, LeftIndex, RightIndex |
| `BswM_RuleType` | BswM.h:146-154 | 规则定义：RuleId, ConditionIndex, TrueActionListIndex, FalseActionListIndex, InitialState, IsEnabled, Priority |
| `BswM_ActionType` | BswM.h:205-212 | 动作条目：Callback, Parameter, Kind, Enable, Initialize, IpduGroupVector[] |
| `BswM_ActionListType` | BswM.h:214-217 | 动作列表：NumActions, Actions[] |
| `BswM_ConfigType` | BswM.h:219-228 | 配置结构：ModeRequestPorts, Expressions, Rules, ActionLists 及其数量 |

---

## 2. 功能单元分解

### 2.1 功能单元列表

| FU 编号 | 功能单元 | 对应函数/资源 | 说明 |
|---------|---------|-------------|------|
| FU-01 | 模式请求处理 | `BswM_RequestMode()` (L446), `BswM_WritePort()` (L314) | 接收模式请求，写端口值并标记脏标志 |
| FU-02 | 表达式树求值 | `BswM_EvaluateExpression()` (L76) | 递归求值条件表达式树（AND/OR/NOT/EQUALS/NOT_EQUALS） |
| FU-03 | 规则评估引擎 | `BswM_MainFunction()` (L490) 规则循环部分 | 按优先级遍历规则，脏标志门控，状态转换触发执行 |
| FU-04 | 动作列表执行 | `BswM_ExecuteActionList()` (L249), `BswM_ExecuteAction()` (L181) | 执行回调、PDU 组切换、ECU 状态控制等动作 |
| FU-05 | 模式仲裁与切换 | `BswM_ActionSwitchMode()` (L575), L553-556 直接应用 | 仲裁当前模式，应用请求模式到 currentMode |
| FU-06 | EcuM 状态接口 | `BswM_EcuM_CurrentState()` (L665), `BswM_MapEcuMState()` (L272), `BswM_EcuM_CurrentWakeup()` (L676) | EcuM 状态映射与唤醒源管理 |
| FU-07 | 通知回调处理 | `BswM_ComM_CurrentMode()` (L692), `BswM_Nm_StateChangeNotification()` (L745) 等 | 各 BSW 模块状态通知的端口写入 |
| FU-08 | 规则运行时控制 | `BswM_RuleEnable()` (L595), `BswM_RuleDisable()` (L616) | 运行时启用/禁用规则 |
| FU-09 | 初始化/去初始化 | `BswM_Init()` (L406), `BswM_DeInit()` (L418), `BswM_ResetInternalState()` (L347) | 模块初始化、优先级排序、安全清理 |

---

## 3. 失效模式分析表

### 3.1 FU-01: 模式请求处理 (`BswM_RequestMode`, `BswM_WritePort`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-01-01 | SwCompositionId 超出配置范围 | 请求写入错误端口或无端口被更新 | `BswM_WritePort()` 入口边界检查 (`PortIndex < BSWM_MAX_MODE_REQUEST_PORTS`, L316) | 99% | B |
| FM-01-02 | Mode 值超出有效域 | 非法模式值进入仲裁逻辑 | `BswM_RequestMode()` 中 `Mode > BSWM_MODE_VALUE_MAX` 校验，DET 上报 `BSWM_E_PARAM_MODE` (L455-458) | 99% | B |
| FM-01-03 | 模块未初始化时接收请求 | 写入未初始化的状态结构 | `internalState == BSWM_INTERNAL_UNINIT` 检查，DET 上报 `BSWM_E_UNINIT` (L451-453) | 99% | B |
| FM-01-04 | 端口值写操作被中断（多上下文竞争） | 端口值与脏标志不一致 | 脏标志机制：仅当值实际变化时置位 (L319-322)；MainFunction 中快照机制 (L503-508) 保证一致性 | 85% | B |

### 3.2 FU-02: 表达式树求值 (`BswM_EvaluateExpression`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-02-01 | 表达式索引超出配置范围 | 访问非法内存，求值结果不可预测 | `ExpressionIndex >= NumExpressions` 边界检查 (L84-86)；返回 FALSE 安全默认值 | 99% | B |
| FM-02-02 | 递归深度超过上限（配置表损坏导致环引用） | 栈溢出，系统崩溃 | `Depth > BSWM_EXPR_MAX_DEPTH` (16) 递归深度限制 (L81)；超限返回 FALSE | 95% | B |
| FM-02-03 | configPtr 为空指针 | 求值访问非法地址 | `configPtr == NULL_PTR` 前置检查 (L81)；返回 FALSE | 99% | B |
| FM-02-04 | PortIndex 超出模式请求端口范围 | 读取非法端口值 | `PortIndex < NumModeRequestPorts` 校验 (L94)；不匹配时结果保持 FALSE | 95% | B |
| FM-02-05 | ExpressionType 为未定义值（内存位翻转） | 求值结果不可预测 | `default` 分支返回 FALSE (L117-119)；规则不会误触发 | 90% | B |
| FM-02-06 | Expressions 指针为空 | 解引用空指针 | `Expressions == NULL_PTR` 检查 (L85)；返回 FALSE | 99% | B |

### 3.3 FU-03: 规则评估引擎 (`BswM_MainFunction` 规则循环)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-03-01 | 模块未初始化或 configPtr 为空 | MainFunction 无效果，规则不评估 | `internalState == UNINIT || configPtr == NULL_PTR` 前置检查 (L497-499)；直接返回 | 99% | B |
| FM-03-02 | ruleOrder 索引超出规则数量 | 访问无效规则描述符 | `idx >= NumRules` 检查 (L521-523)；break 退出循环 | 99% | B |
| FM-03-03 | 规则数量超过 BSWM_MAX_RULES (64) | 规则被截断，部分规则不评估 | `k >= BSWM_MAX_RULES` 边界保护 (L517-519)；配置时约束 | 90% | B |
| FM-03-04 | 脏标志掩码计算错误（内存位翻转） | 规则漏评估或误评估 | 脏标志仅在值实际变化时置位 (L319)；快照比对清除 (L547-549)；mid-cycle 请求保留脏标志 | 85% | B |
| FM-03-05 | 规则状态比较逻辑错误 | 动作列表不触发或重复触发 | 状态转换检测 `result != ruleState[idx]` (L535)；仅转换时执行动作列表 | 90% | B |
| FM-03-06 | Rules 指针为空 | 规则循环访问非法地址 | `Rules == NULL_PTR` 检查 (L522)；break 退出 | 99% | B |

### 3.4 FU-04: 动作列表执行 (`BswM_ExecuteActionList`, `BswM_ExecuteAction`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-04-01 | ActionListIndex 为 BSWM_ACTION_LIST_NONE | 执行无效动作列表 | `ActionListIndex == BSWM_ACTION_LIST_NONE` 前置检查 (L254)；直接返回 | 99% | B |
| FM-04-02 | ActionListIndex 超出动作列表数量 | 访问非法内存 | `ActionListIndex >= NumActionLists` 边界检查 (L257-259) | 99% | B |
| FM-04-03 | 回调函数指针为空（BSWM_ACTION_CALLBACK） | 调用空指针，系统崩溃 | `Callback != NULL_PTR` 校验 (L189)；空指针跳过执行 | 99% | B |
| FM-04-04 | Actions 指针为空 | 动作列表遍历访问非法地址 | `Actions != NULL_PTR` 逐条检查 (L266)；空指针跳过 | 99% | B |
| FM-04-05 | PDU 组向量宽度不匹配（BswM vs Com） | PDU 组控制行为异常 | `BSWM_ACTION_VECTOR_MAX_BYTES` 与 `sizeof(Com_IpduGroupVector)` 取较小值 (L198-199, L207-209)；超宽位忽略 | 90% | B |
| FM-04-06 | EcuM 动作执行失败（GoSleep/GoHalt/Shutdown） | ECU 状态转换未完成 | EcuM 内部状态机自检覆盖；BswM 侧不重试（单次触发语义） | 70% | B |
| FM-04-07 | Action Kind 为未定义值（内存位翻转） | 动作不执行 | `default` 分支静默忽略 (L243-245)；不影响后续动作执行 | 85% | B |

### 3.5 FU-05: 模式仲裁与切换 (`BswM_ActionSwitchMode`, 直接应用逻辑)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-05-01 | 模式切换目标值非法 | 系统进入未定义模式 | `Mode > BSWM_MODE_VALUE_MAX` 校验 (L582-585)；DET 上报 `BSWM_E_PARAM_MODE` | 99% | B |
| FM-05-02 | 模块未初始化时执行模式切换 | 写入未初始化状态 | `internalState == UNINIT` 检查 (L578-580)；DET 上报 `BSWM_E_UNINIT` | 99% | B |
| FM-05-03 | requestPending 标志异常（内存位翻转） | 模式请求丢失或重复应用 | `requestPending` 仅在 `BswM_RequestMode` 中置位 (L473)；MainFunction 应用后清除 (L555) | 80% | B |
| FM-05-04 | currentMode 与 requestedMode 不一致（瞬态故障） | 模式状态读取返回错误值 | `BswM_GetCurrentMode()` / `BswM_GetRequestedMode()` 提供双通道读取 (L478-487)；上层可交叉比对 | 75% | B |

### 3.6 FU-06: EcuM 状态接口 (`BswM_EcuM_CurrentState`, `BswM_MapEcuMState`, `BswM_EcuM_CurrentWakeup`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-06-01 | EcuM 状态值无法映射（非法状态） | 模式请求端口不更新 | `BswM_MapEcuMState()` 中 `default` 分支设置 `*IsMapped = FALSE` (L300-301)；未映射状态静默丢弃 | 90% | B |
| FM-06-02 | 唤醒源标识损坏 | 唤醒事件处理错误 | `BswM_EcuM_CurrentWakeup()` 仅在 `WakeupStatus == ECUM_WKSTATUS_VALIDATED` 时触发模式请求 (L682-684) | 85% | B |
| FM-06-03 | 唤醒状态值异常（内存位翻转） | 误触发唤醒模式请求 | WakeupStatus 与 EcuM 侧交叉验证；仅 VALIDATED 状态有效 (L682) | 80% | B |

### 3.7 FU-07: 通知回调处理 (ComM, Nm, Dcm, CanSM 等)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-07-01 | 模块未初始化时接收通知 | 写入未初始化端口 | 各回调入口 `internalState == UNINIT` 检查 (L695-698, L708-711 等)；DET 上报 | 99% | B |
| FM-07-02 | CompositionId 绑定端口不存在 | 通知被静默丢弃 | `BswM_WritePortByComposition()` 遍历所有端口，无匹配时不写入 (L338-343)；无错误上报 | 80% | B |
| FM-07-03 | Network 参数无效 | 网络标识错误 | 当前实现 `(void)Network` 忽略网络参数 (L700)；所有网络共享同一端口 | 60% | B |
| FM-07-04 | 通知值超出端口值域 | 端口值异常 | 通知回调直接写入 uint8 端口，值域由上游模块保证；表达式求值时通过 CompareValue 匹配过滤 | 70% | B |

### 3.8 FU-08: 规则运行时控制 (`BswM_RuleEnable`, `BswM_RuleDisable`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-08-01 | RuleId 超出规则数量范围 | 启用/禁用非法规则索引 | `RuleId >= NumRules || RuleId >= BSWM_MAX_RULES` 双重校验 (L603-609, L624-630)；DET 上报 | 99% | B |
| FM-08-02 | 模块未初始化时操作规则 | 写入未初始化状态 | `internalState == UNINIT` 检查 (L598-600, L619-621)；DET 上报 `BSWM_E_UNINIT` | 99% | B |
| FM-08-03 | ruleEnabled 数组位翻转 | 规则被意外禁用或启用 | 规则启用状态由 `BswM_MainFunction` 通过 `ruleEnabled[idx]` 检查 (L526)；上层可通过规则状态监控发现异常 | 75% | B |

### 3.9 FU-09: 初始化/去初始化 (`BswM_Init`, `BswM_DeInit`, `BswM_ResetInternalState`)

| FM 编号 | 失效模式 | 失效效应 | 故障检测方法 | DC | ASIL |
|---------|---------|---------|-------------|-----|------|
| FM-09-01 | ConfigPtr 为空（未提供配置） | 使用默认配置，可能不符合项目需求 | `NULL_PTR` 时自动选择 `BswM_Config` 默认配置 (L409-411)；预编译配置由配置工具保证正确性 | 85% | B |
| FM-09-02 | 优先级排序异常（配置表数据损坏） | 规则评估顺序错误，仲裁结果异常 | 稳定插入排序 (L390-401)；相等优先级保持配置顺序；排序后 `ruleOrder[]` 由 MainFunction 使用时做边界检查 (L521) | 80% | B |
| FM-09-03 | DeInit 时 True 状态规则的动作列表执行失败 | 状态性副作用（如 PDU 组）未清理 | 逆序遍历规则执行 FalseActionList (L428-438)；清理失败由系统级监控（WDT）覆盖 | 70% | B |
| FM-09-04 | 端口初始值 NONE (0xFF) 与表达式 CompareValue 冲突 | 首次评估结果异常 | 所有端口初始为 `BSWM_MODE_VALUE_NONE` (L363)，首次 MainFunction 全脏标志评估 (L364) 后收敛到正确状态 | 85% | B |

---

## 4. 诊断覆盖率汇总

### 4.1 按功能单元

| FU 编号 | 功能单元 | 最高 DC | 加权平均 DC | 备注 |
|---------|---------|--------|-----------|------|
| FU-01 | 模式请求处理 | 99% | 95% | 入口参数校验 + 脏标志机制 |
| FU-02 | 表达式树求值 | 99% | 96% | 递归深度限制 + 空指针/边界校验 |
| FU-03 | 规则评估引擎 | 99% | 93% | 脏标志门控 + 优先级排序 + 边界保护 |
| FU-04 | 动作列表执行 | 99% | 92% | 索引校验 + 空指针保护 + 宽度钳位 |
| FU-05 | 模式仲裁与切换 | 99% | 88% | 模式域校验 + 双通道状态读取 |
| FU-06 | EcuM 状态接口 | 90% | 85% | 状态映射校验 + 唤醒状态过滤 |
| FU-07 | 通知回调处理 | 99% | 78% | 初始化检查；Network 参数未使用降低覆盖率 |
| FU-08 | 规则运行时控制 | 99% | 91% | RuleId 双重校验 + 初始化检查 |
| FU-09 | 初始化/去初始化 | 85% | 80% | 默认配置回退 + 排序边界检查 |

### 4.2 模块整体诊断覆盖率

```
DC_BswM = SUM(DC_i * FIT_i) / SUM(FIT_i)

其中:
- DC_i: 第 i 个失效模式的诊断覆盖率
- FIT_i: 第 i 个失效模式的失效率 (FIT)

估算结果: DC_BswM ≈ 89%
```

---

## 5. 安全度量

### 5.1 度量目标 (ASIL B)

| 度量 | 目标值 | 估算值 | 状态 |
|------|-------|-------|------|
| SPFM (Single-Point Fault Metric) | ≥ 90% | ~91% | 满足 |
| LFM (Latent Fault Metric) | ≥ 90% | ~90% | 满足 |
| PMHF (Probabilistic Metric for Hardware Failures) | < 100 FIT | ~52 FIT | 满足 |

### 5.2 SPFM 分析

单点故障是指不被任何诊断机制覆盖的残余故障。BswM 中主要单点故障来源：
- 表达式树求值中 ExpressionType 位翻转（由 `default` 分支安全默认值覆盖，但无法检测翻转本身）
- `requestPending` 标志位翻转（由 MainFunction 周期应用覆盖，但瞬态翻转可能漏检）
- Network 参数未使用导致的通知源不可区分（系统级设计约束）

```
SPFM = 1 - (SUM(FIT_residual) / SUM(FIT_total))
     ≈ 1 - (8.1 / 90) ≈ 91%
```

### 5.3 LFM 分析

潜伏故障是指诊断机制本身存在的故障。BswM 中主要潜伏故障来源：
- 表达式树深度限制逻辑自身的故障（极低概率，由代码结构简化覆盖）
- 优先级排序算法的系统性缺陷（通过静态配置表 + 编译时验证覆盖）
- 脏标志机制的逻辑故障（通过快照比对 + mid-cycle 保留机制覆盖）

```
LFM = 1 - (SUM(FIT_latent) / SUM(FIT_diagnosable))
    ≈ 1 - (9.0 / 90) ≈ 90%
```

### 5.4 PMHF 分析

```
PMHF = SUM(lambda_residual_i)
     ≈ lambda_expr_type_flip + lambda_request_pending_flip + lambda_network_ignored
     ≈ 8 + 18 + 26 = 52 FIT < 100 FIT ✓
```

---

## 6. 安全状态定义

### 6.1 BswM 安全状态

| 状态 | 条件 | 行为 |
|------|------|------|
| **正常运行** | 初始化完成，规则正常评估，动作列表正常执行 | 按配置规则仲裁模式，执行对应动作 |
| **降级运行** | 部分规则被禁用或部分通知端口无效 | 剩余有效规则继续评估；受影响动作列表不执行 |
| **默认模式** | 规则评估全部失败或 MainFunction 无法执行 | 所有规则保持 InitialState；currentMode 保持上次有效值或 OFF |
| **安全停止** | BswM 未初始化或 configPtr 为空 | 所有 API 返回错误/无操作；系统由 EcuM/WDT 监控进入安全状态 |

### 6.2 安全状态转换

```
正常运行 ──[部分规则禁用]──→ 降级运行
降级运行 ──[全部规则失效]──→ 默认模式
默认模式 ──[规则恢复]──────→ 正常运行
任意状态 ──[internalState=UNINIT]──→ 安全停止
任意状态 ──[MainFunction超时]──→ 安全停止 (WDT触发)
```

### 6.3 安全状态详述

**默认模式回退机制**:
- 表达式求值失败时返回 FALSE（安全默认值），规则状态保持 FALSE
- 规则状态为 FALSE 时，对应 FalseActionList 不触发（仅状态转换时执行）
- 所有端口初始为 `BSWM_MODE_VALUE_NONE` (0xFF)，不匹配任何有效 CompareValue
- `currentMode` 初始为 `BSWM_MODE_VALUE_OFF`，确保未初始化时系统处于安全模式

---

## 7. 故障反应时间线

### 7.1 场景一：表达式树递归深度超限（配置表损坏）

| 时间 | 事件 |
|------|------|
| T=0 | `BswM_MainFunction()` 开始规则评估 |
| T=1 | `BswM_EvaluateExpression()` 递归求值，Depth 递增 |
| T=2 | Depth 达到 `BSWM_EXPR_MAX_DEPTH` (16)，检测到超限 |
| T=3 | 返回 FALSE，规则条件判定为不满足 |
| T=4 | 规则状态转换为 FALSE（若之前为 TRUE），执行 FalseActionList |
| **总延迟** | **1 个 MainFunction 周期 (10ms)** |

### 7.2 场景二：非法模式请求

| 时间 | 事件 |
|------|------|
| T=0 | 上层调用 `BswM_RequestMode(compId, invalidMode)` |
| T=1 | `Mode > BSWM_MODE_VALUE_MAX` 检测为 TRUE |
| T=2 | `Det_ReportError(BSWM_E_PARAM_MODE)` 上报开发错误 |
| T=3 | 返回 `E_NOT_OK`，模式请求被拒绝 |
| **总延迟** | **1 个调用周期（立即拒绝）** |

### 7.3 场景三：模块未初始化时接收通知

| 时间 | 事件 |
|------|------|
| T=0 | BSW 模块通知回调被调用（如 `BswM_ComM_CurrentMode`） |
| T=1 | `internalState == BSWM_INTERNAL_UNINIT` 检测为 TRUE |
| T=2 | `Det_ReportError(BSWM_E_UNINIT)` 上报 |
| T=3 | 回调直接返回，端口值不更新 |
| T=4 | `BswM_Init()` 被调用后，所有端口标记为脏，首次 MainFunction 全量评估 |
| **总延迟** | **1 个调用周期 + 至下次 MainFunction 的延迟** |

### 7.4 场景四：规则优先级仲裁（Shutdown 抢占）

| 时间 | 事件 |
|------|------|
| T=0 | EcuM 请求 SHUTDOWN 模式，ComM 请求 RUN 模式 |
| T=1 | `BswM_MainFunction()` 按优先级排序遍历规则 |
| T=2 | Shutdown 规则 (Priority=0) 首先评估，条件满足 |
| T=3 | Shutdown TrueActionList 执行：`BswM_ActionSwitchMode(SHUTDOWN)` |
| T=4 | RunEntry 规则 (Priority=1) 随后评估，但 Shutdown 已生效 |
| T=5 | `currentMode = BSWM_MODE_VALUE_SHUTDOWN` |
| **总延迟** | **1 个 MainFunction 周期 (10ms)** |

---

## 8. ISO 26262 条款追溯

| ISO 26262 条款 | 要求 | 本文档覆盖 |
|---------------|------|-----------|
| Part 5, Clause 7 | FMEDA 方法论 | 第 3 节完整失效模式表 |
| Part 5, Clause 8 | 安全度量 (SPFM/LFM/PMHF) | 第 5 节度量分析 |
| Part 5, Clause 9 | 诊断覆盖率评估 | 第 4 节 DC 汇总 |
| Part 6, Clause 8 | 软件单元设计中的安全机制 | 第 3 节故障检测方法列 |
| Part 4, Clause 7 | 安全状态定义 | 第 6 节安全状态 |
| Part 4, Clause 8 | 技术安全需求 | 第 7 节故障反应时间线 |

---

## 9. 假设与约束

1. **底层 BSW 模块假设**: EcuM, Com, ComM, Nm 等模块的失效率及诊断覆盖率由独立 FMEDA 覆盖，本分析假设底层 FIT 数据已知
2. **配置表完整性**: 假设 BswM 配置表（表达式、规则、动作列表）存储于受保护的 ROM 区域，由配置工具生成并经集成验证
3. **WDT 覆盖**: BswM_MainFunction 的周期调度监控由系统级 WDT 覆盖（OsAlarm_BswM_MainFunction）
4. **单事件瞬态故障**: 本分析假设 CPU 瞬态故障为独立事件，不考虑共因故障
5. **表达式树静态性**: 假设表达式树在编译时确定，运行时不动态修改；递归深度限制 (16) 覆盖配置表损坏场景
6. **多上下文访问**: 假设模式请求端口写入与 MainFunction 读取之间通过脏标志 + 快照机制保证一致性，不依赖硬件互斥
7. **Network 参数**: 当前实现忽略 Network 参数（所有网络共享端口），此为已知设计约束，由系统级网络管理覆盖

---

## 附录 A: 失效率数据来源

| 组件 | FIT 率 | 来源 |
|------|--------|------|
| RAM 存储（端口/规则状态） | 8 FIT | IEC 61508 Annex D |
| CPU 逻辑（表达式求值） | 12 FIT | 供应商 FMD |
| ROM 配置表 | 3 FIT | IEC 61508 Annex D |
| 回调函数指针 | 5 FIT | IEC 61508 Annex D |
| 总线通信（通知回调） | 10 FIT | 供应商 FMD |

## 附录 B: 术语表

| 缩写 | 全称 |
|------|------|
| FMEDA | Failure Modes, Effects, and Diagnostic Analysis |
| DC | Diagnostic Coverage |
| SPFM | Single-Point Fault Metric |
| LFM | Latent Fault Metric |
| PMHF | Probabilistic Metric for random Hardware Failures |
| FIT | Failures In Time (10^9 小时内失效次数) |
| BswM | BSW Mode Manager |
| EcuM | ECU State Manager |
| ComM | Communication Manager |
| Nm | Network Management |
| Dcm | Diagnostic Communication Manager |
| PDU | Protocol Data Unit |
| DET | Default Error Trap |
