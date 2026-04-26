/**
 * @file Mcu_RamInit.c
 * @brief Mcu (Microcontroller Driver) RAM Initialization
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Mcu Module - RAM Initialization Implementation
 * Compliant with AUTOSAR R22-11 MCAL Specification
 * Module ID: 0x12
 * MISRA C:2012 compliant
 */

#include "mcal/mcu/Mcu.h"
#include "mcal/mcu/Mcu_Cfg.h"
#include <string.h>

/*============================================================================*
 * Memory Barriers
 *============================================================================*/

/**
 * @brief Data Memory Barrier
 *
 * Ensures all memory accesses are completed before continuing.
 */
static void Mcu_DataMemoryBarrier(void)
{
#if defined(__GNUC__)
    __asm__ volatile ("dsb" ::: "memory");
#elif defined(__ICCARM__)
    __DSB();
#else
    /* Generic implementation */
    volatile uint32_t dummy = 0U;
    (void)dummy;
#endif
}

/**
 * @brief Instruction Synchronization Barrier
 *
 * Ensures all instructions are completed before continuing.
 */
static void Mcu_InstructionSyncBarrier(void)
{
#if defined(__GNUC__)
    __asm__ volatile ("isb" ::: "memory");
#elif defined(__ICCARM__)
    __ISB();
#else
    /* Generic implementation */
    volatile uint32_t dummy = 0U;
    (void)dummy;
#endif
}

/*============================================================================*
 * ECC Support
 *============================================================================*/

/**
 * @brief Initialize RAM with ECC support
 *
 * Writes initial pattern to RAM with ECC calculation.
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @param pattern Pattern to write
 * @return MCU_OK on success, error code otherwise
 */
static Mcu_ErrorCode_t Mcu_InitRamEcc(
    uint32_t startAddr,
    uint32_t size,
    uint8_t pattern
)
{
    volatile uint32_t* ptr32;
    volatile uint8_t* ptr8;
    uint32_t remaining;
    uint32_t i;
    uint32_t pattern32;

    /* Validate parameters */
    if (startAddr == 0U) {
        return MCU_E_PARAM_RAM;
    }

    if (size == 0U) {
        return MCU_OK;
    }

    /* Build 32-bit pattern */
    pattern32 = ((uint32_t)pattern << 24U) |
                ((uint32_t)pattern << 16U) |
                ((uint32_t)pattern << 8U)  |
                (uint32_t)pattern;

    /* Align start address to 32-bit boundary */
    ptr32 = (volatile uint32_t*)(uintptr_t)(startAddr & ~0x3U);
    remaining = size;

    /* Handle unaligned start */
    if ((startAddr & 0x3U) != 0U) {
        uint32_t offset = startAddr & 0x3U;
        uint32_t firstBytes = 4U - offset;

        if (firstBytes > remaining) {
            firstBytes = remaining;
        }

        ptr8 = (volatile uint8_t*)(uintptr_t)startAddr;
        for (i = 0U; i < firstBytes; i++) {
            ptr8[i] = pattern;
        }

        ptr32 = (volatile uint32_t*)(uintptr_t)((startAddr + 3U) & ~0x3U);
        remaining -= firstBytes;
    }

    /* Write 32-bit words for speed */
    while (remaining >= 4U) {
        *ptr32 = pattern32;
        ptr32++;
        remaining -= 4U;

        /* Memory barrier every 256 bytes to prevent optimization issues */
        if ((((uintptr_t)ptr32) & 0xFFU) == 0U) {
            Mcu_DataMemoryBarrier();
        }
    }

    /* Handle remaining bytes */
    if (remaining > 0U) {
        ptr8 = (volatile uint8_t*)ptr32;
        for (i = 0U; i < remaining; i++) {
            ptr8[i] = pattern;
        }
    }

    Mcu_DataMemoryBarrier();
    Mcu_InstructionSyncBarrier();

    return MCU_OK;
}

/**
 * @brief Clear RAM (set to zero)
 *
 * Optimized zero initialization with ECC support.
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_ClearRam(uint32_t startAddr, uint32_t size)
{
    return Mcu_InitRamEcc(startAddr, size, 0U);
}

/**
 * @brief Initialize RAM with pattern
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @param pattern Pattern to write
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitRamPattern(
    uint32_t startAddr,
    uint32_t size,
    uint8_t pattern
)
{
    return Mcu_InitRamEcc(startAddr, size, pattern);
}

/*============================================================================*
 * Section-Specific Initialization
 *============================================================================*/

/**
 * @brief Initialize .data section
 *
 * Copies initialized data from flash to RAM.
 *
 * @param ramStart RAM start address
 * @param romStart ROM start address (flash)
 * @param size Size in bytes
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitDataSection(
    uint32_t ramStart,
    uint32_t romStart,
    uint32_t size
)
{
    volatile uint8_t* dest;
    const uint8_t* src;
    uint32_t i;

    if (size == 0U) {
        return MCU_OK;
    }

    if ((ramStart == 0U) || (romStart == 0U)) {
        return MCU_E_PARAM_RAM;
    }

    dest = (volatile uint8_t*)(uintptr_t)ramStart;
    src = (const uint8_t*)(uintptr_t)romStart;

    /* Copy data from ROM to RAM */
    for (i = 0U; i < size; i++) {
        dest[i] = src[i];
    }

    Mcu_DataMemoryBarrier();

    return MCU_OK;
}

/**
 * @brief Initialize .bss section
 *
 * Clears BSS section to zero.
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitBssSection(uint32_t startAddr, uint32_t size)
{
    return Mcu_ClearRam(startAddr, size);
}

/**
 * @brief Initialize stack section
 *
 * Initializes stack with known pattern for overflow detection.
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitStackSection(uint32_t startAddr, uint32_t size)
{
    /* Initialize with 0xCC pattern for stack overflow detection */
    return Mcu_InitRamEcc(startAddr, size, 0xCCU);
}

/**
 * @brief Initialize heap section
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitHeapSection(uint32_t startAddr, uint32_t size)
{
    /* Initialize with 0xAA pattern for heap debugging */
    return Mcu_InitRamEcc(startAddr, size, 0xAAU);
}

/*============================================================================*
 * TCM (Tightly Coupled Memory) Initialization
 *============================================================================*/

/**
 * @brief Initialize Instruction TCM (ITCM)
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitITCM(uint32_t startAddr, uint32_t size)
{
    /* ITCM typically doesn't need ECC, just clear to zero */
    return Mcu_ClearRam(startAddr, size);
}

/**
 * @brief Initialize Data TCM (DTCM)
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitDTCM(uint32_t startAddr, uint32_t size)
{
    /* DTCM may have ECC */
    return Mcu_InitRamEcc(startAddr, size, 0U);
}

/*============================================================================*
 * Cache Initialization
 *============================================================================*/

/**
 * @brief Initialize cache memory
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitCache(uint32_t startAddr, uint32_t size)
{
    /* Cache initialization is typically hardware-managed */
    (void)startAddr;
    (void)size;
    return MCU_OK;
}

/**
 * @brief Invalidate cache
 *
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InvalidateCache(void)
{
    /* Platform-specific cache invalidation */
    Mcu_DataMemoryBarrier();
    Mcu_InstructionSyncBarrier();
    return MCU_OK;
}

/**
 * @brief Clean cache
 *
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_CleanCache(void)
{
    /* Platform-specific cache clean */
    Mcu_DataMemoryBarrier();
    return MCU_OK;
}

/**
 * @brief Clean and invalidate cache
 *
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_CleanInvalidateCache(void)
{
    (void)Mcu_CleanCache();
    return Mcu_InvalidateCache();
}

/*============================================================================*
 * Retention RAM
 *============================================================================*/

/**
 * @brief Initialize retention RAM
 *
 * Only initializes if explicitly requested (retains data across resets).
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @param forceInit Force initialization even if retention is enabled
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_InitRetentionRam(
    uint32_t startAddr,
    uint32_t size,
    bool forceInit
)
{
    if (forceInit) {
        return Mcu_InitRamEcc(startAddr, size, 0U);
    }
    /* Skip initialization to retain data */
    return MCU_OK;
}

/**
 * @brief Check if retention RAM is valid
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @param magic Magic value to check
 * @return true if valid, false otherwise
 */
bool Mcu_IsRetentionRamValid(uint32_t startAddr, uint32_t size, uint32_t magic)
{
    volatile uint32_t* ptr;

    (void)size;

    if (startAddr == 0U) {
        return false;
    }

    ptr = (volatile uint32_t*)(uintptr_t)startAddr;
    return (*ptr == magic);
}

/**
 * @brief Mark retention RAM as valid
 *
 * @param startAddr Start address
 * @param magic Magic value to write
 */
void Mcu_MarkRetentionRamValid(uint32_t startAddr, uint32_t magic)
{
    volatile uint32_t* ptr;

    if (startAddr == 0U) {
        return;
    }

    ptr = (volatile uint32_t*)(uintptr_t)startAddr;
    *ptr = magic;
    Mcu_DataMemoryBarrier();
}

/*============================================================================*
 * Memory Testing
 *============================================================================*/

/**
 * @brief Simple RAM walk test
 *
 * Writes and verifies walking 1 and walking 0 patterns.
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_TestRamWalk(uint32_t startAddr, uint32_t size)
{
    volatile uint32_t* ptr;
    uint32_t numWords;
    uint32_t i;
    uint32_t testVal;
    uint32_t readBack;

    if ((startAddr == 0U) || (size == 0U)) {
        return MCU_E_PARAM_RAM;
    }

    /* Align to 32-bit boundary */
    ptr = (volatile uint32_t*)(uintptr_t)startAddr;
    numWords = size / 4U;

    /* Walking 1 test */
    for (i = 0U; i < numWords; i++) {
        testVal = 1U << (i % 32U);
        ptr[i] = testVal;
        Mcu_DataMemoryBarrier();
        readBack = ptr[i];
        if (readBack != testVal) {
            return MCU_E_HW_ERROR;
        }
    }

    /* Walking 0 test */
    for (i = 0U; i < numWords; i++) {
        testVal = ~(1U << (i % 32U));
        ptr[i] = testVal;
        Mcu_DataMemoryBarrier();
        readBack = ptr[i];
        if (readBack != testVal) {
            return MCU_E_HW_ERROR;
        }
    }

    return MCU_OK;
}

/**
 * @brief March RAM test
 *
 * Performs March C- test pattern.
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_TestRamMarch(uint32_t startAddr, uint32_t size)
{
    volatile uint32_t* ptr;
    uint32_t numWords;
    uint32_t i;

    if ((startAddr == 0U) || (size == 0U)) {
        return MCU_E_PARAM_RAM;
    }

    ptr = (volatile uint32_t*)(uintptr_t)startAddr;
    numWords = size / 4U;

    /* March 1: Write 0s (up) */
    for (i = 0U; i < numWords; i++) {
        ptr[i] = 0x00000000U;
    }
    Mcu_DataMemoryBarrier();

    /* March 2: Read 0, write 1 (up) */
    for (i = 0U; i < numWords; i++) {
        if (ptr[i] != 0x00000000U) {
            return MCU_E_HW_ERROR;
        }
        ptr[i] = 0xFFFFFFFFU;
    }
    Mcu_DataMemoryBarrier();

    /* March 3: Read 1, write 0 (up) */
    for (i = 0U; i < numWords; i++) {
        if (ptr[i] != 0xFFFFFFFFU) {
            return MCU_E_HW_ERROR;
        }
        ptr[i] = 0x00000000U;
    }
    Mcu_DataMemoryBarrier();

    /* March 4: Read 0, write 1 (down) */
    for (i = numWords; i > 0U; i--) {
        if (ptr[i - 1U] != 0x00000000U) {
            return MCU_E_HW_ERROR;
        }
        ptr[i - 1U] = 0xFFFFFFFFU;
    }
    Mcu_DataMemoryBarrier();

    /* March 5: Read 1, write 0 (down) */
    for (i = numWords; i > 0U; i--) {
        if (ptr[i - 1U] != 0xFFFFFFFFU) {
            return MCU_E_HW_ERROR;
        }
        ptr[i - 1U] = 0x00000000U;
    }
    Mcu_DataMemoryBarrier();

    /* March 6: Read 0 (up) */
    for (i = 0U; i < numWords; i++) {
        if (ptr[i] != 0x00000000U) {
            return MCU_E_HW_ERROR;
        }
    }

    return MCU_OK;
}

/**
 * @brief Quick RAM test (checkerboard pattern)
 *
 * @param startAddr Start address
 * @param size Size in bytes
 * @return MCU_OK on success, error code otherwise
 */
Mcu_ErrorCode_t Mcu_TestRamQuick(uint32_t startAddr, uint32_t size)
{
    volatile uint32_t* ptr;
    uint32_t numWords;
    uint32_t i;
    const uint32_t pattern1 = 0x55555555U;
    const uint32_t pattern2 = 0xAAAAAAAAU;

    if ((startAddr == 0U) || (size == 0U)) {
        return MCU_E_PARAM_RAM;
    }

    ptr = (volatile uint32_t*)(uintptr_t)startAddr;
    numWords = size / 4U;

    /* Write checkerboard */
    for (i = 0U; i < numWords; i++) {
        ptr[i] = (i % 2U == 0U) ? pattern1 : pattern2;
    }
    Mcu_DataMemoryBarrier();

    /* Verify */
    for (i = 0U; i < numWords; i++) {
        uint32_t expected = (i % 2U == 0U) ? pattern1 : pattern2;
        if (ptr[i] != expected) {
            return MCU_E_HW_ERROR;
        }
    }

    return MCU_OK;
}

/*============================================================================*
 * Startup Initialization
 *============================================================================*/

/**
 * @brief Early RAM initialization (pre-main)
 *
 * This function is called before main() to initialize critical RAM sections.
 * It assumes minimal hardware setup (clock may not be fully configured).
 */
void Mcu_EarlyRamInit(void)
{
    /* Initialize .bss section to zero */
    extern uint32_t __bss_start__;
    extern uint32_t __bss_end__;
    uintptr_t bssStart = (uintptr_t)&__bss_start__;
    uintptr_t bssSize = (uintptr_t)&__bss_end__ - bssStart;

    if (bssSize > 0U) {
        (void)Mcu_InitBssSection((uint32_t)bssStart, (uint32_t)bssSize);
    }

    /* Initialize .data section from flash */
    extern uint32_t __data_start__;
    extern uint32_t __data_end__;
    extern uint32_t __data_load__;
    uintptr_t dataStart = (uintptr_t)&__data_start__;
    uintptr_t dataSize = (uintptr_t)&__data_end__ - dataStart;
    uintptr_t dataLoad = (uintptr_t)&__data_load__;

    if (dataSize > 0U) {
        (void)Mcu_InitDataSection((uint32_t)dataStart, (uint32_t)dataLoad, (uint32_t)dataSize);
    }

    /* Initialize stack with pattern for overflow detection */
    extern uint32_t __stack_bottom__;
    extern uint32_t __stack_top__;
    uintptr_t stackBottom = (uintptr_t)&__stack_bottom__;
    uintptr_t stackSize = (uintptr_t)&__stack_top__ - stackBottom;

    if (stackSize > 0U) {
        (void)Mcu_InitStackSection((uint32_t)stackBottom, (uint32_t)stackSize);
    }
}

/*============================================================================*
 * Stack Monitoring
 *============================================================================*/

/**
 * @brief Get used stack size
 *
 * Scans stack to find lowest used address based on initialization pattern.
 *
 * @return Used stack size in bytes
 */
uint32_t Mcu_GetUsedStackSize(void)
{
    extern uint32_t __stack_bottom__;
    extern uint32_t __stack_top__;
    volatile uint8_t* stackBottom = (volatile uint8_t*)&__stack_bottom__;
    volatile uint8_t* stackTop = (volatile uint8_t*)&__stack_top__;
    uint32_t stackSize = (uint32_t)(stackTop - stackBottom);
    uint32_t used = 0U;
    uint32_t i;

    /* Scan for first non-0xCC byte (from bottom up) */
    for (i = 0U; i < stackSize; i++) {
        if (stackBottom[i] != 0xCCU) {
            used = stackSize - i;
            break;
        }
    }

    return used;
}

/**
 * @brief Get available stack size
 *
 * @return Available stack size in bytes
 */
uint32_t Mcu_GetAvailableStackSize(void)
{
    extern uint32_t __stack_bottom__;
    extern uint32_t __stack_top__;
    uintptr_t stackSize = (uintptr_t)&__stack_top__ - (uintptr_t)&__stack_bottom__;
    return (uint32_t)stackSize - Mcu_GetUsedStackSize();
}

/**
 * @brief Check for stack overflow
 *
 * @return true if stack overflow detected, false otherwise
 */
bool Mcu_IsStackOverflow(void)
{
    extern uint32_t __stack_bottom__;
    volatile uint8_t* stackBottom = (volatile uint8_t*)&__stack_bottom__;

    /* Check if bottom of stack has been overwritten */
    return (stackBottom[0] != 0xCCU);
}
