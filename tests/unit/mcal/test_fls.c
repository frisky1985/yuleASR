/**
 * @file test_fls.c
 * @brief Fls (Flash Driver) Unit Tests
 * @version 1.0.0
 * @date 2026
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "mcal/fls/fls.h"

/* Test configuration */
#define TEST_FLS_FLASH_SIZE     (256u * 1024u)
#define TEST_FLS_SECTOR_SIZE    65536u
#define TEST_FLS_PAGE_SIZE      256u

/* Test result counters */
static int gTestsPassed = 0;
static int gTestsFailed = 0;

/*============================================================================*
 * Test Helper Functions
 *============================================================================*/

static void test_assert_true(bool condition, const char* testName)
{
    if (condition) {
        printf("  [PASS] %s\n", testName);
        gTestsPassed++;
    } else {
        printf("  [FAIL] %s\n", testName);
        gTestsFailed++;
    }
}

static void test_print_summary(void)
{
    printf("\n========================================\n");
    printf("Test Summary:\n");
    printf("  Passed: %d\n", gTestsPassed);
    printf("  Failed: %d\n", gTestsFailed);
    printf("  Total:  %d\n", gTestsPassed + gTestsFailed);
    printf("========================================\n\n");
}

/*============================================================================*
 * Test Cases - Initialization
 *============================================================================*/

static void test_fls_init_deinit(void)
{
    Fls_ErrorCode_t result;

    printf("\n--- Test: Fls_Init / Fls_Deinit ---\n");

    /* Test uninitialized state */
    test_assert_true(!Fls_IsInitialized(), "Not initialized before Init");
    test_assert_true(Fls_GetStatus() == FLS_UNINIT, "Status is UNINIT");

    /* Test initialization */
    result = Fls_Init(NULL);
    test_assert_true(result == FLS_OK, "Init with NULL config returns OK");
    test_assert_true(Fls_IsInitialized(), "Initialized after Init");
    test_assert_true(Fls_GetStatus() == FLS_IDLE, "Status is IDLE after Init");

    /* Test double initialization */
    result = Fls_Init(NULL);
    test_assert_true(result == FLS_E_ALREADY_INITIALIZED,
                     "Double init returns ALREADY_INITIALIZED");

    /* Test deinitialization */
    result = Fls_Deinit();
    test_assert_true(result == FLS_OK, "Deinit returns OK");
    test_assert_true(!Fls_IsInitialized(), "Not initialized after Deinit");

    /* Test deinit without init */
    result = Fls_Deinit();
    test_assert_true(result == FLS_E_UNINIT, "Deinit without init returns UNINIT");
}

/*============================================================================*
 * Test Cases - Read/Write Operations
 *============================================================================*/

static void test_fls_basic_read_write(void)
{
    Fls_ErrorCode_t result;
    uint8_t writeData[TEST_FLS_PAGE_SIZE];
    uint8_t readData[TEST_FLS_PAGE_SIZE];
    uint32_t i;

    printf("\n--- Test: Fls_Read / Fls_Write ---\n");

    /* Initialize */
    result = Fls_Init(NULL);
    if (result != FLS_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Prepare test data */
    for (i = 0; i < TEST_FLS_PAGE_SIZE; i++) {
        writeData[i] = (uint8_t)(i & 0xFF);
    }

    /* Erase sector first */
    result = Fls_Erase(0, TEST_FLS_SECTOR_SIZE);
    test_assert_true(result == FLS_OK, "Erase sector 0");

    /* Write data */
    result = Fls_Write(0, TEST_FLS_PAGE_SIZE, writeData);
    test_assert_true(result == FLS_OK, "Write page at address 0");

    /* Read data back */
    (void)memset(readData, 0, TEST_FLS_PAGE_SIZE);
    result = Fls_Read(0, TEST_FLS_PAGE_SIZE, readData);
    test_assert_true(result == FLS_OK, "Read page from address 0");

    /* Verify data */
    test_assert_true(memcmp(writeData, readData, TEST_FLS_PAGE_SIZE) == 0,
                     "Read data matches written data");

    /* Test unaligned write (should fail or handle gracefully) */
    /* Implementation dependent - document behavior */

    /* Cleanup */
    (void)Fls_Deinit();
}

/*============================================================================*
 * Test Cases - Erase Operations
 *============================================================================*/

static void test_fls_erase_operations(void)
{
    Fls_ErrorCode_t result;
    uint8_t readData[16];
    uint32_t i;

    printf("\n--- Test: Fls_Erase ---\n");

    /* Initialize */
    result = Fls_Init(NULL);
    if (result != FLS_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Write some data */
    uint8_t writeData[16] = {0xAA, 0xBB, 0xCC, 0xDD};
    result = Fls_Write(0, 16, writeData);
    test_assert_true(result == FLS_OK, "Write test data");

    /* Erase the sector */
    result = Fls_Erase(0, TEST_FLS_SECTOR_SIZE);
    test_assert_true(result == FLS_OK, "Erase sector containing data");

    /* Verify erased (all 0xFF) */
    result = Fls_Read(0, 16, readData);
    test_assert_true(result == FLS_OK, "Read after erase");

    bool allErased = true;
    for (i = 0; i < 16; i++) {
        if (readData[i] != 0xFF) {
            allErased = false;
            break;
        }
    }
    test_assert_true(allErased, "All bytes are 0xFF after erase");

    /* Test blank check */
    result = Fls_BlankCheck(0, 16);
    test_assert_true(result == FLS_OK, "Blank check passes after erase");

    /* Write and verify blank check fails */
    result = Fls_Write(0, 4, writeData);
    if (result == FLS_OK) {
        result = Fls_BlankCheck(0, 4);
        test_assert_true(result == FLS_E_COMPARE_FAILED,
                         "Blank check fails after write");
    }

    /* Cleanup */
    (void)Fls_Deinit();
}

/*============================================================================*
 * Test Cases - Compare Operations
 *============================================================================*/

static void test_fls_compare_operations(void)
{
    Fls_ErrorCode_t result;
    uint8_t writeData[16] = {0x11, 0x22, 0x33, 0x44};
    uint8_t matchData[16] = {0x11, 0x22, 0x33, 0x44};
    uint8_t diffData[16] = {0x11, 0x22, 0x33, 0x55};

    printf("\n--- Test: Fls_Compare ---\n");

    /* Initialize */
    result = Fls_Init(NULL);
    if (result != FLS_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Erase and write */
    (void)Fls_Erase(0, TEST_FLS_SECTOR_SIZE);
    result = Fls_Write(0, 16, writeData);
    test_assert_true(result == FLS_OK, "Write test data for compare");

    /* Compare matching data */
    result = Fls_Compare(0, 16, matchData);
    test_assert_true(result == FLS_OK, "Compare with matching data");

    /* Compare different data */
    result = Fls_Compare(0, 16, diffData);
    test_assert_true(result == FLS_E_COMPARE_FAILED,
                     "Compare with different data fails");

    /* Cleanup */
    (void)Fls_Deinit();
}

/*============================================================================*
 * Test Cases - Protection
 *============================================================================*/

static void test_fls_protection(void)
{
    Fls_ErrorCode_t result;
    uint8_t writeData[16] = {0x01, 0x02, 0x03, 0x04};

    printf("\n--- Test: Fls Protection ---\n");

    /* Initialize */
    result = Fls_Init(NULL);
    if (result != FLS_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Erase first */
    (void)Fls_Erase(0, TEST_FLS_SECTOR_SIZE);

    /* Set write protection */
    result = Fls_SetProtection(0, FLS_PROTECTION_WRITE);
    test_assert_true(result == FLS_OK, "Set write protection");

    /* Try to write to protected sector */
    result = Fls_Write(0, 16, writeData);
    test_assert_true(result == FLS_E_WRITE_PROTECTED,
                     "Write to protected sector fails");

    /* Remove protection */
    result = Fls_SetProtection(0, FLS_PROTECTION_NONE);
    test_assert_true(result == FLS_OK, "Remove write protection");

    /* Write should now succeed */
    result = Fls_Write(0, 16, writeData);
    test_assert_true(result == FLS_OK, "Write after removing protection");

    /* Cleanup */
    (void)Fls_Deinit();
}

/*============================================================================*
 * Test Cases - Sector Information
 *============================================================================*/

static void test_fls_sector_info(void)
{
    Fls_ErrorCode_t result;
    uint32_t sectorCount;
    uint32_t sectorSize;
    uint32_t pageSize;

    printf("\n--- Test: Fls Sector Info ---\n");

    /* Initialize */
    result = Fls_Init(NULL);
    if (result != FLS_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Get sector count */
    sectorCount = Fls_GetNumSectors();
    test_assert_true(sectorCount > 0, "GetNumSectors returns positive value");

    /* Get sector size */
    sectorSize = Fls_GetSectorSize(0);
    test_assert_true(sectorSize > 0, "GetSectorSize returns positive value");

    /* Get page size */
    pageSize = Fls_GetPageSize(0);
    test_assert_true(pageSize > 0, "GetPageSize returns positive value");

    /* Get sector from address */
    uint32_t sector = Fls_GetSectorNumber(0);
    test_assert_true(sector == 0, "Address 0 is in sector 0");

    /* Cleanup */
    (void)Fls_Deinit();
}

/*============================================================================*
 * Test Cases - Asynchronous Operations
 *============================================================================*/

static void test_fls_async_operations(void)
{
    Fls_ErrorCode_t result;
    uint8_t writeData[TEST_FLS_PAGE_SIZE];
    uint8_t readData[TEST_FLS_PAGE_SIZE];
    uint32_t i;

    printf("\n--- Test: Fls Asynchronous Operations ---\n");

    /* Initialize */
    result = Fls_Init(NULL);
    if (result != FLS_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Prepare test data */
    for (i = 0; i < TEST_FLS_PAGE_SIZE; i++) {
        writeData[i] = (uint8_t)(0xA5 ^ i);
    }

    /* Erase first */
    (void)Fls_Erase(0, TEST_FLS_SECTOR_SIZE);

    /* Test async write */
    result = Fls_Write_Async(0, TEST_FLS_PAGE_SIZE, writeData);
    test_assert_true(result == FLS_OK, "Async write starts successfully");

    /* Process job */
    int timeout = 1000;
    while ((Fls_GetJobStatus() == FLS_JOB_PENDING ||
            Fls_GetJobStatus() == FLS_JOB_IN_PROGRESS) && timeout > 0) {
        Fls_MainFunction();
        timeout--;
    }

    test_assert_true(Fls_GetJobStatus() == FLS_JOB_COMPLETED,
                     "Async write completes");

    /* Test async read */
    (void)memset(readData, 0, TEST_FLS_PAGE_SIZE);
    result = Fls_Read_Async(0, TEST_FLS_PAGE_SIZE, readData);
    test_assert_true(result == FLS_OK, "Async read starts successfully");

    /* Process job */
    timeout = 1000;
    while ((Fls_GetJobStatus() == FLS_JOB_PENDING ||
            Fls_GetJobStatus() == FLS_JOB_IN_PROGRESS) && timeout > 0) {
        Fls_MainFunction();
        timeout--;
    }

    test_assert_true(Fls_GetJobStatus() == FLS_JOB_COMPLETED,
                     "Async read completes");
    test_assert_true(memcmp(writeData, readData, TEST_FLS_PAGE_SIZE) == 0,
                     "Async read data correct");

    /* Cleanup */
    (void)Fls_Deinit();
}

/*============================================================================*
 * Test Cases - Wear Leveling
 *============================================================================*/

static void test_fls_wear_leveling(void)
{
    printf("\n--- Test: Fls Wear Leveling ---\n");

    Fls_ErrorCode_t result = Fls_Init(NULL);
    if (result != FLS_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Erase multiple sectors and check counts */
    (void)Fls_Erase(0, TEST_FLS_SECTOR_SIZE);
    (void)Fls_Erase(TEST_FLS_SECTOR_SIZE, TEST_FLS_SECTOR_SIZE);

    uint32_t count0 = Fls_GetEraseCount(0);
    uint32_t count1 = Fls_GetEraseCount(1);

    test_assert_true(count0 > 0, "Erase count for sector 0 incremented");
    test_assert_true(count1 > 0, "Erase count for sector 1 incremented");

    /* Test least/most worn sector functions */
    uint32_t least = Fls_GetLeastWornSector();
    uint32_t most = Fls_GetMostWornSector();

    test_assert_true(least != 0xFFFFFFFF, "GetLeastWornSector returns valid");
    test_assert_true(most != 0xFFFFFFFF, "GetMostWornSector returns valid");

    (void)Fls_Deinit();
}

/*============================================================================*
 * Main Test Runner
 *============================================================================*/

int main(void)
{
    printf("\n========================================\n");
    printf("Fls (Flash Driver) Unit Tests\n");
    printf("========================================\n");

    /* Run all test suites */
    test_fls_init_deinit();
    test_fls_basic_read_write();
    test_fls_erase_operations();
    test_fls_compare_operations();
    test_fls_protection();
    test_fls_sector_info();
    test_fls_async_operations();
    test_fls_wear_leveling();

    /* Print summary */
    test_print_summary();

    return gTestsFailed > 0 ? 1 : 0;
}
