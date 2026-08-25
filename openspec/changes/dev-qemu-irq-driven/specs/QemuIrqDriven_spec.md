# QemuIrqDriven (QEMU IRQ 驱动验证) Module Specification

> **Module:** QemuIrqDriven (QEMU IRQ 驱动验证)  
> **Layer:** Test Infrastructure  
> **Standard:** AUTOSAR Classic Platform 4.4.0 / ARM Cortex-M33 NVIC  
> **Platform:** QEMU mps2-an521 (Cortex-M33)  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

QemuIrqDriven 在 QEMU mps2-an521 上通过 CMSDK APB Timer0 触发周期 NVIC 中断，验证 yuleASR MCAL 中断层（Gpt_Irq、Wdg_Irq、Can_Irq 等）的 ISR 注册、中断上下文切换、NVIC 优先级抢占以及 RTOS 任务唤醒路径。当前 QEMU 验证仅在 FreeRTOS tick（SysTick）驱动的软件轮询模式下运行，IRQ 向量未经验证；本模块补全该缺口。

### Key Responsibilities
- 配置 CMSDK APB Timer0 产生周期 IRQ，驱动 `Gpt_Isr` 执行路径
- 验证 ISR 注册和执行计数（IRQ 累计触发次数）
- 验证从 ISR 调用 FreeRTOS `xTaskNotifyFromISR` 正确唤醒等待任务
- 验证嵌套 IRQ 场景下堆栈完整性（FreeRTOS stack watermark > 0）
- 验证 NVIC 优先级配置（`NVIC_SetPriority` / `NVIC_EnableIRQ`）与抢占顺序
- 输出 `IRQ_DRIVEN_PASS` 标记供 CI 自动判定

---

## 2. API List

### 2.1 IRQ Timer Stub APIs

| API | Description |
|-----|-------------|
| `void IrqTimer_Init(uint32 periodMs)` | 初始化 APB Timer0，设置周期（ms），配置控制寄存器（EN/IRQEN） |
| `void IrqTimer_Start(void)` | 使能 Timer 并开启 NVIC 中断 |
| `void IrqTimer_Stop(void)` | 关闭 Timer 和 NVIC 中断 |
| `uint32 IrqTimer_GetCount(void)` | 返回 IRQ 累计触发次数 |
| `void TIMER0_IRQHandler(void)` | ISR 实现（弱符号，可重写），递增计数并调用 `xTaskNotifyFromISR` |

### 2.2 Verification Result APIs

| API | Called By | Description |
|-----|-----------|-------------|
| `void Qemu_ReportPass(void)` | 验证入口 | 输出 `IRQ_DRIVEN_PASS` 并调用 semihosting `SYS_EXIT(0)` |
| `void Qemu_ReportFail(const char *reason)` | 验证入口 | 输出失败原因并调用 semihosting `SYS_EXIT(1)` |

### 2.3 Nested IRQ / Priority APIs

| API | Called By | Description |
|-----|-----------|-------------|
| `void IrqTimer_RegisterNestedUart(void)` | 验证入口 | 注册 UART0 IRQ 用于嵌套中断场景 |
| `bool QemuIrqDriven_CheckStackWatermark(void)` | 验证入口 | 检查 `uxTaskGetStackHighWaterMark(NULL) > 0` |
| `void QemuIrqDriven_RecordOrder(uint8 irqId)` | ISR | 记录 ISR 执行顺序至 `order[]` 数组 |

---

## 3. Data Types

### 3.1 CMSDK APB Timer0 Register Defines

```c
/* CMSDK APB Timer0 base address (mps2-an521) */
#define TIMER0_BASE        0x40000000u
/* Register offsets */
#define TIMER_LOAD_OFFSET  0x00u   /* 定时器重装载值（写入即重载） */
#define TIMER_VALUE_OFFSET 0x04u   /* 当前计数值（只读） */
#define TIMER_CTRL_OFFSET  0x08u   /* 控制寄存器（bit0=EN, bit3=IRQEN） */
#define TIMER_INTCLR_OFFSET 0x0Cu  /* 中断清除（写任意值清除） */
```

### 3.2 NVIC IRQ Numbers

```c
/* CMSDK APB Timer0 IRQ number (mps2-an521) */
#define TIMER0_IRQn   16   /* IRQ0 */
#define TIMER1_IRQn   17   /* IRQ1 */
#define UART0_TX_IRQn 21   /* IRQ5 */
```

### 3.3 Priority Constants

```c
#define IRQ_PRIO_TIMER0_HIGH  1u   /* 高优先级（数值小优先级高） */
#define IRQ_PRIO_UART0_LOW    2u   /* 低优先级 */
```

### 3.4 Verification Counters

```c
volatile uint32_t IRQ_COUNT = 0;            /* 累计 IRQ 触发次数 */
volatile uint8_t  IRQ_ORDER[4] = {0};       /* ISR 执行顺序记录 */
volatile uint32_t IRQ_ORDER_IDX = 0;        /* 顺序记录索引 */
```

---

## 4. Error Handling

本模块为测试基础设施，不做 DET 错误报告。验证失败时调用 `Qemu_ReportFail` 直接终止。

| Error Code | Value | Description |
|------------|-------|-------------|
| N/A | — | 模块不使用 DET |

---

## 5. Configuration Parameters

### Pre-Compile Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| `IRQ_TARGET_COUNT` | 10 | 目标 IRQ 触发次数（验收 S7.1） |
| `IRQ_TIMER_PERIOD_MS` | 10 | APB Timer0 中断周期（毫秒） |
| `IRQ_WAIT_DURATION_MS` | 150 | 验证等待时长（毫秒） |
| `IRQ_NESTED_ENABLE` | STD_ON | 启用嵌套中断场景验证 |
| `IRQ_WAKEUP_TICK_LIMIT` | 1 | 等待任务唤醒最大 tick 数（验收 S7.2） |
| `QEMU_TARGET` | 1 | QEMU 目标平台标识 |
| `QEMU_TIMEOUT` | 30 | CI 脚本超时秒数（环境变量） |
| `QEMU_MACHINE` | mps2-an521 | QEMU 机器类型 |
| `QEMU_CPU` | cortex-m33 | QEMU CPU 型号 |

### Build Configuration

```cmake
add_executable(qemu_irq_driven
    p3d_irq_driven/main_irq_driven.c
    p3d_irq_driven/irq_timer_stub.c
)
target_link_libraries(qemu_irq_driven Os freertos_config)
target_compile_definitions(qemu_irq_driven PRIVATE QEMU_TARGET=1)
```

---

## 6. Scenarios

### Scenario S7.1: PeriodicIrqCount
**Description:** APB Timer0 周期 IRQ 触发 ≥ 10 次，计数器累计正确
**Flow:**
1. 初始化 UART 与 APB Timer0（`IrqTimer_Init(10)`，10ms 周期）
2. 调用 `IrqTimer_Start()` 使能 Timer 与 NVIC
3. 等待任务挂起 150ms
4. 调用 `IrqTimer_Stop()` 关闭 Timer
**Expected Result:** `IrqTimer_GetCount() >= 10`（`IRQ_COUNT == 10`）；CI exit code == 0；UART 含 `IRQ_DRIVEN_PASS`

### Scenario S7.2: IsrTaskNotifyWakeup
**Description:** ISR 中调用 `xTaskNotifyFromISR` 后，等待任务在 1 个 tick 内被唤醒
**Flow:**
1. 创建 TaskA 并调用 `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)` 挂起
2. 启动 APB Timer0
3. `TIMER0_IRQHandler` 中调用 `xTaskNotifyFromISR(&TaskA, ...)`
4. 记录唤醒时间戳并对比
**Expected Result:** TaskA 在 1 个 tick 内恢复运行（验收 S7.2）

### Scenario S7.3: NestedIrqStackIntegrity
**Description:** 嵌套中断场景下（Timer0 中嵌套 UART0 IRQ）任务堆栈无溢出
**Flow:**
1. 配置 TIMER0（IRQ 优先级=2）与 UART0（IRQ 优先级=1）
2. 触发 TIMER0_IRQHandler 执行过程中 UART0 IRQ 抢占
3. 嵌套返回后检查堆栈
4. 调用 `QemuIrqDriven_CheckStackWatermark()`
**Expected Result:** `uxTaskGetStackHighWaterMark(NULL) > 0`（验收 S7.3）

### Scenario S7.4: NvicPriorityPreemption
**Description:** 低优先级 ISR 被高优先级 ISR 抢占，执行顺序正确
**Flow:**
1. 两路 IRQ 同时 pending，优先级不同
2. 高优先级 ISR 先执行并返回，再执行低优先级 ISR
3. 通过 `IRQ_ORDER[]` 顺序标志数组确认
**Expected Result:** `order[0] == HIGH_IRQ`，`order[1] == LOW_IRQ`（验收 S7.4）

---

## 7. Dependencies

### Upper Layer Modules
- CI `run_qemu_test.sh`：通过 exit code 与 `IRQ_DRIVEN_PASS` 标记判定结果
- QemuAssert 基础设施：`Qemu_ReportPass` / `Qemu_ReportFail`

### Lower Layer Modules
- **FreeRTOS**: `xTaskNotifyFromISR` / `ulTaskNotifyTake` / `uxTaskGetStackHighWaterMark` / `configMAX_SYSCALL_INTERRUPT_PRIORITY`
- **NVIC**: `NVIC_SetPriority` / `NVIC_EnableIRQ` / 向量表注册
- **CMSDK APB Timer0**: QEMU 提供的虚拟定时器外设
- **Uart_Cfg**: CMSDK UART 驱动（复用 `tests/qemu_m33/src/Uart_Cfg.c`）
- **startup_m33.s**: Cortex-M33 启动代码（复用 `tests/qemu_m33/src/startup_m33.s`）

### External Dependencies
- **QEMU**: mps2-an521 机器类型，需启用 `--semihosting-config enable=on,target=native`
- **arm-none-eabi-gcc**: 交叉编译工具链

### CI Integration

```bash
# run_qemu_test.sh 中追加
run_test "qemu_irq_driven" "IRQ_DRIVEN_PASS"
```

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-08-22 | YuleTech | Initial QEMU IRQ driven verification specification |
