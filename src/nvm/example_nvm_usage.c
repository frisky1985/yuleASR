/**
 * @file example_nvm_usage.c
 * @brief NvM Module Usage Example
 * @version 1.0.0
 * @date 2025
 * 
 * This example demonstrates how to use the NVM module for:
 * - Basic read/write operations
 * - Block configuration
 * - Redundancy setup
 * - Asynchronous operations
 * - Error handling
 */

#include "nvm.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*============================================================================*
 * Example Configuration
 *============================================================================*/
#define EXAMPLE_STORAGE_BASE    0x00010000u
#define EXAMPLE_STORAGE_SIZE    0x00010000u  /* 64KB */

#define BLOCK_CONFIG_DATA       0u
#define BLOCK_RUNTIME_DATA      1u
#define BLOCK_DIAGNOSTIC_DATA   2u
#define BLOCK_REDUNDANT_DATA    3u
#define BLOCK_BACKUP_DATA       4u

#define DATA_LENGTH             128u

/*============================================================================*
 * Example Data Structures
 *============================================================================*/
typedef struct {
    uint32_t vehicleId;
    uint32_t configVersion;
    uint8_t  features[16];
    uint32_t checksum;
} ConfigData_t;

typedef struct {
    uint32_t timestamp;
    uint32_t mileage;
    uint32_t errorCodes[4];
} DiagnosticData_t;

/*============================================================================*
 * Example Data Buffers
 *============================================================================*/
static uint8_t g_readBuffer[DATA_LENGTH];
static uint8_t g_writeBuffer[DATA_LENGTH];

/*============================================================================*
 * Callback Functions
 *============================================================================*/

static void AsyncWriteCallback(
    uint16_t blockId,
    Nvm_JobType_t jobType,
    Nvm_ErrorCode_t result,
    void* userData)
{
    (void)jobType;
    
    printf("Async write completed - Block %d: ", blockId);
    
    if (result == NVM_OK) {
        printf("SUCCESS\n");
    } else {
        printf("FAILED (error %d)\n", result);
    }
    
    if (userData != NULL) {
        printf("User data: %s\n", (char*)userData);
    }
}

static void AsyncReadCallback(
    uint16_t blockId,
    Nvm_JobType_t jobType,
    Nvm_ErrorCode_t result,
    void* userData)
{
    (void)jobType;
    (void)userData;
    
    printf("Async read completed - Block %d: ", blockId);
    
    if (result == NVM_OK) {
        printf("SUCCESS\n");
        printf("Data: %s\n", g_readBuffer);
    } else {
        printf("FAILED (error %d)\n", result);
    }
}

/*============================================================================*
 * Example Functions
 *============================================================================*/

/**
 * @brief Example: Initialize NVM module and configure blocks
 */
void Example_InitAndConfigure(void)
{
    Nvm_ErrorCode_t result;
    
    printf("=== Example: Initialize NVM Module ===\n");
    
    /* Initialize NVM module */
    result = Nvm_Init();
    if (result != NVM_OK) {
        printf("NVM Init failed: %d\n", result);
        return;
    }
    printf("NVM Module initialized successfully\n");
    
    /* Register storage device (simulated) */
    result = Nvm_RegisterStorage(
        0,                          /* Device ID */
        NVM_DEV_FLASH_INTERNAL,     /* Device type */
        EXAMPLE_STORAGE_BASE,       /* Base address */
        EXAMPLE_STORAGE_SIZE        /* Total size */
    );
    if (result != NVM_OK) {
        printf("Storage registration failed: %d\n", result);
        return;
    }
    printf("Storage device registered\n");
    
    /* Configure configuration block (with redundancy) */
    result = Nvm_ConfigureBlock(
        BLOCK_CONFIG_DATA,
        "CONFIG_DATA",
        NVM_BLOCK_TYPE_CONFIG,
        EXAMPLE_STORAGE_BASE,
        sizeof(ConfigData_t),
        true                        /* Enable redundancy */
    );
    if (result != NVM_OK) {
        printf("Block configuration failed: %d\n", result);
        return;
    }
    printf("Configuration block configured (with redundancy)\n");
    
    /* Configure runtime data block */
    result = Nvm_ConfigureBlock(
        BLOCK_RUNTIME_DATA,
        "RUNTIME_DATA",
        NVM_BLOCK_TYPE_DATA,
        EXAMPLE_STORAGE_BASE + 0x1000,
        DATA_LENGTH,
        false                       /* No redundancy */
    );
    if (result != NVM_OK) {
        printf("Block configuration failed: %d\n", result);
        return;
    }
    printf("Runtime data block configured\n");
    
    /* Configure diagnostic block */
    result = Nvm_ConfigureBlock(
        BLOCK_DIAGNOSTIC_DATA,
        "DIAG_DATA",
        NVM_BLOCK_TYPE_DIAGNOSTIC,
        EXAMPLE_STORAGE_BASE + 0x2000,
        sizeof(DiagnosticData_t),
        true                        /* Enable redundancy for critical data */
    );
    if (result != NVM_OK) {
        printf("Block configuration failed: %d\n", result);
        return;
    }
    printf("Diagnostic block configured (with redundancy)\n");
    
    printf("\n");
}

/**
 * @brief Example: Basic write and read operations
 */
void Example_BasicReadWrite(void)
{
    Nvm_ErrorCode_t result;
    ConfigData_t config = {0};
    ConfigData_t configRead = {0};
    
    printf("=== Example: Basic Read/Write ===\n");
    
    /* Prepare configuration data */
    config.vehicleId = 0x12345678;
    config.configVersion = 0x00010001;
    memset(config.features, 0xAA, sizeof(config.features));
    config.checksum = 0;
    
    /* Write configuration */
    printf("Writing configuration data...\n");
    result = Nvm_WriteBlock(
        BLOCK_CONFIG_DATA,
        (uint8_t*)&config,
        sizeof(ConfigData_t)
    );
    if (result != NVM_OK) {
        printf("Write failed: %d\n", result);
        return;
    }
    printf("Write successful\n");
    
    /* Read configuration back */
    printf("Reading configuration data...\n");
    result = Nvm_ReadBlock(
        BLOCK_CONFIG_DATA,
        (uint8_t*)&configRead,
        sizeof(ConfigData_t)
    );
    if (result != NVM_OK) {
        printf("Read failed: %d\n", result);
        return;
    }
    printf("Read successful\n");
    
    /* Verify data */
    if (configRead.vehicleId == config.vehicleId &&
        configRead.configVersion == config.configVersion) {
        printf("Data verification: PASSED\n");
    } else {
        printf("Data verification: FAILED\n");
    }
    
    printf("\n");
}

/**
 * @brief Example: Write string data
 */
void Example_WriteStringData(void)
{
    Nvm_ErrorCode_t result;
    const char* testString = "Hello, NVM World! This is test data.";
    
    printf("=== Example: Write String Data ===\n");
    
    /* Prepare write buffer */
    memset(g_writeBuffer, 0, sizeof(g_writeBuffer));
    strncpy((char*)g_writeBuffer, testString, sizeof(g_writeBuffer) - 1);
    
    /* Write string */
    printf("Writing string: '%s'\n", testString);
    result = Nvm_WriteBlock(
        BLOCK_RUNTIME_DATA,
        g_writeBuffer,
        strlen(testString) + 1
    );
    if (result != NVM_OK) {
        printf("Write failed: %d\n", result);
        return;
    }
    printf("Write successful\n");
    
    /* Read back */
    memset(g_readBuffer, 0, sizeof(g_readBuffer));
    result = Nvm_ReadBlock(
        BLOCK_RUNTIME_DATA,
        g_readBuffer,
        strlen(testString) + 1
    );
    if (result != NVM_OK) {
        printf("Read failed: %d\n", result);
        return;
    }
    
    printf("Read back: '%s'\n", g_readBuffer);
    
    if (strcmp((char*)g_readBuffer, testString) == 0) {
        printf("String comparison: PASSED\n");
    } else {
        printf("String comparison: FAILED\n");
    }
    
    printf("\n");
}

/**
 * @brief Example: Block validation and error handling
 */
void Example_BlockValidation(void)
{
    Nvm_ErrorCode_t result;
    
    printf("=== Example: Block Validation ===\n");
    
    /* Validate configuration block */
    result = Nvm_ValidateBlock(BLOCK_CONFIG_DATA);
    if (result == NVM_OK) {
        printf("Config block validation: VALID\n");
    } else {
        printf("Config block validation: INVALID (error %d)\n", result);
    }
    
    /* Validate runtime data block */
    result = Nvm_ValidateBlock(BLOCK_RUNTIME_DATA);
    if (result == NVM_OK) {
        printf("Runtime block validation: VALID\n");
    } else {
        printf("Runtime block validation: INVALID (error %d)\n", result);
    }
    
    /* Try to validate non-existent block */
    result = Nvm_ValidateBlock(99);
    if (result == NVM_OK) {
        printf("Block 99 validation: VALID\n");
    } else {
        printf("Block 99 validation: INVALID (error %d) - Expected!\n", result);
    }
    
    printf("\n");
}

/**
 * @brief Example: Asynchronous operations
 */
void Example_AsyncOperations(void)
{
    Nvm_ErrorCode_t result;
    const char* asyncData = "Async write test data";
    
    printf("=== Example: Asynchronous Operations ===\n");
    
    /* Prepare data */
    memset(g_writeBuffer, 0, sizeof(g_writeBuffer));
    strncpy((char*)g_writeBuffer, asyncData, sizeof(g_writeBuffer) - 1);
    
    /* Submit async write */
    printf("Submitting async write...\n");
    result = Nvm_WriteBlockAsync(
        BLOCK_RUNTIME_DATA,
        g_writeBuffer,
        strlen(asyncData) + 1,
        AsyncWriteCallback,
        (void*)"Write context"
    );
    if (result != NVM_OK) {
        printf("Async write submit failed: %d\n", result);
        return;
    }
    
    /* Process jobs (in real app, this would be done in cyclic task) */
    printf("Processing jobs...\n");
    while (!Nvm_JobQueue_IsEmpty()) {
        Nvm_MainFunction();
    }
    
    /* Submit async read */
    printf("Submitting async read...\n");
    memset(g_readBuffer, 0, sizeof(g_readBuffer));
    result = Nvm_ReadBlockAsync(
        BLOCK_RUNTIME_DATA,
        g_readBuffer,
        strlen(asyncData) + 1,
        AsyncReadCallback,
        NULL
    );
    if (result != NVM_OK) {
        printf("Async read submit failed: %d\n", result);
        return;
    }
    
    /* Process jobs */
    printf("Processing jobs...\n");
    while (!Nvm_JobQueue_IsEmpty()) {
        Nvm_MainFunction();
    }
    
    printf("\n");
}

/**
 * @brief Example: NVM Statistics
 */
void Example_GetStatistics(void)
{
    Nvm_ErrorCode_t result;
    uint32_t blocks, jobs, wearLevel;
    uint8_t major, minor, patch;
    Nvm_SafetyMonitor_t safety;
    
    printf("=== Example: NVM Statistics ===\n");
    
    /* Get version */
    result = Nvm_GetVersion(&major, &minor, &patch);
    if (result == NVM_OK) {
        printf("NVM Version: %d.%d.%d\n", major, minor, patch);
    }
    
    /* Get general statistics */
    result = Nvm_GetStatistics(&blocks, &jobs, &wearLevel);
    if (result == NVM_OK) {
        printf("Configured Blocks: %d\n", blocks);
        printf("Pending Jobs: %d\n", jobs);
        printf("Wear Level: %d%%\n", wearLevel);
    }
    
    /* Get safety monitor data */
    result = Nvm_Core_GetSafetyMonitor(&safety);
    if (result == NVM_OK) {
        printf("Read Count: %d\n", safety.readCount);
        printf("Write Count: %d\n", safety.writeCount);
        printf("Error Count: %d\n", safety.errorCount);
        printf("Retry Count: %d\n", safety.retryCount);
        printf("Recovery Count: %d\n", safety.recoveryCount);
    }
    
    /* Get block manager statistics */
    printf("Total Data Size: %d bytes\n", Nvm_BlockMgr_GetTotalDataSize());
    
    /* Dump block info */
    printf("\nBlock Information:\n");
    Nvm_BlockMgr_DumpBlockInfo(BLOCK_CONFIG_DATA);
    
    printf("\n");
}

/**
 * @brief Example: Write protection
 */
void Example_WriteProtection(void)
{
    Nvm_ErrorCode_t result;
    Nvm_Block_t* block;
    
    printf("=== Example: Write Protection ===\n");
    
    /* Enable global write protection */
    result = Nvm_Core_EnableWriteProtect();
    if (result == NVM_OK) {
        printf("Global write protection: ENABLED\n");
    }
    
    /* Try to write while protected */
    result = Nvm_WriteBlock(
        BLOCK_RUNTIME_DATA,
        (uint8_t*)"Test",
        5
    );
    if (result == NVM_E_BLOCK_LOCKED) {
        printf("Write blocked as expected (protection active)\n");
    } else {
        printf("Unexpected result: %d\n", result);
    }
    
    /* Disable protection */
    result = Nvm_Core_DisableWriteProtect(0);
    if (result == NVM_OK) {
        printf("Global write protection: DISABLED\n");
    }
    
    /* Check if block is protected */
    block = Nvm_BlockMgr_GetBlock(BLOCK_CONFIG_DATA);
    if (block != NULL) {
        printf("Block protection level: %d\n", block->protection);
    }
    
    printf("\n");
}

/**
 * @brief Example: Cleanup
 */
void Example_Cleanup(void)
{
    Nvm_ErrorCode_t result;
    
    printf("=== Example: Cleanup ===\n");
    
    result = Nvm_Deinit();
    if (result == NVM_OK) {
        printf("NVM Module deinitialized successfully\n");
    } else {
        printf("Deinit failed: %d\n", result);
    }
    
    printf("\n");
}

/*============================================================================*
 * Main Function
 *============================================================================*/

int main(void)
{
    printf("\n");
    printf("============================================\n");
    printf("    NVM Module Usage Example               \n");
    printf("    ASIL-D Compliant Non-Volatile Memory  \n");
    printf("============================================\n");
    printf("\n");
    
    /* Run examples */
    Example_InitAndConfigure();
    Example_BasicReadWrite();
    Example_WriteStringData();
    Example_BlockValidation();
    Example_AsyncOperations();
    Example_WriteProtection();
    Example_GetStatistics();
    Example_Cleanup();
    
    printf("============================================\n");
    printf("    Example Completed                      \n");
    printf("============================================\n");
    
    return 0;
}
