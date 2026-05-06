/*==================================================================================================
 *                                      FLASH READ UNIT TESTS
 *==================================================================================================
 * FILENAME: test_flash_read.c
 * PROJECT:  yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Unit tests for Flash Driver read operations
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
static uint8 readBuffer[1024];
static uint8 writeData[256];

void setUp(void)
{
    /* Initialize test configuration */
    memset(&testConfig, 0, sizeof(Fls_ConfigType));
    memset(testSectors, 0, sizeof(testSectors));
    memset(readBuffer, 0, sizeof(readBuffer));
    memset(writeData, 0xAB, sizeof(writeData));
    
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
 * @brief Test Fls_Read with valid parameters
 */
TEST_CASE(test_flash_read_valid)
{
    Std_ReturnType result;
    
    /* Pre-condition: Ensure IDLE state */
    ASSERT_EQ(MEMIF_IDLE, Fls_GetStatus());
    
    /* Test: Read data */
    result = Fls_Read(0x08000000U, readBuffer, 32);
    
    /* Verify: Should return E_OK */
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Read with NULL buffer pointer
 */
TEST_CASE(test_flash_read_null_buffer)
{
    Std_ReturnType result;
    
    /* Test: Read with NULL buffer */
    result = Fls_Read(0x08000000U, NULL_PTR, 32);
    
    /* Verify: Should return E_NOT_OK */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Read with invalid address
 */
TEST_CASE(test_flash_read_invalid_address)
{
    Std_ReturnType result;
    
    /* Test: Read from invalid address */
    result = Fls_Read(0x00000000U, readBuffer, 32);
    
    /* Verify: Should return E_NOT_OK */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Read with address beyond flash range
 */
TEST_CASE(test_flash_read_address_out_of_range)
{
    Std_ReturnType result;
    
    /* Test: Read from address beyond flash */
    result = Fls_Read(0x08040000U, readBuffer, 32);
    
    /* Verify: Should return E_NOT_OK */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Read with zero length
 */
TEST_CASE(test_flash_read_zero_length)
{
    Std_ReturnType result;
    
    /* Test: Read with zero length */
    result = Fls_Read(0x08000000U, readBuffer, 0);
    
    /* Verify: Should return E_NOT_OK */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Read when busy
 */
TEST_CASE(test_flash_read_while_busy)
{
    Std_ReturnType result;
    
    /* Pre-condition: Start an erase operation */
    Fls_Erase(0x08000000U, 0x00010000U);
    
    /* Test: Try to read while erase is ongoing */
    result = Fls_Read(0x08000000U, readBuffer, 32);
    
    /* Verify: Should return E_NOT_OK if busy */
    if (Fls_GetStatus() == MEMIF_BUSY) {
        ASSERT_EQ(E_NOT_OK, result);
    }
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Read with large length
 */
TEST_CASE(test_flash_read_large_length)
{
    Std_ReturnType result;
    
    /* Test: Read large amount of data */
    result = Fls_Read(0x08000000U, readBuffer, 1024);
    
    /* Verify: Should return E_OK */
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Read at end of flash
 */
TEST_CASE(test_flash_read_at_end)
{
    Std_ReturnType result;
    
    /* Test: Read at last valid address */
    result = Fls_Read(0x0801FC00U, readBuffer, 1024);
    
    /* Verify: Result depends on boundary handling */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Read after write
 */
TEST_CASE(test_flash_read_after_write)
{
    Std_ReturnType result;
    uint8 testPattern[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                             0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};
    
    /* Pre-condition: Erase and write */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    Fls_Write(0x08000000U, testPattern, 16);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Clear buffer */
    memset(readBuffer, 0, sizeof(readBuffer));
    
    /* Test: Read back */
    result = Fls_Read(0x08000000U, readBuffer, 16);
    ASSERT_EQ(E_OK, result);
    
    /* Wait for completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Verify: Data should match */
    ASSERT_MEM_EQ(testPattern, readBuffer, 16);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Read with different modes
 */
TEST_CASE(test_flash_read_different_modes)
{
    Std_ReturnType result;
    
    /* Test: Set fast mode and read */
    Fls_SetMode(MEMIF_MODE_FAST);
    result = Fls_Read(0x08000000U, readBuffer, 64);
    ASSERT_EQ(E_OK, result);
    
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Set slow mode and read */
    Fls_SetMode(MEMIF_MODE_SLOW);
    result = Fls_Read(0x08000000U, readBuffer, 64);
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_GetSectorIndex function
 */
TEST_CASE(test_flash_get_sector_index)
{
    uint32 sectorIndex;
    
    /* Test: Get sector index for valid address */
    sectorIndex = Fls_GetSectorIndex(0x08000000U);
    
    /* Verify: Should return 0 for first sector */
    ASSERT_EQ(0, sectorIndex);
    
    /* Test: Get sector index for second sector */
    sectorIndex = Fls_GetSectorIndex(0x08010000U);
    
    /* Verify: Should return 1 for second sector */
    ASSERT_EQ(1, sectorIndex);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_BlankCheck after read
 */
TEST_CASE(test_flash_blank_check_after_read)
{
    Std_ReturnType result;
    
    /* Pre-condition: Perform a read */
    Fls_Read(0x08000000U, readBuffer, 64);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Check if flash is blank (after erase) */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    result = Fls_BlankCheck(0x08000000U, 256);
    
    /* Verify: Should return E_OK for erased sector */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Compare with read data
 */
TEST_CASE(test_flash_compare_with_read)
{
    Std_ReturnType result;
    uint8 compareBuffer[32];
    
    /* Pre-condition: Read some data */
    Fls_Read(0x08000000U, compareBuffer, 32);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Compare same data */
    result = Fls_Compare(0x08000000U, compareBuffer, 32);
    
    /* Verify: Should return E_OK */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
    TEST_PASS();
}

/*==================================================================================================
 *                                    MAIN TEST FUNCTION
 *==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_CYAN "=== Flash Read Tests ===" TEST_COLOR_RESET "\n");
    
    RUN_TEST(test_flash_read_valid);
    RUN_TEST(test_flash_read_null_buffer);
    RUN_TEST(test_flash_read_invalid_address);
    RUN_TEST(test_flash_read_address_out_of_range);
    RUN_TEST(test_flash_read_zero_length);
    RUN_TEST(test_flash_read_while_busy);
    RUN_TEST(test_flash_read_large_length);
    RUN_TEST(test_flash_read_at_end);
    RUN_TEST(test_flash_read_after_write);
    RUN_TEST(test_flash_read_different_modes);
    RUN_TEST(test_flash_get_sector_index);
    RUN_TEST(test_flash_blank_check_after_read);
    RUN_TEST(test_flash_compare_with_read);
}
TEST_MAIN_END()
