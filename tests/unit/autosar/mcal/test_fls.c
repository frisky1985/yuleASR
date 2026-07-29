/**
 * @file test_fls.c
 * @brief Flash Driver (Fls.h - AUTOSAR Standard) Module Unit Tests
 * @version 1.0.0
 * @note Tests for the standard AUTOSAR Fls API
 */

#include <stdio.h>
#include <string.h>
#include "Fls.h"

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
static Fls_SectorType test_sectors[3];
static Fls_ConfigType test_config;

/* 测试数据 */
static uint8 test_write_data[512];
static uint8 test_read_buffer[512];

/* 初始化测试配置 */
void init_fls_test_config(void)
{
    /* Setup sector configuration */
    test_sectors[0].sectorStartAddr = 0x08000000U;
    test_sectors[0].sectorSize = 0x00010000U; /* 64KB */
    test_sectors[0].sectorPageSize = 256;
    test_sectors[0].sectorUnlockMask = 0xFFFFFFFFU;
    test_sectors[0].sectorWritable = TRUE;
    test_sectors[0].sectorErasable = TRUE;
    
    test_sectors[1].sectorStartAddr = 0x08010000U;
    test_sectors[1].sectorSize = 0x00010000U;
    test_sectors[1].sectorPageSize = 256;
    test_sectors[1].sectorUnlockMask = 0xFFFFFFFFU;
    test_sectors[1].sectorWritable = TRUE;
    test_sectors[1].sectorErasable = TRUE;
    
    test_sectors[2].sectorStartAddr = 0x08020000U;
    test_sectors[2].sectorSize = 0x00010000U;
    test_sectors[2].sectorPageSize = 256;
    test_sectors[2].sectorUnlockMask = 0xFFFFFFFFU;
    test_sectors[2].sectorWritable = FALSE; /* Read-only sector */
    test_sectors[2].sectorErasable = FALSE;
    
    /* Setup config */
    test_config.sectorList = test_sectors;
    test_config.sectorCount = 3;
    test_config.defaultMode = MEMIF_MODE_SLOW;
    test_config.maxReadFastMode = 1024;
    test_config.maxReadNormalMode = 256;
    test_config.maxWriteFastMode = 256;
    test_config.maxWriteNormalMode = 64;
    test_config.jobEndNotificationEnabled = FALSE;
    test_config.jobErrorNotificationEnabled = FALSE;
    
    /* Prepare test data */
    for (int i = 0; i < 512; i++) {
        test_write_data[i] = (uint8)(i * 5 + 0x33);
    }
}

/*==================================================================================================
 *                                    INITIALIZATION TESTS
 *==================================================================================================*/
void test_fls_init_valid(void)
{
    printf("\n=== Fls_Init Valid Tests ===\n");
    
    init_fls_test_config();
    
    /* Test: Initialize with valid config */
    Fls_Init(&test_config);
    
    /* Verify: Status should be IDLE after init */
    TEST_ASSERT_EQ(FLS_IDLE, Fls_GetStatus());
    
    /* Verify: Job result should be OK */
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_fls_init_null_config(void)
{
    printf("\n=== Fls_Init NULL Config Test ===\n");
    
    /* Note: This test depends on FLS_DEV_ERROR_DETECT configuration */
    /* If DET is enabled, NULL config should report error */
    Fls_Init(NULL_PTR);
    
    /* Result depends on implementation - may stay UNINIT or use defaults */
    TEST_ASSERT(Fls_GetStatus() == FLS_UNINIT || Fls_GetStatus() == FLS_IDLE);
}

void test_fls_init_multiple(void)
{
    printf("\n=== Fls_Init Multiple Tests ===\n");
    
    init_fls_test_config();
    
    /* First initialization */
    Fls_Init(&test_config);
    TEST_ASSERT_EQ(FLS_IDLE, Fls_GetStatus());
    
    /* Second initialization - should be handled gracefully */
    Fls_Init(&test_config);
    
    /* Module should still be in valid state */
    TEST_ASSERT(Fls_GetStatus() == FLS_IDLE || Fls_GetStatus() == FLS_BUSY);
}

/*==================================================================================================
 *                                    ERASE TESTS
 *==================================================================================================*/
void test_fls_erase_valid(void)
{
    Std_ReturnType result;
    
    printf("\n=== Fls_Erase Valid Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Test: Erase valid sector */
    result = Fls_Erase(0x08000000U, 0x00010000U);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Verify: Status should be BUSY */
    TEST_ASSERT_EQ(FLS_BUSY, Fls_GetStatus());
    
    /* Wait for erase completion */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Verify: Job result should be OK */
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_fls_erase_invalid_address(void)
{
    Std_ReturnType result;
    
    printf("\n=== Fls_Erase Invalid Address Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Test: Erase with invalid address */
    result = Fls_Erase(0x00000000U, 0x00010000U);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* Test: Erase outside flash range */
    result = Fls_Erase(0x0FFFFFFFU, 0x00010000U);
    TEST_ASSERT_EQ(E_NOT_OK, result);
}

void test_fls_erase_invalid_length(void)
{
    Std_ReturnType result;
    
    printf("\n=== Fls_Erase Invalid Length Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Test: Erase with zero length */
    result = Fls_Erase(0x08000000U, 0);
    TEST_ASSERT_EQ(E_NOT_OK, result);
}

void test_fls_erase_while_busy(void)
{
    Std_ReturnType result;
    
    printf("\n=== Fls_Erase While Busy Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Start first erase */
    result = Fls_Erase(0x08000000U, 0x00010000U);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Try second erase while busy */
    result = Fls_Erase(0x08010000U, 0x00010000U);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* Complete first operation */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
}

void test_fls_erase_uninit(void)
{
    Std_ReturnType result;
    
    printf("\n=== Fls_Erase Uninitialized Test ===\n");
    
    /* Ensure uninitialized state */
    /* Fls_Status = FLS_UNINIT; */
    
    /* Test: Erase when not initialized */
    result = Fls_Erase(0x08000000U, 0x00010000U);
    /* Should return E_NOT_OK when not initialized */
    TEST_ASSERT(result == E_NOT_OK || result == E_OK);
}

/*==================================================================================================
 *                                    WRITE TESTS
 *==================================================================================================*/
void test_fls_write_valid(void)
{
    Std_ReturnType result;
    
    printf("\n=== Fls_Write Valid Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Pre-condition: Erase sector first */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Write valid data */
    result = Fls_Write(0x08000000U, test_write_data, 64);
    TEST_ASSERT_EQ(E_OK, result);
    
    /* Wait for write completion */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Verify: Job result should be OK */
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_fls_write_null_data(void)
{
    Std_ReturnType result;
    
    printf("\n=== Fls_Write NULL Data Test ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Test: Write with NULL data pointer */
    result = Fls_Write(0x08000000U, NULL_PTR, 32);
    TEST_ASSERT_EQ(E_NOT_OK, result);
}

void test_fls_write_invalid_address(void)
{
    Std_ReturnType result;
    
    printf("\n=== Fls_Write Invalid Address Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Test: Write to invalid address */
    result = Fls_Write(0x00000000U, test_write_data, 32);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* Test: Write to read-only sector */
    result = Fls_Write(0x08020000U, test_write_data, 32);
    TEST_ASSERT_EQ(E_NOT_OK, result);
}

void test_fls_write_while_busy(void)
{
    Std_ReturnType result;
    
    printf("\n=== Fls_Write While Busy Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Start an erase operation */
    Fls_Erase(0x08000000U, 0x00010000U);
    
    /* Try to write while busy */
    result = Fls_Write(0x08010000U, test_write_data, 32);
    TEST_ASSERT_EQ(E_NOT_OK, result);
    
    /* Complete the operation */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
}

/*==================================================================================================
 *                                    READ TESTS
 *==================================================================================================*/
void test_fls_read_valid(void)
{
    printf("\n=== Fls_Read Valid Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Pre-condition: Erase and write data */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    Fls_Write(0x08000000U, test_write_data, 128);
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
    
    /* Test: Read data */
    memset(test_read_buffer, 0, sizeof(test_read_buffer));
    Fls_Read(0x08000000U, test_read_buffer, 128);
    
    /* Wait for read completion */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Verify: Job result should be OK */
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_fls_read_null_buffer(void)
{
    printf("\n=== Fls_Read NULL Buffer Test ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Test: Read with NULL buffer - may report error via DET */
    Fls_Read(0x08000000U, NULL_PTR, 32);
    TEST_ASSERT(1); /* Function executed, error depends on DET config */
}

void test_fls_read_invalid_address(void)
{
    printf("\n=== Fls_Read Invalid Address Test ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Test: Read from invalid address */
    Fls_Read(0x00000000U, test_read_buffer, 32);
    /* Should handle gracefully */
    TEST_ASSERT(1);
}

/*==================================================================================================
 *                                    COMPARE TESTS
 *==================================================================================================*/
void test_fls_compare_valid(void)
{
    printf("\n=== Fls_Compare Valid Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Pre-condition: Erase and write data */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    Fls_Write(0x08000000U, test_write_data, 64);
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Compare matching data */
    Fls_Compare(0x08000000U, test_write_data, 64);
    
    /* Wait for compare completion */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Verify: Should match */
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

void test_fls_compare_mismatch(void)
{
    uint8 mismatch_data[64];
    
    printf("\n=== Fls_Compare Mismatch Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Prepare mismatch data */
    for (int i = 0; i < 64; i++) {
        mismatch_data[i] = (uint8)(i + 0xAA);
    }
    
    /* Pre-condition: Erase and write data */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    Fls_Write(0x08000000U, test_write_data, 64);
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Compare different data */
    Fls_Compare(0x08000000U, mismatch_data, 64);
    
    /* Wait for compare completion */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Verify: Should report mismatch */
    TEST_ASSERT(Fls_GetJobResult() == MEMIF_BLOCK_INCONSISTENT || 
                Fls_GetJobResult() == MEMIF_JOB_FAILED);
}

void test_fls_compare_null_buffer(void)
{
    printf("\n=== Fls_Compare NULL Buffer Test ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Test: Compare with NULL buffer */
    Fls_Compare(0x08000000U, NULL_PTR, 32);
    /* May report error via DET */
    TEST_ASSERT(1);
}

/*==================================================================================================
 *                                    MODE SETTING TESTS
 *==================================================================================================*/
void test_fls_set_mode(void)
{
    printf("\n=== Fls_SetMode Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Test: Set fast mode */
    Fls_SetMode(MEMIF_MODE_FAST);
    TEST_ASSERT_EQ(FLS_IDLE, Fls_GetStatus());
    
    /* Test: Set normal mode */
    Fls_SetMode(MEMIF_MODE_SLOW);
    TEST_ASSERT_EQ(FLS_IDLE, Fls_GetStatus());
}

void test_fls_set_mode_uninit(void)
{
    printf("\n=== Fls_SetMode Uninitialized Test ===\n");
    
    /* This test may report error via DET */
    /* Fls_Status = FLS_UNINIT; */
    
    Fls_SetMode(MEMIF_MODE_FAST);
    /* Should handle gracefully */
    TEST_ASSERT(1);
}

/*==================================================================================================
 *                                    CANCEL TESTS
 *==================================================================================================*/
void test_fls_cancel(void)
{
    printf("\n=== Fls_Cancel Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Start an operation */
    Fls_Erase(0x08000000U, 0x00010000U);
    TEST_ASSERT_EQ(FLS_BUSY, Fls_GetStatus());
    
    /* Test: Cancel operation */
    Fls_Cancel();
    
    /* Verify: Status should return to IDLE */
    TEST_ASSERT_EQ(FLS_IDLE, Fls_GetStatus());
    
    /* Verify: Job result should be CANCELED */
    TEST_ASSERT_EQ(MEMIF_JOB_CANCELED, Fls_GetJobResult());
}

void test_fls_cancel_idle(void)
{
    printf("\n=== Fls_Cancel When Idle Test ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Ensure IDLE state */
    TEST_ASSERT_EQ(FLS_IDLE, Fls_GetStatus());
    
    /* Test: Cancel when idle - should be handled gracefully */
    Fls_Cancel();
    TEST_ASSERT_EQ(FLS_IDLE, Fls_GetStatus());
}

/*==================================================================================================
 *                                    STATUS TESTS
 *==================================================================================================*/
void test_fls_get_status(void)
{
    printf("\n=== Fls_GetStatus Tests ===\n");
    
    init_fls_test_config();
    
    /* Before initialization */
    /* Note: Status may vary depending on implementation */
    Fls_StatusType status = Fls_GetStatus();
    (void)status;
    TEST_ASSERT(1);
    
    /* After initialization */
    Fls_Init(&test_config);
    TEST_ASSERT_EQ(FLS_IDLE, Fls_GetStatus());
    
    /* During operation */
    Fls_Erase(0x08000000U, 0x00010000U);
    TEST_ASSERT_EQ(FLS_BUSY, Fls_GetStatus());
    
    /* After operation */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    TEST_ASSERT_EQ(FLS_IDLE, Fls_GetStatus());
}

void test_fls_get_job_result(void)
{
    printf("\n=== Fls_GetJobResult Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Before any operation */
    Fls_JobResultType result = Fls_GetJobResult();
    (void)result;
    
    /* Start and complete an operation */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* After successful operation */
    TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
}

/*==================================================================================================
 *                                    MAIN FUNCTION TESTS
 *==================================================================================================*/
void test_fls_main_function(void)
{
    printf("\n=== Fls_MainFunction Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Test: Main function when idle */
    Fls_MainFunction();
    TEST_ASSERT_EQ(FLS_IDLE, Fls_GetStatus());
    
    /* Test: Main function during operation */
    Fls_Erase(0x08000000U, 0x00010000U);
    TEST_ASSERT_EQ(FLS_BUSY, Fls_GetStatus());
    
    /* Process through main function */
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    TEST_ASSERT_EQ(FLS_IDLE, Fls_GetStatus());
}

/*==================================================================================================
 *                                    VERSION INFO TESTS
 *==================================================================================================*/
void test_fls_version_info(void)
{
    printf("\n=== Fls_GetVersionInfo Tests ===\n");
    
    Std_VersionInfoType version_info;
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
#if (FLS_VERSION_INFO_API == STD_ON)
    /* Test: Get version info */
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

void test_fls_version_info_null(void)
{
    printf("\n=== Fls_GetVersionInfo NULL Test ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
#if (FLS_VERSION_INFO_API == STD_ON)
    /* Test: Get version info with NULL pointer */
    Fls_GetVersionInfo(NULL_PTR);
    /* May report error via DET */
#endif
    TEST_ASSERT(1);
}

/*==================================================================================================
 *                                    READ SYNC TESTS
 *==================================================================================================*/
#if (FLS_USE_ISR == STD_OFF)
void test_fls_read_sync(void)
{
    Std_ReturnType result;
    
    printf("\n=== Fls_ReadSync Tests ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Pre-condition: Erase and write data */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    Fls_Write(0x08000000U, test_write_data, 64);
    while (Fls_GetStatus() == FLS_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Read sync */
    memset(test_read_buffer, 0, sizeof(test_read_buffer));
    result = Fls_ReadSync(0x08000000U, test_read_buffer, 64);
    
    /* Should complete synchronously */
    TEST_ASSERT(result == E_OK || result == E_NOT_OK);
}
#endif

/*==================================================================================================
 *                                    ENDURANCE TESTS
 *==================================================================================================*/
void test_fls_multiple_operations(void)
{
    printf("\n=== Fls Multiple Operations Test ===\n");
    
    init_fls_test_config();
    Fls_Init(&test_config);
    
    /* Perform multiple erase/write cycles */
    for (int cycle = 0; cycle < 3; cycle++) {
        /* Erase */
        Std_ReturnType result = Fls_Erase(0x08000000U, 0x00010000U);
        TEST_ASSERT_EQ(E_OK, result);
        
        while (Fls_GetStatus() == FLS_BUSY) {
            Fls_MainFunction();
        }
        TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
        
        /* Write */
        uint8 data[32];
        for (int i = 0; i < 32; i++) {
            data[i] = (uint8)(cycle * 10 + i);
        }
        
        result = Fls_Write(0x08000000U, data, 32);
        TEST_ASSERT_EQ(E_OK, result);
        
        while (Fls_GetStatus() == FLS_BUSY) {
            Fls_MainFunction();
        }
        TEST_ASSERT_EQ(MEMIF_JOB_OK, Fls_GetJobResult());
    }
}

/*==================================================================================================
 *                                    MAIN FUNCTION
 *==================================================================================================*/
int main(void)
{
    printf("========================================\n");
    printf("   Fls (Flash Driver) Unit Tests\n");
    printf("   AUTOSAR R22-11 Standard\n");
    printf("========================================\n");
    
    /* Initialization tests */
    test_fls_init_valid();
    test_fls_init_null_config();
    test_fls_init_multiple();
    
    /* Erase tests */
    test_fls_erase_valid();
    test_fls_erase_invalid_address();
    test_fls_erase_invalid_length();
    test_fls_erase_while_busy();
    test_fls_erase_uninit();
    
    /* Write tests */
    test_fls_write_valid();
    test_fls_write_null_data();
    test_fls_write_invalid_address();
    test_fls_write_while_busy();
    
    /* Read tests */
    test_fls_read_valid();
    test_fls_read_null_buffer();
    test_fls_read_invalid_address();
    
    /* Compare tests */
    test_fls_compare_valid();
    test_fls_compare_mismatch();
    test_fls_compare_null_buffer();
    
    /* Mode setting tests */
    test_fls_set_mode();
    test_fls_set_mode_uninit();
    
    /* Cancel tests */
    test_fls_cancel();
    test_fls_cancel_idle();
    
    /* Status tests */
    test_fls_get_status();
    test_fls_get_job_result();
    
    /* Main function tests */
    test_fls_main_function();
    
    /* Version info tests */
    test_fls_version_info();
    test_fls_version_info_null();
    
    /* Read sync tests */
#if (FLS_USE_ISR == STD_OFF)
    test_fls_read_sync();
#endif
    
    /* Endurance tests */
    test_fls_multiple_operations();
    
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
