/**
 * @file fls_hw_emulator.c
 * @brief Fls Hardware Emulator for Testing
 * @version 1.0.0
 * @date 2026
 *
 * Software-based flash emulator for unit testing and development.
 * Simulates flash behavior including:
 * - Page-based read/write
 * - Sector erase (all 0xFF)
 * - Write-before-erase protection
 * - Wear leveling counters
 */

#include "mcal/fls/fls.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*============================================================================*
 * Emulator Configuration
 *============================================================================*/
#define FLS_EMU_FLASH_SIZE      (256u * 1024u)  /* 256KB emulated flash */
#define FLS_EMU_SECTOR_SIZE     65536u          /* 64KB sectors */
#define FLS_EMU_PAGE_SIZE       256u            /* 256B pages */
#define FLS_EMU_NUM_SECTORS     (FLS_EMU_FLASH_SIZE / FLS_EMU_SECTOR_SIZE)
#define FLS_EMU_ERASED_VALUE    0xFFu

/*============================================================================*
 * Static Variables
 *============================================================================*/
static uint8_t* gFls_EmuFlashMemory = NULL;
static uint32_t gFls_EmuEraseCount[FLS_EMU_NUM_SECTORS];
static Fls_ProtectionType_t gFls_EmuProtection[FLS_EMU_NUM_SECTORS];
static bool gFls_EmuInitialized = false;
static Fls_ModuleStatus_t gFls_EmuStatus = FLS_UNINIT;

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

static uint32_t Fls_Emu_GetSectorIndex(uint32_t address)
{
    return address / FLS_EMU_SECTOR_SIZE;
}

static bool Fls_Emu_IsAddressValid(uint32_t address)
{
    return (address < FLS_EMU_FLASH_SIZE);
}

static bool Fls_Emu_IsRangeValid(uint32_t address, uint32_t length)
{
    if (length == 0u) {
        return false;
    }
    return ((address < FLS_EMU_FLASH_SIZE) &&
            ((address + length) <= FLS_EMU_FLASH_SIZE));
}

static Fls_ErrorCode_t Fls_Emu_CheckWriteAllowed(
    uint32_t address,
    uint32_t length,
    const uint8_t* data
)
{
    uint32_t i;
    uint32_t sector;

    /* Check if already programmed bits would need to be cleared */
    for (i = 0u; i < length; i++) {
        uint8_t flashByte = gFls_EmuFlashMemory[address + i];
        uint8_t newByte = data[i];

        /* Can only change 1s to 0s without erase */
        if ((flashByte & newByte) != newByte) {
            return FLS_E_WRITE_FAILED; /* Would need erase first */
        }
    }

    /* Check protection */
    sector = Fls_Emu_GetSectorIndex(address);
    if ((gFls_EmuProtection[sector] & FLS_PROTECTION_WRITE) != 0u) {
        return FLS_E_WRITE_PROTECTED;
    }

    return FLS_OK;
}

/*============================================================================*
 * Hardware Interface Implementation
 *============================================================================*/

static Fls_ErrorCode_t Fls_Emu_Init(const Fls_ConfigType* config)
{
    uint32_t i;

    (void)config; /* Unused */

    if (gFls_EmuInitialized) {
        return FLS_E_ALREADY_INITIALIZED;
    }

    /* Allocate emulated flash memory */
    gFls_EmuFlashMemory = (uint8_t*)malloc(FLS_EMU_FLASH_SIZE);
    if (gFls_EmuFlashMemory == NULL) {
        return FLS_E_NOT_OK;
    }

    /* Initialize to erased state (0xFF) */
    (void)memset(gFls_EmuFlashMemory, FLS_EMU_ERASED_VALUE, FLS_EMU_FLASH_SIZE);

    /* Initialize counters and protection */
    for (i = 0u; i < FLS_EMU_NUM_SECTORS; i++) {
        gFls_EmuEraseCount[i] = 0u;
        gFls_EmuProtection[i] = FLS_PROTECTION_NONE;
    }

    gFls_EmuInitialized = true;
    gFls_EmuStatus = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Emu_Deinit(void)
{
    if (!gFls_EmuInitialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_EmuFlashMemory != NULL) {
        free(gFls_EmuFlashMemory);
        gFls_EmuFlashMemory = NULL;
    }

    gFls_EmuInitialized = false;
    gFls_EmuStatus = FLS_UNINIT;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Emu_Read(
    uint32_t address,
    uint8_t* data,
    uint32_t length
)
{
    if (!gFls_EmuInitialized) {
        return FLS_E_UNINIT;
    }

    if (data == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_Emu_IsRangeValid(address, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    /* Check read protection */
    uint32_t sector = Fls_Emu_GetSectorIndex(address);
    if ((gFls_EmuProtection[sector] & FLS_PROTECTION_READ) != 0u) {
        return FLS_E_WRITE_PROTECTED;
    }

    gFls_EmuStatus = FLS_BUSY;

    (void)memcpy(data, &gFls_EmuFlashMemory[address], length);

    gFls_EmuStatus = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Emu_Write(
    uint32_t address,
    const uint8_t* data,
    uint32_t length
)
{
    Fls_ErrorCode_t result;
    uint32_t i;

    if (!gFls_EmuInitialized) {
        return FLS_E_UNINIT;
    }

    if (data == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_Emu_IsRangeValid(address, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    result = Fls_Emu_CheckWriteAllowed(address, length, data);
    if (result != FLS_OK) {
        return result;
    }

    gFls_EmuStatus = FLS_BUSY;

    /* Program bytes (only change 1s to 0s) */
    for (i = 0u; i < length; i++) {
        gFls_EmuFlashMemory[address + i] &= data[i];
    }

    gFls_EmuStatus = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Emu_Erase(uint32_t sectorIndex, uint32_t sectorCount)
{
    uint32_t i;
    uint32_t address;
    uint32_t length;

    if (!gFls_EmuInitialized) {
        return FLS_E_UNINIT;
    }

    if ((sectorIndex >= FLS_EMU_NUM_SECTORS) ||
        ((sectorIndex + sectorCount) > FLS_EMU_NUM_SECTORS)) {
        return FLS_E_INVALID_ADDRESS;
    }

    gFls_EmuStatus = FLS_BUSY;

    for (i = 0u; i < sectorCount; i++) {
        /* Check erase protection */
        if ((gFls_EmuProtection[sectorIndex + i] & FLS_PROTECTION_ERASE) != 0u) {
            gFls_EmuStatus = FLS_IDLE;
            return FLS_E_WRITE_PROTECTED;
        }
    }

    /* Erase sectors */
    for (i = 0u; i < sectorCount; i++) {
        address = (sectorIndex + i) * FLS_EMU_SECTOR_SIZE;
        length = FLS_EMU_SECTOR_SIZE;

        (void)memset(&gFls_EmuFlashMemory[address], FLS_EMU_ERASED_VALUE, length);
        gFls_EmuEraseCount[sectorIndex + i]++;
    }

    gFls_EmuStatus = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Emu_Compare(
    uint32_t address,
    const uint8_t* data,
    uint32_t length
)
{
    uint32_t i;

    if (!gFls_EmuInitialized) {
        return FLS_E_UNINIT;
    }

    if (data == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_Emu_IsRangeValid(address, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    gFls_EmuStatus = FLS_BUSY;

    for (i = 0u; i < length; i++) {
        if (gFls_EmuFlashMemory[address + i] != data[i]) {
            gFls_EmuStatus = FLS_IDLE;
            return FLS_E_COMPARE_FAILED;
        }
    }

    gFls_EmuStatus = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Emu_BlankCheck(uint32_t address, uint32_t length)
{
    uint32_t i;

    if (!gFls_EmuInitialized) {
        return FLS_E_UNINIT;
    }

    if (!Fls_Emu_IsRangeValid(address, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    gFls_EmuStatus = FLS_BUSY;

    for (i = 0u; i < length; i++) {
        if (gFls_EmuFlashMemory[address + i] != FLS_EMU_ERASED_VALUE) {
            gFls_EmuStatus = FLS_IDLE;
            return FLS_E_COMPARE_FAILED;
        }
    }

    gFls_EmuStatus = FLS_IDLE;

    return FLS_OK;
}

static Fls_ModuleStatus_t Fls_Emu_GetStatus(void)
{
    return gFls_EmuStatus;
}

static bool Fls_Emu_IsBusy(void)
{
    return (gFls_EmuStatus == FLS_BUSY);
}

static uint32_t Fls_Emu_GetSectorSize(uint32_t sectorIndex)
{
    if (sectorIndex >= FLS_EMU_NUM_SECTORS) {
        return 0u;
    }
    return FLS_EMU_SECTOR_SIZE;
}

static uint32_t Fls_Emu_GetPageSize(uint32_t sectorIndex)
{
    (void)sectorIndex;
    return FLS_EMU_PAGE_SIZE;
}

static Fls_ErrorCode_t Fls_Emu_SetProtection(
    uint32_t sectorIndex,
    Fls_ProtectionType_t protection
)
{
    if (sectorIndex >= FLS_EMU_NUM_SECTORS) {
        return FLS_E_INVALID_ADDRESS;
    }

    gFls_EmuProtection[sectorIndex] = protection;

    return FLS_OK;
}

static uint32_t Fls_Emu_GetEraseCount(uint32_t sectorIndex)
{
    if (sectorIndex >= FLS_EMU_NUM_SECTORS) {
        return 0u;
    }

    return gFls_EmuEraseCount[sectorIndex];
}

static Fls_ErrorCode_t Fls_Emu_Lock(uint32_t sectorIndex)
{
    return Fls_Emu_SetProtection(sectorIndex, FLS_PROTECTION_WRITE);
}

static Fls_ErrorCode_t Fls_Emu_Unlock(uint32_t sectorIndex)
{
    return Fls_Emu_SetProtection(sectorIndex, FLS_PROTECTION_NONE);
}

/*============================================================================*
 * Public Interface
 *============================================================================*/

static Fls_HwInterfaceType gFls_EmuHwInterface = {
    .Init = Fls_Emu_Init,
    .Deinit = Fls_Emu_Deinit,
    .Read = Fls_Emu_Read,
    .Write = Fls_Emu_Write,
    .Erase = Fls_Emu_Erase,
    .Compare = Fls_Emu_Compare,
    .BlankCheck = Fls_Emu_BlankCheck,
    .GetStatus = Fls_Emu_GetStatus,
    .IsBusy = Fls_Emu_IsBusy,
    .GetSectorIndex = Fls_Emu_GetSectorIndex,
    .GetSectorSize = Fls_Emu_GetSectorSize,
    .GetPageSize = Fls_Emu_GetPageSize,
    .SetProtection = Fls_Emu_SetProtection,
    .GetEraseCount = Fls_Emu_GetEraseCount,
    .Lock = Fls_Emu_Lock,
    .Unlock = Fls_Emu_Unlock
};

Fls_ErrorCode_t Fls_Emu_Register(void)
{
    return Fls_RegisterHwInterface(&gFls_EmuHwInterface);
}

Fls_ErrorCode_t Fls_Emu_Dump(uint32_t address, uint32_t length)
{
    uint32_t i;
    uint32_t j;

    if (!gFls_EmuInitialized) {
        return FLS_E_UNINIT;
    }

    if (!Fls_Emu_IsRangeValid(address, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    printf("Flash Memory Dump (0x%08X - 0x%08X):\n",
           (unsigned int)address, (unsigned int)(address + length - 1));
    printf("--------------------------------------------------\n");

    for (i = 0u; i < length; i += 16u) {
        printf("%08X: ", (unsigned int)(address + i));

        for (j = 0u; (j < 16u) && ((i + j) < length); j++) {
            printf("%02X ", gFls_EmuFlashMemory[address + i + j]);
        }

        printf(" |");

        for (j = 0u; (j < 16u) && ((i + j) < length); j++) {
            uint8_t c = gFls_EmuFlashMemory[address + i + j];
            if ((c >= 32u) && (c < 127u)) {
                printf("%c", c);
            } else {
                printf(".");
            }
        }

        printf("|\n");
    }

    printf("--------------------------------------------------\n");

    return FLS_OK;
}

Fls_ErrorCode_t Fls_Emu_GetStats(
    uint32_t* totalReads,
    uint32_t* totalWrites,
    uint32_t* totalErases,
    uint32_t* maxEraseCount
)
{
    uint32_t i;
    uint32_t maxCount = 0u;

    if (!gFls_EmuInitialized) {
        return FLS_E_UNINIT;
    }

    if ((totalReads == NULL) || (totalWrites == NULL) ||
        (totalErases == NULL) || (maxEraseCount == NULL)) {
        return FLS_E_PARAM_POINTER;
    }

    *totalReads = 0u;  /* Not tracked in emulator */
    *totalWrites = 0u; /* Not tracked in emulator */
    *totalErases = 0u;

    for (i = 0u; i < FLS_EMU_NUM_SECTORS; i++) {
        *totalErases += gFls_EmuEraseCount[i];
        if (gFls_EmuEraseCount[i] > maxCount) {
            maxCount = gFls_EmuEraseCount[i];
        }
    }

    *maxEraseCount = maxCount;

    return FLS_OK;
}

uint8_t* Fls_Emu_GetMemoryPtr(void)
{
    return gFls_EmuFlashMemory;
}

uint32_t Fls_Emu_GetFlashSize(void)
{
    return FLS_EMU_FLASH_SIZE;
}
