/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Runtime Environment)
* Dependencies         : NvM, Rte
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-15
* Author               : AI Agent (RTE Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Rte.h"
#include "Rte_Type.h"
#include "NvM.h"
#include "MemMap.h"
#include "string.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define RTE_NVM_INSTANCE_ID             (0x00U)

/* NVM Block to RTE Port Mapping Size */
#define RTE_NVM_MAP_SIZE                (8U)

/* NVM Job Queue Size */
#define RTE_NVM_JOB_QUEUE_SIZE          (4U)

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* NVM Job Type */
typedef enum
{
    RTE_NVM_JOB_NONE = 0,
    RTE_NVM_JOB_READ,
    RTE_NVM_JOB_WRITE,
    RTE_NVM_JOB_RESTORE
} Rte_NvmJobType;

/* NVM Job Entry */
typedef struct
{
    Rte_NvmJobType JobType;
    NvM_BlockIdType BlockId;
    void* DataPtr;
    boolean IsPending;
} Rte_NvmJobEntryType;

/* NVM Block Mapping Entry */
typedef struct
{
    NvM_BlockIdType BlockId;
    Rte_PortHandleType RtePortHandle;
    uint16 DataLength;
    boolean IsMapped;
    boolean IsValid;
} Rte_NvmBlockMappingType;

/* NVM Interface State */
typedef struct
{
    boolean IsInitialized;
    Rte_NvmBlockMappingType BlockMap[RTE_NVM_MAP_SIZE];
    Rte_NvmJobEntryType JobQueue[RTE_NVM_JOB_QUEUE_SIZE];
    uint8 NumMappedBlocks;
    uint8 JobQueueHead;
    uint8 JobQueueTail;
    uint8 JobQueueCount;
} Rte_NvmInterfaceStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define RTE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC Rte_NvmInterfaceStateType Rte_NvmInterfaceState;

#define RTE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC sint8 Rte_NvmFindBlockMapping(NvM_BlockIdType blockId);
STATIC Std_ReturnType Rte_NvmMapBlock(NvM_BlockIdType blockId, Rte_PortHandleType rtePortHandle, uint16 dataLength);
STATIC Std_ReturnType Rte_NvmQueueJob(Rte_NvmJobType jobType, NvM_BlockIdType blockId, void* dataPtr);
STATIC Std_ReturnType Rte_NvmProcessJobQueue(void);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Find NVM block mapping by block ID
 */
STATIC sint8 Rte_NvmFindBlockMapping(NvM_BlockIdType blockId)
{
    sint8 result = -1;
    uint8 i;

    for (i = 0U; i < Rte_NvmInterfaceState.NumMappedBlocks; i++)
    {
        if (Rte_NvmInterfaceState.BlockMap[i].BlockId == blockId)
        {
            result = (sint8)i;
            break;
        }
    }

    return result;
}

/**
 * @brief   Map NVM block to RTE port handle
 */
STATIC Std_ReturnType Rte_NvmMapBlock(NvM_BlockIdType blockId, Rte_PortHandleType rtePortHandle, uint16 dataLength)
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_NvmInterfaceState.NumMappedBlocks < RTE_NVM_MAP_SIZE)
    {
        uint8 index = Rte_NvmInterfaceState.NumMappedBlocks;
        Rte_NvmInterfaceState.BlockMap[index].BlockId = blockId;
        Rte_NvmInterfaceState.BlockMap[index].RtePortHandle = rtePortHandle;
        Rte_NvmInterfaceState.BlockMap[index].DataLength = dataLength;
        Rte_NvmInterfaceState.BlockMap[index].IsMapped = TRUE;
        Rte_NvmInterfaceState.BlockMap[index].IsValid = FALSE;
        Rte_NvmInterfaceState.NumMappedBlocks++;
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Queue NVM job
 */
STATIC Std_ReturnType Rte_NvmQueueJob(Rte_NvmJobType jobType, NvM_BlockIdType blockId, void* dataPtr)
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_NvmInterfaceState.JobQueueCount < RTE_NVM_JOB_QUEUE_SIZE)
    {
        uint8 index = Rte_NvmInterfaceState.JobQueueTail;
        Rte_NvmInterfaceState.JobQueue[index].JobType = jobType;
        Rte_NvmInterfaceState.JobQueue[index].BlockId = blockId;
        Rte_NvmInterfaceState.JobQueue[index].DataPtr = dataPtr;
        Rte_NvmInterfaceState.JobQueue[index].IsPending = TRUE;

        Rte_NvmInterfaceState.JobQueueTail = (Rte_NvmInterfaceState.JobQueueTail + 1U) % RTE_NVM_JOB_QUEUE_SIZE;
        Rte_NvmInterfaceState.JobQueueCount++;

        result = E_OK;
    }

    return result;
}

/**
 * @brief   Process NVM job queue
 */
STATIC Std_ReturnType Rte_NvmProcessJobQueue(void)
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_NvmInterfaceState.JobQueueCount > 0U)
    {
        uint8 index = Rte_NvmInterfaceState.JobQueueHead;
        Rte_NvmJobEntryType* jobPtr = &Rte_NvmInterfaceState.JobQueue[index];

        if (jobPtr->IsPending)
        {
            switch (jobPtr->JobType)
            {
                case RTE_NVM_JOB_READ:
                    result = NvM_ReadBlock(jobPtr->BlockId, jobPtr->DataPtr);
                    break;

                case RTE_NVM_JOB_WRITE:
                    result = NvM_WriteBlock(jobPtr->BlockId, jobPtr->DataPtr);
                    break;

                case RTE_NVM_JOB_RESTORE:
                    result = NvM_RestoreBlockDefaults(jobPtr->BlockId, jobPtr->DataPtr);
                    break;

                default:
                    break;
            }

            jobPtr->IsPending = FALSE;
        }

        Rte_NvmInterfaceState.JobQueueHead = (Rte_NvmInterfaceState.JobQueueHead + 1U) % RTE_NVM_JOB_QUEUE_SIZE;
        Rte_NvmInterfaceState.JobQueueCount--;
    }

    return result;
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes RTE NVM Interface
 */
void Rte_NvmInterface_Init(void)
{
    uint8 i;

    Rte_NvmInterfaceState.IsInitialized = FALSE;
    Rte_NvmInterfaceState.NumMappedBlocks = 0U;
    Rte_NvmInterfaceState.JobQueueHead = 0U;
    Rte_NvmInterfaceState.JobQueueTail = 0U;
    Rte_NvmInterfaceState.JobQueueCount = 0U;

    for (i = 0U; i < RTE_NVM_MAP_SIZE; i++)
    {
        Rte_NvmInterfaceState.BlockMap[i].BlockId = 0U;
        Rte_NvmInterfaceState.BlockMap[i].RtePortHandle = 0xFFFFU;
        Rte_NvmInterfaceState.BlockMap[i].DataLength = 0U;
        Rte_NvmInterfaceState.BlockMap[i].IsMapped = FALSE;
        Rte_NvmInterfaceState.BlockMap[i].IsValid = FALSE;
    }

    for (i = 0U; i < RTE_NVM_JOB_QUEUE_SIZE; i++)
    {
        Rte_NvmInterfaceState.JobQueue[i].JobType = RTE_NVM_JOB_NONE;
        Rte_NvmInterfaceState.JobQueue[i].BlockId = 0U;
        Rte_NvmInterfaceState.JobQueue[i].DataPtr = NULL_PTR;
        Rte_NvmInterfaceState.JobQueue[i].IsPending = FALSE;
    }

    /* Initialize default block mappings */
    (void)Rte_NvmMapBlock(RTE_NVMBLOCK_ODOMETER, RTE_PORT_NVM_READ, 4U);
    (void)Rte_NvmMapBlock(RTE_NVMBLOCK_VIN, RTE_PORT_NVM_READ, 17U);
    (void)Rte_NvmMapBlock(RTE_NVMBLOCK_ERROR_LOG, RTE_PORT_NVM_READ, 256U);
    (void)Rte_NvmMapBlock(RTE_NVMBLOCK_CALIBRATION_DATA, RTE_PORT_NVM_READ, 256U);
    (void)Rte_NvmMapBlock(RTE_NVMBLOCK_USER_SETTINGS, RTE_PORT_NVM_READ, 64U);

    Rte_NvmInterfaceState.IsInitialized = TRUE;
}

/**
 * @brief   Reads block from NVM
 */
Std_ReturnType Rte_NvmReadBlock(NvM_BlockIdType blockId, void* dataPtr)
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_NvmInterfaceState.IsInitialized && (dataPtr != NULL_PTR))
    {
        sint8 mapIndex = Rte_NvmFindBlockMapping(blockId);

        if (mapIndex >= 0)
        {
            /* Queue read job */
            result = Rte_NvmQueueJob(RTE_NVM_JOB_READ, blockId, dataPtr);
        }
    }

    return result;
}

/**
 * @brief   Writes block to NVM
 */
Std_ReturnType Rte_NvmWriteBlock(NvM_BlockIdType blockId, const void* dataPtr)
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_NvmInterfaceState.IsInitialized && (dataPtr != NULL_PTR))
    {
        sint8 mapIndex = Rte_NvmFindBlockMapping(blockId);

        if (mapIndex >= 0)
        {
            /* Queue write job */
            result = Rte_NvmQueueJob(RTE_NVM_JOB_WRITE, blockId, (void*)dataPtr);
        }
    }

    return result;
}

/**
 * @brief   Restores block defaults from ROM
 */
Std_ReturnType Rte_NvmRestoreBlockDefaults(NvM_BlockIdType blockId, void* dataPtr)
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_NvmInterfaceState.IsInitialized && (dataPtr != NULL_PTR))
    {
        sint8 mapIndex = Rte_NvmFindBlockMapping(blockId);

        if (mapIndex >= 0)
        {
            /* Queue restore job */
            result = Rte_NvmQueueJob(RTE_NVM_JOB_RESTORE, blockId, dataPtr);
        }
    }

    return result;
}

/**
 * @brief   Gets NVM block status
 */
Std_ReturnType Rte_NvmGetBlockStatus(NvM_BlockIdType blockId, NvM_RequestResultType* requestResult)
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_NvmInterfaceState.IsInitialized && (requestResult != NULL_PTR))
    {
        result = NvM_GetErrorStatus(blockId, requestResult);
    }

    return result;
}

/**
 * @brief   RTE NVM callback for job completion
 */
void Rte_NvmCallback(uint16 blockId, Std_ReturnType jobResult)
{
    if (Rte_NvmInterfaceState.IsInitialized)
    {
        sint8 mapIndex = Rte_NvmFindBlockMapping((NvM_BlockIdType)blockId);

        if (mapIndex >= 0)
        {
            /* Update block validity */
            if (jobResult == E_OK)
            {
                Rte_NvmInterfaceState.BlockMap[mapIndex].IsValid = TRUE;
            }

            /* Notify RTE about job completion */
            /* This would typically trigger a runnable or update a status */
            (void)jobResult;
        }
    }
}

/**
 * @brief   Main function for NVM interface processing
 */
void Rte_NvmInterface_MainFunction(void)
{
    if (Rte_NvmInterfaceState.IsInitialized)
    {
        /* Process job queue */
        (void)Rte_NvmProcessJobQueue();

        /* Call NvM main function */
        NvM_MainFunction();
    }
}

/**
 * @brief   Write all NVM blocks
 */
Std_ReturnType Rte_NvmWriteAll(void)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;

    if (Rte_NvmInterfaceState.IsInitialized)
    {
        /* Queue write jobs for all mapped blocks */
        for (i = 0U; i < Rte_NvmInterfaceState.NumMappedBlocks; i++)
        {
            if (Rte_NvmInterfaceState.BlockMap[i].IsMapped)
            {
                /* Get block data from RTE port and write to NVM */
                /* This is a placeholder - actual implementation would read from port */
            }
        }
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Read all NVM blocks
 */
Std_ReturnType Rte_NvmReadAll(void)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;

    if (Rte_NvmInterfaceState.IsInitialized)
    {
        /* Queue read jobs for all mapped blocks */
        for (i = 0U; i < Rte_NvmInterfaceState.NumMappedBlocks; i++)
        {
            if (Rte_NvmInterfaceState.BlockMap[i].IsMapped)
            {
                /* Read block from NVM and write to RTE port */
                /* This is a placeholder - actual implementation would write to port */
            }
        }
        result = E_OK;
    }

    return result;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
