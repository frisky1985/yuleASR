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
} NvM_JobQueueEntryType;

/* Block runtime state */
typedef struct
{
    NvM_RequestResultType LastResult;
    uint8 JobPending;
    uint8 WriteCounter;
    boolean DataValid;
    boolean DataChanged;
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

    /* Current active job */
    NvM_JobQueueEntryType* CurrentJob;
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
STATIC Std_ReturnType NvM_QueuePush(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, const NvM_JobQueueEntryType* Entry);
STATIC Std_ReturnType NvM_QueuePop(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, NvM_JobQueueEntryType* Entry);
STATIC boolean NvM_QueueIsEmpty(uint8 Count);
STATIC boolean NvM_QueueIsFull(uint8 Count, uint8 MaxSize);

STATIC const NvM_BlockDescriptorType* NvM_GetBlockDescriptor(NvM_BlockIdType BlockId);
STATIC Std_ReturnType NvM_ValidateBlockId(NvM_BlockIdType BlockId);

STATIC void NvM_ProcessReadJob(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_ProcessWriteJob(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_ProcessRestoreJob(NvM_JobQueueEntryType* JobPtr);

STATIC uint32 NvM_CalculateCrc(const void* DataPtr, uint16 Length, NvM_BlockCrcType CrcType);
STATIC uint8 NvM_CalculateCrc8(const uint8* DataPtr, uint16 Length);
STATIC uint16 NvM_CalculateCrc16(const uint8* DataPtr, uint16 Length);
STATIC uint32 NvM_CalculateCrc32(const uint8* DataPtr, uint16 Length);

STATIC void NvM_CopyRomDataToRam(NvM_BlockIdType BlockId, void* DestPtr);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define NVM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Push job into queue
 */
STATIC Std_ReturnType NvM_QueuePush(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, const NvM_JobQueueEntryType* Entry)
{
    Std_ReturnType result = E_NOT_OK;

    if ((Queue != NULL_PTR) && (Head != NULL_PTR) && (Tail != NULL_PTR) && (Count != NULL_PTR) && (Entry != NULL_PTR))
    {
        if (*Count < MaxSize)
        {
            Queue[*Tail] = *Entry;
            *Tail = (*Tail + 1U) % MaxSize;
            (*Count)++;
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Pop job from queue
 */
STATIC Std_ReturnType NvM_QueuePop(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, NvM_JobQueueEntryType* Entry)
{
    Std_ReturnType result = E_NOT_OK;

    if ((Queue != NULL_PTR) && (Head != NULL_PTR) && (Tail != NULL_PTR) && (Count != NULL_PTR) && (Entry != NULL_PTR))
    {
        if (*Count > 0U)
        {
            *Entry = Queue[*Head];
            *Head = (*Head + 1U) % MaxSize;
            (*Count)--;
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Check if queue is empty
 */
STATIC boolean NvM_QueueIsEmpty(uint8 Count)
{
    return (Count == 0U) ? TRUE : FALSE;
}

/**
 * @brief   Check if queue is full
 */
STATIC boolean NvM_QueueIsFull(uint8 Count, uint8 MaxSize)
{
    return (Count >= MaxSize) ? TRUE : FALSE;
}

/**
 * @brief   Get block descriptor for given block ID
 */
STATIC const NvM_BlockDescriptorType* NvM_GetBlockDescriptor(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* result = NULL_PTR;
    uint8 i;

    if (NvM_InternalState.ConfigPtr != NULL_PTR)
    {
        for (i = 0U; i < NvM_InternalState.ConfigPtr->NumBlockDescriptors; i++)
        {
            if (NvM_InternalState.ConfigPtr->BlockDescriptors[i].BlockId == BlockId)
            {
                result = &NvM_InternalState.ConfigPtr->BlockDescriptors[i];
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Validate block ID
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
 * @brief   Process read job
 */
STATIC void NvM_ProcessReadJob(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            blockNumber = blockDesc->BlockBaseNumber;

            /* Call MemIf to read from NV memory */
            memIfResult = MemIf_Read(blockDesc->DeviceId, blockNumber, 0U,
                                     (uint8*)JobPtr->DataPtr, blockDesc->NvBlockLength);

            if (memIfResult == E_OK)
            {
                JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
            }
            else
            {
                /* Read failed immediately, try to restore from ROM */
                NvM_CopyRomDataToRam(JobPtr->BlockId, JobPtr->DataPtr);
                JobPtr->Result = NVM_REQ_OK;
                JobPtr->JobState = NVM_JOB_STATE_IDLE;
                NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_OK;
                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
            }
        }
        else
        {
            JobPtr->Result = NVM_REQ_NOT_OK;
            JobPtr->JobState = NVM_JOB_STATE_IDLE;
        }
    }
}

/**
 * @brief   Process write job
 */
STATIC void NvM_ProcessWriteJob(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            blockNumber = blockDesc->BlockBaseNumber;

            /* Call MemIf to write to NV memory */
            memIfResult = MemIf_Write(blockDesc->DeviceId, blockNumber,
                                      (uint8*)JobPtr->DataPtr);

            if (memIfResult == E_OK)
            {
                JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
            }
            else
            {
                JobPtr->Result = NVM_REQ_NOT_OK;
                JobPtr->JobState = NVM_JOB_STATE_IDLE;
                NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_NOT_OK;
                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
            }
        }
        else
        {
            JobPtr->Result = NVM_REQ_NOT_OK;
            JobPtr->JobState = NVM_JOB_STATE_IDLE;
        }
    }
}

/**
 * @brief   Process restore job
 */
STATIC void NvM_ProcessRestoreJob(NvM_JobQueueEntryType* JobPtr)
{
    if (JobPtr != NULL_PTR)
    {
        /* Copy ROM data to RAM */
        NvM_CopyRomDataToRam(JobPtr->BlockId, JobPtr->DataPtr);

        JobPtr->Result = NVM_REQ_OK;
        JobPtr->JobState = NVM_JOB_STATE_IDLE;
        NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_OK;
        NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the NvM module
 * @param   ConfigPtr - Pointer to configuration structure
 * @return  None
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
    }

    /* Clear current job */
    NvM_InternalState.CurrentJob = NULL_PTR;

    /* Set module state to idle */
    NvM_InternalState.State = NVM_STATE_IDLE;
}

/**
 * @brief   Read block from NV memory
 * @param   BlockId     - Block identifier
 * @param   NvM_DstPtr  - Destination pointer for read data
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)
{
    Std_ReturnType result = E_NOT_OK;
    NvM_JobQueueEntryType jobEntry;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x04U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_DstPtr == NULL_PTR)
    {
        NVM_DET_REPORT_ERROR(0x04U, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x04U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Check if block already has pending job */
    if (NvM_InternalState.BlockStates[BlockId].JobPending == 0U)
    {
        /* Prepare job entry */
        jobEntry.BlockId = BlockId;
        jobEntry.JobType = NVM_JOB_TYPE_READ;
        jobEntry.JobState = NVM_JOB_STATE_PENDING;
        jobEntry.DataPtr = NvM_DstPtr;
        jobEntry.Result = NVM_REQ_PENDING;
        jobEntry.RetryCount = 0U;

        /* Add to standard queue */
        if (NvM_QueuePush(NvM_InternalState.StandardQueue,
                          &NvM_InternalState.StandardQueueHead,
                          &NvM_InternalState.StandardQueueTail,
                          &NvM_InternalState.StandardQueueCount,
                          NVM_SIZE_STANDARD_JOB_QUEUE,
                          &jobEntry) == E_OK)
        {
            NvM_InternalState.BlockStates[BlockId].JobPending = 1U;
            result = E_OK;
        }
    }
    else
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(0x04U, NVM_E_BLOCK_PENDING);
#endif
    }

    return result;
}

/**
 * @brief   Write block to NV memory
 * @param   BlockId     - Block identifier
 * @param   NvM_SrcPtr  - Source pointer for write data
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)
{
    Std_ReturnType result = E_NOT_OK;
    NvM_JobQueueEntryType jobEntry;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x05U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_SrcPtr == NULL_PTR)
    {
        NVM_DET_REPORT_ERROR(0x05U, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x05U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Check if block already has pending job */
    if (NvM_InternalState.BlockStates[BlockId].JobPending == 0U)
    {
        /* Prepare job entry */
        jobEntry.BlockId = BlockId;
        jobEntry.JobType = NVM_JOB_TYPE_WRITE;
        jobEntry.JobState = NVM_JOB_STATE_PENDING;
        jobEntry.DataPtr = (void*)NvM_SrcPtr;
        jobEntry.Result = NVM_REQ_PENDING;
        jobEntry.RetryCount = 0U;

        /* Add to standard queue */
        if (NvM_QueuePush(NvM_InternalState.StandardQueue,
                          &NvM_InternalState.StandardQueueHead,
                          &NvM_InternalState.StandardQueueTail,
                          &NvM_InternalState.StandardQueueCount,
                          NVM_SIZE_STANDARD_JOB_QUEUE,
                          &jobEntry) == E_OK)
        {
            NvM_InternalState.BlockStates[BlockId].JobPending = 1U;
            NvM_InternalState.BlockStates[BlockId].DataChanged = TRUE;
            result = E_OK;
        }
    }
    else
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(0x05U, NVM_E_BLOCK_PENDING);
#endif
    }

    return result;
}

/**
 * @brief   Restore block defaults from ROM
 * @param   BlockId     - Block identifier
 * @param   NvM_DestPtr - Destination pointer
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_RestoreBlockDefaults(NvM_BlockIdType BlockId, void* NvM_DestPtr)
{
    Std_ReturnType result = E_NOT_OK;
    NvM_JobQueueEntryType jobEntry;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x06U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_DestPtr == NULL_PTR)
    {
        NVM_DET_REPORT_ERROR(0x06U, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x06U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Check if block already has pending job */
    if (NvM_InternalState.BlockStates[BlockId].JobPending == 0U)
    {
        /* Prepare job entry */
        jobEntry.BlockId = BlockId;
        jobEntry.JobType = NVM_JOB_TYPE_RESTORE;
        jobEntry.JobState = NVM_JOB_STATE_PENDING;
        jobEntry.DataPtr = NvM_DestPtr;
        jobEntry.Result = NVM_REQ_PENDING;
        jobEntry.RetryCount = 0U;

        /* Add to immediate queue (high priority) */
        if (NvM_QueuePush(NvM_InternalState.ImmediateQueue,
                          &NvM_InternalState.ImmediateQueueHead,
                          &NvM_InternalState.ImmediateQueueTail,
                          &NvM_InternalState.ImmediateQueueCount,
                          NVM_SIZE_IMMEDIATE_JOB_QUEUE,
                          &jobEntry) == E_OK)
        {
            NvM_InternalState.BlockStates[BlockId].JobPending = 1U;
            result = E_OK;
        }
    }
    else
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(0x06U, NVM_E_BLOCK_PENDING);
#endif
    }

    return result;
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
 * @brief   Main function for NvM processing
 * @param   None
 * @return  None
 */
void NvM_MainFunction(void)
{
    NvM_JobQueueEntryType jobEntry;
    MemIf_StatusType memIfStatus;
    const NvM_BlockDescriptorType* blockDesc;

    if (NvM_InternalState.State == NVM_STATE_IDLE)
    {
        /* Process immediate queue first (high priority) */
        if (!NvM_QueueIsEmpty(NvM_InternalState.ImmediateQueueCount))
        {
            if (NvM_QueuePop(NvM_InternalState.ImmediateQueue,
                             &NvM_InternalState.ImmediateQueueHead,
                             &NvM_InternalState.ImmediateQueueTail,
                             &NvM_InternalState.ImmediateQueueCount,
                             NVM_SIZE_IMMEDIATE_JOB_QUEUE,
                             &jobEntry) == E_OK)
            {
                NvM_InternalState.CurrentJob = &jobEntry;

                switch (jobEntry.JobType)
                {
                    case NVM_JOB_TYPE_RESTORE:
                        NvM_ProcessRestoreJob(&jobEntry);
                        break;

                    default:
                        break;
                }

                NvM_InternalState.CurrentJob = NULL_PTR;
            }
        }
        /* Process standard queue */
        else if (!NvM_QueueIsEmpty(NvM_InternalState.StandardQueueCount))
        {
            if (NvM_QueuePop(NvM_InternalState.StandardQueue,
                             &NvM_InternalState.StandardQueueHead,
                             &NvM_InternalState.StandardQueueTail,
                             &NvM_InternalState.StandardQueueCount,
                             NVM_SIZE_STANDARD_JOB_QUEUE,
                             &jobEntry) == E_OK)
            {
                NvM_InternalState.CurrentJob = &jobEntry;
                NvM_InternalState.State = NVM_STATE_BUSY;

                /* Process the job based on type */
                switch (jobEntry.JobType)
                {
                    case NVM_JOB_TYPE_READ:
                        NvM_ProcessReadJob(&jobEntry);
                        break;

                    case NVM_JOB_TYPE_WRITE:
                        NvM_ProcessWriteJob(&jobEntry);
                        break;

                    default:
                        NvM_InternalState.State = NVM_STATE_IDLE;
                        break;
                }
            }
        }
    }
    else if (NvM_InternalState.State == NVM_STATE_BUSY)
    {
        /* Check if current job is complete */
        if (NvM_InternalState.CurrentJob != NULL_PTR)
        {
            blockDesc = NvM_GetBlockDescriptor(NvM_InternalState.CurrentJob->BlockId);

            if (blockDesc != NULL_PTR)
            {
                memIfStatus = MemIf_GetStatus(blockDesc->DeviceId);

                if (memIfStatus == MEMIF_IDLE)
                {
                    /* Job completed */
                    NvM_InternalState.CurrentJob->Result = NVM_REQ_OK;
                    NvM_InternalState.CurrentJob->JobState = NVM_JOB_STATE_IDLE;
                    NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_OK;
                    NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].JobPending = 0U;

                    NvM_InternalState.CurrentJob = NULL_PTR;
                    NvM_InternalState.State = NVM_STATE_IDLE;
                }
                else if (memIfStatus == MEMIF_UNINIT)
                {
                    /* Job failed */
                    NvM_InternalState.CurrentJob->Result = NVM_REQ_NOT_OK;
                    NvM_InternalState.CurrentJob->JobState = NVM_JOB_STATE_IDLE;
                    NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_NOT_OK;
                    NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].JobPending = 0U;

                    NvM_InternalState.CurrentJob = NULL_PTR;
                    NvM_InternalState.State = NVM_STATE_IDLE;
                }
                /* else: still busy, wait for next cycle */
            }
            else
            {
                NvM_InternalState.CurrentJob = NULL_PTR;
                NvM_InternalState.State = NVM_STATE_IDLE;
            }
        }
        else
        {
            NvM_InternalState.State = NVM_STATE_IDLE;
        }
    }
    else
    {
        /* NVM_STATE_UNINIT - do nothing */
    }
}

#define NVM_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
