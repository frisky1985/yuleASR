/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Service Layer)
* Dependencies         : MemIf, Fee, Ea
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-15
* Author               : AI Agent (NvM Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "NvM.h"
#include "NvM_Cfg.h"
#include "Det.h"
#include "MemMap.h"
#include <string.h>

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define NVM_INSTANCE_ID                 (0x00U)

/* Development error codes */
#define NVM_E_PARAM_POINTER             (0x01U)
#define NVM_E_PARAM_BLOCK_ID            (0x02U)
#define NVM_E_NOT_INITIALIZED           (0x03U)
#define NVM_E_BLOCK_PENDING             (0x04U)
#define NVM_E_BLOCK_CONFIG              (0x05U)

/* Module state */
#define NVM_STATE_UNINIT                (0x00U)
#define NVM_STATE_IDLE                  (0x01U)
#define NVM_STATE_BUSY                  (0x02U)

/* Job types */
#define NVM_JOB_TYPE_NONE               (0x00U)
#define NVM_JOB_TYPE_READ               (0x01U)
#define NVM_JOB_TYPE_WRITE              (0x02U)
#define NVM_JOB_TYPE_RESTORE            (0x03U)
#define NVM_JOB_TYPE_ERASE              (0x04U)
#define NVM_JOB_TYPE_INVALIDATE         (0x05U)

/* Job states */
#define NVM_JOB_STATE_IDLE              (0x00U)
#define NVM_JOB_STATE_PENDING           (0x01U)
#define NVM_JOB_STATE_PROCESSING        (0x02U)

/* CRC calculation constants */
#define NVM_CRC8_POLYNOMIAL             (0x1DU)
#define NVM_CRC16_POLYNOMIAL            (0x1021U)
#define NVM_CRC32_POLYNOMIAL            (0x04C11DB7U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (NVM_DEV_ERROR_DETECT == STD_ON)
    #define NVM_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(NVM_MODULE_ID, NVM_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define NVM_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* Job queue entry */
typedef struct
{
    NvM_BlockIdType BlockId;
    uint8 JobType;
    uint8 JobState;
    void* DataPtr;
    NvM_RequestResultType Result;
    uint8 RetryCount;
    uint8 CopyIndex;
} NvM_JobQueueEntryType;

/* Block runtime state */
typedef struct
{
    NvM_RequestResultType LastResult;
    uint8 JobPending;
    uint8 WriteCounter;
    boolean DataValid;
    boolean DataChanged;
    uint8 DataIndex;
    boolean BlockLocked;
    boolean WriteOnceDone;
} NvM_BlockStateType;

/* Module internal state */
typedef struct
{
    uint8 State;
    const NvM_ConfigType* ConfigPtr;

    /* Standard job queue */
    NvM_JobQueueEntryType StandardQueue[NVM_SIZE_STANDARD_JOB_QUEUE];
    uint8 StandardQueueHead;
    uint8 StandardQueueTail;
    uint8 StandardQueueCount;

    /* Immediate job queue (high priority) */
    NvM_JobQueueEntryType ImmediateQueue[NVM_SIZE_IMMEDIATE_JOB_QUEUE];
    uint8 ImmediateQueueHead;
    uint8 ImmediateQueueTail;
    uint8 ImmediateQueueCount;

    /* Block states */
    NvM_BlockStateType BlockStates[NVM_NUM_OF_NVRAM_BLOCKS];

    /* Current active job (stored by value to avoid dangling pointer) */
    NvM_JobQueueEntryType ActiveJob;
    NvM_JobQueueEntryType* CurrentJob;

    /* Multi-block operation flags */
    boolean ReadAllInProgress;
    boolean WriteAllInProgress;
    boolean KillReadAllRequested;
    boolean KillWriteAllRequested;
    uint16 ReadAllPendingCount;
    uint16 WriteAllPendingCount;
} NvM_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define NVM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC NvM_InternalStateType NvM_InternalState;

#define NVM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType NvM_ValidateBlockId(NvM_BlockIdType BlockId);

STATIC uint32 NvM_CalculateCrc(const void* DataPtr, uint16 Length, NvM_BlockCrcType CrcType);
STATIC uint8 NvM_CalculateCrc8(const uint8* DataPtr, uint16 Length);
STATIC uint16 NvM_CalculateCrc16(const uint8* DataPtr, uint16 Length);
STATIC uint32 NvM_CalculateCrc32(const uint8* DataPtr, uint16 Length);
STATIC uint8 NvM_GetCrcSize(NvM_BlockCrcType CrcType);

STATIC void NvM_CopyRomDataToRam(NvM_BlockIdType BlockId, void* DestPtr);

STATIC void NvM_InvokeJobEndCallback(NvM_BlockIdType BlockId, NvM_RequestResultType Result);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/

STATIC Std_ReturnType NvM_QueuePush(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, const NvM_JobQueueEntryType* Entry);
STATIC Std_ReturnType NvM_QueuePop(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, NvM_JobQueueEntryType* Entry);
STATIC boolean NvM_QueueIsEmpty(uint8 Count);
STATIC boolean NvM_QueueIsFull(uint8 Count, uint8 MaxSize);
STATIC const NvM_BlockDescriptorType* NvM_GetBlockDescriptor(NvM_BlockIdType BlockId);
STATIC void NvM_ProcessReadJob(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_ProcessWriteJob(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_ProcessRestoreJob(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_ProcessEraseJob(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_ProcessInvalidateJob(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_ReadRedundantBlock(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_WriteRedundantBlock(NvM_JobQueueEntryType* JobPtr);
#define NVM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Push job into queue
 */
STATIC Std_ReturnType NvM_ValidateBlockId(NvM_BlockIdType BlockId)
{
    Std_ReturnType result = E_NOT_OK;

    if ((BlockId > 0U) && (BlockId < NVM_NUM_OF_NVRAM_BLOCKS))
    {
        if (NvM_GetBlockDescriptor(BlockId) != NULL_PTR)
        {
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Calculate CRC8
 */
STATIC uint8 NvM_CalculateCrc8(const uint8* DataPtr, uint16 Length)
{
    uint8 crc = 0xFFU;
    uint16 i;
    uint8 bit;

    for (i = 0U; i < Length; i++)
    {
        crc ^= DataPtr[i];
        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x80U) != 0U)
            {
                crc = (crc << 1U) ^ NVM_CRC8_POLYNOMIAL;
            }
            else
            {
                crc = crc << 1U;
            }
        }
    }

    return crc;
}

/**
 * @brief   Calculate CRC16
 */
STATIC uint16 NvM_CalculateCrc16(const uint8* DataPtr, uint16 Length)
{
    uint16 crc = 0xFFFFU;
    uint16 i;
    uint8 bit;

    for (i = 0U; i < Length; i++)
    {
        crc ^= ((uint16)DataPtr[i] << 8U);
        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (crc << 1U) ^ NVM_CRC16_POLYNOMIAL;
            }
            else
            {
                crc = crc << 1U;
            }
        }
    }

    return crc;
}

/**
 * @brief   Calculate CRC32
 */
STATIC uint32 NvM_CalculateCrc32(const uint8* DataPtr, uint16 Length)
{
    uint32 crc = 0xFFFFFFFFU;
    uint16 i;
    uint8 bit;

    for (i = 0U; i < Length; i++)
    {
        crc ^= ((uint32)DataPtr[i] << 24U);
        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x80000000U) != 0U)
            {
                crc = (crc << 1U) ^ NVM_CRC32_POLYNOMIAL;
            }
            else
            {
                crc = crc << 1U;
            }
        }
    }

    return crc;
}

/**
 * @brief   Calculate CRC based on type
 */
STATIC uint32 NvM_CalculateCrc(const void* DataPtr, uint16 Length, NvM_BlockCrcType CrcType)
{
    uint32 crc = 0U;
    const uint8* dataPtr = (const uint8*)DataPtr;

    switch (CrcType)
    {
        case NVM_CRC_8:
            crc = (uint32)NvM_CalculateCrc8(dataPtr, Length);
            break;

        case NVM_CRC_16:
            crc = (uint32)NvM_CalculateCrc16(dataPtr, Length);
            break;

        case NVM_CRC_32:
            crc = NvM_CalculateCrc32(dataPtr, Length);
            break;

        case NVM_CRC_NONE:
        default:
            crc = 0U;
            break;
    }

    return crc;
}

/**
 * @brief   Get CRC size in bytes based on CRC type
 */
STATIC uint8 NvM_GetCrcSize(NvM_BlockCrcType CrcType)
{
    uint8 size = 0U;

    switch (CrcType)
    {
        case NVM_CRC_8:
            size = 1U;
            break;

        case NVM_CRC_16:
            size = 2U;
            break;

        case NVM_CRC_32:
            size = 4U;
            break;

        case NVM_CRC_NONE:
        default:
            size = 0U;
            break;
    }

    return size;
}

/**
 * @brief   Copy ROM default data to RAM
 */
STATIC void NvM_CopyRomDataToRam(NvM_BlockIdType BlockId, void* DestPtr)
{
    const NvM_BlockDescriptorType* blockDesc = NvM_GetBlockDescriptor(BlockId);

    if ((blockDesc != NULL_PTR) && (DestPtr != NULL_PTR) && (blockDesc->RomBlockData != NULL_PTR))
    {
        (void)memcpy(DestPtr, blockDesc->RomBlockData, blockDesc->NvBlockLength);
        NvM_InternalState.BlockStates[BlockId].DataValid = TRUE;
    }
}

/**
 * @brief   Invoke JobEndCallback for a block if configured
 */
STATIC void NvM_InvokeJobEndCallback(NvM_BlockIdType BlockId, NvM_RequestResultType Result)
{
    const NvM_BlockDescriptorType* blockDesc = NvM_GetBlockDescriptor(BlockId);

    (void)Result; /* Parameter reserved for future use */

    if ((blockDesc != NULL_PTR) && (blockDesc->JobEndCallback != NULL_PTR))
    {
        blockDesc->JobEndCallback();
    }
}

/**
 * @brief   Process redundant block read (first or second copy)
 */
void NvM_Init(const NvM_ConfigType* ConfigPtr)
{
    uint8 i;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        NVM_DET_REPORT_ERROR(0x01U, NVM_E_PARAM_POINTER);
        return;
    }
#endif

    /* Store configuration pointer */
    NvM_InternalState.ConfigPtr = ConfigPtr;

    /* Initialize queues */
    NvM_InternalState.StandardQueueHead = 0U;
    NvM_InternalState.StandardQueueTail = 0U;
    NvM_InternalState.StandardQueueCount = 0U;

    NvM_InternalState.ImmediateQueueHead = 0U;
    NvM_InternalState.ImmediateQueueTail = 0U;
    NvM_InternalState.ImmediateQueueCount = 0U;

    /* Initialize block states */
    for (i = 0U; i < NVM_NUM_OF_NVRAM_BLOCKS; i++)
    {
        NvM_InternalState.BlockStates[i].LastResult = NVM_REQ_OK;
        NvM_InternalState.BlockStates[i].JobPending = 0U;
        NvM_InternalState.BlockStates[i].WriteCounter = 0U;
        NvM_InternalState.BlockStates[i].DataValid = FALSE;
        NvM_InternalState.BlockStates[i].DataChanged = FALSE;
        NvM_InternalState.BlockStates[i].DataIndex = 0U;
        NvM_InternalState.BlockStates[i].BlockLocked = FALSE;
        NvM_InternalState.BlockStates[i].WriteOnceDone = FALSE;
    }

    /* Clear current job */
    NvM_InternalState.CurrentJob = NULL_PTR;

    /* Set module state to idle */
    NvM_InternalState.State = NVM_STATE_IDLE;

    /* Initialize multi-block operation flags */
    NvM_InternalState.ReadAllInProgress = FALSE;
    NvM_InternalState.WriteAllInProgress = FALSE;
    NvM_InternalState.KillReadAllRequested = FALSE;
    NvM_InternalState.KillWriteAllRequested = FALSE;
    NvM_InternalState.ReadAllPendingCount = 0U;
    NvM_InternalState.WriteAllPendingCount = 0U;
}

/**
 * @brief   Read block from NV memory
 * @param   BlockId     - Block identifier
 * @param   NvM_DstPtr  - Destination pointer for read data
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
void NvM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID = NVM_VENDOR_ID;
        versioninfo->moduleID = NVM_MODULE_ID;
        versioninfo->sw_major_version = NVM_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = NVM_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = NVM_SW_PATCH_VERSION;
    }
}

/**
 * @brief   Get error status for a block
 * @param   BlockId - Block identifier
 * @param   RequestResultPtr - Output pointer for request result
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType* RequestResultPtr)
{
    Std_ReturnType result = E_NOT_OK;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x0BU, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (RequestResultPtr == NULL_PTR)
    {
        NVM_DET_REPORT_ERROR(0x0BU, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if ((BlockId >= NVM_NUM_OF_NVRAM_BLOCKS) && (BlockId != 0xFFFFU))
    {
        NVM_DET_REPORT_ERROR(0x0BU, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    if (RequestResultPtr != NULL_PTR)
    {
        if (BlockId == 0xFFFFU)
        {
            /* Multi-block request status */
            *RequestResultPtr = NVM_REQ_OK;
        }
        else
        {
            *RequestResultPtr = NvM_InternalState.BlockStates[BlockId].LastResult;
        }
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Set RAM block status
 * @param   BlockId     - Block identifier
 * @param   BlockChanged - TRUE if block data has changed
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged)
{
    Std_ReturnType result = E_NOT_OK;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x0FU, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x0FU, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    if ((BlockId > 0U) && (BlockId < NVM_NUM_OF_NVRAM_BLOCKS))
    {
        NvM_InternalState.BlockStates[BlockId].DataChanged = BlockChanged;
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Erase NV block
 * @param   BlockId - Block identifier
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */

/*==================================================================================================
 *  子文件包含 (批量拆分)
 *================================================================================================*/
#include "nvm_jobs.c"
#include "nvm_read.c"
#include "nvm_write.c"
#define NVM_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
