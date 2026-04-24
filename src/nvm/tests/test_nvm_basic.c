/**
 * @file test_nvm_basic.c
 * @brief Basic unit tests for NVM module
 * @version 1.0.0
 * @date 2025
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nvm.h"

/* Test storage area (simulated) */
static uint8_t g_testStorage[0x10000];

/* Test data */
static uint8_t g_testData[256];
static uint8_t g_readBuffer[256];

/* Test counters */
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(condition, msg) \
    do { \
        if (condition) { \
            printf("  [PASS] %s\n", msg); \
            g_testsPassed++; \
        } else { \
            printf("  [FAIL] %s\n", msg); \
            g_testsFailed++; \
        } \
    } while(0)

/**
 * @brief Test NVM initialization
 */
void Test_Init(void)
{
    Nvm_ErrorCode_t result;
    
    printf("\n=== Test: Initialization ===\n");
    
    /* Test deinit when not initialized */
    result = Nvm_Deinit();
    TEST_ASSERT(result == NVM_E_NOT_INITIALIZED, "Deinit when not initialized");
    
    /* Test init */
    result = Nvm_Init();
    TEST_ASSERT(result == NVM_OK, "Nvm_Init");
    
    /* Test double init */
    result = Nvm_Init();
    TEST_ASSERT(result == NVM_E_ALREADY_INITIALIZED, "Double init");
    
    /* Test is initialized */
    TEST_ASSERT(Nvm_IsInitialized() == true, "IsInitialized");
    
    /* Cleanup */
    Nvm_Deinit();
}

/**
 * @brief Test block configuration
 */
void Test_BlockConfiguration(void)
{
    Nvm_ErrorCode_t result;
    Nvm_Block_t* block;
    
    printf("\n=== Test: Block Configuration ===\n");
    
    Nvm_Init();
    Nvm_RegisterStorage(0, NVM_DEV_FLASH_INTERNAL, 0x10000, 0x10000);
    
    Nvm_RegisterStorage(0, NVM_DEV_FLASH_INTERNAL, 0x00000, 0x10000);
    Nvm_ConfigureBlock(0, "TEST_BLOCK", NVM_BLOCK_TYPE_DATA, 
                                0x00100, 256, false);
    TEST_ASSERT(result == NVM_OK, "Configure block");
    
    /* Get block */
    block = Nvm_BlockMgr_GetBlock(0);
    TEST_ASSERT(block != NULL, "Get configured block");
    TEST_ASSERT(block->configured == true, "Block is configured");
    TEST_ASSERT(strcmp(block->name, "TEST_BLOCK") == 0, "Block name correct");
    TEST_ASSERT(block->dataLength == 256, "Block size correct");
    
    /* Get non-existent block */
    block = Nvm_BlockMgr_GetBlock(99);
    TEST_ASSERT(block == NULL, "Get non-existent block returns NULL");
    
    /* Get configured count */
    uint32_t count = Nvm_BlockMgr_GetConfiguredCount();
    TEST_ASSERT(count == 1, "Configured count is 1");
    
    /* Cleanup */
    Nvm_Deinit();
}

/**
 * @brief Test read/write operations
 */
void Test_ReadWrite(void)
{
    Nvm_ErrorCode_t result;
    int i;
    
    printf("\n=== Test: Read/Write Operations ===\n");
    
    /* Initialize */
    Nvm_Init();
    Nvm_RegisterStorage(0, NVM_DEV_FLASH_INTERNAL, 0x10000, 0x10000);
    Nvm_ConfigureBlock(0, "RW_TEST", NVM_BLOCK_TYPE_DATA, 0x10000, 256, false);
    
    /* Prepare test data */
    for (i = 0; i < 256; i++) {
        g_testData[i] = (uint8_t)i;
    }
    
    /* Write block */
    result = Nvm_WriteBlock(0, g_testData, 256);
    TEST_ASSERT(result == NVM_OK, "Write block");
    
    /* Read block */
    memset(g_readBuffer, 0, sizeof(g_readBuffer));
    result = Nvm_ReadBlock(0, g_readBuffer, 256);
    TEST_ASSERT(result == NVM_OK, "Read block");
    
    /* Verify data */
    int match = (memcmp(g_testData, g_readBuffer, 256) == 0);
    TEST_ASSERT(match, "Read data matches written data");
    
    /* Test write with NULL data */
    result = Nvm_WriteBlock(0, NULL, 256);
    TEST_ASSERT(result == NVM_E_PARAM_POINTER, "Write with NULL data fails");
    
    /* Test read with NULL buffer */
    result = Nvm_ReadBlock(0, NULL, 256);
    TEST_ASSERT(result == NVM_E_PARAM_POINTER, "Read with NULL buffer fails");
    
    /* Cleanup */
    Nvm_Deinit();
}

/**
 * @brief Test write protection
 */
void Test_WriteProtection(void)
{
    Nvm_ErrorCode_t result;
    
    printf("\n=== Test: Write Protection ===\n");
    
    /* Initialize */
    Nvm_Init();
    Nvm_RegisterStorage(0, NVM_DEV_FLASH_INTERNAL, 0x10000, 0x10000);
    Nvm_ConfigureBlock(0, "PROT_TEST", NVM_BLOCK_TYPE_DATA, 0x10000, 256, false);
    
    /* Enable global write protection */
    result = Nvm_Core_EnableWriteProtect();
    TEST_ASSERT(result == NVM_OK, "Enable write protection");
    TEST_ASSERT(Nvm_Core_IsWriteProtected() == true, "Write protection active");
    
    /* Try to write while protected */
    result = Nvm_WriteBlock(0, g_testData, 256);
    TEST_ASSERT(result == NVM_E_BLOCK_LOCKED, "Write blocked when protected");
    
    /* Disable protection */
    result = Nvm_Core_DisableWriteProtect(0);
    TEST_ASSERT(result == NVM_OK, "Disable write protection");
    TEST_ASSERT(Nvm_Core_IsWriteProtected() == false, "Write protection inactive");
    
    /* Cleanup */
    Nvm_Deinit();
}

/**
 * @brief Test redundancy
 */
void Test_Redundancy(void)
{
    Nvm_ErrorCode_t result;
    Nvm_RedundancyPair_t pair;
    uint32_t checksum;
    
    printf("\n=== Test: Redundancy ===\n");
    
    /* Initialize */
    Nvm_Init();
    Nvm_RegisterStorage(0, NVM_DEV_FLASH_INTERNAL, 0x10000, 0x10000);
    
    /* Configure redundant blocks */
    Nvm_ConfigureBlock(0, "PRIMARY", NVM_BLOCK_TYPE_DATA, 0x10000, 256, true);
    Nvm_ConfigureBlock(1, "BACKUP", NVM_BLOCK_TYPE_DATA, 0x11000, 256, false);
    
    /* Configure redundancy pair */
    result = Nvm_Redundancy_ConfigurePair(0, 1, true);
    TEST_ASSERT(result == NVM_OK, "Configure redundancy pair");
    
    /* Write with redundancy */
    result = Nvm_Redundancy_Write(0, g_testData, 256);
    TEST_ASSERT(result == NVM_OK, "Write with redundancy");
    
    /* Read with redundancy */
    memset(g_readBuffer, 0, sizeof(g_readBuffer));
    result = Nvm_Redundancy_Read(0, g_readBuffer, 256);
    TEST_ASSERT(result == NVM_OK, "Read with redundancy");
    
    /* Verify pair status */
    result = Nvm_Redundancy_GetPairStatus(0, &pair);
    TEST_ASSERT(result == NVM_OK, "Get pair status");
    TEST_ASSERT(pair.enabled == true, "Pair is enabled");
    TEST_ASSERT(pair.primaryBlockId == 0, "Primary block ID correct");
    TEST_ASSERT(pair.backupBlockId == 1, "Backup block ID correct");
    
    /* Test checksum calculation */
    checksum = Nvm_Redundancy_CalculateChecksum(g_testData, 256);
    TEST_ASSERT(checksum != 0, "Checksum calculated");
    
    /* Cleanup */
    Nvm_Deinit();
}

/**
 * @brief Test job queue
 */
void Test_JobQueue(void)
{
    Nvm_ErrorCode_t result;
    Nvm_Job_t* job;
    uint32_t queued, completed, failed;
    
    printf("\n=== Test: Job Queue ===\n");
    
    /* Initialize */
    Nvm_Init();
    
    /* Test queue empty */
    TEST_ASSERT(Nvm_JobQueue_IsEmpty() == true, "Queue is empty initially");
    
    /* Create job */
    job = Nvm_JobQueue_CreateJob(NVM_JOB_READ, NVM_PRIO_NORMAL, 0);
    TEST_ASSERT(job != NULL, "Create job");
    
    /* Submit job */
    result = Nvm_JobQueue_Submit(job);
    TEST_ASSERT(result == NVM_OK, "Submit job");
    TEST_ASSERT(Nvm_JobQueue_IsEmpty() == false, "Queue not empty");
    
    /* Get pending count */
    queued = Nvm_JobQueue_GetPendingCount();
    TEST_ASSERT(queued == 1, "One job pending");
    
    /* Process job */
    result = Nvm_JobQueue_ProcessOne();
    TEST_ASSERT(result == NVM_OK, "Process one job");
    
    /* Get stats */
    result = Nvm_JobQueue_GetStats(&queued, &completed, &failed);
    TEST_ASSERT(result == NVM_OK, "Get queue stats");
    TEST_ASSERT(queued == 0, "No jobs pending after processing");
    
    /* Cleanup */
    Nvm_Deinit();
}

/**
 * @brief Test CRC32 calculation
 */
void Test_CRC32(void)
{
    uint32_t crc1, crc2;
    uint8_t data1[] = "Hello, World!";
    uint8_t data2[] = "Hello, World!";
    uint8_t data3[] = "Different data";
    
    printf("\n=== Test: CRC32 Calculation ===\n");
    
    /* Same data should produce same CRC */
    crc1 = Nvm_BlockMgr_CalculateCRC32(data1, sizeof(data1));
    crc2 = Nvm_BlockMgr_CalculateCRC32(data2, sizeof(data2));
    TEST_ASSERT(crc1 == crc2, "Same data produces same CRC");
    
    /* Different data should produce different CRC */
    crc2 = Nvm_BlockMgr_CalculateCRC32(data3, sizeof(data3));
    TEST_ASSERT(crc1 != crc2, "Different data produces different CRC");
    
    /* Empty data */
    crc1 = Nvm_BlockMgr_CalculateCRC32(NULL, 100);
    TEST_ASSERT(crc1 == 0, "NULL data returns 0");
    
    crc1 = Nvm_BlockMgr_CalculateCRC32(data1, 0);
    TEST_ASSERT(crc1 == 0, "Zero length returns 0");
}

/**
 * @brief Test version API
 */
void Test_Version(void)
{
    Nvm_ErrorCode_t result;
    uint8_t major, minor, patch;
    
    printf("\n=== Test: Version API ===\n");
    
    result = Nvm_GetVersion(&major, &minor, &patch);
    TEST_ASSERT(result == NVM_OK, "Get version");
    TEST_ASSERT(major == NVM_MAJOR_VERSION, "Major version matches");
    TEST_ASSERT(minor == NVM_MINOR_VERSION, "Minor version matches");
    TEST_ASSERT(patch == NVM_PATCH_VERSION, "Patch version matches");
    
    /* NULL pointer check */
    result = Nvm_GetVersion(NULL, &minor, &patch);
    TEST_ASSERT(result == NVM_E_PARAM_POINTER, "NULL major pointer fails");
}

/**
 * @brief Run all tests
 */
void RunAllTests(void)
{
    printf("\n");
    printf("============================================\n");
    printf("    NVM Module Unit Tests                  \n");
    printf("============================================\n");
    
    Test_Init();
    Test_BlockConfiguration();
    Test_ReadWrite();
    Test_WriteProtection();
    Test_Redundancy();
    Test_JobQueue();
    Test_CRC32();
    Test_Version();
    
    printf("\n");
    printf("============================================\n");
    printf("    Test Summary                           \n");
    printf("============================================\n");
    printf("  Passed: %d\n", g_testsPassed);
    printf("  Failed: %d\n", g_testsFailed);
    printf("  Total:  %d\n", g_testsPassed + g_testsFailed);
    printf("============================================\n");
}

/**
 * @brief Main entry point
 */
int main(void)
{
    RunAllTests();
    return (g_testsFailed > 0) ? 1 : 0;
}
