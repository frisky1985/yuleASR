/** @file test_e2e_protection.c
 * @brief Unit tests for AUTOSAR E2E Protection library (e2e_protection.c)
 *
 * Covers: module/context lifecycle, all 8 profiles (P01/P02/P04/P05/P06/P07/
 * P11/P22) Protect->Check round trips, CRC tamper detection, counter
 * REPEATED/WRONGSEQUENCE semantics, parameter guards, CRC primitives against
 * independent bit-wise reference implementations, CRC table initializers and
 * utility helpers.
 *
 * NOTE (actual-behavior annotations, verified against the implementation):
 *  - P01_Check returns E_NOT_OK on a length violation, while P02..P22 Check
 *    return E_OK with *status == E2E_ERROR_LENGTH.
 *  - In P02/P06/P07/P11/P22 Check the WRONGSEQUENCE branch assigns
 *    *status = E2E_ERROR_SEQUENCE, but the common tail overwrites it with
 *    E2E_ERROR_NONE; only state->status == E2E_P_WRONGSEQUENCE is observable
 *    (REPEATED returns early and IS observable via *status).
 *  - Profile counter fields are 4-bit (masked 0x0F) for P02/P06/P07 and
 *    8-bit/16-bit for P11/P22.
 *  - P11/P22 report a Data-ID mismatch as E2E_ERROR_INIT (not a dedicated code).
 *  - PRODUCT DEFECT (recorded, not fixed): for P06/P07 the Protect CRC input
 *    range is [0, dataLength) which INCLUDES the CRC field when
 *    crcOffset < dataLength; Protect then overwrites those bytes, so Check can
 *    never reproduce the CRC. Tests therefore configure crcOffset == dataLength
 *    (CRC appended after the covered range), the only workable layout.
 *  - PRODUCT DEFECT (recorded, not fixed): E2E_InitCRC8Table/E2E_InitCRC16Table/
 *    E2E_InitCRC32Table are declared in e2e_protection.h but NOT implemented in
 *    e2e_protection.c (only the private E2E_InitCRCTables() exists). Tests do
 *    not reference the missing symbols; the internal tables are validated
 *    indirectly through the CRC primitives against bit-wise references.
 *  - P22_Check increments messageCounter like P22_Protect does, so one
 *    Protect + one Check yields messageCounter == 2.
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */
#include "unity.h"
#include "e2e_protection.h"
#include <string.h>

/* ==========================================================================
 * Helpers
 * ========================================================================== */

static void FillPattern(uint8_t *buf, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++) {
        buf[i] = (uint8_t)(0x10U + (i & 0x0FU));
    }
}

/* Independent bit-wise reference CRC8: SAE J1850, poly 0x1D, MSB first,
 * no reflection, no xor-out (matches E2E_CalculateCRC8 semantics). */
static uint8_t RefCrc8(const uint8_t *data, uint32_t len, uint8_t crc)
{
    uint32_t i;
    int b;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (b = 0; b < 8; b++) {
            crc = (crc & 0x80U) ? (uint8_t)((uint8_t)(crc << 1) ^ 0x1DU)
                                : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* Independent bit-wise reference CRC16: CCITT-FALSE, poly 0x1021, MSB first,
 * init/xor per caller (matches E2E_CalculateCRC16 semantics). */
static uint16_t RefCrc16(const uint8_t *data, uint32_t len, uint16_t crc)
{
    uint32_t i;
    int b;
    for (i = 0; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (b = 0; b < 8; b++) {
            crc = (crc & 0x8000U) ? (uint16_t)((uint16_t)(crc << 1) ^ 0x1021U)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/* Configure one context for the given profile (config must be set AFTER
 * E2E_InitContext because it memsets the whole context). */
static void SetupProfile(E2E_ContextType *ctx, uint8_t profile)
{
    TEST_ASSERT_EQUAL(E_OK, E2E_Protection_Init());
    TEST_ASSERT_EQUAL(E_OK, E2E_InitContext(ctx, profile));

    switch (profile) {
        case E2E_PROFILE_01:
            ctx->config.p01.dataId = 0x1234U;
            ctx->config.p01.dataLength = 8U;
            ctx->config.p01.crcOffset = 0U;
            break;
        case E2E_PROFILE_02:
            ctx->config.p02.dataId = 0x00ABU;
            ctx->config.p02.dataLength = 8U;
            ctx->config.p02.crcOffset = 0U;
            ctx->config.p02.counterOffset = 7U;
            break;
        case E2E_PROFILE_04:
            ctx->config.p04.dataId = 0xDEADBEEFU;
            ctx->config.p04.dataLength = 8U;
            ctx->config.p04.crcOffset = 8U;
            break;
        case E2E_PROFILE_05:
            ctx->config.p05.dataId = 0x3210U;
            ctx->config.p05.dataLength = 8U;
            ctx->config.p05.crcOffset = 8U;
            break;
        case E2E_PROFILE_06:
            /* NOTE: crcOffset must equal dataLength (CRC appended) — with
             * crcOffset < dataLength the Protect CRC input covers its own
             * CRC field, making Check unable to ever match (impl defect).
             * counterOffset must also sit BEHIND the 2-byte CRC field:
             * Protect writes the counter first and the CRC second, so an
             * overlapping counterOffset would be clobbered by the CRC. */
            ctx->config.p06.dataId = 0x0055U;
            ctx->config.p06.dataLength = 8U;
            ctx->config.p06.crcOffset = 8U;
            ctx->config.p06.counterOffset = 10U;
            break;
        case E2E_PROFILE_07:
            ctx->config.p07.dataId = 0x11223344U;
            ctx->config.p07.dataLength = 8U;
            ctx->config.p07.crcOffset = 8U;
            ctx->config.p07.counterOffset = 12U;
            break;
        case E2E_PROFILE_11:
            ctx->config.p11.dataId = 0x00ABU;
            ctx->config.p11.dataLength = 8U;
            ctx->config.p11.maxDataLength = 16U;
            ctx->config.p11.minDataLength = 4U;
            ctx->config.p11.crcOffset = 8U;    /* CRC appended after payload */
            ctx->config.p11.counterOffset = 2U;
            ctx->config.p11.dataIdOffset = 3U;
            break;
        case E2E_PROFILE_22:
            ctx->config.p22.dataId = 0x00C3U;
            ctx->config.p22.dataLength = 32U;
            ctx->config.p22.maxDataLength = 64U;
            ctx->config.p22.minDataLength = 16U;
            ctx->config.p22.crcOffset = 32U;   /* CRC after CRC-covered range */
            ctx->config.p22.counterOffset = 4U;
            ctx->config.p22.dataIdOffset = 6U;
            ctx->config.p22.lengthOffset = 8U;
            ctx->config.p22.includeLengthInCrc = true;
            break;
        default:
            break;
    }
}

void setUp(void)
{
    /* Start every test from the module-uninitialized state */
    (void)E2E_Deinit();
}

void tearDown(void)
{
}

/* ==========================================================================
 * Module / context lifecycle
 * ========================================================================== */

static void test_e2e_lifecycle(void)
{
    TEST_ASSERT_FALSE(E2E_IsInitialized());
    TEST_ASSERT_EQUAL(E_OK, E2E_Protection_Init());
    TEST_ASSERT_TRUE(E2E_IsInitialized());
    /* Idempotent re-init */
    TEST_ASSERT_EQUAL(E_OK, E2E_Protection_Init());
    TEST_ASSERT_TRUE(E2E_IsInitialized());
    TEST_ASSERT_EQUAL(E_OK, E2E_Deinit());
    TEST_ASSERT_FALSE(E2E_IsInitialized());
    /* Deinit twice is harmless */
    TEST_ASSERT_EQUAL(E_OK, E2E_Deinit());
}

static void test_e2e_not_initialized_rejects_api(void)
{
    E2E_ContextType ctx;
    uint8_t buf[16] = {0};
    uint32_t len = 8U;
    uint16_t status = 0xFFFFU;

    /* Context creation does not require module init */
    TEST_ASSERT_EQUAL(E_OK, E2E_InitContext(&ctx, E2E_PROFILE_01));
    ctx.config.p01.dataId = 0x1234U;
    ctx.config.p01.dataLength = 8U;
    ctx.config.p01.crcOffset = 0U;
    TEST_ASSERT_FALSE(E2E_IsInitialized());
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01_Protect(&ctx, buf, &len));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01_Check(&ctx, buf, 8U, &status));
}

static void test_e2e_initcontext_invalid(void)
{
    E2E_ContextType ctx;
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_InitContext(NULL, E2E_PROFILE_01));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_InitContext(&ctx, 0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_InitContext(&ctx, 0x99U));
}

static void test_e2e_initcontext_deinitcontext(void)
{
    E2E_ContextType ctx;
    TEST_ASSERT_EQUAL(E_OK, E2E_Protection_Init());
    TEST_ASSERT_EQUAL(E_OK, E2E_InitContext(&ctx, E2E_PROFILE_02));
    TEST_ASSERT_TRUE(ctx.initialized);
    TEST_ASSERT_EQUAL_UINT32(0x45324521U, ctx.initMagic);
    TEST_ASSERT_EQUAL_UINT8(E2E_PROFILE_02, ctx.profile);
    /* Profile-specific defaults */
    TEST_ASSERT_EQUAL_UINT8(1U, ctx.state.p02.maxDeltaCounter);
    TEST_ASSERT_FALSE(ctx.state.p02.synced);

    TEST_ASSERT_EQUAL(E_OK, E2E_DeinitContext(&ctx));
    TEST_ASSERT_FALSE(ctx.initialized);
    TEST_ASSERT_EQUAL_UINT32(0U, ctx.initMagic);
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_DeinitContext(NULL));
}

static void test_e2e_wrong_profile_rejected(void)
{
    E2E_ContextType ctx;
    uint8_t buf[16] = {0};
    uint32_t len = 8U;
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_01);
    /* P02 API against a P01 context must be refused */
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P02_Protect(&ctx, buf, &len));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P02_Check(&ctx, buf, 8U, &status));
}

/* ==========================================================================
 * CRC primitives
 * ========================================================================== */

static void test_e2e_crc8_reference_vectors(void)
{
    static const uint8_t vec[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    uint8_t one = 0x00U;
    uint8_t ff = 0xFFU;

    /* Library (builtin J1850 table) must match the bit-wise reference */
    TEST_ASSERT_EQUAL_HEX8(RefCrc8(vec, 9U, 0xFFU),
                           E2E_CalculateCRC8(vec, 9U, 0xFFU, NULL));
    TEST_ASSERT_EQUAL_HEX8(RefCrc8(&one, 1U, 0xFFU),
                           E2E_CalculateCRC8(&one, 1U, 0xFFU, NULL));
    TEST_ASSERT_EQUAL_HEX8(RefCrc8(&ff, 1U, 0xFFU),
                           E2E_CalculateCRC8(&ff, 1U, 0xFFU, NULL));
    /* Known check value for CRC-8/SAE J1850 (poly 0x1D, init 0xFF) */
    TEST_ASSERT_EQUAL_HEX8(0xB4U, E2E_CalculateCRC8(vec, 9U, 0xFFU, NULL));
}

static void test_e2e_crc8_zero_length(void)
{
    uint8_t dummy = 0U;
    /* Zero-length input returns the seed unchanged */
    TEST_ASSERT_EQUAL_HEX8(0xFFU, E2E_CalculateCRC8(&dummy, 0U, 0xFFU, NULL));
    TEST_ASSERT_EQUAL_HEX8(0x5AU, E2E_CalculateCRC8(&dummy, 0U, 0x5AU, NULL));
}

static void test_e2e_crc8_builtin_table_is_consistent(void)
{
    /* No public table-init API exists (see defect note in file header), so the
     * builtin J1850 table is validated through its results only */
    uint8_t data[32];
    uint32_t i;

    FillPattern(data, sizeof(data));
    /* The stream checksum must equal the bit-wise reference for every prefix */
    for (i = 1; i <= sizeof(data); i++) {
        TEST_ASSERT_EQUAL_HEX8(RefCrc8(data, i, 0xFFU),
                               E2E_CalculateCRC8(data, i, 0xFFU, NULL));
    }
}

static void test_e2e_crc16_reference_vectors(void)
{
    static const uint8_t vec[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    uint8_t data[16];

    (void)E2E_Protection_Init();   /* initializes g_crc16Table */

    TEST_ASSERT_EQUAL_HEX16(RefCrc16(vec, 9U, 0xFFFFU),
                            E2E_CalculateCRC16(vec, 9U, 0xFFFFU));
    /* Known check value for CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) */
    TEST_ASSERT_EQUAL_HEX16(0x29B1U, E2E_CalculateCRC16(vec, 9U, 0xFFFFU));

    FillPattern(data, sizeof(data));
    TEST_ASSERT_EQUAL_HEX16(RefCrc16(data, sizeof(data), 0xFFFFU),
                            E2E_CalculateCRC16(data, sizeof(data), 0xFFFFU));
    /* Zero-length returns the seed */
    TEST_ASSERT_EQUAL_HEX16(0xFFFFU, E2E_CalculateCRC16(data, 0U, 0xFFFFU));
    TEST_ASSERT_EQUAL_HEX16(0x1234U, E2E_CalculateCRC16(data, 0U, 0x1234U));
}

static void test_e2e_crc32_zero_length_and_sensitivity(void)
{
    uint8_t a = 0x00U;
    uint8_t b = 0x01U;

    (void)E2E_Protection_Init();

    /* crc = init ^ XOR_OUT for zero length: 0xFFFFFFFF ^ 0xFFFFFFFF = 0 */
    TEST_ASSERT_EQUAL_HEX32(0x00000000U,
                            E2E_CalculateCRC32(&a, 0U, 0xFFFFFFFFU));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, E2E_CalculateCRC32(&a, 0U, 0U));

    /* Deterministic and input-sensitive */
    TEST_ASSERT_EQUAL_HEX32(E2E_CalculateCRC32(&a, 1U, 0xFFFFFFFFU),
                            E2E_CalculateCRC32(&a, 1U, 0xFFFFFFFFU));
    TEST_ASSERT_NOT_EQUAL(E2E_CalculateCRC32(&a, 1U, 0xFFFFFFFFU),
                          E2E_CalculateCRC32(&b, 1U, 0xFFFFFFFFU));
}

/* ==========================================================================
 * Profile 1 (CRC8)
 * ========================================================================== */

static void test_p01_roundtrip(void)
{
    E2E_ContextType ctx;
    uint8_t buf[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_01);
    memset(buf, 0, sizeof(buf));
    FillPattern(&buf[1], 7);                 /* byte 0 is the CRC slot */

    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P01_Protect(&ctx, buf, &len));
    TEST_ASSERT_EQUAL_UINT32(8U, len);

    TEST_ASSERT_EQUAL(E_OK, E2E_P01_Check(&ctx, buf, len, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    TEST_ASSERT_EQUAL(E2E_P_OK, ctx.state.p01.status);

    /* CRC actually depends on the payload: tamper -> CRC error */
    buf[3] ^= 0xFFU;
    TEST_ASSERT_EQUAL(E_OK, E2E_P01_Check(&ctx, buf, len, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_CRC, status);
    TEST_ASSERT_EQUAL(E2E_P_ERROR, ctx.state.p01.status);
}

static void test_p01_two_segment_crc(void)
{
    E2E_ContextType ctx;
    uint8_t buf[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_01);
    ctx.config.p01.crcOffset = 2U;           /* CRC covers [0,2) + [3,8) */
    memset(buf, 0, sizeof(buf));
    FillPattern(&buf[1], 7);

    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P01_Protect(&ctx, buf, &len));
    TEST_ASSERT_EQUAL_UINT32(8U, len);
    TEST_ASSERT_EQUAL(E_OK, E2E_P01_Check(&ctx, buf, len, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);

    buf[5] ^= 0x55U;                          /* tamper second segment */
    TEST_ASSERT_EQUAL(E_OK, E2E_P01_Check(&ctx, buf, len, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_CRC, status);
}

static void test_p01_length_and_null_guards(void)
{
    E2E_ContextType ctx;
    uint8_t buf[16] = {0};
    uint32_t len = 8U;
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_01);

    /* NOTE: P01_Check returns E_NOT_OK on length violation (unlike P02+) */
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01_Check(&ctx, buf, 4U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_LENGTH, status);
    TEST_ASSERT_EQUAL(E2E_P_ERROR, ctx.state.p01.status);

    /* NULL guards */
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01_Protect(NULL, buf, &len));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01_Protect(&ctx, NULL, &len));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01_Protect(&ctx, buf, NULL));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01_Check(NULL, buf, 8U, &status));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01_Check(&ctx, NULL, 8U, &status));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01_Check(&ctx, buf, 8U, NULL));
}

/* ==========================================================================
 * Profile 2 (CRC8 + 4-bit counter)
 * ========================================================================== */

static void test_p02_roundtrip_sequence(void)
{
    E2E_ContextType sender, receiver;
    uint8_t b1[16], b2[16], b3[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;
    int i;

    /* Separate TX/RX contexts, as in real AUTOSAR usage */
    SetupProfile(&sender, E2E_PROFILE_02);
    SetupProfile(&receiver, E2E_PROFILE_02);
    for (i = 0; i < 3; i++) {
        uint8_t *buf = (i == 0) ? b1 : ((i == 1) ? b2 : b3);
        memset(buf, 0, 16);
        FillPattern(&buf[1], 7);
        len = 0U;
        TEST_ASSERT_EQUAL(E_OK, E2E_P02_Protect(&sender, buf, &len));
        TEST_ASSERT_EQUAL_UINT32(8U, len);
        TEST_ASSERT_EQUAL_UINT8((uint8_t)i, buf[sender.config.p02.counterOffset]);
        TEST_ASSERT_EQUAL(E_OK, E2E_P02_Check(&receiver, buf, len, &status));
        TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    }
    TEST_ASSERT_EQUAL(E2E_P_OK, receiver.state.p02.status);
    TEST_ASSERT_TRUE(receiver.state.p02.synced);
}

static void test_p02_repeated(void)
{
    E2E_ContextType sender, receiver;
    uint8_t buf[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;

    SetupProfile(&sender, E2E_PROFILE_02);
    SetupProfile(&receiver, E2E_PROFILE_02);
    memset(buf, 0, sizeof(buf));
    FillPattern(&buf[1], 7);
    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Protect(&sender, buf, &len));
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Check(&receiver, buf, len, &status));   /* sync */
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);

    /* Same frame again -> REPEATED */
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Check(&receiver, buf, len, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_COUNTER, status);
    TEST_ASSERT_EQUAL(E2E_P_REPEATED, receiver.state.p02.status);
}

static void test_p02_wrong_sequence(void)
{
    E2E_ContextType sender, receiver;
    uint8_t b1[16], b2[16], b3[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;
    int i;

    SetupProfile(&sender, E2E_PROFILE_02);
    SetupProfile(&receiver, E2E_PROFILE_02);
    for (i = 0; i < 3; i++) {
        uint8_t *buf = (i == 0) ? b1 : ((i == 1) ? b2 : b3);
        memset(buf, 0, 16);
        FillPattern(&buf[1], 7);
        len = 0U;
        TEST_ASSERT_EQUAL(E_OK, E2E_P02_Protect(&sender, buf, &len));
    }
    /* Sync with counter 1, then accept counter 2 */
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Check(&receiver, b2, 8U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Check(&receiver, b3, 8U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);

    /* Backwards counter (0 after 2) -> WRONGSEQUENCE.
     * NOTE (impl quirk): the WRONGSEQUENCE branch sets *status =
     * E2E_ERROR_SEQUENCE but the common tail overwrites it with
     * E2E_ERROR_NONE; only state->status is observable. */
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Check(&receiver, b1, 8U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    TEST_ASSERT_EQUAL(E2E_P_WRONGSEQUENCE, receiver.state.p02.status);

    /* Jump of 3 > maxDeltaCounter(1) -> WRONGSEQUENCE as well */
    memset(b1, 0, 16);
    FillPattern(&b1[1], 7);
    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Protect(&sender, b1, &len));   /* counter 3 */
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Check(&receiver, b1, len, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    TEST_ASSERT_EQUAL(E2E_P_WRONGSEQUENCE, receiver.state.p02.status);
}

static void test_p02_crc_tamper_and_length(void)
{
    E2E_ContextType sender, receiver;
    uint8_t buf[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;

    SetupProfile(&sender, E2E_PROFILE_02);
    SetupProfile(&receiver, E2E_PROFILE_02);
    memset(buf, 0, sizeof(buf));
    FillPattern(&buf[1], 7);
    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Protect(&sender, buf, &len));

    buf[3] ^= 0xFFU;
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Check(&receiver, buf, 8U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_CRC, status);
    TEST_ASSERT_EQUAL(E2E_P_ERROR, receiver.state.p02.status);

    /* NOTE: P02+ Check return E_OK with LENGTH status (P01 returns E_NOT_OK) */
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Check(&receiver, buf, 4U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_LENGTH, status);
}

static void test_p02_counter_wraparound(void)
{
    E2E_ContextType sender;
    uint8_t buf[16];
    uint32_t len;
    int i;

    SetupProfile(&sender, E2E_PROFILE_02);
    for (i = 0; i < 16; i++) {
        memset(buf, 0, sizeof(buf));
        FillPattern(&buf[1], 7);
        len = 0U;
        TEST_ASSERT_EQUAL(E_OK, E2E_P02_Protect(&sender, buf, &len));
        /* 4-bit counter: 0..15 written, then wraps to 0 */
        TEST_ASSERT_EQUAL_UINT8((uint8_t)(i & 0x0FU), buf[sender.config.p02.counterOffset]);
    }
    TEST_ASSERT_EQUAL_UINT8(0U, sender.state.p02.counter);

    /* 17th frame reuses counter value 0 */
    memset(buf, 0, sizeof(buf));
    FillPattern(&buf[1], 7);
    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P02_Protect(&sender, buf, &len));
    TEST_ASSERT_EQUAL_UINT8(0U, buf[sender.config.p02.counterOffset]);
}

/* ==========================================================================
 * Profile 4 (CRC32)
 * ========================================================================== */

static void test_p04_roundtrip_tamper_length(void)
{
    E2E_ContextType ctx;
    uint8_t buf[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_04);
    memset(buf, 0, sizeof(buf));
    FillPattern(buf, 8);

    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P04_Protect(&ctx, buf, &len));
    TEST_ASSERT_EQUAL_UINT32(12U, len);       /* 8 data + 4 CRC */

    TEST_ASSERT_EQUAL(E_OK, E2E_P04_Check(&ctx, buf, len, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    TEST_ASSERT_EQUAL(E2E_P_OK, ctx.state.p04.status);

    buf[5] ^= 0xA5U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P04_Check(&ctx, buf, len, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_CRC, status);

    TEST_ASSERT_EQUAL(E_OK, E2E_P04_Check(&ctx, buf, 8U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_LENGTH, status);
}

/* ==========================================================================
 * Profile 5 (CRC16)
 * ========================================================================== */

static void test_p05_roundtrip_tamper_length(void)
{
    E2E_ContextType ctx;
    uint8_t buf[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_05);
    memset(buf, 0, sizeof(buf));
    FillPattern(buf, 8);

    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P05_Protect(&ctx, buf, &len));
    TEST_ASSERT_EQUAL_UINT32(10U, len);       /* 8 data + 2 CRC */

    TEST_ASSERT_EQUAL(E_OK, E2E_P05_Check(&ctx, buf, len, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);

    buf[2] ^= 0x5AU;
    TEST_ASSERT_EQUAL(E_OK, E2E_P05_Check(&ctx, buf, len, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_CRC, status);

    TEST_ASSERT_EQUAL(E_OK, E2E_P05_Check(&ctx, buf, 9U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_LENGTH, status);
}

/* ==========================================================================
 * Profile 6 (CRC16 + 4-bit counter)
 * ========================================================================== */

static void test_p06_roundtrip_repeated_tamper_length(void)
{
    E2E_ContextType sender, receiver;
    uint8_t b1[16], b2[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;
    int i;

    SetupProfile(&sender, E2E_PROFILE_06);
    SetupProfile(&receiver, E2E_PROFILE_06);
    for (i = 0; i < 2; i++) {
        uint8_t *buf = (i == 0) ? b1 : b2;
        memset(buf, 0, 16);
        FillPattern(buf, 8);
        len = 0U;
        TEST_ASSERT_EQUAL(E_OK, E2E_P06_Protect(&sender, buf, &len));
        TEST_ASSERT_EQUAL_UINT32(12U, len);   /* 8 data + 2 CRC @8 + counter @10 */
        TEST_ASSERT_EQUAL_UINT8((uint8_t)i, buf[sender.config.p06.counterOffset]);
        TEST_ASSERT_EQUAL(E_OK, E2E_P06_Check(&receiver, buf, len, &status));
        TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    }

    /* Frame 2 replayed -> REPEATED */
    TEST_ASSERT_EQUAL(E_OK, E2E_P06_Check(&receiver, b2, 12U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_COUNTER, status);
    TEST_ASSERT_EQUAL(E2E_P_REPEATED, receiver.state.p06.status);

    /* Tamper -> CRC error */
    b1[5] ^= 0x77U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P06_Check(&receiver, b1, 12U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_CRC, status);

    /* Length guard (needs dataLength + 4) */
    TEST_ASSERT_EQUAL(E_OK, E2E_P06_Check(&receiver, b1, 11U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_LENGTH, status);
}

/* ==========================================================================
 * Profile 7 (CRC32 + 4-bit counter)
 * ========================================================================== */

static void test_p07_roundtrip_repeated_tamper_length(void)
{
    E2E_ContextType sender, receiver;
    uint8_t b1[16], b2[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;
    int i;

    SetupProfile(&sender, E2E_PROFILE_07);
    SetupProfile(&receiver, E2E_PROFILE_07);
    for (i = 0; i < 2; i++) {
        uint8_t *buf = (i == 0) ? b1 : b2;
        memset(buf, 0, 16);
        FillPattern(buf, 8);
        len = 0U;
        TEST_ASSERT_EQUAL(E_OK, E2E_P07_Protect(&sender, buf, &len));
        TEST_ASSERT_EQUAL_UINT32(16U, len);
        TEST_ASSERT_EQUAL_UINT8((uint8_t)i, buf[sender.config.p07.counterOffset]);
        TEST_ASSERT_EQUAL(E_OK, E2E_P07_Check(&receiver, buf, len, &status));
        TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    }

    TEST_ASSERT_EQUAL(E_OK, E2E_P07_Check(&receiver, b2, 16U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_COUNTER, status);
    TEST_ASSERT_EQUAL(E2E_P_REPEATED, receiver.state.p07.status);

    b1[6] ^= 0x3CU;
    TEST_ASSERT_EQUAL(E_OK, E2E_P07_Check(&receiver, b1, 16U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_CRC, status);

    TEST_ASSERT_EQUAL(E_OK, E2E_P07_Check(&receiver, b1, 15U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_LENGTH, status);
}

/* ==========================================================================
 * Profile 11 (dynamic CRC8 + sequence)
 * ========================================================================== */

static void test_p11_roundtrip(void)
{
    E2E_ContextType ctx;
    uint8_t buf[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_11);
    memset(buf, 0, sizeof(buf));
    FillPattern(buf, 8);                      /* payload 0..7, CRC at 8 */

    len = 8U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P11_Protect(&ctx, buf, &len));
    TEST_ASSERT_EQUAL_UINT32(9U, len);        /* +1 CRC byte */
    TEST_ASSERT_EQUAL_UINT8(0xABU, buf[3]);   /* Data ID little-endian */
    TEST_ASSERT_EQUAL_UINT8(0x00U, buf[4]);
    TEST_ASSERT_EQUAL_UINT8(0U, buf[2]);      /* counter */

    TEST_ASSERT_EQUAL(E_OK, E2E_P11_Check(&ctx, buf, len, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    TEST_ASSERT_EQUAL(E2E_P_OK, ctx.state.p11.status);
}

static void test_p11_repeated_and_wrong_sequence(void)
{
    E2E_ContextType ctx;
    uint8_t b1[16], b2[16], b3[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;
    int i;

    SetupProfile(&ctx, E2E_PROFILE_11);
    for (i = 0; i < 3; i++) {
        uint8_t *buf = (i == 0) ? b1 : ((i == 1) ? b2 : b3);
        memset(buf, 0, 16);
        FillPattern(buf, 8);
        len = 8U;
        TEST_ASSERT_EQUAL(E_OK, E2E_P11_Protect(&ctx, buf, &len));
    }
    /* Sync on counter 1, accept counter 2, then replay counter 0 backwards */
    TEST_ASSERT_EQUAL(E_OK, E2E_P11_Check(&ctx, b2, 9U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    TEST_ASSERT_EQUAL(E_OK, E2E_P11_Check(&ctx, b3, 9U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);

    /* NOTE (impl quirk): WRONGSEQUENCE leaves *status == E2E_ERROR_NONE */
    TEST_ASSERT_EQUAL(E_OK, E2E_P11_Check(&ctx, b1, 9U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    TEST_ASSERT_EQUAL(E2E_P_WRONGSEQUENCE, ctx.state.p11.status);

    /* Duplicate frame -> REPEATED */
    TEST_ASSERT_EQUAL(E_OK, E2E_P11_Check(&ctx, b1, 9U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_COUNTER, status);
    TEST_ASSERT_EQUAL(E2E_P_REPEATED, ctx.state.p11.status);
}

static void test_p11_dataid_mismatch(void)
{
    E2E_ContextType ctx;
    uint8_t buf[16];
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_11);
    memset(buf, 0, sizeof(buf));
    FillPattern(buf, 8);
    /* Craft frame with a wrong Data ID but a valid CRC (E2E_ERROR_INIT path) */
    buf[2] = 0U;
    buf[3] = 0xCDU;
    buf[4] = 0x00U;
    buf[8] = E2E_CalculateCRC8(buf, 8U, 0xFFU, NULL);

    TEST_ASSERT_EQUAL(E_OK, E2E_P11_Check(&ctx, buf, 9U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_INIT, status);
    TEST_ASSERT_EQUAL(E2E_P_ERROR, ctx.state.p11.status);
}

static void test_p11_length_and_tamper(void)
{
    E2E_ContextType ctx;
    uint8_t buf[16];
    uint32_t len;
    uint16_t status = 0xFFFFU;
    uint8_t copy[16];

    SetupProfile(&ctx, E2E_PROFILE_11);
    memset(buf, 0, sizeof(buf));
    FillPattern(buf, 8);
    len = 8U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P11_Protect(&ctx, buf, &len));

    memcpy(copy, buf, sizeof(copy));
    copy[5] ^= 0x11U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P11_Check(&ctx, copy, 9U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_CRC, status);

    /* Outside [minDataLength, maxDataLength] = [4, 16] */
    TEST_ASSERT_EQUAL(E_OK, E2E_P11_Check(&ctx, buf, 3U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_LENGTH, status);
    TEST_ASSERT_EQUAL(E_OK, E2E_P11_Check(&ctx, buf, 17U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_LENGTH, status);
}

/* ==========================================================================
 * Profile 22 (dynamic CRC16 + 16-bit sequence, large data)
 * ========================================================================== */

static void test_p22_roundtrip(void)
{
    E2E_ContextType ctx;
    uint8_t buf[64];
    uint32_t len;
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_22);
    memset(buf, 0, sizeof(buf));
    FillPattern(&buf[10], 22);                /* payload region 10..31 */

    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Protect(&ctx, buf, &len));
    TEST_ASSERT_EQUAL_UINT32(32U, len);
    TEST_ASSERT_EQUAL_UINT8(0xC3U, buf[6]);   /* Data ID little-endian */
    TEST_ASSERT_EQUAL_UINT8(0x00U, buf[7]);
    TEST_ASSERT_EQUAL_UINT8(32U, buf[8]);     /* length little-endian */
    TEST_ASSERT_EQUAL_UINT8(0U, buf[9]);
    TEST_ASSERT_EQUAL_UINT8(0U, buf[4]);      /* counter big-endian */
    TEST_ASSERT_EQUAL_UINT8(0U, buf[5]);

    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Check(&ctx, buf, 34U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    TEST_ASSERT_EQUAL(E2E_P_OK, ctx.state.p22.status);
    /* NOTE: P22_Check increments messageCounter just like Protect */
    TEST_ASSERT_EQUAL_UINT32(2U, ctx.state.p22.messageCounter);
}

static void test_p22_roundtrip_length_not_in_crc(void)
{
    E2E_ContextType ctx;
    uint8_t buf[64];
    uint32_t len;
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_22);
    ctx.config.p22.includeLengthInCrc = false;
    ctx.config.p22.crcOffset = 30U;           /* CRC covers [0, dataLength-2) */

    memset(buf, 0, sizeof(buf));
    FillPattern(&buf[10], 22);

    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Protect(&ctx, buf, &len));
    TEST_ASSERT_EQUAL_UINT32(32U, len);
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Check(&ctx, buf, 32U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
}

static void test_p22_repeated_and_wrong_sequence(void)
{
    E2E_ContextType ctx;
    uint8_t bA[64], bB[64];
    uint32_t len;
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_22);
    memset(bA, 0, sizeof(bA));
    FillPattern(&bA[10], 22);
    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Protect(&ctx, bA, &len));   /* counter 0 */

    memset(bB, 0, sizeof(bB));
    FillPattern(&bB[10], 22);
    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Protect(&ctx, bB, &len));   /* counter 1 */

    /* Sync with counter 1, then old counter 0 -> delta < 0 -> WRONGSEQUENCE
     * (NOTE impl quirk: *status is overwritten to E2E_ERROR_NONE) */
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Check(&ctx, bB, 34U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Check(&ctx, bA, 34U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    TEST_ASSERT_EQUAL(E2E_P_WRONGSEQUENCE, ctx.state.p22.status);

    /* Replay of last frame -> REPEATED */
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Check(&ctx, bA, 34U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_COUNTER, status);
    TEST_ASSERT_EQUAL(E2E_P_REPEATED, ctx.state.p22.status);
}

static void test_p22_crafted_id_and_length_fields(void)
{
    E2E_ContextType ctx;
    uint8_t buf[64];
    uint16_t status = 0xFFFFU;
    uint16_t crc;

    SetupProfile(&ctx, E2E_PROFILE_22);
    memset(buf, 0, sizeof(buf));
    FillPattern(&buf[10], 22);

    /* Wrong Data ID with otherwise valid CRC -> E2E_ERROR_INIT */
    buf[6] = 0x34U;
    buf[7] = 0x12U;
    crc = (uint16_t)(E2E_CalculateCRC16(buf, 32U, 0xFFFFU) ^ 0x00C3U);
    buf[32] = (uint8_t)(crc >> 8);
    buf[33] = (uint8_t)(crc & 0xFFU);
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Check(&ctx, buf, 34U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_INIT, status);
    TEST_ASSERT_EQUAL(E2E_P_ERROR, ctx.state.p22.status);

    /* Length field above maxDataLength (64) -> E2E_ERROR_LENGTH */
    buf[6] = 0xC3U;
    buf[7] = 0x00U;
    buf[8] = 100U;
    buf[9] = 0x00U;
    crc = (uint16_t)(E2E_CalculateCRC16(buf, 32U, 0xFFFFU) ^ 0x00C3U);
    buf[32] = (uint8_t)(crc >> 8);
    buf[33] = (uint8_t)(crc & 0xFFU);
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Check(&ctx, buf, 34U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_LENGTH, status);
}

static void test_p22_length_guard_and_tamper(void)
{
    E2E_ContextType ctx;
    uint8_t buf[64];
    uint32_t len;
    uint16_t status = 0xFFFFU;
    uint8_t copy[64];

    SetupProfile(&ctx, E2E_PROFILE_22);
    memset(buf, 0, sizeof(buf));
    FillPattern(&buf[10], 22);
    len = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Protect(&ctx, buf, &len));

    /* Frame shorter than E2E_P22_MIN_DATA_LENGTH */
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Check(&ctx, buf, 8U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_LENGTH, status);

    memcpy(copy, buf, sizeof(copy));
    copy[15] ^= 0x99U;                        /* tamper payload */
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_Check(&ctx, copy, 34U, &status));
    TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_CRC, status);
    TEST_ASSERT_EQUAL(E2E_P_ERROR, ctx.state.p22.status);
}

static void test_p22_set_data_length(void)
{
    E2E_ContextType ctxP22, ctxP02;

    SetupProfile(&ctxP22, E2E_PROFILE_22);
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_SetDataLength(&ctxP22, 16U));
    TEST_ASSERT_EQUAL_UINT16(16U, ctxP22.config.p22.dataLength);
    TEST_ASSERT_EQUAL(E_OK, E2E_P22_SetDataLength(&ctxP22, 4096U));
    TEST_ASSERT_EQUAL_UINT16(4096U, ctxP22.config.p22.dataLength);

    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P22_SetDataLength(&ctxP22, 15U));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P22_SetDataLength(&ctxP22, 4097U));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P22_SetDataLength(NULL, 16U));

    SetupProfile(&ctxP02, E2E_PROFILE_02);
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P22_SetDataLength(&ctxP02, 16U));
}

/* ==========================================================================
 * Generic dispatch interface
 * ========================================================================== */

static void test_e2e_generic_dispatch_all_profiles(void)
{
    static const uint8_t profiles[] = {
        E2E_PROFILE_01, E2E_PROFILE_02, E2E_PROFILE_04, E2E_PROFILE_05,
        E2E_PROFILE_06, E2E_PROFILE_07, E2E_PROFILE_11, E2E_PROFILE_22
    };
    E2E_ContextType ctx;
    uint8_t buf[64];
    uint32_t len;
    uint16_t status;
    size_t k;

    for (k = 0; k < sizeof(profiles); k++) {
        SetupProfile(&ctx, profiles[k]);
        memset(buf, 0, sizeof(buf));
        FillPattern(&buf[1], 31);
        /* P11 CRC covers the *length input bytes and appends at crcOffset,
         * so its input length must equal crcOffset (8) */
        len = (profiles[k] == E2E_PROFILE_11) ? 8U : 32U;
        TEST_ASSERT_EQUAL(E_OK, E2E_Protect(&ctx, buf, &len, profiles[k]));
        TEST_ASSERT_TRUE(len > 0U);
        status = 0xFFFFU;
        TEST_ASSERT_EQUAL(E_OK, E2E_Check(&ctx, buf, len, &status, profiles[k]));
        TEST_ASSERT_EQUAL_UINT16(E2E_ERROR_NONE, status);
    }
}

static void test_e2e_generic_dispatch_invalid(void)
{
    E2E_ContextType ctx;
    uint8_t buf[16] = {0};
    uint32_t len = 8U;
    uint16_t status = 0xFFFFU;

    SetupProfile(&ctx, E2E_PROFILE_02);
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_Protect(&ctx, buf, &len, 0x99U));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_Check(&ctx, buf, 8U, &status, 0x99U));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_Protect(NULL, buf, &len, E2E_PROFILE_02));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_Protect(&ctx, NULL, &len, E2E_PROFILE_02));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_Protect(&ctx, buf, NULL, E2E_PROFILE_02));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_Check(NULL, buf, 8U, &status, E2E_PROFILE_02));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_Check(&ctx, NULL, 8U, &status, E2E_PROFILE_02));
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_Check(&ctx, buf, 8U, NULL, E2E_PROFILE_02));
}

/* ==========================================================================
 * Utility helpers
 * ========================================================================== */

static void test_e2e_get_header_size(void)
{
    TEST_ASSERT_EQUAL_UINT32(1U, E2E_GetHeaderSize(E2E_PROFILE_01));
    TEST_ASSERT_EQUAL_UINT32(2U, E2E_GetHeaderSize(E2E_PROFILE_02));
    TEST_ASSERT_EQUAL_UINT32(4U, E2E_GetHeaderSize(E2E_PROFILE_04));
    TEST_ASSERT_EQUAL_UINT32(2U, E2E_GetHeaderSize(E2E_PROFILE_05));
    TEST_ASSERT_EQUAL_UINT32(4U, E2E_GetHeaderSize(E2E_PROFILE_06));
    TEST_ASSERT_EQUAL_UINT32(8U, E2E_GetHeaderSize(E2E_PROFILE_07));
    TEST_ASSERT_EQUAL_UINT32(3U, E2E_GetHeaderSize(E2E_PROFILE_11));
    TEST_ASSERT_EQUAL_UINT32(12U, E2E_GetHeaderSize(E2E_PROFILE_22));
    TEST_ASSERT_EQUAL_UINT32(0U, E2E_GetHeaderSize(0x99U));
}

static void test_e2e_get_profile_name(void)
{
    TEST_ASSERT_EQUAL_STRING("Profile 1 (CRC8)",
                             E2E_GetProfileName(E2E_PROFILE_01));
    TEST_ASSERT_EQUAL_STRING("Profile 2 (CRC8+Counter)",
                             E2E_GetProfileName(E2E_PROFILE_02));
    TEST_ASSERT_EQUAL_STRING("Profile 4 (CRC32)",
                             E2E_GetProfileName(E2E_PROFILE_04));
    TEST_ASSERT_EQUAL_STRING("Profile 5 (CRC16)",
                             E2E_GetProfileName(E2E_PROFILE_05));
    TEST_ASSERT_EQUAL_STRING("Profile 6 (CRC16+Counter)",
                             E2E_GetProfileName(E2E_PROFILE_06));
    TEST_ASSERT_EQUAL_STRING("Profile 7 (CRC32+Counter)",
                             E2E_GetProfileName(E2E_PROFILE_07));
    TEST_ASSERT_EQUAL_STRING("Profile 11 (Dynamic)",
                             E2E_GetProfileName(E2E_PROFILE_11));
    TEST_ASSERT_EQUAL_STRING("Profile 22 (Dynamic Large Data)",
                             E2E_GetProfileName(E2E_PROFILE_22));
    TEST_ASSERT_EQUAL_STRING("Unknown Profile", E2E_GetProfileName(0x99U));
}

static void test_e2e_get_version(void)
{
    TEST_ASSERT_EQUAL_STRING("22.11.0", E2E_GetVersion());
}

static void test_e2e_status_to_string(void)
{
    TEST_ASSERT_EQUAL_STRING("OK", E2E_StatusToString(E2E_P_OK));
    TEST_ASSERT_EQUAL_STRING("REPEATED", E2E_StatusToString(E2E_P_REPEATED));
    TEST_ASSERT_EQUAL_STRING("WRONG_SEQUENCE",
                             E2E_StatusToString(E2E_P_WRONGSEQUENCE));
    TEST_ASSERT_EQUAL_STRING("ERROR", E2E_StatusToString(E2E_P_ERROR));
    TEST_ASSERT_EQUAL_STRING("NOT_AVAILABLE",
                             E2E_StatusToString(E2E_P_NOTAVAILABLE));
    TEST_ASSERT_EQUAL_STRING("NO_NEW_DATA",
                             E2E_StatusToString(E2E_P_NONEWDATA));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN",
                             E2E_StatusToString((E2E_PCheckStatusType)99));
}

static void test_e2e_validate_data_id(void)
{
    TEST_ASSERT_TRUE(E2E_ValidateDataID(0xFFFFU, E2E_PROFILE_01));
    TEST_ASSERT_FALSE(E2E_ValidateDataID(0x10000U, E2E_PROFILE_01));
    TEST_ASSERT_TRUE(E2E_ValidateDataID(0xFFFFU, E2E_PROFILE_02));
    TEST_ASSERT_FALSE(E2E_ValidateDataID(0x10000U, E2E_PROFILE_05));
    TEST_ASSERT_TRUE(E2E_ValidateDataID(0xFFFFFFFFU, E2E_PROFILE_04));
    TEST_ASSERT_TRUE(E2E_ValidateDataID(0x12345678U, E2E_PROFILE_07));
    TEST_ASSERT_FALSE(E2E_ValidateDataID(0x1FFFFU, E2E_PROFILE_11));
    TEST_ASSERT_TRUE(E2E_ValidateDataID(0x1234U, E2E_PROFILE_22));
    TEST_ASSERT_FALSE(E2E_ValidateDataID(1U, 0x99U));
}

static void test_e2e_validate_length(void)
{
    TEST_ASSERT_FALSE(E2E_ValidateLength(0U, E2E_PROFILE_01));
    TEST_ASSERT_FALSE(E2E_ValidateLength(4097U, E2E_PROFILE_01));
    TEST_ASSERT_TRUE(E2E_ValidateLength(256U, E2E_PROFILE_01));
    TEST_ASSERT_FALSE(E2E_ValidateLength(257U, E2E_PROFILE_01));
    TEST_ASSERT_TRUE(E2E_ValidateLength(8U, E2E_PROFILE_02));
    TEST_ASSERT_TRUE(E2E_ValidateLength(4096U, E2E_PROFILE_04));
    TEST_ASSERT_TRUE(E2E_ValidateLength(4096U, E2E_PROFILE_07));
    TEST_ASSERT_FALSE(E2E_ValidateLength(3U, E2E_PROFILE_11));
    TEST_ASSERT_TRUE(E2E_ValidateLength(4U, E2E_PROFILE_11));
    TEST_ASSERT_TRUE(E2E_ValidateLength(256U, E2E_PROFILE_11));
    TEST_ASSERT_FALSE(E2E_ValidateLength(15U, E2E_PROFILE_22));
    TEST_ASSERT_TRUE(E2E_ValidateLength(16U, E2E_PROFILE_22));
    TEST_ASSERT_TRUE(E2E_ValidateLength(4096U, E2E_PROFILE_22));
    TEST_ASSERT_TRUE(E2E_ValidateLength(5U, 0x99U));    /* default: accept */
}

/* ==========================================================================
 * main
 * ========================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_e2e_lifecycle);
    RUN_TEST(test_e2e_not_initialized_rejects_api);
    RUN_TEST(test_e2e_initcontext_invalid);
    RUN_TEST(test_e2e_initcontext_deinitcontext);
    RUN_TEST(test_e2e_wrong_profile_rejected);

    RUN_TEST(test_e2e_crc8_reference_vectors);
    RUN_TEST(test_e2e_crc8_zero_length);
    RUN_TEST(test_e2e_crc8_builtin_table_is_consistent);
    RUN_TEST(test_e2e_crc16_reference_vectors);
    RUN_TEST(test_e2e_crc32_zero_length_and_sensitivity);

    RUN_TEST(test_p01_roundtrip);
    RUN_TEST(test_p01_two_segment_crc);
    RUN_TEST(test_p01_length_and_null_guards);

    RUN_TEST(test_p02_roundtrip_sequence);
    RUN_TEST(test_p02_repeated);
    RUN_TEST(test_p02_wrong_sequence);
    RUN_TEST(test_p02_crc_tamper_and_length);
    RUN_TEST(test_p02_counter_wraparound);

    RUN_TEST(test_p04_roundtrip_tamper_length);
    RUN_TEST(test_p05_roundtrip_tamper_length);
    RUN_TEST(test_p06_roundtrip_repeated_tamper_length);
    RUN_TEST(test_p07_roundtrip_repeated_tamper_length);

    RUN_TEST(test_p11_roundtrip);
    RUN_TEST(test_p11_repeated_and_wrong_sequence);
    RUN_TEST(test_p11_dataid_mismatch);
    RUN_TEST(test_p11_length_and_tamper);

    RUN_TEST(test_p22_roundtrip);
    RUN_TEST(test_p22_roundtrip_length_not_in_crc);
    RUN_TEST(test_p22_repeated_and_wrong_sequence);
    RUN_TEST(test_p22_crafted_id_and_length_fields);
    RUN_TEST(test_p22_length_guard_and_tamper);
    RUN_TEST(test_p22_set_data_length);

    RUN_TEST(test_e2e_generic_dispatch_all_profiles);
    RUN_TEST(test_e2e_generic_dispatch_invalid);

    RUN_TEST(test_e2e_get_header_size);
    RUN_TEST(test_e2e_get_profile_name);
    RUN_TEST(test_e2e_get_version);
    RUN_TEST(test_e2e_status_to_string);
    RUN_TEST(test_e2e_validate_data_id);
    RUN_TEST(test_e2e_validate_length);

    return UNITY_END();
}
