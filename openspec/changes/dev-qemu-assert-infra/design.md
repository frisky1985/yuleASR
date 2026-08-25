# QEMU 断言基础设施 — 设计文档

## Architecture Overview

```
┌─────────────────────────────────────────────┐
│  Test Image (main_*.c)                       │
│  ├── Qemu_Assert(cond, msg)                  │
│  └── Unity TEST_ASSERT_*                    │
├─────────────────────────────────────────────┤
│  common/qemu_assert.c                        │
│  ├── Qemu_ReportPass() → UART + SYS_EXIT(0) │
│  └── Qemu_ReportFail() → UART + SYS_EXIT(1) │
├─────────────────────────────────────────────┤
│  common/unity_uart_output.c                  │
│  └── UnityOutputChar() → Uart_WriteByte()   │
├─────────────────────────────────────────────┤
│  Uart_Cfg.c (复用 qemu_m33)                  │
│  └── CMSDK UART @ 0x40200000                │
├─────────────────────────────────────────────┤
│  QEMU mps2-an521 (Cortex-M33)               │
│  ├── -serial stdio  →  UART 输出            │
│  └── --semihosting   →  SYS_EXIT → exit(N)  │
└─────────────────────────────────────────────┘
```

## Component Design

### 1. Semihosting Exit 实现

ARM semihosting 通过 `bkpt #0xAB` 指令触发，R0 传入操作号，R1 传入参数块地址。

`SYS_EXIT (0x18)` 的参数格式：
- R1 指向一个 2 字（word）的参数块
- word[0] = reason code（1 = ADP_Stopped_ApplicationExit）
- word[1] = exit code（0 = success，非零 = error）

```c
static void qemu_semihosting_exit(int code)
{
    volatile uint32_t params[2] = { 1, (uint32_t)code };
    register uint32_t r0 __asm("r0") = 0x18;       /* SYS_EXIT */
    register uint32_t r1 __asm("r1") = (uint32_t)params;
    __asm volatile("bkpt #0xAB" : : "r"(r0), "r"(r1) : "memory");
}
```

### 2. Unity UART 移植

Unity 框架通过 `UNITY_OUTPUT_CHAR` 宏定义输出函数。在 `unity_config.h` 或编译命令中定义：

```c
#define UNITY_OUTPUT_CHAR(c)  UnityOutputChar(c)
#define UNITY_OUTPUT_FLUSH()  UnityOutputFlush()
```

`UnityOutputChar` 实现逐字符转发到 `Uart_WriteByte`，处理 `\n` → `\r\n` 转换。

## Testing Strategy

### 验证镜像入口

```c
/* p0_assert_infra/main_assert_test.c */
#include "qemu_assert.h"
#include "Uart_Cfg.h"

/* 测试 Unity UART 输出 */
#include "unity.h"

void test_pass(void)   { TEST_ASSERT_EQUAL(1, 1); }
void test_fail(void)   { TEST_ASSERT_EQUAL(1, 2); }
void test_ignore(void) { TEST_IGNORE(); }

int main(void)
{
    Uart_Init();

    /* S1.1: Pass */
    Qemu_ReportPass();  /* 不返回 */
    return 0;
}
```

### CI 判定流程

```
run_qemu_test.sh <elf> <marker> <log>
  │
  ├── timeout 30s
  ├── qemu-system-arm ... | tee log
  ├── 检查 exit code (0 = pass, 1 = fail, 124 = timeout)
  └── grep -q MARKER log
```

## Configuration Strategy

### QEMU 启动参数

```bash
qemu-system-arm \
    -machine mps2-an521 \
    -cpu cortex-m33 \
    -kernel <elf> \
    -nographic \
    -serial stdio \
    --semihosting-config enable=on,target=native
```

- `-serial stdio`：UART 输出重定向到 stdout（供 `tee`/`grep`）
- `--semihosting-config enable=on,target=native`：semihosting SYS_EXIT 转为 QEMU 进程退出码
