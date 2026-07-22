## RamSafety — 模块审查

- **审查时间**: 2026-07-21
- **审查人**: 小马 🐴 (质量架构师)
- **文件范围**: 2 个 .c (`RamSafety.c`, `RamSafety_Cfg.c`), 2 个 .h (`RamSafety.h`, `RamSafety_Cfg.h`)
- **MISRA 合规**: ⚠️ 部分合规（需补充偏差许可）

---

### 架构概述

RamSafety 模块为 yuleASR 提供 **ASIL-D 级** RAM 安全检查能力，实现三项核心内存测试算法：

1. **March C- 算法**: 覆盖 stuck-at、coupling、bridging 等 RAM 失效模式。6 阶段走步 (↑w0 / ↑r0w1r1 / ↑r1w0r0 / ↓r0w1r1 / ↓r1w0r0 / ↓r0)
2. **Walking (走步) 模式**: 全地址线/数据线测试，检测地址线短路和信号耦合
3. **地址线测试 (Address Line Test)**: 检测地址解码器故障
4. **数据线测试 (Data Line Test)**: 检测数据总线故障

启动时执行完整测试 (StartupTest)，运行时周期执行分散检查 (MainFunction 分片)。

### MISRA 合规 — 主要发现

| 发现 | 级别 | 说明 |
|------|------|------|
| **Rule 11.4 (Required)** | **P0** | `(volatile uint8*)(addr + i)` — 整数到指针转换，RamSafety 核心操作 |
| Rule 2.5 (Required) | P1 | Include guard `RAMSAFETY_H` 宏命名冲突 |
| Rule 10.1 (Required) | P1 | 配置宏 `#if (RAMSAFETY_DEV_ERROR_DETECT == STD_ON)` 布尔上下文 |
| Rule 14.4 (Required) | P1 | switch-case 无 default（枚举类型状态机） |
| Rule 15.5 (Advisory) | P2 | 错误处理多 return 路径 |
| Rule 17.7 (Advisory) | P2 | `(void)Det_ReportError(...)` 未使用返回值 |
| Rule 12.1 (Advisory) | P2 | `for (i = size; i > 0U; i--)` 等循环中操作符优先级隐式依赖 |

### 安全关键分析 (ASIL-D)

#### P0 — 必须评估

1. **`volatile` 使用一致**: March C- 测试中使用 `volatile uint8*` 确保编译器不优化掉读写操作。✅ 正确
2. **中断管理**: `RamSafety_Init` 中调用 `Mcal_DisableAllInterrupts()` 保护关键段。✅ 正确
3. **安全魔数**: `RAMSAFETY_SAFETY_MAGIC_INIT` / `RAMSAFETY_SAFETY_MAGIC_ACTIVE` 用于状态完整性校验。✅ 良好实践
4. **错误回调机制**: `RamSafety_ErrorCallbackType` 允许注册错误回调。✅ 符合 ASIL-D 故障检测要求

#### P1 — 安全相关

1. **中断恢复缺失**: `RamSafety_Init` 中 `Mcal_DisableAllInterrupts()` 后，成功路径有隐含重入保护，但失败路径可能不恢复中断。
2. **March C- 阶段串行依赖**: 阶段 2 必须依赖阶段 1 写入的值。当前实现为严格的顺序执行，但无看门狗定时器保护，测试可能卡住。
3. **运行时测试侵入**: `RamSafety_MainFunction` 在运行时执行内存读写测试，若与被测应用的内存区域冲突可能导致数据损坏 — 需确认 Region 配置不会与活跃变量冲突。
4. **ErrorCb 回调上下文**: 在 for 循环中调用回调函数，若回调中尝试重入 RamSafety 可能导致递归死锁。

#### P2 — 建议改进

1. **`STATIC` 联合 `volatile`**: `RamSafety_State` 声明为 `STATIC volatile`，但 `STATIC` 宏可能非标准 `static`。

### 测试覆盖

| 维度 | 状态 | 说明 |
|------|------|------|
| March C- 算法 | ⚠️ 部分 | 6 阶段实现完整，但边界条件和大 RAM 区域测试未确认 |
| Walk Pattern | ✅ 有框架 | Walking mode 实现 |
| 地址线测试 | ✅ 有框架 | Address line test 已实现 |
| 数据线测试 | ✅ 有框架 | Data line test 已实现 |
| 运行时测试 | ⚠️ 部分 | MainFunction 分片检查框架完整，但分片边界测试不充分 |
| 故障注入测试 | ❌ 无 | 无故障注入验证测试准确性 |

### 架构对齐

| 要求 | 状态 | 说明 |
|------|------|------|
| AUTOSAR 安全需求 | ✅ 对齐 | 安全机制嵌入 AUTOSAR 框架 |
| Startup Hook 集成 | ✅ 完整 | `RamSafety_RunStartupTest` 用于 early boot 阶段 |
| MainFunction 调度 | ✅ 完整 | `RamSafety_MainFunction` 每周期处理 |
| Error 上报链路 | ✅ 完整 | 错误回调 + `Det` 报告 + 统计信息 |
| 平台抽象层 | ✅ 良好 | 使用 `Platform_RamSafety_Init` 等平台 API |
| MemMap 正确性 | ✅ 完整 | 所有内存段正确使用 `RamSafety_MemMap.h` |

### 代码量统计

| 文件 | 行数 | 说明 |
|------|------|------|
| RamSafety.h | 358 | 宏定义、类型、API 声明 |
| RamSafety.c | 1,092 | 核心算法实现 |
| RamSafety_Cfg.c | 111 | 配置表 |
| RamSafety_Cfg.h | (分离) | 编译期配置 |
| **合计** | **~1,561** | 可管理的模块大小 |

### 发现汇总

| 分类 | P0 | P1 | P2 |
|------|----|----|----|
| 安全 (ASIL-D) | 0 | 4 | 1 |
| 可靠性 | 0 | 0 | 0 |
| 可维护性 | 0 | 0 | 1 |
| **合计** | **0** | **4** | **2** |

### 结论

**有条件的通过** ✅ — RamSafety 是 yuleASR 中最重要的安全关键模块之一，March C- 和走步测试算法实现严谨，架构对齐 ASIL-D 要求。

**前提条件**:
1. **中断恢复验证**: 确认 `Mcal_DisableAllInterrupts()` 在初始化失败路径中正确恢复 (P1)
2. **运行时 Region 隔离**: 确认运行时测试不会干扰活跃应用内存 (P1)
3. **回调重入保护**: 增加 ErrorCallback 调用时的重入防护 (P1)
4. **看门狗保护**: March C- 长耗时阶段应考虑看门狗喂狗 (P1)
5. DP-AUTOSAR-007 (Rule 11.4) 偏差必须正式登记 (P0 级架构决策)
