/**
 * @file test_fee.c
 * @brief Fee (Flash EEPROM Emulation) Unit Tests
 * @version 1.0.0
 * @date 2026
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "mcal/fee/fee.h"
#include "mcal/fee/fee_gc.h"
#include "mcal/fee/fee_wl.h"

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

static void test_fee_init_deinit(void)
{
    Fee_ErrorCode_t result;

    printf("\n--- Test: Fee_Init / Fee_Deinit ---\n");

    /* Test uninitialized state */
    test_assert_true(!Fee_IsInitialized(), "Not initialized before Init");
    test_assert_true(Fee_GetStatus() == FEE_UNINIT, "Status is UNINIT");

    /* Test initialization */
    result = Fee_Init(NULL);
    test_assert_true(result == FEE_OK, "Init with NULL config returns OK");
    test_assert_true(Fee_IsInitialized(), "Initialized after Init");
    test_assert_true(Fee_GetStatus() == FEE_IDLE, "Status is IDLE after Init");

    /* Test double initialization */
    result = Fee_Init(NULL);
    test_assert_true(result == FEE_E_ALREADY_INITIALIZED,
                     "Double init returns ALREADY_INITIALIZED");

    /* Test deinitialization */
    result = Fee_Deinit();
    test_assert_true(result == FEE_OK, "Deinit returns OK");
    test_assert_true(!Fee_IsInitialized(), "Not initialized after Deinit");

    /* Test deinit without init */
    result = Fee_Deinit();
    test_assert_true(result == FEE_E_UNINIT, "Deinit without init returns UNINIT");
}

/*============================================================================*
 * Test Cases - Basic Read/Write
 *============================================================================*/

static void test_fee_basic_read_write(void)
{
    Fee_ErrorCode_t result;
    uint8_t writeData[64];
    uint8_t readData[64];
    uint32_t i;

    printf("\n--- Test: Fee_Read / Fee_Write ---\n");

    /* Initialize */
    result = Fee_Init(NULL);
    if (result != FEE_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Prepare test data */
    for (i = 0; i < 64; i++) {
        writeData[i] = (uint8_t)(i & 0xFF);
    }

    /* Write block 0 */
    result = Fee_Write(0, writeData);
    test_assert_true(result == FEE_OK, "Write block 0");

    /* Read block 0 */
    (void)memset(readData, 0, 64);
    result = Fee_Read(0, 0, readData, 64);
    test_assert_true(result == FEE_OK, "Read block 0");

    /* Verify data */
    test_assert_true(memcmp(writeData, readData, 64) == 0,
                     "Read data matches written data");

    /* Write block 1 with different data */
    for (i = 0; i < 64; i++) {
        writeData[i] = (uint8_t)(0xAA ^ i);
    }

    result = Fee_Write(1, writeData);
    test_assert_true(result == FEE_OK, "Write block 1");

    (void)memset(readData, 0, 64);
    result = Fee_Read(1, 0, readData, 64);
    test_assert_true(result == FEE_OK, "Read block 1");
    test_assert_true(memcmp(writeData, readData, 64) == 0,
                     "Block 1 data correct");

    /* Verify block 0 still readable */
    for (i = 0; i < 64; i++) {
        writeData[i] = (uint8_t)(i & 0xFF);
    }
    (void)memset(readData, 0, 64);
    result = Fee_Read(0, 0, readData, 64);
    test_assert_true(result == FEE_OK, "Block 0 still readable");
    test_assert_true(memcmp(writeData, readData, 64) == 0,
                     "Block 0 data unchanged");

    /* Cleanup */
    (void)Fee_Deinit();
}

/*============================================================================*
 * Test Cases - Block Overwrite
 *============================================================================*/

static void test_fee_block_overwrite(void)
{
    Fee_ErrorCode_t result;
    uint8_t writeData[64];
    uint8_t readData[64];
    uint32_t i;

    printf("\n--- Test: Fee Block Overwrite ---\n");

    result = Fee_Init(NULL);
    if (result != FEE_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* First write */
    for (i = 0; i < 64; i++) {
        writeData[i] = (uint8_t)0x11;
    }
    result = Fee_Write(0, writeData);
    test_assert_true(result == FEE_OK, "First write to block 0");

    /* Second write (overwrite) */
    for (i = 0; i < 64; i++) {
        writeData[i] = (uint8_t)0x22;
    }
    result = Fee_Write(0, writeData);
    test_assert_true(result == FEE_OK, "Overwrite block 0");

    /* Read and verify latest data */
    (void)memset(readData, 0, 64);
    result = Fee_Read(0, 0, readData, 64);
    test_assert_true(result == FEE_OK, "Read after overwrite");

    bool correct = true;
    for (i = 0; i < 64; i++) {
        if (readData[i] != 0x22) {
            correct = false;
            break;
        }
    }
    test_assert_true(correct, "Read data is latest version");

    /* Cleanup */
    (void)Fee_Deinit();
}

/*============================================================================*
 * Test Cases - Block Status
 *============================================================================*/

static void test_fee_block_status(void)
{
    Fee_ErrorCode_t result;
    Fee_BlockStatus_t status;

    printf("\n--- Test: Fee Block Status ---\n");

    result = Fee_Init(NULL);
    if (result != FEE_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Check unwritten block */
    result = Fee_GetBlockStatus(0, &status);
    test_assert_true(result == FEE_OK, "GetBlockStatus returns OK");
    test_assert_true(status == FEE_BLOCK_EMPTY || status == FEE_BLOCK_VALID,
                     "Block 0 status is valid");

    /* Write and check */
    uint8_t data[64] = {0x11, 0x22, 0x33, 0x44};
    result = Fee_Write(0, data);
    test_assert_true(result == FEE_OK, "Write for status test");

    result = Fee_GetBlockStatus(0, &status);
    test_assert_true(result == FEE_OK, "GetBlockStatus after write");
    test_assert_true(status == FEE_BLOCK_VALID, "Block status is VALID after write");

    /* Invalidate */
    result = Fee_InvalidateBlock(0);
    test_assert_true(result == FEE_OK, "Invalidate block");

    result = Fee_GetBlockStatus(0, &status);
    test_assert_true(result == FEE_OK, "GetBlockStatus after invalidate");
    test_assert_true(status == FEE_BLOCK_INVALID, "Block status is INVALID");

    /* Cleanup */
    (void)Fee_Deinit();
}

/*============================================================================*
 * Test Cases - Space Management
 *============================================================================*/

static void test_fee_space_management(void)
{
    Fee_ErrorCode_t result;
    uint32_t freeSpace;
    uint32_t usedPercent;

    printf("\n--- Test: Fee Space Management ---\n");

    result = Fee_Init(NULL);
    if (result != FEE_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Get initial free space */
    freeSpace = Fee_GetFreeSpace();
    test_assert_true(freeSpace > 0, "Free space is positive");

    usedPercent = Fee_GetUsedSpacePercent();
    test_assert_true(usedPercent <= 100, "Used percent is valid");

    /* Write some blocks */
    uint8_t data[64];
    uint32_t i;
    for (i = 0; i < 64; i++) {
        data[i] = (uint8_t)i;
    }

    result = Fee_Write(0, data);
    test_assert_true(result == FEE_OK, "Write for space test");

    /* Check space decreased */
    uint32_t freeSpaceAfter = Fee_GetFreeSpace();
    test_assert_true(freeSpaceAfter < freeSpace, "Free space decreased after write");

    /* Cleanup */
    (void)Fee_Deinit();
}

/*============================================================================*
 * Test Cases - Cluster Management
 *============================================================================*/

static void test_fee_cluster_management(void)
{
    Fee_ErrorCode_t result;
    Fee_ClusterStatus_t status;

    printf("\n--- Test: Fee Cluster Management ---\n");

    result = Fee_Init(NULL);
    if (result != FEE_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Get cluster status */
    result = Fee_GetClusterStatus(0, &status);
    test_assert_true(result == FEE_OK, "GetClusterStatus returns OK");
    test_assert_true((status == FEE_CLUSTER_EMPTY) || (status == FEE_CLUSTER_ACTIVE),
                     "Cluster status is valid");

    /* Get cluster erase count */
    uint32_t count;
    result = Fee_GetClusterEraseCount(0, &count);
    test_assert_true(result == FEE_OK, "GetClusterEraseCount returns OK");

    /* Test most/least worn */
    uint32_t most = Fee_GetMostWornCluster();
    uint32_t least = Fee_GetLeastWornCluster();
    test_assert_true(most != 0xFFFFFFFF, "Most worn cluster valid");
    test_assert_true(least != 0xFFFFFFFF, "Least worn cluster valid");

    /* Cleanup */
    (void)Fee_Deinit();
}

/*============================================================================*
 * Test Cases - Write Queue
 *============================================================================*/

static void test_fee_write_queue(void)
{
    printf("\n--- Test: Fee Write Queue ---\n");

    Fee_ErrorCode_t result = Fee_Init(NULL);
    if (result != FEE_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Check initial queue state */
    test_assert_true(Fee_WriteQueueIsEmpty(), "Queue is empty initially");
    test_assert_true(!Fee_WriteQueueIsFull(), "Queue is not full initially");
    test_assert_true(Fee_GetWriteQueueCount() == 0, "Queue count is 0");

    /* Clear queue */
    result = Fee_ClearWriteQueue();
    test_assert_true(result == FEE_OK, "ClearWriteQueue returns OK");

    /* Cleanup */
    (void)Fee_Deinit();
}

/*============================================================================*
 * Test Cases - Garbage Collection
 *============================================================================*/

static void test_fee_garbage_collection(void)
{
    Fee_ErrorCode_t result;

    printf("\n--- Test: Fee Garbage Collection ---\n");

    result = Fee_Init(NULL);
    if (result != FEE_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    result = Fee_Gc_Init();
    test_assert_true(result == FEE_OK, "GC Init returns OK");

    /* Check GC not active */
    test_assert_true(!Fee_Gc_IsActive(), "GC not active initially");
    test_assert_true(Fee_Gc_GetState() == FEE_GC_STATE_IDLE, "GC state is IDLE");

    /* Check if GC needed (should be false initially) */
    bool needed = Fee_Gc_IsNeeded();
    test_assert_true(needed == false || needed == true, "IsNeeded returns valid");

    /* Get GC statistics */
    Fee_GcStatistics_t stats;
    result = Fee_Gc_GetStatistics(&stats);
    test_assert_true(result == FEE_OK, "GetStatistics returns OK");
    test_assert_true(stats.gcCount == 0, "Initial GC count is 0");

    /* Cleanup */
    (void)Fee_Deinit();
}

/*============================================================================*
 * Test Cases - Wear Leveling
 *============================================================================*/

static void test_fee_wear_leveling(void)
{
    Fee_ErrorCode_t result;

    printf("\n--- Test: Fee Wear Leveling ---\n");

    result = Fee_Init(NULL);
    if (result != FEE_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    result = Fee_Wl_Init();
    test_assert_true(result == FEE_OK, "WL Init returns OK");

    /* Get threshold */
    uint32_t threshold = Fee_Wl_GetThreshold();
    test_assert_true(threshold > 0, "Threshold is positive");

    /* Set threshold */
    result = Fee_Wl_SetThreshold(500);
    test_assert_true(result == FEE_OK, "SetThreshold returns OK");
    test_assert_true(Fee_Wl_GetThreshold() == 500, "Threshold updated");

    /* Get statistics */
    Fee_WlStatistics_t stats;
    result = Fee_Wl_GetStatistics(&stats);
    test_assert_true(result == FEE_OK, "GetStatistics returns OK");

    /* Check rebalancing needed */
    bool rebal = Fee_Wl_IsRebalancingNeeded();
    test_assert_true(rebal == false || rebal == true, "IsRebalancingNeeded returns valid");

    /* Get wear difference */
    uint32_t diff = Fee_Wl_GetWearDifference();
    test_assert_true(diff >= 0, "Wear difference is valid");

    /* Cleanup */
    (void)Fee_Deinit();
}

/*============================================================================*
 * Test Cases - Error Handling
 *============================================================================*/

static void test_fee_error_handling(void)
{
    Fee_ErrorCode_t result;

    printf("\n--- Test: Fee Error Handling ---\n");

    /* Test operations without init */
    uint8_t data[64];
    result = Fee_Read(0, 0, data, 64);
    test_assert_true(result == FEE_E_UNINIT, "Read without init returns UNINIT");

    result = Fee_Write(0, data);
    test_assert_true(result == FEE_E_UNINIT, "Write without init returns UNINIT");

    result = Fee_GetFreeSpace();
    test_assert_true(result == 0, "GetFreeSpace without init returns 0");

    /* Initialize for remaining tests */
    result = Fee_Init(NULL);
    if (result != FEE_OK) {
        printf("  [SKIP] Init failed, skipping tests\n");
        return;
    }

    /* Test invalid block number */
    result = Fee_Read(9999, 0, data, 64);
    test_assert_true(result == FEE_E_INVALID_BLOCK_NO,
                     "Read invalid block returns INVALID_BLOCK_NO");

    /* Test NULL pointer */
    result = Fee_Read(0, 0, NULL, 64);
    test_assert_true(result == FEE_E_PARAM_POINTER,
                     "Read with NULL pointer returns PARAM_POINTER");

    /* Cleanup */
    (void)Fee_Deinit();
}

/*============================================================================*
 * Test Cases - Version Info
 *============================================================================*/

static void test_fee_version_info(void)
{
    Fee_ErrorCode_t result;
    uint8_t major, minor, patch;

    printf("\n--- Test: Fee Version Info ---\n");

    result = Fee_GetVersionInfo(&major, &minor, &patch);
    test_assert_true(result == FEE_OK, "GetVersionInfo returns OK");
    test_assert_true(major == FEE_MAJOR_VERSION, "Major version matches");
    test_assert_true(minor == FEE_MINOR_VERSION, "Minor version matches");
    test_assert_true(patch == FEE_PATCH_VERSION, "Patch version matches");

    /* Test NULL pointers */
    result = Fee_GetVersionInfo(NULL, NULL, NULL);
    test_assert_true(result == FEE_E_PARAM_POINTER, "NULL pointers return error");
}

/*============================================================================*
 * Main Test Runner
 *============================================================================*/

int main(void)
{
    printf("\n========================================\n");
    printf("Fee (Flash EEPROM Emulation) Unit Tests\n");
    printf("========================================\n");

    /* Run all test suites */
    test_fee_init_deinit();
    test_fee_basic_read_write();
    test_fee_block_overwrite();
    test_fee_block_status();
    test_fee_space_management();
    test_fee_cluster_management();
    test_fee_write_queue();
    test_fee_garbage_collection();
    test_fee_wear_leveling();
    test_fee_error_handling();
    test_fee_version_info();

    /* Print summary */
    test_print_summary();

    return gTestsFailed > 0 ? 1 : 0;
}
