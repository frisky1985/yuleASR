/**
 * @file MemIf.c
 * @brief Memory Interface implementation
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "MemIf.h"
#include "MemIf_Cfg.h"
#include "Det.h"

/* Forward declarations for underlying drivers */
/* These would normally be included from Fee.h, Ea.h, Eep.h */
extern Std_ReturnType Fee_Read(uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length);
extern Std_ReturnType Fee_Write(uint16 BlockNumber, const uint8* DataBufferPtr);
extern MemIf_StatusType Fee_GetStatus(void);
extern MemIf_JobResultType Fee_GetJobResult(void);
extern void Fee_Cancel(void);
extern Std_ReturnType Fee_InvalidateBlock(uint16 BlockNumber);
extern Std_ReturnType Fee_EraseImmediateBlock(uint16 BlockNumber);

extern Std_ReturnType Ea_Read(uint16 BlockNumber, uint16 BlockOffset, uint8* DataBufferPtr, uint16 Length);
extern Std_ReturnType Ea_Write(uint16 BlockNumber, const uint8* DataBufferPtr);
extern MemIf_StatusType Ea_GetStatus(void);
extern MemIf_JobResultType Ea_GetJobResult(void);
extern void Ea_Cancel(void);
extern Std_ReturnType Ea_InvalidateBlock(uint16 BlockNumber);
extern Std_ReturnType Ea_EraseImmediateBlock(uint16 BlockNumber);

#define MEMIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean MemIf_Initialized = FALSE;
static const MemIf_ConfigType* MemIf_ConfigPtr = NULL_PTR;
static MemIf_ModeType MemIf_CurrentMode[MEMIF_NUM_DEVICES];

#define MEMIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define MEMIF_START_SEC_CODE
#include "MemMap.h"

void MemIf_Init(const MemIf_ConfigType* ConfigPtr)
{
    #if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_INIT, MEMIF_E_PARAM_POINTER);
        return;
    }
    #endif

    MemIf_ConfigPtr = ConfigPtr;

    /* Initialize mode for each device */
    for (uint8 i = 0U; i < MEMIF_NUM_DEVICES; i++) {
        if (i < ConfigPtr->NumDevices) {
            MemIf_CurrentMode[i] = ConfigPtr->Devices[i].DefaultMode;
        }
    }

    MemIf_Initialized = TRUE;
}

Std_ReturnType MemIf_Read(MemIf_DeviceIdType DeviceIndex,
                           MemIf_BlockIdType BlockNumber,
                           uint16 BlockOffset,
                           uint8* DataBufferPtr,
                           uint16 Length)
{
    #if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (MemIf_Initialized == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_READ, MEMIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (DeviceIndex >= MEMIF_NUM_DEVICES) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_READ, MEMIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }
    if (BlockNumber >= MEMIF_MAX_BLOCK_NUMBER) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_READ, MEMIF_E_PARAM_BLOCK);
        return E_NOT_OK;
    }
    if (DataBufferPtr == NULL_PTR) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_READ, MEMIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (Length == 0U || Length > MEMIF_MAX_BLOCK_SIZE) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_READ, MEMIF_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
    #endif

    Std_ReturnType result = E_NOT_OK;

    if (MemIf_ConfigPtr != NULL_PTR && DeviceIndex < MemIf_ConfigPtr->NumDevices) {
        const MemIf_DeviceConfigType* deviceConfig = &MemIf_ConfigPtr->Devices[DeviceIndex];

        switch (deviceConfig->UnderlyingDriver) {
            case MEMIF_UNDERLYING_FEE:
                result = Fee_Read(BlockNumber, BlockOffset, DataBufferPtr, Length);
                break;

            case MEMIF_UNDERLYING_EA:
                result = Ea_Read(BlockNumber, BlockOffset, DataBufferPtr, Length);
                break;

            case MEMIF_UNDERLYING_EEP:
                /* Direct EEPROM access - would call Eep_Read */
                result = E_NOT_OK;
                break;

            default:
                result = E_NOT_OK;
                break;
        }
    }

    return result;
}

Std_ReturnType MemIf_Write(MemIf_DeviceIdType DeviceIndex,
                            MemIf_BlockIdType BlockNumber,
                            const uint8* DataBufferPtr)
{
    #if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (MemIf_Initialized == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_WRITE, MEMIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (DeviceIndex >= MEMIF_NUM_DEVICES) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_WRITE, MEMIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }
    if (BlockNumber >= MEMIF_MAX_BLOCK_NUMBER) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_WRITE, MEMIF_E_PARAM_BLOCK);
        return E_NOT_OK;
    }
    if (DataBufferPtr == NULL_PTR) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_WRITE, MEMIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    Std_ReturnType result = E_NOT_OK;

    if (MemIf_ConfigPtr != NULL_PTR && DeviceIndex < MemIf_ConfigPtr->NumDevices) {
        const MemIf_DeviceConfigType* deviceConfig = &MemIf_ConfigPtr->Devices[DeviceIndex];

        switch (deviceConfig->UnderlyingDriver) {
            case MEMIF_UNDERLYING_FEE:
                result = Fee_Write(BlockNumber, DataBufferPtr);
                break;

            case MEMIF_UNDERLYING_EA:
                result = Ea_Write(BlockNumber, DataBufferPtr);
                break;

            case MEMIF_UNDERLYING_EEP:
                /* Direct EEPROM access */
                result = E_NOT_OK;
                break;

            default:
                result = E_NOT_OK;
                break;
        }
    }

    return result;
}

void MemIf_Cancel(MemIf_DeviceIdType DeviceIndex)
{
    #if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (MemIf_Initialized == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_CANCEL, MEMIF_E_UNINIT);
        return;
    }
    if (DeviceIndex >= MEMIF_NUM_DEVICES) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_CANCEL, MEMIF_E_PARAM_DEVICE);
        return;
    }
    #endif

    if (MemIf_ConfigPtr != NULL_PTR && DeviceIndex < MemIf_ConfigPtr->NumDevices) {
        const MemIf_DeviceConfigType* deviceConfig = &MemIf_ConfigPtr->Devices[DeviceIndex];

        switch (deviceConfig->UnderlyingDriver) {
            case MEMIF_UNDERLYING_FEE:
                Fee_Cancel();
                break;

            case MEMIF_UNDERLYING_EA:
                Ea_Cancel();
                break;

            case MEMIF_UNDERLYING_EEP:
                /* Direct EEPROM access */
                break;

            default:
                break;
        }
    }
}

MemIf_StatusType MemIf_GetStatus(MemIf_DeviceIdType DeviceIndex)
{
    #if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (MemIf_Initialized == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_GETSTATUS, MEMIF_E_UNINIT);
        return MEMIF_IDLE;
    }
    if (DeviceIndex >= MEMIF_NUM_DEVICES) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_GETSTATUS, MEMIF_E_PARAM_DEVICE);
        return MEMIF_IDLE;
    }
    #endif

    MemIf_StatusType status = MEMIF_IDLE;

    if (MemIf_ConfigPtr != NULL_PTR && DeviceIndex < MemIf_ConfigPtr->NumDevices) {
        const MemIf_DeviceConfigType* deviceConfig = &MemIf_ConfigPtr->Devices[DeviceIndex];

        switch (deviceConfig->UnderlyingDriver) {
            case MEMIF_UNDERLYING_FEE:
                status = Fee_GetStatus();
                break;

            case MEMIF_UNDERLYING_EA:
                status = Ea_GetStatus();
                break;

            case MEMIF_UNDERLYING_EEP:
                /* Direct EEPROM access */
                status = MEMIF_IDLE;
                break;

            default:
                status = MEMIF_IDLE;
                break;
        }
    }

    return status;
}

MemIf_JobResultType MemIf_GetJobResult(MemIf_DeviceIdType DeviceIndex)
{
    #if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (MemIf_Initialized == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_GETJOBRESULT, MEMIF_E_UNINIT);
        return MEMIF_JOB_FAILED;
    }
    if (DeviceIndex >= MEMIF_NUM_DEVICES) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_GETJOBRESULT, MEMIF_E_PARAM_DEVICE);
        return MEMIF_JOB_FAILED;
    }
    #endif

    MemIf_JobResultType result = MEMIF_JOB_FAILED;

    if (MemIf_ConfigPtr != NULL_PTR && DeviceIndex < MemIf_ConfigPtr->NumDevices) {
        const MemIf_DeviceConfigType* deviceConfig = &MemIf_ConfigPtr->Devices[DeviceIndex];

        switch (deviceConfig->UnderlyingDriver) {
            case MEMIF_UNDERLYING_FEE:
                result = Fee_GetJobResult();
                break;

            case MEMIF_UNDERLYING_EA:
                result = Ea_GetJobResult();
                break;

            case MEMIF_UNDERLYING_EEP:
                /* Direct EEPROM access */
                result = MEMIF_JOB_OK;
                break;

            default:
                result = MEMIF_JOB_FAILED;
                break;
        }
    }

    return result;
}

Std_ReturnType MemIf_InvalidateBlock(MemIf_DeviceIdType DeviceIndex,
                                      MemIf_BlockIdType BlockNumber)
{
    #if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (MemIf_Initialized == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_INVALIDATEBLOCK, MEMIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (DeviceIndex >= MEMIF_NUM_DEVICES) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_INVALIDATEBLOCK, MEMIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }
    if (BlockNumber >= MEMIF_MAX_BLOCK_NUMBER) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_INVALIDATEBLOCK, MEMIF_E_PARAM_BLOCK);
        return E_NOT_OK;
    }
    #endif

    Std_ReturnType result = E_NOT_OK;

    if (MemIf_ConfigPtr != NULL_PTR && DeviceIndex < MemIf_ConfigPtr->NumDevices) {
        const MemIf_DeviceConfigType* deviceConfig = &MemIf_ConfigPtr->Devices[DeviceIndex];

        switch (deviceConfig->UnderlyingDriver) {
            case MEMIF_UNDERLYING_FEE:
                result = Fee_InvalidateBlock(BlockNumber);
                break;

            case MEMIF_UNDERLYING_EA:
                result = Ea_InvalidateBlock(BlockNumber);
                break;

            case MEMIF_UNDERLYING_EEP:
                /* Direct EEPROM access */
                result = E_NOT_OK;
                break;

            default:
                result = E_NOT_OK;
                break;
        }
    }

    return result;
}

Std_ReturnType MemIf_EraseImmediateBlock(MemIf_DeviceIdType DeviceIndex,
                                          MemIf_BlockIdType BlockNumber)
{
    #if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (MemIf_Initialized == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_ERASEIMMEDIATEBLOCK, MEMIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (DeviceIndex >= MEMIF_NUM_DEVICES) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_ERASEIMMEDIATEBLOCK, MEMIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }
    if (BlockNumber >= MEMIF_MAX_BLOCK_NUMBER) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_ERASEIMMEDIATEBLOCK, MEMIF_E_PARAM_BLOCK);
        return E_NOT_OK;
    }
    #endif

    Std_ReturnType result = E_NOT_OK;

    if (MemIf_ConfigPtr != NULL_PTR && DeviceIndex < MemIf_ConfigPtr->NumDevices) {
        const MemIf_DeviceConfigType* deviceConfig = &MemIf_ConfigPtr->Devices[DeviceIndex];

        switch (deviceConfig->UnderlyingDriver) {
            case MEMIF_UNDERLYING_FEE:
                result = Fee_EraseImmediateBlock(BlockNumber);
                break;

            case MEMIF_UNDERLYING_EA:
                result = Ea_EraseImmediateBlock(BlockNumber);
                break;

            case MEMIF_UNDERLYING_EEP:
                /* Direct EEPROM access */
                result = E_NOT_OK;
                break;

            default:
                result = E_NOT_OK;
                break;
        }
    }

    return result;
}

void MemIf_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, MEMIF_SID_GETVERSIONINFO, MEMIF_E_PARAM_POINTER);
        return;
    }
    #endif

    versioninfo->vendorID = MEMIF_VENDOR_ID;
    versioninfo->moduleID = MEMIF_MODULE_ID;
    versioninfo->sw_major_version = MEMIF_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = MEMIF_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = MEMIF_SW_PATCH_VERSION;
}

void MemIf_SetMode(MemIf_DeviceIdType DeviceIndex, MemIf_ModeType Mode)
{
    #if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (MemIf_Initialized == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, 0x09U, MEMIF_E_UNINIT);
        return;
    }
    if (DeviceIndex >= MEMIF_NUM_DEVICES) {
        Det_ReportError(MEMIF_MODULE_ID, 0U, 0x09U, MEMIF_E_PARAM_DEVICE);
        return;
    }
    #endif

    if (DeviceIndex < MEMIF_NUM_DEVICES) {
        MemIf_CurrentMode[DeviceIndex] = Mode;

        /* In a real implementation, this would propagate the mode change
         * to the underlying driver (Fee_SetMode, Ea_SetMode, etc.) */
    }
}

#define MEMIF_STOP_SEC_CODE
#include "MemMap.h"
