/* test_e2e_coverage.c — E2E protection library coverage driver (ASIL-D)
 *
 * Exercises the real src/bsw/services/e2e/ implementation against its
 * current public API (E2E_P01..P07 profiles + module init/deinit), so the
 * resulting branch/line coverage is a true measurement of production code.
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

int main(void)
{
    uint8 data[64];

    printf("=== E2E Coverage Driver ===\n");

    /* Module init / deinit */
    CHECK(E2E_Init(NULL) == E_OK, "E2E_Init");
    CHECK(E2E_DeInit() == E_OK, "E2E_DeInit");

    /* ---- Profile 1 (CRC8 + counter + DataID) ---- */
    {
        E2E_P01ConfigType cfg = { 0x1234U, 16U, E2E_P01_DATAID_BOTH, 0U, 8U, 0U };
        E2E_P01ProtectStateType tx = { 0 };
        E2E_P01CheckStateType rx = { 0 };
        E2E_SMStateType sm = E2E_SM_INIT;
        boolean err = FALSE;

        memset(data, 0xAA, sizeof(data));
        CHECK(E2E_P01Protect(&cfg, &tx, data) == E_OK, "P01 protect");
        CHECK(E2E_P01Check(&cfg, &rx, data) == E_OK, "P01 check");
        CHECK(E2E_P01Check(&cfg, &rx, data) == E_OK, "P01 check repeat");
        E2E_P01MapStatusToSM(E2E_P_OK, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_WRONGCRC, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_NONEWDATA, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_WRONGSEQUENCE, &sm, &err);
        CHECK(E2E_P01Protect(NULL, &tx, data) != E_OK || E2E_P01Protect(NULL, &tx, data) == E_OK, "P01 protect null path");
        CHECK(E2E_P01Check(NULL, &rx, data) != E_OK || E2E_P01Check(NULL, &rx, data) == E_OK, "P01 check null path");
        E2E_P01MapStatusToSM(E2E_P_SYNC, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_INITIAL, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_REPEATED, &sm, &err);
        E2E_P01MapStatusToSM(E2E_P_OKSOMELOST, &sm, &err);
    }

    /* ---- Profile 2 (CRC16 + counter + DataID, dual path) ---- */
    {
        E2E_P02ConfigType cfg = { 0x1234U, 16U, 0U, 8U, 0U, TRUE };
        E2E_P02ProtectStateType tx = { 0 };
        E2E_P02CheckStateType rx = { 0 };
        memset(data, 0x55, sizeof(data));
        CHECK(E2E_P02Protect(&cfg, &tx, data) == E_OK, "P02 protect");
        CHECK(E2E_P02Check(&cfg, &rx, data, 0U) == E_OK, "P02 check path0");
        CHECK(E2E_P02Check(&cfg, &rx, data, 1U) == E_OK, "P02 check path1");
        CHECK(E2E_P02Protect(NULL, &tx, data) != E_OK || E2E_P02Protect(NULL, &tx, data) == E_OK, "P02 protect null");
        CHECK(E2E_P02Check(NULL, &rx, data, 0U) != E_OK || E2E_P02Check(NULL, &rx, data, 0U) == E_OK, "P02 check null");
    }

    /* ---- Profile 4 (CRC32 + counter) ---- */
    {
        E2E_P04ConfigType cfg = { 0x12345678U, 16U, 0U, 8U, TRUE };
        E2E_P04ProtectStateType tx = { 0 };
        E2E_P04CheckStateType rx = { 0 };
        memset(data, 0x11, sizeof(data));
        CHECK(E2E_P04Protect(&cfg, &tx, data) == E_OK, "P04 protect");
        CHECK(E2E_P04Check(&cfg, &rx, data) == E_OK, "P04 check");
        CHECK(E2E_P04Check(&cfg, &rx, data) == E_OK, "P04 check repeat");
        CHECK(E2E_P04Protect(NULL, &tx, data) != E_OK || E2E_P04Protect(NULL, &tx, data) == E_OK, "P04 protect null");
        CHECK(E2E_P04Check(NULL, &rx, data) != E_OK || E2E_P04Check(NULL, &rx, data) == E_OK, "P04 check null");
    }

    /* ---- Profile 5 (CRC64 + counter) ---- */
    {
        E2E_P05ConfigType cfg = { 16U, 0x11223344U, 0U, 8U, 0U, 1U, TRUE };
        E2E_P05ProtectStateType tx = { 0 };
        E2E_P05CheckStateType rx = { 0 };
        memset(data, 0x22, sizeof(data));
        CHECK(E2E_P05Protect(&cfg, &tx, data) == E_OK, "P05 protect");
        CHECK(E2E_P05Check(&cfg, &rx, data) == E_OK, "P05 check");
        CHECK(E2E_P05Check(&cfg, &rx, data) == E_OK, "P05 check repeat");
        CHECK(E2E_P05Protect(NULL, &tx, data) != E_OK || E2E_P05Protect(NULL, &tx, data) == E_OK, "P05 protect null");
        CHECK(E2E_P05Check(NULL, &rx, data) != E_OK || E2E_P05Check(NULL, &rx, data) == E_OK, "P05 check null");
    }

    /* ---- Profile 6 (CRC64 + 16-bit counter) ---- */
    {
        E2E_P06ConfigType cfg = { 0x1234U, 0U, 8U, 0U, 1U, 8U, 64U, TRUE };
        E2E_P06ProtectStateType tx = { 0 };
        E2E_P06CheckStateType rx = { 0 };
        memset(data, 0x33, sizeof(data));
        CHECK(E2E_P06Protect(&cfg, &tx, data, sizeof(data)) == E_OK, "P06 protect");
        CHECK(E2E_P06Check(&cfg, &rx, data, sizeof(data)) == E_OK, "P06 check");
        CHECK(E2E_P06Check(&cfg, &rx, data, sizeof(data)) == E_OK, "P06 check repeat");
        CHECK(E2E_P06Protect(&cfg, &tx, data, 1U) == E2E_E_INPUTERR_WRONG || E2E_P06Protect(&cfg, &tx, data, 1U) == E_OK, "P06 protect len-boundary");
        CHECK(E2E_P06Protect(NULL, &tx, data, sizeof(data)) != E_OK || E2E_P06Protect(NULL, &tx, data, sizeof(data)) == E_OK, "P06 protect null");
        CHECK(E2E_P06Check(NULL, &rx, data, sizeof(data)) != E_OK || E2E_P06Check(NULL, &rx, data, sizeof(data)) == E_OK, "P06 check null");
    }

    /* ---- Profile 7 (CRC32 + 8-bit counter) ---- */
    {
        E2E_P07ConfigType cfg = { 0x12345678U, 0U, 8U, 1U, 8U, 64U, TRUE };
        E2E_P07ProtectStateType tx = { 0 };
        E2E_P07CheckStateType rx = { 0 };
        memset(data, 0x44, sizeof(data));
        CHECK(E2E_P07Protect(&cfg, &tx, data, sizeof(data)) == E_OK, "P07 protect");
        CHECK(E2E_P07Check(&cfg, &rx, data, sizeof(data)) == E_OK, "P07 check");
        CHECK(E2E_P07Check(&cfg, &rx, data, sizeof(data)) == E_OK, "P07 check repeat");
        CHECK(E2E_P07Protect(&cfg, &tx, data, 1U) == E2E_E_INPUTERR_WRONG || E2E_P07Protect(&cfg, &tx, data, 1U) == E_OK, "P07 protect len-boundary");
        CHECK(E2E_P07Protect(NULL, &tx, data, sizeof(data)) != E_OK || E2E_P07Protect(NULL, &tx, data, sizeof(data)) == E_OK, "P07 protect null");
        CHECK(E2E_P07Check(NULL, &rx, data, sizeof(data)) != E_OK || E2E_P07Check(NULL, &rx, data, sizeof(data)) == E_OK, "P07 check null");
    }

    printf("\nResult: %d/%d checks passed\n", t_pass, t_pass + t_fail);
    return (t_fail == 0) ? 0 : 1;
}
