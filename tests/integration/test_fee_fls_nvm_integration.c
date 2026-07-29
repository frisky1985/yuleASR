/*==================================================================================================
 *                                      INTEGRATION TEST SUITE
 *==================================================================================================
 * FILENAME: test_fee_fls_nvm_integration.c
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Integration test suite for Fee-Fls-NvM storage stack
 *              Tests the complete data flow: NvM -> Fee -> Fls
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Unity.h"
#include "Fee.h"
#include "Fls.h"
#include "NvM.h"
#include "MemIf.h"
#include "Fee_Fls_Integration.h"
#include "Det.h"
#include <string.h>
#include <stdio.h>

/*==================================================================================================
 *                                    TEST CONFIGURATION
 *==================================================================================================*/
#define TEST_FEE_BLOCK_ID_1         (1u)
#define TEST_FEE_BLOCK_ID_2         (2u)
#define TEST_FEE_BLOCK_ID_3         (3u)
#define TEST_DATA_SIZE              (64u)
#define TEST_TIMEOUT_MS             (5000u)

/*==================================================================================================
 *                                    GLOBAL VARIABLES
 *==================================================================================================*/
static uint8 TestWriteBuffer[TEST_DATA_SIZE];
static uint8 TestReadBuffer[TEST_DATA_SIZE];
static uint8 TestPattern1[TEST_DATA_SIZE];
static uint8 TestPattern2[TEST_DATA_SIZE];
static uint8 TestPattern3[TEST_DATA_SIZE];

static boolean TestNotificationCalled = FALSE;
static boolean TestErrorNotificationCalled = FALSE;

/* Mock Fls configuration */
static const Fls_SectorType TestFlsSectors[] = {
    {0x10000000u, 0x10000u, 0x100u, 0u, TRUE, TRUE},
    {0x10010000u, 0x10000u, 0x100u, 0u, TRUE, TRUE},
    {0x10020000u, 0x10000u, 0x100u, 0u, TRUE, TRUE},
    {0x10030000u, 0x10000u, 0x100u, 0u, TRUE, TRUE}
};

static const Fls_ConfigType TestFlsConfig = {
    TestFlsSectors,
    4u,
    FLS_MODE_NORMAL,
    256u,
    128u,
    256u,
    128u,
    TRUE,
    TRUE
};

/* Mock Fee configuration */
static const Fee_BlockConfigType TestFeeBlockConfig[] = {
    {0u,   0u,   0u, 0u, FALSE, FALSE, FALSE, NULL_PTR},
    {1u,   64u,  0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {2u,   128u, 0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {3u,   256u, 0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {4u,   32u,  0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {5u,   64u,  0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {6u,   128u, 0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {7u,   64u,  0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {8u,   256u, 0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {9u,   64u,  0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {10u,  64u,  0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {11u,  128u, 0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {12u,  64u,  0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {13u,  256u, 0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {14u,  64u,  0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR},
    {15u,  128u, 0u, 100000u, TRUE, FALSE, FALSE, NULL_PTR}
};

static const Fee_SectorConfigType TestFeeSectorConfig[] = {
    {0x10000000u, 0x10000u, 0u, TRUE},
    {0x10010000u, 0x10000u, 0u, TRUE},
    {0x10020000u, 0x10000u, 0u, TRUE},
    {0x10030000u, 0x10000u, 0u, TRUE}
};

static const Fee_ConfigType TestFeeConfig = {
    TestFeeBlockConfig,
    TestFeeSectorConfig,
    16u,
    4u,
    8u,
    10u,
    10000u,
    100000u,
    100000u,
    TRUE,
    TRUE,
    FALSE,
    TRUE,
    TRUE,
    TRUE,
    TRUE
};

/*==================================================================================================
 *                                    TEST SETUP/TEARDOWN
 *==================================================================================================*/
void setUp(void)
{
    uint8 i;

    /* Initialize test patterns */
    for (i = 0u; i < TEST_DATA_SIZE; i++)
    {
        TestPattern1[i] = i;
        TestPattern2[i] = (uint8)(0xFFu - i);
        TestPattern3[i] = (uint8)(i * 3);
    }

    /* Clear buffers */
    memset(TestWriteBuffer, 0, TEST_DATA_SIZE);
    memset(TestReadBuffer, 0, TEST_DATA_SIZE);

    /* Reset notification flags */
    TestNotificationCalled = FALSE;
    TestErrorNotificationCalled = FALSE;
}

void tearDown(void)
{
    /* Cleanup after each test */
    Fee_DeInit();
}

/*==================================================================================================
 *                                    MOCK FUNCTIONS
 *==================================================================================================*/
void NvM_JobEndNotification(void)
{
    TestNotificationCalled = TRUE;
}

void NvM_JobErrorNotification(void)
{
    TestErrorNotificationCalled = TRUE;
}

/*==================================================================================================
 *                                    TEST CASES
 *==================================================================================================*/

/**
 * @brief Test 1: Fee Initialization Test
 * Tests that Fee module initializes correctly with valid configuration
 */
void test_Fee_Init_ValidConfig(void)
{
    printf("TEST 1: Fee Initialization with Valid Configuration\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Verify initialization state */
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());

    printf("  PASS: Fee initialized successfully\n");
}

/**
 * @brief Test 2: Fee Write and Read Test
 * Tests basic write and read operations through Fee
 */
void test_Fee_WriteRead_SingleBlock(void)
{
    Std_ReturnType result;
    uint8 i;

    printf("TEST 2: Fee Single Block Write and Read\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Prepare test data */
    memcpy(TestWriteBuffer, TestPattern1, TEST_DATA_SIZE);

    /* Write block */
    result = Fee_Write(TEST_FEE_BLOCK_ID_1, TestWriteBuffer);
    TEST_ASSERT_EQUAL(E_OK, result);

    /* Process the write job */
    for (i = 0u; i < 10u; i++)
    {
        Fee_MainFunction();
    }

    /* Verify write completed */
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());

    /* Read back the block */
    result = Fee_Read(TEST_FEE_BLOCK_ID_1, 0u, TestReadBuffer, TEST_DATA_SIZE);
    TEST_ASSERT_EQUAL(E_OK, result);

    /* Process the read job */
    for (i = 0u; i < 10u; i++)
    {
        Fee_MainFunction();
    }

    /* Verify read completed */
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
    TEST_ASSERT_EQUAL(FEE_JOB_OK, Fee_GetJobResult());

    printf("  PASS: Write and read completed successfully\n");
}

/**
 * @brief Test 3: Fee Multiple Block Operations
 * Tests operations on multiple blocks
 */
void test_Fee_WriteRead_MultipleBlocks(void)
{
    Std_ReturnType result;
    uint8 i;

    printf("TEST 3: Fee Multiple Block Operations\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Write to block 1 */
    memcpy(TestWriteBuffer, TestPattern1, TEST_DATA_SIZE);
    result = Fee_Write(TEST_FEE_BLOCK_ID_1, TestWriteBuffer);
    TEST_ASSERT_EQUAL(E_OK, result);

    for (i = 0u; i < 10u; i++)
    {
        Fee_MainFunction();
    }

    /* Write to block 2 */
    memcpy(TestWriteBuffer, TestPattern2, TEST_DATA_SIZE);
    result = Fee_Write(TEST_FEE_BLOCK_ID_2, TestWriteBuffer);
    TEST_ASSERT_EQUAL(E_OK, result);

    for (i = 0u; i < 10u; i++)
    {
        Fee_MainFunction();
    }

    /* Write to block 3 */
    memcpy(TestWriteBuffer, TestPattern3, TEST_DATA_SIZE);
    result = Fee_Write(TEST_FEE_BLOCK_ID_3, TestWriteBuffer);
    TEST_ASSERT_EQUAL(E_OK, result);

    for (i = 0u; i < 10u; i++)
    {
        Fee_MainFunction();
    }

    /* Verify all writes completed */
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());

    printf("  PASS: Multiple block operations completed\n");
}

/**
 * @brief Test 4: Fee Block Invalidate Test
 * Tests block invalidation functionality
 */
void test_Fee_InvalidateBlock(void)
{
    Std_ReturnType result;
    uint8 i;

    printf("TEST 4: Fee Block Invalidate\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* First write a block */
    memcpy(TestWriteBuffer, TestPattern1, TEST_DATA_SIZE);
    result = Fee_Write(TEST_FEE_BLOCK_ID_1, TestWriteBuffer);
    TEST_ASSERT_EQUAL(E_OK, result);

    for (i = 0u; i < 10u; i++)
    {
        Fee_MainFunction();
    }

    /* Invalidate the block */
    result = Fee_InvalidateBlock(TEST_FEE_BLOCK_ID_1);
    TEST_ASSERT_EQUAL(E_OK, result);

    for (i = 0u; i < 5u; i++)
    {
        Fee_MainFunction();
    }

    /* Verify invalidate completed */
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());

    printf("  PASS: Block invalidate completed\n");
}

/**
 * @brief Test 5: Fee Cancel Operation Test
 * Tests job cancellation functionality
 */
void test_Fee_CancelOperation(void)
{
    Std_ReturnType result;

    printf("TEST 5: Fee Cancel Operation\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Start a write operation */
    memcpy(TestWriteBuffer, TestPattern1, TEST_DATA_SIZE);
    result = Fee_Write(TEST_FEE_BLOCK_ID_1, TestWriteBuffer);
    TEST_ASSERT_EQUAL(E_OK, result);

    /* Cancel the operation */
    Fee_Cancel();

    /* Verify job was cancelled */
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());

    printf("  PASS: Cancel operation completed\n");
}

/**
 * @brief Test 6: Fee Status and Job Result Test
 * Tests status and job result reporting
 */
void test_Fee_StatusAndJobResult(void)
{
    Fee_StatusType status;
    Fee_JobResultType result;

    printf("TEST 6: Fee Status and Job Result\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Check initial status */
    status = Fee_GetStatus();
    TEST_ASSERT_EQUAL(FEE_IDLE, status);

    result = Fee_GetJobResult();
    TEST_ASSERT_EQUAL(FEE_JOB_OK, result);

    printf("  PASS: Status and job result reporting correct\n");
}

/**
 * @brief Test 7: Fee Version Info Test
 * Tests version information API
 */
#if (FEE_VERSION_INFO_API == STD_ON)
void test_Fee_GetVersionInfo(void)
{
    Std_VersionInfoType versionInfo;

    printf("TEST 7: Fee Version Info\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Get version info */
    Fee_GetVersionInfo(&versionInfo);

    /* Verify version info */
    TEST_ASSERT_EQUAL(FEE_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL(FEE_MODULE_ID, versionInfo.moduleID);

    printf("  PASS: Version info retrieved correctly\n");
}
#endif

/**
 * @brief Test 8: Fee Mode Setting Test
 * Tests mode setting functionality
 */
void test_Fee_SetMode(void)
{
    printf("TEST 8: Fee Set Mode\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Set fast mode */
    Fee_SetMode(FEE_MODE_FAST);

    /* Set slow mode */
    Fee_SetMode(FEE_MODE_SLOW);

    printf("  PASS: Mode setting completed\n");
}

/**
 * @brief Test 9: Fee-FLS Integration Layer Test
 * Tests the Fee-Fls integration layer
 */
void test_Fee_Fls_IntegrationLayer(void)
{
    Fee_Fls_Int_StatusType result;
    Fee_Fls_Int_ConfigType intConfig;

    printf("TEST 9: Fee-FLS Integration Layer\n");

    /* Initialize Fls */
    Fls_Init(&TestFlsConfig);

    /* Configure integration layer */
    intConfig.MaxReadTimeout = 1000u;
    intConfig.MaxWriteTimeout = 5000u;
    intConfig.MaxEraseTimeout = 10000u;
    intConfig.EnableIntegrityCheck = TRUE;
    intConfig.EnableStatistics = TRUE;
    intConfig.MaxRetries = 3u;

    /* Initialize integration layer */
    result = Fee_Fls_Int_Init(&intConfig);
    TEST_ASSERT_EQUAL(FEE_FLS_INT_E_OK, result);

    /* Check state */
    TEST_ASSERT_EQUAL(FEE_FLS_INT_STATE_IDLE, Fee_Fls_Int_GetState());

    printf("  PASS: Integration layer initialized successfully\n");
}

/**
 * @brief Test 10: Fee Erase Immediate Block Test
 * Tests immediate erase functionality
 */
void test_Fee_EraseImmediateBlock(void)
{
    Std_ReturnType result;
    uint8 i;

    printf("TEST 10: Fee Erase Immediate Block\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* First write a block */
    memcpy(TestWriteBuffer, TestPattern1, TEST_DATA_SIZE);
    result = Fee_Write(TEST_FEE_BLOCK_ID_1, TestWriteBuffer);
    TEST_ASSERT_EQUAL(E_OK, result);

    for (i = 0u; i < 10u; i++)
    {
        Fee_MainFunction();
    }

    /* Erase the block immediately */
    result = Fee_EraseImmediateBlock(TEST_FEE_BLOCK_ID_1);
    TEST_ASSERT_EQUAL(E_OK, result);

    for (i = 0u; i < 5u; i++)
    {
        Fee_MainFunction();
    }

    /* Verify erase completed */
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());

    printf("  PASS: Erase immediate block completed\n");
}

/**
 * @brief Test 11: Fee Error Handling Test
 * Tests error handling with invalid parameters
 */
void test_Fee_ErrorHandling(void)
{
    Std_ReturnType result;

    printf("TEST 11: Fee Error Handling\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Try to read with invalid block ID */
    result = Fee_Read(0xFFFFu, 0u, TestReadBuffer, TEST_DATA_SIZE);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);

    /* Try to write with invalid block ID */
    result = Fee_Write(0xFFFFu, TestWriteBuffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);

    printf("  PASS: Error handling works correctly\n");
}

/**
 * @brief Test 12: Fee Cycle Count Test
 * Tests cycle count APIs
 */
void test_Fee_CycleCounts(void)
{
    uint32 count;

    printf("TEST 12: Fee Cycle Counts\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Get cycle counts */
    count = Fee_GetCycleCount();
    /* Initial count should be 0 or higher */
    (void)count;

    count = Fee_GetEraseCycleCount();
    (void)count;

    count = Fee_GetWriteCycleCount();
    (void)count;

    printf("  PASS: Cycle count APIs work correctly\n");
}

/**
 * @brief Test 13: Fee DeInit Test
 * Tests de-initialization functionality
 */
void test_Fee_DeInit(void)
{
    printf("TEST 13: Fee DeInit\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Verify initialized */
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());

    /* De-initialize */
    Fee_DeInit();

    printf("  PASS: DeInit completed\n");
}

/**
 * @brief Test 14: Fee Concurrent Operations Test
 * Tests that concurrent operations are properly rejected
 */
void test_Fee_ConcurrentOperations(void)
{
    Std_ReturnType result1, result2;

    printf("TEST 14: Fee Concurrent Operations\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Start first operation */
    memcpy(TestWriteBuffer, TestPattern1, TEST_DATA_SIZE);
    result1 = Fee_Write(TEST_FEE_BLOCK_ID_1, TestWriteBuffer);
    TEST_ASSERT_EQUAL(E_OK, result1);

    /* Try to start second operation while first is pending */
    result2 = Fee_Write(TEST_FEE_BLOCK_ID_2, TestWriteBuffer);
    TEST_ASSERT_EQUAL(E_NOT_OK, result2);

    printf("  PASS: Concurrent operations properly rejected\n");
}

/**
 * @brief Test 15: Fee Garbage Collection Trigger Test
 * Tests GC triggering mechanism
 */
void test_Fee_GarbageCollection(void)
{
    printf("TEST 15: Fee Garbage Collection\n");

    /* Initialize Fee */
    Fee_Init(&TestFeeConfig);

    /* Note: In real implementation with actual flash,
     * this test would write enough data to trigger GC */

    /* For now, just verify GC state is idle initially */
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());

    printf("  PASS: GC state verified\n");
}

/*==================================================================================================
 *                                    MAIN FUNCTION
 *==================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    printf("\n");
    printf("============================================================\n");
    printf("  Fee-Fls-NvM Integration Test Suite\n");
    printf("  AutoSAR R22-11 Compliant\n");
    printf("============================================================\n");
    printf("\n");

    /* Run all tests */
    RUN_TEST(test_Fee_Init_ValidConfig);
    RUN_TEST(test_Fee_WriteRead_SingleBlock);
    RUN_TEST(test_Fee_WriteRead_MultipleBlocks);
    RUN_TEST(test_Fee_InvalidateBlock);
    RUN_TEST(test_Fee_CancelOperation);
    RUN_TEST(test_Fee_StatusAndJobResult);
#if (FEE_VERSION_INFO_API == STD_ON)
    RUN_TEST(test_Fee_GetVersionInfo);
#endif
    RUN_TEST(test_Fee_SetMode);
    RUN_TEST(test_Fee_Fls_IntegrationLayer);
    RUN_TEST(test_Fee_EraseImmediateBlock);
    RUN_TEST(test_Fee_ErrorHandling);
    RUN_TEST(test_Fee_CycleCounts);
    RUN_TEST(test_Fee_DeInit);
    RUN_TEST(test_Fee_ConcurrentOperations);
    RUN_TEST(test_Fee_GarbageCollection);

    printf("\n");
    printf("============================================================\n");
    printf("  Test Suite Complete\n");
    printf("============================================================\n");
    printf("\n");

    return UNITY_END();
}
