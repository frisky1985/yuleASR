# QemuRamEcc (QEMU RAM ECC 故障注入验证) Module Specification

> **Module:** QemuRamEcc (QEMU RAM ECC 故障注入验证)  
> **Layer:** Test Infrastructure / Safety  
> **Standard:** AUTOSAR Classic Platform 4.4.0 / ISO 26262 ASIL-D  
> **Platform:** QEMU mps2-an521 (Cortex-M33)  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

QemuRamEcc 在 QEMU mps2-an521 上通过软件位翻转模拟 RAM ECC 故障，验证 yuleASR RAM ECC 检测模块（`src/safety/ram/`、`src/safety/saferam/`、`src/bsw/cdd/Cdd_RamEcc_1.0.0.c`）的完整端到端链路：ECC 故障注入 → SafeRAM 检测 → Dem 故障上报 → 安全状态迁移。S32K312 硬件 ECC（MSCM/FCCU 寄存器操作）从未在 QEMU 上运行过端到端验证。QEMU 无硬件 ECC 支持，故使用软件故障注入钩子（fault injection hook）模拟 MSCM 中断。

### Key Responsibilities
- 通过软件位翻转模拟 1-bit 纠正性 ECC 错误（SEC），验证 RamSafety 检测并纠正后继续运行
- 通过软件位翻转模拟 2-bit 不可纠正 ECC 错误（DED），验证 RamSafety 触发安全状态迁移
- 验证 `RamSafety_MainFunction` 周期检测路径
- 验证 Dem 故障事件（`DTC_RAM_ECC_CORRECTED` / `DTC_RAM_ECC_UNCORRECTED`）正确上报
- 验证 SafeRAM 写保护区域在故障后不被修改（SafeRAM 分区完整性）
- 输出 `RAM_ECC_PASS` 标记供 CI 自动判定

---

## 2. API List

### 2.1 RamSafety APIs Under Test

| API | Description |
|-----|-------------|
| `void RamSafety_Init(const ConfigType *Config)` | RamSafety 初始化（4 分区：DTCM/SRAM/FlexRAM/Stack） |
| `void RamSafety_RunStartupTest(void)` | 启动时全量 March C- 检测 |
| `void RamSafety_MainFunction(void)` | 周期抽样检测（运行时） |
| `Std_ReturnType RamSafety_TriggerTest(uint8 RegionId)` | 手动触发指定区域检测 |
| `void RamSafety_EnterSafeState(void)` | 安全状态迁移（测试桩记录调用并正常退出） |
| `void RamSafety_GetStatistics(RamSafety_StatsType *Stats)` | 获取检测统计（errorCount / correctedCount） |

### 2.2 Fault Injection APIs

| API | Called By | Description |
|-----|-----------|-------------|
| `void FaultInject_FlipBit1(uint8 *addr, uint8 bitPos)` | 验证入口 | 翻转指定地址的 1 个 bit（SEC 模拟） |
| `void FaultInject_FlipBit2(uint8 *addr, uint8 bitPos1, uint8 bitPos2)` | 验证入口 | 翻转指定地址的 2 个 bit（DED 模拟） |
| `void FaultInject_RegisterEccCallback(SecCb_t SecCb, DedCb_t DedCb)` | 验证入口 | 注册 SEC/DED 回调（替代 MSCM 中断） |

### 2.3 Verification Result APIs

| API | Called By | Description |
|-----|-----------|-------------|
| `void Qemu_ReportPass(void)` | 验证入口 | 输出 `RAM_ECC_PASS` 并调用 semihosting `SYS_EXIT(0)` |
| `void Qemu_ReportFail(const char *reason)` | 验证入口 | 输出失败原因并调用 semihosting `SYS_EXIT(1)` |

---

## 3. Data Types

### 3.1 RamSafety State Enum

```c
typedef enum {
    RAMSAFETY_UNINIT    = 0x00,
    RAMSAFETY_OK        = 0x01,   /* 正常运行，SEC 已纠正 */
    RAMSAFETY_DEGRADED  = 0x02,   /* 降级（SEC 错误计数超阈值） */
    RAMSAFETY_FAILED    = 0x03    /* DED 触发安全状态迁移 */
} RamSafety_StateType;
```

### 3.2 Algorithm & Protection Enums

```c
typedef enum {
    RAMSAFETY_ALGO_MARCH_C_MINUS = 0x00,  /* March C- 算法 */
    RAMSAFETY_ALGO_CRC32         = 0x01   /* CRC32 校验 */
} RamSafety_AlgoType;

typedef enum {
    RAMSAFETY_WRITE_PROTECT_DISABLED = 0x00,
    RAMSAFETY_WRITE_PROTECT_ENABLED  = 0x01
} RamSafety_ProtectionType;
```

### 3.3 Dem DTC Defines

```c
/* RAM ECC Dem 故障事件 */
#define DTC_RAM_ECC_CORRECTED     0xB00001u  /* 1-bit SEC 错误已纠正 */
#define DTC_RAM_ECC_UNCORRECTED   0xB00002u  /* 2-bit DED 错误不可纠正 */
```

### 3.4 Fault Injection Constants

```c
#define RAMSAFETY_TEST_REGION_SIZE  256u   /* March C- 检测区域大小（字节），限制执行时间 < 10ms */
#define SEC_ERROR_THRESHOLD         5u     /* SEC 错误计数降级阈值 */
```

### 3.5 RAM Region Configuration (Test)

```c
/* 测试环境简化配置（256 字节测试区域，避免长时间 March C-） */
static const RamSafety_RegionConfigType RamSafety_TestRegions[] = {
    { .StartAddress = (uint32)&TestRamBuffer[0],
      .EndAddress   = (uint32)&TestRamBuffer[255],
      .Algorithm    = RAMSAFETY_ALGO_MARCH_C_MINUS,
      .Protection   = RAMSAFETY_WRITE_PROTECT_ENABLED },
};
static uint8 TestRamBuffer[256];
```

---

## 4. Error Handling

本模块为测试基础设施，不做 DET 错误报告。验证失败时调用 `Qemu_ReportFail` 直接终止。`RamSafety_EnterSafeState` 重写为 QEMU 测试桩（`RAMSAFETY_TEST_MODE=1`），记录调用后调用 `Qemu_ReportPass` 正常退出，避免安全状态迁移导致验证进程无法退出。

| Error Code | Value | Description |
|------------|-------|-------------|
| N/A | — | 模块不使用 DET |

---

## 5. Configuration Parameters

### Pre-Compile Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| `RAMSAFETY_TEST_REGION_SIZE` | 256 | March C- 检测区域大小（字节），限制执行时间 < 10ms |
| `SEC_ERROR_THRESHOLD` | 5 | SEC 错误计数降级阈值（验收 S9.4） |
| `RAMSAFETY_TEST_MODE` | 1 | 重写 `EnterSafeState` 为测试桩，正常退出 |
| `QEMU_TARGET` | 1 | QEMU 目标平台标识 |
| `QEMU_TIMEOUT` | 30 | CI 脚本超时秒数（环境变量） |
| `QEMU_MACHINE` | mps2-an521 | QEMU 机器类型 |
| `QEMU_CPU` | cortex-m33 | QEMU CPU 型号 |

### Build Configuration

```cmake
add_executable(qemu_ram_ecc
    p3a_ram_ecc/main_ram_ecc.c
    p3a_ram_ecc/ram_ecc_fault_inject.c
)
target_link_libraries(qemu_ram_ecc RamSafety Dem Det Os)
target_compile_definitions(qemu_ram_ecc PRIVATE
    QEMU_TARGET=1
    RAMSAFETY_TEST_MODE=1          # 重写 EnterSafeState 为测试桩
    RAMSAFETY_TEST_REGION_SIZE=256 # 限制检测区域大小
)
```

---

## 6. Scenarios

### Scenario S9.1: SecFaultCorrected
**Description:** 注入 1-bit 翻转（SEC 错误），RamSafety 检测并纠正后继续运行
**Flow:**
1. `RamSafety_Init`，TestRamBuffer 已知初始值
2. 调用 `FaultInject_FlipBit1(&TestRamBuffer[10], 3)` 翻转第 3 位
3. 调用 `RamSafety_TriggerTest(REGION_ID_TEST)` 执行检测
4. 检查模块状态与 Dem 上报
**Expected Result:** 模块状态保持 `RAMSAFETY_OK`；`Statistics.correctedCount == 1`；Dem 上报 `DTC_RAM_ECC_CORRECTED`；`TestRamBuffer[10]` 恢复原值（纠正成功）（验收 S9.1）

### Scenario S9.2: DedFaultSafeState
**Description:** 注入 2-bit 翻转（DED 错误），RamSafety 触发安全状态迁移
**Flow:**
1. `RamSafety_Init`
2. 调用 `FaultInject_FlipBit2(&TestRamBuffer[20], 1, 5)` 翻转 2 位
3. 调用 `RamSafety_TriggerTest(REGION_ID_TEST)` 执行检测
4. 检查 `RamSafety_EnterSafeState` 被调用
**Expected Result:** 测试桩 `safe_state_entered == TRUE`；Dem 上报 `DTC_RAM_ECC_UNCORRECTED`；模块状态迁移至 `RAMSAFETY_FAILED`（验收 S9.2）

### Scenario S9.3: SafeRamWriteProtect
**Description:** SafeRAM 写保护分区在故障注入后数据完整
**Flow:**
1. 写保护分区（`RAMSAFETY_WRITE_PROTECT_ENABLED`）填充已知模式
2. 调用 `FaultInject_FlipBit1` 与 `FaultInject_FlipBit2` 注入故障
3. `RamSafety_MainFunction` 运行 5 个周期
4. 调用 `RamSafety_VerifyRegion(REGION_ID_TEST)` 校验
**Expected Result:** `RamSafety_VerifyRegion` 返回 `E_OK`；写保护区域数据未被故障注入破坏（March C- 最终校验通过）（验收 S9.3）

### Scenario S9.4: SecThresholdDegrade
**Description:** 连续 5 次 SEC 错误后触发降级策略
**Flow:**
1. `RamSafety_Init`，`ErrorCountThreshold = 5`（Cfg 配置），错误计数清零
2. 循环 5 次调用 `FaultInject_FlipBit1` + `RamSafety_TriggerTest`
3. 检查第 5 次后模块状态
**Expected Result:** 累计阈值触发降级，`RamSafety_EnterSafeState` 被调用；`Statistics.errorCount == 5`（验收 S9.4）

---

## 7. Dependencies

### Upper Layer Modules
- CI `run_qemu_test.sh`：通过 exit code 与 `RAM_ECC_PASS` 标记判定结果
- QemuAssert 基础设施：`Qemu_ReportPass` / `Qemu_ReportFail`

### Lower Layer Modules
- **RamSafety**: `RamSafety_Init` / `RamSafety_MainFunction` / `RamSafety_TriggerTest` / `RamSafety_EnterSafeState` / `RamSafety_GetStatistics` / `RamSafety_VerifyRegion`
- **SafeRAM**: 写保护分区与 March C- 校验算法
- **Dem**: 故障事件上报桩（`DTC_RAM_ECC_CORRECTED` / `DTC_RAM_ECC_UNCORRECTED`）
- **Det**: 错误追踪（链接依赖）
- **Uart_Cfg**: CMSDK UART 驱动（复用 `tests/qemu_m33/src/Uart_Cfg.c`）
- **startup_m33.s**: Cortex-M33 启动代码（复用 `tests/qemu_m33/src/startup_m33.s`）

### External Dependencies
- **QEMU**: mps2-an521 机器类型，需启用 `--semihosting-config enable=on,target=native`
- **arm-none-eabi-gcc**: 交叉编译工具链

### CI Integration

```bash
# run_qemu_test.sh 中追加
run_test "qemu_ram_ecc" "RAM_ECC_PASS"
```

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-08-22 | YuleTech | Initial QEMU RAM ECC fault injection verification specification |
