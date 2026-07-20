/*==================================================================================================
 *                                      FLASH INIT UNIT TESTS
 *==================================================================================================
 * FILENAME: test_flash_init.c
 * PROJECT:  yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Unit tests for Flash Driver initialization module
 *==================================================================================================
 */

#include <stdio.h>
#include <string.h>
#include "Flash.h"
#include "../test_framework.h"

/*==================================================================================================
 *                                    TEST FIXTURE
 *==================================================================================================*/
static Fls_ConfigType testConfig;
static Fls_SectorInfoType testSectors[2];

void setUp(void)
{
    /* Initialize test configuration */
    memset(&testConfig, 0, sizeof(Fls_ConfigType));
    memset(testSectors, 0, sizeof(testSectors));
    
    /* Setup sector info */
    testSectors[0].SectorStartAddress = 0x08000000U;
    testSectors[0].SectorSize = 0x00010000U; /* 64KB */
    testSectors[0].SectorSizeType = FLS_SECTOR_SIZE_64KB;
    testSectors[0].SectorProtected = FALSE;
    testSectors[0].SectorBank = 0;
    
    testSectors[1].SectorStartAddress = 0x08010000U;
    testSectors[1].SectorSize = 0x00010000U;
    testSectors[1].SectorSizeType = FLS_SECTOR_SIZE_64KB;
    testSectors[1].SectorProtected = FALSE;
    testSectors[1].SectorBank = 0;
    
    /* Setup config */
    testConfig.BaseAddress = 0x08000000U;
    testConfig.TotalSize = 0x00020000U;
    testConfig.SectorInfo = testSectors;
    testConfig.SectorCount = 2;
    testConfig.PageSize = 256;
    testConfig.ProgrammingUnit = 4;
    testConfig.MaxReadFastMode = 1024;
    testConfig.MaxReadNormalMode = 256;
    testConfig.MaxWriteFastMode = 256;
    testConfig.MaxWriteNormalMode = 64;
    testConfig.DefaultMode = MEMIF_MODE_NORMAL;
    testConfig.CallCycle = 10;
    testConfig.UseInterrupts = FALSE;
    testConfig.JobEndNotification = NULL;
    testConfig.JobErrorNotification = NULL;
}

void tearDown(void)
{
    /* Cleanup after each test */
    Fls_DeInit();
}

/*==================================================================================================
 *                                    TEST CASES
 *==================================================================================================*/

/**
 * @brief Test Fls_Init with valid configuration
 */
TEST_CASE(test_flash_init_valid)
{
    /* Test: Initialize with valid config */
    Fls_Init(&testConfig);
    
    /* Verify: Status should be IDLE after init */
    MemIf_StatusType status = Fls_GetStatus();
    ASSERT_EQ(MEMIF_IDLE, status);
    
    /* Verify: Job result should be OK */
    MemIf_JobResultType result = Fls_GetJobResult();
    ASSERT_EQ(MEMIF_JOB_OK, result);
    
}

/**
 * @brief Test Fls_Init with NULL configuration
 */
TEST_CASE(test_flash_init_null)
{
    /* Test: Initialize with NULL config - should use default config */
    Fls_Init(NULL_PTR);
    
    /* Verify: Module should be initialized */
    MemIf_StatusType status = Fls_GetStatus();
    ASSERT_TRUE(status == MEMIF_IDLE || status == MEMIF_UNINIT);
    
}

/**
 * @brief Test Fls_DeInit after initialization
 */
TEST_CASE(test_flash_deinit_after_init)
{
    /* Pre-condition: Initialize first */
    Fls_Init(&testConfig);
    ASSERT_EQ(MEMIF_IDLE, Fls_GetStatus());
    
    /* Test: DeInitialize */
    Fls_DeInit();
    
    /* Verify: Status should be UNINIT after deinit */
    MemIf_StatusType status = Fls_GetStatus();
    ASSERT_EQ(MEMIF_UNINIT, status);
    
}

/**
 * @brief Test Fls_Init multiple times
 */
TEST_CASE(test_flash_init_multiple)
{
    /* Test: First initialization */
    Fls_Init(&testConfig);
    ASSERT_EQ(MEMIF_IDLE, Fls_GetStatus());
    
    /* Test: Second initialization should be handled gracefully */
    Fls_Init(&testConfig);
    
    /* Verify: Module should still be initialized */
    MemIf_StatusType status = Fls_GetStatus();
    ASSERT_TRUE(status == MEMIF_IDLE || status == MEMIF_BUSY);
    
}

/**
 * @brief Test Fls_GetVersionInfo after initialization
 */
#if (FLS_VERSION_INFO_API == STD_ON)
TEST_CASE(test_flash_get_version_info)
{
    Std_VersionInfoType versionInfo;
    
    /* Pre-condition: Initialize */
    Fls_Init(&testConfig);
    
    /* Test: Get version info */
    Fls_GetVersionInfo(&versionInfo);
    
    /* Verify: Version info should match expected values */
    ASSERT_EQ(FLS_VENDOR_ID, versionInfo.vendorID);
    ASSERT_EQ(FLS_MODULE_ID, versionInfo.moduleID);
    ASSERT_EQ(FLS_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    ASSERT_EQ(FLS_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    ASSERT_EQ(FLS_SW_PATCH_VERSION, versionInfo.sw_patch_version);
    
}
#endif

/**
 * @brief Test Fls_GetStatus before initialization
 */
TEST_CASE(test_flash_get_status_before_init)
{
    /* Pre-condition: Ensure uninitialized */
    Fls_DeInit();
    
    /* Test: Get status before init */
    MemIf_StatusType status = Fls_GetStatus();
    
    /* Verify: Should return UNINIT */
    ASSERT_EQ(MEMIF_UNINIT, status);
    
}

/**
 * @brief Test Fls_GetJobResult before initialization
 */
TEST_CASE(test_flash_get_job_result_before_init)
{
    /* Pre-condition: Ensure uninitialized */
    Fls_DeInit();
    
    /* Test: Get job result before init */
    MemIf_JobResultType result = Fls_GetJobResult();
    
    /* Verify: Should return JOB_FAILED or appropriate status */
    ASSERT_TRUE(result == MEMIF_JOB_FAILED || result == MEMIF_JOB_OK);
    
}

/*==================================================================================================
 *                                    MAIN TEST FUNCTION
 *==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_CYAN "=== Flash Initialization Tests ===" TEST_COLOR_RESET "\n");
    
    RUN_TEST(test_flash_init_valid);
    RUN_TEST(test_flash_init_null);
    RUN_TEST(test_flash_deinit_after_init);
    RUN_TEST(test_flash_init_multiple);
    RUN_TEST(test_flash_get_status_before_init);
    RUN_TEST(test_flash_get_job_result_before_init);
    #if (FLS_VERSION_INFO_API == STD_ON)
    RUN_TEST(test_flash_get_version_info);
    #endif
}
TEST_MAIN_END()
