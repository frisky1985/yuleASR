# src/libs — 独立算法库 (Libraries Layer)

> B3-2 (2026-08-09) · 对齐 XMEN `Libraries/` 独立算法库亮点
> (AES / CAN 校验等可复用算法与 BSW 解耦)

## 目标

算法与 BSW 模块解耦：纯 C99 + `stdint.h`，**不依赖** AUTOSAR
`Std_Types.h` / `Det.h` / 配置头，可独立复用、独立单测、可移植到
任意平台（MCU / PC / 工具链）。

## 目录

| 库 | 内容 | CMake target | 单测 target |
|:---|:-----|:-------------|:------------|
| `crc/` | CRC-8 SAE J1850 / CRC-8 AUTOSAR (H2F) / CRC-16 CCITT-FALSE / CRC-16 XMODEM / CRC-32 ISO-HDLC (IEEE 802.3)，流式增量 API | `libs_crc` | `LibCrc_UnitTest` |
| `aes/` | AES-128/192/256 (FIPS-197)：单块加密/解密、ECB、CBC | `libs_aes` | `LibAes_UnitTest` |

## 用法

```c
#include "Lib_Crc.h"
uint32_t crc = Lib_Crc32IsoHdlc(data, len);              /* 一次性 */

/* 流式（分块增量，结果与一次性一致） */
uint32_t state = LIB_CRC32_ISO_HDLC_INIT;
state = Lib_Crc32IsoHdlcUpdate(state, chunk1, len1);
state = Lib_Crc32IsoHdlcUpdate(state, chunk2, len2);
uint32_t crc = state ^ LIB_CRC32_ISO_HDLC_INIT;          /* 收尾 XOR */

#include "Lib_Aes.h"
Lib_AesContextType ctx;
Lib_AesInit(&ctx, key, LIB_AES_KEY_128);
Lib_AesEncryptEcb(&ctx, plaintext, ciphertext, 16u);
```

## 与原内嵌实现的边界（调研结论）

- **Crc 服务** (`src/bsw/services/crc`)：AUTOSAR 标准接口（`Crc_CalculateCRC*`
  带 DET/初始化校验），保持原样；`libs_crc` 提供同一算法的解耦内核，新算法
  或工具链场景直接用 lib，服务层未来可切换到 lib 内核（接口签名不同，需
  适配层，列为后续项）。
- **Crypto/AES** (`src/bsw/mcal/crypto` + `third_party/crypto/aes_modes`)：
  Crypto 驱动依赖 mbedTLS / aes_modes 的 GCM/CCM/CTR 全套模式与 HSM 硬件
  集成，抽取风险大 → 保留原实现；`libs_aes` 提供无依赖的 ECB/CBC 内核，
  用于 bootloader / 安全启动 / 非 Crypto 栈场景。

## 构建与测试

```sh
cmake -DBUILD_TESTING=ON -S . -B build-b3
cmake --build build-b3 --target LibCrc_UnitTest LibAes_UnitTest
ctest --test-dir build-b3 -R "LibCrc|LibAes"
```
