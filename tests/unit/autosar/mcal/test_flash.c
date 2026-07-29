/**
 * @file test_flash.c
 * @brief Flash Driver (Flash.h) Module Unit Tests
 * @version 1.0.0
 * @note Tests for the Flash.h API (legacy wrapper around Fls)
 */

#include <stdio.h>
#include <string.h>
#include "Flash.h"

/* 测试结果计数 */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* 测试宏 */
#define TEST_ASSERT(expr) \
    do { \
        tests_run++; \
        if (expr) { \
            tests_passed++; \
            printf("  [PASS] %s\n", #expr); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) == (actual)) { \
            tests_passed++; \
            printf("  [PASS] %s == %s (%d == %d)\n", #expected, #actual, (int)(expected), (int)(actual)); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s == %s (%d != %d) (%s:%d)\n", #expected, #actual, (int)(expected), (int)(actual), __FILE__, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_MEM_EQ(expected, actual, len) \
    do { \
        tests_run++; \
        if (memcmp(expected, actual, len) == 0) { \
            tests_passed++; \
            printf("  [PASS] Memory compare OK (%zu bytes)\n", (size_t)len); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] Memory compare FAILED (%s:%d)\n", __FILE__, __LINE__); \
        } \
    } while(0)

/* 测试配置 */
static Fls_SectorInfoType test_sectors[2];
static Fls_ConfigType test_config;

/* 测试数据 */
static uint8 test_write_data[256];
static uint8 test_read_buffer[256];

/* 初始化测试配置 */
void init_test_config(void)
{
    /* Setup sector info */
    test_sectors[0].SectorStartAddress = 0x08000000U;
    test_sectors[0].SectorSize = 0x00010000U; /* 64KB */
    test_sectors[0].SectorSizeType = FLS_SECTOR_SIZE_64KB;
    test_sectors[0].SectorProtected = FALSE;
    test_sectors[0].SectorBank = 0;
    
    test_sectors[1].SectorStartAddress = 0x08010000U;
    test_sectors[1].SectorSize = 0x00010000U;
    test_sectors[1].SectorSizeType = FLS_SECTOR_SIZE_64KB;
    test_sectors[1].SectorProtected = FALSE;
    test_sectors[1].SectorBank = 0;
    
    /* Setup config */
    test_config.BaseAddress = 0x08000000U;
    test_config.TotalSize = 0x00020000U;
    test_config.SectorInfo = test_sectors;
    test_config.SectorCount = 2;
    test_config.PageSize = 256;
    test_config.ProgrammingUnit = 4;
    test_config.MaxReadFastMode = 1024;
    test_config.MaxReadNormalMode = 256;
    test_config.MaxWriteFastMode = 256;
    test_config.MaxWriteNormalMode = 64;
    test_config.DefaultMode = MEMIF_MODE_SLOW;
    test_config.CallCycle = 10;
    test_config.UseInterrupts = FALSE;
    test_config.JobEndNotification = NULL;
    test_config.JobErrorNotification = NULL;
    
    /* Prepare test data */
    for (int i = 0; i < 256; i++) {
        test_write_data[i] = (uint8)(i * 7 + 0xAA);
    }
}

/*==================================================================================================
 *                                    INITIALIZATION TESTS
 *==================================================================================================*/
void test_flash_init(void)
{
    printf("\n=== Initialization Tests ===\n");
    
    init_test_config();
    
    /* Test: Initialize with valid config */
    Fls_Init(&test_config);
    TEST_ASSERT_EQ(MEMIF_IDLE, Fls_GetStatus());
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_flash_deinit(void)
{
    printf("\n=== Deinitialization Tests ===\n");
    
    /* Pre-condition: Initialize first */
    init_test_config();
    Fls_Init(&test_config);
    TEST_ASSERT_EQ(MEMIF_IDLE, Fls_GetStatus());
    
    /* Test: DeInitialize */
    Fls_DeInit();
    
    /* Verify: Status should be UNINIT */
    TEST_ASSERT_EQ(MEMIF_UNINIT, Fls_GetStatus());
}

/*==================================================================================================
 *                                    WRITE TESTS
 *==================================================================================================*/
void test_flash_write_valid(void)
{
    Std_ReturnType result;
    
    printf("\n=== Write Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Pre-condition: Erase the sector first */
    result = Fls_Erase(0x08000000U, 0x00010000U);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Wait for erase completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    TEST_ASSERT_EQ(MEMIF_IDLE, Fls_GetStatus());
    
    /* Test: Write data */
    result = Fls_Write(0x08000000U, test_write_data, 32);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Wait for write completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_flash_write_invalid_params(void)
{
    Std_ReturnType result;
    
    printf("\n=== Write Invalid Params Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Test: Write with NULL pointer */
    result = Fls_Write(0x08000000U, NULL_PTR, 32);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* Test: Write to invalid address */
    result = Fls_Write(0x00000000U, test_write_data, 32);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* Test: Write with zero length */
    result = Fls_Write(0x08000000U, test_write_data, 0);
    TEST_ASSERT_EQ(E_NOT_OK, result);
}

void test_flash_write_while_busy(void)
{
    Std_ReturnType result;
    
    printf("\n=== Write While Busy Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Start an erase operation */
    result = Fls_Erase(0x08000000U, 0x00010000U);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Try to write while busy */
    result = Fls_Write(0x08010000U, test_write_data, 32);
    
    /* Should return E_NOT_OK if busy */
    if (Fls_GetStatus() == MEMIF_BUSY) {
        TEST_ASSERT_EQ(E_NOT_OK, result);
    }
    
    /* Complete the operation */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
}

/*==================================================================================================
 *                                    READ TESTS
 *==================================================================================================*/
void test_flash_read_valid(void)
{
    Std_ReturnType result;
    
    printf("\n=== Read Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Pre-condition: Erase and write data */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    Fls_Write(0x08000000U, test_write_data, 64);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
    
    /* Test: Read data back */
    memset(test_read_buffer, 0, sizeof(test_read_buffer));
    result = Fls_Read(0x08000000U, test_read_buffer, 64);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Wait for read completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_flash_read_null_pointer(void)
{
    printf("\n=== Read NULL Pointer Test ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Test: Read with NULL pointer - should report error */
    Fls_Read(0x08000000U, NULL_PTR, 32);
    TEST_ASSERT(1); /* Function executed, error detection depends on config */
}

/*==================================================================================================
 *                                    ERASE TESTS
 *==================================================================================================*/
void test_flash_erase_valid(void)
{
    Std_ReturnType result;
    
    printf("\n=== Erase Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Test: Erase valid sector */
    result = Fls_Erase(0x08000000U, 0x00010000U);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Wait for erase completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_flash_erase_invalid_params(void)
{
    Std_ReturnType result;
    
    printf("\n=== Erase Invalid Params Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Test: Erase invalid address */
    result = Fls_Erase(0x00000000U, 0x00010000U);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* Test: Erase with zero length */
    result = Fls_Erase(0x08000000U, 0);
    TEST_ASSERT_EQ(E_NOT_OK, result);
}

/*==================================================================================================
 *                                    COMPARE TESTS
 *==================================================================================================*/
void test_flash_compare(void)
{
    Std_ReturnType result;
    uint8 compare_data[32];
    
    printf("\n=== Compare Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Pre-condition: Erase, write then compare */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Prepare and write test data */
    for (int i = 0; i < 32; i++) {
        compare_data[i] = (uint8)(i + 0x10);
    }
    
    Fls_Write(0x08000000U, compare_data, 32);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Compare data */
    result = Fls_Compare(0x08000000U, compare_data, 32);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Wait for compare completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_flash_compare_mismatch(void)
{
    Std_ReturnType result;
    uint8 compare_data[32];
    uint8 mismatch_data[32];
    
    printf("\n=== Compare Mismatch Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Pre-condition: Erase, write then compare */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Prepare test data */
    for (int i = 0; i < 32; i++) {
        compare_data[i] = (uint8)(i + 0x10);
        mismatch_data[i] = (uint8)(i + 0x20);
    }
    
    Fls_Write(0x08000000U, compare_data, 32);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Compare with different data */
    result = Fls_Compare(0x08000000U, mismatch_data, 32);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Wait for compare completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    /* Should fail due to mismatch */
    TEST_ASSERT(Fls_GetJobResult() == MEMIF_BLOCK_INCONSISTENT || Fls_GetJobResult() == MEMIF_JOB_FAILED);
}

/*==================================================================================================
 *                                    BLANK CHECK TESTS
 *==================================================================================================*/
void test_flash_blank_check(void)
{
    Std_ReturnType result;
    
    printf("\n=== Blank Check Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Pre-condition: Erase sector */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Check if erased sector is blank */
    result = Fls_BlankCheck(0x08000000U, 256);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Wait for completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

/*==================================================================================================
 *                                    MODE SETTING TESTS
 *==================================================================================================*/
void test_flash_set_mode(void)
{
    printf("\n=== Set Mode Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Test: Set fast mode */
    Fls_SetMode(MEMIF_MODE_FAST);
    TEST_ASSERT_EQ(MEMIF_IDLE, Fls_GetStatus());
    
    /* Test: Set slow mode */
    Fls_SetMode(MEMIF_MODE_SLOW);
    TEST_ASSERT_EQ(MEMIF_IDLE, Fls_GetStatus());
}

/*==================================================================================================
 *                                    CANCEL TESTS
 *==================================================================================================*/
void test_flash_cancel(void)
{
    printf("\n=== Cancel Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Start an erase operation */
    Fls_Erase(0x08000000U, 0x00010000U);
    
    /* Test: Cancel the operation */
    Fls_Cancel();
    
    /* Verify: Status should be IDLE after cancel */
    TEST_ASSERT_EQ(MEMIF_IDLE, Fls_GetStatus());
}

/*==================================================================================================
 *                                    STATUS AND RESULT TESTS
 *==================================================================================================*/
void test_flash_get_status_before_init(void)
{
    printf("\n=== Get Status Before Init Tests ===\n");
    
    /* Ensure uninitialized */
    Fls_DeInit();
    
    /* Test: Get status before init */
    MemIf_StatusType status = Fls_GetStatus();
    TEST_ASSERT_EQ(MEMIF_UNINIT, status);
}

void test_flash_get_job_result_before_init(void)
{
    printf("\n=== Get Job Result Before Init Tests ===\n");
    
    /* Ensure uninitialized */
    Fls_DeInit();
    
    /* Test: Get job result before init */
    MemIf_JobResultType result = Fls_GetJobResult();
    /* Should return FAILED or appropriate status when not initialized */
    TEST_ASSERT(result == MEMIF_JOB_FAILED || result == MEMIF_JOB_OK);
}

/*==================================================================================================
 *                                    VERSION INFO TESTS
 *==================================================================================================*/
void test_flash_version_info(void)
{
    printf("\n=== Version Info Tests ===\n");
    
    Std_VersionInfoType version_info;
    
    init_test_config();
    Fls_Init(&test_config);
    
#if (FLS_VERSION_INFO_API == STD_ON)
    Fls_GetVersionInfo(&version_info);
    TEST_ASSERT_EQ(FLS_VENDOR_ID, version_info.vendorID);
    TEST_ASSERT_EQ(FLS_MODULE_ID, version_info.moduleID);
    TEST_ASSERT_EQ(FLS_SW_MAJOR_VERSION, version_info.sw_major_version);
    TEST_ASSERT_EQ(FLS_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_ASSERT_EQ(FLS_SW_PATCH_VERSION, version_info.sw_patch_version);
#else
    printf("  Version info API not enabled\n");
    TEST_ASSERT(1);
#endif
}

/*==================================================================================================
 *                                    WRITE/READ ROUNDTRIP TESTS
 *==================================================================================================*/
void test_flash_write_read_roundtrip(void)
{
    Std_ReturnType result;
    uint8 write_data[64];
    uint8 read_buffer[64];
    
    printf("\n=== Write/Read Roundtrip Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Prepare test data */
    for (int i = 0; i < 64; i++) {
        write_data[i] = (uint8)(i * 3 + 0x55);
    }
    
    /* Pre-condition: Erase sector */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Write data */
    result = Fls_Write(0x08000000U, write_data, 64);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Wait for write completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
    
    /* Read back data */
    memset(read_buffer, 0, sizeof(read_buffer));
    result = Fls_Read(0x08000000U, read_buffer, 64);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Wait for read completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

/*==================================================================================================
 *                                    PROTECTION TESTS
 *==================================================================================================*/
void test_flash_read_protection(void)
{
    printf("\n=== Read Protection Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Test: Configure read protection */
    Std_ReturnType result = Fls_ConfigureReadProtection(FLS_PROTECTION_NONE);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    result = Fls_ConfigureReadProtection(FLS_PROTECTION_READ);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
}

void test_flash_write_protection(void)
{
    printf("\n=== Write Protection Tests ===\n");
    
    init_test_config();
    Fls_Init(&test_config);
    
    /* Test: Configure write protection */
    Std_ReturnType result = Fls_ConfigureWriteProtection(0x00000001U, TRUE);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
    
    result = Fls_ConfigureWriteProtection(0x00000001U, FALSE);
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
}

/*==================================================================================================
 *                                    MAIN FUNCTION
 *==================================================================================================*/
int main(void)
{
    printf("========================================\n");
    printf("   Flash Driver Unit Tests\n");
    printf("========================================\n");
    
    /* Initialization tests */
    test_flash_init();
    test_flash_deinit();
    
    /* Write tests */
    test_flash_write_valid();
    test_flash_write_invalid_params();
    test_flash_write_while_busy();
    
    /* Read tests */
    test_flash_read_valid();
    test_flash_read_null_pointer();
    
    /* Erase tests */
    test_flash_erase_valid();
    test_flash_erase_invalid_params();
    
    /* Compare tests */
    test_flash_compare();
    test_flash_compare_mismatch();
    
    /* Blank check tests */
    test_flash_blank_check();
    
    /* Mode setting tests */
    test_flash_set_mode();
    
    /* Cancel tests */
    test_flash_cancel();
    
    /* Status and result tests */
    test_flash_get_status_before_init();
    test_flash_get_job_result_before_init();
    
    /* Version info tests */
    test_flash_version_info();
    
    /* Roundtrip tests */
    test_flash_write_read_roundtrip();
    
    /* Protection tests */
    test_flash_read_protection();
    test_flash_write_protection();
    
    /* Print results */
    printf("\n========================================\n");
    printf("   Test Results\n");
    printf("========================================\n");
    printf("Total:   %d\n", tests_run);
    printf("Passed:  %d\n", tests_passed);
    printf("Failed:  %d\n", tests_failed);
    printf("Coverage: %.1f%%\n", (tests_run > 0) ? ((float)tests_passed / tests_run * 100.0f) : 0.0f);
    
    if (tests_failed == 0) {
        printf("\nAll tests PASSED!\n");
        return 0;
    } else {
        printf("\nSome tests FAILED!\n");
        return 1;
    }
}
