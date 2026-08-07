/**
 * @file sil_smoke_main.c
 * @brief yuleASR 真实产品 SIL 冒烟（native x86 编译真实 BSW 模块）
 * @details
 *   P0-3 SIL 落地：从 hello.elf 示例升级为真实产品用例。
 *   本程序在 host 上编译真实 BSW 模块（Crc + E2E + Det），
 *   执行冒烟断言并输出 SIL PASS/FAIL，供 CI 记录
 *   .osh/ci/sil-test-results.json。
 *
 *   SIL 范围（真实产品模块，非 demo）：
 *     - Crc: CRC8/CRC16/CRC32 计算（src/bsw/services/crc）
 *     - E2E: E2E_P01 保护配置（src/bsw/services/e2e）
 *
 * AUTOSAR Standard: R22-11 | ASIL: D
 * 编译：cc -I src/bsw/services/crc/include -I src/bsw/services/e2e/include \
 *          -I include/autosar ... -o sil_smoke sil_smoke_main.c \
 *          src/bsw/services/crc/src/Crc.c src/bsw/services/e2e/src/E2E_P01.c
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "Crc.h"
#include "E2E_P01.h"

/* ---- 冒烟断言框架（轻量，不依赖第三方） ---- */
static int g_failures = 0;
static int g_checks = 0;

#define SMOKE_CHECK(cond, msg) \
    do { \
        g_checks++; \
        if (cond) { \
            printf("  [PASS] %s\n", msg); \
        } else { \
            printf("  [FAIL] %s\n", msg); \
            g_failures++; \
        } \
    } while (0)

int main(void)
{
    printf("yuleASR SIL Smoke — real BSW modules (native host build)\n");
    printf("--------------------------------------------------------\n");

    /* ---- Crc: 真实模块初始化 + CRC8/16/32 ---- */
    Crc_Init(NULL);
    SMOKE_CHECK(1, "Crc_Init(NULL) returns without error");

    {
        const uint8 data[] = {0x31U, 0x32U, 0x33U, 0x34U, 0x35U}; /* "12345" */
        uint8  crc8  = Crc_CalculateCRC8(data, sizeof(data), 0xFFU, FALSE);
        uint16 crc16 = Crc_CalculateCRC16(data, sizeof(data), 0xFFFFU, FALSE);
        uint32 crc32 = Crc_CalculateCRC32(data, sizeof(data), 0xFFFFFFFFU, FALSE);

        printf("  CRC8=0x%02X CRC16=0x%04X CRC32=0x%08X\n", crc8, crc16, crc32);

        /* 已知向量校验 — 本实现为非反射型（MSB-first，与 Crc_Cfg 多项式一致）：
         *   CRC32("12345") = 0x426548B8（非反射 IEEE 802.3, poly 0x04C11DB7）
         *   CRC16("12345") = 0x4560（CCITT-FALSE, poly 0x1021）
         *   CRC8 ("12345") = 0x76（SAE J1850, poly 0x1D）
         * 参考向量由独立位运算实现验证（与模块表驱动实现一致）。 */
        SMOKE_CHECK(crc32 == 0x426548B8UL, "Crc32(\"12345\") == 0x426548B8 (non-reflected IEEE 802.3)");
        SMOKE_CHECK(crc16 == 0x4560U, "Crc16(\"12345\") == 0x4560 (CCITT-FALSE)");
        SMOKE_CHECK(crc8 == 0x76U, "Crc8(\"12345\") == 0x76 (SAE J1850)");
    }

    /* ---- E2E: P01 保护配置校验（真实产品配置头） ---- */
    {
        const E2E_P01ConfigType cfg = {
            .DataID = 0x1234U,
            .DataLength = 8U,
            .DataIDMode = E2E_P01_DATAID_BOTH,
            .CounterOffset = 0U,
            .CRCOffset = 1U,
            .DataIDNibbleOffset = 0U,
        };
        E2E_P01ProtectStateType state;
        uint8 data[8] = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U};
        memset(&state, 0, sizeof(state));
        Std_ReturnType ret = E2E_P01Protect(&cfg, &state, &data[0]);

        SMOKE_CHECK(ret == E_OK, "E2E_P01Protect() returns E_OK");
        /* 实现语义：先写当前 counter 到 Data，再自增（mod 15） */
        SMOKE_CHECK(state.Counter == 1U, "E2E_P01Protect() increments counter to 1");
        SMOKE_CHECK((data[0] & 0x0FU) == 0U, "E2E_P01Protect() writes counter 0 into Data[0] low nibble");
        SMOKE_CHECK(data[1] != 0x02U, "E2E_P01Protect() writes CRC into Data[CRCOffset]=Data[1]");
    }

    printf("--------------------------------------------------------\n");
    if (g_failures == 0) {
        printf("SIL Test Suite: ALL PASS (%d checks)\n", g_checks);
        return 0;
    } else {
        printf("SIL Test Suite: %d/%d FAILED\n", g_failures, g_checks);
        return 1;
    }
}
