/**
 * @file fls.c
 * @brief Fls (Flash Driver) Implementation
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Fls Module - Flash EEPROM Emulation Support
 * Compliant with AUTOSAR R22-11 MCAL Specification
 * MISRA C:2012 compliant
 */

#include "mcal/fls/fls.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================*
 * Static Variables
 *============================================================================*/
static Fls_RuntimeStateType gFls_State;
static const Fls_HwInterfaceType* gFls_HwInterface = NULL;

/*============================================================================*
 * Default Configuration
 *============================================================================*/
static const Fls_SectorConfigType gFls_DefaultSectors[] = {
    {
        .sectorIndex = 0u,
        .startAddress = 0x00000000u,
        .size = 65536u,
        .pageSize = 256u,
        .protection = FLS_PROTECTION_NONE,
        .eraseAuto = false,
        .eraseCount = 0u
    },
    {
        .sectorIndex = 1u,
        .startAddress = 0x00010000u,
        .size = 65536u,
        .pageSize = 256u,
        .protection = FLS_PROTECTION_NONE,
        .eraseAuto = false,
        .eraseCount = 0u
    },
    {
        .sectorIndex = 2u,
        .startAddress = 0x00020000u,
        .size = 65536u,
        .pageSize = 256u,
        .protection = FLS_PROTECTION_NONE,
        .eraseAuto = false,
        .eraseCount = 0u
    },
    {
        .sectorIndex = 3u,
        .startAddress = 0x00030000u,
        .size = 65536u,
        .pageSize = 256u,
        .protection = FLS_PROTECTION_NONE,
        .eraseAuto = false,
        .eraseCount = 0u
    }
};

static const Fls_GeneralConfigType gFls_DefaultGeneral = {
    .baseAddress = 0x00000000u,
    .totalSize = 262144u,
    .defaultSectorSize = 65536u,
    .defaultPageSize = 256u,
    .maxReadMode = 0u,
    .maxWriteMode = 0u,
    .jobCallCycle = 10u,
    .loadPageOnDemand = false,
    .jobEndNotificationEnabled = true,
    .jobErrorNotificationEnabled = true
};

const Fls_ConfigType Fls_Config = {
    .general = &gFls_DefaultGeneral,
    .sectors = gFls_DefaultSectors,
    .sectorCount = 4u,
    .hwType = FLS_HW_GENERIC,
    .hwConfig = NULL
};

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Check if address is valid
 */
static bool Fls_IsAddressValid(Fls_AddressType address)
{
    const Fls_GeneralConfigType* general;

    if (gFls_State.config == NULL) {
        return false;
    }

    general = gFls_State.config->general;
    if (general == NULL) {
        return false;
    }

    return ((address >= general->baseAddress) &&
            (address < (general->baseAddress + general->totalSize)));
}

/**
 * @brief Check if range is valid
 */
static bool Fls_IsRangeValid(Fls_AddressType address, Fls_LengthType length)
{
    const Fls_GeneralConfigType* general;

    if (gFls_State.config == NULL) {
        return false;
    }

    general = gFls_State.config->general;
    if (general == NULL) {
        return false;
    }

    if (length == 0u) {
        return false;
    }

    return ((address >= general->baseAddress) &&
            ((address + length) <= (general->baseAddress + general->totalSize)));
}

/**
 * @brief Get sector index from address
 */
static uint32_t Fls_FindSectorIndex(Fls_AddressType address)
{
    const Fls_SectorConfigType* sectors;
    uint32_t sectorCount;
    uint32_t i;

    if (gFls_State.config == NULL) {
        return 0xFFFFFFFFu;
    }

    sectors = gFls_State.config->sectors;
    sectorCount = gFls_State.config->sectorCount;

    if ((sectors == NULL) || (sectorCount == 0u)) {
        return 0xFFFFFFFFu;
    }

    for (i = 0u; i < sectorCount; i++) {
        if ((address >= sectors[i].startAddress) &&
            (address < (sectors[i].startAddress + sectors[i].size))) {
            return i;
        }
    }

    return 0xFFFFFFFFu;
}

/**
 * @brief Check write protection
 */
static Fls_ErrorCode_t Fls_CheckWriteProtection(
    Fls_AddressType address,
    Fls_LengthType length
)
{
    const Fls_SectorConfigType* sectors;
    uint32_t startSector;
    uint32_t endSector;
    uint32_t i;

    if (gFls_State.config == NULL) {
        return FLS_E_NOT_OK;
    }

    sectors = gFls_State.config->sectors;
    if (sectors == NULL) {
        return FLS_E_NOT_OK;
    }

    startSector = Fls_FindSectorIndex(address);
    endSector = Fls_FindSectorIndex(address + length - 1u);

    if ((startSector == 0xFFFFFFFFu) || (endSector == 0xFFFFFFFFu)) {
        return FLS_E_INVALID_ADDRESS;
    }

    for (i = startSector; i <= endSector; i++) {
        if ((sectors[i].protection & FLS_PROTECTION_WRITE) != 0u) {
            return FLS_E_WRITE_PROTECTED;
        }
    }

    return FLS_OK;
}

/**
 * @brief Check erase protection
 */
static Fls_ErrorCode_t Fls_CheckEraseProtection(
    Fls_AddressType address,
    Fls_LengthType length
)
{
    const Fls_SectorConfigType* sectors;
    uint32_t startSector;
    uint32_t endSector;
    uint32_t i;

    if (gFls_State.config == NULL) {
        return FLS_E_NOT_OK;
    }

    sectors = gFls_State.config->sectors;
    if (sectors == NULL) {
        return FLS_E_NOT_OK;
    }

    startSector = Fls_FindSectorIndex(address);
    endSector = Fls_FindSectorIndex(address + length - 1u);

    if ((startSector == 0xFFFFFFFFu) || (endSector == 0xFFFFFFFFu)) {
        return FLS_E_INVALID_ADDRESS;
    }

    for (i = startSector; i <= endSector; i++) {
        if ((sectors[i].protection & FLS_PROTECTION_ERASE) != 0u) {
            return FLS_E_WRITE_PROTECTED;
        }
    }

    return FLS_OK;
}

/**
 * @brief Reset current job
 */
static void Fls_ResetJob(void)
{
    (void)memset(&gFls_State.currentJob, 0, sizeof(Fls_JobType));
    gFls_State.currentJob.jobType = FLS_JOB_NONE;
    gFls_State.currentJob.status = FLS_JOB_IDLE;
}

/**
 * @brief Process current job (internal)
 */
static Fls_ErrorCode_t Fls_ProcessJob(void)
{
    Fls_JobType* job;
    Fls_ErrorCode_t result;

    job = &gFls_State.currentJob;

    if (job->status != FLS_JOB_IN_PROGRESS) {
        return FLS_OK;
    }

    if (gFls_HwInterface == NULL) {
        job->status = FLS_JOB_FAILED;
        return FLS_E_NOT_OK;
    }

    result = FLS_E_NOT_OK;

    switch (job->jobType) {
        case FLS_JOB_READ:
            if (gFls_HwInterface->Read != NULL) {
                result = gFls_HwInterface->Read(
                    job->address,
                    job->dataBuffer,
                    job->length
                );
            }
            break;

        case FLS_JOB_WRITE:
            if (gFls_HwInterface->Write != NULL) {
                result = gFls_HwInterface->Write(
                    job->address,
                    job->dataBuffer,
                    job->length
                );
            }
            break;

        case FLS_JOB_ERASE:
            if (gFls_HwInterface->Erase != NULL) {
                result = gFls_HwInterface->Erase(
                    Fls_FindSectorIndex(job->address),
                    (job->length / gFls_State.config->general->defaultSectorSize)
                );
            }
            break;

        case FLS_JOB_COMPARE:
            if (gFls_HwInterface->Compare != NULL) {
                result = gFls_HwInterface->Compare(
                    job->address,
                    job->dataBuffer,
                    job->length
                );
            }
            break;

        case FLS_JOB_BLANK_CHECK:
            if (gFls_HwInterface->BlankCheck != NULL) {
                result = gFls_HwInterface->BlankCheck(
                    job->address,
                    job->length
                );
            }
            break;

        default:
            result = FLS_E_NOT_OK;
            break;
    }

    if (result == FLS_OK) {
        job->status = FLS_JOB_COMPLETED;
        gFls_State.status = FLS_IDLE;
    } else if ((result != FLS_E_BUSY) && (result != FLS_OK)) {
        job->status = FLS_JOB_FAILED;
        gFls_State.status = FLS_IDLE;
        gFls_State.errorCount++;
    } else {
        /* Still in progress */
    }

    return result;
}

/**
 * @brief Notify job end
 */
static void Fls_NotifyJobEnd(void)
{
    if (gFls_State.notificationCb != NULL) {
        gFls_State.notificationCb(
            gFls_State.currentJob.jobType,
            FLS_OK,
            gFls_State.currentJob.address,
            gFls_State.currentJob.length
        );
    }

    Fls_JobEndNotification();
}

/**
 * @brief Notify job error
 */
static void Fls_NotifyJobError(void)
{
    if (gFls_State.notificationCb != NULL) {
        gFls_State.notificationCb(
            gFls_State.currentJob.jobType,
            FLS_E_NOT_OK,
            gFls_State.currentJob.address,
            gFls_State.currentJob.length
        );
    }

    Fls_JobErrorNotification();
}

/*============================================================================*
 * Initialization API
 *============================================================================*/

Fls_ErrorCode_t Fls_Init(const Fls_ConfigType* config)
{
    Fls_ErrorCode_t result;

    if (gFls_State.initialized) {
        return FLS_E_ALREADY_INITIALIZED;
    }

    /* Initialize state */
    (void)memset(&gFls_State, 0, sizeof(Fls_RuntimeStateType));
    gFls_State.status = FLS_UNINIT;

    /* Use default config if NULL */
    if (config == NULL) {
        gFls_State.config = &Fls_Config;
    } else {
        gFls_State.config = config;
    }

    /* Initialize hardware interface if registered */
    if (gFls_HwInterface != NULL) {
        if (gFls_HwInterface->Init != NULL) {
            result = gFls_HwInterface->Init(gFls_State.config);
            if (result != FLS_OK) {
                return result;
            }
        }
    }

    gFls_State.status = FLS_IDLE;
    gFls_State.initialized = true;
    Fls_ResetJob();

    return FLS_OK;
}

Fls_ErrorCode_t Fls_Deinit(void)
{
    Fls_ErrorCode_t result = FLS_OK;

    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    /* Deinitialize hardware */
    if (gFls_HwInterface != NULL) {
        if (gFls_HwInterface->Deinit != NULL) {
            result = gFls_HwInterface->Deinit();
        }
    }

    gFls_State.initialized = false;
    gFls_State.status = FLS_UNINIT;

    return result;
}

Fls_ModuleStatus_t Fls_GetStatus(void)
{
    if (!gFls_State.initialized) {
        return FLS_UNINIT;
    }

    if ((gFls_HwInterface != NULL) && (gFls_HwInterface->GetStatus != NULL)) {
        return gFls_HwInterface->GetStatus();
    }

    return gFls_State.status;
}

bool Fls_IsInitialized(void)
{
    return gFls_State.initialized;
}

/*============================================================================*
 * Synchronous API
 *============================================================================*/

Fls_ErrorCode_t Fls_Read(
    Fls_AddressType sourceAddress,
    Fls_LengthType length,
    uint8_t* targetAddressPtr
)
{
    Fls_ErrorCode_t result;

    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (targetAddressPtr == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_IsRangeValid(sourceAddress, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    if (gFls_HwInterface == NULL) {
        return FLS_E_NOT_OK;
    }

    if (gFls_HwInterface->Read == NULL) {
        return FLS_E_NOT_OK;
    }

    gFls_State.status = FLS_BUSY;
    gFls_State.totalReadCount++;

    result = gFls_HwInterface->Read(sourceAddress, targetAddressPtr, length);

    gFls_State.status = FLS_IDLE;

    if (result != FLS_OK) {
        gFls_State.errorCount++;
    }

    return result;
}

Fls_ErrorCode_t Fls_Write(
    Fls_AddressType targetAddress,
    Fls_LengthType length,
    const uint8_t* sourceAddressPtr
)
{
    Fls_ErrorCode_t result;

    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (sourceAddressPtr == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_IsRangeValid(targetAddress, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    result = Fls_CheckWriteProtection(targetAddress, length);
    if (result != FLS_OK) {
        return result;
    }

    if (gFls_HwInterface == NULL) {
        return FLS_E_NOT_OK;
    }

    if (gFls_HwInterface->Write == NULL) {
        return FLS_E_NOT_OK;
    }

    gFls_State.status = FLS_BUSY;
    gFls_State.totalWriteCount++;

    result = gFls_HwInterface->Write(
        targetAddress,
        sourceAddressPtr,
        length
    );

    gFls_State.status = FLS_IDLE;

    if (result != FLS_OK) {
        gFls_State.errorCount++;
    }

    return result;
}

Fls_ErrorCode_t Fls_Erase(
    Fls_AddressType targetAddress,
    Fls_LengthType length
)
{
    Fls_ErrorCode_t result;
    uint32_t sectorIndex;
    uint32_t sectorCount;
    uint32_t sectorSize;

    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (!Fls_IsRangeValid(targetAddress, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    result = Fls_CheckEraseProtection(targetAddress, length);
    if (result != FLS_OK) {
        return result;
    }

    if (gFls_HwInterface == NULL) {
        return FLS_E_NOT_OK;
    }

    if (gFls_HwInterface->Erase == NULL) {
        return FLS_E_NOT_OK;
    }

    /* Calculate sector range */
    sectorIndex = Fls_FindSectorIndex(targetAddress);
    if (sectorIndex == 0xFFFFFFFFu) {
        return FLS_E_INVALID_ADDRESS;
    }

    sectorSize = gFls_State.config->general->defaultSectorSize;
    sectorCount = (length + sectorSize - 1u) / sectorSize;

    gFls_State.status = FLS_BUSY;
    gFls_State.totalEraseCount++;

    result = gFls_HwInterface->Erase(sectorIndex, sectorCount);

    gFls_State.status = FLS_IDLE;

    if (result != FLS_OK) {
        gFls_State.errorCount++;
    }

    return result;
}

Fls_ErrorCode_t Fls_Compare(
    Fls_AddressType sourceAddress,
    Fls_LengthType length,
    const uint8_t* targetAddressPtr
)
{
    Fls_ErrorCode_t result;

    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (targetAddressPtr == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_IsRangeValid(sourceAddress, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    if (gFls_HwInterface == NULL) {
        return FLS_E_NOT_OK;
    }

    if (gFls_HwInterface->Compare == NULL) {
        return FLS_E_NOT_OK;
    }

    gFls_State.status = FLS_BUSY;

    result = gFls_HwInterface->Compare(
        sourceAddress,
        targetAddressPtr,
        length
    );

    gFls_State.status = FLS_IDLE;

    return result;
}

Fls_ErrorCode_t Fls_BlankCheck(
    Fls_AddressType targetAddress,
    Fls_LengthType length
)
{
    Fls_ErrorCode_t result;

    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (!Fls_IsRangeValid(targetAddress, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    if (gFls_HwInterface == NULL) {
        return FLS_E_NOT_OK;
    }

    if (gFls_HwInterface->BlankCheck == NULL) {
        return FLS_E_NOT_OK;
    }

    gFls_State.status = FLS_BUSY;

    result = gFls_HwInterface->BlankCheck(targetAddress, length);

    gFls_State.status = FLS_IDLE;

    return result;
}

/*============================================================================*
 * Asynchronous API
 *============================================================================*/

Fls_ErrorCode_t Fls_Read_Async(
    Fls_AddressType sourceAddress,
    Fls_LengthType length,
    uint8_t* targetAddressPtr
)
{
    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_State.currentJob.status == FLS_JOB_IN_PROGRESS) {
        return FLS_E_BUSY;
    }

    if (targetAddressPtr == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_IsRangeValid(sourceAddress, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    Fls_ResetJob();
    gFls_State.currentJob.jobType = FLS_JOB_READ;
    gFls_State.currentJob.address = sourceAddress;
    gFls_State.currentJob.length = length;
    gFls_State.currentJob.dataBuffer = targetAddressPtr;
    gFls_State.currentJob.status = FLS_JOB_PENDING;
    gFls_State.currentJob.processed = 0u;

    gFls_State.status = FLS_BUSY;
    gFls_State.totalReadCount++;

    return FLS_OK;
}

Fls_ErrorCode_t Fls_Write_Async(
    Fls_AddressType targetAddress,
    Fls_LengthType length,
    const uint8_t* sourceAddressPtr
)
{
    Fls_ErrorCode_t result;

    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_State.currentJob.status == FLS_JOB_IN_PROGRESS) {
        return FLS_E_BUSY;
    }

    if (sourceAddressPtr == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_IsRangeValid(targetAddress, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    result = Fls_CheckWriteProtection(targetAddress, length);
    if (result != FLS_OK) {
        return result;
    }

    Fls_ResetJob();
    gFls_State.currentJob.jobType = FLS_JOB_WRITE;
    gFls_State.currentJob.address = targetAddress;
    gFls_State.currentJob.length = length;
    gFls_State.currentJob.dataBuffer = (uint8_t*)sourceAddressPtr;
    gFls_State.currentJob.status = FLS_JOB_PENDING;
    gFls_State.currentJob.processed = 0u;

    gFls_State.status = FLS_BUSY;
    gFls_State.totalWriteCount++;

    return FLS_OK;
}

Fls_ErrorCode_t Fls_Erase_Async(
    Fls_AddressType targetAddress,
    Fls_LengthType length
)
{
    Fls_ErrorCode_t result;

    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_State.currentJob.status == FLS_JOB_IN_PROGRESS) {
        return FLS_E_BUSY;
    }

    if (!Fls_IsRangeValid(targetAddress, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    result = Fls_CheckEraseProtection(targetAddress, length);
    if (result != FLS_OK) {
        return result;
    }

    Fls_ResetJob();
    gFls_State.currentJob.jobType = FLS_JOB_ERASE;
    gFls_State.currentJob.address = targetAddress;
    gFls_State.currentJob.length = length;
    gFls_State.currentJob.status = FLS_JOB_PENDING;
    gFls_State.currentJob.processed = 0u;

    gFls_State.status = FLS_BUSY;
    gFls_State.totalEraseCount++;

    return FLS_OK;
}

Fls_ErrorCode_t Fls_Compare_Async(
    Fls_AddressType sourceAddress,
    Fls_LengthType length,
    const uint8_t* targetAddressPtr
)
{
    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_State.currentJob.status == FLS_JOB_IN_PROGRESS) {
        return FLS_E_BUSY;
    }

    if (targetAddressPtr == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    if (!Fls_IsRangeValid(sourceAddress, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    Fls_ResetJob();
    gFls_State.currentJob.jobType = FLS_JOB_COMPARE;
    gFls_State.currentJob.address = sourceAddress;
    gFls_State.currentJob.length = length;
    gFls_State.currentJob.dataBuffer = (uint8_t*)targetAddressPtr;
    gFls_State.currentJob.status = FLS_JOB_PENDING;
    gFls_State.currentJob.processed = 0u;

    gFls_State.status = FLS_BUSY;

    return FLS_OK;
}

Fls_ErrorCode_t Fls_BlankCheck_Async(
    Fls_AddressType targetAddress,
    Fls_LengthType length
)
{
    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_State.currentJob.status == FLS_JOB_IN_PROGRESS) {
        return FLS_E_BUSY;
    }

    if (!Fls_IsRangeValid(targetAddress, length)) {
        return FLS_E_INVALID_ADDRESS;
    }

    Fls_ResetJob();
    gFls_State.currentJob.jobType = FLS_JOB_BLANK_CHECK;
    gFls_State.currentJob.address = targetAddress;
    gFls_State.currentJob.length = length;
    gFls_State.currentJob.status = FLS_JOB_PENDING;
    gFls_State.currentJob.processed = 0u;

    gFls_State.status = FLS_BUSY;

    return FLS_OK;
}

Fls_ErrorCode_t Fls_Cancel(void)
{
    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_State.currentJob.status == FLS_JOB_IDLE) {
        return FLS_OK;
    }

    gFls_State.currentJob.status = FLS_JOB_CANCELLED;
    gFls_State.status = FLS_IDLE;

    return FLS_OK;
}

Fls_JobStatus_t Fls_GetJobStatus(void)
{
    if (!gFls_State.initialized) {
        return FLS_JOB_IDLE;
    }

    return gFls_State.currentJob.status;
}

Fls_ErrorCode_t Fls_GetJobResult(void)
{
    Fls_JobStatus_t jobStatus;

    jobStatus = Fls_GetJobStatus();

    switch (jobStatus) {
        case FLS_JOB_COMPLETED:
            return FLS_OK;
        case FLS_JOB_FAILED:
            return FLS_E_NOT_OK;
        case FLS_JOB_PENDING:
        case FLS_JOB_IN_PROGRESS:
            return FLS_E_BUSY;
        case FLS_JOB_CANCELLED:
            return FLS_E_NOT_OK;
        default:
            return FLS_OK;
    }
}

void Fls_MainFunction(void)
{
    Fls_ErrorCode_t result;
    Fls_JobStatus_t prevStatus;

    if (!gFls_State.initialized) {
        return;
    }

    prevStatus = gFls_State.currentJob.status;

    /* Start pending job */
    if (gFls_State.currentJob.status == FLS_JOB_PENDING) {
        gFls_State.currentJob.status = FLS_JOB_IN_PROGRESS;
        gFls_State.currentJob.timestamp = 0u; /* TODO: Get system time */
    }

    /* Process current job */
    if (gFls_State.currentJob.status == FLS_JOB_IN_PROGRESS) {
        result = Fls_ProcessJob();

        if (result == FLS_OK) {
            gFls_State.currentJob.status = FLS_JOB_COMPLETED;
            gFls_State.status = FLS_IDLE;
        } else if ((result != FLS_E_BUSY) && (result != FLS_OK)) {
            gFls_State.currentJob.status = FLS_JOB_FAILED;
            gFls_State.status = FLS_IDLE;
            gFls_State.errorCount++;
        }
    }

    /* Notify on state change */
    if (prevStatus != gFls_State.currentJob.status) {
        if (gFls_State.currentJob.status == FLS_JOB_COMPLETED) {
            Fls_NotifyJobEnd();
        } else if (gFls_State.currentJob.status == FLS_JOB_FAILED) {
            Fls_NotifyJobError();
        }
    }
}

/*============================================================================*
 * Sector/Block Management
 *============================================================================*/

uint32_t Fls_GetSectorNumber(Fls_AddressType address)
{
    if (!gFls_State.initialized) {
        return 0xFFFFFFFFu;
    }

    return Fls_FindSectorIndex(address);
}

uint32_t Fls_GetSectorSize(uint32_t sector)
{
    const Fls_SectorConfigType* sectors;

    if (!gFls_State.initialized) {
        return 0u;
    }

    if (gFls_State.config == NULL) {
        return 0u;
    }

    if (sector >= gFls_State.config->sectorCount) {
        return 0u;
    }

    sectors = gFls_State.config->sectors;
    if (sectors == NULL) {
        return 0u;
    }

    return sectors[sector].size;
}

uint32_t Fls_GetPageSize(uint32_t sector)
{
    const Fls_SectorConfigType* sectors;

    if (!gFls_State.initialized) {
        return 0u;
    }

    if (gFls_State.config == NULL) {
        return 0u;
    }

    if (sector >= gFls_State.config->sectorCount) {
        return 0u;
    }

    sectors = gFls_State.config->sectors;
    if (sectors == NULL) {
        return 0u;
    }

    return sectors[sector].pageSize;
}

uint32_t Fls_GetNumSectors(void)
{
    if (!gFls_State.initialized) {
        return 0u;
    }

    if (gFls_State.config == NULL) {
        return 0u;
    }

    return gFls_State.config->sectorCount;
}

Fls_AddressType Fls_GetSectorStartAddress(uint32_t sector)
{
    const Fls_SectorConfigType* sectors;

    if (!gFls_State.initialized) {
        return 0u;
    }

    if (gFls_State.config == NULL) {
        return 0u;
    }

    if (sector >= gFls_State.config->sectorCount) {
        return 0u;
    }

    sectors = gFls_State.config->sectors;
    if (sectors == NULL) {
        return 0u;
    }

    return sectors[sector].startAddress;
}

/*============================================================================*
 * Protection/Security
 *============================================================================*/

Fls_ErrorCode_t Fls_SetProtection(
    uint32_t sector,
    Fls_ProtectionType_t protection
)
{
    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_State.config == NULL) {
        return FLS_E_NOT_OK;
    }

    if (sector >= gFls_State.config->sectorCount) {
        return FLS_E_INVALID_ADDRESS;
    }

    if (gFls_HwInterface != NULL) {
        if (gFls_HwInterface->SetProtection != NULL) {
            return gFls_HwInterface->SetProtection(sector, protection);
        }
    }

    /* Software protection */
    if (gFls_State.config->sectors != NULL) {
        ((Fls_SectorConfigType*)&gFls_State.config->sectors[sector])->protection = protection;
        return FLS_OK;
    }

    return FLS_E_NOT_OK;
}

Fls_ProtectionType_t Fls_GetProtection(uint32_t sector)
{
    const Fls_SectorConfigType* sectors;

    if (!gFls_State.initialized) {
        return FLS_PROTECTION_NONE;
    }

    if (gFls_State.config == NULL) {
        return FLS_PROTECTION_NONE;
    }

    if (sector >= gFls_State.config->sectorCount) {
        return FLS_PROTECTION_NONE;
    }

    sectors = gFls_State.config->sectors;
    if (sectors == NULL) {
        return FLS_PROTECTION_NONE;
    }

    return sectors[sector].protection;
}

Fls_ErrorCode_t Fls_Lock(uint32_t sector)
{
    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_HwInterface != NULL) {
        if (gFls_HwInterface->Lock != NULL) {
            return gFls_HwInterface->Lock(sector);
        }
    }

    return Fls_SetProtection(sector, FLS_PROTECTION_WRITE);
}

Fls_ErrorCode_t Fls_Unlock(uint32_t sector)
{
    if (!gFls_State.initialized) {
        return FLS_E_UNINIT;
    }

    if (gFls_HwInterface != NULL) {
        if (gFls_HwInterface->Unlock != NULL) {
            return gFls_HwInterface->Unlock(sector);
        }
    }

    return Fls_SetProtection(sector, FLS_PROTECTION_NONE);
}

/*============================================================================*
 * Wear Leveling
 *============================================================================*/

uint32_t Fls_GetEraseCount(uint32_t sector)
{
    const Fls_SectorConfigType* sectors;

    if (!gFls_State.initialized) {
        return 0u;
    }

    if (gFls_State.config == NULL) {
        return 0u;
    }

    if (sector >= gFls_State.config->sectorCount) {
        return 0u;
    }

    sectors = gFls_State.config->sectors;
    if (sectors == NULL) {
        return 0u;
    }

    return sectors[sector].eraseCount;
}

uint32_t Fls_GetLeastWornSector(void)
{
    const Fls_SectorConfigType* sectors;
    uint32_t sectorCount;
    uint32_t i;
    uint32_t minCount;
    uint32_t minSector;

    if (!gFls_State.initialized) {
        return 0xFFFFFFFFu;
    }

    if (gFls_State.config == NULL) {
        return 0xFFFFFFFFu;
    }

    sectors = gFls_State.config->sectors;
    sectorCount = gFls_State.config->sectorCount;

    if ((sectors == NULL) || (sectorCount == 0u)) {
        return 0xFFFFFFFFu;
    }

    minCount = 0xFFFFFFFFu;
    minSector = 0xFFFFFFFFu;

    for (i = 0u; i < sectorCount; i++) {
        if (sectors[i].eraseCount < minCount) {
            minCount = sectors[i].eraseCount;
            minSector = i;
        }
    }

    return minSector;
}

uint32_t Fls_GetMostWornSector(void)
{
    const Fls_SectorConfigType* sectors;
    uint32_t sectorCount;
    uint32_t i;
    uint32_t maxCount;
    uint32_t maxSector;

    if (!gFls_State.initialized) {
        return 0xFFFFFFFFu;
    }

    if (gFls_State.config == NULL) {
        return 0xFFFFFFFFu;
    }

    sectors = gFls_State.config->sectors;
    sectorCount = gFls_State.config->sectorCount;

    if ((sectors == NULL) || (sectorCount == 0u)) {
        return 0xFFFFFFFFu;
    }

    maxCount = 0u;
    maxSector = 0xFFFFFFFFu;

    for (i = 0u; i < sectorCount; i++) {
        if (sectors[i].eraseCount > maxCount) {
            maxCount = sectors[i].eraseCount;
            maxSector = i;
        }
    }

    return maxSector;
}

/*============================================================================*
 * Notification Callbacks
 *============================================================================*/

void Fls_SetNotificationCallback(Fls_NotificationCallback_t callback)
{
    gFls_State.notificationCb = callback;
}

__attribute__((weak)) void Fls_JobEndNotification(void)
{
    /* Application can override this weak function */
}

__attribute__((weak)) void Fls_JobErrorNotification(void)
{
    /* Application can override this weak function */
}

/*============================================================================*
 * Version/Info API
 *============================================================================*/

Fls_ErrorCode_t Fls_GetVersionInfo(
    uint8_t* major,
    uint8_t* minor,
    uint8_t* patch
)
{
    if ((major == NULL) || (minor == NULL) || (patch == NULL)) {
        return FLS_E_PARAM_POINTER;
    }

    *major = FLS_MAJOR_VERSION;
    *minor = FLS_MINOR_VERSION;
    *patch = FLS_PATCH_VERSION;

    return FLS_OK;
}

const Fls_RuntimeStateType* Fls_GetRuntimeState(void)
{
    return &gFls_State;
}

/*============================================================================*
 * Hardware Interface Registration
 *============================================================================*/

Fls_ErrorCode_t Fls_RegisterHwInterface(
    const Fls_HwInterfaceType* hwInterface
)
{
    if (hwInterface == NULL) {
        return FLS_E_PARAM_POINTER;
    }

    gFls_HwInterface = hwInterface;

    return FLS_OK;
}

const Fls_HwInterfaceType* Fls_GetHwInterface(void)
{
    return gFls_HwInterface;
}
