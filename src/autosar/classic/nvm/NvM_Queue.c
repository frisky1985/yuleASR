/**
 * @file NvM_Queue.c
 * @brief AUTOSAR NvM Job Queue Implementation
 * @version 4.4.0
 * @date 2025
 *
 * AUTOSAR Classic Platform - NvM Job Queue (Module ID: 0x0E)
 *
 * Implements a priority-based job queue using single linked list:
 * - FIFO within same priority
 * - Higher priority jobs processed first
 * - Dynamic queue entry allocation from static pool
 *
 * Features:
 * - Single linked list implementation
 * - Priority-based scheduling
 * - Queue overflow protection
 * - Job completion tracking
 *
 * Copyright (c) 2025
 */

#include "NvM_Private.h"

/*============================================================================*
 * Local Types
 *============================================================================*/

typedef struct {
    uint32_t JobsSubmitted;
    uint32_t JobsCompleted;
    uint32_t JobsFailed;
    uint32_t QueueOverflows;
    uint32_t PriorityDistribution[4]; /* LOW, NORMAL, HIGH, CRITICAL */
} NvM_QueueStatsType;

/*============================================================================*
 * Local Variables
 *============================================================================*/
static NvM_QueueStatsType NvM_QueueStats = {0};

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Get a free queue entry from the pool
 * @return Pointer to free entry, NULL if none available
 */
static NvM_JobQueueEntryType* NvM_Queue_GetFreeEntry(void)
{
    uint16_t i;
    
    for (i = 0u; i < NVM_SIZE_OF_JOB_QUEUE; i++) {
        if (NvM_Global.JobQueue[i].JobType == NVM_JOB_TYPE_NONE) {
            return &NvM_Global.JobQueue[i];
        }
    }
    
    return NULL_PTR;
}

/**
 * @brief Find insertion point based on priority
 * @param Priority Priority of the new job
 * @return Pointer to pointer where new job should be inserted
 */
static NvM_JobQueueEntryType** NvM_Queue_FindInsertionPoint(NvM_PriorityType Priority)
{
    NvM_JobQueueEntryType** current;
    
    current = &NvM_Global.QueueHead;
    
    /* Traverse queue to find insertion point based on priority */
    while (*current != NULL_PTR) {
        if ((*current)->Priority < Priority) {
            /* Insert before lower priority job */
            break;
        }
        current = &((*current)->Next);
    }
    
    return current;
}

/*============================================================================*
 * Public API Implementation
 *============================================================================*/

/**
 * @brief Initialize the job queue
 */
Std_ReturnType NvM_Queue_Init(void)
{
    uint16_t i;
    
    /* Clear queue statistics */
    NvM_QueueStats.JobsSubmitted = 0u;
    NvM_QueueStats.JobsCompleted = 0u;
    NvM_QueueStats.JobsFailed = 0u;
    NvM_QueueStats.QueueOverflows = 0u;
    
    for (i = 0u; i < 4u; i++) {
        NvM_QueueStats.PriorityDistribution[i] = 0u;
    }
    
    /* Clear all queue entries */
    for (i = 0u; i < NVM_SIZE_OF_JOB_QUEUE; i++) {
        NvM_Global.JobQueue[i].JobType = NVM_JOB_TYPE_NONE;
        NvM_Global.JobQueue[i].BlockId = 0u;
        NvM_Global.JobQueue[i].DataPtr = NULL_PTR;
        NvM_Global.JobQueue[i].DataIndex = 0u;
        NvM_Global.JobQueue[i].Priority = NVM_PRIORITY_LOW;
        NvM_Global.JobQueue[i].Next = NULL_PTR;
        NvM_Global.JobQueue[i].InProgress = FALSE;
    }
    
    /* Reset queue pointers */
    NvM_Global.QueueHead = NULL_PTR;
    NvM_Global.QueueTail = NULL_PTR;
    NvM_Global.QueueSize = 0u;
    
    return E_OK;
}

/**
 * @brief Add a job to the queue
 */
Std_ReturnType NvM_Queue_AddJob(
    NvM_JobTypeType JobType,
    NvM_BlockIdType BlockId,
    void* DataPtr,
    NvM_PriorityType Priority)
{
    NvM_JobQueueEntryType* newEntry;
    NvM_JobQueueEntryType** insertPoint;
    
    /* Check if queue is full */
    if (NvM_Global.QueueSize >= NVM_SIZE_OF_JOB_QUEUE) {
        NvM_QueueStats.QueueOverflows++;
        return E_NOT_OK;
    }
    
    /* Check if there's already a pending job for this block */
    if (NvM_Global.Blocks[BlockId].Status.State != NVM_BLOCK_STATE_IDLE) {
        return E_NOT_OK;
    }
    
    /* Get free entry from pool */
    newEntry = NvM_Queue_GetFreeEntry();
    if (newEntry == NULL_PTR) {
        NvM_QueueStats.QueueOverflows++;
        return E_NOT_OK;
    }
    
    /* Fill job details */
    newEntry->JobType = JobType;
    newEntry->BlockId = BlockId;
    newEntry->DataPtr = DataPtr;
    newEntry->DataIndex = NvM_Global.Blocks[BlockId].Status.DataIndex;
    newEntry->Priority = Priority;
    newEntry->Next = NULL_PTR;
    newEntry->InProgress = FALSE;
    
    /* Find insertion point based on priority */
    insertPoint = NvM_Queue_FindInsertionPoint(Priority);
    
    /* Insert into linked list */
    newEntry->Next = *insertPoint;
    *insertPoint = newEntry;
    
    /* Update tail if inserting at end */
    if (newEntry->Next == NULL_PTR) {
        NvM_Global.QueueTail = newEntry;
    }
    
    /* Update queue size */
    NvM_Global.QueueSize++;
    
    /* Update statistics */
    NvM_QueueStats.JobsSubmitted++;
    if (Priority < 4u) {
        NvM_QueueStats.PriorityDistribution[Priority]++;
    }
    
    return E_OK;
}

/**
 * @brief Get the next job from the queue (highest priority first)
 */
Std_ReturnType NvM_Queue_GetNextJob(NvM_JobQueueEntryType** JobPtr)
{
    if (JobPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    
    *JobPtr = NULL_PTR;
    
    /* Check if queue is empty */
    if (NvM_Global.QueueHead == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Check if job is already in progress */
    if (NvM_Global.QueueHead->InProgress == TRUE) {
        return E_NOT_OK;
    }
    
    /* Return head of queue */
    *JobPtr = NvM_Global.QueueHead;
    
    return E_OK;
}

/**
 * @brief Mark a job as complete and remove from queue
 */
void NvM_Queue_JobComplete(NvM_JobQueueEntryType* Job)
{
    NvM_JobQueueEntryType** current;
    NvM_JobQueueEntryType* toRemove;
    
    if (Job == NULL_PTR) {
        return;
    }
    
    /* Find job in queue */
    current = &NvM_Global.QueueHead;
    
    while (*current != NULL_PTR) {
        if (*current == Job) {
            /* Remove from list */
            toRemove = *current;
            *current = toRemove->Next;
            
            /* Update tail if removing last element */
            if (toRemove == NvM_Global.QueueTail) {
                NvM_Global.QueueTail = (current == &NvM_Global.QueueHead) ? 
                    NULL_PTR : 
                    (NvM_JobQueueEntryType*)((uint8_t*)current - offsetof(NvM_JobQueueEntryType, Next));
            }
            
            /* Clear entry */
            toRemove->JobType = NVM_JOB_TYPE_NONE;
            toRemove->BlockId = 0u;
            toRemove->DataPtr = NULL_PTR;
            toRemove->Next = NULL_PTR;
            toRemove->InProgress = FALSE;
            
            /* Update queue size */
            if (NvM_Global.QueueSize > 0u) {
                NvM_Global.QueueSize--;
            }
            
            /* Update statistics */
            NvM_QueueStats.JobsCompleted++;
            
            return;
        }
        current = &((*current)->Next);
    }
}

/**
 * @brief Clear all jobs from the queue
 */
void NvM_Queue_Clear(void)
{
    uint16_t i;
    
    /* Clear all queue entries */
    for (i = 0u; i < NVM_SIZE_OF_JOB_QUEUE; i++) {
        /* Reset block state if job was pending */
        if (NvM_Global.JobQueue[i].JobType != NVM_JOB_TYPE_NONE) {
            NvM_Global.Blocks[NvM_Global.JobQueue[i].BlockId].Status.State = NVM_BLOCK_STATE_IDLE;
            NvM_Global.Blocks[NvM_Global.JobQueue[i].BlockId].Status.LastResult = NVM_REQ_CANCELLED;
        }
        
        NvM_Global.JobQueue[i].JobType = NVM_JOB_TYPE_NONE;
        NvM_Global.JobQueue[i].BlockId = 0u;
        NvM_Global.JobQueue[i].DataPtr = NULL_PTR;
        NvM_Global.JobQueue[i].Next = NULL_PTR;
        NvM_Global.JobQueue[i].InProgress = FALSE;
    }
    
    /* Reset queue pointers */
    NvM_Global.QueueHead = NULL_PTR;
    NvM_Global.QueueTail = NULL_PTR;
    NvM_Global.QueueSize = 0u;
}

/**
 * @brief Check if queue is empty
 */
boolean NvM_Queue_IsEmpty(void)
{
    return (NvM_Global.QueueHead == NULL_PTR) ? TRUE : FALSE;
}

/**
 * @brief Check if queue is full
 */
boolean NvM_Queue_IsFull(void)
{
    return (NvM_Global.QueueSize >= NVM_SIZE_OF_JOB_QUEUE) ? TRUE : FALSE;
}

/**
 * @brief Get current queue size
 */
uint16_t NvM_Queue_GetSize(void)
{
    return NvM_Global.QueueSize;
}

/**
 * @brief Get queue statistics
 */
void NvM_Queue_GetStats(
    uint32_t* JobsSubmitted,
    uint32_t* JobsCompleted,
    uint32_t* QueueOverflows)
{
    if (JobsSubmitted != NULL_PTR) {
        *JobsSubmitted = NvM_QueueStats.JobsSubmitted;
    }
    
    if (JobsCompleted != NULL_PTR) {
        *JobsCompleted = NvM_QueueStats.JobsCompleted;
    }
    
    if (QueueOverflows != NULL_PTR) {
        *QueueOverflows = NvM_QueueStats.QueueOverflows;
    }
}

/*============================================================================*
 * Block Management Functions
 *============================================================================*/

/**
 * @brief Initialize a block
 */
Std_ReturnType NvM_Block_Init(NvM_BlockIdType BlockId)
{
    if (BlockId > NVM_MAX_NUMBER_OF_BLOCKS) {
        return E_NOT_OK;
    }
    
    /* Initialize runtime status */
    NvM_Global.Blocks[BlockId].Status.DataChanged = FALSE;
    NvM_Global.Blocks[BlockId].Status.WriteProtected = FALSE;
    NvM_Global.Blocks[BlockId].Status.DataIndex = 0u;
    NvM_Global.Blocks[BlockId].Status.LastResult = NVM_REQ_OK;
    NvM_Global.Blocks[BlockId].Status.State = NVM_BLOCK_STATE_IDLE;
    NvM_Global.Blocks[BlockId].Status.LastWriteTime = 0u;
    NvM_Global.Blocks[BlockId].Status.WriteRetryCount = 0u;
    
    NvM_Global.Blocks[BlockId].CurrentCrc = 0u;
    NvM_Global.Blocks[BlockId].Invalidated = FALSE;
    
    /* Link to configuration */
    if (BlockId <= NvM_Config.NumOfBlocks) {
        NvM_Global.Blocks[BlockId].Config = &NvM_Config.BlockDescriptorTable[BlockId];
    } else {
        NvM_Global.Blocks[BlockId].Config = NULL_PTR;
    }
    
    return E_OK;
}

/**
 * @brief Set block result
 */
void NvM_Block_SetResult(NvM_BlockIdType BlockId, NvM_RequestResultType Result)
{
    if (BlockId > NVM_MAX_NUMBER_OF_BLOCKS) {
        return;
    }
    
    NvM_Global.Blocks[BlockId].Status.LastResult = Result;
    
    /* Reset state to idle */
    if ((Result != NVM_REQ_PENDING) && 
        (NvM_Global.Blocks[BlockId].Status.State != NVM_BLOCK_STATE_IDLE)) {
        NvM_Global.Blocks[BlockId].Status.State = NVM_BLOCK_STATE_IDLE;
    }
    
    /* Call callback if configured */
    if ((NvM_Global.Blocks[BlockId].Config != NULL_PTR) &&
        (NvM_Global.Blocks[BlockId].Config->NvMBlockCallback != NULL_PTR)) {
        /* Determine service ID based on result context */
        uint8_t serviceId = NVM_SID_MAIN_FUNCTION;
        NvM_Global.Blocks[BlockId].Config->NvMBlockCallback(serviceId, Result);
    }
}

/**
 * @brief Restore block from ROM defaults
 */
Std_ReturnType NvM_Block_Restore(NvM_BlockIdType BlockId, void* DataPtr)
{
    const NvM_BlockDescriptorType* config;
    
    if (BlockId > NVM_MAX_NUMBER_OF_BLOCKS) {
        return E_NOT_OK;
    }
    
    config = NvM_Global.Blocks[BlockId].Config;
    
    if (config == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (config->RomBlockDataAddr == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Copy ROM data to RAM */
    memcpy(DataPtr, config->RomBlockDataAddr, config->NvBlockLength);
    
    return E_OK;
}

/*============================================================================*
 * State Machine Functions
 *============================================================================*/

/**
 * @brief Process state machine transitions
 */
void NvM_StateMachine_Process(void)
{
    switch (NvM_Global.State) {
        case NVM_STATE_UNINIT:
            /* Wait for initialization */
            break;
            
        case NVM_STATE_IDLE:
            /* Check if there are jobs to process */
            if (NvM_Global.QueueHead != NULL_PTR) {
                NvM_StateMachine_EnterBusy();
            }
            break;
            
        case NVM_STATE_BUSY:
            /* Processing job - will transition to PENDING when MemIf is called */
            if (NvM_Global.QueueHead == NULL_PTR) {
                NvM_StateMachine_EnterIdle();
            } else {
                /* Check if MemIf is busy */
                if (MemIf_GetStatus(NVM_PRIMARY_MEMORY_DEVICE) == MEMIF_BUSY) {
                    NvM_StateMachine_EnterPending();
                }
            }
            break;
            
        case NVM_STATE_PENDING:
            /* Waiting for MemIf to complete */
            if (MemIf_GetStatus(NVM_PRIMARY_MEMORY_DEVICE) == MEMIF_IDLE) {
                NvM_StateMachine_EnterBusy();
            }
            break;
            
        default:
            /* Invalid state - go to idle */
            NvM_Global.State = NVM_STATE_IDLE;
            break;
    }
}

/**
 * @brief Enter IDLE state
 */
void NvM_StateMachine_EnterIdle(void)
{
    NvM_Global.State = NVM_STATE_IDLE;
    
    /* Check if ReadAll/WriteAll complete */
    if ((NvM_Global.ReadAllActive == TRUE) && (NvM_Queue_IsEmpty() == TRUE)) {
        NvM_Global.ReadAllActive = FALSE;
    }
    
    if ((NvM_Global.WriteAllActive == TRUE) && (NvM_Queue_IsEmpty() == TRUE)) {
        NvM_Global.WriteAllActive = FALSE;
    }
}

/**
 * @brief Enter BUSY state
 */
void NvM_StateMachine_EnterBusy(void)
{
    NvM_Global.State = NVM_STATE_BUSY;
}

/**
 * @brief Enter PENDING state
 */
void NvM_StateMachine_EnterPending(void)
{
    NvM_Global.State = NVM_STATE_PENDING;
}

/*============================================================================*
 * Utility Functions
 *============================================================================*/

/**
 * @brief Check if block ID is valid
 */
boolean NvM_IsBlockIdValid(NvM_BlockIdType BlockId)
{
    if ((BlockId == 0u) || (BlockId > NVM_MAX_NUMBER_OF_BLOCKS)) {
        return FALSE;
    }
    
    return TRUE;
}

/**
 * @brief Check if block is configured
 */
boolean NvM_IsBlockConfigured(NvM_BlockIdType BlockId)
{
    if (NvM_IsBlockIdValid(BlockId) == FALSE) {
        return FALSE;
    }
    
    return (NvM_Global.Blocks[BlockId].Config != NULL_PTR) ? TRUE : FALSE;
}

/**
 * @brief Get internal block structure
 */
NvM_InternalBlockType* NvM_GetInternalBlock(NvM_BlockIdType BlockId)
{
    if (NvM_IsBlockIdValid(BlockId) == FALSE) {
        return NULL_PTR;
    }
    
    return &NvM_Global.Blocks[BlockId];
}

/**
 * @brief Report error (stub for Det)
 */
void NvM_ReportError(uint8_t ApiId, uint8_t ErrorId)
{
    #if (NVM_DEV_ERROR_DETECT == STD_ON)
    /* Error reporting handled by macro */
    #else
    (void)ApiId;
    (void)ErrorId;
    #endif
}
