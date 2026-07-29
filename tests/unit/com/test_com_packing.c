/*
 * test_com_packing.c
 * COM Module Unit Tests - Signal Packing/Unpacking
 */

#include "unity.h"
#include "Com_Private.h"
#include "Com_Cfg.h"

/*==================[Test Setup]===========================================*/

void setUp(void) {
    /* Reset global state before each test */
    memset(&Com_GlobalState, 0, sizeof(Com_GlobalState));
}

void tearDown(void) {
    /* Cleanup after each test */
}

/*==================[Little Endian Tests]==================================*/

void test_extract_signal_little_endian_8bit(void) {
    uint8 data[] = {0xAB, 0xCD, 0xEF, 0x00};
    uint64 result = Com_ExtractSignal(data, 0, 8, COM_LITTLE_ENDIAN);
    TEST_ASSERT_EQUAL_UINT64(0xAB, result);
}

void test_extract_signal_little_endian_16bit(void) {
    uint8 data[] = {0x34, 0x12, 0x00, 0x00};
    uint64 result = Com_ExtractSignal(data, 0, 16, COM_LITTLE_ENDIAN);
    TEST_ASSERT_EQUAL_UINT64(0x1234, result);
}

void test_extract_signal_little_endian_32bit(void) {
    uint8 data[] = {0x78, 0x56, 0x34, 0x12};
    uint64 result = Com_ExtractSignal(data, 0, 32, COM_LITTLE_ENDIAN);
    TEST_ASSERT_EQUAL_UINT64(0x12345678, result);
}

void test_extract_signal_little_endian_offset(void) {
    uint8 data[] = {0x00, 0xAB, 0xCD, 0x00};
    uint64 result = Com_ExtractSignal(data, 8, 16, COM_LITTLE_ENDIAN);
    TEST_ASSERT_EQUAL_UINT64(0xCDAB, result);
}

/*==================[Big Endian Tests]=====================================*/

void test_extract_signal_big_endian_16bit(void) {
    uint8 data[] = {0x12, 0x34, 0x00, 0x00};
    uint64 result = Com_ExtractSignal(data, 0, 16, COM_BIG_ENDIAN);
    TEST_ASSERT_EQUAL_UINT64(0x1234, result);
}

void test_extract_signal_big_endian_32bit(void) {
    uint8 data[] = {0x12, 0x34, 0x56, 0x78};
    uint64 result = Com_ExtractSignal(data, 0, 32, COM_BIG_ENDIAN);
    TEST_ASSERT_EQUAL_UINT64(0x12345678, result);
}

/*==================[Insert Signal Tests]==================================*/

void test_insert_signal_little_endian_16bit(void) {
    uint8 data[4] = {0};
    Com_InsertSignal(data, 0, 16, COM_LITTLE_ENDIAN, 0x1234);
    TEST_ASSERT_EQUAL_UINT8(0x34, data[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, data[1]);
}

void test_insert_signal_big_endian_16bit(void) {
    uint8 data[4] = {0};
    Com_InsertSignal(data, 0, 16, COM_BIG_ENDIAN, 0x1234);
    TEST_ASSERT_EQUAL_UINT8(0x12, data[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34, data[1]);
}

void test_insert_signal_with_offset(void) {
    uint8 data[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    Com_InsertSignal(data, 8, 16, COM_LITTLE_ENDIAN, 0x1234);
    TEST_ASSERT_EQUAL_UINT8(0xFF, data[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34, data[1]);
    TEST_ASSERT_EQUAL_UINT8(0x12, data[2]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, data[3]);
}

/*==================[Round-trip Tests]=====================================*/

void test_roundtrip_little_endian_various_sizes(void) {
    uint8 data[8] = {0};
    
    /* Test 1-bit */
    Com_InsertSignal(data, 0, 1, COM_LITTLE_ENDIAN, 1);
    TEST_ASSERT_EQUAL_UINT64(1, Com_ExtractSignal(data, 0, 1, COM_LITTLE_ENDIAN));
    
    /* Test 4-bit */
    memset(data, 0, 8);
    Com_InsertSignal(data, 0, 4, COM_LITTLE_ENDIAN, 0x0F);
    TEST_ASSERT_EQUAL_UINT64(0x0F, Com_ExtractSignal(data, 0, 4, COM_LITTLE_ENDIAN));
    
    /* Test 12-bit */
    memset(data, 0, 8);
    Com_InsertSignal(data, 0, 12, COM_LITTLE_ENDIAN, 0xABC);
    TEST_ASSERT_EQUAL_UINT64(0xABC, Com_ExtractSignal(data, 0, 12, COM_LITTLE_ENDIAN));
}

void test_roundtrip_big_endian_various_sizes(void) {
    uint8 data[8] = {0};
    
    /* Test 16-bit */
    Com_InsertSignal(data, 0, 16, COM_BIG_ENDIAN, 0xBEEF);
    TEST_ASSERT_EQUAL_UINT64(0xBEEF, Com_ExtractSignal(data, 0, 16, COM_BIG_ENDIAN));
    
    /* Test 24-bit */
    memset(data, 0, 8);
    Com_InsertSignal(data, 0, 24, COM_BIG_ENDIAN, 0xABCDEF);
    TEST_ASSERT_EQUAL_UINT64(0xABCDEF, Com_ExtractSignal(data, 0, 24, COM_BIG_ENDIAN));
}

/*==================[Edge Cases]===========================================*/

void test_extract_64bit_full(void) {
    uint8 data[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    uint64 result = Com_ExtractSignal(data, 0, 64, COM_LITTLE_ENDIAN);
    TEST_ASSERT_EQUAL_UINT64(0xEFCDAB8967452301ULL, result);
}

void test_insert_at_bit_boundary(void) {
    uint8 data[2] = {0x00, 0x00};
    /* Insert 4 bits at bit position 4 */
    Com_InsertSignal(data, 4, 4, COM_LITTLE_ENDIAN, 0x0F);
    TEST_ASSERT_EQUAL_UINT8(0xF0, data[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, data[1]);
}

/*==================[Main]=================================================*/

int main(void) {
    UNITY_BEGIN();
    
    /* Little Endian Tests */
    RUN_TEST(test_extract_signal_little_endian_8bit);
    RUN_TEST(test_extract_signal_little_endian_16bit);
    RUN_TEST(test_extract_signal_little_endian_32bit);
    RUN_TEST(test_extract_signal_little_endian_offset);
    
    /* Big Endian Tests */
    RUN_TEST(test_extract_signal_big_endian_16bit);
    RUN_TEST(test_extract_signal_big_endian_32bit);
    
    /* Insert Tests */
    RUN_TEST(test_insert_signal_little_endian_16bit);
    RUN_TEST(test_insert_signal_big_endian_16bit);
    RUN_TEST(test_insert_signal_with_offset);
    
    /* Round-trip Tests */
    RUN_TEST(test_roundtrip_little_endian_various_sizes);
    RUN_TEST(test_roundtrip_big_endian_various_sizes);
    
    /* Edge Cases */
    RUN_TEST(test_extract_64bit_full);
    RUN_TEST(test_insert_at_bit_boundary);
    
    return UNITY_END();
}
