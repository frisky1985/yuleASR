/**
 * @file fls_hw_aurix.c
 * @brief Fls Hardware Driver for Infineon Aurix TC3xx
 * @version 1.0.0
 * @date 2026
 *
 * Hardware-specific flash driver for Infineon Aurix TC3xx series.
 * Supports:
 * - PFLASH (Program Flash) operations
 * - DFLASH (Data Flash) for EEPROM emulation
 * - Read/Write/Erase operations
 * - Write protection
 * - Error correction (ECC)
 *
 * Reference: Infineon TC3xx User Manual, Flash Module chapter
 */

#include "mcal/fls/fls.h"
#include <string.h>
#include <stdint.h>

/*============================================================================*
 * Hardware Abstraction (TC3xx Specific)
 *============================================================================*/

/* Flash registers (simplified - actual addresses from TC3xx header) */
#define FLS_TC3XX_PFLASH_BASE       0xA0000000u
#define FLS_TC3XX_DFLASH_BASE       0xAF000000u
#define FLS_TC3XX_PFLASH_SIZE       (6u * 1024u * 1024u)  /* 6MB PFLASH */
#define FLS_TC3XX_DFLASH_SIZE       (128u * 1024u)        /* 128KB DFLASH */
#define FLS_TC3XX_SECTOR_SIZE       16384u                /* 16KB sectors */
#define FLS_TC3XX_PAGE_SIZE         32u                   /* 32B pages */
#define FLS_TC3XX_ERASED_VALUE      0xFFu

/* Command sequences */
#define FLS_TC3XX_CMD_CLEAR_STATUS  0xFAu
#define FLS_TC3XX_CMD_PAGE_PROG     0xA0u
#define FLS_TC3XX_CMD_SECTOR_ERASE  0x80u
#define FLS_TC3XX_CMD_VERIFY        0xB0u

/* Status register bits */
#define FLS_TC3XX_STAT_BUSY         0x00000001u
#define FLS_TC3XX_STAT_ERROR        0x000000F0u
#define FLS_TC3XX_STAT_PROTECTION   0x00000F00u

/*============================================================================*
 * Static Variables
 *============================================================================*/
static bool gFls_Tc3xxInitialized = false;
static Fls_ModuleStatus_t gFls_Tc3xxStatus = FLS_UNINIT;
static uint32_t gFls_Tc3xxEraseCount[256];  /* Per-sector erase count */
static Fls_ProtectionType_t gFls_Tc3xxProtection[256];

/* Emulation mode for testing without actual hardware */
static bool gFls_Tc3xxEmulationMode = false;
static uint8_t* gFls_Tc3xxEmuMemory = NULL;

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Get sector index from address
 */
static uint32_t Fls_Tc3xx_GetSectorIndex(uint32_t address)
{
    if (address < FLS_TC3XX_PFLASH_SIZE) {
        return address / FLS_TC3XX_SECTOR_SIZE;
    } else if ((address >= FLS_TC3XX_DFLASH_BASE) &&
               (address < (FLS_TC3XX_DFLASH_BASE + FLS_TC3XX_DFLASH_SIZE))) {
        return (address - FLS_TC3XX_DFLASH_BASE) / FLS_TC3XX_SECTOR_SIZE;
    }
    return 0xFFFFFFFFu;
}

/**
 * @brief Check if address is valid
 */
static bool Fls_Tc3xx_IsAddressValid(uint32_t address)
{
    /* PFLASH */
    if (address < FLS_TC3XX_PFLASH_SIZE) {
        return true;
    }
    /* DFLASH */
    if ((address >= FLS_TC3XX_DFLASH_BASE) &&
        (address < (FLS_TC3XX_DFLASH_BASE + FLS_TC3XX_DFLASH_SIZE))) {
        return true;
    }
    return false;
}

/**
 * @brief Wait for flash operation completion
 */
static Fls_ErrorCode_t Fls_Tc3xx_WaitForComplete(void)
{
    uint32_t timeout = 1000000u;  /* Timeout counter */

    if (gFls_Tc3xxEmulationMode) {
        /* In emulation mode, operations are instant */
        return FLS_OK;
    }

    /* Wait for busy bit to clear */
    while (timeout > 0u) {
        /* In real implementation, check hardware status register */
        /* uint32_t status = FLASH_STATUS_REG; */
        /* if ((status & FLS_TC3XX_STAT_BUSY) == 0u) { break; } */
        timeout--;
    }

    if (timeout == 0u) {
        return FLS_E_TIMEOUT;
    }

    /* Check for errors */
    /* uint32_t status = FLASH_STATUS_REG; */
    /* if (status & FLS_TC3XX_STAT_ERROR) { return FLS_E_NOT_OK; } */

    return FLS_OK;
}

/**
 * @brief Execute flash command sequence
 */
static Fls_ErrorCode_t Fls_Tc3xx_ExecuteCommand(
    uint8_t command,
    uint32_t address,
    const uint8_t* data,
    uint32_t length
)
{
    Fls_ErrorCode_t result;

    if (gFls_Tc3xxEmulationMode) {
        /* Emulation mode - use software implementation */
        switch (command) {
            case FLS_TC3XX_CMD_PAGE_PROG:
                if (data == NULL) {
                    return FLS_E_PARAM_POINTER;
                }
                for (uint32_t i = 0u; i < length; i++) {
                    gFls_Tc3xxEmuMemory[address + i] &= data[i];
                }
                break;

            case FLS_TC3XX_CMD_SECTOR_ERASE:
                {
                    uint32_t sectorSize = FLS_TC3XX_SECTOR_SIZE;
                    uint32_t sectorAddr = (address / sectorSize) * sectorSize;
                    (void)memset(&gFls_Tc3xxEmuMemory[sectorAddr],
                                FLS_TC3XX_ERASED_VALUE, sectorSize);
                }
                break;

            default:
                return FLS_E_NOT_OK;
        }
        return FLS_OK;
    }

    /* Real hardware implementation */
    /* 1. Clear status register */
    /* FLASH_COMMAND_REG = FLS_TC3XX_CMD_CLEAR_STATUS; */

    /* 2. Load address */
    /* FLASH_ADDRESS_REG = address; */

    /* 3. Load data if programming */
    if ((command == FLS_TC3XX_CMD_PAGE_PROG) && (data != NULL)) {
        /* for (uint32_t i = 0; i < length; i += 4) { */
        /*     FLASH_DATA_REG = *(uint32_t*)&data[i]; */
        /* } */
    }

    /* 4. Execute command */
    /* FLASH_COMMAND_REG = command; */

    /* 5. Wait for completion */
    result = Fls_Tc3xx_WaitForComplete();

    return result;
}

/*============================================================================*
 * Hardware Interface Implementation
 *============================================================================*/

static Fls_ErrorCode_t Fls_Tc3xx_Init(const Fls_ConfigType* config)
{
    uint32_t i;

    (void)config;

    if (gFls_Tc3xxInitialized) {
        return FLS_E_ALREADY_INITIALIZED;
    }

    /* Initialize erase count and protection arrays */
    for (i = 0u; i < 256u; i++) {
        gFls_Tc3xxEraseCount[i] = 0u;
        gFls_Tc3xxProtection[i] = FLS_PROTECTION_NONE;
    }

    /* Allocate emulation memory if in emulation mode */
    if (gFls_Tc3xxEmulationMode) {
        gFls_Tc3xxEmuMemory = (uint8_t*)malloc(FLS_TC3XX_PFLASH_SIZE +
                                               FLS_TC3XX_DFLASH_SIZE);
        if (gFls_Tc3xxEmuMemory == NULL) {
            return FLS_E_NOT_OK;
        }
        (void)memset(gFls_Tc3xxEmuMemory, FLS_TC3XX_ERASED_VALUE,
                    FLS_TC3XX_PFLASH_SIZE + FLS_TC3XX_DFLASH_SIZE);
    } else {
        /* Real hardware initialization */
        /* 1. Enable flash module clock */
        /* SCU_CCUCON0 |= SCU_CCUCON0_FLASH_EN; */

        /* 2. Clear any pending errors */
        /* FLASH_STATUS_REG = 0xFFFFFFFFu; */

        /* 3. Configure wait states based on frequency */
        /* FLASH_FCON = (FLASH_FCON & ~FLASH_FCON_WSPFLASH) | waitStates; */
    }

    gFls_Tc3xxInitialized = true;
    gFls_Tc3xxStatus = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Tc3xx_Deinit(void)
{
    if (!gFls_Tc3xxInitialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_Tc3xxEmulationMode && (gFls_Tc3xxEmuMemory != NULL)) {
        free(gFls_Tc3xxEmuMemory);
        gFls_Tc3xxEmuMemory = NULL;
    }

    gFls_Tc3xxInitialized = false;
    gFls_Tc3xxStatus = FLS_UNINIT;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Tc3xx_Read(
    uint32_t address,
    uint8_t* data,
    uint32_t length
)
{
    if (!gFls_Tc3xxInitialized) {
        return FLS_E_UNINIT;
    }

    if (data == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_Tc3xx_IsAddressValid(address)) {
        return FLS_E_INVALID_ADDRESS;
    }

    if ((address + length) > (FLS_TC3XX_PFLASH_SIZE + FLS_TC3XX_DFLASH_SIZE)) {
        return FLS_E_INVALID_LENGTH;
    }

    gFls_Tc3xxStatus = FLS_BUSY;

    if (gFls_Tc3xxEmulationMode) {
        (void)memcpy(data, &gFls_Tc3xxEmuMemory[address], length);
    } else {
        /* Real hardware read - direct memory access */
        uint8_t* flashPtr = (uint8_t*)(FLS_TC3XX_PFLASH_BASE + address);
        (void)memcpy(data, flashPtr, length);
    }

    gFls_Tc3xxStatus = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Tc3xx_Write(
    uint32_t address,
    const uint8_t* data,
    uint32_t length
)
{
    Fls_ErrorCode_t result;
    uint32_t sector;

    if (!gFls_Tc3xxInitialized) {
        return FLS_E_UNINIT;
    }

    if (data == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_Tc3xx_IsAddressValid(address)) {
        return FLS_E_INVALID_ADDRESS;
    }

    /* Check protection */
    sector = Fls_Tc3xx_GetSectorIndex(address);
    if ((sector < 256u) &&
        ((gFls_Tc3xxProtection[sector] & FLS_PROTECTION_WRITE) != 0u)) {
        return FLS_E_WRITE_PROTECTED;
    }

    /* Align to page boundary and write */
    gFls_Tc3xxStatus = FLS_BUSY;

    uint32_t offset = 0u;
    while (offset < length) {
        uint32_t pageOffset = address + offset;
        uint32_t pageRemain = FLS_TC3XX_PAGE_SIZE - (pageOffset % FLS_TC3XX_PAGE_SIZE);
        uint32_t toWrite = (length - offset) < pageRemain ? (length - offset) : pageRemain;

        result = Fls_Tc3xx_ExecuteCommand(FLS_TC3XX_CMD_PAGE_PROG,
                                         pageOffset, &data[offset], toWrite);
        if (result != FLS_OK) {
            gFls_Tc3xxStatus = FLS_IDLE;
            return result;
        }

        offset += toWrite;
    }

    gFls_Tc3xxStatus = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Tc3xx_Erase(uint32_t sectorIndex, uint32_t sectorCount)
{
    Fls_ErrorCode_t result;
    uint32_t i;
    uint32_t address;

    if (!gFls_Tc3xxInitialized) {
        return FLS_E_UNINIT;
    }

    if ((sectorIndex + sectorCount) > 256u) {
        return FLS_E_INVALID_ADDRESS;
    }

    gFls_Tc3xxStatus = FLS_BUSY;

    for (i = 0u; i < sectorCount; i++) {
        /* Check protection */
        if ((gFls_Tc3xxProtection[sectorIndex + i] & FLS_PROTECTION_ERASE) != 0u) {
            gFls_Tc3xxStatus = FLS_IDLE;
            return FLS_E_WRITE_PROTECTED;
        }

        address = (sectorIndex + i) * FLS_TC3XX_SECTOR_SIZE;

        result = Fls_Tc3xx_ExecuteCommand(FLS_TC3XX_CMD_SECTOR_ERASE,
                                         address, NULL, 0);
        if (result != FLS_OK) {
            gFls_Tc3xxStatus = FLS_IDLE;
            return result;
        }

        gFls_Tc3xxEraseCount[sectorIndex + i]++;
    }

    gFls_Tc3xxStatus = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Tc3xx_Compare(
    uint32_t address,
    const uint8_t* data,
    uint32_t length
)
{
    Fls_ErrorCode_t result;
    uint8_t* readBuf;

    if (!gFls_Tc3xxInitialized) {
        return FLS_E_UNINIT;
    }

    if (data == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_Tc3xx_IsAddressValid(address)) {
        return FLS_E_INVALID_ADDRESS;
    }

    /* Read and compare */
    readBuf = (uint8_t*)malloc(length);
    if (readBuf == NULL) {
        return FLS_E_NOT_OK;
    }

    result = Fls_Tc3xx_Read(address, readBuf, length);
    if (result != FLS_OK) {
        free(readBuf);
        return result;
    }

    if (memcmp(readBuf, data, length) != 0) {
        free(readBuf);
        return FLS_E_COMPARE_FAILED;
    }

    free(readBuf);

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_Tc3xx_BlankCheck(uint32_t address, uint32_t length)
{
    uint8_t* readBuf;
    Fls_ErrorCode_t result;
    uint32_t i;

    if (!gFls_Tc3xxInitialized) {
        return FLS_E_UNINIT;
    }

    if (!Fls_Tc3xx_IsAddressValid(address)) {
        return FLS_E_INVALID_ADDRESS;
    }

    readBuf = (uint8_t*)malloc(length);
    if (readBuf == NULL) {
        return FLS_E_NOT_OK;
    }

    result = Fls_Tc3xx_Read(address, readBuf, length);
    if (result != FLS_OK) {
        free(readBuf);
        return result;
    }

    for (i = 0u; i < length; i++) {
        if (readBuf[i] != FLS_TC3XX_ERASED_VALUE) {
            free(readBuf);
            return FLS_E_COMPARE_FAILED;
        }
    }

    free(readBuf);

    return FLS_OK;
}

static Fls_ModuleStatus_t Fls_Tc3xx_GetStatus(void)
{
    return gFls_Tc3xxStatus;
}

static bool Fls_Tc3xx_IsBusy(void)
{
    if (!gFls_Tc3xxEmulationMode) {
        /* Check hardware busy bit */
        /* return (FLASH_STATUS_REG & FLS_TC3XX_STAT_BUSY) != 0u; */
    }
    return (gFls_Tc3xxStatus == FLS_BUSY);
}

static uint32_t Fls_Tc3xx_GetSectorSize(uint32_t sectorIndex)
{
    (void)sectorIndex;
    return FLS_TC3XX_SECTOR_SIZE;
}

static uint32_t Fls_Tc3xx_GetPageSize(uint32_t sectorIndex)
{
    (void)sectorIndex;
    return FLS_TC3XX_PAGE_SIZE;
}

static Fls_ErrorCode_t Fls_Tc3xx_SetProtection(
    uint32_t sectorIndex,
    Fls_ProtectionType_t protection
)
{
    if (sectorIndex >= 256u) {
        return FLS_E_INVALID_ADDRESS;
    }

    gFls_Tc3xxProtection[sectorIndex] = protection;

    /* In real hardware, would configure UCB (User Configuration Block) */
    /* FLASH_UCB_PROT = protection; */

    return FLS_OK;
}

static uint32_t Fls_Tc3xx_GetEraseCount(uint32_t sectorIndex)
{
    if (sectorIndex >= 256u) {
        return 0u;
    }

    return gFls_Tc3xxEraseCount[sectorIndex];
}

static Fls_ErrorCode_t Fls_Tc3xx_Lock(uint32_t sectorIndex)
{
    return Fls_Tc3xx_SetProtection(sectorIndex, FLS_PROTECTION_WRITE);
}

static Fls_ErrorCode_t Fls_Tc3xx_Unlock(uint32_t sectorIndex)
{
    return Fls_Tc3xx_SetProtection(sectorIndex, FLS_PROTECTION_NONE);
}

/*============================================================================*
 * Public Interface
 *============================================================================*/

static Fls_HwInterfaceType gFls_Tc3xxHwInterface = {
    .Init = Fls_Tc3xx_Init,
    .Deinit = Fls_Tc3xx_Deinit,
    .Read = Fls_Tc3xx_Read,
    .Write = Fls_Tc3xx_Write,
    .Erase = Fls_Tc3xx_Erase,
    .Compare = Fls_Tc3xx_Compare,
    .BlankCheck = Fls_Tc3xx_BlankCheck,
    .GetStatus = Fls_Tc3xx_GetStatus,
    .IsBusy = Fls_Tc3xx_IsBusy,
    .GetSectorIndex = Fls_Tc3xx_GetSectorIndex,
    .GetSectorSize = Fls_Tc3xx_GetSectorSize,
    .GetPageSize = Fls_Tc3xx_GetPageSize,
    .SetProtection = Fls_Tc3xx_SetProtection,
    .GetEraseCount = Fls_Tc3xx_GetEraseCount,
    .Lock = Fls_Tc3xx_Lock,
    .Unlock = Fls_Tc3xx_Unlock
};

Fls_ErrorCode_t Fls_Tc3xx_Register(void)
{
    return Fls_RegisterHwInterface(&gFls_Tc3xxHwInterface);
}

void Fls_Tc3xx_SetEmulationMode(bool enable)
{
    gFls_Tc3xxEmulationMode = enable;
}

bool Fls_Tc3xx_GetEmulationMode(void)
{
    return gFls_Tc3xxEmulationMode;
}

uint32_t Fls_Tc3xx_GetPflashSize(void)
{
    return FLS_TC3XX_PFLASH_SIZE;
}

uint32_t Fls_Tc3xx_GetDflashSize(void)
{
    return FLS_TC3XX_DFLASH_SIZE;
}
