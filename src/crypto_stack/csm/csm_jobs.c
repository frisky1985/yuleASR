/**
 * @file csm_jobs.c
 * @brief CSM Job Manager Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * Job queue management and processing logic
 */

#include <string.h>
#include <stdio.h>
#include "csm_jobs.h"

/* ============================================================================
 * Job Pool Operations
 * ============================================================================ */

void csm_job_pool_init(csm_context_t *ctx)
{
    if (ctx == NULL) return;
    
    memset(ctx->jobs, 0, sizeof(ctx->jobs));
    ctx->num_jobs = 0;
}

csm_job_t* csm_job_alloc(csm_context_t *ctx)
{
    if (ctx == NULL || ctx->num_jobs >= CSM_MAX_JOBS) {
        return NULL;
    }
    
    /* Find free slot */
    for (int i = 0; i < CSM_MAX_JOBS; i++) {
        if (ctx->jobs[i].job_id == 0) {
            memset(&ctx->jobs[i], 0, sizeof(csm_job_t));
            return &ctx->jobs[i];
        }
    }
    
    return NULL;
}

void csm_job_free(csm_context_t *ctx, csm_job_t *job)
{
    if (ctx == NULL || job == NULL) return;
    
    /* Clear job data */
    memset(job, 0, sizeof(csm_job_t));
}

csm_job_t* csm_job_find_by_id(csm_context_t *ctx, uint32_t job_id)
{
    if (ctx == NULL || job_id == 0) return NULL;
    
    for (int i = 0; i < CSM_MAX_JOBS; i++) {
        if (ctx->jobs[i].job_id == job_id) {
            return &ctx->jobs[i];
        }
    }
    
    return NULL;
}

/* ============================================================================
 * Queue Operations
 * ============================================================================ */

csm_status_t csm_queue_insert(csm_context_t *ctx, csm_job_t *job,
                              csm_job_priority_t priority)
{
    csm_job_t **queue_head;
    csm_job_t *current;
    
    if (ctx == NULL || job == NULL) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    /* Check queue depth */
    if (ctx->stats.current_queue_depth >= ctx->config.max_jobs) {
        return CSM_ERROR_QUEUE_FULL;
    }
    
    /* Select queue based on priority */
    switch (priority) {
        case CSM_JOB_PRIO_HIGH:
            queue_head = &ctx->high_prio_queue;
            break;
        case CSM_JOB_PRIO_LOW:
            queue_head = &ctx->low_prio_queue;
            break;
        case CSM_JOB_PRIO_NORMAL:
        default:
            queue_head = &ctx->normal_prio_queue;
            break;
    }
    
    /* Insert into priority queue (FIFO within same priority) */
    job->next = NULL;
    job->prev = NULL;
    
    if (*queue_head == NULL) {
        /* Empty queue */
        *queue_head = job;
    } else {
        /* Append to end of queue */
        current = *queue_head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = job;
        job->prev = current;
    }
    
    return CSM_OK;
}

csm_status_t csm_queue_remove(csm_context_t *ctx, csm_job_t *job)
{
    csm_job_t **queue_head = NULL;
    
    if (ctx == NULL || job == NULL) {
        return CSM_ERROR_INVALID_PARAM;
    }
    
    /* Determine which queue */
    if (job->priority == CSM_JOB_PRIO_HIGH) {
        queue_head = &ctx->high_prio_queue;
    } else if (job->priority == CSM_JOB_PRIO_LOW) {
        queue_head = &ctx->low_prio_queue;
    } else {
        queue_head = &ctx->normal_prio_queue;
    }
    
    /* Remove from linked list */
    if (job->prev != NULL) {
        job->prev->next = job->next;
    } else {
        /* Head of queue */
        *queue_head = job->next;
    }
    
    if (job->next != NULL) {
        job->next->prev = job->prev;
    }
    
    job->next = NULL;
    job->prev = NULL;
    
    return CSM_OK;
}

csm_job_t* csm_queue_peek(csm_context_t *ctx)
{
    if (ctx == NULL) return NULL;
    
    /* Check queues in priority order */
    if (ctx->high_prio_queue != NULL) {
        return ctx->high_prio_queue;
    }
    if (ctx->normal_prio_queue != NULL) {
        return ctx->normal_prio_queue;
    }
    if (ctx->low_prio_queue != NULL) {
        return ctx->low_prio_queue;
    }
    
    return NULL;
}

/* ============================================================================
 * Job Execution
 * ============================================================================ */

csm_status_t csm_job_execute(csm_context_t *ctx, csm_job_t *job)
{
    (void)ctx;
    (void)job;
    /* Implementation is in csm_core.c csm_execute_crypto_op() */
    return CSM_OK;
}

void csm_job_complete(csm_context_t *ctx, csm_job_t *job, csm_status_t result)
{
    if (ctx == NULL || job == NULL) return;
    
    job->result = result;
    
    if (result == CSM_OK) {
        job->state = CSM_JOB_STATE_COMPLETED;
        ctx->stats.total_jobs_completed++;
    } else {
        job->state = CSM_JOB_STATE_FAILED;
        ctx->stats.total_jobs_failed++;
    }
    
    /* Trigger callbacks */
    csm_job_trigger_callbacks(ctx, job);
}

void csm_job_trigger_callbacks(csm_context_t *ctx, csm_job_t *job)
{
    if (ctx == NULL || job == NULL) return;
    
    /* Trigger registered callbacks */
    for (int i = 0; i < CSM_MAX_CALLBACKS; i++) {
        if (ctx->callbacks[i].active && ctx->callbacks[i].callback != NULL) {
            ctx->callbacks[i].callback(job->job_id, job->result, 
                                       ctx->callbacks[i].user_data);
        }
    }
}
