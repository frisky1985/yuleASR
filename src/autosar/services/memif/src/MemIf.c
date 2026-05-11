/*==================================================================================================
 *                                MEMORY INTERFACE (MemIf)
 *==================================================================================================
 * FILENAME: MemIf.c
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_MemoryInterface.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Implementation of Memory Interface module
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "MemIf.h"
#include "Det.h"

/* Conditional includes for underlying drivers */
#if (MEMIF_FEE_ENABLED == STD_ON)
#include "Fee.h"
#endif

#if (MEMIF_EA_ENABLED == STD_ON)
#include "Ea.h"
#endif

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
#define MEMIF_VALIDATE_INITIALIZED(sid) \
    do { \
        if (MemIf_ModuleInitialized == FALSE) { \
            Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, (sid), MEMIF_E_UNINIT); \
            return; \
        } \
    } while(0)

#define MEMIF_VALIDATE_INITIALIZED_RET(sid, ret) \
    do { \
        if (MemIf_ModuleInitialized == FALSE) { \
            Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, (sid), MEMIF_E_UNINIT); \
            return (ret); \
        } \
    } while(0)

#define MEMIF_VALIDATE_DEVICE_INDEX(sid, idx) \
    do { \
        if ((idx) >= MEMIF_TOTAL_NUM_DEVICES) { \
            Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, (sid), MEMIF_E_PARAM_DEVICE_INDEX); \
            return; \
        } \
    } while(0)

#define MEMIF_VALIDATE_DEVICE_INDEX_RET(sid, idx, ret) \
    do { \
        if ((idx) >= MEMIF_TOTAL_NUM_DEVICES) { \
            Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, (sid), MEMIF_E_PARAM_DEVICE_INDEX); \
            return (ret); \
        } \
    } while(0)

#define MEMIF_VALIDATE_POINTER(sid, ptr) \
    do { \
        if ((ptr) == NULL_PTR) { \
            Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, (sid), MEMIF_E_PARAM_POINTER); \
            return E_NOT_OK; \
        } \
    } while(0)

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
static boolean MemIf_IsDeviceIndexValid(uint8 DeviceIndex);
static boolean MemIf_IsBlockNumberValid(uint8 DeviceIndex, uint16 BlockNumber);

/*==================================================================================================
 *                                    GLOBAL VARIABLES
 *==================================================================================================*/
#define MEMIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemIf_MemMap.h"

MemIf_DeviceStateType MemIf_DeviceState[MEMIF_NUMBER_OF_DEVICES];
const MemIf_ConfigType* MemIf_ConfigPtr = NULL_PTR;
boolean MemIf_ModuleInitialized = FALSE;

#define MEMIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemIf_MemMap.h"

#define MEMIF_START_SEC_CONST_UNSPECIFIED
#include "MemIf_MemMap.h"

/* Default configuration - Fee and Ea */
static const MemIf_DeviceConfigType MemIf_DefaultDeviceConfig[MEMIF_TOTAL_NUM_DEVICES] = {
#if (MEMIF_FEE_ENABLED == STD_ON)
    {
        MEMIF_FEE_DEVICE_INDEX,
        MEMIF_DEVICE_TYPE_FEE,
        MEMIF_FEE_NUM_BLOCKS
    },
#endif
#if (MEMIF_EA_ENABLED == STD_ON)
    {
        MEMIF_EA_DEVICE_INDEX,
        MEMIF_DEVICE_TYPE_EA,
        MEMIF_EA_NUM_BLOCKS
    }
#endif
};

static const MemIf_ConfigType MemIf_DefaultConfig = {
    MemIf_DefaultDeviceConfig,
    MEMIF_TOTAL_NUM_DEVICES
};

#define MEMIF_STOP_SEC_CONST_UNSPECIFIED
#include "MemIf_MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/
/**
 * @brief Validates device index
 * @param DeviceIndex Device index to validate
 * @return TRUE if valid, FALSE otherwise
 */
static boolean MemIf_IsDeviceIndexValid(uint8 DeviceIndex)
{
    return (DeviceIndex < MEMIF_TOTAL_NUM_DEVICES) ? TRUE : FALSE;
}

/**
 * @brief Validates block number for a device
 * @param DeviceIndex Device index
 * @param BlockNumber Block number to validate
 * @return TRUE if valid, FALSE otherwise
 */
static boolean MemIf_IsBlockNumberValid(uint8 DeviceIndex, uint16 BlockNumber)
{
    boolean result = FALSE;
    
    if (DeviceIndex < MEMIF_TOTAL_NUM_DEVICES) {
        if (BlockNumber < MemIf_DefaultDeviceConfig[DeviceIndex].numBlocks) {
            result = TRUE;
        }
    }
    
    return result;
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/
#define MEMIF_START_SEC_CODE
#include "MemIf_MemMap.h"

/**
 * @brief Initializes the Memory Interface module
 * @param ConfigPtr Pointer to configuration structure
 * @return None
 */
void MemIf_Init(const MemIf_ConfigType* ConfigPtr)
{
#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (MemIf_ModuleInitialized == TRUE) {
        Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, MEMIF_SID_INIT, MEMIF_E_ALREADY_INITIALIZED);
        return;
    }
#endif

    /* Use default configuration if NULL pointer passed */
    if (ConfigPtr == NULL_PTR) {
        MemIf_ConfigPtr = &MemIf_DefaultConfig;
    } else {
        MemIf_ConfigPtr = ConfigPtr;
    }

    /* Initialize device states */
    for (uint8 idx = 0u; idx < MEMIF_TOTAL_NUM_DEVICES; idx++) {
        MemIf_DeviceState[idx].status = MEMIF_IDLE;
        MemIf_DeviceState[idx].jobResult = MEMIF_JOB_OK;
        MemIf_DeviceState[idx].isInitialized = TRUE;
    }

    MemIf_ModuleInitialized = TRUE;
}

/**
 * @brief Deinitializes the Memory Interface module
 * @return None
 */
void MemIf_DeInit(void)
{
#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    MEMIF_VALIDATE_INITIALIZED(MEMIF_SID_DEINIT);
#endif

    /* Reset device states */
    for (uint8 idx = 0u; idx < MEMIF_TOTAL_NUM_DEVICES; idx++) {
        MemIf_DeviceState[idx].status = MEMIF_UNINIT;
        MemIf_DeviceState[idx].jobResult = MEMIF_JOB_OK;
        MemIf_DeviceState[idx].isInitialized = FALSE;
    }

    MemIf_ConfigPtr = NULL_PTR;
    MemIf_ModuleInitialized = FALSE;
}

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @return None
 */
#if (MEMIF_VERSION_INFO_API == STD_ON)
void MemIf_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, MEMIF_SID_GETVERSIONINFO, MEMIF_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = MEMIF_VENDOR_ID;
    versioninfo->moduleID = MEMIF_MODULE_ID;
    versioninfo->sw_major_version = MEMIF_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = MEMIF_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = MEMIF_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Reads data from a block
 * @param DeviceIndex Index of the device (Fee or Ea)
 * @param BlockNumber Number of the block to read
 * @param DataBufferPtr Pointer to data buffer
 * @param Length Number of bytes to read
 * @return E_OK if request accepted, E_NOT_OK if rejected
 */
Std_ReturnType MemIf_Read(uint8 DeviceIndex, uint16 BlockNumber, uint8* DataBufferPtr, uint16 Length)
{
    Std_ReturnType result = E_NOT_OK;

#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    MEMIF_VALIDATE_INITIALIZED_RET(MEMIF_SID_READ, E_NOT_OK);
    MEMIF_VALIDATE_DEVICE_INDEX_RET(MEMIF_SID_READ, DeviceIndex, E_NOT_OK);
    MEMIF_VALIDATE_POINTER(MEMIF_SID_READ, DataBufferPtr);
    
    if (MemIf_IsBlockNumberValid(DeviceIndex, BlockNumber) == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, MEMIF_SID_READ, MEMIF_E_PARAM_BLOCK);
        return E_NOT_OK;
    }
#endif

    if (MemIf_DeviceState[DeviceIndex].status == MEMIF_IDLE) {
        /* Route to appropriate underlying driver */
        if (DeviceIndex == MEMIF_DEVICE_INDEX_FEE) {
#if (MEMIF_FEE_ENABLED == STD_ON)
            result = Fee_Read(BlockNumber, 0u, DataBufferPtr, Length);
#endif
        } else if (DeviceIndex == MEMIF_DEVICE_INDEX_EA) {
#if (MEMIF_EA_ENABLED == STD_ON)
            result = Ea_Read(BlockNumber, 0u, DataBufferPtr, Length);
#endif
        }
        
        if (result == E_OK) {
            MemIf_DeviceState[DeviceIndex].status = MEMIF_BUSY;
            MemIf_DeviceState[DeviceIndex].jobResult = MEMIF_JOB_PENDING;
        }
    }

    return result;
}

/**
 * @brief Writes data to a block
 * @param DeviceIndex Index of the device (Fee or Ea)
 * @param BlockNumber Number of the block to write
 * @param DataBufferPtr Pointer to data buffer
 * @return E_OK if request accepted, E_NOT_OK if rejected
 */
Std_ReturnType MemIf_Write(uint8 DeviceIndex, uint16 BlockNumber, const uint8* DataBufferPtr)
{
    Std_ReturnType result = E_NOT_OK;

#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    MEMIF_VALIDATE_INITIALIZED_RET(MEMIF_SID_WRITE, E_NOT_OK);
    MEMIF_VALIDATE_DEVICE_INDEX_RET(MEMIF_SID_WRITE, DeviceIndex, E_NOT_OK);
    MEMIF_VALIDATE_POINTER(MEMIF_SID_WRITE, DataBufferPtr);
    
    if (MemIf_IsBlockNumberValid(DeviceIndex, BlockNumber) == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, MEMIF_SID_WRITE, MEMIF_E_PARAM_BLOCK);
        return E_NOT_OK;
    }
#endif

    if (MemIf_DeviceState[DeviceIndex].status == MEMIF_IDLE) {
        /* Route to appropriate underlying driver */
        if (DeviceIndex == MEMIF_DEVICE_INDEX_FEE) {
#if (MEMIF_FEE_ENABLED == STD_ON)
            result = Fee_Write(BlockNumber, DataBufferPtr);
#endif
        } else if (DeviceIndex == MEMIF_DEVICE_INDEX_EA) {
#if (MEMIF_EA_ENABLED == STD_ON)
            result = Ea_Write(BlockNumber, DataBufferPtr);
#endif
        }
        
        if (result == E_OK) {
            MemIf_DeviceState[DeviceIndex].status = MEMIF_BUSY;
            MemIf_DeviceState[DeviceIndex].jobResult = MEMIF_JOB_PENDING;
        }
    }

    return result;
}

/**
 * @brief Cancels ongoing job
 * @param DeviceIndex Index of the device
 * @return None
 */
void MemIf_Cancel(uint8 DeviceIndex)
{
#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    MEMIF_VALIDATE_INITIALIZED(MEMIF_SID_CANCEL);
    MEMIF_VALIDATE_DEVICE_INDEX(MEMIF_SID_CANCEL, DeviceIndex);
#endif

    /* Route to appropriate underlying driver */
    if (DeviceIndex == MEMIF_DEVICE_INDEX_FEE) {
#if (MEMIF_FEE_ENABLED == STD_ON)
        Fee_Cancel();
#endif
    } else if (DeviceIndex == MEMIF_DEVICE_INDEX_EA) {
#if (MEMIF_EA_ENABLED == STD_ON)
        Ea_Cancel();
#endif
    }

    MemIf_DeviceState[DeviceIndex].status = MEMIF_IDLE;
    MemIf_DeviceState[DeviceIndex].jobResult = MEMIF_JOB_CANCELED;
}

/**
 * @brief Gets the status of the device
 * @param DeviceIndex Index of the device
 * @return Status of the device
 */
MemIf_StatusType MemIf_GetStatus(uint8 DeviceIndex)
{
    MemIf_StatusType status = MEMIF_UNINIT;

#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    if (MemIf_ModuleInitialized == FALSE) {
        return MEMIF_UNINIT;
    }
    
    if (DeviceIndex >= MEMIF_TOTAL_NUM_DEVICES) {
        Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, MEMIF_SID_GETSTATUS, MEMIF_E_PARAM_DEVICE_INDEX);
        return MEMIF_UNINIT;
    }
#endif

    /* Get status from underlying driver */
    if (DeviceIndex == MEMIF_DEVICE_INDEX_FEE) {
#if (MEMIF_FEE_ENABLED == STD_ON)
        status = (MemIf_StatusType)Fee_GetStatus();
#endif
    } else if (DeviceIndex == MEMIF_DEVICE_INDEX_EA) {
#if (MEMIF_EA_ENABLED == STD_ON)
        status = (MemIf_StatusType)Ea_GetStatus();
#endif
    }

    /* Update internal state */
    MemIf_DeviceState[DeviceIndex].status = status;

    return status;
}

/**
 * @brief Gets the result of the last job
 * @param DeviceIndex Index of the device
 * @return Job result
 */
MemIf_JobResultType MemIf_GetJobResult(uint8 DeviceIndex)
{
    MemIf_JobResultType result = MEMIF_JOB_FAILED;

#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    MEMIF_VALIDATE_INITIALIZED_RET(MEMIF_SID_GETJOBRESULT, MEMIF_JOB_FAILED);
    MEMIF_VALIDATE_DEVICE_INDEX_RET(MEMIF_SID_GETJOBRESULT, DeviceIndex, MEMIF_JOB_FAILED);
#endif

    /* Get result from underlying driver */
    if (DeviceIndex == MEMIF_DEVICE_INDEX_FEE) {
#if (MEMIF_FEE_ENABLED == STD_ON)
        result = (MemIf_JobResultType)Fee_GetJobResult();
#endif
    } else if (DeviceIndex == MEMIF_DEVICE_INDEX_EA) {
#if (MEMIF_EA_ENABLED == STD_ON)
        result = (MemIf_JobResultType)Ea_GetJobResult();
#endif
    }

    MemIf_DeviceState[DeviceIndex].jobResult = result;

    return result;
}

/**
 * @brief Invalidates a block
 * @param DeviceIndex Index of the device
 * @param BlockNumber Number of the block to invalidate
 * @return E_OK if request accepted, E_NOT_OK if rejected
 */
Std_ReturnType MemIf_InvalidateBlock(uint8 DeviceIndex, uint16 BlockNumber)
{
    Std_ReturnType result = E_NOT_OK;

#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    MEMIF_VALIDATE_INITIALIZED_RET(MEMIF_SID_INVALIDATEBLOCK, E_NOT_OK);
    MEMIF_VALIDATE_DEVICE_INDEX_RET(MEMIF_SID_INVALIDATEBLOCK, DeviceIndex, E_NOT_OK);
    
    if (MemIf_IsBlockNumberValid(DeviceIndex, BlockNumber) == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, MEMIF_SID_INVALIDATEBLOCK, MEMIF_E_PARAM_BLOCK);
        return E_NOT_OK;
    }
#endif

    if (MemIf_DeviceState[DeviceIndex].status == MEMIF_IDLE) {
        /* Route to appropriate underlying driver */
        if (DeviceIndex == MEMIF_DEVICE_INDEX_FEE) {
#if (MEMIF_FEE_ENABLED == STD_ON)
            result = Fee_InvalidateBlock(BlockNumber);
#endif
        } else if (DeviceIndex == MEMIF_DEVICE_INDEX_EA) {
#if (MEMIF_EA_ENABLED == STD_ON)
            result = Ea_InvalidateBlock(BlockNumber);
#endif
        }
        
        if (result == E_OK) {
            MemIf_DeviceState[DeviceIndex].status = MEMIF_BUSY;
            MemIf_DeviceState[DeviceIndex].jobResult = MEMIF_JOB_PENDING;
        }
    }

    return result;
}

/**
 * @brief Erases a block
 * @param DeviceIndex Index of the device
 * @param BlockNumber Number of the block to erase
 * @return E_OK if request accepted, E_NOT_OK if rejected
 */
Std_ReturnType MemIf_EraseBlock(uint8 DeviceIndex, uint16 BlockNumber)
{
    Std_ReturnType result = E_NOT_OK;

#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    MEMIF_VALIDATE_INITIALIZED_RET(MEMIF_SID_ERASEBLOCK, E_NOT_OK);
    MEMIF_VALIDATE_DEVICE_INDEX_RET(MEMIF_SID_ERASEBLOCK, DeviceIndex, E_NOT_OK);
    
    if (MemIf_IsBlockNumberValid(DeviceIndex, BlockNumber) == FALSE) {
        Det_ReportError(MEMIF_MODULE_ID, MEMIF_INSTANCE_ID, MEMIF_SID_ERASEBLOCK, MEMIF_E_PARAM_BLOCK);
        return E_NOT_OK;
    }
#endif

    if (MemIf_DeviceState[DeviceIndex].status == MEMIF_IDLE) {
        /* Route to appropriate underlying driver */
        if (DeviceIndex == MEMIF_DEVICE_INDEX_FEE) {
#if (MEMIF_FEE_ENABLED == STD_ON)
            result = Fee_EraseImmediateBlock(BlockNumber);
#endif
        } else if (DeviceIndex == MEMIF_DEVICE_INDEX_EA) {
#if (MEMIF_EA_ENABLED == STD_ON)
            result = Ea_EraseBlock(BlockNumber);
#endif
        }
        
        if (result == E_OK) {
            MemIf_DeviceState[DeviceIndex].status = MEMIF_BUSY;
            MemIf_DeviceState[DeviceIndex].jobResult = MEMIF_JOB_PENDING;
        }
    }

    return result;
}

/**
 * @brief Main function for periodic processing
 * @param DeviceIndex Index of the device
 * @return None
 */
void MemIf_MainFunction(uint8 DeviceIndex)
{
#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    MEMIF_VALIDATE_INITIALIZED(MEMIF_SID_MAINFUNCTION);
    MEMIF_VALIDATE_DEVICE_INDEX(MEMIF_SID_MAINFUNCTION, DeviceIndex);
#endif

    /* Call underlying driver's main function */
    if (DeviceIndex == MEMIF_DEVICE_INDEX_FEE) {
#if (MEMIF_FEE_ENABLED == STD_ON)
        Fee_MainFunction();
#endif
    } else if (DeviceIndex == MEMIF_DEVICE_INDEX_EA) {
#if (MEMIF_EA_ENABLED == STD_ON)
        Ea_MainFunction();
#endif
    }

    /* Update status after main function processing */
    if (MemIf_DeviceState[DeviceIndex].status == MEMIF_BUSY) {
        MemIf_JobResultType jobResult = MemIf_GetJobResult(DeviceIndex);
        
        if (jobResult != MEMIF_JOB_PENDING) {
            MemIf_DeviceState[DeviceIndex].status = MEMIF_IDLE;
        }
    }
}

/**
 * @brief Sets the device mode
 * @param DeviceIndex Index of the device
 * @param Mode Mode to set (slow/fast)
 * @return None
 */
void MemIf_SetMode(uint8 DeviceIndex, MemIf_ModeType Mode)
{
#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    MEMIF_VALIDATE_INITIALIZED(MEMIF_SID_MAINFUNCTION);
    MEMIF_VALIDATE_DEVICE_INDEX(MEMIF_SID_MAINFUNCTION, DeviceIndex);
#endif

    /* Route to appropriate underlying driver */
    if (DeviceIndex == MEMIF_DEVICE_INDEX_FEE) {
#if (MEMIF_FEE_ENABLED == STD_ON)
        Fee_SetMode((MemIf_ModeType)Mode);
#endif
    } else if (DeviceIndex == MEMIF_DEVICE_INDEX_EA) {
#if (MEMIF_EA_ENABLED == STD_ON)
        Ea_SetMode((MemIf_ModeType)Mode);
#endif
    }
}

/**
 * @brief Gets the number of devices
 * @return Number of configured devices
 */
uint8 MemIf_GetNumberOfDevices(void)
{
#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    MEMIF_VALIDATE_INITIALIZED_RET(MEMIF_SID_INIT, 0u);
#endif

    return MEMIF_TOTAL_NUM_DEVICES;
}

#define MEMIF_STOP_SEC_CODE
#include "MemIf_MemMap.h"
