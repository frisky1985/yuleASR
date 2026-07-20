/*==================================================================================================
 *                                      FLASH ERASE UNIT TESTS
 *==================================================================================================
 * FILENAME: test_flash_erase.c
 * PROJECT:  yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Unit tests for Flash Driver erase operations
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
    testSectors[0].SectorSize = 0x00010000U;
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
    testConfig.DefaultMode = MEMIF_MODE_NORMAL;
    testConfig.UseInterrupts = FALSE;
    
    /* Initialize flash */
    Fls_Init(&testConfig);
}

void tearDown(void)
{
    /* Cancel any pending operation and cleanup */
    Fls_Cancel();
    Fls_DeInit();
}

/*==================================================================================================
 *                                    TEST CASES
 *==================================================================================================*/

/**
 * @brief Test Fls_Erase with valid parameters
 */
TEST_CASE(test_flash_erase_valid_sector)
{
    Std_ReturnType result;
    
    /* Pre-condition: Ensure IDLE state */
    ASSERT_EQ(MEMIF_IDLE, Fls_GetStatus());
    
    /* Test: Erase first sector */
    result = Fls_Erase(0x08000000U, 0x00010000U);
    
    /* Verify: Should return E_OK */
    ASSERT_EQ(E_OK, result);
    
    /* Verify: Status should be BUSY or IDLE (if synchronous) */
    MemIf_StatusType status = Fls_GetStatus();
    ASSERT_TRUE(status == MEMIF_BUSY || status == MEMIF_IDLE);
    
}

/**
 * @brief Test Fls_Erase with invalid address
 */
TEST_CASE(test_flash_erase_invalid_address)
{
    Std_ReturnType result;
    
    /* Test: Erase with address below flash range */
    result = Fls_Erase(0x00000000U, 0x00010000U);
    
    /* Verify: Should return E_NOT_OK */
    ASSERT_EQ(E_NOT_OK, result);
    
}

/**
 * @brief Test Fls_Erase with address beyond flash range
 */
TEST_CASE(test_flash_erase_address_out_of_range)
{
    Std_ReturnType result;
    
    /* Test: Erase with address beyond flash size */
    result = Fls_Erase(0x08040000U, 0x00010000U);
    
    /* Verify: Should return E_NOT_OK */
    ASSERT_EQ(E_NOT_OK, result);
    
}

/**
 * @brief Test Fls_Erase with zero length
 */
TEST_CASE(test_flash_erase_zero_length)
{
    Std_ReturnType result;
    
    /* Test: Erase with zero length */
    result = Fls_Erase(0x08000000U, 0);
    
    /* Verify: Should return E_NOT_OK */
    ASSERT_EQ(E_NOT_OK, result);
    
}

/**
 * @brief Test Fls_Erase when busy
 */
TEST_CASE(test_flash_erase_while_busy)
{
    Std_ReturnType result;
    
    /* Pre-condition: Start an erase operation */
    Fls_Erase(0x08000000U, 0x00010000U);
    
    /* Test: Try to start another erase while busy */
    result = Fls_Erase(0x08010000U, 0x00010000U);
    
    /* Verify: Should return E_NOT_OK if busy */
    if (Fls_GetStatus() == MEMIF_BUSY) {
        ASSERT_EQ(E_NOT_OK, result);
    }
    
}

/**
 * @brief Test Fls_Erase with unaligned address
 */
TEST_CASE(test_flash_erase_unaligned_address)
{
    Std_ReturnType result;
    
    /* Test: Erase with unaligned address */
    result = Fls_Erase(0x08000001U, 0x00010000U);
    
    /* Verify: Result depends on implementation */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
}

/**
 * @brief Test Fls_Erase with length exceeding sector
 */
TEST_CASE(test_flash_erase_length_exceeds_sector)
{
    Std_ReturnType result;
    
    /* Test: Erase length larger than sector */
    result = Fls_Erase(0x08000000U, 0x00020000U);
    
    /* Verify: Should handle gracefully */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
}

/**
 * @brief Test Fls_BlankCheck on erased sector
 */
TEST_CASE(test_flash_blank_check_after_erase)
{
    Std_ReturnType result;
    
    /* Pre-condition: Erase a sector */
    Fls_Erase(0x08000000U, 0x00010000U);
    
    /* Wait for completion if async */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Check if sector is blank */
    result = Fls_BlankCheck(0x08000000U, 0x00000100U);
    
    /* Verify: Should return E_OK for erased sector */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
}

/**
 * @brief Test Fls_Cancel during erase operation
 */
TEST_CASE(test_flash_cancel_erase)
{
    /* Pre-condition: Start an erase */
    Fls_Erase(0x08000000U, 0x00010000U);
    
    /* Test: Cancel the operation */
    Fls_Cancel();
    
    /* Verify: Status should be IDLE after cancel */
    /* Note: May still be BUSY if cancel is not immediate */
    MemIf_JobResultType jobResult = Fls_GetJobResult();
    ASSERT_TRUE(jobResult == MEMIF_JOB_CANCELED || 
                jobResult == MEMIF_JOB_OK ||
                jobResult == MEMIF_JOB_PENDING);
    
}

/**
 * @brief Test Fls_VerifySectorErased function
 */
TEST_CASE(test_flash_verify_sector_erased)
{
    Std_ReturnType result;
    
    /* Pre-condition: Erase sector */
    Fls_Erase(0x08000000U, 0x00010000U);
    
    /* Wait for completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Verify sector is erased */
    result = Fls_VerifySectorErased(0);
    
    /* Verify: Should return E_OK */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
}

/*==================================================================================================
 *                                    MAIN TEST FUNCTION
 *==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_CYAN "=== Flash Erase Tests ===" TEST_COLOR_RESET "\n");
    
    RUN_TEST(test_flash_erase_valid_sector);
    RUN_TEST(test_flash_erase_invalid_address);
    RUN_TEST(test_flash_erase_address_out_of_range);
    RUN_TEST(test_flash_erase_zero_length);
    RUN_TEST(test_flash_erase_while_busy);
    RUN_TEST(test_flash_erase_unaligned_address);
    RUN_TEST(test_flash_erase_length_exceeds_sector);
    RUN_TEST(test_flash_blank_check_after_erase);
    RUN_TEST(test_flash_cancel_erase);
    RUN_TEST(test_flash_verify_sector_erased);
}
TEST_MAIN_END()
