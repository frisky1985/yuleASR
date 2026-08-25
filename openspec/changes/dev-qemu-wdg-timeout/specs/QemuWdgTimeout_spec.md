# QemuWdgTimeout (QEMU WDG 超时复位验证) Module Specification

> **Module:** QemuWdgTimeout (QEMU WDG 超时复位验证)  
> **Layer:** Test Infrastructure / Safety  
> **Standard:** AUTOSAR Classic Platform 4.4.0 / AUTOSAR SWS_WdgM  
> **Platform:** QEMU mps2-an521 (Cortex-M33)  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

QemuWdgTimeout 在 QEMU mps2-an521 上验证 yuleASR Wdg 驱动（`src/bsw/mcal/wdg/`）与 WdgM 管理层（`src/bsw/services/wdgm/`）的完整超时复位链路。在 yuleASR QEMU 验证环境中 WDG 从未被真实触发过超时复位。本模块验证两条关键路径：(1) WdgM 活性监控正常时 WDG 被持续喂狗不超时；(2) 任务停止喂狗时 WDG 超时触发系统复位，QEMU 可观测到复位行为。由于 QEMU mps2-an521 CMSDK Watchdog 与 S32K312 SWT（Software Watchdog Timer）接口不同，通过 WdgIf 抽象层提供 QEMU 专用 Wdg stub 适配 CMSDK。

### Key Responsibilities
- 验证正常路径：`WdgM_MainFunction` 周期调用 `Wdg_SetTriggerCondition` 喂狗，系统持续运行不超时
- 验证超时路径：刻意停止喂狗，WDG 超时触发 `POWER_RESET`，QEMU 输出复位标记
- 验证 WdgM 活性监控：被监控任务停止上报 `WdgM_CheckpointReached` 后触发间接复位
- 验证 WdgM 全局模式切换：`WdgM_SetMode(WDGM_GLOBAL_MODE_STOPPED)` 后 WDG 进入 OFF 模式
- 输出 `WDG_TIMEOUT_PASS` 标记供 CI 自动判定

---

## 2. API List

### 2.1 WdgM APIs Under Test

| API | Description |
|-----|-------------|
| `void WdgM_Init(const WdgM_ConfigType *Config)` | WdgM 初始化，配置监控实体 |
| `void WdgM_MainFunction(void)` | 周期主函数（喂狗 + 活性检查） |
| `void WdgM_CheckpointReached(uint8 seid, uint8 cpid)` | 被监控实体上报 Checkpoint 到达 |
| `void WdgM_SetMode(WdgM_GlobalModeType mode)` | 全局模式切换（NORMAL / STOPPED） |
| `void WdgM_GetGlobalStatus(WdgM_GlobalStatusType *status)` | 获取全局状态 |

### 2.2 Wdg QEMU Stub APIs

| API | Called By | Description |
|-----|-----------|-------------|
| `void Wdg_Init(const Wdg_ConfigType *Config)` | 验证入口 | 配置 CMSDK Watchdog，使能中断+复位 |
| `void Wdg_SetTriggerCondition(uint16 timeout)` | WdgM | 重载计数器（喂狗），timeout 单位 ms |
| `void Wdg_SetMode(WdgIf_ModeType mode)` | WdgM | WDGIF_FAST_MODE / WDGIF_OFF_MODE |
| `void WATCHDOG_IRQHandler(void)` | NVIC 向量表 | 超时 ISR：输出 UART 标记 → 触发 SystemReset |

### 2.3 Verification Result APIs

| API | Called By | Description |
|-----|-----------|-------------|
| `void Qemu_ReportPass(void)` | 验证入口 | 输出 `WDG_TIMEOUT_PASS` 并调用 semihosting `SYS_EXIT(0)` |
| `void Qemu_ReportFail(const char *reason)` | 验证入口 | 输出失败原因并调用 semihosting `SYS_EXIT(1)` |

---

## 3. Data Types

### 3.1 WdgIf Mode Enum

```c
typedef enum {
    WDGIF_OFF_MODE   = 0x00,  /* WDG 关闭 */
    WDGIF_FAST_MODE  = 0x01   /* WDG 开启（快速模式） */
} WdgIf_ModeType;
```

### 3.2 WdgM Global Mode / Status Enums

```c
typedef enum {
    WDGM_GLOBAL_MODE_STOPPED  = 0x00,  /* WdgM 停止，WDG 进入 OFF 模式 */
    WDGM_GLOBAL_MODE_NORMAL   = 0x01   /* WdgM 正常运行 */
} WdgM_GlobalModeType;

typedef enum {
    WDGM_GLOBAL_STATUS_OK       = 0x00,  /* 全局状态正常 */
    WDGM_GLOBAL_STATUS_EXPIRED  = 0x01   /* 全局状态超时（活性监控失效） */
} WdgM_GlobalStatusType;
```

### 3.3 CMSDK Watchdog Register Defines

```c
/* CMSDK Watchdog base address (mps2-an521) */
#define WDOG_BASE           0x40008000u
/* Register offsets */
#define WDOGLOAD_OFFSET     0x00u   /* 重装载值（写入即喂狗） */
#define WDOGVALUE_OFFSET    0x04u   /* 当前计数值（只读） */
#define WDOGCONTROL_OFFSET  0x08u   /* 控制寄存器（bit0=INTEN, bit1=RESEN） */
#define WDOGINTCLR_OFFSET   0x0Cu   /* 中断清除 */
#define WDOGRIS_OFFSET      0x10u   /* 原始中断状态 */
/* Control bits */
#define WDG_CTRL_INTEN      (1u << 0)
#define WDG_CTRL_RESEN      (1u << 1)
```

### 3.4 Verification Constants

```c
#define WDG_FEED_PERIOD_MS      10u     /* 喂狗周期（10ms） */
#define WDG_NORMAL_RUN_CYCLES   100u    /* 正常运行周期数（100 × 10ms = 1s） */
#define WDG_TIMEOUT_MS          100u    /* WDG 超时阈值（100ms） */
#define WDG_DEADLINE_MS         1000u   /* WdgM 活性监控 deadline（放宽容差） */
```

### 3.5 Marker Constants

```c
#define WDG_TIMEOUT_RESET_MARKER  "WDG_TIMEOUT_RESET"  /* 超时复位标记 */
```

### 3.6 WdgM Configuration (Test)

```c
/* 简化配置：1 个监控实体，deadline = 1000ms */
static const WdgM_SupervisedEntityConfigType WdgM_SeConfig[] = {
    {
        .SupervisedEntityId    = WDGM_SE_MAIN_TASK,
        .AliveSupervisedRef    = &WdgM_AliveSupervision,
        .DeadlineSupervisedRef = NULL,
        .LocalStatusRef       = &WdgM_LocalStatus,
    },
};
static const WdgM_AliveSupervisionType WdgM_AliveSupervision = {
    .SupervisionReferenceCycleCounter = 1U,    /* 每个主函数周期检查一次 */
    .MinMargin                        = 1U,    /* 最少 1 次 Checkpoint */
    .MaxMargin                        = 3U,    /* 最多 3 次 Checkpoint */
};
```

---

## 4. Error Handling

本模块为测试基础设施，不做 DET 错误报告。WDG 超时导致 QEMU 进程退出，无法输出 PASS 标记，故在超时 ISR 中先输出 `WDG_TIMEOUT_RESET` 标记再执行复位（`WDG_TEST_MODE=1` 允许在超时前捕获标记）；或使用 semihosting 在复位前同步输出。验证失败时调用 `Qemu_ReportFail` 直接终止。

| Error Code | Value | Description |
|------------|-------|-------------|
| N/A | — | 模块不使用 DET |

---

## 5. Configuration Parameters

### Pre-Compile Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| `WDG_FEED_PERIOD_MS` | 10 | 喂狗周期（毫秒） |
| `WDG_NORMAL_RUN_CYCLES` | 100 | 正常运行周期数（100 × 10ms = 1s） |
| `WDG_TIMEOUT_MS` | 100 | WDG 超时阈值（毫秒） |
| `WDG_DEADLINE_MS` | 1000 | WdgM 活性监控 deadline（毫秒，放宽容差避免 QEMU 抖动误判） |
| `WDG_TEST_MODE` | 1 | 允许在超时前捕获标记 |
| `QEMU_TARGET` | 1 | QEMU 目标平台标识 |
| `QEMU_TIMEOUT` | 30 | CI 脚本超时秒数（环境变量） |
| `QEMU_MACHINE` | mps2-an521 | QEMU 机器类型 |
| `QEMU_CPU` | cortex-m33 | QEMU CPU 型号 |

### Build Configuration

```cmake
add_executable(qemu_wdg_timeout
    p3d_wdg_timeout/main_wdg_timeout.c
    p3d_wdg_timeout/wdg_qemu_stub.c
)
target_link_libraries(qemu_wdg_timeout WdgM WdgIf Os Det)
target_compile_definitions(qemu_wdg_timeout PRIVATE
    QEMU_TARGET=1
    WDG_TEST_MODE=1            # 允许在超时前捕获标记
    WDGM_TEST_DEADLINE_MS=1000 # 放宽 deadline，避免 QEMU 抖动误判
)
```

---

## 6. Scenarios

### Scenario S10.1: NormalFeedingNoTimeout
**Description:** 正常喂狗路径 — 系统运行 1 秒（100 个 10ms 周期），WDG 未超时
**Flow:**
1. `WdgM_Init`，配置 WDG 超时 2 秒
2. 循环 100 次（每次 10ms）调用 `WdgM_CheckpointReached(WDGM_SE_MAIN_TASK, WDGM_CP_START)` 与 `WdgM_MainFunction()`
3. 检查超时计数与全局状态
**Expected Result:** `wdg_timeout_count == 0`（`WATCHDOG_IRQHandler` 未被调用）；`WdgM_GetGlobalStatus() == WDGM_GLOBAL_STATUS_OK`（验收 S10.1）

### Scenario S10.2: TimeoutReset
**Description:** 超时复位路径 — 停止喂狗后 WDG 超时触发系统复位
**Flow:**
1. `WdgM_Init`，配置 WDG 超时 100ms
2. 停止调用 `WdgM_MainFunction`，等待 200ms
3. CMSDK Watchdog 超时，`WATCHDOG_IRQHandler` 执行
**Expected Result:** UART 输出 `WDG_TIMEOUT_RESET\n` 标记；系统复位；CI grep UART 日志验证（本 Scenario 在独立 QEMU 实例中运行，复位后进程退出）（验收 S10.2）

### Scenario S10.3: WdgmActivityMonitor
**Description:** WdgM 活性监控 — 被监控任务停止 `WdgM_CheckpointReached` 后触发超时
**Flow:**
1. `WdgM_Init`，任务 A 注册为被监控实体
2. 任务 A 正常上报 Checkpoint 5 次
3. 任务 A 停止上报（模拟任务死锁）
4. `WdgM_MainFunction` 继续调用，检测到活性失效
**Expected Result:** WdgM 检测到 `AliveSupervision` 失效；停止调用 `Wdg_SetTriggerCondition`；WDG 在 deadline 内超时复位（验收 S10.3）

### Scenario S10.4: WdgmModeSwitch
**Description:** WdgM 模式切换 — `SetMode(STOPPED)` 后 WDG 进入 OFF 模式
**Flow:**
1. `WdgM_Init`，WDG 处于 FAST_MODE
2. 调用 `WdgM_SetMode(WDGM_GLOBAL_MODE_STOPPED)`
3. 检查 Wdg 模式切换
4. 验证 `Wdg_SetMode(WDGIF_OFF_MODE)` 被调用
**Expected Result:** `Wdg_SetMode(WDGIF_OFF_MODE)` 被调用（调用记录桩验证）；WDG 停止计数（`wdg_off_mode_count == 1`）（验收 S10.4）

---

## 7. Dependencies

### Upper Layer Modules
- CI `run_qemu_test.sh`：通过 exit code 与 `WDG_TIMEOUT_PASS` 标记判定结果
- QemuAssert 基础设施：`Qemu_ReportPass` / `Qemu_ReportFail`

### Lower Layer Modules
- **WdgM**: `WdgM_Init` / `WdgM_MainFunction` / `WdgM_CheckpointReached` / `WdgM_SetMode` / `WdgM_GetGlobalStatus` / `WdgM_GlobalModeType`
- **Wdg (WdgIf)**: `Wdg_Init` / `Wdg_SetTriggerCondition` / `Wdg_SetMode` / `WdgIf_ModeType` 抽象层
- **CMSDK Watchdog**: QEMU 提供的虚拟看门狗外设
- **OS tick**: FreeRTOS tick 驱动周期调度
- **Det**: 错误追踪（链接依赖）
- **Uart_Cfg**: CMSDK UART 驱动（复用 `tests/qemu_m33/src/Uart_Cfg.c`）
- **startup_m33.s**: Cortex-M33 启动代码（复用 `tests/qemu_m33/src/startup_m33.s`）

### External Dependencies
- **QEMU**: mps2-an521 机器类型，需启用 `--semihosting-config enable=on,target=native`
- **arm-none-eabi-gcc**: 交叉编译工具链

### CI Integration

```bash
# run_qemu_test.sh 中追加（S10.1/S10.3/S10.4 合并为一个镜像）
run_test "qemu_wdg_timeout" "WDG_TIMEOUT_PASS"

# S10.2 单独实例（期望超时复位后日志含标记）
run_test_expect_reset "qemu_wdg_reset" "WDG_TIMEOUT_RESET"
```

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-08-22 | YuleTech | Initial QEMU WDG timeout reset verification specification |
