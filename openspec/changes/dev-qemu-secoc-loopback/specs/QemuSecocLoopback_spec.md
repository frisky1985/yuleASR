# QemuSecocLoopback (QEMU SecOC 回环验证) Module Specification

> **Module:** QemuSecocLoopback (QEMU SecOC 回环验证)  
> **Layer:** Test Infrastructure  
> **Standard:** AUTOSAR Classic Platform 4.4.0 / AUTOSAR SWS_SecOC  
> **Platform:** QEMU mps2-an521 (Cortex-M33)  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

QemuSecocLoopback 在 QEMU mps2-an521 上构建 SecOC 发送 → CAN loopback → SecOC 接收的完整回环路径，验证 yuleASR SecOC 模块（`src/bsw/services/secoc/`）的 CMAC 鉴权路径端到端运行。SecOC 验证链路为：发送方生成 CMAC → 附加到 PDU → 接收方验证 CMAC → 通过/拒绝。当前仅有单元测试验证孤立函数（`SecOC_VerifyParsedAuthenticatedPdu` → `Csm_MacVerify` → CSM/Crypto），缺少全栈回环验证。由于 QEMU 无硬件 HSM，使用软件 AES-128 CMAC 桩。

### Key Responsibilities
- 构建 SecOC 发送 → CAN loopback → SecOC 接收的完整回环路径
- 验证 SecOC 发送方 CMAC 生成（`SecOC_IfTransmit`）
- 验证 SecOC 接收方 CMAC 验证（`SecOC_RxIndication`）
- 验证合法 PDU（正确 CMAC）被接受（`SECOC_AUTHPDU_ACCEPTED`）
- 验证篡改 PDU（错误 CMAC）被拒绝（`SECOC_AUTHPDU_REJECTED`）
- 验证 FreshnessValue（防重放）单调性检查机制生效
- 输出 `SECOC_LOOPBACK_PASS` 标记供 CI 自动判定

---

## 2. API List

### 2.1 SecOC APIs Under Test

| API | Description |
|-----|-------------|
| `void SecOC_Init(const SecOC_ConfigType *Config)` | SecOC 初始化 |
| `Std_ReturnType SecOC_IfTransmit(PduIdType TxPduId, const PduInfoType *PduInfo)` | 发送鉴权 PDU（附加 CMAC + FreshnessValue 截断） |
| `void SecOC_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfo)` | 接收并验证鉴权 PDU（分离 CMAC + FreshnessValue） |
| `void SecOC_MainFunctionRx(void)` | 接收主函数 |
| `void SecOC_MainFunctionTx(void)` | 发送主函数 |

### 2.2 CSM Stub APIs (Software Implementation)

| API | Called By | Description |
|-----|-----------|-------------|
| `Std_ReturnType Csm_MacGenerate(uint32 jobId, ...)` | SecOC | 软件桩：AES-128-CMAC 生成 |
| `Std_ReturnType Csm_MacVerify(uint32 jobId, ...)` | SecOC | 软件桩：AES-128-CMAC 验证，绕过 Csm 配置层 |

### 2.3 FreshnessValue Management Stub APIs

| API | Called By | Description |
|-----|-----------|-------------|
| `void FvM_GetTxFreshnessValue(uint16 id, uint8 *buf, uint8 len)` | SecOC 发送方 | 获取发送侧 FreshnessValue |
| `void FvM_GetRxFreshnessValue(uint16 id, uint8 truncFV, uint8 *buf, uint8 len)` | SecOC 接收方 | 获取接收侧 FreshnessValue（单调性检查） |
| `void FvM_UpdateCounter(uint16 id)` | SecOC 发送方 | 发送后单调递增计数器 |

### 2.4 Verification Result APIs

| API | Called By | Description |
|-----|-----------|-------------|
| `void Qemu_ReportPass(void)` | 验证入口 | 输出 `SECOC_LOOPBACK_PASS` 并调用 semihosting `SYS_EXIT(0)` |
| `void Qemu_ReportFail(const char *reason)` | 验证入口 | 输出失败原因并调用 semihosting `SYS_EXIT(1)` |

---

## 3. Data Types

### 3.1 SecOC Verification Result Enum

```c
typedef enum {
    SECOC_AUTHPDU_IDLE      = 0x00,
    SECOC_AUTHPDU_ACCEPTED  = 0x01,  /* CMAC 验证通过 */
    SECOC_AUTHPDU_REJECTED  = 0x02   /* CMAC 验证失败 */
} SecOC_AuthPduResultType;
```

### 3.2 CAN PDU / CMAC Constants

```c
#define SECOC_TEST_KEY_LEN      16U     /* AES-128 密钥长度（字节） */
#define SECOC_CMAC_TRUNC_LEN    4U      /* 截断 CMAC 长度（字节） */
#define SECOC_FV_TRUNC_LEN      4U      /* 截断 FreshnessValue 长度（字节） */
#define SECOC_TEST_PDU_LEN      8U      /* CAN 帧 DLC（固定 8 字节） */
#define SECOC_PAYLOAD_MAX_LEN   4U     /* 最大 payload 长度（DLC - CMAC） */
```

### 3.3 FreshnessValue Defines

```c
#define FRESHNESS_VALUE_INIT   0u       /* FreshnessValue 初始值 */
#define FRESHNESS_VALUE_INC    1u       /* 每帧递增步长 */
```

### 3.4 Test Key Defines

```c
/* 硬编码 AES-128 CMAC 测试密钥（仅测试环境，非生产密钥） */
static const uint8 SecOC_TestKey[16] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};
```

### 3.5 Loopback Counters

```c
volatile uint32_t SECOC_ACCEPTED_COUNT = 0;   /* 合法帧接受计数 */
volatile uint32_t SECOC_REJECTED_COUNT = 0;   /* 篡改帧拒绝计数 */
volatile uint32_t REPLAY_REJECTED_COUNT = 0;  /* 重放帧拒绝计数 */
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
| `SECOC_TEST_PDU_LEN` | 8 | 固定 CAN DLC（字节） |
| `SECOC_CMAC_TRUNC_LEN` | 4 | CMAC 截断长度（字节） |
| `SECOC_FV_TRUNC_LEN` | 4 | FreshnessValue 截断长度（字节） |
| `SECOC_LOOPBACK_FRAME_COUNT` | 10 | 连续回环帧数（验收 S8.4） |
| `SECOC_FRESHNESS_CHECK_ENABLE` | STD_ON | 启用 FreshnessValue 防重放检查 |
| `SECOC_TEST_MODE` | 1 | 启用硬编码测试密钥，绕过密钥管理 |
| `QEMU_TARGET` | 1 | QEMU 目标平台标识 |
| `QEMU_TIMEOUT` | 30 | CI 脚本超时秒数（环境变量） |
| `QEMU_MACHINE` | mps2-an521 | QEMU 机器类型 |
| `QEMU_CPU` | cortex-m33 | QEMU CPU 型号 |

### Build Configuration

```cmake
add_executable(qemu_secoc_loopback
    p3c_secoc_loopback/main_secoc_loopback.c
    p3c_secoc_loopback/secoc_crypto_stub.c
)
target_link_libraries(qemu_secoc_loopback SecOC PduR CanIf Os libs_aes)
target_compile_definitions(qemu_secoc_loopback PRIVATE QEMU_TARGET=1 SECOC_TEST_MODE=1)
```

---

## 6. Scenarios

### Scenario S8.1: ValidPduAccepted
**Description:** 合法 PDU（CMAC 匹配）被 SecOC 接收方接受
**Flow:**
1. 初始化 SecOC、CAN loopback 与 CSM 桩
2. 输入 4 字节数据 + 正确 AES-128-CMAC（4 字节截断）+ 当前 FreshnessValue
3. 调用 `SecOC_IfTransmit` 发送，CAN loopback 回环
4. 调用 `SecOC_RxIndication` 接收并验证
**Expected Result:** `Csm_MacVerify` 返回 `E_OK`；触发 `SECOC_AUTHPDU_ACCEPTED` 回调；`accepted_count == 1`（验收 S8.1）

### Scenario S8.2: TamperedPduRejected
**Description:** 篡改 PDU（CMAC 最后 1 字节翻转）被 SecOC 接收方拒绝
**Flow:**
1. 初始化 SecOC 与 CAN loopback
2. 同 S8.1 输入，但 CMAC 最后字节 XOR 0xFF（翻转）
3. 调用 `SecOC_IfTransmit` 发送，CAN loopback 回环
4. 调用 `SecOC_RxIndication` 接收并验证
**Expected Result:** `Csm_MacVerify` 返回 `E_NOT_OK`；触发 `SECOC_AUTHPDU_REJECTED` 回调；`rejected_count == 1`（验收 S8.2）

### Scenario S8.3: ReplayAttackBlocked
**Description:** 重放攻击（FreshnessValue 不变的重发帧）被 SecOC 验证失败
**Flow:**
1. 初始化 SecOC 与 FreshnessValue 管理
2. 发送合法 PDU 并记录当前 FreshnessValue
3. 保持 FreshnessValue 不变，重发已接受的帧
4. 调用 `SecOC_RxIndication` 验证重放帧
**Expected Result:** `FvM_GetRxFreshnessValue` 因单调性失败；SecOC 验证拒绝；`replay_rejected_count == 1`（验收 S8.3）

### Scenario S8.4: ContinuousLoopbackBatch
**Description:** 10 帧连续回环，合法帧全通过，篡改帧全拒绝，无误判
**Flow:**
1. 初始化 SecOC 与 CAN loopback
2. 循环 10 次：5 帧合法 + 5 帧篡改，交替发送
3. 统计 `SECOC_AUTHPDU_ACCEPTED` 与 `SECOC_AUTHPDU_REJECTED` 计数
4. 检查无误判
**Expected Result:** `accepted_count == 5`，`rejected_count == 5`；无误判（accepted + rejected == 10）（验收 S8.4）

---

## 7. Dependencies

### Upper Layer Modules
- CI `run_qemu_test.sh`：通过 exit code 与 `SECOC_LOOPBACK_PASS` 标记判定结果
- QemuAssert 基础设施：`Qemu_ReportPass` / `Qemu_ReportFail`

### Lower Layer Modules
- **SecOC**: `SecOC_Init` / `SecOC_IfTransmit` / `SecOC_RxIndication` / `SecOC_MainFunctionRx` / `SecOC_MainFunctionTx` / `SecOC_AuthPduResultType`
- **CSM/Crypto**: `Csm_MacGenerate` / `Csm_MacVerify`（软件桩实现 AES-128-CMAC）
- **FvM**: `FvM_GetTxFreshnessValue` / `FvM_GetRxFreshnessValue` / `FvM_UpdateCounter`（RAM 模拟，单调递增）
- **PduR / CanIf**: CAN PDU 路由与接口层
- **CAN loopback**: 复用 dev-qemu-can-loopback 的 QEMU CAN loopback 基础设施
- **libs_aes**: 软件 AES-128 库
- **Uart_Cfg**: CMSDK UART 驱动（复用 `tests/qemu_m33/src/Uart_Cfg.c`）

### External Dependencies
- **QEMU**: mps2-an521 机器类型，需启用 `--semihosting-config enable=on,target=native`
- **arm-none-eabi-gcc**: 交叉编译工具链

### CI Integration

```bash
# run_qemu_test.sh 中追加
run_test "qemu_secoc_loopback" "SECOC_LOOPBACK_PASS"
```

---

## 8. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-08-22 | YuleTech | Initial QEMU SecOC loopback verification specification |
