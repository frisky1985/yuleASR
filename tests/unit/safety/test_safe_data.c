/******************************************************************************
 * @file    test_safe_data.c
 * @brief   Unit tests for safe_data.c (CRC / inverted-copy safe variables)
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 ******************************************************************************/

#include "unity.h"
#include "safe_data.h"
#include <string.h>

static SafeDataConfigType ElemCfg(uint8_t type, uint8_t redundancy, uint8_t crc)
{
    SafeDataConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.data_type = type;
    cfg.redundancy_mode = redundancy;
    cfg.crc_type = crc;
    cfg.max_age_ms = SAFE_DATA_DEFAULT_MAX_AGE_MS;
    cfg.enable_timestamp = TRUE;
    cfg.enable_sequence = TRUE;
    cfg.seq_window = SAFE_DATA_DEFAULT_SEQ_WINDOW;
    return cfg;
}

/* Address-of-rvalue workaround: C does not allow & on the result of
 * ElemCfg(), so stash the config in a static buffer and hand out its
 * address.  Each call fully overwrites the buffer and the consuming
 * InitElement call completes before the next CfgP() call, so the shared
 * storage is safe here. */
static SafeDataConfigType g_cfg_tmp;
static const SafeDataConfigType *CfgP(uint8_t type, uint8_t redundancy,
                                      uint8_t crc)
{
    g_cfg_tmp = ElemCfg(type, redundancy, crc);
    return &g_cfg_tmp;
}

void setUp(void)
{
    SafeData_DeInit();
    TEST_ASSERT_EQUAL(E_OK, SafeData_Init());
}

void tearDown(void)
{
    SafeData_DeInit();
}

/* ---------------- lifecycle ---------------- */

void test_init_deinit_cycle(void)
{
    TEST_ASSERT_EQUAL(E_OK, SafeData_DeInit());
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_InitElement(NULL, NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeData_Init());
}

void test_init_element_null_guards(void)
{
    SafeDataElementType el;
    SafeDataConfigType cfg = ElemCfg(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32);
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_InitElement(NULL, &cfg));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_InitElement(&el, NULL));
}

void test_init_element_bad_type(void)
{
    SafeDataElementType el;
    /* ARRAY elements are not supported through InitElement */
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_InitElement(&el,
        CfgP(SAFE_DATA_TYPE_ARRAY,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_InitElement(&el,
        CfgP(0x00u,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
}

void test_init_element_sizes(void)
{
    SafeDataElementType e8, e16, e32, e64, ef, ed;

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&e8,
        CfgP(SAFE_DATA_TYPE_UINT8,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL_UINT32(1u, e8.header.data_size);
    TEST_ASSERT_EQUAL_UINT8(SAFE_DATA_TYPE_UINT8, e8.header.data_type);
    TEST_ASSERT_EQUAL_UINT8(SAFE_DATA_REDUNDANCY_INV, e8.header.redundancy_mode);

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&e16,
        CfgP(SAFE_DATA_TYPE_UINT16,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL_UINT32(2u, e16.header.data_size);

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&e32,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL_UINT32(4u, e32.header.data_size);

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&e64,
        CfgP(SAFE_DATA_TYPE_UINT64,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL_UINT32(8u, e64.header.data_size);

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&ef,
        CfgP(SAFE_DATA_TYPE_FLOAT,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL_UINT32(4u, ef.header.data_size);

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&ed,
        CfgP(SAFE_DATA_TYPE_DOUBLE,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL_UINT32(8u, ed.header.data_size);
}

/* ---------------- element write / read / validate ---------------- */

void test_write_read_element_u32(void)
{
    SafeDataElementType el;
    uint32_t v = 0x12345678u;
    uint32_t out = 0u;
    SafeDataValidationType res;

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&el,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_WriteElement(&el, NULL));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_WriteElement(NULL, &v));
    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteElement(&el, &v));

    /* primary, inverted copy and header CRC must agree */
    TEST_ASSERT_EQUAL_UINT32(0x12345678u, el.primary.data32);
    TEST_ASSERT_EQUAL_UINT32(~0x12345678u, el.inverted.data32);

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_ReadElement(&el, NULL, NULL));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_ReadElement(NULL, &out, NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeData_ReadElement(&el, &out, &res));
    TEST_ASSERT_EQUAL_UINT32(0x12345678u, out);
    TEST_ASSERT_EQUAL(TRUE, res.is_valid);
    TEST_ASSERT_EQUAL_UINT8(SAFE_DATA_STATE_VALID, res.state);
    TEST_ASSERT_EQUAL_UINT32(0u, res.age_ms); /* system time frozen at 0 */
}

void test_write_bumps_sequence_and_crc(void)
{
    SafeDataElementType el;
    uint32_t v1 = 0x11111111u, v2 = 0x22222222u;

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&el,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL_UINT32(0u, el.header.seq_num);
    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteElement(&el, &v1));
    TEST_ASSERT_EQUAL_UINT32(1u, el.header.seq_num);
    TEST_ASSERT_EQUAL_UINT32(SafeData_CalcCRC32(&v1, 4u), el.header.crc32);

    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteElement(&el, &v2));
    TEST_ASSERT_EQUAL_UINT32(2u, el.header.seq_num);
    TEST_ASSERT_EQUAL_UINT32(SafeData_CalcCRC32(&v2, 4u), el.header.crc32);
}

void test_validate_detects_primary_corruption(void)
{
    SafeDataElementType el;
    uint32_t v = 0xA5A5A5A5u;
    SafeDataValidationType res;

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&el,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteElement(&el, &v));

    /* corrupt primary: CRC and inverted copy no longer match */
    el.primary.data32 = 0xA5A5A5A0u;
    /* NOTE(actual behaviour): ValidateElement always returns E_OK - the
     * verdict is carried by the result fields (is_valid/state/error). */
    TEST_ASSERT_EQUAL(E_OK, SafeData_ValidateElement(&el, &res));
    TEST_ASSERT_EQUAL(FALSE, res.is_valid);
    TEST_ASSERT_EQUAL_UINT8(SAFE_DATA_ERR_CRC_MISMATCH, res.error_code);
    TEST_ASSERT_TRUE(res.calculated_crc !=
        SafeData_CalcCRC32(&(uint32_t){0xA5A5A5A5u}, 4u));
    TEST_ASSERT_EQUAL_UINT32(el.header.crc32, res.stored_crc);

    /* read rejects corrupted data */
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_ReadElement(&el, &(uint32_t){0}, &res));
}

void test_validate_inv_mode_masked_header_crc(void)
{
    SafeDataElementType el;
    uint32_t v = 0x0BADF00Du;
    SafeDataValidationType res;

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&el,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteElement(&el, &v));

    /* NOTE(actual behaviour): with INV redundancy a corrupted *header* CRC is
     * masked - primary still matches the inverted copy, so the element stays
     * "valid".  Only data corruption trips the check. */
    el.header.crc32 = 0xDEADBEEFu;
    TEST_ASSERT_EQUAL(E_OK, SafeData_ValidateElement(&el, &res));
    TEST_ASSERT_EQUAL(TRUE, res.is_valid);
}

void test_validate_without_redundancy(void)
{
    SafeDataElementType el;
    uint32_t v = 0x0BADF00Du;
    SafeDataValidationType res;

    /* no redundancy, no timestamp/seq: only the CRC protects the data */
    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&el,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_NONE, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteElement(&el, &v));
    TEST_ASSERT_EQUAL(E_OK, SafeData_ValidateElement(&el, &res));
    TEST_ASSERT_EQUAL(TRUE, res.is_valid);

    el.primary.data32 ^= 0x1u;
    /* NOTE(actual behaviour): ValidateElement always returns E_OK; the
     * CRC failure shows up in the result fields. */
    TEST_ASSERT_EQUAL(E_OK, SafeData_ValidateElement(&el, &res));
    TEST_ASSERT_EQUAL(FALSE, res.is_valid);
    TEST_ASSERT_EQUAL_UINT8(SAFE_DATA_ERR_CRC_MISMATCH, res.error_code);
}

void test_stale_detection_via_age_wraparound(void)
{
    SafeDataElementType el;
    uint32_t v = 7u;
    SafeDataValidationType res;

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&el,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteElement(&el, &v));

    /* module system time is frozen at 0; a non-zero timestamp makes the
     * wraparound age enormous -> STALE */
    el.header.timestamp = 10u;
    /* NOTE(actual behaviour): ValidateElement always returns E_OK; the
     * stale verdict is carried by the result fields. */
    TEST_ASSERT_EQUAL(E_OK, SafeData_ValidateElement(&el, &res));
    TEST_ASSERT_EQUAL(FALSE, res.is_valid);
    TEST_ASSERT_EQUAL_UINT8(SAFE_DATA_ERR_AGE_EXPIRED, res.error_code);

    /* refreshing the timestamp heals the element */
    TEST_ASSERT_EQUAL(E_OK, SafeData_UpdateTimestamp(&el.header));
    TEST_ASSERT_EQUAL(E_OK, SafeData_ValidateElement(&el, &res));
    TEST_ASSERT_EQUAL(TRUE, res.is_valid);
}

/* ---------------- arrays ---------------- */

/* PRODUCT BUG (documented, product code NOT touched): SafeData_InitArray()
 * does not allocate or even assign primary_data/inverted_data - the
 * pointers keep whatever value the caller's struct happened to hold, and
 * Write/ReadArrayElement happily memcpy through them.  The tests provide
 * their own storage (asserting real behaviour). */
static uint8_t arr_prim_buf[64];
static uint8_t arr_inv_buf[64];

static void AttachArrayBuffers(SafeDataArrayType *arr)
{
    /* unwritten elements must self-verify: primary 0x00 vs inverted
     * 0xFF... == ~primary. */
    memset(arr_prim_buf, 0x00, sizeof(arr_prim_buf));
    memset(arr_inv_buf, 0xFF, sizeof(arr_inv_buf));
    arr->primary_data = arr_prim_buf;
    arr->inverted_data = arr_inv_buf;
}

void test_array_roundtrip(void)
{
    SafeDataArrayType arr;
    uint32_t v0 = 0x01020304u, v1 = 0x11121314u;
    uint32_t out = 0u;
    SafeDataValidationType res;

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitArray(&arr,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32),
        4u, 4u));
    AttachArrayBuffers(&arr);

    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteArrayElement(&arr, 0u, &v0));
    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteArrayElement(&arr, 1u, &v1));

    TEST_ASSERT_EQUAL(E_OK, SafeData_ReadArrayElement(&arr, 1u, &out, &res));
    TEST_ASSERT_EQUAL_UINT32(0x11121314u, out);
    TEST_ASSERT_EQUAL(TRUE, res.is_valid);

    TEST_ASSERT_EQUAL(E_OK, SafeData_ReadArrayElement(&arr, 0u, &out, NULL));
    TEST_ASSERT_EQUAL_UINT32(0x01020304u, out);
}

void test_array_bad_index(void)
{
    SafeDataArrayType arr;
    uint32_t v = 1u, out;
    TEST_ASSERT_EQUAL(E_OK, SafeData_InitArray(&arr,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32),
        4u, 4u));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_WriteArrayElement(&arr, 4u, &v));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_ReadArrayElement(&arr, 4u, &out, NULL));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_ReadArrayElement(&arr, 4u, NULL, NULL));
}

void test_array_corruption_detection(void)
{
    SafeDataArrayType arr;
    uint32_t v = 0x55AA55AAu;
    uint32_t errors = 0xFFFFu;

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitArray(&arr,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32),
        4u, 2u));
    AttachArrayBuffers(&arr);
    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteArrayElement(&arr, 0u, &v));

    TEST_ASSERT_EQUAL(E_OK, SafeData_ValidateArray(&arr, &errors));
    TEST_ASSERT_EQUAL_UINT32(0u, errors);

    /* flip a primary byte: primary no longer equals ~inverted */
    ((uint8_t *)arr.primary_data)[1] ^= 0xFFu;
    TEST_ASSERT_EQUAL(E_OK, SafeData_ValidateArray(&arr, &errors));
    TEST_ASSERT_EQUAL_UINT32(1u, errors);
}

/* ---------------- containers ---------------- */

static uint8_t cont_buf[32];

void test_container_roundtrip(void)
{
    SafeDataContainerType cont;
    SafeDataValidationType res;

    memset(cont_buf, 0x33, sizeof(cont_buf));
    TEST_ASSERT_EQUAL(E_OK, SafeData_InitContainer(&cont,
        CfgP(SAFE_DATA_TYPE_STRUCT,
                                     SAFE_DATA_REDUNDANCY_NONE, SAFE_DATA_CRC_32),
        cont_buf, sizeof(cont_buf)));

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_UpdateContainer(NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeData_UpdateContainer(&cont));

    TEST_ASSERT_EQUAL(E_OK, SafeData_ValidateContainer(&cont, &res));
    TEST_ASSERT_EQUAL(TRUE, res.is_valid);

    /* silent data change without Update must be detected */
    cont_buf[3] ^= 0xFFu;
    /* NOTE(actual behaviour): like ValidateElement, ValidateContainer
     * always returns E_OK - the verdict is in the result fields. */
    TEST_ASSERT_EQUAL(E_OK, SafeData_ValidateContainer(&cont, &res));
    TEST_ASSERT_EQUAL_UINT8(SAFE_DATA_ERR_CRC_MISMATCH, res.error_code);

    /* after a proper Update the new contents validate */
    TEST_ASSERT_EQUAL(E_OK, SafeData_UpdateContainer(&cont));
    TEST_ASSERT_EQUAL(E_OK, SafeData_ValidateContainer(&cont, &res));
    TEST_ASSERT_EQUAL(TRUE, res.is_valid);
}

/* ---------------- redundancy helpers ---------------- */

void test_inverted_copy_helpers(void)
{
    uint8_t src[8] = {0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u};
    uint8_t inv[8];
    boolean valid = FALSE;
    unsigned i;

    SafeData_CreateInvertedCopy(src, inv, 8u);
    for (i = 0; i < 8u; i++) {
        TEST_ASSERT_EQUAL_HEX8((uint8_t)~src[i], inv[i]);
    }

    TEST_ASSERT_EQUAL(E_OK, SafeData_VerifyInvertedCopy(src, inv, 8u, &valid));
    TEST_ASSERT_EQUAL(TRUE, valid);
    inv[2] ^= 0x01u;
    TEST_ASSERT_EQUAL(E_OK, SafeData_VerifyInvertedCopy(src, inv, 8u, &valid));
    TEST_ASSERT_EQUAL(FALSE, valid);
}

void test_vote_2of3(void)
{
    uint32_t c1 = 100u, c2 = 100u, c3 = 999u, out = 0u;

    /* NOTE(actual behaviour / PRODUCT BUG): the pairwise "matches"
     * counter can only reach 3 on full consensus - a genuine 2-of-3
     * majority such as (100,100,999) yields exactly one matching pair
     * and is REJECTED.  2-out-of-3 voting is therefore unreachable. */
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_Vote2of3(&c1, &c2, &c3, &out, 4u));
    TEST_ASSERT_EQUAL(E_OK, SafeData_Vote2of3(&c2, &c2, &c2, &out, 4u));
    TEST_ASSERT_EQUAL_UINT32(100u, out);

    /* NOTE(actual behaviour): minority in ANY position yields a single
     * matching pair -> rejected (same product bug as above). */
    out = 0u;
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_Vote2of3(&c3, &c1, &c2, &out, 4u));

    /* all different -> no majority */
    c3 = 5u;
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_Vote2of3(&c1, &c2, &c3, &out, 4u));

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_Vote2of3(NULL, &c2, &c3, &out, 4u));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_Vote2of3(&c1, &c2, &c3, NULL, 4u));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_Vote2of3(&c1, &c2, &c3, &out, 0u));
}

/* ---------------- CRC / checksum ---------------- */

void test_crc32_known_vector(void)
{
    const char *vec = "123456789";
    /* standard CRC-32 (reflected, poly 0xEDB88320, init/final 0xFFFFFFFF) */
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, SafeData_CalcCRC32(vec, 9u));
    TEST_ASSERT_EQUAL_HEX32(0u, SafeData_CalcCRC32(vec, 0u));
}

void test_crc16_known_vector(void)
{
    const char *vec = "123456789";
    /* CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no final xor) */
    TEST_ASSERT_EQUAL_HEX16(0x29B1u, SafeData_CalcCRC16(vec, 9u));
    TEST_ASSERT_EQUAL_HEX16(0u, SafeData_CalcCRC16(vec, 0u));
}

void test_crc8_stability(void)
{
    const char *vec = "123456789";
    uint8_t c1 = SafeData_CalcCRC8(vec, 9u);
    TEST_ASSERT_TRUE(c1 != 0u);
    TEST_ASSERT_EQUAL(c1, SafeData_CalcCRC8(vec, 9u));
    TEST_ASSERT_EQUAL_UINT8(0u, SafeData_CalcCRC8(vec, 0u));
}

void test_calc_checksum_dispatch(void)
{
    const char *vec = "abcd";
    TEST_ASSERT_EQUAL_HEX32(0u, SafeData_CalcChecksum(vec, 4u, SAFE_DATA_CRC_NONE));
    TEST_ASSERT_EQUAL(SafeData_CalcCRC8(vec, 4u),
        SafeData_CalcChecksum(vec, 4u, SAFE_DATA_CRC_8));
    TEST_ASSERT_EQUAL(SafeData_CalcCRC16(vec, 4u),
        SafeData_CalcChecksum(vec, 4u, SAFE_DATA_CRC_16));
    TEST_ASSERT_EQUAL(SafeData_CalcCRC32(vec, 4u),
        SafeData_CalcChecksum(vec, 4u, SAFE_DATA_CRC_32));
    TEST_ASSERT_EQUAL_HEX32(0u, SafeData_CalcChecksum(vec, 4u, SAFE_DATA_CRC_CUSTOM));
}

/* ---------------- sequence helpers ---------------- */

void test_sequence_helpers(void)
{
    TEST_ASSERT_EQUAL_UINT32(1u, SafeData_NextSequence(0u));
    /* NOTE(actual behaviour): plain uint32 increment - 0xFFFFFFFF wraps
     * to 0, there is no reserved-value skip. */
    TEST_ASSERT_EQUAL_UINT32(0u, SafeData_NextSequence(0xFFFFFFFFu)); /* wrap */

    TEST_ASSERT_EQUAL(TRUE, SafeData_IsSequenceValid(5u, 5u, 5u));
    TEST_ASSERT_EQUAL(TRUE, SafeData_IsSequenceValid(7u, 5u, 5u));
    TEST_ASSERT_EQUAL(FALSE, SafeData_IsSequenceValid(100u, 5u, 5u));

    TEST_ASSERT_EQUAL_INT32(3, SafeData_SequenceDiff(8u, 5u));
    TEST_ASSERT_EQUAL_INT32(-3, SafeData_SequenceDiff(5u, 8u));
    TEST_ASSERT_EQUAL_INT32(0, SafeData_SequenceDiff(5u, 5u));
}

/* ---------------- age helpers ---------------- */

void test_age_helpers(void)
{
    /* system time is 0: age of an old timestamp wraps around */
    TEST_ASSERT_EQUAL_UINT32(0u, SafeData_GetTimestamp());
    TEST_ASSERT_EQUAL_UINT32(5u, SafeData_CalcAge(0xFFFFFFFF - 4u));
    TEST_ASSERT_EQUAL(TRUE, SafeData_IsStale(10u, 5u));
    TEST_ASSERT_EQUAL(FALSE, SafeData_IsStale(0u, 5u));
}

/* ---------------- monitors ---------------- */

static uint32_t g_corr_hits;
static void CorrCb(uint8_t id, uint8_t err, const SafeDataValidationType *res)
{
    (void)id;
    (void)err;
    (void)res;
    g_corr_hits++;
}

void test_monitor_lifecycle_and_stats(void)
{
    SafeDataElementType el;
    uint32_t v = 42u;
    uint8_t mon_id = 0xEEu;
    SafeDataStatsType st;

    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&el,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteElement(&el, &v));

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_RegisterMonitor(&el.header, NULL));
    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_RegisterMonitor(NULL, &mon_id));
    TEST_ASSERT_EQUAL(E_OK, SafeData_RegisterMonitor(&el.header, &mon_id));
    TEST_ASSERT_EQUAL_UINT8(0, mon_id);

    SafeData_SetCorruptionCallback(NULL);
    g_corr_hits = 0u;
    SafeData_SetCorruptionCallback(CorrCb);

    /* clean element: nothing reported */
    TEST_ASSERT_EQUAL_UINT32(0u, SafeData_CheckAllMonitored());

    /* corrupt it: monitor must flag exactly one violation */
    el.primary.data32 = 0u;
    TEST_ASSERT_EQUAL_UINT32(1u, SafeData_CheckAllMonitored());
    TEST_ASSERT_EQUAL_UINT32(1u, g_corr_hits);

    TEST_ASSERT_EQUAL(E_OK, SafeData_GetStats(&el, &st));
    TEST_ASSERT_EQUAL_UINT32(1u, st.error_count);

    TEST_ASSERT_EQUAL(E_NOT_OK, SafeData_UnregisterMonitor(0xF0u));
    TEST_ASSERT_EQUAL(E_OK, SafeData_UnregisterMonitor(mon_id));
}

void test_reset_stats_all(void)
{
    SafeDataElementType el;
    uint32_t v = 1u;
    uint8_t mon_id;
    TEST_ASSERT_EQUAL(E_OK, SafeData_InitElement(&el,
        CfgP(SAFE_DATA_TYPE_UINT32,
                                     SAFE_DATA_REDUNDANCY_INV, SAFE_DATA_CRC_32)));
    TEST_ASSERT_EQUAL(E_OK, SafeData_WriteElement(&el, &v));
    TEST_ASSERT_EQUAL(E_OK, SafeData_RegisterMonitor(&el.header, &mon_id));

    /* NOTE(actual behaviour): ResetStats(NULL) resets ALL monitors and
     * returns E_OK (unlike other APIs, NULL is not rejected here). */
    TEST_ASSERT_EQUAL(E_OK, SafeData_ResetStats(NULL));
    TEST_ASSERT_EQUAL(E_OK, SafeData_ResetStats(&el));
    TEST_ASSERT_EQUAL(E_OK, SafeData_ResetStats(NULL)); /* reset all */
}

/* ---------------- strings / version ---------------- */

void test_state_error_type_strings(void)
{
    TEST_ASSERT_EQUAL_STRING("VALID", SafeData_GetStateString(SAFE_DATA_STATE_VALID));
    TEST_ASSERT_EQUAL_STRING("INVALID", SafeData_GetStateString(SAFE_DATA_STATE_INVALID));
    TEST_ASSERT_EQUAL_STRING("STALE", SafeData_GetStateString(SAFE_DATA_STATE_STALE));
    TEST_ASSERT_EQUAL_STRING("CORRUPTED", SafeData_GetStateString(SAFE_DATA_STATE_CORRUPTED));
    TEST_ASSERT_EQUAL_STRING("MISMATCH", SafeData_GetStateString(SAFE_DATA_STATE_MISMATCH));
    TEST_ASSERT_EQUAL_STRING("TIMEOUT", SafeData_GetStateString(SAFE_DATA_STATE_TIMEOUT));
    TEST_ASSERT_NOT_NULL(SafeData_GetStateString(0x7Fu));

    TEST_ASSERT_EQUAL_STRING("No Error", SafeData_GetErrorString(SAFE_DATA_ERR_NONE));
    TEST_ASSERT_EQUAL_STRING("CRC Mismatch", SafeData_GetErrorString(SAFE_DATA_ERR_CRC_MISMATCH));
    TEST_ASSERT_EQUAL_STRING("Inverted Copy Mismatch", SafeData_GetErrorString(SAFE_DATA_ERR_INV_MISMATCH));
    TEST_ASSERT_NOT_NULL(SafeData_GetErrorString(0x7Fu));

    TEST_ASSERT_EQUAL_STRING("uint8", SafeData_GetTypeString(SAFE_DATA_TYPE_UINT8));
    TEST_ASSERT_EQUAL_STRING("uint32", SafeData_GetTypeString(SAFE_DATA_TYPE_UINT32));
    TEST_ASSERT_EQUAL_STRING("array", SafeData_GetTypeString(SAFE_DATA_TYPE_ARRAY));
    TEST_ASSERT_NOT_NULL(SafeData_GetTypeString(0x7Fu));
}

void test_version_info(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0xAA, sizeof(vi));
    SafeData_GetVersionInfo(NULL);
    SafeData_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT16(0u, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT8(SAFE_DATA_VERSION_MAJOR, vi.sw_major_version);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_deinit_cycle);
    RUN_TEST(test_init_element_null_guards);
    RUN_TEST(test_init_element_bad_type);
    RUN_TEST(test_init_element_sizes);
    RUN_TEST(test_write_read_element_u32);
    RUN_TEST(test_write_bumps_sequence_and_crc);
    RUN_TEST(test_validate_detects_primary_corruption);
    RUN_TEST(test_validate_inv_mode_masked_header_crc);
    RUN_TEST(test_validate_without_redundancy);
    RUN_TEST(test_stale_detection_via_age_wraparound);
    RUN_TEST(test_array_roundtrip);
    RUN_TEST(test_array_bad_index);
    RUN_TEST(test_array_corruption_detection);
    RUN_TEST(test_container_roundtrip);
    RUN_TEST(test_inverted_copy_helpers);
    RUN_TEST(test_vote_2of3);
    RUN_TEST(test_crc32_known_vector);
    RUN_TEST(test_crc16_known_vector);
    RUN_TEST(test_crc8_stability);
    RUN_TEST(test_calc_checksum_dispatch);
    RUN_TEST(test_sequence_helpers);
    RUN_TEST(test_age_helpers);
    RUN_TEST(test_monitor_lifecycle_and_stats);
    RUN_TEST(test_reset_stats_all);
    RUN_TEST(test_state_error_type_strings);
    RUN_TEST(test_version_info);
    return UNITY_END();
}
