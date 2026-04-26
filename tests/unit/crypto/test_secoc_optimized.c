/**
 * @file test_secoc_optimized.c
 * @brief Unit tests for optimized SecOC implementation
 * @note MISRA C:2012 Compliant test code
 */

#include "unity.h"
#include "secoc_core.h"
#include "secoc_optimized.c"  /* Include source for testing */

/* Test setup and teardown */
void setUp(void)
{
    /* Reset static state before each test */
    s_incrementalState.active = false;
    s_incrementalState.accumulatedLength = 0U;
    s_secocBufferInUse = false;
    (void)memset(s_secocStaticBuffer, 0, SECOC_STATIC_BUFFER_SIZE);
}

void tearDown(void)
{
    /* Clean up after each test */
}

/* Test optimized CRC32 calculation */
void test_SecOC_OptimizedCRC32(void)
{
    uint8_t data[] = {0x01U, 0x02U, 0x03U, 0x04U};
    uint32_t crc = SecOC_OptimizedCRC32(data, sizeof(data));
    
    /* CRC should be deterministic */
    uint32_t crc2 = SecOC_OptimizedCRC32(data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT32(crc, crc2);
    
    /* Different data should give different CRC */
    uint8_t data2[] = {0x01U, 0x02U, 0x03U, 0x05U};
    uint32_t crc3 = SecOC_OptimizedCRC32(data2, sizeof(data2));
    TEST_ASSERT_NOT_EQUAL(crc, crc3);
}

/* Test static memory pool allocation */
void test_SecOC_AllocStaticBuffer(void)
{
    /* First allocation should succeed */
    void *ptr1 = SecOC_AllocStaticBuffer(1024U);
    TEST_ASSERT_NOT_NULL(ptr1);
    
    /* Second allocation should fail - buffer in use */
    void *ptr2 = SecOC_AllocStaticBuffer(1024U);
    TEST_ASSERT_NULL(ptr2);
    
    /* Free and reallocate */
    SecOC_FreeStaticBuffer();
    void *ptr3 = SecOC_AllocStaticBuffer(512U);
    TEST_ASSERT_NOT_NULL(ptr3);
    SecOC_FreeStaticBuffer();
}

/* Test incremental verification start */
void test_SecOC_StartIncrementalVerify(void)
{
    /* First start should succeed */
    SecOC_VerifyResultType result = SecOC_StartIncrementalVerify(0U);
    TEST_ASSERT_EQUAL(SECOC_E_OK, result);
    TEST_ASSERT_TRUE(s_incrementalState.active);
    
    /* Second start should fail - already active */
    result = SecOC_StartIncrementalVerify(0U);
    TEST_ASSERT_EQUAL(SECOC_E_VERIFICATION_FAILED, result);
}

/* Test incremental verification update */
void test_SecOC_UpdateIncrementalVerify(void)
{
    uint8_t data[] = {0x01U, 0x02U, 0x03U};
    uint8_t mac[] = {0x00U, 0x00U, 0x00U, 0x00U};
    
    /* Should fail without starting */
    SecOC_VerifyResultType result = SecOC_UpdateIncrementalVerify(data, sizeof(data), mac, sizeof(mac));
    TEST_ASSERT_EQUAL(SECOC_E_VERIFICATION_FAILED, result);
    
    /* Start and update */
    (void)SecOC_StartIncrementalVerify(0U);
    result = SecOC_UpdateIncrementalVerify(data, sizeof(data), mac, sizeof(mac));
    TEST_ASSERT_EQUAL(SECOC_E_OK, result);
    TEST_ASSERT_EQUAL_UINT32(sizeof(data), s_incrementalState.accumulatedLength);
}

/* Test incremental verification finalize */
void test_SecOC_FinalizeIncrementalVerify(void)
{
    uint8_t data[] = {0x01U, 0x02U, 0x03U};
    
    /* Start and update */
    (void)SecOC_StartIncrementalVerify(0U);
    (void)SecOC_UpdateIncrementalVerify(data, sizeof(data), NULL, 0U);
    
    /* Calculate expected CRC */
    uint32_t expectedCrc = SecOC_OptimizedCRC32(data, sizeof(data));
    uint8_t expectedMac[4];
    expectedMac[0] = (uint8_t)((expectedCrc >> 24U) & 0xFFU);
    expectedMac[1] = (uint8_t)((expectedCrc >> 16U) & 0xFFU);
    expectedMac[2] = (uint8_t)((expectedCrc >> 8U) & 0xFFU);
    expectedMac[3] = (uint8_t)(expectedCrc & 0xFFU);
    
    /* Finalize with correct MAC */
    SecOC_VerifyResultType result = SecOC_FinalizeIncrementalVerify(expectedMac, sizeof(expectedMac));
    TEST_ASSERT_EQUAL(SECOC_E_OK, result);
    TEST_ASSERT_FALSE(s_incrementalState.active);
    
    /* Finalize without starting should fail */
    result = SecOC_FinalizeIncrementalVerify(expectedMac, sizeof(expectedMac));
    TEST_ASSERT_EQUAL(SECOC_E_VERIFICATION_FAILED, result);
}

/* Test buffer overflow protection */
void test_SecOC_UpdateIncrementalVerify_Overflow(void)
{
    uint8_t data[SECOC_MAX_DATA_LENGTH];
    uint8_t mac[] = {0x00U};
    
    (void)memset(data, 0xAA, sizeof(data));
    (void)SecOC_StartIncrementalVerify(0U);
    
    /* First update should succeed */
    SecOC_VerifyResultType result = SecOC_UpdateIncrementalVerify(data, SECOC_MAX_DATA_LENGTH, mac, sizeof(mac));
    TEST_ASSERT_EQUAL(SECOC_E_OK, result);
    
    /* Second update should fail - overflow */
    result = SecOC_UpdateIncrementalVerify(data, 1U, mac, sizeof(mac));
    TEST_ASSERT_EQUAL(SECOC_E_VERIFICATION_FAILED, result);
}

/* Test null parameter handling */
void test_SecOC_NullParameters(void)
{
    uint8_t data[] = {0x01U};
    
    /* Null data should fail */
    SecOC_VerifyResultType result = SecOC_UpdateIncrementalVerify(NULL, sizeof(data), NULL, 0U);
    TEST_ASSERT_EQUAL(SECOC_E_VERIFICATION_FAILED, result);
    
    /* Null expected MAC should fail on finalize */
    (void)SecOC_StartIncrementalVerify(0U);
    (void)SecOC_UpdateIncrementalVerify(data, sizeof(data), NULL, 0U);
    result = SecOC_FinalizeIncrementalVerify(NULL, 0U);
    TEST_ASSERT_EQUAL(SECOC_E_VERIFICATION_FAILED, result);
}

/* Main test runner */
int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_SecOC_OptimizedCRC32);
    RUN_TEST(test_SecOC_AllocStaticBuffer);
    RUN_TEST(test_SecOC_StartIncrementalVerify);
    RUN_TEST(test_SecOC_UpdateIncrementalVerify);
    RUN_TEST(test_SecOC_FinalizeIncrementalVerify);
    RUN_TEST(test_SecOC_UpdateIncrementalVerify_Overflow);
    RUN_TEST(test_SecOC_NullParameters);
    
    return UNITY_END();
}
