/******************************************************************************
 * @file    test_ram_ecc.c
 * @brief   Unit tests for RAM ECC protection module
 *
 * Tests for:
 * - ECC encoder functionality
 * - ECC checker (error detection and correction)
 * - RAM monitor
 * - ECC allocator
 * - Unified safety interface
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "ram_safety.h"

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            tests_passed++; \
            printf("[PASS] %s\n", msg); \
        } else { \
            tests_failed++; \
            printf("[FAIL] %s\n", msg); \
        } \
    } while(0)

/******************************************************************************
 * Test: ECC Encoder
 ******************************************************************************/
static void test_ecc_encoder(void)
{
    Std_ReturnType status;
    uint8_t ecc_code;
    uint32_t test_data;
    EccEncodedType result;
    
    printf("\n=== Testing ECC Encoder ===\n");
    
    /* Test initialization */
    {
        EccEncoderConfigType config;
        memset(&config, 0, sizeof(config));
        config.mode = ECC_MODE_32BIT;
        config.asil_level = ASIL_D;
        
        status = EccEncoder_Init(&config);
        TEST_ASSERT(status == E_OK, "ECC Encoder initialization");
    }
    
    /* Test encoding */
    test_data = 0x12345678U;
    status = EccEncoder_Encode32(test_data, &ecc_code);
    TEST_ASSERT(status == E_OK, "ECC Encode32 basic");
    TEST_ASSERT(ecc_code != 0, "ECC code is non-zero");
    
    /* Test encoding same data produces same ECC */
    {
        uint8_t ecc_code2;
        EccEncoder_Encode32(test_data, &ecc_code2);
        TEST_ASSERT(ecc_code == ecc_code2, "Consistent ECC encoding");
    }
    
    /* Test different data produces different ECC */
    {
        uint8_t ecc_code2;
        EccEncoder_Encode32(test_data ^ 0xFFFFFFFFU, &ecc_code2);
        TEST_ASSERT(ecc_code != ecc_code2, "Different data different ECC");
    }
    
    /* Test full encode function */
    status = EccEncoder_EncodeFull32(test_data, &result);
    TEST_ASSERT(status == E_OK, "ECC EncodeFull32");
    TEST_ASSERT(result.data == test_data, "EncodeFull preserves data");
    TEST_ASSERT(result.mode == ECC_MODE_32BIT, "EncodeFull mode correct");
    
    /* Test batch encoding */
    {
        uint32_t data_batch[4] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
        uint8_t ecc_batch[4];
        status = EccEncoder_BatchEncode32(data_batch, ecc_batch, 4);
        TEST_ASSERT(status == E_OK, "ECC Batch encoding");
    }
    
    /* Test get state */
    {
        const EccEncoderStateType *state = EccEncoder_GetState();
        TEST_ASSERT(state != NULL, "Encoder state accessible");
        TEST_ASSERT(state->initialized == TRUE, "Encoder marked initialized");
        TEST_ASSERT(state->encode_count > 0, "Encode count incremented");
    }
    
    EccEncoder_DeInit();
    printf("ECC Encoder tests complete.\n");
}

/******************************************************************************
 * Test: ECC Checker
 ******************************************************************************/
static void test_ecc_checker(void)
{
    Std_ReturnType status;
    EccCheckerConfigType config;
    uint32_t test_data = 0xABCDEF01U;
    uint8_t ecc_code;
    EccCheckResultType result;
    
    printf("\n=== Testing ECC Checker ===\n");
    
    /* Initialize */
    memset(&config, 0, sizeof(config));
    config.auto_correct = TRUE;
    config.log_errors = TRUE;
    status = EccChecker_Init(&config);
    TEST_ASSERT(status == E_OK, "ECC Checker initialization");
    
    /* Re-initialize encoder for testing */
    {
        EccEncoderConfigType enc_config;
        memset(&enc_config, 0, sizeof(enc_config));
        enc_config.mode = ECC_MODE_32BIT;
        EccEncoder_Init(&enc_config);
    }
    
    /* Test correct data (no error) */
    EccEncoder_Encode32(test_data, &ecc_code);
    status = EccChecker_CheckAndCorrect32(test_data, ecc_code, &result);
    TEST_ASSERT(status == E_OK, "Check correct data returns OK");
    TEST_ASSERT(result.error_type == ECC_ERROR_NONE, "No error detected");
    TEST_ASSERT(result.corrected_data == test_data, "Data unchanged");
    
    /* Test single-bit error correction */
    {
        uint32_t corrupted_data = test_data ^ (1U << 5);  /* Flip bit 5 */
        status = EccChecker_CheckAndCorrect32(corrupted_data, ecc_code, &result);
        TEST_ASSERT(status == E_OK, "Single-bit error corrected");
        TEST_ASSERT(result.error_type == ECC_ERROR_SINGLE_BIT, "Single-bit error detected");
        TEST_ASSERT(result.corrected_data == test_data, "Data corrected");
        TEST_ASSERT(result.was_corrected == TRUE, "Correction applied");
    }
    
    /* Test ECC in data only (correctable) */
    {
        uint8_t corrupted_ecc = ecc_code ^ (1U << 2);  /* Flip ECC bit */
        status = EccChecker_CheckAndCorrect32(test_data, corrupted_ecc, &result);
        TEST_ASSERT(status == E_OK, "ECC error detected, data correct");
    }
    
    /* Test error statistics */
    {
        const EccErrorStatsType *stats = EccChecker_GetStats();
        TEST_ASSERT(stats != NULL, "Statistics accessible");
        TEST_ASSERT(stats->total_checks > 0, "Checks counted");
        TEST_ASSERT(stats->single_bit_errors > 0, "Errors counted");
        TEST_ASSERT(stats->corrections_made > 0, "Corrections counted");
    }
    
    /* Test error strings */
    {
        const char *str = EccChecker_GetErrorString(ECC_ERROR_SINGLE_BIT);
        TEST_ASSERT(str != NULL, "Error string returned");
        TEST_ASSERT(strlen(str) > 0, "Error string non-empty");
    }
    
    EccChecker_DeInit();
    EccEncoder_DeInit();
    printf("ECC Checker tests complete.\n");
}

/******************************************************************************
 * Test: ECC Allocator
 ******************************************************************************/
static void test_ecc_allocator(void)
{
    Std_ReturnType status;
    void *ptr1, *ptr2;
    uint32_t free_size, heap_size;
    
    printf("\n=== Testing ECC Allocator ===\n");
    
    /* Initialize allocator */
    status = EccAllocator_Init(NULL);  /* Use defaults */
    TEST_ASSERT(status == E_OK, "ECC Allocator initialization");
    
    /* Basic allocation */
    ptr1 = EccAllocator_Malloc(100, ECC_ALLOC_FLAG_NONE);
    TEST_ASSERT(ptr1 != NULL, "Basic allocation");
    
    /* Write and verify */
    if (ptr1 != NULL) {
        memset(ptr1, 0xAA, 100);
        uint8_t *p = (uint8_t*)ptr1;
        TEST_ASSERT(p[0] == 0xAA && p[99] == 0xAA, "Memory writable");
    }
    
    /* Calloc test */
    ptr2 = EccAllocator_Calloc(1, 50, ECC_ALLOC_FLAG_ZERO);
    TEST_ASSERT(ptr2 != NULL, "Calloc allocation");
    if (ptr2 != NULL) {
        uint8_t *p = (uint8_t*)ptr2;
        TEST_ASSERT(p[0] == 0 && p[49] == 0, "Calloc zeroes memory");
    }
    
    /* Free test */
    status = EccAllocator_Free(ptr1);
    TEST_ASSERT(status == E_OK, "Free allocation");
    
    /* Double-free detection */
    status = EccAllocator_Free(ptr1);
    TEST_ASSERT(status == E_ECC_ALLOC_DOUBLE_FREE, "Double-free detected");
    
    /* Heap statistics */
    {
        const EccAllocStatsType *stats = EccAllocator_GetStats();
        TEST_ASSERT(stats != NULL, "Statistics accessible");
        TEST_ASSERT(stats->total_allocations > 0, "Allocations counted");
        TEST_ASSERT(stats->total_frees > 0, "Frees counted");
    }
    
    /* Heap size queries */
    heap_size = EccAllocator_GetHeapSize();
    TEST_ASSERT(heap_size > 0, "Heap size query");
    
    free_size = EccAllocator_GetFreeSize();
    TEST_ASSERT(free_size > 0, "Free size query");
    TEST_ASSERT(free_size <= heap_size, "Free size <= heap size");
    
    /* Cleanup */
    EccAllocator_Free(ptr2);
    
    EccAllocator_DeInit();
    printf("ECC Allocator tests complete.\n");
}

/******************************************************************************
 * Test: RAM Monitor
 ******************************************************************************/
static void test_ram_monitor(void)
{
    Std_ReturnType status;
    RamMonitorConfigType config;
    RamRegionType region;
    
    printf("\n=== Testing RAM Monitor ===\n");
    
    /* Initialize */
    memset(&config, 0, sizeof(config));
    config.max_regions = RAM_MONITOR_MAX_REGIONS;
    config.auto_start = FALSE;
    
    status = RamMonitor_Init(&config);
    TEST_ASSERT(status == E_OK, "RAM Monitor initialization");
    
    /* Register a test region */
    memset(&region, 0, sizeof(region));
    region.region_id = 1;
    region.start_address = 0x20000000U;  /* Example address */
    region.size_bytes = 1024;
    region.word_size = 4;
    region.scan_interval_ms = 100;
    region.priority = 1;
    region.enabled = TRUE;
    region.attributes = RAM_REGION_ATTR_SAFEGUARD;
    
    status = RamMonitor_RegisterRegion(&region);
    TEST_ASSERT(status == E_OK, "Region registration");
    
    /* Verify region count */
    TEST_ASSERT(RamMonitor_GetRegionCount() == 1, "Region count correct");
    
    /* Get region info */
    {
        const RamRegionType *reg = RamMonitor_GetRegion(1);
        TEST_ASSERT(reg != NULL, "Get region info");
        TEST_ASSERT(reg->region_id == 1, "Region ID correct");
    }
    
    /* Start/Stop monitor */
    status = RamMonitor_Start();
    TEST_ASSERT(status == E_OK, "Monitor start");
    
    status = RamMonitor_Stop();
    TEST_ASSERT(status == E_OK, "Monitor stop");
    
    /* Unregister region */
    status = RamMonitor_UnregisterRegion(1);
    TEST_ASSERT(status == E_OK, "Region unregistration");
    TEST_ASSERT(RamMonitor_GetRegionCount() == 0, "Region count after remove");
    
    RamMonitor_DeInit();
    printf("RAM Monitor tests complete.\n");
}

/******************************************************************************
 * Test: Unified RAM Safety Interface
 ******************************************************************************/
static void test_ram_safety_interface(void)
{
    Std_ReturnType status;
    RamSafetyConfigType config;
    void *ptr;
    uint8_t ecc_code;
    
    printf("\n=== Testing RAM Safety Interface ===\n");
    
    /* Initialize safety module */
    memset(&config, 0, sizeof(config));
    config.check_mode = RAM_SAFETY_CHECK_FULL;
    config.enable_monitor = TRUE;
    config.enable_allocator = TRUE;
    config.asil_level = ASIL_D;
    
    memset(&config.encoder_config, 0, sizeof(config.encoder_config));
    config.encoder_config.mode = ECC_MODE_32BIT;
    config.encoder_config.asil_level = ASIL_D;
    
    memset(&config.checker_config, 0, sizeof(config.checker_config));
    config.checker_config.auto_correct = TRUE;
    config.checker_config.log_errors = TRUE;
    
    memset(&config.monitor_config, 0, sizeof(config.monitor_config));
    config.monitor_config.max_regions = 16;
    config.monitor_config.auto_start = FALSE;
    
    status = RamSafety_Init(&config);
    TEST_ASSERT(status == E_OK, "RAM Safety initialization");
    TEST_ASSERT(RamSafety_IsInitialized() == TRUE, "Initialization status");
    
    /* Test wrapper functions */
    status = RamSafety_Encode32(0x12345678U, &ecc_code);
    TEST_ASSERT(status == E_OK, "Safety interface encode");
    
    ptr = RamSafety_Malloc(256);
    TEST_ASSERT(ptr != NULL, "Safety interface malloc");
    
    if (ptr != NULL) {
        /* Test safe memory operations */
        status = RamSafety_MemSet(ptr, 0xBB, 256, RAM_OP_FLAG_VERIFY_DST);
        TEST_ASSERT(status == E_OK, "Safe memset");
        
        /* Test memory map */
        status = RamSafety_MapMemory(0x10000000U, 0x20000000U, 4096, 0);
        TEST_ASSERT(status == E_OK, "Memory mapping");
        
        RamSafety_Free(ptr);
    }
    
    /* Quick check */
    status = RamSafety_QuickCheck();
    TEST_ASSERT(status == E_OK, "Quick check");
    
    /* Get status */
    {
        const RamSafetyStatusType *s = RamSafety_GetStatus();
        TEST_ASSERT(s != NULL, "Get status");
        TEST_ASSERT(s->initialized == TRUE, "Status shows initialized");
    }
    
    RamSafety_DeInit();
    TEST_ASSERT(RamSafety_IsInitialized() == FALSE, "After deinit");
    
    printf("RAM Safety Interface tests complete.\n");
}

/******************************************************************************
 * Main Test Entry
 ******************************************************************************/
int main(void)
{
    printf("========================================\n");
    printf("RAM ECC Protection Module Test Suite\n");
    printf("========================================\n");
    
    test_ecc_encoder();
    test_ecc_checker();
    test_ecc_allocator();
    test_ram_monitor();
    test_ram_safety_interface();
    
    printf("\n========================================\n");
    printf("Test Summary: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");
    
    return (tests_failed > 0) ? 1 : 0;
}
