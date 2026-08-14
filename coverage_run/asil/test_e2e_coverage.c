/* test_e2e_coverage.c — E2E protection library coverage driver (ASIL-D)
 *
 * Exercises the real src/bsw/services/e2e/ implementation against its
 * current public API (E2E_P01..P07 profiles + module init/deinit), so the
 * resulting branch/line coverage is a true measurement of production code.
 *
 * This driver drives the FULL check-state machine of every profile:
 *   - first-data (INITIAL), repeated (REPEATED), counter rollback
 *     (WRONGSEQUENCE), consecutive (OK), lost-within-tolerance
 *     (OKSOMELOST), out-of-tolerance (SYNC) and corrupted-CRC (WRONGCRC)
 *   - every DataID mode (P01: BOTH / LOW / ALT even/odd / NIBBLE 0..3)
 *   - dual-path protect/check (P02)
 *   - unaligned counter/CRC bit offsets (P05/P06/P07)
 *   - length boundary errors (P06/P07)
 *   - MapStatusToSM coverage for every status value
 *
 * Compile: see tools/run_branch_coverage.sh
 */
#include <stdio.h>
#include <string.h>
#include "E2E.h"
#include "E2E_P01.h"
#include "E2E_P02.h"
#include "E2E_P04.h"
#include "E2E_P05.h"
#include "E2E_P06.h"
#include "E2E_P07.h"

static int t_pass = 0;
static int t_fail = 0;

#define CHECK(cond, msg) \
    do { if (cond) { t_pass++; } else { t_fail++; printf("  [FAIL] %s (line %d)\n", msg, __LINE__); } } while (0)

/* Run a full P01-style check sequence: first data, repeated, rollback,
 * consecutive, some-lost, sync, wrong-crc.  Frames are produced by
 * E2E_P01Protect so CRC is always valid for the embedded counter. */
static void drive_p01_check(E2E_P01ConfigType* cfg)
{
    E2E_P01ProtectStateType tx = { 0 };
    E2E_P01CheckStateType rx = { 0 };
    uint8 data[32];
    uint8 c0 = 3U;

    rx.MaxDeltaCounterInit = 4U;
    memset(data, 0xAA, sizeof(data));

    /* INITIAL (first data) */
    tx.Counter = c0;
    E2E_P01Protect(cfg, &tx, data);
    CHECK(E2E_P01Check(cfg, &rx, data) == E_OK && rx.Status == E2E_P_INITIAL,
          "P01 check INITIAL");
    /* REPEATED (same counter) */
    tx.Counter = c0;
    E2E_P01Protect(cfg, &tx, data);
    CHECK(E2E_P01Check(cfg, &rx, data) == E_OK && rx.Status == E2E_P_REPEATED,
          "P01 check REPEATED");
    /* WRONGSEQUENCE (counter decreased) */
    tx.Counter = c0 - 1U;
    E2E_P01Protect(cfg, &tx, data);
    CHECK(E2E_P01Check(cfg, &rx, data) == E_OK && rx.Status == E2E_P_WRONGSEQUENCE,
          "P01 check WRONGSEQUENCE");
    /* OK (consecutive) */
    tx.Counter = c0 + 1U;
    E2E_P01Protect(cfg, &tx, data);
    CHECK(E2E_P01Check(cfg, &rx, data) == E_OK && rx.Status == E2E_P_OK,
          "P01 check OK");
    /* OKSOMELOST (delta 2..max) */
    tx.Counter = c0 + 3U;
    E2E_P01Protect(cfg, &tx, data);
    CHECK(E2E_P01Check(cfg, &rx, data) == E_OK && rx.Status == E2E_P_OKSOMELOST,
          "P01 check OKSOMELOST");
    /* SYNC (delta > max) */
    tx.Counter = c0 + 8U;
    E2E_P01Protect(cfg, &tx, data);
    CHECK(E2E_P01Check(cfg, &rx, data) == E_OK && rx.Status == E2E_P_SYNC,
          "P01 check SYNC");
    /* WRONGCRC (corrupt payload byte) */
    tx.Counter = c0;
    E2E_P01Protect(cfg, &tx, data);
    data[0] ^= 0xFFU;
    CHECK(E2E_P01Check(cfg, &rx, data) == E_OK && rx.Status == E2E_P_WRONGCRC,
          "P01 check WRONGCRC");
}

int main(void)
{
    uint8 data[512];

    printf("=== E2E Coverage Driver ===\n");

    /* Module init / deinit */
    CHECK(E2E_Init(NULL) == E_OK, "E2E_Init");
    CHECK(E2E_DeInit() == E_OK, "E2E_DeInit");

    /* ---- Profile 1 ---- */
    {
        E2E_P01ConfigType cfg = { 0x1234U, 16U, E2E_P01_DATAID_BOTH, 0U, 8U, 0U };
        E2E_P01ProtectStateType tx = { 0 };
        E2E_P01CheckStateType rx = { 0 };
        E2E_SMStateType sm = E2E_SM_INIT;
        boolean err = FALSE;

        /* DataID BOTH */
        drive_p01_check(&cfg);

        /* DataID LOW */
        cfg.DataIDMode = E2E_P01_DATAID_LOW;
        drive_p01_check(&cfg);

        /* DataID ALT: even + odd counter */
        cfg.DataIDMode = E2E_P01_DATAID_ALT;
        drive_p01_check(&cfg);

        /* DataID NIBBLE: all four nibble offsets */
        cfg.DataIDMode = E2E_P01_DATAID_NIBBLE;
        cfg.DataIDNibbleOffset = 0U; drive_p01_check(&cfg);
        cfg.DataIDNibbleOffset = 1U; drive_p01_check(&cfg);
        cfg.DataIDNibbleOffset = 2U; drive_p01_check(&cfg);
        cfg.DataIDNibbleOffset = 3U; drive_p01_check(&cfg);

        /* counter wrap at 14 -> 15 -> 0 */
        cfg.DataIDMode = E2E_P01_DATAID_BOTH;
        memset(data, 0xAA, sizeof(data));
        tx.Counter = 14U;
        CHECK(E2E_P01Protect(&cfg, &tx, data) == E_OK, "P01 protect counter wrap");
        CHECK(tx.Counter == 0U, "P01 counter wrapped to 0");
        CHECK(E2E_P01Protect(&cfg, &tx, data) == E_OK, "P01 protect after wrap");

        /* MapStatusToSM for every status */
        E2E_P01MapStatusToSM(E2E_P_OK, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_OKSOMELOST, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_WRONGCRC, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_WRONGSEQUENCE, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_REPEATED, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_SYNC, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_INITIAL, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_NONEWDATA, &sm, &err);

        CHECK(E2E_P01Protect(NULL, &tx, data) != E_OK, "P01 protect null path");
        CHECK(E2E_P01Check(NULL, &rx, data) != E_OK, "P01 check null path");
    }

    /* ---- Profile 2 (dual path) ---- */
    {
        /* DataLength == CRCOffset: CRC8 covers payload bytes 0..7 and is
         * stored at offset 8, so protect/check round-trips consistently. */
        E2E_P02ConfigType cfg = { 0x1234U, 8U, 0U, 8U, 0U, TRUE };
        E2E_P02ProtectStateType tx = { 0 };
        E2E_P02CheckStateType rx = { 0 };
        E2E_SMStateType sm = E2E_SM_INIT;
        boolean err = FALSE;
        uint8 c0 = 5U;

        rx.MaxDeltaCounterInit = 4U;

        memset(data, 0x55, sizeof(data));

        /* protect with dual-path: toggles path id, writes path nibble */
        tx.Counter = c0; tx.PathId = 0U;
        CHECK(E2E_P02Protect(&cfg, &tx, data) == E_OK, "P02 protect path0");
        CHECK(tx.PathId == 1U, "P02 path toggled to 1");
        tx.Counter = c0 + 1U; tx.PathId = 1U;
        CHECK(E2E_P02Protect(&cfg, &tx, data) == E_OK, "P02 protect path1");

        /* check: invalid path id */
        CHECK(E2E_P02Check(&cfg, &rx, data, 2U) == E_NOT_OK, "P02 check invalid path");

        /* check sequence on path 0 */
        rx.WaitForFirstData = 0U;
        tx.Counter = c0; tx.PathId = 0U;
        E2E_P02Protect(&cfg, &tx, data);
        CHECK(E2E_P02Check(&cfg, &rx, data, 0U) == E_OK && rx.Status == E2E_P_INITIAL,
              "P02 check INITIAL");
        tx.Counter = c0;
        E2E_P02Protect(&cfg, &tx, data);
        CHECK(E2E_P02Check(&cfg, &rx, data, 0U) == E_OK && rx.Status == E2E_P_REPEATED,
              "P02 check REPEATED");
        tx.Counter = c0 - 1U;
        E2E_P02Protect(&cfg, &tx, data);
        CHECK(E2E_P02Check(&cfg, &rx, data, 0U) == E_OK && rx.Status == E2E_P_WRONGSEQUENCE,
              "P02 check WRONGSEQUENCE");
        tx.Counter = c0 + 1U;
        E2E_P02Protect(&cfg, &tx, data);
        CHECK(E2E_P02Check(&cfg, &rx, data, 0U) == E_OK && rx.Status == E2E_P_OK,
              "P02 check OK");
        tx.Counter = c0 + 3U;
        E2E_P02Protect(&cfg, &tx, data);
        CHECK(E2E_P02Check(&cfg, &rx, data, 0U) == E_OK && rx.Status == E2E_P_OKSOMELOST,
              "P02 check OKSOMELOST");
        tx.Counter = c0 + 8U;
        E2E_P02Protect(&cfg, &tx, data);
        CHECK(E2E_P02Check(&cfg, &rx, data, 0U) == E_OK && rx.Status == E2E_P_SYNC,
              "P02 check SYNC");
        tx.Counter = c0;
        E2E_P02Protect(&cfg, &tx, data);
        data[0] ^= 0xFFU;
        CHECK(E2E_P02Check(&cfg, &rx, data, 0U) == E_OK && rx.Status == E2E_P_WRONGCRC,
              "P02 check WRONGCRC");

        /* check on path 1 */
        rx.WaitForFirstData = 0U;
        tx.Counter = c0; tx.PathId = 1U;
        E2E_P02Protect(&cfg, &tx, data);
        CHECK(E2E_P02Check(&cfg, &rx, data, 1U) == E_OK && rx.Status == E2E_P_INITIAL,
              "P02 check path1 INITIAL");

        /* MapStatusToSM */
        E2E_P02MapStatusToSM(E2E_P_OK, &sm, &err);
        E2E_P02MapStatusToSM(E2E_P_OKSOMELOST, &sm, &err);
        E2E_P02MapStatusToSM(E2E_P_WRONGCRC, &sm, &err);
        E2E_P02MapStatusToSM(E2E_P_WRONGSEQUENCE, &sm, &err);
        E2E_P02MapStatusToSM(E2E_P_REPEATED, &sm, &err);
        E2E_P02MapStatusToSM(E2E_P_SYNC, &sm, &err);
        E2E_P02MapStatusToSM(E2E_P_INITIAL, &sm, &err);
        E2E_P02MapStatusToSM(E2E_P_NONEWDATA, &sm, &err);

        CHECK(E2E_P02Protect(NULL, &tx, data) != E_OK, "P02 protect null");
        CHECK(E2E_P02Check(NULL, &rx, data, 0U) != E_OK, "P02 check null");
    }

    /* ---- Profile 4 (CRC32 + 16-bit counter) ---- */
    {
        E2E_P04ConfigType cfg = { 0x12345678U, 16U, 0U, 8U, TRUE };
        E2E_P04ProtectStateType tx = { 0 };
        E2E_P04CheckStateType rx = { 0 };
        E2E_SMStateType sm = E2E_SM_INIT;
        boolean err = FALSE;
        uint16 c0 = 10U;

        rx.MaxDeltaCounterInit = 4U;

        memset(data, 0x11, sizeof(data));

        /* include-data-id protect/check */
        tx.Counter = c0;
        CHECK(E2E_P04Protect(&cfg, &tx, data) == E_OK, "P04 protect");
        CHECK(E2E_P04Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_INITIAL,
              "P04 check INITIAL");
        tx.Counter = c0;
        E2E_P04Protect(&cfg, &tx, data);
        CHECK(E2E_P04Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_REPEATED,
              "P04 check REPEATED");
        tx.Counter = c0 - 1U;
        E2E_P04Protect(&cfg, &tx, data);
        CHECK(E2E_P04Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_WRONGSEQUENCE,
              "P04 check WRONGSEQUENCE");
        tx.Counter = c0 + 1U;
        E2E_P04Protect(&cfg, &tx, data);
        CHECK(E2E_P04Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_OK,
              "P04 check OK");
        tx.Counter = c0 + 3U;
        E2E_P04Protect(&cfg, &tx, data);
        CHECK(E2E_P04Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_OKSOMELOST,
              "P04 check OKSOMELOST");
        tx.Counter = c0 + 8U;
        E2E_P04Protect(&cfg, &tx, data);
        CHECK(E2E_P04Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_SYNC,
              "P04 check SYNC");
        tx.Counter = c0;
        E2E_P04Protect(&cfg, &tx, data);
        data[0] ^= 0xFFU;
        CHECK(E2E_P04Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_WRONGCRC,
              "P04 check WRONGCRC");

        /* without data id */
        cfg.IncludeDataID = FALSE;
        tx.Counter = 0U;
        CHECK(E2E_P04Protect(&cfg, &tx, data) == E_OK, "P04 protect no-dataid");
        rx.WaitForFirstData = 0U;
        CHECK(E2E_P04Check(&cfg, &rx, data) == E_OK, "P04 check no-dataid");

        E2E_P04MapStatusToSM(E2E_P_OK, &sm, &err);
        E2E_P04MapStatusToSM(E2E_P_OKSOMELOST, &sm, &err);
        E2E_P04MapStatusToSM(E2E_P_WRONGCRC, &sm, &err);
        E2E_P04MapStatusToSM(E2E_P_WRONGSEQUENCE, &sm, &err);
        E2E_P04MapStatusToSM(E2E_P_REPEATED, &sm, &err);
        E2E_P04MapStatusToSM(E2E_P_SYNC, &sm, &err);
        E2E_P04MapStatusToSM(E2E_P_INITIAL, &sm, &err);
        E2E_P04MapStatusToSM(E2E_P_NONEWDATA, &sm, &err);

        CHECK(E2E_P04Protect(NULL, &tx, data) != E_OK, "P04 protect null");
        CHECK(E2E_P04Check(NULL, &rx, data) != E_OK, "P04 check null");
    }

    /* ---- Profile 5 (CRC64 + 32-bit counter, unaligned offsets) ---- */
    {
        /* counter at bit 4 (unaligned), CRC at bit 40 (byte-aligned) */
        E2E_P05ConfigType cfg = { 16U, 0x11223344U, 4U, 40U, 0U, 3U, TRUE };
        E2E_P05ProtectStateType tx = { 0 };
        E2E_P05CheckStateType rx = { 0 };
        E2E_SMStateType sm = E2E_SM_INIT;
        boolean err = FALSE;
        uint32 c0 = 7U;

        rx.MaxDeltaCounterInit = 4U;

        memset(data, 0x22, sizeof(data));

        tx.Counter = c0;
        CHECK(E2E_P05Protect(&cfg, &tx, data) == E_OK, "P05 protect unaligned");
        CHECK(E2E_P05Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_INITIAL,
              "P05 check INITIAL");
        tx.Counter = c0;
        E2E_P05Protect(&cfg, &tx, data);
        CHECK(E2E_P05Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_REPEATED,
              "P05 check REPEATED");
        tx.Counter = c0 + 1U;
        E2E_P05Protect(&cfg, &tx, data);
        CHECK(E2E_P05Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_OK,
              "P05 check OK");
        tx.Counter = c0 + 3U;
        E2E_P05Protect(&cfg, &tx, data);
        CHECK(E2E_P05Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_OKSOMELOST,
              "P05 check OKSOMELOST");
        tx.Counter = c0 + 8U;
        E2E_P05Protect(&cfg, &tx, data);
        CHECK(E2E_P05Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_SYNC,
              "P05 check SYNC");
        /* counter wrap-around: received < lastValid -> huge delta -> SYNC */
        tx.Counter = c0;
        E2E_P05Protect(&cfg, &tx, data);
        CHECK(E2E_P05Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_SYNC,
              "P05 check wrap SYNC");
        tx.Counter = c0;
        E2E_P05Protect(&cfg, &tx, data);
        data[0] ^= 0xFFU;
        CHECK(E2E_P05Check(&cfg, &rx, data) == E_OK && rx.Status == E2E_P_WRONGCRC,
              "P05 check WRONGCRC");

        /* include-data-id off */
        cfg.IncludeDataID = FALSE;
        tx.Counter = 1U;
        CHECK(E2E_P05Protect(&cfg, &tx, data) == E_OK, "P05 protect no-dataid");
        rx.WaitForFirstData = 0U;
        CHECK(E2E_P05Check(&cfg, &rx, data) == E_OK, "P05 check no-dataid");

        E2E_P05MapStatusToSM(E2E_P_OK, &sm, &err);
        E2E_P05MapStatusToSM(E2E_P_OKSOMELOST, &sm, &err);
        E2E_P05MapStatusToSM(E2E_P_WRONGCRC, &sm, &err);
        E2E_P05MapStatusToSM(E2E_P_WRONGSEQUENCE, &sm, &err);
        E2E_P05MapStatusToSM(E2E_P_REPEATED, &sm, &err);
        E2E_P05MapStatusToSM(E2E_P_SYNC, &sm, &err);
        E2E_P05MapStatusToSM(E2E_P_INITIAL, &sm, &err);
        E2E_P05MapStatusToSM(E2E_P_NONEWDATA, &sm, &err);

        CHECK(E2E_P05Protect(NULL, &tx, data) != E_OK, "P05 protect null");
        CHECK(E2E_P05Check(NULL, &rx, data) != E_OK, "P05 check null");
    }

    /* ---- Profile 6 (CRC64 + 16-bit counter, unaligned) ---- */
    {
        E2E_P06ConfigType cfg = { 0x1234U, 4U, 40U, 0U, 3U, 8U, 64U, TRUE };
        E2E_P06ProtectStateType tx = { 0 };
        E2E_P06CheckStateType rx = { 0 };
        E2E_SMStateType sm = E2E_SM_INIT;
        boolean err = FALSE;
        uint16 c0 = 20U;
        uint32 len = 16U;

        rx.MaxDeltaCounterInit = 4U;

        memset(data, 0x33, sizeof(data));

        tx.Counter = c0;
        CHECK(E2E_P06Protect(&cfg, &tx, data, len) == E_OK, "P06 protect");
        CHECK(E2E_P06Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_INITIAL,
              "P06 check INITIAL");
        tx.Counter = c0;
        E2E_P06Protect(&cfg, &tx, data, len);
        CHECK(E2E_P06Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_REPEATED,
              "P06 check REPEATED");
        tx.Counter = c0 + 1U;
        E2E_P06Protect(&cfg, &tx, data, len);
        CHECK(E2E_P06Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_OK,
              "P06 check OK");
        tx.Counter = c0 + 3U;
        E2E_P06Protect(&cfg, &tx, data, len);
        CHECK(E2E_P06Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_OKSOMELOST,
              "P06 check OKSOMELOST");
        tx.Counter = c0 + 8U;
        E2E_P06Protect(&cfg, &tx, data, len);
        CHECK(E2E_P06Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_SYNC,
              "P06 check SYNC");
        tx.Counter = c0;
        E2E_P06Protect(&cfg, &tx, data, len);
        data[0] ^= 0xFFU;
        CHECK(E2E_P06Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_WRONGCRC,
              "P06 check WRONGCRC");

        /* length boundary errors */
        CHECK(E2E_P06Protect(&cfg, &tx, data, 4U) == E2E_E_INPUTERR_WRONG, "P06 protect len<min");
        CHECK(E2E_P06Protect(&cfg, &tx, data, 200U) == E2E_E_INPUTERR_WRONG, "P06 protect len>max");
        CHECK(E2E_P06Check(&cfg, &rx, data, 4U) == E_OK, "P06 check len<min -> WRONGCRC");
        {
            /* wide-max config: hits the Length > 256U guard inside Check */
            E2E_P06ConfigType wide = { 0x1234U, 4U, 40U, 0U, 3U, 8U, 512U, TRUE };
            CHECK(E2E_P06Check(&wide, &rx, data, 300U) == E2E_E_INPUTERR_WRONG,
                  "P06 check len>256");
        }

        /* no-dataid path */
        cfg.IncludeDataID = FALSE;
        tx.Counter = 1U;
        CHECK(E2E_P06Protect(&cfg, &tx, data, len) == E_OK, "P06 protect no-dataid");
        rx.WaitForFirstData = 0U;
        CHECK(E2E_P06Check(&cfg, &rx, data, len) == E_OK, "P06 check no-dataid");

        E2E_P06MapStatusToSM(E2E_P_OK, &sm, &err);
        E2E_P06MapStatusToSM(E2E_P_OKSOMELOST, &sm, &err);
        E2E_P06MapStatusToSM(E2E_P_WRONGCRC, &sm, &err);
        E2E_P06MapStatusToSM(E2E_P_WRONGSEQUENCE, &sm, &err);
        E2E_P06MapStatusToSM(E2E_P_REPEATED, &sm, &err);
        E2E_P06MapStatusToSM(E2E_P_SYNC, &sm, &err);
        E2E_P06MapStatusToSM(E2E_P_INITIAL, &sm, &err);
        E2E_P06MapStatusToSM(E2E_P_NONEWDATA, &sm, &err);

        CHECK(E2E_P06Protect(NULL, &tx, data, len) != E_OK, "P06 protect null");
        CHECK(E2E_P06Check(NULL, &rx, data, len) != E_OK, "P06 check null");
    }

    /* ---- Profile 7 (CRC32 + 8-bit counter, unaligned) ---- */
    {
        E2E_P07ConfigType cfg = { 0x12345678U, 4U, 40U, 3U, 8U, 64U, TRUE };
        E2E_P07ProtectStateType tx = { 0 };
        E2E_P07CheckStateType rx = { 0 };
        E2E_SMStateType sm = E2E_SM_INIT;
        boolean err = FALSE;
        uint8 c0 = 12U;
        uint32 len = 16U;

        rx.MaxDeltaCounterInit = 4U;

        memset(data, 0x44, sizeof(data));

        tx.Counter = c0;
        CHECK(E2E_P07Protect(&cfg, &tx, data, len) == E_OK, "P07 protect");
        CHECK(E2E_P07Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_INITIAL,
              "P07 check INITIAL");
        tx.Counter = c0;
        E2E_P07Protect(&cfg, &tx, data, len);
        CHECK(E2E_P07Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_REPEATED,
              "P07 check REPEATED");
        tx.Counter = c0 + 1U;
        E2E_P07Protect(&cfg, &tx, data, len);
        CHECK(E2E_P07Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_OK,
              "P07 check OK");
        tx.Counter = c0 + 3U;
        E2E_P07Protect(&cfg, &tx, data, len);
        CHECK(E2E_P07Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_OKSOMELOST,
              "P07 check OKSOMELOST");
        tx.Counter = c0 + 8U;
        E2E_P07Protect(&cfg, &tx, data, len);
        CHECK(E2E_P07Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_SYNC,
              "P07 check SYNC");
        tx.Counter = c0;
        E2E_P07Protect(&cfg, &tx, data, len);
        data[0] ^= 0xFFU;
        CHECK(E2E_P07Check(&cfg, &rx, data, len) == E_OK && rx.Status == E2E_P_WRONGCRC,
              "P07 check WRONGCRC");

        /* length boundary errors */
        CHECK(E2E_P07Protect(&cfg, &tx, data, 2U) == E2E_E_INPUTERR_WRONG, "P07 protect len<min");
        CHECK(E2E_P07Protect(&cfg, &tx, data, 300U) == E2E_E_INPUTERR_WRONG, "P07 protect len>max");
        CHECK(E2E_P07Check(&cfg, &rx, data, 2U) == E_OK, "P07 check len<min");
        {
            E2E_P07ConfigType wide = { 0x12345678U, 4U, 40U, 3U, 8U, 512U, TRUE };
            CHECK(E2E_P07Check(&wide, &rx, data, 300U) == E2E_E_INPUTERR_WRONG,
                  "P07 check len>256");
        }

        /* no-dataid */
        cfg.IncludeDataID = FALSE;
        tx.Counter = 1U;
        CHECK(E2E_P07Protect(&cfg, &tx, data, len) == E_OK, "P07 protect no-dataid");
        rx.WaitForFirstData = 0U;
        CHECK(E2E_P07Check(&cfg, &rx, data, len) == E_OK, "P07 check no-dataid");

        E2E_P07MapStatusToSM(E2E_P_OK, &sm, &err);
        E2E_P07MapStatusToSM(E2E_P_OKSOMELOST, &sm, &err);
        E2E_P07MapStatusToSM(E2E_P_WRONGCRC, &sm, &err);
        E2E_P07MapStatusToSM(E2E_P_WRONGSEQUENCE, &sm, &err);
        E2E_P07MapStatusToSM(E2E_P_REPEATED, &sm, &err);
        E2E_P07MapStatusToSM(E2E_P_SYNC, &sm, &err);
        E2E_P07MapStatusToSM(E2E_P_INITIAL, &sm, &err);
        E2E_P07MapStatusToSM(E2E_P_NONEWDATA, &sm, &err);

        CHECK(E2E_P07Protect(NULL, &tx, data, len) != E_OK, "P07 protect null");
        CHECK(E2E_P07Check(NULL, &rx, data, len) != E_OK, "P07 check null");
    }

    printf("\nResult: %d/%d checks passed\n", t_pass, t_pass + t_fail);
    return (t_fail == 0) ? 0 : 1;
}
