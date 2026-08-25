# QemuEcuMStartup (QEMU EcuM 启动序列验证) Module Specification

> **Module:** QemuEcuMStartup (QEMU EcuM 启动序列验证)  
> **Layer:** Test Infrastructure  
> **Standard:** AUTOSAR Classic Platform 4.4.0  
> **Platform:** QEMU mps2-an521 (Cortex-M33)  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

在 QEMU 仿真器上用真实 EcuM.c + BswM.c 驱动 AUTOSAR 标准三阶段启动序列，验证 BSW 初始化顺序约束和 BswM 状态切换。

### Key Responsibilities
- 验证 EcuM 三阶段启动顺序（Init → StartupTwo → StartupThree）
- 验证 MCAL 先于 ECUAL 初始化的分层约束
- 验证 BswM 在 StartupThree 后进入 RUN 状态
- 验证有序关机（Service → ECUAL → MCAL 逆序 deinit）

---

## 2. API List

### 2.1 EcuM APIs Under Test

| API | Description |
|-----|-------------|
| `EcuM_Init(void)` | 阶段一：驱动 MCAL + ECUAL 初始化 |
| `EcuM_StartupTwo(void)` | 阶段二：OS 启动后调用，驱动 Service 层初始化 |
| `EcuM_StartupThree(void)` | 阶段三：BswM RUN 请求，进入正常工作 |
| `EcuM_RequestShutdown(void)` | 请求有序关机 |

### 2.2 BswM APIs Under Test

| API | Description |
|-----|-------------|
| `BswM_Init(void)` | BswM 初始化 |
| `BswM_GetState(void)` | 获取当前 BswM 状态 |

---

## 3. Data Types

### 3.1 BswM State Type

```c
typedef enum {
    BSWM_STATE_INIT,
    BSWM_STATE_RUN,
    BSWM_STATE_POST_RUN,
    BSWM_STATE_SHUTDOWN
} BswM_StateType;
```

---

## 4. Error Handling

| Error Code | Value | Description |
|------------|-------|-------------|
| `QEMU_FAIL_PHASE_ORDER` | 10 | 启动阶段顺序错误 |
| `QEMU_FAIL_BSWM_STATE` | 11 | BswM 状态不是 RUN |
| `QEMU_FAIL_INIT_ORDER` | 12 | MCAL 未先于 ECUAL 初始化 |
| `QEMU_FAIL_DEINIT_ORDER` | 13 | Deinit 逆序错误 |

---

## 5. Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `ECUM_QEMU_STUB_UART` | STD_ON | 启用桩函数 UART 打点 |

---

## 6. Scenarios

### Scenario 1: StartupPhaseOrder
**Description:** EcuM 三阶段按序执行
**Flow:**
1. `EcuM_Init()` — 阶段一，输出 `PHASE1_DONE`
2. `StartOS()` 启动调度器
3. `EcuM_StartupTwo()` — 阶段二，输出 `PHASE2_DONE`
4. `EcuM_StartupThree()` — 阶段三，输出 `PHASE3_DONE`
**Expected Result:** UART 顺序包含 `PHASE1_DONE PHASE2_DONE PHASE3_DONE`

### Scenario 2: BswMRUNRequest
**Description:** StartupThree 后 BswM 进入 RUN 状态
**Flow:**
1. 完成 EcuM 三阶段启动
2. 调用 `BswM_GetState()`
3. 断言返回值为 `BSWM_STATE_RUN`
**Expected Result:** `Qemu_Assert(state == BSWM_STATE_RUN, "BswM RUN")`

### Scenario 3: McalInitFirst
**Description:** MCAL 在 ECUAL 之前初始化
**Flow:**
1. EcuM_Init 调用 MCAL 桩（Mcu/Port/Dio），每个输出 `MCAL_INIT_DONE`
2. EcuM_Init 调用 ECUAL 桩（CanIf），输出 `CANIF_INIT_DONE`
3. 检查 UART 输出顺序
**Expected Result:** `MCAL_INIT_DONE` 出现在 `CANIF_INIT_DONE` 之前

### Scenario 4: OrderlyShutdown
**Description:** EcuM_RequestShutdown 触发逆序 deinit
**Flow:**
1. 系统进入 RUN 状态
2. 调用 `EcuM_RequestShutdown()`
3. Service 层 deinit，输出 `SERVICE_DEINIT`
4. ECUAL 层 deinit，输出 `ECUAL_DEINIT`
5. MCAL 层 deinit，输出 `MCAL_DEINIT`
**Expected Result:** UART 顺序 `SERVICE_DEINIT ECUAL_DEINIT MCAL_DEINIT`

---

## 7. Dependencies

### Upper Layer Modules
- 无

### Lower Layer Modules
- **EcuM.c** (`src/bsw/services/ecum/src/EcuM.c`): 生产代码
- **BswM.c** (`src/bsw/services/bswm/src/BswM.c`): 生产代码
- **Os.c**: OS 调度（C2 验证通过后可用）
- **qemu_assert**: 断言报告

### Test Stubs
- `ecum_test_stubs.c`: MCAL 桩（Mcu/Port/Dio）+ ECUAL 桩（CanIf），带 UART 打点

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-08-22 | YuleTech | Initial EcuM startup verification specification |
