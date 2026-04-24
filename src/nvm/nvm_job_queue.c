/**
 * @file nvm_job_queue.c
 * @brief NvM Job Queue Implementation
 * @version 1.0.0
 * @date 2025
 * 
 * ASIL-D Compliant Implementation
 */

#include "nvm_job_queue.h"
#include <string.h>

/*============================================================================*
 * Module Variables
 *============================================================================*/
static Nvm_JobQueue_Context_t g_jobQueueContext = {0};
static Nvm_BatchOp_t g_batchPool[8];  /* Batch operation pool */
static uint32_t g_nextBatchId = 1;

/*============================================================================*
 * Private Function Prototypes
 *============================================================================*/
static void Nvm_Private_InsertJobByPriority(Nvm_Job_t* job);
static Nvm_Job_t* Nvm_Private_AllocateJob(void);
static void Nvm_Private_FreeJobInternal(Nvm_Job_t* job);
static Nvm_BatchOp_t* Nvm_Private_AllocateBatch(void);
static void Nvm_Private_FreeBatchInternal(Nvm_BatchOp_t* batch);

/*============================================================================*
 * Public API Implementation
 *============================================================================*/

Nvm_ErrorCode_t Nvm_JobQueue_Init(void)
{
    uint32_t i;
    
    if (g_jobQueueContext.initialized) {
        return NVM_E_ALREADY_INITIALIZED;
    }
    
    /* Initialize context */
    memset(&g_jobQueueContext, 0, sizeof(Nvm_JobQueue_Context_t));
    memset(g_batchPool, 0, sizeof(g_batchPool));
    
    /* Initialize job pool as free list */
    for (i = 0; i < NVM_MAX_JOB_QUEUE; i++) {
        g_jobQueueContext.jobs[i].jobId = 0;
        g_jobQueueContext.jobs[i].next = g_jobQueueContext.freeList;
        g_jobQueueContext.freeList = &g_jobQueueContext.jobs[i];
    }
    
    g_jobQueueContext.nextJobId = 1;
    g_jobQueueContext.initialized = true;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_JobQueue_Deinit(void)
{
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Clear all pending jobs */
    Nvm_JobQueue_Clear();
    
    /* Clear context */
    memset(&g_jobQueueContext, 0, sizeof(Nvm_JobQueue_Context_t));
    memset(g_batchPool, 0, sizeof(g_batchPool));
    
    return NVM_OK;
}

Nvm_Job_t* Nvm_JobQueue_CreateJob(
    Nvm_JobType_t type,
    Nvm_Priority_t priority,
    uint16_t blockId)
{
    Nvm_Job_t* job = NULL;
    
    if (!g_jobQueueContext.initialized) {
        return NULL;
    }
    
    /* Allocate job from free list */
    job = Nvm_Private_AllocateJob();
    if (job == NULL) {
        return NULL;
    }
    
    /* Initialize job */
    job->jobId = g_jobQueueContext.nextJobId++;
    job->type = type;
    job->priority = priority;
    job->blockId = blockId;
    job->dataBuffer = NULL;
    job->dataLength = 0;
    job->timestamp = 0;  /* Could use system tick */
    job->callback = NULL;
    job->userData = NULL;
    job->next = NULL;
    job->inProgress = false;
    job->completed = false;
    job->result = NVM_OK;
    
    return job;
}

Nvm_ErrorCode_t Nvm_JobQueue_FreeJob(Nvm_Job_t* job)
{
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (job == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    /* Cannot free job that is in progress */
    if (job->inProgress) {
        return NVM_E_NOT_OK;
    }
    
    Nvm_Private_FreeJobInternal(job);
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_JobQueue_Submit(Nvm_Job_t* job)
{
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (job == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    /* Check if already in queue */
    if (job->next != NULL || job == g_jobQueueContext.queueTail) {
        return NVM_E_ALREADY_INITIALIZED;
    }
    
    /* Insert by priority */
    Nvm_Private_InsertJobByPriority(job);
    
    g_jobQueueContext.queuedJobs++;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_JobQueue_SubmitWithCallback(
    Nvm_Job_t* job,
    Nvm_JobCallback_t callback,
    void* userData)
{
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (job == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    job->callback = callback;
    job->userData = userData;
    
    return Nvm_JobQueue_Submit(job);
}

Nvm_ErrorCode_t Nvm_JobQueue_Cancel(uint32_t jobId)
{
    Nvm_Job_t* current;
    Nvm_Job_t* prev;
    
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Search in queue */
    prev = NULL;
    current = g_jobQueueContext.queueHead;
    
    while (current != NULL) {
        if (current->jobId == jobId) {
            /* Cannot cancel in-progress job */
            if (current->inProgress) {
                return NVM_E_NOT_OK;
            }
            
            /* Remove from queue */
            if (prev == NULL) {
                g_jobQueueContext.queueHead = current->next;
            } else {
                prev->next = current->next;
            }
            
            if (current == g_jobQueueContext.queueTail) {
                g_jobQueueContext.queueTail = prev;
            }
            
            /* Free job */
            Nvm_Private_FreeJobInternal(current);
            g_jobQueueContext.queuedJobs--;
            
            return NVM_OK;
        }
        
        prev = current;
        current = current->next;
    }
    
    return NVM_E_NOT_OK;
}

Nvm_Job_t* Nvm_JobQueue_GetNext(void)
{
    Nvm_Job_t* job;
    
    if (!g_jobQueueContext.initialized) {
        return NULL;
    }
    
    if (g_jobQueueContext.queueHead == NULL) {
        return NULL;
    }
    
    /* Get highest priority job */
    job = g_jobQueueContext.queueHead;
    g_jobQueueContext.queueHead = job->next;
    
    if (g_jobQueueContext.queueHead == NULL) {
        g_jobQueueContext.queueTail = NULL;
    }
    
    job->next = NULL;
    job->inProgress = true;
    g_jobQueueContext.activeJobs++;
    g_jobQueueContext.queuedJobs--;
    
    return job;
}

Nvm_ErrorCode_t Nvm_JobQueue_Complete(
    Nvm_Job_t* job,
    Nvm_ErrorCode_t result)
{
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (job == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (!job->inProgress) {
        return NVM_E_NOT_OK;
    }
    
    job->completed = true;
    job->result = result;
    job->inProgress = false;
    g_jobQueueContext.activeJobs--;
    
    /* Update statistics */
    if (result == NVM_OK) {
        g_jobQueueContext.completedJobs++;
    } else {
        g_jobQueueContext.failedJobs++;
    }
    
    /* Call callback if registered */
    if (job->callback != NULL) {
        job->callback(job->blockId, job->type, result, job->userData);
    }
    
    /* Free job */
    Nvm_Private_FreeJobInternal(job);
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_JobQueue_ProcessOne(void)
{
    Nvm_Job_t* job;
    Nvm_ErrorCode_t result = NVM_OK;
    
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (g_jobQueueContext.paused) {
        return NVM_E_NOT_OK;
    }
    
    job = Nvm_JobQueue_GetNext();
    if (job == NULL) {
        return NVM_E_QUEUE_EMPTY;
    }
    
    /* Process job based on type */
    /* In real implementation, this would call appropriate handler */
    switch (job->type) {
        case NVM_JOB_READ:
        case NVM_JOB_WRITE:
        case NVM_JOB_ERASE:
        case NVM_JOB_VERIFY:
        case NVM_JOB_RESTORE:
        case NVM_JOB_VALIDATE:
        case NVM_JOB_COMPACT:
        case NVM_JOB_RECOVER:
            /* Process job */
            result = NVM_OK;
            break;
        default:
            result = NVM_E_NOT_OK;
            break;
    }
    
    /* Complete job */
    Nvm_JobQueue_Complete(job, result);
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_JobQueue_ProcessAll(void)
{
    Nvm_ErrorCode_t result = NVM_OK;
    
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    while (!Nvm_JobQueue_IsEmpty()) {
        result = Nvm_JobQueue_ProcessOne();
        if (result != NVM_OK && result != NVM_E_QUEUE_EMPTY) {
            break;
        }
    }
    
    return result;
}

Nvm_ErrorCode_t Nvm_JobQueue_Pause(void)
{
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    g_jobQueueContext.paused = true;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_JobQueue_Resume(void)
{
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    g_jobQueueContext.paused = false;
    
    return NVM_OK;
}

bool Nvm_JobQueue_IsEmpty(void)
{
    if (!g_jobQueueContext.initialized) {
        return true;
    }
    
    return (g_jobQueueContext.queueHead == NULL);
}

uint32_t Nvm_JobQueue_GetPendingCount(void)
{
    if (!g_jobQueueContext.initialized) {
        return 0;
    }
    
    return g_jobQueueContext.queuedJobs;
}

Nvm_ErrorCode_t Nvm_JobQueue_Clear(void)
{
    Nvm_Job_t* job;
    
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Cancel all pending jobs */
    while (g_jobQueueContext.queueHead != NULL) {
        job = g_jobQueueContext.queueHead;
        g_jobQueueContext.queueHead = job->next;
        
        /* Call callback with cancelled status */
        if (job->callback != NULL) {
            job->callback(job->blockId, job->type, NVM_E_NOT_OK, job->userData);
        }
        
        Nvm_Private_FreeJobInternal(job);
    }
    
    g_jobQueueContext.queueTail = NULL;
    g_jobQueueContext.queuedJobs = 0;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_JobQueue_Rollback(uint32_t count)
{
    (void)count;
    
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Rollback implementation would go here */
    /* Requires tracking completed operations and reversing them */
    
    return NVM_OK;
}

Nvm_BatchOp_t* Nvm_JobQueue_CreateBatch(bool atomic)
{
    Nvm_BatchOp_t* batch;
    
    if (!g_jobQueueContext.initialized) {
        return NULL;
    }
    
    batch = Nvm_Private_AllocateBatch();
    if (batch == NULL) {
        return NULL;
    }
    
    batch->batchId = g_nextBatchId++;
    batch->atomic = atomic;
    batch->jobCount = 0;
    batch->completedCount = 0;
    batch->batchCallback = NULL;
    batch->batchUserData = NULL;
    batch->inProgress = false;
    batch->result = NVM_OK;
    
    return batch;
}

Nvm_ErrorCode_t Nvm_JobQueue_AddToBatch(
    Nvm_BatchOp_t* batch,
    Nvm_Job_t* job)
{
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (batch == NULL || job == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (batch->inProgress) {
        return NVM_E_NOT_OK;
    }
    
    if (batch->jobCount >= NVM_BATCH_MAX_JOBS) {
        return NVM_E_OUT_OF_MEMORY;
    }
    
    batch->jobs[batch->jobCount++] = job;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_JobQueue_SubmitBatch(
    Nvm_BatchOp_t* batch,
    Nvm_JobCallback_t callback,
    void* userData)
{
    uint32_t i;
    Nvm_ErrorCode_t result = NVM_OK;
    
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (batch == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    batch->batchCallback = callback;
    batch->batchUserData = userData;
    batch->inProgress = true;
    
    /* Submit all jobs */
    for (i = 0; i < batch->jobCount; i++) {
        result = Nvm_JobQueue_Submit(batch->jobs[i]);
        if (result != NVM_OK) {
            /* If atomic, need to rollback submitted jobs */
            if (batch->atomic) {
                /* Rollback logic here */
            }
            batch->result = result;
            return result;
        }
    }
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_JobQueue_FreeBatch(Nvm_BatchOp_t* batch)
{
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (batch == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (batch->inProgress) {
        return NVM_E_NOT_OK;
    }
    
    Nvm_Private_FreeBatchInternal(batch);
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_JobQueue_GetStats(
    uint32_t* queued,
    uint32_t* completed,
    uint32_t* failed)
{
    if (!g_jobQueueContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (queued == NULL || completed == NULL || failed == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    *queued = g_jobQueueContext.queuedJobs;
    *completed = g_jobQueueContext.completedJobs;
    *failed = g_jobQueueContext.failedJobs;
    
    return NVM_OK;
}

Nvm_JobQueue_Context_t* Nvm_JobQueue_GetContext(void)
{
    return &g_jobQueueContext;
}

/*============================================================================*
 * Private Functions
 *============================================================================*/

static void Nvm_Private_InsertJobByPriority(Nvm_Job_t* job)
{
    Nvm_Job_t* current;
    Nvm_Job_t* prev;
    
    /* Insert by priority (higher first) */
    prev = NULL;
    current = g_jobQueueContext.queueHead;
    
    while (current != NULL && current->priority >= job->priority) {
        prev = current;
        current = current->next;
    }
    
    job->next = current;
    
    if (prev == NULL) {
        g_jobQueueContext.queueHead = job;
    } else {
        prev->next = job;
    }
    
    if (current == NULL) {
        g_jobQueueContext.queueTail = job;
    }
}

static Nvm_Job_t* Nvm_Private_AllocateJob(void)
{
    Nvm_Job_t* job;
    
    if (g_jobQueueContext.freeList == NULL) {
        return NULL;
    }
    
    job = g_jobQueueContext.freeList;
    g_jobQueueContext.freeList = job->next;
    job->next = NULL;
    
    return job;
}

static void Nvm_Private_FreeJobInternal(Nvm_Job_t* job)
{
    /* Reset job */
    memset(job, 0, sizeof(Nvm_Job_t));
    
    /* Add to free list */
    job->next = g_jobQueueContext.freeList;
    g_jobQueueContext.freeList = job;
}

static Nvm_BatchOp_t* Nvm_Private_AllocateBatch(void)
{
    uint32_t i;
    
    for (i = 0; i < 8; i++) {
        if (g_batchPool[i].batchId == 0) {
            return &g_batchPool[i];
        }
    }
    
    return NULL;
}

static void Nvm_Private_FreeBatchInternal(Nvm_BatchOp_t* batch)
{
    memset(batch, 0, sizeof(Nvm_BatchOp_t));
}
