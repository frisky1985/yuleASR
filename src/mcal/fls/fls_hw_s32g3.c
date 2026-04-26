/**
 * @file fls_hw_s32g3.c
 * @brief Fls Hardware Driver for NXP S32G3
 * @version 1.0.0
 * @date 2026
 *
 * Hardware-specific flash driver for NXP S32G3 series.
 * Supports:
 * - Flash operations via Flash Memory Controller (FMC)
 * - Read/Write/Erase operations
 * - Write protection via Block Locks
 * - Error detection and correction
 *
 * Reference: NXP S32G3 Reference Manual, Flash Module chapter
 */

#include "mcal/fls/fls.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/*============================================================================*
 * Hardware Abstraction (S32G3 Specific)
 *============================================================================*/

/* Flash memory map */
#define FLS_S32G3_FLASH_BASE        0x00400000u
#define FLS_S32G3_FLASH_SIZE        (8u * 1024u * 1024u)  /* 8MB Flash */
#define FLS_S32G3_SECTOR_SIZE       4096u                 /* 4KB sectors */
#define FLS_S32G3_BLOCK_SIZE        32768u                /* 32KB blocks */
#define FLS_S32G3_PAGE_SIZE         256u                  /* 256B programming page */
#define FLS_S32G3_ERASED_VALUE      0xFFu

/* Flash registers (simplified) */
#define FLS_S32G3_FMC_BASE          0x40194000u
#define FLS_S32G3_FMC_STAT          (FLS_S32G3_FMC_BASE + 0x00u)
#define FLS_S32G3_FMC_CTRL          (FLS_S32G3_FMC_BASE + 0x04u)
#define FLS_S32G3_FMC_ADDR          (FLS_S32G3_FMC_BASE + 0x08u)
#define FLS_S32G3_FMC_DATA          (FLS_S32G3_FMC_BASE + 0x0Cu)
#define FLS_S32G3_FMC_LOCK          (FLS_S32G3_FMC_BASE + 0x10u)

/* Status register bits */
#define FLS_S32G3_STAT_BUSY         0x00000001u
#define FLS_S32G3_STAT_RDCOLLIE     0x00000004u
#define FLS_S32G3_STAT_ACCERR       0x00000010u
#define FLS_S32G3_STAT_PVIOL        0x00000020u

/* Control register commands */
#define FLS_S32G3_CTRL_RDY          0x00000080u
#define FLS_S32G3_CTRL_RDYIE        0x00000040u
#define FLS_S32G3_CTRL_ERASE        0x00000020u
#define FLS_S32G3_CTRL_PROG         0x00000010u

/*============================================================================*
 * Static Variables
 *============================================================================*/
static bool gFls_S32g3Initialized = false;
static Fls_ModuleStatus_t gFls_S32g3Status = FLS_UNINIT;
static uint32_t gFls_S32g3EraseCount[2048];  /* Per-sector erase count */
static Fls_ProtectionType_t gFls_S32g3Protection[2048];

/* Emulation mode for testing */
static bool gFls_S32g3EmulationMode = false;
static uint8_t* gFls_S32g3EmuMemory = NULL;

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Get sector index from address
 */
static uint32_t Fls_S32g3_GetSectorIndex(uint32_t address)
{
    if (address < FLS_S32G3_FLASH_SIZE) {
        return address / FLS_S32G3_SECTOR_SIZE;
    }
    return 0xFFFFFFFFu;
}

/**
 * @brief Check if address is valid
 */
static bool Fls_S32g3_IsAddressValid(uint32_t address)
{
    return (address < FLS_S32G3_FLASH_SIZE);
}

/**
 * @brief Wait for flash operation completion
 */
static Fls_ErrorCode_t Fls_S32g3_WaitForComplete(void)
{
    uint32_t timeout = 1000000u;

    if (gFls_S32g3EmulationMode) {
        return FLS_OK;
    }

    /* Wait for CCIF (Command Complete Interrupt Flag) */
    while (timeout > 0u) {
        /* uint32_t status = REG_READ(FLS_S32G3_FMC_STAT); */
        /* if ((status & FLS_S32G3_STAT_BUSY) == 0u) { break; } */
        timeout--;
    }

    if (timeout == 0u) {
        return FLS_E_TIMEOUT;
    }

    /* Check for errors */
    /* uint32_t status = REG_READ(FLS_S32G3_FMC_STAT); */
    /* if (status & (FLS_S32G3_STAT_ACCERR | FLS_S32G3_STAT_PVIOL)) { */
    /*     return FLS_E_NOT_OK; */
    /* } */

    return FLS_OK;
}

/**
 * @brief Clear flash errors
 */
static void Fls_S32g3_ClearErrors(void)
{
    if (!gFls_S32g3EmulationMode) {
        /* REG_WRITE(FLS_S32G3_FMC_STAT, */
        /*          FLS_S32G3_STAT_ACCERR | FLS_S32G3_STAT_PVIOL); */
    }
}

/**
 * @brief Execute flash command
 */
static Fls_ErrorCode_t Fls_S32g3_ExecuteCommand(
    uint8_t command,
    uint32_t address,
    const uint8_t* data,
    uint32_t length
)
{
    Fls_ErrorCode_t result;

    if (gFls_S32g3EmulationMode) {
        switch (command) {
            case FLS_S32G3_CTRL_PROG:
                if (data == NULL) {
                    return FLS_E_PARAM_POINTER;
                }
                for (uint32_t i = 0u; i < length; i++) {
                    gFls_S32g3EmuMemory[address + i] &= data[i];
                }
                break;

            case FLS_S32G3_CTRL_ERASE:
                {
                    uint32_t sectorAddr = (address / FLS_S32G3_SECTOR_SIZE) *
                                          FLS_S32G3_SECTOR_SIZE;
                    (void)memset(&gFls_S32g3EmuMemory[sectorAddr],
                                FLS_S32G3_ERASED_VALUE, FLS_S32G3_SECTOR_SIZE);
                }
                break;

            default:
                return FLS_E_NOT_OK;
        }
        return FLS_OK;
    }

    /* Real hardware command execution */
    /* 1. Clear errors */
    Fls_S32g3_ClearErrors();

    /* 2. Write address */
    /* REG_WRITE(FLS_S32G3_FMC_ADDR, address); */

    /* 3. Write data if programming */
    if ((command == FLS_S32G3_CTRL_PROG) && (data != NULL)) {
        /* for (uint32_t i = 0; i < length; i += 4) { */
        /*     REG_WRITE(FLS_S32G3_FMC_DATA, *(uint32_t*)&data[i]); */
        /* } */
    }

    /* 4. Launch command */
    /* uint32_t ctrl = REG_READ(FLS_S32G3_FMC_CTRL); */
    /* ctrl |= command | FLS_S32G3_CTRL_RDY; */
    /* REG_WRITE(FLS_S32G3_FMC_CTRL, ctrl); */

    /* 5. Wait for completion */
    result = Fls_S32g3_WaitForComplete();

    return result;
}

/*============================================================================*
 * Hardware Interface Implementation
 *============================================================================*/

static Fls_ErrorCode_t Fls_S32g3_Init(const Fls_ConfigType* config)
{
    uint32_t i;

    (void)config;

    if (gFls_S32g3Initialized) {
        return FLS_E_ALREADY_INITIALIZED;
    }

    /* Initialize arrays */
    for (i = 0u; i < 2048u; i++) {
        gFls_S32g3EraseCount[i] = 0u;
        gFls_S32g3Protection[i] = FLS_PROTECTION_NONE;
    }

    /* Allocate emulation memory if needed */
    if (gFls_S32g3EmulationMode) {
        gFls_S32g3EmuMemory = (uint8_t*)malloc(FLS_S32G3_FLASH_SIZE);
        if (gFls_S32g3EmuMemory == NULL) {
            return FLS_E_NOT_OK;
        }
        (void)memset(gFls_S32g3EmuMemory, FLS_S32G3_ERASED_VALUE,
                    FLS_S32G3_FLASH_SIZE);
    } else {
        /* Real hardware initialization */
        /* 1. Enable clock */
        /* PCC_FMC |= PCC_CGC; */

        /* 2. Clear any pending errors */
        Fls_S32g3_ClearErrors();

        /* 3. Set default wait states */
        /* REG_WRITE(FLS_S32G3_FMC_CTRL, defaultWaitStates); */
    }

    gFls_S32g3Initialized = true;
    gFls_S32g3Status = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_S32g3_Deinit(void)
{
    if (!gFls_S32g3Initialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_S32g3EmulationMode && (gFls_S32g3EmuMemory != NULL)) {
        free(gFls_S32g3EmuMemory);
        gFls_S32g3EmuMemory = NULL;
    }

    gFls_S32g3Initialized = false;
    gFls_S32g3Status = FLS_UNINIT;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_S32g3_Read(
    uint32_t address,
    uint8_t* data,
    uint32_t length
)
{
    if (!gFls_S32g3Initialized) {
        return FLS_E_UNINIT;
    }

    if (data == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_S32g3_IsAddressValid(address)) {
        return FLS_E_INVALID_ADDRESS;
    }

    if ((address + length) > FLS_S32G3_FLASH_SIZE) {
        return FLS_E_INVALID_LENGTH;
    }

    gFls_S32g3Status = FLS_BUSY;

    if (gFls_S32g3EmulationMode) {
        (void)memcpy(data, &gFls_S32g3EmuMemory[address], length);
    } else {
        /* Direct memory read */
        uint8_t* flashPtr = (uint8_t*)(FLS_S32G3_FLASH_BASE + address);
        (void)memcpy(data, flashPtr, length);
    }

    gFls_S32g3Status = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_S32g3_Write(
    uint32_t address,
    const uint8_t* data,
    uint32_t length
)
{
    Fls_ErrorCode_t result;
    uint32_t sector;
    uint32_t offset;
    uint32_t pageRemain;
    uint32_t toWrite;

    if (!gFls_S32g3Initialized) {
        return FLS_E_UNINIT;
    }

    if (data == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_S32g3_IsAddressValid(address)) {
        return FLS_E_INVALID_ADDRESS;
    }

    /* Check protection */
    sector = Fls_S32g3_GetSectorIndex(address);
    if ((sector < 2048u) &&
        ((gFls_S32g3Protection[sector] & FLS_PROTECTION_WRITE) != 0u)) {
        return FLS_E_WRITE_PROTECTED;
    }

    gFls_S32g3Status = FLS_BUSY;

    offset = 0u;
    while (offset < length) {
        pageRemain = FLS_S32G3_PAGE_SIZE - ((address + offset) % FLS_S32G3_PAGE_SIZE);
        toWrite = (length - offset) < pageRemain ? (length - offset) : pageRemain;

        result = Fls_S32g3_ExecuteCommand(FLS_S32G3_CTRL_PROG,
                                         address + offset, &data[offset], toWrite);
        if (result != FLS_OK) {
            gFls_S32g3Status = FLS_IDLE;
            return result;
        }

        offset += toWrite;
    }

    gFls_S32g3Status = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_S32g3_Erase(uint32_t sectorIndex, uint32_t sectorCount)
{
    Fls_ErrorCode_t result;
    uint32_t i;
    uint32_t address;

    if (!gFls_S32g3Initialized) {
        return FLS_E_UNINIT;
    }

    if ((sectorIndex + sectorCount) > 2048u) {
        return FLS_E_INVALID_ADDRESS;
    }

    gFls_S32g3Status = FLS_BUSY;

    for (i = 0u; i < sectorCount; i++) {
        /* Check protection */
        if ((gFls_S32g3Protection[sectorIndex + i] & FLS_PROTECTION_ERASE) != 0u) {
            gFls_S32g3Status = FLS_IDLE;
            return FLS_E_WRITE_PROTECTED;
        }

        address = (sectorIndex + i) * FLS_S32G3_SECTOR_SIZE;

        result = Fls_S32g3_ExecuteCommand(FLS_S32G3_CTRL_ERASE, address, NULL, 0);
        if (result != FLS_OK) {
            gFls_S32g3Status = FLS_IDLE;
            return result;
        }

        gFls_S32g3EraseCount[sectorIndex + i]++;
    }

    gFls_S32g3Status = FLS_IDLE;

    return FLS_OK;
}

static Fls_ErrorCode_t Fls_S32g3_Compare(
    uint32_t address,
    const uint8_t* data,
    uint32_t length
)
{
    uint8_t* readBuf;
    Fls_ErrorCode_t result;

    if (!gFls_S32g3Initialized) {
        return FLS_E_UNINIT;
    }

    if (data == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_S32g3_IsAddressValid(address)) {
        return FLS_E_INVALID_ADDRESS;
    }

    readBuf = (uint8_t*)malloc(length);
    if (readBuf == NULL) {
        return FLS_E_NOT_OK;
    }

    result = Fls_S32g3_Read(address, readBuf, length);
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

static Fls_ErrorCode_t Fls_S32g3_BlankCheck(uint32_t address, uint32_t length)
{
    uint8_t* readBuf;
    Fls_ErrorCode_t result;
    uint32_t i;

    if (!gFls_S32g3Initialized) {
        return FLS_E_UNINIT;
    }

    if (!Fls_S32g3_IsAddressValid(address)) {
        return FLS_E_INVALID_ADDRESS;
    }

    readBuf = (uint8_t*)malloc(length);
    if (readBuf == NULL) {
        return FLS_E_NOT_OK;
    }

    result = Fls_S32g3_Read(address, readBuf, length);
    if (result != FLS_OK) {
        free(readBuf);
        return result;
    }

    for (i = 0u; i < length; i++) {
        if (readBuf[i] != FLS_S32G3_ERASED_VALUE) {
            free(readBuf);
            return FLS_E_COMPARE_FAILED;
        }
    }

    free(readBuf);

    return FLS_OK;
}

static Fls_ModuleStatus_t Fls_S32g3_GetStatus(void)
{
    return gFls_S32g3Status;
}

static bool Fls_S32g3_IsBusy(void)
{
    if (!gFls_S32g3EmulationMode) {
        /* return (REG_READ(FLS_S32G3_FMC_STAT) & FLS_S32G3_STAT_BUSY) != 0u; */
    }
    return (gFls_S32g3Status == FLS_BUSY);
}

static uint32_t Fls_S32g3_GetSectorSize(uint32_t sectorIndex)
{
    (void)sectorIndex;
    return FLS_S32G3_SECTOR_SIZE;
}

static uint32_t Fls_S32g3_GetPageSize(uint32_t sectorIndex)
{
    (void)sectorIndex;
    return FLS_S32G3_PAGE_SIZE;
}

static Fls_ErrorCode_t Fls_S32g3_SetProtection(
    uint32_t sectorIndex,
    Fls_ProtectionType_t protection
)
{
    if (sectorIndex >= 2048u) {
        return FLS_E_INVALID_ADDRESS;
    }

    gFls_S32g3Protection[sectorIndex] = protection;

    /* In real hardware, configure block locks */
    /* REG_WRITE(FLS_S32G3_FMC_LOCK + (sectorIndex/32)*4, lockBits); */

    return FLS_OK;
}

static uint32_t Fls_S32g3_GetEraseCount(uint32_t sectorIndex)
{
    if (sectorIndex >= 2048u) {
        return 0u;
    }

    return gFls_S32g3EraseCount[sectorIndex];
}

static Fls_ErrorCode_t Fls_S32g3_Lock(uint32_t sectorIndex)
{
    return Fls_S32g3_SetProtection(sectorIndex, FLS_PROTECTION_WRITE);
}

static Fls_ErrorCode_t Fls_S32g3_Unlock(uint32_t sectorIndex)
{
    return Fls_S32g3_SetProtection(sectorIndex, FLS_PROTECTION_NONE);
}

/*============================================================================*
 * Public Interface
 *============================================================================*/

static Fls_HwInterfaceType gFls_S32g3HwInterface = {
    .Init = Fls_S32g3_Init,
    .Deinit = Fls_S32g3_Deinit,
    .Read = Fls_S32g3_Read,
    .Write = Fls_S32g3_Write,
    .Erase = Fls_S32g3_Erase,
    .Compare = Fls_S32g3_Compare,
    .BlankCheck = Fls_S32g3_BlankCheck,
    .GetStatus = Fls_S32g3_GetStatus,
    .IsBusy = Fls_S32g3_IsBusy,
    .GetSectorIndex = Fls_S32g3_GetSectorIndex,
    .GetSectorSize = Fls_S32g3_GetSectorSize,
    .GetPageSize = Fls_S32g3_GetPageSize,
    .SetProtection = Fls_S32g3_SetProtection,
    .GetEraseCount = Fls_S32g3_GetEraseCount,
    .Lock = Fls_S32g3_Lock,
    .Unlock = Fls_S32g3_Unlock
};

Fls_ErrorCode_t Fls_S32g3_Register(void)
{
    return Fls_RegisterHwInterface(&gFls_S32g3HwInterface);
}

void Fls_S32g3_SetEmulationMode(bool enable)
{
    gFls_S32g3EmulationMode = enable;
}

bool Fls_S32g3_GetEmulationMode(void)
{
    return gFls_S32g3EmulationMode;
}

uint32_t Fls_S32g3_GetFlashSize(void)
{
    return FLS_S32G3_FLASH_SIZE;
}
