# MCAL Production Defects (MCAL 量产阻断缺陷修复) Module Specification

> **Module:** McalProductionDefects (Crypto / Mcu / Spi 缺陷修复)
> **Layer:** MCAL
> **Standard:** AUTOSAR Classic Platform R22-11 (SWS Crypto / SWS Spi / SWS Mcu)
> **Platform:** Host (MockHAL) / i.MX8M Mini ECSPI / S32K312
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

本变更修复 4 处 MCAL 源码缺陷，其行为已被接线测试以 "Source quirk" 注释固化为错误黄金断言。修复后驱动行为符合 AUTOSAR SWS 与驱动内部寄存器契约，测试断言回归正确语义。

### Key Responsibilities
- D1: `Crypto_GetVersionInfo` 空指针路径 DET 使用本 API 的 SID
- D2: `Mcu_DistributePllClock` 区分"未配置时钟"与"配置索引 0"，合法索引 0 可正常分发
- D3: `Spi_SetBaudRateInternal` 写出的 PERIODREG 分频组合无 +1 偏差
- D4: `Spi_GetVersionInfo` 空指针路径 DET 使用 `SPI_SERVICE_ID_GETVERSIONINFO`

---

## 2. API List

### 2.1 Modified APIs

| API | Defect | Change |
|-----|--------|--------|
| `void Crypto_GetVersionInfo(Std_VersionInfoType*)` | D1 | NULL 路径 `Det_ReportError` 第 3 参数：`CRYPTO_SID_DEINIT` → `CRYPTO_SID_GETVERSIONINFO` |
| `void Mcu_DistributePllClock(void)` | D2 | 未配置时钟判定：`currentClock == 0U` → `currentClock == MCU_CLOCK_INVALID` |
| `void Spi_GetVersionInfo(Std_VersionInfoType*)` | D4 | NULL 路径 `Det_ReportError` 第 3 参数：`0x02U` → `SPI_SERVICE_ID_GETVERSIONINFO` |

### 2.2 Internal Functions

| API | Defect | Change |
|-----|--------|--------|
| `Spi_SetBaudRateInternal(uint8, uint32)` | D3 | 分频搜索用独立变量捕获匹配组合；未匹配回退 15/15 |

---

## 3. Data Types

```c
/* Mcu.c — 新增 */
#define MCU_CLOCK_INVALID   ((Mcu_ClockType)0xFFFFFFFFU)  /* "未配置时钟"哨兵，合法索引远小于此值 */

/* Crypto.h — 新增（AUTOSAR SWS Crypto 服务 ID 表）*/
#define CRYPTO_SID_GETVERSIONINFO   (0x02U)   /* Init=0x00, DeInit=0x01, GetVersionInfo=0x02 */

/* Spi.h 无需新增 — SPI_SERVICE_ID_GETVERSIONINFO (9U) 已定义于 Spi_Cfg.h:26 */
```

---

## 4. Error Handling

| API | Condition | Error Code | SID |
|-----|-----------|------------|-----|
| `Crypto_GetVersionInfo` | `versioninfo == NULL` | `CRYPTO_E_PARAM_POINTER` | `CRYPTO_SID_GETVERSIONINFO` (0x02) |
| `Mcu_DistributePllClock` | 未配置任何时钟（`currentClock == MCU_CLOCK_INVALID`） | `MCU_E_PLL_NOT_LOCKED` | `MCU_SID_DISTRIBUTE_PLL_CLOCK` |
| `Spi_GetVersionInfo` | `versioninfo == NULL` | `SPI_E_PARAM_POINTER` | `SPI_SERVICE_ID_GETVERSIONINFO` (9) |

---

## 5. Configuration Parameters

无新增配置项。`MCU_NO_PLL`、`SPI_DEV_ERROR_DETECT`、`CRYPTO_CFG_DEV_ERROR_DETECT` 等现有配置语义不变。

---

## 6. Scenarios

### Scenario S1: CryptoVersionInfoCorrectSid
**Description:** `Crypto_GetVersionInfo(NULL)` 的 DET 报告使用本 API 的 SID
**Flow:**
1. `Crypto_Init(&cfg)`（DET 开启）
2. `Crypto_GetVersionInfo(NULL_PTR)`
**Expected Result:** `Det_ReportError` 收到 `(CRYPTO_MODULE_ID, 0, CRYPTO_SID_GETVERSIONINFO, CRYPTO_E_PARAM_POINTER)`（验收 S1）

### Scenario S2: McuClockIndexZeroDistributable
**Description:** 时钟配置索引 0 是合法配置，InitClock(0) 后可正常分发
**Flow:**
1. `Mcu_Init(&cfg)`（含 1 个 clock config）
2. Mock 寄存器：PLL LOCK=1，CCSR 匹配 ClockSource
3. `Mcu_InitClock(0U)` → 返回 `E_OK`
4. `Mcu_DistributePllClock()`
**Expected Result:** `MCU_CCM_CCR & CLK_ENABLE != 0`，无 DET 上报（验收 S2）
**Negative:** 未调用 `Mcu_InitClock` 时 `Mcu_DistributePllClock()` 仍上报 `MCU_E_PLL_NOT_LOCKED` 且 CCR 不变。

### Scenario S3: SpiBaudDividerExact
**Description:** PERIODREG 分频组合精确匹配目标波特率（无 +1 偏差）
**Flow:**
1. 配置通道 0：80MHz 参考时钟 / 1Mbit/s → `tempDiv = 80`
2. `Spi_Init(&cfg)`
**Expected Result:** PERIODREG = `(3) | (9 << 4) = 0x93`，即首个满足 `2^preDiv × (postDiv+1) >= 80` 的组合 `preDiv=3, postDiv=9`（验收 S3）
**Channel 1:** 8Mbit/s → `tempDiv = 10` → PERIODREG = `(0) | (9 << 4) = 0x90`

### Scenario S4: SpiVersionInfoCorrectSid
**Description:** `Spi_GetVersionInfo(NULL)` 的 DET 报告使用 `SPI_SERVICE_ID_GETVERSIONINFO`
**Flow:**
1. `Spi_GetVersionInfo(NULL_PTR)`
**Expected Result:** `Det_ReportError` 收到 `(SPI_MODULE_ID, SPI_INSTANCE_ID, SPI_SERVICE_ID_GETVERSIONINFO, SPI_E_PARAM_POINTER)`（验收 S4）

---

## 7. Dependencies

- `Spi_Cfg.h`（`SPI_SERVICE_ID_GETVERSIONINFO` 定义源）
- `Mcu_Reg.h` / `Mcu_Cfg.h`（寄存器地址与 DET 开关）
- 接线测试框架：MockHAL（`mock_hal.h` 拦截 REG_READ32/REG_WRITE32）+ `mock_det.c` + `mock_registers.c`
- CI：`ctest` 全量回归

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-09-25 | YuleTech | Initial spec: D1-D4 缺陷修复与测试黄金断言回归 |
