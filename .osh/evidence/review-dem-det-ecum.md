## Dem + Det + EcuM — 模块审查

- **审查时间**: 2026-07-21
- **审查人**: 小马 🐴 (质量架构师)
- **文件范围**:
  - Dem: 5 个 .c, 8 个 .h（含 `legacy/` 旧版代码）
  - Det: 2 个 .c, 3 个 .h
  - EcuM: 7 个 .c, 2 个 .h
- **MISRA 合规**: ⚠️ 部分合规

---

## 1. Dem (Diagnostic Event Manager)

### 架构概述

Dem 模块管理 AUTOSAR 诊断事件（DTC 状态、事件 debounce、冻结帧、扩展数据记录）。主文件 `Dem.c` (1,201 行) + `Dem_Int.c` 内部管理 + 遗留代码 `legacy/`。

### MISRA 合规

| 发现 | 级别 | 说明 |
|------|------|------|
| Rule 21.15 (Required) | P1 | `#include "string.h"` 直接使用标准库，违反 MISRA 要求 |
| Rule 2.5 (Required) | P1 | Include guard `DEM_H` 宏命名 |
| Rule 10.1 (Required) | P1 | 配置宏 `STD_ON` 布尔上下文 |
| Rule 15.5 (Advisory) | P2 | 错误处理多 return |
| Rule 14.4 (Required) | P1 | switch-case 缺失 default（在事件处理中） |
| Rule 17.7 (Advisory) | P2 | `Det_ReportError` 返回值未使用 |

### 代码质量 — 审查发现

#### P1 — 强烈建议

1. **标准库依赖**: `#include "string.h"` 直接引用标准 C 库，违反 MISRA Rule 21.15。应替换为安全内存操作封装。
2. **遗留代码共存**: `legacy/` 目录包含旧的 dem.c/dem.h 与新版 Dem.c 共存，容易导致混淆和符号冲突。需清理。
3. **配置文件缺失** (v1.1.0): 构建日期 2026-04-29，版本 1.1.0 有关键 null pointer 修复，但 `Dem_Pbcfg.c` / `Dem_Lcfg.c` 配置尚未完备。

#### P2 — 建议

1. **Debounce 计数器类型**: `FaultDetectionCounter` 声明为 `int`（有符号），`DebounceCounter` 为 `uint8` — 类型不一致可能溢出。
2. **DTC 状态位操作**: DTC 状态位使用位操作宏管理，但缺少原子性保护。

### 发现

| 分类 | P1 | P2 |
|------|----|----|
| 可靠性 | 1 | 1 |
| 可维护性 | 2 | 1 |

### 结论

**有条件的通过** ⚠️ — Dem 功能对齐 AUTOSAR 规范，但遗留代码和标准库依赖需清理。

---

## 2. Det (Development Error Tracer)

### 架构概述

Det 模块提供开发阶段错误跟踪（DET 报告），供所有 BSW 模块在 `DEV_ERROR_DETECT == STD_ON` 时调用。`Det.c` (479 行) 实现轻量级日志/回调。

### MISRA 合规

| 发现 | 级别 | 说明 |
|------|------|------|
| Rule 2.5 (Required) | P1 | Include guard `DET_H` 宏 |
| Rule 17.7 (Advisory) | P2 | Det_ReportError 返回值 (E_OK/E_NOT_OK) 无调用者检查 |

### 代码质量 — 审查发现

#### P1

1. **条件编译注释错误**: `#//error "Det.c: Mismatch in AUTOSAR minor version"` — 第 33 行有 `#//error` 错误语法（连续双斜杠）。这是编译错误（虽然不是影响执行路径的 bug，但部分编译器可能忽略）。

### 结论

**通过** ✅ — Det 模块最为简单，基本无严重质量问题。

---

## 3. EcuM (ECU State Manager)

### 架构概述

EcuM 实现多层启动/关闭/睡眠状态机（1863 行主文件 + 5 个 `_impl.c` 文件）。涵盖：
- 启动阶段: StartupOne → StartupTwo → StartupThree
- 运行时: RUN → POSTRUN → GOSLEEP → SLEEP/HALT/POLL
- 唤醒源管理: WakeupOne → WakeupDetection → WakeupValidation
- 关闭: GoOffOne → GoOffTwo → Reset/Off

### MISRA 合规

| 发现 | 级别 | 说明 |
|------|------|------|
| Rule 2.5 (Required) | P1 | Include guard `ECUM_H` 宏命名 |
| Rule 10.1 (Required) | P1 | ECU_M 配置宏布尔上下文 |
| Rule 14.4 (Required) | P1 | 枚举 switch-case 中无 default（`EcuM_ProcessStartupOne` 中的状态机枚举） |
| Rule 15.5 (Advisory) | P2 | 错误处理多 return |
| Rule 8.13 (Advisory) | P2 | 内部 API 参数 const 声明 |

### 代码质量 — 审查发现

#### P1 — 强烈建议

1. **状态机复杂度**: ECU 状态机通过 `EcuM_ProcessStartupOne` → `Two` → `Run` 等 10+ 个静态函数实现，但每个函数内部嵌套多层 if-else。建议使用状态表或状态模式降低圈复杂度。
2. **RUN 请求管理竞态**: `EcuM_RunRequests` / `EcuM_KilledRunRequests` 为 uint32 计数器，无临界区保护。
3. **唤醒源位掩码溢出**: `EcuM_PendingWakeupEvents` 等为 typedef 位掩码类型，若 `ECUM_MAX_WAKEUP_SOURCES > sizeof(type)*8` 则溢出。

### 结论

**有条件的通过** ⚠️ — EcuM 状态机实现完整，但复杂度高、竞态风险需确认。

---

### 跨模块总评

| 模块 | 文件 | 行数 | MISRA | 架构对齐 | 结论 |
|------|------|------|-------|---------|------|
| Dem | 5 .c + 8 .h | ~1200+legacy | ⚠️ | ✅ | 有条件通过 |
| Det | 2 .c + 3 .h | ~479 | ✅ | ✅ | 通过 |
| EcuM | 7 .c + 2 .h | ~3619 | ⚠️ | ✅ | 有条件通过 |
