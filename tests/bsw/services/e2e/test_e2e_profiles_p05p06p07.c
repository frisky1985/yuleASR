/**
 * @file test_e2e_profiles_p05p06p07.c
 * @brief E2E Profile 5/6/7 unit tests
 * @details Covers E2E_P05/P06/P07 Protect + Check + MapStatusToSM against the
 *          production implementation in src/bsw/services/e2e.
 *
 *          Profile 5: CRC-64/ECMA + 32-bit counter, fixed DataLength.
 *          Profile 6: CRC-64/ECMA + 16-bit counter, dynamic length.
 *          Profile 7: CRC-32/IEEE  +  8-bit counter, dynamic length.
 *
 *          These profiles delegate CRC computation to the Crc service
 *          (Crc_CalculateCRC64 / Crc_CalculateCRC32), so the test target links
 *          service_crc in addition to service_e2e.
 */

#include <string.h>
#include "unity.h"
#include "E2E.h"
#include "E2E_P05.h"
#include "E2E_P06.h"
#include "E2E_P07.h"

/* ──────────────────────────────────────────────────────────────────────────
 * Common frame buffer (large enough for every config below)
 * ────────────────────────────────────────────────────────────────────────── */
static uint8 frame[64];

/* ──────────────────────────────────────────────────────────────────────────
 * Profile 5 configs
 *   aligned:   CounterOffset=0  (bits  0..31, bytes 0..3)
 *              CRCOffset=32     (bits 32..95, bytes 4..11)
 *   unaligned: CounterOffset=4  (bits  4..35, bytes 0..4)
 *              CRCOffset=36     (bits 36..99, bytes 4..12)
 * ────────────────────────────────────────────────────────────────────────── */
static const E2E_P05ConfigType cfgP05_aligned = {
    16U,            /* DataLength          */
    0x12345678U,    /* DataID              */
    0U,             /* CounterOffset       */
    32U,            /* CRCOffset           */
    0U,             /* DataIDOffset (unused)*/
    3U,             /* MaxDeltaCounterInit */
    TRUE            /* IncludeDataID       */
};

static const E2E_P05ConfigType cfgP05_unaligned = {
    16U, 0xABCDEF01U, 4U, 36U, 0U, 3U, FALSE
};

static E2E_P05ProtectStateType ps5;
static E2E_P05CheckStateType   cs5;

/* ──────────────────────────────────────────────────────────────────────────
 * Profile 6 configs (CRCOffset must be byte-aligned — WriteCRC/ReadCRC are
 * aligned-only in P06).
 * ────────────────────────────────────────────────────────────────────────── */
static const E2E_P06ConfigType cfgP06_aligned = {
    0x11223344U,    /* DataID              */
    0U,             /* CounterOffset       */
    16U,            /* CRCOffset (bytes 2..9) */
    0U,             /* LengthOffset (unused) */
    3U,             /* MaxDeltaCounterInit */
    4U,             /* MinDataLength       */
    64U,            /* MaxDataLength       */
    TRUE            /* IncludeDataID       */
};

static const E2E_P06ConfigType cfgP06_unaligned = {
    0x55667788U, 4U, 24U, 0U, 3U, 4U, 64U, FALSE
};

/* MaxDataLength > 256 so the Check-side `Length > 256` guard is reachable. */
static const E2E_P06ConfigType cfgP06_long = {
    0U, 0U, 0U, 0U, 3U, 1U, 300U, FALSE
};

static E2E_P06ProtectStateType ps6;
static E2E_P06CheckStateType   cs6;

/* ──────────────────────────────────────────────────────────────────────────
 * Profile 7 configs
 * ────────────────────────────────────────────────────────────────────────── */
static const E2E_P07ConfigType cfgP07_aligned = {
    0xDEADBEEFU,    /* DataID              */
    0U,             /* CounterOffset       */
    8U,             /* CRCOffset (bytes 1..4) */
    3U,             /* MaxDeltaCounterInit */
    5U,             /* MinDataLength       */
    64U,            /* MaxDataLength       */
    TRUE            /* IncludeDataID       */
};

static const E2E_P07ConfigType cfgP07_unaligned = {
    0xCAFEBABEU, 4U, 16U, 3U, 5U, 64U, FALSE
};

static const E2E_P07ConfigType cfgP07_long = {
    0U, 0U, 0U, 3U, 1U, 300U, FALSE
};

static E2E_P07ProtectStateType ps7;
static E2E_P07CheckStateType   cs7;

/* ────────────────────────────────────────────────────────────────────────── */

void setUp(void)
{
    memset(&ps5, 0, sizeof(ps5));
    memset(&cs5, 0, sizeof(cs5));
    memset(&ps6, 0, sizeof(ps6));
    memset(&cs6, 0, sizeof(cs6));
    memset(&ps7, 0, sizeof(ps7));
    memset(&cs7, 0, sizeof(cs7));
    memset(frame, 0, sizeof(frame));
    frame[14] = 0xABU;   /* payload (within all 16-byte frames) */
    frame[15] = 0xCDU;
}

void tearDown(void)
{
}

/* Build a valid protected P05 frame carrying the requested counter. */
static void p05_makeFrame(const E2E_P05ConfigType* cfg, uint32 counter)
{
    ps5.Counter = counter;
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P05Protect(cfg, &ps5, frame));
}

/* Build a valid protected P06 frame of the given length/counter. */
static void p06_makeFrame(const E2E_P06ConfigType* cfg, uint16 counter, uint32 length)
{
    ps6.Counter = counter;
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P06Protect(cfg, &ps6, frame, length));
}

/* Build a valid protected P07 frame of the given length/counter. */
static void p07_makeFrame(const E2E_P07ConfigType* cfg, uint8 counter, uint32 length)
{
    ps7.Counter = counter;
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P07Protect(cfg, &ps7, frame, length));
}

/* Shared MapStatusToSM checker — all three profiles share identical mapping. */
typedef void (*MapFn)(E2E_PCheckStatusType, E2E_SMStateType*, boolean*);

static void check_mapper(MapFn map)
{
    E2E_SMStateType sm;
    boolean error;

    sm = (E2E_SMStateType)0xFF; error = TRUE;
    map(E2E_P_OK, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_VALID, sm);
    TEST_ASSERT_FALSE(error);

    map(E2E_P_OKSOMELOST, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_VALID, sm);
    TEST_ASSERT_FALSE(error);

    map(E2E_P_WRONGCRC, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_INVALID, sm);
    TEST_ASSERT_TRUE(error);

    map(E2E_P_WRONGSEQUENCE, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_INVALID, sm);
    TEST_ASSERT_TRUE(error);

    map(E2E_P_REPEATED, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_INVALID, sm);
    TEST_ASSERT_TRUE(error);

    map(E2E_P_SYNC, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_INIT, sm);
    TEST_ASSERT_FALSE(error);

    map(E2E_P_INITIAL, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_NODATA, sm);
    TEST_ASSERT_FALSE(error);

    /* Unknown status -> default branch (INVALID, no error) */
    map((E2E_PCheckStatusType)0x7F, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_INVALID, sm);
    TEST_ASSERT_FALSE(error);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Profile 5
 * ══════════════════════════════════════════════════════════════════════════ */

static void test_P05_Protect_NullArgs(void)
{
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P05Protect(NULL_PTR, &ps5, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P05Protect(&cfgP05_aligned, NULL_PTR, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P05Protect(&cfgP05_aligned, &ps5, NULL_PTR));
}

static void test_P05_Check_NullArgs(void)
{
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P05Check(NULL_PTR, &cs5, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P05Check(&cfgP05_aligned, NULL_PTR, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P05Check(&cfgP05_aligned, &cs5, NULL_PTR));
}

/* Aligned config round-trip exercises aligned WriteCounter/ReadCounter/WriteCRC/ReadCRC
 * and the IncludeDataID == TRUE branch. */
static void test_P05_Aligned_RoundTripAndStateMachine(void)
{
    cs5.MaxDeltaCounterInit = 3U;

    p05_makeFrame(&cfgP05_aligned, 10U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P05Check(&cfgP05_aligned, &cs5, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_INITIAL, cs5.Status);

    p05_makeFrame(&cfgP05_aligned, 10U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P05Check(&cfgP05_aligned, &cs5, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_REPEATED, cs5.Status);

    p05_makeFrame(&cfgP05_aligned, 11U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P05Check(&cfgP05_aligned, &cs5, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OK, cs5.Status);
    TEST_ASSERT_EQUAL_UINT32(11U, cs5.LastValidCounter);

    p05_makeFrame(&cfgP05_aligned, 14U);   /* delta 3 within tolerance */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P05Check(&cfgP05_aligned, &cs5, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OKSOMELOST, cs5.Status);
    TEST_ASSERT_EQUAL_UINT8(2U, cs5.LostData);

    p05_makeFrame(&cfgP05_aligned, 20U);   /* delta 6 > tolerance -> SYNC */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P05Check(&cfgP05_aligned, &cs5, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_SYNC, cs5.Status);

    /* Backward counter exercises the wrap-around delta calculation branch. */
    p05_makeFrame(&cfgP05_aligned, 5U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P05Check(&cfgP05_aligned, &cs5, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_SYNC, cs5.Status);
}

/* Unaligned config round-trip exercises unaligned WriteCounter/ReadCounter/
 * WriteCRC/ReadCRC and the IncludeDataID == FALSE branch. */
static void test_P05_Unaligned_RoundTripAndCorrupt(void)
{
    cs5.MaxDeltaCounterInit = 3U;

    p05_makeFrame(&cfgP05_unaligned, 1U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P05Check(&cfgP05_unaligned, &cs5, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_INITIAL, cs5.Status);

    p05_makeFrame(&cfgP05_unaligned, 2U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P05Check(&cfgP05_unaligned, &cs5, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OK, cs5.Status);

    /* Corrupt a payload byte -> WRONGCRC (unaligned ReadCRC still reads correctly) */
    p05_makeFrame(&cfgP05_unaligned, 3U);
    frame[14] ^= 0xFFU;
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P05Check(&cfgP05_unaligned, &cs5, frame));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_WRONGCRC, cs5.Status);
}

static void test_P05_MapStatusToSM(void)
{
    check_mapper(E2E_P05MapStatusToSM);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Profile 6
 * ══════════════════════════════════════════════════════════════════════════ */

static void test_P06_Protect_NullArgs(void)
{
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P06Protect(NULL_PTR, &ps6, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P06Protect(&cfgP06_aligned, NULL_PTR, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P06Protect(&cfgP06_aligned, &ps6, NULL_PTR, 16U));
}

static void test_P06_Protect_InvalidLength(void)
{
    /* Length below MinDataLength -> E2E_E_INPUTERR_WRONG */
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_WRONG, E2E_P06Protect(&cfgP06_aligned, &ps6, frame, 2U));
    /* Length above MaxDataLength -> E2E_E_INPUTERR_WRONG */
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_WRONG, E2E_P06Protect(&cfgP06_aligned, &ps6, frame, 100U));
}

static void test_P06_Check_NullArgs(void)
{
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P06Check(NULL_PTR, &cs6, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P06Check(&cfgP06_aligned, NULL_PTR, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P06Check(&cfgP06_aligned, &cs6, NULL_PTR, 16U));
}

static void test_P06_Check_InvalidLength_ReportsWrongCrc(void)
{
    /* Check-side length validation sets Status = WRONGCRC but returns E_OK. */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P06Check(&cfgP06_aligned, &cs6, frame, 2U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_WRONGCRC, cs6.Status);
}

static void test_P06_Check_LengthOver256_InputErrWrong(void)
{
    /* MaxDataLength(300) accepts 260, so the explicit `Length > 256` guard fires. */
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_WRONG,
                            E2E_P06Check(&cfgP06_long, &cs6, frame, 260U));
}

static void test_P06_Aligned_RoundTripAndStateMachine(void)
{
    cs6.MaxDeltaCounterInit = 3U;

    p06_makeFrame(&cfgP06_aligned, 10U, 16U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P06Check(&cfgP06_aligned, &cs6, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_INITIAL, cs6.Status);

    p06_makeFrame(&cfgP06_aligned, 10U, 16U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P06Check(&cfgP06_aligned, &cs6, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_REPEATED, cs6.Status);

    p06_makeFrame(&cfgP06_aligned, 11U, 16U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P06Check(&cfgP06_aligned, &cs6, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OK, cs6.Status);
    TEST_ASSERT_EQUAL_UINT16(11U, cs6.LastValidCounter);

    p06_makeFrame(&cfgP06_aligned, 14U, 16U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P06Check(&cfgP06_aligned, &cs6, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OKSOMELOST, cs6.Status);
    TEST_ASSERT_EQUAL_UINT8(2U, cs6.LostData);

    p06_makeFrame(&cfgP06_aligned, 20U, 16U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P06Check(&cfgP06_aligned, &cs6, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_SYNC, cs6.Status);

    p06_makeFrame(&cfgP06_aligned, 5U, 16U);   /* wrap-around delta */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P06Check(&cfgP06_aligned, &cs6, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_SYNC, cs6.Status);
}

static void test_P06_Unaligned_RoundTripAndCorrupt(void)
{
    cs6.MaxDeltaCounterInit = 3U;

    p06_makeFrame(&cfgP06_unaligned, 1U, 16U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P06Check(&cfgP06_unaligned, &cs6, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_INITIAL, cs6.Status);

    p06_makeFrame(&cfgP06_unaligned, 2U, 16U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P06Check(&cfgP06_unaligned, &cs6, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OK, cs6.Status);

    p06_makeFrame(&cfgP06_unaligned, 3U, 16U);
    frame[14] ^= 0xFFU;
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P06Check(&cfgP06_unaligned, &cs6, frame, 16U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_WRONGCRC, cs6.Status);
}

static void test_P06_MapStatusToSM(void)
{
    check_mapper(E2E_P06MapStatusToSM);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Profile 7
 * ══════════════════════════════════════════════════════════════════════════ */

static void test_P07_Protect_NullArgs(void)
{
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P07Protect(NULL_PTR, &ps7, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P07Protect(&cfgP07_aligned, NULL_PTR, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P07Protect(&cfgP07_aligned, &ps7, NULL_PTR, 8U));
}

static void test_P07_Protect_InvalidLength(void)
{
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_WRONG, E2E_P07Protect(&cfgP07_aligned, &ps7, frame, 2U));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_WRONG, E2E_P07Protect(&cfgP07_aligned, &ps7, frame, 100U));
}

static void test_P07_Check_NullArgs(void)
{
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P07Check(NULL_PTR, &cs7, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P07Check(&cfgP07_aligned, NULL_PTR, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_P07Check(&cfgP07_aligned, &cs7, NULL_PTR, 8U));
}

static void test_P07_Check_InvalidLength_ReportsWrongCrc(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P07Check(&cfgP07_aligned, &cs7, frame, 2U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_WRONGCRC, cs7.Status);
}

static void test_P07_Check_LengthOver256_InputErrWrong(void)
{
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_WRONG,
                            E2E_P07Check(&cfgP07_long, &cs7, frame, 260U));
}

static void test_P07_Aligned_RoundTripAndStateMachine(void)
{
    cs7.MaxDeltaCounterInit = 3U;

    p07_makeFrame(&cfgP07_aligned, 10U, 8U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P07Check(&cfgP07_aligned, &cs7, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_INITIAL, cs7.Status);

    p07_makeFrame(&cfgP07_aligned, 10U, 8U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P07Check(&cfgP07_aligned, &cs7, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_REPEATED, cs7.Status);

    p07_makeFrame(&cfgP07_aligned, 11U, 8U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P07Check(&cfgP07_aligned, &cs7, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OK, cs7.Status);
    TEST_ASSERT_EQUAL_UINT8(11U, cs7.LastValidCounter);

    p07_makeFrame(&cfgP07_aligned, 14U, 8U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P07Check(&cfgP07_aligned, &cs7, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OKSOMELOST, cs7.Status);
    TEST_ASSERT_EQUAL_UINT8(2U, cs7.LostData);

    p07_makeFrame(&cfgP07_aligned, 20U, 8U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P07Check(&cfgP07_aligned, &cs7, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_SYNC, cs7.Status);

    p07_makeFrame(&cfgP07_aligned, 5U, 8U);    /* wrap-around delta */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P07Check(&cfgP07_aligned, &cs7, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_SYNC, cs7.Status);
}

static void test_P07_Unaligned_RoundTripAndCorrupt(void)
{
    cs7.MaxDeltaCounterInit = 3U;

    p07_makeFrame(&cfgP07_unaligned, 1U, 8U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P07Check(&cfgP07_unaligned, &cs7, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_INITIAL, cs7.Status);

    p07_makeFrame(&cfgP07_unaligned, 2U, 8U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P07Check(&cfgP07_unaligned, &cs7, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OK, cs7.Status);

    p07_makeFrame(&cfgP07_unaligned, 3U, 8U);
    frame[6] ^= 0xFFU;     /* payload byte within 8-byte frame */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P07Check(&cfgP07_unaligned, &cs7, frame, 8U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_WRONGCRC, cs7.Status);
}

static void test_P07_MapStatusToSM(void)
{
    check_mapper(E2E_P07MapStatusToSM);
}

/* ══════════════════════════════════════════════════════════════════════════ */

int main(void)
{
    UNITY_BEGIN();

    /* Profile 5 */
    RUN_TEST(test_P05_Protect_NullArgs);
    RUN_TEST(test_P05_Check_NullArgs);
    RUN_TEST(test_P05_Aligned_RoundTripAndStateMachine);
    RUN_TEST(test_P05_Unaligned_RoundTripAndCorrupt);
    RUN_TEST(test_P05_MapStatusToSM);

    /* Profile 6 */
    RUN_TEST(test_P06_Protect_NullArgs);
    RUN_TEST(test_P06_Protect_InvalidLength);
    RUN_TEST(test_P06_Check_NullArgs);
    RUN_TEST(test_P06_Check_InvalidLength_ReportsWrongCrc);
    RUN_TEST(test_P06_Check_LengthOver256_InputErrWrong);
    RUN_TEST(test_P06_Aligned_RoundTripAndStateMachine);
    RUN_TEST(test_P06_Unaligned_RoundTripAndCorrupt);
    RUN_TEST(test_P06_MapStatusToSM);

    /* Profile 7 */
    RUN_TEST(test_P07_Protect_NullArgs);
    RUN_TEST(test_P07_Protect_InvalidLength);
    RUN_TEST(test_P07_Check_NullArgs);
    RUN_TEST(test_P07_Check_InvalidLength_ReportsWrongCrc);
    RUN_TEST(test_P07_Check_LengthOver256_InputErrWrong);
    RUN_TEST(test_P07_Aligned_RoundTripAndStateMachine);
    RUN_TEST(test_P07_Unaligned_RoundTripAndCorrupt);
    RUN_TEST(test_P07_MapStatusToSM);

    return UnityEnd();
}
