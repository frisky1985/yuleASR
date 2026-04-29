/*==================================================================================================
 *                                      FLS HW UNIT TESTS
 *==================================================================================================
 * FILENAME: test_fls_hw.c
 * DESCRIPTION: Unit tests for Flash Driver Hardware Abstraction Layer
 *==================================================================================================
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Fls_Hw.h"

/*==================================================================================================
 *                                    TEST CONFIGURATION
 *==================================================================================================*/
#define TEST_FLS_BASE_ADDR              (0x08000000u)
#define TEST_FLS_SIZE                   (0x00100000u)  /* 1 MB */

/* Mock register values */
static uint32 mockFlashCR = 0u;
static uint32 mockFlashSR = 0u;
static uint32 mockFlashKEYR = 0u;

/* Test data */
static uint8 testWriteBuffer[256];
static uint8 testReadBuffer[256];

/*==================================================================================================
 *                                    TEST HELPERS
 *==================================================================================================*/

void setUp(void)
{
    /* Reset state before each test */
    mockFlashCR = 0x80000000u;  /* Locked */
    mockFlashSR = 0u;
    mockFlashKEYR = 0u;
    memset(testWriteBuffer, 0xAA, sizeof(testWriteBuffer));
    memset(testReadBuffer, 0x00, sizeof(testReadBuffer));
}

void tearDown(void)
{
    /* Cleanup after each test */
}

/*==================================================================================================
 *                                    TEST CASES
 *==================================================================================================*/

/**
 * @brief Test Fls_Hw_Init with valid configuration
 */
void test_Fls_Hw_Init_ValidConfig(void)
{
    Fls_Hw_ConfigType config = {
        .flashBaseAddress = TEST_FLS_BASE_ADDR,
        .flashSize = TEST_FLS_SIZE,
        .sectorCount = 16u,
        .pageSize = 4u,
        .useInterrupts = FALSE,
        .timeoutMs = 1000u,
        .clockFreqHz = 16000000u
    };

    Std_ReturnType result = Fls_Hw_Init(&config);

    assert(result == E_OK);
    assert(Fls_Hw_GetStatus() == FLS_HW_STATUS_IDLE);
    printf("PASS: test_Fls_Hw_Init_ValidConfig\n");
}

/**
 * @brief Test Fls_Hw_Init with NULL pointer
 */
void test_Fls_Hw_Init_NullConfig(void)
{
    Std_ReturnType result = Fls_Hw_Init(NULL_PTR);

    /* Generic implementation accepts NULL and uses defaults */
    assert(result == E_OK);
    printf("PASS: test_Fls_Hw_Init_NullConfig\n");
}

/**
 * @brief Test Fls_Hw_DeInit
 */
void test_Fls_Hw_DeInit(void)
{
    Fls_Hw_ConfigType config = {
        .flashBaseAddress = TEST_FLS_BASE_ADDR,
        .flashSize = TEST_FLS_SIZE,
        .sectorCount = 16u,
        .pageSize = 4u,
        .useInterrupts = FALSE,
        .timeoutMs = 1000u,
        .clockFreqHz = 16000000u
    };

    Fls_Hw_Init(&config);
    Std_ReturnType result = Fls_Hw_DeInit();

    assert(result == E_OK);
    printf("PASS: test_Fls_Hw_DeInit\n");
}

/**
 * @brief Test Fls_Hw_GetSectorNumber with valid address
 */
void test_Fls_Hw_GetSectorNumber_Valid(void)
{
    Fls_Hw_ConfigType config = {
        .flashBaseAddress = TEST_FLS_BASE_ADDR,
        .flashSize = TEST_FLS_SIZE,
        .sectorCount = 16u,
        .pageSize = 4u,
        .useInterrupts = FALSE,
        .timeoutMs = 1000u,
        .clockFreqHz = 16000000u
    };

    Fls_Hw_Init(&config);

    uint32 sector = Fls_Hw_GetSectorNumber(TEST_FLS_BASE_ADDR);
    assert(sector == 0u);

    sector = Fls_Hw_GetSectorNumber(TEST_FLS_BASE_ADDR + 0x10000u);
    assert(sector == 1u);

    printf("PASS: test_Fls_Hw_GetSectorNumber_Valid\n");
}

/**
 * @brief Test Fls_Hw_GetSectorNumber with invalid address
 */
void test_Fls_Hw_GetSectorNumber_Invalid(void)
{
    Fls_Hw_ConfigType config = {
        .flashBaseAddress = TEST_FLS_BASE_ADDR,
        .flashSize = TEST_FLS_SIZE,
        .sectorCount = 16u,
        .pageSize = 4u,
        .useInterrupts = FALSE,
        .timeoutMs = 1000u,
        .clockFreqHz = 16000000u
    };

    Fls_Hw_Init(&config);

    uint32 sector = Fls_Hw_GetSectorNumber(0x00000000u);
    assert(sector == 0xFFFFFFFFu);

    sector = Fls_Hw_GetSectorNumber(TEST_FLS_BASE_ADDR + TEST_FLS_SIZE);
    assert(sector == 0xFFFFFFFFu);

    printf("PASS: test_Fls_Hw_GetSectorNumber_Invalid\n");
}

/**
 * @brief Test Fls_Hw_GetSectorSize
 */
void test_Fls_Hw_GetSectorSize(void)
{
    Fls_Hw_ConfigType config = {
        .flashBaseAddress = TEST_FLS_BASE_ADDR,
        .flashSize = TEST_FLS_SIZE,
        .sectorCount = 16u,
        .pageSize = 4u,
        .useInterrupts = FALSE,
        .timeoutMs = 1000u,
        .clockFreqHz = 16000000u
    };

    Fls_Hw_Init(&config);

    uint32 size = Fls_Hw_GetSectorSize(0u);
    assert(size == 0x10000u);  /* 64KB for generic */

    size = Fls_Hw_GetSectorSize(15u);
    assert(size == 0x10000u);

    size = Fls_Hw_GetSectorSize(16u);  /* Invalid */
    assert(size == 0u);

    printf("PASS: test_Fls_Hw_GetSectorSize\n");
}

/**
 * @brief Test Fls_Hw_Lock and Fls_Hw_Unlock
 */
void test_Fls_Hw_LockUnlock(void)
{
    Fls_Hw_ConfigType config = {
        .flashBaseAddress = TEST_FLS_BASE_ADDR,
        .flashSize = TEST_FLS_SIZE,
        .sectorCount = 16u,
        .pageSize = 4u,
        .useInterrupts = FALSE,
        .timeoutMs = 1000u,
        .clockFreqHz = 16000000u
    };

    Fls_Hw_Init(&config);

    Std_ReturnType result = Fls_Hw_Unlock();
    assert(result == E_OK);

    result = Fls_Hw_Lock();
    assert(result == E_OK);

    printf("PASS: test_Fls_Hw_LockUnlock\n");
}

/**
 * @brief Test Fls_Hw_ReadWord
 */
void test_Fls_Hw_ReadWord(void)
{
    Fls_Hw_ConfigType config = {
        .flashBaseAddress = TEST_FLS_BASE_ADDR,
        .flashSize = TEST_FLS_SIZE,
        .sectorCount = 16u,
        .pageSize = 4u,
        .useInterrupts = FALSE,
        .timeoutMs = 1000u,
        .clockFreqHz = 16000000u
    };

    Fls_Hw_Init(&config);

    uint32 data = 0u;
    Std_ReturnType result = Fls_Hw_ReadWord(TEST_FLS_BASE_ADDR, &data);

    assert(result == E_OK);
    printf("PASS: test_Fls_Hw_ReadWord\n");
}

/**
 * @brief Test Fls_Hw_ReadBuffer
 */
void test_Fls_Hw_ReadBuffer(void)
{
    Fls_Hw_ConfigType config = {
        .flashBaseAddress = TEST_FLS_BASE_ADDR,
        .flashSize = TEST_FLS_SIZE,
        .sectorCount = 16u,
        .pageSize = 4u,
        .useInterrupts = FALSE,
        .timeoutMs = 1000u,
        .clockFreqHz = 16000000u
    };

    Fls_Hw_Init(&config);

    uint8 buffer[16];
    Std_ReturnType result = Fls_Hw_ReadBuffer(TEST_FLS_BASE_ADDR, buffer, 16u);

    assert(result == E_OK);
    printf("PASS: test_Fls_Hw_ReadBuffer\n");
}

/**
 * @brief Test Fls_Hw_ClearFlags
 */
void test_Fls_Hw_ClearFlags(void)
{
    Fls_Hw_ConfigType config = {
        .flashBaseAddress = TEST_FLS_BASE_ADDR,
        .flashSize = TEST_FLS_SIZE,
        .sectorCount = 16u,
        .pageSize = 4u,
        .useInterrupts = FALSE,
        .timeoutMs = 1000u,
        .clockFreqHz = 16000000u
    };

    Fls_Hw_Init(&config);

    Fls_Hw_ClearFlags();

    assert(Fls_Hw_GetStatus() == FLS_HW_STATUS_IDLE);
    assert(Fls_Hw_GetLastError() == FLS_HW_ERROR_NONE);

    printf("PASS: test_Fls_Hw_ClearFlags\n");
}

/**
 * @brief Test Fls_Hw_Verify
 */
void test_Fls_Hw_Verify(void)
{
    Fls_Hw_ConfigType config = {
        .flashBaseAddress = TEST_FLS_BASE_ADDR,
        .flashSize = TEST_FLS_SIZE,
        .sectorCount = 16u,
        .pageSize = 4u,
        .useInterrupts = FALSE,
        .timeoutMs = 1000u,
        .clockFreqHz = 16000000u
    };

    Fls_Hw_Init(&config);

    /* Generic implementation will read from mock memory */
    uint8 verifyData[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    Std_ReturnType result = Fls_Hw_Verify(TEST_FLS_BASE_ADDR, verifyData, 16u);

    /* Should pass since mock memory is initialized to 0xFF (erased state) */
    assert(result == E_OK);

    printf("PASS: test_Fls_Hw_Verify\n");
}

/*==================================================================================================
 *                                    MAIN TEST RUNNER
 *==================================================================================================*/

int main(void)
{
    printf("========================================\n");
    printf("    FLS HW Unit Tests Starting...\n");
    printf("========================================\n\n");

    setUp();
    test_Fls_Hw_Init_ValidConfig();
    tearDown();

    setUp();
    test_Fls_Hw_Init_NullConfig();
    tearDown();

    setUp();
    test_Fls_Hw_DeInit();
    tearDown();

    setUp();
    test_Fls_Hw_GetSectorNumber_Valid();
    tearDown();

    setUp();
    test_Fls_Hw_GetSectorNumber_Invalid();
    tearDown();

    setUp();
    test_Fls_Hw_GetSectorSize();
    tearDown();

    setUp();
    test_Fls_Hw_LockUnlock();
    tearDown();

    setUp();
    test_Fls_Hw_ReadWord();
    tearDown();

    setUp();
    test_Fls_Hw_ReadBuffer();
    tearDown();

    setUp();
    test_Fls_Hw_ClearFlags();
    tearDown();

    setUp();
    test_Fls_Hw_Verify();
    tearDown();

    printf("\n========================================\n");
    printf("    All FLS HW Tests Passed!\n");
    printf("========================================\n");

    return 0;
}
