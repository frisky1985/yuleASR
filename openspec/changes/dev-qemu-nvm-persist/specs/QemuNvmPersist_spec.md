# QemuNvmPersist (QEMU NvM 掉电恢复验证) Module Specification

> **Module:** QemuNvmPersist (QEMU NvM 掉电恢复验证)  
> **Layer:** Test Infrastructure  
> **Standard:** AUTOSAR Classic Platform 4.4.0  
> **Platform:** QEMU mps2-an521 (Cortex-M33)  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

验证 NvM 模块在 QEMU 上的掉电恢复路径：通过 semihosting 将 Flash 模拟数据导出到 host 文件，QEMU 重启后加载该文件，验证 `NvM_ReadAll` 恢复数据一致性。

### Key Responsibilities
- 验证 `NvM_WriteBlock` + `NvM_WriteAll` 写入路径
- 验证 semihosting 文件导出（Fls_Hw_MockFlash → flash.bin）
- 验证 QEMU 重启后 `NvM_ReadAll` 数据恢复
- 验证 CRC 损坏检测

---

## 2. API List

### 2.1 NvM APIs Under Test

| API | Description |
|-----|-------------|
| `NvM_Init(Config)` | NvM 初始化 |
| `NvM_WriteBlock(BlockId, Data)` | 写入单个块 |
| `NvM_WriteAll(void)` | 写入所有块 |
| `NvM_ReadAll(void)` | 读取所有块 |
| `NvM_GetErrorStatus(BlockId)` | 获取块错误状态 |

### 2.2 Fls Export API (新增)

| API | Description |
|-----|-------------|
| `Fls_Hw_ExportToHost(path)` | semihosting SYS_WRITE 导出 Flash 到 host 文件 |
| `Fls_Hw_ImportFromHost(path)` | semihosting SYS_READ 从 host 文件加载 |

---

## 3. Data Types

### 3.1 Flash Persist Constants

```c
#define FLASH_PERSIST_MAGIC     0xAA55U
#define FLASH_PERSIST_SIZE       4096U   /* 4KB 最小化 Flash 模拟区 */
#define NVM_TEST_BLOCK_ID       0x01U
#define NVM_TEST_BLOCK_VALUE    0xDEADBEEFU
```

---

## 4. Error Handling

| Error Code | Value | Description |
|------------|-------|-------------|
| `QEMU_FAIL_NVM_WRITE` | 30 | NvM 写入失败 |
| `QEMU_FAIL_FLASH_EXPORT` | 31 | flash.bin 导出失败 |
| `QEMU_FAIL_NVM_RESTORE` | 32 | NvM 读回值不匹配 |
| `QEMU_FAIL_CRC_CORRUPT` | 33 | CRC 损坏未检测到 |

---

## 5. Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `QEMU_NVM_PERSIST` | STD_ON | 启用 semihosting 导出宏 |

---

## 6. Scenarios

### Scenario 1: NvMWriteAndFlush
**Description:** NvM 写入并完成 WriteAll
**Flow:**
1. 初始化 NvM + Fls + Fee
2. `NvM_WriteBlock(0x01, &0xDEADBEEF)`
3. `NvM_WriteAll()`
4. 断言 `NvM_GetErrorStatus(0x01) == NVM_REQ_OK`
**Expected Result:** `Qemu_Assert(status == NVM_REQ_OK, "NvM write")`

### Scenario 2: FlashBinDump
**Description:** Flash 数据导出到 host 文件
**Flow:**
1. 完成 S5.1 写入
2. 调用 `Fls_Hw_ExportToHost("flash.bin")`
3. QEMU 退出
4. CI 检查 flash.bin 存在且大小 > 0
5. 检查 magic 字节 == 0xAA55
**Expected Result:** flash.bin 存在，magic == 0xAA55

### Scenario 3: PowerCycleRestore
**Description:** QEMU 重启后 NvM 读回数据
**Flow:**
1. 第二次 QEMU 启动
2. `Fls_Hw_ImportFromHost("flash.bin")` 加载数据
3. `NvM_ReadAll()`
4. 通过 RTE 读取 `Rte_Read_OdometerValue()`
5. 断言值 == 0xDEADBEEF
**Expected Result:** `Qemu_Assert(value == 0xDEADBEEF, "NvM restore")`

### Scenario 4: CorruptedBlockHandling
**Description:** CRC 损坏后 NVM_REQ_INTEGRITY_FAILED
**Flow:**
1. 加载 flash.bin 后翻转一个字节
2. `NvM_ReadAll()`
3. 断言 `NvM_GetErrorStatus(0x01) == NVM_REQ_INTEGRITY_FAILED`
**Expected Result:** `Qemu_Assert(status == NVM_REQ_INTEGRITY_FAILED, "CRC corrupt")`

---

## 7. Dependencies

### Upper Layer Modules
- 无

### Lower Layer Modules
- **NvM.c** (`src/bsw/services/nvm/src/NvM.c`): 生产代码
- **Fls_Hw.c** (`src/bsw/mcal/fls/src/Fls_Hw.c`): 生产代码，修改点（semihosting 宏）
- **Fee.c**: Flash 仿真 EEPROM
- **qemu_assert**: 断言报告

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-08-22 | YuleTech | Initial NvM persist verification specification |
