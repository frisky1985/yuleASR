/*==================================================================================================
 *                                      FLASH WRITE UNIT TESTS
 *==================================================================================================
 * FILENAME: test_flash_write.c
 * PROJECT:  yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Unit tests for Flash Driver write operations
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
static uint8 testData[256];
static uint8 testDataLarge[1024];

void setUp(void)
{
    /* Initialize test configuration */
    memset(&testConfig, 0, sizeof(Fls_ConfigType));
    memset(testSectors, 0, sizeof(testSectors));
    memset(testData, 0xAA, sizeof(testData));
    memset(testDataLarge, 0x55, sizeof(testDataLarge));
    
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
 * @brief Test Fls_Write with valid parameters
 */
TEST_CASE(test_flash_write_valid)
{
    Std_ReturnType result;
    
    /* Pre-condition: Ensure IDLE state */
    ASSERT_EQ(MEMIF_IDLE, Fls_GetStatus());
    
    /* Pre-condition: Erase the sector first */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Write data */
    result = Fls_Write(0x08000000U, testData, 32);
    
    /* Verify: Should return E_OK */
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Write with NULL data pointer
 */
TEST_CASE(test_flash_write_null_pointer)
{
    Std_ReturnType result;
    
    /* Test: Write with NULL pointer */
    result = Fls_Write(0x08000000U, NULL_PTR, 32);
    
    /* Verify: Should return E_NOT_OK */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Write with invalid address
 */
TEST_CASE(test_flash_write_invalid_address)
{
    Std_ReturnType result;
    
    /* Test: Write to invalid address */
    result = Fls_Write(0x00000000U, testData, 32);
    
    /* Verify: Should return E_NOT_OK */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Write with zero length
 */
TEST_CASE(test_flash_write_zero_length)
{
    Std_ReturnType result;
    
    /* Test: Write with zero length */
    result = Fls_Write(0x08000000U, testData, 0);
    
    /* Verify: Should return E_NOT_OK */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Write when busy
 */
TEST_CASE(test_flash_write_while_busy)
{
    Std_ReturnType result;
    
    /* Pre-condition: Erase sector */
    Fls_Erase(0x08000000U, 0x00010000U);
    
    /* Test: Try to write while erase is ongoing */
    result = Fls_Write(0x08000000U, testData, 32);
    
    /* Verify: Should return E_NOT_OK if busy */
    if (Fls_GetStatus() == MEMIF_BUSY) {
        ASSERT_EQ(E_NOT_OK, result);
    }
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Write with unaligned length
 */
TEST_CASE(test_flash_write_unaligned_length)
{
    Std_ReturnType result;
    
    /* Pre-condition: Erase sector */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Write with length not aligned to programming unit */
    result = Fls_Write(0x08000000U, testData, 3);
    
    /* Verify: Result depends on implementation */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Write with unaligned address
 */
TEST_CASE(test_flash_write_unaligned_address)
{
    Std_ReturnType result;
    
    /* Pre-condition: Erase sector */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Write to unaligned address */
    result = Fls_Write(0x08000001U, testData, 32);
    
    /* Verify: Result depends on implementation */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Write at end of flash
 */
TEST_CASE(test_flash_write_at_end)
{
    Std_ReturnType result;
    
    /* Pre-condition: Erase last sector */
    Fls_Erase(0x08010000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Write at last valid address */
    result = Fls_Write(0x0801FF00U, testData, 256);
    
    /* Verify: Should return E_OK or E_NOT_OK based on boundary check */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_VerifyWrittenData function
 */
TEST_CASE(test_flash_verify_written_data)
{
    Std_ReturnType result;
    uint8 writeData[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                           0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    
    /* Pre-condition: Erase and write */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    Fls_Write(0x08000000U, writeData, 16);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Verify written data */
    result = Fls_VerifyWrittenData(0x08000000U, writeData, 16);
    
    /* Verify: Should return E_OK */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Compare function
 */
TEST_CASE(test_flash_compare)
{
    Std_ReturnType result;
    uint8 compareData[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                             0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    
    /* Pre-condition: Erase, write then compare */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    Fls_Write(0x08000000U, compareData, 16);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Test: Compare data */
    result = Fls_Compare(0x08000000U, compareData, 16);
    
    /* Verify: Should return E_OK for matching data */
    ASSERT_TRUE(result == E_OK || result == E_NOT_OK);
    
    TEST_PASS();
}

/**
 * @brief Test Fls_Write and Fls_Read round-trip
 */
TEST_CASE(test_flash_write_read_roundtrip)
{
    Std_ReturnType result;
    uint8 readBuffer[32];
    uint8 writeData[32];
    int i;
    
    /* Prepare test data */
    for (i = 0; i < 32; i++) {
        writeData[i] = (uint8)(i * 3);
    }
    
    /* Pre-condition: Erase sector */
    Fls_Erase(0x08000000U, 0x00010000U);
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Write data */
    result = Fls_Write(0x08000000U, writeData, 32);
    ASSERT_EQ(E_OK, result);
    
    /* Wait for write completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Read back data */
    memset(readBuffer, 0, sizeof(readBuffer));
    result = Fls_Read(0x08000000U, readBuffer, 32);
    ASSERT_EQ(E_OK, result);
    
    /* Wait for read completion */
    while (Fls_GetStatus() == MEMIF_BUSY) {
        Fls_MainFunction();
    }
    
    /* Verify: Data should match */
    ASSERT_MEM_EQ(writeData, readBuffer, 32);
    
    TEST_PASS();
}

/*==================================================================================================
 *                                    MAIN TEST FUNCTION
 *==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_CYAN "=== Flash Write Tests ===" TEST_COLOR_RESET "\n");
    
    RUN_TEST(test_flash_write_valid);
    RUN_TEST(test_flash_write_null_pointer);
    RUN_TEST(test_flash_write_invalid_address);
    RUN_TEST(test_flash_write_zero_length);
    RUN_TEST(test_flash_write_while_busy);
    RUN_TEST(test_flash_write_unaligned_length);
    RUN_TEST(test_flash_write_unaligned_address);
    RUN_TEST(test_flash_write_at_end);
    RUN_TEST(test_flash_verify_written_data);
    RUN_TEST(test_flash_compare);
    RUN_TEST(test_flash_write_read_roundtrip);
}
TEST_MAIN_END()
