# remaining-errors.md — yuleASR v1.3.0 编译错误跟踪

> 本文件记录 yuleASR v1.3.0 分支编译错误的修复进度与真实实现方案。
> 修复 commit：`8323821`、`cb28c1e`、`37a6629`、`4961a17` + 本轮（真实化）。

## 状态：✅ 编译全绿（宿主 + ARM 交叉编译）

- `cmake --build build -j4`（宿主 aarch64）：**0 error**
- `cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake build-arm`（**ARM Cortex-M7**，`-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard`）：**0 error**
- 单元测试 `tests/unit/test_yuleasr_monitor.py`：6/6 通过

---

## 真实实现替换明细（本轮，按老板验收标准）

### 1. mbedTLS：自建 stub 头 → 真实库（Mbed-TLS 3.6.2 LTS）
- 原自建 `third_party/crypto/mbedtls/include/mbedtls/*.h`（16 个声明级 stub）**已删除**。
- 下载并 vendoring **真实 mbedTLS 3.6.2 源码**到 `third_party/mbedtls/`（含官方 `framework/` 子模块），通过 `add_subdirectory` 编译出真实静态库：
  - `libmbedtls.a` / `libmbedx509.a` / `libmbedcrypto.a`
- 真实库**链接**到消费方：`mcal_crypto`、`service_mqtt`、`boot`（`target_link_libraries`）。
- 统一配置 `third_party/mbedtls/configs/yuleasr_config.h`（基于官方默认全功能配置）：
  - `MBEDTLS_ALLOW_PRIVATE_ACCESS`（后端访问文档化内部结构）
  - `MBEDTLS_NO_PLATFORM_ENTROPY`（裸机熵源由平台 TRNG 提供）
  - `#undef MBEDTLS_HAVE_TIME(_DATE)` / `MBEDTLS_TIMING_C` / `MBEDTLS_NET_C`（无 RTC/OS 定时器，TLS 超时由 MQTT 状态机处理；网络由 TcpIp 层提供）
- 代码适配 3.x API（真实调用真实符号）：
  - `mbedtls_sha256_ret` → `mbedtls_sha256`（3.x 更名，签名相同）
  - `mbedtls_ecdsa_setup` → `mbedtls_ecp_group_load(&ctx.grp, curve)`（3.x 移除 setup）
  - `MBEDTLS_OID_CMP`（3.x 移除）→ 本地 `MQTT_OID_CMP`（按字符串 OID 宏真实比较）；OID 宏名对齐 3.x（`MBEDTLS_OID_AT_ORGANIZATION` / `MBEDTLS_OID_AT_STATE`）
  - 补齐 `mbedtls/error.h`、`mbedtls/net_sockets.h` 真实头 include

### 2. MCAL 中断控制：空声明 → 真实实现（新 `src/bsw/mcal/mcu/src/Mcal.c`）
- `Mcal_DisableAllInterrupts()`：Cortex-M7 `cpsid i`（置 PRIMASK）；aarch64 宿主 `msr daifset, #2`
- `Mcal_EnableAllInterrupts()`：Cortex-M7 `cpsie i`（清 PRIMASK）；aarch64 `msr daifclr, #2`
- `Mcal_ResetSystem()`：Cortex-M7 写 `SCB->AIRCR = 0x05FA0000 | SYSRESETREQ`（S32K312 SCB 基址 0xE000ED00）
- 已编译进 `libmcal_mcu.a`（ARM 与宿主均含符号）

### 3. SchM 临界区：空宏 → 真实中断屏蔽
- `SchM_Enter_*()` → `Mcal_DisableAllInterrupts()`；`SchM_Exit_*()` → `Mcal_EnableAllInterrupts()`
- 涉及：SchM_SecOC / SchM_Uart / SchM_Fee / SchM_DoIP / SchM_Mem

### 4. 寄存器访问宏：no-op → 真实解引用
- `REG_READ32(addr)` → `(*(volatile uint32*)(addr))`；`REG_WRITE32(addr,val)` → 真实写
- 与 `Std_Types.h` / `Compiler.h` 的真实定义统一（去掉 autosar/Mcal.h 的 0U 兜底）

### 5. 类型缺陷修复（ARM GCC 16 默认 `-Werror=incompatible-pointer-types` 暴露）
- `Std_Types.h`：移除对标准类型名 `int8_t..uint64_t` 的非法 typedef（归 `<stdint.h>` 负责）；受影响文件显式 `#include <stdint.h>`
- `Crypto_Aes.c`：AES/GCM/CCM 调用处显式 `(const uint8*)/(uint8*)` 字节指针转换（真实语义：crypto 数据是字节流）
- `Crypto_Cfg.c` / `Crypto_MbedTLS.c`：config/输出指针类型对齐
- `EthTrcv_Lcfg.c` / `LinTrcv_Lcfg.c`：InterfaceConfig/ChannelConfig 数组类型与头文件对齐

### 6. 裸机交叉编译支撑（无 newlib 环境）
- `third_party/freestanding-include/`：标准 C99 头（stdint/stddef/stdbool/stdarg/string/stdlib/stdio/time/limits/errno/inttypes/assert/unistd/signal/setjmp + sys/types|stat|time、dirent）— 真实标准声明，实现由量产链接阶段（newlib/picolibc）提供
- 仅 `CMAKE_CROSSCOMPILING` 时加入 include 路径

## 遗留说明（非编译阻塞）
- 集成测试 `tests/integration/` 依赖 `.yuleosh/evidence-bundle` CI 产物，本地未生成（与编译无关）
- 量产链接阶段需提供：newlib/picolibc 实现、平台 TRNG 熵源、Flash 寄存器按 S32K312 实际基址确认（当前为驱动默认值）、TcpIp/lwIP 后端
