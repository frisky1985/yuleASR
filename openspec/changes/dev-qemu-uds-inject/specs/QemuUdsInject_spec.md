# QemuUdsInject (QEMU UDS 诊断注入验证) Module Specification

> **Module:** QemuUdsInject (QEMU UDS 诊断注入验证)  
> **Layer:** Test Infrastructure  
> **Standard:** AUTOSAR Classic Platform 4.4.0 / ISO 14229 (UDS)  
> **Platform:** QEMU mps2-an521 (Cortex-M33)  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

通过 CAN loopback 注入 UDS 诊断请求帧，验证 Dcm 模块生成的响应帧内容和状态机行为。

### Key Responsibilities
- 验证 UDS ReadDataByIdentifier (0x22) 服务
- 验证 EcuReset (0x11) 服务
- 验证 NegativeResponse (0x7F) 生成
- 验证 CanTp 多帧重组

---

## 2. API List

### 2.1 Dcm APIs Under Test

| API | Description |
|-----|-------------|
| `Dcm_Init(Config)` | Dcm 初始化 |
| `Dcm_MainFunction(void)` | Dcm 主循环（轮询模式） |

### 2.2 Injection APIs

| API | Description |
|-----|-------------|
| `CanIf_RxIndication(Mailbox, PduInfo)` | 注入 UDS 请求帧 |
| `Can_Write(Hth, PduInfo)` | 捕获 Dcm 响应帧（重写 stub） |

---

## 3. Data Types

### 3.1 UDS Constants

```c
#define UDS_REQ_CAN_ID     0x7E0U   /* 诊断请求 CAN ID */
#define UDS_RESP_CAN_ID    0x7E8U   /* 诊断响应 CAN ID */

#define SID_READ_DATA_BY_ID    0x22U
#define SID_ECU_RESET          0x11U
#define SID_NEGATIVE_RESPONSE  0x7FU

#define DID_VIN                0xF190U
#define ECU_RESET_SOFT         0x03U
#define NRC_SERVICE_NOT_SUPPORTED  0x11U
```

---

## 4. Error Handling

| Error Code | Value | Description |
|------------|-------|-------------|
| `QEMU_FAIL_UDS_RESPONSE` | 40 | 响应帧 CAN ID 或 SID 不匹配 |
| `QEMU_FAIL_UDS_NEGATIVE` | 41 | NegativeResponse NRC 不匹配 |
| `QEMU_FAIL_UDS_RESET` | 42 | EcuReset 未触发 Mcu_PerformReset |
| `QEMU_FAIL_UDS_MULTIFRAME` | 43 | 多帧重组失败 |

---

## 5. Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `DCM_QEMU_TEST_MODE` | STD_ON | 启用 Dcm QEMU 测试配置 |
| `P2_SERVER_MAX` | 50 | P2 超时（ms） |
| `S3_SERVER` | 5000 | S3 超时（ms） |

---

## 6. Scenarios

### Scenario 1: ReadDataByIdentifier_F190
**Description:** 注入 `22 F1 90`（VIN DID），Dcm 响应 `62 F1 90 <VIN[17]>`
**Flow:**
1. 初始化 Dcm + CanIf
2. 构造 UDS 请求帧 `[22 F1 90]`
3. 调用 `CanIf_RxIndication` 注入
4. 调用 `Dcm_MainFunction()` 处理
5. 检查响应帧
**Expected Result:** 响应 CAN ID == 0x7E8，data[0] == 0x62，data[1..2] == 0xF1 0x90

### Scenario 2: EcuReset_SoftReset
**Description:** 注入 `11 03`，触发 Mcu_PerformReset
**Flow:**
1. 注入请求 `[11 03]`
2. Dcm 处理
3. 检查 `Mcu_PerformReset` stub 被调用
**Expected Result:** `Qemu_Assert(reset_called_count == 1, "EcuReset")`

### Scenario 3: NegativeResponse_ServiceNotSupported
**Description:** 注入不存在的 SID `0xFF`，响应 `7F FF 11`
**Flow:**
1. 注入请求 `[FF]`
2. Dcm 处理
3. 检查响应帧
**Expected Result:** data[0] == 0x7F，data[1] == 0xFF，data[2] == 0x11

### Scenario 4: MultiFrameIsoTp
**Description:** CanTp 多帧重组 + 多帧响应
**Flow:**
1. 构造超过 8 字节的 UDS 请求
2. CanTp 分片注入（FF + CF 序列）
3. Dcm 处理
4. 验证多帧响应
**Expected Result:** 收到连续帧序列，重组后数据正确

---

## 7. Dependencies

### Upper Layer Modules
- 无

### Lower Layer Modules
- **Dcm.c** (`src/bsw/services/dcm/src/Dcm.c`): 生产代码
- **CanTp.c**: 传输协议分片重组
- **CanIf.c**: CAN 接口
- **C4 can-loopback**: CAN 软件回环依赖

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-08-22 | YuleTech | Initial UDS inject verification specification |
