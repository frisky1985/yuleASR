/**
 * @file test_E2E_P01_branch.c
 * @brief Targeted branch coverage tests for E2E_P01.c
 *
 * Supplements existing e2e_test with tests for branches that were
 * previously uncovered:
 *   - Counter wrap from 15 -> 0 (State->Counter > COUNTER_MAX when == 15)
 *   - MapStatusToSM default branch (invalid/unknown status value)
 *   - CalculateCRC default DataIDMode branch
 *   - DataID NIBBLE mode with all four nibble offsets (0..3)
 *   - Check with WaitForFirstData already set (non-first data paths)
 *   - Protect/Check individual NULL pointer args (State, Data separately)
 */

#include <string.h>
#include <stdio.h>
#include "unity.h"
#include "E2E.h"
#include "E2E_P01.h"

/* Include production source so we can exercise the static CalculateCRC
 * through the public Protect/Check APIs with full branch coverage. */
#include "src/bsw/services/e2e/src/E2E_P01.c"

/* ---- Shared fixtures --------------------------------------------------- */

static E2E_P01ConfigType cfg;
static E2E_P01ProtectStateType pState;
static E2E_P01CheckStateType cState;
static uint8 frame[32];

void setUp(void) {
    memset(&cfg, 0, sizeof(cfg));
    cfg.DataID       = 0x1234U;
    cfg.DataLength   = 8U;
    cfg.DataIDMode   = E2E_P01_DATAID_BOTH;
    cfg.CounterOffset = 0U;
    cfg.CRCOffset     = 1U;
    cfg.DataIDNibbleOffset = 0U;

    memset(&pState, 0, sizeof(pState));
    memset(&cState, 0, sizeof(cState));
    memset(frame, 0xAA, sizeof(frame));
}

void tearDown(void) {}

/* ========================================================================
 * 1. Counter wrap 15 -> 0
 *    Branch: State->Counter > E2E_P01_COUNTER_MAX  (true when counter == 15)
 * ======================================================================== */
void test_P01Protect_CounterWrapFrom15To0(void) {
    pState.Counter = 15U;  /* above COUNTER_MAX (14) */
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Protect(&cfg, &pState, frame));
    /* After protect: counter was 15, incremented to 16, then wrapped to 0 */
    TEST_ASSERT_EQUAL_UINT8(0U, pState.Counter);
}

/* Verify normal wrap at boundary 14 -> 0 still works */
void test_P01Protect_CounterWrapFrom14To0(void) {
    pState.Counter = 14U;  /* == COUNTER_MAX */
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Protect(&cfg, &pState, frame));
    /* 14 + 1 = 15 > 14 -> wraps to 0 */
    TEST_ASSERT_EQUAL_UINT8(0U, pState.Counter);
}

/* Verify counter = 13 -> 14 (no wrap) */
void test_P01Protect_CounterIncrementWithoutWrap(void) {
    pState.Counter = 13U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Protect(&cfg, &pState, frame));
    TEST_ASSERT_EQUAL_UINT8(14U, pState.Counter);
}

/* ========================================================================
 * 2. MapStatusToSM default branch
 *    The default case handles E2E_P_NONEWDATA and any invalid value.
 *    Existing tests cover E2E_P_NONEWDATA via explicit case, but we also
 *    test with a truly invalid value to hit the default label.
 * ======================================================================== */
void test_P01MapStatusToSM_NoneWData_MapsToInvalid(void) {
    E2E_SMStateType sm = E2E_SM_VALID;
    boolean err = TRUE;
    E2E_P01MapStatusToSM(E2E_P_NONEWDATA, &sm, &err);
    TEST_ASSERT_EQUAL(E2E_SM_INVALID, sm);
    TEST_ASSERT_FALSE(err);
}

void test_P01MapStatusToSM_InvalidValue_MapsToInvalid(void) {
    E2E_SMStateType sm = E2E_SM_VALID;
    boolean err = TRUE;
    /* Cast an arbitrary value that doesn't match any enum member */
    E2E_P01MapStatusToSM((E2E_PCheckStatusType)0xFFU, &sm, &err);
    TEST_ASSERT_EQUAL(E2E_SM_INVALID, sm);
    TEST_ASSERT_FALSE(err);
}

/* ========================================================================
 * 3. CalculateCRC default DataIDMode branch
 *    Pass an invalid DataIDMode through Protect (which forwards to
 *    CalculateCRC).  The default case behaves like DATAID_LOW.
 * ======================================================================== */
void test_P01Protect_InvalidDataIDMode_HitsDefault(void) {
    cfg.DataIDMode = 0x05U;  /* invalid mode, not 0..3 */
    pState.Counter = 0U;
    memset(frame, 0xBB, sizeof(frame));
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Protect(&cfg, &pState, frame));
    /* CRC should still be written (non-zero for non-zero data) */
    TEST_ASSERT_NOT_EQUAL(0U, frame[cfg.CRCOffset]);
}

/* Verify that the CRC produced by the default mode matches DATAID_LOW */
void test_P01Protect_InvalidDataIDMode_MatchesDataIDLow(void) {
    uint8 frame1[32], frame2[32];
    memset(frame1, 0xBB, sizeof(frame1));
    memset(frame2, 0xBB, sizeof(frame2));

    /* Protect with DATAID_LOW */
    cfg.DataIDMode = E2E_P01_DATAID_LOW;
    pState.Counter = 5U;
    E2E_P01Protect(&cfg, &pState, frame1);

    /* Protect with invalid mode (hits default, same as LOW) */
    cfg.DataIDMode = 0x05U;
    pState.Counter = 5U;
    E2E_P01Protect(&cfg, &pState, frame2);

    /* CRC bytes should be identical */
    TEST_ASSERT_EQUAL_UINT8(frame1[cfg.CRCOffset], frame2[cfg.CRCOffset]);
}

/* ========================================================================
 * 4. DataID NIBBLE mode: all four nibble offsets
 *    Existing tests cover nibble offsets 0..3 through drive_p01_check,
 *    but we verify each nibble produces a distinct CRC.
 * ======================================================================== */
void test_P01Protect_NibbleMode_AllOffsets(void) {
    uint8 crcValues[4];
    uint8 localFrame[32];
    cfg.DataIDMode = E2E_P01_DATAID_NIBBLE;
    cfg.DataID = 0xABCDU;  /* low=0xCD, high=0xAB */

    for (uint8 offset = 0U; offset < 4U; offset++) {
        memset(localFrame, 0x55, sizeof(localFrame));
        cfg.DataIDNibbleOffset = offset;
        pState.Counter = 0U;
        TEST_ASSERT_EQUAL(E_OK, E2E_P01Protect(&cfg, &pState, localFrame));
        crcValues[offset] = localFrame[cfg.CRCOffset];
    }

    /* At least some of the CRC values should differ since different
     * nibbles of the DataID are used */
    int allSame = 1;
    for (int i = 1; i < 4; i++) {
        if (crcValues[i] != crcValues[0]) {
            allSame = 0;
            break;
        }
    }
    /* With DataID=0xABCD, nibbles are C, D, A, B - all different */
    TEST_ASSERT_FALSE(allSame);
}

/* ========================================================================
 * 5. Check: individual NULL pointer args
 * ======================================================================== */
void test_P01Check_NullConfig_ReturnsNotOk(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01Check(NULL, &cState, frame));
}

void test_P01Check_NullState_ReturnsNotOk(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01Check(&cfg, NULL, frame));
}

void test_P01Check_NullData_ReturnsNotOk(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01Check(&cfg, &cState, NULL));
}

/* ========================================================================
 * 6. Protect: individual NULL pointer args
 * ======================================================================== */
void test_P01Protect_NullConfig_ReturnsNotOk(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01Protect(NULL, &pState, frame));
}

void test_P01Protect_NullState_ReturnsNotOk(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01Protect(&cfg, NULL, frame));
}

void test_P01Protect_NullData_ReturnsNotOk(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, E2E_P01Protect(&cfg, &pState, NULL));
}

/* ========================================================================
 * 7. Check: delta == 1 (consecutive OK) with counter update verification
 * ======================================================================== */
void test_P01Check_ConsecutiveDelta1_VerifyLastCounter(void) {
    cState.MaxDeltaCounterInit = 4U;

    /* First data: counter = 3 */
    pState.Counter = 3U;
    E2E_P01Protect(&cfg, &pState, frame);
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Check(&cfg, &cState, frame));
    TEST_ASSERT_EQUAL(E2E_P_INITIAL, cState.Status);

    /* Consecutive: counter = 4 (delta == 1 -> OK) */
    pState.Counter = 4U;
    E2E_P01Protect(&cfg, &pState, frame);
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Check(&cfg, &cState, frame));
    TEST_ASSERT_EQUAL(E2E_P_OK, cState.Status);
    TEST_ASSERT_EQUAL_UINT8(4U, cState.LastValidCounter);
}

/* ========================================================================
 * 8. Check: OKSOMELOST with delta == MaxDeltaCounterInit (boundary)
 * ======================================================================== */
void test_P01Check_OkSomeLost_DeltaEqualsMax(void) {
    cState.MaxDeltaCounterInit = 3U;

    /* First data: counter = 0 */
    pState.Counter = 0U;
    E2E_P01Protect(&cfg, &pState, frame);
    E2E_P01Check(&cfg, &cState, frame);
    TEST_ASSERT_EQUAL(E2E_P_INITIAL, cState.Status);

    /* delta == 3 == MaxDeltaCounterInit -> OKSOMELOST (boundary) */
    pState.Counter = 3U;
    E2E_P01Protect(&cfg, &pState, frame);
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Check(&cfg, &cState, frame));
    TEST_ASSERT_EQUAL(E2E_P_OKSOMELOST, cState.Status);
}

/* ========================================================================
 * 9. Check: delta > MaxDeltaCounterInit -> SYNC (boundary)
 * ======================================================================== */
void test_P01Check_Sync_DeltaExceedsMaxByOne(void) {
    cState.MaxDeltaCounterInit = 3U;

    /* First data: counter = 0 */
    pState.Counter = 0U;
    E2E_P01Protect(&cfg, &pState, frame);
    E2E_P01Check(&cfg, &cState, frame);

    /* delta == 4 > MaxDeltaCounterInit(3) -> SYNC */
    pState.Counter = 4U;
    E2E_P01Protect(&cfg, &pState, frame);
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Check(&cfg, &cState, frame));
    TEST_ASSERT_EQUAL(E2E_P_SYNC, cState.Status);
}

/* ========================================================================
 * 10. MapStatusToSM: all individual status values
 * ======================================================================== */
void test_P01MapStatusToSM_AllStatusValues(void) {
    E2E_SMStateType sm;
    boolean err;

    /* E2E_P_OK -> VALID, no error */
    E2E_P01MapStatusToSM(E2E_P_OK, &sm, &err);
    TEST_ASSERT_EQUAL(E2E_SM_VALID, sm);
    TEST_ASSERT_FALSE(err);

    /* E2E_P_OKSOMELOST -> VALID, no error */
    E2E_P01MapStatusToSM(E2E_P_OKSOMELOST, &sm, &err);
    TEST_ASSERT_EQUAL(E2E_SM_VALID, sm);
    TEST_ASSERT_FALSE(err);

    /* E2E_P_WRONGCRC -> INVALID, error */
    E2E_P01MapStatusToSM(E2E_P_WRONGCRC, &sm, &err);
    TEST_ASSERT_EQUAL(E2E_SM_INVALID, sm);
    TEST_ASSERT_NOT_EQUAL(0, err);

    /* E2E_P_WRONGSEQUENCE -> INVALID, error */
    E2E_P01MapStatusToSM(E2E_P_WRONGSEQUENCE, &sm, &err);
    TEST_ASSERT_EQUAL(E2E_SM_INVALID, sm);
    TEST_ASSERT_NOT_EQUAL(0, err);

    /* E2E_P_REPEATED -> INVALID, error */
    E2E_P01MapStatusToSM(E2E_P_REPEATED, &sm, &err);
    TEST_ASSERT_EQUAL(E2E_SM_INVALID, sm);
    TEST_ASSERT_NOT_EQUAL(0, err);

    /* E2E_P_SYNC -> INIT, no error */
    E2E_P01MapStatusToSM(E2E_P_SYNC, &sm, &err);
    TEST_ASSERT_EQUAL(E2E_SM_INIT, sm);
    TEST_ASSERT_FALSE(err);

    /* E2E_P_INITIAL -> NODATA, no error */
    E2E_P01MapStatusToSM(E2E_P_INITIAL, &sm, &err);
    TEST_ASSERT_EQUAL(E2E_SM_NODATA, sm);
    TEST_ASSERT_FALSE(err);

    /* E2E_P_NONEWDATA -> INVALID (falls to case + default), no error */
    E2E_P01MapStatusToSM(E2E_P_NONEWDATA, &sm, &err);
    TEST_ASSERT_EQUAL(E2E_SM_INVALID, sm);
    TEST_ASSERT_FALSE(err);
}

/* ========================================================================
 * 11. DataID ALT mode: even counter uses low byte, odd uses high byte
 * ======================================================================== */
void test_P01Protect_AltMode_EvenOddCounter(void) {
    uint8 frameEven[32], frameOdd[32];
    cfg.DataIDMode = E2E_P01_DATAID_ALT;
    cfg.DataID = 0xABCDU;  /* low=0xCD, high=0xAB */

    /* Even counter -> uses dataIdLow */
    memset(frameEven, 0x55, sizeof(frameEven));
    pState.Counter = 0U;  /* even */
    E2E_P01Protect(&cfg, &pState, frameEven);

    /* Odd counter -> uses dataIdHigh */
    memset(frameOdd, 0x55, sizeof(frameOdd));
    pState.Counter = 1U;  /* odd */
    E2E_P01Protect(&cfg, &pState, frameOdd);

    /* CRC values should differ because different DataID bytes are used */
    TEST_ASSERT_NOT_EQUAL(frameEven[cfg.CRCOffset], frameOdd[cfg.CRCOffset]);
}

/* ========================================================================
 * 12. Protect preserves upper nibble of counter byte
 * ======================================================================== */
void test_P01Protect_PreservesUpperNibble(void) {
    frame[cfg.CounterOffset] = 0xF0U;  /* upper nibble = 0xF */
    pState.Counter = 7U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Protect(&cfg, &pState, frame));
    /* Lower nibble should be counter (7), upper preserved (0xF) */
    TEST_ASSERT_EQUAL_UINT8(0xF7U, frame[cfg.CounterOffset]);
}

/* ========================================================================
 * 13. CRCOffset at various positions in the data
 * ======================================================================== */
void test_P01Protect_CrcOffsetAtEnd(void) {
    cfg.CRCOffset = 7U;  /* CRC at last byte of 8-byte data */
    cfg.DataIDMode = E2E_P01_DATAID_BOTH;
    memset(frame, 0x33, sizeof(frame));
    pState.Counter = 0U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Protect(&cfg, &pState, frame));
    /* CRC should be written at offset 7 */
    /* Verify roundtrip */
    cState.MaxDeltaCounterInit = 4U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Check(&cfg, &cState, frame));
    TEST_ASSERT_EQUAL(E2E_P_INITIAL, cState.Status);
}

void test_P01Protect_CrcOffsetAtZero(void) {
    cfg.CRCOffset = 0U;
    cfg.CounterOffset = 1U;  /* swap CRC and counter offsets */
    memset(frame, 0x44, sizeof(frame));
    pState.Counter = 2U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Protect(&cfg, &pState, frame));
    /* Verify roundtrip */
    cState.MaxDeltaCounterInit = 4U;
    TEST_ASSERT_EQUAL(E_OK, E2E_P01Check(&cfg, &cState, frame));
    TEST_ASSERT_EQUAL(E2E_P_INITIAL, cState.Status);
}

/* ========================================================================
 * main
 * ======================================================================== */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_P01Protect_CounterWrapFrom15To0);
    RUN_TEST(test_P01Protect_CounterWrapFrom14To0);
    RUN_TEST(test_P01Protect_CounterIncrementWithoutWrap);
    RUN_TEST(test_P01MapStatusToSM_NoneWData_MapsToInvalid);
    RUN_TEST(test_P01MapStatusToSM_InvalidValue_MapsToInvalid);
    RUN_TEST(test_P01Protect_InvalidDataIDMode_HitsDefault);
    RUN_TEST(test_P01Protect_InvalidDataIDMode_MatchesDataIDLow);
    RUN_TEST(test_P01Protect_NibbleMode_AllOffsets);
    RUN_TEST(test_P01Check_NullConfig_ReturnsNotOk);
    RUN_TEST(test_P01Check_NullState_ReturnsNotOk);
    RUN_TEST(test_P01Check_NullData_ReturnsNotOk);
    RUN_TEST(test_P01Protect_NullConfig_ReturnsNotOk);
    RUN_TEST(test_P01Protect_NullState_ReturnsNotOk);
    RUN_TEST(test_P01Protect_NullData_ReturnsNotOk);
    RUN_TEST(test_P01Check_ConsecutiveDelta1_VerifyLastCounter);
    RUN_TEST(test_P01Check_OkSomeLost_DeltaEqualsMax);
    RUN_TEST(test_P01Check_Sync_DeltaExceedsMaxByOne);
    RUN_TEST(test_P01MapStatusToSM_AllStatusValues);
    RUN_TEST(test_P01Protect_AltMode_EvenOddCounter);
    RUN_TEST(test_P01Protect_PreservesUpperNibble);
    RUN_TEST(test_P01Protect_CrcOffsetAtEnd);
    RUN_TEST(test_P01Protect_CrcOffsetAtZero);

    return UnityEnd();
}
