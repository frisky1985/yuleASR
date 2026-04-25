/******************************************************************************
 * @file    test_double_bit_errors.c
 * @brief   Double-bit error detection tests for RAM ECC module
 *
 * CRITICAL FIX: Added missing double-bit error test coverage (C2)
 * 
 * Tests SECDED capability to detect (but not correct) double-bit errors
 * 
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "unity.h"
#include "ecc_encoder.h"
#include "ecc_checker.h"
#include <string.h>

/* Test setup and teardown */
void setUp(void)
{
    /* Initialize ECC modules */
    EccEncoder_Init(NULL);
    EccChecker_Init(NULL);
}

void tearDown(void)
{
    /* Cleanup */
    EccChecker_DeInit();
    EccEncoder_DeInit();
}

/******************************************************************************
 * Helper Functions
 ******************************************************************************/

/**
 * @brief Inject double-bit error into data
 * @param data Pointer to data
 * @param bit1 First bit position to flip
 * @param bit2 Second bit position to flip
 */
static void inject_double_bit_error_uint32(uint32_t *data, uint8_t bit1, uint8_t bit2)
{
    *data ^= (1U << bit1);
    *data ^= (1U << bit2);
}

static void inject_double_bit_error_uint64(uint64_t *data, uint8_t bit1, uint8_t bit2)
{
    *data ^= (1ULL << bit1);
    *data ^= (1ULL << bit2);
}

/**
 * @brief Inject double-bit error into ECC code
 */
static void inject_double_bit_error_ecc(uint8_t *ecc, uint8_t bit1, uint8_t bit2)
{
    *ecc ^= (1U << bit1);
    *ecc ^= (1U << bit2);
}

/******************************************************************************
 * Double-Bit Error Detection Tests - 32-bit Data
 ******************************************************************************/

/**
 * @brief Test double-bit error detection in 32-bit data
 * Verifies that 2 bit flips in data are detected as uncorrectable
 */
void test_double_bit_error_in_data_32bit(void)
{
    uint32_t original_data = 0x12345678U;
    uint8_t ecc_code;
    EccCheckedType result;
    
    /* Encode original data */
    TEST_ASSERT_EQUAL(E_OK, EccEncoder_Encode32(original_data, &ecc_code));
    
    /* Test various double-bit error patterns */
    for (uint8_t bit1 = 0; bit1 < 31; bit1++) {
        for (uint8_t bit2 = bit1 + 1; bit2 < 32; bit2++) {
            uint32_t corrupted_data = original_data;
            inject_double_bit_error_uint32(&corrupted_data, bit1, bit2);
            
            /* Check should detect double-bit error */
            Std_ReturnType status = EccChecker_Check32(corrupted_data, ecc_code, &result);
            
            /* Double-bit errors should be detected */
            TEST_ASSERT_TRUE(
                result.result == ECC_CHECK_DOUBLE_ERROR || 
                result.result == ECC_CHECK_ERROR
            );
            
            /* Should NOT be marked as OK */
            TEST_ASSERT_NOT_EQUAL(ECC_CHECK_OK, result.result);
            
            /* Should NOT be marked as single-bit error */
            TEST_ASSERT_NOT_EQUAL(ECC_CHECK_SINGLE_ERROR, result.result);
        }
    }
}

/**
 * @brief Test double-bit error detection in ECC code (32-bit)
 */
void test_double_bit_error_in_ecc_code_32bit(void)
{
    uint32_t original_data = 0xABCDEF00U;
    uint8_t original_ecc;
    EccCheckedType result;
    
    /* Encode original data */
    TEST_ASSERT_EQUAL(E_OK, EccEncoder_Encode32(original_data, &original_ecc));
    
    /* Test double-bit errors in ECC code */
    for (uint8_t bit1 = 0; bit1 < 6; bit1++) {
        for (uint8_t bit2 = bit1 + 1; bit2 < 7; bit2++) {
            uint8_t corrupted_ecc = original_ecc;
            inject_double_bit_error_ecc(&corrupted_ecc, bit1, bit2);
            
            /* Check should detect error */
            Std_ReturnType status = EccChecker_Check32(original_data, corrupted_ecc, &result);
            
            /* Should detect error */
            TEST_ASSERT_NOT_EQUAL(ECC_CHECK_OK, result.result);
        }
    }
}

/**
 * @brief Test double-bit error with one bit in data, one in ECC
 */
void test_double_bit_error_data_and_ecc_32bit(void)
{
    uint32_t original_data = 0x55555555U;
    uint8_t original_ecc;
    EccCheckedType result;
    
    TEST_ASSERT_EQUAL(E_OK, EccEncoder_Encode32(original_data, &original_ecc));
    
    /* Corrupt one bit in data and one in ECC */
    uint32_t corrupted_data = original_data ^ 0x00000001U;  /* Bit 0 of data */
    uint8_t corrupted_ecc = original_ecc ^ 0x01U;            /* Bit 0 of ECC */
    
    Std_ReturnType status = EccChecker_Check32(corrupted_data, corrupted_ecc, &result);
    
    /* Should detect error (cannot correct reliably) */
    TEST_ASSERT_NOT_EQUAL(ECC_CHECK_OK, result.result);
}

/******************************************************************************
 * Double-Bit Error Detection Tests - 64-bit Data
 ******************************************************************************/

/**
 * @brief Test double-bit error detection in 64-bit data
 */
void test_double_bit_error_in_data_64bit(void)
{
    uint64_t original_data = 0x123456789ABCDEF0ULL;
    uint8_t ecc_code;
    EccCheckedType result;
    
    /* Encode original data */
    TEST_ASSERT_EQUAL(E_OK, EccEncoder_Encode64(original_data, &ecc_code));
    
    /* Test sample of double-bit error patterns (not all combinations - too many) */
    uint8_t test_pairs[][2] = {
        {0, 1}, {0, 31}, {0, 63},
        {15, 16}, {31, 32}, {47, 48},
        {1, 62}, {7, 56}, {23, 40}
    };
    
    for (size_t i = 0; i < sizeof(test_pairs) / sizeof(test_pairs[0]); i++) {
        uint64_t corrupted_data = original_data;
        inject_double_bit_error_uint64(&corrupted_data, test_pairs[i][0], test_pairs[i][1]);
        
        Std_ReturnType status = EccChecker_Check64(corrupted_data, ecc_code, &result);
        
        /* Should detect double-bit error */
        TEST_ASSERT_TRUE(
            result.result == ECC_CHECK_DOUBLE_ERROR || 
            result.result == ECC_CHECK_ERROR
        );
        
        TEST_ASSERT_NOT_EQUAL(ECC_CHECK_OK, result.result);
        TEST_ASSERT_NOT_EQUAL(ECC_CHECK_SINGLE_ERROR, result.result);
    }
}

/**
 * @brief Test double-bit error in ECC code (64-bit)
 */
void test_double_bit_error_in_ecc_code_64bit(void)
{
    uint64_t original_data = 0x0F0F0F0F0F0F0F0FULL;
    uint8_t original_ecc;
    EccCheckedType result;
    
    TEST_ASSERT_EQUAL(E_OK, EccEncoder_Encode64(original_data, &original_ecc));
    
    /* Test double-bit errors in 8-bit ECC code */
    for (uint8_t bit1 = 0; bit1 < 7; bit1++) {
        for (uint8_t bit2 = bit1 + 1; bit2 < 8; bit2++) {
            uint8_t corrupted_ecc = original_ecc;
            inject_double_bit_error_ecc(&corrupted_ecc, bit1, bit2);
            
            Std_ReturnType status = EccChecker_Check64(original_data, corrupted_ecc, &result);
            
            /* Should detect error */
            TEST_ASSERT_NOT_EQUAL(ECC_CHECK_OK, result.result);
        }
    }
}

/******************************************************************************
 * Adjacent Bit Error Tests (relevant for hardware failures)
 ******************************************************************************/

/**
 * @brief Test adjacent double-bit errors
 * Simulates hardware failures affecting adjacent bits
 */
void test_adjacent_double_bit_errors_32bit(void)
{
    uint32_t original_data = 0xAAAAAAAAU;
    uint8_t ecc_code;
    EccCheckedType result;
    
    TEST_ASSERT_EQUAL(E_OK, EccEncoder_Encode32(original_data, &ecc_code));
    
    /* Test all adjacent bit pairs */
    for (uint8_t bit = 0; bit < 31; bit++) {
        uint32_t corrupted_data = original_data;
        inject_double_bit_error_uint32(&corrupted_data, bit, bit + 1);
        
        Std_ReturnType status = EccChecker_Check32(corrupted_data, ecc_code, &result);
        
        /* Adjacent errors should be detected */
        TEST_ASSERT_NOT_EQUAL(ECC_CHECK_OK, result.result);
        TEST_ASSERT_NOT_EQUAL(ECC_CHECK_SINGLE_ERROR, result.result);
    }
}

void test_adjacent_double_bit_errors_64bit(void)
{
    uint64_t original_data = 0x5555555555555555ULL;
    uint8_t ecc_code;
    EccCheckedType result;
    
    TEST_ASSERT_EQUAL(E_OK, EccEncoder_Encode64(original_data, &ecc_code));
    
    /* Test sample of adjacent pairs */
    uint8_t adjacent_pairs[][2] = {
        {0, 1}, {15, 16}, {31, 32}, {47, 48}, {62, 63}
    };
    
    for (size_t i = 0; i < sizeof(adjacent_pairs) / sizeof(adjacent_pairs[0]); i++) {
        uint64_t corrupted_data = original_data;
        inject_double_bit_error_uint64(&corrupted_data, adjacent_pairs[i][0], adjacent_pairs[i][1]);
        
        Std_ReturnType status = EccChecker_Check64(corrupted_data, ecc_code, &result);
        
        TEST_ASSERT_NOT_EQUAL(ECC_CHECK_OK, result.result);
    }
}

/******************************************************************************
 * Statistical Tests
 ******************************************************************************/

/**
 * @brief Test error statistics tracking for double-bit errors
 */
void test_double_bit_error_statistics(void)
{
    uint32_t original_data = 0x11111111U;
    uint8_t ecc_code;
    EccCheckedType result;
    
    /* Reset statistics */
    EccChecker_ResetStats();
    
    /* Get initial stats */
    const EccErrorStatsType *stats_before = EccChecker_GetStats();
    uint32_t errors_before = stats_before->doubleBitErrors;
    
    /* Inject and detect several double-bit errors */
    for (int i = 0; i < 5; i++) {
        uint32_t corrupted_data = original_data;
        inject_double_bit_error_uint32(&corrupted_data, i * 2, i * 2 + 1);
        
        EccEncoder_Encode32(original_data, &ecc_code);
        EccChecker_Check32(corrupted_data, ecc_code, &result);
    }
    
    /* Get updated stats */
    const EccErrorStatsType *stats_after = EccChecker_GetStats();
    
    /* Verify error count increased */
    TEST_ASSERT_GREATER_THAN(errors_before, stats_after->doubleBitErrors);
    TEST_ASSERT_EQUAL(5, stats_after->doubleBitErrors - errors_before);
}

/******************************************************************************
 * Run All Tests
 ******************************************************************************/

int main(void)
{
    UNITY_BEGIN();
    
    /* 32-bit double-bit error tests */
    RUN_TEST(test_double_bit_error_in_data_32bit);
    RUN_TEST(test_double_bit_error_in_ecc_code_32bit);
    RUN_TEST(test_double_bit_error_data_and_ecc_32bit);
    
    /* 64-bit double-bit error tests */
    RUN_TEST(test_double_bit_error_in_data_64bit);
    RUN_TEST(test_double_bit_error_in_ecc_code_64bit);
    
    /* Adjacent bit error tests */
    RUN_TEST(test_adjacent_double_bit_errors_32bit);
    RUN_TEST(test_adjacent_double_bit_errors_64bit);
    
    /* Statistical tests */
    RUN_TEST(test_double_bit_error_statistics);
    
    return UNITY_END();
}
