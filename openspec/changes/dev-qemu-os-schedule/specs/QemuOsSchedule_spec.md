# QemuOsSchedule (QEMU OS 调度验证) Module Specification

> **Module:** QemuOsSchedule (QEMU OS 调度验证)  
> **Layer:** Test Infrastructure  
> **Standard:** AUTOSAR OS / FreeRTOS V10.6.x  
> **Platform:** QEMU mps2-an521 (Cortex-M33)  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

在 QEMU Cortex-M33 仿真器上验证 yuleASR Os.c 包装层与 FreeRTOS 内核的集成正确性，覆盖 SysTick 推进、任务优先级抢占、任务终止、Alarm 到期回调四个核心场景。

### Key Responsibilities
- 验证 FreeRTOS SysTick 在 1000Hz 配置下正确递增 tick counter
- 验证 AUTOSAR OS 任务优先级抢占语义
- 验证 `TerminateTask()` API 行为
- 验证 AUTOSAR Alarm 定时到期回调

---

## 2. API List

### 2.1 Test Entry Point

| API | Description |
|-----|-------------|
| `int main(void)` | 初始化 UART + 调用 `StartOS()` 启动调度器 |

### 2.2 AUTOSAR OS APIs Under Test

| API | Called By | Description |
|-----|-----------|-------------|
| `StartOS(OS_Mode)` | main | 启动 OS |
| `ActivateTask(TaskType)` | Alarm 回调 | 激活任务 |
| `TerminateTask(void)` | Task-B | 终止自身 |
| `SetRelAlarm(AlarmType, tick, cycle)` | main | 设置相对定时 alarm |

---

## 3. Data Types

### 3.1 Task Configuration

```c
/* 测试用任务配置（2 个任务 + 1 个 alarm） */
#define TASK_PRIO_HIGH    3
#define TASK_PRIO_LOW     1
#define ALARM_PERIOD_MS   500
```

---

## 4. Error Handling

| Error Code | Value | Description |
|------------|-------|-------------|
| `QEMU_FAIL_TASK_ORDER` | 1 | 任务执行顺序不符合优先级预期 |
| `QEMU_FAIL_TICK_COUNT` | 2 | SysTick 计数不达标 |
| `QEMU_FAIL_ALARM_CB` | 3 | Alarm 回调未触发 |
| `QEMU_FAIL_TERMINATE` | 4 | TerminateTask 后任务仍被调度 |

---

## 5. Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `TICK_RATE_HZ` | 1000 | FreeRTOS tick 频率 |
| `TASK_A_PERIOD_MS` | 100 | Task-A 执行周期 |
| `TASK_B_PERIOD_MS` | 1000 | Task-B 执行周期 |
| `ALARM_CYCLE_MS` | 500 | Alarm 周期 |

---

## 6. Scenarios

### Scenario 1: SysTickAdvance
**Description:** 验证 1000Hz SysTick 在 100ms 内将 tick counter 推进 ≥ 100 次
**Flow:**
1. `StartOS()` 启动调度器
2. Task-A 在首次运行时记录 `xTaskGetTickCount()`
3. 延时 100ms 后再次读取
4. 断言差值 ≥ 90（允许 QEMU 时钟偏差）
**Expected Result:** `Qemu_Assert(tick_diff >= 90, "SysTick advance")`

### Scenario 2: TaskPriorityPreempt
**Description:** 高优先级 Task-A 抢占低优先级 Task-B
**Flow:**
1. Task-A（优先级 3）和 Task-B（优先级 1）同时激活
2. Task-A 执行 3 轮，每轮输出 `A:1 A:2 A:3`
3. Task-B 执行 1 轮，输出 `B:1`
4. 检查 UART 输出顺序
**Expected Result:** `A:1 A:2 A:3` 出现在 `B:1` 之前

### Scenario 3: TerminateTask
**Description:** Task-B 调用 TerminateTask() 后不再被调度
**Flow:**
1. Task-B 首次运行后调用 `TerminateTask()`
2. Task-A 继续运行 2 轮
3. 检查 UART 无第二个 `B:` 输出
**Expected Result:** `Qemu_Assert(b_run_count == 1, "TerminateTask")`

### Scenario 4: AlarmExpiry
**Description:** 500ms alarm 到期回调被调用 ≥ 3 次
**Flow:**
1. `SetRelAlarm(Alarm_500ms, 500, 500)` 设置周期 alarm
2. Alarm 回调递增计数器
3. 延时 2000ms
4. 断言计数器 ≥ 3
**Expected Result:** `Qemu_Assert(alarm_cb_count >= 3, "Alarm expiry")`

---

## 7. Dependencies

### Upper Layer Modules
- 无（此为终端验证镜像）

### Lower Layer Modules
- **Os.c** (`src/bsw/os/src/Os.c`): 生产代码，被验证目标
- **FreeRTOS Kernel**: tasks.c, queue.c, list.c, timers.c, port.c (ARM_CM33_NTZ)
- **Uart_Cfg**: UART 输出
- **qemu_assert**: 断言报告

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-08-22 | YuleTech | Initial OS schedule verification specification |
