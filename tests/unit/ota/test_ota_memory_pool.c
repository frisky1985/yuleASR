/**
 * @file test_ota_memory_pool.c
 * @brief Unit tests for OTA Memory Pool
 * @note MISRA C:2012 Compliant test code
 */

#include "unity.h"
#include "ota_memory_pool.h"
#include "ota_memory_pool.c"  /* Include source for testing */

void setUp(void)
{
    /* Reset pool before each test */
    s_otaPool.initialized = false;
    (void)ota_memory_pool_init();
}

void tearDown(void)
{
    /* Clean up after each test */
}

/* Test pool initialization */
void test_ota_memory_pool_init(void)
{
    /* Reset and reinitialize */
    s_otaPool.initialized = false;
    ota_error_t result = ota_memory_pool_init();
    
    TEST_ASSERT_EQUAL(OTA_ERR_OK, result);
    TEST_ASSERT_TRUE(s_otaPool.initialized);
    
    /* All blocks should be free */
    for (uint8_t i = 0U; i < OTA_POOL_NUM_BLOCKS; i++) {
        TEST_ASSERT_FALSE(s_otaPool.blockUsed[i]);
    }
}

/* Test allocation */
void test_ota_memory_pool_alloc(void)
{
    /* First allocation */
    void *ptr1 = ota_memory_pool_alloc(1024U);
    TEST_ASSERT_NOT_NULL(ptr1);
    
    /* Write to memory */
    (void)memset(ptr1, 0xAA, 1024U);
    
    /* Second allocation */
    void *ptr2 = ota_memory_pool_alloc(1024U);
    TEST_ASSERT_NOT_NULL(ptr2);
    TEST_ASSERT_NOT_EQUAL(ptr1, ptr2);
    
    /* All subsequent allocations */
    void *ptr3 = ota_memory_pool_alloc(1024U);
    TEST_ASSERT_NOT_NULL(ptr3);
    
    void *ptr4 = ota_memory_pool_alloc(1024U);
    TEST_ASSERT_NOT_NULL(ptr4);
    
    /* Should be out of memory now */
    void *ptr5 = ota_memory_pool_alloc(1024U);
    TEST_ASSERT_NULL(ptr5);
}

/* Test free */
void test_ota_memory_pool_free(void)
{
    void *ptr1 = ota_memory_pool_alloc(1024U);
    TEST_ASSERT_NOT_NULL(ptr1);
    
    /* Free and reallocate */
    ota_memory_pool_free(ptr1);
    
    void *ptr2 = ota_memory_pool_alloc(1024U);
    TEST_ASSERT_NOT_NULL(ptr2);
    /* Should get same block back */
    TEST_ASSERT_EQUAL(ptr1, ptr2);
}

/* Test oversize allocation */
void test_ota_memory_pool_alloc_Oversize(void)
{
    /* Try to allocate more than block size */
    void *ptr = ota_memory_pool_alloc(OTA_POOL_BLOCK_SIZE + 1U);
    TEST_ASSERT_NULL(ptr);
}

/* Test free count */
void test_ota_memory_pool_get_free_blocks(void)
{
    TEST_ASSERT_EQUAL_UINT32(OTA_POOL_NUM_BLOCKS, ota_memory_pool_get_free_blocks());
    
    (void)ota_memory_pool_alloc(1024U);
    TEST_ASSERT_EQUAL_UINT32(OTA_POOL_NUM_BLOCKS - 1U, ota_memory_pool_get_free_blocks());
    
    (void)ota_memory_pool_alloc(1024U);
    TEST_ASSERT_EQUAL_UINT32(OTA_POOL_NUM_BLOCKS - 2U, ota_memory_pool_get_free_blocks());
}

/* Test statistics */
void test_ota_memory_pool_get_stats(void)
{
    uint32_t allocCount = 0U;
    uint32_t freeCount = 0U;
    
    ota_memory_pool_get_stats(&allocCount, &freeCount);
    TEST_ASSERT_EQUAL_UINT32(0U, allocCount);
    TEST_ASSERT_EQUAL_UINT32(0U, freeCount);
    
    void *ptr = ota_memory_pool_alloc(1024U);
    (void)ptr;
    
    ota_memory_pool_get_stats(&allocCount, &freeCount);
    TEST_ASSERT_EQUAL_UINT32(1U, allocCount);
}

/* Test null free */
void test_ota_memory_pool_free_Null(void)
{
    /* Free NULL should not crash */
    ota_memory_pool_free(NULL);
    
    /* Pool should still be valid */
    TEST_ASSERT_TRUE(ota_memory_pool_is_initialized());
}

/* Test invalid pointer free */
void test_ota_memory_pool_free_Invalid(void)
{
    uint8_t dummy[100];
    
    /* Free invalid pointer should not crash */
    ota_memory_pool_free(dummy);
    
    /* Pool should still be valid */
    TEST_ASSERT_TRUE(ota_memory_pool_is_initialized());
}

/* Test memory is cleared on free */
void test_ota_memory_pool_free_Cleared(void)
{
    void *ptr = ota_memory_pool_alloc(100U);
    TEST_ASSERT_NOT_NULL(ptr);
    
    /* Write pattern */
    (void)memset(ptr, 0xFF, 100U);
    
    /* Free clears memory */
    ota_memory_pool_free(ptr);
    
    /* Reallocate same block */
    void *ptr2 = ota_memory_pool_alloc(100U);
    TEST_ASSERT_EQUAL(ptr, ptr2);
    
    /* Memory should be cleared */
    uint8_t *bytePtr = (uint8_t *)ptr2;
    for (uint32_t i = 0U; i < 100U; i++) {
        TEST_ASSERT_EQUAL_UINT8(0U, bytePtr[i]);
    }
}

/* Main test runner */
int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_ota_memory_pool_init);
    RUN_TEST(test_ota_memory_pool_alloc);
    RUN_TEST(test_ota_memory_pool_free);
    RUN_TEST(test_ota_memory_pool_alloc_Oversize);
    RUN_TEST(test_ota_memory_pool_get_free_blocks);
    RUN_TEST(test_ota_memory_pool_get_stats);
    RUN_TEST(test_ota_memory_pool_free_Null);
    RUN_TEST(test_ota_memory_pool_free_Invalid);
    RUN_TEST(test_ota_memory_pool_free_Cleared);
    
    return UNITY_END();
}
