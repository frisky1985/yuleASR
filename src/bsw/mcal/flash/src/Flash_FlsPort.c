/**********************************************************************************************************************
 * @file       Flash_FlsPort.c
 * @brief      Legacy Flash port implementation backed by the S32K3 Fls driver
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-11-28
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      Implements the neutral Flash_LegacyPort contract by delegating to the
 *      canonical AUTOSAR Fls driver (src/bsw/mcal/fls).  This translation unit
 *      is the ONLY place where the legacy Flash shim meets Fls.h, keeping the
 *      conflicting Fls_* typedefs of Flash.h and Fls.h in separate TUs.
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Flash_LegacyPort.h"
#include "Fls.h"

/**********************************************************************************************************************
 * LOCAL DATA
 *********************************************************************************************************************/
static Fls_ConfigType Fls_SectorBufferConfig;
static Fls_SectorType Fls_SectorBuffer[FLASH_PORT_MAX_SECTORS];
static boolean        Flash_PortInitialized = FALSE;

/**********************************************************************************************************************
 * LOCAL FUNCTIONS
 *********************************************************************************************************************/
static uint8 Flash_PortMapStatus(Fls_StatusType status)
{
    uint8 result;

    switch (status)
    {
        case FLS_IDLE:
            result = FLASH_PORT_STATUS_IDLE;
            break;
        case FLS_BUSY:
            result = FLASH_PORT_STATUS_BUSY;
            break;
        case FLS_UNINIT:
        default:
            result = FLASH_PORT_STATUS_UNINIT;
            break;
    }
    return result;
}

static uint8 Flash_PortMapJobResult(Fls_JobResultType result)
{
    uint8 mapped;

    switch (result)
    {
        case MEMIF_JOB_OK:
            mapped = FLASH_PORT_JOB_OK;
            break;
        case MEMIF_JOB_PENDING:
            mapped = FLASH_PORT_JOB_PENDING;
            break;
        case MEMIF_JOB_CANCELED:
            mapped = FLASH_PORT_JOB_CANCELED;
            break;
        case MEMIF_JOB_FAILED:
        default:
            mapped = FLASH_PORT_JOB_FAILED;
            break;
    }
    return mapped;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS
 *********************************************************************************************************************/
void Flash_PortInit(const Flash_PortConfigType* config)
{
    uint32 i;

    if (config == NULL_PTR)
    {
        return;
    }
    if (config->sectorCount > FLASH_PORT_MAX_SECTORS)
    {
        return;
    }

    for (i = 0U; i < config->sectorCount; i++)
    {
        const Flash_PortSectorType* src = &config->sectorList[i];

        Fls_SectorBuffer[i].sectorStartAddr  = src->sectorStartAddr;
        Fls_SectorBuffer[i].sectorSize       = src->sectorSize;
        Fls_SectorBuffer[i].sectorPageSize   = src->sectorPageSize;
        Fls_SectorBuffer[i].sectorUnlockMask = src->sectorUnlockMask;
        Fls_SectorBuffer[i].sectorWritable   = src->sectorWritable;
        Fls_SectorBuffer[i].sectorErasable   = src->sectorErasable;
    }

    Fls_SectorBufferConfig.sectorList          = Fls_SectorBuffer;
    Fls_SectorBufferConfig.sectorCount         = config->sectorCount;
    Fls_SectorBufferConfig.defaultMode         = config->defaultMode;
    Fls_SectorBufferConfig.maxReadFastMode     = config->maxReadFastMode;
    Fls_SectorBufferConfig.maxReadNormalMode   = config->maxReadNormalMode;
    Fls_SectorBufferConfig.maxWriteFastMode    = config->maxWriteFastMode;
    Fls_SectorBufferConfig.maxWriteNormalMode  = config->maxWriteNormalMode;
    Fls_SectorBufferConfig.jobEndNotificationEnabled   = FALSE;
    Fls_SectorBufferConfig.jobErrorNotificationEnabled = FALSE;

    Fls_Init(&Fls_SectorBufferConfig);
    Flash_PortInitialized = TRUE;
}

void Flash_PortDeInit(void)
{
    if (Flash_PortInitialized == TRUE)
    {
        Fls_Cancel();
        Flash_PortInitialized = FALSE;
    }
}

Std_ReturnType Flash_PortErase(uint32 address, uint32 length)
{
    Std_ReturnType result;

    if (Flash_PortInitialized == FALSE)
    {
        return E_NOT_OK;
    }
    result = Fls_Erase(address, length);
    if (result == E_OK)
    {
        Fls_MainFunction();
    }
    return result;
}

Std_ReturnType Flash_PortWrite(uint32 address, const uint8* source, uint32 length)
{
    Std_ReturnType result;

    if (Flash_PortInitialized == FALSE)
    {
        return E_NOT_OK;
    }
    result = Fls_Write(address, source, length);
    if (result == E_OK)
    {
        Fls_MainFunction();
    }
    return result;
}

Std_ReturnType Flash_PortRead(uint32 address, uint8* target, uint32 length)
{
    if (Flash_PortInitialized == FALSE)
    {
        return E_NOT_OK;
    }
    return Fls_ReadSync(address, target, length);
}

Std_ReturnType Flash_PortCompare(uint32 address, const uint8* target, uint32 length)
{
    if (Flash_PortInitialized == FALSE)
    {
        return E_NOT_OK;
    }
    Fls_Compare(address, target, length);
    Fls_MainFunction();
    return (Fls_GetJobResult() == MEMIF_JOB_OK) ? E_OK : E_NOT_OK;
}

void Flash_PortSetMode(uint8 mode)
{
    if (Flash_PortInitialized == FALSE)
    {
        return;
    }
    Fls_SetMode((mode == FLASH_PORT_MODE_FAST) ? MEMIF_MODE_FAST : MEMIF_MODE_SLOW);
}

void Flash_PortCancel(void)
{
    if (Flash_PortInitialized == TRUE)
    {
        Fls_Cancel();
    }
}

uint8 Flash_PortGetStatus(void)
{
    if (Flash_PortInitialized == FALSE)
    {
        return FLASH_PORT_STATUS_UNINIT;
    }
    return Flash_PortMapStatus(Fls_GetStatus());
}

uint8 Flash_PortGetJobResult(void)
{
    if (Flash_PortInitialized == FALSE)
    {
        return FLASH_PORT_JOB_FAILED;
    }
    return Flash_PortMapJobResult(Fls_GetJobResult());
}

void Flash_PortMainFunction(void)
{
    if (Flash_PortInitialized == TRUE)
    {
        Fls_MainFunction();
    }
}

/**********************************************************************************************************************
 * END OF FILE
 *********************************************************************************************************************/
