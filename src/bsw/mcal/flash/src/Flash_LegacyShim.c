/**********************************************************************************************************************
 * @file       Flash_LegacyShim.c
 * @brief      Legacy Flash_* API implemented on top of the S32K3 Fls driver
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2026-11-28
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      The former Flash.c dereferenced STM32H743 absolute addresses
 *      (0x40023C14 ...) directly and therefore could not run on the target or
 *      on the host.  This shim keeps the legacy Flash_* entry points for
 *      source compatibility but forwards every access to the AUTOSAR Fls
 *      driver through the neutral Flash_LegacyPort contract.
 *
 *      Operations are completed synchronously here (the port drives
 *      Fls_MainFunction internally); Flash_MainFunction is still provided so
 *      that existing callers that poll keep working.
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Flash.h"
#include "Flash_LegacyPort.h"

/**********************************************************************************************************************
 * LOCAL DATA
 *********************************************************************************************************************/
static Flash_StatusType    Flash_ShimStatus = FLASH_STATUS_UNINIT;
static Flash_JobResultType Flash_ShimJobResult = FLASH_JOB_OK;
static Flash_OpModeType    Flash_ShimMode = FLASH_MODE_NORMAL;
static uint32              Flash_ShimBaseAddress = 0U;
static uint32              Flash_ShimEndAddress = 0U;
static uint32              Flash_ShimSectorCount = 0U;
static const Flash_SectorInfoType* Flash_ShimSectorInfo = NULL_PTR;
static Flash_PortSectorType Flash_ShimPortSectors[FLASH_PORT_MAX_SECTORS];
static Flash_PortConfigType Flash_ShimPortConfig;

/**********************************************************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/
Std_ReturnType Flash_Compare(Flash_AddressType SourceAddress, const uint8* TargetAddressPtr, Flash_LengthType Length);
Std_ReturnType Flash_BlankCheck(Flash_AddressType TargetAddress, Flash_LengthType Length);
void           Flash_Cancel(void);
Std_ReturnType Flash_Suspend(void);
Std_ReturnType Flash_Resume(void);
const Flash_SectorInfoType* Flash_GetSectorInfo(Flash_AddressType Address);

/**********************************************************************************************************************
 * LOCAL HELPERS
 *********************************************************************************************************************/
static Flash_StatusType Flash_ShimMapStatus(uint8 portStatus)
{
    Flash_StatusType status;

    switch (portStatus)
    {
        case FLASH_PORT_STATUS_IDLE:
            status = FLASH_STATUS_IDLE;
            break;
        case FLASH_PORT_STATUS_BUSY:
            status = FLASH_STATUS_BUSY;
            break;
        case FLASH_PORT_STATUS_UNINIT:
        default:
            status = FLASH_STATUS_UNINIT;
            break;
    }
    return status;
}

static Flash_JobResultType Flash_ShimMapJobResult(uint8 portResult)
{
    Flash_JobResultType result;

    switch (portResult)
    {
        case FLASH_PORT_JOB_OK:
            result = FLASH_JOB_OK;
            break;
        case FLASH_PORT_JOB_PENDING:
            result = FLASH_JOB_PENDING;
            break;
        case FLASH_PORT_JOB_CANCELED:
            result = FLASH_JOB_CANCELLED;
            break;
        case FLASH_PORT_JOB_FAILED:
        default:
            result = FLASH_JOB_FAILED;
            break;
    }
    return result;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS
 *********************************************************************************************************************/
void Flash_Init(const Flash_ConfigType* ConfigPtr)
{
    uint32 i;

    if (ConfigPtr == NULL_PTR)
    {
        Flash_ShimStatus = FLASH_STATUS_UNINIT;
        return;
    }

    Flash_ShimSectorCount = ConfigPtr->numOfSectors;
    if (Flash_ShimSectorCount > FLASH_PORT_MAX_SECTORS)
    {
        Flash_ShimSectorCount = FLASH_PORT_MAX_SECTORS;
    }
    Flash_ShimSectorInfo = ConfigPtr->sectorConfig;

    for (i = 0U; i < Flash_ShimSectorCount; i++)
    {
        const Flash_SectorInfoType* sector = &ConfigPtr->sectorConfig[i];

        Flash_ShimPortSectors[i].sectorStartAddr  = sector->sectorStartAddr;
        Flash_ShimPortSectors[i].sectorSize       = sector->sectorSize;
        Flash_ShimPortSectors[i].sectorPageSize   = sector->pageSize;
        Flash_ShimPortSectors[i].sectorUnlockMask = 0U;
        Flash_ShimPortSectors[i].sectorWritable   = (boolean)(sector->isProtected == FALSE);
        Flash_ShimPortSectors[i].sectorErasable   = TRUE;
    }

    Flash_ShimPortConfig.sectorList         = Flash_ShimPortSectors;
    Flash_ShimPortConfig.sectorCount        = Flash_ShimSectorCount;
    Flash_ShimPortConfig.defaultMode        = (uint32)ConfigPtr->defaultMode;
    Flash_ShimPortConfig.maxReadFastMode    = ConfigPtr->maxReadFastMode;
    Flash_ShimPortConfig.maxReadNormalMode  = ConfigPtr->maxReadNormalMode;
    Flash_ShimPortConfig.maxWriteFastMode   = ConfigPtr->maxWriteFastMode;
    Flash_ShimPortConfig.maxWriteNormalMode = ConfigPtr->maxWriteNormalMode;

    Flash_ShimBaseAddress = ConfigPtr->baseAddress;
    Flash_ShimEndAddress  = ConfigPtr->endAddress;
    Flash_ShimMode        = ConfigPtr->defaultMode;

    Flash_PortInit(&Flash_ShimPortConfig);

    Flash_ShimStatus    = FLASH_STATUS_IDLE;
    Flash_ShimJobResult = FLASH_JOB_OK;
}

void Flash_DeInit(void)
{
    Flash_PortDeInit();
    Flash_ShimStatus    = FLASH_STATUS_UNINIT;
    Flash_ShimJobResult = FLASH_JOB_OK;
    Flash_ShimSectorInfo = NULL_PTR;
    Flash_ShimSectorCount = 0U;
}

Std_ReturnType Flash_Write(Flash_AddressType TargetAddress, const uint8* SourceAddressPtr, Flash_LengthType Length)
{
    Std_ReturnType result;

    if (Flash_ShimStatus == FLASH_STATUS_UNINIT)
    {
        return E_NOT_OK;
    }
    Flash_ShimStatus = FLASH_STATUS_BUSY;
    result = Flash_PortWrite((uint32)TargetAddress, SourceAddressPtr, (uint32)Length);
    Flash_ShimJobResult = (result == E_OK) ? FLASH_JOB_OK : FLASH_JOB_FAILED;
    Flash_ShimStatus = FLASH_STATUS_IDLE;
    return result;
}

Std_ReturnType Flash_Read(Flash_AddressType SourceAddress, uint8* TargetAddressPtr, Flash_LengthType Length)
{
    Std_ReturnType result;

    if (Flash_ShimStatus == FLASH_STATUS_UNINIT)
    {
        return E_NOT_OK;
    }
    result = Flash_PortRead((uint32)SourceAddress, TargetAddressPtr, (uint32)Length);
    Flash_ShimJobResult = (result == E_OK) ? FLASH_JOB_OK : FLASH_JOB_FAILED;
    return result;
}

Std_ReturnType Flash_Compare(Flash_AddressType SourceAddress, const uint8* TargetAddressPtr, Flash_LengthType Length)
{
    Std_ReturnType result;

    if (Flash_ShimStatus == FLASH_STATUS_UNINIT)
    {
        return E_NOT_OK;
    }
    result = Flash_PortCompare((uint32)SourceAddress, TargetAddressPtr, (uint32)Length);
    Flash_ShimJobResult = (result == E_OK) ? FLASH_JOB_OK : FLASH_JOB_FAILED;
    return result;
}

Std_ReturnType Flash_BlankCheck(Flash_AddressType TargetAddress, Flash_LengthType Length)
{
    uint8  buffer[32];
    uint32 address = (uint32)TargetAddress;
    uint32 remaining = (uint32)Length;
    uint32 chunk;
    uint32 i;
    Std_ReturnType result = E_OK;

    if (Flash_ShimStatus == FLASH_STATUS_UNINIT)
    {
        return E_NOT_OK;
    }

    while ((remaining > 0U) && (result == E_OK))
    {
        chunk = (remaining > (uint32)sizeof(buffer)) ? (uint32)sizeof(buffer) : remaining;
        result = Flash_PortRead(address, buffer, chunk);
        if (result == E_OK)
        {
            for (i = 0U; i < chunk; i++)
            {
                if (buffer[i] != 0xFFU)
                {
                    result = E_NOT_OK;
                    break;
                }
            }
        }
        address   += chunk;
        remaining -= chunk;
    }

    Flash_ShimJobResult = (result == E_OK) ? FLASH_JOB_OK : FLASH_JOB_FAILED;
    return result;
}

void Flash_SetMode(Flash_OpModeType Mode)
{
    Flash_ShimMode = Mode;
    Flash_PortSetMode((uint8)Mode);
}

void Flash_Cancel(void)
{
    Flash_PortCancel();
    Flash_ShimJobResult = FLASH_JOB_CANCELLED;
    Flash_ShimStatus    = FLASH_STATUS_IDLE;
}

Flash_StatusType Flash_GetStatus(void)
{
    Flash_ShimStatus = Flash_ShimMapStatus(Flash_PortGetStatus());
    return Flash_ShimStatus;
}

Flash_JobResultType Flash_GetJobResult(void)
{
    return Flash_ShimJobResult;
}

void Flash_MainFunction(void)
{
    if (Flash_ShimStatus != FLASH_STATUS_UNINIT)
    {
        Flash_PortMainFunction();
        Flash_ShimJobResult = Flash_ShimMapJobResult(Flash_PortGetJobResult());
    }
}

Std_ReturnType Flash_Suspend(void)
{
    /* Suspend/resume are not supported by the S32K3 Fls shim. */
    return E_NOT_OK;
}

Std_ReturnType Flash_Resume(void)
{
    return E_NOT_OK;
}

void Flash_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
    if (VersionInfoPtr == NULL_PTR)
    {
        return;
    }
    VersionInfoPtr->vendorID          = FLS_VENDOR_ID;
    VersionInfoPtr->moduleID          = FLS_MODULE_ID;
    VersionInfoPtr->sw_major_version  = FLS_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version  = FLS_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version  = FLS_SW_PATCH_VERSION;
}

boolean Flash_IsAddressValid(Flash_AddressType Address)
{
    return (boolean)((((uint32)Address) >= Flash_ShimBaseAddress) &&
                     (((uint32)Address) < Flash_ShimEndAddress));
}

const Flash_SectorInfoType* Flash_GetSectorInfo(Flash_AddressType Address)
{
    uint32 i;

    if (Flash_ShimSectorInfo == NULL_PTR)
    {
        return NULL_PTR;
    }
    for (i = 0U; i < Flash_ShimSectorCount; i++)
    {
        const Flash_SectorInfoType* sector = &Flash_ShimSectorInfo[i];

        if (((uint32)Address >= sector->sectorStartAddr) &&
            (((uint32)Address) < (sector->sectorStartAddr + sector->sectorSize)))
        {
            return sector;
        }
    }
    return NULL_PTR;
}

/**********************************************************************************************************************
 * END OF FILE
 *********************************************************************************************************************/
