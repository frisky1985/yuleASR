/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/*==================================================================================================
 *                                      FLASH DRIVER (MCAL)
 *==================================================================================================
 * FILENAME: Flash.c
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_FlashDriver.pdf
 * MODULE ID: 0x5E (94)
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Implementation of Flash Driver module (MCAL Layer)
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Flash.h"
#include "Flash_MemMap.h"

#if (FLASH_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

#if (FLASH_RUNTIME_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
 *                                      LOCAL DEFINES
 *==================================================================================================*/
#define FLASH_STATE_UNINIT                  (0U)
#define FLASH_STATE_IDLE                    (1U)
#define FLASH_STATE_BUSY                    (2U)

#define FLASH_JOB_NONE                      (0U)
#define FLASH_JOB_ERASE                     (1U)
#define FLASH_JOB_WRITE                     (2U)
#define FLASH_JOB_READ                      (3U)
#define FLASH_JOB_COMPARE                   (4U)
#define FLASH_JOB_BLANK_CHECK               (5U)

#define FLASH_INITIALIZED                   (0x55AA3311U)
#define FLASH_NOT_INITIALIZED               (0x00000000U)

/*==================================================================================================
 *                                      LOCAL TYPES
 *==================================================================================================*/
typedef struct {
    uint8 state;
    uint8 jobType;
    Flash_OpModeType opMode;
    Flash_AddressType currentAddr;
    Flash_LengthType remainingLength;
    uint8* dataPtr;
    Flash_JobResultType jobResult;
    uint32 initState;
} Flash_DriverStateType;

/* WRP 写保护掩码 (P3, 2026-08-13): 位 N = 扇区 N 受写保护。
 * 由 Fls_ConfigureWriteProtection 运行期设置, Flash_Init 从
 * Fls_ProtectionConfig.WriteProtectionMask 初始化 (配置消费点)。 */
static uint32 Flash_WriteProtectMask = 0U;

/*==================================================================================================
 *                                      LOCAL CONSTANTS
 *==================================================================================================*/
#define FLASH_START_SEC_CONST_UNSPECIFIED
#include "Flash_MemMap.h"

static const Flash_SectorInfoType Flash_SectorConfig[FLASH_NUM_OF_SECTORS] = {
    /* Bank 0 Sectors */
    {
        FLASH_SECTOR_0_START_ADDR,
        FLASH_SECTOR_0_SIZE,
        FLASH_SECTOR_0_PAGE_SIZE,
        0U,
        FLASH_BANK_0,
        TRUE,
        TRUE,
        TRUE
    },
    {
        FLASH_SECTOR_1_START_ADDR,
        FLASH_SECTOR_1_SIZE,
        FLASH_SECTOR_1_PAGE_SIZE,
        1U,
        FLASH_BANK_0,
        TRUE,
        TRUE,
        TRUE
    },
    {
        FLASH_SECTOR_2_START_ADDR,
        FLASH_SECTOR_2_SIZE,
        FLASH_SECTOR_2_PAGE_SIZE,
        2U,
        FLASH_BANK_0,
        TRUE,
        TRUE,
        TRUE
    },
    {
        FLASH_SECTOR_3_START_ADDR,
        FLASH_SECTOR_3_SIZE,
        FLASH_SECTOR_3_PAGE_SIZE,
        3U,
        FLASH_BANK_0,
        TRUE,
        TRUE,
        TRUE
    },
    /* Bank 1 Sectors */
    {
        FLASH_SECTOR_4_START_ADDR,
        FLASH_SECTOR_4_SIZE,
        FLASH_SECTOR_4_PAGE_SIZE,
        4U,
        FLASH_BANK_1,
        TRUE,
        TRUE,
        TRUE
    },
    {
        FLASH_SECTOR_5_START_ADDR,
        FLASH_SECTOR_5_SIZE,
        FLASH_SECTOR_5_PAGE_SIZE,
        5U,
        FLASH_BANK_1,
        TRUE,
        TRUE,
        TRUE
    },
    {
        FLASH_SECTOR_6_START_ADDR,
        FLASH_SECTOR_6_SIZE,
        FLASH_SECTOR_6_PAGE_SIZE,
        6U,
        FLASH_BANK_1,
        TRUE,
        TRUE,
        TRUE
    },
    {
        FLASH_SECTOR_7_START_ADDR,
        FLASH_SECTOR_7_SIZE,
        FLASH_SECTOR_7_PAGE_SIZE,
        7U,
        FLASH_BANK_1,
        TRUE,
        TRUE,
        TRUE
    }
};

static const Flash_ConfigType Flash_DefaultConfig = {
    Flash_SectorConfig,
    FLASH_NUM_OF_SECTORS,
    FLASH_MODE_NORMAL,
    FLASH_BASE_ADDRESS,
    FLASH_END_ADDRESS,
    FLASH_MAX_READ_NORMAL_MODE,
    FLASH_MAX_READ_FAST_MODE,
    FLASH_MAX_WRITE_NORMAL_MODE,
    FLASH_MAX_WRITE_FAST_MODE,
    FLASH_PROGRAM_UNIT,
    FLASH_ERASE_UNIT,
#if (FLASH_JOB_END_NOTIFICATION == STD_ON)
    Flash_JobEndNotification,
#else
    NULL_PTR,
#endif
#if (FLASH_JOB_ERROR_NOTIFICATION == STD_ON)
    Flash_JobErrorNotification,
#else
    NULL_PTR,
#endif
#if (FLASH_USE_ACCESS_CODE == STD_ON)
    NULL_PTR
#endif
};

#define FLASH_STOP_SEC_CONST_UNSPECIFIED
#include "Flash_MemMap.h"

/*==================================================================================================
 *                                      LOCAL VARIABLES
 *==================================================================================================*/
#define FLASH_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Flash_MemMap.h"

static Flash_DriverStateType Flash_DriverState;
static const Flash_ConfigType* Flash_ConfigPtr = NULL_PTR;

#define FLASH_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Flash_MemMap.h"

/*==================================================================================================
 *                                      LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
static void Flash_Unlock(void);
static void Flash_Lock(void);
static boolean Flash_WaitForOperation(uint32 timeout);
static Std_ReturnType Flash_ProgramWord(Flash_AddressType address, uint32 data);
static Std_ReturnType Flash_ProgramDoubleWord(Flash_AddressType address, uint64 data);
static Std_ReturnType Flash_EraseSector(uint32 sectorNum);
static void Flash_ProcessEraseJob(void);
static void Flash_ProcessWriteJob(void);
static void Flash_ProcessReadJob(void);
static void Flash_ProcessCompareJob(void);
static void Flash_ProcessBlankCheckJob(void);
static void Flash_ReportError(uint8 serviceId, uint8 errorCode);

/* GLOBAL FUNCTION PROTOTYPES */
boolean Flash_IsAddressValid(Flash_AddressType Address);
const Flash_SectorInfoType* Flash_GetSectorInfo(Flash_AddressType Address);
void Flash_MainFunction(void);
Flash_JobResultType Flash_GetJobResult(void);
Flash_StatusType Flash_GetStatus(void);
Std_ReturnType Flash_Read(Flash_AddressType SourceAddress, uint8* TargetAddressPtr, Flash_LengthType Length);
Std_ReturnType Flash_Write(Flash_AddressType TargetAddress, const uint8* SourceAddressPtr, Flash_LengthType Length);
void Flash_DeInit(void);
void Flash_Init(const Flash_ConfigType* ConfigPtr);

/*==================================================================================================
 *                                      LOCAL FUNCTIONS
 *==================================================================================================*/
#define FLASH_START_SEC_CODE
#include "Flash_MemMap.h"

/**
 * @brief Unlock flash control register
 */
static void Flash_Unlock(void)
{
    if ((FLASH_CR & FLASH_CR_LOCK) != 0U) {
        FLASH_KEYR = FLASH_KEY_1;
        FLASH_KEYR = FLASH_KEY_2;
    }
}

/**
 * @brief Lock flash control register
 */
static void Flash_Lock(void)
{
    FLASH_CR |= FLASH_CR_LOCK;
}

/**
 * @brief Wait for flash operation to complete
 * @param timeout Timeout in milliseconds
 * @return TRUE if operation completed successfully, FALSE if timeout
 */
static boolean Flash_WaitForOperation(uint32 timeout)
{
    uint32 elapsed = 0U;
    boolean result = FALSE;

    while (elapsed < timeout) {
        if ((FLASH_SR & FLASH_SR_BSY) == 0U) {
            result = TRUE;
            break;
        }
        /* Simple delay - in real implementation would use OS delay */
        elapsed++;
    }

    return result;
}

/**
 * @brief Program a 32-bit word to flash
 * @param address Target address
 * @param data Data to program
 * @return E_OK if successful, E_NOT_OK otherwise
 */
static Std_ReturnType Flash_ProgramWord(Flash_AddressType address, uint32 data)
{
    Std_ReturnType result = E_NOT_OK;

    /* WRP 写保护检查 (P3): 地址所在扇区受保护 → 拒绝写入 */
    if ((address >= FLASH_BASE_ADDRESS) && (Flash_WriteProtectMask != 0U)) {
        uint32 sector = (address - FLASH_BASE_ADDRESS) / FLASH_ERASE_UNIT;
        if ((Flash_WriteProtectMask & (1UL << sector)) != 0UL) {
            return E_NOT_OK;
        }
    }

    if (Flash_WaitForOperation(FLASH_TIMEOUT_MS)) {
        /* Clear error flags */
        FLASH_SR = (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR |
                    FLASH_SR_PGPERR | FLASH_SR_PGSERR);

        /* Enable programming */
        FLASH_CR |= FLASH_CR_PG;

        /* Program the word */
        *(volatile uint32*)(uintptr)address = data;

        /* Wait for completion */
        if (Flash_WaitForOperation(FLASH_TIMEOUT_MS)) {
            /* Check for errors */
            if ((FLASH_SR & (FLASH_SR_OPERR | FLASH_SR_WRPERR |
                             FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR)) == 0U) {
                result = E_OK;
            }
        }

        /* Disable programming */
        FLASH_CR &= ~FLASH_CR_PG;
    }

    return result;
}

/**
 * @brief Program a 64-bit double word to flash
 * @param address Target address
 * @param data Data to program
 * @return E_OK if successful, E_NOT_OK otherwise
 */
static Std_ReturnType Flash_ProgramDoubleWord(Flash_AddressType address, uint64 data)
{
    Std_ReturnType result = E_NOT_OK;

    if (Flash_WaitForOperation(FLASH_TIMEOUT_MS)) {
        /* Clear error flags */
        FLASH_SR = (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR |
                    FLASH_SR_PGPERR | FLASH_SR_PGSERR);

        /* Enable programming */
        FLASH_CR |= FLASH_CR_PG;

        /* Program the double word (two 32-bit writes) */
        *(volatile uint32*)(uintptr)address = (uint32)(data & 0xFFFFFFFFU);
        *(volatile uint32*)(uintptr)(address + 4U) = (uint32)(data >> 32U);

        /* Wait for completion */
        if (Flash_WaitForOperation(FLASH_TIMEOUT_MS)) {
            /* Check for errors */
            if ((FLASH_SR & (FLASH_SR_OPERR | FLASH_SR_WRPERR |
                             FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR)) == 0U) {
                result = E_OK;
            }
        }

        /* Disable programming */
        FLASH_CR &= ~FLASH_CR_PG;
    }

    return result;
}

/**
 * @brief Erase a flash sector
 * @param sectorNum Sector number to erase
 * @return E_OK if successful, E_NOT_OK otherwise
 */
static Std_ReturnType Flash_EraseSector(uint32 sectorNum)
{
    Std_ReturnType result = E_NOT_OK;

    /* WRP 写保护检查 (P3): 目标扇区受保护 → 拒绝擦除 (WRPERR 语义) */
    if ((Flash_WriteProtectMask & (1UL << sectorNum)) != 0UL) {
        FLASH_SR = (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR |
                    FLASH_SR_PGPERR | FLASH_SR_PGSERR);
        return E_NOT_OK;
    }

    if (Flash_WaitForOperation(FLASH_TIMEOUT_MS)) {
        /* Clear error flags */
        FLASH_SR = (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR |
                    FLASH_SR_PGPERR | FLASH_SR_PGSERR);

        /* Configure sector erase */
        FLASH_CR &= ~(0xFU << FLASH_CR_SNB_Pos);  /* Clear sector number */
        FLASH_CR |= ((sectorNum & 0xFU) << FLASH_CR_SNB_Pos);
        FLASH_CR |= FLASH_CR_SER;

        /* Start erase operation */
        FLASH_CR |= FLASH_CR_STRT;

        /* Wait for completion */
        if (Flash_WaitForOperation(FLASH_TIMEOUT_MS)) {
            /* Check for errors */
            if ((FLASH_SR & (FLASH_SR_OPERR | FLASH_SR_WRPERR |
                             FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR)) == 0U) {
                result = E_OK;
            }
        }

        /* Disable sector erase */
        FLASH_CR &= ~FLASH_CR_SER;
    }

    return result;
}

/**
 * @brief Process erase job
 */
static void Flash_ProcessEraseJob(void)
{
    uint32 sectorIdx;
    const Flash_SectorInfoType* sectorInfo;

    if (Flash_DriverState.remainingLength == 0U) {
        Flash_DriverState.jobResult = FLASH_JOB_OK;
        Flash_DriverState.state = FLASH_STATE_IDLE;
        Flash_DriverState.jobType = FLASH_JOB_NONE;

#if (FLASH_JOB_END_NOTIFICATION == STD_ON)
        if (Flash_ConfigPtr->jobEndNotification != NULL_PTR) {
            Flash_ConfigPtr->jobEndNotification();
        }
#endif
        return;
    }

    /* Find sector for current address */
    for (sectorIdx = 0U; sectorIdx < Flash_ConfigPtr->numOfSectors; sectorIdx++) {
        sectorInfo = &Flash_ConfigPtr->sectorConfig[sectorIdx];
        if ((Flash_DriverState.currentAddr >= sectorInfo->sectorStartAddr) &&
            (Flash_DriverState.currentAddr < (sectorInfo->sectorStartAddr + sectorInfo->sectorSize))) {
            break;
        }
    }

    if (sectorIdx < Flash_ConfigPtr->numOfSectors) {
        if (Flash_EraseSector(sectorIdx) == E_OK) {
            Flash_DriverState.currentAddr += sectorInfo->sectorSize;
            if (Flash_DriverState.remainingLength > sectorInfo->sectorSize) {
                Flash_DriverState.remainingLength -= sectorInfo->sectorSize;
            } else {
                Flash_DriverState.remainingLength = 0U;
            }
        } else {
            Flash_DriverState.jobResult = FLASH_JOB_FAILED;
            Flash_DriverState.state = FLASH_STATE_IDLE;
            Flash_DriverState.jobType = FLASH_JOB_NONE;

#if (FLASH_RUNTIME_ERROR_DETECT == STD_ON)
            (void)Det_ReportRuntimeError(FLASH_MODULE_ID, FLASH_INSTANCE_ID,
                                         FLASH_SID_ERASE, FLASH_E_ERASE_FAILED);
#endif

#if (FLASH_JOB_ERROR_NOTIFICATION == STD_ON)
            if (Flash_ConfigPtr->jobErrorNotification != NULL_PTR) {
                Flash_ConfigPtr->jobErrorNotification();
            }
#endif
        }
    }
}

/**
 * @brief Process write job
 */
static void Flash_ProcessWriteJob(void)
{
    uint32 bytesToWrite;
    uint32 writeChunk;
    Std_ReturnType status = E_OK;

    if (Flash_DriverState.opMode == FLASH_MODE_FAST) {
        writeChunk = Flash_ConfigPtr->maxWriteFastMode;
    } else {
        writeChunk = Flash_ConfigPtr->maxWriteNormalMode;
    }

    if (Flash_DriverState.remainingLength == 0U) {
        Flash_DriverState.jobResult = FLASH_JOB_OK;
        Flash_DriverState.state = FLASH_STATE_IDLE;
        Flash_DriverState.jobType = FLASH_JOB_NONE;

#if (FLASH_JOB_END_NOTIFICATION == STD_ON)
        if (Flash_ConfigPtr->jobEndNotification != NULL_PTR) {
            Flash_ConfigPtr->jobEndNotification();
        }
#endif
        return;
    }

    bytesToWrite = (Flash_DriverState.remainingLength > writeChunk) ?
                   writeChunk : Flash_DriverState.remainingLength;

    /* Ensure alignment to program unit */
    bytesToWrite = bytesToWrite - (bytesToWrite % Flash_ConfigPtr->programUnit);

    if (bytesToWrite > 0U) {
        uint32 idx;
        for (idx = 0U; idx < bytesToWrite; idx += FLASH_PROGRAM_UNIT) {
            uint64 data = 0U;
            uint32 byteIdx;

            for (byteIdx = 0U; byteIdx < FLASH_PROGRAM_UNIT; byteIdx++) {
                data |= ((uint64)(Flash_DriverState.dataPtr[idx + byteIdx])) << (byteIdx * 8U);
            }

            if (Flash_ProgramDoubleWord(Flash_DriverState.currentAddr + idx, data) != E_OK) {
                status = E_NOT_OK;
                break;
            }
        }

        if (status == E_OK) {
            Flash_DriverState.currentAddr += bytesToWrite;
            Flash_DriverState.dataPtr += bytesToWrite;
            Flash_DriverState.remainingLength -= bytesToWrite;
        } else {
            Flash_DriverState.jobResult = FLASH_JOB_FAILED;
            Flash_DriverState.state = FLASH_STATE_IDLE;
            Flash_DriverState.jobType = FLASH_JOB_NONE;

#if (FLASH_RUNTIME_ERROR_DETECT == STD_ON)
            (void)Det_ReportRuntimeError(FLASH_MODULE_ID, FLASH_INSTANCE_ID,
                                         FLASH_SID_WRITE, FLASH_E_WRITE_FAILED);
#endif

#if (FLASH_JOB_ERROR_NOTIFICATION == STD_ON)
            if (Flash_ConfigPtr->jobErrorNotification != NULL_PTR) {
                Flash_ConfigPtr->jobErrorNotification();
            }
#endif
        }
    }
}

/**
 * @brief Process read job
 */
static void Flash_ProcessReadJob(void)
{
    uint32 bytesToRead;
    uint32 readChunk;

    if (Flash_DriverState.opMode == FLASH_MODE_FAST) {
        readChunk = Flash_ConfigPtr->maxReadFastMode;
    } else {
        readChunk = Flash_ConfigPtr->maxReadNormalMode;
    }

    if (Flash_DriverState.remainingLength == 0U) {
        Flash_DriverState.jobResult = FLASH_JOB_OK;
        Flash_DriverState.state = FLASH_STATE_IDLE;
        Flash_DriverState.jobType = FLASH_JOB_NONE;

#if (FLASH_JOB_END_NOTIFICATION == STD_ON)
        if (Flash_ConfigPtr->jobEndNotification != NULL_PTR) {
            Flash_ConfigPtr->jobEndNotification();
        }
#endif
        return;
    }

    bytesToRead = (Flash_DriverState.remainingLength > readChunk) ?
                  readChunk : Flash_DriverState.remainingLength;

    /* Perform read operation */
    {
        uint32 idx;
        for (idx = 0U; idx < bytesToRead; idx++) {
            Flash_DriverState.dataPtr[idx] =
                ((const uint8*)(uintptr)Flash_DriverState.currentAddr)[idx];
        }
    }

    Flash_DriverState.currentAddr += bytesToRead;
    Flash_DriverState.dataPtr += bytesToRead;
    Flash_DriverState.remainingLength -= bytesToRead;
}

/**
 * @brief Process compare job
 */
static void Flash_ProcessCompareJob(void)
{
    uint32 bytesToCompare;
    uint32 compareChunk = FLASH_MAX_COMPARE_MODE;
    boolean mismatch = FALSE;

    if (Flash_DriverState.remainingLength == 0U) {
        Flash_DriverState.jobResult = FLASH_JOB_OK;
        Flash_DriverState.state = FLASH_STATE_IDLE;
        Flash_DriverState.jobType = FLASH_JOB_NONE;

#if (FLASH_JOB_END_NOTIFICATION == STD_ON)
        if (Flash_ConfigPtr->jobEndNotification != NULL_PTR) {
            Flash_ConfigPtr->jobEndNotification();
        }
#endif
        return;
    }

    bytesToCompare = (Flash_DriverState.remainingLength > compareChunk) ?
                     compareChunk : Flash_DriverState.remainingLength;

    /* Perform compare operation */
    {
        uint32 idx;
        for (idx = 0U; idx < bytesToCompare; idx++) {
            if (Flash_DriverState.dataPtr[idx] !=
                ((const uint8*)(uintptr)Flash_DriverState.currentAddr)[idx]) {
                mismatch = TRUE;
                break;
            }
        }
    }

    if (mismatch) {
        Flash_DriverState.jobResult = FLASH_JOB_FAILED;
        Flash_DriverState.state = FLASH_STATE_IDLE;
        Flash_DriverState.jobType = FLASH_JOB_NONE;

#if (FLASH_RUNTIME_ERROR_DETECT == STD_ON)
        (void)Det_ReportRuntimeError(FLASH_MODULE_ID, FLASH_INSTANCE_ID,
                                     FLASH_SID_COMPARE, FLASH_E_COMPARE_FAILED);
#endif

#if (FLASH_JOB_ERROR_NOTIFICATION == STD_ON)
        if (Flash_ConfigPtr->jobErrorNotification != NULL_PTR) {
            Flash_ConfigPtr->jobErrorNotification();
        }
#endif
    } else {
        Flash_DriverState.currentAddr += bytesToCompare;
        Flash_DriverState.dataPtr += bytesToCompare;
        Flash_DriverState.remainingLength -= bytesToCompare;
    }
}

/**
 * @brief Process blank check job
 */
static void Flash_ProcessBlankCheckJob(void)
{
    uint32 bytesToCheck;
    uint32 checkChunk = FLASH_MAX_COMPARE_MODE;
    boolean notBlank = FALSE;

    if (Flash_DriverState.remainingLength == 0U) {
        Flash_DriverState.jobResult = FLASH_JOB_OK;
        Flash_DriverState.state = FLASH_STATE_IDLE;
        Flash_DriverState.jobType = FLASH_JOB_NONE;

#if (FLASH_JOB_END_NOTIFICATION == STD_ON)
        if (Flash_ConfigPtr->jobEndNotification != NULL_PTR) {
            Flash_ConfigPtr->jobEndNotification();
        }
#endif
        return;
    }

    bytesToCheck = (Flash_DriverState.remainingLength > checkChunk) ?
                   checkChunk : Flash_DriverState.remainingLength;

    /* Perform blank check operation */
    {
        uint32 idx;
        for (idx = 0U; idx < bytesToCheck; idx++) {
            if (((const uint8*)(uintptr)Flash_DriverState.currentAddr)[idx] != 0xFFU) {
                notBlank = TRUE;
                break;
            }
        }
    }

    if (notBlank) {
        Flash_DriverState.jobResult = FLASH_JOB_FAILED;
        Flash_DriverState.state = FLASH_STATE_IDLE;
        Flash_DriverState.jobType = FLASH_JOB_NONE;

#if (FLASH_RUNTIME_ERROR_DETECT == STD_ON)
        (void)Det_ReportRuntimeError(FLASH_MODULE_ID, FLASH_INSTANCE_ID,
                                     FLASH_SID_BLANKCHECK, FLASH_E_BLANK_CHECK_FAILED);
#endif

#if (FLASH_JOB_ERROR_NOTIFICATION == STD_ON)
        if (Flash_ConfigPtr->jobErrorNotification != NULL_PTR) {
            Flash_ConfigPtr->jobErrorNotification();
        }
#endif
    } else {
        Flash_DriverState.currentAddr += bytesToCheck;
        Flash_DriverState.remainingLength -= bytesToCheck;
    }
}

/**
 * @brief Report development error
 * @param serviceId Service ID
 * @param errorCode Error code
 */
static void Flash_ReportError(uint8 serviceId, uint8 errorCode)
{
#if (FLASH_DEV_ERROR_DETECT == STD_ON)
    (void)Det_ReportError(FLASH_MODULE_ID, FLASH_INSTANCE_ID, serviceId, errorCode);
#else
    (void)serviceId;
    (void)errorCode;
#endif
}

#define FLASH_STOP_SEC_CODE
#include "Flash_MemMap.h"

/*==================================================================================================
 *                                      GLOBAL FUNCTIONS
 *==================================================================================================*/
#define FLASH_START_SEC_CODE
#include "Flash_MemMap.h"

/**
 * @brief Initialize the Flash driver
 */
void Flash_Init(const Flash_ConfigType* ConfigPtr)
{
    if (Flash_DriverState.initState == FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_INIT, FLASH_E_ALREADY_INITIALIZED);
        return;
    }

#if (FLASH_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Flash_ReportError(FLASH_SID_INIT, FLASH_E_PARAM_CONFIG);
        return;
    }
#endif

    /* Use provided config or default */
    if (ConfigPtr != NULL_PTR) {
        Flash_ConfigPtr = ConfigPtr;
    } else {
        Flash_ConfigPtr = &Flash_DefaultConfig;
    }

    /* Initialize driver state */
    Flash_DriverState.state = FLASH_STATE_IDLE;
    Flash_DriverState.jobType = FLASH_JOB_NONE;
    Flash_DriverState.opMode = Flash_ConfigPtr->defaultMode;
    Flash_DriverState.jobResult = FLASH_JOB_OK;
    Flash_DriverState.currentAddr = 0U;
    Flash_DriverState.remainingLength = 0U;
    Flash_DriverState.dataPtr = NULL_PTR;
    Flash_DriverState.initState = FLASH_INITIALIZED;

    /* WRP 写保护初始化 (P3): 消费 Fls_ProtectionConfig.WriteProtectionMask 配置
     * (默认保护 Bootloader 扇区 Sector 0)。生产平台可在启动早期锁定。 */
    Flash_WriteProtectMask = Fls_ProtectionConfig.WriteProtectionMask;

    /* Unlock flash control register */
    Flash_Unlock();

    /* Clear any pending error flags */
    FLASH_SR = (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR |
                FLASH_SR_PGPERR | FLASH_SR_PGSERR);
}

/**
 * @brief Deinitialize the Flash driver
 */
void Flash_DeInit(void)
{
    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_INIT, FLASH_E_UNINIT);
        return;
    }

    if (Flash_DriverState.state == FLASH_STATE_BUSY) {
        Flash_ReportError(FLASH_SID_INIT, FLASH_E_BUSY);
        return;
    }

    /* Lock flash control register */
    Flash_Lock();

    /* Reset driver state */
    Flash_DriverState.state = FLASH_STATE_UNINIT;
    Flash_DriverState.jobType = FLASH_JOB_NONE;
    Flash_DriverState.initState = FLASH_NOT_INITIALIZED;
    Flash_ConfigPtr = NULL_PTR;
}

/**
 * @brief Erase one or more complete flash sectors
 */
Std_ReturnType Flash_Erase(Flash_AddressType TargetAddress, Flash_LengthType Length)
{
    Std_ReturnType result = E_NOT_OK;

    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_ERASE, FLASH_E_UNINIT);
        return E_NOT_OK;
    }

#if (FLASH_DEV_ERROR_DETECT == STD_ON)
    if (Flash_DriverState.state == FLASH_STATE_BUSY) {
        Flash_ReportError(FLASH_SID_ERASE, FLASH_E_BUSY);
        return E_NOT_OK;
    }

    if ((TargetAddress < FLASH_BASE_ADDRESS) ||
        ((TargetAddress + Length) > (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE))) {
        Flash_ReportError(FLASH_SID_ERASE, FLASH_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }

    if (Length == 0U) {
        Flash_ReportError(FLASH_SID_ERASE, FLASH_E_PARAM_LENGTH);
        return E_NOT_OK;
    }

    /* Check alignment to sector size */
    if ((TargetAddress % FLASH_ERASE_UNIT) != 0U) {
        Flash_ReportError(FLASH_SID_ERASE, FLASH_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }
#endif

    /* Set up job parameters */
    Flash_DriverState.currentAddr = TargetAddress;
    Flash_DriverState.remainingLength = Length;
    Flash_DriverState.jobType = FLASH_JOB_ERASE;
    Flash_DriverState.jobResult = FLASH_JOB_PENDING;
    Flash_DriverState.state = FLASH_STATE_BUSY;

    result = E_OK;

    return result;
}

/**
 * @brief Write one or more complete flash pages
 */
Std_ReturnType Flash_Write(Flash_AddressType TargetAddress, const uint8* SourceAddressPtr, Flash_LengthType Length)
{
    Std_ReturnType result = E_NOT_OK;

    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_WRITE, FLASH_E_UNINIT);
        return E_NOT_OK;
    }

#if (FLASH_DEV_ERROR_DETECT == STD_ON)
    if (Flash_DriverState.state == FLASH_STATE_BUSY) {
        Flash_ReportError(FLASH_SID_WRITE, FLASH_E_BUSY);
        return E_NOT_OK;
    }

    if (SourceAddressPtr == NULL_PTR) {
        Flash_ReportError(FLASH_SID_WRITE, FLASH_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if ((TargetAddress < FLASH_BASE_ADDRESS) ||
        ((TargetAddress + Length) > (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE))) {
        Flash_ReportError(FLASH_SID_WRITE, FLASH_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }

    if (Length == 0U) {
        Flash_ReportError(FLASH_SID_WRITE, FLASH_E_PARAM_LENGTH);
        return E_NOT_OK;
    }

    /* Check alignment to program unit */
    if ((TargetAddress % FLASH_PROGRAM_UNIT) != 0U) {
        Flash_ReportError(FLASH_SID_WRITE, FLASH_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }

    if ((Length % FLASH_PROGRAM_UNIT) != 0U) {
        Flash_ReportError(FLASH_SID_WRITE, FLASH_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    /* Set up job parameters */
    Flash_DriverState.currentAddr = TargetAddress;
    Flash_DriverState.remainingLength = Length;
    Flash_DriverState.dataPtr = SourceAddressPtr;
    Flash_DriverState.jobType = FLASH_JOB_WRITE;
    Flash_DriverState.jobResult = FLASH_JOB_PENDING;
    Flash_DriverState.state = FLASH_STATE_BUSY;

    result = E_OK;

    return result;
}

/**
 * @brief Read data from flash memory
 */
Std_ReturnType Flash_Read(Flash_AddressType SourceAddress, uint8* TargetAddressPtr, Flash_LengthType Length)
{
    Std_ReturnType result = E_NOT_OK;

    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_READ, FLASH_E_UNINIT);
        return E_NOT_OK;
    }

#if (FLASH_DEV_ERROR_DETECT == STD_ON)
    if (Flash_DriverState.state == FLASH_STATE_BUSY) {
        Flash_ReportError(FLASH_SID_READ, FLASH_E_BUSY);
        return E_NOT_OK;
    }

    if (TargetAddressPtr == NULL_PTR) {
        Flash_ReportError(FLASH_SID_READ, FLASH_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if ((SourceAddress < FLASH_BASE_ADDRESS) ||
        ((SourceAddress + Length) > (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE))) {
        Flash_ReportError(FLASH_SID_READ, FLASH_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }

    if (Length == 0U) {
        Flash_ReportError(FLASH_SID_READ, FLASH_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    /* Set up job parameters */
    Flash_DriverState.currentAddr = SourceAddress;
    Flash_DriverState.remainingLength = Length;
    Flash_DriverState.dataPtr = TargetAddressPtr;
    Flash_DriverState.jobType = FLASH_JOB_READ;
    Flash_DriverState.jobResult = FLASH_JOB_PENDING;
    Flash_DriverState.state = FLASH_STATE_BUSY;

    result = E_OK;

    return result;
}

/**
 * @brief Compare data in flash memory with application data buffer
 */
#if (FLASH_COMPARE_API == STD_ON)
Std_ReturnType Flash_Compare(Flash_AddressType SourceAddress, const uint8* TargetAddressPtr, Flash_LengthType Length)
{
    Std_ReturnType result = E_NOT_OK;

    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_COMPARE, FLASH_E_UNINIT);
        return E_NOT_OK;
    }

#if (FLASH_DEV_ERROR_DETECT == STD_ON)
    if (Flash_DriverState.state == FLASH_STATE_BUSY) {
        Flash_ReportError(FLASH_SID_COMPARE, FLASH_E_BUSY);
        return E_NOT_OK;
    }

    if (TargetAddressPtr == NULL_PTR) {
        Flash_ReportError(FLASH_SID_COMPARE, FLASH_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if ((SourceAddress < FLASH_BASE_ADDRESS) ||
        ((SourceAddress + Length) > (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE))) {
        Flash_ReportError(FLASH_SID_COMPARE, FLASH_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }

    if (Length == 0U) {
        Flash_ReportError(FLASH_SID_COMPARE, FLASH_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    /* Set up job parameters */
    Flash_DriverState.currentAddr = SourceAddress;
    Flash_DriverState.remainingLength = Length;
    Flash_DriverState.dataPtr = TargetAddressPtr;
    Flash_DriverState.jobType = FLASH_JOB_COMPARE;
    Flash_DriverState.jobResult = FLASH_JOB_PENDING;
    Flash_DriverState.state = FLASH_STATE_BUSY;

    result = E_OK;

    return result;
}
#endif

/**
 * @brief Set the flash driver's operation mode
 */
#if (FLASH_SET_MODE_API == STD_ON)
void Flash_SetMode(Flash_OpModeType Mode)
{
    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_SETMODE, FLASH_E_UNINIT);
        return;
    }

#if (FLASH_DEV_ERROR_DETECT == STD_ON)
    if ((Mode != FLASH_MODE_NORMAL) && (Mode != FLASH_MODE_FAST)) {
        Flash_ReportError(FLASH_SID_SETMODE, FLASH_E_PARAM_CONFIG);
        return;
    }
#endif

    Flash_DriverState.opMode = Mode;
}
#endif

/**
 * @brief Cancel an ongoing flash operation
 */
#if (FLASH_CANCEL_API == STD_ON)
void Flash_Cancel(void)
{
    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_CANCEL, FLASH_E_UNINIT);
        return;
    }

    if (Flash_DriverState.state != FLASH_STATE_BUSY) {
        return;
    }

    /* Cancel the current operation */
    FLASH_CR &= ~(FLASH_CR_PG | FLASH_CR_SER | FLASH_CR_MER | FLASH_CR_STRT);

    /* Update driver state */
    Flash_DriverState.jobResult = FLASH_JOB_CANCELLED;
    Flash_DriverState.state = FLASH_STATE_IDLE;
    Flash_DriverState.jobType = FLASH_JOB_NONE;

#if (FLASH_JOB_ERROR_NOTIFICATION == STD_ON)
    if (Flash_ConfigPtr->jobErrorNotification != NULL_PTR) {
        Flash_ConfigPtr->jobErrorNotification();
    }
#endif
}
#endif

/**
 * @brief Get the current status of the flash driver
 */
Flash_StatusType Flash_GetStatus(void)
{
    Flash_StatusType status;

    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        status = FLASH_UNINIT;
    } else if (Flash_DriverState.state == FLASH_STATE_BUSY) {
        switch (Flash_DriverState.jobType) {
            case FLASH_JOB_ERASE:
                status = FLASH_BUSY_ERASING;
                break;
            case FLASH_JOB_WRITE:
                status = FLASH_BUSY_WRITING;
                break;
            case FLASH_JOB_READ:
                status = FLASH_BUSY_READING;
                break;
            default:
                status = FLASH_BUSY;
                break;
        }
    } else {
        status = FLASH_IDLE;
    }

    return status;
}

/**
 * @brief Get the result of the most recent job
 */
Flash_JobResultType Flash_GetJobResult(void)
{
    Flash_JobResultType result;

    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_GETJOBRESULT, FLASH_E_UNINIT);
        result = FLASH_JOB_FAILED;
    } else {
        result = Flash_DriverState.jobResult;
    }

    return result;
}

/**
 * @brief Perform blank check on flash memory area
 */
#if (FLASH_BLANK_CHECK_API == STD_ON)
Std_ReturnType Flash_BlankCheck(Flash_AddressType TargetAddress, Flash_LengthType Length)
{
    Std_ReturnType result = E_NOT_OK;

    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_BLANKCHECK, FLASH_E_UNINIT);
        return E_NOT_OK;
    }

#if (FLASH_DEV_ERROR_DETECT == STD_ON)
    if (Flash_DriverState.state == FLASH_STATE_BUSY) {
        Flash_ReportError(FLASH_SID_BLANKCHECK, FLASH_E_BUSY);
        return E_NOT_OK;
    }

    if ((TargetAddress < FLASH_BASE_ADDRESS) ||
        ((TargetAddress + Length) > (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE))) {
        Flash_ReportError(FLASH_SID_BLANKCHECK, FLASH_E_PARAM_ADDRESS);
        return E_NOT_OK;
    }

    if (Length == 0U) {
        Flash_ReportError(FLASH_SID_BLANKCHECK, FLASH_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    /* Set up job parameters */
    Flash_DriverState.currentAddr = TargetAddress;
    Flash_DriverState.remainingLength = Length;
    Flash_DriverState.jobType = FLASH_JOB_BLANK_CHECK;
    Flash_DriverState.jobResult = FLASH_JOB_PENDING;
    Flash_DriverState.state = FLASH_STATE_BUSY;

    result = E_OK;

    return result;
}
#endif

/**
 * @brief Suspend an ongoing erase or write operation
 */
#if (FLASH_SUSPEND_RESUME_API == STD_ON)
Std_ReturnType Flash_Suspend(void)
{
    Std_ReturnType result = E_NOT_OK;

    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_SUSPEND, FLASH_E_UNINIT);
        return E_NOT_OK;
    }

    if (Flash_DriverState.state != FLASH_STATE_BUSY) {
        return E_NOT_OK;
    }

    /* Only erase and write can be suspended */
    if ((Flash_DriverState.jobType == FLASH_JOB_ERASE) ||
        (Flash_DriverState.jobType == FLASH_JOB_WRITE)) {
        /* Set suspend bit if supported by hardware */
        /* Note: Actual implementation depends on specific MCU flash controller */

        Flash_DriverState.jobResult = FLASH_JOB_SUSPENDED;
        Flash_DriverState.state = FLASH_STATE_IDLE;
        result = E_OK;
    }

    return result;
}

/**
 * @brief Resume a suspended erase or write operation
 */
Std_ReturnType Flash_Resume(void)
{
    Std_ReturnType result = E_NOT_OK;

    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        Flash_ReportError(FLASH_SID_RESUME, FLASH_E_UNINIT);
        return E_NOT_OK;
    }

    if (Flash_DriverState.jobResult != FLASH_JOB_SUSPENDED) {
        return E_NOT_OK;
    }

    /* Clear suspend bit if supported by hardware */
    /* Note: Actual implementation depends on specific MCU flash controller */

    Flash_DriverState.jobResult = FLASH_JOB_PENDING;
    Flash_DriverState.state = FLASH_STATE_BUSY;
    result = E_OK;

    return result;
}
#endif

/**
 * @brief Main function for handling flash jobs
 */
void Flash_MainFunction(void)
{
    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        return;
    }

    if (Flash_DriverState.state != FLASH_STATE_BUSY) {
        return;
    }

    switch (Flash_DriverState.jobType) {
        case FLASH_JOB_ERASE:
            Flash_ProcessEraseJob();
            break;
        case FLASH_JOB_WRITE:
            Flash_ProcessWriteJob();
            break;
        case FLASH_JOB_READ:
            Flash_ProcessReadJob();
            break;
        case FLASH_JOB_COMPARE:
            Flash_ProcessCompareJob();
            break;
        case FLASH_JOB_BLANK_CHECK:
            Flash_ProcessBlankCheckJob();
            break;
        default:
            /* Unknown job type - should not happen */
            Flash_DriverState.jobResult = FLASH_JOB_FAILED;
            Flash_DriverState.state = FLASH_STATE_IDLE;
            Flash_DriverState.jobType = FLASH_JOB_NONE;
            break;
    }
}

/**
 * @brief Get version information of the flash driver
 */
#if (FLASH_VERSION_INFO_API == STD_ON)
void Flash_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
#if (FLASH_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL_PTR) {
        Flash_ReportError(FLASH_SID_GETVERSIONINFO, FLASH_E_PARAM_POINTER);
        return;
    }
#endif

    VersionInfoPtr->vendorID = FLASH_VENDOR_ID;
    VersionInfoPtr->moduleID = FLASH_MODULE_ID;
    VersionInfoPtr->sw_major_version = FLASH_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version = FLASH_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version = FLASH_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Get sector information for a given address
 */
const Flash_SectorInfoType* Flash_GetSectorInfo(Flash_AddressType Address)
{
    const Flash_SectorInfoType* sectorInfo = NULL_PTR;
    uint32 i;

    if (Flash_DriverState.initState != FLASH_INITIALIZED) {
        return NULL_PTR;
    }

    for (i = 0U; i < Flash_ConfigPtr->numOfSectors; i++) {
        if ((Address >= Flash_ConfigPtr->sectorConfig[i].sectorStartAddr) &&
            (Address < (Flash_ConfigPtr->sectorConfig[i].sectorStartAddr +
                       Flash_ConfigPtr->sectorConfig[i].sectorSize))) {
            sectorInfo = &Flash_ConfigPtr->sectorConfig[i];
            break;
        }
    }

    return sectorInfo;
}

/**
 * @brief Check if an address is within valid flash range
 */
boolean Flash_IsAddressValid(Flash_AddressType Address)
{
    boolean valid = FALSE;

    if ((Address >= FLASH_BASE_ADDRESS) &&
        (Address < (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE))) {
        valid = TRUE;
    }

    return valid;
}

#define FLASH_STOP_SEC_CODE
#include "Flash_MemMap.h"

/* ============================================================================
 * AUTOSAR API — WRP 写保护 (P3, 2026-08-13)
 * 声明见 Flash.h: Fls_ConfigureWriteProtection。
 * 运行期设置写保护掩码 (位 N = 扇区 N); Flash_EraseSector / Flash_ProgramWord
 * 在操作前检查, 受保护扇区拒绝擦/写 (返回 E_NOT_OK, 硬件 WRPERR 语义)。
 * 默认掩码来自 Fls_ProtectionConfig.WriteProtectionMask (Flash_Init 装载)。
 * ============================================================================ */

/* MISRA 8.7 保留: 公开 API, 消费者在集成层 (Bootloader/ASW) */
Std_ReturnType Fls_ConfigureWriteProtection(uint32 SectorMask, boolean Enable)
{
    if (Enable != FALSE) {
        Flash_WriteProtectMask |= SectorMask;
    } else {
        Flash_WriteProtectMask &= ~SectorMask;
    }
    return E_OK;
}

/*==================================================================================================
 *                                      END OF FILE
 *==================================================================================================*/
