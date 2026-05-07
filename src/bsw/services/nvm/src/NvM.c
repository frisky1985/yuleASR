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
STATIC Std_ReturnType NvM_QueuePush(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, const NvM_JobQueueEntryType* Entry);
STATIC Std_ReturnType NvM_QueuePop(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, NvM_JobQueueEntryType* Entry);
STATIC boolean NvM_QueueIsEmpty(uint8 Count);
STATIC boolean NvM_QueueIsFull(uint8 Count, uint8 MaxSize);

STATIC const NvM_BlockDescriptorType* NvM_GetBlockDescriptor(NvM_BlockIdType BlockId);
STATIC Std_ReturnType NvM_ValidateBlockId(NvM_BlockIdType BlockId);

STATIC void NvM_ProcessReadJob(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_ProcessWriteJob(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_ProcessRestoreJob(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_ProcessEraseJob(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_ProcessInvalidateJob(NvM_JobQueueEntryType* JobPtr);

STATIC uint32 NvM_CalculateCrc(const void* DataPtr, uint16 Length, NvM_BlockCrcType CrcType);
STATIC uint8 NvM_CalculateCrc8(const uint8* DataPtr, uint16 Length);
STATIC uint16 NvM_CalculateCrc16(const uint8* DataPtr, uint16 Length);
STATIC uint32 NvM_CalculateCrc32(const uint8* DataPtr, uint16 Length);
STATIC uint8 NvM_GetCrcSize(NvM_BlockCrcType CrcType);

STATIC void NvM_CopyRomDataToRam(NvM_BlockIdType BlockId, void* DestPtr);

STATIC void NvM_ReadRedundantBlock(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_WriteRedundantBlock(NvM_JobQueueEntryType* JobPtr);
STATIC void NvM_InvokeJobEndCallback(NvM_BlockIdType BlockId, NvM_RequestResultType Result);

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
STATIC void NvM_ReadRedundantBlock(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;
    uint16 readLength;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            blockNumber = blockDesc->BlockBaseNumber + JobPtr->CopyIndex;
            readLength = blockDesc->NvBlockLength;

            if (blockDesc->BlockUseCrc == TRUE)
            {
                readLength += NvM_GetCrcSize(blockDesc->CrcType);
            }

            /* Call MemIf to read from NV memory */
            memIfResult = MemIf_Read(blockDesc->DeviceId, blockNumber, 0U,
                                     (uint8*)JobPtr->DataPtr, readLength);

            if (memIfResult == E_OK)
            {
                JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
            }
            else
            {
                /* Read failed immediately, try next copy or ROM fallback */
                if (JobPtr->CopyIndex == 0U)
                {
                    JobPtr->CopyIndex = 1U;
                    NvM_ReadRedundantBlock(JobPtr);
                }
                else
                {
                    NvM_CopyRomDataToRam(JobPtr->BlockId, JobPtr->DataPtr);
                    JobPtr->Result = NVM_REQ_RESTORED_FROM_ROM;
                    JobPtr->JobState = NVM_JOB_STATE_IDLE;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_RESTORED_FROM_ROM;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
                }
            }
        }
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
    uint16 readLength;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            if (blockDesc->ManagementType == NVM_BLOCK_REDUNDANT)
            {
                JobPtr->CopyIndex = 0U;
                NvM_ReadRedundantBlock(JobPtr);
            }
            else
            {
                if (blockDesc->ManagementType == NVM_BLOCK_DATASET)
                {
                    blockNumber = blockDesc->BlockBaseNumber + NvM_InternalState.BlockStates[JobPtr->BlockId].DataIndex;
                }
                else
                {
                    blockNumber = blockDesc->BlockBaseNumber;
                }

                readLength = blockDesc->NvBlockLength;
                if (blockDesc->BlockUseCrc == TRUE)
                {
                    readLength += NvM_GetCrcSize(blockDesc->CrcType);
                }

                /* Call MemIf to read from NV memory */
                memIfResult = MemIf_Read(blockDesc->DeviceId, blockNumber, 0U,
                                         (uint8*)JobPtr->DataPtr, readLength);

                if (memIfResult == E_OK)
                {
                    JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
                }
                else
                {
                    /* Read failed immediately, try to restore from ROM */
                    NvM_CopyRomDataToRam(JobPtr->BlockId, JobPtr->DataPtr);
                    JobPtr->Result = NVM_REQ_RESTORED_FROM_ROM;
                    JobPtr->JobState = NVM_JOB_STATE_IDLE;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_RESTORED_FROM_ROM;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
                }
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
 * @brief   Process redundant block write (first or second copy)
 */
STATIC void NvM_WriteRedundantBlock(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;
    uint8 crcSize;
    uint32 calcCrc;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            blockNumber = blockDesc->BlockBaseNumber + JobPtr->CopyIndex;

            /* Append CRC if configured */
            if ((blockDesc->BlockUseCrc == TRUE) && (blockDesc->CrcType != NVM_CRC_NONE))
            {
                crcSize = NvM_GetCrcSize(blockDesc->CrcType);
                calcCrc = NvM_CalculateCrc(JobPtr->DataPtr, blockDesc->NvBlockLength, blockDesc->CrcType);

                if (crcSize == 1U)
                {
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)calcCrc;
                }
                else if (crcSize == 2U)
                {
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)(calcCrc >> 8U);
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 1U] = (uint8)calcCrc;
                }
                else if (crcSize == 4U)
                {
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)(calcCrc >> 24U);
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 1U] = (uint8)(calcCrc >> 16U);
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 2U] = (uint8)(calcCrc >> 8U);
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 3U] = (uint8)calcCrc;
                }
                else
                {
                    /* No action needed */
                }
            }

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
                /* Write failed immediately */
                if (JobPtr->CopyIndex == 0U)
                {
                    JobPtr->CopyIndex = 1U;
                    NvM_WriteRedundantBlock(JobPtr);
                }
                else
                {
                    JobPtr->Result = NVM_REQ_NOT_OK;
                    JobPtr->JobState = NVM_JOB_STATE_IDLE;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_NOT_OK;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
                }
            }
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
    uint8 crcSize;
    uint32 calcCrc;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            if (blockDesc->ManagementType == NVM_BLOCK_REDUNDANT)
            {
                JobPtr->CopyIndex = 0U;
                NvM_WriteRedundantBlock(JobPtr);
            }
            else
            {
                if (blockDesc->ManagementType == NVM_BLOCK_DATASET)
                {
                    blockNumber = blockDesc->BlockBaseNumber + NvM_InternalState.BlockStates[JobPtr->BlockId].DataIndex;
                }
                else
                {
                    blockNumber = blockDesc->BlockBaseNumber;
                }

                /* Append CRC if configured */
                if ((blockDesc->BlockUseCrc == TRUE) && (blockDesc->CrcType != NVM_CRC_NONE))
                {
                    crcSize = NvM_GetCrcSize(blockDesc->CrcType);
                    calcCrc = NvM_CalculateCrc(JobPtr->DataPtr, blockDesc->NvBlockLength, blockDesc->CrcType);

                    if (crcSize == 1U)
                    {
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)calcCrc;
                    }
                    else if (crcSize == 2U)
                    {
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)(calcCrc >> 8U);
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 1U] = (uint8)calcCrc;
                    }
                    else if (crcSize == 4U)
                    {
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)(calcCrc >> 24U);
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 1U] = (uint8)(calcCrc >> 16U);
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 2U] = (uint8)(calcCrc >> 8U);
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 3U] = (uint8)calcCrc;
                    }
                    else
                    {
                        /* No action needed */
                    }
                }

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

/**
 * @brief   Process erase job
 */
STATIC void NvM_ProcessEraseJob(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            if (blockDesc->ManagementType == NVM_BLOCK_DATASET)
            {
                blockNumber = blockDesc->BlockBaseNumber + NvM_InternalState.BlockStates[JobPtr->BlockId].DataIndex;
            }
            else
            {
                blockNumber = blockDesc->BlockBaseNumber;
            }

            memIfResult = MemIf_EraseImmediateBlock(blockDesc->DeviceId, blockNumber);

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
 * @brief   Process invalidate job
 */
STATIC void NvM_ProcessInvalidateJob(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            if (blockDesc->ManagementType == NVM_BLOCK_DATASET)
            {
                blockNumber = blockDesc->BlockBaseNumber + NvM_InternalState.BlockStates[JobPtr->BlockId].DataIndex;
            }
            else
            {
                blockNumber = blockDesc->BlockBaseNumber;
            }

            memIfResult = MemIf_InvalidateBlock(blockDesc->DeviceId, blockNumber);

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
 * @brief   Cancel all jobs for a given BlockId in a queue
 */
STATIC void NvM_QueueCancelJobs(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, NvM_BlockIdType BlockId)
{
    uint8 tempCount;
    uint8 i;
    NvM_JobQueueEntryType tempEntries[NVM_SIZE_STANDARD_JOB_QUEUE];
    boolean canceled = FALSE;

    tempCount = *Count;

    for (i = 0U; i < tempCount; i++)
    {
        (void)NvM_QueuePop(Queue, Head, Tail, Count, MaxSize, &tempEntries[i]);
    }

    for (i = 0U; i < tempCount; i++)
    {
        if (tempEntries[i].BlockId == BlockId)
        {
            canceled = TRUE;
        }
        else
        {
            (void)NvM_QueuePush(Queue, Head, Tail, Count, MaxSize, &tempEntries[i]);
        }
    }

    if (canceled == TRUE)
    {
        NvM_InternalState.BlockStates[BlockId].JobPending = 0U;
    }
}

/**
 * @brief   Update batch operation status after a job completes
 */
STATIC void NvM_UpdateBatchOperationStatus(uint8 JobType)
{
    if ((NvM_InternalState.ReadAllInProgress == TRUE) && (JobType == NVM_JOB_TYPE_READ))
    {
        if (NvM_InternalState.ReadAllPendingCount > 0U)
        {
            NvM_InternalState.ReadAllPendingCount--;
        }
        if (NvM_InternalState.ReadAllPendingCount == 0U)
        {
            NvM_InternalState.ReadAllInProgress = FALSE;
        }
    }

    if ((NvM_InternalState.WriteAllInProgress == TRUE) && (JobType == NVM_JOB_TYPE_WRITE))
    {
        if (NvM_InternalState.WriteAllPendingCount > 0U)
        {
            NvM_InternalState.WriteAllPendingCount--;
        }
        if (NvM_InternalState.WriteAllPendingCount == 0U)
        {
            NvM_InternalState.WriteAllInProgress = FALSE;
        }
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
    const NvM_BlockDescriptorType* blockDesc;

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

    blockDesc = NvM_GetBlockDescriptor(BlockId);

    /* Check write protection */
    if (blockDesc != NULL_PTR)
    {
        if ((blockDesc->BlockWriteProt == TRUE) ||
            (blockDesc->BlockWriteOnce == TRUE) ||
            (NvM_InternalState.BlockStates[BlockId].BlockLocked == TRUE))
        {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
            NVM_DET_REPORT_ERROR(0x05U, NVM_E_WRITE_PROTECTED);
#endif
            NvM_InternalState.BlockStates[BlockId].LastResult = NVM_REQ_NOT_OK;
            return E_NOT_OK;
        }
    }

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
        jobEntry.CopyIndex = 0U;

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
 * @brief   Set data index for a dataset block
 * @param   BlockId   - Block identifier
 * @param   DataIndex - Data index to set
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_SetDataIndex(NvM_BlockIdType BlockId, uint8 DataIndex)
{
    Std_ReturnType result = E_NOT_OK;
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x01U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x01U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);

    if (blockDesc != NULL_PTR)
    {
        if (blockDesc->ManagementType == NVM_BLOCK_DATASET)
        {
            if (DataIndex < blockDesc->NumberOfDataSets)
            {
                NvM_InternalState.BlockStates[BlockId].DataIndex = DataIndex;
                result = E_OK;
            }
            else
            {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
                NVM_DET_REPORT_ERROR(0x01U, NVM_E_PARAM_DATA_IDX);
#endif
            }
        }
        else
        {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
            NVM_DET_REPORT_ERROR(0x01U, NVM_E_PARAM_BLOCK_TYPE);
#endif
        }
    }

    return result;
}

/**
 * @brief   Write block once (write-once protection)
 * @param   BlockId     - Block identifier
 * @param   NvM_SrcPtr  - Source pointer for write data
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_WriteBlockOnce(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)
{
    Std_ReturnType result = E_NOT_OK;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x0FU, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_SrcPtr == NULL_PTR)
    {
        NVM_DET_REPORT_ERROR(0x0FU, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x0FU, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Check if already written once */
    if (NvM_InternalState.BlockStates[BlockId].WriteOnceDone == TRUE)
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(0x0FU, NVM_E_WRITE_PROTECTED);
#endif
        NvM_InternalState.BlockStates[BlockId].LastResult = NVM_REQ_NOT_OK;
        return E_NOT_OK;
    }

    /* Delegate to regular write block */
    result = NvM_WriteBlock(BlockId, NvM_SrcPtr);

    return result;
}

/**
 * @brief   Set block lock status
 * @param   BlockId     - Block identifier
 * @param   BlockLocked - TRUE to lock, FALSE to unlock
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_SetBlockLockStatus(NvM_BlockIdType BlockId, boolean BlockLocked)
{
    Std_ReturnType result = E_NOT_OK;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x13U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x13U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    if ((BlockId > 0U) && (BlockId < NVM_NUM_OF_NVRAM_BLOCKS))
    {
        NvM_InternalState.BlockStates[BlockId].BlockLocked = BlockLocked;
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Set block protection
 * @param   BlockId            - Block identifier
 * @param   ProtectionEnabled  - TRUE to enable protection, FALSE to disable
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_SetBlockProtection(NvM_BlockIdType BlockId, boolean ProtectionEnabled)
{
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x14U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x14U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);
    if (blockDesc != NULL_PTR)
    {
        /* This is a runtime protection toggle; in a full implementation
           it would modify configuration. Here we treat it as block lock. */
        NvM_InternalState.BlockStates[BlockId].BlockLocked = ProtectionEnabled;
        return E_OK;
    }

    return E_NOT_OK;
}

/**
 * @brief   Set write once status
 * @param   BlockId   - Block identifier
 * @param   WriteOnce - TRUE to enable write-once, FALSE to disable
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_SetWriteOnceStatus(NvM_BlockIdType BlockId, boolean WriteOnce)
{
    (void)WriteOnce;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x15U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x15U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Write-once status is typically configuration-time only.
       Runtime modification is not supported in this implementation. */
    return E_NOT_OK;
}

/**
 * @brief   Read permanent RAM block
 * @param   BlockId - Block identifier
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_ReadPRAMBlock(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x16U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x16U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);
    if ((blockDesc != NULL_PTR) && (blockDesc->RamBlockData != NULL_PTR))
    {
        return NvM_ReadBlock(BlockId, blockDesc->RamBlockData);
    }

    return E_NOT_OK;
}

/**
 * @brief   Write permanent RAM block
 * @param   BlockId - Block identifier
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_WritePRAMBlock(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x17U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x17U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);
    if ((blockDesc != NULL_PTR) && (blockDesc->RamBlockData != NULL_PTR))
    {
        return NvM_WriteBlock(BlockId, blockDesc->RamBlockData);
    }

    return E_NOT_OK;
}

/**
 * @brief   Cancel jobs for a block
 * @param   BlockId - Block identifier
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_CancelJobs(NvM_BlockIdType BlockId)
{
#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x10U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x10U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Cancel is not fully implemented in this version */
    return E_NOT_OK;
}

/**
 * @brief   Get version information
 * @param   versioninfo - Pointer to version info structure
 * @return  None
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
Std_ReturnType NvM_EraseNvBlock(NvM_BlockIdType BlockId)
{
    Std_ReturnType result = E_NOT_OK;
    NvM_JobQueueEntryType jobEntry;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x09U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x09U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Check if block already has pending job */
    if (NvM_InternalState.BlockStates[BlockId].JobPending == 0U)
    {
        /* Prepare job entry */
        jobEntry.BlockId = BlockId;
        jobEntry.JobType = NVM_JOB_TYPE_ERASE;
        jobEntry.JobState = NVM_JOB_STATE_PENDING;
        jobEntry.DataPtr = NULL_PTR;
        jobEntry.Result = NVM_REQ_PENDING;
        jobEntry.RetryCount = 0U;
        jobEntry.CopyIndex = 0U;

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
        NVM_DET_REPORT_ERROR(0x09U, NVM_E_BLOCK_PENDING);
#endif
    }

    return result;
}

/**
 * @brief   Invalidate NV block
 * @param   BlockId - Block identifier
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_InvalidateNvBlock(NvM_BlockIdType BlockId)
{
    Std_ReturnType result = E_NOT_OK;
    NvM_JobQueueEntryType jobEntry;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x0AU, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x0AU, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Check if block already has pending job */
    if (NvM_InternalState.BlockStates[BlockId].JobPending == 0U)
    {
        /* Prepare job entry */
        jobEntry.BlockId = BlockId;
        jobEntry.JobType = NVM_JOB_TYPE_INVALIDATE;
        jobEntry.JobState = NVM_JOB_STATE_PENDING;
        jobEntry.DataPtr = NULL_PTR;
        jobEntry.Result = NVM_REQ_PENDING;
        jobEntry.RetryCount = 0U;
        jobEntry.CopyIndex = 0U;

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
        NVM_DET_REPORT_ERROR(0x0AU, NVM_E_BLOCK_PENDING);
#endif
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
    boolean jobComplete;
    uint8 i;
    uint8 tempCount;
    NvM_JobQueueEntryType tempEntries[NVM_SIZE_STANDARD_JOB_QUEUE];

    /* Handle KillReadAll request */
    if (NvM_InternalState.KillReadAllRequested == TRUE)
    {
        NvM_InternalState.KillReadAllRequested = FALSE;
        if (NvM_InternalState.ReadAllInProgress == TRUE)
        {
            /* Remove all READ jobs from standard queue */
            tempCount = NvM_InternalState.StandardQueueCount;
            for (i = 0U; i < tempCount; i++)
            {
                (void)NvM_QueuePop(NvM_InternalState.StandardQueue,
                                   &NvM_InternalState.StandardQueueHead,
                                   &NvM_InternalState.StandardQueueTail,
                                   &NvM_InternalState.StandardQueueCount,
                                   NVM_SIZE_STANDARD_JOB_QUEUE,
                                   &tempEntries[i]);
            }
            for (i = 0U; i < tempCount; i++)
            {
                if (tempEntries[i].JobType != NVM_JOB_TYPE_READ)
                {
                    (void)NvM_QueuePush(NvM_InternalState.StandardQueue,
                                        &NvM_InternalState.StandardQueueHead,
                                        &NvM_InternalState.StandardQueueTail,
                                        &NvM_InternalState.StandardQueueCount,
                                        NVM_SIZE_STANDARD_JOB_QUEUE,
                                        &tempEntries[i]);
                }
                else
                {
                    NvM_InternalState.BlockStates[tempEntries[i].BlockId].JobPending = 0U;
                    NvM_InternalState.BlockStates[tempEntries[i].BlockId].LastResult = NVM_REQ_CANCELED;
                }
            }
            NvM_InternalState.ReadAllInProgress = FALSE;
            NvM_InternalState.ReadAllPendingCount = 0U;
        }
    }

    /* Handle KillWriteAll request */
    if (NvM_InternalState.KillWriteAllRequested == TRUE)
    {
        NvM_InternalState.KillWriteAllRequested = FALSE;
        if (NvM_InternalState.WriteAllInProgress == TRUE)
        {
            /* Remove all WRITE jobs from standard queue */
            tempCount = NvM_InternalState.StandardQueueCount;
            for (i = 0U; i < tempCount; i++)
            {
                (void)NvM_QueuePop(NvM_InternalState.StandardQueue,
                                   &NvM_InternalState.StandardQueueHead,
                                   &NvM_InternalState.StandardQueueTail,
                                   &NvM_InternalState.StandardQueueCount,
                                   NVM_SIZE_STANDARD_JOB_QUEUE,
                                   &tempEntries[i]);
            }
            for (i = 0U; i < tempCount; i++)
            {
                if (tempEntries[i].JobType != NVM_JOB_TYPE_WRITE)
                {
                    (void)NvM_QueuePush(NvM_InternalState.StandardQueue,
                                        &NvM_InternalState.StandardQueueHead,
                                        &NvM_InternalState.StandardQueueTail,
                                        &NvM_InternalState.StandardQueueCount,
                                        NVM_SIZE_STANDARD_JOB_QUEUE,
                                        &tempEntries[i]);
                }
                else
                {
                    NvM_InternalState.BlockStates[tempEntries[i].BlockId].JobPending = 0U;
                    NvM_InternalState.BlockStates[tempEntries[i].BlockId].LastResult = NVM_REQ_CANCELED;
                }
            }
            NvM_InternalState.WriteAllInProgress = FALSE;
            NvM_InternalState.WriteAllPendingCount = 0U;
        }
    }

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
                NvM_InternalState.ActiveJob = jobEntry;
                NvM_InternalState.CurrentJob = &NvM_InternalState.ActiveJob;

                switch (NvM_InternalState.ActiveJob.JobType)
                {
                    case NVM_JOB_TYPE_RESTORE:
                        NvM_ProcessRestoreJob(&NvM_InternalState.ActiveJob);
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
                NvM_InternalState.ActiveJob = jobEntry;
                NvM_InternalState.CurrentJob = &NvM_InternalState.ActiveJob;
                NvM_InternalState.State = NVM_STATE_BUSY;

                /* Process the job based on type */
                switch (NvM_InternalState.ActiveJob.JobType)
                {
                    case NVM_JOB_TYPE_READ:
                        NvM_ProcessReadJob(&NvM_InternalState.ActiveJob);
                        break;

                    case NVM_JOB_TYPE_WRITE:
                        NvM_ProcessWriteJob(&NvM_InternalState.ActiveJob);
                        break;

                    case NVM_JOB_TYPE_ERASE:
                        NvM_ProcessEraseJob(&NvM_InternalState.ActiveJob);
                        break;

                    case NVM_JOB_TYPE_INVALIDATE:
                        NvM_ProcessInvalidateJob(&NvM_InternalState.ActiveJob);
                        break;

                    default:
                        NvM_InternalState.CurrentJob = NULL_PTR;
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
                    MemIf_JobResultType jobResult = MemIf_GetJobResult(blockDesc->DeviceId);
                    jobComplete = TRUE;

                    if (jobResult == MEMIF_JOB_OK)
                    {
                        /* Job completed successfully */
                        if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_READ)
                        {
#if (NVM_CALC_RAM_BLOCK_CRC == STD_ON)
                            /* Validate CRC if configured */
                            if (blockDesc->BlockUseCrc == TRUE)
                            {
                                uint8 crcSize = NvM_GetCrcSize(blockDesc->CrcType);
                                uint32 storedCrc = 0U;
                                uint32 calcCrc;
                                boolean crcMatch = FALSE;
                                uint8 idx;

                                calcCrc = NvM_CalculateCrc(NvM_InternalState.CurrentJob->DataPtr,
                                                           blockDesc->NvBlockLength,
                                                           blockDesc->CrcType);

                                /* Extract stored CRC from tail of data buffer */
                                for (idx = 0U; idx < crcSize; idx++)
                                {
                                    storedCrc = (storedCrc << 8U) |
                                                ((const uint8*)NvM_InternalState.CurrentJob->DataPtr)[blockDesc->NvBlockLength + idx];
                                }

                                if (crcSize == 1U)
                                {
                                    crcMatch = ((uint8)calcCrc == (uint8)storedCrc);
                                }
                                else if (crcSize == 2U)
                                {
                                    crcMatch = ((uint16)calcCrc == (uint16)storedCrc);
                                }
                                else if (crcSize == 4U)
                                {
                                    crcMatch = (calcCrc == storedCrc);
                                }
                                else
                                {
                                    crcMatch = TRUE;
                                }

                                if (crcMatch == FALSE)
                                {
                                    /* CRC mismatch - try redundant copy or ROM fallback */
                                    if ((blockDesc->ManagementType == NVM_BLOCK_REDUNDANT) &&
                                        (NvM_InternalState.CurrentJob->CopyIndex == 0U))
                                    {
                                        NvM_InternalState.CurrentJob->CopyIndex = 1U;
                                        NvM_ReadRedundantBlock(NvM_InternalState.CurrentJob);
                                        jobComplete = FALSE;
                                    }
                                    else
                                    {
                                        NvM_CopyRomDataToRam(NvM_InternalState.CurrentJob->BlockId,
                                                             NvM_InternalState.CurrentJob->DataPtr);
                                        NvM_InternalState.CurrentJob->Result = NVM_REQ_INTEGRITY_FAILED;
                                        NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_INTEGRITY_FAILED;
                                        jobComplete = TRUE;
                                    }
                                }
                                else
                                {
                                    /* CRC OK */
                                    NvM_InternalState.CurrentJob->Result = NVM_REQ_OK;
                                    NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_OK;
                                    jobComplete = TRUE;
                                }
                            }
                            else
#endif
                            {
                                NvM_InternalState.CurrentJob->Result = NVM_REQ_OK;
                                NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_OK;
                                jobComplete = TRUE;
                            }
                        }
                        else if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_WRITE)
                        {
                            if ((blockDesc->ManagementType == NVM_BLOCK_REDUNDANT) &&
                                (NvM_InternalState.CurrentJob->CopyIndex == 0U))
                            {
                                /* Write second redundant copy */
                                NvM_InternalState.CurrentJob->CopyIndex = 1U;
                                NvM_WriteRedundantBlock(NvM_InternalState.CurrentJob);
                                jobComplete = FALSE;
                            }
                            else
                            {
                                /* Increment write counter on successful write */
                                if (NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].WriteCounter < 0xFFU)
                                {
                                    NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].WriteCounter++;
                                }
                                NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].DataChanged = FALSE;

                                if (blockDesc->BlockWriteOnce == TRUE)
                                {
                                    NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].WriteOnceDone = TRUE;
                                }

                                NvM_InternalState.CurrentJob->Result = NVM_REQ_OK;
                                NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_OK;
                                jobComplete = TRUE;
                            }
                        }
                        else
                        {
                            NvM_InternalState.CurrentJob->Result = NVM_REQ_OK;
                            NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_OK;
                            jobComplete = TRUE;
                        }
                    }
                    else
                    {
                        /* Job failed - check retry count */
                        uint16 maxRetries;

                        if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_READ)
                        {
                            maxRetries = NvM_InternalState.ConfigPtr->MaxNumberOfReadRetries;
                        }
                        else
                        {
                            maxRetries = NvM_InternalState.ConfigPtr->MaxNumberOfWriteRetries;
                        }

                        if (NvM_InternalState.CurrentJob->RetryCount < maxRetries)
                        {
                            NvM_InternalState.CurrentJob->RetryCount++;
                            jobComplete = FALSE;

                            /* Re-submit the job */
                            switch (NvM_InternalState.CurrentJob->JobType)
                            {
                                case NVM_JOB_TYPE_READ:
                                    NvM_ProcessReadJob(NvM_InternalState.CurrentJob);
                                    break;

                                case NVM_JOB_TYPE_WRITE:
                                    NvM_ProcessWriteJob(NvM_InternalState.CurrentJob);
                                    break;

                                case NVM_JOB_TYPE_ERASE:
                                    NvM_ProcessEraseJob(NvM_InternalState.CurrentJob);
                                    break;

                                case NVM_JOB_TYPE_INVALIDATE:
                                    NvM_ProcessInvalidateJob(NvM_InternalState.CurrentJob);
                                    break;

                                default:
                                    jobComplete = TRUE;
                                    break;
                            }
                        }
                        else
                        {
                            /* Max retries exceeded */
                            if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_READ)
                            {
                                NvM_CopyRomDataToRam(NvM_InternalState.CurrentJob->BlockId,
                                                     NvM_InternalState.CurrentJob->DataPtr);
                                NvM_InternalState.CurrentJob->Result = NVM_REQ_RESTORED_FROM_ROM;
                                NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_RESTORED_FROM_ROM;
                            }
                            else
                            {
                                NvM_InternalState.CurrentJob->Result = NVM_REQ_NOT_OK;
                                NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_NOT_OK;
                            }
                            jobComplete = TRUE;
                        }
                    }

                    if (jobComplete == TRUE)
                    {
                        NvM_UpdateBatchOperationStatus(NvM_InternalState.CurrentJob->JobType);

                        NvM_InternalState.CurrentJob->JobState = NVM_JOB_STATE_IDLE;
                        NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].JobPending = 0U;
                        NvM_InvokeJobEndCallback(NvM_InternalState.CurrentJob->BlockId,
                                                 NvM_InternalState.CurrentJob->Result);
                        NvM_InternalState.CurrentJob = NULL_PTR;
                        NvM_InternalState.State = NVM_STATE_IDLE;
                    }
                }
                else if ((memIfStatus != MEMIF_BUSY) && (memIfStatus != MEMIF_BUSY_INTERNAL))
                {
                    /* Unexpected error state - treat as failure with retry */
                    uint16 maxRetries;

                    if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_READ)
                    {
                        maxRetries = NvM_InternalState.ConfigPtr->MaxNumberOfReadRetries;
                    }
                    else
                    {
                        maxRetries = NvM_InternalState.ConfigPtr->MaxNumberOfWriteRetries;
                    }

                    if (NvM_InternalState.CurrentJob->RetryCount < maxRetries)
                    {
                        NvM_InternalState.CurrentJob->RetryCount++;
                        /* Re-submit */
                        switch (NvM_InternalState.CurrentJob->JobType)
                        {
                            case NVM_JOB_TYPE_READ:
                                NvM_ProcessReadJob(NvM_InternalState.CurrentJob);
                                break;
                            case NVM_JOB_TYPE_WRITE:
                                NvM_ProcessWriteJob(NvM_InternalState.CurrentJob);
                                break;
                            case NVM_JOB_TYPE_ERASE:
                                NvM_ProcessEraseJob(NvM_InternalState.CurrentJob);
                                break;
                            case NVM_JOB_TYPE_INVALIDATE:
                                NvM_ProcessInvalidateJob(NvM_InternalState.CurrentJob);
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        NvM_UpdateBatchOperationStatus(NvM_InternalState.CurrentJob->JobType);

                        if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_READ)
                        {
                            NvM_CopyRomDataToRam(NvM_InternalState.CurrentJob->BlockId,
                                                 NvM_InternalState.CurrentJob->DataPtr);
                            NvM_InternalState.CurrentJob->Result = NVM_REQ_RESTORED_FROM_ROM;
                            NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_RESTORED_FROM_ROM;
                        }
                        else
                        {
                            NvM_InternalState.CurrentJob->Result = NVM_REQ_NOT_OK;
                            NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_NOT_OK;
                        }
                        NvM_InternalState.CurrentJob->JobState = NVM_JOB_STATE_IDLE;
                        NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].JobPending = 0U;
                        NvM_InvokeJobEndCallback(NvM_InternalState.CurrentJob->BlockId,
                                                 NvM_InternalState.CurrentJob->Result);
                        NvM_InternalState.CurrentJob = NULL_PTR;
                        NvM_InternalState.State = NVM_STATE_IDLE;
                    }
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

/**
 * @brief   Read all permanent RAM blocks from NV memory (startup recovery)
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_ReadAll(void)
{
    Std_ReturnType result = E_OK;
    const NvM_BlockDescriptorType* blockDesc;
    uint16 i;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_READALL, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif

    if (NvM_InternalState.ReadAllInProgress == TRUE)
    {
        return E_NOT_OK;
    }

    for (i = 0U; i < NvM_InternalState.ConfigPtr->NumBlockDescriptors; i++)
    {
        blockDesc = &NvM_InternalState.ConfigPtr->BlockDescriptors[i];

        if (blockDesc->RamBlockData != NULL_PTR)
        {
            if (NvM_ReadBlock(blockDesc->BlockId, blockDesc->RamBlockData) == E_OK)
            {
                NvM_InternalState.ReadAllPendingCount++;
            }
            else
            {
                result = E_NOT_OK;
            }
        }
    }

    if (NvM_InternalState.ReadAllPendingCount > 0U)
    {
        NvM_InternalState.ReadAllInProgress = TRUE;
    }

    return result;
}

/**
 * @brief   Write all dirty permanent RAM blocks to NV memory (shutdown flush)
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_WriteAll(void)
{
    Std_ReturnType result = E_OK;
    const NvM_BlockDescriptorType* blockDesc;
    uint16 i;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_WRITEALL, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif

    if (NvM_InternalState.WriteAllInProgress == TRUE)
    {
        return E_NOT_OK;
    }

    for (i = 0U; i < NvM_InternalState.ConfigPtr->NumBlockDescriptors; i++)
    {
        blockDesc = &NvM_InternalState.ConfigPtr->BlockDescriptors[i];

        if ((blockDesc->RamBlockData != NULL_PTR) &&
            (NvM_InternalState.BlockStates[blockDesc->BlockId].DataChanged == TRUE))
        {
            if (NvM_WriteBlock(blockDesc->BlockId, blockDesc->RamBlockData) == E_OK)
            {
                NvM_InternalState.WriteAllPendingCount++;
            }
            else
            {
                result = E_NOT_OK;
            }
        }
    }

    if (NvM_InternalState.WriteAllPendingCount > 0U)
    {
        NvM_InternalState.WriteAllInProgress = TRUE;
    }

    return result;
}

/**
 * @brief   Read a permanent RAM block (uses configured RamBlockData)
 * @param   BlockId - Block identifier
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_ReadPRAMBlock(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_READPRAMBLOCK, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_READPRAMBLOCK, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);

    if ((blockDesc == NULL_PTR) || (blockDesc->RamBlockData == NULL_PTR))
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(NVM_SID_READPRAMBLOCK, NVM_E_BLOCK_CONFIG);
#endif
        return E_NOT_OK;
    }

    return NvM_ReadBlock(BlockId, blockDesc->RamBlockData);
}

/**
 * @brief   Write a permanent RAM block (uses configured RamBlockData)
 * @param   BlockId - Block identifier
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_WritePRAMBlock(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_WRITEPRAMBLOCK, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_WRITEPRAMBLOCK, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);

    if ((blockDesc == NULL_PTR) || (blockDesc->RamBlockData == NULL_PTR))
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(NVM_SID_WRITEPRAMBLOCK, NVM_E_BLOCK_CONFIG);
#endif
        return E_NOT_OK;
    }

    return NvM_WriteBlock(BlockId, blockDesc->RamBlockData);
}

/**
 * @brief   Cancel all pending jobs for a block
 * @param   BlockId - Block identifier
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_CancelJobs(NvM_BlockIdType BlockId)
{
#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_CANCELJOBS, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_CANCELJOBS, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Cancel pending jobs in standard queue */
    NvM_QueueCancelJobs(NvM_InternalState.StandardQueue,
                        &NvM_InternalState.StandardQueueHead,
                        &NvM_InternalState.StandardQueueTail,
                        &NvM_InternalState.StandardQueueCount,
                        NVM_SIZE_STANDARD_JOB_QUEUE,
                        BlockId);

    /* Cancel pending jobs in immediate queue */
    NvM_QueueCancelJobs(NvM_InternalState.ImmediateQueue,
                        &NvM_InternalState.ImmediateQueueHead,
                        &NvM_InternalState.ImmediateQueueTail,
                        &NvM_InternalState.ImmediateQueueCount,
                        NVM_SIZE_IMMEDIATE_JOB_QUEUE,
                        BlockId);

    /* If current job matches, mark result as canceled */
    if ((NvM_InternalState.CurrentJob != NULL_PTR) &&
        (NvM_InternalState.CurrentJob->BlockId == BlockId))
    {
        NvM_InternalState.CurrentJob->Result = NVM_REQ_CANCELED;
        NvM_InternalState.BlockStates[BlockId].LastResult = NVM_REQ_CANCELED;
    }

    NvM_InternalState.BlockStates[BlockId].LastResult = NVM_REQ_CANCELED;

    return E_OK;
}

/**
 * @brief   Kill WriteAll operation
 */
void NvM_KillWriteAll(void)
{
    NvM_InternalState.KillWriteAllRequested = TRUE;
}

/**
 * @brief   Kill ReadAll operation
 */
void NvM_KillReadAll(void)
{
    NvM_InternalState.KillReadAllRequested = TRUE;
}

#define NVM_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
