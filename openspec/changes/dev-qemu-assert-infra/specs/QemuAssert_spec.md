# QemuAssert (QEMU Assert Infrastructure) Module Specification

> **Module:** QemuAssert (QEMU 断言基础设施)  
> **Layer:** Test Infrastructure  
> **Standard:** ARM Semihosting  
> **Platform:** QEMU mps2-an521 (Cortex-M33)  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

QemuAssert 提供 QEMU 仿真环境下的测试断言基础设施，通过 ARM semihosting `SYS_EXIT` 指令报告测试结果（PASS/FAIL），并通过 CMSDK UART 输出可 grep 的标记字符串供 CI 自动判定。

### Key Responsibilities
- 提供 `Qemu_ReportPass()` / `Qemu_ReportFail()` / `Qemu_Assert()` 三个断言 API
- 通过 semihosting `SYS_EXIT(0x18)` 实现 QEMU 进程退出码报告
- 通过 UART 输出标准化 PASS/FAIL 标记字符串
- 移植 Unity 测试框架输出到 UART

---

## 2. API List

### 2.1 Core Lifecycle APIs

| API | Description |
|-----|-------------|
| `void Qemu_ReportPass(void)` | 输出 PASS 标记，调用 semihosting SYS_EXIT(0)，不再返回 |
| `void Qemu_ReportFail(const char *msg)` | 输出 FAIL 标记 + 消息，调用 semihosting SYS_EXIT(1)，不再返回 |
| `void Qemu_Assert(bool cond, const char *msg)` | 条件断言：cond 为假时调用 Qemu_ReportFail |

### 2.2 Unity Integration APIs

| API | Called By | Description |
|-----|-----------|-------------|
| `void UnityOutputChar(char c)` | Unity 框架 | 将单个字符转发到 Uart_WriteByte |
| `void UnityOutputFlush(void)` | Unity 框架 | 空实现（UART 无缓冲） |

---

## 3. Data Types

### 3.1 Marker Constants

```c
#define QEMU_PASS_MARKER  "QEMU_FULL_STACK_PASS"
#define QEMU_FAIL_MARKER  "QEMU_FULL_STACK_FAIL"
```

### 3.2 Semihosting Operations

```c
/* ARM Semihosting operation numbers */
#define SYS_EXIT     0x18   /* Exit QEMU process */
#define SYS_WRITE0   0x04   /* Write null-terminated string to host stdout */
```

---

## 4. Error Handling

本模块为测试基础设施，不做 DET 错误报告。所有错误通过 `Qemu_ReportFail` 直接终止。

| Error Code | Value | Description |
|------------|-------|-------------|
| N/A | — | 模块不使用 DET |

---

## 5. Configuration Parameters

### Pre-Compile Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| `QEMU_TIMEOUT` | 30 | CI 脚本超时秒数（环境变量） |
| `QEMU_MACHINE` | mps2-an521 | QEMU 机器类型 |
| `QEMU_CPU` | cortex-m33 | QEMU CPU 型号 |

---

## 6. Scenarios

### Scenario 1: SemihostingExit_Pass
**Description:** 调用 Qemu_ReportPass() 后 QEMU 以 exit(0) 退出
**Flow:**
1. 初始化 UART
2. 调用 `Qemu_ReportPass()`
3. QEMU 进程退出
**Expected Result:** QEMU exit code == 0；UART 输出含 `QEMU_FULL_STACK_PASS`

### Scenario 2: SemihostingExit_Fail
**Description:** 调用 Qemu_ReportFail("test error") 后 QEMU 以 exit(1) 退出
**Flow:**
1. 初始化 UART
2. 调用 `Qemu_ReportFail("test error")`
3. QEMU 进程退出
**Expected Result:** QEMU exit code == 1；UART 输出含 `QEMU_FULL_STACK_FAIL: test error`

### Scenario 3: UnityUartOutput
**Description:** 运行 3 个 Unity 测试用例（1 pass / 1 fail / 1 ignored），摘要通过 UART 输出
**Flow:**
1. 初始化 UART + Unity
2. 运行 test_pass（TEST_ASSERT_EQUAL(1, 1)）
3. 运行 test_fail（TEST_ASSERT_EQUAL(1, 2)）
4. 运行 test_ignore（TEST_IGNORE）
5. UNITY_END() 输出摘要
**Expected Result:** UART 输出含 `3 Tests 1 Failures 1 Ignored`

### Scenario 4: TimeoutGuard
**Description:** run_qemu_test.sh 在 30s 超时时返回非零 exit code
**Flow:**
1. 构造一个不调用 Qemu_ReportPass 的镜像（死循环）
2. 执行 `timeout 5 qemu-system-arm ...`
3. 检查 exit code
**Expected Result:** exit code == 124（timeout）；CI step 标红

---

## 7. Dependencies

### Upper Layer Modules
- 各 P1-P3 测试镜像（C2-C10）：通过 `Qemu_ReportPass/Fail` 报告结果

### Lower Layer Modules
- **Uart_Cfg**: CMSDK UART 驱动（复用 `tests/qemu_m33/src/Uart_Cfg.c`）
- **startup_m33.s**: Cortex-M33 启动代码（复用 `tests/qemu_m33/src/startup_m33.s`）

### External Dependencies
- **QEMU**: mps2-an521 机器类型，需启用 `--semihosting-config enable=on,target=native`
- **arm-none-eabi-gcc**: 交叉编译工具链

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-08-22 | YuleTech | Initial QEMU assert infrastructure specification |
