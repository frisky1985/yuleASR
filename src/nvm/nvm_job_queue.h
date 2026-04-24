/**
 * @file nvm_job_queue.h
 * @brief NvM Job Queue Header
 * @version 1.0.0
 * @date 2025
 * 
 * Job queue management including:
 * - Asynchronous operation support
 * - Priority queue
 * - Batch operations
 * - Rollback support
 */

#ifndef NVM_JOB_QUEUE_H
#define NVM_JOB_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nvm_types.h"

/*============================================================================*
 * Job Structure
 *============================================================================*/
typedef struct Nvm_Job_s {
    uint32_t jobId;                     /* Unique job ID */
    Nvm_JobType_t type;                 /* Job type */
    Nvm_Priority_t priority;            /* Job priority */
    uint16_t blockId;                   /* Target block ID */
    uint8_t* dataBuffer;                /* Data buffer (for write) */
    uint32_t dataLength;                /* Data length */
    uint32_t timestamp;                 /* Job creation timestamp */
    Nvm_JobCallback_t callback;         /* Completion callback */
    void* userData;                     /* User data for callback */
    struct Nvm_Job_s* next;             /* Next job in queue */
    bool inProgress;                    /* Job is being processed */
    bool completed;                     /* Job completed */
    Nvm_ErrorCode_t result;             /* Job result */
} Nvm_Job_t;

/*============================================================================*
 * Job Queue Context
 *============================================================================*/
typedef struct {
    bool initialized;
    Nvm_Job_t jobs[NVM_MAX_JOB_QUEUE];  /* Job pool */
    Nvm_Job_t* queueHead;               /* Queue head (highest priority) */
    Nvm_Job_t* queueTail;               /* Queue tail */
    Nvm_Job_t* freeList;                /* Free job list */
    uint32_t queuedJobs;                /* Number of queued jobs */
    uint32_t activeJobs;                /* Number of active jobs */
    uint32_t completedJobs;             /* Total completed jobs */
    uint32_t failedJobs;                /* Total failed jobs */
    uint32_t nextJobId;                 /* Next job ID */
    bool processing;                    /* Queue is being processed */
    bool paused;                        /* Queue processing paused */
} Nvm_JobQueue_Context_t;

/*============================================================================*
 * Batch Operation Structure
 *============================================================================*/
#define NVM_BATCH_MAX_JOBS  16u

typedef struct {
    uint32_t batchId;
    Nvm_Job_t* jobs[NVM_BATCH_MAX_JOBS];
    uint32_t jobCount;
    uint32_t completedCount;
    bool atomic;                        /* All or nothing */
    Nvm_JobCallback_t batchCallback;
    void* batchUserData;
    bool inProgress;
    Nvm_ErrorCode_t result;
} Nvm_BatchOp_t;

/*============================================================================*
 * Job Queue API
 *============================================================================*/

/**
 * @brief Initialize job queue
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_Init(void);

/**
 * @brief Deinitialize job queue
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_Deinit(void);

/**
 * @brief Create a new job
 * @param type Job type
 * @param priority Job priority
 * @param blockId Target block ID
 * @return Pointer to job, NULL on error
 */
Nvm_Job_t* Nvm_JobQueue_CreateJob(
    Nvm_JobType_t type,
    Nvm_Priority_t priority,
    uint16_t blockId
);

/**
 * @brief Free a job
 * @param job Job to free
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_FreeJob(Nvm_Job_t* job);

/**
 * @brief Submit job to queue
 * @param job Job to submit
 * @return NVM_OK on success, NVM_E_QUEUE_FULL if queue full
 */
Nvm_ErrorCode_t Nvm_JobQueue_Submit(Nvm_Job_t* job);

/**
 * @brief Submit job with callback
 * @param job Job to submit
 * @param callback Completion callback
 * @param userData User data for callback
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_SubmitWithCallback(
    Nvm_Job_t* job,
    Nvm_JobCallback_t callback,
    void* userData
);

/**
 * @brief Cancel a pending job
 * @param jobId Job ID to cancel
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_Cancel(uint32_t jobId);

/**
 * @brief Get next job from queue (highest priority)
 * @return Pointer to job, NULL if queue empty
 */
Nvm_Job_t* Nvm_JobQueue_GetNext(void);

/**
 * @brief Mark job as complete
 * @param job Job to complete
 * @param result Job result
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_Complete(
    Nvm_Job_t* job,
    Nvm_ErrorCode_t result
);

/**
 * @brief Process single job from queue
 * @return NVM_OK on success, NVM_E_QUEUE_EMPTY if empty
 */
Nvm_ErrorCode_t Nvm_JobQueue_ProcessOne(void);

/**
 * @brief Process all pending jobs
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_ProcessAll(void);

/**
 * @brief Pause queue processing
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_Pause(void);

/**
 * @brief Resume queue processing
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_Resume(void);

/**
 * @brief Check if queue is empty
 * @return true if empty, false otherwise
 */
bool Nvm_JobQueue_IsEmpty(void);

/**
 * @brief Get number of pending jobs
 * @return Number of queued jobs
 */
uint32_t Nvm_JobQueue_GetPendingCount(void);

/**
 * @brief Clear all pending jobs
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_Clear(void);

/**
 * @brief Rollback completed jobs (if supported)
 * @param count Number of jobs to rollback
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_Rollback(uint32_t count);

/**
 * @brief Create batch operation
 * @param atomic If true, all jobs must succeed
 * @return Pointer to batch, NULL on error
 */
Nvm_BatchOp_t* Nvm_JobQueue_CreateBatch(bool atomic);

/**
 * @brief Add job to batch
 * @param batch Batch operation
 * @param job Job to add
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_AddToBatch(
    Nvm_BatchOp_t* batch,
    Nvm_Job_t* job
);

/**
 * @brief Submit batch operation
 * @param batch Batch to submit
 * @param callback Batch completion callback
 * @param userData User data for callback
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_SubmitBatch(
    Nvm_BatchOp_t* batch,
    Nvm_JobCallback_t callback,
    void* userData
);

/**
 * @brief Free batch operation
 * @param batch Batch to free
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_FreeBatch(Nvm_BatchOp_t* batch);

/**
 * @brief Get job queue statistics
 * @param queued Pointer to store queued count
 * @param completed Pointer to store completed count
 * @param failed Pointer to store failed count
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_JobQueue_GetStats(
    uint32_t* queued,
    uint32_t* completed,
    uint32_t* failed
);

/**
 * @brief Get job queue context (internal use)
 * @return Pointer to context
 */
Nvm_JobQueue_Context_t* Nvm_JobQueue_GetContext(void);

#ifdef __cplusplus
}
#endif

#endif /* NVM_JOB_QUEUE_H */
